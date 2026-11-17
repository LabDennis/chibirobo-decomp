// The Wii's __ppc_eabi_init, after xenoblade's RVL_SDK os/__ppc_eabi_init.c: __init_user is an asm function
// with its own frame (as there and in Garigari), so __init_cpp stays a call, and exit runs the static
// destructors before it halts (retail 0x8018E654).
#include "revolution/os.h"
#include "Dolphin/PPCArch.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*voidfunctionptr)(); // pointer to function returning void
DECL_SECT(".ctors") extern voidfunctionptr _ctors[];
DECL_SECT(".dtors") extern voidfunctionptr _dtors[];

static void __init_cpp();

DECL_SECT(".init")
ASM void __init_hardware() {
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mfmsr r0
	ori r0,r0,0x2000
	mtmsr r0
	mflr r31
	bl __OSPSInit
	bl __OSFPRInit
	bl __OSCacheInit
	mtlr r31
	blr
#endif // clang-format on
}

DECL_SECT(".init") ASM void __flush_cache(u32 addr, int size)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc
	lis r5, 0xFFFFFFF1@h
	ori r5, r5, 0xFFFFFFF1@l
	and r5, r5, r3
	subf r3, r5, r3
	add r4, r4, r3
loop:
	dcbst 0, r5
	sync
	icbi 0, r5
	addic r5, r5, 8
	addic. r4, r4, -8
	bge loop
	isync
	blr
#endif // clang-format on
}

ASM void __init_user()
{
#ifdef __MWERKS__ // clang-format off
	fralloc
	bl __init_cpp
	frfree
	blr
#endif // clang-format on
}

static void __init_cpp()
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
