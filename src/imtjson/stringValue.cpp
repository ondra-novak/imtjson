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

static const std::string_view magic("imtjson");

void *StringValue::putMagic(void *obj) {
	StringValue *s = reinterpret_cast<StringValue *>(obj);
	std::memcpy(s->charbuff, magic.data(), magic.size());
	return obj;
}

StringValue::StringValue(BinaryEncoding enc, const std::string_view& str):sz(str.size()),encoding(enc) {
	char *trg = charbuff;
	if (std::string_view(trg,magic.size()) != magic) throw std::runtime_error("StringView must be allocated by special new operator");
	std::memcpy(trg, str.data(), str.size());
	trg[str.size()] = 0;
}

StringView StringValue::getString() const {
	return StringView(charbuff, sz);
}

Int AbstractStringValue::getInt() const {

	try {
		return Value::fromString(getString()).getInt();
	} catch (const ParseError &) {
		return 0;
	}
}

UInt AbstractStringValue::getUInt() const {

	try {
		return Value::fromString(getString()).getUInt();
	} catch (const ParseError &) {
		return 0;
	}
}

LongInt AbstractStringValue::getIntLong() const {

	try {
		return Value::fromString(getString()).getIntLong();
	} catch (const ParseError &) {
		return 0;
	}
}

ULongInt AbstractStringValue::getUIntLong() const {

	try {
		return Value::fromString(getString()).getUIntLong();
	} catch (const ParseError &) {
		return 0;
	}
}


void* StringValue::operator new(std::size_t sz, const std::size_t &strsz) {
	std::size_t objsz = sz - sizeof(StringValue::charbuff);
	std::size_t needsz = objsz + std::max(strsz+1, magic.size());
	return putMagic(Value::allocator->alloc(needsz));
}

void StringValue::operator delete(void* ptr, const std::size_t &) {
	Value::allocator->dealloc(ptr);
}

void StringValue::operator delete(void* ptr, std::size_t ) {
	Value::allocator->dealloc(ptr);
}

double AbstractStringValue::getNumber() const {
	try {
		return Value::fromString(getString()).getNumber();
	} catch (const ParseError &) {
		return 0;
	}
}

PValue StringValue::create(const std::string_view& str) {
	if (str.empty()) return AbstractStringValue::getEmptyString();
	else return new(str.size()) StringValue(nullptr,str);
}

void StringValue::stringOverflow() {
	std::cerr << "String buffer overflow, aborting" << std::endl << std::flush;
	abort();
}


PValue StringValue::create(const BinaryView& str, BinaryEncoding enc) {
	StringValue *v = new(str.size()) StringValue(enc,map_bin2str(str));
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

template<typename T>
T parseUnsigned(const char *v) {
	T x = 0;
	while (isdigit(*v)) {
		x = x * 10 +(*v - '0');++v;
	}
	return x;

}

template<typename T>
T parseSigned(const char *v) {
	T x = 0;
	bool neg = false;
	switch (*v) {
	case '+': ++v;break;
	case '-': ++v;neg = true;
	default:break;
	};
	while (isdigit(*v)) {
		x = x * 10 +(*v - '0');++v;
	}
	return neg?-x:x;
}




template<> struct NumbParser<UInt> {
	static UInt parse(const char *v) {return parseUnsigned<UInt>(v);}
	static const ValueTypeFlags flags = preciseNumber|numberUnsignedInteger;
};

template<> struct NumbParser<Int> {
	static Int parse(const char *v) {return parseSigned<Int>(v);}
	static const ValueTypeFlags flags = preciseNumber|numberInteger;
};
template<> struct NumbParser<ULongInt> {
	static ULongInt parse(const char *v) {return parseUnsigned<ULongInt>(v);}
	static const ValueTypeFlags flags = preciseNumber|numberUnsignedInteger|longInt;
};

template<> struct NumbParser<LongInt> {
	static LongInt parse(const char *v) {return parseSigned<LongInt>(v);}
	static const ValueTypeFlags flags = preciseNumber|numberInteger|longInt;
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
Int PreciseNumberValue<T>::getInt() const {
	if (!cached) cacheValue();
	return (Int)n;
}

template<typename T>
UInt PreciseNumberValue<T>::getUInt() const {
	if (!cached) cacheValue();
	return (UInt)n;
}

template<typename T>
LongInt PreciseNumberValue<T>::getIntLong() const {
	if (!cached) cacheValue();
	return (LongInt)n;
}

template<typename T>
ULongInt PreciseNumberValue<T>::getUIntLong() const {
	if (!cached) cacheValue();
	return (ULongInt)n;
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
StringView PreciseNumberValue<T>::getString() const {
	return StringView(charbuff,sizeof(charbuff));
}

template<typename T>
PValue PreciseNumberValue<T>::create(const std::string_view& str) {
	return new(str.size()) PreciseNumberValue<T>(str);

}

template<typename T>
PValue PreciseNumberValue<T>::create(const T& value, const std::string_view& str) {
	return new(str.size()) PreciseNumberValue<T>(value,str);
}

template<typename T>
PreciseNumberValue<T>::~PreciseNumberValue() {
}

template<typename T>
void PreciseNumberValue<T>::operator delete(void* ptr,	std::size_t ) {
	Value::allocator->dealloc(ptr);
}

template<typename T>
PreciseNumberValue<T>::PreciseNumberValue(const std::string_view& strNum):cached(false) {
	char *c = charbuff;
	for (char a: strNum) *c++ = a;
	*c = 0;
}

template<typename T>
PreciseNumberValue<T>::PreciseNumberValue(const T &v, const std::string_view &strNum):PreciseNumberValue(strNum) {
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
template class PreciseNumberValue<UInt>;
template class PreciseNumberValue<Int>;
template class PreciseNumberValue<ULongInt>;
template class PreciseNumberValue<LongInt>;


Value ParserHelper::numberFromStringRaw(std::string_view str, bool force_double) {
	if (force_double) return PreciseNumberValue<double>::create(str);
	ULongInt acclm = 0;
	bool neg = false;
	UInt pos = 0;
	if (str[0] == '-' || str[0] == '+') {
		neg = str[0] == '-';
		pos++;
	}
	while (pos < str.size()) {
		char c = str[pos++];
		if  (!isdigit(c))  {
			return PreciseNumberValue<double>::create(str);
		}
		ULongInt newval = acclm * 10 + (c - '0');
		if (newval < acclm) {
			return PreciseNumberValue<double>::create(str);
		}
		acclm = newval;
	}
	if (neg) {
		if (acclm > (acclm<<1))  {
			return PreciseNumberValue<double>::create(str);
		} else {
			return PreciseNumberValue<LongInt>::create(-(LongInt)acclm,str);
		}
	} else {
		return PreciseNumberValue<ULongInt>::create(acclm,str);
	}
}




}
