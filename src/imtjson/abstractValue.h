#pragma once

#include "ivalue.h"

namespace json {



	class AbstractValue : public IValue {
	public:
		virtual ValueType type() const override { return undefined; }
		virtual ValueTypeFlags flags() const override { return 0; }

		virtual UInt getUInt() const override { return 0; }
		virtual Int getInt() const override { return 0; }
		virtual ULongInt getUIntLong() const override { return 0; }
		virtual LongInt getIntLong() const override { return 0; }
		virtual double getNumber() const override { return 0.0; }
		virtual bool getBool() const override { return false; }
		virtual StringView getString() const override { return StringView(); }
		virtual std::size_t size() const override { return 0; }
		virtual const IValue *itemAtIndex(std::size_t ) const override { return getUndefined(); }
		virtual const IValue *member(const std::string_view &) const override { return getUndefined(); }
		virtual bool enumItems(const IEnumFn &) const override { return true; }

		///some values are proxies with member name - this retrieves name
		virtual StringView getMemberName() const override { return StringView(); }
		///some values are proxies with member name - this retrieve directly the internal value
		virtual const IValue *unproxy() const override { return this; }

		virtual bool equal(const IValue *) const  override {return false;}

		///
		/**
		 * @retval <0 *this < *other
		 * @retval ==0 *this == *other
		 * @retval >0 *this == *other
		 */
		virtual int compare(const IValue *other) const override;

		static const IValue *getUndefined();
	};

}
