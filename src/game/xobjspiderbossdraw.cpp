#include "game/xobjspiderboss.h"

/**
 * @note Address: 0x80105CDC
 * @note Size: 0x4
 */
void XObjSpiderBoss::DrawObjectEffects()
{
}

/**
 * @note Address: 0x80105CE0
 * @note Size: 0x4
 */
void XObjSpiderBoss::DrawDebugStatus()
{
}

/**
 * @note Address: 0x80105CE4
 * @note Size: 0x4
 */
void XObjSpiderBoss::vfC8()
{
}

/**
 * @note Address: 0x80105CE8
 * @note Size: 0x20
 */
void XObjSpiderBoss::DrawGroundShadowAt(const Vec* origin, f32 opacity)
{
	DrawGroundShadowAt__4XObjFPC3Vecf(this, origin, opacity);
}

/**
 * @note Address: 0x80105D08
 * @note Size: 0x20
 */
void XObjSpiderBoss::DrawGroundShadow()
{
	DrawGroundShadow__4XObjFv(this);
}

/**
 * @note Address: 0x80105D28
 * @note Size: 0x30
 */
void XObjSpiderBoss::vfAC()
{
	DrawWithView(NULL);
}
