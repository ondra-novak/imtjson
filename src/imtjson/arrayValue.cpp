#include "arrayValue.h"

namespace json {



	ArrayValue::ArrayValue(AllocInfo &info):Container<PValue>(info) {}



	std::size_t ArrayValue::size() const
	{
		return curSize;
	}

	const IValue * ArrayValue::itemAtIndex(std::size_t index) const
	{
		if (index < curSize) return (const IValue *)((*this)[index]);
		return getUndefined();
	}

	bool ArrayValue::enumItems(const IEnumFn &fn) const
	{
		for (auto &&x : *this) {
			if (!fn(x)) return false;
		}
		return true;
	}

	RefCntPtr<ArrayValue> ArrayValue::create(std::size_t capacity) {
		AllocInfo req(capacity);
		return new(req) ArrayValue(req);
	}

}

