/*
 * validatorTests.cpp
 *
 *  Created on: 19. 12. 2016
 *      Author: ondra
 */

#include <iostream>
#include "testClass.h"
#include "../immujson/validator.h"
#include "../immujson/object.h"
#include "../immujson/string.h"

using namespace json;


void ok(std::ostream &out, bool res) {
	if (res) out <<"ok"; else out << "fail";
}


void vtest(std::ostream &out, Value def, Value test) {
	Validator vd(def);
	if (vd.validate(test)) {
		out << "ok";
	} else {
		out << vd.getRejections().toString();
	}

}

void runValidatorTests(TestSimple &tst) {

	tst.test("Validator.string","ok") >> [](std::ostream &out) {
		vtest(out,Object("","string"),"aaa");
	};
	tst.test("Validator.not_string","[[[],\"string\"]]") >> [](std::ostream &out) {
		vtest(out,Object("","string"),12.5);
	};
	tst.test("Validator.number","ok") >> [](std::ostream &out) {
		vtest(out,Object("","number"),12.5);
	};
	tst.test("Validator.not_number","[[[],\"number\"]]") >> [](std::ostream &out) {
		vtest(out,Object("","number"),"12.5");
	};
	tst.test("Validator.boolean","ok") >> [](std::ostream &out) {
		vtest(out,Object("","boolean"),true);
	};
	tst.test("Validator.not_boolean","[[[],\"boolean\"]]") >> [](std::ostream &out) {
		vtest(out,Object("","boolean"),"w2133");
	};
	tst.test("Validator.array","ok") >> [](std::ostream &out) {
		vtest(out,Object("",{{"array","number"}}),{12,32,87,21});
	};
	tst.test("Validator.arrayMixed","ok") >> [](std::ostream &out) {
		vtest(out,Object("",{{"array","number","boolean"}}),{12,32,87,true,12});
	};
	tst.test("Validator.array.fail","[[[1],[\"array\",\"number\"]]]") >> [](std::ostream &out) {
		vtest(out,Object("",{{"array","number"}}),{12,"pp",32,87,21});
	};
	tst.test("Validator.arrayMixed-fail","[[[2],[\"array\",\"number\",\"boolean\"]]]") >> [](std::ostream &out) {
		vtest(out,Object("",{{"array","number","boolean"}}),{12,32,"aa",87,true,12});
	};
	tst.test("Validator.array.limit","ok") >> [](std::ostream &out) {
		vtest(out,Object("",{{"array","number"},{"maxsize",3}}),{10,20,30});
	};
	tst.test("Validator.array.limit-fail","[[[],[[\"array\",\"number\"],[\"maxsize\",3]]]]") >> [](std::ostream &out) {
		vtest(out,Object("",{{"array","number"},{"maxsize",3}}),{10,20,30,40});
	};
	tst.test("Validator.tuple","ok") >> [](std::ostream &out) {
		vtest(out,Object("",{{"tuple","number","string","string"}}),{12,"abc","cdf"});
	};
	tst.test("Validator.tuple-fail1","[[[],[[\"tuple\",\"number\",\"string\",\"string\"]]]]") >> [](std::ostream &out) {
		vtest(out,Object("",{{"tuple","number","string","string"}}),{12,"abc","cdf",232});
	};
	tst.test("Validator.tuple-fail2","[[[2],\"string\"]]") >> [](std::ostream &out) {
		vtest(out,Object("",{{"tuple","number","string","string"}}),{12,"abc"});
	};
	tst.test("Validator.tuple-optional","ok") >> [](std::ostream &out) {
		vtest(out,Object("",{{"tuple","number","string",{"string","optional"}}}),{12,"abc"});
	};
	tst.test("Validator.tuple-fail3","[[[1],\"string\"]]") >> [](std::ostream &out) {
		vtest(out,Object("",{{"tuple","number","string","string"}}),{12,21,"abc"});
	};
	tst.test("Validator.tuple+","ok") >> [](std::ostream &out) {
		vtest(out,Object("",{{"tuple+","number","string","number"}}),{12,"abc",11,12,13,14});
	};
	tst.test("Validator.object", "ok") >> [](std::ostream &out) {
		vtest(out, Object("", Object("aaa","number")("bbb","string")("ccc","boolean")), Object("aaa",12)("bbb","xyz")("ccc",true));
	};
	tst.test("Validator.object.failExtra", "[[[\"ddd\"],\"undefined\"]]") >> [](std::ostream &out) {
		vtest(out, Object("", Object("aaa", "number")("bbb", "string")("ccc", "boolean")), Object("aaa", 12)("bbb", "xyz")("ccc", true)("ddd",12));
	};
	tst.test("Validator.object.failMissing", "[[[\"bbb\"],\"string\"]]") >> [](std::ostream &out) {
		vtest(out, Object("", Object("aaa", "number")("bbb", "string")("ccc", "boolean")), Object("aaa", 12)("ccc", true));
	};
	tst.test("Validator.object.okExtra", "ok") >> [](std::ostream &out) {
		vtest(out, Object("", { {"object",Object("aaa", "number")("bbb", "string")("ccc", "boolean"),"number"} }), Object("aaa", 12)("bbb", "xyz")("ccc", true)("ddd", 12));
	};
	tst.test("Validator.object.okMissing", "ok") >> [](std::ostream &out) {
		vtest(out, Object("", { { "object",Object("aaa", "number")("bbb", {"string","optional"})("ccc", "boolean"),"number" } }), Object("aaa", 12)("ccc", true));
	};


}


