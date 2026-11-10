#include "types.h"
#include "PowerPC_EABI_Support/Runtime/exception.h"

extern "C" WEAK const char* what__Q23std9exceptionCFv(const std::exception*)
{
	return "exception";
}
