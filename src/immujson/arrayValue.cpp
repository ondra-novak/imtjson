#include "arrayValue.h"

namespace json {




	ArrayValue::ArrayValue(std::vector<PValue>&& value)
		:v(std::move(value))
	{
	}

	std::size_t ArrayValue::size() const
	{
		return v.size();
	}

	const IValue * ArrayValue::itemAtIndex(std::size_t index) const
	{
		if (index < v.size()) return (const IValue *)(v[index]);
		return getUndefined();
	}

	bool ArrayValue::enumItems(const IEnumFn &fn) const
	{
		for (auto &&x : v) {
			if (!fn(x)) return false;
		}
		return true;
	}

}