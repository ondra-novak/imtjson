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


RpcRequest::RequestData::RequestData(const Value& request, RpcFlags::Type flags)
	:flags(flags)
{
	methodName = String(request["method"]);
	args = request["params"];
	id = request["id"];
	context = request["context"];
	Value jsonrpcver = request["jsonrpc"];
	if (jsonrpcver.defined()) {
		if (jsonrpcver.getString() == "2.0") {
			ver = RpcVersion::ver2;
		} else if (jsonrpcver.getString() == "1.0") {
			ver = RpcVersion::ver1;
		} else {
			throw std::runtime_error("Unknown JSONRPC version");
		}
	} else {
		ver = RpcVersion::ver1;
	}
	if (args.type() != json::array && (flags & RpcFlags::namedParams) == 0) args = Value(json::array, {args});
	if (flags & RpcFlags::preResponseNotify) notifyEnabled = true;
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

StrViewA RpcRequest::getVer() const {
	switch (data->ver) {
	case RpcVersion::ver1: return StrViewA("1.0");
	case RpcVersion::ver2: return StrViewA("2.0");
	}
}


void RpcRequest::setResult(const Value& result) {
	setResult(result, undefined);

}

void RpcRequest::setResult(const Value& result, const Value& context) {

	Value resp;
	Value ctxch = context.empty()?Value():context;
	switch (data->ver) {
	case RpcVersion::ver1:
		resp = Object("id",data->id)
				("result",result)
				("error",nullptr)
				("context", ctxch);
		break;
	case RpcVersion::ver2:
		resp = Object("id",data->id)
				("result",result)
				("jsonrpc","2.0")
				("context", ctxch);
		break;
	}

	data->setResponse(resp);


}

void RpcRequest::setError(const Value& error) {
	Value resp;
	switch (data->ver) {
	case RpcVersion::ver1:
		resp = Object("id",data->id)
				("result",nullptr)
				("error",error);;
		break;
	case RpcVersion::ver2:
		resp = Object("id",data->id)
				("error",error)
				("jsonrpc","2.0");
		break;
	}

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
		curPath = &Path::root;
		return evalRuleArray(subject, pattern, pattern.size(),0);
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
	return false;
}

bool RpcRequest::checkArgs(const Value& argDefTuple, const Value& optionalArgs) {
	RpcArgValidator val;
	if (val.checkArgs(data->args, argDefTuple, optionalArgs)) return true;
	data->rejections = val.getRejections();
	return false;
}

bool RpcRequest::checkArgs(const Value& argDefTuple, const Value& optionalArgs,
		const Value& customClasses) {
	RpcArgValidator val(customClasses);
	if (val.checkArgs(data->args, argDefTuple, optionalArgs)) return true;
	data->rejections = val.getRejections();
	return false;
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

void RpcRequest::setArgError() {
	setArgError(getRejections());
}

void RpcRequest::setError(int code, String message, Value d) {
	if (data->formatter == nullptr) {
		setError(Value(object,{
				key/"code" =code,
				key/"message"=message,
				key/"data"=d
		}));
	} else {
		setError(data->formatter->formatError(code,message,d));
	}
}

void RpcRequest::RequestData::setResponse(const Value& v) {
	if (responseSent) return;
	responseSent = true;
	notifyEnabled =  (flags & RpcFlags::postResponseNotify) != 0;
	if (!id.isNull()) response(v);
}

void RpcServer::operator ()(RpcRequest req) const throw() {
	try {
		req.setErrorFormatter(this);
		Value name = req.getMethodName();
		Value args = req.getArgs();
		Value context = req.getContext();
		if (name.defined() && name.type() == string && (args.type() == array || args.type() == object || args.type() == undefined)
				&& (!context.defined() || context.type() == object)) {
			AbstractMethodReg *m = find(req.getMethodName());
			if (m == nullptr) {
				req.setError(errorMethodNotFound,"Method not found",name);
			} else {
				m->call(req);
			}
		} else {
			req.setError(errorInvalidRequest,"Invalid request");
		}
	} catch (std::exception &e) {
		req.setInternalError(e.what());
	} catch (...) {
		req.setInternalError(nullptr);
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
				RpcRequest req = RpcRequest::create(String(p.first), p.second,this->req.getId(),context,
						[&](Value res) {
							if (res.defined()) {
								if (!res["error"].isNull() ) {
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
				delete this;
				return ;
			}
		} while (++mtfork == 2);

	}


protected:

};


void RpcServer::add_multicall(const String& name) {
	add(name, [this](RpcRequest req) {

		if (req.getArgs().empty()) req.setError(errorInvalidParams,"Empty arguments");
		else if (req[0].type() == json::string) {
			int pos = 1;
			MulticallContext *m = new MulticallContext(*this,req, [pos,req] () mutable {
				if (pos < req.getArgs().size())
					return std::make_pair(req[0],req[pos++]);
				else
					return std::make_pair(Value(),Value());
			});
			m->run();
		} else if (req[0].type() == json::array) {
			int pos = 0;
			MulticallContext *m = new MulticallContext(*this,req,[pos,req] () mutable {
				int x = pos++;
				if (x < req.getArgs().size())
					return std::make_pair(req[x][0],req[x][1]);
				else
					return std::make_pair(Value(),Value());
			});
			m->run();
		} else {
			req.setError(errorInvalidParams,"The first argument must be either string or array");
		}
	});
}

void RpcServer::add_ping(const String &name) {
	add(name, [](RpcRequest req) {
		req.setResult(req.getArgs());
	});
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

	LocalPendingCall(std::recursive_mutex &lock)
		:received(false),lock(lock) {}

	void release() {
		std::unique_lock<std::recursive_mutex> _(lock);
		//when the storage is destroyed - set received to true
		received = true;
		//notify conditional variable
		trig.notify_one();
	}

	RpcResult res;
	bool received;
	std::condition_variable_any trig;
	std::recursive_mutex &lock;
};



AbstractRpcClient::PreparedCall::operator RpcResult() {

	if (executed) return RpcResult();

	executed = true;

	Sync _(owner.lock);

	//declare storage here
	LocalPendingCall lpc(owner.lock);
	//register the storage (under its id)
	owner.addPendingCall(id,PPendingCall(&lpc, [](PendingCall *d){
		static_cast<LocalPendingCall *>(d)->release();}));
	//send request now
	owner.sendRequest(msg);


	lpc.trig.wait(_,[&]{return lpc.received;});


	//return received result
	return lpc.res;
}

AbstractRpcClient::PreparedCall AbstractRpcClient::operator ()(String methodName, Value args) {
	Sync _(lock);
	unsigned int id = ++idCounter;
	Value ctx = context.empty()?Value():context;
	switch (ver) {
	case RpcVersion::ver1:
		return PreparedCall(*this, id, Object("method", methodName)
											 ("params",args)
											 ("id",id)
											 ("context",ctx));
	case RpcVersion::ver2:
		return PreparedCall(*this, id, Object("method", methodName)
											 ("params",args)
											 ("id",id)
											 ("jsonrpc","2.0")
											 ("context",ctx));
	};
	throw std::runtime_error("Invalid JSONRPC version (client-call)");
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
	if (id.defined() && !id.isNull()) {
		Value result = response["result"];
		Value error = response["error"];
		if (id.type() == number && (result.defined() || error.defined())) {
			unsigned int nid = id.getUInt();
			Value ctx = response["context"];
			updateContext(ctx);
			if (result.defined() && (!error.defined() || error.isNull()))  {
				if (!cancelAsyncCall(nid,RpcResult(result,false,ctx))) return unexpected;
			} else {
				if (!cancelAsyncCall(nid,RpcResult(error,true,ctx))) return unexpected;
			}
			return success;

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

RpcRequest::RequestData::RequestData(const String& methodName,
		const Value& args, const Value& id, const Value& context, RpcFlags::Type flags)
	:methodName(methodName), args(args), id(id), context(context),flags(flags) {
	if (flags & RpcFlags::preResponseNotify) notifyEnabled = true;
}


Value RpcServer::formatError(int code,
							const String& message, Value data) const {
	return Value(object,{
			key/"code"=code,
			key/"message"=message,
			key/"data"=data
	});
}

void RpcRequest::setNoResultError(RequestData *r) {
	Value err =r->formatter->formatError(RpcServer::errorMethodDidNotProduceResult,"The method did not produce a result");
	r->setResponse(Value(object,{
		key/"error"=err,
		key/"result"=nullptr,
		key/"id"=r->id
	}));
}

void RpcRequest::setNoResultError() {
	setNoResultError(data);
}

void RpcRequest::setErrorFormatter(const IErrorFormatter *fmt) {
	data->formatter = fmt;
}

void RpcRequest::setInternalError(const char *what) {
	if (what == nullptr) what = "Internal error";
	Value err = data->formatter->formatError(RpcServer::errorInternalError,what);
	setError(err);
}

static Value formatNotify(RpcVersion::Type ver, const String name, Value data) {
	Value obj;
	switch (ver) {
	case RpcVersion::ver1:
		obj = Object("id", nullptr)("method", name)("params", data);
		break;
	case RpcVersion::ver2:
		obj = Object("method", name)("params", data)("jsonrpc", "2.0");
		break;
	}
	return obj;
}

///Send notify - note that owner can disable notify, then this function fails
bool RpcRequest::sendNotify(const String name, Value data) {
	Value obj= formatNotify(this->data->ver, name, data);
	return this->data->sendNotify(obj);


}
///Determines whether notification is enabled
bool RpcRequest::isSendNotifyEnabled() const {
		return data->notifyEnabled;
}

bool RpcRequest::RequestData::sendNotify(const Value& v) {
	if (notifyEnabled) {
		response(v);
		return true;
	} else {
		return false;
	}
}

void AbstractRpcClient::notify(String notifyName, Value args) {
	Sync _(lock);
	sendRequest(formatNotify(ver, notifyName, args));
}

AbstractRpcClient::AbstractRpcClient(RpcVersion::Type version)
: idCounter(0)
, ver(version)
{
}

void AbstractRpcClient::rejectAllPendingCalls() {
	callMap.clear();
}


Notify::Notify(Value js)
	:eventName(String(js["method"]))
	,data(js["params"])
{
}

RpcRequest Notify::asRequest() const {
	return RpcRequest::create(eventName, data, nullptr, Value(), [](Value){}, 0);
}

}
