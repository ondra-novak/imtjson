#pragma once

#include <vector>
#include "basicValues.h"
#include "container.h"

namespace json {

	class Array;

	class ArrayValue : public AbstractArrayValue, public Container<PValue> {
	public:

		typedef Container<PValue>::AllocInfo AllocInfo;

		ArrayValue(AllocInfo &info);
		
		virtual std::size_t size() const override;
		virtual RefCntPtr<const IValue> itemAtIndex(std::size_t index) const override;

		virtual bool getBool() const override {return true;}


		static RefCntPtr<ArrayValue> create(std::size_t capacity);

		using Container<PValue>::operator new;
		using Container<PValue>::operator delete;
	};


}
