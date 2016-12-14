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
 *
 * Structure definition:
 *
 * allowed types
 * [type,type,type,type,....,type-with-arg,[args],....] - list of types alloved for the value
 *
 * types:
 *
 *   "null" - null is allowed (not null but "null")
 *   "number" - numbers are allowed
 *   "string" - strings are allowed
 *   "object" - any object
 *   "array" - any array
 *   "boolean" - boolean
 *
 * types with arguments
 *
 *   "number:" - ["<>", number, number,   - define valid range
 *               ,"(>", number, number,   - define valid range
 *               ,"<)", number, number,   - define valid range
 *               ,"()", number, number,   - define valid range
 *               ,">" , number            - define numbers below
 *               ,"<" , number,           - define numbers above
 *               ,"(" , number,           - define numbers above
 *               ,")" , number,           - define numbers below
 *               ,number,... ]            - specified numbers
 *   "set:" - [val, val, val, val ]       - define set of specified values
 *   "wildset:" - [val, val, val ]        - if val is string, it can have wildcards * and ?
 *   "array":    ["type","type",....]     - allowed types
 *   "tuple":    ["type","type",["type",type"],...]  - defines type for exac position. If more types are allowed, then array must be used at specifed place
 *   "optional"   - specifies, that field, which is specified at current position is optinal.
 *                                       - this can be used for member fields and tuples.
 *
 *   "#name"                             - tags current definition. The tag can be later used instead the definition to avoid
 *                                           repeating. It is also possible to make recursive definitions
 *
 *   object validation
 *   {
 *       "field":"type",
 *       "field":["type","type",.....]
 *       "field":["optional","type],
 *       "field": {... object ... },
 *       "field":["optional",{ ... object ...}]
 *   }
 *
 *   example:
 *
 *   [#person,
 *	   {
 *  	    "first name":"string",                          //first name is string
 *	        "second name":"string",                         //second name is string
 *  	    "age":["number:",["<>",0,150]],                 //age is integer number in specified range
 *      	"children":["array:","#person"],                //children is array of persons
 *      	"parents":["tuple:",["#person","#person"]],     //parents are two persons
 * 	  }
 *   ]
 *
 *
 *
 *
 *
 *
 *
 *
 */


namespace json {




}




#endif /* IMMUJSON_VALIDATOR_H_ */
