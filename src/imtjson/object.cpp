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

	template<typename Fn>
	static RefCntPtr<ObjectValue> applyDiffImpl(const Value &baseObject, const Value &diffObject, Fn mergeFn);

	const Value &Object::defaultMerge(const Value &, const Value &v) {
		return v;
	}

	Value Object::recursiveMerge(const Value &b, const Value &d) {
		if (b.type() == json::object && d.type() == json::object)
			return applyDiff(b,d,recursiveMerge);
		else
			return d;
	}

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

	Object::Object(const StringView<char>& name, const Value & value): base(AbstractObjectValue::getEmptyObject())
	{
		set(name, value);
	}

	Object::~Object()
	{
		//force delection here
		ordered = nullptr;
		unordered = nullptr;
	}

	Object::Object(const Object &other):base(other.base) {
		ordered = other.commitAsDiffObject();
		unordered = ordered != nullptr?ObjectValue::create(ordered->size()):nullptr;
	}

	Object &Object::operator=(const Object &other) {
		clear();
		base = other.base;
		ordered = other.commitAsDiffObject();
		unordered = ordered != nullptr?ObjectValue::create(ordered->size()):nullptr;
		return *this;
	}

	Object &Object::operator=(Object &&other) {
		clear();
		base = std::move(other.base);
		ordered = std::move(other.ordered);
		unordered = std::move(other.unordered);
		return *this;
	}


void Object::set_internal(const Value& v) {
	StringView<char> curName = v.getKey();
	lastIterSnapshot = Value();

	if (unordered == nullptr) {
		unordered = ObjectValue::create(8);
	}

	bool ok = unordered->push_back(v.getHandle());
	if (!ok) {
		RefCntPtr<ObjectValue> newcont (ObjectValue::create(unordered->size()*2));
		optimize();
		unordered = newcont;
		unordered->push_back(v.getHandle());
	}
}

	Object & Object::set(const StringView<char>& name, const Value & value)
	{
		PValue v = value.getHandle();
		if (v->flags() & proxy ) {
			StringView<char> curName = v->getMemberName();
			if (curName == name) {
				set_internal(v);
				return *this;
			} else {
				v = v->unproxy();
			}

		}
		v = new(name) ObjectProxy(name, v);
		set_internal(v);
		return *this;
	}

	Object & Object::set(const Value & value)
	{
		set_internal(value.getHandle());
		return *this;
	}

	Object & Object::unset(const StringView<char>& name)
	{
		return set(name, AbstractValue::getUndefined());
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

			if (ordered != nullptr) {
				const IValue *f = ordered->findSorted(name);
				if (f) return f;
			}
		}

		return base[name];
	}

	ValueRef Object::makeRef(const StringView<char> &name) {
		return ValueRef(*this, name);
	}

	Object &Object::operator()(const StringView<char> &name, const Value &value) {
		return set(name, value);		
	}

	static const Value &defaultConflictResolv(const Path &, const Value &, const Value &right) {
		return right;
	}

	

	void Object::optimize() const {

		if (unordered == nullptr || unordered->size() == 0) return;
		unordered->sort();
		if (ordered == nullptr) {
			ordered = ordered->create(unordered->size());
			*ordered = *unordered;
			ordered->isDiff = true;
		} else {
			//wild casting here, because applyDiffImpl need Value as argumeny
			//however, it returns new ObjectValue
			ordered = applyDiffImpl(
					Value(PValue::staticCast(ordered)),
					Value(PValue::staticCast(unordered)),
					defaultMerge);

		}
		unordered->clear();
	}


	PValue Object::commit() const {
		optimize();
		if (ordered == nullptr) {
			return base.v;
		} else {
			return applyDiff(base,commitAsDiff()).getHandle();
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
	Object & Object::merge(Value object)
	{
		object.forEach([&](Value v) {
			set(v); return true;
		});
		return *this;
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

	auto merge = [](const Value &old, const Value &nw) {
		if (nw.type() & valueDiff) return old.replace(Path::root,nw);
		else return nw;
	};

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
	ObjectValue *obj = commitAsDiffObject();
	if (obj == nullptr) return json::object;
	else return Value(static_cast<const IValue *>(obj));
}

Value Object::applyDiff(const Value& baseObject, const Value& diffObject) {
	if (diffObject.type() != json::object) return baseObject;
	if (baseObject.type() != json::object) return applyDiff(json::object, diffObject);

	return PValue::staticCast(applyDiffImpl(baseObject,diffObject,defaultMerge));
}

Value Object::applyDiff(const Value &baseObject, const Value &diffObject,
		const std::function<Value(const Value &base, const Value &diff)> &mergeFn) {

	if (diffObject.type() != json::object) return baseObject;
	if (baseObject.type() != json::object) return applyDiff(json::object, diffObject);

	return PValue::staticCast(applyDiffImpl(baseObject,diffObject,mergeFn));

}

template<typename Fn>
static RefCntPtr<ObjectValue> applyDiffImpl(const Value &baseObject, const Value &diffObject, Fn mergeFn) {

	RefCntPtr<ObjectValue> newobj = ObjectValue::create(baseObject.size()+diffObject.size());
	bool createDiff = (baseObject.flags() & valueDiff) != 0;
	newobj->isDiff = createDiff;

	auto bp = baseObject.begin();
	auto dp = diffObject.begin();
	auto be = baseObject.end();
	auto de = diffObject.end();

	auto merge = [&mergeFn](const Value &old, const Value &nw) {
		if (nw.type() & valueDiff) return Value(old.getKey(),old.replace(Path::root,nw));
		else return Value(old.getKey(),mergeFn(old,nw));
	};

	auto merge2 = [](const Value &nw) {
		if (nw.type() & valueDiff) return Value(nw.getKey(),Value(json::object).replace(Path::root,nw));
		else return nw;
	};

	while (bp != be && dp != de) {
		int cmp = (*bp).getKey().compare((*dp).getKey());
		if (cmp < 0) {
			newobj->push_back((*bp).getHandle());
			++bp;
		} else if (cmp > 0){
			Value v = *dp;
			if (v.defined()) {
				newobj->push_back(merge2(v).getHandle());
			} else if (createDiff) {
				newobj->push_back(v.getHandle());
			}
			++dp;
		} else {
			Value v = *dp;
			if (v.defined()) {
				newobj->push_back(merge(*bp,v).getHandle());
			} else if (createDiff) {
				newobj->push_back(v.getHandle());
			}
			++dp;
			++bp;
		}
	}
	while (bp != be) {
		newobj->push_back((*bp).getHandle());
		++bp;
	}
	while (dp != de) {
		if ((*dp).defined()) {
			newobj->push_back(merge2(*dp).getHandle());
		} else if (createDiff) {
			newobj->push_back((*dp).getHandle());
		}
		++dp;
	}

	return newobj;
}

StringView<PValue> Object::getItems(const Value& v) {
	const IValue *pv = v.getHandle();
	const ObjectValue *ov = dynamic_cast<const ObjectValue *>(pv->unproxy());
	if (ov) return ov->getItems(); else return StringView<PValue>();
}

std::size_t Object::size() const {
	return base.size() + (ordered == nullptr?0:ordered->size())+unordered->size();
}

ObjectValue *Object::commitAsDiffObject() const {
	optimize();
	return ordered;
}

Object::Object(Object &&other)
	:base(std::move(other.base))
	,ordered(std::move(other.ordered))
	,unordered(std::move(other.unordered)) {

}



Value::Value(const StrViewA key, const Value &value) :v(bindKeyToPValue(key, value.getHandle())) {}


}

