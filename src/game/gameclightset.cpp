#include "game/hsd_jobj.h"
#include "game/clight.h"

/**
 * @note Address: 0x8000DF78
 * @note Size: 0x30
 */
void CLight::SetPosition(f32 x, f32 y, f32 z)
{
	mPosition.x = x;
	mPosition.y = y;
	mPosition.z = z;
	HSD_LObjSetPosition(this, &mPosition);
}
