#pragma once

#include <vector>
#include "basicValues.h"
#include "container.h"

namespace json {

	class Object;

	class ObjectValue : public AbstractObjectValue, public Container<PValue> {
	public:

		using Container<PValue>::Container;

		virtual std::size_t size() const override;
		virtual const IValue *itemAtIndex(std::size_t index) const override;
		virtual bool enumItems(const IEnumFn &) const override;
		virtual const IValue *member(const StringView<char> &name) const override;

		StringView<PValue> getItems() const { return StringView<PValue>(items,curSize); }
		virtual bool getBool() const override {return true;}

		void sort();

		static RefCntPtr<ObjectValue> create(std::size_t capacity);

		using Container<PValue>::operator new;
		using Container<PValue>::operator delete;

	};


}
