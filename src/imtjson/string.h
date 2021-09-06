/*
 * string.h
 *
 *  Created on: 14. 10. 2016
 *      Author: ondra
 */

#ifndef SRC_IMMUJSON_STRING_H_
#define SRC_IMMUJSON_STRING_H_


#pragma once

#include <functional>
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
	explicit String(Value v);
	///Creates new string from a string view
	/**
	 * @param str a string's view. The StringView object can be constructed from
	 * c-string and std::string.
	 */
	String(const std::string_view &str);

	///Creates new string from a wide-char string view
	/**
	 * @param str a wide-char string's view. The constructor converts wide-char string to utf-8 string.
	 *
	 * @note this constructor is explicit.
	 */
	explicit String(const std::wstring_view &str);

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
	String(const std::initializer_list<std::string_view > &strlist);

	///Construct the string using the function.
	/**Allows to construct the string by some specific way
	 * This reduces any posibility of unnecesery copying for long strings.
	 *
	 * The constructor need to know the exact length of the string.
	 * You must not write more characters. You can write less characters, however
	 * the extra space remains allocated.
	 *
	 * @param sz initial size of the buffer
	 * @param fillFn function which is called to fill the allocated buffer. It receives
	 * only one argument - pointer to the buffer. The size of the buffer is
	 * equal to size specified in the constructor, so the function must receive
	 * the size some other way (for example though capture list of a lamba function declaration)
	 *
	 * The function returns count of written characters. Returned value must be
	 * less or equal to initial size, otherwise the function abort() is called for security reasons
	 */
	String(std::size_t sz, std::function<std::size_t(char *)> fillFn);


	///Retrieve substring
	/**
	 * @param pos position where substring starts
	 * @param length lenght of the string.
	 * @return substring is stored as StringView.
	 *
	 * If arguments are out of the bounds, they are adjusted. This may
	 * cause that empty string will be returned
	 */
	std::string_view substr(std::size_t pos, std::size_t length) const;
	///Retrieve substring
	/**
	 * @param pos position where substring starts.
	 * @return substring from position to the end of the string is stored as StringView.
	 *
	 * If argument is out of the bounds, the empty string is returned
	 */
	std::string_view substr(std::size_t pos) const;
	///Retrieves the left part of the string (prefix)
	/**
	 * @param length length of the string (length of the prefix)
	 * @return substring stored ast StringView
	 *
	 * @note if argument is out of the bounds, whole string is returned
	 */
	std::string_view left(std::size_t length) const;
	///Retrieves the right part of the string (suffix)
	/**
	 * @param length length of the string (length of the suffix) is counted from the end of the string
	 * @return substring stored ast StringView
	 *
	 * @note if argument is out of the bounds, whole string is returned
	 */
	std::string_view right(std::size_t length) const;

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
//	String operator+(const std::string_view &other) const;
//	String operator+(const String &other) const;

	///Converts String to StringView
	operator std::string_view() const;
	///Converts String to StringView
	StringView str() const;
	///Converts String to c-string (with terminating zero character)
	const char *c_str() const;
	///Converts string stored internally encoded to utf-8 to wide-char string.
	std::wstring wstr() const;
	///Insert substring to the string
	String insert(std::size_t pos, const std::string_view &what);
	///Replaces substring
	String replace(std::size_t pos, std::size_t size, const std::string_view &what);
	///Splits string to array
	Value split(const std::string_view separator, std::size_t maxCount = -1) const;
	///split at given position
	Value::TwoValues splitAt(Int pos) const;
	///Appends sting
	String &operator += (const std::string_view other);
	///Finds substring
	std::size_t indexOf(const std::string_view other, std::size_t start = 0) const;
	///Finds substring
	std::size_t lastIndexOf(const std::string_view other, std::size_t start = 0) const;

	const char *begin() const {return c_str();}
	const char *end() const {return c_str()+length();}



	bool operator==(const std::string_view &other) const {return std::string_view(*this) == other;}
	bool operator!=(const std::string_view &other) const {return std::string_view(*this) != other;}
	bool operator>=(const std::string_view &other) const {return std::string_view(*this) >= other;}
	bool operator<=(const std::string_view &other) const {return std::string_view(*this) <= other;}
	bool operator>(const std::string_view &other) const  {return std::string_view(*this) > other;}
	bool operator<(const std::string_view &other) const  {return std::string_view(*this) < other;}
	friend String operator+(std::string_view str, std::string_view str2);

	///Allows to test whether string created from Value is defined
	/** The string is defined only when underlying value is type of string
	 * otherwise string is undefined
	 *
	 * @retval true string is defined
	 * @retval false string is not defined.
	 */
	bool defined() const {return impl->type() == string;}


	PValue getHandle() const;


protected:
	PValue impl;
};


class Split{
public:
	Split(const std::string_view &string, const std::string_view &sep, std::size_t limit = std::string_view::npos);
	std::string_view operator()();
	bool operator!() const;
protected:
	std::string_view str;
	std::string_view sep;
	std::size_t limit;
};

std::ostream &operator<<(std::ostream &out, const String &x);



}



#endif /* SRC_IMMUJSON_STRING_H_ */
