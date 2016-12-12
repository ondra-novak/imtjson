/*
 * stringValue.cpp
 *
 *  Created on: Oct 25, 2016
 *      Author: ondra
 */

#include <cstring>

#include "stringValue.h"

#include "parser.h"

namespace json {

static const StringView<char> magic("immujson");

void *StringValue::putMagic(void *obj) {
	StringValue *s = reinterpret_cast<StringValue *>(obj);
	std::memcpy(s->charbuff, magic.data, magic.length);
	return obj;
}

json::StringValue::StringValue(const StringView<char>& str):size(str.length) {
	char *trg = charbuff;
	if (StringView<char>(trg,magic.length) != magic) throw std::runtime_error("StringView must be allocated by special new operator");
	std::memcpy(trg, str.data, str.length);
	trg[str.length] = 0;
}

StringView<char> json::StringValue::getString() const {
	return StringView<char>(charbuff, size);
}

std::intptr_t json::AbstractStringValue::getInt() const {
	std::size_t pos = 0;
	return Value::fromString(getString()).getInt();
}

std::uintptr_t json::AbstractStringValue::getUInt() const {
	std::size_t pos = 0;
	return Value::fromString(getString()).getUInt();
}

void* json::StringValue::operator new(std::size_t sz, const StringView<char>& str) {
	std::size_t needsz = sz - sizeof(StringValue::charbuff) + std::max(str.length+1,magic.length);
	return putMagic(::operator new(needsz));
}

void json::StringValue::operator delete(void* ptr,const StringView<char>& str) {
	::operator delete(ptr);
}

void* json::StringValue::operator new(std::size_t sz, const std::size_t &strsz) {
	std::size_t needsz = sz - sizeof(StringValue::charbuff) + std::max(strsz+1, magic.length);
	return putMagic(::operator new(needsz));
}

void json::StringValue::operator delete(void* ptr, const std::size_t &sz) {
	::operator delete(ptr);
}

void json::StringValue::operator delete(void* ptr, std::size_t sz) {
	::operator delete(ptr);
}

double json::AbstractStringValue::getNumber() const {
	std::size_t pos = 0;
	return Value::fromString(getString()).getNumber();
}

void json::StringValue::stringOverflow() {
	std::cerr << "String buffer overflow, aborting" << std::endl << std::flush;
	abort();
}


}

