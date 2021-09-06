/*
 * rpc_tests.cpp
 *
 *  Created on: 2. 3. 2017
 *      Author: ondra
 */
#include <iostream>

#include "../imtjson/object.h"
#include "testClass.h"
#include "../imtjson/rpc.h"


using namespace json;

class TestClass: public RefCntObj {
public:
	void method(RpcRequest x) {

	}
};

void runRpcTests(TestSimple &tst) {

	tst.test("RpcServer.listMethods","{\"error\":null,\"id\":1,\"result\":[\"Server.help\",\"Server.listMethods\",\"Server.multicall\",\"test\"]}") >> [](std::ostream &out) {
		RefCntPtr<TestClass> x = new TestClass;
		RpcServer srv;
		srv.add_listMethods();
		srv.add_multicall();
		srv.add_help(object);
		srv.add("test",x,&TestClass::method);
		RpcRequest req = RpcRequest::create(Value(object, {
				key/"method"="Server.listMethods",
				key/"params"=Value(array),
				key/"id"=1}),[&](Value res) {
			res.toStream(out);
			return true;
		});
		srv(req);
	};


	tst.test("RpcServer.ping","{\"error\":null,\"id\":10,\"result\":[1,2,3,true,{\"aaa\":4}]}") >> [](std::ostream &out) {
		RpcServer srv;
		srv.add_ping();
		RpcRequest req = RpcRequest::create(RpcRequest::ParseRequest("Server.ping",{1,2,3,true,Object({{"aaa",4}}),nullptr},10),[&](Value res) {
			res.toStream(out);
			return true;
		});
		srv(req);
	};
	tst.test("RpcServer.multicall1","{\"error\":null,\"id\":0,\"result\":{\"errors\":[],\"results\":[[1,2,3],[4,5,6],[7,8,9]]}}") >> [](std::ostream &out) {
		RpcServer srv;
		srv.add_ping();
		srv.add_multicall();
		RpcRequest req = RpcRequest::create(RpcRequest::ParseRequest("Server.multicall",{"Server.ping",{1,2,3},{4,5,6},{7,8,9}}),[&](Value res) {
			res.toStream(out);
			return true;
		});
		srv(req);
	};
	tst.test("RpcServer.multicall2","{\"error\":null,\"id\":0,\"result\":{\"errors\":[],\"results\":[[1,2,3],[\"Server.listMethods\",\"Server.multicall\",\"Server.ping\"]]}}") >> [](std::ostream &out) {
		RpcServer srv;
		srv.add_listMethods();
		srv.add_ping();
		srv.add_multicall();
		RpcRequest req = RpcRequest::create(RpcRequest::ParseRequest("Server.multicall",{{"Server.ping",{1,2,3}},{"Server.listMethods",json::array}}),[&](Value res) {
			res.toStream(out);
			return true;
		});
		srv(req);
	};
	tst.test("RpcServer.multicall_async","{\"error\":null,\"id\":0,\"result\":\{\"errors\":[],\"results\":[[1,2,3],30,[4,5,6]]}}") >> [](std::ostream &out) {
		RpcServer srv;
		//will contain asynchronous context
		std::unique_ptr<RpcRequest> asyncreq;
		//function which doesn't set result, just stores asynchronous context (emulating the asynchornous processing)
		auto asyncFn=[&](RpcRequest r) {
			asyncreq = std::unique_ptr<RpcRequest>(new RpcRequest(r));
		};
		//register the function
		srv.add("test", asyncFn);
		//register ping
		srv.add_ping();
		//register multicall
		srv.add_multicall();
		//create request
		RpcRequest req = RpcRequest::create(RpcRequest::ParseRequest("Server.multicall",{{"Server.ping",{1,2,3}},{"test",{10,20}},{"Server.ping",{4,5,6}}}),[&](Value res) {
			res.toStream(out);
			return true;
		});
		//call request
		srv(req);
		//first ping is done, now processing is postponed, because the method test running asynchronously


		//async part (emulation) - execute method "test"
		double sum = 0;
		for (Value x : asyncreq->getArgs() ) {
			sum = sum +x.getNumber();
		}
		//set the result fo the test
		asyncreq->setResult(sum);
		//now, result should be rendered
	};
	tst.test("RpcServer.multicall_undef","{\"error\":null,\"id\":0,\"result\":{\"errors\":[{\"code\":-32099,\"message\":\"The method did not produce a result\"}],\"results\":[[1,2,3],null,[4,5,6]]}}") >> [](std::ostream &out) {
		RpcServer srv;
		auto asyncFn=[&](RpcRequest) {/* empty - will not produce result */};
		srv.add("test", asyncFn);
		srv.add_ping();
		srv.add_multicall();
		RpcRequest req = RpcRequest::create({"Server.multicall",{{"Server.ping",{1,2,3}},{"test",{10,20}},{"Server.ping",{4,5,6}}}},[&](Value res) {
			res.toStream(out);
			return true;
		});
		srv(req);
	};
}





