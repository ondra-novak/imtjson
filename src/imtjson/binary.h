/*
 * binary.h
 *
 *  Created on: Jan 11, 2017
 *      Author: ondra
 */

#ifndef SRC_IMTJSON_BINARY_H_
#define SRC_IMTJSON_BINARY_H_

#pragma once

#include <string_view>
#include "string.h"

namespace json {

using BinaryView = std::basic_string_view<unsigned char>;

inline BinaryView map_str2bin(const std::string_view &str) {return BinaryView(reinterpret_cast<const unsigned char *>(str.data()), str.size());}
inline StringView map_bin2str(const BinaryView &str) {return StringView(reinterpret_cast<const char *>(str.data()), str.size());}
inline StringView map_bin2str(const std::vector<unsigned char> &str) {return StringView(reinterpret_cast<const char *>(str.data()), str.size());}

class Binary;

class IBinaryEncoder {
public:
	typedef std::function<void(std::string_view)> WriteFn;

	///Function decodes string and stores result as BinaryValue to the Value variable
	virtual Value decodeBinaryValue(const std::string_view &string) const = 0;
	///Function encodes a binary view and writes it through the given function
	virtual void encodeBinaryValue(const BinaryView &binary, const WriteFn &fn) const = 0;
	///Encode binary value and return it as string value
	virtual Value encodeBinaryValue(const BinaryView &binary) const = 0;
	///Retrieves name of the encoding type
	/** This is handful if you need to put choosen encoding into the output*/
	virtual std::string_view getName() const = 0;
	virtual ~IBinaryEncoder() {}

};



///Contains binary value
/** Object can be created by function getBinary(). You can alse call decodeBinaryValue to receive this object.
 *
 * Function holds binary content which can be either decoded from the string, or a direct binary data. The object
 * also remembers type of encoding used to encoded the binary into the string
 */
class Binary: public BinaryView {
public:

	Binary() {}

	BinaryEncoding getEncoding() const;
	static BinaryEncoding getEncoding(const IValue *v);

	static Binary decodeBinaryValue(const Value &s, BinaryEncoding encoding);
	static void encodeBinaryValue(std::function<void(std::string_view)> writeFn);

	PValue getHandle() const {return s;}


protected:

	Binary(const Value &s):BinaryView(createBinaryView(s)),s(s.getHandle()) {}

	PValue s;

	static BinaryView createBinaryView(const Value &s) {
		std::string_view str(s.getString());
		return BinaryView(reinterpret_cast<const unsigned char *>(str.data()), str.size());
	}
};


class AbstractBinaryEncoder: public IBinaryEncoder {
public:
	using IBinaryEncoder::encodeBinaryValue;
	virtual Value encodeBinaryValue(const BinaryView &binary) const override ;
};


}




#endif /* SRC_IMTJSON_BINARY_H_ */
