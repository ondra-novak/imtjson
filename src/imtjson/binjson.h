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
	BinaryParser(const Fn &fn, BinaryEncoding binaryEncoding);

	Value parse();
protected:

	Value parseItem();

	Value parseKey(unsigned char tag);
	Value parseArray(unsigned char tag, bool diff);
	Value parseObject(unsigned char tag,bool diff);
	Value parseString(unsigned char tag, BinaryEncoding encoding);
	std::size_t parseInteger(unsigned char tag);
	Value parseNumberDouble();
	Value parseNumberFloat();
	Value parseDiff();

	template<typename T>
	void readPOD(T &x);


	Fn fn;
	BinaryEncoding curBinaryEncoding;

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
	BinarySerializer(const Fn &fn, bool compressKeys):fn(fn),compressKeys(compressKeys) {}

	void serialize(const Value &v);


protected:
	Fn fn;

	struct ZeroID {
		unsigned int value;
		ZeroID():value(0) {}
	};

	typedef std::map<StrViewA, ZeroID> KeyMap;
	KeyMap keyMap;
	unsigned int nextKeyId;
	bool compressKeys;

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

	int tryCompressKey(const StrViewA &keyName);


};

template<typename Fn>
inline void Value::serializeBinary(const Fn &fn , bool compressKeys) {
	BinarySerializer<Fn> ser(fn);
	ser.serialize(*this);
}

template<typename Fn>
inline Value Value::parseBinary(const Fn &fn, BinaryEncoding binEnc) {
	BinaryParser<Fn> parser(fn,binEnc);
	Value r = parser.parse();
	return r;
}

}

#endif /* SRC_IMTJSON_BINJSON_H_ */

