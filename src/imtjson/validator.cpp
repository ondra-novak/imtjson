/*
 * validator.cpp
 *
 *  Created on: 15. 12. 2016
 *      Author: ondra
 */

#include <cstdarg>
#include "validator.h"

#include <cmath>

#include "array.h"
#include "string.h"
#include "operations.h"
#include "wrap.h"

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
StrViewA Validator::strExplode = "explode"; ///< ["explode","delimiter",rule,limit]
StrViewA Validator::strNull = "null";
StrViewA Validator::strOptional = "optional";
StrViewA Validator::strUndefined = "undefined";
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
StrViewA Validator::strEmpty = "empty";
StrViewA Validator::strNonEmpty = "nonempty";

char Validator::valueEscape = '\'';
char Validator::charSetBegin = '[';
char Validator::charSetEnd = ']';
char Validator::commentEscape = '#';


 NamedEnum<Validator::Keyword> Validator::keywords({
	{Validator::kString, "string"},
	{Validator::kNumber, "number"},
	{Validator::kInteger, "integer"},
	{Validator::kUnsigned, "unsigned"},
	{Validator::kObject, "object"},
	{Validator::kArray, "array"},
	{Validator::kBoolean, "boolean"},
	{Validator::kBool, "bool"},
	{Validator::kNull, "null"},
	{Validator::kUndefined, "undefined"},
	{Validator::kAny, "any"},
	{Validator::kBase64, "base64"},
	{Validator::kBase64url,"base64url"},
	{Validator::kHex, "hex"},
	{Validator::kUpper, "upper"},
	{Validator::kLower, "lower"},
	{Validator::kIdentifier, "identifier"},
	{Validator::kCamelCase, "camelcase"},
	{Validator::kAlpha, "alpha"},
	{Validator::kAlnum, "alnum"},
	{Validator::kDigits, "digits"},
	{Validator::kAsc, "<<"},
	{Validator::kDesc,">>"},
	{Validator::kAscDup, "<<="},
	{Validator::kDescDup, ">>="},
	{Validator::kStart, "start"},
	{Validator::kRootRule,"rootrule"}
});


bool Validator::validate(const Value& subject, const StrViewA& rule, const Path &path) {

	Value ver = def["_version"];
	if (ver.defined() && ver.getNumber() != 1.0 && ver.getNumber() != 2.0) {
		addRejection(Path::root/"_version", ver);
		return false;
	}

	rejections.clear();
	emits.clear();
	curPath = &path;
	if (ver.getNumber() == 2.0) {
		return v2validate(subject, path, rule);
	} else {
		return validateInternal(subject,rule);
	}
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
	StackSave(T &var):value(var),var(var) {}
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
	if (str.empty()) return false;
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
	if (str.length % 4 == 1) return false;
	std::size_t len = str.length;
	for (std::size_t i = 0; i < len; i++) {
		char c = str[i];
		if (!isalnum(c) && c != '-' && c != '_') return false;
	}
	return true;
}

static bool opUppercase(const Value &v) {
	StrViewA str = v.getString();
	if (str.empty()) return false;
	for (auto &v : str) {
		if (v < 'A' || v>'Z') return false;
	}
	return true;
}

static bool opLowercase(const Value &v) {
	StrViewA str = v.getString();
	if (str.empty()) return false;
	for (auto &v : str) {
		if (v < 'a' || v>'z') return false;
	}
	return true;
}

static bool opAlpha(const Value &v) {
	StrViewA str = v.getString();
	if (str.empty()) return false;
	for (auto &v : str) {
		if (!isalpha(v)) return false;
	}
	return true;
}

static bool opAlnum(const Value &v) {
	StrViewA str = v.getString();
	if (str.empty()) return false;
	for (auto &v : str) {
		if (!isalnum(v)) return false;
	}
	return true;
}

static bool opDigits(const Value &v) {
	StrViewA str = v.getString();
	if (str.empty()) return false;
	for (auto &v : str) {
		if (!isdigit(v)) return false;
	}
	return true;
}


/*static bool opCamelcase(const Value &v) {
	StrViewA str = v.getString();
	for (std::size_t i = 0; i < str.length; i++) {
		char c = str[i];
		if (!isalpha(c) || (i == 0 && c < 'a')) return false;
	}
	return true;
}*/

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
//	c = 0, //milisec
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

static bool opTestCharset(const std::wstring &subject, const std::wstring &rule) {




	StrViewW subj(subject);
	StrViewW r(rule);
	r = r. substr(1, r.length-2);
	if (r.empty()) return subject.empty();
	if (subj.empty()) return false;
	bool neg = false;
	if (r[0] == '^' && r.length>1) {
		neg = !neg;
		r = r.substr(1);
		if (r.empty()) return !subject.empty();
	}
	std::size_t adjlen = r.length-2;
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
			for (std::size_t i = adjlen; i < r.length;i++) {
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
	else if (name.data[0] == charSetBegin && name[name.length-1] == charSetEnd) {
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
	else if (name == strIdentifier) {
		return opIsIdentifier(subject.toString());
	}
	else if (name == strEmpty) {
		return opIsEmpty(subject);
	}
	else if (name == strNonEmpty) {
		return opIsNonEmpty(subject);
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


		auto siter = subject.begin();
		auto send = subject.end();
		auto titer = templateObj.begin();
		auto tend = templateObj.end();

		Value wildRules = templateObj["%"];

		while (siter != send && titer != tend) {
			Value s = *siter;
			Value t = *titer;
			bool isSpec;
			StrViewA sk = s.getKey();
			StrViewA tk = parseKey(t.getKey(),isSpec);
			StrViewA k;
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
			StrViewA k = s.getKey();
			bool match = evalRuleSubObj(s,wildRules, k);
			if (!match) {
				addRejection(*curPath/k, wildRules);
				return false;
			}
		}
		while (titer != tend) {
			Value t = *titer;
			++titer;
			StrViewA k = t.getKey();
			bool match = evalRuleSubObj(undefined,t, k);
			if (!match) {
				addRejection(*curPath/k, t);
				return false;
			}
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
bool Validator::opExplode(const Value& subject, StrViewA str, const Value& rule, const Value& limit) {

	StrViewA subtxt = subject.getString();
	if (limit.defined()) {
		unsigned int l = limit.getUInt();
		if (l) {
			Array newsubj;
			auto splt = subtxt.split(str,l);
			while (!!splt) {
				StrViewA part = splt();
				newsubj.push_back(part);
			}
			return evalRule(newsubj,rule);
		}
	}
	{
		Array newsubj;
		auto splt = subtxt.split(str);
		while (!!splt) {
			StrViewA part = splt();
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

bool Validator::addRejection(const Path& path, const Value& rule, bool result) {
	if (!result) addRejection(path,rule);
	return result;
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

//------------------------------- version 2 ------------------------------------------

Validator::Stack Validator::path2stack(const Path &path, const Stack &initial) {
	if (path.isRoot()) return initial;
	else {
		Stack prev = path2stack(path.getParent(), initial);
		if (path.isIndex()) prev.push(path.getIndex()); else prev.push(path.getKey());
		return prev;
	}
}
Value Validator::stack2path(const Stack &stack) {
	Array x;
	x.reserve(stack.size());
	Stack s = stack;
	while (!s.empty()) {
		x.push_back(s.top()); s = s.pop();
	}
	x.reverse();
	return x;
}



bool Validator::v2validate(Value subj, const Path &path, StrViewA rule) {
	Value jrule = def[rule];
	rootRule = jrule;
	rootValue = subj;
	return v2processRule(State(subj, path2stack(path)),rule, false);
}

bool Validator::v2processRule(State &&state, const Value &rule, bool sequence) {
	switch (rule.type()) {
		case array:
			if (sequence) return v2processSequence(std::move(state),rule);
			else return v2processAlternative(std::move(state),rule);
		case object:
			return v2processObject(std::move(state),rule);
		case null:
			return state.subject == nullptr;
		case boolean:
			return state.subject == rule;
		case number:
			return state.subject == rule;
		case string:
			return v2processSimpleRule(std::move(state),rule);
		default:
			v2report(state,rule);
			return false;
	}
}

class Validator::V2Guard {
public:
	V2Guard(Validator *owner):owner(*owner),emitpos(owner->output.size()) {}
	~V2Guard() {
		owner.output.resize(emitpos);
	}
	bool commit() {
		emitpos = owner.output.size();return true;
	}
	V2Guard(const V2Guard &) = delete;
	V2Guard &operator=(const V2Guard &) = delete;

protected:
	Validator &owner;
	std::size_t emitpos;

};

bool Validator::v2processAlternative(State &&state, const Value &rule) {
	V2Guard g(this);
	for (Value x: rule) {
		try {
			bool r = v2processRule(State(state), x, true);
			if (r) return g.commit();
		} catch (std::exception &e) {
			v2report_exception(state, rule, e.what());
			return false;
		}
	}
	v2report(state,rule);
}

Value Validator::collapseObject(const Value &obj, const Path &seen) {


	Value parentRef = obj["^^^"];
	Value parentObj;
	Object m;
	StrViewA str;

	auto merge = [&](StrViewA str) {
		const Path *x = &seen;
		while (x != &x->root && x->getKey() != str) x = &x->getParent();
		if (x != &x->root) return ;

		m.setBaseObject(collapseObject(def[str],Path(seen,str)));
		m.unset("...");
		m.merge(obj);

	};

	switch (parentRef.type()) {
	default:
		m.setBaseObject(obj);
		break;
	case string:
		merge(parentRef.getString());
		break;
	case array:
		for (Value v: parentRef) {
			merge(v.getString());
			break;
		}
	}
	m.unset("^^^");
	return m;
}

bool Validator::v2processObject(State &&state, Value rule) {
	V2Guard g(this);


	Value crule = collapseObject(rule,Path::root);
	for (Value r: crule) {
		Value v = state.subject[v.getKey()];
		if (!v2processRule(state.enter(r.getKey()),r,false)) {
			return false;
		}
	}

	Value others = crule["..."];
	for (Value v: state.subject) {
		if (!crule[v.getKey()].defined()) {
			if (!v2processRule(state.enter(v.getKey()), others, false)) {
				return false;
			}
		}
	}
	return g.commit();
}

template<typename Fn>
static Value toXCase(const Value &v, Fn &&fn) {
	String s = v.toString();
	std::wstring w = s.wstr();
	std::transform(w.begin(),w.end(),w.begin(),fn);
	return String(w);
}


static Value doExplode(const Value &v, const Value arg) {
	String src = v.toString();
	String sep = arg.toString();
	Array res;
	auto splt = StrViewA(src).split(sep);
	while (!!splt) {
		res.push_back(splt());
	}
	return res;
}

static Value doPrefix(const Value &v, const Value arg) {
	if (v.type() == array) {
		if (arg.type() == array) {
			if (v.size() >= arg.size()) {
				if (v.slice(0,arg.size()) == arg)
					return v.slice(arg.size());
			}
		}
	} else if (v.type() == string) {
		StrViewA sv = v.getString();
		if (arg.type() == string) {
			StrViewA sa = arg.getString();
			if (sv.length >= sa.length) {
				if (sv.substr(0,sa.length) == sa)
					return sv.substr(sa.length);
			}
		}
	}
	return undefined;
}

static Value doSuffix(const Value &v, const Value arg) {
	if (v.type() == array) {
		if (arg.type() == array) {
			if (v.size() >= arg.size()) {
				auto df = v.size()-arg.size();
				if (v.slice(df) == arg)
					return v.slice(0,df);
			}
		}
	} else if (v.type() == string) {
		StrViewA sv = v.getString();
		if (arg.type() == string) {
			StrViewA sa = arg.getString();
			if (sv.length >= sa.length) {
				auto df = sv.length-sa.length;
				if (sv.substr(df) == sa)
					return sv.substr(0,df);
			}
		}
	}
	return undefined;
}

bool Validator::v2processArray(State &&state, const Value rule, ValueType vt) {
	V2Guard g(this);
	if (state.subject.type() != vt) return false;
	std::size_t idx = 0;
	for (Value v:state.subject) {
		if (!v2processRule(state.enter(idx), rule, false)) return false;
		++idx;
	}
	return g.commit();
}

bool Validator::v2processTuple(State &&state, const Value rule) {
	V2Guard g(this);
	if (state.subject.type() != array) return false;
	std::size_t cnt = state.subject.size();
	for(std::size_t i = 0; i < cnt; i++) {
		Value x = state.subject[i];
		Value r = rule[i];
		if (!v2processRule(state.enter(i), r, false)) return false;
	}
	return g.commit();
}


bool Validator::v2processSequence(State &&state, Value rule) {
	if (rule.type() != array) rule = Value(array, {rule});
	V2Guard g(this);
	std::size_t len = rule.size();
	std::size_t i = 0;
	while (i<len) {
		Value rv = rule[i++];
		try {

			if (rv.type() != string) {
				if (!v2processRule(std::move(state), rv)) return false;
			} else {
				StrViewA r = rv.getString();
				if (r.begins("//")) continue;
				if (r.begins(".")) {
					state = state.enter(r.substr(1));
				} else if (r.begins("#")) {
					state = state.enter(atol(r.substr(1).data));
				} else {
					Value arg;
					if (r.ends("()")) {
						arg = rule[i++];
						r = r.substr(0,r.length-2);
					} else if (r.ends("(:)")) {
						arg = rule[i++];
						r = r.substr(0,r.length-3);
						if (!runConversion(State(state), arg, arg)) return v2report(state, rv, false);
					} else if (r.ends("(^)")) {
						arg = rule[i++];
						r = r.substr(0,r.length-3);
						arg = doMap(State(state), arg);
						if (!arg.defined()) return v2report(state,rv,false);
					}


					if (r.begins(":")) {

						StrViewA simple_rule_name = r;
						//conversion
						r = r.substr(1);
						Value out;
						if (r.begins("/")) {
							r = r.substr(1);
							if (!extractRegExpr(r,arg.toString(),out)) return v2report(state,rv,false);
						} else {
							auto k = keywords.find(r);
							auto kk = k?*k:kUnknown;
							switch (kk) {
								case kLength:
								case kLen:  out = state.subject.type() == string
														?state.subject.getString().length
														:state.subject.size();
											break;
								case kKey: out = state.getKey(); break;
								case kString: out = state.subject.toString();break;
								case kStringify: out = state.subject.stringify();break;
								case kParse:out = Value::fromString(state.subject.getString());break;
								case kUpper: out = toXCase(state.subject, towupper);break;
								case kLower: out = toXCase(state.subject, towlower);break;
								case kSign: out = state.subject.getNumber()<0?-1:state.subject.getNumber()>0?1:0;break;
								case kNeg: out = -state.subject.getNumber();break;
								case kSum: out = state.subject.reduce([](Value a,Value b){
										if (a.defined()) return a.merge(b);else return a;
									},Value());
									break;
								case kSub: out = state.subject.reduce([](Value a,Value b){
										if (a.defined()) return a.diff(b);else return a;
									},Value());
									break;
								case kMul: out = state.subject.reduce([](Value a,Value b){
										return a.getNumber() * b.getNumber();
									},Value(1.0));
									break;
								case kDiv: out = state.subject.reduce([](Value a,Value b){
										if (a.defined())
											return Value(a.getNumber() / b.getNumber());
										else
											return a;
									},Value());
									break;
								case kInv: out = 1.0/state.subject.getNumber();break;
								case kKeys: out = state.subject.map([](Value a){return a.getKey();});break;
								case kValues: out = state.subject.map([](Value a){return a.stripKey();});break;
								case kFirst: out = state.subject[0];break;
								case kLast: out = state.subject[state.subject.size()-1];break;
								case kShift: out = state.subject.slice(1);break;
								case kRev: out = state.subject.reverse();break;
								case kSlice: out = state.subject.slice(arg[0].getInt(),arg[1].getInt());break;
								case kExplode: out = doExplode(state.subject, arg);break;
								case kSet: out = arg;break;
								case kPrefix: out = doPrefix(state.subject, arg);break;
								case kSuffix: out = doSuffix(state.subject, arg);break;
								case kPop: out = state.pop();break;
								case kDelete: out = state.subject.replace(PPath::fromValue(arg),undefined);break;
								case kRound: out = std::round(state.subject.getNumber());break;
								case kMerge: out = Object(state.subject).merge(arg);break;
								case kRMerge: out = state.subject.merge(arg);break;
								case KRoot: out = rootValue;break;
								default:
									out = v2processCustomTransform(State(state), simple_rule_name, arg);
									break;
							}
						}
						if (!out.defined()) return v2report(state,rv,false);
						state.subject = out;
						state.path.push(Object
								("arg",arg)
								("node",state.subject)
								("transform",rv));
					} else {
						auto k = keywords.find(r);
						auto kk = k?*k:kUnknown;
						switch (kk) {
							case kPush: if (arg.defined()) state.push(arg);else state.push(state.subject);break;
							case kArray: if (!v2processArray(std::move(state),arg,array)) return false;break;
							case kTuple: if (!v2processTuple(std::move(state),arg)) return false;break;
							case kObject: if (!v2processArray(std::move(state),arg,object)) return false;break;
							case kEmit: output.push_back(Output(Output::emit,state, arg));break;
							case kPostpone: output.push_back(Output(Output::postpone,state, arg));break;
							case kExtern: output.push_back(Output(Output::external,state, arg));break;
						}
					}

				}
			}



		} catch (const std::exception &e) {

		}
	}





}

Value Validator::State::getPath() const {
	Array r;
	Stack z = path;
	while (!z.empty()) {
		r.push_back(z.top());
		z = z.pop();
	}
	r.reverse();
	return r;
}


bool Validator::testRegExpr(StrViewA pattern, StrViewA subject) {
	return false;
}

bool Validator::v2report(const State &state, Value rule, bool result) {
	if (!result) {
		rejections.push_back(Object
				("node", state.subject)
				("path", state.getPath())
				("rule", rule.defined()?rule:"undefined")
		);
	}
	return result;
}

bool Validator::v2report_exception(const State &state, Value rule, const char *what) {
	rejections.push_back(Object
			("node", state.subject)
			("path", state.getPath())
			("rule", rule.defined()?rule:"undefined")
			("error", what)
	);
	return false;

}

template<typename Cmp>
static bool checkOrder(const Value &array, Cmp &&cmp) {
	std::size_t cnt = array.size();
	if (cnt < 2) return true;
	Value ref = array[0];
	for (std::size_t i = 1; i < cnt; i++) {
		Value x = array[i];
		if (!cmp(Value::compare(ref, x),0)) return false;
		ref = x;
	}
	return true;
}

bool Validator::v2processSimpleRule(State &&state, Value rule) {
	StrViewA srule = rule.getString();
	if (srule.begins("'")) {
		return v2report(state, rule, srule.substr(1) == state.subject.toString());
	}
	if (srule.begins("//")) {
		return v2report(state, rule);
	}
	if (srule.begins("/")) {
		return v2report(state, rule, testRegExpr(rule.getString(), state.subject.toString()));
	}
	auto k = keywords.find(srule);
	if (k) {
		switch (*k) {
		case kString: return v2report(state,rule, state.subject.type() == string);
		case kNumber: return v2report(state,rule, state.subject.type() == number);
		case kInteger: return v2report(state,rule, state.subject.type() == number && state.subject.flags() & numberInteger);
		case kUnsigned: return v2report(state,rule, state.subject.type() == number && state.subject.flags() & numberUnsignedInteger);
		case kObject: return v2report(state,rule, state.subject.type() == object);
		case kArray: return v2report(state,rule, state.subject.type() == array);
		case kBool:
		case kBoolean: return v2report(state,rule, state.subject.type() == boolean);
		case kNull: return v2report(state,rule, state.subject.type() == null);
		case kUndefined: return v2report(state,rule, !state.subject.defined());
		case kAny: return v2report(state,rule, state.subject.defined());
		case kBase64: return v2report(state,rule, opBase64(state.subject));
		case kBase64url: return v2report(state,rule, opBase64url(state.subject));
		case kHex: return v2report(state,rule, opHex(state.subject));
		case kUpper: return v2report(state,rule, opUppercase(state.subject));
		case kLower: return v2report(state,rule, opLowercase(state.subject));
		case kIdentifier: return v2report(state,rule, opIsIdentifier(state.subject));
		case kAlpha: return v2report(state,rule, opAlpha(state.subject));
		case kAlnum: return v2report(state,rule, opAlnum(state.subject));
		case kDigits: return v2report(state,rule, opDigits(state.subject));
		case kAsc: return v2report(state,rule, checkOrder(state.subject, std::less<int>()));
		case kAscDup: return v2report(state,rule, checkOrder(state.subject, std::less_equal<int>()));
		case kDesc: return v2report(state,rule, checkOrder(state.subject, std::greater<int>()));
		case kDescDup: return v2report(state,rule, checkOrder(state.subject, std::greater_equal<int>()));
		case kStart:
		case kRootRule: return v2processRule(std::move(state), rootRule, false);
		}
	}
	Value custom = def[srule];
	return v2report(state, rule, v2processRule(std::move(state), custom,false));
}

Value Validator::doMap(State &&state, Value rule) {
	Value tmp;
	if (rule.type() == object) {
		Object obj;
		for(Value x: rule) {
			if (!runConversion(State(state),x,tmp)) return undefined;
			obj.set(rule.getKey(), tmp);
		};
		return obj;
	} else if (rule.type() == array) {
		Array arr;
		for(Value x: rule) {
			if (!runConversion(State(state),x,tmp)) return undefined;
			arr.push_back(tmp);
		};
		return arr;
	} else {
		if (!runConversion(State(state),rule,tmp)) return undefined;
		return tmp;
	}
}

bool Validator::runConversion(State &&state, Value rule, Value &output) {
	State st(state);
	if (!v2processSequence(std::move(st), rule)) return false;
	output = st.subject;
	return true;
}

Value Validator::v2processCustomTransform(State &&state, StrViewA name, Value arg) {
	state.push(arg);
	Value rule = def[name];
	Value out;
	if (!runConversion(std::move(state),rule,out)) v2report(state,rule,false);
	return out;

}

}
