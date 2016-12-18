/*
 * validator.cpp
 *
 *  Created on: 15. 12. 2016
 *      Author: ondra
 */

#include "validator.h"
#include "array.h"
#include "string.h"

namespace json {



bool Validator::validate(const Value& subject, const StrViewA& rule, const Value& args, const Path &path) {
	rejections.clear();
	curPath = &path;
	return validateInternal(subject,rule,args);
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


bool Validator::validateInternal(const Value& subject, const StrViewA& rule, const Value& args) {


	StackSave<const Value *> argsSave(curArgs);

	curArgs = &args;
	Value ruleline = def[rule];


	return validateRuleLine(subject, ruleline);
}

bool Validator::validateRuleLine(const Value& subject, const Value& ruleLine) {
	if (ruleLine.type() == array) {
		return validateRuleLine2(subject, ruleLine,0);
	} else {
		bool ok = evalRuleAccept(subject, ruleLine.getString(), {})
				&& evalRuleReject(subject, ruleLine.getString(), {});
		if (!ok) {
			rejections.add({curPath->toValue(), ruleLine});
		}
		return ok;
	}
}

bool Validator::onNativeRuleAccept(const Value& , const StrViewA& , const Value& ) {
	return false;
}

bool Validator::onNativeRuleReject(const Value& , const StrViewA& , const Value& ) {
	return false;
}

bool Validator::validateRuleLine2(const Value& subject, const Value& ruleLine, unsigned int offset) {

	int o = offset;
	bool ok = false;
	for (auto &&x : ruleLine) {
		if (o) {
			--o;
		} else {
			bool ok = validateSingleRuleForAccept(subject, x);
			if (ok) break;
		}
	}
	if (ok)
	o = offset;
	for (auto &&x : ruleLine) {
		if (o) {
			--o;
		} else {
			bool ok = validateSingleRuleForReject(subject, x);
			if (!ok) break;
		}
	}
	if (!ok) {
		rejections.add({curPath->toValue(), ruleLine});
		return false;
	}
}


static bool isLess(const Value &a, const Value &b) {
	if (a.type() != b.type()) return false;
	switch (a.type()) {
	case number: 
		if ((a.flags() & numberInteger) && (b.flags() & numberInteger))
			return a.getInt() < b.getInt();
		else if ((a.flags() & numberUnsignedInteger) && (b.flags() & numberUnsignedInteger))
			return a.getUInt() < b.getUInt();
		else
			return a.getNumber() < b.getNumber();
	case string:
		return a.getString() < b.getString();
		break;
	case boolean:
		return a.getBool() == false && b.getBool() == true;
	default:
		return false;
	}
}



bool Validator::evalRuleAccept(const Value& subject, StrViewA name, const Value& args) {

		if (name.empty()) {
			return false;
		} else if (name.data[0] == '\'') {
			return subject.getString() == name.substr(1);
		} else if (name == "string") {
			return subject.type() == string;
		} else if (name == "number") {
			return subject.type() == number;
		} else if (name == "boolean") {
			return subject.type() == boolean;
		} else if (name == "any") {
			return subject.defined();
		} else if (name == "null") {
			return subject.isNull();
		} else if (name == "undefined" || name == "optional") {
			return !subject.defined();
		} else if (name == "array") {
			if (subject.type() != array) return false;
			if (args.empty()) return true;
			unsigned int pos = 0;
			for (Value v : subject) {
				StackSave<const Path *> pathSave(curPath);
				Path newPath(*curPath, pos);
				curPath = &newPath;
				return validateRuleLine2(v,args,1);
			}
		} else if (name == "object") {
			if (subject.type() != object) return false;
			if (args.empty()) return true;
			return validateObject(subject,args[1],args[2]);
		} else if (name == "tuple") {
			return opTuple(subject,args,false);
		}
		else if (name == "vartuple") {
			return opTuple(subject, args, true);
		} else if (name == "()") {
			return isLess(args[1], subject) && isLess(subject, args[2]) ;
		} else if (name == "<)") {
			return !isLess(subject, args[1])  && isLess(subject, args[2]) ;
		} else if (name == "<>") {
			return !isLess(subject, args[1])  && !isLess( args[2], subject) ;
		} else if (name == "(>") {
			return isLess(args[1], subject)  && !isLess( args[2], subject) ;
		}
		else if (name == "value") {
			return subject == args;
		}
		else if (name == "sub") {
			return validateRuleLine2(v, args, 1);
		}
		else if (name == "to-string") {
			return validateRuleLine(subject.toString(), args[1]);
		}
		else if (name == "to-number") {
			return validateRuleLine(subject.getNumber(), args[1]);
		} else if (name[0] == '$') {

			int pos = atoi(name.data + 1);
			if (pos<0 || pos >= curArgs->size()) {
				return false;
			}
			else {
				return validateSingleRuleForAccept(subject, curArgs->operator[](pos)
			}
		} else {
			return checkClassAccept(subject,name, args);
		}
}

static bool checkMaxSize(const Value &subject, std::size_t sz) {
	switch (subject.type()) {
	case array:
	case object: return subject.size() <= sz;
	case string: return subject.getString().length <=sz;
	default:return true;
	}
}

static bool checkMinSize(const Value &subject, std::size_t sz) {
	switch (subject.type()) {
	case array:
	case object: return subject.size() >= sz;
	case string: return subject.getString().length >=sz;
	default:return true;
	}
}

bool Validator::checkKey(const Value &subject, const Value &args) {
	StrViewA key = subject.getKey();
	return validateRuleLine2(key,args,1);
}


bool Validator::evalRuleReject(const Value& subject, StrViewA name, const Value& args) {
		if (name == "max-size") {
			return checkMaxSize(subject, args[1].getUInt());
		} else if (name == "min-size") {
			return checkMinSize(subject, args[1].getUInt());
		} else if (name == "key") {
			return checkKey(subject, args);
		} else if (name == "prefix") {
			return opPrefix(subject,args);
		} else if (name == "suffix") {
			return opSuffix(subject,args);
		} else {
			return checkClassReject(subject,name, args);
		}



}

bool Validator::validateObject(const Value& subject, const Value& templateObj, const Value& extraRules) {
	if (subject.type() != object) {
		return false;
	} else {
		for (Value v : templateObj) {
			StackSave<const Path *> store(curPath);

			StrViewA key = v.getKey();
			Value item = subject[key];
			Path nxtPath(*curPath, key);
			curPath = &nxtPath;
			if (!validateRuleLine(item, v)) {
				return false;
			}
		}
		for (Value v : subject) {
			if (templateObj[v.getKey()].defined()) continue;
			StackSave<const Path *> store(curPath);
			Path nxtPath(*curPath, v.getKey());
			curPath = &nxtPath;
			if (!validateRuleLine2(v,extraRules,0)) return false;
		}
	}



	return true;

}

bool Validator::validateSingleRuleForAccept(const Value& subject, const Value& ruleLine) {

	return  ruleLine.type() == object?validateObject(subject,ruleLine, {}):
			   ruleLine.type() == array?evalRuleAccept(subject,ruleLine[0].getString(),ruleLine):
			   ruleLine.type() == string?evalRuleAccept(subject,ruleLine.getString(),{}):
			   subject == ruleLine;
}

bool Validator::validateSingleRuleForReject(const Value& subject, const Value& ruleLine) {

	return  ruleLine.type() == array?evalRuleReject(subject,ruleLine[0].getString(),ruleLine):
			ruleLine.type() == string?evalRuleReject(subject,ruleLine.getString(),{}):
			true;
}

bool Validator::checkClassAccept(const Value& subject, StrViewA name, const Value& args) {
	Value ruleline = def[name];
	if (!ruleline.defined()) {
		throw std::runtime_error(std::string("Undefined class: ")+std::string(name));
	} else if (ruleline.getString() == "native") {
		return onNativeRuleAccept(subject,name,args);
	} else {
		return validateInternal(subject,name,args);
	}

}

bool Validator::checkClassReject(const Value& subject, StrViewA name, const Value& args) {
	Value ruleline = def[name];
	if (ruleline.getString() == "native")
		return onNativeRuleReject(subject,name,args);
	else
		return true;
}

bool Validator::opPrefix(const Value& subject, const Value& args) {
	Value pfix = args[1];
	Value rule = args[2];
	if (pfix.type() == array) {
		auto spl = subject.splitAt(pfix.size());
		if (spl.first != pfix)  return false;
		if (rule.defined()) return validateRuleLine(spl.second, rule);
		return true;
	}
	else {
		StrViewA txt = subject.getString();
		StrViewA str = pfix.getString();
		if (txt.substr(0, str.length) != str) return false;
		if (rule.defined()) return validateRuleLine(txt.substr(str.length), rule);
		return true;
	}

}

bool Validator::opSuffix(const Value& subject, const Value& args) {
	Value sfix = args[1];
	Value rule = args[2];
	if (sfix.type() == array) {
		auto spl = subject.splitAt(-sfix.size());
		if (spl.second != sfix)  return false;
		if (rule.defined()) return validateRuleLine(spl.first, rule);
		return true;

	}
	else {
		StrViewA txt = subject.getString();
		StrViewA str = sfix.getString();
		if (txt.length < str.length) return false;
		std::size_t pos = txt.length - str.length;
		if (txt.substr(pos) != str) return false;
		if (args.size() > 2) return validateRuleLine(txt.substr(0, pos), rule);		
		return true;
	}
}

bool Validator::opTuple(const Value& subject, const Value& args,
		bool varTuple) {

	if (subject.type() != array) return false;
	std::size_t cnt = args.size() - 1;
	if (varTuple && cnt) cnt--;
	for (std::size_t i = 0; i < cnt; i++) {

		StackSave<const Path *> store(curPath);
		Path nxtPath(*curPath, i);
		curPath = &nxtPath;
		
		Value v = subject[i];
		if (!validateRuleLine(v, args[i + 1])) {
			return false;
		}
	}
	if (varTuple) {

		if (subject.size() <= cnt) {

			StackSave<const Path *> store(curPath);
			Path nxtPath(*curPath, cnt);
			curPath = &nxtPath;

			Value v = subject[cnt];

			if (!validateRuleLine(v, undefined))
				return false;
		}
		else for (std::size_t i = cnt; i < subject.size();++i) {

			StackSave<const Path *> store(curPath);
			Path nxtPath(*curPath, i);
			curPath = &nxtPath;

			Value v = subject[cnt];

			if (!validateRuleLine(v, subject[i]))
				return false;

		}

	}
	return true;
}

}
