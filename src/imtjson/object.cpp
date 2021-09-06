#include <cstring>
#include "object.h"
#include "objectValue.h"
#include "array.h"
#include "allocator.h"
#include <time.h>
#include "operations.h"
#include "key.h"
#include "path.h"

namespace json {


	PValue bindKeyToPValue(const StrViewA &key, const PValue &value) {
		if (key.empty()) {
			return value->unproxy();
		}else if (key == value->getMemberName()) {
			return value;
		}else {
			return new(key) ObjectProxy(key, value->unproxy());
		}
	}


	class ObjectDiff: public ObjectValue {
	public:
		ObjectDiff(AllocInfo &nfo):ObjectValue(nfo) {}
		virtual ValueTypeFlags flags() const override { return valueDiff;}

		static RefCntPtr<ObjectDiff> create(std::size_t capacity) {
			AllocInfo nfo(capacity);
			return new(nfo) ObjectDiff(nfo);

		}
	};

	Object::Object(Value value):base(value)
	{
	}

	Object::Object(): base(AbstractObjectValue::getEmptyObject())
	{
	}


	Object::~Object()
	{
	}

	Object::Object(const Object &other):base(other.base) {
		other.optimize();
		ordered = other.ordered;
		unordered = nullptr;
	}

	Object &Object::operator=(const Object &other) {
		if (this == &other) return *this;
		clear();
		base = other.base;
		other.optimize();
		ordered = other.ordered;
		unordered = nullptr;
		return *this;
	}

	Object &Object::operator=(Object &&other) {
		if (this == &other) return *this;
		clear();
		base = std::move(other.base);
		ordered = std::move(other.ordered);
		unordered = std::move(other.unordered);
		return *this;
	}


void Object::set_internal(const Value& v) {
//	StringView<char> curName = v.getKey();
	lastIterSnapshot = Value();

	if (unordered == nullptr) {
		unordered = ObjectValue::create(ordered?ordered->size():8);
	}

	bool ok = unordered->push_back(v.getHandle());
	if (!ok) {
		RefCntPtr<ObjectValue> newcont (ObjectValue::create(unordered->size()*2));
		optimize();
		unordered = newcont;
		unordered->push_back(v.getHandle());
	}
}

	void Object::set(const StringView<char>& name, const Value & value)
	{
		PValue v = value.getHandle();
		if (v->flags() & proxy ) {
			StringView<char> curName = v->getMemberName();
			if (curName == name) {
				set_internal(v);
			} else {
				v = v->unproxy();
			}

		}
		v = new(name) ObjectProxy(name, v);
		set_internal(v);

	}

	void Object::set(const Value & value)
	{
		set_internal(value.getHandle());
	}

	void Object::unset(const StringView<char>& name)
	{
		set(name, AbstractValue::getUndefined());
	}

	const Value Object::operator[](const StringView<char> &name) const {

		if (unordered != nullptr) {
			std::size_t usz = unordered->size();
			if (usz>16 && usz > ordered->size()/2) {
				optimize();
				usz = 0;
			}
			for (std::size_t i = usz; i>0; i--) {
				const PValue &z = (*unordered)[i-1];
				if (z->getMemberName() == name) return z;
			}
		}
		if (ordered != nullptr) {
			const IValue *f = ordered->findSorted(name);
			if (f) return f;
		}

		return base[name];
	}

	ValueRef Object::makeRef(const StringView<char> &name) {
		return ValueRef(*this, name);
	}

	
	static RefCntPtr<ObjectValue> mergeObjectsDiff(const ObjectValue &back, const ObjectValue &front) {
		RefCntPtr<ObjectValue> out = ObjectValue::create(back.size()+front.size());
		auto bi = back.begin();
		auto fi = front.begin();
		auto be = back.end();
		auto fe = front.end();
		while (bi != be && fi != fe) {
			auto kf = (*fi)->getMemberName();
			auto kb = (*bi)->getMemberName();
			if (kf < kb) {
				out->push_back(*fi);
				++fi;
			} else if (kf > kb) {
				out->push_back(*bi);
				++bi;
			} else {
				out->push_back(*fi);
				++fi;
				++bi;
			}
		}
		while (bi != be) {
			out->push_back(*bi);
			++bi;
		}
		while (fi != fe) {
			out->push_back(*fi);
			++fi;
		}
		out->isDiff = true;
		return out;
	}

	static RefCntPtr<ObjectValue> mergeObjects(const ObjectValue &back, const ObjectValue &front) {
		RefCntPtr<ObjectValue> out = ObjectValue::create(back.size()+front.size());
		auto bi = back.begin();
		auto fi = front.begin();
		auto be = back.end();
		auto fe = front.end();
		while (bi != be && fi != fe) {
			auto kf = (*fi)->getMemberName();
			auto kb = (*bi)->getMemberName();
			if (kf < kb) {
				if ((*fi)->type() != json::undefined) out->push_back(*fi);
				++fi;
			} else if (kf > kb) {
				if ((*bi)->type() != json::undefined) out->push_back(*bi);
				++bi;
			} else {
				if ((*fi)->type() != json::undefined) out->push_back(*fi);
				++fi;
				++bi;
			}
		}
		while (bi != be) {
			if ((*bi)->type() != json::undefined) out->push_back(*bi);
			++bi;
		}
		while (fi != fe) {
			if ((*fi)->type() != json::undefined) out->push_back(*fi);
			++fi;
		}
		out->isDiff = false;
		return out;
	}


	void Object::optimize() const {

		if (unordered == nullptr || unordered->size() == 0) return;
		unordered->sort();
		if (ordered == nullptr) {
			ordered = unordered;
			ordered->isDiff = true;
		} else {
			ordered = mergeObjectsDiff(*ordered, *unordered);
		}
		unordered = nullptr;
	}


	PValue Object::commit() const {
		optimize();
		if (ordered == nullptr) {
			return base.v;
		} else {
			const ObjectValue *oval = dynamic_cast<const ObjectValue *>(static_cast<const IValue *>(base.getHandle()->unproxy()));
			RefCntPtr<ObjectValue> res;
			if (oval == nullptr) {
				res = mergeObjects(ObjectValue(),*ordered);
			} else {
				res = mergeObjects(*oval,*ordered);
			}
			return PValue(res);
		}
	}

	Object2Object Object::object(const StringView<char>& name)
	{
		return Object2Object((*this)[name],*this,name);
	}
	Array2Object Object::array(const StringView<char>& name)
	{
		return Array2Object((*this)[name], *this, name);
	}
	void Object::clear()
	{
		base = Value(AbstractObjectValue::getEmptyObject());
		revert();
	}
	bool Object::dirty() const
	{
		return (ordered != nullptr && !ordered->empty()) || (unordered != nullptr && !unordered->empty());
	}

	void Object::revert() {
		ordered = nullptr;
		unordered = nullptr;
	}

	ValueIterator Object::begin() const {
		if (!lastIterSnapshot.defined()) lastIterSnapshot = commit();
		return lastIterSnapshot.begin();
	}

	ValueIterator Object::end() const {
		return lastIterSnapshot.end();
	}




bool Object::createDiff(const Value oldObject, Value newObject, unsigned int recursive) {
	if (newObject.type() != json::object || oldObject.type() != json::object) return false;

	auto bp = oldObject.begin();
	auto dp = newObject.begin();
	auto be = oldObject.end();
	auto de = newObject.end();

/*	auto merge = [](const Value &old, const Value &nw) {
		if (nw.type() & valueDiff) return old.replace(Path::root,nw);
		else return nw;
	};*/

	while (bp != be && dp != de) {
		int cmp = (*bp).getKey().compare((*dp).getKey());
		if (cmp < 0) {
			set(Value((*bp).getKey(),json::undefined));
			++bp;
		} else if (cmp > 0){
			set(*dp);
			++dp;
		} else {
			if (recursive > 0) {
				Object obj;
				if (obj.createDiff(*bp,*dp,recursive-1)) {
					set(obj.commitAsDiff());
				} else {
					if (*dp != *bp) set(*dp);
				}
			} else {
				if (*dp != *bp) set(*dp);
			}
			++dp;
			++bp;

		}
	}
	while (bp != be) {
		set(Value((*bp).getKey(),json::undefined));
		++bp;
	}
	while (dp != de) {
		set(*dp);
		++dp;
	}
	return true;

}

Value Object::commitAsDiff() const {
	optimize();
	if (ordered == nullptr) return json::object;
	else return Value(PValue(ordered));
}



StringView<PValue> Object::getItems(const Value& v) {
	const IValue *pv = v.getHandle();
	const ObjectValue *ov = dynamic_cast<const ObjectValue *>(pv->unproxy());
	if (ov) return ov->getItems(); else return StringView<PValue>();
}

std::size_t Object::size() const {
	return base.size() + (ordered == nullptr?0:ordered->size())+(unordered == nullptr?0:unordered->size());
}


Object::Object(Object &&other)
	:base(std::move(other.base))
	,ordered(std::move(other.ordered))
	,unordered(std::move(other.unordered)) {

}



Value::Value(const StrViewA key, const Value &value) :v(bindKeyToPValue(key, value.getHandle())) {}

Object::Object(const std::initializer_list<std::pair<StrViewA, Value> > &fields) {
	auto obj = ObjectValue::create(fields.size());
	for (const auto &x: fields) {
		if (x.second.defined()) {
			obj->push_back(bindKeyToPValue(x.first, x.second.getHandle()));
		}
	}
	obj->sort();
	base = PValue(obj);

}


}

