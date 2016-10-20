/*
 * string.h
 *
 *  Created on: 14. 10. 2016
 *      Author: ondra
 */

#ifndef SRC_IMMUJSON_STRING_H_
#define SRC_IMMUJSON_STRING_H_


#pragma once

#include "value.h"

namespace json {


class String {
public:

	String();
	String(Value v);
	String(const StringView<char> &str);
	String(const char *str);
	String(const std::string &str);

	StringView<char> substr(std::size_t pos, std::size_t length) const;
	StringView<char> substr(std::size_t pos) const;
	StringView<char> left(std::size_t length) const;
	StringView<char> right(std::size_t length) const;

	std::size_t length() const;
	std::size_t empty() const;

	char operator[](std::size_t pos) const;
	String operator+(const StringView<char> &other) const;
	String operator+(const String &other) const;

	operator StringView<char>() const;
	const char *c_str() const;
	String insert(std::size_t pos, const StringView<char> &what);
	String replace(std::size_t pos, std::size_t size, const StringView<char> &what);
	Value split(const StringView<char> separator, std::size_t maxCount = -1) const;
	String &operator += (const StringView<char> other);
	std::size_t indexOf(const StringView<char> other, std::size_t start = 0) const;
	Value substrValue(std::size_t pos, std::size_t length) const;

	bool operator==(const String &other) const {return StringView<char>(*this) == StringView<char>(other);}
	bool operator!=(const String &other) const {return StringView<char>(*this) != StringView<char>(other);}
	bool operator>=(const String &other) const {return StringView<char>(*this) >= StringView<char>(other);}
	bool operator<=(const String &other) const {return StringView<char>(*this) <= StringView<char>(other);}
	bool operator>(const String &other) const  {return StringView<char>(*this) > StringView<char>(other);}
	bool operator<(const String &other) const  {return StringView<char>(*this) < StringView<char>(other);}


	PValue getHandle() const;
protected:
	PValue impl;
};

std::ostream &operator<<(std::ostream &out, const String &x);

}



#endif /* SRC_IMMUJSON_STRING_H_ */
