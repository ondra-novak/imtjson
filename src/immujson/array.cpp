#include "array.h"
#include "arrayValue.h"
#include "edit.h"
#include <time.h>

namespace json {

	Array::Array(Value value): base(value),changes(value)
	{
	}
	Array::Array() :  base(AbstractArrayValue::getEmptyArray())
	{
	}
	Array & Array::push_back(const Value & v)
	{
		changes.push_back(v.v);
		return *this;
	}
	Array & Array::add(const Value & v)
	{
		changes.push_back(v.v->unproxy());
		return *this;
	}
	Array & Array::addSet(const StringView<Value>& v)
	{
		changes.reserve(changes.size() + v.length);
		for (std::size_t i = 0; i < v.length; i++)
			changes.push_back(v.data[i].v->unproxy());
		return *this;
	}
	Array & Array::insert(std::size_t pos, const Value & v)
	{
		extendChanges(pos);
		changes.insert(changes.begin()+(pos - changes.offset), v.v->unproxy());
		return *this;
	}
	Array & Array::insertSet(std::size_t pos, const StringView<Value>& v)
	{
		extendChanges(pos);
		changes.insert(changes.begin() + (pos - changes.offset), v.length, PValue());
		for (std::size_t i = 0; i < v.length; i++) {
			changes[pos + changes.offset + i] = v.data[i].v->unproxy();
		}
		return *this;
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
		std::vector<PValue> result;
		result.reserve(changes.offset + changes.size());
		for (std::size_t x = 0; x < changes.offset; ++x) {
			result.push_back(base[x].v);
		}
		for (auto &&x : changes) {
			if (x->type() != undefined) result.push_back(x);
		}
		return new ArrayValue(std::move(result));


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

}

