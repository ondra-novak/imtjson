#include "abstractValue.h"

namespace json {

	class UndefinedValue : public AbstractValue {
	public:
		UndefinedValue() {
			addRef();
		}
	};

	static UndefinedValue undefinedVal;

	const IValue *AbstractValue::getUndefined() {
		return &undefinedVal;
	}

}