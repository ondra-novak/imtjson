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
	StringValue(const StringView<char> &str);
	template<typename Fn> StringValue(std::size_t strSz, const Fn &fn);

	virtual StringView<char> getString() const override;
	virtual bool getBool() const override {return true;}

	virtual std::intptr_t getInt() const override;
	virtual std::uintptr_t getUInt() const override;
	virtual double getNumber() const override;

	void *operator new(std::size_t sz, const StringView<char> &str );
	void operator delete(void *ptr, const StringView<char> &str);
	void *operator new(std::size_t sz, const std::size_t &strsz );
	void operator delete(void *ptr, const std::size_t &sz);
	void operator delete(void *ptr, std::size_t sz);

protected:
	StringValue(StringValue &&) = delete;
	std::size_t size;
	char charbuff[65536];

	static void *putMagic(void *obj);


};

template<typename Fn>
inline StringValue::StringValue(std::size_t strSz, const Fn& fn):size(strSz) {
	fn(charbuff);
	charbuff[strSz] = 0;
}


}


#endif /* SRC_IMMUJSON_STRINGVALUE_H_ */
