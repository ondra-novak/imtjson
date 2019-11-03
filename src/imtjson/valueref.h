#pragma once

#include "value.h"

namespace json {

class Object;
class Array;

///Stores a reference to a JSON variable
/**
 * Reference to value. It acts like ordinary Value, in exception you can actually assign a value to it.
 * The objects remembers the source of the value, and assigning to the variable causes, that original
 * variable is modified, like an ordinary reference supose to work. However, this object allows to
 * make a reference to a member variable of a container - object or array. Setting the value to the reference
 * also overwrites the value in the source container.
 *
 * This feature doesn't break immutability, because you cannot modify containers stored as the Value. Only
 * containers stored as the Object or the Array can create reference.
 *
 * Similar to common reference, the reference remains valid as long as source value is valid. Accessing
 * the reference after destroying the source container causes undefined behavoir or crash.
 *
 * Unlike to common reference, content of this reference is not synced with other references created from
 * the same source. For implementation reason, the ValueRef objects keeps copy of the value for fast
 * access. The copy is updated everytime the value is modified throught the reference object, but not in
 * situation, when the source is modified by another way. If you need to have fresh copy of the source value,
 * you need to manually call the function sync()
 *

 */
class ValueRef: public Value {
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
	ValueRef(Array &arr, UInt index);
	///Constructs a reference to a standalone variable
	/**
	 * @param var reference to the variable
	 *
	 * @note the constructor creates something like Value &. However you need ValueRef when you
	 * need to other two reference types
	 */
	explicit ValueRef(Value &var);


	///Make copy of the reference
	/**
	 * This makes copy of the reference.
	 * @param other source reference
	 */
	ValueRef(const ValueRef &other);

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


	///Obtains the fresh copy of the source value
	/**
	 * Unlike the common C++ reference, this object keeps copy of the source value. This is the
	 * reason, why modifying the source value doesn't change the value stored in the reference. You
	 * need to call this function to obtain fresh copy of the source value.
	 */
	const Value &sync();

protected:

	///contains index in array. For non-array container, it contains type of container, see declaration of constants
	UInt typeOrIndex;


	///Reference to object's member variable. The key can be retrieved by the function getKey()
	static const UInt objectRef = (UInt)-1;
	///Direct reference to an instance of Value
	static const UInt valueRef = (UInt)-2;

	union {
		///pointer to object container, valid when typeOrIndex is equal to objectRef
		Object *objptr;
		///pointer to array container, valid when typeOrIndex contains valid index
		Array *arrptr;
		///pointer to the source value (directly), valid when typeOrIndex is equal to valueRef
		Value *valptr;
	};

};



}
