#pragma once

#include "abstractValue.h"


namespace json {


	class NullValue : public AbstractValue {
	public:
		virtual ValueType type() const override { return null; }

		virtual bool equal(const IValue *other) const override;


		static const IValue *getNull();
	};

	class BoolValue : public AbstractValue {
	public:
		virtual ValueType type() const override { return boolean; }
		virtual bool getBool() const override  = 0;		
		virtual double getNumber() const override {return getBool()?1.0:0;}
		virtual Int getInt() const override {return getBool()?1:0;}
		virtual UInt getUInt() const override {return getBool()?1:0;}
		virtual LongInt getIntLong() const override {return getBool()?1:0;}
		virtual ULongInt getUIntLong() const override {return getBool()?1:0;}

		static const IValue *getBool(bool v);


		virtual bool equal(const IValue *other) const override;


	};

	class AbstractNumberValue : public AbstractValue {
	public:
		virtual ValueType type() const override { return number; }
		virtual double getNumber() const override = 0;
		virtual Int getInt() const override = 0;
		virtual UInt getUInt() const override = 0;
		virtual LongInt getIntLong() const override = 0;
		virtual ULongInt getUIntLong() const override  = 0;

		static const IValue *getZero();


		virtual bool equal(const IValue *other) const override;

	};

	class AbstractStringValue : public AbstractValue {
	public:
		virtual ValueType type() const override { return string; }
		virtual StringView getString() const override = 0;

		virtual Int getInt() const override;
		virtual UInt getUInt() const override;
		virtual LongInt getIntLong() const override;
		virtual ULongInt getUIntLong() const override;
		virtual double getNumber() const override;


		static const IValue *getEmptyString();
		virtual bool equal(const IValue *other) const override;

	};

	class AbstractArrayValue : public AbstractValue {
	public:
		virtual ValueType type() const override { return array; }
		virtual std::size_t size() const override = 0;
		virtual const IValue *itemAtIndex(std::size_t index) const override = 0;
		virtual bool enumItems(const IEnumFn &) const override = 0;

		static const IValue *getEmptyArray();
		virtual bool equal(const IValue *other) const override;
	};

	class AbstractObjectValue : public AbstractValue {
	public:
		virtual ValueType type() const override { return object; }
		virtual std::size_t size() const override = 0;
		virtual const IValue *itemAtIndex(std::size_t index) const override = 0;
		virtual const IValue *member(const std::string_view &name) const override = 0;
		virtual bool enumItems(const IEnumFn &) const override = 0;
		virtual bool equal(const IValue *other) const override;

		static const IValue *getEmptyObject();
	};
	
	template<typename T, ValueTypeFlags f >
	class NumberValueT : public AbstractNumberValue {
	public:
		NumberValueT(const T &v) :v(v) {}

		virtual double getNumber() const override { return double(v); }
		virtual Int getInt() const override { return Int(v); }
		virtual UInt getUInt() const override { return UInt(v); }
		virtual LongInt getIntLong() const override {return LongInt(v);}
		virtual ULongInt getUIntLong() const override {return ULongInt(v);}
		virtual ValueTypeFlags flags() const override { return f; }
		virtual bool equal(const IValue *other) const  override {
			if (other->type() == number) {
				const NumberValueT *k = dynamic_cast<const NumberValueT *>(other->unproxy());
				if (k) return k->v == v;
				else return  AbstractNumberValue::equal(other);
			} else {
				return false;
			}
		}
		virtual bool getBool() const override {return true;}


	protected:
		T v;
	};

	using UnsignedIntegerValue = NumberValueT<UInt, numberUnsignedInteger>;
	using IntegerValue = NumberValueT<Int, numberInteger>;
	using UnsignedLongValue = NumberValueT<ULongInt, numberUnsignedInteger|longInt>;
	using LongValue = NumberValueT<LongInt, numberInteger|longInt>;
	using NumberValue = NumberValueT<double,0>;

}


