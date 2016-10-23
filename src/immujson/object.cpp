#include "object.h"
#include "objectValue.h"
#include "array.h"
#include <time.h>

#include "path.h"

namespace json {

	class ObjectProxy : public AbstractValue {
	public:

		ObjectProxy(const StringView<char> &name, const PValue &value)
			:name(name), value(value) {}
		
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
		virtual StringView<char> getMemberName() const override { return name; }
		virtual const IValue *unproxy() const override { return value->unproxy(); }
		virtual bool equal(const IValue *other) const override {
				return value->equal(other->unproxy());
		}

	protected:
		std::string name;
		PValue value;

	};

	class ObjectDiff: public ObjectValue {
	public:
		ObjectDiff(const std::vector<PValue> &value):ObjectValue(value) {}
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

	Object & Object::set(const StringView<char>& name, const Value & value)
	{
		PValue v = value.getHandle();
		if (v->flags() & proxy ) {
			StringView<char> curName = v->getMemberName();
			if (curName == name) {
				changes[curName] = v;
				return *this;
			} else {
				v = v->unproxy();
			}

		}
		v = new ObjectProxy(name, v);
		StringView<char> curName = v->getMemberName();
		changes[curName] = v;
		return *this;
	}

	Object & Object::set(const Value & value)
	{
		StringView<char> curName = value.v->getMemberName();
		changes[curName] = value.v;
		return *this;
	}

	Object & Object::unset(const StringView<char>& name)
	{
		return set(name, AbstractValue::getUndefined());
	}

	Value Object::operator[](const StringView<char> &name) const {
		Changes::const_iterator it = changes.find(name);
		if (it == changes.end()) {
			return base[name];
		}
		else {
			return Value(it->second);
		}
	}

	Object &Object::operator()(const StringView<char> &name, const Value &value) {
		return set(name, value);		
	}

	static void append(std::vector<PValue> &merged, const PValue &v) {
		if (v->type() != undefined) merged.push_back(v);
	}
	

	PValue Object::commit() const {
		if (base.empty() && changes.empty()) {
			return AbstractObjectValue::getEmptyObject();
		}
		if (changes.empty()) {
			return base.v;
		}
		std::vector<PValue> merged = commitToVector();
		return new ObjectValue(std::move(merged));
	}

	class Object::NameValueIter: public Changes::const_iterator {
	public:
		NameValueIter(const Changes::const_iterator &iter):Changes::const_iterator(iter) {}

		struct FakeValue:public std::pair<StringView<char>, Value> {
			FakeValue(const std::pair<StringView<char>, Value> &p)
					:std::pair<StringView<char>, Value>(p) {}
			const StringView<char> getKey() const {return first;}
			ValueType type() const {return second.type();}
			ValueTypeFlags flags() const {return second.flags();}
			operator const Value &() const {return second;}
		};

		FakeValue operator *() const {
			return FakeValue(Changes::const_iterator::operator *());
		}
	};

	std::vector<PValue> Object::commitToVector() const {

		std::vector<PValue> merged;
		merged.reserve(base.size()+changes.size());
		NameValueIter chbeg(changes.begin());
		NameValueIter chend(changes.end());
		mergeDiffsImpl(base.begin(), base.end(), chbeg, chend,
				[](const Path &, const Value &left, const Value &right) {
						return right;
				},Path::root,[&merged](const StringView<char> &, const Value &v) {
					append(merged, v.getHandle());
				});

		return std::move(merged);
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

	ObjectIterator Object::begin() const {
		return ObjectIterator(std::move(commitToVector()));
	}

	ObjectIterator Object::end() const {
		return ObjectIterator(std::vector<PValue>());
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
	std::vector<PValue> chvect;
	chvect.reserve(changes.size());
	for(auto &&item: changes) chvect.push_back(item.second.getHandle());
	return new ObjectDiff(std::move(chvect));
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
				} else {
					setFn(name ,applyDiff(lv,rv));
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
		else out.push_back(new ObjectProxy(name,v.getHandle()->unproxy()));
	});
	return (new ObjectValue(std::move(out)));
}

Value Value::setKey(const StringView<char> &key) const {
	if (getKey() == key) return *this;
	if (key.empty()) return Value(v->unproxy());
	return Value(new ObjectProxy(key,v->unproxy()));
}

StringView<PValue> Object::getItems(const Value& v) {
	const IValue *pv = v.getHandle();
	const ObjectValue *ov = dynamic_cast<const ObjectValue *>(pv->unproxy());
	if (ov) return ov->getItems(); else return StringView<PValue>();
}

}

