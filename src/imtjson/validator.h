/*
 * validator.h
 *
 *  Created on: 14. 12. 2016
 *      Author: ondra
 */

#ifndef IMMUJSON_VALIDATOR_H_
#define IMMUJSON_VALIDATOR_H_
#include <vector>
#include "path.h"
#include "string.h"
#include "value.h"



namespace json {


class Validator {
public:
	virtual ~Validator() {}


	static std::string_view strString;
	static std::string_view strNumber;
	static std::string_view strBoolean;
	static std::string_view strAny;
	static std::string_view strBase64;
	static std::string_view strBase64url;
	static std::string_view strHex;
	static std::string_view strUppercase;
	static std::string_view strLowercase;
	static std::string_view strIdentifier;
	static std::string_view strCamelCase;
	static std::string_view strAlpha;
	static std::string_view strAlnum;
	static std::string_view strDigits;
	static std::string_view strInteger;
	static std::string_view strUnsigned;
	static std::string_view strNative;
	static std::string_view strNull;
	static std::string_view strOptional;
	static std::string_view strUndefined;
	static std::string_view strDateTimeZ;
	static std::string_view strDate;
	static std::string_view strTimeZ;
	static std::string_view strTime;
	static std::string_view strEmpty;
	static std::string_view strNonEmpty;
	static std::string_view strObject;
	static std::string_view strArray;

	static std::string_view strGreater;
	static std::string_view strGreaterEqual;
	static std::string_view strLess;
	static std::string_view strLessEqual;
	static std::string_view strMinSize;
	static std::string_view strMaxSize;
	static std::string_view strKey;
	static std::string_view strToString;
	static std::string_view strToNumber;
	static std::string_view strPrefix;
	static std::string_view strSuffix;
	static std::string_view strSplit;
	static std::string_view strExplode;
	static std::string_view strAll;
	static std::string_view strAndSymb;
	static std::string_view strNot;
	static std::string_view strNotSymb;
	static std::string_view strDateTime;
	static std::string_view strSetVar;
	static std::string_view strUseVar;
	static std::string_view strEmit;

	static char valueEscape;
	static char commentEscape;
	static char charSetBegin;
	static char charSetEnd ;



	///Evaluates the native rule
	/**
	 * Native rule is declared as "native" in class table. The function
	 *
	 * @param ruleName name of the rule
	 * @param args arguments if the rule. Array is always here
	 * @param subject the item it is subject of validation
	 */
	virtual bool onNativeRule(const Value &, const std::string_view &  ) { return false; }



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
	bool validate(const Value &subject,const std::string_view &rule = std::string_view("_root"), const Path &path = Path::root);


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


	///Sets variables
	/**
	 *
	 * @param varList container with variables (object).
	 *
	 * Function replaces current variables
	 */
	void setVariables(const Value &varList);


	Value getEmits() const;

protected:


	bool validateInternal(const Value &subject,const std::string_view &rule);

	bool evalRuleSubObj(const Value & subject, const Value & rule, const std::string_view & key);

	bool evalRuleSubObj(const Value & subject, const Value & rule, unsigned int index);

	bool evalRuleSubObj(const Value & subject, const Value & rule, unsigned int index, unsigned int offset);

	bool evalRule(const Value & subject, const Value & ruleLine);

	bool evalRuleWithParams(const Value & subject, const Value & rule);

	bool opRangeDef(const Value & subject, const Value & rule, std::size_t offset);

	bool evalRuleArray(const Value & subject, const Value & rule, unsigned int tupleCnt, unsigned int offset);

	bool evalRuleAlternatives(const Value & subject, const Value & rule, unsigned int offset);

	bool evalRuleSimple(const Value & subject, const Value & rule);

	
	///Definition
	Value def;


	std::vector<Value> rejections;
	std::vector<Value> emits;

	Value lastRejectedRule;

	///current path (for logging)
	const Path *curPath;



	typedef std::vector<Value> VarList;
	VarList varList;


	bool evalRuleObject(const Value & subject, const Value & templateObj);

	bool checkClass(const Value& subject, std::string_view name);


	bool opPrefix(const Value &subject, const Value &args);

	bool opSuffix(const Value &subject, const Value &args);
	bool opSplit(const Value &subject, std::size_t at, const Value &left, const Value &right);


	void addRejection(const Path &path, const Value &rule);
	
	void pushVar(String name, Value value);
	void popVar();
	Value findVar(const std::string_view &name, const Value &thisVar);
	Value getVar(const Value &path, const Value &thisVar);


	bool opSetVar(const Value &subject, const Value &args);
	bool opUseVar(const Value &subject, const Value &args);
	bool opCompareVar(const Value &subject, const Value &rule);
	bool opEmit(const Value &subject, const Value &args);
	Value walkObject(const Value &subject,  const Value &v);
	bool opExplode(const Value& subject, std::string_view str, const Value& rule, const Value& limit);


};



}




#endif /* IMMUJSON_VALIDATOR_H_ */
