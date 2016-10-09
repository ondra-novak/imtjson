#include "stackProtection.h" 
#include <stdexcept>
#include <time.h>

namespace json {




	StackProtected::StackProtected():cookie(cookieValue)
	{
	}

	StackProtected::~StackProtected()
	{
		cookie = ~cookie;
	}

	void StackProtected::checkInstance() const
	{
		if (cookie != cookieValue)
			throw std::runtime_error("Attempt to work with already destroyed object");
	}

	std::uintptr_t StackProtected::cookieValue = (std::uintptr_t)time(0);
}