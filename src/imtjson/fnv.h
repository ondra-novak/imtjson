/*
 * fnv.h
 *
 *  Created on: 27. 10. 2016
 *      Author: ondra
 */

#include <cstdint>

template<int bytes>
class FNV1a;


class FNV1a64 {
public:
	explicit FNV1a64(std::uint64_t &hash):hash(hash) {
		hash = 14695981039346656037UL;
	}
	template<typename X>
	void operator()(X c) {
		hash = (hash ^ (std::uint8_t)c) * 1099511628211UL;
	}

protected:
	std::uint64_t &hash;
};
class FNV1a32 {
public:
	explicit FNV1a32(std::uint32_t &hash):hash(hash) {
		hash = 2166136261;
	}
	template<typename X>
	void operator()(X c) {
		hash = (hash ^ (std::uint8_t)c) * 16777619;
	}

protected:
	std::uint32_t &hash;
};

template<> class FNV1a<4>: public FNV1a32 {
	using FNV1a32::FNV1a32;

};
template<> class FNV1a<8>: public FNV1a64 {
	using FNV1a64::FNV1a64;
};



