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
#include "range.h"
namespace json {




///Ordered container
/** Container which has been ordered by an ordering/sorting function. There are several methods
 * available for ordered containers
 */
template<typename Cmp>
class Ordered: public Value {

	class SortFn: public RefCntObj, public AllocObject {
	public:
		SortFn(Cmp &&sortFn):sortFn(std::forward<Cmp>(sortFn)) {}

		Value sort(const Value &container) const;
		ValueIterator find(const Value &haystack, const Value &needle) const;
		Value uniq(const Value &container) const;

		template<typename ConflictFn>
		Value comm(const Value &container1, const Value &container2, int sides, const ConflictFn &fn) const;

		template<typename ReduceFn>
		Value group(const Value &container1, const ReduceFn &reduceFn) const;


		int operator()(const Value &a, const Value &b) const {
			return sortFn(a,b);
		}

	protected:
		Cmp sortFn;
	};

	static const Value &defaultConflictFn(const Value &, const Value &b) {return b;}


	enum Side {
		diffLeft = 1,
		diffRight = 2,
		common = 4
	};

public:


	///Constructs Ordered instance
	/**
	 * @param sortFn - function that defines ordering
	 */
	Ordered(Cmp &&sortFn): sortFn(new SortFn(std::forward<Cmp>(sortFn)) ) {}
	///Constructs Ordered instance, called mostly internally by various functions
	/**
	 * @param value a container - must be ordered. It is better to call Value::sort to create Ordered instance
	 * @param sortFn - function that defines ordering
	 */
	Ordered(const Value &value, const RefCntPtr<SortFn> &sortFn):Value(value), sortFn(sortFn) {}


	///Perform sort operation on other container
	/**
	 * @param value unordered container
	 * @return ordered container
	 */
	Ordered sort(const Value &value) const {
		return Ordered(sortFn->sort(value), sortFn);
	}
	///Find in ordered container
	/**
	 * @param needle value to find
	 * @return value found. Function returns undefined for not found value
	 */
	Value find(const Value &needle) const {
		ValueIterator iter = sortFn->find(*this, needle);
		if (iter.atEnd()) return undefined;
		else return *iter;
	}

	///Finds first equal item
	ValueIterator lowerBound(const Value &needle) const {
		ValueIterator iter = sortFn->find(*this, needle);
		if (iter.atEnd()) return iter;
		while (!iter.atBegin()) {
			--iter;
			if ((*sortFn)(*iter,needle) != 0) return iter+1;
		}
		return iter;
	}
	///Finds first item above specified item
	ValueIterator upperBound(const Value &needle) const {
		ValueIterator iter = sortFn->find(*this, needle);
		while (!iter.atEnd()) {
			++iter;
			if ((*sortFn)(*iter,needle) != 0) return iter;
		}
		return iter;

	}

	///Finds range where needle is equal.
	Range<ValueIterator> equalRange(const Value &needle) const {
		ValueIterator iter1 = sortFn->find(*this, needle);
		if (iter1.atEnd()) Range<ValueIterator>(iter1,iter1);
		while (!iter1.atBegin()) {
			--iter1;
			if ((*sortFn)(*iter1,needle) != 0) {
				++iter1;
				break;
			}
		}
		ValueIterator iter2 = iter1;
		while (!iter2.atEnd()) {
			++iter2;
			if ((*sortFn)(*iter2,needle) != 0) break;
		}
		return Range<ValueIterator>(iter1,iter2);
	}
	///Remove duplicated elements from ordered container
	/**
	 * Making container to have unique values
	 * @return container with unique values
	 */
	Ordered uniq() const {
		return Ordered(sortFn->uniq(*this),sortFn);
	}
	///Reduces container grouping equal values.
	/**
	 * @param fn Reduce function. The function accepts Range object, which contains range of equal
	 * items (they don't need to be equal, the sort function just considered them equal).
	 *
	 * @return Result as container
	 *
	 * The reduce function is different then Array::reduce. It has following prototype
	 *
	 * @code
	 * Value reduceFn(const Range<ValueIterator> &range);
	 * @endcode
	 */
	template<typename ReduceFn>
	Value group(const ReduceFn &fn) const {
		return sortFn->group(*this,fn);
	}


	///Merge two ordered containers
	/**
	 * @param other other container
	 *
	 * @return merged container - still ordered
	 *
	 * @note Function automatically merges same elemenets
	 */
	Ordered merge(const Ordered &other) const {
		return Ordered(sortFn->comm(*this, other, common | diffLeft | diffRight, defaultConflictFn), sortFn);
	}
	template<typename ConflictFn>
	Ordered merge(const Ordered &other, const ConflictFn &conflictFn) const {
		return Ordered(sortFn->comm(*this, other, common | diffLeft | diffRight, conflictFn), sortFn);
	}
	Ordered complement(const Ordered &other) const {
		return Ordered(sortFn->comm(*this, other, diffLeft, defaultConflictFn ), sortFn);
	}
	Ordered intersection(const Ordered &other) const  {
		return Ordered(sortFn->comm(*this, other, common, defaultConflictFn), sortFn);
	}
	template<typename ConflictFn>
	Ordered intersection(const Ordered &other, const ConflictFn &conflictFn) const  {
		return Ordered(sortFn->comm(*this, other, common, conflictFn), sortFn);
	}
	Ordered symdiff(const Ordered &other) const  {
		return Ordered(sortFn->comm(*this, other, diffLeft | diffRight, defaultConflictFn), sortFn);
	}


protected:
	RefCntPtr<SortFn> sortFn;
};

template<typename Cmp>
Value Ordered<Cmp>::SortFn::sort(const Value &container) const {
	if (container.empty()) return json::array;
	RefCntPtr<ArrayValue> av = ArrayValue::create(container.size());
	for(Value v : container) av->push_back(v.getHandle());

	std::stable_sort(av->begin(), av->end(), [&](const PValue &a, const PValue &b) {
		return this->sortFn(a,b) < 0;
	});
	return Value(PValue::staticCast(av));
}

template<typename Cmp>
ValueIterator Ordered<Cmp>::SortFn::find(const Value &haystack, const Value &needle) const {
	std::size_t l = 0, h = haystack.size();
	while (l < h) {
		std::size_t m = (l+h)/2;
		Value v = haystack[m];
		int c = sortFn(needle,v);
		if (c < 0) {
			h = m;
		} else if (c > 0) {
			l = m+1;
		} else {
			return haystack.begin()+m;
		}
	}
	return haystack.end();
}

template<typename Cmp>
Value Ordered<Cmp>::SortFn::uniq(const Value &container) const {
	std::size_t sz = container.size();
	if (sz == 0) return container;
	RefCntPtr<ArrayValue> ar = ArrayValue::create(sz);
	Value prevValue = container[0];
	for (std::size_t i = 1; i < sz; i++) {
		Value v = container[i];
		if (sortFn(prevValue, v) != 0) {
			ar->push_back(prevValue.getHandle());
			prevValue = v;
		}
	}
	ar->push_back(prevValue.getHandle());
	return PValue::staticCast(ar);
}

template<typename Cmp>
template<typename ConflictFn>
Value Ordered<Cmp>::SortFn::comm(const Value &container1,const Value &container2,int side, const ConflictFn &conflictFn) const {

	auto it1 = container1.begin();
	auto it2 = container2.begin();
	auto end1 = container1.end();
	auto end2 = container2.end();
	std::size_t estSize;
	switch (side) {
		case diffLeft | common:
		case diffLeft: estSize = container1.size();break;
		case diffRight | common:
		case diffRight: estSize = container2.size();break;
		case common:estSize = std::max(container1.size(),container2.size());break;
		default: estSize = container1.size()+container2.size();break;
	}
	RefCntPtr<ArrayValue> av = ArrayValue::create(estSize);

	bool cont = it1 != end1 && it2 != end2;
	if (cont) {

		Value a1 = *it1;
		Value a2 = *it2;

		while (true) {
			int r = sortFn(a1,a2);
			if (r < 0) {
				if (side & diffLeft) av->push_back(a1.getHandle());
				++it1;
				cont = it1 != end1;
				if (cont) a1 = *it1; else break;
			} else if (r > 0) {
				if (side & diffRight) av->push_back(a2.getHandle());
				++it2;
				cont = it2 != end2;
				if (cont) a2 = *it2; else break;
			} else {
				if (side & common) av->push_back(Value(conflictFn(a1,a2)).getHandle());
				++it1;
				++it2;
				cont = it1 != end1 && it2 != end2;
				if (cont) {
					a1 = *it1;
					a2 = *it2;
				} else {
					break;
				}
			}
		}
	}
	if (side & diffLeft) {
		while (it1 != end1) {
			av->push_back((*it1).getHandle());
			++it1;
		}
	}
	if (side & diffRight) {
		while (it2 != end2) {
			av->push_back((*it2).getHandle());
			++it2;
		}
	}


	return Value(PValue::staticCast(av));
}

template<typename Cmp>
template<typename ReduceFn>
Value Ordered<Cmp>::SortFn::group(const Value &container, const ReduceFn &reduceFn) const {
	ReduceFn fn(reduceFn);
	std::size_t sz = container.size();
	if (sz == 0) return container;
	Array result;
	ValueIterator start = container.begin();
	ValueIterator pos = start;
	ValueIterator end = container.end();
	Value prevValue = *pos;
	++pos;
	while (pos != end) {
		Value v  = *pos;
		if (sortFn(prevValue,v) != 0) {
			result.push_back(Value(fn(Range<ValueIterator>(start,pos))));
			prevValue = v;
			start = pos;
		}
		++pos;
	}
	result.push_back(Value(fn(Range<ValueIterator>(start,pos))));
	return result;
}


template<typename Fn>
inline Value Value::map(Fn&& mapFn) const {
	if (type() == object) {
		return Object(*this).map(std::forward<Fn>(mapFn));
	} else {
		return Array(*this).map(std::forward<Fn>(mapFn));
	}
}

template<typename Fn, typename Total>
inline Total Value::reduce(Fn&& reduceFn, Total &&curVal) const {
	return Array(*this).reduce(std::forward<Fn>(reduceFn),std::forward<Total>(curVal));
}


template<typename Fn>
inline Ordered<Fn> Value::sort(Fn&& sortFn) const {
	return Ordered<Fn>(std::forward<Fn>(sortFn)).sort(*this);
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

template<typename CompareFn, typename ReduceFn, typename InitVal>
inline Value Value::group(const CompareFn& cmp, const ReduceFn& reduceFn, InitVal initVal) {
	return Array(*this).group(cmp,reduceFn,initVal);
}


template<typename Fn>
inline Array Array::map(Fn &&mapFn) const {
	Array out;
	for (auto item:*this) {
		out.add(mapFn(item));
	}
	return Array(std::move(out));
}

template<typename Cmp, typename Src, typename T>
inline T genReduce(Cmp &&reduceFn, Src &&src, T &&init) {
	T acc = init;
	for (auto &&item:src) {
		acc = T(reduceFn(std::move(acc), item));
	}
	return T(std::move(acc));
}



template<typename T, typename ReduceFn>
inline T Array::reduce(ReduceFn&& reduceFn, T &&init) const {
	return genReduce(std::forward<ReduceFn>(reduceFn),*this,std::forward<T>(init));
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
	return res;
}

template<typename Fn>
inline Object Object::map(Fn &&mapFn) const {
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
	optimize();
	return genSort(cmp,*this, this->size());
}

template<typename Iter1, typename Iter2>
UInt Value::unshift(const Iter1 &start, const Iter2 &end) {
	Array newval;
	Iter1 p(start);
	while (p != end) {
		newval.push_back(Value(*p));
		++p;
	}
	for (Value x: *this) {
		newval.push_back(x);
	}
	*this = newval;
	return size();
}

template<typename Fn, typename A, typename B, typename C>
auto call_fn_3args(Fn &&fn, A &&a, B &&, C &&) -> decltype(fn(std::forward<A>(a))) {
	return fn(std::forward<A>(a));
}
template<typename Fn, typename A, typename B, typename C>
auto call_fn_3args(Fn &&fn, A &&a, B &&b, C &&) -> decltype(fn(std::forward<A>(a),std::forward<B>(b))) {
	return fn(std::forward<A>(a),std::forward<B>(b));
}
template<typename Fn, typename A, typename B, typename C>
auto call_fn_3args(Fn &&fn, A &&a, B &&b, C &&c) -> decltype(fn(std::forward<A>(a),std::forward<B>(b),std::forward<C>(c))) {
	return fn(std::forward<A>(a),std::forward<B>(b),std::forward<C>(c));
}

template<typename Fn>
Int Value::findIndex(Fn &&fn) const {
	for (UInt x = 0, cnt = size(); x < cnt; x++)
		if (call_fn_3args(std::forward<Fn>(fn), (*this)[x], x, *this)) return x;
	return -1;
}

template<typename Fn>
Int Value::rfindIndex(Fn &&fn) const {
	for (UInt x = size(); x > 0; x--)
		if (call_fn_3args(std::forward<Fn>(fn), (*this)[x-1], x-1, *this)) return x-1;
	return -1;
}
template<typename Fn>
Value Value::find(Fn &&fn) const {
	auto f = findIndex(std::forward<Fn>(fn));
	if (f == -1) return undefined;
	else return (*this)[f];
}
template<typename Fn>
Value Value::rfind(Fn &&fn) const {
	auto f = rfindIndex(std::forward<Fn>(fn));
	if (f == -1) return undefined;
	else return (*this)[f];
}
template<typename Fn>
Value Value::filter(Fn &&fn) const {
	std::vector<Value> buffer;
	buffer.reserve(size());
	for (UInt x = 0, cnt = size(); x < cnt; x++)
		if (call_fn_3args(std::forward<Fn>(fn), (*this)[x], x, *this))
			buffer.push_back((*this)[x]);

	ValueType vt;
	switch (type()) {default:case array: vt = array;break;case object: vt = object;break;}

	return Value(vt, StringView<Value>(buffer.data(),buffer.size()));
}




}



#endif /* SRC_IMMUJSON_OPERATIONS_H_ */
