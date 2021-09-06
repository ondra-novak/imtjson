#include "binary.h"

#include <cstring>
#include <sstream>

#include "stringValue.h"
#include "base64.h"
#include "utf8.h"

namespace json {



BinaryEncoding Binary::getEncoding() const {
	return StringValue::getEncoding(s);
}

BinaryEncoding Binary::getEncoding(const IValue *v)  {
	return StringValue::getEncoding(v);
}

Binary Binary::decodeBinaryValue(const Value &s, BinaryEncoding encoding) {
	std::string_view str = s.getString();
	if (str.empty()) {
		return Binary(json::string);
	} else if ((s.flags() & binaryString) == 0) {
		return Binary(encoding->decodeBinaryValue(str));
	} else {
		return Binary(s);
	}
}

class DirectEncoding: public AbstractBinaryEncoder {
public:
	using AbstractBinaryEncoder::encodeBinaryValue;

	virtual Value decodeBinaryValue(const std::string_view &string) const override {
		return string;
	}
	virtual void encodeBinaryValue(const BinaryView &binary, const WriteFn &fn) const override{
		fn(map_bin2str(binary));
	}
	virtual std::string_view getName() const override {
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
	using AbstractBinaryEncoder::encodeBinaryValue;
	virtual Value decodeBinaryValue(const std::string_view &string) const override {
		std::size_t srclen = string.size();
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
			if (c && (isalnum(c) || strchr("-_.!~*'()", c) != 0)) fn(std::string_view(reinterpret_cast<char *>(&c),1));
			else {
				char buff[3];
				buff[0]='%';
				buff[1]=hexchar[(c >> 4)];
				buff[2]=hexchar[(c & 0xF)];
				fn(std::string_view(buff,3));
			}
		}
	}
	virtual std::string_view getName() const override {
		return "urlencoding";
	}
};

Value AbstractBinaryEncoder::encodeBinaryValue(const BinaryView &binary) const  {
	std::ostringstream buffer;
	encodeBinaryValue(binary,[&](std::string_view str){buffer << str;});
	return Value(buffer.str());
}


class UTF8Encoding: public AbstractBinaryEncoder {
protected:

	virtual Value decodeBinaryValue(const std::string_view &string) const override {
		Utf8ToWide conv;
		std::size_t needsz;
		conv(fromString(string), WriteCounter<std::size_t>(needsz));
		RefCntPtr<StringValue> strVal = new(needsz) StringValue(utf8encoding,needsz, [&](char *c) {
			unsigned char *cc = reinterpret_cast<unsigned char *>(c);
			conv(fromString(string),[&](int d){*cc++=static_cast<unsigned char>(d);});
			return needsz;
		});
		return PValue::staticCast(strVal);
	}
	virtual void encodeBinaryValue(const BinaryView &binary, const WriteFn &fn) const override{
		WideToUtf8 conv;
		char buff[256];
		int pos = 0;
		conv(fromBinary(binary),[&](char c) {
			buff[pos++] = c;
			if (pos == sizeof(buff)) {
				fn(std::string_view(buff,pos));
				pos=0;
			}
		});
		if (pos) fn(std::string_view(buff,pos));
	}
	Value encodeBinaryValue(const BinaryView &binary) const  override {
		WideToUtf8 conv;
		std::size_t needsz;
		conv(fromBinary(binary), WriteCounter<std::size_t>(needsz));
		RefCntPtr<StringValue> strVal = new(needsz) StringValue(nullptr,needsz, [&](char *c) {
			conv(fromBinary(binary),[&](char d){*c++=d;});
			return needsz;
		});
		return PValue::staticCast(strVal);
	}


	virtual std::string_view getName() const override {
		return "utf8encoding";
	}



};


static Base64Encoding base64Var;
static Base64EncodingUrl base64UrlVar;
static UrlEncoding urlEncodingVar;
static UTF8Encoding utf8encodingVar;
BinaryEncoding base64 = &base64Var;
BinaryEncoding base64url = &base64UrlVar;
BinaryEncoding urlEncoding = &urlEncodingVar;
BinaryEncoding defaultBinaryEncoding = &base64Var;
BinaryEncoding utf8encoding = &utf8encodingVar;

}
