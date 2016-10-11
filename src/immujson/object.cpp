#include "object.h"
#include "objectValue.h"
#include "edit.h"
#include <time.h>

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
			if ((other->flags() & proxy) == 0 || other->getMemberName() != name) return false;
			else return value->equal(other->unproxy());
		}

	protected:
		std::string name;
		PValue value;

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
	

	PValue Object::commit() const
	{
		if (base.empty() && changes.empty()) {
			return AbstractObjectValue::getEmptyObject();
		}
		if (changes.empty()) {
			return base.v;
		}

		std::vector<PValue> merged;
		std::size_t baseCnt = base.size();
		std::size_t oldit = 0;
		Changes::const_iterator newCnt = changes.cend();
		Changes::const_iterator newit = changes.cbegin();
		while (oldit != baseCnt && newit != newCnt) {
			const PValue &olditem = base[oldit].v;
			const PValue &newitem = newit->second;
			StringView<char> baseName = olditem->getMemberName();
			StringView<char> newName = newitem->getMemberName();
			int cmp = baseName.compare(newName);
			if (cmp < 0) {
				append(merged, olditem);
				++oldit;
			}
			else if (cmp > 0) {
				append(merged, newitem);
				++newit;
			}
			else {
				append(merged, newitem);
				++oldit;
				++newit;
			}
		}
		while (oldit != baseCnt) {
			append(merged, base[oldit].v);
			++oldit;
		}
		while (newit != newCnt) {
			append(merged, newit->second);
			++newit;
		}
		return new ObjectValue(merged);
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

}
