#include "value.h"
#include "basicValues.h"
#include "arrayValue.h"
#include "objectValue.h"
#include "array.h"
#include "object.h"
#include "parser.h"
#include "serializer.h"
#include "string.h"
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

	static const IValue *allocString(const StringView<char> &str) {
		if (str.empty()) return AbstractStringValue::getEmptyString();
		else {
			return new(str) StringValue(str);
		}
	}

	Value::Value(const char * value):v(allocString(value))
	{
	}

	Value::Value(const std::string & value):v(allocString(value))
	{
	}

	Value::Value(const StringView<char>& value):v(allocString(value))
	{
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

	void Value::toStream(std::ostream & output) const
	{
		serialize([&](char c) {
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
			String s = *this;
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
				return TwoValues(*this, array);
			}
			return std::pair<Value, Value>();
		}
	}

	std::vector<PValue> Value::prepareValues(const std::initializer_list<Value>& data) {
		std::vector<PValue> out;
		out.reserve(data.size());
		for (auto &&x : data) out.push_back(x.v);
		return std::vector<PValue>(std::move(out));
	}

	Value::Value(const std::initializer_list<Value>& data)
		:v(data.size() == 0?
			AbstractArrayValue::getEmptyArray()
			:new ArrayValue(prepareValues(data)))
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




}

