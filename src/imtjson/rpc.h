/*
 * @file Basic support for JSONRPC client/server. Version 1.0 is implemented extended by
 * special section "context" which allows to server store some data on client like a cookie. These data
 * are later send back with each request of the same client
 *
 * rpcserver.h
 *
 *  Created on: 28. 2. 2017
 *      Author: ondra
 */

#ifndef SRC_IMTJSON_RPCSERVER_H_
#define SRC_IMTJSON_RPCSERVER_H_
#include <map>
#include <atomic>
#include <memory>
#include <functional>
#include "refcnt.h"
#include "value.h"
#include "string.h"

namespace json {


///Carries result or error
class RpcResult: public Value {
public:
	RpcResult(Value result, bool isError, Value context);
	RpcResult():error(true) {}

	Value getContext() const {return context;}
	bool isError() const {return error;}

	bool operator!() const {return error;}
	operator bool() const {return !error;}

protected:
	Value context;
	bool error;
};

///Packs typical JSONRPC request into object. You can call RPC function with this object.
/** Object can be copied, because it is internally shared. It can be stored when
 * the method is executed asynchronous way. Until the request is resolved, it
 * alive */

class RpcRequest {
public:
	class IErrorFormatter {
	public:
		virtual Value formatError(bool exception,  int code, const String &message, Value data=Value()) const = 0;
		virtual ~IErrorFormatter() {}
	};


private:

	struct RequestData: public RefCntObj {
	public:
		RequestData(const Value &request);
		RequestData(const String &methodName,const Value &args,
				const Value &id,const Value &context);

		String methodName;
		Value args;
		Value id;
		Value context;
		Value rejections;
		const IErrorFormatter *formatter = nullptr;
		bool responseSent = false;
		virtual void response(const Value &result) = 0;

		void setResponse(const Value &v);
		virtual ~RequestData() {}

	};

public:


	///Creates the request
	/**
	 * @param request specify JSON parsed request. It should contain method name
	 * params and optionally context. The request should have ID, otherwise it is not replied
	 *
	 * @param context caller's context. It could be anything from the caller, interface
	 *   can carry any service need from caller. Can be set to nullptr
	 * @param fn function which will be executed to process the result. The result contains
	 *    a response prepared to be serialized to the output stream. Note that
	 *    this argument can be set to "undefined" in case, that method did not produced
	 *    a result.
	 * @return RpcRequest instance. Value can be copied.
	 */
	template<typename Fn>
	static RpcRequest create(const Value &request, const Fn &fn);


	template<typename Fn>
	static RpcRequest create(const String &methodName, const Value &args,const Value& id,  const Value &context, const Fn &fn);

	template<typename Fn>
	static RpcRequest create(const String &methodName, const Value &args, const Fn &fn);

	///Changes request's error formatter
	/** Functions which helps to format error message will use this formatter.
	 *  There is always a formatter defined, because this value is set when the request is processed
	 *  by the RpcServer
	 * @param fmt pointer to the formater.
	 *
	 * @note always ensure, that formatter is valid during lifetime of the request. The RpcServer
	 * must be destoyed after the all requests are finished (including asynchronous requests).
	 */
	void setErrorFormatter(const IErrorFormatter *fmt);

	///Name of method
	const String &getMethodName() const;
	///Arguments as array
	const Value &getArgs() const;
	///Id
	const Value &getId() const;
	///Context (optional)
	const Value &getContext() const;
	///access to arguments
	Value operator[](unsigned int index) const;
	///acfcess to context
	Value operator[](const StrViewA name) const;

	///Checks arguments
	/** Perform argument checking's.
	 *
	 * @param argDefTuple Array of argument types defined in the validator. Each position
	 *     match to argument position. Alternated types are allowed. "optional" is also
	 *     allowed to signal optional arguments. The count of arguments must be less or equal
	 *     to the count of items in argDefTuple, otherwise the rule is rejected.
	 *
	 * @retval true arguments valid, false validation failed. To retrieve rejections, call
	 * getRejections()
	 */
	bool checkArgs(const Value &argDefTuple);

	///Checks arguments
	/** Perform argument checking's.
	 *
	 * @param argDefTuple Array of argument types defined in the validator. Each position
	 *     match to argument position. Alternated types are allowed. "optional" is also
	 *     allowed to signal optional arguments.
	 * @param optionalArgs Rules for additional arguments.
	 * @retval true arguments valid, false validation failed. To retrieve rejections, call
	 * getRejections()
	 */
	bool checkArgs(const Value &argDefTuple, const Value &optionalArgs);

	///Checks arguments
	/** Perform argument checking's.
	 *
	 * @param argDefTuple Array of argument types defined in the validator. Each position
	 *     match to argument position. Alternated types are allowed. "optional" is also
	 *     allowed to signal optional arguments.
	 * @param optionalArgs Rules for additional arguments.
	 * @param customClasses contains validator's definition of custom rules. The default
	 * rule don't need to be present, because the checkArgs doesn't use default rule
	 * @retval true arguments valid, false validation failed. To retrieve rejections, call
	 * getRejections()
	 */
	bool checkArgs(const Value &argDefTuple, const Value &optionalArgs, const Value &customClasses);

	///Retrieves rejections of the previous checkArgs call
	Value getRejections() const;

	///Sets standard error 'invalid arguments'. You can supply rejections by calling getRejections
	void setArgError(Value rejections);

	///Sets standard error 'invalid arguments'. It pickups rejections
	void setArgError();

	///Sets standard error 'Method did not produce a result'
	void setNoResultError();

	///Sets internal error
	/**
	 * @param what what string from the exception
	 */
	void setInternalError(bool exception, const char *what);

	///set result (can be called once only)
	void setResult(const Value &result);
	///set result (can be called once only)
	void setResult(const Value &result, const Value &context);
	///set error (can be called once only)
	void setError(const Value &error);
	///set error (can be called once only)
	void setError(int code, String message, Value data = Value());

protected:
	RpcRequest(RefCntPtr<RequestData> data):data(data) {}

	static void setNoResultError(RequestData *data);

	RefCntPtr<RequestData> data;
};


template<typename Fn>
inline RpcRequest RpcRequest::create(const Value& request, const Fn& fn) {
	class Call: public RequestData {
	public:
		Call(const Value &request,  const Fn &fn)
			:RequestData(request)
			,fn(fn) {}
		virtual void response(const Value &result) {fn(result);}
		~Call() {
			if (!responseSent) {
				RpcRequest::setNoResultError(this);
			}
		}
	protected:
		Fn fn;
	};

	return RpcRequest(new Call(request, fn));
}

template<typename Fn>
inline RpcRequest json::RpcRequest::create(const String& methodName,
		const Value& args, const Value& id, const Value& context, const Fn& fn) {
	class Call: public RequestData {
	public:
		Call(const String& methodName, const Value& args, const Value& id, const Value& context, const Fn& fn)
			:RequestData(methodName,args,id,context)
			,fn(fn) {}
		virtual void response(const Value &result) {fn(result);}
		~Call() {
			if (!responseSent) {
				RpcRequest::setNoResultError(this);
			}
		}
	protected:
		Fn fn;
	};

	return RpcRequest(new Call(methodName, args,id,context,  fn));
}

template<typename Fn>
inline RpcRequest json::RpcRequest::create(const String& methodName, const Value& args, const Fn& fn) {
	return create(methodName, args, 0,Value(),fn);
}


///RpcServer object - it helps to build JSONRPC server.
/** The object doesn't care about reading, parsing and serializing the request. It
 * expects RpcRequest which can be created from json::Value. The request also defines
 * how the result is processed
 *
 * This class contains directory of methods. Each method can be registered along with
 * a function.
 *
 * The RpcServer itself is declared as a method.
 *
 * @note No methods are MT safe! For MT safety you need to wrap the class by locks
 */
class RpcServer: private RpcRequest::IErrorFormatter {

	class AbstractMethodReg: public RefCntObj {
	public:
		const String name;
		virtual void call(const RpcRequest &req) const = 0;
		AbstractMethodReg(const String &name):name(name) {}
	};

public:

	///JSON parse error is actually cannot happen here, but it is defined because the guidlines
	static const int errorParseError = -32700;
	///Invalid request (missing some mandatory field)
	static const int errorInvalidRequest = -32600;
	///Method not found
	static const int errorMethodNotFound = -32601;
	///Invalid parameters in method
	static const int errorInvalidParams = -32602;
	///Any exception thrown from the method
	static const int errorInternalError = -32603;
	///In situation, when request is finished without producing any result (extension)
	static const int errorMethodDidNotProduceResult = -32099;


	///call the request
	void operator()(RpcRequest req) const throw();

	///Register a method
	/**
	 * @param name name of the method
	 * @param fn function which accepts RpcRequest and processes the method. Can
	 * be lambda function or any function with prototype void(RpcRequest)
	 *
	 */
	template<typename Fn>
	void add(const String &name, const Fn fn);

	///Register a member method
	/**
	 * @param name name of the method
	 * @param objPtr pointer to object
	 * @param fn pointer to member function of given prototype
	 */
	template<typename ObjPtr, typename Fn>
	void add(const String &name, ObjPtr objPtr, void (Fn::*fn)(RpcRequest));

	///Remove method
	void remove(const StrViewA &name);

	///Find method
	AbstractMethodReg *find(const StrViewA &name) const;


	///Called to format error object
	/** Default implementation formats error message by specification of JSONRPC 2.0
	 *
	 * @param req the request
	 * @param exception true if function is called inside exception handler (so you can explore exception)
	 * @param code error code
	 * @param message error message
	 * @param data optional data
	 * @return error object
	 */
	virtual Value formatError(bool exception,  int code, const String &message, Value data=Value()) const;

	///Adds buildin method Server.listMethods
	void add_listMethods(const String &name = "Server.listMethods" );

	///Adds buildin method Server.multicall
	void add_multicall(const String &name = "Server.multicall");

	///Adds buildin method Server.ping
	void add_ping(const String &name = "Server.ping");

	///Adds buildin method Server.help
	/**
	 * @param helpContent content of help. For each method it contains key nad value is text which is returned as help
	 * @param name of the method
	 */
	void add_help(const Value &helpContent, const String &name = "Server.help");

protected:
	typedef std::map<StrViewA, RefCntPtr<AbstractMethodReg> > MapReg;
	typedef MapReg::value_type MapValue;

	MapReg mapReg;
};



template<typename Fn>
inline void RpcServer::add(const String& name, const Fn fn) {
	class F: public AbstractMethodReg {
	public:
		F(const String &name, const Fn &fn):AbstractMethodReg(name), fn(fn) {}
		virtual void call(const RpcRequest &req) const override {
			fn(req);
		}
	protected:
		Fn fn;
	};
	RefCntPtr<AbstractMethodReg> f = new F(name, fn);
	mapReg.insert(MapValue(f->name, f));
}

template<typename ObjPtr, typename Fn>
inline void RpcServer::add(const String& name, ObjPtr objPtr,
		void (Fn::*fn)(RpcRequest)) {
	add(name, [=](const RpcRequest &req) {((*objPtr).*fn)(req);});
}


///RpcClient core
/** The client doesn't contain serialization and desterialization. It expects
 *  that someone is able to perform this tasks. This class is abstract, it must be
 *  inherited and the function sendRequest must be implemented.
 *
 *  Receiving response is done by processResponse.
 *
 *  @note The class is not MT Safe. To achieve MT safety, you need to wrap all calls by a lock.
 *  The function onWait must release lock before it is called, and acquire the lock on return
 */
class AbstractRpcClient {
public:

	enum ReceiveStatus {
		///response has been received and processed
		success,
		///delivered RPC request (send the request to RpcServer)
		request,
		///delivered RPC notification (can be processed as request through RpcServer)
		notification,
		///valid response, but nobody is waiting for it
		unexpected

	};



	///Intermediate object created by operator()
	/**
	 * For asynchronous call use >>
	 * @code
	 * client("method",args) >> [](RpcResult &r) {...proces result ...};
	 * @endcode
	 *
	 * Asynchronous call return request's unique ID which can be used to cancel operation before the
	 * response arrives
	 *
	 * For synchronous call receive result directlu
	 * @code
	 * RpcResult result = client("method",args);
	 * @endcode
	 */
	class PreparedCall {
	public:
		///Perform asynchronous call
		/**
		 * @param fn function called to pick result
		 * @return request's unique identifier
		 */
		template<typename Fn>
		unsigned int operator >> (const Fn &fn);

		///Perform synchronous call
		/**
		 * @return result of synchronous call;
		 */
		operator RpcResult();

	protected:
		PreparedCall(AbstractRpcClient &owner, unsigned int id, const Value &msg);

		AbstractRpcClient &owner;
		unsigned int id;
		Value msg;
		bool executed;

		friend class AbstractRpcClient;
	};

	///Prepares a call
	/**
	 * @param methodName name of method
	 * @param args arguments
	 * @return object PreparedCall which can be send synchronously or asynchronously
	 */
	PreparedCall operator()(String methodName, Value args);

	///Cancels asynchronous call
	bool cancelAsyncCall(unsigned int id, RpcResult result);

	///Processes delivered response.
	/**
	 * @param response delivered response
	 * @return status of the response
	 */
	ReceiveStatus processResponse(Value response);

	///Retrieves current context
	Value getContext() const;
	///Sets current context
	void setContext(const Value &value);
	///Updates current context (by rules of updating the context)
	void updateContext(const Value &value);
	static Value updateContext(const Value &context, const Value &value);

	AbstractRpcClient(): idCounter(0){}
	virtual ~AbstractRpcClient() {}
	AbstractRpcClient(const AbstractRpcClient &other):idCounter(0) {}

protected:
	Value context;

	class LocalPendingCall;

	///Implements serializing and sending the request to the output channel.
	virtual void sendRequest(Value request) = 0;
	///Function must be overriden when it is wrapped by locks
	/** This function is called when a thread is blocked during synchronous call. If
	 * the locks are used, the function must release locks before it is called and acquire
	 * lock after return
	 *
	 * @param lcp handle to pending call. The value must be only passed to original function
	 *
	 * @code
	 * void MyRpcClient::onWait(LocalPendingCall &lpc) {
	 *    lk.unlock();
	 *    AbstractRpcClient::onWait(lpc);
	 *    lk.lock();
	 * }
	 * @endcode
	 */
	virtual void onWait(LocalPendingCall &lcp) throw() ;



	class PendingCall {
	public:
		virtual void onResponse(RpcResult response) = 0;
		virtual ~PendingCall() {}
	};

	typedef std::unique_ptr<PendingCall, void (*)(PendingCall *)> PPendingCall;


	typedef std::map<unsigned int, PPendingCall > CallMap;
	typedef CallMap::value_type CallItemType;
	CallMap callMap;

	std::atomic<unsigned int> idCounter;

	void addPendingCall(unsigned int id, PPendingCall &&pcall);


};

template<typename Fn>
inline unsigned int AbstractRpcClient::PreparedCall::operator >>(const Fn& fn) {
	if (!executed) {
		class PC: public PendingCall {
		public:
			PC(const Fn &fn):fn(fn) {}
			virtual void onResponse(RpcResult response) override {
				executed = true;
				fn(response);
			}
			~PC() {
				if (!executed)
					try {
						fn(RpcResult(undefined, true,undefined));
					} catch (...) {

					}
				}
		protected:
			Fn fn;
			bool executed = false;
		};

		owner.addPendingCall(id,PPendingCall(new PC(fn),[](PendingCall *p){delete p;}));
		owner.sendRequest(msg);
		executed = true;
	}
	return id;
}




}


#endif /* SRC_IMTJSON_RPCSERVER_H_ */

