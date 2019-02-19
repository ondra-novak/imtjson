#pragma once

#include "refcnt.h"
#include "stringview.h"
#include "allocator.h"
namespace json {

	///Type of JSON value
	enum ValueType {

		///null value - JSON's null value is also value as others
		null = 0,
		///boolean - true or false
		boolean = 1,
		///number, which can be floating or integer type (this is transparent)
		number = 2,
		///string - currently only ascii and utf-8 is supported 
		string = 3,
		///array
		array = 4,
		///object
		object = 5,
		///special type used everytime when value is not defined (it is also default for Value() )
		undefined = 6

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


	///Holds type of binary encoding
	class IBinaryEncoder;
	typedef const IBinaryEncoder *BinaryEncoding;

	extern BinaryEncoding base64;  ///< base64 standard encoding
	extern BinaryEncoding base64url;  ///< base64 for urls and files (with "-" and "_" and without padding character)
	extern BinaryEncoding urlEncoding; ///url encoding - for encoding uri components
	extern BinaryEncoding directEncoding;
	extern BinaryEncoding utf8encoding; ///<encode binary to UTF-8 valid characters
	extern BinaryEncoding defaultBinaryEncoding;

	///Various flags tied with JSON's type
	/**@see userDefined, numberInteger, numberUnsignedInteger, proxy,valueDiff,binaryString,customObject,preciseNumber
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

	/// States that value contains difference
	/**
	 * Diff value can be object or array. If it is applied, the original value
	 * is changed by the difference.
	 */
	const ValueTypeFlags valueDiff = 16;

	/// States that object is binary string
	/** This flag appears with a string only. It causes, that content of the string will be encoded
	 * using encoding specified during creation of this object. Parser cannot create binary strings. However
	 * you can use function Value::getBinary() to decode the parsed string as well as to receive content
	 * of binary string without decoding.
	 */
	const ValueTypeFlags binaryString = 32;

	/// States that value contains a custom object
	/** Value holds a custom object, which won't be serialized. Using
	 * standard API of the Value class returns data of a value supplied during
	 * creation thus it is disconnected from the object carried by the value
	 *
	 * @see makeValue
	 */
	const ValueTypeFlags customObject = 64;

	/// States that object stores precise number
	/** Precise numbers are stored directly as strings. This flag can appear
	 * with type string or number. If it appears on string, it states, that
	 * the value has been parsed as string, but it contains precise number. It
	 * it appears on number, th states, that the value has been parsed as number,
	 * but is contains precise number, which can be retrieved as string.
	 *
	 * Precise number can be accesed without conversion through the method
	 * Value::getString(). You can also convert such Value to String object.
	 * The function Value::toString() is slightly faster with precise numbers
	 *
	 *
	 * @note precise numbers are currently not supported by binjson. They are
	 * converted to standard double value
	 */

	const ValueTypeFlags preciseNumber = 128;


	typedef int BinarySerializeFlags;

	///This flag allowes compression of the key
	/**  Compressed keys takes less space, On other hand,
		serializer must lookup for each key in dictionary and this can
		harm the performance. There is also limit up to 127 keys
		that can be effectively compressed. For larger objects,
		with many keys this feature can be less efficient */	     
	
	const BinarySerializeFlags compressKeys = 0x1;
	/// Binary serialize will maintain 32bit compatibility.
	/** This allows to parse the binary format under 32bit system. The
	    flag causes, that numbers with more then 32bits will be transfered
		as floating point number. 

		This flag has no effect on 32bit system
	*/
	    
	const BinarySerializeFlags maintain32BitComp = 0x2;
	///This flag enables of compression token strings enums and base64url strings
	/** The compression works only for strings, which contain only
	 * letters, capitals, numbers, underscores and minus (base64url set).
	 *
	 * These strings are stored using 6 bits instead eights, so two bits
	 * are saved on every byte.
	 *
	 * The feature is disabled by default, because it can make serialization
	 * slower, because the serialized must check, whether the string can
	 * be compressed by this way.
	 *
	 * The first byte is always uncompressed, folloing bytes are
	 * compressed. The compressed string starts with byte between 0x80 and 0x8BF
	 * which is invalid UTF-8 character. The byte stores six bits of the
	 * first character. Following characters are stored using 6 bits per each.
	 *
	 * Compression is effective for strings longer then 4 characters
	 * "A" -> 0x80
	 * "AB" -> 0x80, 0x04
	 * "ABC -> 0x80, 0x04, 0x20
	 * "ABCD -> 0x80, 0x04, 0x20, 0xC0
	 * "ABCDE -> 0x80, 0x04, 0x20, 0xC4
	 *
	 */

	const BinarySerializeFlags compressTokenStrings = 0x4;

	class IValue;
	typedef RefCntPtr<const IValue> PValue;

	class IEnumFn;

	///Interface access internal value of JSON Value
	class IValue: public RefCntObj, public AllocObject {
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

		virtual int compare(const IValue *other) const = 0;
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
