#pragma once

#include "value.h"
#include "stackProtection.h"
#include <map>

namespace json {


	class Object2Object;
	class Array2Object;

	///The class helps to build JSON object
	/** To build or modify JSON object, construct this class. Then you can add, remove
	or replace members. All changes are recorded by the instance of this class. Once
	you are done with modification, you can pass this instance to the constructor of 
	the class Value. This makes changes immutable. Original source object is not changed.
	*/

	class Object : public StackProtected {
	public:
		///Create instance from existing object
		/** 
		@param value instance of Value, which should have type equal to object. The value
		is used as template to build new object
		*/
		Object(Value value);
		///Create instance of empty object
		Object();
		///Create instance of empty object and add first member
		/**
		This constructor makes syntax more eye-candy. You can chain multiple parentheses to
		   add multiple members inline.

		@param name name of first member
		@param value value of first member

		@code
		Value v = Object("m1","v1")
		                ("m2","v2")
						...;
		@endcode		
		*/
		Object(const StringView<char> &name, const Value &value);
		///Destructor - performs cleanup, discards any changes.
		~Object();

		///Sets member to value
		/**
		 @param name name of key
		 @param value value of member
		 @return reference to this object to allow to chain functions
		*/
		Object &set(const StringView<char> &name, const Value &value);
		///Set new member
		/**
		This function can be used if you need to copy the whole pair key-value from 
		to this object. It expects, that you have key-value-pair. It can
		be obtained from any Value of object type, and inside forEach loop.

		@param value value paired with key. You should be able to retrieve key using the
			function Value::getKey(). If the value is not key-value-pair, it is
			stored under empty key (or replaces empty key)
		*/
		Object &set(const Value &value);
		///Retrieves value under given key
		/**
		@param name name of key to retrieve
		@return value stored under given key. Function can ask original object as well as
		the modified version. Modified version has priority. If the key doesn't exists or
		has been deleter, the function returns undefined
		*/
		Value operator[](const StringView<char> &name) const;

		///Removes key
		/** Function removes already inserted key as well as the key that was defined on
		the original object. This is achieved to save "undefined" under specified key. The
		undefined values are skipped during commit()
		*/
		Object &unset(const StringView<char> &name);


		///Sets member to value
		/**
		@param name name of key
		@param value value of member
		@return reference to this object to allow to chain functions
		*/
		Object &operator()(const StringView<char> &name, const Value &value);

		///Commints all changes and creates new Value
		/** This function is used by Value constructor. It is better to
		pass whole Object to the Value constructor, which performs all necesery operations
		to create new object. The constructor also call function commit() 
		
		@return Smart pointer to newly created value which contains object and
		all members added to this object. The function commit is declared as const, so it
		doesn't modify the content of current instance
		
		*/
		PValue commit() const;

		//@{
		///Allows to create subobject or subobject of subobject inline or pass it to a function
		/**
			The function creates special object which inherits Object and which is
			programmed to perform commit operation during destruction. As result, this
			allows you to define members in a subobject and commit changes to the parent
			member automatically. There is only one condition:  destroy
			this subobject before the parent object is commited

			@param name name of the key of the member where the subobject will be stored. If 
			the member already exists, it is used to initialize the Object as well as the constructor
			of this class does, so you is able to modify the subobject.

			Some examples

			@code
			Object x;
			x.subobject("test")
			    ("first":1)
				("second":2);
			@endcode
			Above code will create {"test":{"first":1,"second":2}}

			@code
			Object x;
			x.subobject("test").subobject("other")("third",{1,2});
			@endcode
			Above code will create {"test":{"other":{"third":[1,2]}}}

			@code
			Object x;
			{  //don't forget section here
			   auto sub(x.subobject("test"));
			   x.set("first":1);
			   x.set("second":2);
			}
			@endcode
			Above code will create {"test":{"first":1,"second":2}}

			@code
			Object x;
			makeSubobject(x.subobject("test").subobject("other"))
			@endcode
			Allows to delegate creation of subobject into a function. The
			function can be declared as 			
			@code
			void makeSubobject(Object &&obj);
			@encode

			Following code is not allowed and will cause run_time exception
			@code
			Object x;
			{  
				auto sub(x.subobject("test").subobject("test2"); //<-- ERROR
				x.set("first":1);
				x.set("second":2);   
			} <-- Exception
			@endcode
			Above code is errnous, because it creates temporary object which is immediately
			destroyed - it is created for "test" member. Subobject "test2" then
			refers already destroyed object. There is some kind of protection which
			doesn't allow to commit changes to already destroyed object. To workaround
			this limitation, you have to declare each subobject as standalone variable,
			that should be destroyed in the reversed order

		*/
		Object2Object object(const StringView<char> &name);
		Array2Object array(const StringView<char> &name);
		//@}

		///clears the object
		void clear();

		///revert all changes
		void revert();

		///determines dirty status (modified)
		/**
		@retval true some modifications has been recorded
		@retval false no modifications recorded.
		*/
		bool dirty() const;

		///Merges two objects
		/** Function inserts items from the object specified by parameter to this object
		which can replace existing items or add some new ones. The result is merge
		of two objects 
		
		@param object second object, which replaces all existing keys in current object
		@return reference to this to create chains

		*/
		Object &merge(Value object);

	protected:
		Value base;
		typedef std::map<StringView<char>, PValue> Changes;
		
		Changes changes;

	};


}