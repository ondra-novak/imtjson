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
		Object(const StringRef<char> &name, const Value &value);
		~Object();


		Object &set(const StringRef<char> &name, const Value &value);
		Object &set(const Value &value);
		Value operator[](const StringRef<char> &name) const;
		Object &unset(const StringRef<char> &name);


		Object &operator()(const StringRef<char> &name, const Value &value);

		PValue commit() const;

		Object2Object object(const StringRef<char> &name);
		Array2Object array(const StringRef<char> &name);

		void clear();

		bool dirty() const;

		Object &merge(Value object);

	protected:
		Value base;
		typedef std::map<StringRef<char>, PValue> Changes;
		
		Changes changes;

	};


}