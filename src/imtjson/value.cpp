#include <cstring>
#include "value.h"
#include "basicValues.h"
#include "arrayValue.h"
#include "objectValue.h"
#include "array.h"
#include "object.h"
#include "parser.h"
#include "serializer.h"
#include "binary.h"
#include "stringValue.h"
#include "fnv.h"
#include "binjson.tcc"
#include "path.h"
#include "operations.h"

namespace json {

	KeyStart key;


	const IValue *createByType(ValueType vtype) {
		switch (vtype) {
		case undefined: return AbstractValue::getUndefined();
		case null: return NullValue::getNull();
		case boolean: return BoolValue::getBool(false);
		case string: return AbstractStringValue::getEmptyString();
		case number: return AbstractNumberValue::getZero();
		case array: return AbstractArrayValue::getEmptyArray();
		case object: return AbstractObjectValue::getEmptyObject();
		default: throw std::runtime_error("json::createByType - Invalid type requested");
		}
	}

	Value::Value(ValueType vtype):v(createByType(vtype))
	{
	}

	Value::Value(bool value):v(BoolValue::getBool(value))
	{
	}

	Value::Value(const char * value):v(StringValue::create(value))
	{
	}

	Value::Value(const std::string & value):v(StringValue::create(value))
	{
	}

	Value::Value(const StringView<char>& value):v(StringValue::create(value))
	{
	}
	Value::Value(const StringView<Value>& value)
	{
		RefCntPtr<ArrayValue> res = ArrayValue::create(value.length);
		for (Value z: value) res->push_back(z.getHandle());
		v = PValue::staticCast(res);
	}

	String Value::toString() const
	{
		switch (type()) {
		case null:
		case boolean:
		case number: if (flags() & json::preciseNumber)
						return String(*this);
					else
						return stringify();

		case undefined: return String("<undefined>");
		case object: return stringify();
		case array: return stringify();
		case string:
			if (flags() & binaryString) {
				BinaryEncoding enc = Binary::getEncoding(v->unproxy());
				Value v = enc->encodeBinaryValue(getBinary(enc));
				return String(v);
			}
			else return String(*this);
		default: return String("<unknown>");
		}
	}

	Value Value::fromString(const StringView<char>& string)
	{
		std::size_t pos = 0;
		return parse([&pos,&string]() -> char {
			if (pos == string.length)
				return -1;
			else
				return string.data[pos++];
		});
	}

	Value Value::fromStream(std::istream & input)
	{
		return parse([&]() {
			return (char)input.get();
		});
	}

	Value Value::fromFile(FILE * f)
	{
		return parse([&] {
			return (char)fgetc(f);
		});
	}

	String Value::stringify() const
	{
		std::string buff;
		serialize([&](char c) {
			buff.push_back(c);
		});
		return String(buff);
	}

	String Value::stringify(UnicodeFormat format) const
	{
		std::string buff;
		serialize(format,[&](char c) {
			buff.push_back(c);
		});
		return String(buff);
	}

	void Value::toStream(std::ostream & output) const
	{
		serialize([&](char c) {
			output.put(c);
		});
	}

	void Value::toStream(UnicodeFormat format, std::ostream & output) const
	{
		serialize(format, [&](char c) {
			output.put(c);
		});
	}

	template<typename T>
	PValue allocUnsigned(T x) {
		if (x == (T)0) return AbstractNumberValue::getZero();
		if (sizeof(T) <= sizeof(uintptr_t)) return new UnsignedIntegerValue(uintptr_t(x));
		else if (sizeof(T) <= sizeof(ULongInt)) return new UnsignedLongValue(ULongInt(x));
		else return new NumberValue((double)x);
	}

	template<typename T>
	PValue allocSigned(T x) {
		if (x == (T)0) return AbstractNumberValue::getZero();
		if (sizeof(T) <= sizeof(intptr_t)) return new IntegerValue(intptr_t(x));
		else if (sizeof(T) <= sizeof(LongInt)) return new LongValue(LongInt(x));
		else return new NumberValue((double)x);
	}

	Value::Value(signed short x):v(allocSigned(x)) {
	}

	Value::Value(unsigned short x):v(allocUnsigned(x)) {
	}

	Value::Value(signed int x):v(allocSigned(x)) {
	}

	Value::Value(unsigned int x):v(allocUnsigned(x)) {
	}

	Value::Value(signed long x):v(allocSigned(x)) {
	}

	Value::Value(unsigned long x):v(allocUnsigned(x)) {
	}

	Value::Value(signed long long x):v(allocSigned(x)) {
	}

	Value::Value(unsigned long long x):v(allocUnsigned(x)) {
	}

	void Value::toFile(FILE * f) const
	{
		serialize([&](char c) {
			fputc(c, f);
		});
	}

	void Value::toFile(UnicodeFormat format, FILE * f) const
	{
		serialize(format, [&](char c) {
			fputc(c, f);
		});
	}

	class SubArray : public AbstractArrayValue {
	public:

		SubArray(PValue parent, std::size_t start, std::size_t len)
			:parent(parent), start(start), len(len) {

			const IValue *p = parent;
			if (typeid(*p) == typeid(SubArray)) {
				const SubArray *x = static_cast<const SubArray *>((const IValue *)parent);
				this->parent = x->parent;
				this->start = this->start + x->start;
			}
		}

		virtual std::size_t size() const override {
			return len;
		}
		virtual const IValue *itemAtIndex(std::size_t index) const override {
			return parent->itemAtIndex(start + index);
		}
		virtual bool enumItems(const IEnumFn &fn) const override {
			for (std::size_t x = 0; x < len; x++) {
				if (!fn(itemAtIndex(x))) return false;
			}
			return true;
		}

		virtual bool getBool() const override { return true; }
	protected:
		PValue parent;
		std::size_t start;
		std::size_t len;
	};

	
	Value::TwoValues Value::splitAt(int pos) const
	{
		if (type() == string) {
			String s( *this);
			return s.splitAt(pos);
		}
		else {
			std::size_t sz = size();
			if (pos > 0) {
				if ((unsigned)pos < sz) {
					return TwoValues(new SubArray(v, 0, pos), new SubArray(v, pos, sz - pos));
				}
				else {
					return TwoValues(*this, array);
				}
			}
			else if (pos < 0) {
				if ((int)sz + pos > 0) return splitAt((int)sz + pos);
				else return TwoValues(array, *this);
			}
			else {
				return TwoValues(array, *this);
			}
			return std::pair<Value, Value>();
		}
	}

	PValue prepareValues(const std::initializer_list<Value>& data) {
			return Value(array, data).getHandle();
	}

	Value::Value(const std::initializer_list<Value>& data)
		:v(data.size() == 0?
			PValue(AbstractArrayValue::getEmptyArray())
			:prepareValues(data))
	{

	}

/*	Value::Value(UInt value):v(new UnsignedIntegerValue(value))
	{
	}

	Value::Value(Int value):v(new IntegerValue(value))
	{
	}
*/
	Value::Value(double value):v(
			value == 0?AbstractNumberValue::getZero()
			:new NumberValue(value)
	)
	{
	}

	Value::Value(float value):v(
			value == 0?AbstractNumberValue::getZero()
			:new NumberValue(value)
	)
	{
	}

	Value::Value(std::nullptr_t):v(NullValue::getNull())
	{
	}

	Value::Value():v(AbstractValue::getUndefined())
	{
	}

	Value::Value(const IValue * v):v(v)
	{
	}

	Value::Value(const PValue & v):v(v)
	{
	}

	Value::Value(const Array & value):v(value.commit())
	{

	}

	Value::Value(const Object & value) : v(value.commit())
	{
	}

	uintptr_t maxPrecisionDigits = 9;
	UnicodeFormat defaultUnicodeFormat = emitEscaped;
	bool enableParsePreciseNumbers = false;

	///const double maxMantisaMult = pow(10.0, floor(log10(UInt(-1))));

	bool Value::operator ==(const Value& other) const {
		if (other.v == v) return true;
		else return other.v->equal(v);
	}

	bool Value::operator !=(const Value& other) const {
		if (other.v == v) return false;
		else return !other.v->equal(v);
	}


	ValueIterator Value::begin() const  {return ValueIterator(*this,0);}
	ValueIterator Value::end() const  {return ValueIterator(*this,size());}

	Value::Value(const String& value):v(value.getHandle()) {
	}

	Value Value::reverse() const {
		Array out;
		for (UInt i = size(); i > 0; i--) {
			out.add(this->operator [](i-1));
		}
		return out;
	}


	Binary Value::getBinary(BinaryEncoding be) const {
		return Binary::decodeBinaryValue(*this,be);
	}

	Value::Value(const BinaryView& binary, BinaryEncoding enc):v(StringValue::create(binary,enc)) {}

	Value::Value(const Binary& binary):v(binary.getHandle()) {}



	static Allocator defaultAllocator = {
		&::operator new,
		&::operator delete
	};


	const Allocator * Value::allocator = &defaultAllocator;

	void *AllocObject::operator new(std::size_t sz) {
		return Value::allocator->alloc(sz);
	}
	void AllocObject::operator delete(void *ptr, std::size_t) {
		return Value::allocator->dealloc(ptr);
	}

	const Allocator *Allocator::getInstance() {
		return Value::allocator;
	}

	Value::Value(ValueType type, const StringView<Value>& values, bool skipUndef) {
		std::vector<PValue> vp;
		switch (type) {
		case array: {
			RefCntPtr<ArrayValue> vp = ArrayValue::create(values.length);
			for (const Value &v: values) {
				if (!skipUndef || v.defined())
					vp->push_back(v.getHandle());
			}
			v = PValue::staticCast(vp);
			break;
		}
		case object: {
			RefCntPtr<ObjectValue> vp = ObjectValue::create(values.length);
			StrViewA lastKey;
			bool ordered = true;
			for (const Value &v: values) {
				if (!skipUndef || v.defined()) {
					StrViewA k = v.getKey();
					int c = lastKey.compare(k);
					if (c > 0) {
						ordered = false;
					}
					else if (c == 0) {
						if (!vp->empty()) vp->pop_back();
					}
					vp->push_back(v.getHandle());
					lastKey = k;
				}
			}
			if (!ordered) {
				vp->sort();
			}
			v = PValue::staticCast(vp);
			break;
		}
		case string: {
			std::vector<String> tmpStr;
			tmpStr.reserve(values.length);
			std::size_t needsz = 0;
			for (auto &&x : values) {
				String s = x.toString();
				tmpStr.push_back(s);
				needsz += s.length();
			}
			v = PValue(new (needsz) StringValue(nullptr, needsz, [&](char *c) {
				for (auto &&x : tmpStr) {
					std::memcpy(c, x.c_str(), x.length());
					c += x.length();
				}
				return needsz;
			}));
			break;
			

		}
		default:
			 throw std::runtime_error("json::Value(type,...) - Invalid type requested");

		}
	}

	static Value setToPathRev(const Path &p, const Value &oldVal, const  Value &newVal) {
		if (p.isRoot()) {
			if (newVal.flags() & json::valueDiff) {
				switch (newVal.type()) {
					case json::object:
						return Object::applyDiff(oldVal,newVal);
					case array:
						//currently unsupported
						return newVal;
					default:
						//cannot merge noncontainer
						return newVal;
				}
			} else {
				return newVal;
			}
		}
		if (p.isIndex()) {
			Array a;
			std::size_t i = p.getIndex();
			if (oldVal.type() == array) a.setBaseObject(oldVal);
			if (i >= a.size()) a.push_back(setToPathRev(p.getParent(),undefined,newVal));
			else a.set(i, setToPathRev(p.getParent(),a[i],newVal));
			return a;
		}
		if (p.isKey()) {
			Object o;
			if (oldVal.type() == object) o.setBaseObject(oldVal);
			StrViewA k = p.getKey();
			o.set(k,setToPathRev(p.getParent(),o[k], newVal));
			return o;
		}
		throw std::runtime_error("Unreachable section reached (setToPathRev)");
	}

	static Value reversePath(const Path &p, const Path &newPath, const Value &oldval, const Value &newval) {
		if (p.isRoot())
			return setToPathRev(newPath, oldval, newval);
		if (p.isIndex())
			return reversePath(p.getParent(),Path(newPath,p.getIndex()), oldval, newval);
		if (p.isKey())
			return reversePath(p.getParent(),Path(newPath,p.getKey()), oldval, newval);
		throw std::runtime_error("Unreachable section reached (reversePath)");
	}

	Value Value::replace(const Path& path, const Value& val) const {
		const Path &newpath = Path::root;
		return reversePath(path, newpath, *this, val);

	}

	Value Value::replace(const StrViewA &key, const Value& val) const {
		return replace(Path::root/key,val);

	}
	Value Value::replace(const uintptr_t index, const Value& val) const {
		return replace(Path::root/index,val);
	}


	int Value::compare(const Value &a, const Value &b) {
		return a.v->compare(b.v);
	}

	Range<ValueIterator> Value::range() const {
		return Range<ValueIterator>(begin(),end());
	}


	std::size_t Value::indexOf(const Value &v, std::size_t start) const {
		std::size_t len = size();
		for (std::size_t i = start; i < len; i++) {
			if ((*this)[i] == v) return i;
		}
		return npos;

	}
	std::size_t Value::lastIndexOf(const Value &v, std::size_t start) const {

		for (std::size_t i = std::min(start,size()); i > 0; ){
			--i;
			if ((*this)[i] == v) return i;
		}
		return npos;

	}

	static bool findInArray(const Array &a, const Value &v, std::size_t start, std::size_t &pos) {
		std::size_t i = start;
		while (i < a.size()) {
			if (a[i] == v) {
				pos = i;
				return true;
			}
			++i;
		}
		i = 0;
		while (i < start) {
			if (a[i] == v) {
				pos = i;
				return true;
			}
			++i;
		}
		pos = start;
		return false;
	}

	Value json::Value::merge(const Value& other) const {

		if (type() != other.type()) return undefined;
		switch (type()) {
		case undefined:
		case null: return other;
		case boolean: return getBool() != other.getBool();
		case number: return getNumber() + other.getNumber();
		case string: return String({String(*this), String(other)});
		case array: {
			Array p(*this);
			auto itr = other.begin();
			while (itr != other.end()) {
				Value v = *itr;
				if (v.defined()) {
					p.push_back(v);
					++itr;
				} else {
					++itr;
					std::size_t lastPos = 0;
					while (itr != other.end()) {
						Value find = *itr;
						++itr;
						if (itr != other.end()) {
							Value insert = *itr;
							++itr;
							std::size_t pos;
							if (findInArray(p, find, lastPos, pos)) {
								if (insert.defined()) {
									p.insert(pos, insert);
									lastPos = pos+1;
								}
								else {
									p.erase(pos);
								}
							} else {
								if (insert.defined()) p.push_back(insert);
							}
						}
					}
					break;
				}
			}
			return p;
		}
		case object: {
			auto i1 = begin(), e1 = end();
			auto i2 = other.begin(), e2 = other.end();
			Object res;
			while (i1 != e1 && i2 != e2) {
				Value v1 = *i1;
				Value v2 = *i2;
				if (v1.getKey() < v2.getKey()) {
					res.set(v1); ++i1;
				} else if (v1.getKey() > v2.getKey()) {
					res.set(v2); ++i2;
				} else {
					if (v1.type() == json::object && v2.type() == json::object) {
						res.set(v1.getKey(),v1.merge(v2));
					} else {
						res.set(v2);
					}
					++i1; ++i2;
				}
			}
			while (i1 != e1) {
				res.set(*i1); ++i1;
			}
			while (i2 != e2) {
				res.set(*i2); ++i2;
			}
			return res;
		}

		}

		return undefined;
	}

	Value json::Value::diff(const Value& other) const {
		if (type() != other.type()) return undefined;
		switch (type()) {
		case undefined:
		case null: return other;
		case boolean: return getBool() != other.getBool();
		case number: return getNumber() - other.getNumber();
		case string: {
			auto str1 = getString();
			auto str2 = other.getString();
			if (str1.length < str2.length) return *this;
			if (str1.substr(str1.length - str2.length) == str2) {
				return str1.substr(0,str1.length - str2.length);
			}
			else {
				return *this;
			}
		}
		case array: {
			std::size_t tsz = size();
			std::size_t osz = other.size();
			std::vector<Value> diff;
			std::vector<Value> append;
			std::size_t t = 0, o = 0;
			while (t < tsz && o < osz) {
				if ((*this)[t] != other[o]) {
					if (t + 1 < tsz && (*this)[t+1] == other[o]) {
						diff.push_back((*this)[t]);
						diff.push_back(json::undefined);
						t++;
					} else if (o < osz && (*this)[t] == other[o+1]) {
						diff.push_back((*this)[t]);
						diff.push_back(other[o]);
						o++;
					} else {
						diff.push_back((*this)[t]);
						diff.push_back(other[o]);
						diff.push_back((*this)[t]);
						diff.push_back(json::undefined);
						o++;
						t++;
					}
				} else{
					o++;
					t++;
				}
			}
			while (o < osz) {
				append.push_back(other[o]);
				t++;
			}
			while (t < tsz) {
				diff.push_back((*this)[t]);
				diff.push_back(json::undefined);
				o++;
			}
			if (!diff.empty()) {
				append.push_back(json::undefined);
				for (auto &&v : diff) {
					append.push_back(v);
				}
			}
			return Value(StringView<Value>(append));
		}
		case object: {
			auto i1 = begin(), e1 = end();
			auto i2 = other.begin(), e2 = other.end();
			Object res;
			while (i1 != e1 && i2 != e2) {
				const Value &v1 = *i1;
				const Value &v2 = *i2;
				if (v1.getKey() < v2.getKey()) {
					res.unset(v1.getKey()); ++i1;
				} else if (v1.getKey() > v2.getKey()) {
					res.set(v2); ++i2;
				} else {
					if (v1.type() == json::object && v2.type() == json::object) {
						Value x = v1.diff(v2);
						if (!x.empty()) res.set(v1.getKey(),x);
					} else if (v1 != v2) {
						res.set(v2);
					}
					++i1; ++i2;
				}
			}
			while (i1 != e1) {
				res.unset((*i1).getKey()); ++i1;
			}
			while (i2 != e2) {
				res.set(*i2); ++i2;
			}
			return res.commitAsDiff();
		}

		}

		return undefined;
	}


	Value Value::preciseNumber(const StrViewA number) {
		UInt pos = 0;
		auto reader = [&]() -> int {
			if (pos > number.length) {
				pos++;
				return -1;
			}
			else {
				return number[pos++];
			}
		};
		typedef Parser<decltype(reader)> P;
		P parser(std::move(reader), P::allowPreciseNumbers);
		try {
			Value v = parser.parseNumber();
			if (pos <= number.length) throw InvalidNumericFormat(number);
			return v;
		} catch (const ParseError &) {
			throw InvalidNumericFormat(number);
		}
	}

	InvalidNumericFormat::InvalidNumericFormat(std::string &&val):val(std::move(val)) {}
	const std::string &InvalidNumericFormat::getValue() const {return val;}
	const char *InvalidNumericFormat::what() const noexcept  {
		msg = "Invalid numeric format (precise number): " + val;
		return msg.c_str();
	}


	Value Value::shift() {
		Value ret;
		switch (type()) {
		case string: ret = String(*this).substr(0,1);
					 (*this) = String(*this).substr(1);
					 break;
		case object:
		case array: ret = (*this)[0];
					(*this) = slice(1);
					break;
		default:	ret = *this;
					*this = undefined;
					break;
		}
		return ret;
	}

	UInt Value::unshift(const Value &item) {
		Array newval;
		newval.reserve(1+item.size());
		newval.push_back(item);
		for (Value x: *this) {
			newval.push_back(x);
		}
		*this = newval;
		return size();
	}


	UInt Value::push(const Value &item) {
		Array nw(*this);
		nw.push_back(item);
		*this = nw;
		return size();
	}

	Value Value::pop() {
		auto sz = size();
		if (!sz) return undefined;
		Array nw(*this);

		Value ret = nw[sz-1];
		nw.erase(sz-1);
		*this = nw;
		return ret;
	}

	String Value::join(StrViewA separator) const {
		Array tmp;
		tmp.reserve(size());
		std::size_t needsz = 0;
		for(Value x: *this) {
			String s = x.toString();
			tmp.push_back(s);
			needsz+=s.length();
		}
		needsz+=(tmp.size()-1)*separator.length;
		return String(needsz, [&](char *c){
			char *anchor = c;
			for (std::size_t i =0, cnt = tmp.size(); i < cnt; i++) {
				if (i) {
					std::copy(separator.begin(), separator.end(), c);
					c+=separator.length;
				}
				String v(tmp[i]);
				std::copy(v.begin(), v.end(), c);
				c+=v.length();
			}
			return c - anchor;
		});
	}

	Value Value::slice(Int start) const {
		return slice(start, size());
	}
	Value Value::slice(Int start, Int end) const {
		Int sz = size();
		if (empty()) return json::array;
		if (start < 0) {
			if (-start >= sz) return *this;
			return slice(sz+start);
		}
		if (end < 0) {
			if (-end >= sz) return json::array;
			return slice(start,sz+end);
		}
		if (start >= end || start >= sz) return json::array;
		if (end >= sz) end = sz;
		UInt cnt = end - start;
		std::vector<Value> out;
		out.reserve(cnt);
		for (Int x = start; x < end; x++)
			out.push_back((*this)[x]);

		if (type() == object) {
			return Value(json::object, StringView<Value>(out.data(), out.size()));
		} else {
			return Value(json::array, StringView<Value>(out.data(), out.size()));
		}

	}


	UInt Value::unshift(const std::initializer_list<Value> &items) {
		return unshift(items.begin(), items.end());
	}

	template Value Value::find(std::function<bool(Value)> &&) const;
	template Value Value::rfind(std::function<bool(Value)> &&) const;
	template Value Value::filter(std::function<bool(Value)> &&) const;
	template Value Value::find(std::function<bool(Value, UInt)> &&) const;
	template Value Value::rfind(std::function<bool(Value, UInt)> &&) const;
	template Value Value::filter(std::function<bool(Value, UInt)> &&) const;
	template Value Value::find(std::function<bool(Value, UInt, Value)> &&) const;
	template Value Value::rfind(std::function<bool(Value, UInt, Value)> &&) const;
	template Value Value::filter(std::function<bool(Value, UInt, Value)> &&) const;

	String Value::getValueOrDefault(String defval) const {
		return type() == json::string?toString():defval;
	}
	const char *Value::getValueOrDefault(const char *defval) const {
		return type() == json::string?toString().c_str():defval;
	}


}

namespace std {
size_t hash<::json::Value>::operator()(const ::json::Value &v)const {
	size_t ret;
	FNV1a<sizeof(ret)> fnvcalc(ret);
	v.stripKey().serializeBinary(fnvcalc,0);
	return ret;
}




}
