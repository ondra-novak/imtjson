/*
 * binjson.cpp
 *
 *  Created on: Jan 18, 2017
 *      Author: ondra
 */
#include "binjson.h"
#include <cstdint>
#include <cstring>
#include "objectValue.h"
#include "arrayValue.h"
#include "stringValue.h"

namespace json {

namespace opcode {

///padding - value 0 is ignored and can be used to define padding
static const unsigned char padding = 0;
///null value
static const unsigned char null = 1;
///undefined value
static const unsigned char undefined = 2;
///boolean true
static const unsigned char booltrue = 3;
///boolean false
static const unsigned char boolfalse = 4;
///number float 32bit (currently not used)
static const unsigned char numberFloat = 5;
///number float 64bit (double)
static const unsigned char numberDouble = 6;


static const unsigned char size8bit = 0xA;
static const unsigned char size16bit = 0xB;
static const unsigned char size32bit = 0xC;
static const unsigned char size64bit = 0xD;

///tag item as diff
/** it appears before object or array and tags that item as diff
 *
 * 0x0F 0x61 - diff array with one item
 * 0x0F 0x55 - diff object with 5 items
 * */
static const unsigned char diff = 0x0F;
///binary string
static const unsigned char binstring = 0x10;
///unsigned or signed integer positive number
static const unsigned char posint = 0x20;
///signed integer negative number
static const unsigned char negint = 0x30;
///string
static const unsigned char string = 0x40;
///object
static const unsigned char object= 0x50;
///array
static const unsigned char array= 0x60;
///set key to the item
/**
 *  0x72 0x41 0x42 0x01 - "AB":true
 */
static const unsigned char key= 0x70;
}


template<typename Fn>
void BinarySerializer<Fn>::serialize(const Value &v) {
	nextKeyId = 256;
	serialize((const IValue *)v.getHandle());
	keyMap.clear();

}

template<typename Fn>
void BinarySerializer<Fn>::serialize(const IValue *v) {
	StrViewA key = v->getMemberName();
	if (!key.empty()) {
		if (flags & compressKeys) {
			int code = tryCompressKey(key);
			if (code == -1) serializeString(key, opcode::key);
			else fn(0x80 | (unsigned char)code);
		}
		else {
			serializeString(key, opcode::key);
		}
	}
	switch(v->type()) {
	case object: serializeContainer(v,opcode::object);break;
	case array: serializeContainer(v,opcode::array);break;
	case number: serializeNumber(v);break;
	case string: serializeString(v->getString(),opcode::string);break;
	case boolean: serializeBoolean(v);break;
	case null: serializeNull(v);break;
	default: serializeUndefined(v);break;
	}
}

template<typename Fn>
void BinarySerializer<Fn>::serializeContainer(const IValue *v, unsigned char type) {
	std::size_t cnt = v->size();
	serializeInteger(cnt, type);
	for (std::size_t i = 0; i < cnt; i++) {
		serialize(v->itemAtIndex(i));
	}
}

template<typename Fn>
void BinarySerializer<Fn>::serializeString(const StrViewA &str, unsigned char type) {
	serializeInteger(str.length,type);
	for (std::size_t i = 0; i < str.length; i++) {
		fn((unsigned char)str[i]);
	}
}

template<typename Fn>
void BinarySerializer<Fn>::serializeInteger(std::size_t n, unsigned char type) {

	if (n < 10) {
		fn(type | (unsigned char)n);
	}
	else if (n <= 0xFF) {
		std::uint8_t v = (std::uint8_t)n;
		writePOD(v, type | opcode::size8bit);
	} else if (n <= 0xFFFF) {
		std::uint16_t v = (std::uint16_t)n;
		writePOD(v, type | opcode::size16bit);
	} else if (n <= 0xFFFFFFFFF) {
		std::uint32_t v = (std::uint32_t)n;
		writePOD(v, type | opcode::size32bit);
	}
	else if (flags & maintain32BitComp) {
		double v = (double)n;
		writePOD(v, opcode::numberDouble);
	} else {
		std::uint64_t v = (std::uint64_t)n;
		writePOD(v, type | opcode::size64bit);
	}

}


template<typename Fn>
void BinarySerializer<Fn>::serializeNumber(const IValue *v) {
	ValueTypeFlags flags = v->flags();
	if (flags & numberInteger) {
		std::intptr_t n = v->getInt();
		if (n < 0) serializeInteger((std::size_t)(-n), opcode::negint);
		else serializeInteger((std::size_t)(n), opcode::posint);
	} else if (flags & numberUnsignedInteger) {
		std::size_t n = v->getUInt();
		serializeInteger(n, opcode::posint);
	} else {
		double d = v->getNumber();
		writePOD(d,opcode::numberDouble);
	}

}
template<typename Fn>
void BinarySerializer<Fn>::serializeBoolean(const IValue *v) {

	unsigned char opc = v->getBool()?opcode::booltrue:opcode::boolfalse;
	fn(opc);
}
template<typename Fn>
void BinarySerializer<Fn>::serializeNull(const IValue *v) {
	unsigned char opc = opcode::null;
	fn(opc);
}
template<typename Fn>
void BinarySerializer<Fn>::serializeUndefined(const IValue *) {
	unsigned char opc = opcode::undefined;
	fn(opc);
}


template<typename Fn>
BinaryParser<Fn>::BinaryParser(const Fn &fn, BinaryEncoding binaryEncoding):fn(fn),curBinaryEncoding(binaryEncoding) {}

template<typename Fn>
Value json::BinaryParser<Fn>::parseKey(unsigned char tag) {
	std::size_t sz = parseInteger(tag);
	keybuffer.resize(sz);
	for (std::size_t i = 0; i < sz; i++) keybuffer[i] = (unsigned char)fn();
	Value v = parseItem();
	return v.setKey(StrViewA(keybuffer.data(),sz));
}

template<typename Fn>
Value json::BinaryParser<Fn>::parseArray(unsigned char tag, bool ) {
	std::size_t sz = parseInteger(tag);
	auto arr = ArrayValue::create(sz);
	for (std::size_t i = 0; i < sz; i++) {
		arr->push_back(parseItem().getHandle());
	}
	return PValue::staticCast(arr);
}

template<typename Fn>
Value json::BinaryParser<Fn>::parseObject(unsigned char tag, bool diff) {
	std::size_t sz = parseInteger(tag);
	auto arr = ObjectValue::create(sz);
	arr->isDiff = diff;
	for (std::size_t i = 0; i < sz; i++) {
		arr->push_back(parseItem().getHandle());
	}
	return PValue::staticCast(arr);
}

template<typename Fn>
Value json::BinaryParser<Fn>::parseString(unsigned char tag, BinaryEncoding encoding) {
	std::size_t sz = parseInteger(tag);
	RefCntPtr<StringValue> s = new(sz) StringValue(encoding, sz, [&](char *buff) {
		for (std::size_t i = 0; i < sz; i++) buff[i] = fn();
		return sz;
	});
	return PValue::staticCast(s);
}

template<typename Fn>
std::size_t json::BinaryParser<Fn>::parseInteger(unsigned char tag) {
	switch (tag & 0xF) {
	case opcode::size8bit: {
		std::uint8_t x;
		readPOD(x);
		return x;
	}
	case opcode::size16bit: {
		std::uint16_t x;
		readPOD(x);
		return x;
	}
	case opcode::size32bit: {
		std::uint32_t x;
		readPOD(x);
		return x;
	}
	case opcode::size64bit : {
		if (sizeof(std::size_t) == sizeof(std::uint64_t)) {
			std::uint64_t x;
			readPOD(x);
			return (std::size_t) x;
		}
		else {
			throw std::runtime_error("Too large integer for this platform");
		}
	}
	default:
		return tag & 0xF;
	}
}




template<typename Fn>
template<typename T>
void BinaryParser<Fn>::readPOD(T &x) {
	unsigned char *t = reinterpret_cast<unsigned char *>(&x);
	for (std::size_t i = 0; i <sizeof(T);i++)
		t[i] = fn();
}

template<typename Fn>
Value BinaryParser<Fn>::parse() {	
	return parseItem();
 }

template<typename Fn>
Value BinaryParser<Fn>::parseItem() {
	unsigned char tag;
	tag = (unsigned char)fn();
	switch (tag & 0xF0) {
	case 0: switch (tag) {
		case opcode::null: return json::null;
		case opcode::boolfalse: return false;
		case opcode::booltrue: return true;
		case opcode::undefined: return json::undefined;
		case opcode::numberDouble: return parseNumberDouble();
		case opcode::numberFloat: return parseNumberFloat();
		case opcode::diff: return parseDiff();
		default: throw std::runtime_error("Found unknown byte");
	};
	case opcode::array:
		return parseArray(tag,false);
	case opcode::object:
		return parseObject(tag,false);
	case opcode::string:
		return parseString(tag,nullptr);
	case opcode::binstring:
		return parseString(tag,curBinaryEncoding);
	case opcode::posint:
		return Value((std::intptr_t)parseInteger(tag));
	case opcode::negint:
		return Value(-(std::intptr_t)parseInteger(tag));
	case opcode::key: {
		String s(parseString(tag, nullptr));
		keyHistory[keyIndex & 0x7F] = s;
		keyIndex++;
		Value v = parseItem();
		return Value(s, v);
	}
	default: {
		unsigned int p = (keyIndex - 1 - tag) & 0x7F;
		String s = keyHistory[p];
		Value v = parseItem();
		return Value(s,v);
	}
		
	}
}

template<typename Fn>
template<typename T>
void BinarySerializer<Fn>::writePOD(const T &val) {
	auto c = reinterpret_cast<const unsigned char *>(&val);
	for (unsigned int i = 0; i < sizeof(T); i++) fn(*c++);
}

template<typename Fn>
template<typename T>
void BinarySerializer<Fn>::writePOD(const T &val, unsigned char type) {
	fn(type);
	writePOD(val);
}

template<typename Fn>
inline Value BinaryParser<Fn>::parseNumberDouble() {
	double d;
	readPOD(d);
	return d;
}

template<typename Fn>
inline Value BinaryParser<Fn>::parseNumberFloat() {
	float d;
	readPOD(d);
	return d;
}

template<typename Fn>
inline Value BinaryParser<Fn>::parseDiff() {
	unsigned char opcode = fn();
	switch (opcode & 0xF0) {
		case opcode::object: return parseObject(opcode,true);
		default: throw std::runtime_error("undefined opcode sequence");
	}
}

template<typename Fn>
inline int BinarySerializer<Fn>::tryCompressKey(const StrViewA &key){
	ZeroID &id = keyMap[key];
	unsigned int dist = (nextKeyId - id.value) - 1;
	if (dist < 128) {
		return dist;
	}
	else {
		id.value = nextKeyId;
		++nextKeyId;
		return -1;
	}
}

template<typename Fn>
std::size_t  BinarySerializer<Fn>::HashStr::operator()(const StrViewA &str) const {
	std::size_t acc = 2166136261;
	for(auto c : str) {
		unsigned char b = (unsigned char)c;
		acc = acc ^ b;
		acc = acc * 16777619;
	}
	return acc;

}

}

