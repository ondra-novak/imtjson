/*
 * validatorTests.cpp
 *
 *  Created on: 19. 12. 2016
 *      Author: ondra
 */

#include <iostream>
#include <fstream>
#include "testClass.h"
#include "../imtjson/validator.h"
#include "../imtjson/object.h"
#include "../imtjson/string.h"

using namespace json;


void ok(std::ostream &out, bool res) {
	if (res) out <<"ok"; else out << "fail";
}


void vtest(std::ostream &out, Value def, Value test) {
	Validator vd(def);
	if (vd.validate(test,"_root")) {
		out << "ok";
	} else {
		out << vd.getRejections().toString();
	}

}

void runValidatorTests(TestSimple &tst) {

	tst.test("Validator.string","ok") >> [](std::ostream &out) {
		vtest(out,Object("_root","string"),"aaa");
	};
	tst.test("Validator.not_string","[[[],\"string\"]]") >> [](std::ostream &out) {
		vtest(out,Object("_root","string"),12.5);
	};
	tst.test("Validator.charset","ok") >> [](std::ostream &out) {
		vtest(out,Object("_root","[a-z-0-9XY]"),"ahoj-123");
	};
	tst.test("Validator.charset.neg","ok") >> [](std::ostream &out) {
		vtest(out,Object("_root","[^a-z-0-9XY]"),"AHOJ!");
	};
	tst.test("Validator.charset.not_ok",R"([[[],"[a-z-123]"]])") >> [](std::ostream &out) {
		vtest(out,Object("_root","[a-z-123]"),"ahoj_123");
	};
	tst.test("Validator.charset.neg.not_ok",R"([[[],"[^a-z-]"]])") >> [](std::ostream &out) {
		vtest(out,Object("_root","[^a-z-]"),"AHOJ-AHOJ");
	};
	tst.test("Validator.not_string","[[[],\"string\"]]") >> [](std::ostream &out) {
		vtest(out,Object("_root","string"),12.5);
	};
	tst.test("Validator.number","ok") >> [](std::ostream &out) {
		vtest(out,Object("_root","number"),12.5);
	};
	tst.test("Validator.not_number","[[[],\"number\"]]") >> [](std::ostream &out) {
		vtest(out,Object("_root","number"),"12.5");
	};
	tst.test("Validator.boolean","ok") >> [](std::ostream &out) {
		vtest(out,Object("_root","boolean"),true);
	};
	tst.test("Validator.not_boolean","[[[],\"boolean\"]]") >> [](std::ostream &out) {
		vtest(out,Object("_root","boolean"),"w2133");
	};
	tst.test("Validator.array","ok") >> [](std::ostream &out) {
		vtest(out,Object("_root",{array,"number"}),{12,32,87,21});
	};
	tst.test("Validator.arrayMixed","ok") >> [](std::ostream &out) {
		vtest(out,Object("_root",{array,"number","boolean"}),{12,32,87,true,12});
	};
	tst.test("Validator.array.fail","[[[1],\"number\"]]") >> [](std::ostream &out) {
		vtest(out,Object("_root",{array,"number"}),{12,"pp",32,87,21});
	};
	tst.test("Validator.arrayMixed-fail","[[[2],\"number\"],[[2],\"boolean\"]]") >> [](std::ostream &out) {
		vtest(out,Object("_root",{array,"number","boolean"}),{12,32,"aa",87,true,12});
	};
	tst.test("Validator.array.limit","ok") >> [](std::ostream &out) {
		vtest(out,Object("_root",{{array,"number"},{"maxsize",3}}),{10,20,30});
	};
	tst.test("Validator.array.limit-fail","[[[],[\"maxsize\",3]]]") >> [](std::ostream &out) {
		vtest(out,Object("_root",{"^",{array,"number"},{"maxsize",3}}),{10,20,30,40});
	};
	tst.test("Validator.tuple","ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { {3},"number","string","string" }), { 12,"abc","cdf" });
	};
	tst.test("Validator.tuple-fail1","[[[3],[[3],\"number\",\"string\",\"string\"]]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", { {3},"number","string","string" }), { 12,"abc","cdf",232 });
	};
	tst.test("Validator.tuple-fail2","[[[2],\"string\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", { {3},"number","string","string" }), { 12,"abc" });
	};
	tst.test("Validator.tuple-optional","ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { {3},"number","string",{"string","optional"} }), { 12,"abc" });
	};
	tst.test("Validator.tuple-fail3","[[[1],\"string\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", { {3},"number","string","string" }), { 12,21,"abc" });
	};
	tst.test("Validator.tuple+","ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { {2},"number","string","number" }), { 12,"abc",11,12,13,14 });
	};
	tst.test("Validator.object", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", Object("aaa","number")("bbb","string")("ccc","boolean")), Object("aaa",12)("bbb","xyz")("ccc",true));
	};
	tst.test("Validator.object.failExtra", "[[[\"ddd\"],\"undefined\"],[[],{\"aaa\":\"number\",\"bbb\":\"string\",\"ccc\":\"boolean\"}]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", Object("aaa", "number")("bbb", "string")("ccc", "boolean")), Object("aaa", 12)("bbb", "xyz")("ccc", true)("ddd",12));
	};
	tst.test("Validator.object.failMissing", "[[[\"bbb\"],\"string\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", Object("aaa", "number")("bbb", "string")("ccc", "boolean")), Object("aaa", 12)("ccc", true));
	};
	tst.test("Validator.object.okExtra", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root",  Object("aaa", "number")("bbb", "string")("ccc", "boolean")("%","number")), Object("aaa", 12)("bbb", "xyz")("ccc", true)("ddd", 12));
	};
	tst.test("Validator.object.extraFailType", "[[[\"ddd\"],\"number\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", Object("aaa", "number")("bbb", "string")("ccc", "boolean")("%", "number")), Object("aaa", 12)("bbb", "xyz")("ccc", true)("ddd", "3232"));
	};
	tst.test("Validator.object.okMissing", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", Object("aaa", "number")("bbb", {"string","optional"})("ccc", "boolean")("%","number")), Object("aaa", 12)("ccc", true));
	};
	tst.test("Validator.object.array", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", Object("_xxx", {array,"any"})), Object("_xxx", {10,20,30}));
	};
	tst.test("Validator.userClass", "ok") >> [](std::ostream &out) {
		vtest(out, Object("test", { "string","number" })("_root", Object("aaa", "test")("bbb","test")), Object("aaa", 12)("bbb","ahoj"));
	};
	tst.test("Validator.recursive", "ok") >> [](std::ostream &out) {
		vtest(out, Object("test", { "string",{array,"test"} })("_root", { array, "number","test" }), { 10,{{"aaa"}} });
	};

	tst.test("Validator.recursive.fail", "[[[],\"number\"],[[],\"string\"],[[0,1,0],[[],\"test\"]],[[0,1,0],\"test\"],[[0,1],\"test\"],[[0],\"test\"],[[],\"test\"]]") >> [](std::ostream &out) {
		vtest(out, Object("test", { "string",{ array,"test" } })("_root", { "number","test" }), { { "ahoj",{ 12 } } });
	};
	tst.test("Validator.range", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { array, {">",0,"<",10} ,"string" } ), { "ahoj",4 });
	};
	tst.test("Validator.range.fail", "[[[1],\"string\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", { array, {">",0,"<",10},"string" }), { "ahoj",15 });
	};
	tst.test("Validator.range.multiple", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { array, {">",0,"<",10},{">=","A","<=","Z" } }), { "A",5 });
	};
	tst.test("Validator.range.multiple.fail", "[[[0],[\">\",0,\"<\",10]],[[0],[\">=\",\"A\",\"<=\",\"Z\"]]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", { array, {">",0,"<",10},{">=","A","<=","Z" } }), { "a",5 });
	};
	tst.test("Validator.fnAll", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "^","hex",">=","0","<=","9", "#comment" } ), "01255");
	};
	tst.test("Validator.fnAll.fail", "[[[],[\"^\",\"hex\",\">=\",\"0\",\"<=\",\"9\",\"#comment\"]]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "^","hex", ">=","0","<=","9", "#comment"  }), "A589");
	};
	tst.test("Validator.fnAll.fail2", "[[[],\"hex\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "^","hex",{ ">=","0","<=","9" } }), "lkoo");
	};
	tst.test("Validator.prefix.ok", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "prefix","abc." }), "abc.123");
	};
	tst.test("Validator.prefix.fail", "[[[],[\"prefix\",\"abc.\"]]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "prefix","abc." }), "abd.123");
	};
	tst.test("Validator.suffix.ok", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "suffix",".abc" }), "123.abc");
	};
	tst.test("Validator.suffix.fail", "[[[],[\"suffix\",\".abc\"]]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "suffix",".abc" }), "123.abd");
	};
	tst.test("Validator.prefix.tonumber.ok", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "prefix","abc.", {"tonumber",{">",100}} }), "abc.123");
	};
	tst.test("Validator.suffix.tonumber.ok", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "suffix",".abc",{ "tonumber",{ ">",100 } } }), "123.abc");
	};
	tst.test("Validator.comment.any", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "#comment","any" }), 123);
	};
	tst.test("Validator.comment.any2", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "#comment","any" }), "3232");
	};
	tst.test("Validator.comment.any.fail", "[[[],\"#comment\"],[[],\"any\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "#comment","any" }), undefined);
	};
	tst.test("Validator.enum1", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", {"'aaa","'bbb","'ccc" }), "aaa");
	};
	tst.test("Validator.enum2", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "'aaa","'bbb","'ccc" }), "bbb");
	};
	tst.test("Validator.enum3", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "'aaa","'bbb","'ccc" }), "ccc");
	};
	tst.test("Validator.enum4", "[[[],\"'aaa\"],[[],\"'bbb\"],[[],\"'ccc\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "'aaa","'bbb","'ccc" }), "ddd");
	};
	tst.test("Validator.enum5", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { 111,222,333 }), 111);
	};
	tst.test("Validator.enum6", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { 111,222,333 }),222);
	};
	tst.test("Validator.enum7", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { 111,222,333 }), 333);
	};
	tst.test("Validator.enum8", "[[[],[111,222,333]]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", { 111,222,333 }), "111");
	};
	tst.test("Validator.set1", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { array, 1,2,3 }), { 1,2 });
	};
	tst.test("Validator.set2", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { array, 1,2,3 }), { 1,3 });
	};
	tst.test("Validator.set3", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { array, 1,2,3 }), array);
	};
	tst.test("Validator.set4", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { array, 1,2,3 }), { 2,3 });
	};
	tst.test("Validator.set5", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { array, 1,2,3 }), { 2, 3,3 });
	};
	tst.test("Validator.set6", "[[[1],[[],1,2,3]]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", { array, 1,2,3 }), { 2,4});
	};
	tst.test("Validator.prefix-array.ok", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "prefix",{"a","b"} }), { "a","b","c","d" });
	};
	tst.test("Validator.prefix-array.fail", "[[[],[\"prefix\",[\"a\",\"b\"]]]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "prefix",{ "a","b" } }), { "a","c","b","d" });
	};
	tst.test("Validator.suffix-arrau.ok", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "suffix",{ "c","d" } }), { "a","b","c","d" });
	};
	tst.test("Validator.suffix-array.fail", "[[[],[\"suffix\",[\"c\",\"d\"]]]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", { "suffix",{ "c","d" } }), { "a","c","b","d" });
	};
	tst.test("Validator.key.ok", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", Object("%", { "^",{ "key",{"^","lowercase",{"maxsize",2} } },"number" })), Object("ab", 123)("cx", 456));
	};
	tst.test("Validator.key.fail", "[[[\"abc\"],[\"maxsize\",2]]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", Object("%", { "^",{ "key",{ "^","lowercase",{ "maxsize",2 } } },"number" })), Object("abc", 123)("cx", 456));
	};
	tst.test("Validator.base64.ok", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", "base64"), "flZhTGlEYVRvUn5+flRlc1R+fg==");
	};
	tst.test("Validator.base64.fail1", "[[[],\"base64\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", "base64"), "flZhTGl@YVRvUn5+flRlc1R+fg==");
	};
	tst.test("Validator.base64.fail2", "[[[],\"base64\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", "base64"), "flZhTGlEYVRvUn5+flRlc1R+f===");
	};
	tst.test("Validator.base64.fail3", "[[[],\"base64\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", "base64"), "flZhTGlEYVRvUn5+flRlc1R+f==");
	};
	tst.test("Validator.base64url.ok", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", "base64url"), "flZhTGlEYVRvUn5_flRlc1R_fg==");
	};
	tst.test("Validator.base64url.fail1", "[[[],\"base64url\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", "base64url"), "flZhTGlEYVRvUn5+flRlc1R+fg==");
	};
	tst.test("Validator.not1", "ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", { array, {"!",1,2,3} }), { 10,20 });
	};
	tst.test("Validator.not2", "[[[0],[\"!\",1,2,3]]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", { array,{ "!",1,2,3 } }), { 1,3 });
	};
	tst.test("Validator.datetime","ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", "datetime"), "2016-12-20T12:38:00Z");
	};
	tst.test("Validator.datetime.fail1","[[[],\"datetime\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", "datetime"), "2016-13-20T12:38:00Z");
	};
	tst.test("Validator.datetime.fail2","[[[],\"datetime\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", "datetime"), "2016-00-20T12:38:00Z");
	};
	tst.test("Validator.datetime.leap.fail","[[[],\"datetime\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", "datetime"), "2015-02-29T12:38:00Z");
	};
	tst.test("Validator.datetime.leap.ok","ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", "datetime"), "2016-02-29T12:38:00Z");
	};
	tst.test("Validator.datetime.fail3","[[[],\"datetime\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", "datetime"), "2016-01-29T25:38:00Z");
	};
	tst.test("Validator.datetime.fail4","[[[],\"datetime\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", "datetime"), "2016-12-20T12:70:00Z");
	};
	tst.test("Validator.datetime.fail5","[[[],\"datetime\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", "datetime"), "2016-12-20T12:38:72Z");
	};
	tst.test("Validator.datetime.fail6","[[[],\"datetime\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", "datetime"), "K<oe23HBY932pPLO(*JN");
	};
	tst.test("Validator.date.ok","ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", "date"), "1985-10-17");
	};
	tst.test("Validator.date.fail","[[[],\"date\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", "date"), "1985-10-17 EOX");
	};
	tst.test("Validator.time.ok","ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", "timez"), "12:50:34Z");
	};
	tst.test("Validator.time.fail","[[[],\"timez\"]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", "timez"), "A2xE0:34Z");
	};
	tst.test("Validator.explode.ok","ok") >> [](std::ostream &out) {
		vtest(out, Value::fromString(R"({"_root":["explode",".",[[3],"alpha","alpha","alpha"]]})"),"ahoj.nazdar.cau");
	};
	tst.test("Validator.explode.fail1",R"([[[2],"alpha"]])") >> [](std::ostream &out) {
		vtest(out, Value::fromString(R"({"_root":["explode",".",[[3],"alpha","alpha","alpha"]]})"),"ahoj.nazdar.123");
	};
	tst.test("Validator.explode.fail2",R"([[[2],"alpha"]])") >> [](std::ostream &out) {
		vtest(out, Value::fromString(R"({"_root":["explode",".",[[3],"alpha","alpha","alpha"]]})"),"ahoj.nazdar");
	};
	tst.test("Validator.explode.fail3",R"([[[3],[[3],"alpha","alpha","alpha"]]])") >> [](std::ostream &out) {
		vtest(out, Value::fromString(R"({"_root":["explode",".",[[3],"alpha","alpha","alpha"]]})"),"ahoj.nazdar.cau.tepic");
	};
	tst.test("Validator.explode.ok2","ok") >> [](std::ostream &out) {
		vtest(out, Value::fromString(R"({"_root":["explode",".",[[2],"alpha","any"],1]})"),"ahoj.nazdar.cau.tepic");
	};
	tst.test("Validator.explode.fail4",R"([[[1],"alpha"]])") >> [](std::ostream &out) {
		vtest(out, Value::fromString(R"({"_root":["explode",".",[[2],"alpha","alpha"],1]})"),"ahoj.123");
	};
	tst.test("Validator.explode.fail5",R"([[[1],"any"]])") >> [](std::ostream &out) {
		vtest(out, Value::fromString(R"({"_root":["explode",".",[[2],"alpha","any"],1]})"),"ahoj");
	};
	tst.test("Validator.customtime.ok","ok") >> [](std::ostream &out) {
		vtest(out, Object("_root", {"datetime","DD.MM.YYYY hh:mm:ss"} ), "02.04.2004 12:32:46");
	};
	tst.test("Validator.customtime.fail","[[[],[\"datetime\",\"DD.MM.YYYY hh:mm:ss\"]]]") >> [](std::ostream &out) {
		vtest(out, Object("_root", {"datetime","DD.MM.YYYY hh:mm:ss"} ), "2016-12-20T12:38:72Z");
	};
	tst.test("Validator.entime.ok1","ok") >> [](std::ostream &out) {
		vtest(out, Object("tm",{ {"datetime","M:mm"},{"datetime","MM:mm"} })("_root",{{"suffix","am","tm"},{"suffix","pm"}}), "1:23am");
	};
	tst.test("Validator.entime.ok2","ok") >> [](std::ostream &out) {
		vtest(out, Object("tm",{ {"datetime","M:mm"},{"datetime","MM:mm"} })("_root",{{"suffix","am","tm"},{"suffix","pm"}}), "11:45pm");
	};
	tst.test("Validator.entime.fail1","[[[],[\"datetime\",\"M:mm\"]],[[],[\"datetime\",\"MM:mm\"]],[[],\"tm\"],[[],[\"suffix\",\"pm\"]]]") >> [](std::ostream &out) {
		vtest(out, Object("tm",{ {"datetime","M:mm"},{"datetime","MM:mm"} })("_root",{{"suffix","am","tm"},{"suffix","pm"}}), "13:23am");
	};
	tst.test("Validator.entime.fail2","[[[],[\"suffix\",\"am\",\"tm\"]],[[],[\"suffix\",\"pm\"]]]") >> [](std::ostream &out) {
		vtest(out, Object("tm",{ {"datetime","M:mm"},{"datetime","MM:mm"} })("_root",{{"suffix","am","tm"},{"suffix","pm"}}), "2:45xa");
	};
	tst.test("Validator.setvar1","ok") >> [](std::ostream &out) {
		vtest(out, Object("_root",{"setvar","$test",Object("aaa",{"$test","bbb"})("bbb","number")}), Object("aaa",10)("bbb",10));
	};
	tst.test("Validator.setvar1.fail","[[[\"aaa\"],[\"$test\",\"bbb\"]]]") >> [](std::ostream &out) {
		vtest(out, Object("_root",{"setvar","$test",Object("aaa",{"$test","bbb"})("bbb","number")}), Object("aaa",20)("bbb",10));
	};
	tst.test("Validator.setvar2","ok") >> [](std::ostream &out) {
		vtest(out, Object("_root",{"setvar","$test",Object("aaa",{"<",{"$test","bbb"}})("bbb","number")}), Object("aaa",10)("bbb",20));
	};
	tst.test("Validator.setvar2.fail","[[[\"aaa\"],[\"<\",[\"$test\",\"bbb\"]]]]") >> [](std::ostream &out) {
		vtest(out, Object("_root",{"setvar","$test",Object("aaa",{"<",{"$test","bbb"}})("bbb","number")}), Object("aaa",10)("bbb",5));
	};
	tst.test("Validator.setvar3","ok") >> [](std::ostream &out) {
		vtest(out, Object("_root",{"setvar","$test",Object("count","digits")("data",{"maxsize",{"$test","count"}})}), Object("count",3)("data",{10,20,30}));
	};
	tst.test("Validator.setvar3.fail","[[[\"data\"],[\"maxsize\",[\"$test\",\"count\"]]]]") >> [](std::ostream &out) {
		vtest(out, Object("_root",{"setvar","$test",Object("count","digits")("data",{"maxsize",{"$test","count"}})}), Object("count",3)("data",{10,20,30,40}));
	};
	tst.test("Validator.version.fail","[[[\"_version\"],2]]") >> [](std::ostream &out) {
		vtest(out, Object("_root","any")("_version",2.0), 42);
	};
	tst.test("Validator.selfValidate", "ok") >> [](std::ostream &out) {
		std::ifstream fstr("src/imtjson/validator.json", std::ios::binary);
		Value def = Value::fromStream(fstr);
		Validator vd(def);
		if (vd.validate(def)) {
			out << "ok";
		}
		else {
			out << vd.getRejections().toString();
		}
	};
}


