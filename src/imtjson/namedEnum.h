#pragma once
#include <vector>
#include <string_view>
#include <string>
#include <algorithm>


namespace json {


///JSON is often full of enums. To easy convert enum to string and vice versa, you can use this class
/**
 * To use this class, you need to define table where each row has enum value and string value. This
 * table must be used to construct NamedEnum instance. The object act as associative array. You
 * can easily pick value by string or string by value
 */
template<typename EnumType>
class NamedEnum {
public:

	struct Def {
		EnumType val;
		std::string_view name;
	};


	template<std::size_t N>
	NamedEnum(const Def(&x)[N]) {

		byVal.reserve(N);
		byName.reserve(N);
		for (auto v : x) {
			byVal.push_back(v);
			byName.push_back(v);
		}
		std::sort(byVal.begin(), byVal.end(), &cmpByVal);
		std::sort(byName.begin(), byName.end(), &cmpByName);

	}

	EnumType operator[](const std::string_view &name) const;
	std::string_view operator[](EnumType val) const;

	const EnumType *find(const std::string_view &name) const;



	typename std::vector<Def>::const_iterator begin() const {return byName.begin();}
	typename std::vector<Def>::const_iterator end() const {return byName.end();}
	std::size_t size() const {return byName.size();}
	std::string allEnums(std::string_view separator=", ") const;

protected:
	std::vector<Def> byVal;
	std::vector<Def> byName;

	static bool cmpByVal(const Def &a, const Def &b) {
		return a.val < b.val;
	}

	static bool cmpByName(const Def &a, const Def &b) {
		return a.name < b.name;
	}


};


class UnknownEnumException: public std::exception {
public:

	UnknownEnumException(std::string &&errorEnum, std::vector<std::string_view> &&availableEnums);

	virtual const char *what() const throw();

	const std::string& getErrorEnum() const {
		return errorEnum;
	}

	const std::vector<std::string_view>& getListEnums() const {
		return listEnums;
	}

protected:
	mutable std::string whatMsg;
	std::vector<std::string_view> listEnums;
	std::string errorEnum;

};



template<typename EnumType>
inline EnumType NamedEnum<EnumType>::operator [](const std::string_view &name) const {
	Def d;
	d.name = name;
	auto f = std::lower_bound(byName.begin(), byName.end(), d, &cmpByName);
	if (f == byName.end() || f->name != name) {
		std::vector<std::string_view> lst;
		lst.reserve(byName.size());
		for (auto &x:byName) lst.push_back(x.name);
		throw UnknownEnumException(std::string(name.data(),name.size()),std::move(lst));
	}
	else {
		return f->val;
	}

}

template<typename EnumType>
inline const EnumType *NamedEnum<EnumType>::find(const std::string_view &name) const {
	Def d;
	d.name = name;
	auto f = std::lower_bound(byName.begin(), byName.end(), d, &cmpByName);
	if (f == byName.end() || f->name != name) {
		return nullptr;
	}
	else {
		return &f->val;
	}

}

template<typename EnumType>
inline std::string_view NamedEnum<EnumType>::operator [](EnumType val) const {
	Def d;
	d.val=val;
	auto f = std::lower_bound(byVal.begin(), byVal.end(), d, &cmpByVal);
	if (f == byVal.end() || f->val != val) return std::string_view();
	else return f->name;
}

}

template<typename EnumType>
inline std::string json::NamedEnum<EnumType>::allEnums(std::string_view separator) const {
	std::size_t needsz=0;
	for (auto &x : byName) needsz+=x.name.length;
	needsz += separator.size()*byName.size();
	std::string res;
	res.reserve(needsz);
	std::string_view cursep;
	for (auto &x : byName) {
		res.append(cursep.data(),cursep.size());
		res.append(x.name.data(), x.name.size());
		cursep = separator;
	}
	return res;
}
