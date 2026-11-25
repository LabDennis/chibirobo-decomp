#ifndef _GAME_HSD_LOBJ_H
#define _GAME_HSD_LOBJ_H

extern "C" {
#include <sysdolphin/lobj.h>
}

inline void HSD_LObjUnref(HSD_LObj* lobj)
{
	if (lobj != NULL && ref_DEC(lobj)) {
		hsdDelete(lobj);
	}
}

#endif
