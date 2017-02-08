/*
 * streams.h
 *
 *  Created on: Feb 8, 2017
 *      Author: ondra
 */

#ifndef SRC_IMTJSON_STREAMS_H_
#define SRC_IMTJSON_STREAMS_H_

#include <iostream>

#pragma once

namespace json {

///eof value
/** It is defined as -1. However, for object streams, you can use Eof_t to mask return value as "no more items" */
enum Eof_t {
	eof = -1//!< eof
};


///A helper class that provides reading from the standard stl-stream
class StreamFromStdStream {
public:
	StreamFromStdStream(std::istream &stream):stream(stream) {}
	int operator()() {
		return stream.get();
	}
private:
	std::istream &stream;
};

///Creates stream for the parser which reads bytes from the standard stream
/**
 * @param stream reference to a standard stream.
 * @return Function which returns next byte for each call. It returns -1 when eof
 */
inline StreamFromStdStream fromStream(std::istream &stream) {
	return StreamFromStdStream(stream);
}

///A helper class which provides reading from a string
class StreamFromString {
public:
	StreamFromString(const StrViewA &string):string(string),pos(0) {}
	int operator()() {
		if (pos < string.length) return (unsigned char)string[pos++];
		else return eof;
	}
private:
	StrViewA string;
	std::size_t pos;
};

///Creates stream for the parser which reads bytes from the string
/**
 * @param string string or string view to read
 * @return Function which returns next byte for each call. It returns -1 when eof
 */
inline StreamFromString fromString(const StrViewA &string) {
	return StreamFromString(string);
}

///A helper class which provides writing to a stream
class StreamToStdStream {
public:
	StreamToStdStream(std::ostream &stream):stream(stream) {}
	void operator()(char c) {
		stream.put(c);
	}
private:
	std::ostream &stream;
};

///Creates stream for the parser which writes bytes to the stl stream
/**
 * @param stream reference to the stream
 * @return Function which accepts one byte which is send to the output stream
 *  */

inline StreamToStdStream toStream(std::ostream &stream) {
	return StreamToStdStream(stream);
}

class OneCharStream {
public:
	OneCharStream(int item):item(item) {}
	int operator()() {
		int z =item;
		item = eof;
		return z;
	}
protected:
	int item;
};

inline OneCharStream oneCharStream(int item) {
	return OneCharStream(item);
}

}



#endif /* SRC_IMTJSON_STREAMS_H_ */
