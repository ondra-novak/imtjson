#pragma once
#include <utility>

namespace json {


template<typename Fn>
class Compress {
public:

	Compress(const Fn &output);


	void operator()(char c);


	void reset();



protected:
	Fn output;


	typedef unsigned int ChainCode;
	typedef std::pair<ChainCode, char> Sequence;
	struct SeqInfo {
		ChainCode nextCode;
		bool used;
		SeqInfo(ChainCode nextCode):nextCode(nextCode),used(false) {}
	};

	typedef std::map<Sequence, SeqInfo> SeqDB;

	SeqDB seqdb;
	ChainCode nextCode;
	ChainCode lastSeq;
	static const ChainCode initialChainCOde = -1;
	static const ChainCode maxCode = (2<<6) + (2 << 14) - 8;

	void write(ChainCode code);






};



}

