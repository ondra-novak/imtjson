#pragma once

#include <algorithm>

namespace json {


	class StackProtected {
	public:
		StackProtected();
		~StackProtected();
		void checkInstance() const;

	protected:
		std::uintptr_t cookie;
		static std::uintptr_t cookieValue;


	};

}