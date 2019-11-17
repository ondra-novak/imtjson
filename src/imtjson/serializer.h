#pragma once

#include <cstdlib>
#include <cmath>
#include <vector>
#include "value.h"
#include "binary.h"
#include "utf8.h"

namespace json {

	///Configures precision of floating numbers
	/** Global variable specifies count of decimal digits of float numbers. 
	The default value is 9 and 9 is also maximum.
	*/
	extern uintptr_t maxPrecisionDigits;
	///Specifies default output format for unicode character during serialization
	/** If output format is not specified by serialization function. Default
	is emitEscaped*/
	extern UnicodeFormat defaultUnicodeFormat;


	template<typename Fn>
	class Serializer {
	public:



		Serializer(const Fn &target, bool utf8output) :target(target), utf8output(utf8output) {}

		void serialize(const Value &obj);
		virtual void serialize(const IValue *ptr);

		void serializeObject(const IValue *ptr);
		void serializeArray(const IValue *ptr);
		void serializeNumber(const IValue *ptr);
		void serializeString(const IValue *ptr);
		void serializeBoolean(const IValue *ptr);
		void serializeNull(const IValue *ptr);

		void serializeKeyValue(const IValue *ptr);

	protected:
		Fn target;
		bool utf8output;

		void write(const StringView<char> &text);
		void writeUnsigned(UInt value);
		void writeUnsignedRec(UInt value);
		void writeUnsignedLong(ULongInt value);
		void writeUnsignedRecLong(ULongInt value);
		void writeUnsigned(UInt value, UInt digits);
		void writeSigned(Int value);
		void writeSignedLong(LongInt value);
		void writeDouble(double value);
		void writeUnicode(unsigned int uchar);
		void writeString(const StringView<char> &text);
		void writeStringBody(const StringView<char> &text);
		void writePreciseNumber(const StringView<char> &text);

	};

	class SerializerError :public std::runtime_error {
	public:
		SerializerError(std::string msg) :std::runtime_error(msg) {}
	};



	template<typename Fn>
	inline void Value::serialize(Fn && target) const
	{
		serialize<Fn>(defaultUnicodeFormat,std::forward<Fn>(target));
	}

	template<typename Fn>
	inline void Value::serialize(UnicodeFormat format, Fn && target) const
	{
		Serializer<Fn> serializer(std::forward<Fn>(target), format == emitUtf8);
		return serializer.serialize(*this);
	}

	template<typename Fn>
	inline void Serializer<Fn>::serialize(const Value & obj)
	{
		serialize((const IValue *)(obj.getHandle()));
	}

	template<typename Fn>
	inline void Serializer<Fn>::serialize(const IValue * ptr)
	{
		switch (ptr->type()) {
		case object: serializeObject(ptr); break;
		case array: serializeArray(ptr); break;
		case number: serializeNumber(ptr); break;
		case string: serializeString(ptr); break;
		case boolean: serializeBoolean(ptr); break;
		case null: serializeNull(ptr); break;
		case undefined: writeString("undefined"); break;
		}


	}

	template<typename Fn>
	inline void Serializer<Fn>::serializeKeyValue(const IValue * ptr) {
		StringView<char> name = ptr->getMemberName();
		writeString(name);
		target(':');
		serialize(ptr);

	}

	template<typename Fn>
	inline void Serializer<Fn>::serializeObject(const IValue * ptr)
	{
		target('{');
		bool comma = false;
		auto fn = [&](const IValue *v) {
			if (comma) target(','); else comma = true;
			serializeKeyValue(v);
			return true;
		};
		ptr->enumItems(EnumFn<decltype(fn)>(fn));
		target('}');
	}

	template<typename Fn>
	inline void Serializer<Fn>::serializeArray(const IValue * ptr)
	{
		target('[');
		bool comma = false;
		auto fn = [&](const IValue *v) {
			if (comma) target(','); else comma = true;
			serialize(v);
			return true;
		};
		ptr->enumItems(EnumFn<decltype(fn)>(fn));
		target(']');
	}

	template<typename Fn>
	inline void Serializer<Fn>::serializeNumber(const IValue * ptr)
	{
		ValueTypeFlags f = ptr->flags();
		if (f & preciseNumber) writePreciseNumber(ptr->getString());
		else if (f & numberUnsignedInteger) {
			if (f & longInt) {
				writeUnsignedLong(ptr->getUIntLong());
			} else {
				writeUnsigned(ptr->getUInt());
			}
		}
		else if (f & numberInteger) {
			if (f & longInt) {
				writeSignedLong(ptr->getIntLong());
			} else {
				writeSigned(ptr->getInt());
			}
		}
		else writeDouble(ptr->getNumber());			
	}

	template<typename Fn>
	inline void Serializer<Fn>::serializeString(const IValue * ptr)
	{
		StringView<char> str = ptr->getString();
		if (ptr->flags()  & binaryString) {
			BinaryEncoding enc = Binary::getEncoding(ptr->unproxy());
			target('"');
			enc->encodeBinaryValue(BinaryView(str), [&](const StrViewA &str) {writeStringBody(str);});
			target('"');
		} else {
			writeString(str);
		}
	}

	template<typename Fn>
	inline void Serializer<Fn>::serializeBoolean(const IValue * ptr)
	{
		if (ptr->getBool() == false) write("false"); else write("true");
	}

	template<typename Fn>
	inline void Serializer<Fn>::serializeNull(const IValue *)
	{
		write("null");
	}

	template<typename Fn>
	inline void Serializer<Fn>::write(const StringView<char>& text)
	{
		for (auto &&x : text) target(x);
	}

	template<typename Fn>
	inline void Serializer<Fn>::writeUnsignedRec(UInt value) {
		if (value) {
			writeUnsignedRec(value / 10);
			target('0' + (value % 10));
		}
	}

	template<typename Fn>
	inline void Serializer<Fn>::writeUnsigned(UInt value)
	{
		if (value) writeUnsignedRec(value);
		else target('0');
	}
	template<typename Fn>
	inline void Serializer<Fn>::writeUnsignedRecLong(ULongInt value) {
		if (value) {
			writeUnsignedRecLong(value / 10);
			target('0' + (value % 10));
		}
	}

	template<typename Fn>
	inline void Serializer<Fn>::writeUnsignedLong(ULongInt value)
	{
		if (value) writeUnsignedRecLong(value);
		else target('0');
	}

	template<typename Fn>
	inline void Serializer<Fn>::writeUnsigned(UInt value, UInt digits)
	{
		if (digits>1) writeUnsigned(value/10,digits-1);
		target('0'+(value % 10));
	}

	template<typename Fn>
	inline void Serializer<Fn>::writeSigned(Int value)
	{
		if (value < 0) {
			target('-');
			writeUnsigned(-value);
		}
		else {
			writeUnsigned(value);
		}
	}
	template<typename Fn>
	inline void Serializer<Fn>::writeSignedLong(LongInt value)
	{
		if (value < 0) {
			target('-');
			writeUnsignedLong(-value);
		}
		else {
			writeUnsignedLong(value);
		}
	}


	template<typename Fn>
	inline void Serializer<Fn>::writeDouble(double value)
	{
		static UInt fracMultTable[10] = {
				1, //0
				10, //1
				100, //2
				1000, //3
				10000, //4
				100000, //5
				1000000, //6
				10000000, //7
				100000000, //8
 				1000000000 //9
		};

		if (!std::isfinite(value)) {
			if (value<0) writeString("-∞");
			else writeString("∞");
			return;
		}

		const char *inf = "Inf";

		bool sign = value < 0;
		UInt precisz = std::min<UInt>(maxPrecisionDigits, 9);

		value = std::abs(value);
		//calculate exponent of value
		//123897 -> 5 (1.23897e5)
		//0.001248 -> 3 (1.248e-3)
		double fexp = floor(log10(fabs(value)));
		//if fexp is Inf, handle special case. -Inf means, that value is zero or very nearly zero
		if (!std::isfinite(fexp)) {
				if (fexp < 0) {
					//print zero
					target('0');
				} else {
					//in this case, value is infinity
					if (sign) target('-');
					const char *z = inf;
					while (*z) target(*z++);
				}
				return;
			}
		//convert it to integer
		Int iexp = (Int)fexp;
		//if exponent is in some reasonable range, set iexp to 0
		if (iexp > -3 && iexp < 8) {
			iexp = 0;
		}
		else {
			//otherwise normalize number to be between 1 and 10
			value = value * pow(0.1, iexp);
		}		
		double fint;
		//separate number to integer and fraction
		double frac = modf(value, &fint);
		//calculate multiplication of fraction
		UInt fractMultiply = fracMultTable[precisz];
		//multiplicate fraction to receive best rounded number to given decimal places
		double fm = floor(frac * fractMultiply +0.5);

		//convert finteger to integer
		UInt intp(fint);
		//mantisa as integer number (without left zeroes)
		UInt m(fm);


		//if floating multiplied fraction is above or equal fractMultiply
		//something very bad happen, probably rounding error happened, so we need adjust
		if (m >= fractMultiply) {
			//increment integer part number
			intp = intp+1;
			//decrease fm
			m-=fractMultiply;
			//if integer part is equal or above 10
			if (intp >= 10 && iexp) {
				//set  integer part to 1 (because 9.99999 -> 10 -> 1e1)
				intp=1;
				//increase exponent
				iexp++;
			}
		}

		//write signum for negative number
		if (sign) target('-');
		//write absolute integer number (remove sign)
		writeUnsigned(intp);
		UInt digits = precisz;

		if (m) {
			//put dot
			target('.');
			//remove any rightmost zeroes
			while (m && (m % 10) == 0) {m = m / 10;--digits;}
			//write final number
			writeUnsigned(m, digits);
		}
		//if exponent is set
		if (iexp) {
			//put E
			target('e');
			if (iexp > 0) target('+');
			//write signed exponent
			writeSigned(iexp);
		}
		//all done
	}

	static inline void notValidUTF8(const std::string &text) {
		throw SerializerError(std::string("String is not valid UTF-8: ") + text);
	}

	template<typename Fn>
	inline void Serializer<Fn>::writeUnicode(unsigned int uchar) {


		if (utf8output && uchar >=0x80 && uchar != 0x2028 && uchar != 0x2029) {
			WideToUtf8 conv;
			conv(oneCharStream(uchar),[&](char c){target(c);});
		} else {
			target('\\');
			target('u');
			const char hex[] = "0123456789ABCDEF";
			for (unsigned int i = 0; i < 4; i++) {
				unsigned int b = (uchar >> ((3 - i) * 4)) & 0xF;
				target(hex[b]);
			}
		}
	}


	template<typename Fn>
	inline void Serializer<Fn>::writeString(const StringView<char>& text)
	{
		target('"');
		writeStringBody(text);
		target('"');
	}

	template<typename Fn>
	inline void Serializer<Fn>::writePreciseNumber(const StringView<char>& text) {
		for (auto c: text) target(c);
	}

	template<typename Fn>
	inline void Serializer<Fn>::writeStringBody(const StringView<char>& text)
	{
		Utf8ToWide conv;
		auto strrd = fromString(text);
		conv([&] {

			do {
				int c = strrd();
				if (c == eof) return c;
				switch (c) {
					case '\\':
//					case '/':
					case '"':target('\\'); target(c); break;
					case '\f':target('\\'); target('f'); break;
					case '\b':target('\\'); target('b'); break;
					case '\r':target('\\'); target('r'); break;
					case '\n':target('\\'); target('n'); break;
					case '\t':target('\\'); target('t'); break;
					default:
						if (c < 32 || c>=128) return c;
						else target(c);
						//if (c < 32) writeUnicode(c);else target(c); break;
				}
			} while (true);

		},[&](int c) {
			writeUnicode(c);
		});
/*
		unsigned int uchar = 0;
		unsigned int extra = 0;
		for (auto&& c : text) {
			if ((c & 0x80) == 0x80) {
				if ((c & 0xC0) == 0x80) {
					if (extra) {
						uchar = (uchar << 6) | (c & 0x3F);
						extra--;
						if (extra == 0) writeUnicode(uchar);
					}
					else {
						notValidUTF8(text);
					}
				}
				else {
					if (extra) {
						notValidUTF8(text);
					}
					if ((c & 0xe0) == 0xc0) {
						extra = 1;
						uchar = c & 0x1F;
					}
					else if ((c & 0xf0) == 0xe0) {
						extra = 2;
						uchar = c & 0x0F;
					}
					else if ((c & 0xf8) == 0xf0) {
						extra = 3;
						uchar = c & 0x07;
					}
					else {
						notValidUTF8(text);
					}
				}
			}
			else {
				}
			}
		}
		if (extra) {
			notValidUTF8(text);
		}
		*/
	}

}

