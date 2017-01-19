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
	}

void Object::set_internal(const Value& v) {
	StringView<char> curName = v.getKey();
	lastIterSnapshot = Value();
	if (changes.empty()) {
		changes.reserve(16);
		changes.push_back(v);
		orderedPart = 1;
		return;
	}
	StringView<char> prevName = changes[orderedPart-1].getKey();
	if (orderedPart == changes.size()) {
		if (prevName == curName) {
			changes[orderedPart-1] = v;
			return;
		}
		if (prevName < curName) {
			orderedPart++;

		}
	}

	changes.push_back(v);
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

	Value Object::operator[](const StringView<char> &name) const {

		//determine whether optimize is needed
		if (changes.size()>16 && changes.size() - orderedPart > orderedPart / 2) {
			optimize();
		}

		//search for the item in unordered part
		PValue f;
		for (std::size_t i = changes.size(); i > orderedPart; i--) {
			const Value &z = changes[i-1];
			if (z.getKey() == name) return z;
		}
		//search for the item in ordered part
		std::size_t l = 0;
		std::size_t h = orderedPart;
		while (l < h) {
			std::size_t m = (l+h)/2;
			const Value &z = changes[m];
			StringView<char> n = z.getKey();
			if (name < n) {
				h = m;
			} else if (name > n) {
				l = m+1;
			} else {
				return z;
			}
		}

		//search for the item in original object
		return base[name];
	}

	Object &Object::operator()(const StringView<char> &name, const Value &value) {
		return set(name, value);		
	}

	static const Value &defaultConflictResolv(const Path &, const Value &, const Value &right) {
		return right;
	}

	

	void Object::optimize() const {

		if (orderedPart < changes.size()) {
			//perform ordering of unordered area
			std::sort(changes.begin()+orderedPart, changes.end(), [](const Value &l, const Value &r) {
				return l.getKey() < r.getKey();
			});

			//remove duplicated items
			std::size_t wrpos = orderedPart+1;
			for (std::size_t i = orderedPart+1, cnt = changes.size(); i < cnt; ++i) {
				if (changes[wrpos-1].getKey() != changes[i].getKey()) {
					changes[wrpos] = changes[i];
					wrpos++;
				} else {
					changes[wrpos-1] = changes[i];
				}
			}
			//adjutst size if necesery
			changes.resize(wrpos);

			//we will store ordered changes here
			Changes newChanges;
			//reserve space
			newChanges.reserve(wrpos);

			//we have two ordered lists inside of changes
			//we will merge these two lists
			//when conflict detected, right item is used
			//result is stored to the newChanges
			//it will store also undefined, because result is still diff
			mergeDiffsImpl(changes.begin(), changes.begin()+orderedPart, changes.begin()+orderedPart, changes.end(),
					defaultConflictResolv,Path::root,
					[&newChanges](const StringView<char> &, const Value &v) {
						newChanges.push_back(v);
					});
			//all done - now swap containers
			std::swap(changes,newChanges);
			//update ordered part (everything is ordered)
			orderedPart = wrpos;
		}
	}


	PValue Object::commit() const {
		if (base.empty() && changes.empty()) {
			return AbstractObjectValue::getEmptyObject();
		}
		else if (changes.empty()) {
			return base.v;
		}
		else if (base.empty()) {
			RefCntPtr<ObjectValue> merged = ObjectValue::create(changes.size());
			optimize();
			for (const Value &v: changes) {
				merged->push_back(v.getHandle());
			}
			return PValue::staticCast(merged);
		} else {
			std::size_t needsz = 0;
			optimize();

			mergeDiffsImpl(base.begin(), base.end(), changes.begin(), changes.end(),
					defaultConflictResolv,Path::root,
					[&] (const StringView<char> &, const Value &v) {
					    if (v.defined()) needsz++;
			});
			RefCntPtr<ObjectValue> merged = ObjectValue::create(needsz);

			mergeDiffsImpl(base.begin(), base.end(), changes.begin(), changes.end(),
					defaultConflictResolv,Path::root,
					[&merged](const StringView<char> &n, const Value &v) {
						if (v.defined()) merged->push_back(v.setKey(n).getHandle());
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
		changes.clear();
	}
	bool Object::dirty() const
	{
		return !changes.empty();
	}
	Object & Object::merge(Value object)
	{
		object.forEach([&](Value v) {
			set(v); return true;
		});
		return *this;
	}


	void Object::revert() {
		changes.clear();
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
	RefCntPtr<ObjectDiff> res = ObjectDiff::create(changes.size());
	optimize();
	for(const Value &x: changes) res->push_back(x.getHandle());
	return PValue::staticCast(res);
}

Value json::Object::applyDiff(const Value& baseObject, const Value& diffObject) {

	std::size_t needsz = 0;

	mergeDiffsImpl(baseObject.begin(), baseObject.end(), diffObject.begin(), diffObject.end(),
			defaultConflictResolv,Path::root,
			[&needsz](const StrViewA &, const Value &v) {
		if (v.defined()) needsz++;
	});

	RefCntPtr<ObjectValue> res = ObjectValue::create(needsz);

	mergeDiffsImpl(baseObject.begin(), baseObject.end(), diffObject.begin(), diffObject.end(),
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
		int cmp = lv.getKey().compare(rv.getKey());
		if (cmp < 0) {
			setFn(lv.getKey(),lv);++lit;
		} else if (cmp>0) {
			setFn(rv.getKey(),rv);++rit;
		} else {
			StringView<char> name = lv.getKey();
			if (lv.type() == ::json::object && rv.type() == ::json::object) {
				if (lv.flags() & objectDiff) {
					if (rv.flags() & objectDiff) {
						setFn(name, mergeDiffsObjs(lv,rv,resolver,path));
					} else {
						setFn(name ,applyDiff(rv,lv));
					}
				} else if (rv.flags() & objectDiff) {
					setFn(name ,applyDiff(lv,rv));
				} else {
					setFn(name , resolver(Path(path,name), lv, rv));
				}

			} else {
				setFn(name , resolver(Path(path,name), lv, rv));
			}
			++lit;
			++rit;
		}
	}
	while (lit != lend) {
		auto &&lv = *lit;
		const StringView<char> name = lv.getKey();
		setFn(name,lv);
		++lit;
	}
	while (rit != rend) {
		auto &&rv = *rit;
		const StringView<char> name = rv.getKey();
		setFn(name, rv);
		++rit;
	}
}


void Object::mergeDiffs(const Object& left, const Object& right, const ConflictResolver& resolver) {
	mergeDiffs(left,right,resolver,Path::root);
}

void Object::mergeDiffs(const Object& left, const Object& right, const ConflictResolver& resolver, const Path &path) {
	mergeDiffsImpl(left.begin(),left.end(),right.begin(),right.end(),resolver,path,
			[&](const StringView<char> &name, const Value &v) {
		this->set(name,v);
	});
}

Value Object::mergeDiffsObjs(const Value& lv, const Value& rv,
		const ConflictResolver& resolver, const Path& path) {

	std::size_t needsz = 0;
	mergeDiffsImpl(lv.begin(),lv.end(),rv.begin(),rv.end(),
			defaultConflictResolv,Path::root,[&](const StringView<char> &, const Value &){
		needsz++;
	});
	RefCntPtr<ObjectDiff> obj = ObjectDiff::create(needsz);
	mergeDiffsImpl(lv.begin(),lv.end(),rv.begin(),rv.end(),resolver,Path(path,lv.getKey()),
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

}

