#ifndef _GAME_HSD_DOBJ_H
#define _GAME_HSD_DOBJ_H

// c++ dobj.h and pobj.h getters, missing from our c headers
// todo: copied inlines, clean up
extern "C" {
#include <sysdolphin/dobj.h>
#include <sysdolphin/pobj.h>
}

inline HSD_MObj* HSD_DObjGetMObj(HSD_DObj* dobj)
{
	return dobj ? dobj->mobj : NULL;
}

inline HSD_PObj* HSD_DObjGetPObj(HSD_DObj* dobj)
{
	return dobj ? dobj->pobj : NULL;
}

inline HSD_DObj* HSD_DObjGetNext(HSD_DObj* dobj)
{
	return dobj ? dobj->next : NULL;
}

static inline HSD_PObj* HSD_PObjGetNext(HSD_PObj* pobj)
{
	return pobj ? pobj->next : NULL;
}

#endif
