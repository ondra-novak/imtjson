#pragma once

#include "compress.h"
#include "parser.h"

namespace json {


template<typename Fn>
Compress<Fn>::Compress(const Fn& output):output(output),nextCode(firstCode),lastSeq(initialChainCode), utf8len(0) {
	seqdb.reserve(hashMapReserve);
}

template<typename Fn>
Compress<Fn>::~Compress() {
	if (lastSeq != initialChainCode) {
		write(lastSeq);
		write(flushCode); //flush code
	}
}

template<typename Fn>
void Compress<Fn>::reset() {
	nextCode = firstCode;
	lastSeq = initialChainCode;
	seqdb.clear();
	seqdb.reserve(hashMapReserve);

}

template<typename Fn>
void Compress<Fn>::operator ()(char c) {
	if (utf8len) {
		utf8len--;
		compress(c & 0x7F);
	}
	else if (c & 0x80) {
		if ((c & 0xE0) == 0xC0) {
			compress(1);
			compress(c & 0x3F);
			utf8len = 1;
		}
		else if ((c & 0xF0) == 0xE0) {
			compress(2);
			compress(c & 0x1F);
			utf8len = 2;
		}
		else if ((c & 0xF8) == 0xF0) {
			compress(3);
			compress(c & 0x0F);
			utf8len = 3;
		}
		else if ((c & 0xFC) == 0xF8) {
			compress(4);
			compress(c & 0x0F);
			utf8len = 4;
		}
		else {
			throw std::runtime_error("Unsupported character - invalid UTF-8 sequence");
		}
	}
	else if ((unsigned int)c < 32) {
		throw std::runtime_error("Unsupported character - jsons containing control characters are not supported");
	}
	else {
		compress(c-codeShift);
	}
}

template<typename Fn>
void Compress<Fn>::compress(char c) {

	if ((unsigned int)c >= firstCode)
		throw std::runtime_error("Invalid input");
	//for very first character...
	if (lastSeq == initialChainCode) {
		//initialize lastSeq
		lastNewSeq = lastSeq = c;
	} else {
		//search for pair <lastSeq, c> if we able to compress it
		auto iter = seqdb.find(Sequence(lastSeq, c));
		//no such pair
		if (iter == seqdb.end()) {
			//so first close previous sequence and send it to the output
			write(lastSeq);
			//register new pair 
			//current c can be part of next sequence, this is why decompresser need first character of the sequence
			seqdb.insert(std::make_pair(Sequence(lastSeq,c),SeqInfo(nextCode)));
			//increase next code
			++nextCode;
			//store lastSeq as c - we starting accumulate next sequence
			lastNewSeq = lastSeq = c;
		} else if (iter->second.nextCode >= maxCode) {

			write(lastSeq);

			write(optimizeCode);

			optimizeDB();

			lastNewSeq = lastSeq = c;
		} else {
			//in case that pair has been found
			//mark the pair used
			if (!iter->second.used) {
				optimizer.addCode(lastNewSeq, c, iter->second);
			}
			//and advence current sequence
			lastSeq = iter->second.nextCode;
			lastNewSeq = iter->second.newCode;
		}
	}
}

template<typename Fn>
inline void Compress<Fn>::write(ChainCode code) {
	//code below 0xC0 are written directly
	//0x00-0x7F = ascii directly
	//0x80-0xBF = first 64 compress-codes
	if (code < extCodeMask) {
		output((unsigned char)code);
	} else {
		//first 0xC0 codes are written above
		code -= extCodeMask;
		//numbers equal and above 0xC0 are written in two bytes
		//11xxxxxx xxxxxxxx - 14 bits (max 0x3FFF)
		output((unsigned char)(code >> 8) | extCodeMask);
		output(((unsigned char)(code & 0xFF)));
	}
}

template<typename Fn>
void Compress<Fn>::optimizeDB()
{
#if 0
	//this table converts old chain code into new chain code
	//optimization strips unused sequences.
	//all sequences that are part of other sequences are also used
	ChainCode translateTable[maxCode];
	//initialize table to map codes below 128 in the identity
	for (unsigned int i = 0; i < firstCode; i++) translateTable[i] = i;
	SeqDBOrdered seqdbo(seqdb.begin(), seqdb.end());
	SeqDB newdb;
	newdb.reserve(hashMapReserve);
	//we starting here
	ChainCode newNextCode = firstCode;
	for (auto &&x : seqdbo) {
		//only used codes
		if (x.second.used && x.second.nextCode < maxCodeForOptimize) {
			//insert to new database - translated key + reference to newly created code
			newdb.insert(std::make_pair(
				Sequence(translateTable[x.first.first], x.first.second),
				SeqInfo(newNextCode)));
			//register translation of the code
			translateTable[x.second.nextCode] = newNextCode;
			//increase code counter
			newNextCode++;
		}
	}
	//make new database current
	newdb.swap(seqdb);
	//update nextCode
	nextCode = newNextCode;
#endif
	nextCode = optimizer.optimize(seqdb);
}

template<typename Fn>
inline Decompress<Fn>::Decompress(const Fn & input) 
	:input(input)
	,prevCode(initialChainCode), utf8len(0) {}

template<typename Fn>
char Decompress<Fn>::operator()()
{
	char c = decompress();
	if (utf8len) {
		utf8len--;
		return (c | 0x80);
	}
	else {
		switch (c) {
		case -1: return -1;
		case 1: utf8len = 1; return decompress() | 0xC0;
		case 2: utf8len = 2;  return decompress() | 0xE0;
		case 3: utf8len = 3;  return decompress() | 0xF0;
		case 4: utf8len = 4;  return decompress() | 0xF8;
		default: return c+codeShift;
		}
	}
}
template<typename Fn>
char Decompress<Fn>::decompress() {
	//decompressor first checks whether there are ready bytes
	if (!readychars.empty()) {
		//if do, pick top of stack
		char out = readychars.top();
		//remove byte
		readychars.pop();
		//return byte
		return out;
	}
	else {
		//no raady bytes, we must generate some
		//first read the code
		ChainCode cc = read();
		//if it is flushcode, then reset state and return EOF
		if (cc == flushCode) {
			reset();
			return -1;
		}
		if (cc == optimizeCode) {
			optimizeDB();
			prevCode = initialChainCode;
			return decompress();
		}
/*		//for very first code
		if (prevCode == initialChainCode) {
			//store it as prevCode
			prevCode = cc;
			//store it as first char (because it is sequence of one character)
			firstChar = (char)cc;
			//returns this character
			return firstChar;
		}*/
		//p is walk-pointer
		ChainCode p = cc;
		//calculate next code from size of database
		ChainCode nextCode = (ChainCode)(seqdb.size() + firstCode);
		//used flag
		bool used = false;

		//in situation, that chaincode is equal to nextCode
		//i.e. it referes to code which will be just created
		//this is special situation which may happen and it has solution
		if (p == nextCode) {
			//it happens when firstChar appears as last character of the same sequence
			//so push first char now
			readychars.push(firstChar);
			//and expand previous sequence
			p = prevCode;
			//new sequence is also immediately used
			used = true;

		}
		//if p is above nextCode, this is sync error
		else if (p > nextCode) {
			throw ParseError("Corrupted compressed stream");
		}
		//expand the sequence
		while (p >= firstCode) {
			//walk from top to bottom
			DecInfo &nfo = seqdb[p - firstCode];
			//push to stack
			readychars.push(nfo.outchar);
			p = nfo.prevCode;
			//mark every code as used
			if (nfo.used == false) {
				ChainCode pt = translatePrevCode(p);
				optimizer.addCode(nfo, pt);
			}
		}

		//remaining code is firstChar
		firstChar = (char)p;
		//register new code which is pair of prevCode and firstChar of current code
		if (prevCode != initialChainCode) {
			seqdb.push_back(DecInfo(prevCode, firstChar, false));
			if (used) {
				translatePrevCode(nextCode);
			}
		}
		//make current code as prev code
		prevCode = cc;
		//first char is returned as result
		return firstChar;
	}
	
}

template<typename Fn>
typename Decompress<Fn>::ChainCode Decompress<Fn>::translatePrevCode(ChainCode p) {
	if (p < firstCode) return p;
	DecInfo &sq = seqdb[p - firstCode];
	if (!sq.used) {
		ChainCode prev = translatePrevCode(sq.prevCode);
		optimizer.addCode(sq,prev);
	}
	return sq.newCode;


}
template<typename Fn>
void Decompress<Fn>::reset()
{
	seqdb.clear();	
	prevCode = initialChainCode;
}

template<typename Fn>
Decompress<Fn>::~Decompress()
{
}

template<typename Fn>
void Decompress<Fn>::optimizeDB()
{
#if 0
	ChainCode translateTable[maxCode];
	decltype(seqdb) newdb;
	ChainCode oldcc = firstCode;
	SeqDBOrdered tmpdb;
	for (unsigned int i = 0; i < firstCode; i++) translateTable[i] = i;
	for (auto &&x : seqdb) {
		if (x.used && oldcc < maxCodeForOptimize) {
			tmpdb.insert(std::make_pair(Sequence(x.prevCode, x.outchar), SeqInfo(oldcc)));
		}
		oldcc++;
	}
	for (auto &&x : tmpdb) {
		ChainCode newcc = (ChainCode)(newdb.size() + firstCode);
		newdb.push_back(DecInfo(translateTable[x.first.first], x.first.second, false));
		translateTable[x.second.nextCode] = newcc;
	}
	prevCode = firstChar;
	seqdb.swap(newdb);
#endif
	optimizer.optimize(seqdb);
}

template<typename Fn>
CompressDecompresBase::ChainCode Decompress<Fn>::read()
{
	unsigned char b = input();
	ChainCode cc = b;
	if ((b & extCodeMask) != extCodeMask) return cc;
	b = input();
	cc = (cc & ~extCodeMask) << 8 | b;
	return cc+ extCodeMask;
}


}
