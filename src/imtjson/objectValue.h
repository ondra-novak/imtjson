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
		virtual RefCntPtr<const IValue> itemAtIndex(std::size_t index) const override;
		virtual RefCntPtr<const IValue> member(const std::string_view &name) const override;

		virtual bool getBool() const override {return true;}

		void sort();
		const IValue *findSorted(const std::string_view &name) const;

		static RefCntPtr<ObjectValue> create(std::size_t capacity);
		RefCntPtr<ObjectValue> clone() const;

		using Container<PValue>::operator new;
		using Container<PValue>::operator delete;

		bool isDiff = false;

		virtual ValueTypeFlags flags() const override {return isDiff?valueDiff:0;}

		ObjectValue &operator=(const ObjectValue &other);

	};


}
