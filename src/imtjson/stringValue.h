/*
 * stringValue.h
 *
 *  Created on: Oct 25, 2016
 *      Author: ondra
 */

#ifndef SRC_IMMUJSON_STRINGVALUE_H_
#define SRC_IMMUJSON_STRINGVALUE_H_

#pragma once

#include "basicValues.h"

namespace json {

class StringValue: public AbstractStringValue {
public:
	StringValue(const StringView<char> &str, bool isBinary);
	template<typename Fn> StringValue(std::size_t strSz, const Fn &fn, bool isBinary);

	virtual StringView<char> getString() const override;
	virtual bool getBool() const override {return true;}
	virtual ValueTypeFlags flags() const override;


	void *operator new(std::size_t sz, const StringView<char> &str );
	void operator delete(void *ptr, const StringView<char> &str);
	void *operator new(std::size_t sz, const std::size_t &strsz );
	void operator delete(void *ptr, const std::size_t &sz);
	void operator delete(void *ptr, std::size_t sz);

protected:
	StringValue(StringValue &&) = delete;
	std::size_t size;
	bool isBinary;
	char charbuff[65536];

	static void *putMagic(void *obj);
	void stringOverflow();


};

template<typename Fn>
inline StringValue::StringValue(std::size_t strSz, const Fn& fn, bool isBinary)
	:size(strSz)
	,isBinary(isBinary)
{
	charbuff[strSz] = 0;
	std::size_t wrsz = fn(charbuff);
	if (wrsz > strSz || charbuff[strSz] != 0) stringOverflow();
	charbuff[wrsz] = 0;
	size = wrsz;
}

}


#endif /* SRC_IMMUJSON_STRINGVALUE_H_ */
