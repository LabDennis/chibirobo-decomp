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

// in this revision's cobj.c, missing from our cobj.h
void HSD_CObjAddAnim(HSD_CObj* cobj, HSD_CameraAnim* anim);
void HSD_CObjAnim(HSD_CObj* cobj);
void HSD_CObjReqAnim(HSD_CObj* cobj, f32 frame);
void HSD_CObjSetTop(HSD_CObj* cobj, f32 top);
void HSD_CObjSetBottom(HSD_CObj* cobj, f32 bottom);
void HSD_CObjSetLeft(HSD_CObj* cobj, f32 left);
void HSD_CObjSetRight(HSD_CObj* cobj, f32 right);
u32 HSD_CObjGetFlags(HSD_CObj* cobj);
void HSD_CObjInit(HSD_CObj* cobj, HSD_CObjDesc* desc);
}

// A perspective camera's desc stops after fov and aspect. CCamera builds one on the stack
// from a 0x38 byte .rodata template; frustum and ortho descs use the full 0x40. name is ours
typedef struct HSD_CObjPerspectiveDesc {
	char* class_name;                    // _00
	u16 flags;                           // _04
	u16 projection_type;                 // _06
	HSD_RectS16 viewport;                // _08
	Scissor scissor;                     // _10
	struct _HSD_WObjDesc* eye_desc;      // _18
	struct _HSD_WObjDesc* interest_desc; // _1C
	f32 roll;                            // _20
	Vec* vector;                         // _24
	f32 near;                            // _28
	f32 far;                             // _2C
	f32 fov;                             // _30
	f32 aspect;                          // _34
} HSD_CObjPerspectiveDesc;
typedef char HSD_CObjPerspectiveDesc_size_check[sizeof(HSD_CObjPerspectiveDesc) == 0x38 ? 1 : -1];

#define COBJ_H_ASSERT(line, cond) ((cond) ? ((void)0) : __assert("cobj.h", line, #cond))

// hands the camera a finished view matrix. bit 1 makes hsd use it as is, bit 31 marks the
// inverse dirty
inline void HSD_CObjSetViewingMtx(HSD_CObj* cobj, Mtx mtx)
{
	COBJ_H_ASSERT(0x1A2, cobj);
	cobj->flags |= 0x80000002;
	PSMTXCopy(mtx, cobj->view_mtx);
}

// back to building the view from the eye and interest
inline void HSD_CObjClearViewingMtx(HSD_CObj* cobj)
{
	COBJ_H_ASSERT(0x1AE, cobj);
	cobj->flags &= ~2;
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

inline f32 HSD_AObjGetEndFrame(HSD_AObj* aobj, const char* file, const char* condition)
{
	if (!aobj)
		__assert(file, 0xAB, condition);
	return aobj->end_frame;
}

inline f32 HSD_AObjGetEndFrame(HSD_AObj* aobj)
{
	return HSD_AObjGetEndFrame(aobj, "aobj.h", "aobj");
}

inline f32 HSD_AObjGetCurrentFrame(HSD_AObj* aobj, const char* file, const char* condition)
{
	if (!aobj)
		__assert(file, 0x95, condition);
	return aobj->curr_frame;
}

inline f32 HSD_AObjGetCurrentFrame(HSD_AObj* aobj)
{
	return HSD_AObjGetCurrentFrame(aobj, "aobj.h", "aobj");
}

#endif
