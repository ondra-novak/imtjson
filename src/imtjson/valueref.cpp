/*
 * valueref.cpp
 *
 *  Created on: 3. 3. 2017
 *      Author: ondra
 */

#include "valueref.h"

#include "object.h"
#include "array.h"

namespace json {

ValueRef::ValueRef(Object& obj, const StrViewA& name)
	:typeOrIndex(objectRef),objptr(&obj),curValue(const_cast<const Object &>(obj).operator [](name))
{
	//we need to save name somewhere
	if (!curValue.defined()) {
		curValue = Value(name, undefined);
	}
}

ValueRef::ValueRef(Array& arr, std::uintptr_t index)
	:typeOrIndex(index),arrptr(&arr)
{

}

ValueRef::ValueRef(Value& var)
	:typeOrIndex(valueRef),valptr(&var)
{
}

ValueRef::operator Value() const {
	switch (typeOrIndex) {
	case objectRef:
		return const_cast<const Object *>(objptr)->operator [](curValue.getKey());
	case valueRef: return *valptr;
	default:
		return const_cast<const Array *>(arrptr)->operator [](typeOrIndex);
	}
}

ValueRef& ValueRef::operator =(const Value& val) {
	switch (typeOrIndex) {
	case objectRef:
		curValue = Value(curValue.getKey(),val);
		objptr->checkInstance();
		objptr->set(curValue);
		break;
	case valueRef:
		*valptr = val;
		break;
	default:
		arrptr->checkInstance();
		arrptr->set(typeOrIndex, val);
		break;
	}
	return *this;
}

ValueRef& json::ValueRef::operator =(const ValueRef& val) {
	return this->operator=((Value)val);
}



}

