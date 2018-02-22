#include "base64.h"

#include "stringValue.h"

namespace json {


Base64Table::Base64Table(const char *chars) {
	for (std::size_t i = 0; i <256; i++) table[i] = 0xFF;
	for (std::size_t i = 0; i < 64; i++) table[(unsigned char)(chars[i])] = i;
}

char Base64Table::base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				  "abcdefghijklmnopqrstuvwxyz"
				  "0123456789"
	              "+/";
char Base64Table::base64urlchars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				  "abcdefghijklmnopqrstuvwxyz"
				  "0123456789"
	              "-_";


StrViewA Base64Encoding::getName() const {
	return "base64";
}


void Base64Encoding::decoderCore(unsigned char *buff, const StrViewA &str, std::size_t len, const Base64Table &t) {
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

Value Base64Encoding::decodeBinaryValue(const StrViewA &str) const {
	static Base64Table t(Base64Table::base64chars);
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
		decoderCore(reinterpret_cast<unsigned char *>(buff),str, len,t);
		return finlen;
	});
	return PValue::staticCast(strVal);
}
void Base64Encoding::encodeBinaryValue(const BinaryView &data, const WriteFn &fn) const {



	encodeCore(data, Base64Table::base64chars, fn,true);
}

void Base64Encoding::encodeCore(const BinaryView& data, char *chars, WriteFn fn, bool tail)  {
	std::size_t len = data.length;
	if (len != 0) {
		std::size_t nlen = (len + 2) / 3;
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

StrViewA Base64EncodingUrl::getName() const  {
	return "base64url";
}
Value Base64EncodingUrl::decodeBinaryValue(const StrViewA &str) const  {
		static Base64Table t(Base64Table::base64urlchars);
		std::size_t len = str.length;
		std::size_t finlen = (len*3+3)/4;
		RefCntPtr<StringValue> strVal = new(len) StringValue(base64,len,[&] (char *buff) {
			decoderCore(reinterpret_cast<unsigned char *>(buff),str, len,t);
			return finlen;
		});
		return PValue::staticCast(strVal);
}
void Base64EncodingUrl::encodeBinaryValue(const BinaryView &data, const WriteFn &fn) const {
	encodeCore(data,Base64Table::base64urlchars, fn, false);
}

}
