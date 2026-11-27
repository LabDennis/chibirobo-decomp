#include "game/cgame.h"
#include "game/cjobjinlineattach.h"
#include "game/cparticle.h"
#include "game/stagedata.h"
#include "game/xchr.h"
#include "game/xobjs.h"

extern "C" {
#include <sysdolphin/mtx_transform.h>
}

extern "C" {
void ConfigureSubwindowCamera(FbSubwin* subwin, const Vec& interest, f32 angleY, f32 angleX, f32 distance, f32 fov);

/**
 * @note Address: 0x800D3A5C
 * @note Size: 0x94
 */
HSD_Generator* CreateParticleAtPosition(CGame* game, u32 bank, u32 id, s32 status, const Vec& pos, BOOL keep)
{
	const Vec rotation = { 0.0f, 0.0f, 0.0f };
	const Vec scale    = { 1.0f, 1.0f, 1.0f };
	return game->mParticle.CreateSRT(id | (bank << 16), status, pos, scale, rotation, keep);
}

/**
 * @note Address: 0x800D3AF0
 * @note Size: 0xB4
 */
Vec GetJointMatrixScale(const CJObj*, HSD_JObj* jobj)
{
	Vec scale;
	HSD_MtxGetScale(HSD_JObjGetMtxPtr(jobj), &scale);
	return scale;
}

/**
 * @note Address: 0x800D3BA4
 * @note Size: 0x5C
 */
HSD_Generator* CreateScaledParticleAtPosition(CGame* game, u32 bank, u32 id, s32 status, const Vec& pos, const Vec& scale, BOOL keep)
{
	const Vec rotation = { 0.0f, 0.0f, 0.0f };
	return game->mParticle.CreateSRT(id | (bank << 16), status, pos, scale, rotation, keep);
}

/**
 * @note Address: 0x800D3C00
 * @note Size: 0xB8
 */
Vec GetObjectJointMatrixScale(const CJObj* obj)
{
	HSD_JObj* jobj = obj->mJObj;
	Vec scale;
	HSD_MtxGetScale(HSD_JObjGetMtxPtr(jobj), &scale);
	return scale;
}

/**
 * @note Address: 0x800D3CB8
 * @note Size: 0x24
 */
s32 RemovePlayerInventoryItemCount(Params* params, s16 id, s32 amount)
{
	return RemoveInventoryItemCount(&params->mInventory, id, amount);
}

/**
 * @note Address: 0x800D3CDC
 * @note Size: 0x30
 */
BOOL IsObjectLoaded(XObj* obj)
{
	return !(obj->mStatus.mFlags & 0x0040000000000000ULL);
}

/**
 * @note Address: 0x800D3D0C
 * @note Size: 0x64
 */
XObj* SpawnObjectFromPlacementIndex(CGame* game, s16 id)
{
	CoordInfo info(GetStageObjectPlacementByIndex(id));
	return SpawnObjectFromPlacement(&game->mXObjs, &info, 0, TRUE);
}

/**
 * @note Address: 0x800D3D70
 * @note Size: 0x44
 */
void SetClampedPlayerBattery(Params* params, f32 battery)
{
	params->SetBattery(battery);
}

/**
 * @note Address: 0x800D3DB4
 * @note Size: 0x20
 */
void EnableSubwindowWithFlags(CGame* game, u32 flags)
{
	game->mSubwin.mEnabled = TRUE;
	game->mSubwin.mFlags.Clear();
	game->mSubwin.mFlags.mFlags |= flags;
}

/**
 * @note Address: 0x800D3DD4
 * @note Size: 0x24
 */
void SetSubwindowCamera(CGame* game, const Vec& interest, f32 angleY, f32 angleX, f32 distance, f32 fov)
{
	ConfigureSubwindowCamera(&game->mSubwin, interest, angleY, angleX, distance, fov);
}

/**
 * @note Address: 0x800D3DF8
 * @note Size: 0xB8
 */
Vec GetObjectJointWorldPosition(const CJObj* obj)
{
	HSD_JObj* jobj = obj->mJObj;
	Vec pos;
	HSD_MtxGetTranslate(HSD_JObjGetMtxPtr(jobj), &pos);
	return pos;
}

/**
 * @note Address: 0x800D3EB0
 * @note Size: 0x14
 */
void SetSubwindowRect(CGame* game, f32 x, f32 y, f32 width, f32 height)
{
	game->mSubwin.mRectX      = x;
	game->mSubwin.mRectY      = y;
	game->mSubwin.mRectWidth  = width;
	game->mSubwin.mRectHeight = height;
}

/**
 * @note Address: 0x800D3EC4
 * @note Size: 0x50
 */
void ClearObjectAnimationQueue(XObj* obj)
{
	obj->mAnmQueue.Clear();
}

} // extern "C"

/**
 * @note Address: 0x800D3F14
 * @note Size: 0x1D8
 */
CXObj::CXObj(HSD_JObj* jobj)
    : CJObj(jobj)
{
	SyncMoveFromJoint(this);
}

extern "C" {

/**
 * @note Address: 0x800D40EC
 * @note Size: 0x164
 */
void SyncMoveFromJoint(CXObj* obj)
{
	Vec translate;
	Quaternion rotate;
	Vec scale;
	HSD_JObjGetTranslation(obj->mJObj, &translate);
	obj->mMoveTranslate.Reset(translate);
	HSD_JObjGetRotation(obj->mJObj, &rotate);
	obj->mMoveRotate.Reset(rotate.x, rotate.y, rotate.z);
	HSD_JObjGetScale(obj->mJObj, &scale);
	obj->mMoveScale.Reset(scale);
}

/**
 * @note Address: 0x800D4250
 * @note Size: 0x44
 */
f32 GetObjectJointRotationY(CJObj* obj)
{
	return HSD_JObjGetRotationY(obj->mJObj);
}

/**
 * @note Address: 0x800D4294
 * @note Size: 0x60
 */
void SetPlayerStandAnim(XObjChr* chr, s32 anim, f32 blend)
{
	chr->SetAnim(anim, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, blend, 10.0f);
	chr->mIdleLoops = 3;
}

/**
 * @note Address: 0x800D42F4
 * @note Size: 0x3C
 */
HSD_Generator* CreateParticleByBankAndId(CGame* game, s32 bank, s32 id, u8 type, HSD_JObj* jobj, BOOL keep)
{
	const u32 particleId = static_cast<u32>(id) | (static_cast<u32>(bank) << 16);
	return game->mParticle.Create(particleId, type, jobj, keep);
}

/**
 * @note Address: 0x800D4330
 * @note Size: 0x10
 */
s32 GetInventoryCountByIndex(Params* params, u32 index)
{
	return params->mInventory.mItems[index].mCount;
}

/**
 * @note Address: 0x800D4340
 * @note Size: 0x28
 */
s16 GetObjectItemId(XObj* obj)
{
	return obj->GetItem();
}

} // extern "C"
