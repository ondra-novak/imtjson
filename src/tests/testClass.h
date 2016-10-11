/*
 * testClass.h
 *
 *  Created on: 10. 10. 2016
 *      Author: ondra
 */

#ifndef SRC_TESTS_TESTCLASS_H_
#define SRC_TESTS_TESTCLASS_H_
#include <sstream>


class TestSimple {
public:

	TestSimple():failed(false) {}

	class ChargedTest {
	public:
		ChargedTest(TestSimple &owner, const char *name, const char *expected_result)
			:owner(owner),name(name),expected_result(expected_result) {}

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

	template<typename Fn>
	void runTest(const Fn &fn, const std::string &name, const std::string &expected_result );

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
		} else {
			std::cout<<"FAILED" << std::endl;
			std::cout<<"\tExpected:" << expected_result << std::endl;
			std::cout<<"\tProduced:" << produced << std::endl;
			failed = true;
		}
	} catch (std::exception &e) {
		std::cout<<"FAILED"<< std::endl;
		std::cout<<"\tCrashed by exception: " << e.what() << std::endl;
		failed = true;
	}
}




#endif /* SRC_TESTS_TESTCLASS_H_ */
