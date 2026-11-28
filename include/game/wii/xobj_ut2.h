#ifndef _GAME_WII_XOBJ_UT2_H
#define _GAME_WII_XOBJ_UT2_H

// the second half of the object utility header: the archive's extra animation file finishes
// loading inside the object's update
#include "game/wii/cresfile.h"

inline BOOL StartArchiveAnmRead(XObj* object, const char* path)
{
	s32 result = object->mArchive.mAnm.mFile.ReadAsync(path, 0, 0, FALSE);
	return result != 0 ? TRUE : (OSPanic("xobj_ut2.h", 0xe2, ""), FALSE);
}

// once the archive's file read finished, its resources bind to the object's file and the
// archive becomes the anims' name source; name from the panic file/line it carries
inline void UpdateArchiveAnmSource(XObj* object)
{
	if (object->mStatus.mFlags & 0x80000000000000ULL) {
		BOOL ok;
		s32 result = object->mArchive.mAnm.mFile.GetResult();
		if (result == 0) {
			((CResFile*)&object->mArchive.mAnm)->Init(object->mArchive.mAnm.mFile.mBuffer, TRUE, FALSE);
			ok = TRUE;
		} else {
			if (result < 0) {
				OSPanic("xobj_ut2.h", 0xee, "");
			}
			ok = FALSE;
		}
		if (ok) {
			object->mArchive.mAnm.GetResFile().Bind(object->mObject.GetResFile());
			object->SetAnmSource(&object->mArchive.mAnm);
			object->mStatus.mFlags &= ~0x80000000000000ULL;
		}
	}
}

#endif
