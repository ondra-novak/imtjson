#pragma once

#include "object.h"
#include "array.h"

namespace json {

	class Object2Object: public Object {
	public:
		Object2Object(Value oldVal, Object &parent, const StringRef<char> &name)
			:Object(oldVal), parent(parent), name(name) {}
		~Object2Object() noexcept(false) {
			if (dirty()) {
				parent.checkInstance();
				parent.set(name, *this);
			}
		}
	protected:
		Object &parent;
		const StringRef<char> &name;
	};

	class Object2Array : public Object {
	public:
		Object2Array(Value oldVal, Array &parent, std::size_t index)
			:Object(oldVal), parent(parent), index(index) {}
		~Object2Array() noexcept(false) {
			if (dirty()) {
				parent.checkInstance();
				parent.set(index, *this);
			}
		}
	protected:
		Array &parent;
		std::size_t index;
	};

	class Array2Object : public Array {
	public:
		Array2Object(Value oldVal, Object &parent, const StringRef<char> &name)
			:Array(oldVal), parent(parent), name(name) {}
		~Array2Object() noexcept(false) {
			if (dirty()) {
				parent.checkInstance();
				parent.set(name, *this);
			}
		}
	protected:
		Object &parent;
		const StringRef<char> &name;
	};

	class Array2Array : public Array {
	public:
		Array2Array(Value oldVal, Array &parent, std::size_t index)
			:Array(oldVal), parent(parent), index(index) {}
		~Array2Array() noexcept(false) {
			if (dirty()) {
				parent.checkInstance();
				parent.set(index, *this);
			}
		}
	protected:
		Array &parent;
		std::size_t index;
	};
}