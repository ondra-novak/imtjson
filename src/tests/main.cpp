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

#include "../immujson/edit.h"
#include "../immujson/parser.h"
#include "../immujson/serializer.h"
#include "../immujson/basicValues.h"
#include "testClass.h"
#include <memory>
#include <fstream>
#include "../immujson/compress.tcc"
#include "../immujson/path.h"
#include "../immujson/string.h"
#include "../immujson/operations.h"

using namespace json;

void compressDemo(std::string file) {
	std::ifstream infile(file);
	{
		std::ofstream outfile(file+".cmp",std::ofstream::out|std::ostream::trunc| std::ofstream::binary);
		{
			auto compressor = compress([&outfile](unsigned char b){
				outfile.put(b);
			});
			int z;
			while ((z = infile.get()) != -1) {
				compressor((char)z);
			}
		}
	}
	{
		std::ifstream infile(file+".cmp",std::ifstream::binary);
		std::ofstream outfile(file+".decmp",std::ofstream::out|std::ostream::trunc|std::ofstream::binary);
		{
			auto decompressor = decompress([&infile](){
				return infile.get();
			});
			int z;
			while ((z = decompressor()) != -1) {
				outfile.put((char)z);
			}
		}
	}

}

void subobject(Object &&obj) {
	obj("outbreak", 19)
		("outgrow", 21);
}


int main(int , char **) {
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
		out << String(s+t);
	};
	tst.test("String.insert","Hello big world!") >> [](std::ostream &out) {
		String s("Hello world!");
		out << s.insert(6,"big ");
	};
	tst.test("String.replace","Hello whole planet!") >> [](std::ostream &out) {
		String s("Hello world!");
		out << s.replace(6,5, "whole planet");
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

	tst.test("Operation.merge_objects","{\"aaa\":42,\"bbb\":\"foo\",\"ccc\":true}") >> [](std::ostream &out) {
		Object obj;
		Value({Object("aaa",10),Object("bbb","foo"),Object("ccc",true),Object("aaa",32)})
				.reduce([](Object *obj, const Value &v){
			obj->merge(v,[](Value orig, Value newV) {
				return orig.getNumber()+newV.getNumber();
			});
			return obj;
		},&obj);
		Value(obj).toStream(out);
	};

	tst.test("Operation.sort","[-33,3,8,11,11,21,43,87,90,97]") >> [](std::ostream &out) {
		Value v = {21,87,11,-33,43,90,11,8,3,97};
		Value w = v.sort([](const Value &a, const Value &b){
			return a.getNumber()-b.getNumber();
		});
		w.toStream(out);
	};
	tst.test("Operation.mergeReduce","{\"added\":[10,17,55,99],\"removed\":[-33,8,11,21]}") >> [](std::ostream &out) {
		Value oldSet = {21,87,11,-33,43,90,11,8,3,97};
		Value newSet = {10,55,17,90,11,99,3,97,43,87};
		Array added;
		Array removed;
		auto sortNumbers = [](const Value &a, const Value &b){
			return a.getNumber()-b.getNumber();
		};


		oldSet.sort(sortNumbers).mergeReduce(
			newSet.sort(sortNumbers),
			[](const Value &left, const Value &right) -> MergeResult {
				if (!right.defined()) return chooseLeft({false,left});
				if (!left.defined()) return chooseRight({true,right});
				double diff = left.getNumber() - right.getNumber();
				if (diff<0) return chooseLeft({false,left});
				if (diff>0) return chooseRight({true,right});
				return chooseBoth(undefined);
			},
			[&added,&removed](std::nullptr_t, const Value &v) {
				if (v[0].getBool()) added.add(v[1]);
				else removed.add(v[1]);
				return nullptr;
			},
			nullptr);
		Value w = Object("added",added)("removed",removed);
		w.toStream(out);
	};
	tst.test("Operation.mergeToArray","[3,11,43,87,90,97]") >> [](std::ostream &out) {
		Value leftSet = {21,87,11,-33,43,90,11,8,3,97};
		Value rightSet = {10,55,17,90,11,99,3,97,43,87};


		auto cmp= [](const Value &a, const Value &b){
			return a.getNumber()-b.getNumber();
		};

		Value w = leftSet.sort(cmp).merge(
				 rightSet.sort(cmp),
					[&cmp](const Value &left, const Value &right) {
						if (!right.defined()) return chooseLeft(undefined);
						if (!left.defined()) return chooseRight(undefined);
						auto c = cmp(left,right);
						if (c<0) return chooseLeft(undefined);
						if (c>0) return chooseRight(undefined);
						return chooseBoth(left);
					});
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


	tst.test("compress.basic", "ok") >> [](std::ostream &out) {
		std::string buff;
		std::ifstream testfile("src/tests/test.json",std::ifstream::binary);
		if (!testfile) throw std::runtime_error("test file not found");
		Value v1 = Value::fromStream(testfile);
		String s = v1.stringify();
		std::string cs;
		{
			auto cmp = compress([&cs](char c) {
				cs.push_back(c);
			});

			for (auto &&x : (StringView<char>)s) { cmp(x); }
		}

		std::string ds;
		std::string::const_iterator csiter = cs.begin();
		{
			auto decmp = decompress([&csiter]() {
				char c = *csiter;
				++csiter;
				return c; });

			char z;
			while ((z = decmp()) != -1) ds.push_back(z);
		}

		if (StringView<char>(ds) == s) out << "ok";
	};
	tst.test("compress.demo1","ok") >> [](std::ostream &out) {
		compressDemo("src/tests/test.json");
		out<<"ok";
	};
	
	tst.test("MemoryLeaks", "") >> [](std::ostream &out) {
#ifdef _WIN32
		if (_CrtDumpMemoryLeaks()) {
			out << "Detected memory leaks!";
		}
#else
		throw TestNotImplemented();
#endif

	};

	return tst.didFail()?1:0;
}
