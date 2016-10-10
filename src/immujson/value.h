#pragma once

#include <vector>
#include "ivalue.h"

namespace json {

	class Array;
	class Object;

	class Value {
	public:

		Value(ValueType vtype);
		Value(bool value);
		Value(const char *value);
		Value(const std::string &value);
		Value(const StringRef<char> &value);
		Value(signed short x);
		Value(unsigned short x);
		Value(signed int x);
		Value(unsigned int x);
		Value(signed long x);
		Value(unsigned long x);
		Value(signed long long x);
		Value(unsigned long long x);
		Value(double value);
		Value(float value);
		Value(std::nullptr_t);
		Value();
		Value(const IValue *v);
		Value(const PValue &v);
		Value(const Array &value);
		Value(const Object &value);
		Value(const StringRef<Value> &value);
		Value(const std::initializer_list<Value> &data);

		ValueType type() const { return v->type(); }

		std::uintptr_t getUInt() const { return v->getUInt(); }
		std::intptr_t getInt() const { return v->getInt(); }
		double getNumber() const { return v->getNumber(); }
		bool getBool() const { return v->getBool(); }
		StringRef<char> getString() const { return v->getString(); }
		std::size_t size() const { return v->size(); }
		bool empty() const { return v->size() == 0; }
		Value operator[](uintptr_t index) const { return v->itemAtIndex(index); }
		Value operator[](const StringRef<char> &name) const { return v->member(name); }

		template<typename Fn>
		bool forEach(const Fn &fn) {
			return v->enumItems(EnumFn<Fn>(fn));
		}

		template<typename Fn>
		static Value parse(const Fn &source);

		static Value fromString(const StringRef<char> &string);
		static Value fromStream(std::istream &input);
		static Value fromFile(FILE *f);

		template<typename Fn>
		void serialize(const Fn &target) const;

		std::string toString() const;
		void toStream(std::ostream &output) const;
		void toFile(FILE *f) const;

		const PValue &getHandle() const { return v; }

	protected:

		PValue v;

		friend class Object;
		friend class Array;

		std::vector<PValue> prepareValues(const std::initializer_list<Value>& data);
	};

	inline static std::istream &operator >> (std::istream &stream, Value &v) {
		v = Value::fromStream(stream);
		return stream;
	}
	inline static std::ostream &operator << (std::ostream &stream, const Value &v) {
		v.toStream(stream);
		return stream;
	}



}
