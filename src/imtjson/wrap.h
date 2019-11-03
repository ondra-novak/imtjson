#include "value.h"

namespace json {




namespace _details {

	class IWrap: public IValue {
	public:

		virtual const void *cast(const std::type_info &type) const = 0;
		virtual void throwPtr() const = 0;

	};

	template<typename Object>
	class ObjWrap: public IWrap {
	public:

		template<typename T>
		ObjWrap(T &&obj, PValue pv):obj(std::forward<T>(obj)),pv(pv) {}

		virtual ValueType type() const {return pv->type();}
		virtual ValueTypeFlags flags() const {return pv->flags() | customObject;}

		virtual UInt getUInt() const {return pv->getUInt();}
		virtual Int getInt() const {return pv->getInt();}
		virtual ULongInt getUIntLong() const {return pv->getUIntLong();}
		virtual LongInt getIntLong() const {return pv->getIntLong();}
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

		virtual const void *cast(const std::type_info &type) const override {
			const std::type_info &myti = typeid(obj);
			if (myti == type) return &obj;
			return nullptr;
		}
		virtual void throwPtr() const override{
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
				wp.throwPtr(); throw;
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
				wp->throwPtr(); throw;
			} catch (const Object *ptr) {
				return ptr;
			} catch (...) {
				return nullptr;
			}
		}
	};

}

///Packs arbitrary value into json::Value
/** Packed value can be used as json value, so you are able to put the value into an object or an array.
 * You can later extract the value using the function cast.
 *
 * @param object value to be packed
 * @param value json substitute value. You need the supply subsitute value to retain all features of the json::Value.
 *  Anywhere is requested particular feature of the value, the subsitute value is used instead. This is also applied
 * during serialization (packed value is serialized using its substitute)
 * @return json::Value value
 *
 * @note result value is immutable. There is no way to change content of packed value.
 */



template<typename T>
inline Value makeValue(T&& object, Value value) {
	PValue v = new _details::ObjWrap<typename std::remove_reference<T>::type>(std::forward<T>(object),value.getHandle()->unproxy());
	return Value(v);
}

///Converts the packed value back to original type
/**
 *
 * @tparam target type
 * @param value a Value object.
 * @return if target is pointer, then the function return pointer to the content. If the target is not pointer,
 * then reference is returned. In case that value cannot be cast to the target type, the function returns nullptr.
 * In case that non-pointer result is request, then function throws std::bad_cast exception
 * @exception std::bad_cast function is unable to cast the value. This exception happen, when non-pointer target is
 * requested and the target type is not compatibile with the content
 */
template<typename T>
inline auto cast(Value value) -> decltype(_details::CastHelper<T>::cast(value)) {
	return _details::CastHelper<T>::cast(value);
}



}
