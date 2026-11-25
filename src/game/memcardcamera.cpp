#include "game/hsd_jobj.h"
#include "game/ccamera.h"

/**
 * @note Address: 0x80087C88
 * @note Size: 0x38
 */
void CCamera::SetAngle(const Vec& angle)
{
	SetAngle(angle.x, angle.y, angle.z);
}
