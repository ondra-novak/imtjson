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
	Base64Table(const char *chars) {
		for (std::size_t i = 0; i <256; i++) table[i] = 0xFF;
		for (std::size_t i = 0; i < 64; i++) table[(unsigned char)(chars[i])] = i;
	}
};


static char base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				  "abcdefghijklmnopqrstuvwxyz"
				  "0123456789"
	              "+/";
static char base64urlchars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				  "abcdefghijklmnopqrstuvwxyz"
				  "0123456789"
	              "-_";


class Base64Encoding: public IBinaryEncoder {
public:


	virtual StrViewA getName() const {
		return "base64";
	}


	static void decoderCore(char *buff, const StrViewA &str, std::size_t len, const Base64Table &t) {
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
	}

	virtual Value decodeBinaryValue(const StrViewA &str) const override {
		static Base64Table t(base64chars);
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
			decoderCore(buff,str, len,t);
			return finlen;
		});
		return PValue::staticCast(strVal);
	}
	virtual void encodeBinaryValue(const BinaryView &data, const WriteFn &fn) const override {



		encodeCore(data, base64chars, fn,true);
	}

	void encodeCore(const BinaryView& data, char *chars, WriteFn fn, bool tail) const {
		std::size_t len = data.length;
		if (len != 0) {
			std::size_t nlen = (len + 2) / 3;
			std::size_t finlen = nlen * 4;
			std::size_t padd = (nlen * 3) - len;
			char buff[256];
			std::size_t rdpos = 0;
			std::size_t wrpos = 0;
			unsigned char nx = 0;
			while (rdpos < len) {
				unsigned char c = data[rdpos];
				switch (rdpos % 3) {
				case 0:
					buff[wrpos++] = chars[c >> 2];
					nx = (c << 4) & 0x3F;
					break;
				case 1:
					buff[wrpos++] = chars[nx | (c >> 4)];
					nx = (c << 2) & 0x3F;
					break;
				case 2:
					buff[wrpos++] = chars[nx | (c >> 6)];
					buff[wrpos++] = chars[c & 0x3F];
					break;
				}
				rdpos++;
				if (wrpos > 250) {
					fn(StrViewA(buff, wrpos));
					wrpos = 0;
				}
			}
			if (rdpos % 3) {
				buff[wrpos++] = chars[nx];
			}
			if (tail) {
				for (std::size_t i = 0; i < padd; i++) {
					buff[wrpos++] = '=';
				}
			}
			fn(StrViewA(buff, wrpos));
		}
	}
};

class Base64EncodingUrl: public Base64Encoding {
public:
	virtual StrViewA getName() const {
		return "base64url";
	}
	virtual Value decodeBinaryValue(const StrViewA &str) const override {
			static Base64Table t(base64urlchars);
			std::size_t len = str.length;
			std::size_t endlen = (len & 3);
			if (endlen == 1) return String();
			std::size_t finlen = (len / 4) * 3 + (endlen > 1?endlen-1:0);
			RefCntPtr<StringValue> strVal = new(len) StringValue(base64,len,[&] (char *buff) {
				decoderCore(buff,str, len,t);
				return finlen;
			});
			return PValue::staticCast(strVal);
	}
	virtual void encodeBinaryValue(const BinaryView &data, const WriteFn &fn) const override {
		encodeCore(data,base64urlchars, fn, false);
	}

};

static Base64Encoding base64Var;
static Base64EncodingUrl base64UrlVar;
BinaryEncoding base64 = &base64Var;
BinaryEncoding base64url = &base64UrlVar;
BinaryEncoding defaultBinaryEncoding = &base64Var;

}
