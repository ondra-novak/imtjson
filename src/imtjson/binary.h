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

class Binary: public BinaryView {
public:

	Binary(const String &s):BinaryView(createBinaryView(s)),s(s) {}

protected:
	String s;

	static BinaryView createBinaryView(const String &s) {
		StrViewA str(s);
		return BinaryView(str);
	}
};



}




#endif /* SRC_IMTJSON_BINARY_H_ */
