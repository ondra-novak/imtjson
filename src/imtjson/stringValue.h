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
	StringValue(BinaryEncoding encoding, const StringView<char> &str);
	template<typename Fn> StringValue(BinaryEncoding encoding,std::size_t strSz, const Fn &fn);

	virtual StringView<char> getString() const override;
	virtual bool getBool() const override {return true;}
	virtual ValueTypeFlags flags() const override {return encoding != 0?binaryString:0;}

	void *operator new(std::size_t sz, const std::size_t &strsz );
	void operator delete(void *ptr, const std::size_t &sz);
	void operator delete(void *ptr, std::size_t sz);


	static PValue create(const StringView<char> &str);
	static PValue create(const BinaryView &str, BinaryEncoding enc);

	static BinaryEncoding getEncoding(const IValue *v);

protected:
	StringValue(StringValue &&) = delete;
	StringValue(const StringValue &) = delete;
	std::size_t sz;
	BinaryEncoding encoding;
	char charbuff[100]; ///< the string can be larger than 100 bytes - this is for preview in the debugger

	static void *putMagic(void *obj);
	void stringOverflow();


};

template<typename Fn>
inline StringValue::StringValue(BinaryEncoding encoding, std::size_t strSz, const Fn& fn)
	:sz(strSz)
	,encoding(encoding) {
	charbuff[strSz] = 0;
	std::size_t wrsz = fn(charbuff);
	if (wrsz > strSz || charbuff[strSz] != 0) stringOverflow();
	charbuff[wrsz] = 0;
	sz = wrsz;
}

}
#endif /* SRC_IMMUJSON_STRINGVALUE_H_ */
