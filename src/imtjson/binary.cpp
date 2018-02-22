#include "binary.h"

#include <cstring>
#include <sstream>

#include "stringValue.h"
#include "base64.h"

namespace json {



BinaryEncoding Binary::getEncoding() const {
	return StringValue::getEncoding(s);
}

BinaryEncoding Binary::getEncoding(const IValue *v)  {
	return StringValue::getEncoding(v);
}

Binary Binary::decodeBinaryValue(const Value &s, BinaryEncoding encoding) {
	StrViewA str = s.getString();
	if (str.empty()) {
		return Binary(json::string);
	} else if (s.flags() != binaryString) {
		return Binary(encoding->decodeBinaryValue(str));
	} else {
		return Binary(s);
	}
}

class DirectEncoding: public AbstractBinaryEncoder {
public:

	virtual Value decodeBinaryValue(const StrViewA &string) const override {
		return string;
	}
	virtual void encodeBinaryValue(const BinaryView &binary, const WriteFn &fn) const override{
		fn(StrViewA(binary));
	}
	virtual StrViewA getName() const override {
		return "direct";
	}
};

static DirectEncoding directEncodingVar;
BinaryEncoding directEncoding = &directEncodingVar;


static char hexchar[] = "0123456789ABCDEF";
static class UnhexChar {
public:
	unsigned int operator[](char h) const {
		if (isdigit(h)) return h-'0';
		else if (h >= 'A' && h<='F') return h-'A'+10;
		else if (h >= 'a' && h<='f') return h-'a'+10;
		return 0;
	}
} unhexchar;

class UrlEncoding: public AbstractBinaryEncoder {
public:
	virtual Value decodeBinaryValue(const StrViewA &string) const override {
		std::size_t srclen = string.length;
		std::size_t trglen = 0;
		for (std::size_t i = 0; i < srclen; i++, trglen++)
			if (string[i] == '%') i+=2;

		RefCntPtr<StringValue> strVal = new(trglen) StringValue(urlEncoding,trglen, [&](char *c) {

			for (std::size_t i = 0; i < srclen; i++)
				if (string[i] == '%') {
					if (i + 3 > srclen) {
						return trglen-1;
					} else {
						unsigned char val = (unhexchar[string[i+1]] << 4) | (unhexchar[string[i+2]] & 0xF);
						*c++ = val;
						i+=2;
					}
				} else if (string[i] == '+') {
					*c++ = ' ';
				} else {
					*c++ = string[i];
				}
			return trglen;
		});

		return PValue::staticCast(strVal);
	}
	virtual void encodeBinaryValue(const BinaryView &binary, const WriteFn &fn) const override{
		for (unsigned char c : binary) {
			if (c && (isalnum(c) || strchr("-_.!~*'()", c) != 0)) fn(StrViewA(reinterpret_cast<char *>(&c),1));
			else {
				char buff[3];
				buff[0]='%';
				buff[1]=hexchar[(c >> 4)];
				buff[2]=hexchar[(c & 0xF)];
				fn(StrViewA(buff,3));
			}
		}
	}
	virtual StrViewA getName() const override {
		return "urlencoding";
	}
};

Value AbstractBinaryEncoder::encodeBinaryValue(const BinaryView &binary) const  {
	std::ostringstream buffer;
	encodeBinaryValue(binary,[&](StrViewA str){buffer << str;});
	return Value(buffer.str());
}


static Base64Encoding base64Var;
static Base64EncodingUrl base64UrlVar;
static UrlEncoding urlEncodingVar;
BinaryEncoding base64 = &base64Var;
BinaryEncoding base64url = &base64UrlVar;
BinaryEncoding urlEncoding = &urlEncodingVar;
BinaryEncoding defaultBinaryEncoding = &base64Var;

}
