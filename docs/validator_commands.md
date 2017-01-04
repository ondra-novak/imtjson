# Validator rules reference

## Simple rules

 - **string** - string expected
 - **number** - number expected
 - **boolean** - boolean expected
 - **any** - any value is expected (must be defined), this includes also **null**
 - **base64** - a string which is valid base64 expected
 - **base64url** - a string which is valid base64 for url is expected
 - **hex** - string cointaining a hex number
 - **uppercase** - string having all character uppercase
 - **lowercase** - string having all character lowercase
 - **identifier** - a string starting with a letter or underscore followed by letters, numbers and underscore
 - **camelcase** - an identifier, which is not starting by uppercase letter
 - **alpha** - only letters
 - **alnum** - letters and digits
 - **digits** - digits expected
 - **integer** - expected number is integer
 - **native** - native rule (defined in C++)
 - **optional** - value is not defined (it often appear with other rules, so this rule is not accepted when value is defined)
 - **datetime** - date and time stored as YYYY-MM-DDThh:mm:ssZ
 - **datetimez** - date and time stored as YYYY-MM-DDThh:mm:ssZ
 - **date** - date stored as YYYY-MM-DD
 - **time** - time stored as hh:mm:ss
 - **timez** - time stored as hh:mm:ssZ


## Functional rules

### minsize 
```
["minsize",<number>]
```
If used with a container, it allows only a container with count of items up to specified limit. If used
with string it allows strings limited by specified count of characters. You should use this rule inside "all"
rule

```
["all","string", ["minsize",10] ]
```

### maxsize 
```
["maxsize",<number>]
```
If used with a container, it allows only a container with count of items above or equal to specified limit. If used
with string it allows strings larger or equal to specified count of characters. You should use this rule inside "all"
rule

```
["all","string", ["maxsize",10] ]
```

### key
```
["key", rules...]
```
Check rules for key (instead the value)

```
["key", ["all", "alpha", ["maxsize",2], ["minsize",2]  ] ]
```
Above example allows keys having two letters. 

### tostring
```
["tostring", rules...]
```
Convert value to string and validate it agains rules inside

###tonumber
```
["tonumber", rules...]
```

Convert value to a number and validate it agains rules inside. If the value cannot be converted, rule is rejected

###prefix
```
["prefix", "..."]
["prefix", [...]]
["prefix", "...", rules...]
["prefix", [...], rules...]
```

The rule is accepted only if the value has specified prefix (array or string). If there are rules,
they are validated against rest of the value (prefix is removed)

###suffix
```
["suffix", "..."]
["suffix", [...]]
["suffix", "...", rules...]
["suffix", [...], rules...]
```

The rule is accepted only if the value has specified suffix (array or string). If there are rules,
they are validated against rest of the value (suffix is removed)

###split
```
["split", pos, rule_left, rule_right]
```

Splits value (string or array) at **pos** position and validates each side agains rule_left or rule_right

###range operator
```
[">", v]
["<", v]
[">=", v]
["<=", v]
[">", v1, "<", v2,...]
[">", v1, "<", v2,... rules...]
```

Allows to define range of values. The function works similar as "all", ranges can be defined as comparison operator
and value. Other rules, can be put there while the rule works as "all" rule (all rules must be accepted)

###all
```
["all", rule, rule,...]
```
The rule is accepted only when all rulles specified inside this rule are accepted

###not
```
["not", rule, rule,...]
```
The rule is accepted if none inside rule is accepted.

###datetime
```
["datetime", "format"]
```

Custom datetime format. The format must match exact to the value, otherwise the rule is not accepted. The format
is defined as string, where some letters has a special meaning
 - **Y** - year. 
 - **M** - month
 - **D** - day
 - **h** - hour
 - **m** - minute
 - **s** - second
 - **c** - milisecond
 - **O** - Timezone offset Hour
 - **o** - Timezone offset Minute
 - **+** - sign character (+ or -)
 
 Each letter loads single letter from the source. You will need tp double these letters to retrieve two
 digits for specified item: YY,MM,DD etc... For full year, you need to put Y four times: YYYY
 
 The rule checks ranges for each item. It also tracks leap years. It needs to load a full year YYYY to achieve this feature.
 
 Examples:
 ```
 ["datetime","YYYYMMDDhhmmss"]
 ["datetime","YYYY-MM-DD hh:mm:ss"]
 ["datetime","YYYY/MM/DD hh/mm/ss"]
 ["datetime","YYYY/MM"]
 ```
 
 Trick: Any number having exactly four digits and two decimals: ["datetime","cccc.cc"]
 
 Trick: Birthnumber (Czech personal ID): ["datetime","cccccc/cccc"]
 
 
 
 
