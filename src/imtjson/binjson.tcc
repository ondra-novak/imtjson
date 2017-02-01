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

namespace json {

namespace opcode {

///null value
static const unsigned char null = 0;
///boolean true
static const unsigned char booltrue = 0x01;
///boolean false
static const unsigned char boolfalse = 0x02;
///undefined value
static const unsigned char undefined = 0x03;
///number float 32bit (currently not used)
static const unsigned char numberFloat = 0x04;
///number float 64bit (double)
static const unsigned char numberDouble = 0x05;
///tag item as diff
/** it appears before object or array and tags that item as diff
 *
 * 0x0F 0x61 - diff array with one item
 * 0x0F 0x55 - diff object with 5 items
 * */
static const unsigned char diff = 0x0F;
///unsigned integer
static const unsigned char uint = 0x10;
///signed integer positive number
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
	serialize((const IValue *)v.getHandle());

}

template<typename Fn>
void BinarySerializer<Fn>::serialize(const IValue *v) {
	StrViewA key = v->getMemberName();
	if (!key.empty()) {
		serializeString(key,opcode::key);
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
		writePOD(v, type | 0xA);
	} else if (n <= 0xFFFF) {
		std::uint16_t v = (std::uint16_t)n;
		writePOD(v, type | 0xB);
	} else if (n <= 0xFFFFFFFFF) {
		std::uint32_t v = (std::uint32_t)n;
		writePOD(v, type | 0xC);
	} else {
		std::uint64_t v = (std::uint64_t)n;
		writePOD(v, type | 0xD);
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
		serializeInteger(n, opcode::uint);
	} else {
		double d = v->getNumber();
		writePOD(d,opcode::number|0xD);
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
BinaryParser<Fn>::BinaryParser(const Fn &fn):fn(fn) {}

template<typename Fn>
Value json::BinaryParser<Fn>::parseKey(unsigned char tag) {
	std::size_t sz = parseInteger(tag);
	keybuffer.resize(sz);
	for (std::size_t i = 0; i < sz; i++) keybuffer[i] = (unsigned char)fn();
	Value v = parseItem();
	return v.setKey(StrViewA(keybuffer.data(),sz));
}

template<typename Fn>
Value json::BinaryParser<Fn>::parseArray(unsigned char tag) {
	std::size_t sz = parseInteger(tag);
	auto arr = ArrayValue::create(sz);
	for (std::size_t i = 0; i < sz; i++) {
		arr->push_back(parseItem().getHandle());
	}
	return PValue::staticCast(arr);
}

template<typename Fn>
Value json::BinaryParser<Fn>::parseObject(unsigned char tag) {
	std::size_t sz = parseInteger(tag);
	auto arr = ObjectValue::create(sz);
	for (std::size_t i = 0; i < sz; i++) {
		arr->push_back(parseItem().getHandle());
	}
	return PValue::staticCast(arr);
}

template<typename Fn>
Value json::BinaryParser<Fn>::parseString(unsigned char tag) {
	std::size_t sz = parseInteger(tag);
	String s(sz, [&](char *buff) {
		for (std::size_t i = 0; i < sz; i++) buff[i] = fn();
		return sz;
	});
	return s;
}

template<typename Fn>
std::size_t json::BinaryParser<Fn>::parseInteger(unsigned char tag) {
	switch (tag & 0xF) {
	case 0: return 0;
	case 1: return 1;
	case 2: return 2;
	case 3: return 3;
	case 4: return 4;
	case 5: return 5;
	case 6: return 6;
	case 7: return 7;
	case 8: return 8;
	case 9: return 9;
	case 0xA: {
		std::uint8_t x;
		readPOD(x);
		return x;
	}
	case 0xB : {
		std::uint16_t x;
		readPOD(x);
		return x;
	}
	case 0xC : {
		std::uint32_t x;
		readPOD(x);
		return x;
	}
	case 0xD : {
		std::uint64_t x;
		readPOD(x);
		return x;
	}
	default:
		throw std::runtime_error("Unsupported integer size");
	}
}

template<typename Fn>
Value json::BinaryParser<Fn>::parseNumber(unsigned char tag) {
	if (0xD != (tag & 0xF)) {
		throw std::runtime_error("Unsupported double size");
	}
	double d;
	readPOD(d);
	return d;
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
	case opcode::null: return json::null;
	case opcode::boolfalse: return false;
	case opcode::booltrue: return true;
	case opcode::undefined: return json::undefined;
	case opcode::array:
		return parseArray(tag);
	case opcode::object:
		return parseObject(tag);
	case opcode::string:
		return parseString(tag);
	case opcode::uint:
		return Value((std::uintptr_t)parseInteger(tag));
	case opcode::posint:
		return Value((std::intptr_t)parseInteger(tag));
	case opcode::negint:
		return Value(-(std::intptr_t)parseInteger(tag));
	case opcode::number:
		return parseNumber(tag);
	case opcode::key: {
		String s = parseString(tag);
		Value v = parseItem();
		return v.setKey(s);
	}
	case opcode::padding: {
		std::size_t sz = tag & 0xF;
		for (std::size_t i = 0; i < sz; i++) fn();
		return parseItem();
	}
	default:
		throw std::runtime_error("Found unknown byte");
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


}

