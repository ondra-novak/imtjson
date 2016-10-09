#include "objectValue.h"

namespace json {



	ObjectValue::ObjectValue(const std::vector<PValue>& value)
		:v(value)
	{
	}


	std::size_t ObjectValue::size() const
	{
		return v.size();
	}

	const IValue * ObjectValue::itemAtIndex(std::size_t index) const
	{
		if (index < v.size()) return (const IValue *)(v[index]);
		return getUndefined();
	}

	bool ObjectValue::enumItems(const IEnumFn &fn) const
	{
		for (auto &&x : v) {
			if (!fn(x)) return false;
		}
		return true;
	}

	const IValue * ObjectValue::member(const StringRef<char>& name) const
	{
		std::size_t l = 0;
		std::size_t r = v.size();
		while (l < r) {
			std::size_t m = (l + r) / 2;
			int c = name.compare(v[m]->getMemberName());
			if (c > 0) {
				l = m + 1;
			}
			else if (c < 0) {
				r = m;
			}
			else {
				return v[m];
			}
		}
		return getUndefined();
	}

}
