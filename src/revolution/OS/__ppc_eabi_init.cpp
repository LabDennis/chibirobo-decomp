// The Wii's __ppc_eabi_init, after xenoblade's and ogws' RVL_SDK os/__ppc_eabi_init.c:
// __init_hardware, __flush_cache and __init_user are assembly functions and live in
// os_ppc_eabi_init.s here. exit runs the static destructors before it halts
// (retail 0x8018E654).
// ogws: https://github.com/doldecomp/ogws/blob/master/src/revolution/OS/__ppc_eabi_init.c
#include "revolution/os.h"
#include "Dolphin/PPCArch.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*voidfunctionptr)(); // pointer to function returning void
DECL_SECT(".ctors") extern voidfunctionptr _ctors[];
DECL_SECT(".dtors") extern voidfunctionptr _dtors[];


void __init_cpp()
{
	voidfunctionptr* constructor;

	// call static initializers
	for (constructor = _ctors; *constructor; constructor++) {
		(*constructor)();
	}
}

void __fini_cpp()
{
	voidfunctionptr* destructor;

	// call static destructors
	for (destructor = _dtors; *destructor; destructor++) {
		(*destructor)();
	}
}

void _ExitProcess();

void exit(int status)
{
	__fini_cpp();
	_ExitProcess();
}

void _ExitProcess()
{
	PPCHalt();
}

#ifdef __cplusplus
}
#endif
