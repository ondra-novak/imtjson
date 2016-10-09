#pragma once

#include <vector>
#include "basicValues.h"

namespace json {

	class Object;

	class ObjectValue : public AbstractObjectValue {
	public:

		ObjectValue(const std::vector<PValue> &value);

		virtual std::size_t size() const override;
		virtual const IValue *itemAtIndex(std::size_t index) const override;
		virtual bool enumItems(const IEnumFn &) const override;
		virtual const IValue *member(const StringRef<char> &name) const override;

		const std::vector<PValue> &getItems() const { return v; }

	protected:
		std::vector<PValue> v;



	};


}