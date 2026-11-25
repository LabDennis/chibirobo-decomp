#ifndef _GAME_HSD_COBJ_H
#define _GAME_HSD_COBJ_H

// c++ cobj.h and aobj.h inlines, assert lines from the binary.
// the CObj helper names are ours

extern "C" {
#include <dolphin/gx/gx.h>
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
#include <sysdolphin/aobj.h>
#include <sysdolphin/cobj.h>
}

#define COBJ_H_ASSERT(line, cond) ((cond) ? ((void)0) : __assert("cobj.h", line, #cond))
#define AOBJ_H_ASSERT(line, cond) ((cond) ? ((void)0) : __assert("aobj.h", line, #cond))

// Use the supplied view matrix and rebuild its inverse on demand.
inline void HSD_CObjSetViewingMtx(HSD_CObj* cobj, Mtx mtx)
{
	COBJ_H_ASSERT(0x1A2, cobj);
	cobj->flags |= COBJ_INV_DIRTY | COBJ_FIXED_VIEW_MTX;
	PSMTXCopy(mtx, cobj->view_mtx);
}

// Resume deriving the view matrix from the eye and interest.
inline void HSD_CObjClearViewingMtx(HSD_CObj* cobj)
{
	COBJ_H_ASSERT(0x1AE, cobj);
	cobj->flags &= ~COBJ_FIXED_VIEW_MTX;
}

// the reference drop cobj.c also has as a local, ref_DEC
inline BOOL HSD_CObjRefDec(HSD_CObj* cobj)
{
	BOOL ret;

	if ((ret = (HSD_OBJ(cobj)->ref_count == HSD_OBJ_NOREF))) {
		return ret;
	}
	return HSD_OBJ(cobj)->ref_count-- == 0;
}

// only seen inlined, unlike HSD_JObjUnref
inline void HSD_CObjUnref(HSD_CObj* cobj)
{
	if (cobj == NULL) {
		return;
	}
	if (HSD_CObjRefDec(cobj) && cobj != NULL) {
		HSD_CLASS_METHOD(cobj)->release((HSD_Class*)cobj);
		HSD_CLASS_METHOD(cobj)->destroy((HSD_Class*)cobj);
	}
}

inline f32 HSD_AObjGetEndFrame(HSD_AObj* aobj)
{
	AOBJ_H_ASSERT(0xAB, aobj);
	return aobj->end_frame;
}

inline f32 HSD_AObjGetCurrentFrame(HSD_AObj* aobj)
{
	AOBJ_H_ASSERT(0x95, aobj);
	return aobj->curr_frame;
}

#endif
