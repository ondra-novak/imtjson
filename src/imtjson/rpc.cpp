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
#include "fnv.h"
namespace json {




RpcRequest::ParseRequest::ParseRequest(const Value & request) {

	methodName = request["method"];
	params = request["params"];
	context = request["context"];
	id = request["id"];
	version = request["jsonrpc"];


}

RpcRequest::ParseRequest::ParseRequest(const Value &methodName, const Value &params, const Value &id, const Value &context, const Value &version)
	:methodName(methodName)
	,params(params)
	,id(id)
	,context(context)
	,version(version) {}

template<std::size_t pos>
static inline Value pick(const std::initializer_list<Value> &items) {
	if (pos < items.size()) return *(items.begin()+pos); else return Value();
}

RpcRequest::ParseRequest::ParseRequest(const std::initializer_list<Value> &items)
	:methodName(pick<0>(items))
	,params(pick<1>(items))
	,id(pick<2>(items))
	,context(pick<3>(items))
	,version(pick<4>(items))
{

}


RpcRequest::RequestData::RequestData(const ParseRequest& request, RpcFlags::Type flags,const PRpcConnContext &connctx)
	:ParseRequest(request), flags(flags),connctx(connctx)
{

	if (params.type() != array) {
		if (params.type() != object || (flags & RpcFlags::namedParams) == 0) {
			params = Value(array, {params});
		}
	}
	if (flags & RpcFlags::preResponseNotify) notifyEnabled = true;
}

Value RpcServer::defaultFormatError(int code, const String& message, Value data) {
	return Value(object,{
			Value("code",code),
			Value("message",message),
			Value("data",data)
	});
}

static Value formatNotify(RpcVersion ver, const String name, Value data, Value context) {
	Value obj;
	switch (ver) {
	case RpcVersion::ver1:
		obj = Object("id", nullptr)("method", name)("params", data)("context",context);
		break;
	case RpcVersion::ver2:
		obj = Object("method", name)("params", data)("jsonrpc", "2.0")("context",context);
		break;
	}
	return obj;
}


const Value& RpcRequest::getMethodName() const {
	return data->methodName;
}

const Value& RpcRequest::getArgs() const {
	return data->params;
}

const Value& RpcRequest::getParams() const {
	return data->params;
}

const Value& RpcRequest::getId() const {
	return data->id;
}

const Value& RpcRequest::getContext() const {
	return data->context;
}



void RpcRequest::RequestData::sendResult(const Value &result, const Value &context) {
	if (id.defined() && !id.isNull() && !responseSent) {
		Value ctxch = context.empty()? Value():context;
		switch (getVersion()) {
		case RpcVersion::ver1:
				return postSendResponse(
					response(Object("id",id)
									 ("result",result)
									 ("error",nullptr)
									 ("context",ctxch)));

		case RpcVersion::ver2:
			return postSendResponse(
					response(Object("id",id)
							 ("result",result)
							 ("jsonrpc","2.0")
							 ("context",ctxch)));
		}
	}
}
void RpcRequest::RequestData::sendError(const Value &error) {
	if (id.defined() && !id.isNull() && !responseSent) {
		responseSent = true;
		errorSent = true;
		switch (getVersion()) {
		case RpcVersion::ver1:
			return postSendResponse(
					response(Object("id",id)
								 ("error",error)
								 ("result",nullptr)));
		case RpcVersion::ver2:
			return postSendResponse(
					response(Object("id",id)
							 ("error",error)
							 ("jsonrpc","2.0")));
				return;
		}
	}

}
void RpcRequest::RequestData::postSendResponse(bool sendResult) {
	responseSent = true;
	notifyEnabled =  (flags & RpcFlags::postResponseNotify) != 0 && sendResult;
}

bool RpcRequest::RequestData::sendNotify(const RpcNotify &notify) {
	if (notifyEnabled) {
		notifyEnabled = response(formatNotify(getVersion(),notify.eventName, notify.data, notify.context));
	}
	return notifyEnabled;

}
bool RpcRequest::RequestData::sendCallback(RpcRequest request) {
	request.setError(RpcServer::errorCallbackIsNotSupported,"Callback is not supported");
	return true;
}


void RpcRequest::setResult(const Value& result) {
	setResult(result, undefined);

}

void RpcRequest::setResult(const Value& result, const Value& context) {


	data->sendResult(result,context);
	/*
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
	*/

	//data->setResponse(resp);


}

void RpcRequest::setError(const Value& error) {
	data->sendError(error);
	/*
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

	return data->setResponse(resp);
	*/
}

Value RpcRequest::operator [](unsigned int index) const {
	return data->params[index];
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
};

void RpcRequest::setDiagData(Value v) {
	data->diagData = v;
}
///Retrieve diagnostic data
const Value &RpcRequest::getDiagData() const {
	return data->diagData;
}

bool RpcRequest::checkArgs(const Value& argDefTuple) {
	Value v = data->srvsvc->validateArgs(data->params,argDefTuple);
	if (v.isNull() || !v.defined()) return true;
	data->rejections = v;
	return false;
}


Value RpcRequest::getRejections() const {
	return data->rejections;
}

void RpcRequest::setArgError(Value rejections) {
	setError(-32602, "Invalid params", rejections);
}

void RpcRequest::setArgError() {
	return setArgError(getRejections());
}

void RpcRequest::setError(int code, String message, Value d) {
	if (data->srvsvc == nullptr) {
		return setError(RpcServer::defaultFormatError(code,message,d));
	} else {
		return setError(data->srvsvc->formatError(code,message,d));
	}
}


void RpcServer::operator ()(const RpcRequest &req) const noexcept {
	exec(req);
}
void RpcServer::exec( RpcRequest req) const noexcept {
	try {
		req.init(this);
		Value name = req.getMethodName();
		Value args = req.getArgs();
		Value context = req.getContext();
		if (name.defined() && name.type() == string && (args.type() == array || args.type() == object || args.type() == undefined)
				&& (!context.defined() || context.type() == object)) {
			AbstractMethodReg *m = find(name.getString());
			if (m == nullptr) {
				if (proxy != nullptr)
					if (proxy(req)) return;
				req.setError(errorMethodNotFound,"Method not found",name);
			} else {
				m->call(req);
			}
		} else {
			req.setError(errorInvalidRequest,"Invalid request");
		}
	} catch (std::exception &e) {
		try {
			if (exception_handler==nullptr || !exception_handler(req))
					req.setInternalError(e.what());
		} catch (...) {
			req.setInternalError(e.what());
		}
	} catch (...) {
		try {
			if (exception_handler==nullptr || !exception_handler(req))
					req.setInternalError(nullptr);
		} catch (...) {
			req.setInternalError(nullptr);
		}
	}
}

void RpcServer::remove(const StrViewA& name) {
	mapReg.erase(name);
}

void RpcServer::removeAll() {
	mapReg.clear();
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
		Value r(res);

		if (proxy != nullptr) {
			auto proxyReq = RpcRequest::create(RpcRequest::ParseRequest(
							req.getMethodName(),
							req.getArgs(),
							req.getId(),
							req.getContext(),
							req.getVersionField()),
					[=](Value resp) mutable {

					Array res2(r);
					res2.addSet(resp["result"]);
					req.setResult(res2);

					return true;
				});
			if (proxy(proxyReq)) return;

		}
		req.setResult(r);
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

	MulticallContext(RpcServer &server, RpcRequest req, const NextFn &nextFn):req(req),nextfn(nextFn),server(server) {
		context = req.getContext();

	}

	void run() {
		do {
			auto p = nextfn();
			mtfork = 0;
			if (p.first.defined()) {
				RpcRequest::ParseRequest pr (p.first, p.second, req.getId(), context, req.getVersionField());
				class CBS: public RpcRequest::Callbacks {
				public:
					virtual bool onResult(const RpcRequest &, const Value &res, const Value &ctx) noexcept override  {
						owner.results.push_back(res);
						if (ctx.defined()) {
							owner.context = AbstractRpcClient::updateContext(owner.context,ctx);
							owner.contextchange = AbstractRpcClient::updateContext(owner.contextchange,ctx);
						}
						if (++owner.mtfork == 2) {
							owner.run();
						}
						return true;
					}
					virtual bool onError(const RpcRequest &, const Value &err) noexcept override  {
						owner.results.push_back(Value(nullptr));
						owner.errors.push_back(err);
						if (++owner.mtfork == 2) {
							owner.run();
						}
						return true;
					}
					virtual bool onNotify(const RpcRequest &, const RpcNotify &ntf) noexcept override  {
						return req.sendNotify(ntf.eventName, ntf.data);
					}
					virtual bool onCallback(const RpcRequest &, RpcRequest &) noexcept override  {
						return false;
					}

					MulticallContext &owner;
					RpcRequest req;
					CBS(MulticallContext &owner):owner(owner),req(owner.req) {}
				};

				RpcRequest req = RpcRequest::create(pr,new CBS(*this));

				server.exec(req);
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
		else if (req[0].type() == string) {
			std::size_t pos = 1;
			MulticallContext *m = new MulticallContext(*this,req, [pos,req] () mutable {
				if (pos < req.getArgs().size())
					return std::make_pair(req[0],req[pos++]);
				else
					return std::make_pair(Value(),Value());
			});
			m->run();
		} else if (req[0].type() == array) {
			std::size_t pos = 0;
			MulticallContext *m = new MulticallContext(*this,req,[pos,req] () mutable {
				std::size_t x = pos++;
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

Value RpcServer::validateArgs(const Value& args, const Value& def) const {
	RpcArgValidator v(customRules);
	if (v.checkArgs(args, def)) return null;
	else return v.getRejections();
}

void RpcServer::add_help(const Value& helpContent, const String &name) {

	Value helpCtx = helpContent;
	add(name, [helpCtx](RpcRequest req) {
		String n (req[0]);
		req.setResult(helpCtx[n]);
	});

}

RpcResult::RpcResult(Value result, bool isError, Value context)
	:Value(result),context(context),error(isError)
{}

AbstractRpcClient::PreparedCall::PreparedCall(AbstractRpcClient& owner, const Value &id, const Value& msg)
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
	Value id = genRequestID();
	Value ctx = context.empty()?Value():context;
	return this->operator ()(methodName, args, ctx);
}

AbstractRpcClient::PreparedCall AbstractRpcClient::operator ()(String methodName, Value args, Value context) {
	Sync _(lock);
	Value id = genRequestID();
	switch (ver) {
	case RpcVersion::ver1:
		return PreparedCall(*this, id, Object("method", methodName)
											 ("params",args)
											 ("id",id)
											 ("context",context));
	case RpcVersion::ver2:
		return PreparedCall(*this, id, Object("method", methodName)
											 ("params",args)
											 ("id",id)
											 ("jsonrpc","2.0")
											 ("context",context));
	};
	throw std::runtime_error("Invalid JSONRPC version (client-call)");
}

bool AbstractRpcClient::cancelAsyncCall(Value id, RpcResult result) {
	return cancelPendingCall(id, result);
}

bool AbstractRpcClient::cancelPendingCall(Value id, RpcResult result) {
	Sync _(lock);
	auto it = callMap.find(id);
	if (it == callMap.end()) return false;
	PPendingCall pc = std::move(it->second);
	callMap.erase(it);
	onUnregister();
	_.unlock();
	pc->onResponse(result);
	return true;
}

void AbstractRpcClient::cancelAllPendingCalls(RpcResult result) {
	std::vector<Value> ids;
	{
		Sync _(lock);
		ids.reserve(callMap.size());
		for (auto &&x: callMap) ids.push_back(x.first);
	}
	for (auto &&x: ids) cancelPendingCall(x, result);
}

AbstractRpcClient::ReceiveStatus AbstractRpcClient::processResponse(Value response) {
	Value id = response["id"];
	if (id.defined() && !id.isNull()) {
		Value result = response["result"];
		Value error = response["error"];
		if (result.defined() || error.defined()) {
			Value ctx = response["context"];
			updateContext(ctx);
			if (result.defined() && (!error.defined() || error.isNull()))  {
				if (!cancelAsyncCall(id,RpcResult(result,false,ctx))) return unexpected;
			} else {
				if (!cancelAsyncCall(id,RpcResult(error,true,ctx))) return unexpected;
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
	Object newContext(context);
	for (Value v: value) {
		if (v.isNull()) newContext.unset(v.getKey());
		else newContext.set(v);
	}
	return newContext;
}

void AbstractRpcClient::addPendingCall(const Value &id, PPendingCall &&pcall) {
	callMap.insert(CallItemType(id,std::move(pcall)));
}




Value RpcServer::formatError(int code, const String& message, Value data) const {
	return defaultFormatError(code,message,data);
}

void RpcRequest::setNoResultError(RequestData *r) {
	r->addRef();
	if (r->srvsvc != nullptr) {
		try {
			Value err =r->srvsvc->formatError(RpcServer::errorMethodDidNotProduceResult,"The method did not produce a result");
			r->sendError(err);
		} catch (...) {

		}
	}
	r->release();

}

void RpcRequest::setNoResultError() {
	setNoResultError(data);
}

void RpcRequest::init(const IServerServices *srvsvc) {
	data->srvsvc = srvsvc;
}

void RpcRequest::setInternalError(const char *what) {
	Value whatV = what?Value(what):Value();
	setError(RpcServer::errorInternalError, "Internal error", whatV);
}


///Send notify - note that owner can disable notify, then this function fails
bool RpcRequest::sendNotify(const String name, Value data) {
//	Value obj= formatNotify(this->data->ver, name, data);
	return this->data->sendNotify(RpcNotify(name, data));


}

bool RpcRequest::RequestData::hearthbeat() {
	return response(Value());
}

bool RpcRequest::hearthbeat() noexcept {
	return this->data->hearthbeat();
}

///Determines whether notification is enabled
bool RpcRequest::isSendNotifyEnabled() const {
		return data->notifyEnabled;
}


void AbstractRpcClient::notify(String notifyName, Value args) {
	Sync _(lock);
	sendRequest(formatNotify(ver, notifyName, args, Value()));
}
void AbstractRpcClient::notify(String notifyName, Value args, Value context) {
	Sync _(lock);
	sendRequest(formatNotify(ver, notifyName, args, context));
}

AbstractRpcClient::AbstractRpcClient(RpcVersion version)
: idCounter(0)
, ver(version)
{
}

void AbstractRpcClient::rejectAllPendingCalls() {
	callMap.clear();
}


RpcNotify::RpcNotify(Value js)
	:eventName(String(js["method"]))
	,data(js["params"])
{
}


RpcNotify::RpcNotify(String eventName, Value data)
	:eventName(eventName)
	,data(data)
{

}
RpcNotify::RpcNotify(String eventName, Value data, Value context)
	:eventName(eventName)
	,data(data)
	,context(context)
{

}

RpcRequest RpcNotify::asRequest() const {
	return RpcRequest::create(RpcRequest::ParseRequest(eventName, data, Value(), context), [](Value){return false;}, 0);
}


bool RpcRequest::isResponseSent() const {
	return data->responseSent;
}

bool RpcRequest::isErrorSent() const {
	return data->errorSent;
}

void RpcServer::setCustomValidationRules(Value customRules) {
	this->customRules = customRules;
}

Value AbstractRpcClient::genRequestID () {
	unsigned int id = ++idCounter;
	return id;

}

std::size_t RpcServer::HashStr::operator()(StrViewA str) const {
	std::size_t h = 0;
	FNV1a<sizeof(std::size_t)> c(h);
	for(char x:str) c(x);
	return h;
}


RpcVersion RpcRequest::RequestData::getVersion() const {
	if (version.defined()) return RpcVersion::ver2;
	else return RpcVersion::ver1;
}


RpcRequest RpcRequest::create(const ParseRequest& reqdata, Callbacks* cbs, const PRpcConnContext &connCtx) {
	class Call: public RequestData {
	public:
		Call(const ParseRequest &req, Callbacks* cbs, RpcFlags::Type flags, const PRpcConnContext &connCtx)
			:RequestData(req,flags,connCtx)
			,cbs(cbs) {}

		virtual void sendResult(const Value &result, const Value &context) override {
			postSendResponse(cbs->onResult(RpcRequest(this),result,context));
		}
		virtual void sendError(const Value &error) noexcept override {
			postSendResponse(cbs->onError(RpcRequest(this),error));
		}
		virtual bool sendNotify(const RpcNotify &notify) noexcept override{
			return cbs->onNotify(RpcRequest(this),notify);
		}
		virtual bool sendCallback(RpcRequest request) noexcept override {
			return cbs->onCallback(RpcRequest(this), request);
		}
		virtual bool response(const Value &) noexcept override {return false;}

		virtual bool hearthbeat() noexcept override {
			return cbs->onHearthbeat(RpcRequest(this));
		}

		~Call() {
			if (!responseSent) {
				RpcRequest::setNoResultError(this);
			}
			cbs->release();
		}
	protected:
		Callbacks* cbs;
	};

	return RpcRequest(new Call(reqdata, cbs,RpcFlags::notify, connCtx));


}

RpcVersion RpcRequest::getVersion() const {
	return data->getVersion();
}

Value RpcRequest::getVersionField() const {
	return data->version;
}

void RpcRequest::setParams(Value args) {
	data->params = args;
}

}
