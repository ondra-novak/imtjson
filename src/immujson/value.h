#pragma once

#include <vector>
#include "ivalue.h"

namespace json {

	class Array;
	class Object;
	class Path;
	class PPath;
	class ValueIterator;

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
		///Initialize the variable as array of values
		/**
		 * @param value refernce to a string view containing values
		 */
		Value(const StringView<Value> &value);
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
		 * Items in the object are kept as pair which contains the key and the value. Every time
		 * the item is retrieved, it also carries its original key until the key is changed by
		 * putting the item into other object (to achieve immutability, the key is not actually
		 * changed, but inserted item is paired with a new key, so it is possible to have
		 * the item with many paired keys)
		 *
		 * @return name of key. If the value is not paired with a key, return is empty string. To
		 * determine whether value is paired see @proxy
		 */
		StringView<char> getKey() const { return v->getMemberName(); }


		///Converts the value to string
		/**
		 * In compare to getString(), this function converts anything in the variable to a
		 * string representation. It doesn mean, that numbers are converted to string, boolean
		 * is converted to "true" or "false", null is returned as "null".
		 *
		 * @return string representation of the value
		 */
		std::string toString() const ;


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

		///Converts value to JSON string
		/**
		 * @return string contains valid JSON
		 */
		std::string stringify() const;

		///Sends the value as JSON string to output stream
		/**
		 * @param output output stream
		 */
		void toStream(std::ostream &output) const;
		///Sends the value as JSON string to C compatible file
		/**
		 * @param f C-compatible file
		 */
		void toFile(FILE *f) const;

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

	protected:

		PValue v;

		friend class Object;
		friend class Array;

		std::vector<PValue> prepareValues(const std::initializer_list<Value>& data);
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
		bool operator==(const ValueIterator &other) const {
			return index == other.index && v.getHandle() == other.v.getHandle();
		}
		bool operator!=(const ValueIterator &other) const {
			return !operator==(other);
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
