#include "value.h"
#include "basicValues.h"
#include "arrayValue.h"
#include "objectValue.h"
#include "array.h"
#include "object.h"
#include "parser.h"
#include "serializer.h"

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

	Value::Value(const char * value):v(new StringValue(value))
	{
	}

	Value::Value(const std::string & value):v(new StringValue(value))
	{
	}

	Value::Value(const StringRef<char>& value):v(new StringValue(value))
	{
	}

	Value Value::fromString(const StringRef<char>& string)
	{
		std::size_t pos = 0;
		return parse([&pos,string]()  {
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
		return Value();
	}

	std::string Value::toString() const
	{
		std::string buff;
		serialize([&](char c) {
			buff.push_back(c);
		});
		return buff;
	}

	void Value::toStream(std::ostream & output) const
	{
	}

	void Value::toFile(FILE * f) const
	{
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

	Value::Value(std::uintptr_t value):v(new UnsignedIntegerValue(value))
	{
	}

	Value::Value(std::intptr_t value):v(new IntegerValue(value))
	{
	}

	Value::Value(double value):v(new NumberValue(value))
	{
	}

	Value::Value(float value):v(new NumberValue(value))
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

	const double maxMantisaMult = pow(10.0, floor(log10(std::uintptr_t(-1))));
}