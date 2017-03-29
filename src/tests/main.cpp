/*
 * main.cpp
 *
 *  Created on: 10. 10. 2016
 *      Author: ondra
 */

#ifdef _WIN32
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <memory>
#include <fstream>
#include "../imtjson/json.h"
#include "../imtjson/compress.tcc"
#include "../imtjson/basicValues.h"
#include "../imtjson/key.h"
#include "../imtjson/comments.h"
#include "../imtjson/binjson.tcc"
#include "../imtjson/streams.h"
#include "../imtjson/valueref.h"
#include "testClass.h"

using namespace json;

void runValidatorTests(TestSimple &tst);
void runRpcTests(TestSimple &tst);


bool compressTest(std::string file) {
	Value input,compressed;
	{
		std::ifstream infile(file, std::ifstream::binary);
		input = Value::fromStream(infile);
		//re-parse because float numbers can be rounded (maxPrecisionDigits in effect)
		input = Value::fromString(input.stringify());
	}
	{
		std::ofstream outfile(file + ".cmp", std::ifstream::binary);
		input.serialize(json::emitUtf8, compress(toStream(outfile)));
	}
	{
		std::ifstream infile(file + ".cmp", std::ifstream::binary);
		compressed = Value::parse(decompress(fromStream(infile)));
	}
	{
		std::ofstream outfile(file + ".decmp", std::ifstream::binary);
		compressed.toStream(json::emitUtf8, outfile);
	}
/*	{
		std::ofstream outfile(file + ".orig", std::ifstream::binary);
		input.toStream(json::emitUtf8, outfile);
	}*/
	return input == compressed;
}


bool binTest(std::string file)  {
	Value input,binloaded;
	{
		std::ifstream infile(file, std::ifstream::binary);
		input = Value::fromStream(infile);
	}
	{
		std::ofstream outfile(file + ".bin", std::ifstream::binary);
		input.serializeBinary(toStream(outfile));
	}
	{
		std::ifstream infile(file + ".bin", std::ifstream::binary);
		binloaded = Value::parseBinary(fromStream(infile));
	}
	return input == binloaded;
}


void subobject(Object &&obj) {
	obj("outbreak", 19)
		("outgrow", 21);
}


int testMain() {
	TestSimple tst;

	//Normalize test across platforms
	json::maxPrecisionDigits = 4;

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
	tst.test("Parse.numberDouble2","50.051") >> [](std::ostream &out) {
		out << Value::fromString("50.051").getNumber();
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
	tst.test("Parse.numberLong","Parse error: 'Too long number' at <root>") >> [](std::ostream &out) {
		int counter = 0;
		try {
			out << Value::parse([&](){
				if (counter < 1000) return (((++counter)*7) % 10) + '0';
				return 0;
			}).getNumber();
		} catch (ParseError &e) {
			out << e.what();
		}
	};
	tst.test("Parse.numberLong2","7.41853e+299") >> [](std::ostream &out) {
		int counter = 0;
		try {
			out << Value::parse([&](){
				if (counter < 300) return (((++counter)*7) % 10) + '0';
				return 0;
			}).getNumber();
		} catch (ParseError &e) {
			out << e.what();
		}
	};
	
		if (sizeof(uintptr_t) == 8) {
			tst.test("Parse.numberMaxUINT", "18446744073709551615") >> [](std::ostream &out) {
				out << Value::fromString("18446744073709551615").getUInt();
			};
		}
		else {
			tst.test("Parse.numberMaxUINT", "4294967295") >> [](std::ostream &out) {
				out << Value::fromString("4294967295").getUInt();
			};

		}
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
			out << v.getKey() << "=" << v.toString() << " "; return true;
		});
	};
	tst.test("Parse.objectFindValue", "12.2578") >> [](std::ostream &out) {
		Value v = Value::fromString("{\"aaa\":123,\"bbb\":\"xyz\", \"ccc\":true, \"\":null, \"neco\":12.2578}");
		out << v["neco"].toString();
	};
	tst.test("Parse.objectValueNotFound", "<undefined>") >> [](std::ostream &out) {
		Value v = Value::fromString("{\"aaa\":123,\"bbb\":\"xyz\", \"ccc\":true, \"\":null, \"neco\":12.2578}");
		out << v["caa"].toString();
	};
	tst.test("Parse.objectInObject", "2 3 2 1") >> [](std::ostream &out) {
		Value v = Value::fromString("{\"a\":1,\"b\":{\"a\":2,\"b\":{\"a\":3,\"b\":{\"a\":4}},\"c\":6},\"a\":7}");
		out << v.size() << " " << v["b"].size() << " " << v["b"]["b"].size() << " " << v["b"]["b"]["b"].size();
	};
	tst.test("Serialize.number", "50.0075") >> [](std::ostream &out) {
		Value v = 50.0075;
		v.toStream(out);
	};
	tst.test("Serialize.objects", "{\"a\":7,\"b\":{\"a\":2,\"b\":{\"a\":3,\"b\":{\"a\":4}},\"c\":6}}") >> [](std::ostream &out) {
		Value v = Value::fromString("{\"a\":1,\"b\":{\"a\":2,\"b\":{\"a\":3,\"b\":{\"a\":4}},\"c\":6},\"a\":7}");
		v.toStream(out);
	};
	tst.test("Serialize.binary","[\"\",\"Zg==\",\"Zm8=\",\"Zm9v\",\"Zm9vYg==\",\"Zm9vYmE=\",\"Zm9vYmFy\"]") >> [](std::ostream &out) {
		//Tests whether binary values are properly encoded to base64
		StrViewA v[] = {"","f","fo","foo","foob","fooba","foobar"};
		Array res;
		for (auto x : v) {
			res.push_back(Value(BinaryView(x),json::base64));
		}
		Value(res).toStream(out);
	};
	tst.test("Serialize.valueHash", "13532798998175024480") >> [](std::ostream &out) {
		Value v = Value::fromString("{\"a\":1,\"b\":{\"a\":2,\"b\":{\"a\":3,\"b\":{\"a\":4}},\"c\":6},\"a\":7}");
		std::hash<Value> hv;
		out << hv(v);
	};
	tst.test("Parse.binary",",f,fo,foo,foob,fooba,foobar") >> [](std::ostream &out) {
		//Tests whether base64 values are properly decoded to the Binary type
		Value v = Value::fromString("[\"\",\"Zg==\",\"Zm8=\",\"Zm9v\",\"Zm9vYg==\",\"Zm9vYmE=\",\"Zm9vYmFy\"]");
		Binary a( v[0].getBinary(base64));
		Binary b( v[1].getBinary(base64));
		Binary c( v[2].getBinary(base64));
		Binary d( v[3].getBinary(base64));
		Binary e( v[4].getBinary(base64));
		Binary f( v[5].getBinary(base64));
		Binary g( v[6].getBinary(base64));

		out << StrViewA(a) << "," << StrViewA(b)
				<< "," << StrViewA(c) << "," << StrViewA(d)
				<< "," << StrViewA(e) << "," << StrViewA(f)
				<< "," << StrViewA(g);
	};
	tst.test("Parse.base64.keepEncoding","\"Zm9vYmFy\"") >> [](std::ostream &out) {
		//Tests whether the encoding is remembered after decode and properly used to encode data back.
		Value v = Value::fromString("\"Zm9vYmFy\"");
		Binary a( v.getBinary(base64));
		Value w (a);
		w.toStream(out);
	};
	tst.test("StringValue.concat", "hello world 1") >> [](std::ostream &out) {
		Value v(string, { "hello"," world ", 1 });
		out << v.getString();
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
	tst.test("Object.create3", "{\"arte\":true,\"data\":[90,60,90],\"frobla\":0.1,\"kabrt\":123,\"name\":\"Azaxe\"}") >> [](std::ostream &out) {
		//using default initializer list, but all values have key assigned - creates object
		Value(object,{"kabrt"_ = 123,
			"frobla"_= 0.1,
			"arte"_= true,
			"name"_= "Azaxe",
			"data"_= {90,60,90} }).toStream(out);
	};
	tst.test("Object.create4", "{\"arte\":true,\"data\":[90,60,90],\"frobla\":0.1,\"kabrt\":123,\"name\":\"Azaxe\"}") >> [](std::ostream &out) {
		//using _ suffix can have issues. If you disable it, you can use key/ as prefix to create key-value pair
		Value(object, { key/"kabrt" = 123,
			key/"frobla" = 0.1,
			key/"arte" = true,
			key/"name" = "Azaxe",
			key/"data" = { 90,60,90 } }).toStream(out);
	};
	tst.test("Object.create5", "{\"\":123,\"arte\":true,\"data\":[90,60,90],\"frobla\":0.1,\"name\":\"Azaxe\"}") >> [](std::ostream &out) {
		//without type, constructor will create array, because of empty key appear. You can enforce object by specifying type as argument
		Value(object, { ""_ = 123,
			"frobla"_ = 0.1,
			"arte"_ = true,
			"name"_ = "Azaxe",
			"data"_ = { 90,60,90 } }).toStream(out);
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

	tst.test("Object.edit.move", "{\"age\":19,\"data\":[90,60,90],\"frobla\":12.3,\"kabrt\":289,\"name\":\"Azaxe\"}") >> [](std::ostream &out) {
		Value v = Value::fromString("{\"arte\":true,\"data\":[90,60,90],\"frobla\":12.3,\"kabrt\":123,\"name\":\"Azaxe\"}");
		Object o1(v);
			o1("kabrt", 289)
				("arte", undefined)
				("age",19);
		Object o2(std::move(o1));
		o1("aaa",10);
		o1.optimize();
		Value(o2).toStream(out);
	};

tst.test("Object.enumItems", "age:19,data:[90,60,90],frobla:12.3,kabrt:289,name:Azaxe,") >> [](std::ostream &out) {
		Value v = Value::fromString("{\"arte\":true,\"data\":[90,60,90],\"frobla\":12.3,\"kabrt\":123,\"name\":\"Azaxe\"}");
		Object obj(v);
		obj("kabrt", 289)
			("arte", undefined)
			("age",19);
		for(auto&& item: obj) {
			out << item.getKey()<<":"<<item.toString() << ",";
		}
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
	tst.test("Object.addSubobject2", "{\"arte\":true,\"data\":[90,60,90],\"frobla\":12.3,\"kabrt\":123,\"name\":\"Azaxe\",\"sub\":{\"aaa\":true,\"kiki\":-32.431,\"kuku\":false}}") >> [](std::ostream &out) {
		Value v = Value::fromString("{\"arte\":true,\"data\":[90,60,90],\"frobla\":12.3,\"kabrt\":123,\"name\":\"Azaxe\",\"sub\":{\"kiki\":3,\"aaa\":true}}");
		Object o(v);
		o.object("sub")
			("kiki", -32.431)
			("kuku", false);
		v = o;
		v.toStream(out);
	};
	tst.test("Object.addSubobjectFunction", "{\"arte\":true,\"data\":[90,60,90],\"frobla\":12.3,\"kabrt\":123,\"name\":\"Azaxe\",\"sub\":{\"outbreak\":19,\"outgrow\":21}}") >> [](std::ostream &out) {
		Value v = Value::fromString("{\"arte\":true,\"data\":[90,60,90],\"frobla\":12.3,\"kabrt\":123,\"name\":\"Azaxe\"}");
		Object o(v);
		subobject(o.object("sub"));
		v = o;
		v.toStream(out);
	};
	tst.test("Object.addSubobjectFunction2", "{\"arte\":true,\"data\":[90,60,90],\"frobla\":12.3,\"kabrt\":123,\"name\":\"Azaxe\",\"sub\":{\"sub2\":{\"outbreak\":19,\"outgrow\":21}}}") >> [](std::ostream &out) {
		Value v = Value::fromString("{\"arte\":true,\"data\":[90,60,90],\"frobla\":12.3,\"kabrt\":123,\"name\":\"Azaxe\"}");
		Object o(v);
		subobject(o.object("sub").object("sub2"));
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
	tst.test("Object.huge","hit{}") >> [](std::ostream &out) {
		Object o;
		o.set("test",o);
		for (int i = 0; i < 1000; i++) {
			String k = Value(rand()).toString();
			o.set(k,i);
		}
		o.set("5000","hit");
		out << o["5000"].getString() << o["test"].toString();

	};
	tst.test("Object.huge.search-delete","hit{\"aaa\":10}10<undefined><undefined><undefined><undefined><undefined><undefined>") >> [](std::ostream &out) {
		Value base = Object("aaa",10);
		Object o(base);
		o.set("test",o);
		for (int i = 0; i < 300; i++) {
			String k = Value(rand()).toString();
			o.set(k,i);
		}
		o.set("120","hit");
		out << o["120"].toString();
		out << o["test"].toString();
		out << o["aaa"].toString();
		o.unset("aaa");
		o.unset("120");
		o.unset("test");
		out << o["120"].toString();
		out << o["test"].toString();
		out << o["aaa"].toString();
		Value der = o;
		out << der["120"].toString();
		out << der["test"].toString();
		out << der["aaa"].toString();
	};

	tst.test("Object.diff","{\"a\":\"undefined\",\"b\":5,\"d\":4}") >> [](std::ostream &out) {
		Value v1 = Value::fromString("{\"a\":1,\"b\":2,\"c\":3}");
		Value v2 = Value::fromString("{\"d\":4,\"b\":5,\"c\":3}");
		Object o;
		o.createDiff(v1,v2);
		Value v = o.commitAsDiff();
		out << v.toString();
	};
	tst.test("Object.applyNonDiff","{\"a\":4,\"b\":2,\"c\":{\"x\":42,\"y\":56,\"z\":25}}") >> [](std::ostream &out) {
		Value v1 = Value::fromString("{\"a\":1,\"b\":2,\"c\":{\"x\":42,\"y\":56,\"z\":78}}");
		Value v2 = Value::fromString("{\"a\":4,\"c\":{\"z\":25}}");
		Value r = Object::applyDiff(v1,v2,Object::recursiveMerge);
		r.toStream(out);
	};

	tst.test("Array.create","[\"hi\",\"hola\",1,2,3,5,8,13,21,7.5579e+27]") >> [](std::ostream &out){
		Array a;
		a.add("hi").add("hola");
		a.addSet({1,2,3,5,8,13,21});
		a.add(7557941563989796531369787923.2568971236);
		Value v = a;
		v.toStream(out);
	};
	tst.test("Array.editInsert","[\"hi\",\"hola\",{\"inserted\":\"here\"},1,2,3,5,8,13,21,7.5579e+27]") >> [](std::ostream &out){
		Value v = Value::fromString("[\"hi\",\"hola\",1,2,3,5,8,13,21,7.5579e+27]");
		Array a(v);
		a.insert(2,Object("inserted","here"));
		v = a;
		v.toStream(out);
	};
	tst.test("Array.enumItems","hi,hola,{\"inserted\":\"here\"},1,2,3,5,8,13,21,7.5579e+27,") >> [](std::ostream &out){
		Value v = Value::fromString("[\"hi\",\"hola\",1,2,3,5,8,13,21,7.5579e+27]");
		Array a(v);
		a.insert(2,Object("inserted","here"));
		for(auto &&item: a) {
			out << item.toString() << ",";
		}
	};
	tst.test("Array.editDelete","[\"hi\",\"hola\",5,8,13,21,7.5579e+27]") >> [](std::ostream &out){
		Value v = Value::fromString("[\"hi\",\"hola\",1,2,3,5,8,13,21,7.5579e+27]");
		Array a(v);
		a.eraseSet(2,3);
		v = a;
		v.toStream(out);
	};
	tst.test("Array.editTrunc","[\"hi\",\"hola\",1]") >> [](std::ostream &out){
		Value v = Value::fromString("[\"hi\",\"hola\",1,2,3,5,8,13,21,7.5579e+27]");
		Array a(v);
		a.trunc(3);
		v = a;
		v.toStream(out);
	};
	tst.test("Array.editInsertTrunc","[\"hi\",true,\"hola\"]") >> [](std::ostream &out){
		Value v = Value::fromString("[\"hi\",\"hola\",1,2,3,5,8,13,21,7.5579e+27]");
		Array a(v);
		a.insert(1,true);
		a.trunc(3);
		v = a;
		v.toStream(out);
	};
	tst.test("Array.editTruncInsert","[\"hi\",true,\"hola\",1]") >> [](std::ostream &out){
		Value v = Value::fromString("[\"hi\",\"hola\",1,2,3,5,8,13,21,7.5579e+27]");
		Array a(v);
		a.trunc(3);
		a.insert(1,true);
		v = a;
		v.toStream(out);
	};
	tst.test("Sharing","{\"shared1\":[10,20,30],\"shared2\":{\"a\":1,\"n\":[10,20,30],\"z\":5},\"shared3\":{\"k\":[10,20,30],\"l\":{\"a\":1,\"n\":[10,20,30],\"z\":5}}}") >> [](std::ostream &out){
		Value v1 = {10,20,30};
		Value v2(Object("a",1)("z",5)("n",v1));
		Value v3(Object("k",v1)("l",v2));
		Value v4(Object("shared1",v1)("shared2",v2)("shared3",v3));
		v4.toStream(out);

	};
	tst.test("StackProtection","Attempt to work with already destroyed object") >> [](std::ostream &out){
		try {
			Object o;
			{
				auto x = o.object("aa").object("bb").object("cc"); //forbidden usage
				x("a",10);
			}
			Value(o).toStream(out);
		} catch (std::exception &e) {
			out << e.what();
		}
	};
	tst.test("refcnt","destroyed") >> [](std::ostream &out) {
		class MyAbstractValue: public NumberValue {
		public:
			MyAbstractValue(std::ostream &out):NumberValue(0),out(out) {}
			~MyAbstractValue() {out << "destroyed";}

			std::ostream &out;

		};
		Value v = new MyAbstractValue(out);
		Value v2 = v;
		Value v3 (Object("x",v));
		Value v4 = {v3,v2};
		v.stringify();
	};


	tst.test("parse.corruptedFile.string","Parse error: 'Unexpected end of file' at <root>") >> [](std::ostream &out) {
		try {
			Value v = Value::fromString("\"qweqweq");
		} catch (std::exception &e) {
			out << e.what();
		}
	};
	tst.test("parse.corruptedFile.token","Parse error: 'Unknown keyword' at <root>") >> [](std::ostream &out) {
		try {
			Value v = Value::fromString("tru");
		} catch (std::exception &e) {
			out << e.what();
		}
	};
	tst.test("parse.corruptedFile.array","Parse error: 'Expected ',' or ']'' at <root>/[4]") >> [](std::ostream &out) {
		try {
			Value v = Value::fromString("[10,20,[30,40],30");
		} catch (std::exception &e) {
			out << e.what();
		}
	};
	tst.test("parse.corruptedFile.object","Parse error: 'Expected ':'' at <root>/xyz") >> [](std::ostream &out) {
		try {
			Value v = Value::fromString("{\"abc\":123,\"xyz\"");
		} catch (std::exception &e) {
			out << e.what();
		}
	};

	tst.test("compare.strings","falsefalsetruetrue") >> [](std::ostream &out) {
		Value a("ahoj");
		Value b("nazdar");
		Value c("ahoj");

		Value r1(a == b);
		Value r2(b == c);
		Value r3(a == c);
		Value r4(a == a);

		out << r1 << r2 << r3 << r4;

	};
	tst.test("compare.numbers","falsefalsetruetrue") >> [](std::ostream &out) {
		Value a(10);
		Value b(25);
		Value c(10.0);

		Value r1(a == b);
		Value r2(b == c);
		Value r3(a == c);
		Value r4(a == a);

		out << r1 << r2 << r3 << r4;

	};
	tst.test("compare.arrays","falsefalsefalsetruetrue") >> [](std::ostream &out) {
		Value a({1,2,3,"xxx"});
		Value b({1,2,3,"aaa"});
		Value c({1,2,3});
		Value d({1,2,3,"xxx"});

		Value r1(a == b);
		Value r2(b == c);
		Value r3(a == c);
		Value r4(a == a);
		Value r5(d == a);

		out << r1 << r2 << r3 << r4 << r5;

	};

	tst.test("compare.objects","falsefalsefalsetruetruetruetrue") >> [](std::ostream &out) {
		Value a(Object("a",1)("b",20));
		Value b(Object("a",1)("b",20)("c",32));
		Value c(Object("x",1)("y",20));
		Value d(Object("a",1)("b",20));

		Value r1(a == b);
		Value r2(b == c);
		Value r3(a == c);
		Value r4(a == a);
		Value r5(d == a);
		Value r6(Value(1) == a["a"]);
		Value r7(c["x"] == a["a"]);

		out << r1 << r2 << r3 << r4 << r5 << r6 << r7;

	};
	tst.test("compare.booleans","falsetruetruetrue") >> [](std::ostream &out) {
		Value a(true);
		Value b(false);
		Value c(false);

		Value r1(a == b);
		Value r2(a == a);
		Value r3(b == b);
		Value r4(c == b);

		out << r1 << r2 << r3 << r4;

	};
	tst.test("compare.null","truetruetrue") >> [](std::ostream &out) {
		Value a(null);
		Value b(null);

		Value r1(a == b);
		Value r2(a == a);
		Value r3(b == b);

		out << r1 << r2 << r3;
	};

	tst.test("compare.undefined","truetruetrue") >> [](std::ostream &out) {
		Value a;
		Value b;

		Value r1(a == b);
		Value r2(a == a);
		Value r3(b == b);

		out << r1 << r2 << r3;
	};

	tst.test("compare.various","falsetruefalsefalsefalsefalsefalsefalsefalsefalsefalsefalsefalsefalsefalse") >> [](std::ostream &out) {
		Value a(1);
		Value b("1");
		Value c(1.0);
		Value d(true);
		Value e((Array()));
		Value f((Object()));

		Value r1(a == b);
		Value r2(a == c);
		Value r3(a == d);
		Value r4(a == e);
		Value r5(a == f);
		Value r6(b == c);
		Value r7(b == d);
		Value r8(b == e);
		Value r9(b == f);
		Value r10(c == d);
		Value r11(c == e);
		Value r12(c == f);
		Value r13(d == e);
		Value r14(d == f);
		Value r15(e == f);

		out << r1 << r2 << r3 << r4 << r5 << r6 << r7 << r8 << r9 << r10 << r11 << r12 << r13 << r14 << r15;
	};
	tst.test("conversions.bools","truefalsefalsefalsetruetruetruefalsetruefalsefalsefalsetruefalse") >> [](std::ostream &out) {
		out << Value(Value(true).getBool())
			<< Value(Value(false).getBool())
			<< Value(Value("").getBool())
			<< Value(Value(0).getBool())
			<< Value(Value("x").getBool())
			<< Value(Value(1).getBool())
			<< Value(Value(Object("a",1)).getBool())
			<< Value(Value(Object()).getBool())
			<< Value(Value({1,2}).getBool())
			<< Value(Value({}).getBool())
			<< Value(Value(null).getBool())
			<< Value(Value().getBool())
			<< Value(Value(1.0).getBool())
			<< Value(Value(0.0).getBool());

	};
	tst.test("conversions.numbers","123.45,123,123,-123.45,-123,65413") >> [](std::ostream &out) {
		Value t1("123.45");
		Value t2("-123.45");
		out << t1.getNumber() << ","
			<< t1.getInt() << ","
			<< t1.getUInt() << ","
			<< t2.getNumber() << ","
			<< t2.getInt() << ","
			<< (t2.getUInt() & 0xFFFF);
	};
	tst.test("enumKeys","aaa,karel,lkus,macho,zucha,") >> [](std::ostream &out) {
		Value v(
			Object
				("aaa",10)
				("karel",20)
				("zucha",true)
				("macho",21)
				("lkus",11)
		);
		v.forEach([&out](Value v) {
			out << v.getKey() << ",";
			return true;
		});
	};
	tst.test("enumKeys.iterator","aaa,karel,lkus,macho,zucha,") >> [](std::ostream &out) {
		Value v(
			Object
				("aaa",10)
				("karel",20)
				("zucha",true)
				("macho",21)
				("lkus",11)
		);
		for(auto &&item:v) {
			out << item.getKey() << ",";
		}
	};

	tst.test("Path.access1","2,\"super\"") >> [](std::ostream &out) {
		Value v1(Object
				("abc",1)
				("bcd",Object
						("z",Object
								("x",{12,23,Object("u",2)})
								("krk",true)))
				("blb","trp"));

		Value v2(Object
				("xyz","kure")
				("bcd",Object
						("z",Object
								("x",{52,23,Object("u","super")})
								("krk",true))
						("amine","slepice"))
				("bkabla",{1,2,3}));

		PPath path = (Path::root/"bcd"/"z"/"x"/2/"u").copy();
		Value r1 = v1[path];
		Value r2 = v2[path];
		out << r1 << "," << r2;

	};

	tst.test("Path.invalidAccess","Attempt to work with already destroyed object") >> [](std::ostream &out) {
		try {
			Path p = Path::root /"aaa"/"bbb";
			Value v;
			out << v[p];
		} catch (std::exception &e) {
			out << e.what();
		}
	};
	tst.test("Path.toValue","[\"bcd\",\"z\",\"x\",2,\"u\"]") >> [](std::ostream &out) {
		Value v = (Path::root/"bcd"/"z"/"x"/2/"u").toValue();
		out << v;
	};
	tst.test("Path.fromValue","0") >> [](std::ostream &out) {
		Value v = Value::fromString("[\"bcd\",\"z\",\"x\",2,\"u\"]");
		PPath p = PPath::fromValue(v);
		out << p.compare(Path::root/"bcd"/"z"/"x"/2/"u");
	};
	tst.test("String.substr (offset)","world!") >> [](std::ostream &out) {
		String s("Hello world!");
		out << s.substr(6) << s.substr(24);
	};
	tst.test("String.substr (offset, limit)","llo wo!") >> [](std::ostream &out) {
		String s("Hello world!");
		out << s.substr(2,6) << s.substr(11,2000) << s.substr(123,321);
	};
	tst.test("String.left","Hello") >> [](std::ostream &out) {
		String s("Hello world!");
		out << s.left(5) << s.left(0);
	};
	tst.test("String.right","ld!") >> [](std::ostream &out) {
		String s("Hello world!");
		out << s.right(3) << s.right(0);
	};
	tst.test("String.right2","Hello world!") >> [](std::ostream &out) {
		String s("Hello world!");
		out << s.right(300);
	};
	tst.test("String.length","12") >> [](std::ostream &out) {
		String s("Hello world!");
		out << s.length();
	};
	tst.test("String.charAt","lw") >> [](std::ostream &out) {
		String s("Hello world!");
		out << s[2] << s[6];
	};
	tst.test("String.concat","Hello world!") >> [](std::ostream &out) {
		String s("Hello ");
		String t("world!");
		out << s+t;
	};
	tst.test("String.insert","Hello big world!") >> [](std::ostream &out) {
		String s("Hello world!");
		out << s.insert(6,"big ");
	};
	tst.test("String.replace","Hello whole planet!") >> [](std::ostream &out) {
		String s("Hello world!");
		out << s.replace(6,5, "whole planet");
	};
	tst.test("String.concat2","Hello world and poeple!") >> [](std::ostream &out) {
		String s = {"Hello"," ","world"," ","and"," ","poeple!"};
		out << s;
	};
	tst.test("String.split","[\"one\",\"two\",\"three\"]") >> [](std::ostream &out) {
		String s("one::two::three");
		Value v = s.split("::");
		v.toStream(out);
	};
	tst.test("String.split.limit","[\"one\",\"two::three\"]") >> [](std::ostream &out) {
		String s("one::two::three");
		Value v = s.split("::",1);
		v.toStream(out);
	};
	tst.test("String.append","Hello world!") >> [](std::ostream &out) {
		String s("Hello ");
		s+="world!";
		out << s;
	};
	tst.test("String.indexOf","6") >> [](std::ostream &out) {
		String s("Hello world!");
		out << s.indexOf("w");
	};
	tst.test("String.lastIndexOf","36") >> [](std::ostream &out) {
		String s("Hello planet earth, you are a great planet.");
		out << s.lastIndexOf("planet");
	};
	tst.test("Operation.object_map","{\"aaa\":\"aaa10\",\"bbb\":\"bbbfoo\",\"ccc\":\"ccctrue\"}") >> [](std::ostream &out) {
		Value v(
				Object
				("aaa",10)
				("bbb","foo")
				("ccc",true)
			);
		Value w = v.map([](const Value &v){
			return String(v.getKey())+v.toString();
		});
		w.toStream(out);
	};

	tst.test("Operation.array_map","[166,327,120,1799,580]") >> [](std::ostream &out) {
		Value v = Value({5,12,3,76,23}).map([](const Value &v){
			return 23*v.getNumber()+51;
		});
		v.toStream(out);
	};

	tst.test("Operation.reduce_values","119") >> [](std::ostream &out) {
		Value v = Value({5,12,3,76,23}).reduce([](const Value &total, const Value &v){
			return total.getNumber()+v.getNumber();
		},0);
		v.toStream(out);
	};

	tst.test("Operation.sort","[-33,3,8,11,11,21,43,87,90,97]") >> [](std::ostream &out) {
		Value v = {21,87,11,-33,43,90,11,8,3,97};
		Value w = v.sort([](const Value &a, const Value &b){
			return a.getNumber()-b.getNumber();
		});
		w.toStream(out);
	};
	tst.test("Operation.find","[11,\"ccc\"][-33,\"ddd\"][90,\"fff\"]<undefined>") >> [](std::ostream &out) {
		Value v = {{21,"aaa"},{87,"bbb"},{11,"ccc"},{-33,"ddd"},{43,"eee"},{90,"fff"}};
		auto orderFn = [](const Value &a, const Value &b){
			return a[0].getNumber()-b[0].getNumber();
		};
		Value w = v.sort(orderFn);
		Value found1 = w.find(orderFn,Value(array,{11}));
		Value found2 = w.find(orderFn,Value(array,{-33}));
		Value found3 = w.find(orderFn,Value(array,{90}));
		Value found4 = w.find(orderFn,Value(array,{55}));
		out << found1.toString() << found2.toString() << found3.toString() << found4.toString();
	};
	tst.test("Operation.merge","{\"added\":[10,17,55,99],\"removed\":[-33,8,11,21]}") >> [](std::ostream &out) {
		Value oldSet = {21,87,11,-33,43,90,11,8,3,97};
		Value newSet = {10,55,17,90,11,99,3,97,43,87};
		Array added;
		Array removed;
		auto cmp = [](const Value &a, const Value &b){
			return a.getNumber()-b.getNumber();
		};

		oldSet.sort(cmp).merge(newSet.sort(cmp),
				[&added,&removed,&cmp](const Value &left, const Value &right) -> decltype(cmp(undefined,undefined)) {
			if (!left.defined()) {
				added.add(right);return 0;
			} else if (!right.defined()) {
				removed.add(left);return 0;
			} else {
				auto c = cmp(left,right);
				if (c > 0)
					added.add(right);
				else if (c < 0)
					removed.add(left);
				return c;
			}
		});


		Value w = Object("added",added)("removed",removed);
		w.toStream(out);
	};
	tst.test("Operation.intersection","[3,11,43,87,90,97]") >> [](std::ostream &out) {
		Value leftSet = {21,87,11,-33,43,90,11,8,3,97};
		Value rightSet = {10,55,17,90,11,99,3,97,43,87};


		auto cmp= [](const Value &a, const Value &b){
			return a.getNumber()-b.getNumber();
		};

		Value w = leftSet.sort(cmp).makeIntersection(rightSet.sort(cmp),cmp);
		w.toStream(out);
	};
		tst.test("Operation.union","[-33,3,8,10,11,11,17,21,43,55,87,90,97,99]") >> [](std::ostream &out) {
		Value leftSet = {21,87,11,-33,43,90,11,8,3,97};
		Value rightSet = {10,55,17,90,11,99,3,97,43,87};


		auto cmp= [](const Value &a, const Value &b){
			return a.getNumber()-b.getNumber();
		};

		Value w = leftSet.sort(cmp).makeUnion(rightSet.sort(cmp),cmp);
		w.toStream(out);
	};


	tst.test("Operation.uniq","[-33,3,8,11,21,43,87,90,97]") >> [](std::ostream &out) {
		Value testset = {21,87,11,-33,43,90,11,8,3,97,3,11,87,90,3};

		auto cmp= [](const Value &a, const Value &b){
			return a.getNumber()-b.getNumber();
		};

		Value res= testset.sort(cmp).uniq(cmp);
		res.toStream(out);
	};

	tst.test("Operation.split","[[12,14],[23,25,27],[34,36],[21,22],[78]]") >> [](std::ostream &out) {
		Value testset = {12,14,23,25,27,34,36,21,22,78};

		auto defSet= [](const Value &a, const Value &b){
			return a.getUInt()/10 - b.getUInt()/10;
		};

		Value res= testset.split(defSet);
		res.toStream(out);
	};

	tst.test("Operation.splitAt", "[1,2,3,4],[5,6,7,8]") >> [](std::ostream &out) {
		Value testset = { 1,2,3,4,5,6,7,8 };
		Value::TwoValues v = testset.splitAt(4);
		out << v.first.toString() << "," << v.second.toString();
	};
	tst.test("Operation.splitAt2", "[],[1,2,3,4,5,6,7,8]") >> [](std::ostream &out) {
		Value testset = { 1,2,3,4,5,6,7,8 };
		Value::TwoValues v = testset.splitAt(0);
		out << v.first.toString() << "," << v.second.toString();
	};
	tst.test("Operation.splitAt3", "[1,2,3,4,5],[6,7,8]") >> [](std::ostream &out) {
		Value testset = { 1,2,3,4,5,6,7,8 };
		Value::TwoValues v = testset.splitAt(-3);
		out << v.first.toString() << "," << v.second.toString();
	};
	tst.test("Operation.splitAt4", "Hello, world") >> [](std::ostream &out) {
		Value testset = "Hello world";
		Value::TwoValues v = testset.splitAt(5);
		out << v.first.toString() << "," << v.second.toString();
	};
	tst.test("Value.setKey", "Hello world") >> [](std::ostream &out) {
		Value v = "world";
		v = Value("Hello", v);
		out << v.getKey() << " " << v.getString();
	};
	tst.test("Parser.commented", "{\"blockComment\\/*not here*\\/\":\"here\\\"\\r\\n\",\"lineComment\\/\\/not here\":\"here\"}") >> [](std::ostream &out) {
		StrViewA str = "{\r\n"
			"\"lineComment//not here\":\r\n"
			"\"here\",//there is comment\r\n"
			"\"blockComment/*not here*/\":\"here\\\"\\r\\n\"/*there is comment*/"
			"}";
		std::size_t pos = 0;
		Value v = Value::parse(removeComments([&pos, &str]() {
			int i = str[pos++];
			return i;
		}));
		out << v.stringify();

	};
	tst.test("Misc.wideToUtf8",u8"testing-ěščřžýáíé") >> [](std::ostream &out) {
		std::wstring text = L"testing-ěščřžýáíé";
		String s(text);
		out << s;
	};

	tst.test("Misc.utf8ToWide",u8"testing-ěščřžýáíé") >> [](std::ostream &out) {
		std::string text = u8"testing-ěščřžýáíé";
		String s(text);
		std::wstring wtext = s.wstr();
		String t(wtext);
		out << s;
	};
	tst.test("Misc.stringSplit","-aa-b-1231-ewx-123131--xdwew-32324")>> [](std::ostream &out) {
		StrViewA x("aa/*/b/*/1231/*/ewx/*/123131/*//*/xdwew/*/32324");
		auto splitFn = x.split("/*/");
		for (StrViewA s = splitFn(); !x.isSplitEnd(s); s = splitFn()) {
			out << "-" << s;
		}
	};
	tst.test("Misc.stringSplit.limit","-aa-b-1231-ewx-123131/*//*/xdwew/*/32324")>> [](std::ostream &out) {
		StrViewA x("aa/*/b/*/1231/*/ewx/*/123131/*//*/xdwew/*/32324");
		auto splitFn = x.split("/*/",4);
		for (StrViewA s = splitFn(); !x.isSplitEnd(s); s = splitFn()) {
			out << "-" << s;
		}
	};
	tst.test("Reference.object","42,42,52,52") >> [](std::ostream &o) {
		Object x;
		x.set("test",42);
		ValueRef ref = x.makeRef("test"); //ref = &(x.test)
		o << x["test"] << "," << ref << ",";
		ref = 52;
		o << x["test"] << "," << ref;
	};
	tst.test("Reference.array","42,42,52,52") >> [](std::ostream &o) {
		Array x;
		x.push_back(10);
		x.push_back(42);
		ValueRef ref = x.makeRef(1);
		o << x[1] << "," << ref << ",";
		ref = 52;
		o << x[1] << "," << ref;
	};
	tst.test("Reference.value","42,42,52,52") >> [](std::ostream &o) {
		Value v = 42;
		ValueRef ref(v);
		o << v << "," << ref << ",";
		ref = 52;
		o << v << "," << ref;
	};
	tst.test("Reference.sync.object","42,42,52,42,52") >> [](std::ostream &o) {
		Object x;
		x.set("test",42);
		ValueRef ref = x.makeRef("test"); //ref = &(x.test)
		o << x["test"] << "," << ref << ",";
		x.set("test",52);
		o << x["test"] << "," << ref << ",";
		o << ref.sync();

	};
	tst.test("Reference.sync.array","42,42,52,42,52") >> [](std::ostream &o) {
		Array x;
		x.push_back(10);
		x.push_back(42);
		ValueRef ref = x.makeRef(1);
		o << x[1] << "," << ref << ",";
		x.set(1,52);
		o << x[1] << "," << ref << ",";
		o << ref.sync();
	};
	tst.test("Reference.sync.value","42,42,52,42,52") >> [](std::ostream &o) {
		Value v = 42;
		ValueRef ref(v);
		o << v << "," << ref << ",";
		v = 52;
		o << v << "," << ref << ",";
		o << ref.sync();
	};
	tst.test("Value.replace", "[{\"aaa\":1,\"bbb\":2},{\"ccc\":3,\"ddd\":[\"a\",\"x\",\"c\"]}]") >> [](std::ostream &out) {
		Value tstv ({Object("aaa",1)("bbb",2),Object("ccc",3)("ddd",{"a","b","c"})});
		Value modv = tstv.replace(Path::root/1/"ddd"/1,"x");
		modv.toStream(out);
	};
	tst.test("Value.replace2", "[{\"aaa\":1,\"bbb\":2},{\"ccc\":3,\"ddd\":[\"a\",\"b\",\"c\",\"x\"]}]") >> [](std::ostream &out) {
		Value tstv ({Object("aaa",1)("bbb",2),Object("ccc",3)("ddd",{"a","b","c"})});
		Value modv = tstv.replace(Path::root/1/"ddd"/10,"x");
		modv.toStream(out);
	};
	tst.test("Value.replace3", "[{\"aaa\":{\"pokus\":\"x\"},\"bbb\":2},{\"ccc\":3,\"ddd\":[\"a\",\"b\",\"c\"]}]") >> [](std::ostream &out) {
		Value tstv ({Object("aaa",1)("bbb",2),Object("ccc",3)("ddd",{"a","b","c"})});
		Value modv = tstv.replace(Path::root/0/"aaa"/"pokus","x");
		modv.toStream(out);
	};
	tst.test("Value.replace4", "[{\"aaa\":1,\"bbb\":2},{\"1\":{\"2\":{\"3\":\"x\"}},\"ccc\":3,\"ddd\":[\"a\",\"b\",\"c\"]}]") >> [](std::ostream &out) {
		Value tstv ({Object("aaa",1)("bbb",2),Object("ccc",3)("ddd",{"a","b","c"})});
		Value modv = tstv.replace(Path::root/1/"1"/"2"/"3","x");
		modv.toStream(out);
	};
	tst.test("Value.compare", "-10-110-1-11100-111-1100-11-10-1-1-1") >> [](std::ostream &out) {
		out << Value::compare(null,true);//-1
		out << Value::compare(null,null);//0
		out << Value::compare(false,true);//-1
		out << Value::compare(true,false);//1
		out << Value::compare(true,true);//0
		out << Value::compare(100U,256U);//-1
		out << Value::compare(-256,279U);//-1
		out << Value::compare(256,-279);//1
		out << Value::compare(-256,-279);//1
		out << Value::compare(100U,100U);//0
		out << Value::compare(-100,-100);//0
		out << Value::compare(100.5,100.9);//-1
		out << Value::compare(100,-58.9);//1
		out << Value::compare(-23.2,-58.9);//1
		out << Value::compare("aaa","bbb");//-1
		out << Value::compare("ccc","bbb");//1
		out << Value::compare("bbb","bbb");//0
		out << Value::compare("","");//0
		out << Value::compare({1,2,3},{3,4,1});//-1
		out << Value::compare({1,2,3},{true,4,1});//1
		out << Value::compare({1,2,3},{1,2,3,4});//-1
		out << Value::compare({1,2,3,4},{1,2,3,4});//0
		out << Value::compare(Value(object,{key/"aaa"=10}),Value(object,{key/"aaa"=20}));//-1
		out << Value::compare(Value(object,{key/"aaa"=10}),Value(object,{key/"bbb"=10}));//-1
		out << Value::compare(null,Value(object,{key/"aaa"=10}));//-1
	};


	runValidatorTests(tst);
	runRpcTests(tst);


	tst.test("binary.basic", "ok") >> [](std::ostream &out) {
		if (binTest("src/tests/test.json")) out << "ok"; else out << "not same";
	};
	tst.test("binary.utf-8","ok") >> [](std::ostream &out) {
		if (binTest("src/tests/test2.json")) out << "ok"; else out << "not same";
	};
	tst.test("compress.basic", "ok") >> [](std::ostream &out) {
		if (compressTest("src/tests/test.json")) out << "ok"; else out << "not same";
	};
	tst.test("compress.utf-8","ok") >> [](std::ostream &out) {
		if (compressTest("src/tests/test2.json")) out << "ok"; else out << "not same";
	};


	return tst.didFail()?1:0;
}

int main(int, char **) {
	int r = testMain();
#ifdef _WIN32
	_CrtDumpMemoryLeaks();
#endif
	return r;

}
