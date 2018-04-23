/*
0 * @file Basic support for JSONRPC client/server. Version 1.0 is implemented extended by
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
#include <condition_variable>
#include <mutex>
#include <unordered_map>
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


namespace RpcFlags {

	typedef unsigned int Type;
	///Allow pre-response notify
	/**sending notifications is allowed until the response is generatedm then notification are disabled
	 *
	 * Thsi could be case of HTTP response, when the connection is avaialable until the response is sent
	 *
	 * */
	static const Type preResponseNotify= 1;
	///Allow post-response notify
	/**sending notifications is allowed only after the response is generated */
	static const Type postResponseNotify = 2;
	///Allow notifications at all
	static const Type notify = 3;

	///Enable named parameters
	/** This option is disabled by default to achieve best compatibility with version 1.0. Request
	 * which uses named parameters is converted to request containing one parameter which
	 * contains object with named parameters. Set this flag if you need to disable this conversion.
	 */
	static const Type namedParams = 4;


}

namespace RpcVersion {

enum Type {
	///protocol version 1.0 or unspecified
	ver1,
	///protocol version 2.0
	ver2
};


}

///Packs typical JSONRPC request into object. You can call RPC function with this object.
/** Object can be copied, because it is internally shared. It can be stored when
 * the method is executed asynchronous way. Until the request is resolved, it
 * alive */

class RpcRequest {
public:
	class IServerServices {
	public:
		virtual Value formatError(int code, const String &message, Value data=Value()) const = 0;
		virtual Value validateArgs(const Value &args, const Value &def) const = 0;
		virtual ~IServerServices() {}
	};



private:

	struct RequestData: public RefCntObj {
	public:
		RequestData(const Value &request, RpcFlags::Type flags);
		RequestData(const String &methodName,const Value &args,
				const Value &id,const Value &context, RpcFlags::Type flags);

		String methodName;
		Value args;
		Value id;
		Value context;
		Value rejections;
		Value diagData;
		RpcVersion::Type ver;
		const IServerServices *srvsvc = nullptr;
		bool responseSent = false;
		bool errorSent = false;
		bool notifyEnabled = false;
		RpcFlags::Type flags;
		virtual void response(const Value &result) = 0;

		void setResponse(const Value &v);
		bool sendNotify(const Value &v);
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
	 * @param flags some extra settings of the request
	 * @return RpcRequest instance. Value can be copied.
	 */
	template<typename Fn>
	static RpcRequest create(const Value &request, const Fn &fn, RpcFlags::Type flags = 0);

	template<typename Fn>
	static RpcRequest create(const String &methodName, const Value &args,const Value& id,  const Value &context, const Fn &fn, RpcFlags::Type flags = 0);

	template<typename Fn>
	static RpcRequest create(const String &methodName, const Value &args, const Fn &fn, RpcFlags::Type flags = 0);


	///Function is called by RpcServer before the request is being processed
	void init(const IServerServices *srvsvc);

	///Name of method
	const String &getMethodName() const;
	///Arguments as array
	const Value &getArgs() const;
	///Id
	const Value &getId() const;
	///Context (optional)
	const Value &getContext() const;

	StrViewA getVer() const;
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
	void setInternalError(const char *what);

	///set result (can be called once only)
	void setResult(const Value &result);
	///set result (can be called once only)
	void setResult(const Value &result, const Value &context);
	///set error (can be called once only)
	void setError(const Value &error);
	///set error (can be called once only)
	void setError(int code, String message, Value data = Value());
	///Send notify - note that owner can disable notify, then this function fails
	bool sendNotify(const String name, Value data);
	///Determines whether notification is enabled
	bool isSendNotifyEnabled() const;
	///Sets value which can be later send to the log. It can be any diagnostic data
	void setDiagData(Value v);
	///Retrieve diagnostic data
	const Value &getDiagData() const;
	///allows to modify args, for example if the request is forwarded to different method
	void setArgs(Value args);

	bool isResponseSent() const;

	bool isErrorSent() const;


protected:
	RpcRequest(RefCntPtr<RequestData> data):data(data) {}

	static void setNoResultError(RequestData *data);

	RefCntPtr<RequestData> data;
};


namespace _details {

	template<typename Fn>
	auto callCB(Fn &&fn, const Value &v, const RpcRequest &) -> decltype(std::declval<Fn>()(v)) {
		return fn(v);
	}
	template<typename Fn>
	auto callCB(Fn &&fn, const Value &v, const RpcRequest &req) -> decltype(std::declval<Fn>()(v,req)) {
		return fn(v,req);
	}

	template<typename Fn>
	auto callCB(Fn &&fn, const Value &v, const RpcRequest &req) -> decltype(std::declval<Fn>()(req,v)) {
		return fn(req,v);
	}
}

template<typename Fn>
inline RpcRequest RpcRequest::create(const Value& request, const Fn& fn, RpcFlags::Type flags) {
	class Call: public RequestData {
	public:
		Call(const Value &request,  const Fn &fn, RpcFlags::Type flags)
			:RequestData(request,flags)
			,fn(fn) {}
		virtual void response(const Value &result) {
			_details::callCB(fn,result,RefCntPtr<RequestData>(this));
		}
		~Call() {
			if (!responseSent) {
				RpcRequest::setNoResultError(this);
			}
		}
	protected:
		Fn fn;
	};

	return RpcRequest(new Call(request, fn, flags));
}

template<typename Fn>
inline RpcRequest json::RpcRequest::create(const String& methodName,
		const Value& args, const Value& id, const Value& context, const Fn& fn, RpcFlags::Type flags) {
	class Call: public RequestData {
	public:
		Call(const String& methodName, const Value& args, const Value& id, const Value& context, const Fn& fn, RpcFlags::Type flags)
			:RequestData(methodName,args,id,context,flags)
			,fn(fn) {}
		virtual void response(const Value &result) {
			_details::callCB(fn,result,RefCntPtr<RequestData>(this));
		}
		~Call() {
			if (!responseSent) {
				RpcRequest::setNoResultError(this);
			}
		}
	protected:
		Fn fn;
	};

	return RpcRequest(new Call(methodName, args,id,context,  fn,flags));
}

template<typename Fn>
inline RpcRequest json::RpcRequest::create(const String& methodName, const Value& args, const Fn& fn, RpcFlags::Type flags) {
	return create(methodName, args, 0,Value(),fn, flags);
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
class RpcServer: private RpcRequest::IServerServices {

	class AbstractMethodReg: public RefCntObj {
	public:
		const String name;
		virtual void call(const RpcRequest &req) const = 0;
		AbstractMethodReg(const String &name):name(name) {}

		virtual ~AbstractMethodReg() {}
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
	 * @param code error code
	 * @param message error message
	 * @param data optional data
	 * @return error object
	 */
	virtual Value formatError(int code, const String &message, Value data=Value()) const override;

	virtual Value validateArgs(const Value &args, const Value &def) const override;

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

	///define custom rules
	/**
	 * @param customRules - object containing custom rules for the validator
	 */
	void setCustomValidationRules(Value curstomRules);

protected:
	struct HashStr {
		std::size_t operator()(StrViewA str) const;
	};
	typedef std::unordered_map<StrViewA, RefCntPtr<AbstractMethodReg>, HashStr> MapReg;
	typedef MapReg::value_type MapValue;

	MapReg mapReg;

	Value customRules;
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

 */
class AbstractRpcClient {
public:


	AbstractRpcClient(RpcVersion::Type version);

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

		///Receives ID of the request
		/** Y can use the ID to cancel pending call later */
		const Value &getID() const {
			return id;
		}

	protected:
		PreparedCall(AbstractRpcClient &owner, const Value &id, const Value &msg);

		AbstractRpcClient &owner;
		Value id;
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

	///Send notify to the server
	/**
	 * @param notifyName name of notify (or method)
	 * @param args arguments
	 *
	 * @nore notify is always asynchronous.
	 */
	void notify(String notifyName, Value args);

	///Cancels asynchronous call
	/** Obsolete function */
	bool cancelAsyncCall(Value id, RpcResult result);

	///Cancels pending call
	/** This allows to cancel one pending call identified by the id.
	 * Use this if you need to cancel waiting in situation like a timeout or similar.
	 *
	 * @param id identifier of the call
	 * @param result value passed as result to canceled request
	 * @retval true canceled
	 * @retval false not found - probably already processed
	 */
	bool cancelPendingCall(Value id, RpcResult result);

	///Cancels all asynchronous calls
	/** Use this to cleanup the all pending calls after the connection is lost
	 *
	 * @param result result is send to every canceled call
	 */
	void cancelAllPendingCalls(RpcResult result);

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

	virtual ~AbstractRpcClient() {}
	AbstractRpcClient(const AbstractRpcClient &):idCounter(0) {}

protected:
	Value context;

	class LocalPendingCall;

	///Implements serializing and sending the request to the output channel.
	/**
	 * @param request request which must be serialized to the output stream
	 *
	 * @note It is possible to use the client during sendRequest as the function
	 * can be recursive. You can for example connect the target server, and call several
	 * methods before the requested method is called. However during this phase, you need
	 * to avoid using synchronous calls.
	 */
	virtual void sendRequest(Value request) = 0;

	class PendingCall {
	public:
		virtual void onResponse(RpcResult response) = 0;
		virtual ~PendingCall() {}
	};

	typedef std::unique_ptr<PendingCall, void (*)(PendingCall *)> PPendingCall;


	typedef std::unordered_map<Value, PPendingCall > CallMap;
	typedef CallMap::value_type CallItemType;
	CallMap callMap;

	std::recursive_mutex lock;
	typedef std::unique_lock<std::recursive_mutex> Sync;
	typedef std::condition_variable_any CondVar;


	unsigned int idCounter;

	RpcVersion::Type ver;

	void addPendingCall(const Value &id, PPendingCall &&pcall);

	///call this function if you need to reject all pending calls
	/** this can be needed when client lost connection so all pending calls are lost. Rejected
	 * pending calls can be repeated, but it isn't often good idea
	 */
	void rejectAllPendingCalls();

	///Generates ID for the request
	/** this allows to override default implementation of ID generation */
	/** @note called under lock */
	virtual Value genRequestID();

};

template<typename Fn>
inline unsigned int AbstractRpcClient::PreparedCall::operator >>(const Fn& fn) {
	if (!executed) {

		class PC: public PendingCall {
		public:
			PC(const Fn &fn):fn(fn) {}
			virtual void onResponse(RpcResult response) override {
				res = response;
			}
			~PC() {
				try {
					fn(res);
				} catch (...) {

				}
			}
		protected:
			Fn fn;
			RpcResult res;
		};

		Sync _(owner.lock);
		owner.addPendingCall(id,PPendingCall(new PC(fn),[](PendingCall *p){delete p;}));
		owner.sendRequest(msg);
		executed = true;
	}
	return id;
}

///Simple object which can be used to store notification which arrived from the client
/** you can construct Notify from the JSON. You can later convert it to RpcRequest */
struct Notify {
	String eventName;
	Value data;

	explicit Notify(Value js);
	RpcRequest asRequest() const;
};




}


#endif /* SRC_IMTJSON_RPCSERVER_H_ */

