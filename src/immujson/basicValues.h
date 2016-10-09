#pragma once

#include "abstractValue.h"


namespace json {


	class NullValue : public AbstractValue {
	public:
		virtual ValueType type() const override { return null; }

		static const IValue *getNull();
	};

	class BoolValue : public AbstractValue {
	public:
		virtual ValueType type() const override { return boolean; }
		virtual bool getBool() const override  = 0;

		static const IValue *getBool(bool v);

	};

	class AbstractNumberValue : public AbstractValue {
	public:
		virtual ValueType type() const override { return number; }
		virtual double getNumber() const override = 0;
		virtual std::intptr_t getInt() const override = 0;
		virtual std::uintptr_t getUInt() const override = 0;

		static const IValue *getZero();
	};

	class AbstractStringValue : public AbstractValue {
	public:
		virtual ValueType type() const override { return string; }
		virtual StringRef<char> getString() const override = 0;

		static const IValue *getEmptyString();
	};

	class AbstractArrayValue : public AbstractValue {
	public:
		virtual ValueType type() const override { return array; }
		virtual std::size_t size() const override = 0;
		virtual const IValue *itemAtIndex(std::size_t index) const override = 0;
		virtual bool enumItems(const IEnumFn &) const override = 0;

		static const IValue *getEmptyArray();
	};

	class AbstractObjectValue : public AbstractValue {
	public:
		virtual ValueType type() const override { return object; }
		virtual std::size_t size() const override = 0;
		virtual const IValue *itemAtIndex(std::size_t index) const override = 0;
		virtual const IValue *member(const StringRef<char> &name) const override = 0;
		virtual bool enumItems(const IEnumFn &) const override = 0;

		static const IValue *getEmptyObject();
	};
	
	template<typename T, ValueTypeFlags f >
	class NumberValueT : public AbstractNumberValue {
	public:
		NumberValueT(const T &v) :v(v) {}

		virtual double getNumber() const override { return double(v); }
		virtual std::intptr_t getInt() const override { return std::intptr_t(v); }
		virtual std::uintptr_t getUInt() const override { return std::uintptr_t(v); }
		virtual ValueTypeFlags flags() const override { return f; }


	protected:
		T v;
	};

	using UnsignedIntegerValue = NumberValueT<std::uintptr_t, numberUnsignedInteger>;
	using IntegerValue = NumberValueT<std::intptr_t, numberInteger>;
	using NumberValue = NumberValueT<double,0>;

	class StringValue : public AbstractStringValue {
	public:
		StringValue(const StringRef<char> &str) :v(str) {}

		virtual StringRef<char> getString() const override {
			return v;
		}
	protected:
		std::string v;

	};
}
