#include "object.h"
#include "objectValue.h"
#include "edit.h"
#include <time.h>

namespace json {

	class ObjectProxy : public AbstractValue {
	public:

		ObjectProxy(const StringRef<char> &name, const PValue &value)
			:name(name), value(value) {}
		
		virtual ValueType type() const override { return value->type(); }
		virtual ValueTypeFlags flags() const override { return value->flags() | proxy; }

		virtual std::uintptr_t getUInt() const override { return value->getUInt(); }
		virtual std::intptr_t getInt() const override { return value->getInt(); }
		virtual double getNumber() const override { return value->getNumber(); }
		virtual bool getBool() const override { return value->getBool(); }
		virtual StringRef<char> getString() const override { return value->getString(); }
		virtual std::size_t size() const override { return value->size(); }
		virtual const IValue *itemAtIndex(std::size_t index) const override { return value->itemAtIndex(index); }
		virtual const IValue *member(const StringRef<char> &name) const override { return value->member(name); }
		virtual bool enumItems(const IEnumFn &fn) const override { return value->enumItems(fn); }
		virtual StringRef<char> getMemberName() const override { return name; }
		virtual const IValue *getMemberValue() const override { return value; }

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

	Object::Object(const StringRef<char>& name, const Value & value): base(AbstractObjectValue::getEmptyObject())
	{
		set(name, value);
	}

	Object::~Object()
	{
	}

	Object & Object::set(const StringRef<char>& name, const Value & value)
	{
		StringRef<char> curName = value.v->getMemberName();
		if (curName == name) {
			changes[curName] = value.v;
		}
		else {
			PValue v = new ObjectProxy(name, value.v->getMemberValue());
			StringRef<char> curName = v->getMemberName();
			changes[curName] = v;
		}
		return *this;
	}

	Object & Object::set(const Value & value)
	{
		StringRef<char> curName = value.v->getMemberName();
		changes[curName] = value.v;
		return *this;
	}

	Object & Object::unset(const StringRef<char>& name)
	{
		return set(name, AbstractValue::getUndefined());
	}

	Value Object::operator[](const StringRef<char> &name) const {
		Changes::const_iterator it = changes.find(name);
		if (it == changes.end()) {
			return base[name];
		}
		else {
			return Value(it->second);
		}
	}

	Object &Object::operator()(const StringRef<char> &name, const Value &value) {
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
			StringRef<char> baseName = olditem->getMemberName();
			StringRef<char> newName = newitem->getMemberName();
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
	Object2Object Object::object(const StringRef<char>& name)
	{
		return Object2Object((*this)[name],*this,name);
	}
	Array2Object Object::array(const StringRef<char>& name)
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