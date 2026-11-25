#include "types.h"
#include "string.h"
#include "Dolphin/dvd.h"

#define DVD_DISK_ID_WILDCARD 0xFF

/**
 * @note Address: 0x80170774
 * @note Size: 0xF8
 */
BOOL DVDCompareDiskID(const DVDDiskID* id1, const DVDDiskID* id2)
{
	if (id1->gameName[0] != 0 && id2->gameName[0] != 0
	    && strncmp(id1->gameName, id2->gameName, sizeof(id1->gameName)) != 0) {
		return FALSE;
	}
	if (id1->company[0] == 0 || id2->company[0] == 0
	    || strncmp(id1->company, id2->company, sizeof(id1->company)) != 0) {
		return FALSE;
	}

	if (id1->diskNumber != DVD_DISK_ID_WILDCARD && id2->diskNumber != DVD_DISK_ID_WILDCARD
	    && id1->diskNumber != id2->diskNumber) {
		return FALSE;
	}

	if (id1->gameVersion != DVD_DISK_ID_WILDCARD && id2->gameVersion != DVD_DISK_ID_WILDCARD
	    && id1->gameVersion != id2->gameVersion) {
		return FALSE;
	}
	return TRUE;
}
