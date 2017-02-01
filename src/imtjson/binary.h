/*
 * binary.h
 *
 *  Created on: Jan 11, 2017
 *      Author: ondra
 */

#ifndef SRC_IMTJSON_BINARY_H_
#define SRC_IMTJSON_BINARY_H_

#pragma once

#include "string.h"

namespace json {

class Binary;

class IBinaryEncoder {
public:
	typedef std::function<void(StrViewA)> WriteFn;

	///Function decodes string and stores result as BinaryValue to the Value variable
	virtual Value decodeBinaryValue(const StrViewA &string) const = 0;
	///Function encodes a binary view and writes it through the given function
	virtual void encodeBinaryValue(const BinaryView &binary, const WriteFn &fn) const = 0;
	///Retrieves name of the encoding type
	/** This is handful if you need to put choosen encoding into the output*/
	virtual StrViewA getName() const = 0;
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

	BinaryEncoding getEncoding() const;
	static BinaryEncoding getEncoding(const IValue *v);

	static Binary decodeBinaryValue(const Value &s, BinaryEncoding encoding);
	static void encodeBinaryValue(std::function<void(StrViewA)> writeFn);

	PValue getHandle() const {return s;}


protected:

	Binary(const Value &s):BinaryView(createBinaryView(s)),s(s.getHandle()) {}

	PValue s;

	static BinaryView createBinaryView(const Value &s) {
		StrViewA str(s.getString());
		return BinaryView(str);
	}
};



}




#endif /* SRC_IMTJSON_BINARY_H_ */
