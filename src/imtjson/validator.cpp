/*
 * validator.cpp
 *
 *  Created on: 15. 12. 2016
 *      Author: ondra
 */

#include <cstdarg>
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
StrViewA Validator::strBase64url = "base64url";
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
StrViewA Validator::strAndSymb = "^";
StrViewA Validator::strNot = "not";
StrViewA Validator::strNotSymb = "!";
StrViewA Validator::strDateTime = "datetime";
StrViewA Validator::strDateTimeZ = "datetimez";
StrViewA Validator::strDate = "date";
StrViewA Validator::strTimeZ = "timez";
StrViewA Validator::strTime = "time";
StrViewA Validator::strSetVar = "setvar";
StrViewA Validator::strUseVar = "usevar";
StrViewA Validator::strEmit = "emit";

char Validator::valueEscape = '\'';
char Validator::commentEscape = '#';


bool Validator::validate(const Value& subject, const StrViewA& rule, const Path &path) {

	Value ver = def["_version"];
	if (ver.defined() && ver.getNumber() != 1.0) {
		addRejection(Path::root/"_version", ver);
		return false;
	}

	rejections.clear();
	emits.clear();
	curPath = &path;
	return validateInternal(subject,rule);
}

Validator::Validator(const Value& definition):def(definition),curPath(nullptr) {

}

Value Validator::getRejections() const {
	return StringView<Value>(rejections);
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
	if (str.empty()) return false;
	if (str.length % 4 != 0) return false;
	std::size_t len = str.length;
	if (str[len - 1] == '=') len--;
	if (str[len - 1] == '=') len--;
	for (std::size_t i = 0; i < len; i++) {
		char c = str[i];
		if (!isalnum(c) && c != '/' && c != '+') return false;
	}
	return true;
}

static bool opBase64url(const Value &v) {
	StrViewA str = v.getString();
	if (str.empty()) return false;
	if (str.length % 4 != 0) return false;
	std::size_t len = str.length;
	if (str[len - 1] == '=') len--;
	if (str[len - 1] == '=') len--;
	for (std::size_t i = 0; i < len; i++) {
		char c = str[i];
		if (!isalnum(c) && c != '-' && c != '_') return false;
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




static bool checkDateTimeGen(const StrViewA &format, const StrViewA &text) {
	if (format.length != text.length) return false;

	unsigned int D = 0, //day
	M = 0, //month
	Y = 0, //year
	h = 0, //hour
	m = 0, //minute
	s = 0, //second
	c = 0, //milisec
	O = 0, //offset hour
	o = 0; //offset minute

	bool hasD = false, hasM = false, hasY = false;

	for (std::size_t cnt = format.length, i  = 0; i < cnt; i++) {
		char c = format[i];
		char d = text[i];
		char ac = d - '0';
		bool digit = true;
		switch (c) {
		case 'D': D = (D * 10) + ac; hasD = true;break;
		case 'M': M = (M * 10) + ac; hasM = true;break;
		case 'Y': Y = (Y * 10) + ac; hasY = true;break;
		case 'h': h = (h * 10) + ac; break;
		case 'm': m = (m * 10) + ac; break;
		case 's': s = (s * 10) + ac; break;
		case 'c': c = (c * 10) + ac; break;
		case 'O': O = (O * 10) + ac; break;
		case 'o': o = (O * 10) + ac; break;
		case '+': if (d != '-' && d != '+') return false;
				  digit = false;
				  break;
		default: if (d != c) return false;
 				  digit = false;
			      break;

		}
		if (digit && !isdigit(d)) return false;
	}

	if (hasM && (M == 0 || M > 12)) return false;
	if (hasD && hasM) {
		const unsigned int days[2][12] = {
				{31,28,31,30,31,30,31,31,30,31,30,31},
				{31,29,31,30,31,30,31,31,30,31,30,31}
				};
		bool isleap = ((Y % 4 == 0 && Y % 100 != 0) || ( Y % 400 == 0));
		const unsigned int *d = days[isleap?1:0];
		if (D == 0 || D > d[M-1]) return false;
	}
	if (h>23 || m>59 || s>59) return false;
	if (O>23 || o>59) return false;
	return true;
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
	std::size_t rejsz = rejections.size();
	StackSave<const Path *> _(curPath);
	Path newPath(*curPath, index);
	curPath = &newPath;
	bool b = evalRuleAlternatives(subject, rule, offset);
	if (!b) {
		addRejection(*curPath, rule);
		return false;
	}
	else {
		rejections.resize(rejsz);
		return true;
	}
}


bool Validator::evalRule(const Value& subject, const Value& rule) {
	
	std::size_t rejsz = rejections.size();
	bool res;
	switch (rule.type()) {
	case object: res = evalRuleObject(subject, rule); break;
	case array: res = evalRuleWithParams(subject, rule); break;
	case string: res = evalRuleSimple(subject, rule); break;
	case undefined: res = false; break;
	default: 
		return subject == rule;
	}

	if (res == false) {
		addRejection(*curPath, rule);
	} else {
		rejections.resize(rejsz);
	}
	return res;

}

bool Validator::evalRuleWithParams(const Value& subject, const Value& rule) {
	if (rule.empty()) {
		return subject.type() == array && subject.empty();
	}
	else {
		Value fn = rule[0];
		if (fn.type() == array) {
			if (fn.empty()) {
				return evalRuleArray(subject, rule, 0,1);
			}
			else if (fn.size() == 1 && fn[0].type() == number) {
				return evalRuleArray(subject, rule, fn[0].getInt(),1);
			}
			else {
				return evalRuleAlternatives(subject, rule, 0);
			}
		}
		else if (fn.type() == string) {
			StrViewA token = fn.getString();
			if (token == strMinSize) {
				return checkMinSize(subject, getVar(rule[1],subject).getUInt());
			}
			if (token == strMaxSize) {
				return checkMaxSize(subject, getVar(rule[1],subject).getUInt());
			}
			if (token == strKey) {
				Value newSubj = subject.getKey();
				return evalRuleAlternatives(newSubj, rule, 1);
			}
			if (token == strToString) {
				Value newSubj = subject.toString();
				return evalRuleAlternatives(newSubj, rule, 1);
			}
			if (token == strToNumber) {
				Value newSubj = subject.getNumber();
				return evalRuleAlternatives(newSubj, rule, 1);
			}
			if (token == strPrefix) {
				return opPrefix(subject, rule);
			}
			if (token == strSuffix) {
				return opSuffix(subject, rule);
			}
			if (token == strSplit) {
				return opSplit(subject, getVar(rule[1],subject).getUInt(),rule[2],rule[3]);
			}
			if (token == strGreater || token == strGreaterEqual || token == strLess || token == strLessEqual ) {
				return opRangeDef(subject, rule, 0);
			}
			if ( token == strAll || token == strAndSymb) {
				return opRangeDef(subject, rule, 1);
			}
			if (token == strDateTime) {
				return checkDateTimeGen(getVar(rule[1],subject).getString(),subject.toString());
			}
			if (token == strNot || token == strNotSymb) {
				return !evalRuleAlternatives(subject, rule, 1);
			}
			if (token == strSetVar) {
				return opSetVar(subject, rule);
			}
			if (token == strUseVar) {
				return opUseVar(subject, rule);
			}
			if (token == strEmit) {
				return opEmit(subject, rule);
			}
			if (findVar(token,subject).defined()) {
				return opCompareVar(subject,rule);
			}
			return evalRuleAlternatives(subject, rule, 0);
			
		}
		else {
			return evalRuleAlternatives(subject, rule, 0);
		}
	}
}

bool Validator::opRangeDef(const Value& subject, const Value& rule, std::size_t offset) {
	std::size_t pos = offset;
	std::size_t cnt = rule.size();
	while (pos < cnt) {
		Value v = rule[pos++];
		if (v.type() == string) {
			StrViewA name = v.getString();
			if (name.substr(0, 1) == StrViewA(&commentEscape, 1)) 
				continue;
			if (name == strLess) {
				Value p = getVar(rule[pos++],subject);
				if (p.type() != subject.type() || !isLess(subject, p)) return false; // subject >= p
			}
			else if (name == strGreaterEqual) {
				Value p = getVar(rule[pos++],subject);
				if (p.type() != subject.type() || isLess(subject, p)) return false; //  subject < p
			} 
			else if (name == strGreater) {
				Value p = getVar(rule[pos++],subject);
				if (p.type() != subject.type() || !isLess(p, subject)) return false; // subject <= p
			}
			else if (name == strLessEqual) {
				Value p = getVar(rule[pos++],subject);
				if (p.type() != subject.type() || isLess(p, subject)) return false; //subject > p
			} 
			else {
				if (!evalRule(subject, v)) return false;
			}
		}
		else {
			
			if (!evalRule(subject, v)) return false;
		}
	}
	return true;

}


bool Validator::evalRuleArray(const Value& subject, const Value& rule, unsigned int tupleCnt, unsigned int offset) {
	if (subject.type() != array) return false;

	for (unsigned int i = 0; i < tupleCnt; i++) {
		if (!evalRuleSubObj(subject[i], rule[i + offset], i)) return false;
	}
	if (subject.size() >= tupleCnt) {
		for (std::size_t i = tupleCnt; i < subject.size(); ++i) {
			if (!evalRuleSubObj(subject[i], rule, i, tupleCnt + offset)) return false;
		}
	}
	return true;
}

bool Validator::evalRuleAlternatives(const Value& subject, const Value& rule, unsigned int offset) {
	unsigned int pos = offset;
	unsigned int cnt = rule.size();
	while (pos < cnt) {
		Value v = rule[pos++];
		if (evalRule(subject, v)) return true;		
	}
	return false;
}

bool Validator::evalRuleSimple(const Value& subject, const Value& rule) {
	StrViewA name = rule.getString();



	if (name.empty()) {
		return false;
	}
	else if (name.data[0] == valueEscape) {
		return subject.getString() == name.substr(1);
	}
	else if (name.data[0] == commentEscape) {
		return false;
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
	else if (name == strBase64url) {
		return opBase64url(subject.toString());
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
	else if (name == strDateTime || name == strDateTimeZ) {
		return checkDateTimeGen("YYYY-MM-DDThh:mm:ssZ", subject.toString());
	}
	else if (name == strDate) {
		return checkDateTimeGen("YYYY-MM-DD", subject.toString());
	}
	else if (name == strTime) {
		return checkDateTimeGen("hh:mm:ss", subject.toString());
	}
	else if (name == strTimeZ) {
		return checkDateTimeGen("hh:mm:ssZ", subject.toString());
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
				if (notMatch) addRejection(*curPath/rk,undefined);
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
				if (notMatch) addRejection(*curPath/rk,right);
				return 1;
			}
			else {
				notMatch = !evalRuleSubObj(left, right, lk);
				if (notMatch) addRejection(*curPath/lk,right);
				return 0;
			}
		});
		//when notMatch?
		if (notMatch)
			//report unsuccess
			return false;
		//if extra rules are not defined
		if (!extraRules.defined()) {
			//succes only if extra is empty
			if (extra.empty()) return true;
			addRejection(*curPath/extra[0].getKey(),"undefined");
			return false;
		}
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
	Value pfix = getVar(args[1],subject);
	Value rule = args[2];
	if (pfix.type() == array) {
		auto spl = subject.splitAt((int)pfix.size());
		if (spl.first != pfix)  return false;
		if (rule.defined()) return evalRuleAlternatives(spl.second, args,2);
		return true;
	}
	else {
		StrViewA txt = subject.getString();
		StrViewA str = pfix.getString();
		if (txt.substr(0, str.length) != str) return false;
		if (rule.defined()) return evalRuleAlternatives(txt.substr(str.length), args,2);
		return true;
	}

}

bool Validator::opSuffix(const Value& subject, const Value& args) {
	Value sfix = getVar(args[1],subject);
	Value rule = args[2];
	if (sfix.type() == array) {
		auto spl = subject.splitAt(-(int)sfix.size());
		if (spl.second != sfix)  return false;
		if (rule.defined()) return evalRuleAlternatives(spl.first, args,2);
		return true;

	}
	else {
		StrViewA txt = subject.getString();
		StrViewA str = sfix.getString();
		if (txt.length < str.length) return false;
		std::size_t pos = txt.length - str.length;
		if (txt.substr(pos) != str) return false;
		if (args.size() > 2) return evalRuleAlternatives(txt.substr(0, pos), args,2);		
		return true;
	}
}


bool Validator::opSplit(const Value& subject, std::size_t at, const Value& left, const Value& right) {
	Value::TwoValues s = subject.splitAt((int)at);
	return evalRule(s.first,left) && evalRule(s.second,right);
}




void Validator::addRejection(const Path& path, const Value& rule) {
	if (rule == lastRejectedRule) return;
	for (Value v : rule) {
		if (v.isCopyOf(lastRejectedRule)) {
			lastRejectedRule = rule;
			return;
		}
	}
	Value vp = path.toValue();
	if (rule.defined())
		rejections.push_back({ vp, rule });
	else
		rejections.push_back({ vp, "undefined" });

	lastRejectedRule = rule;
}

void Validator::pushVar(String name, Value value)
{
	varList.push_back(Value(name,value));
}

void Validator::popVar()
{
	if (!varList.empty()) varList.pop_back();
}

Value Validator::findVar(const StrViewA & name, const Value &thisVar)
{
	if (name == "$this") return thisVar;
	std::size_t cnt = varList.size();
	while (cnt > 0) {
		cnt--;
		if (varList[cnt].getKey() == name) return varList[cnt];
	}
	return undefined;
}

Value Validator::getVar(const Value & path, const Value &thisVar)
{
	if (path.type() == array && !path.empty()) {
		Value n = path[0];
		if (n.type() == string) {

			Value x = findVar(n.getString(),thisVar);
			if (x.defined()) {

				std::size_t cnt = path.size();
				for (std::size_t i = 1; i < cnt; i++) {
					Value p = path[i];
					switch (p.type()) {
					case string: x = x[p.getString()]; break;
					case number: x = x[p.getUInt()]; break;
					case array: {
						p = getVar(p,thisVar);
						switch (p.type()) {
						case string: x = x[p.getString()]; break;
						case number: x = x[p.getUInt()]; break;
						default: return path;
						}

					} break;
					default: return path;
					}
				}
				return x;
			}
		}
	}
	return path;
}

bool Validator::opSetVar(const Value& subject, const Value& args) {
	Value var = args[1];
	if (var.getString().substr(0,1) == "$") {
		pushVar(String(var), subject);
		bool res= evalRuleAlternatives(subject, args,2);
		popVar();
		return res;
	} else {
		return false;
	}
}

bool Validator::opUseVar(const Value& subject, const Value& args) {
	Value id = args[1];
	Value var = id.type() == array?getVar(id,subject):findVar(id.getString(),subject);
	if (var.defined()) return evalRuleAlternatives(var, args, 2);
	else return false;
}

void Validator::setVariables(const Value& varList) {
	this->varList.clear();
	this->varList.reserve(varList.size());
	for (auto &&v : varList) this->varList.push_back(v);
}

bool Validator::opCompareVar(const Value &subject, const Value &rule) {
	return subject == getVar(rule,subject);
}

Value Validator::getEmits() const {
	return StringView<Value>(emits);
}

Value Validator::walkObject(const Value &subject, const Value &v) {
	switch (v.type()) {
	case array: {
		Value z = getVar(v,subject);
		if (!z.isCopyOf(v)) return z;
		Array o;
		o.reserve(v.size());
		for(Value x:v) o.push_back(walkObject(subject,x));
		return o;
	}
	case object:{
		Object o;
		for(Value x:v) o.set(x.getKey(),walkObject(subject,x));
		return o;
	}
	default: return v;
	}
}

bool Validator::opEmit(const Value& subject, const Value& args) {
	bool r = evalRule(subject, args[2]);
	if (r) {
		emits.push_back(walkObject(subject,args[1]));
	}
	return r;
}

}

