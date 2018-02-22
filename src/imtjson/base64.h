#pragma once
#include "binary.h"

namespace json {


class Base64Table {
public:
	unsigned char table[256];
	Base64Table(const char *chars);
	static char base64chars[];
	static char base64urlchars[];
};


class Base64Encoding: public AbstractBinaryEncoder {
public:

	virtual StrViewA getName() const override;
	static void decoderCore(unsigned char *buff, const StrViewA &str, std::size_t len, const Base64Table &t);
	virtual Value decodeBinaryValue(const StrViewA &str) const override;
	virtual void encodeBinaryValue(const BinaryView &data, const WriteFn &fn) const override;
	static void encodeCore(const BinaryView& data, char *chars, WriteFn fn, bool tail) ;
};


class Base64EncodingUrl: public Base64Encoding {
public:
	virtual StrViewA getName() const override;
	virtual Value decodeBinaryValue(const StrViewA &str) const override;
	virtual void encodeBinaryValue(const BinaryView &data, const WriteFn &fn) const override;
};


}
