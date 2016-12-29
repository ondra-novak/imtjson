/*
 * validator.cpp
 *
 *  Created on: 15. 12. 2016
 *      Author: ondra
 */

#include "validator.h"
#include "array.h"
#include "string.h"
#include "operations.h"

namespace json {

StrViewA Validator::strString = "string";
StrViewA Validator::strNumber = "number";
StrViewA Validator::strBoolean = "boolean";
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
StrViewA Validator::strGreater = ">";
StrViewA Validator::strGreaterEqual = ">=";
StrViewA Validator::strLess = "<";
StrViewA Validator::strLessEqual = "<=";
StrViewA Validator::strAll = "all";
char Validator::valueEscape = '\'';


bool Validator::validate(const Value& subject, const StrViewA& rule, const Path &path) {
	rejections.clear();
	curPath = &path;
	return validateInternal(subject,rule);
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

static bool checkMaxSize(const Value &subject, std::size_t sz) {
	switch (subject.type()) {
	case array:
	case object: return subject.size() <= sz;
	case string: return subject.getString().length <= sz;
	default:return true;
	}
}

static bool checkMinSize(const Value &subject, std::size_t sz) {
	switch (subject.type()) {
	case array:
	case object: return subject.size() >= sz;
	case string: return subject.getString().length >= sz;
	default:return true;
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


static bool opHex(const Value &v) {
	StrViewA str = v.getString();
	for (auto &v : str) {
		if (!isxdigit(v)) return false;
	}
	return true;
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
		if (!isalnum(c) && c != '_' && c != '-' && c != '/' && c != '+') return false;
	}
	return true;
}

static bool opUppercase(const Value &v) {
	StrViewA str = v.getString();
	for (auto &v : str) {
		if (v < 'A' || v>'Z') return false;
	}
	return true;
}

static bool opLowercase(const Value &v) {
	StrViewA str = v.getString();
	for (auto &v : str) {
		if (v < 'a' || v>'z') return false;
	}
	return true;
}

static bool opAlpha(const Value &v) {
	StrViewA str = v.getString();
	for (auto &v : str) {
		if (!isalpha(v)) return false;
	}
	return true;
}

static bool opAlnum(const Value &v) {
	StrViewA str = v.getString();
	for (auto &v : str) {
		if (!isalnum(v)) return false;
	}
	return true;
}

static bool opDigits(const Value &v) {
	StrViewA str = v.getString();
	for (auto &v : str) {
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



bool Validator::validateInternal(const Value& subject, const StrViewA& rule) {


	Value r = def[rule];
	return evalRule(subject, r);
}

bool Validator::evalRuleSubObj(const Value& subject, const Value& rule, const StrViewA &key) {
	StackSave<const Path *> _(curPath);
	Path newPath(*curPath, key);
	curPath = &newPath;
	return evalRule(subject, rule);
}
bool Validator::evalRuleSubObj(const Value& subject, const Value& rule, unsigned int index) {
	StackSave<const Path *> _(curPath);
	Path newPath(*curPath, index);
	curPath = &newPath;
	return evalRule(subject, rule);
}
bool Validator::evalRuleSubObj(const Value& subject, const Value& rule, unsigned int index, unsigned int offset) {
	StackSave<const Path *> _(curPath);
	Path newPath(*curPath, index);
	curPath = &newPath;
	int i = evalRuleAlternatives(subject, rule, offset, false);
	if (i < 1) {
		addRejection(*curPath, rule);
		return false;
	}
	else {
		return true;
	}
}


bool Validator::evalRule(const Value& subject, const Value& rule) {
	
	bool res;
	switch (rule.type()) {
	case object: res = evalRuleObject(subject, rule); break;
	case array: res = (evalRuleWithParams(subject, rule, false) == 1); break;
	case string: res = evalRuleSimple(subject, rule); break;
	case undefined: res = false; break;
	default: return subject == rule;
	}

	if (res == false) {
		addRejection(*curPath, rule);
	}
	return res;

}

int Validator::evalRuleWithParams(const Value& subject, const Value& rule, bool alreadyAccepted) {
	if (rule.empty()) {
		return subject.type() == array ? 1 : 0;
	}
	else {
		Value fn = rule[0];
		if (fn.type() == array) {
			if (fn.empty()) {
				return alreadyAccepted ? 0 : (evalRuleArray(subject, rule, 0) ? 1 : 0);
			}
			else if (fn.size() == 1 && fn[0].type() == number) {
				return alreadyAccepted ? 0 : (evalRuleArray(subject, rule, fn[0].getInt()) ? 1 : 0);
			}
			else {
				return evalRuleAlternatives(subject, rule, 0, alreadyAccepted);
			}
		}
		else if (fn.type() == string) {
			StrViewA token = fn.getString();
			if (token == strMinSize) {
				return checkMinSize(subject, rule[1].getUInt()) ? 0 : -1;
			}
			if (token == strMaxSize) {
				return checkMaxSize(subject, rule[1].getUInt()) ? 0 : -1;
			}
			if (token == strKey) {
				Value newSubj = subject.getKey();
				return evalRuleAlternatives(subject, rule, 1, alreadyAccepted);
			}
			if (token == strToString) {
				Value newSubj = subject.toString();
				return evalRuleAlternatives(subject, rule, 1, alreadyAccepted);
			}
			if (token == strToNumber) {
				Value newSubj = subject.getNumber();
				return evalRuleAlternatives(subject, rule, 1, alreadyAccepted);
			}
			if (token == strPrefix) {
				return alreadyAccepted ? 0 : opPrefix(subject, rule) ? 1 : 0;
			}
			if (token == strSuffix) {
				return alreadyAccepted ? 0 : opSuffix(subject, rule) ? 1 : 0;
			}
			if (token == strSplit) {
				return alreadyAccepted ? 0 : opSplit(subject, rule[1].getUInt(),rule[2],rule[3]) ? 1 : 0;
			}
			if (token == strAll) {
				for (std::size_t cnt = rule.size(), i = 1; i < cnt; i++) {
					if (!evalRule(subject, rule[i])) return 0;
				}
				return true;
			}
			return evalRuleAlternatives(subject, rule, 0, alreadyAccepted);
			
		}
		else {
			return evalRuleAlternatives(subject, rule, 0, alreadyAccepted);
		}
	}
}

bool Validator::evalRuleArray(const Value& subject, const Value& rule, unsigned int tupleCnt) {
	if (subject.type() != array) return false;

	for (unsigned int i = 0; i < tupleCnt; i++) {
		if (!evalRuleSubObj(subject[i], rule[i + 1], i)) return false;
	}
	if (subject.size() >= tupleCnt) {
		for (std::size_t i = tupleCnt; i < subject.size(); ++i) {
			if (!evalRuleSubObj(subject[i], rule, i, tupleCnt + 1)) return false;
		}
	}
	return true;
}

int Validator::evalRuleAlternatives(const Value& subject, const Value& rule, unsigned int offset, bool alreadyAccepted) {
	unsigned int pos = offset;
	unsigned int cnt = rule.size();
	while (pos < cnt) {
		Value v = rule[pos++];
		if (v.type() == array) {
			int r = evalRuleWithParams(subject, v, alreadyAccepted);
			if (r == -1) return -1;
			if (r == 1) alreadyAccepted = true;
		}
		else {
			StrViewA name = v.getString();
			if (name == strGreater) {
				Value p = rule[pos++];
				if (p.type() == subject.type()) {
					alreadyAccepted = true;
					if (!isLess(p, subject)) return -1;
				}
			}
			else if (name == strGreaterEqual) {
				Value p = rule[pos++];
				if (p.type() == subject.type()) {
					alreadyAccepted = true;
					if (isLess(subject, p)) return -1;
				}
			}
			else if (name == strLess) {
				Value p = rule[pos++];
				if (p.type() == subject.type()) {
					alreadyAccepted = true;
					if (!isLess(subject, p)) return -1;
				}
			}
			else if (name == strLessEqual) {
				Value p = rule[pos++];
				if (p.type() == subject.type()) {
					alreadyAccepted = true;
					if (isLess(p, subject)) return -1;
				}
			}
			else if (!alreadyAccepted) {
				if (evalRuleSimple(subject, v))  alreadyAccepted = true;
			}
		}
	}
	return alreadyAccepted ? 1 : 0;
}

bool Validator::evalRuleSimple(const Value& subject, const Value& rule) {
	StrViewA name = rule.getString();

	if (name.empty()) {
		return false;
	}
	else if (name.data[0] == valueEscape) {
		return subject.getString() == name.substr(1);
	}
	else if (name == strString) {
		return subject.type() == string;
	}
	else if (name == strNumber) {
		return subject.type() == number;
	}
	else if (name == strBoolean) {
		return subject.type() == boolean;
	}
	else if (name == strAny) {
		return subject.defined();
	}
	else if (name == strNull) {
		return subject.isNull();
	}
	else if (name == strOptional) {
		return !subject.defined();
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
	else {
		return checkClass(subject, name);
	}
	return false;

}





/*
If key is '%' then value contains properties for extra keys
To write '%' as key, you have to double first '%', => '%%', '%%' => '%%%' etc.
Other strings are returned unchanged
*/
static StrViewA parseKey(const StrViewA &key, bool &isProp) {
	if (key == "%") {
		isProp = true;
		return StrViewA();
	}
	else {
		isProp = false;
		for (unsigned int i = 0; i < key.length; i++) {
			if (key[i] != '%') return key;
		} 
		return key.substr(1);

	}
}

bool Validator::evalRuleObject(const Value& subject, const Value& templateObj) {
	Value extraRules;
	if (subject.type() != object) {
		return false;
	}
	else {

		//this is set to true for not-matching object
		bool notMatch = false;
		//extra fields are put there
		std::vector<Value> extra;
		//reserve space, there cannot be more than count items in the object
		extra.reserve(subject.size());

		//Merge - because member items are ordered by the key
		//we can perform merged walk through the two containers		
		subject.merge(templateObj, [&](const Value &left, const Value &right) {
			//if notMatch is signaled, we will leave this cycle quickly as we can
			if (notMatch) return 0;
			
			//detect extra properties
			bool prop;
			//retrieve subject's key
			StrViewA lk = left.getKey();
			//retrieve template's key - also handle '%'
			StrViewA rk = parseKey(right.getKey(), prop);
			//for '%' 
			if (prop) {
				//store extra rules
				extraRules = right;
				//skip this item
				return 1;
			}
			int b = 0;
			if (!left.defined() && right.defined()) {
				notMatch = !evalRuleSubObj(left, right, rk);
				return 1;
			}
			else if (left.defined() && !right.defined()) {
				extra.push_back(left);
				return -1;
			}
			else if (lk < rk) {
				extra.push_back(left);
				return -1;
			}
			else if (lk > rk) {
				notMatch = !evalRuleSubObj(undefined, right, rk);
				return 1;
			}
			else {
				notMatch = !evalRuleSubObj(left, right, lk);
				return 0;
			}
		});
		//when notMatch?
		if (notMatch)
			//report unsuccess
			return false;
		//if extra rules are not defined
		if (!extraRules.defined())
			//succes only if extra is empty
			return extra.empty();
		//for extra rules
		for (Value v : extra) {
			//process every extra field
			StackSave<const Path *> store(curPath);
			Path nxtPath(*curPath, v.getKey());
			curPath = &nxtPath;
			//test extra field against to extra rules
			if (!evalRule(v, extraRules)) 
				return false;

		}
		return true;
	}

}
bool Validator::checkClass(const Value& subject, StrViewA name) {
	Value ruleline = def[name];
	if (!ruleline.defined()) {
		throw std::runtime_error(std::string("Undefined class: ")+std::string(name));
	} else if (ruleline.getString() == strNative) {
		return onNativeRule(subject,name);
	} else {
		return validateInternal(subject,name);
	}

}
bool Validator::opPrefix(const Value& subject, const Value& args) {
	Value pfix = args[1];
	Value rule = args[2];
	if (pfix.type() == array) {
		auto spl = subject.splitAt((int)pfix.size());
		if (spl.first != pfix)  return false;
		if (rule.defined()) return evalRule(spl.second, rule);
		return true;
	}
	else {
		StrViewA txt = subject.getString();
		StrViewA str = pfix.getString();
		if (txt.substr(0, str.length) != str) return false;
		if (rule.defined()) return evalRule(txt.substr(str.length), rule);
		return true;
	}

}

bool Validator::opSuffix(const Value& subject, const Value& args) {
	Value sfix = args[1];
	Value rule = args[2];
	if (sfix.type() == array) {
		auto spl = subject.splitAt(-(int)sfix.size());
		if (spl.second != sfix)  return false;
		if (rule.defined()) return evalRule(spl.first, rule);
		return true;

	}
	else {
		StrViewA txt = subject.getString();
		StrViewA str = sfix.getString();
		if (txt.length < str.length) return false;
		std::size_t pos = txt.length - str.length;
		if (txt.substr(pos) != str) return false;
		if (args.size() > 2) return evalRule(txt.substr(0, pos), rule);		
		return true;
	}
}


bool Validator::opSplit(const Value& subject, std::size_t at, const Value& left, const Value& right) {
	Value::TwoValues s = subject.splitAt((int)at);
	return evalRule(s.first,left) && evalRule(s.second,right);
}

void Validator::addRejection(const Path& path, const Value& rule) {
	Value vp = path.toValue();
	if (!rejections.empty()) {
		Value pvp = rejections[rejections.size()-1][0];
		if (pvp.size()>vp.size() && pvp.splitAt((int)vp.size()).first == vp) return;
	}
	if (rule.defined())
		rejections.add({ vp, rule });
	else
		rejections.add({ vp, "undefined" });
}

}

