/*
 * nameToEnum.cpp
 *
 *  Created on: 25.3.2014
 *      Author: ondra
 */

#include "namedEnum.h"

#include <sstream>

namespace json {


UnknownEnumException::UnknownEnumException(std::string&& errorEnum,
		std::vector<std::string_view>&& availableEnums)
:listEnums(std::move(availableEnums))
,errorEnum(std::move(errorEnum))
{
}

const char* UnknownEnumException::what() const throw () {

	if (whatMsg.empty()) {

		std::ostringstream pr;
		pr << "Unknown enum name '" << errorEnum << "'. The value is not on following list:";
		char sep = ' ';
		for (auto x : listEnums) {
			pr << sep << "'" << x << "'";
			sep = ',';
		}
		whatMsg = pr.str();
	}

	return whatMsg.c_str();

}


}
