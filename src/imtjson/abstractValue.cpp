#include "abstractValue.h"

namespace json {

	class UndefinedValue : public AbstractValue {
	public:
		UndefinedValue() {
			addRef();
		}
	};


	const IValue *AbstractValue::getUndefined() {
		static UndefinedValue undefinedVal;
		return &undefinedVal;
	}

	int AbstractValue::compare(const IValue *other) const {
		ValueType lt = type();
		ValueType rt = other->type();
		if (lt != rt) {
			return lt<rt?-1:lt>rt?1:0;
		} else {
			switch (lt) {
			default:
				case undefined:
				case null: return 0;
				case boolean: return getBool()==other->getBool()?0:getBool()?1:-1;
				case number:{
					if ((flags() & numberUnsignedInteger) && (other->flags() & numberUnsignedInteger)) {
						std::uintptr_t l = getUInt();
						std::uintptr_t r = other->getUInt();
						return l<r?-1:l>r?1:0;
					}
					if ((flags() & (numberInteger | numberUnsignedInteger)) && (other->flags() & (numberInteger|numberUnsignedInteger))) {
						std::intptr_t l = getInt();
						std::intptr_t r = other->getInt();
						return l<r?-1:l>r?1:0;
					}
					{
						double l = getNumber();
						double r = other->getNumber();
						return l<r?-1:l>r?1:0;
					}
				}break;
				case string:
					return getString().compare(other->getString());
					break;
				case array: {
					std::size_t s1 = size();
					std::size_t s2 = other->size();
					std::size_t sz = std::min(s1,s2);
					for (std::size_t i = 0; i < sz; i++) {
						int z = this->itemAtIndex(i)->compare(other->itemAtIndex(i));
						if (z != 0) return z;
					}
					return s1<s2?-1:s1>s2?1:0;
				}break;
				case object: {
					std::size_t s1 = size();
					std::size_t s2 = other->size();
					std::size_t sz = std::min(s1,s2);
					for (std::size_t i = 0; i < sz; i++) {
						const IValue *l = itemAtIndex(i);
						const IValue *r = other->itemAtIndex(i);
						int zk = l->getMemberName().compare(r->getMemberName());
						if (zk != 0) return zk;
						int zv = l->compare(r);
						if (zv != 0) return zv;
					}
					return s1<s2?-1:s1>s2?1:0;
				}break;
			}
		}

	}

}
