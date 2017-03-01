/*
 * rpcserver.cpp
 *
 *  Created on: 28. 2. 2017
 *      Author: ondra
 */

#include "rpcserver.h"

#include "array.h"
namespace json {


RpcRequest::RequestData::RequestData(const Value& request,IRpcCaller* caller) {
	methodName = String(request["method"]);
	args = request["params"];
	id = request["id"];
	context = request["context"];
	this->caller = caller;
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

const IRpcCaller* RpcRequest::getCaller() const {
	return data->caller;
}

void RpcRequest::setResult(const Value& result) {
	setResult(result, undefined);

}

void RpcRequest::setResult(const Value& result, const Value& context) {
	Value resp (object,{
			"id"_=data->id,
			"result"_=result,
			"error"_=nullptr,
			"context"_=context});
	data->setResponse(resp);

}

void RpcRequest::setError(const Value& error) {
	Value resp (object,{
			"id"_=data->id,
			"result"_=nullptr,
			"error"_=error});
	data->setResponse(resp);
}


RpcRequest::RequestData::~RequestData() {
	if (caller) caller->release();
	if (!responseSent) setResponse(undefined);

}

Value RpcRequest::operator [](unsigned int index) const {
	return data->args[index];
}

Value RpcRequest::operator [](const StrViewA name) const {
	return data->context[name];
}

void RpcRequest::setError(unsigned int status, const String& message) {
	setError(Value(object,{"status"_=status,"statusMessage"_=message}));
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
	creq.setError(404,{"Method ot found: ", req.getMethodName() } );
}

void RpcServer::onMalformedRequest(const RpcRequest& req) const {
	RpcRequest creq(req);
	creq.setError(400,{"Malformed request"} );

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

}
