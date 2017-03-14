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

void RpcServer::operator ()(const RpcRequest& req) const throw() {
	try {
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
	} catch (...) {
		onException(req);
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
	creq.setError(3,"Malformed request" );

}

void RpcServer::onInvalidParams(const RpcRequest &req) const {
	RpcRequest creq(req);
	creq.setError(4,"Invalid params" );

}
void RpcServer::onException(const RpcRequest &req) const {
	RpcRequest creq(req);
	try {
		throw;
	} catch (std::exception &e) {
		creq.setError(5, e.what());
	} catch (...) {
		creq.setError(5, "Uncaught exception");
	}

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

class MulticallContext {
public:
	typedef std::function<std::pair<Value,Value>()> NextFn;
	Array results;
	Array errors;
	Value context;
	Value contextchange;

	std::atomic<int> mtfork;

	RpcRequest req;
	NextFn nextfn;
	RpcServer &server;

	MulticallContext(RpcServer &server, RpcRequest req, const NextFn &nextFn):server(server),req(req),nextfn(nextFn) {
		context = req.getContext();

	}

	void run() {
		do {
			auto p = nextfn();
			mtfork = 0;
			if (p.first.defined()) {
				RpcRequest req = RpcRequest::create(
						Value(object,{key/"method"=p.first, key/"params" = p.second, key/"context" = context, key/"id" = 1}),
						[&](Value res) {
							if (res.defined()) {
								if (res["error"].defined() ) {
									results.push_back(nullptr);
									errors.push_back(res["error"]);
								} else {
									results.push_back(res["result"]);
									Value ctx = res["context"];
									if (ctx.defined()) {
										context = AbstractRpcClient::updateContext(context,ctx);
										contextchange = AbstractRpcClient::updateContext(contextchange,ctx);
									}
								}
							} else {
								results.push_back(nullptr);
								errors.push_back(nullptr);
							}
							if (++mtfork == 2) {
								this->run();
							}
						}
				);
				server(req);
			} else {
				req.setResult(Value(object,{key/"results"=results, key/"errors"= errors}));
				++mtfork;
			}
		} while (++mtfork == 2);

	}


protected:

};


void RpcServer::add_multicall(const String& name) {
	add(name, [this](RpcRequest req) {

		if (req.getArgs().empty()) onInvalidParams(req);
		else if (req[0].type() == json::string) {
			int pos = 1;
			MulticallContext *m = new MulticallContext(*this,req, [&] {
				if (pos < req.getArgs().size())
					return std::make_pair(req[0],req[pos++]);
				else
					return std::make_pair(Value(),Value());
			});
			m->run();
		} else if (req[0].type() == json::array) {
			int pos = 0;
			MulticallContext *m = new MulticallContext(*this,req,[&] {
				int x = pos++;
				if (x < req.getArgs().size())
					return std::make_pair(req[x][0],req[x]);
				else
					return std::make_pair(Value(),Value());
			});
			m->run();
		} else {
			onInvalidParams(req);
		}
	});
}


void RpcServer::runMulticall(RpcRequest req, std::function<std::pair<Value,Value>()> nextItem) const {


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
	setContext(updateContext(context,value));
}
Value AbstractRpcClient::updateContext(const Value& context, const Value& value) {
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
	return newContext;
}

void AbstractRpcClient::addPendingCall(unsigned int id, PPendingCall &&pcall) {
	callMap.insert(CallItemType(id,std::move(pcall)));
}


}
