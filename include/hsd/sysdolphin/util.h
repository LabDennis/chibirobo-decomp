#ifndef _util_h_
#define _util_h_

#include <global.h>

#include <dolphin/gx/gxtypes.h>
#include <sysdolphin/objalloc.h>

void HSD_MulColor(GXColor* arg0, GXColor* arg1, GXColor* dest);
s32 HSD_GetNbBits(u32 arg0);
s32 HSD_Index2PosNrmMtx(u32 index);
void HSD_ObjSetHeap(u32 size, void* ptr);

static inline f32 HSD_ClampFloat(f32 value, f32 min, f32 max)
{
	if (value <= min) {
		return min;
	}
	if (value >= max) {
		return max;
	}
	return value;
}

#endif
