#include "array.h"
#include "arrayValue.h"
#include "object.h"
#include <time.h>

namespace json {

Array::Array(Value base):std::vector<Value>(base.begin(), base.end()),base(base) {

}



PValue Array::commit() const {
	return fromVector(*this).getHandle();

}

Object2Array Array::object(std::size_t pos) {
	return Object2Array(at(pos), *this, pos);
}

Array2Array Array::array(std::size_t pos) {
	return Array2Array(at(pos), *this, pos);
}

Object2Array Array::appendObject() {
	auto idx = size();
	push_back(Value());
	return object(idx);
}

Array2Array Array::appendArray() {
	auto idx = size();
	push_back(Value());
	return array(idx);
}

Value Array::fromVector(const std::vector<Value> &v) {
	RefCntPtr<ArrayValue> val = ArrayValue::create(v.size());
	for (Value x: v) val->push_back(x.getHandle());
	return Value(PValue::staticCast(val));
}

void Array::set(std::size_t idx, Value item) {
	(*this)[idx] = item;
	dirty_flag = true;
}

Array::iterator Array::insert(const_iterator pos, const Value &value) {
	dirty_flag = true;
	return Super::insert(pos, value);
}

Array::iterator Array::insert(const_iterator pos, Value &&value) {
	dirty_flag = true;
	return Super::insert(pos, std::move(value));
}

Array::iterator Array::insert(const_iterator pos, size_type count, const Value &value) {
	dirty_flag = count>0;
	return Super::insert(pos, count, value);
}

Array::iterator Array::insert(const_iterator pos, std::initializer_list<Value> ilist) {
	dirty_flag = ilist.size()>0;
	return Super::insert(pos, ilist);
}

Array::iterator Array::erase(const_iterator pos) {
	dirty_flag = true;
	return Super::erase(pos);
}

Array::iterator Array::erase(const_iterator first, const_iterator last) {
	dirty_flag = true;
	return Super::erase(first, last);
}

void Array::push_back(const Value &value) {
	dirty_flag = true;
	return Super::push_back(value);
}

void Array::push_back(Value &&value) {
	dirty_flag = true;
	return Super::push_back(std::move(value));
}

void Array::pop_back() {
	dirty_flag = true;
	return Super::pop_back();
}

void Array::resize(size_type count) {
	dirty_flag = count != size();
	return Super::resize(count);
}

void Array::resize(size_type count, const value_type &value) {
	dirty_flag = count != size();
	return Super::resize(count, value);
}

void Array::swap(Array &other) noexcept {
	dirty_flag = &other != this;
	return Super::swap(other);
}

Array& Array::append(const Value &arrayValue) {
	insert(end(), arrayValue.begin(), arrayValue.end());
	return *this;
}

bool Array::dirty() const {
	return dirty_flag;
}

void Array::clear() {
	base = json::array;
	Super::clear();
}

void Array::revert() {
	Super::clear();
	insert(end(),base.begin(),base.end());
}

ValueRef Array::makeRef(std::size_t index) {
	return ValueRef(*this, index);
}
}

