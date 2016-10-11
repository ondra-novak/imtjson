#pragma once

#include "value.h"
#include "stackProtection.h"
#include <map>

namespace json {


	class Object2Object;
	class Array2Object;


	class Object : public StackProtected {
	public:
		Object(Value value);
		Object();
		Object(const StringView<char> &name, const Value &value);
		~Object();


		Object &set(const StringView<char> &name, const Value &value);
		Object &set(const Value &value);
		Value operator[](const StringView<char> &name) const;
		Object &unset(const StringView<char> &name);


		Object &operator()(const StringView<char> &name, const Value &value);

		PValue commit() const;

		Object2Object object(const StringView<char> &name);
		Array2Object array(const StringView<char> &name);

		void clear();

		bool dirty() const;

		Object &merge(Value object);

	protected:
		Value base;
		typedef std::map<StringView<char>, PValue> Changes;
		
		Changes changes;

	};


}