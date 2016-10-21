/*
 * operations.h
 *
 *  Created on: 21. 10. 2016
 *      Author: ondra
 */

#ifndef SRC_IMMUJSON_OPERATIONS_H_
#define SRC_IMMUJSON_OPERATIONS_H_

#include "arrayValue.h"
#include "value.h"
namespace json {

template<typename Fn>
inline Value json::Value::map(const Fn& mapFn) const {
	if (type() == object) {
		Object out;
		this->forEach([mapFn, &out](const Value &v){
			out.set(v.getKey(),mapFn(v));
			return true;
		});
		return out;
	} else {
		Array out;
		this->forEach([mapFn, &out](const Value &v){
			out.add(mapFn(v));
			return true;
		});
		return out;

	}
}

template<typename Fn, typename Total>
inline Total json::Value::reduce(const Fn& reduceFn, Total curVal) const {
	this->forEach([&curVal, reduceFn](const Value &v) {
		curVal= reduceFn(curVal, v);
		return true;
	});
	return curVal;
}


template<typename Fn>
inline Value json::Value::sort(const Fn& sortFn) const {
	std::vector<PValue> tmpvect;
	tmpvect.reserve(this->size());
	this->forEach([&tmpvect](const PValue &v) {
		tmpvect.push_back(v);
		return true;
	});
	std::sort(tmpvect.begin(), tmpvect.end(),
			[sortFn](const PValue &v1, const PValue &v2) {
		return sortFn(v1,v2) < 0;
	});
	return new ArrayValue(std::move(tmpvect));
}


template<typename Fn>
void Value::merge(const Value &other, Fn mergeFn) const {

	auto liter = this->begin();
	auto riter = other.begin();
	auto lend = this->end();
	auto rend = other.end();
	while (liter != lend && riter != rend) {
		int res = mergeFn(*liter,*riter);
		if (res<0) ++liter;
		else if (res>0) ++riter;
		else {
			++liter;
			++riter;
		}
	}
	while (liter != lend) {
		mergeFn(*liter,undefined);
		++liter;
	}
	while (riter != rend) {
		mergeFn(undefined,*riter);
		++riter;
	}
}

template<typename Fn>
inline Value Value::mergeToArray(const Value& other, const Fn& mergeFn) const {
	Array collector;
	merge(other,[&collector,mergeFn](const Value &a, const Value &b) {
		return mergeFn(a,b,collector);
	});
	return collector;
}

template<typename Fn>
inline Value Value::makeIntersection(const Value& other,const Fn& sortFn) const {
	return mergeToArray(other,
			[sortFn](const Value &left, const Value &right, Array &collect) -> decltype(sortFn(Value(),Value())) {
		if (!left.defined() || !right.defined()) return 0;
		auto cmp = sortFn(left,right);
		if (cmp == 0) collect.add(right);
		return cmp;
	});
}

template<typename Fn>
inline Value Value::makeUnion(const Value& other, const Fn& sortFn) const {
	return mergeToArray(other,
			[sortFn](const Value &left, const Value &right, Array &collect) -> decltype(sortFn(Value(),Value())) {
		if (!left.defined()) {
			collect.add(right);return 0;
		} else if (!right.defined()) {
			collect.add(left);return 0;
		} else {
			auto cmp = sortFn(left,right);
			if (cmp < 0) {
				collect.add(left);
			} else {
				collect.add(right);
			}
			return cmp;
		}
	});
}

template<typename Fn>
inline Value Value::mergeToObject(const Value& other, const Fn& mergeFn) const {
	Object collector;
	merge(other,[&collector,mergeFn](const Value &a, const Value &b) {
		return mergeFn(a,b,collector);
	});
	return collector;
}


template<typename Fn>
inline Value json::Value::uniq(const Fn& sortFn) const {
	Array out;
	Value prevValue;
	forEach([&](const Value &v){
		if (!prevValue.defined() || sortFn(prevValue,v) != 0) {
			out.add(v);
			prevValue = v;
		}
		return true;
	});
	if (out.size() == size()) return *this;
	return out;
}

template<typename Fn>
inline Value json::Value::split(const Fn& sortFn) const {
	Array out;
	Array curSet;
	Value prevValue;
	forEach([&]( const Value &v){
		if (!prevValue.defined() ) {
			curSet.add(v);
			prevValue = v;
		} else if (sortFn(prevValue,v) != 0) {
			out.add(curSet);
			curSet.clear();
			curSet.add(v);
			prevValue = v;
		} else {
			curSet.add(v);
		}
		return true;
	});
	out.add(curSet);
	return out;

}


}



#endif /* SRC_IMMUJSON_OPERATIONS_H_ */
