/*
 * rpc_tests.cpp
 *
 *  Created on: 2. 3. 2017
 *      Author: ondra
 */
#include <iostream>
#include "testClass.h"
#include "../imtjson/rpc.h"


using namespace json;

class TestClass: public RefCntObj {
public:
	void method(RpcRequest x) {

	}
};

void runRpcTests(TestSimple &tst) {

	tst.test("RpcServer.listMethods","{\"error\":null,\"id\":1,\"result\":[\"Server.help\",\"Server.listMethods\",\"test\"]}") >> [](std::ostream &out) {
		RefCntPtr<TestClass> x = new TestClass;
		RpcServer srv;
		srv.add_listMethods();
		srv.add_multicall();
		srv.add_help(object);
		srv.add<RefCntPtr<TestClass>, TestClass>("test",x,&TestClass::method);
		RpcRequest req = RpcRequest::create(Value(object, {
				key/"method"="Server.listMethods",
				key/"params"=Value(array),
				key/"id"=1}),[&](Value res) {
			res.toStream(out);
		});
		srv(req);
	};
}





