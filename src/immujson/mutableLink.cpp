/*
 * mutableLink.cpp
 *
 *  Created on: 10. 10. 2016
 *      Author: ondra
 */

#include "mutableLink.h"

#include "basicValues.h"
#include "ivalue.h"
namespace json {


class MutableLink::LinkObject: public IValue {
public:

	LinkObject(const PValue &target):linked(target) {}
	void setLink(const PValue &target) {linked = target;}

	virtual ValueType type() const {
		PValue t = linked;
		return t->type();
	}
	virtual ValueTypeFlags flags() const {
		PValue t = linked;
		return t->flags() |  proxy;
	}
	virtual std::uintptr_t getUInt() const {
		PValue t = linked;
		return t->getUInt();
	}
	virtual std::intptr_t getInt() const {
		PValue t = linked;
		return t->getUInt();
	}
	virtual double getNumber() const {
		PValue t = linked;
		return t->getNumber();
	}
	virtual bool getBool() const {
		PValue t = linked;
		return t->getBool();
	}
	virtual StringRef<char> getString() const {
		PValue t = linked;
		return t->getString();
	}
	virtual std::size_t size() const {
		PValue t = linked;
		return t->size();
	}
	virtual const IValue *itemAtIndex(std::size_t index) const {
		PValue t = linked;
		return t->itemAtIndex(index);
	}
	virtual const IValue *member(const StringRef<char> &name) const {
		PValue t = linked;
		return t->member(name);
	}
	virtual bool enumItems(const IEnumFn &fn) const {
		PValue t = linked;
		return t->enumItems(fn);
	}
	virtual StringRef<char> getMemberName() const {
		PValue t = linked;
		return t->getMemberName();

	}
	virtual const IValue *unproxy() const {
		PValue t = linked;
		return t->unproxy();
	}



protected:
	PValue linked;
};



Value MutableLink::getLink() const {
	return PValue::staticCast(link);
}

void MutableLink::setTarget(const Value& target) {
	link->setLink(target.getHandle());
}

MutableLink::MutableLink() {
	link = new LinkObject(NullValue::getNull());
}
MutableLink::MutableLink(const Value &target) {
	link = new LinkObject(target.getHandle());
}

MutableLink::~MutableLink() {
	setTarget(undefined);
}

}
