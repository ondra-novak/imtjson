#pragma once

#include <sstream>
#include <cmath>
#include <assert.h>
#include "object.h"
#include "array.h"
#include "utf8.h"

namespace json {


	///enables parsing numbers as precise numbers
	/** this option is global for whole application. If you need specify
	 * option locally, construct Parse instance with correct flag
	 */
	extern bool enableParsePreciseNumbers;


	template<typename Fn>
	class Parser {
	public:

		typedef std::size_t Flags;

		///Allows duplicated keys in object
		/** Duplicated keys are considered as invalid JSON. This option allows to relax the rule. Note
		 * that according to JSON standard, when duplicated keys are allowed, the last key is only stored
		 * within the object
		 *
		 *  */
		static const Flags allowDupKeys = 1;
		///Parse numbers as precise
		/** Precise numbers are parsed as they are and stored as string. This is transparent
		 * for the rest of the code. Precise numbers emits special flag preciseNumber. The
		 * function Value::getString() returns the actual form of the number. If such
		 * number is serialized, it is serialized 1:1 to its actual form
		 *
		 */
		static const Flags allowPreciseNumbers = 2;


		Parser(const Fn &source, Flags flags = 0) :rd(source),flags(flags) {}

		virtual Value parse();
		Value parseObject();
		Value parseArray();
		Value parseTrue();
		Value parseFalse();
		Value parseNull();
		Value parseNumber();
		Value parseString();

		void checkString(const StringView<char> &str);

	

	protected:

		typedef std::pair<std::size_t, std::size_t> StrIdx;

		StrViewA getString(StrIdx idx) {
			return StrViewA(tmpstr).substr(idx.first, idx.second);
		}

		StrIdx readString();
		void freeString(const StrIdx &str);

		class Reader {
			///source iterator
			Fn source;
			///temporary stored char here
			int c;
			///true if char is loaded, false if not
			/** It could be possible to avoid such a flag if the commit()
			  performs preload of the next character. However this can be a trap.
			  If the character '}' of the top-level object is the last character
			  in the stream before it blocks, the commit() should not read the
			  next character. However it must be commited, because the inner parser
			  doesn't know anything about on which level operates.

			  Other reason is that any preloaded character is also lost when the
			  parser exits. And this preloaded character might be very important for
			  any following code.
			  */
			    
			bool loaded;


		public:
			int next() {
				//if char is not loaded, load it now
				if (!loaded) {
					//load
					c = source();
					//mark loaded
					loaded = true;
				}
				//return char
				return c;
			}

			int nextWs() {
				char n = next();
				while (isspace(n)) { commit(); n = next(); }
				return n;
			}

			void commit() {
				//just mark character not loaded, which causes, that next() will load next character
				loaded = false;
			}

			int nextCommit() {
				if (loaded) {
					loaded = false;
					return c;
				} else {
					return source();
				}
			}

			void putBack(int c) {
				loaded = true;
				this->c = c;
			}

			///slightly faster read, because it returns character directly from the stream
			/**
			 * Note that previous character must be commited"
			 * @return next character in the stream;
			 */

			int readFast() {
				return source();
			}

			Reader(const Fn &fn) :source(fn), loaded(false) {}

		};

		Reader rd;

		///parses unsigned from the stream
		/**
		@param res there will be stored result
		@param counter there will be stored count of digits has been read
		@retval true number is complete, no more digits in the stream
		@retval false number is not complete, parser stopped on integer overflow
		*/	    
		bool parseUnsigned(std::uintptr_t &res, int &counter);
		///parses integer and stores result to the double
		/** By storing result into double causes, that final number will rounded
		  @param intPart already parsed pard by parseUnsigned
		  @return parsed number as double
		  @note function stops on first non-digit character (including dot)
		*/
		double parseLargeUnsigned(std::uintptr_t intPart);
		///Parses decimal part
		/**
			works similar as parseLargeUnsigned, but result is number between 0 and 1
			(mantisa). Function stops on first non-digit character (including dot, so
			dot must be already extacted)

			@return parsed mantisa
		*/
		double parseDecimalPart();
		///Parse number and store it as floating point number
		/**
		 @param intpart part parsed by parseUnsigned. It can be however 0 to start
		 from the beginning
		 @param neg true if '-' sign has been extracted.
		 @return parsed value
		 @note function doesn't extract leading sign '+' or '-' it expects that
		 caller already used readSign() function
		*/
		Value parseDouble(std::uintptr_t intpart, bool neg);
		///Reads sign '+' or '-' from the stream
		/**
		@retval true there were sign '-'.
		@retval false there were sign '+' or nothing
		*/
		bool readSign();

		///Parses unicode sequence encoded as \uXXXX, stores it as UTF-8 into the tmpstr
		void parseUnicode();
		///Stores unicode character as UTF-8 into the tmpstr
		void storeUnicode(std::uintptr_t uchar);


		StrIdx parsePreciseNumber(bool &hint_is_float);

		///Temporary string - to keep allocated memory
		std::vector<char> tmpstr;

		///Temporary array - to keep allocated memory
		std::vector<Value> tmpArr;

		Flags flags;
	};


	class ParserHelper {
		template<typename Fn>
		friend class Parser;
		static Value numberFromStringRaw(StrViewA str, bool force_double);
	};

	class ParseError:public std::exception {
	public:
		ParseError(std::string msg, int lastInput = -100) :msg(msg), lastInput(lastInput) {}

		virtual char const* what() const throw() {
			if (whatmsg.empty()) {
				std::ostringstream fmt;
				fmt << "Parse error: '" + msg + "' at <root>";
				std::size_t pos = callstack.size();
				while (pos) {
					--pos;
					fmt << "/" << callstack[pos];
				}
				if (lastInput != -100) {
					fmt << ". Last input: " << lastInput << "(";
					if (lastInput == -1) {
						fmt << "EOF";
					} else {
						char c = (char)lastInput;
						fmt << "'" << c << "'";
					}
					fmt << ").";
				} else {
					fmt<< ". Last input not given.";
				}

				whatmsg = fmt.str();

			}
			return whatmsg.c_str();
		}

		void addContext(const std::string &context) {
			whatmsg.clear();
			callstack.push_back(context);
		}


	protected:
		std::string msg;
		std::vector<std::string> callstack;
		mutable std::string whatmsg;
		int lastInput;


	};


	template<typename Fn>
	inline Value Value::parse(const Fn & source)
	{
		Parser<Fn> parser(source, enableParsePreciseNumbers?Parser<Fn>::allowPreciseNumbers:0);
		return parser.parse();
	}

	template<typename Fn>
	inline Value Parser<Fn>::parse()
	{
		int c = rd.nextWs();
		switch (c) {
			case '{': rd.commit(); return parseObject();
			case '[': rd.commit(); return parseArray();
			case '"': rd.commit(); return parseString();
			case 't': return parseTrue();
			case 'f': return parseFalse();
			case 'n': return parseNull();
			case -1: throw ParseError("Unexpected end of stream",c);
			default: if (isdigit(c) || c == '+' || c == '-' || c == '.')
							return parseNumber();
						else
							throw ParseError("Unexpected data",c);

		}
	}

	template<typename Fn>
	inline Value Parser<Fn>::parseObject()
	{
		std::size_t tmpArrPos = tmpArr.size();
		int c = rd.nextWs();
		if (c == '}') {
			rd.commit();
			return Value(object);
		}
		StrIdx name(0,0);
		bool cont;
		do {
			if (c != '"') 
				throw ParseError("Expected a key (string)", c);
			rd.commit();
			try {
				name = readString();
			}
			catch (ParseError &e) {
				e.addContext(getString(name));
				throw;
			}
			try {
				c = rd.nextWs();
				if (c != ':')
					throw ParseError("Expected ':'", c);
				rd.commit();
				Value v = parse();
				tmpArr.push_back(Value(getString(name),v));
				freeString(name);
			}
			catch (ParseError &e) {
				e.addContext(getString(name));
				freeString(name);
				throw;
			}
			c = rd.nextWs();
			rd.commit();
			if (c == '}') {
				cont = false; 
			}
			else if (c == ',') {
				cont = true;
				c = rd.nextWs();
			} 
			else {
				throw ParseError("Expected ',' or '}'", c);
			}
		} while (cont);		
		StringView<Value> data = tmpArr;
		auto mydata=data.substr(tmpArrPos);
		Value res(object, mydata);
		if (((flags & allowDupKeys) == 0) && (res.size() != mydata.length)) {
			throw ParseError("Duplicated keys",c);
		}
		tmpArr.resize(tmpArrPos);
		return res;
	}

	template<typename Fn>
	inline Value Parser<Fn>::parseArray()
	{
		std::size_t tmpArrPos = tmpArr.size();
		int c = rd.nextWs();
		if (c == ']') {
			rd.commit();
			return Value(array);
		}
		bool cont;
		do {
			try {
				tmpArr.push_back(parse());
			}
			catch (ParseError &e) {
				std::ostringstream buff;
				buff << "[" << (tmpArr.size()-tmpArrPos) << "]";
				e.addContext(buff.str());
				throw;
			}
			try {
				c = rd.nextWs();
				rd.commit();
				if (c == ']') {
					cont = false;
				}
				else if (c == ',') {
					cont = true;
				}
				else {
					throw ParseError("Expected ',' or ']'", c);
				}
			}
			catch (ParseError &e) {
				std::ostringstream buff;
				buff << "[" << (tmpArr.size()-tmpArrPos) << "]";
				e.addContext(buff.str());
				throw;
			}
		} while (cont);
		StringView<Value> arrView(tmpArr);
		Value res(arrView.substr(tmpArrPos));
		tmpArr.resize(tmpArrPos);
		return res;
		
	}

	template<typename Fn>
	inline Value Parser<Fn>::parseTrue()
	{
		checkString("true");
		return Value(true);
	}

	template<typename Fn>
	inline Value Parser<Fn>::parseFalse()
	{
		checkString("false");
		return Value(false);
	}

	template<typename Fn>
	inline Value Parser<Fn>::parseNull()
	{
		checkString("null");
		return Value(nullptr);
	}

	template<typename Fn>
	inline bool Parser<Fn>::readSign() {
		bool isneg = false;
		char c = rd.next();
		if (c == '-') {
			isneg = true; rd.commit();
		}
		else if (c == '+') {
			isneg = false; rd.commit();
		}
		return isneg;
		
	}

	template<typename Fn>
	inline void Parser<Fn>::parseUnicode()
	{
		//unicode is parsed from \uXXXX (which can be between 0 and 0xFFFF)
		// and it is stored as UTF-8 (which can be 1...3 bytes)
		std::uintptr_t uchar = 0;
		for (int i = 0; i < 4; i++) {
			int c = rd.readFast();
			uchar *= 16;
			if (isdigit(c)) uchar += (c - '0');
			else if (c >= 'A' && c <= 'F') uchar += (c - 'A' + 10);
			else if (c >= 'a' && c <= 'f') uchar += (c - 'a' + 10);
			else ParseError("Expected '0'...'9' or 'A'...'F' after the escape sequence \\u: ("+std::string(tmpstr.begin(),tmpstr.end())+")", c);
		}
		storeUnicode(uchar);
	}


	template<typename Fn>
	inline void Parser<Fn>::storeUnicode(std::uintptr_t uchar) {
		WideToUtf8 conv;
		conv(oneCharStream((int)uchar),[&](char c){tmpstr.push_back(c);});
	}

	template<typename Fn>
	inline Value Parser<Fn>::parseNumber()
	{		

		if (flags & allowPreciseNumbers) {
			bool hint_is_double = false;
			StrIdx numb = parsePreciseNumber(hint_is_double);
			Value v = ParserHelper::numberFromStringRaw(getString(numb), hint_is_double);
			freeString(numb);
			return Value(v);
		} else {

			//first try to read number as signed or unsigned integer
			std::uintptr_t intpart;
			//read sign and return true whether it is '-' (discard '+')
			bool isneg = readSign();
			//test next character
			int c = rd.next();
			//it should be digit or dot (because .3 is valid number)
			if (!isdigit(c) && c != '.')
				throw ParseError("Expected '0'...'9', '.', '+' or '-'", c);
			//declared dummy var (we don't need counter here)
			int counter;
			//parse sequence of numbers as unsigned integer
			//function returns false, if overflow detected
			bool complete = parseUnsigned(intpart,counter);
			//in case of overflow or dot follows, continue to read as floating number
			if (!complete || rd.next() == '.' || toupper(rd.next()) == 'E') {
				//parse floating number (give it already parsed informations)
				return parseDouble(intpart, isneg);
			}
			//is negative?
			if (isneg) {
				//tests, whether highest bit of unsigned integer is set
				//if so, converting to signed triggers overflow
				if (intpart & (std::uintptr_t(1) << (sizeof(intpart) * 8 - 1))) {
					//convert number to float
					double v = (double)intpart;
					//return negative value
					return Value(-v);
				}
				else {
					//convert to signed and return negative
					return Value(-intptr_t(intpart));
				}
			}
			else {
				//return unsigned version
				return Value(intpart);
			}

		}

	}

	template<typename Fn>
	inline Value Parser<Fn>::parseString()
	{
		static auto createValue = [](StrViewA val) {
			if (val == "∞") return Value(std::numeric_limits<double>::infinity());
			else if (val == "-∞") return Value(-std::numeric_limits<double>::infinity());
			else return Value(val);
		};

		StrIdx str = readString();
		Value res (createValue(getString(str)));
		freeString(str);
		return res;
	}

	template<typename Fn>
	inline void Parser<Fn>::freeString(const StrIdx &str)
	{
		assert(str.first+ str.second== tmpstr.size());
		tmpstr.resize(str.first);

	}

	template<typename Fn>
	inline typename Parser<Fn>::StrIdx Parser<Fn>::readString()
	{
		std::size_t start = tmpstr.size();
		try {
			Utf8ToWide conv;
			conv([&]{
				do {
					int c = rd.readFast();
					if (c == -1) {
						throw ParseError("Unexpected end of file", c);
					}
					else if (c == '"') {
						return -1;
					}else if (c == '\\') {
						//parse escape sequence
						c = rd.readFast();
						switch (c) {
						case '"':
						case '\\':
						case '/': tmpstr.push_back(c); break;
						case 'b': tmpstr.push_back('\b'); break;
						case 'f': tmpstr.push_back('\f'); break;
						case 'n': tmpstr.push_back('\n'); break;
						case 'r': tmpstr.push_back('\r'); break;
						case 't': tmpstr.push_back('\t'); break;
						case 'u': parseUnicode();break;
						default:
							throw ParseError("Unexpected escape sequence in the string", c);
						}
					}
					else {
						return c;
					}
				} while (true);
			},[&](int w) {
				storeUnicode(w);
			});
			return StrIdx(start, tmpstr.size()-start);

		} catch (...) {
			tmpstr.resize(start);
			throw;
		}

	}

	template<typename Fn>
	inline void Parser<Fn>::checkString(const StringView<char>& str)
	{
		int c = rd.next();
		rd.commit();
		if (c != str[0]) throw ParseError("Unknown keyword", c);
		for (std::size_t i = 1; i < str.length; ++i) {
			int c = rd.readFast();
			if (c != str.data[i]) throw ParseError("Unknown keyword", c);
		}
	}



	template<typename Fn>
	inline bool Parser<Fn>::parseUnsigned(std::uintptr_t & res, int &counter)
	{
		const std::uintptr_t overflowDetection = ((std::uintptr_t)-1) / 10; //429496729
		//start at zero
		res = 0;
		//count read charactes
		counter = 0;
		//read first character
		char c = rd.next();
		//repeat while it is digit
		while (isdigit(c)) {
			if (res > overflowDetection) {
				return false;
			}
			//calculate next val
			std::uintptr_t nextVal = res * 10 + (c - '0');
			//detect overflow
			if (nextVal < res) {
				//report overflow (maintain already parsed value
				return false;
			}
			//commit chracter
			rd.commit();
			//read next character
			c = rd.next();
			//store next value
			res = nextVal;
			//count character
			counter++;
		}
		//found non digit character, exit parsing - number is complete
		return true;
	}

	template<typename Fn>
	inline double Parser<Fn>::parseLargeUnsigned(std::uintptr_t intPart)
	{
		//starts with given integer part - convert to double
		double res = (double)intPart;
		//read while there are digits

		int c;

		while (isdigit(c = rd.next())) {
			//prepare next unsigned integer
			std::uintptr_t part;
			//prepare counter
			int cnt;
			//read next sequence of digits as integer
			parseUnsigned(part, cnt);
			//now res = res * 10^cnt + part 
			//this should reduce rounding errors because count of multiplies will be minimal
			res = floor(res * pow(10.0, cnt) + double(part));

			if (std::isinf(res)) throw ParseError("Too long number", c);
		}
		//return the result
		return res;
	}


	template<typename Fn>
	inline double Parser<Fn>::parseDecimalPart()
	{
		//parses decimal part as series of unsigned number each is multipled by its position
		double res = 0;
		unsigned int pos = 0;
		//lookup table is slightly faster
		static double numbers[10] = {0.0, 1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0};

		//first, read digits and create large number
		//count of decimals
		//collect all numbers
		while (isdigit(rd.next())) {

			//pick digit ascii
			char c = rd.nextCommit();

			//convert to index
			unsigned int n = c - '0';

			//calculate 10*prev_value + number
			//always truncate the number to prevent rounding errors
			res = floor(res * 10 + numbers[n]);

			//increase count of decimals
			pos ++;
		}
		//finally, calculate decimal point shift (as number 10^digits)
		//we use 10 because it doesn't generate rounding errors
		double m = pow(10,pos);
		//one operation divide large number by m, to shift whole number behind deximal page
		res = res / m;
		//result
		return res;
	}

	template<typename Fn>
	inline Value Parser<Fn>::parseDouble(std::uintptr_t intpart, bool neg)
	{
		//float number is in format [+/-]digits[.digits][E[+/-]digits]
		//sign is stored in neg
		//complete to reading integer part
		double d = parseLargeUnsigned(intpart);

		int c = rd.next();
		//if next char is dot
		if (c == '.') {
			//commit dot
			rd.commit();
			//next character must be a digit
			if (isdigit(c = rd.next())) {
				//so parse decimal part
				double frac = parseDecimalPart();
				//combine with integer part
				d = d + frac;
			}
			else {
				//throw exception that next character is not digit
				throw ParseError("Expected '0'...'9' after '.'", c);
			}
			c = rd.next();
		}
		//next character is 'E' (exponent part)
		if (toupper(c) == 'E') {
			//commit the 'E'
			rd.commit();
			//read and discard any possible sign
			bool negexp = readSign();
			//prepare integer exponent
			std::uintptr_t expn;
			//prepare counter (we don't need it)
			int counter;
			//next character must be a digit
			if (!isdigit(c = rd.next()))
				//throw exception is doesn't it
				throw ParseError("Expected '0'...'9' after 'E'", c);
			//parse the exponent
			if (!parseUnsigned(expn, counter))
				//complain about too large exponent (two or three digits must match to std::uintptr_t)
				throw ParseError("Exponent is too large", c);
			//if exponent is negative
			if (negexp) {
				//calculate d * 0.1^expn
				d = d * pow(0.1, expn);
			}
			else {
				//calculate d * 10^expn
				d = d * pow(10.0, expn);
			}			
		}
		//negative value
		if (neg) {
			//return negative
			return Value(-d);
		} 
		else {
			//return positive
			return Value(d);
		}
	}


	template<typename Fn>
	typename Parser<Fn>::StrIdx Parser<Fn>::parsePreciseNumber(bool& hint_is_float) {
		std::uintptr_t start = tmpstr.size();
		try {
			int c = rd.nextCommit();
			if (c == '+' || c== '-') {
				tmpstr.push_back((char)c);
				c = rd.readFast();
			}
			if (!isdigit(c)) {
				rd.putBack(c);
				throw ParseError("Expected a number", c);
			}

			while (isdigit(c)) {
				tmpstr.push_back((char)c);
				c = rd.readFast();
			}
			if (c == '.') {
				hint_is_float = true;
				tmpstr.push_back((char)c);
				c = rd.readFast();
				while (isdigit(c)) {
					tmpstr.push_back((char)c);
					c = rd.readFast();
				}
			}
			if (c == 'e' || c == 'E') {
				hint_is_float = true;
				tmpstr.push_back((char)c);
				c = rd.readFast();
				if (c == '+' || c== '-') {
					tmpstr.push_back((char)c);
					c = rd.readFast();
				}
				if (!isdigit(c)) {
					rd.putBack(c);
					throw ParseError("Expected a number", c);
				}

				while (isdigit(c)) {
					tmpstr.push_back((char)c);
					c = rd.readFast();
				}

			}

			rd.putBack(c);
			return StrIdx(start, tmpstr.size()-start);

		} catch (...) {
			tmpstr.resize(start);
			throw;
		}

	}

}

