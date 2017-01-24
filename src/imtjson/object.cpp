#include <cstring>
#include "object.h"
#include "objectValue.h"
#include "array.h"
#include "allocator.h"
#include <time.h>
#include "operations.h"

#include "path.h"

namespace json {

	class ObjectProxy : public AbstractValue {
	public:

		ObjectProxy(const StringView<char> &name, const PValue &value);
		
		virtual ValueType type() const override { return value->type(); }
		virtual ValueTypeFlags flags() const override { return value->flags() | proxy; }

		virtual std::uintptr_t getUInt() const override { return value->getUInt(); }
		virtual std::intptr_t getInt() const override { return value->getInt(); }
		virtual double getNumber() const override { return value->getNumber(); }
		virtual bool getBool() const override { return value->getBool(); }
		virtual StringView<char> getString() const override { return value->getString(); }
		virtual std::size_t size() const override { return value->size(); }
		virtual const IValue *itemAtIndex(std::size_t index) const override { return value->itemAtIndex(index); }
		virtual const IValue *member(const StringView<char> &name) const override { return value->member(name); }
		virtual bool enumItems(const IEnumFn &fn) const override { return value->enumItems(fn); }
		virtual StringView<char> getMemberName() const override { return StringView<char>(key,keysize); }
		virtual const IValue *unproxy() const override { return value->unproxy(); }
		virtual bool equal(const IValue *other) const override {
				return value->equal(other->unproxy());
		}

		void *operator new(std::size_t sz, const StringView<char> &str );
		void operator delete(void *ptr, const StringView<char> &str);
		void operator delete(void *ptr, std::size_t sz);

	protected:
		ObjectProxy(ObjectProxy &&) = delete;
		PValue value;
		std::size_t keysize;
		char key[256];

	};

	ObjectProxy::ObjectProxy(const StringView<char> &name, const PValue &value):value(value),keysize(name.length) {
		std::memcpy(key,name.data,name.length);
		key[name.length] = 0;
	}


	void *ObjectProxy::operator new(std::size_t sz, const StringView<char> &str ) {
		std::size_t needsz = sz - sizeof(ObjectProxy::key) + str.length+1;
		return Value::allocator->alloc(needsz);
	}
	void ObjectProxy::operator delete(void *ptr, const StringView<char> &) {
		Value::allocator->dealloc(ptr);
	}
	void ObjectProxy::operator delete(void *ptr, std::size_t sz) {
		Value::allocator->dealloc(ptr);
	}


	class ObjectDiff: public ObjectValue {
	public:
		ObjectDiff(AllocInfo &nfo):ObjectValue(nfo) {}
		virtual ValueTypeFlags flags() const override { return objectDiff;}

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
		unordered = ObjectValue::create(ordered->size());;
	}

	Object &Object::operator=(const Object &other) {
		clear();
		ordered = other.commitAsDiffObject();
		unordered = ObjectValue::create(ordered->size());;
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

	Object &Object::operator()(const StringView<char> &name, const Value &value) {
		return set(name, value);		
	}

	static const Value &defaultConflictResolv(const Path &, const Value &, const Value &right) {
		return right;
	}

	

	void Object::optimize() const {

		if (unordered == nullptr || unordered->size() == 0) return;
		static_cast<ObjectValue *>(unordered)->sort();
		if (ordered == nullptr) {
			ordered = ordered->create(unordered->size());
			ordered->isDiff = true;
			*static_cast<Container<PValue> *>(ordered) = *unordered;
		} else {
			std::size_t newSz = unordered->size()+(ordered?ordered->size():0);
			RefCntPtr<ObjectValue> newdiff = ObjectValue::create(newSz);
			newdiff->isDiff = true;

			mergeDiffsImpl(ordered->begin(), ordered->end(),
					unordered->begin(), unordered->end(),
					defaultConflictResolv,
					Path::root,
					[&](const StringView<char> &name, const Value &v) {
							newdiff->push_back(v.setKey(name).getHandle());
					});

			ordered = newdiff;
		}
	unordered->clear();
	}


	PValue Object::commit() const {
		optimize();
		if (base.empty() && ordered== nullptr) {
			return AbstractObjectValue::getEmptyObject();
		}
		else if (ordered == nullptr) {
			return base.v;
		} else {
			std::size_t needsz = 0;


			StringView<PValue> items = getItems(base);

			mergeDiffsImpl(items.begin(), items.end(), ordered->begin(), ordered->end(),
					defaultConflictResolv,Path::root,
					[&] (const StringView<char> &, const Value &v) {
					    if (v.type() != undefined) needsz++;
			});
			RefCntPtr<ObjectValue> merged = ObjectValue::create(needsz);

			mergeDiffsImpl(items.begin(), items.end(), ordered->begin(), ordered->end(),
					defaultConflictResolv,Path::root,
					[&merged](const StringView<char> &n, const Value &v) {
						if (v.type() != undefined) merged->push_back(v.setKey(n).getHandle());
					});
			return PValue::staticCast(merged);
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




void Object::createDiff(const Value oldObject, Value newObject, unsigned int recursive) {

	oldObject.merge(newObject,[&](const Value &oldO, const Value &newO){
		if (!oldO.defined()) {
			set_internal(newO);
			return 0;
		} else if (!newO.defined()) {
			set(oldO.getKey(),undefined);
			return 0;
		} else if (oldO.getKey() < newO.getKey()) {
			set(oldO.getKey(),undefined);
			return -1;
		} else if (oldO.getKey() > newO.getKey()) {
			set_internal(newO);
			return -1;
		} else {
			if (oldO.type() == json::object && newO.type() == json::object && recursive) {
				Object tmp;
				tmp.createDiff(oldO,newO,recursive-1);
				set(oldO.getKey(), tmp.commitAsDiff());
			} else {
				set_internal(newO);
			}
		}
	});
}

Value Object::commitAsDiff() const {
	Value(commitAsDiffObject());
}

Value json::Object::applyDiff(const Value& baseObject, const Value& diffObject) {

	std::size_t needsz = 0;

	StringView<PValue> items = getItems(baseObject);
	StringView<PValue> diff = getItems(baseObject);

	mergeDiffsImpl(items.begin(), items.end(), diff.begin(), diff.end(),
			defaultConflictResolv,Path::root,
			[&needsz](const StrViewA &, const Value &v) {
		if (v.defined()) needsz++;
	});

	RefCntPtr<ObjectValue> res = ObjectValue::create(needsz);

	mergeDiffsImpl(items.begin(), items.end(), diff.begin(), diff.end(),
			defaultConflictResolv,Path::root,
			[&res](const StrViewA &n, const Value &v) {
		if (v.defined()) res->push_back(v.setKey(n).getHandle());
	});

	return PValue::staticCast(res);

}

template<typename It, typename It2, typename Fn>
void Object::mergeDiffsImpl(It lit, It lend, It2 rit, It2 rend, const ConflictResolver &resolver,const Path &path, const Fn &setFn) {
	while (lit != lend && rit != rend) {
		auto &&lv = *lit;
		auto &&rv = *rit;
		StrViewA nlv = lv->getMemberName();
		StrViewA nrv = rv->getMemberName();
		int cmp = nlv.compare(nrv);
		if (cmp < 0) {
			setFn(nlv,lv);++lit;
		} else if (cmp>0) {
			setFn(nrv,rv);++rit;
		} else {
			StringView<char> name = nlv;
			if (lv->type() == ::json::object && rv->type() == ::json::object) {
				if (lv->flags() & objectDiff) {
					if (rv->flags() & objectDiff) {
						setFn(name, mergeDiffsObjs(lv,rv,resolver,path).getHandle());
					} else {
						setFn(name ,applyDiff(rv,lv).getHandle());
					}
				} else if (rv->flags() & objectDiff) {
					setFn(name ,applyDiff(lv,rv));
				} else {
					setFn(name , resolver(Path(path,name), lv, rv).getHandle());
				}

			} else {
				setFn(name , resolver(Path(path,name), lv, rv).getHandle());
			}
			++lit;
			++rit;
		}
	}
	while (lit != lend) {
		auto &&lv = *lit;
		const StringView<char> name = lv->getMemberName();
		setFn(name,lv);
		++lit;
	}
	while (rit != rend) {
		auto &&rv = *rit;
		const StringView<char> name = rv->getMemberName();
		setFn(name, rv);
		++rit;
	}
}


void Object::mergeDiffs(const Object& left, const Object& right, const ConflictResolver& resolver) {
	mergeDiffs(left,right,resolver,Path::root);
}

void Object::mergeDiffs(const Object& left, const Object& right, const ConflictResolver& resolver, const Path &path) {

	StringView<PValue> ileft = getItems(left);
	StringView<PValue> iright = getItems(right);


	mergeDiffsImpl(ileft.begin(),ileft.end(),iright.begin(),iright.end(),resolver,path,
			[&](const StringView<char> &name, const Value &v) {
		this->set(name,v);
	});
}

Value Object::mergeDiffsObjs(const Value& lv, const Value& rv,
		const ConflictResolver& resolver, const Path& path) {

	StringView<PValue> ilv = getItems(lv);
	StringView<PValue> irv= getItems(rv);


	std::size_t needsz = 0;
	mergeDiffsImpl(ilv.begin(),ilv.end(),irv.begin(),irv.end(),
			defaultConflictResolv,Path::root,[&](const StringView<char> &, const Value &){
		needsz++;
	});
	RefCntPtr<ObjectDiff> obj = ObjectDiff::create(needsz);
	mergeDiffsImpl(ilv.begin(),ilv.end(),irv.begin(),irv.end(),resolver,Path(path,lv.getKey()),
			[&](const StringView<char> &name, const Value &v){
		obj->push_back(v.setKey(name).getHandle());
	});
	return PValue::staticCast(obj);
}

Value Value::setKey(const StringView<char> &key) const {
	if (getKey() == key) return *this;
	if (key.empty()) return Value(v->unproxy());
	return Value(new(key) ObjectProxy(key,v->unproxy()));
}

StringView<PValue> Object::getItems(const Value& v) {
	const IValue *pv = v.getHandle();
	const ObjectValue *ov = dynamic_cast<const ObjectValue *>(pv->unproxy());
	if (ov) return ov->getItems(); else return StringView<PValue>();
}

std::size_t Object::size() const {
	return base.size() + (ordered == nullptr?0:ordered->size())+unordered->size();
}

const ObjectValue *Object::commitAsDiffObject() const {
	if (ordered == nullptr) return static_cast<const ObjectValue *>(AbstractObjectValue::getEmptyObject());
	optimize();
	return ordered;
}

Object::Object(Object &&other)
	:base(std::move(other.base))
	,ordered(std::move(other.ordered))
	,unordered(std::move(other.unordered)) {

}


}

