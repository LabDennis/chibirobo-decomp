#ifndef REVOLUTION_OS_STATE_FLAGS_H
#define REVOLUTION_OS_STATE_FLAGS_H

#include "types.h"

typedef struct OSStateFlags {
	u32 checksum;
	union {
		u32 words[7];
		struct {
			u8 unknown04;
			u8 menuState;
			u8 discState;
			u8 unknown07;
			u32 reserved[6];
		} fields;
	} data;
} OSStateFlags;
STATIC_ASSERT(sizeof(OSStateFlags) == 0x20);

BEGIN_SCOPE_EXTERN_C
BOOL __OSWriteStateFlags(const OSStateFlags* state);
BOOL __OSReadStateFlags(OSStateFlags* state);
END_SCOPE_EXTERN_C

#endif
