#pragma once

#include <vector>
#include "ivalue.h"
#include <functional>
#include "range.h"

namespace json {

	class Array;
	class Object;
	class Path;
	class PPath;
	class ValueIterator;
	class String;
	class Binary;
	struct Allocator;
	template<typename T> class ConvValueAs;
	template<typename T> class ConvValueFrom;
	template<typename Cmp> class Ordered;

	///Stores one JSON value
	/** The instance of the class Value can store one value of any JSON type:
	 * array, object, string, number, boolean, null and undefined
	 *
	 * It acts as common variable for the user, so you can copy it, assign it, declare it on stack,
	 * or allocate on heap. There should be no limitations. Content of the variable cannot be changed,
	 * this is general rule of the library. Only way to change content of variable is to perform
	 * assignment from another variable.
	 *
	 * To modify complex structure, such an object or an array, see the description of the classes
	 * Object and Array. However, result of every (inner) modification is the new instance of the Value.
	 */
	class Value {
	public:

		typedef std::pair<Value, Value> TwoValues;
		///Initialize variable with value of specified type
		/** You can use this constructor, if you need to get a value of specified type regardless
		 * on content. The constructor just creates the empty value. The main advantage
		 * of this function is that creation is very fast, because it typically shares already pre-created
		 * stock object of given type.
		 *
		 * @param vtype required type
		 * @arg @c undefined create undefined type - equivalent to default constructor
		 * @arg @c null create null variable
		 * @arg @c boolean create boolean variable which is initialized to false
		 * @arg @c string create empty string
		 * @arg @c number create number which is initialized to 0
		 * @arg @c array create empty array
		 * @arg @c object create empty object
		 */
		Value(ValueType vtype);
		///Initialize the variable to boolean value
		/**
		 * @param value value of the variable
		 */
		Value(bool value);
		///Initialize the variable to string type
		/**
		 * @param value pointer to C-like string (terminated by character zero)
		 */
		Value(const char *value);
		///Initialize the variable to string type
		/**
		 * @param value reference to C++ string
		 */
		Value(const std::string &value);

		///Initialize the variable to string type
		/**
		 * @param value reference to C++ string-view		*/

#if __cplusplus >= 201703L
		Value(const std::string_view &value):Value(StringView<char>(value)) {}
#endif

		///Initialize the variable to string type
		/**
		 * @param value Reference to string view (introduced in this library improve performance
		 *   in string manipulation.
		 *
		 */
		Value(const StringView<char> &value);
		///@{
		///Initialize the variable to signed or unsigned number
		/**
		 * @param x new value.
		 *
		 */
		Value(signed short x);
		Value(unsigned short x);
		Value(signed int x);
		Value(unsigned int x);
		Value(signed long x);
		Value(unsigned long x);
		Value(signed long long x);
		Value(unsigned long long x);
		///@}

		///@{
		///Initialize the variable to floating point number
		/**
		 * @param value new value.
		 *
		 */
		Value(double value);
		Value(float value);
		///@}
		///Initialize the variable to null
		/**
		 * @note Value null is treated as ordinary value without special meaning for the library.
		 *
		 * @see undefined
		 */
		Value(std::nullptr_t);
		///Initialize the variable to undefined
		/**
		 * Variable with undefined value cannot be serialized to the stream. Undefined
		 * member of an object is deleted. Undefined value in an array is skipped.
		 */
		Value();
		///@{
		///Initialize the variable with pointer to underlying interface
		/** This constructor is used by internals. However you can declare a new
		 * class which inherits one of descendants of IValue interface (or you can
		 * inherit IValue directly) and this is only way, how to pass the new custom
		 * value to the variable of Value type
		 *
		 * @param v pointer to fresh instance of object which implements IValue
		 */
		Value(const IValue *v);
		Value(const PValue &v);
		///@}

		///Initializes the variable by content of the Array
		/**
		 * To modify the array , you have to convert Value to Array first. See the description
		 * of the class Array. To commit changes made in the Array, you have to convert the Array
		 * back to the Value through this constructor. Note that a new instance is created
		 * (no modifications is carried back to the original array)
		 * @param value modified array
		 */
		Value(const Array &value);
		///Initializes the variable by content of the Object
		/**
		 * To modify the object , you have to convert Value to Object first. See the description
		 * of the class Object. To commit changes made in the Object, you have to convert the AObject
		 * back to the Value through this constructor. Note that a new instance is created
		 * (no modification is carried back to the original object)
		 * @param value modified object
		 */
		Value(const Object &value);

		///Initailize the variable directly from the String
		/**
		 * @param value a string value
		 */
		Value(const String &value);


		///Initialize the variable as array of values
		/**
		 * @param value refernce to a string view containing values
		 */
		Value(const StringView<Value> &value);


		///Initailize the variable from a container
		/**
		 * @param type type of value to construct. Only supported values are array and object. Other
		 * values will construct undefined value
		 * @param values container of values. If type is object, the values must be
		 * associated with keys and the keys must be unique. Duplicated keys are removed
		 * (there is no rule, which key=value pair is removed during this process)
		 * @param skipUndef if true (default), function skips undefined values. Set this to false to
		 * also include undefined values
		 */
		Value(ValueType type, const StringView<Value> &values, bool skipUndef = true);

		///Initialize the variable using initializer list {....}
		/**
		 * This allows to use initializer list {...} to initialize the variable.
		 * @code
		 * Value v = {10,20,"hello", null, {"subarray", 21, true } };
		 * @endcode
		 *
		 * @param data argument is created by compiler.
		 *
		 * @note the constructor detects, whether the container is object or array
		 * by exploring the first value. If there is key, which can be defined by
		 * either lexical or key/ prefix, the constructor creates the object. Otherwise
		 * it creates array.
		 *
		 */
		Value(const std::initializer_list<Value> &data);

		///Serializes any container to JSON array or object through the map function
		/**
		 * @param type type of container, Only array or object are supported. If object
		 * is selected, then every returned value must have a key
		 * @param begin begin of iteration
		 * @param end end of iteration
		 * @param mapFn mapping function
		 * @param skipUndef skip undefined results (on by default)
		 */
		template<typename InputIterator, typename Fn>
		Value(ValueType type, InputIterator begin, InputIterator end, Fn &&mapFn, bool skipUndef = true);

		template<typename  Iter>
		Value(const Range<Iter> &data);


		///Create binary value
		/** Binary values are not supported by JSON. They are emulated through encoding
		 *
		 * @param binary binary content
		 */
		Value(const BinaryView &binary, BinaryEncoding enc = defaultBinaryEncoding);


		Value(const StrViewA key, const Value &value);

		///Create binary value
		/** Binary values are not supported by JSON. They are emulated through encoding
		 *
		 * @param binary binary content
		 */
		Value(const Binary &binary);


		///Creates precise number value
		/*** Precise number is number stored as string. It is serialized diractly as
		 * it is. It is still can be used as ordinary number, but it can be damaged
		 * while it is converted to the double type
		 *
		 * @param number number written as string. Note that input value must be valid JSON number, otherwise function throws ParseError exception
		 * @return value which stores precise number
		 * @exception InvalidNumericFormat thrown when input is not valid number
		 */
		static Value preciseNumber(const StrViewA number);

		///Retrieves type of value
		/**
		 * @return type of value
		 * @see ValueType
		 */
		ValueType type() const { return v->type(); }

		///Retrieves flags associated with the type
		/** The flags represents more specific details about type stored in the variable. These
		 * flags are not part of JSON standard, but they can be sometimes useful to determine
		 * optimal way to work with the variable
		 *
		 * @return combination of flags
		 * @see ValueTypeFlags
		 */
		ValueTypeFlags flags() const {return v->flags();}

		///Retrieve unsigned integer
		/**
		 * @return value as unsigned integer
		 * @note Automatic conversion is limited to numbers only. This conversion is transparent. If
		 * there is floating point number stored, the function getUInt() returns only the integer part
		 * of the number. The function returns zero for other types.
		 */
		UInt getUInt() const { return v->getUInt(); }
		///Retrieve signed integer
		/**
		 * @return value as signed integer
		 * @note Automatic conversion is limited to numbers only. This conversion is transparent. If
		 * there is floating point number stored, the function getInt() returns only the integer part
		 * of the number. The function returns zero for other types.
		 */
		Int getInt() const { return v->getInt(); }
		///Retrieve unsigned integer of 64bit width.
		/**
		 * @return value as unsigned integer
		 * @note Automatic conversion is limited to numbers only. This conversion is transparent. If
		 * there is floating point number stored, the function getUIntLong() returns only the integer part
		 * of the number. The function returns zero for other types.
		 *
		 * @note to maintain 32-bit compatibility
		 */
		ULongInt getUIntLong() const { return v->getUIntLong(); }
		///Retrieve signed integer of 64bit width
		/**
		 * @return value as signed integer
		 * @note Automatic conversion is limited to numbers only. This conversion is transparent. If
		 * there is floating point number stored, the function getInt() returns only the integer part
		 * of the number. The function returns zero for other types.
		 *
		 * @note to maintain 32-bit compatibility
		 *
		 */
		LongInt getIntLong() const { return v->getIntLong(); }
		///Retrieve floating point number
		/**
		 * @return value as floating point
		 * @note Automatic conversion is limited to numbers only. This conversion is transparent.
		 * The function returns zero for other types.
		 */
		double getNumber() const { return v->getNumber(); }
		///Retrieves boolean value
		/**@retval true the variable contains true or non-empty string or non-zero value or non-empty object
		 *  or non-empty array
		 * @retval false the variable contains false or undefined or null or empty string or
		 *  empty object or empty array
		 * @return
		 */
		bool getBool() const { return v->getBool(); }
		///Retrieves string value
		/**
		 * @return string value
		 * @note The function works only for string type. Otherwise it returns empty string. No
		 * automatic conversion from other type to string is performed. If you
		 * need such automatic conversion, use toString()
		 */
		StringView<char> getString() const { return v->getString(); }

		///Retrieves binary content encoded by specified method
		/**
		 * @param be specify binary encoding.
		 * @return Function returns decoded binary string.
		 */
		Binary getBinary(BinaryEncoding enc = base64) const;
		///Retrieves count of items in the container (the array or the object)
		/**
		 * @return For arrays or objects, the function returns count of items inside. For
		 * other types, fhe function returns 0
		 *
		 * @note For strings, the function returns always zero. You have to
		 * call String::length() or getString().length to receive length of
		 * the string
		 */
		std::size_t size() const { return v->size(); }
		///Determines whether value is an empty container
		/**
		 * @retval false the object or the array is not empty
		 * @retval true the object or the array is empty, or the value is neither an array nor an object.
		 *
		 * @note For strings, the function returns always true. You have to
		 * call String::empty() or getString().empty() to determine, whether
		 * the string is empty.
		 *
		 */
		bool empty() const { return v->size() == 0; }
		///Access to an item stored in the container (array or object)
		/**
		 * Item in both array and object can be accessed via zero based index. First item has
		 * index 0, last item has index equal to size()-1.
		 *
		 * @param index index of the item to retrieve
		 * @return Value of item in the container at the given index. If the container is an object,
		 * returned value can be requested for the key - see getKey() function
		 *
		 * You cannot use the operator on strings to obtain n-th character.
		 */
		Value operator[](uintptr_t index) const { return v->itemAtIndex(index); }
		///Access to an item stored in the object
		/**
		 * @param key the key to retrieve.
		 * @return If the object has such an item, it is returned. Otherwise, undefined is returned
		 */
		Value operator[](const StringView<char> &key) const { return v->member(key); }
		///Access to an item in a container identified using the Path object
		/**
		 * @param path Instance o Path which should refer a desired item
		 * @return Item or undefined
		 */
		Value operator[](const Path &path) const;

		///Retrieve key of the value if the value is part of an object
		/**
		 * Items in the object are kept as pairs where each contains the key and the value.
		 * Every time the item is retrieved, it also carries its original key-name until
		 * the key is changed by putting the item into other object
		 * (to achieve immutability, the key is not actually changed,
		 * but inserted item is paired with a new key, so it is possible to have
		 * an item with many keys paired with it)
		 *
		 * @return name of key. If the value is not paired with a key,
		 * return is empty string. To determine whether value is paired see proxy.
		 *
		 * @see proxy, setKey
		 */
		StringView<char> getKey() const { return v->getMemberName(); }

		///Removes key from the value
		/** Items in objects are stored with keys. This key is part of the value, and
		 * even if there are many operations ignoring the key, it still can often
		 * obstruct to some processing. Function strips the key from the value. If the
		 * value has no key, the value itself is returned
		 * @return value where key is stripped.
		 */
		Value stripKey() const {return v->unproxy();}

		///Binds a key-name to the item
		/** The function is used by objects, however you can freely bind any value to a specified key outside of the object.
		 * @param key-name which is bind to the value
		 * @return new value with bound key.
		 *
		 * @note due the immutable nature of the  value, you cannot change or set
		 * the key-name to the existing value. A new value is always created.
		 *
		 * @see getKey
		 *
		 * @note function allocates a space for the key. It is faster than converting to the String and bind that object
		 *
		 */
		Value setKey(const StringView<char> &key) const {return Value(key, *this);}

		///Converts the value to string
		/**
		 * In compare to getString(), this function converts anything in the variable to a
		 * string representation. Numbers are converted to string, boolean
		 * is converted to "true" or "false", null is returned as word "null". Strings are
		 * returned as they are.
		 *
		 * @note Main difference between toString an strigify is that strings are returned as
		 * they are. The function strigify returns the string with quotes and all non-ascii
		 * characters are escaped
		 *
		 * @return string representation of the value
		 *
		 *
		 *  - null -> stringify() to "null"
		 *  - boolean -> stringify() to true/false
		 *  - number -> stringify() to number
		 *  - undefined -> returned "<undefined>"
		 *  - object -> stringify()
		 *  - array -> strigify()
		 *  - string -> direct value
		 */
		String toString() const ;


		///Performs iteration through all items in the container
		/**
		 * @param fn a function which will be called for each item. The function
		 *  has to accept a single argument - Value. It has to return true to continue
		 *  iteration or false to stop
		 * @retval true iteration processed all the items
		 * @retval false iteration has been stopped
		 *
		 * @code
		 * v.forEach([](Value v) { foo(v) ; return true;});
		 * @endcode
		 */
		template<typename Fn>
		bool forEach(const Fn &fn) const  {
			return v->enumItems(EnumFn<Fn>(fn));
		}

		///Function parses JSON and returns it as Value
		/**
		 * @param source a function which returns next character in a stream (or a string).
		 * The function accepts no arguments and returns char. To emit the end of stream, the function
		 * have to return -1 (0xFF) (which is invalid character in an UTF-8 stream). The function
		 * can throw an exception. It is recommended to throw an exception which implements
		 * ParseError, which allows to include better description about the error
		 * @return parsed JSON as value
		 * @exception ParseError parsing error
		 *
		 * @code
		 * Value::parse([]() { return getchar();});
		 * @endcode
		 *
		 *
		 */
		template<typename Fn>
		static Value parse(Fn &&source);

		///Function parses JSON from string
		/**
		 * @param string any string which can be converted to StringView (see the class description)
		 * @return parsed JSON as value
		 * @exception ParseError parsing error
		 */
		static Value fromString(const StringView<char> &string);
		///Function parses JSON from standard istream
		/**
		 * @param input input stream
		 * @return parsed JSON as value
		 * @exception ParseError parsing error
		 */
		static Value fromStream(std::istream &input);
		///Function parses JSON from C compatible FILE
		/**
		 * @param f input stream
		 * @return parsed JSON as value
		 * @exception ParseError parsing error
		 */
		static Value fromFile(FILE *f);

		///Serializes the value to JSON
		/**
		 * @param target a function which accepts one argument of type char. The function serialize
		 * calls the target for each character that has to be sent to the output. In case that function
		 * is unable to accept more characters, it should throw an exception
		 *
		 * @code
		 * v.serialize([](char c) { putchar(c);});
		 * @endcode
		 *
		 */
		template<typename Fn>
		void serialize(Fn &&target) const;

		///Serializes the value to JSON
		/**
		* @param format Specify how unicode character should be written. 
		* @param target a function which accepts one argument of type char. The function serialize
		* calls the target for each character that has to be sent to the output. In case that function
		* is unable to accept more characters, it should throw an exception
		*
		*/
		template<typename Fn>
		void serialize(UnicodeFormat format, Fn &&target) const;



		///Converts value to JSON string
		/**
		 * @return string contains valid JSON
		 */
		String stringify() const;


		String stringify(UnicodeFormat format) const;

		///Sends the value as JSON string to output stream
		/**
		 * @param output output stream
		 */
		void toStream(std::ostream &output) const;


		void toStream(UnicodeFormat format, std::ostream &output) const;

		///Sends the value as JSON string to C compatible file
		/**
		 * @param f C-compatible file
		 */
		void toFile(FILE *f) const;

		void toFile(UnicodeFormat format, FILE *f) const;

		///Retrieves a smart pointer (PValue) to underlying object
		/**
		 * @return smart pointer to underlying object. Useful only in some special cases.
		 */
		const PValue &getHandle() const { return v; }

		///Compares two variables
		/**
		 *
		 * @param other other variable
		 * @retval true variables contains the equal content. Function compares values including container
		 * so even two values created separately can be considered as equal if they contain same content.
		 * @retval false variables are different
		 * @note function can be slow, because it need to traverse deep into structure and compare
		 * every item in every nested container
		 *
		 *
		 */
		bool operator==(const Value &other) const;
		///Compares two variables
		/**
		 * @param other other variable
		 * @retval false variables contains the equal content. Function compares values including container
		 * so even two values created separately can be considered as equal if they contain same content.
		 * @retval true variables are different
		 * @note function can be slow, because it need to traverse deep into structure and compare
		 * every item in every nested container
		 *
		 */
		bool operator!=(const Value &other) const;


		///Returns iterator to the first item
		/**@note You should be able to iterate through arrays and objects as well */
		ValueIterator begin() const;
		///Returns iterator to the first item
		/**@note You should be able to iterate through arrays and objects as well */
		ValueIterator end() const;

		///Returns whole range
		Range<ValueIterator> range() const;

		///Retrieves true, whether value is defined
		/**
		 * @retval true value is defined
		 * @retval false value is undefined
		 */
		bool defined() const {return type() != undefined;}


		///Returns true if content is null
		bool isNull() const {return type() == null;}

		///Returns true, if contains a value
		/**
		 * The object contains a value, if it is defined and is not null;
		 *
		 * @retval true has value (i.e. defined and not null)
		 * @retval false doesn't have value - undefined or null
		 */
		bool hasValue() const {ValueType t = type(); return t != undefined && t != null;}

		///Returns true, if the value is container
		/** The container is either array or object. It also means, that it can be iteratable.
		 * Non-container values are treat as an empty container. To distinguish between
		 * empty container and non-container value, you can use this function
		 *
		 * @retval true is container
		 * @retval false not container (undefined and null are also non-container values)
		 */
		bool isContainer() const {ValueType t = type(); return t == array || t == object;}


		///Perform operation map on every item of the array or the object
		/** Function works on containers. Result depend on type of value.
		 * If called on object, result is object and values are bound with
		 * same keys.
		 *
		 * @param mapFn function called for every item. The function must return
		 * new value. It can return undefined to remove the value from the
		 * result (note that for arrays it means, that result will have less
		 * items then source).
		 *
		 * @param target_type required container type. Default value is undefined,
		 * which means, that result type will be equal to source. You can specify
		 * json::array or json::object if you need to output different container
		 * type.
		 *
		 * @note if you map to object, every item must have a key.
		 *
		 * @return
		 */
		template<typename Fn>
		Value map(Fn &&mapFn)  const;

		template<typename Fn>
		Value map(Fn &&mapFn, ValueType target_type)  const;



		///Performs reduce operaton on the container
		/**
		 * @param reduceFn function to call on every item
		 * @param initalValue initial value
		 * @return reduced result
		 *
		 * The function has following prototype
		 * @code
		 * Value reduceFn(const Value &total, const Value &currentValue)
		 * @endcode
		 */
		template<typename Fn, typename Total>
		Total reduce(Fn &&reduceFn, Total &&initalValue)  const;

		///Sort object or array
		/** Result of the operation sort is always array (because objects
		 * are sorted by the key)
		 *
		 * @param sortFn sort function
		 * @return Ordered array. Function returns the container as type Ordered which is compatible
		 * with Value, so you can assign result to the Value. However, the Ordered still remembers
		 * the function sortFn and it can be used to invoke operations which require an ordered container
		 *
		 * The function has following prototype
		 * @code
		 * int sortFn(const Value &v1, const Value &v2);
		 * @endcode
		 *
		 * the function returns <0 if v1 < v2, >0 if v1 > v2 or 0 if v1 == v2
		 *
		 */
		template<typename Fn>
		Ordered<Fn> sort(Fn &&sortFn)  const;

		///Reverses container
		Value reverse() const;


		///Replace value at given path (generates new value)
		/**
		 * Function searches the value in the JSON container hierarchy specified by the path
		 * and replaces it by a new value. The result is returned as new value. Function
		 * handles changes in all parent containers.
		 *
		 * @param path path to the value in JSON container hierarchy. If the path
		 * doesn't exists, it is created. if the index in the path doesn't exists,
		 * the new value is added back to the container.
		 * @param val new value replacing the value referenced by the path.
		 *
		 * @return new value contains the change
		 *
		 * @note if the value is marked as diff, it is applied to original value. If there
		 * is no original value, the diff is applied onto empty container
		 *
		 */
		Value replace(const Path &path, const Value &val) const;

		///Replace value at given path (generates new value)
		/**
		 * @param key key to replace
		 * @param val new value
		 * @return
		 */
		Value replace(const StrViewA &key, const Value &val) const;

		///Replace value at given path (generates new value)
		/**
		 * @param index of element in array
		 * @param val new value
		 * @return
		 */
		Value replace(const uintptr_t index, const Value &val) const;

		///Merges two containers
		/** If both values are array, the result is combined array. If both values
		 * are objects, the result is object with merged keys (recursively), where keys from the
		 * other value replaces original value.
		 *
		 * @param other Value of the same type. If the value is different type, result is undefined
		 * @return merged value
		 *
		 * @p @b arrays - result is concatenated array
		 * @p @b objects - result is merged object (per key recursively]
		 * @p @b numbers - result is sum
		 * @p @b strings - result is concatenated string
		 * @p @b bools - result is xor
		 * @p @b nulls - result is null
		 * @p @b undefined - result is undefined
		 */
		Value merge(const Value &other) const;


		///Calculates difference from one value to other
		/**
		 * @p @b arrays - from the current array removes the same values specified by second array, from the end
		 * @p @b objects - creates diff of two objects (per key recusrively)
		 * @p @b numbers - returns substraction this value from other value
		 * @p @b strings - same as array per character
		 * @p @b bools - result is xor
		 * @p @b nulls - result is null
		 * @p @b undefined - result is undefined
		 *
		 * @param other other value
		 * @return if values of the same type are specified, result is calculated otherwise undefined
		 * is result
		 */

		Value diff(const Value &other) const;



		///Splits container into two arrays
		/**
			@param pos position where to split. Positive number specifies how
			many items will be put to left side, other items will be put to right
			side. Negative number specifies how many characters counted from the
			end will be put to right side, other items will be put to left side.

			If absolute number of position is highter then count of items, the
			value is capped.
		*/
		TwoValues splitAt(int pos) const;



		///Combination split+reduce.
		/** Splits rows to groups according to compare function. Note that
		 * source container should be sorted by same order. After split the reduce is performed for each group.
		 * Result is single-dimensional array with reduced results
		 * @param cmp compare function
		 * @param reduce reduce function
		 * @param initVal initial value for each reduce() call
		 * @return array with result
		 */
		template<typename CompareFn, typename ReduceFn, typename InitVal>
		Value group(const CompareFn &cmp, const ReduceFn &reduce, InitVal initVal);

		///Convert Value to user defined type
		/** Template function accepts a name of a type to convert value into.
		 * To achieve correct function, you have to declare a function
		 * ConvValueAs<T>::convert which receives the value. The function must
		 * return converted value
		 *
		 * @return result type
		 */
		template<typename T>
		T as() const {return ConvValueAs<T>::convert(*this);}

		///Convert user defined type to a Value
		/**
		 * @param v a value as user defined type. You have to declare a
		 * function ConvValueFrom<T>::convert which receives the argument and
		 * it must return the value
		 * @return Value from the "v"
		 */
		template<typename T>
		static Value from(const T &v) {return ConvValueFrom<T>::convert(v);}

		///Determines, whether this value is copy of other value
		/** When values are copied, they are shared instead performing the deep copy.
		This function determines, whether the values has been created by copying one
		from the other (so they are shared). In contrast to comparison operator, this
		function just compares internal pointers without comparing of actual values.

		@code
		Value a = 42;
		Value b = 42;
		Value c = a;
		

		bool b1 = a == b; // b1=true
		bool b2 = a == c; // b2=true
		bool b3 = a.isCopyOf(b); // b3=false
		bool b4 = a.isCOpyOf(c); // b4=true
		@endcode

		@param other other object

		@retval true object are the same
		@retval false object are not same (however, they can be still equal)

		@note Some values are statically allocated, so even if they are created separatedly,
		the function can still return true. This is the case of following values:
		 an empty array, an empty object, an empty string, boolean values, null, and number zero

		 @note function is reflective. if a is copy of b, then b is copy of a
		*/
		bool isCopyOf(const Value &other) const {
			return v->unproxy() == other.v->unproxy();
		}

		///Same as isCopyOf but also checks if it is same "key"
		/**
		 * @param other
		 * @return
		 */
		bool isSame(const Value &other) const {
			return v == other.v;
		}

		///Allows to order values by internal address
		/**
		 Every value has internal address. This function allows to make order of
		 values by is internall address. Function can allow to store some values in
		 sets easyier especially objects and array, where comparison of each value can
		 be inefficient. However, ordering doesn't reflect the value.

		 @param other other object

		 @retval true this object is before the other object
		 @retval false this object is not before the other object, or they are the same.

		 @note Some values are statically allocated, so even if they are created separatedly,
		 the function can still return false, because they are considered as the same. 
		 This is the case of following values:
		 an empty array, an empty object, an empty string, boolean values, null, and number zero

		*/
		bool before(const Value &other) const {
			return v->unproxy() < other.v->unproxy();
		}


		///Parse binary format
		/** Parses and reconstructs JSON-Value from binary stream. The binary stream is
		 * not standardised. You can use binary format to transfer values through  various
		 * IPC tools, such a pipes, shared memory or sockets. Binary format is optimized for
		 * speed, so it is not ideal to be transfered through the network (except fast local area network,
		 * or localhost)
		 *
		 * @param fn function which returns next byte in the stream
		 * @param enc specifies encoding for binary items. Original encoding is not stored in
		 * binary format, so the parser must know, which encoding need to be assigned to
		 * binary items. If not specified, then defaultBinaryEncoding is used. This value
		 * defaults to base64
		 *
		 * @return value reconstructed value
		 *
		 * @code
		 * unsigned char fn();
		 * #endcode
		 *
		 * @note the function is able to transfer all value types including "undefined" It also
		 * supports arrays where values are bound with a key. However, only the string keys are
		 * supported.
		 */
		template<typename Fn>
		static Value parseBinary(const Fn &fn, BinaryEncoding enc = defaultBinaryEncoding);



		///Writes values to the binary stream
		/** result binary stream is not by any standard. It is only recoginzed by the function parseBinary().
		 * You can use binary format to transfer values through  various
		 * IPC tools, such a pipes, shared memory or sockets. Binary format is optimized for
		 * speed, so it is not ideal to be transfered through the network (except fast local area network,
		 * or localhost)
		 *
		 *
		 * @param fn function which receives a byte to write to the output stream
		 * @param flags combination of flags: compressKeys and  maintain32BitComp
		 * @code
		 * void fn(unsigned char c);
		 * @endcode
		 */

		template<typename Fn>
		void serializeBinary(const Fn &fn, BinarySerializeFlags flags = compressKeys) const;

		///Compare two jsons
		/**
		 *
		 * @param a left value
		 * @param b right value
		 * @retval <0 left is less then right
		 * @retval >0 left is greater then right
		 * @retval ==0 left is equal to right
		 */
		static int compare(const Value &a, const Value &b);


		static const std::size_t npos = (std::size_t)-1;

		///Finds first occurence of first value in the container
		/**
		 * @param v value to search
		 * @param start starting position
		 * @return index of found item, or npos if not found
		 */
		std::size_t indexOf(const Value &v, std::size_t start = 0) const;
		///Finds occurence of first value in the container
		/**
		 * @param v value to search
		 * @param start starting position
		 * @return index of found item, or npos if not found
		 */
		std::size_t lastIndexOf(const Value &v, std::size_t start = npos) const;


		///Shifts all items of the container left and returns first item
		/** The function modifies the variable, but it doesn't modifies original
		 * array. So after return, current variable contains shifted sequence and
		 * the first item is returned
		 *
		 * @return The first item removed from the array.
		 *
		 * If the value is not container, the function returns undefined
		 *
		 * @note The function works with objects.
		 *
		 * @note complexity O(N)
		 */
		Value shift();


		///Inserts item at begin of container
		/**
		 * The function modifies current variable, but it doesn't modifies
		 * original conatiner. If the current value is object, result is stored as array
		 *
		 * @param item item to insert at the beging
		 * @return count of items in the result

		 * @note complexity O(N)

		 */
		UInt unshift(const Value &item);

		///Inserts items at begin of container
		/**
		 * The function modifies current variable, but it doesn't modifies
		 * original conatiner. If the current value is object, result is stored as array
		 *
		 * @param start start iterator
		 * @param end end iterator
		 * @return count of items in the result
		 *
		 * @note complexity O(N)
		 *
		 */
		template<typename Iter1, typename Iter2>
		UInt unshift(const Iter1 &start, const Iter2 &end);

		///Inserts items at begin of container
		/**
		 * The function modifies current variable, but it doesn't modifies
		 * original conatiner. If the current value is object, result is stored as array
		 *
		 * @param items list of items
		 * @return count of items in the result
		 */
		UInt unshift(const std::initializer_list<Value> &items);

		///Removes item from the pop and returns it
		/** Note:due the immutable nature, the function only modifies current variable,
		 * but not the original container. Current variable receives rest of the
		 * array while the last item is returned as result
		 *
		 * @return removed item.
		 *
		 * @note also works with the objects. If the
		 * @note complexity O(N)
		 */
		Value pop();

		///Appends one item
		/**
		 *
		 * @param v item to put
		 * @return count of items in final container
		 *
		 *
		 * @note complexity O(N)
		 *@note complexity O(N)
		 * @note due the immutable nature, the function only modifies current variable,
		 * but not the original container.
		 */
		UInt push(const Value &v);


		String join(StrViewA separator=",") const;

		Value slice(Int start) const;
		Value slice(Int start, Int end) const;


		///Returns the value of the first element in the array that satisfies the provided testing function. Otherwise undefined is returned.
		/**
		 * @param fn function which receives the item. The function must return true or false to accept or reject item
		 * @return found item or undefined
		 *
		 * @note the function can have one, two or three arguments
		 *    - 1st argument is element of type Value
		 *    - 2nd argument is index
		 *    - 3rd argument is this object
		 */
		template<typename Fn>
		Value find(Fn &&fn) const;

		///Returns the value of the last element in the array that satisfies the provided testing function. Otherwise undefined is returned.
		/**
		 * @param fn function which receives the item. The function must return true or false to accept or reject item
		 * @return found item or undefined
		 * @note the function can have one, two or three arguments
		 *    - 1st argument is element of type Value
		 *    - 2nd argument is index
		 *    - 3rd argument is this object
		 */
		template<typename Fn>
		Value rfind(Fn &&fn) const;

		///returns the index of the first element in the array that satisfies the provided testing function. Otherwise -1 is returned.
		/**
		 * @see find
		 */
		template<typename Fn>
		Int findIndex(Fn &&fn) const;

		///returns the index of the last element in the array that satisfies the provided testing function. Otherwise -1 is returned.
		/**
		 * @see find
		 */
		template<typename Fn>
		Int  rfindIndex(Fn &&fn) const;

		///The filter() method creates a new array with all elements that pass the test implemented by the provided function.
		/**
		 *
		 * @param fn Function is a predicate, to test each element of the array. Return true to keep the element, false otherwise. It accepts one, two or three arguments
		 *    - 1st argument is element of type Value
		 *    - 2nd argument is index
		 *    - 3rd argument is this object
		 *
		 * @return new array or object
		 *
		 * @note if the function is called on a object, the result is also the object
		 */
		template<typename Fn>
		Value filter(Fn &&fn) const;


		template<typename Fn>
		void walk(Fn &&fn) const;


		///Returns number or default value, if the object is not number
		double getValueOrDefault(double defval) const {return type() == json::number?getNumber():defval;}
		///Returns number or default value, if the object is not number
		float getValueOrDefault(float defval) const {return type() == json::number?static_cast<float>(getNumber()):defval;}
		///Returns number or default value, if the object is not number
		unsigned int getValueOrDefault(unsigned int defval) const {return type() == json::number?static_cast<unsigned int>(getUInt()):defval;}
		///Returns number or default value, if the object is not number
		unsigned long getValueOrDefault(unsigned long defval) const {return type() == json::number?static_cast<unsigned long>(getUInt()):defval;}
		///Returns number or default value, if the object is not number
		unsigned long long getValueOrDefault(unsigned long long defval) const {return type() == json::number?static_cast<unsigned long long>(getUIntLong()):defval;}
		///Returns number or default value, if the object is not number
		signed int getValueOrDefault(signed int defval) const {return type() == json::number?static_cast<signed int>(getInt()):defval;}
		///Returns number or default value, if the object is not number
		signed long getValueOrDefault(signed long defval) const {return type() == json::number?static_cast<signed long>(getInt()):defval;}
		///Returns number or default value, if the object is not number
		signed long long getValueOrDefault(signed long long defval) const {return type() == json::number?static_cast<signed long long>(getIntLong()):defval;}
		///Returns boolean or default value, if the object is not boolean
		bool getValueOrDefault(bool defval) const {return type() == json::boolean?getBool():defval;}
		///Returns string or default value, if the object is not string
		String getValueOrDefault(String defval) const ;
		///Returns string or default value, if the object is not string
		const char *getValueOrDefault(const char *defval) const ;
		///Returns string or default value, if the object is not string
		std::string getValueOrDefault(const std::string &defval) const {return type() == json::string?std::string(getString()):defval;}
		///Returns string or default value, if the object is not string
		StrViewA getValueOrDefault(const StrViewA &defval) const {return type() == json::string?getString():defval;}
		///Converts this to default value, if the type of the value is not expected
		Value getValueOrDefault(const Value &defval) const {return type() == defval.type()?*this:defval;}
		///Converts this to default value, if the type of the value is not expected
		/** @code
		 * getValueOrDefault(json::array) - returns array, or empty array
		 * @return
		 */
		Value getValueOrDefault(const ValueType &defval) const {return type() == defval?*this:Value(defval);}

public:

		///Pointer to custom allocator
		/** You can change allocator to achieve better results with allocation 
		   of values of the JSON. By default, standard new and delete is used.
		   This variable is global. By changing it, your allocator must
		   expect, that some objects are already allocated and will be deallocated
		   through your deallocator.		   
		*/
		static const Allocator *allocator;



protected:

		PValue v;

		friend class Object;
		friend class Array;

		static Value numberFromStringRaw(StrViewA str, bool force_double);

		//std::vector<PValue> prepareValues(const std::initializer_list<Value>& data);
	};

	inline static std::istream &operator >> (std::istream &stream, Value &v) {
		v = Value::fromStream(stream);
		return stream;
	}
	inline static std::ostream &operator << (std::ostream &stream, const Value &v) {
		v.toStream(stream);
		return stream;
	}

	///Simple iterator
	/** Iterator can be slower then the forEach() function, because
	 * it need to go through the virtual interface for each item.
	 *
	 * @code
	 * for(Value item: v) { /... work with item .../ }
	 * @endcode
	 *
	 *@note You should be able to iterate through arrays and objects as well
	 */
	class ValueIterator {
	public:
		Value v;
		UInt index;

		ValueIterator(Value v,UInt index):v(v),index(index) {}
		Value operator *() const {return v[index];}
		ValueIterator &operator++() {++index;return *this;}
		ValueIterator operator++(int) {++index;return ValueIterator(v,index-1);}
		ValueIterator &operator--() {--index;return *this;}
		ValueIterator operator--(int) {--index;return ValueIterator(v,index+1);}

		bool operator==(const ValueIterator &other) const {
			return (other.atEnd() && atEnd())
					|| (index == other.index && v.isCopyOf(other.v));
		}
		bool operator!=(const ValueIterator &other) const {
			return !operator==(other);
		}
		///Returns whether iterator points to the end
		bool atEnd() const {
			return index >= v.size();
		}
		bool atBegin() const {
			return index == 0;
		}
		ValueIterator &operator+=(int p) {index+=p;return *this;}
		ValueIterator &operator-=(int p) {index-=p;return *this;}
		ValueIterator operator+(int p) const {return ValueIterator(v,index+p);}
		ValueIterator operator-(int p) const {return ValueIterator(v,index-p);}
		Int operator-(ValueIterator &other) const {return index - other.index;}

		typedef std::random_access_iterator_tag iterator_category;
	    typedef Value        value_type;
	    typedef Value *        pointer;
	    typedef Value &        reference;
	    typedef Int  difference_type;

	};


	class InvalidNumericFormat: public std::exception {
	public:
		InvalidNumericFormat(std::string &&val);
		const std::string &getValue() const;
		const char *what() const noexcept override;
	protected:
		std::string val;
		mutable std::string msg;
	};


	///Just for fun
	/**
	 * @code
	 * void foo(json::var x) {
	 * 	json::var y = x["bar"];
	 * 	//...
	 * }
	 * @endcode
	 */
	typedef Value var;

	class KeyKeeper : public StrViewA {
	public:
		explicit KeyKeeper(const StrViewA &k) :StrViewA(k) {}
		Value operator=(const Value &v) const { return Value(*this, v); }
	};

	class KeyStart {
	public:
		KeyKeeper operator/(const StrViewA &a) const { return KeyKeeper(a); }
		KeyKeeper operator()(const StrViewA &a) const {return KeyKeeper(a);}

	};
	extern KeyStart key;

	template<typename InputIterator, typename Fn>
	Value::Value(ValueType type, InputIterator begin, InputIterator end, Fn &&mapFn, bool skipUndef) {
		std::vector<Value> buffer;
		while (begin != end) {
			buffer.push_back(mapFn(*begin));
			++begin;
		}
		(*this) = Value(type,StringView<Value>(buffer.data(), buffer.size()),skipUndef);
	}

	template<typename Fn>
	void Value::walk(Fn &&fn) const {
		bool enter = fn(*this);
		if (enter && isContainer()) {
			for (Value v : *this) {
				v.walk(fn);
			}
		}
	}

}



namespace std {
template<> struct hash<::json::Value> {
	size_t operator()(const ::json::Value &v) const;
};
}



#ifndef IMTJSON_NOKEYLITERAL
static inline json::KeyKeeper operator"" _(const char *k, std::size_t len) {
	return json::KeyKeeper(json::StrViewA(k, len));
}
#endif


#include "conv.h"

