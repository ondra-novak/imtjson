/*
 * range.h
 *
 *  Created on: 7. 2. 2018
 *      Author: ondra
 */

#ifndef SRC_IMTJSON_RANGE_H_
#define SRC_IMTJSON_RANGE_H_
#include <type_traits>
#include <utility>

#pragma once


///Extends std::pair with ranged-loop featire
/**
 *
 * auto r = mmap.equal_range(...);
 * for (auto &x : range(r)) {
 *
 * }
 *
 */
template<typename Iter>
class Range: public std::pair<Iter, Iter> {
public:
	typedef std::pair<Iter, Iter> Super;

	using std::pair<Iter, Iter>::pair;

	typename Super::first_type &begin() {return this->first;}
	const typename Super::first_type &begin() const {return this->first;}
	typename Super::second_type &end() {return this->second;}
	const typename Super::second_type &end() const {return this->second;}

	auto size() const -> decltype(std::distance(
			std::declval<typename Super::first_type>(),
			std::declval<typename Super::second_type>())) {
		return std::distance(this->first, this->second);
	}

};

template<typename Iter>
Range<Iter> range(const std::pair<Iter, Iter> &r) {return Range<Iter>(r);}
template<typename Iter>
Range<Iter> range(std::pair<Iter, Iter> &&r) {return Range<Iter>(std::move(r));}




#endif /* SRC_IMTJSON_RANGE_H_ */
