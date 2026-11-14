#include "revolution/nand.h"
#include "revolution/os_state_flags.h"
#include <string.h>

static char StateFlagsPath[] = "/title/00000001/00000002/data/state.dat";
static OSStateFlags StateFlags ATTRIBUTE_ALIGN(32);

static u32 CheckSum(const OSStateFlags* state)
{
	int i;
	u32 checksum = 0;

	for (i = 0; i < ARRAY_SIZE(state->data.words); i++) {
		checksum += state->data.words[i];
	}

	return checksum;
}

BOOL __OSWriteStateFlags(const OSStateFlags* state)
{
	NANDFileInfo file;

	memcpy(&StateFlags, state, sizeof(OSStateFlags));
	StateFlags.checksum = CheckSum(&StateFlags);

	if (NANDOpen(StateFlagsPath, &file, NAND_ACCESS_WRITE) == NAND_RESULT_OK) {
		if (NANDWrite(&file, &StateFlags, sizeof(OSStateFlags)) != sizeof(OSStateFlags)) {
			NANDClose(&file);
			return FALSE;
		}
		if (NANDClose(&file) != NAND_RESULT_OK) {
			return FALSE;
		}
	} else {
		return FALSE;
	}

	return TRUE;
}

BOOL __OSReadStateFlags(OSStateFlags* state)
{
	NANDFileInfo file;

	if (NANDOpen(StateFlagsPath, &file, NAND_ACCESS_READ) == NAND_RESULT_OK) {
		s32 bytesRead = NANDRead(&file, &StateFlags, sizeof(OSStateFlags));
		NANDClose(&file);

		if (bytesRead != sizeof(OSStateFlags)) {
			NANDDelete(StateFlagsPath);
			memset(state, 0, sizeof(OSStateFlags));
			return FALSE;
		}
	} else {
		memset(state, 0, sizeof(OSStateFlags));
		return FALSE;
	}

	if (CheckSum(&StateFlags) != StateFlags.checksum) {
		memset(state, 0, sizeof(OSStateFlags));
		return FALSE;
	}

	memcpy(state, &StateFlags, sizeof(OSStateFlags));
	return TRUE;
}
