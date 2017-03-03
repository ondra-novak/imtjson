#pragma once

#include "value.h"

namespace json {

class Object;
class Array;

///Stores a reference to a JSON variable
/**
 * It makes universal reference to a JSON variable and allows change it later. This doesn't
 * break immutability, because you can create reference only to mutable objects. You
 * can create the reference to variable of type Value, similar to Value &, however, you can
 * also create the reference to a object's member variable or to a value located in an array.
 *
 * The object ValueRef can be converted to Value, which perform reading the value of the referenced
 * variable. The object ValueRef also exposes the assigment operator which allows to change the
 * referenced variable. This is particular useful for variables located in some container.
 *
 * @note Similar to standard references, this object also becomes invalid, when referenced variable
 * or container is destroyed. Accessing the destroyed variable can involve the crash. You should
 * remove all references to destroying referenced variable
 *

 */
class ValueRef {
public:

	///Constructs a reference to member variable of the object
	/**
	 * @param obj object where the variable is located
	 * @param name name of the variable
	 *
	 * It is better to call Object::makeRef(name);
	 *
	 * It is possible to make reference to variable which doesn't exists yet. First assigment will
	 * create it.
	 */
	ValueRef(Object &obj, const StrViewA &name);
	///Constructs a reference to member variable of the array
	/**
	 * @param arr an array where the variable is located
	 * @param index index to the array
	 *
	 * It is better to call Array::makeRef(index);
	 *
	 * The variable should exist
	 */
	ValueRef(Array &arr, std::uintptr_t index);
	///Constructs a reference to a standalone variable
	/**
	 * @param var reference to the variable
	 *
	 * @note the constructor creates something like Value &. However you need ValueRef when you
	 * need to other two reference types
	 */
	explicit ValueRef(Value &var);


	///Reads the value
	operator Value() const;
	///Writes the value
	ValueRef &operator=(const Value &val);
	///Once the reference is constructed, it cannot be redirected.
	/**
	 * The same rule is applied for standard C++ references. Assignment causes
	 * that value is transferred.
	 *
	 * @param val reference to value. The value is transfered
	 * @return reference to this reference
	 */
	ValueRef &operator=(const ValueRef &val);


protected:

	static const std::uintptr_t objectRef = (std::uintptr_t)-1;
	static const std::uintptr_t valueRef = (std::uintptr_t)-2;

	std::uintptr_t typeOrIndex;

	union {
		Object *objptr;
		Array *arrptr;
		Value *valptr;
	};

	Value curValue;
};



}
