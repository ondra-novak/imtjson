/*
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
#include "refcnt.h"
#include "value.h"
#include "string.h"

namespace json {

///Caller's context - it can carry services that are available during processing the call
class IRpcCaller {
public:
	///Implements this to release instance. It can contain pure delete this, or some kind of reference counting
	/** Function is called when the request is closed */
	virtual void release() = 0;
	virtual ~IRpcCaller() {}
};


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

	struct RequestData: public RefCntObj {
	public:
		RequestData(const Value &request, IRpcCaller *caller);
		~RequestData();

		String methodName;
		Value args;
		Value id;
		Value context;
		IRpcCaller *caller;
		bool responseSent;
		virtual void response(const Value &result) = 0;

		void setResponse(const Value &v);
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
	static RpcRequest create(const Value &request, IRpcCaller *context, const Fn &fn);

	///Name of method
	const String &getMethodName() const;
	///Arguments as array
	const Value &getArgs() const;
	///Id
	const Value &getId() const;
	///Context (optional)
	const Value &getContext() const;
	///pointer to caller
	const IRpcCaller *getCaller() const;
	///access to arguments
	Value operator[](unsigned int index) const;
	///acfcess to context
	Value operator[](const StrViewA name) const;


	///set result (can be called once only)
	void setResult(const Value &result);
	///set result (can be called once only)
	void setResult(const Value &result, const Value &context);
	///set error (can be called once only)
	void setError(const Value &error);
	///set error (can be called once only)
	void setError(unsigned int status, const String &message);
protected:

	RefCntPtr<RequestData> data;
};


template<typename Fn>
inline RpcRequest RpcRequest::create(const Value& request, IRpcCaller* context, const Fn& fn) {
	class Call: public RequestData {
	public:
		Call(const Value &request, IRpcCaller *caller, const Fn &fn)
			:RequestData(request,caller)
			,fn(fn) {}
		virtual void response(const Value &result) {fn(result);}
	protected:
		Fn fn;
	};

	return RpcRequest(new Call(request, context, fn));
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
class RpcServer {

	class AbstractMethodReg: public RefCntObj {
	public:
		const String name;
		virtual void call(const RpcRequest &req) const = 0;
		AbstractMethodReg(const String &name):name(name) {}
	};

public:

	///call the request
	void operator()(const RpcRequest &req) const;

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
	void add(const String &name, const ObjPtr &objPtr, const void (Fn::*fn)(RpcRequest));

	///Remove method
	void remove(const StrViewA &name);

	///Find method
	AbstractMethodReg *find(const StrViewA &name) const;

	///Called when method not found
	/**
	 * @param req request
	 */
	virtual void onMethodNotFound(const RpcRequest &req) const;
	///Called when request is malformed (doesn't contain mandatory fields)
	/**
	 * @param req request
	 */
	virtual void onMalformedRequest(const RpcRequest &req) const;

	///Adds buildin method Server.listMethods
	void add_listMethods(const String &name = "Server.listMethods" );

	///Adds buildin method Server.multicall
	void add_multicall(const String &name = "Server.multicall");

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
inline void RpcServer::add(const String& name, const ObjPtr& objPtr,
		const void (Fn::*fn)(RpcRequest)) {
	add(name, [=](const RpcRequest &req) {(objPtr->*fn)(req);});
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

