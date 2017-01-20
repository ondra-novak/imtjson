#include "objectValue.h"

namespace json {




	std::size_t ObjectValue::size() const
	{
		return curSize;
	}

	const IValue * ObjectValue::itemAtIndex(std::size_t index) const
	{
		if (index < curSize) return (const IValue *)((*this)[index]);
		return getUndefined();
	}

	bool ObjectValue::enumItems(const IEnumFn &fn) const
	{
		for (auto &&x : *this) {
			if (!fn(x)) return false;
		}
		return true;
	}

	const IValue * ObjectValue::member(const StringView<char>& name) const {
		const IValue *r = findSorted(name,getUndefined());
		if (r == nullptr) return getUndefined();
		else return r;
	}
	const IValue *findSorted(const StringView<char> &name) const
	{
		std::size_t l = 0;
		std::size_t r = size();
		while (l < r) {
			std::size_t m = (l + r) / 2;
			int c = name.compare(operator[](m)->getMemberName());
			if (c > 0) {
				l = m + 1;
			}
			else if (c < 0) {
				r = m;
			}
			else {
				return operator[](m);
			}
		}
		return nullptr;
	}


	void ObjectValue::sort() {
		std::sort(begin(),end(),[](const PValue &left, const PValue &right) {
			return left->getMemberName().compare(right->getMemberName()) < 0;
		});
		StrViewA lastKey;
		std::size_t wrpos = 0;
		for (const PValue &v: *this) {
			StrViewA k = v->getMemberName();
			if (k == lastKey && wrpos) {
				wrpos--;
			}
			operator[](wrpos) = v;
			wrpos++;
			lastKey = k;
		}
		while (curSize > wrpos)
			pop_back();

	}

	RefCntPtr<ObjectValue> ObjectValue::create(std::size_t capacity) {
		AllocInfo nfo(capacity);
		return new(nfo) ObjectValue(nfo);
	}

	static int balance(const PValue &l, const PValue &r) {
		if (l==nullptr) return 1;
		if (r==nullptr) return -1;
		StrViewA ln = l->getMemberName();
		StrViewA rn = r->getMemberName();
		return ln.compare(rn);
	}

	RefCntPtr<ObjectValue> ObjectValue::merge(const ObjectValue *oldObj, const ObjectValue *newObj) {
		std::size_t needsz;
		const IValue *undef = getUndefined();

		json::merge(oldObj->begin(), oldObj->end(), newObj->begin(), newObj->end(),
				[&](const PValue &o, const PValue &n) {

			int i = balance(o,n);
			if (i == -1) {
				if (n != undef && !(n->flags() & objectDiff))
			}

			if (n == nullptr) {
				if (o != undef && (o->flags() & objectDiff)==0) needsz++;

			}
			if (n == nullptr) {
				needsz++;
				return 0;
			}
			if (o == nullptr) {
				if (n->defined() && (n->type() != object || (n->flags() & objectDiff) == 0))
					needsz++;
				return 0;
			}
			StrViewA on = o->getMemberName();
			StrViewA nn = n->getMemberName();
			if (on < nn) {
				needsz++;
				return -1;
			}
			if (on > nn) {
				if (n->defined()) needsz++;
				return 1;
			} else if (n)
		}
		)
	}

}

