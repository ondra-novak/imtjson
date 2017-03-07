#pragma once

#include "valueref.h"
#include "stackProtection.h"
#include "edit.h"

namespace json {

	class ArrayIterator;


	class Array: public StackProtected {
	public:
		Array(Value value);
		Array(const Array &other);
		Array(Array &&other);
		Array();
		~Array();

		void setBaseObject(Value object) {
			base = object;
			if (changes.empty()) changes.offset = base.size();
			else changes.offset = std::min(changes.offset, base.size());
		}


		Array &operator=(const Array &other);
		Array &operator=(Array &&other);

		///Preallocates memory to hold up to specified items
		/** Function reservers extra space to allow add new items up to specified count
		 *
		 * @param items Expected final count of items. If more items are added, extra allocation
		 * may happen. If less items are added, extra space may be wasted. However, the function
		 * commit() always reallocates the space for the final value
		 */
		Array &reserve(std::size_t items);

		///Push back one item (alias to add())
		/**
		 * @param v item to add to the array
		 * @return reference to this
		 */
		Array &push_back(const Value &v);
		///Add (append) one item
		/**
		 * @param v item to add to the array
		 * @return reference to this
		 */
		Array &add(const Value &v);
		///Add (append) set of items
		/**
		 * @param v set of items to add (append)
		 * @return reference to this
		 */
		Array &addSet(const StringView<Value> &v);

		///Add (append) set of items
		/**
		 * @param v set of items to add (append)
		 * @return reference to this
		 */
		Array &addSet(const std::initializer_list<Value> &v);

		///Add (appned) set of items
		/**
		 * @param v conteiner (object or array) which's items are added
		 * @return reference to this
		 */
		Array &addSet(const Value &v);

		///Inserts an item at the position
		/**
		 * @param pos position. Must be in the range 0 - size()
		 * @param v new item
		 * @return reference to this
		 */
		Array &insert(std::size_t pos, const Value &v);
		///Inserts a set of items at the position
		/**
		 * @param pos position. Must be in the range 0 - size()
		 * @param v set of items
		 * @return reference to this
		 */
		Array &insertSet(std::size_t pos, const StringView<Value> &v);
		///Inserts a set of items at the position
		/**
		 * @param pos position. Must be in the range 0 - size()
		 * @param v set of items
		 * @return reference to this
		 */
		Array &insertSet(std::size_t pos, const std::initializer_list<Value> &v);
		///Inserts a set of items at the position
		/**
		 * @param pos position. Must be in the range 0 - size()
		 * @param v set of items
		 * @return reference to this
		 */
		Array &insertSet(std::size_t pos, const Value &v);

		///Erases one item at the position
		/**
		 * @param pos position of the item
		 * @return reference to this
		 */
		Array &erase(std::size_t pos);
		///Erases set of items
		/**
		 * @param pos position of the first item
		 * @param length count of items
		 * @return reference to this
		 */
		Array &eraseSet(std::size_t pos, std::size_t length);
		
		///Truncates array up to specified length removing all items beyond
		/**
		 * @param length new length
		 * @return reference to this
		 *
		 * @note function is the fastest if applied to existing array without changes made yet, because
		 * it only modifies a single integer variable
		 */
		Array &trunc(std::size_t length);

		Array &setSize(std::size_t length, Value fillVal);

		///Removes all items from the array
		/**
		 * @return reference to this
		 */
		Array &clear();
		///Removes all changes in the array (revert back to base value)
		Array &revert();
		///replace item at position
		/***
		 * @param pos position
		 * @param v new item
		 * @return reference to this
		 */
		Array &set(std::size_t pos, const Value &v);

		///Retrieves value at position
		/**
		 * @param pos position to retrieve
		 * @return value at given postion.
		 */
		Value operator[](std::size_t pos) const;

		///Retrieves value at position
		/**
		 * @param pos position to retrieve
		 * @return value at given postion.
		 */
		ValueRef makeRef(std::size_t pos);

		///Retrieves count of items in the array
		std::size_t size() const;
		///Returns true, when array is empty
		bool empty() const;
		///Copies all changes to new Value
		/** You don't need to call this function directly, because the constructor of the class Value
		 * is perform this anytime the Array is converted to a Value
		 * @return PValue is smart pointer to IValue which can be stored in the class Value.
		 */
		PValue commit() const;

		///Allows to edit an object at given position
		/** Function returns Object2Array which can be used to edit object at given position. The destructor
		 * of this object automatically replaces the item at specified position. You need
		 * to assure, that specified item is still exist. You should also avoid to inserting items
		 * before specified position until the changes are committed
		 *
		 * @param pos position to edit
		 * @return instance of Object2Array which allows to access the Object at given position
		 *
		 * @note The instance of Object2Array refers to parent array by a reference. If you
		 * chaining the instances you have to assure, that none of these instances will be destroyed
		 * before the final instance is committed. The imtjson library has some mechanism to detect such
		 * situation and breaking the mentioned rule can cause an exception.
		 */
		Object2Array object(std::size_t pos);
		///Allows to edit an array at given position
		/** Function returns Array2Array which can be used to edit object at given position. The destructor
		 * of this object automatically replaces the item at specified position. You need
		 * to assure, that specified item is still exist. You should also avoid to inserting items
		 * before specified position until the changes are committed.
		 *
		 * @param pos position to edit
		 * @return instance of Array2Array which allows to access the Object at given position
		 *
		 * @note The instance of Array2Array refers to parent array by a reference. If you
		 * chaining the instances you have to assure, that none of these instances will be destroyed
		 * before the final instance is committed. The imtjson library has some mechanism to detect such
		 * situation and breaking the mentioned rule can cause an exception.
		 */
		Array2Array array(std::size_t pos);
		///Allows to add object into the array
		/** The function insert new item into the Array and allows to create object there.
		 * The function then call the function object()
		 *
		 * @return Instance of Object2Array
		 *
		 * @see object()
		 */
		Object2Array addObject();
		///Allows to add array into the array
		/** The function insert new item into the Array and allows to create array there.
		 * The function then call the function array()
		 *
		 * @return Instance of Object2Array
		 *
		 * @see array()
		 */
		Array2Array addArray();

		///Returns true, when there are changes to commit
		/**
		 * @retval true array has been changed
		 * @return false array has not been changed
		 */
		bool dirty() const;

		///Creates iterator which points at first item
		/**
		 * @note you should not change the content of the object during iteration.
		 */
		ArrayIterator begin() const;
		///Creates iterator which points at the end
		/**
		 * @note you should not change the content of the object during iteration.
		 */
		ArrayIterator end() const;



		///Direct access to the items
		/** Function retrieves iterable view of items if the source value is Object
		 *
		 * @note Function only supports build-in array type.
		 *
		 * @return View to items. Items are stored as PValue-s, so you still need
		 * to convert them to Value-s.
		 *
		 * */
		static StringView<PValue> getItems(const Value &v);

		Array &reverse();


		Array &slice(std::intptr_t start);
		Array &slice(std::intptr_t start, std::intptr_t end);

		template<typename Fn>
		Array map(Fn mapFn) const;

		template<typename T, typename ReduceFn>
		T reduce(const ReduceFn &reduceFn, T init) const ;

		template<typename Cmp>
		Array sort(const Cmp &cmp) const;

		template<typename Cmp>
		Array split(const Cmp &cmp) const;

		template<typename Cmp,typename T, typename ReduceFn>
		Array group(const Cmp &cmp,const ReduceFn &reduceFn, T init) const;


	protected:
		Value base;

		struct Changes: public std::vector<PValue>  {			
			size_t offset;

			Changes(const Value &base) :offset(base.size()) {}
			Changes() :offset(0) {}
			Changes(const Changes &base);
			Changes(Changes &&base);
			Changes &operator=(const Changes &base);
			Changes &operator=(Changes &&base);
		};

		Changes changes;

		void extendChanges(size_t pos);

		template<typename Src, typename Cmp>
		friend Array genSort(const Cmp &cmp, const Src &src, std::size_t expectedSize) ;

	};


	class ArrayIterator {
	public:
		const Array *v;
		std::uintptr_t index;

		ArrayIterator(const Array *v,std::uintptr_t index):v(v),index(index) {}
		Value operator *() const {return (*v)[index];}
		ArrayIterator &operator++() {++index;return *this;}
		ArrayIterator operator++(int) {++index;return ArrayIterator(v,index-1);}
		bool operator==(const ArrayIterator &other) const {
			return index == other.index && v == other.v;
		}
		bool operator!=(const ArrayIterator &other) const {
			return !operator==(other);
		}
	};

}
