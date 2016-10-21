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

class MergeResult {
public:

	enum Side {
		left,
		right,
		both,
		autodetect,
	};

	MergeResult(const Value &value, Side side):value(value),side(side) {}
	MergeResult(const Value &value):value(value),side(autodetect) {}

	void advIterators(ValueIterator &l, ValueIterator &r) {
		switch (side) {
		case left: ++l;break;
		case right: ++r;break;
		case both: ++l;++r;break;
		case autodetect:
			if ((*l).getHandle() == value.getHandle()) {
				++l;
				if ((*r).getHandle() == value.getHandle()) ++r;
			}
			else if ((*r).getHandle() == value.getHandle()) {
				++r;
			} else {
				++l;++r;
			}
			break;
		}
	}
	operator const Value &() const {return value;}

protected:
	Value value;
	Side side;


};

///Inside of merge function (see Value::merge() ) the function states that it chose the left side with the result
MergeResult chooseLeft(const Value &v) {
	return MergeResult(v,MergeResult::left);
}

///Inside of merge function (see Value::merge() ) the function states that it chose the right side with the result
MergeResult chooseRight(const Value &v) {
	return MergeResult(v,MergeResult::right);
}

///Inside of merge function (see Value::merge() ) the function states that it chose both sides with the result
MergeResult chooseBoth(const Value &v) {
	return MergeResult(v,MergeResult::both);
}

template<typename Fn, typename StoreFn>
inline void _runMerge(const Value& left, const Value& right, Fn mergeFn, StoreFn storeFn )  {

	auto liter = left.begin();
	auto riter = right.begin();
	auto lend = left.end();
	auto rend = right.end();
	while (liter != lend && riter != rend) {
		MergeResult mres (mergeFn(*liter,*riter));
		mres.advIterators(liter,riter);
		storeFn(mres);
	}
	while (liter != lend) {
		Value res = mergeFn(*liter, Value(undefined));
		liter++;
		storeFn(res);
	}
	while (riter != rend) {
		Value res = mergeFn(Value(undefined),*riter);
		riter++;
		storeFn(res);
	}

}

template<typename Fn>
inline Value Value::merge(const Value& other, const Fn& mergeFn, bool toObject) const {
	if (toObject) {
		Object obj;
		_runMerge(*this, other, mergeFn, [&obj](const Value &r) {obj.set(r);});
		return obj;
	} else {
		Array arr;
		_runMerge(*this, other, mergeFn, [&arr](const Value &r) {arr.add(r);});
		return arr;
	}
}

template<typename Fn, typename ReduceFn, typename Total>
Total Value::mergeReduce(const Value &other, const Fn &mergeFn, const ReduceFn &reduceFn, Total total) const {
	_runMerge(*this, other, mergeFn, [&total, reduceFn](const Value &r) {
		total = reduceFn(total,r);
	});
	return total;
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
