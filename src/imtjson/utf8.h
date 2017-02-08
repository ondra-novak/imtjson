/*
 * utf8.h
 *
 *  Created on: Feb 8, 2017
 *      Author: ondra
 */

#ifndef SRC_IMTJSON_UTF8_H_
#define SRC_IMTJSON_UTF8_H_

#pragma once

#include "streams.h"

namespace json {

///Instance of this class implements function which is converting wide chars to utf-8
class WideToUtf8 {
public:


	///Converts wide chars generated by Input to utf-8 and sends thme to function Output
	/**
	 * @param inputFn input function returning wide chars
	 * @param outputFn output function which receives utf-8 sequence
	 *
	 * Function stops when Input returns eof
	 */
	template<typename Input, typename Output>
	void operator()(Input inputFn, Output outputFn) {
		int wchar = inputFn();
		while (wchar != eof) {
			if (wchar < 128) {
				outputFn((char)wchar);
			}
			else if (wchar >= 0x80 && wchar <= 0x7FF) {
				outputFn((char)(0xC0 | (wchar >> 6)));
				outputFn((char)(0x80 | (wchar & 0x3F)));
			}
			else if (wchar >= 0x800 && wchar <= 0xFFFF) {
				outputFn((char)(0xE0 | (wchar >> 12)));
				outputFn((char)(0x80 | ((wchar >> 6) & 0x3F)));
				outputFn((char)(0x80 | (wchar & 0x3F)));
			}
			else if (wchar >= 0x10000 && wchar <= 0x10FFFF) {
				outputFn((char)(0xF0 | (wchar >> 18)));
				outputFn((char)(0x80 | ((wchar >> 12) & 0x3F)));
				outputFn((char)(0x80 | ((wchar >> 6) & 0x3F)));
				outputFn((char)(0x80 | (wchar & 0x3F)));
			}
			wchar = inputFn();
		}
	}
};


///Instance of this class acts as function, which is able to convert utf-8 stream into wide-char stream
/** see description of operator() */
class Utf8ToWide {
	unsigned int uchar = 0;
	unsigned int b = 0;
public:
	///Converts utf-8 stream to wide-char stream
	/**
	 * Function reads input stream (which is implemented as function returning int) until the -1 is received.
	 * Each number is considered as character (convert from unsigned char). Result is send to output stream
	 * which is implemented as function accepting wide charactier
	 *
	 * @param inputFn function which returns bytes for each call. It should return -1 to end of stream
	 * @param outputFn function which receives result: wide chars'
	 *
	 * @note inputFn can return -1 to stop converting the characters. The state of the converter
	 * is remembered, so if it is called later, it will continue in conversion process. This allows
	 * to supply input data per blocks without need to track where each utf-8 code
	 *
	 */
	template<typename Input, typename Output>
	void operator()(Input inputFn, Output outputFn) {
		int c = inputFn();
		while (c!= eof) {
			if ((c & 0xC0) == 0x80) {
				uchar = (uchar << 6) | (c & 0x3F);
				b--;
				if (b == 0) outputFn(uchar);
			} else if ((c & 0xE0) == 0xC0) {
				uchar = c & 0x1F;
				b = 1;
			} else if ((c & 0xF0) == 0xE0) {
				uchar = c & 0x0F;
				b = 2;
			} else if ((c & 0xF8) == 0xF0) {
				uchar = c & 0x07;
				b = 3;
			} else {
				outputFn(c);
			}
			c = inputFn();
		}
	}

};


}



#endif /* SRC_IMTJSON_UTF8_H_ */
