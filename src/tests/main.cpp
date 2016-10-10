/*
 * main.cpp
 *
 *  Created on: 10. 10. 2016
 *      Author: ondra
 */

#include "../immujson/edit.h"
#include "testClass.h"
#include <memory>

using namespace json;


int main(int , char **) {
	TestSimple tst;

	tst.test("Parse.string","testing") >> [](std::ostream &out) {
		out << Value::fromString("\"testing\"").getString();
	};
	tst.test("Parse.stringUnicode",u8"testing\uFFFF") >> [](std::ostream &out) {
		out << Value::fromString("\"testing\\uFFFF\"").getString();
	};
	tst.test("Parse.stringUnicode2",u8"testing\u10D0A") >> [](std::ostream &out) {
		out << Value::fromString("\"testing\\u10D0\\u0041\"").getString();
	};
	tst.test("Parse.stringUtf8",u8"testing-ěščřžýáíé") >> [](std::ostream &out) {
		out << Value::fromString(u8"\"testing-ěščřžýáíé\"").getString();
	};
	tst.test("Parse.stringSpecial","line1\nline2\rline3\fline4\bline5\\line6\"line7/line8") >> [](std::ostream &out) {
		out << Value::fromString("\"line1\\nline2\\rline3\\fline4\\bline5\\\\line6\\\"line7\\/line8\"").getString();
	};
	tst.test("Parse.stringEmpty","true") >> [](std::ostream &out) {
		out << ((Value::fromString("\"\"").getHandle() == Value("").getHandle())?"true":"false");
	};
	tst.test("Parse.number","123") >> [](std::ostream &out) {
		out << Value::fromString("123").getUInt();
	};
	tst.test("Parse.numberIntToDouble","123") >> [](std::ostream &out) {
		out << Value::fromString("123").getNumber();
	};
	tst.test("Parse.numberDoubleToInt","123") >> [](std::ostream &out) {
		out << Value::fromString("123.789").getUInt();
	};
	tst.test("Parse.numberDouble","587.3") >> [](std::ostream &out) {
		out << Value::fromString("587.3").getNumber();
	};
	tst.test("Parse.numberDoubleSmall","0.000257") >> [](std::ostream &out) {
		out << Value::fromString("0.000257").getNumber();
	};
	tst.test("Parse.numberDoubleSmallE","8.12e-21") >> [](std::ostream &out) {
		out << Value::fromString("81.2e-22").getNumber();
	};
	tst.test("Parse.numberDoubleLargeE","-1.024e+203") >> [](std::ostream &out) {
		out << Value::fromString("-1024e200").getNumber();
	};
	tst.test("Parse.numberSigned","-1258767987") >> [](std::ostream &out) {
		out << Value::fromString("-1258767987").getInt();
	};
	tst.test("Parse.numberUnusual","23.0001") >> [](std::ostream &out) {
		out << Value::fromString("+23.000078").getNumber();
	};
	tst.test("Parse.arrayEmpty","true") >> [](std::ostream &out) {
		out << ((Value::fromString("[]").getHandle() == Value(array).getHandle())?"true":"false");
	};
	tst.test("Parse.arraySomeValues","10") >> [](std::ostream &out) {
		Value v = Value::fromString("[1,20,0.30,4.5,32.4987,1.32e-18,-23,\"neco\",true,null]");
		out << v.size();
	};
	tst.test("Parse.arrayInArray","3 3 3 1") >> [](std::ostream &out) {
		Value v = Value::fromString("[1,[2,[3,[4],5],6],7]");
		out << v.size() << " " <<v[1].size() << " " <<v[1][1].size() << " " << v[1][1][1].size();
	};
	tst.test("Parse.objects", "=null aaa=123 bbb=xyz ccc=true neco=12.2578 ") >> [](std::ostream &out) {
		Value v = Value::fromString("{\"aaa\":123,\"bbb\":\"xyz\", \"ccc\":true, \"\":null, \"neco\":12.2578}");
		v.forEach([&](Value v) {
			out << v.getKey() << "=" << v.toDebugString() << " "; return true;
		});
	};
	tst.test("Parse.objectFindValue", "12.2578") >> [](std::ostream &out) {
		Value v = Value::fromString("{\"aaa\":123,\"bbb\":\"xyz\", \"ccc\":true, \"\":null, \"neco\":12.2578}");
		out << v["neco"].toDebugString();
	};
	tst.test("Parse.objectValueNotFound", "<undefined>") >> [](std::ostream &out) {
		Value v = Value::fromString("{\"aaa\":123,\"bbb\":\"xyz\", \"ccc\":true, \"\":null, \"neco\":12.2578}");
		out << v["caa"].toDebugString();
	};
	tst.test("Parse.objectInObject", "2 3 2 1") >> [](std::ostream &out) {
		Value v = Value::fromString("{\"a\":1,\"b\":{\"a\":2,\"b\":{\"a\":3,\"b\":{\"a\":4}},\"c\":6},\"a\":7}");
		out << v.size() << " " << v["b"].size() << " " << v["b"]["b"].size() << " " << v["b"]["b"]["b"].size();
	};
	tst.test("Serialize.objects", "{\"a\":7,\"b\":{\"a\":2,\"b\":{\"a\":3,\"b\":{\"a\":4}},\"c\":6}}") >> [](std::ostream &out) {
		Value v = Value::fromString("{\"a\":1,\"b\":{\"a\":2,\"b\":{\"a\":3,\"b\":{\"a\":4}},\"c\":6},\"a\":7}");
		v.toStream(out);
	};
	tst.test("Object.create", "{\"arte\":true,\"data\":[90,60,90],\"frobla\":12.3,\"kabrt\":123,\"name\":\"Azaxe\"}") >> [](std::ostream &out) {
		Object o;
		o.set("kabrt", 123);
		o.set("frobla", 12.3);
		o.set("arte", true);
		o.set("name", "Azaxe");
		o.set("data", { 90,60, 90 });
		Value v(o);
		v.toStream(out);
	};
	tst.test("Object.create2", "{\"arte\":true,\"data\":[90,60,90],\"frobla\":0.1,\"kabrt\":123,\"name\":\"Azaxe\"}") >> [](std::ostream &out) {
		Value(
			Object
				("kabrt", 123)
				("frobla", 0.1)
				("arte", true)
				("name", "Azaxe")
				("data", { 90,60, 90 })
		).toStream(out);
	};
	tst.test("Object.edit", "{\"age\":19,\"data\":[90,60,90],\"frobla\":12.3,\"kabrt\":289,\"name\":\"Azaxe\"}") >> [](std::ostream &out) {
		Value v = Value::fromString("{\"arte\":true,\"data\":[90,60,90],\"frobla\":12.3,\"kabrt\":123,\"name\":\"Azaxe\"}");		
		Value(
			Object(v)
				("kabrt", 289)
				("arte", undefined)
				("age",19)
		).toStream(out);
	};
	tst.test("Object.addSubobject", "{\"arte\":true,\"data\":[90,60,90],\"frobla\":12.3,\"kabrt\":123,\"name\":\"Azaxe\",\"sub\":{\"kiki\":-32.431,\"kuku\":false}}") >> [](std::ostream &out) {
		Value v = Value::fromString("{\"arte\":true,\"data\":[90,60,90],\"frobla\":12.3,\"kabrt\":123,\"name\":\"Azaxe\"}");
		Object o(v);
		o.object("sub")
			("kiki", -32.431)
			("kuku", false);
		v = o;
		v.toStream(out);
	};
	tst.test("Object.addSubarray", "{\"arte\":false,\"data\":[90,60,90],\"frobla\":12.3,\"kabrt\":123,\"name\":\"Azaxe\",\"sub\":[\"kiki\",\"kuku\",\"mio\",\"mao\",69,[\"bing\",\"bang\"]]}") >> [](std::ostream &out) {
		Value v = Value::fromString("{\"arte\":true,\"data\":[90,60,90],\"frobla\":12.3,\"kabrt\":123,\"name\":\"Azaxe\"}");
		Object o(v);
		o.array("sub")
			.add("kiki")
			.add("kuku")
			.addSet({ "mio","mao",69 })
			.add({ "bing","bang" });
		o("arte", false);
		v = o;
		v.toStream(out);
	};

	return tst.getResult()?1:0;
}
