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


json::StringValue::StringValue(const StringView<char>& str):size(str.length) {
	char *trg = charbuff;
	std::memcpy(trg, str.data, str.length);
	trg[str.length] = 0;
}

StringView<char> json::StringValue::getString() const {
	return StringView<char>(charbuff, size);
}

std::intptr_t json::StringValue::getInt() const {
	std::size_t pos = 0;
	return Value::parse([&pos,this](){return charbuff[pos++];}).getInt();
}

std::uintptr_t json::StringValue::getUInt() const {
	std::size_t pos = 0;
	return Value::parse([&pos,this](){return charbuff[pos++];}).getUInt();
}

void* json::StringValue::operator new(std::size_t sz, const StringView<char>& str) {
	std::size_t needsz = sz - sizeof(StringValue::charbuff) + str.length+1;
	return ::operator new(needsz);
}

void json::StringValue::operator delete(void* ptr,const StringView<char>& str) {
	::operator delete(ptr);
}

void* json::StringValue::operator new(std::size_t sz, const std::size_t &strsz) {
	std::size_t needsz = sz - sizeof(StringValue::charbuff) + strsz+1;
	return ::operator new(needsz);
}

void json::StringValue::operator delete(void* ptr, const std::size_t &sz) {
	::operator delete(ptr);
}

void json::StringValue::operator delete(void* ptr, std::size_t sz) {
	::operator delete(ptr);
}

double json::StringValue::getNumber() const {
	std::size_t pos = 0;
	return Value::parse([&pos,this](){return charbuff[pos++];}).getNumber();
}


}

