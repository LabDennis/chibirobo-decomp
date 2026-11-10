#include "game/xobjspiderboss.h"

/**
 * @note Address: 0x80105CDC
 * @note Size: 0x4
 */
extern "C" void DrawObjectEffects__14XObjSpiderBossFv(XObjSpiderBoss* spider)
{
}

/**
 * @note Address: 0x80105CE0
 * @note Size: 0x4
 */
extern "C" void DrawDebugStatus__14XObjSpiderBossFv(XObjSpiderBoss* spider)
{
}

/**
 * @note Address: 0x80105CE4
 * @note Size: 0x4
 */
extern "C" void vfC8__14XObjSpiderBossFv(XObjSpiderBoss* spider)
{
}

/**
 * @note Address: 0x80105CE8
 * @note Size: 0x20
 */
extern "C" void DrawGroundShadowAt__14XObjSpiderBossFPC3Vecf(XObjSpiderBoss* spider, const Vec* origin, f32 opacity)
{
	DrawGroundShadowAt__4XObjFPC3Vecf(spider, origin, opacity);
}

/**
 * @note Address: 0x80105D08
 * @note Size: 0x20
 */
extern "C" void DrawGroundShadow__14XObjSpiderBossFv(XObjSpiderBoss* spider)
{
	DrawGroundShadow__4XObjFv(spider);
}

/**
 * @note Address: 0x80105D28
 * @note Size: 0x30
 */
extern "C" void vfAC__14XObjSpiderBossFv(XObjSpiderBoss* spider)
{
	spider->DrawWithView(NULL);
}
