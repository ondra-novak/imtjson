#include "basicValues.h"

#include "value.h"
namespace json {

	class StaticNullValue: public NullValue {
	public:
		StaticNullValue() {
			addRef();
		}
	};

	template<bool v>
	class StaticBool : public BoolValue {
	public:
		virtual bool getBool() const override { return v; }
		StaticBool() {
			addRef();
		}

	};

	class StaticZeroNumber : public AbstractNumberValue {
	public:
		virtual double getNumber() const override { return 0.0; }
		virtual std::intptr_t getInt() const override { return 0; }
		virtual std::uintptr_t getUInt() const override {return 0;}
		virtual ValueTypeFlags flags() const override { return numberUnsignedInteger; }

		StaticZeroNumber() {
			addRef();
		}

	};

	class StaticEmptyStringValue : public AbstractStringValue {
	public:
		virtual StringView<char> getString() const override { return StringView<char>(); }

		StaticEmptyStringValue() {
			addRef();
		}
	};

	class StaticEmptyArrayValue : public AbstractArrayValue {
	public:
		virtual std::size_t size() const override { return 0; }
		virtual const IValue *itemAtIndex(std::size_t index) const override { return AbstractValue::getUndefined(); }
		virtual bool enumItems(const IEnumFn &) const override {
			return true;
		}
		StaticEmptyArrayValue() {
			addRef();
		}
	};

	class StaticEmptyObjectValue : public AbstractObjectValue {
	public:
		virtual std::size_t size() const override { return 0; }
		virtual const IValue *itemAtIndex(std::size_t index) const override { return getUndefined(); }
		virtual const IValue *member(const StringView<char> &name) const override { return getUndefined(); }
		virtual bool enumItems(const IEnumFn &) const override { return true; }
		StaticEmptyObjectValue() {
			addRef();
		}
	};


	static StaticNullValue staticNull;


	const IValue *NullValue::getNull() {
		return &staticNull;

	}



	static StaticBool<false> boolFalse;
	static StaticBool<true> boolTrue;


	const IValue * BoolValue::getBool(bool v)
	{
		if (v) return &boolTrue;
		else return &boolFalse;
	}

	static StaticZeroNumber zero;

	const IValue * AbstractNumberValue::getZero()
	{
		return &zero;
	}

	static StaticEmptyStringValue emptyStr;

	const IValue * AbstractStringValue::getEmptyString()
	{
		return &emptyStr;
	}

	static StaticEmptyArrayValue emptyArray;

	const IValue * AbstractArrayValue::getEmptyArray()
	{
		return &emptyArray;
	}

	static StaticEmptyObjectValue emptyObject;


	const IValue * AbstractObjectValue::getEmptyObject()
	{
		return &emptyObject;
	}

	template class NumberValueT<std::uintptr_t, numberUnsignedInteger>;
	template class NumberValueT<std::intptr_t, numberInteger>;
	template class NumberValueT<double,0>;

	bool NullValue::equal(const IValue* other) const {
		return other->type() == null;
	}


	bool BoolValue::equal(const IValue* other) const {
		return other->type() == boolean && getBool() == other->getBool();
	}

	bool AbstractNumberValue::equal(const IValue* other) const {
		return other->type() == number && getNumber() == other->getNumber();
	}

	bool AbstractStringValue::equal(const IValue* other) const {
		return other->type() == string && getString() == other->getString();
	}

	bool AbstractArrayValue::equal(const IValue* other) const {
		if (other->type() == array && other->size() == size()) {
			std::size_t cnt = size();
			for (std::size_t i = 0; i < cnt; i++) {
				const IValue *a = itemAtIndex(i);
				const IValue *b = other->itemAtIndex(i);
				if (a != b && !a->equal(b)) return false;
			}
			return true;
		}
		return false;
	}
	bool AbstractObjectValue::equal(const IValue *other) const {
		if (other->type() == object && other->size() == size()) {
			std::size_t cnt = size();
			for (std::size_t i = 0; i < cnt; i++) {
				const IValue *a = itemAtIndex(i);
				const IValue *b = other->itemAtIndex(i);
				if (a != b && (a->getMemberName() != b->getMemberName() || !a->equal(b))) return false;
			}
			return true;
		}
		return false;
	}


	std::intptr_t StringValue::getInt() const {
		return Value::fromString(v).getInt();
	}

	std::uintptr_t StringValue::getUInt() const {
		return Value::fromString(v).getUInt();
	}


	double StringValue::getNumber() const {
		return Value::fromString(v).getNumber();
	}



}

