/*
 * validator.h
 *
 *  Created on: 14. 12. 2016
 *      Author: ondra
 */

#ifndef IMMUJSON_VALIDATOR_H_
#define IMMUJSON_VALIDATOR_H_


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
 "strnum"   - string, which can be converted to a number
 "integer"  - number without decimal dot
 {...}       - object with validation
 null       - null value
 "native"   - declares, that rule is evaluated by user defined function. This
                keyword can be used only along with class. If
				such rule is evaluated, the apropriate call back function
				is called. The declaration only checks, that such native 
				function exists.

 Constants in definition
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

"aaa|bbb|ccc" - Definition of enumeration type (one of strings are allowed
                  above definition is equivalent to ["'aaa","'bbb","'ccc"]
"aaa,bbb,ccc" - Definition of set type: Equivalent to
                       "set:",["aaa","bbb","ccc"]
[val1, val2 ]   - define range, the value must be between val1 and val2
                    it must be same type. This definition must be used inside 
					braced type definition.
					"age":[[0,120]],   - allows single range 0-120
					"ranges":[[0,100],[200,500],null]  - allows given ranges or null

[val, null ] - define value equal to val and above
[null, val]  - define value equal to val and below

Definitions with argument

If the name of type ends with a collon, additional argument is expected, so next item
in the array is not interpreted as type

["def1:",arg,"def2:", arg, "def3", [args...] ]

"array:","type"  - define array with items of given type
"array:",["types",...] - define array, which can contain specified types (classes or constants)
"set:",[val,val, val...] - define array, which can contain just only given values
"assoc:","type" - define associative array which is defined as object, without
           fixed structure. Keys are not validated
"assoc:",["types",...] - define associative array which is defined as object, without
           fixed structure. Keys are not validated
"assoc:",{ advanced assoc definition }



"tuple:",[type1,type2,type3...] - defines tuple. For each item, the type is defined. 
          There can be also multiple types, or classes or values
		     "tuple:",["number",["string",null],[1,2,3],["boolean",null],["optional","number"]]
	     Above example defines tuple, with following arguments:
		    first argument is number
			second argument is string or null
			third argument is one of numbers
			fourth argument is boolean or null  
			fifth argument is optional and if specified, it must be number

"():",[val,val]  - defines range where border numbers are excluded
"<):",[val,val] - defines range where lower value is included but upper is excluded
"(>:",[val,val] - defines range where lower value is excluded but upper is included
"<>:",[val,val] - equivalent to [val,val]
"value:",val - requires value, it can accept string without starting hash
"max-size:",number - if the item can be measured for the size, the rule is checked, otherwise skipped
                     ["string","max-size:",10] - string of maximum 10 characters
					 ["number","string","max-size:10] - for number, the max-size is skipped
"min-size:",number - if the item can be measured for the size, the rule is checked, otherwise skipped
                     ["array:","number","max-size:",2] - an array with two or more items
"key:","type" - the rule is accepted only when the associated key is given type. This
                  rule is useful only with associated arrays. To use with
				  type of value, you should define it as a class

}


					
Nested rules validation.

To achieve rule nesting, you can define classes with complicated conditions. If
one of condition is also class, it is evaluated and acceped or rejected as whole.
For example you need to define array, where there are strings up to 10 characters and
numbers in range 1..5. You cannot achieve this by single definition

below example will not work
"array:",[ [1,5], "string", "max-size:", 10]

because the rule "max-size:" is applied to whole array, not the item. 
However, you can define classes:

{
 "String10":["string","max-size:",10],
 "Number1..5":[[1,5]],
 "": ["array:",["String10", "Number1..5" ] ],
}

Above example works, because classes "String10" and "Number1..5" are checked for
each item in the array.


Classes with arguments

A class name with trailing double collon also accepts an argument. The argument
can be used instead the type

"$*" - whole argument
"$1" - if argument is array, then use the first item



 */


namespace json {




}




#endif /* IMMUJSON_VALIDATOR_H_ */
