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
#include "object.h"

namespace json {


///Carries result or error
class RpcResult: public Value {
public:
	///Initialize the object
	/**
	 * @param result result or error object, depend on isError status
	 * @param isError set to true, if the result is error
	 * @param context optional context. It contains whole updated context
	 */
	RpcResult(Value result, bool isError, Value context);
	///Inicializes empty result. Empty result is always set as error
	/**
	 * @note empty result is always error, because JSONRPC cannot have empty result. Empty
	 * error result is used in situation when neither result nor error is available, but the
	 * is no longer chance that method will return any result. This state is used to solve various
	 * technical issues, such as connection problems etc.
	 */
	RpcResult():error(true) {}

	///retrieve context
	Value getContext() const {return context;}

	///Deterine whether result is error
	bool isError() const {return error;}

	///Deterine whether result is error
	bool operator!() const {return error;}

	int getErrorCode() const {return this->operator []("code").getInt();}
	StrViewA getErrorMessage() const {return this->operator []("message").getString();}
	Value getErrorData() const {return this->operator []("data");}

protected:
	Value context;
	bool error;
};

class RpcRequest;

///Simple object which can be used to store notification which arrived from the client
/** you can construct Notify from the JSON. You can later convert it to RpcRequest */
struct RpcNotify {
	String eventName;
	Value data;

	///Construct notify from JSON
	explicit RpcNotify(Value js);

	///Construct notify
	RpcNotify(String eventName, Value data);

	///Create request from the notify
	/** this helps if you need to pass notify to the server as method call */
	RpcRequest asRequest() const;
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

enum class RpcVersion {
	///protocol version 1.0 or unspecified
	ver1,
	///protocol version 2.0
	ver2,
};


///Interface represents connection state
/**
 * If the client has a persistent connection with the RPC server, this interface can used to
 * query various informations. It can also push new informations into the state. These informations
 * are kept till close of the connection
 *
 */
class RpcConnContext: public RefCntObj {
public:

	///Queries for other interface
	/**
	 * @tparam This allow to server to expose various services. The caller only needs the
	 * name of interface. If the service is available, the function returns pointer to it. In
	 * other case, nullptr is returned
	 *
	 * @tparam Interface name of the interface. Note that service provider may require const
	 * access to the interface, so don't forget to specify const Interface, in this case.
	 *
	 * @return pointer to the specified interface or nullptr if interface is not implemented
	 */
	template<typename Interface>
	Interface *queryInterface() {
		Interface *z =  reinterpret_cast<Interface *>(queryInterfacePtr(typeid(Interface)));
		if (z == nullptr) z = dynamic_cast<Interface *>(this);
		return z;
	}

	///Store some value to the context of the connection
	/**
	 *
	 * @param key key
	 * @param value value
	 * If the key already exists, its value is overriden. To remove key, specify undefined as value
	 */
	virtual void store(StrViewA key, Value value) {data(key,value);}
	///Retrieves value soecified by the key
	/**
	 *
	 * @param key key
	 * @return if key exists, its value is returned, otherwise the undefined is returned
	 */
	virtual Value retrieve(StrViewA key) const {return data[key];}
	virtual ~RpcConnContext() {}
protected:
	///Implements queryInterface
	/**
	 * @param information about desired interface.
	 * @return if the interface is implemented, function should return pointer to it. Const
	 * pointers must be const_casted (they are converted to const on the caller's side
	 *
	 * Interfaces directly inherieted by the implementatio class are automatically available without
	 * need to implement this function. However this function can override the automatic behaviour.
	 * .
	 */
	virtual void *queryInterfacePtr(const std::type_info &) {return nullptr;}
	Object data;

};

typedef RefCntPtr<RpcConnContext> PRpcConnContext;


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


///Packs typical JSONRPC request into object. You can call RPC function with this object.
/** Object can be copied, because it is internally shared. It can be stored when
 * the method is executed asynchronous way. Until the request is resolved, it
 * alive */

class RpcRequest {
public:
	///once the request is being processed by the server, following service are available
	class IServerServices {
	public:
		///Formats the error message
		/** Performs standard formarting for JSONRPC 2.0
		 *
		 * @param code error code
		 * @param message message
		 * @param data optional data
		 * @return
		 */
		virtual Value formatError(int code, const String &message, Value data=Value()) const = 0;
		///Valation of arguments
		/**
		 *
		 * @param args arguments
		 * @param def validation definition
		 * @return if validatiion is successed, undefined is returned. The rejections are returned
		 * otherwise.
		 */
		virtual Value validateArgs(const Value &args, const Value &def) const = 0;
		virtual ~IServerServices() {}
	};

	///Contains request parsed to separated parts
	/** This class allows to build request from parts or from the whole JSONRPC message
	 *
	 *  IF you need just parse JSONRPC message, use it to construct this object. If you already
	 *  have parts, you can combine them and also construct this object. The instance of this
	 *  class is then used to initialize the request.
	 *
	 *  You don't need to explicitly specify the name of the class. It can be implicitly constructed
	 *  from the single json::Value - in case of whole JSONRPC message - or using the initializer_list
	 *  when it is being build from parts.
	 *
	 *  {method, params, id, context, version}
	 *
	 *  You can however specify less items, the other unspecified items become undefined.
	 *
	 *  {method, params}
	 *
	 *
	 */
	struct ParseRequest {
		Value methodName;
		Value params;
		Value id;
		Value context;
		Value version;

		ParseRequest() {}
		ParseRequest(const Value & request);
		ParseRequest(const Value &methodName, const Value &params, const Value &id=Value(0), const Value &context=Value(), const Value &version=Value());
		ParseRequest(const std::initializer_list<Value> &items);
	};


private:

	struct RequestData: public RefCntObj, public ParseRequest {
	public:
		RequestData(const ParseRequest &preq, RpcFlags::Type flags,const PRpcConnContext &connctx);

		Value rejections;
		Value diagData;

		const IServerServices *srvsvc = nullptr;
		bool responseSent = false;
		bool errorSent = false;
		bool notifyEnabled = false;
		RpcFlags::Type flags;
		PRpcConnContext connctx;

		virtual void sendResult(const Value &result, const Value &context);
		virtual void sendError(const Value &error);
		virtual bool sendNotify(const RpcNotify &notify);
		virtual bool sendCallback(RpcRequest request);
		virtual void postSendResponse(bool sendResult);

		virtual bool response(const Value &result) = 0;


		virtual ~RequestData() {}

		RpcVersion getVersion() const;

	};


public:

	///This interface allows you to define various callbacks called as the request is processed
	/** It expects the instance allocated on the heap, however this can be overridden by implementation
	 *
	 *  The base class has all functions empty, you need to override it and define own implementation
	 *
	 *  Also see RpcRequest::create
	 */
	class Callbacks {
	public:

		///called when request is finished by a result
		/**
		 * @param req request being processed
		 * @param result result
		 * @param context context generated by the method (can be undefined)
		 * @retval true post-request notifications are allowed (connection ongoing)
		 * @retval false post-request notifications are disallowed (connection lost)
		 */
		virtual bool onResult(const RpcRequest &req, const Value &result, const Value &context) noexcept{
			(void)context;
			return onResult(req, result);
		}
		///called when request is finished by a result
		/**
		 * @param req request being processed
		 * @param result result
		 * @retval true post-request notifications are allowed (connection ongoing)
		 * @retval false post-request notifications are disallowed (connection lost)
		 *
		 * @note this function is called only if the three-args version is not overridden
		 */
		virtual bool onResult(const RpcRequest &req, const Value &result) noexcept {
			(void)req; (void)result;
			return false;
		}
		///called when request is finished by an error
		/**
		 *
		 * @param req request being processed
		 * @param error error object
		 * @retval true post-request notifications are allowed (connection ongoing)
		 * @retval false post-request notifications are disallowed (connection lost)
		 */
		virtual bool onError(const RpcRequest &req, const Value &error) noexcept {
			(void)req;(void)error;
			return false;
		}
		///Called when request generates notification
		/**
		 * The request can generate notification anytime. Notifications can arrive
		 * before the request is finished and also after the finishing the request.
		 *
		 * However it is possible to stop generating future notification,
		 * by rejecting at least one notification.
		 *
		 * @param req request being processed
		 * @param ntf notification object
		 * @retval true allow future notifications
		 * @retval false disallow future notification
		 *
		 * @note there is no way to reenable disallowed notification
		 */
		virtual bool onNotify(const RpcRequest &req, const RpcNotify &ntf) noexcept {
			(void)req;(void)ntf;
			return false;
		}
		///Called when request is generates callback request
		/**
		 * the callback request is request sent from the server to the client. It is kind
		 * of notification which has also own ID. It is excepted that callback request is
		 * sent to the client and its response is then somehow received and the callback
		 * request is finished. The RpcServer doesn't support callbacks, it better to use
		 * AbstractRpcClient to handle opened callback requests.
		 *
		 * @param req request being processed
		 * @param cbreq callback request. To continue processing the source request, the
		 * callback request must be finished
		 * @retval true operation is in progress
		 * @retval false  operation is not supported (or disallowed)
		 *
		 * @note because the callbacks are built on notifications, once the notifications
		 * are disalloed, the calbacks are disallowed as well.
		 */
		virtual bool onCallback(const RpcRequest &req, RpcRequest &cbreq) noexcept {
			(void)req;(void)cbreq;
			return false;
		}

		///Method is called when the instance is no longer needed
		/**
		 * Default implemenation is to call delete this to end lifetime of the instance because
		 * the source request has been destroyed. However if the object is being allocated
		 * staticaly, you need to override this method to prevent deleting the statically allocated
		 * object.
		 */
		virtual void release()noexcept {delete this;}
		virtual ~Callbacks() {}
	};


	///Creates a request
	/**
	 * This function is mostly call as the first once the JSONRPC message is received
	 *
	 * @param reqdata contains a instance of ParseRequest which can be implicitly constructed from
	 * the JSONRPC message directly (carried as json::Value). By using an initializer list, you can
	 * construct this object from parts of JSONRPC message. See description of ParseRequest
	 * @param fn function which is called when request is finished. The function can accept one or two
	 * arguments. If the function has one argument, it is generated JSONRPC response, which can
	 * be directly serialized into output stream. If the function accepts two arguments, then second
	 * argument is reference to RpcRequest which is being processed. Note that function can be called
	 * by multipletimes, because notifications are also sent through this function.
	 * @param flags contains various flags which can limit or extend processing of the request. Note
	 * that default value disables notifications and maintain compatibility with JSONRPC 1.0
	 * @param connCtx context associated with the connection, if there is such thing. The argument can
	 * be nullptr. The method can access this context.
	 * @return request object. The object can be passed to the server for execution
	 */
	template<typename Fn, typename = decltype(_details::callCB(std::declval<Fn>(), std::declval<Value>(), std::declval<RpcRequest>()))>
	static RpcRequest create(const ParseRequest &reqdata, Fn &&fn, RpcFlags::Type flags = 0, const PRpcConnContext &connCtx = PRpcConnContext());

	///Creates a request which is probably processed internally
	/**
	 *
	 * @param reqdata contains a instance of ParseRequest which can be implicitly constructed from
	 * the JSONRPC message directly (carried as json::Value). By using an initializer list, you can
	 * construct this object from parts of JSONRPC message. See description of ParseRequest
	 * @param cbs pointer to object implementing Callbacks. Function also receives ownership of this
	 * object and the object is destroyed automatically with the destruction of the request (so the
	 * object should be allocated on heap) - however this behavior can be overridden by the
	 * object itself. Object contains various callback called in situations when the request generates
	 * result, error, notification or callback call.
	 * @param connCtx context associated with the connection, if there is such thing. The argument can
	 * be nullptr. The method can access this context.
	 * @return request object. The object can be passed to the server for execution
	 */
	static RpcRequest create(const ParseRequest &reqdata, Callbacks *cbs, const PRpcConnContext &connCtx = PRpcConnContext());



	///Function is called by RpcServer before the request is being processed
	/**
	 * @param srvsvc pointer to server's services
	 */
	void init(const IServerServices *srvsvc);

	///Name of method
	const Value &getMethodName() const;
	///Arguments as array
	const Value &getArgs() const;
	///Arguments as array
	const Value &getParams() const;
	///Id
	const Value &getId() const;
	///Context (optional)
	const Value &getContext() const;

	///access to arguments
	Value operator[](unsigned int index) const;
	///acfcess to context
	Value operator[](const StrViewA name) const;

	///Gets version of the request
	/**The server tracks version of the request to generate response in requested version
	 *
	 * @retval RpcVersion::ver1 version 1.0 (legacy version)
	 * @retval RpcVersion::ver2 version 2.0
	 *
	 * If the request has unknown version, it is processed as version 2.0
	 */
	RpcVersion getVersion() const;

	///Retrieves version field of the JSONRPC request
	/**
	 * @return version field. Note that JSONRPC 1.0 doesn't have version field, so in this
	 * case, result is undefined value. For the JSONRPC 2.0 the result is "2.0". If the request
	 * uses other (unsupported) version, the field contains string of the version.
	 */
	Value getVersionField() const;

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
	/**
	 * @param result
	 */
	void setResult(const Value &result);
	///set result (can be called once only)
	/**
	 * @param result
	 * @param context
	 */
	void setResult(const Value &result, const Value &context);
	///set error (can be called once only)
	/**
	 *
	 * @param error
	 */
	void setError(const Value &error);
	///set error (can be called once only)
	/**
	 *
	 * @param code
	 * @param message
	 * @param data
	 */
	void setError(int code, String message, Value data = Value());
	///Send notify - note that owner can disable notify, then this function fails
	/**
	 * @param name name of the notification
	 * @param data data of the notification
	 * @retval true notification sent
	 * @retval false unable to deliver notification. Notifications are disabled. Because there
	 * is no way how to reenable notifications, it also means, that source service can uninstall
	 * a notification observer for this response.
	 */
	bool sendNotify(const String name, Value data);
	///Determines whether notification is enabled
	/**
	 * @retval true notifications are enabled
	 * @retval false notifications are disabled
	 *
	 * @note the method doesn't actively test notification state. It just simply returns state
	 * of previous call the function sendNotify(). At the begining, it contain static state
	 * defined by the service provider.	 * @retval true response has been sent
	 * @retval false response cannot be sent (disconnected). But it is still considered as sent
	 *
	 */
	bool isSendNotifyEnabled() const;
	///Sets value which can be later send to the log. It can be any diagnostic data
	void setDiagData(Value v);
	///Retrieve diagnostic data
	const Value &getDiagData() const;
	///allows to modify args, for example if the request is forwarded to different method
	void setParams(Value args);

	///Determines whether response has been already sent
	/**
	 * @retval true response has been sent
	 * @retval false response has not been sent
	 */
	bool isResponseSent() const;

	///Determines whether respone has been already sent and was error
	/**
	 * @retval true response has been sent and it was error
	 * @retval false response has not been sent or response was not error
	 */
	bool isErrorSent() const;

	///Changes connection's context
	void setConnContext(const PRpcConnContext &ctx) {data->connctx = ctx;}

	///Retrieves cnnection's context
	PRpcConnContext getConnContext() const {return data->connctx;}

protected:
	RpcRequest(RefCntPtr<RequestData> data):data(data) {}

	static void setNoResultError(RequestData *data);

	RefCntPtr<RequestData> data;
};




template<typename Fn, typename>
inline RpcRequest json::RpcRequest::create(const ParseRequest &req, Fn&& fn, RpcFlags::Type flags, const PRpcConnContext &connCtx) {
	class Call: public RequestData {
	public:
		Call(const ParseRequest &req, const Fn& fn, RpcFlags::Type flags, const PRpcConnContext &connCtx)
			:RequestData(req,flags,connCtx)
			,fn(fn) {}
		virtual bool response(const Value &result) {
			return !!(_details::callCB(fn,result,RefCntPtr<RequestData>(this)));
		}
		~Call() {
			if (!responseSent) {
				RpcRequest::setNoResultError(this);
			}
		}
	protected:
		Fn fn;
	};

	return RpcRequest(new Call(req, fn,flags,connCtx));
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
	///Service provider doesn't support callbacks
	static const int errorCallbackIsNotSupported = -32098;


	///call the request
	void operator()(const RpcRequest &req) const noexcept;

	///Execute the request
	/** function can continue asynchronously */
	void exec(RpcRequest req) const noexcept;

	///Register a method
	/**
	 * @param name name of the method
	 * @param fn function which accepts RpcRequest and processes the method. Can
	 * be lambda function or any function with prototype void(RpcRequest)
	 *
	 */
	template<typename Fn>
	void add(const String &name, Fn &&fn);

	///Register a member method
	/**
	 * @param name name of the method
	 * @param objPtr pointer to object
	 * @param fn pointer to member function of given prototype
	 */
	template<typename ObjPtr, typename Fn>
	void add(const String &name, ObjPtr &&objPtr, void (Fn::*fn)(RpcRequest));

	///Remove method
	void remove(const StrViewA &name);

	void removeAll();

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

	///Override to perform custom validation
	virtual Value validateArgs(const Value &args, const Value &def) const override;

	///Adds buildin method Server.listMethods
	void add_listMethods(const String &name = "methods" );

	///Adds buildin method Server.multicall
	void add_multicall(const String &name = "multicall");

	///Adds buildin method Server.ping
	void add_ping(const String &name = "ping");

	///Adds buildin method Server.help
	/**
	 * @param helpContent content of help. For each method it contains key nad value is text which is returned as help
	 * @param name of the method
	 */
	void add_help(const Value &helpContent, const String &name = "help");


	///Enables or disables proxy
	/**
	 * Proxy allows to forward methods to the other rpc servers. The proxy function is called whenever
	 * the client makes a request for unknown method. The proxy function can forward the request
	 * to the other rpc client and also route the response back
	 *
	 * @param proxy specifty function which handles unknown method call. To disable proxy (default) set this argument to nullptr
	 *
	 * @retval true function can handle request
	 * @retval false function cannot handle request (error is generated to the client)
	 *
	 * @note The proxy need to decide whether it supports the method synchronously. If this is impossible
	 * to achive, the proxy function would return true and in case of calling unknown method it must
	 * generate apropriate error message
	 *
	 *
	 * @note Function "methods" (function added by add_listMedhods())  forwards the request to the proxy
	 * function to achieve listing of all methods. The proxy function need to detect this request
	 * (by testing the method signature) and forward the call to the target server
	 *
	 */
	void enableProxy(std::function<bool(RpcRequest)> proxy) {this->proxy = proxy;}

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
	std::function<bool(RpcRequest)> proxy;
	Value customRules;
};



template<typename Fn>
inline void RpcServer::add(const String& name, Fn &&fn) {
	class F: public AbstractMethodReg {
	public:
		F(const String &name, Fn &&fn):AbstractMethodReg(name), fn(std::forward<Fn>(fn)) {}
		virtual void call(const RpcRequest &req) const override {
			fn(req);
		}
	protected:
		Fn fn;
	};
	RefCntPtr<AbstractMethodReg> f = new F(name, std::forward<Fn>(fn));
	mapReg.insert(MapValue(f->name, f));
}

template<typename ObjPtr, typename Fn>
inline void RpcServer::add(const String& name, ObjPtr &&objPtr, void (Fn::*fn)(RpcRequest)) {
	class F: public AbstractMethodReg {
	public:
		F(const String &name, ObjPtr &&objPtr, void (Fn::*fn)(RpcRequest))
				:AbstractMethodReg(name), objPtr(std::forward<ObjPtr>(objPtr)),fn(fn) {}
		virtual void call(const RpcRequest &req) const override {
			((*objPtr).*fn)(req);
		}
	protected:
		ObjPtr objPtr;
		void (Fn::*fn)(RpcRequest);
	};
	RefCntPtr<AbstractMethodReg> f = new F(name, std::forward<ObjPtr>(objPtr), fn);
	mapReg.insert(MapValue(f->name, f));
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


	AbstractRpcClient(RpcVersion version);

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
		Value operator >> (const Fn &fn);

		///Perform synchronous call
		/**
		 * @return result of synchronous call;
		 */
		operator RpcResult();


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

	RpcVersion ver;

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
inline Value AbstractRpcClient::PreparedCall::operator >>(const Fn& fn) {
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




}


#endif /* SRC_IMTJSON_RPCSERVER_H_ */

