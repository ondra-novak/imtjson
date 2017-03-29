#pragma once

#include "ivalue.h"

namespace json {



	class AbstractValue : public IValue {
	public:
		virtual ValueType type() const override { return undefined; }
		virtual ValueTypeFlags flags() const override { return 0; }

		virtual std::uintptr_t getUInt() const override { return 0; }
		virtual std::intptr_t getInt() const override { return 0; }
		virtual double getNumber() const override { return 0.0; }
		virtual bool getBool() const override { return false; }
		virtual StringView<char> getString() const override { return StringView<char>(); }
		virtual std::size_t size() const override { return 0; }
		virtual const IValue *itemAtIndex(std::size_t index) const override { return getUndefined(); }
		virtual const IValue *member(const StringView<char> &name) const override { return getUndefined(); }
		virtual bool enumItems(const IEnumFn &) const override { return true; }

		///some values are proxies with member name - this retrieves name
		virtual StringView<char> getMemberName() const override { return StringView<char>(); }
		///some values are proxies with member name - this retrieve directly the internal value
		virtual const IValue *unproxy() const override { return this; }

		virtual bool equal(const IValue *other) const  override {return false;}

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
