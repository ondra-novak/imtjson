#pragma once

#include <vector>
#include "valueref.h"
#include "edit.h"

namespace json {


	class Array: public std::vector<Value> {
	public:
		using std::vector<Value>::vector;
		using Super = std::vector<Value>;

		Array() {}
		Array(Value base);

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
		Object2Array appendObject();
		///Allows to add array into the array
		/** The function insert new item into the Array and allows to create array there.
		 * The function then call the function array()
		 *
		 * @return Instance of Object2Array
		 *
		 * @see array()
		 */
		Array2Array appendArray();

		///Returns true, when there are changes to commit
		/**
		 * @retval true array has been changed
		 * @return false array has not been changed
		 */

		static Value fromVector(const std::vector<Value> &v);

		void set(std::size_t idx, Value item);


		bool dirty() const;
		void revert();

		void clear();

		iterator insert( const_iterator pos, const Value& value );
		iterator insert( const_iterator pos, Value&& value );
		iterator insert( const_iterator pos, size_type count, const Value& value );
		template< class InputIt >
		iterator insert( const_iterator pos, InputIt first, InputIt last );
		iterator insert( const_iterator pos, std::initializer_list<Value> ilist );
		template< class... Args >
		iterator emplace( const_iterator pos, Args&&... args );
		iterator erase( const_iterator pos );
		iterator erase( const_iterator first, const_iterator last );
		void push_back( const Value& value );
		void push_back( Value&& value );
		template< class... Args >
		reference emplace_back( Args&&... args );
		void pop_back();
		void resize( size_type count );
		void resize( size_type count, const value_type& value );
		void swap( Array& other ) noexcept;

		Array &append(const Value &arrayValue);

		ValueRef makeRef(std::size_t index);

	protected:
		Value base;
		bool dirty_flag = false;
	};


}

template<class InputIt>
inline json::Array::iterator json::Array::insert(const_iterator pos, InputIt first, InputIt last) {
	dirty_flag = true;
	return Super::insert(pos, first, last);
}

template<class ... Args>
inline json::Array::iterator json::Array::emplace(const_iterator pos, Args &&... args) {
	dirty_flag = true;
	return Super::emplace(pos, std::forward<Args>(args)...);
}

template<class ... Args>
inline json::Array::reference json::Array::emplace_back(Args &&... args) {
	dirty_flag = true;
	return Super::emplace_back(std::forward<Args>(args)...);

}
