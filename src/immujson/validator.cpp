/*
 * validator.cpp
 *
 *  Created on: 15. 12. 2016
 *      Author: ondra
 */

#include "validator.h"

namespace json {


Validator::Result Validator::onNativeRule(const Value& subject,
		const StrViewA& ruleName, const Value& args) {
}

bool Validator::validate(const Value& subject, const StrViewA& rule, const Value& args, const Path &path) {
	rejections.clear();
	return validateInternal(subject,rule,args,path);
}

Validator::Validator(const Value& definition):curPath(nullptr) {

}

Value Validator::getRejections() const {
	return rejections;
}

template<typename T>
class StackSave {
public:
	T value;
	T &var;
	StackSave(T &var):var(var),value(var) {}
	~StackSave() {var = value;}
};


bool Validator::validateInternal(const Value& subject, const StrViewA& rule,
		const Value& args, const Path& path) {


	StackSave<const Path *> pathSave(curPath);
	StackSave<const Value *> argsSave(curArgs);

	curArgs = &args;
	curPath = &path;
	Value ruleline = def[rule];

	return validateRuleLine(subject, ruleline);
}

bool Validator::validateRuleLine(const Value& subject, const Value& ruleLine) {
	if (ruleLine.type() == array) {
		return validateRuleLine2(subject, ruleLine);
	} else {
		Result res = validateSingleRule(subject, rule, {});
		if (res != accepted) {
			rejections.add({curPath->toArray(), ruleLine});
			return false;
		} else {
			return true;
		}
	}
}

bool Validator::validateRuleLine2(const Value& subject, const Value& ruleLine, unsigned int offset) {

	Result curRes = undetermined;
	for (auto &&x : ruleLine) {
		if (offset) {
			--offset;
		} else {

			Result r = validateSingleRule(subject, x);
			if (r == rejected) {
				rejections.add({curPath->toArray(), x});
			} else if (r == accepted) {
				curRes = r;
			}
		}
	}
	if (curRes == undetermined) {
		rejections.add({curPath->toArray(), ruleLine});
		return false;
	} else {
		return curRes == accepted;
	}
}

Validator::Result Validator::validateObject(const Value& subject, const Value& ruleLine) {

	if (subject.type() != object) {
		rejections({curPath->toArray(), "object"});
		return undetermined;
	} else {
		for (Value v : ruleLine) {
			StrViewA key = v.getKey();
			Value item = subject[key];
			if (!validateInternal(item, v, curArgs, Path(*curPath,key))) {
				return undetermined;
			}
		}
	}
	return accepted;


}

Validator::Result Validator::validateSingleRule(const Value& subject, const Value& ruleLine) {

	if (ruleLine.type() == object) {
		return validateObject(subject, ruleLine);
	} else if (ruleLine.type() == array) {
		StrViewA ruleName = ruleLine[0].getString();
		return validateSingleRule2(subject, ruleName, ruleLine);
	} else if (ruleLine.type() == string) {
		return validateSingleRule2(subject, ruleLine.getString(), {});
	} else if (subject == ruleLine) {
		return accepted;
	} else {
		return undetermined;
	}
}

Validator::Result Validator::validateSingleRule2(const Value& subject, StrViewA name, const Value& args) {

	if (name == "string") {
		return subject->type() == string?accepted:undetermined;
	} else if (name == "number") {
		return subject->type() == number?accepted:undetermined;
	} else if (name == "boolean") {
		return subject->type() == boolean?accepted:undetermined;
	} else if (name == "any") {
		return subject.defined()?accepted:undetermined;
	} else if (name == "null") {
		return subject.isNull()?accepted:undetermined;
	} else if (name == "undefined" || name == "optional") {
		return !subject.defined()?accepted:undetermined;
	}

}

}
