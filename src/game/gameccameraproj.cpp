#include "game/hsd_jobj.h"
#include "game/ccamera.h"

/**
 * @note Address: 0x80013784
 * @note Size: 0x24
 */
void CCamera::SetFov(f32 fov)
{
	mFov = fov;
	HSD_CObjSetFov(this, fov);
}

/**
 * @note Address: 0x800137A8
 * @note Size: 0x8
 */
void CCamera::SetDistance(f32 distance)
{
	mDistance = distance;
}

/**
 * @note Address: 0x800137B0
 * @note Size: 0x8
 */
void CCamera::SetAngleX(f32 x)
{
	mAngle.x = x;
}

/**
 * @note Address: 0x800137B8
 * @note Size: 0x8
 */
void CCamera::SetAngleY(f32 y)
{
	mAngle.y = y;
}
