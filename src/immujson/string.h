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

///Manipulation of string value
/** Strings are inherent part of Json. They are also immutable as other types.
 * This class brings the Json's string as standalone object which can be used
 * to store string values and to manipulate with them (moving,copying) faster, than
 * the std::string does, because strings are immutable,they are shared instead copying.
 *
 * The string objects can be also accessed in multithread environment without
 * need additional synchronization.
 */
class String {
public:

	///Construct empty string
	String();
	///Convert Value to String.
	/**
	 * @param v a value. Note if the value is not string, it will appear as empty string.
	 * If you need to stringify the value, you have to use Value::toString
	 * or Value::stringify
	 */
	String(Value v);
	///Creates new string from a string view
	/**
	 * @param str a string's view. The StringView object can be constructed from
	 * c-string and std::string.
	 */
	String(const StringView<char> &str);

	///Creates new string from C-string
	/**
	 * @param str pointer to char array
	 */
	String(const char *str);

	String(const std::basic_string<char> &str);

	///Construct string as concatenation of strings passed as initializer list
	/**
	 * @param strlist list of strings. All strings are concatenated into big-one
	 */
	String(const std::initializer_list<StringView<char> > &strlist);

	///Retrieve substring
	/**
	 * @param pos position where substring starts
	 * @param length lenght of the string.
	 * @return substring is stored as StringView.
	 *
	 * If arguments are out of the bounds, they are adjusted. This may
	 * cause that empty string will be returned
	 */
	StringView<char> substr(std::size_t pos, std::size_t length) const;
	///Retrieve substring
	/**
	 * @param pos position where substring starts.
	 * @return substring from position to the end of the string is stored as StringView.
	 *
	 * If argument is out of the bounds, the empty string is returned
	 */
	StringView<char> substr(std::size_t pos) const;
	///Retrieves the left part of the string (prefix)
	/**
	 * @param length length of the string (length of the prefix)
	 * @return substring stored ast StringView
	 *
	 * @note if argument is out of the bounds, whole string is returned
	 */
	StringView<char> left(std::size_t length) const;
	///Retrieves the right part of the string (suffix)
	/**
	 * @param length length of the string (length of the suffix) is counted from the end of the string
	 * @return substring stored ast StringView
	 *
	 * @note if argument is out of the bounds, whole string is returned
	 */
	StringView<char> right(std::size_t length) const;

	///Retrieves length of the string
	std::size_t length() const;
	///Returns true, if string is empty
	bool empty() const;

	///Access to character of the string
	char operator[](std::size_t pos) const;
	///Concatenation of two strings
	/** Function accept any StringView on the right side
	 *
	 * @param other right-hand of the operator
	 * @return concatenation of the strings
	 */
//	String operator+(const StringView<char> &other) const;
//	String operator+(const String &other) const;

	///Converts String to StringView
	operator StringView<char>() const;
	///Converts String to StringView
	StringView<char> str() const;
	///Converts String to c-string (with terminating zero character)
	const char *c_str() const;
	///Insert substring to the string
	String insert(std::size_t pos, const StringView<char> &what);
	///Replaces substring
	String replace(std::size_t pos, std::size_t size, const StringView<char> &what);
	///Splits string to array
	Value split(const StringView<char> separator, std::size_t maxCount = -1) const;
	///Appends sting
	String &operator += (const StringView<char> other);
	///Finds substring
	std::size_t indexOf(const StringView<char> other, std::size_t start = 0) const;

	bool operator==(const String &other) const {return StringView<char>(*this) == StringView<char>(other);}
	bool operator!=(const String &other) const {return StringView<char>(*this) != StringView<char>(other);}
	bool operator>=(const String &other) const {return StringView<char>(*this) >= StringView<char>(other);}
	bool operator<=(const String &other) const {return StringView<char>(*this) <= StringView<char>(other);}
	bool operator>(const String &other) const  {return StringView<char>(*this) > StringView<char>(other);}
	bool operator<(const String &other) const  {return StringView<char>(*this) < StringView<char>(other);}
	friend String operator+(StringView<char> str, StringView<char> str2);

	PValue getHandle() const;
protected:
	PValue impl;
};

std::ostream &operator<<(std::ostream &out, const String &x);


}



#endif /* SRC_IMMUJSON_STRING_H_ */
