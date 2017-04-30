#include "array.h"
#include "arrayValue.h"
#include "object.h"
#include <time.h>

namespace json {

	Array::Array(Value value): base(value),changes(value)
	{
	}
	Array::Array() :  base(AbstractArrayValue::getEmptyArray())
	{
	}
	Array::Array(const std::initializer_list<Value> &v) {
		reserve(v.size());
		for (auto &&x : v) push_back(x);
	}

	Array & Array::push_back(const Value & v)
	{
		changes.push_back(v.v);
		return *this;
	}
	Array & Array::add(const Value & v)
	{
		changes.push_back(v.v);
		return *this;
	}
	Array & Array::addSet(const StringView<Value>& v)
	{
		changes.reserve(changes.size() + v.length);
		for (std::size_t i = 0; i < v.length; i++)
			changes.push_back(v.data[i].v);
		return *this;
	}
	Array& json::Array::addSet(const Value& v) {
		changes.reserve(changes.size() + v.size());
		for (std::size_t i = 0, cnt = v.size(); i < cnt; i++)
			changes.push_back(v[i].getHandle());
		return *this;
	}

	Array & Array::insert(std::size_t pos, const Value & v)
	{
		extendChanges(pos);
		changes.insert(changes.begin()+(pos - changes.offset), v.v);
		return *this;
	}
	Array & Array::insertSet(std::size_t pos, const StringView<Value>& v)
	{
		extendChanges(pos);
		changes.insert(changes.begin() + (pos - changes.offset), v.length, PValue());
		for (std::size_t i = 0; i < v.length; i++) {
			changes[pos + changes.offset + i] = v.data[i].v;
		}
		return *this;
	}
	Array& json::Array::insertSet(std::size_t pos, const Value& v) {
		extendChanges(pos);
		changes.insert(changes.begin() + (pos - changes.offset), v.size(), PValue());
		for (std::size_t i = 0, cnt = v.size(); i < cnt; i++) {
			changes[pos + changes.offset + i] = v[i].v;
		}
		return *this;
	}

	Array& json::Array::addSet(const std::initializer_list<Value>& v) {
		return addSet(StringView<Value>(v));
	}

	Array& json::Array::insertSet(std::size_t pos,
			const std::initializer_list<Value>& v) {
		return insertSet(pos,StringView<Value>(v));
	}

	Array & Array::erase(std::size_t pos)
	{
		extendChanges(pos);
		changes.erase(changes.begin() + (pos - changes.offset));
		return *this;
	}
	Array & Array::eraseSet(std::size_t pos, std::size_t length)
	{
		extendChanges(pos);
		auto b = changes.begin() + (pos - changes.offset);
		auto e = b + length ;
		changes.erase(b,e);
		return *this;
	}
	Array & Array::trunc(std::size_t pos)
	{
		if (pos < changes.offset) {
			changes.offset = pos;
			changes.clear();
		}
		else {
			changes.erase(changes.begin() + (pos - changes.offset), changes.end());
		}
		return *this;
	}

	Array &Array::clear() {
		changes.offset = 0;
		changes.clear();
		return *this;
	}

	Array &Array::revert() {
		changes.offset = base.size();
		changes.clear();
		return *this;
	}

	Array & Array::set(std::size_t pos, const Value & v)
	{
		extendChanges(pos);
		changes[pos - changes.offset] = v.getHandle();
		return *this;
	}

	Value Array::operator[](std::size_t pos) const
	{
		if (pos < changes.offset) return base[pos];
		else {
			pos -= changes.offset;
			return changes.at(pos);
		}
	}

	ValueRef Array::makeRef(std::size_t pos) {
		return ValueRef(*this, pos);
	}

	std::size_t Array::size() const
	{
		return changes.offset + changes.size();
		
	}

	bool Array::empty() const
	{
		return size() == 0;
	}


	PValue Array::commit() const
	{
		if (empty()) return AbstractArrayValue::getEmptyArray();
		if (!dirty()) return base.getHandle();

		std::size_t needSz = changes.offset + changes.size();
		RefCntPtr<ArrayValue> result = ArrayValue::create(needSz);

		for (std::size_t x = 0; x < changes.offset; ++x) {
			result->push_back(base[x].getHandle());
		}
		for (auto &&x : changes) {
			if (x->type() != undefined) result->push_back(x);
		}
		return PValue::staticCast(result);


	}
	Object2Array Array::object(std::size_t pos)
	{
		return Object2Array((*this)[pos],*this,pos);
	}
	Array2Array Array::array(std::size_t pos)
	{
		return Array2Array((*this)[pos], *this, pos);
	}
	bool Array::dirty() const
	{

		return changes.offset != base.size() || !changes.empty();
	}
	void Array::extendChanges(size_t pos)
	{
		if (pos < changes.offset) {
			changes.insert(changes.begin(), changes.offset - pos, PValue());
			for (std::size_t x = pos; x < changes.offset; ++x) {
				changes[x - pos] = base[x].v;
			}
			changes.offset = pos;
		}
	}

	Array::~Array()
	{
		
	}


	ArrayIterator Array::begin() const {
		return ArrayIterator(this,0);
	}

	ArrayIterator Array::end() const {
		return ArrayIterator(this,size());
	}


	Array &Array::reserve(std::size_t items) {
		changes.reserve(items);
		return *this;
	}

	Object2Array Array::addObject() {
		std::size_t pos = size();
		add(AbstractObjectValue::getEmptyObject());
		return object(pos);
	}

	Array2Array Array::addArray() {
		std::size_t pos = size();
		add(AbstractArrayValue::getEmptyArray());
		return array(pos);
	}

	StringView<PValue> Array::getItems(const Value& v) {
		const IValue *pv = v.getHandle();
		const ArrayValue *av = dynamic_cast<const ArrayValue *>(pv->unproxy());
		if (av) return av->getItems(); else return StringView<PValue>();
	}

Array::Array(const Array& other):base(other.base),changes(other.changes) {

}

Array::Array(Array&& other):base(std::move(other.base)),changes(std::move(other.changes)) {
}

Array& Array::operator =(const Array& other) {
	base = other.base;
	changes = other.changes;
	return *this;
}

Array& Array::operator =(Array&& other) {
	base = std::move(other.base);
	changes = std::move(other.changes);
	return *this;
}


Array::Changes::Changes(const Changes& base)
	:std::vector<PValue>(base),offset(base.offset)
{
}

Array::Changes::Changes(Changes&& base)
	:std::vector<PValue>(std::move(base)),offset(base.offset) {
}

Array::Changes& Array::Changes::operator =(const Changes& base) {
	std::vector<PValue>::operator =(base);
	offset = base.offset;
	return *this;
}

Array::Changes& Array::Changes::operator =(Changes&& base) {
	std::vector<PValue>::operator =(std::move(base));
	offset = base.offset;
	return *this;
}

Array& Array::reverse() {
	Array out;
	for (std::size_t x = size(); x > 0; x--)
		out.add((*this)[x-1]);
	*this = std::move(out);
	return *this;
}


Array& Array::slice(std::intptr_t start) {
	if (start < 0) {
		if (-start < (std::intptr_t)size()) {
			return slice(size()+start);
		}
	} else if (start >= (std::intptr_t)size()) {
		clear();
	} else {
		eraseSet(0,start);
	}
	return *this;
}


Array& Array::slice(std::intptr_t start, std::intptr_t end) {
	if (end < 0) {
		if (-end < (std::intptr_t)size()) {
			slice(start, (std::intptr_t)size()+end);
		} else {
			clear();
		}
	} else if (end < (std::intptr_t)size()) {
		trunc(end);
	}
	return slice(start);
}

Array& json::Array::setSize(std::size_t length, Value fillVal) {
	reserve(length);
	if (size() > length) return trunc(length);
	while (size() < length) add(fillVal);
	return *this;
}


}

