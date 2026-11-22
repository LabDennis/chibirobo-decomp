#include "game/hsd_jobj.h"
#include "game/ccamera.h"

/**
 * @note Address: 0x8001353C
 * @note Size: 0x20
 */
void CCamera::SetScissor(s32 left, u16 right, s32 top, u16 bottom)
{
	HSD_CObjSetScissorx4(this, left, right, top, bottom);
}

/**
 * @note Address: 0x8001355C
 * @note Size: 0x20
 */
void CCamera::SetViewport(f32 left, f32 right, f32 top, f32 bottom)
{
	HSD_CObjSetViewportfx4(this, left, right, top, bottom);
}
