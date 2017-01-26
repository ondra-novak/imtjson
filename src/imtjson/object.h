#pragma once

#include <map>
#include <functional>
#include "value.h"
#include "stackProtection.h"
#include "edit.h"
#include "container.h"

namespace json {





	class ObjectValue;
	class ObjectDiff;

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

		Object(const Object &other);

		Object(Object &&other);

		Object &operator=(const Object &other);
		Object &operator=(Object &&other);

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
		const Value operator[](const StringView<char> &name) const;

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

		Object2Object object(const StringView<char> &name);
		Array2Object array(const StringView<char> &name);

		///Optimize current object
		/** By default, items added to the object are not ordered. It is
		 * not necesery when the items are only put to it. However, if
		 * you need to lookup for items, the object must be optimized
		 *
		 * Optimization is taken often performed automatically.
		 *
		 * Complexity is N.log(N)
		 */
		void optimize() const;

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

		///Returns count of items in the object
		/**
		 * @note function can be inaccurate because it counts duplicated items and
		 * items to remove. Use this value as a hint
		 *
		 * @return count of items in the container
		 */
		std::size_t size() const;

		///Merges two objects
		/** Function inserts items from the object specified by the argument to this object
		which can replace existing items or add some new ones. The result is merge
		of two objects 
		
		@param object second object, which replaces all existing keys in current object
		@return reference to this to create chains

		*/
		Object &merge(Value object);



		///Creates iterator to walk through all changes
		/**@note Iterator walks through changed object. Note that processing changes for iteration
		 * can reduce performance in compare to iteration through the base object (using Value::begin() and
		 * Value::end()) even if there are no changes made yet. The iterator must explore the changes
		 * and apply all of them to make iteration-able set
		 *
		 */
		ValueIterator begin() const;
		ValueIterator end() const;

		///Allows to change the base object
		/** This helps to modify multiple objects using the same change-set. Just create
		 * change-set on single object and then change the base object to apply changes on them
		 * and you can repeat this for multiple objects.
		 * @param object
		 */
		void setBaseObject(Value object) {
			base = object;
		}


		///Inserts changes which are need to change from oldObject to the newObject
		/**
		 * @param oldObject source object
		 * @param newObject target object
		 * @param recursive specifies how deep the recursion must go. Objects beyond the limit
		 * are not stored as diff, but they are stored as one replaces other complete
		 */
		void createDiff(const Value oldObject, Value newObject, unsigned int recursive = 0);

		///Function called to resolve conflicts
		/** @return function returns resolved value.
		 * @param 1 Path contains relative path to in the JSON structure (valid for both values)
		 * @param 2 left value. It can be undefined in case that value is going to be erased
		 * @param 3 right value. It can be undefined in case that value is going to be erased
		 *
		 * Function can return any value to put to the final diff, or undefined to remove the value
		 */
		typedef std::function<Value(Path, Value, Value)> ConflictResolver;

		//@{
		///Picks two diffs and merges them into single diff
		/** Function can be used to perform three-way merge. First you have to create diff
		 * between current revision and common (base) revision. Do this for both sides of the conflict.
		 * Then call this function and supply some conflict resolver, which is called when
		 * both values have the same key
		 *
		 * @param left diff for first value (will appear at left)
		 * @param right diff for second value (will appear at right)
		 * @param resolver function called to resolve conflict
		 * @param path specifies base path for the resolver (you can adjust if you need to add prefix to the path)
		 *
		 * @note depend on type of values can happen
		 *   - two values of different type -> resolver called
		 *   - two non-object values -> resolver called
		 *   - two objects -> resolver called
		 *   - two diff-objects -> diffs are merged recursively
		 *   - one non-diff-object an one diff-object -> diff is applied
		 *   - diff-object and non-object -> resolver called, however you should only choose who wins
		 */
		void mergeDiffs(const Object &left, const Object &right, const ConflictResolver &resolver);
		void mergeDiffs(const Object &left, const Object &right, const ConflictResolver &resolver,const Path &path);
		//@}


		template<typename Fn>
		Object map(Fn mapFn) const;

		template<typename T, typename ReduceFn>
		T reduce(const ReduceFn &reduceFn, T init) const ;

		template<typename Cmp>
		Array sort(const Cmp &cmp) const;

		///Direct access to the items
		/** Function retrieves iterable view of items if the source value is Object
		 *
		 * @note Function only supports build-in object type.
		 *
		 * @return View to items. Items are stored as PValues, so you still need
		 * to convert them to Values.
		 *
		 * @p tip - Items in object are ordered by its key
		 *
		 * */
		static StringView<PValue> getItems(const Value &v);

	protected:

		///Base object which contains an original items
		Value base;
		///Latest snapshot used for iterations
		mutable Value lastIterSnapshot;
		///Ordered part of container
		/** for large objects, this contains already ordered items for faster lookup
		 * The container is created when first lookup is requested
		 */
		mutable RefCntPtr<ObjectValue> ordered;
		///Unordered part of container
		/** contains item just added for append to replace original items
		 * The container is not ordered and can contain duplicates.
		 */
		mutable RefCntPtr<ObjectValue> unordered;
		

		///Creates special object which is used to store a difference between two objects
		/**
		 * @return Retuned object is standard object however it can contain "undefined"
		 * fields. These fields are exists and they have name, but undefined value. They
		 * will appear while iteration, so this is the reason, why this method is protected.
		 *
		 * You can determine, whether the object is a diff using the function isObjectDiff()
		 */
		Value commitAsDiff() const;

		ObjectValue *commitAsDiffObject() const;

		///Applies the diff-object to some other object and returns object with applied diff
		/**
		 *
		 * @param baseObject source object
		 * @param diffObject diff-object create using commitAsDiff. However, the function
		 * will work with an ordinary object, only without ability to remove keys (because
		 * there is no standard way how to write "remove action" into standard object
		 * @return new object with applied changes
		 *
		 */
		static Value applyDiff(const Value &baseObject, const Value &diffObject);


		template<typename It,typename It2, typename Fn>
		static void mergeDiffsImpl(It lbeg, It lend, It2 rbeg, It2 rend, const ConflictResolver &resolver,const Path &path, const Fn &setFn);

		static Value mergeDiffsObjs(const Value &lv,const Value &rv, const ConflictResolver& resolver, const Path &path);


private:
	void set_internal(const Value& v);
};


}

