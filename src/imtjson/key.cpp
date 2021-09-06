/*
 * key.cpp
 *
 *  Created on: 27. 1. 2017
 *      Author: ondra
 */


#include <cstring>

#include "value.h"
#include "key.h"
#include "allocator.h"

namespace json {

ObjectProxy::ObjectProxy(const std::string_view &name, const PValue &value):AbstractProxy(value),keysize(name.size()) {
	std::memcpy(key,name.data(),name.size());
	key[name.size()] = 0;
}


void *ObjectProxy::operator new(std::size_t sz, const std::string_view &str ) {
	std::size_t needsz = sz - sizeof(ObjectProxy::key) + str.size()+1;
	return Value::allocator->alloc(needsz);
}
void ObjectProxy::operator delete(void *ptr, const std::string_view &) {
	Value::allocator->dealloc(ptr);
}
void ObjectProxy::operator delete(void *ptr, std::size_t ) {
	Value::allocator->dealloc(ptr);
}


Value getKeyObject(const Value &v) {

	PValue pv = v.getHandle();
	if (pv->flags() & proxy) {
		RefCntPtr<const ObjectProxyString> str = RefCntPtr<const ObjectProxyString>::dynamicCast(pv);
		if (str != nullptr) {
			return str->getKey();
		}
	}
	return undefined;
}

}
