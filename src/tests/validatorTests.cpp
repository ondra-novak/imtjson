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

void runValidatorTests(TestSimple &tst) {

	tst.test("Validator.string","ok") >> [](std::ostream &out) {

		Object def;
		def("","string");
		Validator vd(def);
		ok(out, vd.validate("aaa"));

	};
	tst.test("Validator.not_string","[[[],\"string\"]]") >> [](std::ostream &out) {

		Object def;
		def("","string");
		Validator vd(def);
		if (!vd.validate(12.5)) {
			out << vd.getRejections().toString();
		}

	};
	tst.test("Validator.number","ok") >> [](std::ostream &out) {

		Object def;
		def("","number");
		Validator vd(def);
		ok(out, vd.validate(21.5));
	};
	tst.test("Validator.not_number","ok") >> [](std::ostream &out) {

		Object def;
		def("","number");
		Validator vd(def);
		ok(out, !vd.validate("231"));
	};
	tst.test("Validator.boolean","ok") >> [](std::ostream &out) {

		Object def;
		def("","boolean");
		Validator vd(def);
		ok(out, vd.validate(true));
	};
	tst.test("Validator.not_boolean","ok") >> [](std::ostream &out) {

		Object def;
		def("","boolean");
		Validator vd(def);
		ok(out, !vd.validate(1));
	};


}


