#pragma once
#include <utility>
#include <unordered_map>
#include <stack>
#include <vector>

namespace json {

class CompressDecompresBase {
public:
	typedef unsigned int ChainCode;
	static const ChainCode initialChainCode = -1;	
	static const ChainCode codeShift = 26;
	static const ChainCode firstCode = 128-codeShift;
	///Specifies mask which selects between single bytes and two bytes chaincodes
	/** It also defines size of the dictionary. Possible values are
		0x80,0xC0,0xE0,0xF0,0xF8,0xFC,0xFE,0xFF.
		Larger values generates smaller dictionary.
		Best compression ration is for 0x80, the dictionary can hold up to 32893 sequences.
	*/
	static const ChainCode extCodeMask = 0x80;
	static const ChainCode maxCodeToEncode = ((extCodeMask ^ 0xFF)<<8) + 0xFF + extCodeMask;
	static const ChainCode flushCode = maxCodeToEncode;
	static const ChainCode optimizeCode = maxCodeToEncode-1;
	static const ChainCode maxCode = maxCodeToEncode-2;
	static const ChainCode maxCodeForOptimize = maxCode - 16;
	static const unsigned int hashMapReserve = maxCode*4;


													//key to db - current chain code and next char
	typedef std::pair<ChainCode, char> Sequence;

	struct SequenceHash {
		std::size_t operator()(const Sequence v) const {
			return (v.first << 8) | v.second;
		}
	};

	//value of db 
	struct SeqInfo {
		//next code for current key
		ChainCode nextCode;
		//new code assigned when key is used - it will be stored under this code in new dictionary
		ChainCode newCode;
		//true, if key has been used to compress data
		bool used;

		SeqInfo(ChainCode nextCode) :nextCode(nextCode), used(false) {}
	};

	typedef std::map<Sequence, SeqInfo> SeqDBOrdered;
	// typedef SeqDBOrdered SeqDB;
	typedef std::unordered_map<Sequence, SeqInfo, SequenceHash> SeqDB;




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

	void compress(char c);



	//database of all created codes
	SeqDB seqdb;
	//next unused code
	ChainCode nextCode;
	//last sequence code - while we walk through the available sequencies
	ChainCode lastSeq;
	//new seq for optimizer
	ChainCode lastNewSeq;

	unsigned int utf8len;

	void write(ChainCode code);


	class Optimizer {
		SeqDB newDb;
		ChainCode newNextCode;
	public:
		Optimizer() {
			reset();
		}
		///Adds code to new dictionary
		/**
		 * @param seqNewCode new code of current sequence or code of first char
		 * @param rdChar next char in sequence
		 * @param sqinfo current SeqInfo for this combination. sqinfo.used must be false
		 */
		void addCode(ChainCode seqNewCode, char rdChar, SeqInfo &sqinfo) {
			if (newNextCode < maxCodeForOptimize) {
				newDb.insert(std::make_pair(
					Sequence(seqNewCode, rdChar),
					SeqInfo(newNextCode)));

				sqinfo.newCode = newNextCode;

				sqinfo.used = true;
				//increase code counter
				newNextCode++;
			}
		}
		ChainCode optimize(SeqDB &db) {
			std::swap(db, newDb);
			ChainCode ret = newNextCode;
			reset();
			return ret;
		}

		void reset() {
			newDb.clear();
			newNextCode = firstCode;
		}
	};


	Optimizer optimizer;

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

	char decompress();


	//information about chain code - key is index in table
	struct DecInfo {
		//previous code -
		ChainCode prevCode;
		//new code in next dictionary (is used)
		ChainCode newCode;
		//next character emited for this chain code
		char outchar;
		//true if code has been used to decompress
		bool used;
		
		DecInfo(ChainCode prevCode, char outchar, bool used)
			:prevCode(prevCode), outchar(outchar), used(used) {}
	};

	typedef std::vector<DecInfo> SeqDB;
	//table of sequences
	SeqDB seqdb;
	//stack of charactes ready to return
	//because characters are generated in reverse order, stack is used to make order correct
	std::stack<char> readychars;
	//first character of previous sequence - it is need to reconstruct special repeating code
	char firstChar;
	//previous code to connect first character of next code and create new code
	ChainCode prevCode;



	class Optimizer {
		SeqDB newDb;
	public:
		Optimizer() {
			reset();
		}
		void addCode(DecInfo &sqinfo, ChainCode prevCodeTranslated) {
			ChainCode newCode = (ChainCode)(newDb.size() + firstCode);
			if (newCode < maxCodeForOptimize) {
				sqinfo.used = true;
				sqinfo.newCode = newCode;
				newDb.push_back(DecInfo(prevCodeTranslated,sqinfo.outchar,false));
			}
		}
		void optimize(SeqDB &db) {
			std::swap(db, newDb);
			reset();
		}

		void reset() {
			newDb.clear();
		}
	};

	Optimizer optimizer;

	void optimizeDB();

	ChainCode translatePrevCode(ChainCode p);

	ChainCode read();

	unsigned int utf8len;


};


template<typename Fn>
Compress<Fn> compress(const Fn &fn) { return Compress<Fn>(fn); }

template<typename Fn>
Decompress<Fn> decompress(const Fn &fn) { return Decompress<Fn>(fn); }


}

