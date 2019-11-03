/*
 * key.h
 *
 *  Created on: 27. 1. 2017
 *      Author: ondra
 */

#ifndef IMTJSON_SOUKEY_H_
#define IMTJSON_SOUKEY_H_

#include "abstractValue.h"
#include "string.h"

namespace json  {



class AbstractProxy : public AbstractValue {
public:
	AbstractProxy(const PValue &value):value(value) {}

	virtual ValueType type() const override { return value->type(); }
	virtual ValueTypeFlags flags() const override { return value->flags() | proxy; }

	virtual UInt getUInt() const override { return value->getUInt(); }
	virtual Int getInt() const override { return value->getInt(); }
	virtual ULongInt getUIntLong() const override { return value->getUIntLong(); }
	virtual LongInt getIntLong() const override { return value->getIntLong(); }
	virtual double getNumber() const override { return value->getNumber(); }
	virtual bool getBool() const override { return value->getBool(); }
	virtual StringView<char> getString() const override { return value->getString(); }
	virtual std::size_t size() const override { return value->size(); }
	virtual const IValue *itemAtIndex(std::size_t index) const override { return value->itemAtIndex(index); }
	virtual const IValue *member(const StringView<char> &name) const override { return value->member(name); }
	virtual bool enumItems(const IEnumFn &fn) const override { return value->enumItems(fn); }
	virtual StringView<char> getMemberName() const override { return value->getMemberName(); }
	virtual const IValue *unproxy() const override { return value->unproxy(); }
	virtual bool equal(const IValue *other) const override {return value->equal(other->unproxy());}

protected:
	PValue value;


};

class ObjectProxy : public AbstractProxy {
public:

	ObjectProxy(const StringView<char> &name, const PValue &value);
	virtual StringView<char> getMemberName() const override { return StringView<char>(key,keysize); }

	void *operator new(std::size_t sz, const StringView<char> &str );
	void operator delete(void *ptr, const StringView<char> &str);
	void operator delete(void *ptr, std::size_t sz);

protected:
	ObjectProxy(ObjectProxy &&) = delete;
	std::size_t keysize;
	char key[256];
};

class ObjectProxyString: public AbstractProxy {
public:
	ObjectProxyString(const String &s, const PValue &value):AbstractProxy(value),key(s) {}

	virtual StringView<char> getMemberName() const override { return key; }
	const String getKey() const {return key;}
protected:
	String key;

};

///Retrieves object which represents the key
/**
 * @param v a value which key's object need to be returned
 * @return The function returns object which represents the key. Return value depends on which function has been used to
 * set the key. By default, the key is set through the StrView object. In this key, function returns undefined. If the
 * String object is used, the function returns the String. However, because the type of the underlying object is not checked
 * (neither for setKey, nor for String itself), you can set anything as key.
 */
Value getKeyObject(const Value &v);




}



#endif /* IMTJSON_SOUKEY_H_ */
