/*
 * testClass.h
 *
 *  Created on: 10. 10. 2016
 *      Author: ondra
 */

#ifndef SRC_TESTS_TESTCLASS_H_
#define SRC_TESTS_TESTCLASS_H_
#include <sstream>
#include <iostream>


class TestSimple {
public:

	TestSimple(bool showOutput = false):failed(false),showOutput(showOutput) {}

	class ChargedTest {
	public:
		ChargedTest(TestSimple &owner, const char *name, const char *expected_result)
			:name(name),expected_result(expected_result),owner(owner) {}

		const std::string name;
		const std::string expected_result;

		template<typename Fn>
		void operator >> (const Fn &fn) const {
			owner.runTest(fn,name, expected_result);
		}

	protected:
		TestSimple &owner;
	};

	ChargedTest test(const char *name, const char *expected_result) {
		return ChargedTest(*this,name,expected_result);
	}

	bool didFail() const {return failed;}

protected:
	bool failed;
	bool showOutput;

	template<typename Fn>
	void runTest(const Fn &fn, const std::string &name, const std::string &expected_result );

};

class TestNotImplemented: public std::exception {
public:

	const char *what() const throw() {return "not implemented";}
};

template<typename Fn>
inline void TestSimple::runTest(const Fn& fn, const std::string &name, const std::string &expected_result ) {

	std::cout<<"Running test: " << name;
	for (std::size_t len = 50-name.length(); len; len--) std::cout << '.';
	std::cout.flush();
	try {
		std::ostringstream stream;
		fn(stream);
		std::string produced = stream.str();
		if (produced == expected_result) {
			std::cout<<"OK" << std::endl;
			if (showOutput) std::cout<<"\tProduced:" << produced << std::endl;
		} else {
			std::cout<<"FAILED" << std::endl;
			std::cout<<"\tExpected:" << expected_result << std::endl;
			std::cout<<"\tProduced:" << produced << std::endl;
			failed = true;
		}
	} catch (const TestNotImplemented &) {
		std::cout<<"Not Impl."<< std::endl;
	} catch (std::exception &e) {
		std::cout<<"FAILED"<< std::endl;
		std::cout<<"\tCrashed by exception: " << e.what() << std::endl;
		failed = true;
	}
}





#endif /* SRC_TESTS_TESTCLASS_H_ */
