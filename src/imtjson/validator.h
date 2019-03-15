/*
 * validator.h
 *
 *  Created on: 14. 12. 2016
 *      Author: ondra
 */



#ifndef IMMUJSON_VALIDATOR_H_
#define IMMUJSON_VALIDATOR_H_
#include <vector>
#include <set>
#include "path.h"
#include "stringview.h"
#include "string.h"
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
	static StrViewA strUndefined;
	static StrViewA strDateTimeZ;
	static StrViewA strDate;
	static StrViewA strTimeZ;
	static StrViewA strTime;
	static StrViewA strEmpty;
	static StrViewA strNonEmpty;

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
	static StrViewA strExplode;
	static StrViewA strAll;
	static StrViewA strAndSymb;
	static StrViewA strNot;
	static StrViewA strNotSymb;
	static StrViewA strDateTime;
	static StrViewA strSetVar;
	static StrViewA strUseVar;
	static StrViewA strEmit;

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
	virtual bool onNativeRule(const Value &, const StrViewA &  ) { return false; }



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
	bool validate(const Value &subject,const StrViewA &rule = StrViewA("_root"), const Path &path = Path::root);


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

	///Default version
	/** Because version 2 is complete different, to compile version 1 schema set defaultVersion=1 at the beginning of the code
	 *
	 */
	static int defaultVersion;




protected:


	bool validateInternal(const Value &subject,const StrViewA &rule);

	bool evalRuleSubObj(const Value & subject, const Value & rule, const StrViewA & key);

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

	bool checkClass(const Value& subject, StrViewA name);


	bool opPrefix(const Value &subject, const Value &args);

	bool opSuffix(const Value &subject, const Value &args);
	bool opSplit(const Value &subject, std::size_t at, const Value &left, const Value &right);


	void addRejection(const Path &path, const Value &rule);
	
	void pushVar(String name, Value value);
	void popVar();
	Value findVar(const StrViewA &name, const Value &thisVar);
	Value getVar(const Value &path, const Value &thisVar);


	bool opSetVar(const Value &subject, const Value &args);
	bool opUseVar(const Value &subject, const Value &args);
	bool opCompareVar(const Value &subject, const Value &rule);
	bool opEmit(const Value &subject, const Value &args);
	Value walkObject(const Value &subject,  const Value &v);
	bool opExplode(const Value& subject, StrViewA str, const Value& rule, const Value& limit);

	//--------------------- v2 --------------------------------------------


	struct State {
		Value subj;
		Value path;

		State setSubj(Value subj) const {return State {subj,path};}
		State enter(Value newNode, Value newPath) const;
	};

	bool v2validate(Value subj, Value rule);
	bool processRule(const State &state, Value rule, bool sequence = false);
	bool processAlternateRule(const State &state, Value rule);

	std::size_t mark_state();
	bool failure(std::size_t mark);
	bool report(const State &state, Value rule, bool result);
	void report_exception(const State &state,Value rule, const char *what);



	Value collapseObjRule(Value rule,std::set<StrViewA> &visited);
	bool processObjectRule(const State &state,Value rule);
	bool processSequence(const State &state, Value rule);

};




}




#endif /* IMMUJSON_VALIDATOR_H_ */
