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

StrViewA Validator::strString = "string";
StrViewA Validator::strNumber = "number";
StrViewA Validator::strBoolean = "boolean";
StrViewA Validator::strArray = "array";
StrViewA Validator::strObject = "object";
StrViewA Validator::strAny = "any";
StrViewA Validator::strBase64 = "base64";
StrViewA Validator::strHex = "hex";
StrViewA Validator::strUppercase = "uppercase";
StrViewA Validator::strLowercase = "lowercase";
StrViewA Validator::strIdentifier = "identifier";
StrViewA Validator::strCamelCase = "camelcase";
StrViewA Validator::strAlpha = "alpha";
StrViewA Validator::strAlnum = "alnum";
StrViewA Validator::strDigits = "digits";
StrViewA Validator::strInteger = "integer";
StrViewA Validator::strNative = "native";
StrViewA Validator::strSet = "set";
StrViewA Validator::strEnum = "enum";
StrViewA Validator::strTuple = "tuple";
StrViewA Validator::strVarTuple = "vartuple";
StrViewA Validator::strRangeOpenOpen = "range()";
StrViewA Validator::strRangeOpenClose = "range(>";
StrViewA Validator::strRangeCloseOpen = "range<)";
StrViewA Validator::strRangeCloseClose = "range<>";
StrViewA Validator::strValue = "value";
StrViewA Validator::strMinSize = "minsize";
StrViewA Validator::strMaxSize = "maxsize";
StrViewA Validator::strKey = "key";
StrViewA Validator::strToString = "tostring";
StrViewA Validator::strToNumber = "tonumber";
StrViewA Validator::strPrefix = "prefix";
StrViewA Validator::strSuffix = "suffix";
StrViewA Validator::strSplit = "split";
StrViewA Validator::strNull = "null";
StrViewA Validator::strOptional = "optional";
StrViewA Validator::strSub = "sub";
StrViewA Validator::strShift = "shift";
char Validator::valueEscape = '\'';
char Validator::argEscape = '$';



bool Validator::validate(const Value& subject, const StrViewA& rule, const Value& args, const Path &path) {
	rejections.clear();
	curPath = &path;
	return validateInternal(subject,rule,args);
}

Validator::Validator(const Value& definition):def(definition),curPath(nullptr) {

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
			addRejection(*curPath, ruleLine);
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
			ok = validateSingleRuleForAccept(subject, x);
			if (ok) break;
		}
	}
	if (ok) {
		o = offset;
		for (auto &&x : ruleLine) {
			if (o) {
				--o;
			} else {
				ok = validateSingleRuleForReject(subject, x);
				if (!ok) break;
			}
		}
	}
	if (!ok) {
		addRejection(*curPath, ruleLine);
		return false;
	}
	return ok;
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


static bool opHex(const Value &v) {
	StrViewA str = v.getString();
	for (auto &v : str ) {
		if (!isxdigit(v)) return false;
	}
	return str.length % 2 == 0;
}

static bool opBase64(const Value &v) {
	StrViewA str = v.getString();
	if (str.length % 4 != 0) return false;
	for (std::size_t i = 0; i < str.length; i++) {
		char c = str[i];
		if (c == '=') {
			if (i - str.length > 2) return false;
			i++;
			while (i < str.length) {
				c = str[i];
				if (c != '=') return false;
				i++;
			}
			return true;

		}
		if (!isalnum(c) && c != '_' && c != '-' && c !='/' && c != '+') return false;
	}
	return true;
}

static bool opUppercase(const Value &v) {
	StrViewA str = v.getString();
	for (auto &v : str ) {
		if (v < 'A' || v>'Z') return false;
	}
	return true;
}

static bool opLowercase(const Value &v) {
	StrViewA str = v.getString();
	for (auto &v : str ) {
		if (v < 'a' || v>'z') return false;
	}
	return true;
}

static bool opAlpha(const Value &v) {
	StrViewA str = v.getString();
	for (auto &v : str ) {
		if (!isalpha(v)) return false;
	}
	return true;
}

static bool opAlnum(const Value &v) {
	StrViewA str = v.getString();
	for (auto &v : str ) {
		if (!isalnum(v)) return false;
	}
	return true;
}

static bool opDigits(const Value &v) {
	StrViewA str = v.getString();
	for (auto &v : str ) {
		if (!isdigit(v)) return false;
	}
	return true;
}


static bool opCamelcase(const Value &v) {
	StrViewA str = v.getString();
	for (std::size_t i = 0; i < str.length; i++) {
		char c = str[i];
		if (!isalpha(c) || (i == 0 && c < 'a')) return false;
	}
}

static bool opIsIdentifier(const Value &v) {
	StrViewA str = v.getString();
	for (std::size_t i = 0; i < str.length; i++) {
		char c = str[i];
		if ((!isalnum(c) || (i == 0 && !isalpha(c))) && c != '_') return false;
	}
	return true;
}

static bool opIsInteger(const Value &v) {
	return v.type() == number && ((v.flags() & (numberInteger | numberUnsignedInteger)) != 0);
}


bool Validator::evalRuleAccept(const Value& subject, StrViewA name, const Value& args) {

		if (name.empty()) {
			return false;
		} else if (name.data[0] == valueEscape ) {
			return subject.getString() == name.substr(1);
		} else if (name == strString) {
			return subject.type() == string;
		} else if (name == strNumber) {
			return subject.type() == number;
		} else if (name == strBoolean) {
			return subject.type() == boolean;
		} else if (name == strAny) {
			return subject.defined();
		} else if (name == strNull) {
			return subject.isNull();
		} else if (name == strOptional) {
			return !subject.defined();
		} else if (name == strArray) {
			if (subject.type() != array) return false;
			if (args.empty()) return true;
			unsigned int pos = 0;
			for (Value v : subject) {
				StackSave<const Path *> pathSave(curPath);
				Path newPath(*curPath, pos);
				curPath = &newPath;
				if (!validateRuleLine2(v, args, 1)) return false;
				++pos;
			}
			return true;
		} else if (name == strObject) {
			if (subject.type() != object) return false;
			if (args.empty()) return true;
			return validateObject(subject,argDeref(args[1]),argDeref(args[2]));
		} else if (name == strTuple) {
			return opTuple(subject,args,false);
		}
		else if (name == strVarTuple) {
			return opTuple(subject, args, true);
		} else if (name == strRangeOpenOpen) {
			return isLess(args[1], subject) && isLess(subject, argDeref(args[2])) ;
		} else if (name == strRangeCloseOpen) {
			return !isLess(subject, args[1])  && isLess(subject, argDeref(args[2])) ;
		} else if (name == strRangeCloseClose) {
			return !isLess(subject, args[1])  && !isLess( argDeref(args[2]), subject) ;
		} else if (name == strRangeOpenClose) {
			return isLess(args[1], subject)  && !isLess( argDeref(args[2]), subject) ;
		}
		else if (name == strValue) {
			return subject == argDeref(args);
		}
		else if (name == strSub) {
			return validateRuleLine2(subject, args, 1);
		}
		else if (name == strShift) {
			StackSave<const Value *> savedArgs(curArgs);
			Value a = curArgs->splitAt(1).second;
			curArgs = &a;
			return validateRuleLine2(subject, args, 1);
		}
		else if (name == strToString) {
			return validateRuleLine(subject.toString(), argDeref(args[1]));
		}
		else if (name == strHex) {
			return opHex(subject.toString());
		}
		else if (name == strBase64) {
			return opBase64(subject.toString());
		}
		else if (name == strUppercase) {
			return opUppercase(subject.toString());
		}
		else if (name == strLowercase) {
			return opLowercase(subject.toString());
		}
		else if (name == strAlpha) {
			return opAlpha(subject.toString());
		}
		else if (name == strAlnum) {
			return opAlnum(subject.toString());
		}
		else if (name == strDigits) {
			return opDigits(subject.toString());
		}
		else if (name == strInteger) {
			return opIsInteger(subject.toString());
		}
		else if (name == strIdentifier) {
			return opIsIdentifier(subject.toString());
		}
		else if (name == strSplit) {
			return opSplit(subject, argDeref(args[1]).getUInt(), argDeref(args[2]), argDeref(args[3]));
		}
		else if (name == strToNumber) {
			return validateRuleLine(subject.getNumber(), argDeref(args[1]));
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
		if (name == strMaxSize) {
			return checkMaxSize(subject, argDeref(args[1]).getUInt());
		} else if (name == strMinSize) {
			return checkMinSize(subject, argDeref(args[1]).getUInt());
		} else if (name == strKey) {
			return checkKey(subject, args);
		} else if (name == strPrefix) {
			return opPrefix(subject,args);
		} else if (name == strSuffix) {
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
			   ruleLine.type() == array?evalRuleAccept(subject,argDeref(ruleLine[0]).getString(),ruleLine):
			   ruleLine.type() == string?evalRuleAccept(subject,argDeref(ruleLine).getString(),{}):
			   subject == ruleLine;
}

bool Validator::validateSingleRuleForReject(const Value& subject, const Value& ruleLine) {

	return  ruleLine.type() == array?evalRuleReject(subject,argDeref(ruleLine[0]).getString(),ruleLine):
			ruleLine.type() == string?evalRuleReject(subject,argDeref(ruleLine).getString(),{}):
			true;
}

bool Validator::checkClassAccept(const Value& subject, StrViewA name, const Value& args) {
	Value ruleline = def[name];
	if (!ruleline.defined()) {
		throw std::runtime_error(std::string("Undefined class: ")+std::string(name));
	} else if (ruleline.getString() == strNative) {
		return onNativeRuleAccept(subject,name,args);
	} else {
		return validateInternal(subject,name,args);
	}

}

bool Validator::checkClassReject(const Value& subject, StrViewA name, const Value& args) {
	Value ruleline = def[name];
	if (ruleline.getString() == strNative)
		return onNativeRuleReject(subject,name,args);
	else
		return true;
}

bool Validator::opPrefix(const Value& subject, const Value& args) {
	Value pfix = args[1];
	Value rule = args[2];
	if (pfix.type() == array) {
		auto spl = subject.splitAt((int)pfix.size());
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
		auto spl = subject.splitAt(-(int)sfix.size());
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
		if (!validateRuleLine(v, argDeref(args[i + 1]))) {
			return false;
		}
	}
	if (varTuple) {

		if (subject.size() <= cnt) {

			StackSave<const Path *> store(curPath);
			Path nxtPath(*curPath, cnt);
			curPath = &nxtPath;

			Value v = argDeref(args[cnt]);

			if (!validateRuleLine(undefined,v))
				return false;
		}
		else for (std::size_t i = cnt; i < subject.size();++i) {

			StackSave<const Path *> store(curPath);
			Path nxtPath(*curPath, i);
			curPath = &nxtPath;

			Value v = argDeref(args[cnt]);

			if (!validateRuleLine(subject[i],v))
				return false;

		}
		return true;

	} else {
		return subject.size() <= cnt;
	}
}

bool Validator::opSplit(const Value& subject, std::size_t at, const Value& left, const Value& right) {
	Value::TwoValues s = subject.splitAt((int)at);
	return validateRuleLine(s.first,left) && validateRuleLine(s.second,right);
}

Value Validator::argDeref(const Value &v) const {
	if (v.type() == string) {
		StrViewA str = v.getString();
		if (!str.empty() && str[0] == argEscape) {
			unsigned int argN = 0;
			for (std::size_t i = 1; i < str.length; i++) {
				char c = str[i];
				if (c < '0' || c > '9') return str.substr(1);
				argN = argN * 10 + (c - '0');
			}
			return curArgs->operator [](argN);
		}
	}
	return v;
}

void Validator::addRejection(const Path& path, const Value& rule) {
	Value vp = path.toValue();
	if (!rejections.empty()) {
		Value pvp = rejections[rejections.size()-1][0];
		if (pvp.size()>vp.size() && pvp.splitAt((int)vp.size()).first == vp) return;
	}
	rejections.add({vp, rule});
}

}

