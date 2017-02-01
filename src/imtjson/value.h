#pragma once

#include <vector>
#include "ivalue.h"

namespace json {

	class Array;
	class Object;
	class Path;
	class PPath;
	class ValueIterator;
	class String;
	class Binary;
	class Allocator;
	template<typename T> class ConvValueAs;
	template<typename T> class ConvValueFrom;

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
		 */
		Value(ValueType type, const StringView<Value> &values);

		///Initialize the variable using initializer list {....}
		/**
		 * This allows to use initializer list {...} to initialize the variable.
		 * @code
		 * Value v = {10,20,"hello", null, {"subarray", 21, true } };
		 * @endcode
		 *
		 * @param data argument is created by compiler.
		 */
		Value(const std::initializer_list<Value> &data);


		///Create binary value
		/** Binary values are not supported by JSON. They are emulated through encoding
		 *
		 * @param binary binary content
		 */
		Value(const BinaryView &binary, BinaryEncoding enc = base64);


		///Create binary value
		/** Binary values are not supported by JSON. They are emulated through encoding
		 *
		 * @param binary binary content
		 */
		Value(const Binary &binary);

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
		std::uintptr_t getUInt() const { return v->getUInt(); }
		///Retrieve signed integer
		/**
		 * @return value as signed integer
		 * @note Automatic conversion is limited to numbers only. This conversion is transparent. If
		 * there is floating point number stored, the function getInt() returns only the integer part
		 * of the number. The function returns zero for other types.
		 */
		std::intptr_t getInt() const { return v->getInt(); }
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
		 */
		std::size_t size() const { return v->size(); }
		///Determines whether value is an empty container
		/**
		 * @retval false the object or the array is not empty
		 * @retval true the object or the array is empty, or the value is neither an array nor an object.
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
		Value setKey(const StringView<char> &key) const;
		Value setKey(const char *k) const {return setKey(StrViewA(k));}
		Value setKey(const std::string &k) const {return setKey(StrViewA(k));}


		///Binds a key-name to the item
		/** The function is used by objects, however you can freely bind any value to a specified key outside of the object.
		 * @param key-name which is bind to the value.
		 * @return new value with bound key.
		 *
		 * @note due the immutable nature of the  value, you cannot change or set
		 * the key-name to the existing value. A new value is always created.
		 *
		 * @see getKey
		 *
		 * @note function shares the object String. It is faster if you already have the key represented as String
		 */
		Value setKey(const String &key) const;

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
		static Value parse(const Fn &source);

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
		void serialize(const Fn &target) const;

		///Serializes the value to JSON
		/**
		* @param format Specify how unicode character should be written. 
		* @param target a function which accepts one argument of type char. The function serialize
		* calls the target for each character that has to be sent to the output. In case that function
		* is unable to accept more characters, it should throw an exception
		*
		*/
		template<typename Fn>
		void serialize(UnicodeFormat format, const Fn &target) const;



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

		///Retrieves true, whether value is defined
		/**
		 * @retval true value is defined
		 * @retval false value is undefined
		 */
		bool defined() const {return type() != undefined;}


		///Returns true if content is null
		bool isNull() const {return type() == null;}

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
		 * @return
		 */
		template<typename Fn>
		Value map(const Fn &mapFn)  const;

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
		Total reduce(const Fn &reduceFn, Total initalValue)  const;

		///Sort object or array
		/** Result of the operation sort is always array (because objects
		 * are sorted by the key)
		 *
		 * @param sortFn sort function
		 * @return sorted array
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
		Value sort(const Fn &sortFn)  const;

		///Reverses container
		Value reverse() const;


		///Merges to array
		/**
		 * @param other other container
		 * @param mergeFn function which compares each items and performs merge item per item
		 *
		 * The function mergeFn has following prototype
		 * @code
		 * int mergeFn(const Value &left, const Value &right);
		 * @endcode
		 *
		 * Where left and right are items to compare. Function need to pick one ot items and
		 * perform some operation which is not part of the function itself. It can
		 * for example store the item to the file
		 *
		 * The function returns <0 to advance left iterator, >0 to advance right iterator or
		 * =0 to advance both iterators
		 *
		 * @note Both container should be sorted
		 */
		template<typename Fn>
		void merge(const Value &other, const Fn &mergeFn) const;

		///Merges to array
		/**
		 * @param other other conatiner
		 * @param mergeFn function which compares each items and performs merge item per item
		 * @return merged array
		 *
		 * The function mergeFn has following prototype
		 * @code
		 * int mergeFn(const Value &left, const Value &right, Array &collector);
		 * @endcode
		 *
		 * Where left and right are items to compare, and collector is object where the
		 * function has to put result of the merge operation
		 *
		 * The function returns <0 to advance left iterator, >0 to advance right iterator or
		 * =0 to advance both iterators
		 *
		 * @note Both container should be sorted
		 */
		template<typename Fn>
		Value mergeToArray(const Value &other, const Fn &mergeFn) const;

		///Merges to object
		/**
		 * @param other other conatiner
		 * @param mergeFn function which compares each items and performs merge item per item
		 * @return merged object
		 *
		 * The function mergeFn has following prototype
		 * @code
		 * int mergeFn(const Value &left, const Value &right, Object &collector);
		 * @endcode
		 *
		 * Where left and right are items to compare, and collector is object where the
		 * function has to put result of the merge operation
		 *
		 * The function returns <0 to advance left iterator, >0 to advance right iterator or
		 * =0 to advance both iterators
		 */
		template<typename Fn>
		Value mergeToObject(const Value &other, const Fn &mergeFn) const;



		///Removes duplicated values after sort
		/** Function expects sorted result. It removes duplicated values */
		template<typename Fn>
		Value uniq(const Fn &sortFn) const;


		///Splits container to an array of sets of values that are considered as equal according to sortfn
		/**
		 * - Example: [12,14,23,25,27,34,36,21,22,78]
		 * - Result (split by tens) [[12,14],[23,25,26],[34,36],[21,22],[78]]
		 *
		 * @param sortFn the function which compares two values and returns zero if they are
		 * equal and nonzero if not equal
		 * @return split result
		 */
		template<typename Fn>
		Value split(const Fn &sortFn) const;

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


		///Makes intersection of two containers
		/**
		 * @param other second container
		 * @param sortFn function which defines order of values.
		 * @return array contains intersection of both containers
		 *
		 * @note both containers must be sorted. Use this function along with sort() if
		 * necessary.
		 */
		template<typename Fn>
		Value makeIntersection(const Value &other, const Fn &sortFn) const;

		///Makes union of two containers
		/**
		 * @param other second container
		 * @param sortFn function which defines order of values.
		 * @return array contains union of both containers. Duplicate keys are stored
		 *
		 *
		 * @note both containers must be sorted. Use this function along with sort() if
		 * necessary.
		 */
		template<typename Fn>
		Value makeUnion(const Value &other, const Fn &sortFn) const;

		///Combination split+reduce.
		/** Splits rows to groups according to compare function. Note that
		 * source container should be sorted by same order. After split the reduce is performed for each group.
		 * Result is single-dimensional array with reduced results
		 * @param cmp compare function
		 * @param reduce refuce function
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
		static Value parseBinary(const Fn &fn);


		///Writes values to the binary stream
		/** result binary stream is not by any standard. It is only recoginzed by the function parseBinary().
		 * You can use binary format to transfer values through  various
		 * IPC tools, such a pipes, shared memory or sockets. Binary format is optimized for
		 * speed, so it is not ideal to be transfered through the network (except fast local area network,
		 * or localhost)
		 *
		 *
		 * @param fn function which receives a byte to write to the output stream
		 * @code
		 * void fn(unsigned char c);
		 * @endcode
		 */

		template<typename Fn>
		void serializeBinary(const Fn &fn);

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
		std::uintptr_t index;

		ValueIterator(Value v,std::uintptr_t index):v(v),index(index) {}
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
}
#include "conv.h"

