#include <cstring>
#include "object.h"
#include "objectValue.h"
#include "array.h"
#include <time.h>

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
		return ::operator new(needsz);
	}
	void ObjectProxy::operator delete(void *ptr, const StringView<char> &) {
		::operator delete (ptr);
	}
	void ObjectProxy::operator delete(void *ptr, std::size_t) {
		::operator delete (ptr);
	}


	class ObjectDiff: public ObjectValue {
	public:
		ObjectDiff(std::vector<PValue> &&value):ObjectValue(std::move(value)) {}
		virtual ValueTypeFlags flags() const override { return objectDiff;}
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
	lastIterSnapshot = nullptr;
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
			return;
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

	static void append(std::vector<PValue> &merged, const PValue &v) {
		if (v->type() != undefined) merged.push_back(v);
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
					[](const Path &, const Value &, const Value &right) {
							return right;
					},Path::root,[&newChanges](const StringView<char> &, const Value &v) {
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
			std::vector<PValue> merged;
			optimize();
			merged.reserve(changes.size());
			for (const Value &v: changes) {
				merged.push_back(v.getHandle());
			}
			return new ObjectValue(std::move(merged));
		} else {
			optimize();
			std::vector<PValue> merged;
			merged.reserve(base.size()+changes.size());
			mergeDiffsImpl(base.begin(), base.end(), changes.begin(), changes.end(),
					[](const Path &, const Value &left, const Value &right) {
							return right;
					},Path::root,[&merged](const StringView<char> &, const Value &v) {
						append(merged, v.getHandle());
					});
			return new ObjectValue(std::move(merged));


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

	auto oldIt = oldObject.begin();
	auto newIt = newObject.begin();
	auto oldEnd = oldObject.end();
	auto newEnd = newObject.end();
	while (oldIt != oldEnd && newIt != newEnd) {

		Value oldV = *oldIt;
		Value newV = *newIt;


		StringView<char> oldName = oldV.getKey();
		StringView<char> newName = newV.getKey();
		int cmp = oldName.compare(newName);

		if (cmp < 0) {
			unset(oldName);
			++oldIt;
		} else if (cmp > 0) {
			set(newName, newV);
			++newIt;
		} else {
			if (oldV != newV) {
				if (recursive && oldV.type() == ::json::object && newV.type() == ::json::object) {
					Object tmp;
					tmp.createDiff(oldV,newV,recursive-1);
					set(newName, tmp.commitAsDiff());
				} else {
					set(newV);
				}
			}
			++oldIt;
			++newIt;
		}
	}
	while (oldIt != oldEnd) {
		unset((*oldIt).getKey());
		++oldIt;
	}
	while (newIt != newEnd) {
		set(*newIt);
		++oldIt;
	}

}

Value Object::commitAsDiff() const {
	optimize();
	std::vector<PValue> vect;
	vect.reserve(changes.size());
	for(const Value &x: changes) vect.push_back(x.getHandle());
	return new ObjectDiff(std::move(vect));
}

Value json::Object::applyDiff(const Value& baseObject, const Value& diffObject) {
	auto oldIt = baseObject.begin();
	auto newIt = diffObject.begin();
	auto oldEnd = baseObject.end();
	auto newEnd = diffObject.end();
	std::vector<PValue> merged;
	merged.reserve(baseObject.size()+diffObject.size());

	while (oldIt != oldEnd && newIt != newEnd) {

		Value oldV = *oldIt;
		Value newV = *newIt;

		StringView<char> oldName = oldV.getKey();
		StringView<char> newName = newV.getKey();
		const PValue &olditem = oldV.getHandle();
		const PValue &newitem = newV.getHandle();

		int cmp = oldName.compare(newName);

		if (cmp < 0) {
			append(merged,olditem);
			++oldIt;
		} else if (cmp > 0) {
			append(merged,newitem);
			++newIt;
		} else {
			if (newitem->flags() & objectDiff) {
				append(merged, applyDiff(olditem,newitem).getHandle());
			} else {
				append(merged, newitem);
			}
			++oldIt;
			++newIt;
		}
	}
	while (oldIt != oldEnd) {
		append(merged,(*oldIt).getHandle());
		++oldIt;
	}
	while (newIt != newEnd) {
		append(merged,(*newIt).getHandle());
		++newIt;
	}
	return new ObjectValue(std::move(merged));

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
	std::vector<PValue> out;
	out.reserve(lv.size()+rv.size());
	const StringView<char> name = lv.getKey();
	mergeDiffsImpl(lv.begin(),lv.end(),rv.begin(),rv.end(),resolver,Path(path,name),
			[&out](const StringView<char> &name, const Value &v){
		if (v.getKey() == name) out.push_back(v.getHandle());
		else out.push_back(new(name) ObjectProxy(name,v.getHandle()->unproxy()));
	});
	return (new ObjectValue(std::move(out)));
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

