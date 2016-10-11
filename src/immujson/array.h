#pragma once

#include "value.h"
#include "stackProtection.h"

namespace json {

	class Object2Array;
	class Array2Array;


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
		Array &clear();

		Array &set(std::size_t pos, const Value &v);

		Value operator[](std::size_t pos) const;


		std::size_t size() const;
		bool empty() const;

		PValue commit() const;

		Object2Array object(std::size_t pos);
		Array2Array array(std::size_t pos);

		bool dirty() const;


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


}