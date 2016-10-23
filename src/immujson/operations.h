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
		curVal = Total(reduceFn(curVal, v));
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


///Perform simple merge (alternative to std::merge)
/**
 * @param liter first starting iterator
 * @param lend first ending iterator
 * @param riter second starting iterator
 * @param rend second ending iterator
 * @param fn a function which receives values from first and second container.
 *  The function must return <0 to advance the first iterator, >0 to advance
 *   the second iterator, or =0 to advance both iterators.
 * @param undefined defines value used as placeholder when one
 *  of the iterators reached its end. The function can detect, that merge is
 *  in the final phase. This value appear as the first argument when the first
 *  iterator reached its end, or as the second argument when to
 *  second iterator reached  its end. In both these situations, return
 *  value of the function is ignored.
 */
template<typename Iter1, typename Iter2, typename Fn, typename Undefined>
void merge(Iter1 liter, Iter1 lend, Iter2 riter, Iter2 rend, Fn fn, Undefined undefined)  {

	using FnRet = decltype(fn(*liter,*riter));
	const FnRet zero(0);

	while (liter != lend && riter != rend) {
		FnRet res = fn(*liter,*riter);
		if (res<zero) ++liter;
		else if (res>zero) ++riter;
		else {
			++liter;
			++riter;
		}
	}
	while (liter != lend) {
		fn(*liter,undefined);
		++liter;
	}
	while (riter != rend) {
		fn(undefined,*riter);
		++riter;
	}

}


template<typename Fn>
void Value::merge(const Value &other, const Fn &mergeFn) const {

	json::merge(this->begin(),this->end(),
				other.begin(),other.end(),
				mergeFn, undefined);
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
