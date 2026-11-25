#ifndef _GAME_ANIMFRAME_H
#define _GAME_ANIMFRAME_H

#include "game/ut.h"
#include "game/hsd_cobj.h"

// the frame the jobj's first anim is at, and the frame it ends at
static inline f32 GetAnimFrame(HSD_JObj* jobj)
{
	HSD_AObj* aobj = GetAObj(jobj);
	return aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
}

static inline f32 GetAnimEndFrame(HSD_JObj* jobj)
{
	HSD_AObj* aobj = GetAObj(jobj);
	return aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
}

#endif
