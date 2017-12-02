#include "value.h"

namespace json {


template<typename Object>
Value makeValue(Object &&object, Value value);


template<typename Object>
const Object &cast(Value value);



namespace _details {

	class IWrap: public IValue {
	public:

		const void *cast(const std::type_info &type) const = 0;
		void throwPtr() const = 0;

	};

	template<typename Object>
	class ObjWrap: public IWrap {
	public:

		ObjWrap(Object &&obj, PValue pv):obj(std::forward<Object>(obj)),pv(pv) {}

		virtual ValueType type() const {return pv->type();}
		virtual ValueTypeFlags flags() const {return pv->flags() | customObject;}

		virtual std::uintptr_t getUInt() const {return pv->getUInd();}
		virtual std::intptr_t getInt() const {return pv->getInt();}
		virtual double getNumber() const {return pv->getNumber();}
		virtual bool getBool() const {return pv->getBool();}
		virtual StringView<char> getString() const {return pv->getString();}
		virtual std::size_t size() const {return pv->size();}
		virtual const IValue *itemAtIndex(std::size_t index) const {return pv->itemAtIndex(index);}
		virtual const IValue *member(const StringView<char> &name) const {return pv->member(name);}
		virtual bool enumItems(const IEnumFn &e) const {return pv->enumItems(e);}

		///some values are proxies with member name - this retrieves name
		virtual StringView<char> getMemberName() const {return pv->getMemberName();}
		///Returns pointer to first no-proxy object
		virtual const IValue *unproxy() const {return pv->unproxy();}

		virtual bool equal(const IValue *other) const {return pv->equal(other);}

		virtual int compare(const IValue *other) const {return pv->compare(other);}

		void *cast(const std::type_info &type) const {
			const std::type_info &myti = typeid(obj);
			if (myti == type) return &obj;
		}
		void throwPtr() const {
			throw &obj;
		}


	protected:
		Object obj;
		PValue pv;

	};


	template<typename Object>
	class CastHelper {
	public:
		static const Object &cast(Value value) {
			const IWrap &wp = dynamic_cast<const IWrap &>(*value.getHandle());
			const void *r = wp.cast(typeid(Object));
			if (r) return *(const Object *)(r);
			try {
				wp.throwPtr();
			} catch (const Object *ptr) {
				return *ptr;
			} catch (...) {
				throw std::bad_cast();
			}
		}
	};

	template<typename Object>
	class CastHelper<Object *> {
	public:
		static const Object *cast(Value value) {
			const IWrap *wp = dynamic_cast<const IWrap *>(value.getHandle());
			if (wp == nullptr) return nullptr;
			const void *r = wp->cast(typeid(Object));
			if (r) return (const Object *)(r);
			try {
				wp->throwPtr();
			} catch (const Object *ptr) {
				return ptr;
			} catch (...) {
				return nullptr;
			}
		}
	};

}




}

template<typename Object>
inline Value json::makeValue(Object&& object, Value value) {
	PValue v = new _details::ObjWrap(std::foward<Object>(obj),value.getHandle()->unproxy());
	return Value(v);
}

template<typename Object>
inline auto json::cast(Value value) -> decltype(_details::CastHelper<Object>::cast(value)) {
	return _details::CastHelper<Object>::cast(value);
}

