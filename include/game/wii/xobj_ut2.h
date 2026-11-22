#ifndef _GAME_WII_XOBJ_UT2_H
#define _GAME_WII_XOBJ_UT2_H

// the second half of the object utility header: the archive's extra animation file finishes
// loading inside the object's update
#include "game/wii/cresfile.h"

// once the archive's file read finished, its resources bind to the object's file and the
// archive becomes the anims' name source; name from the panic file/line it carries
static inline void UpdateArchiveAnmSource(XObj* object)
{
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
		nw4r::g3d::ResFile file = object->mArchive.mAnm.m_resFile;
		file.Bind(object->mObject.m_resFile);
		object->SetAnmSource(&object->mArchive.mAnm);
		object->mStatus.mFlags &= ~0x80000000000000ULL;
	}
}

#endif
