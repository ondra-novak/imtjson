#pragma once

#include "compress.h"
#include "parser.h"

namespace json {


template<typename Fn>
Compress<Fn>::Compress(const Fn& output):output(output),nextCode(firstCode),lastSeq(initialChainCode) {
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

}

template<typename Fn>
void Compress<Fn>::operator ()(char c) {

	//non-ascii characters are rejected with exception
	if (c & 0x80)
		throw std::runtime_error("Unsupported character - only 7-bit ascii are supported");
	//for very first character...
	if (lastSeq == initialChainCode) {
		//initialize lastSeq
		lastSeq = c;
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
			lastSeq = c;
			//however, if we reached end-code, we need to strip some unused codes
			if (nextCode == maxCode) {
				optimizeDB();
			}
		}
		else {
			//in case that pair has been found
			//mark the pair used
			iter->second.used = true;
			//and advence current sequence
			lastSeq = iter->second.nextCode;
		}
	}
}

template<typename Fn>
inline void Compress<Fn>::write(ChainCode code) {
	//code below 0xC0 are written directly
	//0x00-0x7F = ascii directly
	//0x80-0xBF = first 64 compress-codes
	if (code < 0xC0) {
		output((unsigned char)code);
	} else {
		//first 0xC0 codes are written above
		code -= 0x3F;
		//numbers equal and above 0xC0 are written in two bytes
		//11xxxxxx xxxxxxxx - 14 bits (max 0x3FFF)
		output((unsigned char)(code >> 8) | 0xC0);
		output(((unsigned char)(code & 0xFF)));
	}
}

template<typename Fn>
void Compress<Fn>::optimizeDB()
{
	//this table converts old chain code into new chain code
	//optimization strips unused sequences.
	//all sequences that are part of other sequences are also used
	ChainCode translateTable[maxCode];
	//initialize table to map codes below 128 in the identity
	for (unsigned int i = 0; i < firstCode; i++) translateTable[i] = i;
	SeqDB newdb;
	//we starting here
	ChainCode newNextCode = firstCode;
	for (auto &&x : seqdb) {
		//only used codes
		if (x.second.used) {
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
}

template<typename Fn>
inline Decompress<Fn>::Decompress(const Fn & input) 
	:input(input)
	,prevCode(initialChainCode) {}

template<typename Fn>
char Decompress<Fn>::operator()()
{
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
		//for very first code
		if (prevCode == initialChainCode) {
			//store it as prevCode
			prevCode = cc;
			//store it as first char (because it is sequence of one character)
			firstChar = (char)cc;
			//returns this character
			return firstChar;
		}
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
			nfo.used = true;
		}

		//remaining code is firstChar
		firstChar = (char)p;
		//register new code which is pair of prevCode and firstChar of current code
		seqdb.push_back(DecInfo(prevCode, firstChar, used));
		//make current code as prev code
		prevCode = cc;
		//database is full
		if (nextCode + 1 == maxCode) {
			//optimise database
			optimizeDB();
		}
		//first char is returned as result
		return firstChar;
	}
	
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
	ChainCode translateTable[maxCode];
	decltype(seqdb) newdb;
	ChainCode oldcc = firstCode;
	SeqDB tmpdb;
	for (unsigned int i = 0; i < firstCode; i++) translateTable[i] = i;
	for (auto &&x : seqdb) {
		if (x.used) {
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
}

template<typename Fn>
CompressDecompresBase::ChainCode Decompress<Fn>::read()
{
	unsigned char b = input();
	ChainCode cc = b;
	if ((b & 0xC0) != 0xC0) return cc;
	b = input();
	cc = (cc & ~0xC0) << 8 | b;		
	return cc+0x3F;
}


}
