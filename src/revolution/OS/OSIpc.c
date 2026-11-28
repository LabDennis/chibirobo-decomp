#include "types.h"

static void* IpcBufferHi = NULL;
static void* IpcBufferLo = (void*)0xFFFFFFFF;

void* __OSGetIPCBufferHi(void)
{
	return IpcBufferHi;
}

void* __OSGetIPCBufferLo(void)
{
	return IpcBufferLo;
}

void __OSInitIPCBuffer(void)
{
	IpcBufferLo = *(void**)0x80003130;
	IpcBufferHi = *(void**)0x80003134;
}
