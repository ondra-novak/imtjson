/*
 * binjson.h
 *
 *  Created on: Jan 18, 2017
 *      Author: ondra
 */

#ifndef SRC_IMTJSON_BINJSON_H_
#define SRC_IMTJSON_BINJSON_H_

#include "value.h"

namespace json {

///Parse JSON from binary form
/**
 * @tparam Fn function which returns next byte. Function cannot return EOF, it is considered as error
 *
 */
template<typename Fn>
class BinaryParser {
public:
	BinaryParser(const Fn &fn);

	Value parse();
protected:

	Value parseItem();

	Value parseKey(unsigned char tag);
	Value parseArray(unsigned char tag);
	Value parseObject(unsigned char tag);
	Value parseString(unsigned char tag);
	std::size_t parseInteger(unsigned char tag);
	Value parseNumber(unsigned char tag);


	template<typename T>
	void readPOD(T &x);


	Fn fn;

	std::vector<char> keybuffer;
};


///Serializes JSON into binary form
/** Binary form is not necesery smaller, but should be much faster
 * to parse.
 * @tparam Fn function which accepts unsigned char as argument
 *
 */
template<typename Fn>
class BinarySerializer {
public:
	BinarySerializer(const Fn &fn):fn(fn) {}

	void serialize(const Value &v);


protected:
	Fn fn;

	template<typename T>
	void writePOD(const T &val);

	template<typename T>
	void writePOD(const T &val, unsigned char type);

	void serialize(const IValue *v);
	void serializeContainer(const IValue *v, unsigned char type);
	void serializeString(const StrViewA &str, unsigned char type);
	void serializeNumber(const IValue *v);
	void serializeInteger(std::size_t, unsigned char type);
	void serializeBoolean(const IValue *v);
	void serializeNull(const IValue *v);
	void serializeUndefined(const IValue *v);



};

template<typename Fn>
inline void Value::serializeBinary(const Fn &fn ) {
	BinarySerializer<Fn> ser(fn);
	ser.serialize(*this);
}

template<typename Fn>
inline Value Value::parseBinary(const Fn &fn) {
	BinaryParser<Fn> parser(fn);
	Value r = parser.parse();
	return r;
}

}


#endif /* SRC_IMTJSON_BINJSON_H_ */

