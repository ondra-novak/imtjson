/*
 * string.cpp
 *
 *  Created on: 14. 10. 2016
 *      Author: ondra
 */


#include "basicValues.h"
#include "string.h"

#include <cstring>
#include <sstream>
#include "stringValue.h"

#include "array.h"
#include "streams.h"
#include "utf8.h"


namespace json {



String::String():impl(AbstractStringValue::getEmptyString())
{
}

String::String(Value v):impl(v.getHandle()->unproxy()) {
}

String::String(std::size_t sz, std::function<std::size_t(char *)> fillFn):impl(new(sz) StringValue(nullptr, sz, fillFn)) {
}


std::string_view String::substr(std::size_t pos, std::size_t length) const {
	return impl->getString().substr(pos,length);
}


std::string_view String::substr(std::size_t pos) const {
	return impl->getString().substr(pos);
}

std::string_view String::left(std::size_t length) const {
	return impl->getString().substr(0,length);
}

std::string_view String::right(std::size_t length) const {
	std::size_t l = impl->getString().size();
	if (length > l) length = l;
	return impl->getString().substr(l - length, length);
}

std::size_t String::length() const {
	return impl->getString().size();
}

bool String::empty() const {
	return length() == 0;
}

char String::operator [](std::size_t pos) const {
	std::string_view s = impl->getString();
	if (pos >= s.size()) return 0;
	else return s[pos];
}

String::String(const std::initializer_list<std::string_view >& strlist) {
	std::size_t cnt = 0;
	for (auto &&item : strlist) {
		cnt += item.size();
	}

	impl = new(cnt) StringValue(nullptr, cnt, [&](char *buff) {
		for (auto &&item : strlist) {
			memcpy(buff, item.data(), item.size());
			buff+=item.size();
		}
		return cnt;
	});
}


String operator+(std::string_view a, std::string_view b) {
	if (a.empty() && b.empty()) return String();

	std::string v;
	v.reserve(a.size()+b.size());
	v.append(a.data(),a.size());
	v.append(b.data(),b.size());

	return String(std::move(v));
}

/*String String::operator +(const std::string_view& other) const {
	if (other.empty()) return *this;

	std::string_view a = impl->getString();
	std::string_view b = other;
	std::string v;
	v.reserve(a.size()+b.size());
	v.append(a.data,a.size());
	v.append(b.data,b.size());



	return String(new StringValue(std::move(v)));

}*/



String::operator std::string_view() const {
	return impl->getString();
}

StringView String::str() const {
	return impl->getString();
}

const char* String::c_str() const {
	return impl->getString().data();
}


String String::insert(std::size_t pos, const std::string_view& what) {
	using namespace std;

	if (what.empty()) return *this;



	std::string_view a = impl->getString();
	if (pos > a.size()) pos = a.size();
	std::size_t sz = a.size()+what.size();

	return String(new(sz) StringValue(nullptr, sz,[&](char *trg) {
		std::memcpy(trg, a.data(), pos);
		std::memcpy(trg+pos,what.data(),what.size());
		std::memcpy(trg+pos+what.size(),a.data()+pos,a.size()-pos);
		return sz;
	}));


}

String String::replace(std::size_t pos, std::size_t size,
		const std::string_view& what) {
	std::string_view a = impl->getString();
	if (pos > a.size()) pos = a.size();
	if (pos+size > a.size()) size = a.size() - pos;

	std::size_t needsz = a.size()+what.size() - size;
	return String(new(needsz) StringValue(nullptr, needsz,[&](char *buff){
		std::memcpy(buff, a.data(), pos);
		std::memcpy(buff+pos, what.data(), what.size());
		std::memcpy(buff+pos+what.size(), a.data()+pos+size, a.size()-pos-size);
		return needsz;
	}));

}

class SubStrString: public AbstractStringValue {
public:
	SubStrString(const PValue  &s, std::size_t pos, std::size_t length)
		:parent(s),pos(pos),length(length) {}

	virtual StringView getString() const override {
		return parent->getString().substr(pos,length);
	}
	virtual bool getBool() const override {return true;}


	PValue parent;
	std::size_t pos;
	std::size_t length;
};


Value String::split(const std::string_view separator, std::size_t maxCount) const {
	Array res;
	std::size_t curPos = 0;
	std::string_view a = impl->getString();
	std::size_t nextPos = a.find(separator, curPos);
	while (nextPos != a.npos && maxCount) {
		res.push_back(new SubStrString(impl,curPos, nextPos-curPos));
		curPos = nextPos+separator.size();
		nextPos = a.find(separator, curPos);
		maxCount --;
	}
	if (curPos == 0) {
		res.push_back(impl);
	} else {
		res.push_back(new SubStrString(impl, curPos, a.size() - curPos));
	}
	return res;


}

Value::TwoValues String::splitAt(Int pos) const
{
	std::size_t sz = length();
	if (pos > 0) {
		if ((unsigned)pos < sz) {
			return Value::TwoValues(new SubStrString(impl, 0, pos), new SubStrString(impl, pos, sz - pos));
		}
		else {
			return Value::TwoValues(*this, {});
		}
	}
	else if (pos < 0) {
		if ((int)sz + pos > 0) return splitAt((int)sz + pos);
		else return Value::TwoValues({}, *this);
	}
	else {
		return Value::TwoValues(*this, {});
	}	

}

String& String::operator +=(const std::string_view other) {
	impl = ((*this) + other).getHandle();
	return *this;
}

std::size_t String::indexOf(const std::string_view other, std::size_t start) const {
	std::string_view a = impl->getString();
	return a.find(other, start);
}
std::size_t String::lastIndexOf(const std::string_view other, std::size_t start) const {
	std::string_view a = impl->getString();
	return a.rfind(other, start);
}


PValue String::getHandle() const {
	return impl;
}

std::ostream &operator<<(std::ostream &out, const String &x) {
	std::string_view a = x;
	out.write(a.data(), a.size());
	return out;
}

String::String(const std::string_view& str):impl(Value(str).getHandle()) {

}

String::String(const char* str):impl(Value(str).getHandle()) {
}

String::String(const std::basic_string<char>& str):impl(Value(str).getHandle()) {

}


String::String(const std::wstring_view &wstr) {

	WideToUtf8 conv;
	std::vector<char> buff;
	buff.reserve(wstr.size()*3);
	std::size_t pos = 0;
	conv([&]() -> int {
		if (pos < wstr.size()) return wstr[pos++];
		else return eof;
	}, [&](int c) {
		buff.push_back((char)c);
	});

	std::string_view str(buff.data(), buff.size());
	impl = StringValue::create(str);
}

std::wstring json::String::wstr() const {
	Utf8ToWide conv;
	std::wostringstream buff;
	conv(fromString(str()), [&](wchar_t c){
		buff.put(c);
	});
	return buff.str();
}

Split::Split(const std::string_view &string, const std::string_view &sep, std::size_t limit)
	:str(string)
	,sep(sep)
	,limit(limit)
{
}

std::string_view Split::operator ()() {
	auto p = limit?str.find(sep):str.npos;
	if (p == str.npos) {
		auto out = str;
		str = "";
		return out;
	} else {
		auto out = str.substr(0,p);
		str = str.substr(p + sep.size());
		limit--;
		return out;
	}

}

bool Split::operator !() const {
	return str.empty() || sep.empty();
}

}

