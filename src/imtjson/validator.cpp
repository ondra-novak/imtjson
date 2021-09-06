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

std::string_view Validator::strString = "string";
std::string_view Validator::strNumber = "number";
std::string_view Validator::strBoolean = "boolean";
std::string_view Validator::strAny = "any";
std::string_view Validator::strBase64 = "base64";
std::string_view Validator::strBase64url = "base64url";
std::string_view Validator::strHex = "hex";
std::string_view Validator::strUppercase = "uppercase";
std::string_view Validator::strLowercase = "lowercase";
std::string_view Validator::strIdentifier = "identifier";
std::string_view Validator::strCamelCase = "camelcase";
std::string_view Validator::strAlpha = "alpha";
std::string_view Validator::strAlnum = "alnum";
std::string_view Validator::strDigits = "digits";
std::string_view Validator::strInteger = "integer";
std::string_view Validator::strUnsigned = "unsigned";
std::string_view Validator::strNative = "native";
std::string_view Validator::strMinSize = "minsize";
std::string_view Validator::strMaxSize = "maxsize";
std::string_view Validator::strKey = "key";
std::string_view Validator::strToString = "tostring";
std::string_view Validator::strToNumber = "tonumber";
std::string_view Validator::strPrefix = "prefix";
std::string_view Validator::strSuffix = "suffix";
std::string_view Validator::strSplit = "split";
std::string_view Validator::strExplode = "explode"; ///< ["explode","delimiter",rule,limit]
std::string_view Validator::strNull = "null";
std::string_view Validator::strOptional = "optional";
std::string_view Validator::strUndefined = "undefined";
std::string_view Validator::strObject = "object";
std::string_view Validator::strArray= "array";
std::string_view Validator::strGreater = ">";
std::string_view Validator::strGreaterEqual = ">=";
std::string_view Validator::strLess = "<";
std::string_view Validator::strLessEqual = "<=";
std::string_view Validator::strAll = "all";
std::string_view Validator::strAndSymb = "^";
std::string_view Validator::strNot = "not";
std::string_view Validator::strNotSymb = "!";
std::string_view Validator::strDateTime = "datetime";
std::string_view Validator::strDateTimeZ = "datetimez";
std::string_view Validator::strDate = "date";
std::string_view Validator::strTimeZ = "timez";
std::string_view Validator::strTime = "time";
std::string_view Validator::strSetVar = "setvar";
std::string_view Validator::strUseVar = "usevar";
std::string_view Validator::strEmit = "emit";
std::string_view Validator::strEmpty = "empty";
std::string_view Validator::strNonEmpty = "nonempty";

char Validator::valueEscape = '\'';
char Validator::charSetBegin = '[';
char Validator::charSetEnd = ']';
char Validator::commentEscape = '#';


bool Validator::validate(const Value& subject, const std::string_view& rule, const Path &path) {

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
	return Value(array,rejections.begin(),rejections.end());
}

template<typename T>
class StackSave {
public:
	T value;
	T &var;
	StackSave(T &var):value(var),var(var) {}
	~StackSave() {var = value;}
};

static bool checkMaxSize(const Value &subject, std::size_t sz) {
	switch (subject.type()) {
	case array:
	case object: return subject.size() <= sz;
	case string: return subject.getString().size()<= sz;
	default:return true;
	}
}

static bool checkMinSize(const Value &subject, std::size_t sz) {
	switch (subject.type()) {
	case array:
	case object: return subject.size() >= sz;
	case string: return subject.getString().size() >= sz;
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
	std::string_view str = v.getString();
	if (str.empty()) return false;
	for (auto &v : str) {
		if (!isxdigit(v)) return false;
	}
	return true;
}

static bool opBase64(const Value &v) {
	std::string_view str = v.getString();
	if (str.empty()) return false;
	std::size_t len = str.size();
	if (len % 4 != 0) return false;
	if (str[len - 1] == '=') len--;
	if (str[len - 1] == '=') len--;
	for (std::size_t i = 0; i < len; i++) {
		char c = str[i];
		if (!isalnum(c) && c != '/' && c != '+') return false;
	}
	return true;
}

static bool opBase64url(const Value &v) {
	std::string_view str = v.getString();
	if (str.empty()) return false;
	std::size_t len = str.size();
	if (len % 4 == 1) return false;
	for (std::size_t i = 0; i < len; i++) {
		char c = str[i];
		if (!isalnum(c) && c != '-' && c != '_') return false;
	}
	return true;
}

static bool opUppercase(const Value &v) {
	std::string_view str = v.getString();
	if (str.empty()) return false;
	for (auto &v : str) {
		if (v < 'A' || v>'Z') return false;
	}
	return true;
}

static bool opLowercase(const Value &v) {
	std::string_view str = v.getString();
	if (str.empty()) return false;
	for (auto &v : str) {
		if (v < 'a' || v>'z') return false;
	}
	return true;
}

static bool opAlpha(const Value &v) {
	std::string_view str = v.getString();
	if (str.empty()) return false;
	for (auto &v : str) {
		if (!isalpha(v)) return false;
	}
	return true;
}

static bool opAlnum(const Value &v) {
	std::string_view str = v.getString();
	if (str.empty()) return false;
	for (auto &v : str) {
		if (!isalnum(v)) return false;
	}
	return true;
}

static bool opDigits(const Value &v) {
	std::string_view str = v.getString();
	if (str.empty()) return false;
	for (auto &v : str) {
		if (!isdigit(v)) return false;
	}
	return true;
}


/*static bool opCamelcase(const Value &v) {
	std::string_view str = v.getString();
	for (std::size_t i = 0; i < str.length; i++) {
		char c = str[i];
		if (!isalpha(c) || (i == 0 && c < 'a')) return false;
	}
	return true;
}*/

static bool opIsIdentifier(const Value &v) {
	std::string_view str = v.getString();
	for (std::size_t i = 0; i < str.size(); i++) {
		char c = str[i];
		if ((!isalnum(c) || (i == 0 && !isalpha(c))) && c != '_') return false;
	}
	return true;
}

static bool opIsInteger(const Value &v) {
	return v.type() == number && ((v.flags() & (numberInteger | numberUnsignedInteger)) != 0);
}

static bool opIsUnsigned(const Value &v) {
	return v.type() == number && v.getNumber()>=0;
}




static bool checkDateTimeGen(const std::string_view &format, const std::string_view &text) {
	if (format.size() != text.size()) return false;

	unsigned int D = 0, //day
	M = 0, //month
	Y = 0, //year
	h = 0, //hour
	m = 0, //minute
	s = 0, //second
//	c = 0, //milisec
	O = 0, //offset hour
	o = 0; //offset minute

	bool hasD = false, hasM = false, hasY = false;

	for (std::size_t cnt = format.size(), i  = 0; i < cnt; i++) {
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

	if (hasY && Y < 1000) return false;
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

bool Validator::validateInternal(const Value& subject, const std::string_view& rule) {


	Value r = def[rule];
	return evalRule(subject, r);
}

bool Validator::evalRuleSubObj(const Value& subject, const Value& rule, const std::string_view &key) {
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
			std::string_view token = fn.getString();
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
			if (token == strExplode) {
				return opExplode(subject, getVar(rule[1],subject).getString(),rule[2],rule[3]);
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
			std::string_view name = v.getString();
			if (name.substr(0, 1) == std::string_view(&commentEscape, 1))
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

static bool opTestCharset(const std::wstring &subject, const std::wstring &rule) {

	using StrViewW = std::wstring_view;


	StrViewW subj(subject);
	StrViewW r(rule);
	r = r. substr(1, r.size()-2);
	if (r.empty()) return subject.empty();
	if (subj.empty()) return false;
	bool neg = false;
	if (r[0] == '^' && r.size()>1) {
		neg = !neg;
		r = r.substr(1);
		if (r.empty()) return !subject.empty();
	}
	std::size_t adjlen = r.size()-2;
	for (auto &&x: subj) {
		bool found = false;
		for (std::size_t i = 0; i < adjlen; i++) {
			if (r[i+1] == '-') {
				if (x >= r[i] && x <= r[i+2]) {
					found = true;
					break;
				}
				i = i+2;
			} else {
				if (x == r[i]) {
					found = true;
					break;
				}
			}
		}
		if (!found) {
			for (std::size_t i = adjlen; i < r.size();i++) {
				if (x == r[i]) {
					found = true;
					break;
				}
			}
		}
		if (found == neg) return false;
	}
	return true;

}

static bool opIsEmpty(const Value &subject) {
	switch (subject.type()) {
		case json::object:
		case json::array: return subject.empty();
		case json::string: return subject.getString().empty();
		default: return false;
	}
}
static bool opIsNonEmpty(const Value &subject) {
	switch (subject.type()) {
		case json::object:
		case json::array: return !subject.empty();
		case json::string: return !subject.getString().empty();
		default: return false;
	}
}

bool Validator::evalRuleSimple(const Value& subject, const Value& rule) {
	std::string_view name = rule.getString();



	if (name.empty()) {
		return false;
	}
	else if (name[0] == valueEscape) {
		return subject.getString() == name.substr(1);
	}
	else if (name[0] == commentEscape) {
		return false;
	}
	else if (name[0] == charSetBegin && name[name.size()-1] == charSetEnd) {
		return subject.type() == string && opTestCharset(String(subject).wstr(),String(rule).wstr());
	}else if (name == strString) {
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
	else if (name == strOptional || name == strUndefined) {
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
		return opIsInteger(subject);
	}
	else if (name == strUnsigned) {
		return opIsUnsigned(subject);
	}
	else if (name == strIdentifier) {
		return opIsIdentifier(subject.toString());
	}
	else if (name == strEmpty) {
		return opIsEmpty(subject);
	}
	else if (name == strNonEmpty) {
		return opIsNonEmpty(subject);
	}
	else if (name == strObject) {
		return subject.type() == json::object;
	}
	else if (name == strArray) {
		return subject.type() == json::array;
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
static std::string_view parseKey(const std::string_view &key, bool &isProp) {
	if (key == "%") {
		isProp = true;
		return std::string_view();
	}
	else {
		isProp = false;
		for (unsigned int i = 0; i < key.size(); i++) {
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
//		bool notMatch = false;
		//extra fields are put there
		std::vector<Value> extra;
		//reserve space, there cannot be more than count items in the object
		extra.reserve(subject.size());


		auto siter = subject.begin();
		auto send = subject.end();
		auto titer = templateObj.begin();
		auto tend = templateObj.end();

		Value wildRules = templateObj["%"];

		while (siter != send && titer != tend) {
			Value s = *siter;
			Value t = *titer;
			bool isSpec;
			std::string_view sk = s.getKey();
			std::string_view tk = parseKey(t.getKey(),isSpec);
			std::string_view k;
			if (sk < tk) {
				//sk is not in template
				t = wildRules;
				++siter;
				k = sk;
			} else if (sk > tk) {
				//sk is undefined
				s = Value();
				++titer;
				k = tk;
			} else {
				++titer;
				++siter;
				k = sk;
			}
			if (isSpec) continue;

			bool match = evalRuleSubObj(s,t,k);
			if (!match) {
				addRejection(*curPath/k, t);
				return false;
			}
		}
		while (siter != send) {
			Value s = *siter;
			++siter;
			std::string_view k = s.getKey();
			bool match = evalRuleSubObj(s,wildRules, k);
			if (!match) {
				addRejection(*curPath/k, wildRules);
				return false;
			}
		}
		while (titer != tend) {
			Value t = *titer;
			++titer;
			std::string_view k = t.getKey();
			bool match = evalRuleSubObj(undefined,t, k);
			if (!match) {
				addRejection(*curPath/k, t);
				return false;
			}
		}
		return true;
	}

}
bool Validator::checkClass(const Value& subject, std::string_view name) {
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
		std::string_view txt = subject.getString();
		std::string_view str = pfix.getString();
		if (txt.substr(0, str.size()) != str) return false;
		if (rule.defined()) return evalRuleAlternatives(txt.substr(str.size()), args,2);
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
		std::string_view txt = subject.getString();
		std::string_view str = sfix.getString();
		if (txt.size() < str.size()) return false;
		std::size_t pos = txt.size() - str.size();
		if (txt.substr(pos) != str) return false;
		if (args.size() > 2) return evalRuleAlternatives(txt.substr(0, pos), args,2);		
		return true;
	}
}


bool Validator::opSplit(const Value& subject, std::size_t at, const Value& left, const Value& right) {
	Value::TwoValues s = subject.splitAt((int)at);
	return evalRule(s.first,left) && evalRule(s.second,right);
}
bool Validator::opExplode(const Value& subject, std::string_view str, const Value& rule, const Value& limit) {

	std::string_view subtxt = subject.getString();
	if (limit.defined()) {
		unsigned int l = limit.getUInt();
		if (l) {
			Array newsubj;
			Split splt(subtxt, str, l);
			while (!!splt) {
				std::string_view part = splt();
				newsubj.push_back(part);
			}
			return evalRule(newsubj,rule);
		}
	}
	{
		Array newsubj;
		Split splt(subtxt, str);
		while (!!splt) {
			std::string_view part = splt();
			newsubj.push_back(part);
		}
		return evalRule(newsubj,rule);
	}
}




void Validator::addRejection(const Path& path, const Value& rule) {
	if (!rule.defined())
		return addRejection(path,"undefined");
	if (rule == lastRejectedRule) return;
	for (Value v : rule) {
		if (v.isCopyOf(lastRejectedRule)) {
			lastRejectedRule = rule;
			return;
		}
	}
	Value vp = path.toValue();
	rejections.push_back({ vp, rule });

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

Value Validator::findVar(const std::string_view & name, const Value &thisVar)
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
	return Value(array,emits.begin(), emits.end());
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

