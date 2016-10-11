#pragma once

#include "compress.h"

namespace json {


template<typename Fn>
Compress<Fn>::Compress(const Fn& output):nextCode(127),lastSeq(initialChainCOde) {
}

template<typename Fn>
void Compress<Fn>::reset() {
	nextCode = 128;
	lastSeq = initialChainCode;
	seqdb.clear();

}

template<typename Fn>
void Compress<Fn>::operator ()(char c) {

	if (c & 0x80)
		throw std::runtime_error("Unsupported character - only 7-bit ascii are supported");
	if (lastSeq == initialChainCOde) {
		lastSeq = c;
	} else {
		auto iter = seqdb.find(Sequence(lastSeq, c));
		if (iter == seqdb.end()) {
			write(lastSeq);
			seqdb.insert(std::make_pair(Sequence(lastSeq,c),SeqInfo(nextCode++)));
			lastSeq = c;
			if (nextCode == maxCode) {
				optimizeDB();
			}
		}
	}
}

template<typename Fn>
inline void Compress<Fn>::write(ChainCode code) {
	if (code < 0xC0) {
		output((unsigned char)code);
	} else {
		output(((unsigned char)code >> 8) | 0xC0);
		output(((unsigned char)(code & 0xFF)));
	}
}


}
