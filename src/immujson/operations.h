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
		});
		return out;
	} else {
		Array out;
		this->forEach([mapFn, &out](const Value &v){
			out.add(mapFn(v));
		});
		return out;

	}
}

template<typename Fn>
inline Value json::Value::reduce(const Fn& reduceFn,
		const Value& initalValue) const {
	Value curVal = initalValue;
	this->forEach([&curVal, reduceFn](const Value &v) {
		curVal= reduceFn(curVal, v);
	});
	return curVal;
}

template<typename Fn>
inline Value json::Value::reduce(const Fn& reduceFn,
		Object&& initalValue) const {
	this->forEach([&initalValue, reduceFn](const Value &v) {
		reduceFn(initalValue, v);
	});
	return initalValue;
}

template<typename Fn>
inline Value json::Value::reduce(const Fn& reduceFn,
		Array&& initalValue) const {
	this->forEach([&initalValue, reduceFn](const Value &v) {
		reduceFn(initalValue, v);
	});
	return initalValue;
}

template<typename Fn>
inline Value json::Value::sort(const Fn& sortFn) const {
	std::vector<PValue> tmpvect;
	tmpvect.reserve(this->size());
	this->forEach([&tmpvect](const PValue &v) {
		tmpvect.push_back(v);
	});
	std::sort(tmpvect.begin(), tmpvect.end(),
			[sortFn](const PValue &v1, const PValue &v2) {
		return sortFn(v1,v2) < 0;
	});
	return new ArrayValue(std::move(tmpvect));
}



}



#endif /* SRC_IMMUJSON_OPERATIONS_H_ */
