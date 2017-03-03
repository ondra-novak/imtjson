/*
 * binjson.h
 *
 *  Created on: Jan 18, 2017
 *      Author: ondra
 */

#ifndef SRC_IMTJSON_BINJSON_H_
#define SRC_IMTJSON_BINJSON_H_

#include "value.h"
#include <unordered_map>
#include <memory>

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

	///Parse binary json
	Value parse();
	///Initializes dictionary.
	/**The dictionary must be initialized in same order as the dictionary of
	 the serializer. Call this function for one or more keys
	 */
	void preloadKey(const String &str);
	///Clears all keys
	void clearKeys();
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
	String keyHistory[128];
	unsigned int keyIndex = 0;
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
	BinarySerializer(const Fn &fn, BinarySerializeFlags flags):fn(fn), flags(flags) {}

	///Serialize to binary stream
	void serialize(const Value &v);
	///Preloads to achieve better compression from the beginning
	/** The keys preloaded here will occupy minimum space. You can preload up to 128 keys. Note that
	 * if there is more keys in the json, they can eventually slip out from the dictionary and then
	 * they may appear uncompressed.
	 *
	 * Function is inteed to be used to reduce space for short json's with a few keys.
	 * @param str preloaded key. Call this function repatedly for each key to preload
	 *
	 * @note the parser needs to preload exact the same keys in the same order. Not following this rule
	 * will lead to file/stream corruption. Different or empty keys can appear during parsing
	 */
	void preloadKey(const String &str);
	void clearKeys();

protected:
	Fn fn;

	struct ZeroID {
		unsigned int value;
		ZeroID():value(0) {}
	};

	struct HashStr {std::size_t operator()(const StrViewA &str) const;};
	typedef std::unordered_map<StrViewA, ZeroID, HashStr> KeyMap;
	KeyMap keyMap;
	unsigned int nextKeyId = 256;
	BinarySerializeFlags flags;

	struct PreloadedKeys{
		String keys[128];
		unsigned int keyIndex = 0;
	};
	std::unique_ptr<PreloadedKeys> pkeys;


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
inline void Value::serializeBinary(const Fn &fn , BinarySerializeFlags flags) const {
	BinarySerializer<Fn> ser(fn, flags);
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

