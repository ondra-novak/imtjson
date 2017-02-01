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

namespace json {


	const IValue *createByType(ValueType vtype) {
		switch (vtype) {
		case undefined: return AbstractValue::getUndefined();
		case null: return NullValue::getNull();
		case boolean: return BoolValue::getBool(false);
		case string: return AbstractStringValue::getEmptyString();
		case number: return AbstractNumberValue::getZero();
		case array: return AbstractArrayValue::getEmptyArray();
		case object: return AbstractObjectValue::getEmptyObject();
		default: throw std::runtime_error("Invalid argument");
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
		case number: return stringify();
		case undefined: return String("<undefined>");
		case object: return stringify();
		case array: return stringify();
		case string: return String(*this);
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
		else return new NumberValue((double)x);
	}

	template<typename T>
	PValue allocSigned(T x) {
		if (x == (T)0) return AbstractNumberValue::getZero();
		if (sizeof(T) <= sizeof(intptr_t)) return new IntegerValue(intptr_t(x));
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

			if (typeid(*parent) == typeid(SubArray)) {
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

	RefCntPtr<ArrayValue> prepareValues(const std::initializer_list<Value>& data) {
		RefCntPtr<ArrayValue> out = ArrayValue::create(data.size());
		for (auto &&x : data) out->push_back(x.getHandle());
		return out;
	}

	Value::Value(const std::initializer_list<Value>& data)
		:v(data.size() == 0?
			PValue(AbstractArrayValue::getEmptyArray())
			:PValue::staticCast(prepareValues(data)))
	{

	}

/*	Value::Value(std::uintptr_t value):v(new UnsignedIntegerValue(value))
	{
	}

	Value::Value(std::intptr_t value):v(new IntegerValue(value))
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

	uintptr_t maxPrecisionDigits = sizeof(uintptr_t) < 4 ? 4 : (sizeof(uintptr_t) < 8 ? 9 : 12);
	UnicodeFormat defaultUnicodeFormat = emitEscaped;

	///const double maxMantisaMult = pow(10.0, floor(log10(std::uintptr_t(-1))));

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
		for (std::uintptr_t i = size(); i > 0; i++) {
			out.add(this->operator [](i-1));
		}
		return out;
	}



	class Base64Table {
	public:
		unsigned char table[256];
		Base64Table() {
			for (std::size_t i = 0; i <256; i++) table[i] = 0xFF;
			for (std::size_t i = 'A'; i <='Z'; i++) table[i] = (unsigned char)(i - 'A');
			for (std::size_t i = 'a'; i <= 'z'; i++) table[i] = (unsigned char)(i - 'a' + 26);
			for (std::size_t i = '0'; i <='9'; i++) table[i] = (unsigned char)(i - '0' + 52);
			table['+'] = 63;
			table['/'] = 64;
		}
	};

	String decodeBase64(const StrViewA &str) {
		static Base64Table t;
		std::size_t len = str.length;
		if ((len & 0x3) || len == 0) return String();
		std::size_t finlen = (len / 4) * 3;
		if (str[len-1] == '=') {
			--finlen;
			--len;
		}
		if (str[len-1] == '=') {
			--finlen;
			--len;
		}
		return String(finlen, [&] (char *buff) {

			unsigned char *bbuf = reinterpret_cast<unsigned char *>(buff);

			std::size_t rdpos = 0;
			std::size_t wrpos = 0;
			while (rdpos < len) {
				char z = str[rdpos++];
				unsigned char b = t.table[(unsigned char)z];
				switch (rdpos & 0x3) {
				case 0: bbuf[wrpos] = b << 2;break;
				case 1: bbuf[wrpos]  |= b >> 4;
						bbuf[++wrpos] = b << 4;
						break;
				case 2: bbuf[wrpos] |= b >> 2;
						bbuf[++wrpos] = b << 6;
						break;
				case 3: bbuf[++wrpos] |= b;
						break;
				}
			}
			return finlen;
		});
	}

	String encodeBase64(const BinaryView &data) {
		std::size_t len = data.length;
		if (len == 0) return String();
		std::size_t finlen = (len+2)/3 * 4;
		return String(finlen, [&](char *buff){

			static char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
								  "abcdefghijklmnopqrstuvwxyz"
								  "0123456789"
					              "+/";

			std::size_t rdpos = 0;
			std::size_t wrpos = 0;
			unsigned char nx = 0;
			while (rdpos < len) {
				unsigned char c = data[rdpos++];
				switch (rdpos % 3) {
				case 0: buff[wrpos++] = chars[c >> 2];
						nx = (c << 4) & 0x3F;
						break;
				case 1: buff[wrpos++] = chars[nx | (c>>4)];
						nx = (c << 2) & 0x3F;
						break;
				case 2: buff[wrpos++] = chars[nx | (c>> 6)];
						buff[wrpos++] = chars[c & 0x3F];
						break;
				}
			}


			while (wrpos < finlen)
				buff[wrpos++] = '=';
			return finlen;
		});
	}

	String decodeQuotedPrintable(const StrViewA &str) {
		std::size_t reqsize = 0;
		for (std::size_t i = 0; i < str.length; i++) {
			reqsize++;
			if (str[i] == '=') i+=2;
		}
		return String(reqsize, [&](char *buff){
			for (std::size_t i = 0; i < str.length; i++) {
				if (str[i] == '=') {
					char sbuff[3];
					sbuff[0] = str[i+1];
					sbuff[1] = str[i+2];
					sbuff[0] = 0;
					long l = strtol(sbuff,0,16);
					*buff++ = (char)l;
					i+=2;
				} else {
					*buff++ = str[i];
				}
			}

			return reqsize;
		});
	}

	String encodeQuotedPrintable(const BinaryView &data) {
		std::size_t reqsize = 0;
		for (std::size_t i = 0; i < data.length; i++) {
			reqsize++;
			if (data[i] < 32 || data[i] >127 ) reqsize+=2;
		}
		return String(reqsize, [&](char *buff){
			static char hexs[] = "0123456789ABCDEF";
			for (std::size_t i = 0; i < data.length; i++) {
				if (data[i] < 32 || data[i] >127 ) {
					*buff++='=';
					*buff++=hexs[data[i] >> 4];
					*buff++=hexs[data[i] & 0xF];
				} else {
					*buff++ = data[i];
				}
			}

			return reqsize;
		});
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

	void *IValue::operator new(std::size_t sz) {
		return Value::allocator->alloc(sz);
	}
	void IValue::operator delete(void *ptr, std::size_t) {
		return Value::allocator->dealloc(ptr);
	}

	const Allocator *Allocator::getInstance() {
		return Value::allocator;
	}

	Value::Value(ValueType type, const StringView<Value>& values) {
		std::vector<PValue> vp;
		switch (type) {
		case array: {
			RefCntPtr<ArrayValue> vp = ArrayValue::create(values.length);
			for (const Value &v: values) {
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
			if (!ordered) {
				vp->sort();
			}
			v = PValue::staticCast(vp);
			break;
		}
		default:
			 throw std::runtime_error("Invalid argument");

		}
	}

}

