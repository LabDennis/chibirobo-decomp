#include "PowerPC_EABI_Support/Runtime/NMWException.h"
#include "PowerPC_EABI_Support/Runtime/__ppc_eabi_linker.h"

static int fragmentID = -2;

extern "C" {
/**
 * @note Address: 0x80151BE0
 * @note Size: 0x40
 */
void __init_cpp_exceptions()
{
	if ((s32)fragmentID == -2) {
		register char* R2;
		asm { mr R2, r2 }
		fragmentID = __register_fragment(_eti_init_info, R2);
	}
}

/**
 * @note Address: 0x80151BAC
 * @note Size: 0x34
 */
void __fini_cpp_exceptions()
{
	if ((s32)fragmentID != -2) {
		__unregister_fragment(fragmentID);
		fragmentID = -2;
	}
}
}

DECL_SECT(".ctors") extern void* const __init_cpp_exceptions_reference  = __init_cpp_exceptions;
DECL_SECT(".dtors") extern void* const __destroy_global_chain_reference = __destroy_global_chain;
DECL_SECT(".dtors") extern void* const __fini_cpp_exceptions_reference  = __fini_cpp_exceptions;
