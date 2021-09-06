/*
 * path.h
 *
 *  Created on: 13. 10. 2016
 *      Author: ondra
 */

#ifndef SRC_IMMUJSON_PATH_H_
#define SRC_IMMUJSON_PATH_H_
#include <memory>

#include "value.h"


#pragma once

namespace json {

class Path;
class PPath;
class PathRef;

///Defines path in JSON document
/** You can define path as set of keys ordered from specified key to the root
 *
 * Every path has its root element. The parent of the root is always root itself. You can test
 * whether the current element is root using the function isRoot()
 *
 */
class Path {

public:
	///Always the root element
	static const Path &root;

	///Construct path relative to other element
	/**
	 *
	 * @param parent reference to parent path. If you need to construct relative to root, use Path::root as reference
	 * @param key name of key
	 */
	Path(const Path &parent, const std::string_view &key):keyName(key),index(-1),parent(parent) {}

	///Construct path relative to other element in an array
	/**
	 * @param parent reference to parent path. If you need to construct relative to root, use Path::root as reference
	 * @param index index of item in array
	 */
	Path(const Path &parent, UInt index): index(index),parent(parent) {}



	///Retrieves key value
	/**
	 * @return key value. If element is index, returns empty string
	 */
	std::string_view getKey() const {return keyName;}
	///Retrieves index value
	/**
	 * @return index value. if element is key, result is an undetermined number
	 */
	UInt getIndex() const {return index;}

	///Retrieves parent path
	const Path &getParent() const {return parent;}

	///Determines, whether current element is an index
	bool isIndex() const {return index != (UInt)-1;}
	///Determines, whether current element is a key
	bool isKey() const {return index == (UInt)-1;}

	///returns true, if current element is root elemmenet
	bool isRoot() const {return this == &root;}

	///Returns lenhth of the path
	/**
	 * @return length of the path. Note that function has linear complexity
	 */
	UInt length() const {return isRoot()?0:(parent.length()+1);}

	///Allocates extra memory and copies path into it
	/**
	 * @return smart pointer to whole path.
	 */
	PPath copy() const;


	///Compare two paths,
	/** Paths are compared in order from right to left (from leaves to root)
	 * Ordering can be used to store the path in a map. Index is ordered before
	 * key.
	 * @param other
	 * @retval -1 this path is before other
	 * @retval 0 paths are equal
	 * @retval 1 this path is above other
	 */
	int compare(const Path &other) const;

	bool operator == (const Path &other) const {return compare(other) == 0;}
	bool operator >= (const Path &other) const {return compare(other) >= 0;}
	bool operator <= (const Path &other) const {return compare(other) <= 0;}
	bool operator != (const Path &other) const {return compare(other) != 0;}
	bool operator > (const Path &other) const {return compare(other) > 0;}
	bool operator < (const Path &other) const {return compare(other) < 0;}


	//@{
	/** Build path using operator
	 * @code
	 * Value v;
	 * Value item = v[Path::root/"aaa"/"bbb"/10/2/"ccc"];
	 * @endcode
	 *
	 * Note that you cannot not store result of operation if the Path. You can pass result as
	 * argument ot a function. Following code is invalid
	 *
	 * @code
	 * Path p = Path::root/"aaa"/"bbb";  ///<ERROR! item "aaa" will be destroyed
	 * @endcode
	 *
	 * Following code is valid
	 * @code
	 * PPath p = Path::root/"aaa"/"bbb";  ///Path will be allocated before each part is destroyed
	 * @endcode
	 *
	 *
	 * @param key
	 * @return
	 */
	Path operator/(const std::string_view &key) const {
		return Path(*this,key);
	}
	Path operator/(uintptr_t index) const {
		return Path(*this,index);
	}
	//@}

	///Converts path to Value
	/**
	 * Path is represented as array, where each item corresponds to path's item. First item
	 * is item which follows root immediately
	 *
	 * @return Array containing items of the path
	 */
	Value toValue() const;

	///Converts path to Array
	/** Path is represented as array, where each item corresponds to path's item. First item
	 * is item which follows root immediately
	 *
	 * @param array array where each item will be stored
	 *
	 * @note It is allowed to have Array non-empty, this allows to combine multiple paths, or
	 * pretend a prefix to the path
	 */
	void toArray(Array &array) const;

	~Path() {}

	void operator delete(void *p);
private:

	///handles deletion
	void *operator new(size_t sz, void *p);
	void operator delete (void *ptr, void *p);

	friend class Value;

	///name of key;
	std::string_view keyName;
	///index of value (used as keyname when keyName is not null)
	UInt index;
	///Reference to parent path
	const Path &parent;

	void *operator new(size_t sz);

	template<typename T>
	T *copyRecurse(T * trg, char  *strBuff) const;


};

///Object which is allocated on heap to provide sharing of immutable path
class PathRef: public Path, public RefCntObj {
private:
	PathRef(const Path &parent, const std::string_view &key):Path(parent,key) {}
	PathRef(const Path &parent, UInt index):Path(parent,index) {}
	friend class Path;
	friend class RootPath;
};


///Variable contains immutable path allocated on stack through method Path::copy()
/** You can use the value same way as Path. PPath can be implicitly
 * converted to Path.
 */
class PPath  {
private:
	friend class Path;

	PPath(const PathRef *p);
	PPath(const Path p);
public:
	~PPath();

	///Converts Value (array) to path
	/**
	 * @param v a value containing path (array). First item of the array immediately follows the root
	 * @retval PPath object
	 */
	static PPath fromValue(const Value &v);

	///Converts object to Value
	/**
	 * @copydoc Path::toValue
	 */
	Value toValue() const;

	///combines path and value and creates PPath object
	/**
	 * It can takes a path and appends items from the value from given offset
	 *
	 * @param parent some path used as parrent. It can be Path::root as well
	 * @param v value containing array which is processed from the left to the right
	 * @param offset specifies start position in the array. Default is zero, which means, that whole
	 * array is processed
	 * @return the PPath object as result
	 */
	static PPath fromValue(const Path &parent, const Value &v, uintptr_t offset = 0);

	///Compare two paths,
	/** @copydoc Path::compare
	 */

	int compare(const Path &other) const;

	bool operator == (const Path &other) const {return compare(other) == 0;}
	bool operator >= (const Path &other) const {return compare(other) >= 0;}
	bool operator <= (const Path &other) const {return compare(other) <= 0;}
	bool operator != (const Path &other) const {return compare(other) != 0;}
	bool operator > (const Path &other) const {return compare(other) > 0;}
	bool operator < (const Path &other) const {return compare(other) < 0;}

	///Converts PPath to Path so you can use PPath everywhere Path is expected
	operator const Path &() const;
protected:
	RefCntPtr<const PathRef> ref;


};

}


#endif /* SRC_IMMUJSON_PATH_H_ */
