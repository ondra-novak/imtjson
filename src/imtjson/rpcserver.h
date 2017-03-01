/*
 * rpcserver.h
 *
 *  Created on: 28. 2. 2017
 *      Author: ondra
 */

#ifndef SRC_IMTJSON_RPCSERVER_H_
#define SRC_IMTJSON_RPCSERVER_H_
#include <map>
#include "refcnt.h"
#include "value.h"
#include "string.h"

namespace json {

class IRpcCaller {
public:
	virtual void release() = 0;
	virtual ~IRpcCaller() {}
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
 * The RpcServer itself is declared as a method
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
		F fn;
	};
	RefCntPtr<AbstractMethodReg> f = new F(name, fn);
	mapReg.insert(MapValue(f->name, f));
}

template<typename ObjPtr, typename Fn>
inline void RpcServer::add(const String& name, const ObjPtr& objPtr,
		const void (Fn::*fn)(RpcRequest)) {
	add(name, [=](const RpcRequest &req) {(objPtr->*fn)(req);});
}



}

#endif /* SRC_IMTJSON_RPCSERVER_H_ */

