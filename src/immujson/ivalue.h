#pragma once

#include "refcnt.h"
#include "stringview.h"
namespace json {

	///Type of JSON value
	enum ValueType {

		///number, which can be floating or integer type (this is transparent)
		number,
		///string - currently only ascii and utf-8 is supported 
		string,
		///boolean - true or false
		boolean,
		///array
		array,
		///object
		object,
		///null value - JSON's null value is also value as others
		null,
		///special type used everytime when value is not defined (it is also default for Value() )
		undefined

	};

	enum UnicodeFormat {
		///Emit unicode characters using escape sequence \uXXXX
		/** This mode is default*/
		emitEscaped,
		///emit unicode as utf-8 sequence. 
		/** Not ale unicode characters are written as utf-8. Some of them
		are still escaped */
		emitUtf8
	};


	///Various flags tied with JSON's type
	/**@see userDefined, numberInteger, numberUnsignedInteger, proxy
	 *
	 */
	typedef uintptr_t ValueTypeFlags;

	///User defined value
	/** It is possible to inherit any class from the json namespace and 
	    use it to introduce a new type of value. However it has to 
		always specify one of basic types excluding "undefined". Ever
		such type should also emit flag userDefined to allow to other
		parts of the library easily detect such a type and carry the
		value more generaly way */		

	const ValueTypeFlags userDefined = 1;
	/// For number type, this states, that number is stored as integer
	const ValueTypeFlags numberInteger = 2;
	/// For number type, this states, that number is stored as unsigned integer
	const ValueTypeFlags numberUnsignedInteger = 4;
	/// States that object is only proxy to an other object.
	/** Proxy objects are used to carry a key with the value in the objects. Proxies
		are transparent, but can be interfere with dynamic_cast.
		To resolve proxy, you have to use unproxy() function.
		
		While enumerating through the object, returned values are proxies that
		each of them carries both the key and the value
		*/
	const ValueTypeFlags proxy = 8;

	/// States that object is diff-object
	/** Diff-object contains difference between two object which can be applied to a third
	 *  object. Diff-objects are used internally and should not appear outside of the
	 *  library. however this flag has been introduced to easily determine whether the object
	 *  contains differences
	 */
	const ValueTypeFlags objectDiff = 16;

	class IValue;
	typedef RefCntPtr<const IValue> PValue;

	class IEnumFn;

	///Interface access internal value of JSON Value
	class IValue: public RefCntObj {
	public:
		virtual ~IValue() {}

		virtual ValueType type() const = 0;
		virtual ValueTypeFlags flags() const = 0;
		
		virtual std::uintptr_t getUInt() const = 0;
		virtual std::intptr_t getInt() const = 0;
		virtual double getNumber() const = 0;
		virtual bool getBool() const = 0;
		virtual StringView<char> getString() const = 0;
		virtual std::size_t size() const = 0;
		virtual const IValue *itemAtIndex(std::size_t index) const = 0;
		virtual const IValue *member(const StringView<char> &name) const = 0;
		virtual bool enumItems(const IEnumFn &) const = 0;
		
		///some values are proxies with member name - this retrieves name
		virtual StringView<char> getMemberName() const = 0;
		///Returns pointer to first no-proxy object
		virtual const IValue *unproxy() const = 0;

		virtual bool equal(const IValue *other) const = 0;
	};

	class IEnumFn {
	public:
		virtual ~IEnumFn() {}
		virtual bool operator()(const IValue *v) const = 0;
	};

	template<typename Fn>
	class EnumFn : public IEnumFn {
	public:
		EnumFn(const Fn &fn) :fn(fn) {}
		virtual bool operator()(const IValue *v) const {
			return fn(v);
		}
	protected:
		mutable Fn fn;
	};

}
