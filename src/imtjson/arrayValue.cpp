#include "arrayValue.h"

namespace json {



	ArrayValue::ArrayValue(AllocInfo &info):Container<PValue>(info) {}



	std::size_t ArrayValue::size() const
	{
		return curSize;
	}

	RefCntPtr<const IValue> ArrayValue::itemAtIndex(std::size_t index) const
	{
		if (index < curSize) return (*this)[index];
		return getUndefined();
	}


	RefCntPtr<ArrayValue> ArrayValue::create(std::size_t capacity) {
		AllocInfo req(capacity);
		return new(req) ArrayValue(req);
	}

}

