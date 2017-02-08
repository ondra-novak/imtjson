// jsonpack.cpp : Defines the entry point for the console application.
//
#ifdef _WIN32
	#include <io.h>
#endif
#include <fcntl.h>
#include <fstream>
#include "../imtjson/json.h"
#include "../imtjson/binjson.tcc"



int main(int argc, char **argv)
{
#ifdef _WIN32
	_setmode(_fileno(stdin), _O_BINARY);
#endif
	try {
		using namespace json;
		Value v = Value::parseBinary(fromStream(std::cin));
		v.toStream(emitUtf8, std::cout);

		return 0;
	}
	catch (std::exception &e) {
		std::cerr << "Fatal error: " << e.what() << std::endl;
		return 1;
	}
}

