#include "binary.h"
#include "stringValue.h"

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

class DirectEncoding: public IBinaryEncoder {
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

class Base64Table {
public:
	unsigned char table[256];
	Base64Table() {
		for (std::size_t i = 0; i <256; i++) table[i] = 0xFF;
		for (std::size_t i = 'A'; i <='Z'; i++) table[i] = (unsigned char)(i - 'A');
		for (std::size_t i = 'a'; i <= 'z'; i++) table[i] = (unsigned char)(i - 'a' + 26);
		for (std::size_t i = '0'; i <='9'; i++) table[i] = (unsigned char)(i - '0' + 52);
		table['+'] = 63;
		table['/'] = 64;
	}
};


class Base64Encoding: public IBinaryEncoder {
public:

	virtual StrViewA getName() const {
		return "base64";
	}

	virtual Value decodeBinaryValue(const StrViewA &str) const override {
		static Base64Table t;
		std::size_t len = str.length;
		if ((len & 0x3) || len == 0) return String();
		std::size_t finlen = ((len+3) / 4) * 3;
		if (str[len-1] == '=') {
			--finlen;
			--len;
		}
		if (str[len-1] == '=') {
			--finlen;
			--len;
		}
		RefCntPtr<StringValue> strVal = new(len) StringValue(base64,len,[&] (char *buff) {

			unsigned char *bbuf = reinterpret_cast<unsigned char *>(buff);

			std::size_t rdpos = 0;
			std::size_t wrpos = 0;
			while (rdpos < len) {
				char z = str[rdpos];
				unsigned char b = t.table[(unsigned char)z];
				switch (rdpos & 0x3) {
				case 0: bbuf[wrpos] = b << 2;break;
				case 1: bbuf[wrpos]  |= b >> 4;
						bbuf[++wrpos] = b << 4;
						break;
				case 2: bbuf[wrpos] |= b >> 2;
						bbuf[++wrpos] = b << 6;
						break;
				case 3: bbuf[wrpos] |= b;
						++wrpos;
						break;
				}
				rdpos++;
			}
			return finlen;
		});
		return PValue::staticCast(strVal);
	}
	virtual void encodeBinaryValue(const BinaryView &data, const WriteFn &fn) const override {

			std::size_t len = data.length;
			if (len == 0) return ;
			char buff[256];
			std::size_t finlen = (len+2)/3 * 4;


			static char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
								  "abcdefghijklmnopqrstuvwxyz"
								  "0123456789"
					              "+/";

			std::size_t rdpos = 0;
			std::size_t wrpos = 0;
			unsigned char nx = 0;
			while (rdpos < len) {
				unsigned char c = data[rdpos];
				switch (rdpos % 3) {
				case 0: buff[wrpos++] = chars[c >> 2];
						nx = (c << 4) & 0x3F;
						break;
				case 1: buff[wrpos++] = chars[nx | (c>>4)];
						nx = (c << 2) & 0x3F;
						break;
				case 2: buff[wrpos++] = chars[nx | (c>> 6)];
						buff[wrpos++] = chars[c & 0x3F];
						break;
				}
				rdpos++;
				if (wrpos>250) {
					fn(StrViewA(buff,wrpos));
					wrpos = 0;
				}

			}
			if (rdpos % 3) {
				buff[wrpos++] = chars[nx];
			}

			while (wrpos < finlen)
				buff[wrpos++] = '=';
			fn(StrViewA(buff,wrpos));


	}


};

static Base64Encoding base64Var;
BinaryEncoding base64 = &base64Var;
BinaryEncoding defaultBinaryEncoding = &base64Var;

}
