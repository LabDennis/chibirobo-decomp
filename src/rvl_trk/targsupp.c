#include "PowerPC_EABI_Support/MetroTRK/trk.h"

// The original is an assembled file (targsupp.s): each routine traps into the debugger, which does the file work.

ASM u32 TRKAccessFile(u32 command, u32 handle, u32* count, u8* buffer) {
#ifdef __MWERKS__ // clang-format off
	nofralloc
	twi 31, r0, 0
	blr
#endif // clang-format on
}

ASM u32 TRKOpenFile(u32 command, u32 handle, u32* count, u8* buffer) {
#ifdef __MWERKS__ // clang-format off
	nofralloc
	twi 31, r0, 0
	blr
#endif // clang-format on
}

ASM u32 TRKCloseFile(u32 command, u32 handle) {
#ifdef __MWERKS__ // clang-format off
	nofralloc
	twi 31, r0, 0
	blr
#endif // clang-format on
}

ASM u32 TRKPositionFile(u32 command, u32 handle, u32* count, u8* buffer) {
#ifdef __MWERKS__ // clang-format off
	nofralloc
	twi 31, r0, 0
	blr
#endif // clang-format on
}
