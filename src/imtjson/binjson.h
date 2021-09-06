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
#include "base64.h"

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
	///Preloads keys to parse binary jsons created with preloaded keys
	/**
	 * You need to preload exact same keys in exact order as used during serialization
	 * @param str reference to String object containing the keys
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
	std::uint64_t parse64bit();
	Value parseNumberDouble();
	Value parseNumberFloat();
	Value parseDiff();


	template<typename T>
	void readPOD(T &x);


	Fn fn;
	BinaryEncoding curBinaryEncoding;

	std::vector<char> keybuffer;
	std::vector<String> keyHistory;
	unsigned int keyIndex = 0;

private:
	void storeKey(const String& s);
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
	/**
	 * Adds keys to the dictionary, so the key will be compressed on the first appeareance. You need
	 * add the exact same keys to the both serializer and parser in the exact same order, otherwise the
	 * result stream is generated corrupted. You have to avoid duplicated keys as well.
	 *
	 * @param str string to preload. Note the string must persist during serialization, because
	 * content of the string is not copied. If you preload multiple keys, you need also keep then
	 * valid until the serialization is done.
	 *
	 * @retval true stored
	 * @retval false
	 */
	bool preloadKey(const std::string_view &str);
	void clearKeys();

protected:
	Fn fn;

	struct ZeroID {
		unsigned int value;
		ZeroID():value(0) {}
	};

	struct HashStr {std::size_t operator()(const std::string_view &str) const;};
	typedef std::unordered_map<std::string_view, ZeroID, HashStr> KeyMap;
	KeyMap keyMap;
	unsigned int nextKeyId = 256;
	BinarySerializeFlags flags;
	std::unique_ptr<Base64Table> btable;



	template<typename T>
	void writePOD(const T &val);

	template<typename T>
	void writePOD(const T &val, unsigned char type);

	void serialize(const IValue *v);
	void serializeContainer(const IValue *v, unsigned char type);
	void serializeString(const std::string_view &str, unsigned char type);
	void serializeNumber(const IValue *v);
	void serializeInteger(std::size_t, unsigned char type);
	void serializeBoolean(const IValue *v);
	void serializeNull(const IValue *v);
	void serializeUndefined(const IValue *v);
	void serialize64bit(std::uint64_t n, unsigned char type);

	int tryCompressKey(const std::string_view &keyName);


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

