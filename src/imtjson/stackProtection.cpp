#include "stackProtection.h" 
#include <stdexcept>
#include <time.h>

namespace json {

#ifdef _WIN32
	//windows optimization is unable to see why the destructor must be called.
	//so the destructor is removed (value tested beyond lifetime of the object)
	//that why need to disable optimizations here
#pragma optimize("", off)
#endif

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

	const std::uintptr_t StackProtected::cookieValue = (std::uintptr_t)time(0);
}
