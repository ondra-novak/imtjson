/*
 * range.h
 *
 *  Created on: 6. 4. 2017
 *      Author: ondra
 *
 *
 */

#pragma once

namespace json {


///Range of two iterators - it allows range based loops in C++11
/**
 * Because there is no such class in std!!!
 */
template<typename Iter>
class Range {
public:

	Range(const Iter &b, const Iter &e):b(b),e(e) {}
	Range(Iter &&b, Iter &&e):b(std::move(b)),e(std::move(e)) {}

	const Iter &begin() const {return b;}
	const Iter &end() const {return e;}

	std::size_t size() const {return std::distance(b,e);}

public:
	Iter b;
	Iter e;
};



}


