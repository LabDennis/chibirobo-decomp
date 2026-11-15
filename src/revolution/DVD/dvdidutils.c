#include "revolution/hw_regs.h"

#include "string.h"
#include "types.h"

// shortcut: "Dolphin/dvd.h" declares DVDCompareDiskID without const, which retail's code needs;
// include that header instead of this copy once its prototype takes const pointers.
typedef struct DVDDiskID {
	char gameName[4];
	char company[2];
	u8 diskNumber;
	u8 gameVersion;
	u8 streaming;
	u8 streamBufSize;
	u8 padding[22];
} DVDDiskID;

BOOL DVDCompareDiskID(const DVDDiskID* id1, const DVDDiskID* id2)
{
	if (id1->gameName[0] != '\0' && id2->gameName[0] != '\0' && strncmp(id1->gameName, id2->gameName, 4) != 0) {
		return FALSE;
	}

	if (id1->company[0] == '\0' || id2->company[0] == '\0' || strncmp(id1->company, id2->company, 2) != 0) {
		return FALSE;
	}

	if (id1->diskNumber != 0xFF && id2->diskNumber != 0xFF && id1->diskNumber != id2->diskNumber) {
		return FALSE;
	}

	if (id1->gameVersion != 0xFF && id2->gameVersion != 0xFF && id1->gameVersion != id2->gameVersion) {
		return FALSE;
	}

	return TRUE;
}
