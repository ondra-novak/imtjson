#include <fstream>
#include "../imtjson/json.h"
#include "../imtjson/validator.h"

using namespace json;


static void printRej(std::ostream &out, Value path, Value rule) {
	for (Value x : path) {
		out << "/";
		if (x.type() == number) out << "[" << x.getUInt() << "]";
		else out << x.getString();
	}
	out << ":\t";
	out << rule.toString();
	out << std::endl;
}

int runValidator(std::istream &indef, std::istream &in, std::ostream &errors) {
	Value def = Value::fromStream(indef);
	Value input = Value::fromStream(in);
	Validator2 v(def);
	if (v.validate(input)) return 0;
	Value rej = v.getRejections();

	for (Value x : rej) {
		printRej(errors, x[0], x[1]);
	}
	return 1;

}

int main(int argc, char **args) {

	if (argc < 2) {

		std::cout << "Usage:" << std::endl << std::endl

			<< args[0] << " <def.json>" << std::endl
			<< args[0] << " <def.json> <input.json>" << std::endl
			<< std::endl
			<< "def.json          definition file" << std::endl
			<< "input.json        input file to validate" << std::endl
			<< std::endl
			<< "If no input.json is specified, standard input is read" << std::endl
			<< std::endl
			<< "Result:" << std::endl
			<< "status 0 - validation success" << std::endl
			<< "status 1 - validation failed" << std::endl;
		
	}
	else {

		try {
			std::ifstream def(args[1]);
			if (!def) {
				std::cerr << "Unable to open file: " << args[1] << std::endl;
				return 2;
			}
			if (argc == 2) {
				return runValidator(def, std::cin, std::cout);
			}
			else {
				std::ifstream in(args[2]);
				if (!in) {
					std::cerr << "Unable to open file: " << args[2] << std::endl;
					return 3;
				}
				return runValidator(def, in, std::cout);
			}
		}
		catch (std::exception &e) {
			std::cerr << "Fatal error: " << e.what() << std::endl;
			return 255;
		}
	}
}