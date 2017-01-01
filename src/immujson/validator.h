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



namespace json {


class Validator {
public:
	virtual ~Validator() {}


	static StrViewA strString;
	static StrViewA strNumber;
	static StrViewA strBoolean;
	static StrViewA strAny;
	static StrViewA strBase64;
	static StrViewA strBase64url;
	static StrViewA strHex;
	static StrViewA strUppercase;
	static StrViewA strLowercase;
	static StrViewA strIdentifier;
	static StrViewA strCamelCase;
	static StrViewA strAlpha;
	static StrViewA strAlnum;
	static StrViewA strDigits;
	static StrViewA strInteger;
	static StrViewA strNative;
	static StrViewA strNull;
	static StrViewA strOptional;

	static StrViewA strGreater;
	static StrViewA strGreaterEqual;
	static StrViewA strLess;
	static StrViewA strLessEqual;
	static StrViewA strMinSize;
	static StrViewA strMaxSize;
	static StrViewA strKey;
	static StrViewA strToString;
	static StrViewA strToNumber;
	static StrViewA strPrefix;
	static StrViewA strSuffix;
	static StrViewA strSplit;
	static StrViewA strAll;
	static StrViewA strNot;

	static char valueEscape;
	static char commentEscape;


	///Evaluates the native rule
	/**
	 * Native rule is declared as "native" in class table. The function
	 *
	 * @param ruleName name of the rule
	 * @param args arguments if the rule. Array is always here
	 * @param subject the item it is subject of validation
	 */
	virtual bool onNativeRule(const Value &subject, const StrViewA & ruleName) { return false; }



	///Validate the subject
	/**
	 *
	 * @param subject item to validate
	 * @param rule Name of rule to use (probably class).
	 * @param path path to the subject. Default value expects root of the document. You can specify path if you are validating
	 *  a part of the document.
	 * @retval true validated
	 * @retval false not valid
	 */
	bool validate(const Value &subject,const StrViewA &rule = StrViewA(), const Path &path = Path::root);


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


	bool validateInternal(const Value &subject,const StrViewA &rule);

	bool evalRuleSubObj(const Value & subject, const Value & rule, const StrViewA & key);

	bool evalRuleSubObj(const Value & subject, const Value & rule, unsigned int index);

	bool evalRuleSubObj(const Value & subject, const Value & rule, unsigned int index, unsigned int offset);

	bool evalRule(const Value & subject, const Value & ruleLine);

	bool evalRuleWithParams(const Value & subject, const Value & rule);

	bool opRangeDef(const Value & subject, const Value & rule, std::size_t offset);

	bool evalRuleArray(const Value & subject, const Value & rule, unsigned int tupleCnt);

	bool evalRuleAlternatives(const Value & subject, const Value & rule, unsigned int offset);

	bool evalRuleSimple(const Value & subject, const Value & rule);

	
	///Definition
	Value def;


	Array rejections;
	Value lastRejectedRule;

	///current path (for logging)
	const Path *curPath;


	bool evalRuleObject(const Value & subject, const Value & templateObj);

	bool checkClass(const Value& subject, StrViewA name);


	bool opPrefix(const Value &subject, const Value &args);

	bool opSuffix(const Value &subject, const Value &args);
	bool opSplit(const Value &subject, std::size_t at, const Value &left, const Value &right);


	void addRejection(const Path &path, const Value &rule);
};



}




#endif /* IMMUJSON_VALIDATOR_H_ */
