/*
 * operations.h
 *
 *  Created on: 21. 10. 2016
 *      Author: ondra
 */

#ifndef SRC_IMMUJSON_OPERATIONS_H_
#define SRC_IMMUJSON_OPERATIONS_H_

#include "arrayValue.h"
#include "object.h"
#include "array.h"
#include "value.h"
namespace json {

template<typename Fn>
inline Value Value::map(const Fn& mapFn) const {
	if (type() == object) {
		return Object(*this).map(mapFn);
	} else {
		return Array(*this).map(mapFn);
	}
}

template<typename Fn, typename Total>
inline Total Value::reduce(const Fn& reduceFn, Total curVal) const {
	return Array(*this).reduce(reduceFn,curVal);
}


template<typename Fn>
inline Value Value::sort(const Fn& sortFn) const {
	return Array(*this).sort(sortFn);
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
inline Value Value::uniq(const Fn& sortFn) const {
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
inline Value Value::split(const Fn& sortFn) const {
	return Array(*this).split(sortFn);

}

template<typename CompareFn, typename ReduceFn, typename InitVal>
inline Value Value::group(const CompareFn& cmp, const ReduceFn& reduceFn, InitVal initVal) {
	return Array(*this).group(cmp);
}


template<typename Fn>
inline Array Array::map(Fn mapFn) const {
	Array out;
	for (auto item:*this) {
		out.add(mapFn(item));
	}
	return Array(std::move(out));
}

template<typename Cmp, typename Src, typename T>
inline T genReduce(Cmp reduceFn, const Src &src, T init) {
	T acc = init;
	for (auto &&item:src) {
		acc = (T)reduceFn(acc, item);
	}
	return T(std::move(acc));
}



template<typename T, typename ReduceFn>
inline T Array::reduce(const ReduceFn& reduceFn, T init) const {
	return genReduce<ReduceFn, Array, T>(reduceFn,*this,init);
}

template<typename Src, typename Cmp>
inline Array genSort(const Cmp &cmp, const Src &src, std::size_t expectedSize) {
	Array out;
	out.reserve(expectedSize);
	for (auto &&item:src) {
		out.push_back(item);
	}
	std::sort(out.changes.begin(), out.changes.end(),
			[cmp](const PValue &v1, const PValue &v2) {
		return cmp(v1,v2) < 0;
	});
	return Array(std::move(out));
}

template<typename Cmp>
inline Array Array::sort(const Cmp& cmp) const {
	return genSort(cmp,*this, this->size());
}

template<typename Cmp>
inline Array Array::split(const Cmp& cmp) const {
	Array out;
	Array curSet;
	Value prevValue;
	for (Value v:*this) {
		if (!prevValue.defined() ) {
			curSet.add(v);
			prevValue = v;
		} else if (cmp(prevValue,v) != 0) {
			out.add(curSet);
			curSet.clear();
			curSet.add(v);
			prevValue = v;
		} else {
			curSet.add(v);
		}
	};
	out.add(curSet);
	return Array(std::move(out));
}

template<typename Cmp, typename T, typename ReduceFn>
inline Array Array::group(const Cmp& cmp, const ReduceFn& reduceFn, T init) const {
	Array splitted = split(cmp);
	Array res;
	for(auto &&item : splitted) {
		T x = init;
		res.add(Value(item.reduce<ReduceFn, T>(reduceFn,x)));
	}
	return std::move(res);
}

template<typename Fn>
inline Object Object::map(Fn mapFn) const {
	Object out;
	for (auto item:*this) {
		out.set(item.getKey(),mapFn(item));
	}
	return Object(std::move(out));
}

template<typename T, typename ReduceFn>
inline T Object::reduce(const ReduceFn& reduceFn, T init) const {
	return genReduce(reduceFn,*this,init);
}

template<typename Cmp>
inline Array Object::sort(const Cmp& cmp) const {
	return genSort(cmp,*this, this->base.size() + this->changes.size());
}


}



#endif /* SRC_IMMUJSON_OPERATIONS_H_ */
