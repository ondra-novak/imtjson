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

}
