#pragma once
#include <utility>
#include <map>
#include <stack>

namespace json {

class CompressDecompresBase {
public:
	typedef unsigned int ChainCode;
	static const ChainCode initialChainCode = -1;
	static const ChainCode firstCode = 128;
	static const ChainCode flushCode = 0x3FFF;
	static const ChainCode maxCode = flushCode - 2; //there is reservation

													//key to db - current chain code and next char
	typedef std::pair<ChainCode, char> Sequence;
	//value of db 
	struct SeqInfo {
		//next code for current key
		ChainCode nextCode;
		//true, if key has been used to compress data
		bool used;
		SeqInfo(ChainCode nextCode) :nextCode(nextCode), used(false) {}
	};

	typedef std::map<Sequence, SeqInfo> SeqDB;

};


template<typename Fn>
class Compress: public CompressDecompresBase {
public:

	Compress(const Fn &output);

	void operator()(char c);

	void reset();
	~Compress();

protected:
	Fn output;



	//database of all created codes
	SeqDB seqdb;
	//next unused code 
	ChainCode nextCode;
	//last sequence code - while we walk through the available sequencies
	ChainCode lastSeq;

	void write(ChainCode code);


	void optimizeDB();
};

template<typename Fn>
class Decompress: public CompressDecompresBase {
public:

	Decompress(const Fn &input);

	char operator()();

	void reset();
	
	~Decompress();
protected:

	Fn input;

	//information about chain code - key is index in table
	//because first code is always 128, index is shifted about 128 down
	struct DecInfo {
		//previous code - for codes below 128, it is interpreted as char
		ChainCode prevCode;
		//next character emited for this chain code
		char outchar;
		//true if code has been used to decompress
		bool used;
		
		DecInfo(ChainCode prevCode, char outchar, bool used)
			:prevCode(prevCode), outchar(outchar), used(used) {}
	};

	//table of sequences
	std::vector<DecInfo> seqdb;
	//stack of charactes ready to return
	//because characters are generated in reverse order, stack is used to make order correct
	std::stack<char> readychars;
	//first character of previous sequence - it is need to reconstruct special repeating code
	char firstChar;
	//previous code to connect first character of next code and create new code
	ChainCode prevCode;

	void optimizeDB();

	ChainCode read();

	

};


template<typename Fn>
Compress<Fn> compress(const Fn &fn) { return Compress<Fn>(fn); }

template<typename Fn>
Decompress<Fn> decompress(const Fn &fn) { return Decompress<Fn>(fn); }


}

