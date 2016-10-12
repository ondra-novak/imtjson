#pragma once

#include <vector>
#include "basicValues.h"

namespace json {

	class Array;

	class ArrayValue : public AbstractArrayValue {
	public:

		ArrayValue(std::vector<PValue> &&value);
		
		virtual std::size_t size() const override;
		virtual const IValue *itemAtIndex(std::size_t index) const override;
		virtual bool enumItems(const IEnumFn &) const override;
		virtual bool getBool() const override {return true;}


	protected:
		std::vector<PValue> v;

	};


}
