/*
 * stringValue.cpp
 *
 *  Created on: Oct 25, 2016
 *      Author: ondra
 */

#include <cstring>

#include "stringValue.h"

#include "parser.h"
#include "allocator.h"
#include "binary.h"

namespace json {

static const StringView<char> magic("imtjson");

void *StringValue::putMagic(void *obj) {
	StringValue *s = reinterpret_cast<StringValue *>(obj);
	std::memcpy(s->charbuff, magic.data, magic.length);
	return obj;
}

json::StringValue::StringValue(BinaryEncoding enc, const StringView<char>& str):sz(str.length),encoding(enc) {
	char *trg = charbuff;
	if (StringView<char>(trg,magic.length) != magic) throw std::runtime_error("StringView must be allocated by special new operator");
	std::memcpy(trg, str.data, str.length);
	trg[str.length] = 0;
}

StringView<char> json::StringValue::getString() const {
	return StringView<char>(charbuff, sz);
}

std::intptr_t json::AbstractStringValue::getInt() const {
	std::size_t pos = 0;
	return Value::fromString(getString()).getInt();
}

std::uintptr_t json::AbstractStringValue::getUInt() const {
	std::size_t pos = 0;
	return Value::fromString(getString()).getUInt();
}


void* json::StringValue::operator new(std::size_t sz, const std::size_t &strsz) {
	std::size_t objsz = sz - sizeof(StringValue::charbuff);
	std::size_t needsz = objsz + std::max(strsz+1, magic.length);
	return putMagic(Value::allocator->alloc(needsz));
}

void json::StringValue::operator delete(void* ptr, const std::size_t &) {
	Value::allocator->dealloc(ptr);
}

void json::StringValue::operator delete(void* ptr, std::size_t ) {
	Value::allocator->dealloc(ptr);
}

double json::AbstractStringValue::getNumber() const {
	std::size_t pos = 0;
	return Value::fromString(getString()).getNumber();
}

PValue StringValue::create(const StringView<char>& str) {
	if (str.empty()) return AbstractStringValue::getEmptyString();
	else return new(str.length) StringValue(nullptr,str);
}

void json::StringValue::stringOverflow() {
	std::cerr << "String buffer overflow, aborting" << std::endl << std::flush;
	abort();
}


PValue StringValue::create(const BinaryView& str, BinaryEncoding enc) {
	StringValue *v = new(str.length) StringValue(enc,StrViewA(str));
	IValue *iv = v;
	return iv;
}

BinaryEncoding StringValue::getEncoding(const IValue* v) {
	if (v->type() == string && v->flags() & binaryString) {
		const StringValue *bv = static_cast<const StringValue *>(v);
		return bv->encoding;
	} else {
		return directEncoding;
	}
}

}
