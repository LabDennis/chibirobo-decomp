// the scene camera's file: the CCamera virtuals the game defines here, the
// XObjScnCam/XObjLight/XObjMap overrides in 0x800C6584-0x800C7DC0 (wii_units.md Nr. 58),
// and the CustomShadow ctor/draw pair
#include "Version.h"

#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/globals.h"
#include "game/ut.h"

extern MEMAllocator gWiiPadAllocator;

// nw4r's db pointer check (db_assert.h) as the game's sources use it; the line is the game's own
#define NW4R_ASSERT_VALID_PTR_LINE(line, exp)                                                                                              \
	((void)((((u32)(exp) & 0xFF000000) == 0x80000000 || ((u32)(exp) & 0xFF800000) == 0x81000000 || ((u32)(exp) & 0xF8000000) == 0x90000000 \
	         || ((u32)(exp) & 0xFF000000) == 0xC0000000 || ((u32)(exp) & 0xFF800000) == 0xC1000000                                         \
	         || ((u32)(exp) & 0xF8000000) == 0xD0000000 || ((u32)(exp) & 0xFFFFC000) == 0xE0000000)                                        \
	        || (nw4r::db::Panic(__FILE__, line, "NW4R:Pointer Error\n" #exp "(=%p) is not valid pointer.", (exp)), 0)))

// the small scene-camera record this file's functions work with: Init fills it with
// its defaults; the class name and its other members are not recovered
extern "C" void fn_800C659C(u32* self, u32 arg)
{
	self[2]                   = arg;
	*(u16*)((u8*)self + 4)    = 0;
	*(u16*)((u8*)self + 2)    = 0;
	self[3]                   = 0;
	*(f32*)((u8*)self + 0x10) = 0.0f / 0.0f;
	self[8]                   = 0;
	*(f32*)((u8*)self + 0x28) = 0.0f / 0.0f;
	*(f32*)((u8*)self + 0x24) = 0.0f / 0.0f;
	*(f32*)((u8*)self + 0x2C) = 0.0f;
	*(f32*)((u8*)self + 0x30) = 0.0f;
	*(f32*)((u8*)self + 0x34) = 0.0f;
	*(f32*)((u8*)self + 0x38) = 0.0f;
	*(f32*)((u8*)self + 0x3C) = 0.0f;
	*(f32*)((u8*)self + 0x40) = 0.0f;
	*(f32*)((u8*)self + 0x44) = 0.0f;
	*(f32*)((u8*)self + 0x4C) = 0.0f;
	*(f32*)((u8*)self + 0x50) = 0.0f;
	*(f32*)((u8*)self + 0x54) = 0.0f;
	*(f32*)((u8*)self + 0x5C) = 0.0f;
	*(f32*)((u8*)self + 0x60) = 0.0f;
	*(f32*)((u8*)self + 0x64) = 0.0f;
}

// the head bone's world position, or the object's own position when the model has no
// "head_chn" node; the second argument's role is not recovered
extern "C" CVec fn_800C6608(u32, XObj* obj)
{
	if (obj->mObject.m_pScnMdl != NULL) {
		nw4r::g3d::ResNode node = obj->GetResNode("head_chn");
		if (node.IsValid()) {
			return obj->GetWorldPos(node);
		}
	}
	return *obj->GetPos();
}

/**
 * @note Address: 0x800C733C
 * @note Size: 0x1A8
 */
BOOL XObjScnCam::LoadFromPlacement(const CoordInfo& info)
{
	if (!XObj::LoadFromPlacement(info)) {
		return FALSE;
	}
	if (mObject.m_pScnMdl != NULL && mObject.m_resFile.HasResAnmScn() && mObject.m_resFile.GetResAnmScnNumEntries() != 0) {
		mScene = (WiiGsSceneObj*)nw4r::g3d::ScnRoot::Construct(&gWiiPadAllocator, NULL, 1, 1, 1, 1);
		NW4R_ASSERT_VALID_PTR_LINE(0x20, mScene);
	}
	return TRUE;
}

/**
 * @note Address: 0x800C74E4
 * @note Size: 0x98
 */
void XObjScnCam::Release()
{
	if (mScene != NULL) {
		gGame.mMotionControlCamera.vf2C();
		gGame.mMotionControlCamera.vf68();
		ClearAnim();
		((nw4r::g3d::ScnRoot*)mScene)->Destroy();
		mScene = NULL;
	}
	XObj::Release();
}

/**
 * @note Address: 0x800C757C
 * @note Size: 0x14
 */
void CCamera::vf2C()
{
	mProjectionSource = NULL;
	mProjectionDirty  = TRUE;
}

/**
 * @note Address: 0x800C7590
 * @note Size: 0x14
 */
void CCamera::vf68()
{
	mViewSource  = NULL;
	mView.mDirty = TRUE;
}

/**
 * @note Address: 0x800C75A4
 * @note Size: 0x4C
 */
void XObjScnCam::UpdateObject(u32 frames)
{
	XObj::UpdateObject(frames);
	if (mScene != NULL) {
		nw4r::g3d::ScnRoot* scene = (nw4r::g3d::ScnRoot*)mScene;
		scene->CalcWorld();
		scene->CalcView();
		scene->UpdateFrame();
	}
}

/**
 * @note Address: 0x800C75F0
 * @note Size: 0x4
 */
void XObjScnCam::DrawObjectEffects()
{
}

/**
 * @note Address: 0x800C75F4
 * @note Size: 0xFC
 */
// NONMATCHING: the discarded else-call of GetResAnmScnNumEntries is what retail emits;
// whether the source asked for the count in an assertion or a check is not known
void XObjScnCam::SetAnmScn(u32 id, f32 frame, f32 rate)
{
	if (mScene != NULL) {
		if (id < mObject.m_resFile.GetResAnmScnNumEntries()) {
			CObj::SetAnmScn(id, frame, rate);
			nw4r::g3d::ScnRoot* scene = (nw4r::g3d::ScnRoot*)mScene;
			scene->SetAnmScn(mObject.m_resFile.HasResAnmScn() ? mObject.m_resources.scn : NULL);
			scene->CalcWorld();
			scene->CalcView();
			Vec zero = { 0, 0, 0 };
			m_9AC    = zero;
			m_9B8    = 0.0f;
		} else {
			mObject.m_resFile.GetResAnmScnNumEntries();
		}
	}
}

/**
 * @note Address: 0x800C76F0
 * @note Size: 0x74
 */
void XObjScnCam::ReleaseModelWork()
{
	if (mScene != NULL) {
		((nw4r::g3d::ScnRoot*)mScene)->RemoveAnmScn();
		mObject.ReleaseWork();
	}
	gGame.mMotionControlCamera.vf2C();
	gGame.mMotionControlCamera.vf68();
}

/**
 * @note Address: 0x800C7C1C
 * @note Size: 0x24
 */
void CCamera::SetUp(const Vec& up)
{
	mView.mUp.Set(up.x, up.y, up.z);
	mView.mDirty = TRUE;
}

/**
 * @note Address: 0x800C7C40
 * @note Size: 0xC
 */
void XObjScnCam::InitializeObject()
{
	mScene = NULL;
}

/**
 * @note Address: 0x800C7C58
 * @note Size: 0x30
 */
void XObjLight::ResetObjectState()
{
	m_9A8 = 0;
	for (int i = 0; i < 8; i++) {
		mLightIds[i] = -1;
	}
}

/**
 * @note Address: 0x800C7C88
 * @note Size: 0x2C
 */
BOOL XObjLight::LoadFromPlacement(const CoordInfo& info)
{
	return XObj::LoadFromPlacement(info) != FALSE;
}

/**
 * @note Address: 0x800C7CB8
 * @note Size: 0x4
 */
void XObjLight::UpdateObject(u32 frames)
{
}

/**
 * @note Address: 0x800C7CC4
 * @note Size: 0x34
 */
void XObjMap::InitializeObject()
{
	XObj::InitializeObject();
	m_1374 = 0;
}

/**
 * @note Address: 0x800C7CF8
 * @note Size: 0xC8
 */
void XObjMap::ResetObjectState()
{
	XObj::ResetObjectState();
	for (int i = 0; i < 2; i++) {
		u32* fields = (u32*)&_1378[i * 0x40];
		for (int k = 0; k < 8; k++) {
			if (fields[k * 2] != 0) {
				fields[k * 2] = 0;
			}
		}
	}
}
