#pragma once

#include <string>
#include <algorithm>
#include <vector>
#include <iostream>



namespace json {

	///stores a refernece to string and size
	/** Because std::string is very slow and heavy */
	template<typename T>
	class StringView {
	public:
		StringView() :data(0), length(0) {}
		StringView(const T *str) : data(str), length(calcLength(str)) {}
		StringView(const T *str, std::size_t length): data(str),length(length) {}
		StringView(const std::basic_string<T> &string) : data(string.data()), length(string.length()) {}
		StringView(const std::vector<T> &string) : data(string.data()), length(string.size()) {}
		StringView(const std::initializer_list<T> &list) :data(list.begin()), length(list.size()) {}
		StringView(const StringView &other) :data(other.data), length(other.length) {}

		StringView &operator=(const StringView &other) {
			if (&other != this) {
				this->~StringView();
				new(this) StringView(other);
			}
			return *this;
		}

		operator std::basic_string<T>() const { return std::basic_string<T>(data, length); }

		StringView substr(std::size_t index) const {
			std::size_t indexadj = std::min(index, length);
			return StringView(data + indexadj, length - indexadj);
		}
		StringView substr(std::size_t index, std::size_t len) const {
			std::size_t indexadj = std::min(index, length);
			return StringView(data + indexadj, std::min(length-indexadj, len));
		}

		int compare(const StringView &other) const {
			//equal strings by pointer and length
			if (other.data == data && other.length == length) return 0;
			//compare char by char
			std::size_t cnt = std::min(length, other.length);
			for (std::size_t i = 0; i < cnt; ++i) {
				if (data[i] < other.data[i]) return -1;
				if (data[i] > other.data[i]) return 1;
			}
			if (length < other.length) return -1;
			if (length > other.length) return 1;
			return 0;
		}

		bool operator==(const StringView &other) const { return compare(other) == 0; }
		bool operator!=(const StringView &other) const { return compare(other) != 0; }
		bool operator>=(const StringView &other) const { return compare(other) >= 0; }
		bool operator<=(const StringView &other) const { return compare(other) <= 0; }
		bool operator>(const StringView &other) const { return compare(other) > 0; }
		bool operator<(const StringView &other) const { return compare(other) < 0; }

		const T * const data;
		const std::size_t length;

		const T &operator[](std::size_t pos) const { return data[pos]; }
		
		const T *begin() const { return data; }
		const T *end() const { return data + length; }

		static std::size_t calcLength(const T *str) {
			const T *p = str;
			if (p) {
				while (*p) ++p;
				return p - str;
			} else {
				return 0;
			}

		}
		bool empty() const {return length == 0;}

		static const std::size_t npos = -1;

		std::size_t indexOf(const StringView sub, std::size_t pos = 0) const {
			if (sub.length > length) return npos;
			std::size_t eflen = length - sub.length + 1;
			while (pos < eflen) {
				if (substr(pos,sub.length) == sub) return pos;
				pos++;
			}
			return npos;
		}

		std::size_t lastIndexOf(const StringView sub, std::size_t pos = 0) const {
			if (sub.length > length) return -1;
			std::size_t eflen = length - sub.length + 1;
			while (pos < eflen) {
				eflen--;
				if (substr(eflen,sub.length) == sub) return eflen;
			}
			return npos;
		}


		///Helper class to provide operation split
		/** split() function */
		class SplitFn {
		public:
			SplitFn(const StringView &source, const StringView &separator, unsigned int limit)
				:source(source),separator(separator),startPos(0),limit(limit) {}

			///returns next element
			StringView operator()() {
				std::size_t fnd = limit?source.indexOf(separator, startPos):npos;
				std::size_t strbeg = startPos, strlen;
				if (fnd == (std::size_t)-1) {
					strlen = source.length - strbeg;
					startPos = source.length;
				} else {
					strlen = fnd - startPos;
					startPos = fnd + separator.length;
					limit = limit -1;
				}
				return source.substr(strbeg, strlen);
			}
			///Returns rest of the string
			operator StringView() const {
				return source.substr(startPos);
			}
		protected:
			StringView source;
			StringView separator;
			std::size_t startPos;
			unsigned int limit;
		};

		///Function splits string into parts separated by the separator
		/** Result of this function is another function, which
		 * returns part everytime it is called.
		 *
		 * You can use following code to process all parts
		 * @code
		 * auto splitFn = str.split(",");
		 * for (StrViewA s = splitFn(); !str.isEndSplit(s); s = splitFn()) {
		 * 		//work with "s"
		 * }
		 * @endcode
		 * The code above receives the split function into the splitFn
		 * variable. Next, code processes all parts while each part
		 * is available in the variable "s"
		 *
		 * Function starts returning the empty string once it extracts the
		 * last part. However not every empty result means end of the
		 * processing. You should pass the returned value to the
		 * function isSplitEnd() which can determine, whether the
		 * returned value is end of split.
		 * @param separator
		 * @param limit allows to limit count of substrings. Default is unlimited
		 * @return function which provides split operation.
		 */
		SplitFn split(const StringView &separator, unsigned int limit = (unsigned int)-1) const {
			return SplitFn(*this,separator, limit);
		}

		///Determines, whether argument has been returned as end of split cycle
		/** @retval true yes
		 *  @retval false no, this string is not marked as split'send
		 * @param result
		 * @return
		 */
		bool isSplitEnd(const StringView &result) const {
			return result.length == 0 && result.data == data + length;
		}

	};

	template<typename T>
	std::ostream &operator<<(std::ostream &out, const StringView<T> &ref) {
		out.write(ref.data, ref.length);
		return out;
	}


	typedef StringView<char> StrViewA;
	typedef StringView<wchar_t> StrViewW;

	class BinaryView: public StringView<unsigned char>{
	public:
		using StringView<unsigned char>::StringView;

		BinaryView(const BinaryView &from)
			:StringView<unsigned char>(from)
		{}

		BinaryView(const StringView<unsigned char> &from)
			:StringView<unsigned char>(from)
		{}

		///Explicit conversion from any view to binary view
		/** it might be useful for simple serialization, however, T should be POD or flat object */
		template<typename T>
		explicit BinaryView(const StringView<T> &from)
			:StringView<unsigned char>(reinterpret_cast<const unsigned char *>(from.data), from.length*sizeof(T))
		{}


		///Explicit conversion binary view to any view
		/** it might be useful for simple deserialization, however, T should be POD or flat object */
		template<typename T>
		explicit operator StringView<T>() const {
			return StringView<T>(reinterpret_cast<const T *>(data), length/sizeof(T));
		}
	};



}
