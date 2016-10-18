#pragma once

#include "value.h"
#include "stackProtection.h"

namespace json {

	class Object2Array;
	class Array2Array;
	class ArrayIterator;


	class Array: public StackProtected {
	public:
		Array(Value value);
		Array();
		~Array();

		Array &push_back(const Value &v);
		Array &add(const Value &v);
		Array &addSet(const StringView<Value> &v);

		Array &insert(std::size_t pos, const Value &v);
		Array &insertSet(std::size_t pos, const StringView<Value> &v);

		Array &erase(std::size_t pos);
		Array &eraseSet(std::size_t pos, std::size_t length);
		
		Array &trunc(std::size_t pos);
		///Removes all items from the array
		Array &clear();
		///Removes all changes in the array (revert back to base value)
		Array &revert();

		Array &set(std::size_t pos, const Value &v);

		Value operator[](std::size_t pos) const;


		std::size_t size() const;
		bool empty() const;

		PValue commit() const;

		Object2Array object(std::size_t pos);
		Array2Array array(std::size_t pos);

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


	protected:
		Value base;

		struct Changes: public std::vector<PValue>  {			
			size_t offset;

			Changes(const Value &base) :offset(base.size()) {}
			Changes() :offset(0) {}
		};

		Changes changes;

		void extendChanges(size_t pos);

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
