#pragma once

#include <string>
#include <algorithm>
#include <vector>



namespace json {

	///stores a refernece to string and size
	/** Because std::string is very slow and heavy */
	template<typename T>
	class StringRef {
	public:
		StringRef() :data(0), length(0) {}
		StringRef(const T *str) : data(str), length(calcLength(str)) {}
		StringRef(const T *str, std::size_t length): data(str),length(length) {}
		StringRef(const std::basic_string<T> &string) : data(string.data()), length(string.length()) {}
		StringRef(const std::vector<T> &string) : data(string.data()), length(string.length()) {}
		StringRef(const std::initializer_list<T> &list) :data(list.begin()), length(list.size()) {}

		operator std::basic_string<T>() const { return std::basic_string<T>(data, length); }

		StringRef substr(std::size_t index) {
			std::size_t indexadj = std::min(index, length);
			return StringRef(data + indexadj, length - indexadj);
		}
		StringRef substr(std::size_t index, std::size_t len) {
			std::size_t indexadj = std::min(index, length);
			return StringRef(data + indexadj, std::min(length-indexadj, len));
		}

		int compare(const StringRef &other) const {
			std::size_t cnt = std::min(length, other.length);
			for (std::size_t i = 0; i < cnt; ++i) {
				if (data[i] < other.data[i]) return -1;
				if (data[i] > other.data[i]) return 1;
			}
			if (length < other.length) return -1;
			if (length > other.length) return 1;
			return 0;
		}

		bool operator==(const StringRef &other) const { return compare(other) == 0; }
		bool operator!=(const StringRef &other) const { return compare(other) != 0; }
		bool operator>=(const StringRef &other) const { return compare(other) >= 0; }
		bool operator<=(const StringRef &other) const { return compare(other) <= 0; }
		bool operator>(const StringRef &other) const { return compare(other) > 0; }
		bool operator<(const StringRef &other) const { return compare(other) < 0; }

		const T * const data;
		const std::size_t length;

		char operator[](std::size_t pos) const { return data[pos]; }
		
		const char *begin() const { return data; }
		const char *end() const { return data + length; }

		static std::size_t calcLength(const T *str) {
			const T *p = str;
			while (*p) ++p;
			return p - str;
		}
	};



}