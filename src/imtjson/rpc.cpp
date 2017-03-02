/*
 * rpcserver.cpp
 *
 *  Created on: 28. 2. 2017
 *      Author: ondra
 */

#include  <condition_variable>
#include <mutex>
#include "rpc.h"

#include "array.h"
#include "object.h"
#include "operations.h"
#include "validator.h"
namespace json {


RpcRequest::RequestData::RequestData(const Value& request) {
	methodName = String(request["method"]);
	args = request["params"];
	id = request["id"];
	context = request["context"];
	responseSent = false;
}

const String& RpcRequest::getMethodName() const {
	return data->methodName;
}

const Value& RpcRequest::getArgs() const {
	return data->args;
}

const Value& RpcRequest::getId() const {
	return data->id;
}

const Value& RpcRequest::getContext() const {
	return data->context;
}


void RpcRequest::setResult(const Value& result) {
	setResult(result, undefined);

}

void RpcRequest::setResult(const Value& result, const Value& context) {
	Value resp (object,{
			key/"id"=data->id,
			key/"result"=result,
			key/"error"=nullptr,
			key/"context"=context.empty()?Value(undefined):context});
	data->setResponse(resp);

}

void RpcRequest::setError(const Value& error) {
	Value resp (object,{
			key/"id"=data->id,
			key/"result"=nullptr,
			key/"error"=error});
	data->setResponse(resp);
}


RpcRequest::RequestData::~RequestData() {
	if (!responseSent) setResponse(undefined);

}

Value RpcRequest::operator [](unsigned int index) const {
	return data->args[index];
}

Value RpcRequest::operator [](const StrViewA name) const {
	return data->context[name];
}

class RpcArgValidator: public Validator {
public:
	RpcArgValidator():Validator(object) {}
	RpcArgValidator(const Value &customClasses):Validator(customClasses) {}

	bool checkArgs(const Value &subject, const Value &pattern) {
		return evalRuleArray(subject, pattern, pattern.size());
	}
	bool checkArgs(const Value &subject, const Value &pattern, const Value &alt) {
		Value::TwoValues s = subject.splitAt(pattern.size());
		if (checkArgs(s.first, pattern)) {
			return evalRuleAlternatives(s.second, alt, 0);
		}
		else return false;
	}
};

bool RpcRequest::checkArgs(const Value& argDefTuple) {
	RpcArgValidator val;
	if (val.checkArgs(data->args, argDefTuple)) return true;
	data->rejections = val.getRejections();
}

bool RpcRequest::checkArgs(const Value& argDefTuple, const Value& optionalArgs) {
	RpcArgValidator val;
	if (val.checkArgs(data->args, argDefTuple, optionalArgs)) return true;
	data->rejections = val.getRejections();
}

bool RpcRequest::checkArgs(const Value& argDefTuple, const Value& optionalArgs,
		const Value& customClasses) {
	RpcArgValidator val(customClasses);
	if (val.checkArgs(data->args, argDefTuple, optionalArgs)) return true;
	data->rejections = val.getRejections();
}

Value RpcRequest::getRejections() const {
	return data->rejections;
}

void RpcRequest::setArgError(Value rejections) {
	setError(Value(object,{
			key/"code"=1,
			key/"message"="Invalid parameters",
			key/"rejections" = rejections
	}));
}

void RpcRequest::setError(unsigned int status, const String& message) {
	setError(Value(object,{
			key/"code" =status,
			key/"message"=message
	}));
}

void RpcRequest::RequestData::setResponse(const Value& v) {
	if (responseSent) return;
	responseSent = true;
	response(v);
}

void RpcServer::operator ()(const RpcRequest& req) const {
	Value name = req.getMethodName();
	Value args = req.getArgs();
	Value context = req.getContext();
	if (name.defined() && args.defined() && name.type() == string && args.type() == array
			&& (!context.defined() || context.type() == object)) {
		AbstractMethodReg *m = find(req.getMethodName());
		if (m == nullptr) {
			onMethodNotFound(req);
		} else {
			m->call(req);
		}
	} else {
		onMalformedRequest(req);
	}
}

void RpcServer::remove(const StrViewA& name) {
	mapReg.erase(name);
}

RpcServer::AbstractMethodReg* RpcServer::find(const StrViewA& name) const {
	auto f = mapReg.find(name);
	if (f == mapReg.end()) return nullptr;
	return &(*(f->second));
}

void RpcServer::onMethodNotFound(const RpcRequest& req) const {
	RpcRequest creq(req);
	creq.setError(2,{"Method ot found: ", req.getMethodName() } );
}

void RpcServer::onMalformedRequest(const RpcRequest& req) const {
	RpcRequest creq(req);
	creq.setError(3,{"Malformed request"} );

}

void RpcServer::add_listMethods(const String& name) {
	add(name, [this](RpcRequest req){

		Array res;
		for(auto &x : mapReg) {
			res.push_back(x.second->name);
		}
		req.setResult(res);
	});
}

void RpcServer::add_multicall(const String& name) {
}

void RpcServer::add_help(const Value& helpContent, const String &name) {

	Value helpCtx = helpContent;
	add(name, [helpCtx](RpcRequest req) {
		String n (req[0]);
		req.setResult(helpCtx[n]);
	});

}

RpcResult::RpcResult(Value result, bool isError, Value context)
	:Value(result),error(isError),context(context)
{}

AbstractRpcClient::PreparedCall::PreparedCall(AbstractRpcClient& owner, unsigned int id, const Value& msg)
	:owner(owner),id(id),msg(msg),executed(false)
{
}

//object receives result and stores it
class AbstractRpcClient::LocalPendingCall: public AbstractRpcClient::PendingCall {
public:
	virtual void onResponse(RpcResult response) override {
		res = response;
	}

	LocalPendingCall()
		:received(false),trig(nullptr) {}

	void release() {
		//when the storage is destroyed - set received to true
		received = true;
		//if the pointer is valid
		if (trig != nullptr)
			//notify conditional variable
			trig->notify_one();
	}

	RpcResult res;
	bool received;
	std::condition_variable * trig;
};

void  AbstractRpcClient::onWait(LocalPendingCall &lpc) throw() {
	//setup conditional variable
	std::condition_variable t;
	//set pointer - from now other thread must notify the conditional variable (if arrived soon, received is already true)
	lpc.trig = &t;

	//create mutex
	std::mutex lock;
	//lock the mutext
	std::unique_lock<std::mutex> guard(lock);

	//wait for receiving
	t.wait(guard, [&] {return lpc.received;});
	//received here
}



AbstractRpcClient::PreparedCall::operator RpcResult() {

	if (executed) return RpcResult();
	executed = true;

	//declare storage here
	LocalPendingCall lpc;
	//register the storage (under its id)
	owner.addPendingCall(id,PPendingCall(&lpc, [](PendingCall *d){
		static_cast<LocalPendingCall *>(d)->release();}));
	//send request now
	owner.sendRequest(msg);
	//in case that result has not been received here (will arrive later)
	if (!lpc.received) {
		owner.onWait(lpc);
	}

	//return received result
	return lpc.res;
}

AbstractRpcClient::PreparedCall AbstractRpcClient::operator ()(String methodName, Value args) {
	unsigned int id = ++idCounter;
	return PreparedCall(*this,id,Value(object,{
			key/"method"=methodName,
			key/"params"=args,
			key/"id"=id,
			key/"context"=context.empty()?Value(undefined):context}));
}

bool AbstractRpcClient::cancelAsyncCall(unsigned int id, RpcResult result) {
	auto it = callMap.find(id);
	if (it == callMap.end()) return false;
	PPendingCall pc = std::move(it->second);
	callMap.erase(it);
	pc->onResponse(result);
	return true;
}

AbstractRpcClient::ReceiveStatus AbstractRpcClient::processResponse(Value response) {
	Value id = response["id"];
	if (id.defined()) {
		Value result = response["result"];
		Value error = response["error"];
		if (id.type() == number && result.defined() && response.defined()) {
			unsigned int nid = id.getUInt();
			Value ctx = response["context"];
			updateContext(ctx);
			if (error.isNull())  {
				if (!cancelAsyncCall(nid,RpcResult(result,false,ctx))) return unexpected;
			} else {
				if (!cancelAsyncCall(nid,RpcResult(error,true,undefined))) return unexpected;
			}

		} else {
			return request;
		}
	} else {
		return notification;
	}
}

Value AbstractRpcClient::getContext() const {
	return context;
}

void AbstractRpcClient::setContext(const Value& value) {
	context = value;
}

void AbstractRpcClient::updateContext(const Value& value) {
	Object newContext;
	context.merge(value,[&](Value l, Value r) {
		int cmp;
		if (!l.defined()) cmp = 1;
		else if (!r.defined()) cmp = -1;
		else cmp = l.getKey().compare(r.getKey());

		if (cmp < 0) newContext.set(l);
		else if (!r.isNull()) newContext.set(r);
		return cmp;
	});
	setContext(newContext);
}

void AbstractRpcClient::addPendingCall(unsigned int id, PPendingCall &&pcall) {
	callMap.insert(CallItemType(id,std::move(pcall)));
}


}
