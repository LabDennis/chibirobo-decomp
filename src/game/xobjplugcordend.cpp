#include "game/xobjattachment.h"

/**
 * @note Address: 0x800DB7A8
 * @note Size: 0x30
 */
extern "C" void SetPlugCordAlphaMode(XObjPlugCord* cord, BOOL enabled, u8 alpha)
{
	if (enabled) {
		cord->mFlags |= 6;
		cord->mAlpha = alpha;
	} else {
		cord->mFlags &= ~6;
		cord->mAlpha = alpha;
	}
}

// ties one cord end to a position
/**
 * @note Address: 0x800DB7D8
 * @note Size: 0x38
 */
extern "C" void SetPlugCordEndpointSource(XObjPlugCord* cord, u32 index, const Vec* source)
{
	cord->mEndSource[index] = source;
	if (source != NULL) {
		cord->mEnd[index].x = source->x;
		cord->mEnd[index].y = source->y;
		cord->mEnd[index].z = source->z;
	}
}
