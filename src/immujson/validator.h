/*
 * validator.h
 *
 *  Created on: 14. 12. 2016
 *      Author: ondra
 */

#ifndef IMMUJSON_VALIDATOR_H_
#define IMMUJSON_VALIDATOR_H_
#include "array.h"
#include "path.h"
#include "stringview.h"
#include "value.h"


/*
 * JSON structure validation
 *
 * Two jsons, first defines the structure, second JSON is tested, whether it matches the definition
 *

 The validation structure is an object:

 {
    "rule-name-1":[.... rules...],
	"rule-name-2":[.... rules...],
	....
	"" : [... main rules...]

 }

 If not otherwise specified, the main rule is the rule with the empty name. However,
 the validator can be configured to validate agains other rule.

 The name of the rule can be any string expept some reserved words, that will be described
 below. It is recomended to start the name with capital, because it is similar to a class
 definition.

 {
    "Person" : [..... ],
 }

 The rules can be described using following types:

  - object {...} - it says, that the class (name + rules) is always object.
  - string - where the string is name of a type or class. Specifies which type od data can be stored here
  - number - it requires exact value at given place
  - boolean - it requires exact value at given place
  - null - it requires null at given place
  - array - primarly allows to specify multiple types or classes. For example, the
             definition ["string","number"] allows to have at the place string or number.
			 You can use classes as well: ["Person","Subject"]

 {
   "Person": {
               "name":"string",
			   "age":"number",
			   "children":["array:","Person","optional"],
			   "parents":["tuple:",["Person","Person"],"Person","optional],

             }
 }

 Above example defines Person as object, which has up to four fields: "name", "age",
 "children", "parents", where children and parents are optional. The "name" is a string,
 the "age" is a number. The field "children" is array of Person. Using the rule
 recursively is allowed. The field "children" is optional, so it can be ommited. The
 field "parents" can be tuple (array) with two Persons, or single Person (object) and
 it is also optional

 Table of single types
 "string"   - a string
 "number"   - a number
 "boolean"  - a boolean
 "array"    - any array (content is not validated)
 "object"   - any object (content is not validated)
 "any"      - any value of any type
 "base64"   - base64 string
 "hex"      - hex number
 "uppercase" - uppercase string
 "lowercase" - lowercase string
 "identifier" - C/C++ identifier
 "camelcase" - camelcase string
 "alpha"     - just alphabetic string
 "alnum"     - just apha or numbers
 "strnum"   - string, which can be converted to a number
 "integer"  - number without decimal dot
 {...}       - object with validation
 null       - null value
 "native"   - declares, that rule is evaluated by user defined function. This
                keyword can be used only along with class. If
				such rule is evaluated, the apropriate call back function
				is called. The declaration only checks, that such native 
				function exists.
"$N"        - Nth argument of user class invokation

 Constants in definition
 true       - true must be there
 false      - false must be there
 123        - specified number must be there

 "'string"  - putting single quote before the string defines string constant"
                     "Offer":{
					            "type": "'offer",
								....
					         } 
			  Above example requires to have "offer" in the field type, otherwise
			    the rule is rejected. This constant can be used instead the
				type declaraction. If a value is expected, than the single
				quote is part of the string.
				 Equivalent: "type":["value:","offer"],


Definitions with argument

putting the array into definition allows parametrized definition

["type1","type2",["type_with_arg",arg1,arg2,arg3,...],"type3",...]


["array",rule]       - specify allowed types of items in the array

["set",val1,val2,val3 ...]   - array which works as set. You can put to the array only specified values

["enum",val1,val2,val3 ...]   - one of specified value

["object",{..template..}, type]  - more generic object validation
                                      - template is object's template.
                                      - type is default type for items not specified in the template, It is allowed to
                                         have multiple types in [...]

["tuple",valpos1,valpos2,valpos3, ...]  - array, where every position has defined type (or a value).
                                   ["tuple","number","string",["number",null],["boolean","optional"] ] ->
                                   -> [10,"xxx",20], [23,"abc",null], [42,"cde",null,true], ...

["vartuple",valpos1,valpos2,valpos3,...,valposN] - same as tuple, but source can have more items. The last type specifies
                                type of remaining items.
                                ["vartuple","string","integer"] allows ["aaa",1] and ["aaa",1,2], but not ["aaa"]. If you
                                need such function, you have to add "optional" to last rule
                                  ["vartuple","string",["integer","optional"]]




["()", val1, val2 ]           - opened interval (val1 = null menas -infinity, val2 = null means +infinity)
["()", null, val2 ]           - (infinity, val2)
["()", val1, null ]           - (val1, infinity)
["<)", val1, val2 ]           - opened left interval, closed right interval
["(>", val1, val2 ]           - closed left interval, opened right interval
["<>", val1, val2 ]           - closed interval
["value", val] -              - exact value.
["max-size", number ]         - the item is rejected, when its size is above the specified number
["min-size", number ]         - the item is rejected, when its size is below the specified number
["key", rule] - the key must be of given type.
                                     Because the keys are always strings, only string derived
                                     types or string values or string ranges can be used.

Transormations

["to-string",rule ]            - converts item to string and validates against rules in arguments
["to-number",rule ]            - converts item to number and validates against rules in arguments
["suffix", "xxx", rule ]       - string needs to have suffix "xxx". Rest of string is validated against rules in arguments
["prefix", "xxx", rule ]       - string needs to have prefix "xxx". Rest of string is validated against rules in arguments
                                   example: ["prefix","flag","uppercase"] -> accepted: "flagXXX", rejected: "flag123", rejected: "fooBar"
                                          -> "prefix(flag,uppercase)


Shortcuts

"type(arg,arg,arg)"  -> ["type","arg","arg","arg"]

"key('foo)"   -> ["key","'foo"]
"array(string)" ->["array","string"]
"set(aaa,bbb,ccc)" -> ["set","aaa","bbb","ccc"]
"enum(aaa,bbb,ccc)" -> ["enum","aaa","bbb","ccc"]
"type1,type2,type3" -> ["type1","type2","type3"]
"string,array(number)" -> ["string",["array","number"] ]


all these shortcuts above all expanded as strings. They cannot be used with non-string parts. For examle "max-size(10)"
will not work. The shortcut can be used as long as whole definition stays strings only. Each part of the
string should not contain comma ',' and parenthesis

["array",["()",0,10]] - cannot be written as shortcut
["key",["max-size",3]] - cannot be written as shortcur


Nesting is possible
"array(array(number))" - 2D array of numbers -> ["array",["array","number"]]

*/

namespace json {


class Validator {
public:
	virtual ~Validator() {}


	///Evaluates the native rule
	/**
	 * Native rule is declared as "native" in class table. The function
	 *
	 * @param ruleName name of the rule
	 * @param args arguments if the rule. Array is always here
	 * @param subject the item it is subject of validation
	 */
	virtual bool onNativeRuleAccept(const Value &subject,const StrViewA & ruleName, const Value &args);

	virtual bool onNativeRuleReject(const Value &subject,const StrViewA & ruleName, const Value &args);


	///Validate the subject
	/**
	 *
	 * @param subject item to validate
	 * @param rule Name of rule to use (probably class).
	 * @param args optionals arguments for the rule
	 * @param path path to the subject. Default value expects root of the document. You can specify path if you are validating
	 *  a part of the document.
	 * @retval true validated
	 * @retval false not valid
	 */
	bool validate(const Value &subject,const StrViewA &rule = StrViewA(), const Value &args = {}, const Path &path = Path::root);


	///Constructs validator above validator-definition (described above)
	Validator(const Value &definition);

	///Retrieves array rejections
	/**
	 * Each item contains
	 *  [path, rule-def]
	 *
	 *  the path is presented as json array
	 *  the rule-def is either while rule line, or specific rule, which returned rejection
	 *
	 *  The whole rule-line is rejected when no rule accept the item. Then whole rule-line is reported.
	 *  If one of rules rejected the rule-line, then rejecting rule is outputed
	 *
	 * @return The array can contain more rejection if the format allows multiple ways to validate the document. The
	 * document is rejected when there is no way to accept the document.
	 */
	Value getRejections() const;


protected:


	bool validateInternal(const Value &subject,const StrViewA &rule, const Value &args);


	///Definition
	Value def;


	Array rejections;

	///current path (for logging)
	const Path *curPath;
	///Arguments for current class (should be an array)
	const Value *curArgs;


	bool validateRuleLine(const Value& subject, const Value& ruleLine);
	bool validateRuleLine2(const Value& subject, const Value& ruleLine, unsigned int offset);
	bool validateObject(const Value& subject, const Value& ruleLine, const Value& extraRules);
	bool validateSingleRuleForAccept(const Value& subject, const Value& ruleLine);
	bool validateSingleRuleForReject(const Value& subject, const Value& ruleLine);
	bool evalRuleAccept(const Value& subject, StrViewA name, const Value& args);
	bool evalRuleReject(const Value& subject, StrViewA name, const Value& args);
	bool checkClassAccept(const Value& subject, StrViewA name, const Value& args);
	bool checkClassReject(const Value& subject, StrViewA name, const Value& args);

	bool checkKey(const Value &subject, const Value &args);

	bool opPrefix(const Value &subject, const Value &args);

	bool opSuffix(const Value &subject, const Value &args);
	bool opTuple(const Value &subject, const Value &args, bool varTuple);

};



}




#endif /* IMMUJSON_VALIDATOR_H_ */
