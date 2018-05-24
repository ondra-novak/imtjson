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

StringValue::StringValue(BinaryEncoding enc, const StringView<char>& str):sz(str.length),encoding(enc) {
	char *trg = charbuff;
	if (StringView<char>(trg,magic.length) != magic) throw std::runtime_error("StringView must be allocated by special new operator");
	std::memcpy(trg, str.data, str.length);
	trg[str.length] = 0;
}

StringView<char> StringValue::getString() const {
	return StringView<char>(charbuff, sz);
}

std::intptr_t AbstractStringValue::getInt() const {

	return Value::fromString(getString()).getInt();
}

std::uintptr_t AbstractStringValue::getUInt() const {

	return Value::fromString(getString()).getUInt();
}


void* StringValue::operator new(std::size_t sz, const std::size_t &strsz) {
	std::size_t objsz = sz - sizeof(StringValue::charbuff);
	std::size_t needsz = objsz + std::max(strsz+1, magic.length);
	return putMagic(Value::allocator->alloc(needsz));
}

void StringValue::operator delete(void* ptr, const std::size_t &) {
	Value::allocator->dealloc(ptr);
}

void StringValue::operator delete(void* ptr, std::size_t ) {
	Value::allocator->dealloc(ptr);
}

double AbstractStringValue::getNumber() const {

	return Value::fromString(getString()).getNumber();
}

PValue StringValue::create(const StringView<char>& str) {
	if (str.empty()) return AbstractStringValue::getEmptyString();
	else return new(str.length) StringValue(nullptr,str);
}

void StringValue::stringOverflow() {
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

template<typename T> struct NumbParser;
template<> struct NumbParser<double> {
	static double parse(const char *v) {
		return strtod(v,nullptr);
	}
	static const ValueTypeFlags flags = preciseNumber;
};

template<> struct NumbParser<std::uintptr_t> {
	static std::uintptr_t parse(const char *v) {
		std::uintptr_t x = 0;
		while (isdigit(*v)) {
			x = x * 10 +(*v - '0');++v;
		}
		return x;
	}
	static const ValueTypeFlags flags = preciseNumber|numberUnsignedInteger;
};

template<> struct NumbParser<std::intptr_t> {
	static std::intptr_t parse(const char *v) {
		std::uintptr_t x = 0;
		bool neg = false;
		switch (*v) {
		case '+': ++v;break;
		case '-': ++v;neg = true;
		default:break;
		};
		while (isdigit(*v)) {
			x = x * 10 +(*v - '0');++v;
		}
		return neg?-(std::intptr_t)x:(std::intptr_t)x;
	}
	static const ValueTypeFlags flags = preciseNumber|numberInteger;
};

template<typename T>
void PreciseNumberValue<T>::cacheValue() const {
	n = NumbParser<T>::parse(charbuff);
	cached = true;
}


template<typename T>
double PreciseNumberValue<T>::getNumber() const {
	if (!cached) cacheValue();
	return (double)n;
}

template<typename T>
std::intptr_t PreciseNumberValue<T>::getInt() const {
	if (!cached) cacheValue();
	return (std::intptr_t)n;
}

template<typename T>
std::uintptr_t PreciseNumberValue<T>::getUInt() const {
	if (!cached) cacheValue();
	return (std::uintptr_t)n;
}

template<typename T>
bool PreciseNumberValue<T>::getBool() const {
	return n != 0;
}

template<typename T>
ValueTypeFlags PreciseNumberValue<T>::flags() const {
	return NumbParser<T>::flags;
}

template<typename T>
StringView<char> PreciseNumberValue<T>::getString() const {
	return charbuff;
}

template<typename T>
PValue PreciseNumberValue<T>::create(const StringView<char>& str) {
	return new(str.length) PreciseNumberValue<T>(str);

}

template<typename T>
PValue PreciseNumberValue<T>::create(const T& value, const StringView<char>& str) {
	return new(str.length) PreciseNumberValue<T>(value,str);
}

template<typename T>
PreciseNumberValue<T>::~PreciseNumberValue() {
}

template<typename T>
void PreciseNumberValue<T>::operator delete(void* ptr,	std::size_t sz) {
	Value::allocator->dealloc(ptr);
}

template<typename T>
PreciseNumberValue<T>::PreciseNumberValue(const StrViewA& strNum):cached(false) {
	char *c = charbuff;
	for (char a: strNum) *c++ = a;
	*c = 0;
}

template<typename T>
PreciseNumberValue<T>::PreciseNumberValue(const T &v, const StrViewA &strNum):PreciseNumberValue(strNum) {
	n = v;
	cached = true;
}

template<typename T>
void* PreciseNumberValue<T>::operator new(std::size_t sz,	const std::size_t& strsz) {
	std::size_t objsz = sz - sizeof(PreciseNumberValue<T>::charbuff);
	std::size_t needsz = objsz + strsz+1;
	return Value::allocator->alloc(needsz);
}


template<typename T>
void PreciseNumberValue<T>::operator delete(void* ptr, const std::size_t& ) {
	Value::allocator->dealloc(ptr);
}

template class PreciseNumberValue<double>;
template class PreciseNumberValue<std::uintptr_t>;
template class PreciseNumberValue<std::intptr_t>;


Value ParserHelper::numberFromStringRaw(StrViewA str, bool force_double) {
	if (force_double) return PreciseNumberValue<double>::create(str);
	std::uintptr_t acclm;
	bool neg;
	std::uintptr_t pos = 0;
	if (str[0] == '-' || str[0] == '+') {
		neg = str[0] == '-';
		pos++;
	}
	while (pos < str.length) {
		char c = str[pos++];
		if  (!isdigit(c))  {
			return PreciseNumberValue<double>::create(str);
		}
		std::uintptr_t newval = acclm * 10 + (c - '0');
		if (newval < acclm) {
			return PreciseNumberValue<double>::create(str);
		}
		acclm = newval;
	}
	if (neg) {
		if (acclm > (acclm<<1))  {
			return PreciseNumberValue<double>::create(str);
		} else {
			return PreciseNumberValue<std::intptr_t>::create(-(std::intptr_t)acclm,str);
		}
	} else {
		return PreciseNumberValue<std::uintptr_t>::create(acclm,str);
	}
}




}
