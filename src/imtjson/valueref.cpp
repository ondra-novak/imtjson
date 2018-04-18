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

static Value getValueOrName(const Object &source, StrViewA name) {
	Value v = source[name];
	if (v.getKey() != name) return Value(name, v);
	else return v;
}

ValueRef::ValueRef(Object& obj, const StrViewA& name)
	:Value(getValueOrName(obj,name)),typeOrIndex(objectRef),objptr(&obj)
{
}
ValueRef::ValueRef(Array& arr, std::uintptr_t index)
	:Value(arr[index]),typeOrIndex(index),arrptr(&arr)
{

}

ValueRef::ValueRef(Value& var)
	:Value(var),typeOrIndex(valueRef),valptr(&var)
{
}


ValueRef& ValueRef::operator =(const Value& val) {
	switch (typeOrIndex) {
	case objectRef:
		Value::operator= (Value(getKey(),val));
		objptr->set(*this);
		break;
	case valueRef:
		Value::operator= (val);
		*valptr = *this;
		break;
	default:
		Value::operator= (val);
		arrptr->set(typeOrIndex, val);
		break;
	}
	return *this;
}

ValueRef& json::ValueRef::operator =(const ValueRef& val) {
	return this->operator=((Value)val);
}

ValueRef::ValueRef(const ValueRef& other):Value(other),typeOrIndex(other.typeOrIndex) {
	switch (typeOrIndex) {
	case objectRef: arrptr = other.arrptr;break;
	case valueRef: valptr = other.valptr;break;
	default: arrptr = other.arrptr;break;
	}
}

const Value &ValueRef::sync() {
	switch (typeOrIndex) {
	case objectRef:
		Value::operator= (getValueOrName(*objptr, getKey()));
		break;
	case valueRef:
		Value::operator= (*valptr);
		break;
	default:
		Value::operator= (arrptr->operator [](typeOrIndex));
		break;
	}
	return *this;

}


}
