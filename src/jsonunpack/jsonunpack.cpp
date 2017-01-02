// jsonpack.cpp : Defines the entry point for the console application.
//
#ifdef _WIN32
	#include <io.h>
#endif
#include <fcntl.h>
#include <fstream>
#include "../immujson/json.h"
#include "../immujson/compress.tcc"



int main(int argc, char **argv)
{
#ifdef _WIN32
	_setmode(_fileno(stdin), _O_BINARY);
#endif
	try {
		using namespace json;
		Value v = Value::parse(decompress([] {
			return std::cin.get();
		}));
		v.toStream(emitUtf8, std::cout);

		return 0;
	}
	catch (std::exception &e) {
		std::cerr << "Fatal error: " << e.what() << std::endl;
		return 1;
	}
}

