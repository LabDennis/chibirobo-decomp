// the scene camera's file: the CCamera virtuals the game defines here, the
// XObjScnCam/XObjLight/XObjMap overrides in 0x800C6584-0x800C7DC0 (wii_units.md Nr. 58),
// and the CustomShadow ctor/draw pair
#include "Version.h"

#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/globals.h"
#include "game/ut.h"

#include <math.h>

extern MEMAllocator gWiiPadAllocator;

// nw4r's db pointer check (db_assert.h) as the game's sources use it; the line is the game's own
#define NW4R_ASSERT_VALID_PTR_LINE(line, exp)                                                                                              \
	((void)((((u32)(exp) & 0xFF000000) == 0x80000000 || ((u32)(exp) & 0xFF800000) == 0x81000000 || ((u32)(exp) & 0xF8000000) == 0x90000000 \
	         || ((u32)(exp) & 0xFF000000) == 0xC0000000 || ((u32)(exp) & 0xFF800000) == 0xC1000000                                         \
	         || ((u32)(exp) & 0xF8000000) == 0xD0000000 || ((u32)(exp) & 0xFFFFC000) == 0xE0000000)                                        \
	        || (nw4r::db::Panic(__FILE__, line, "NW4R:Pointer Error\n" #exp "(=%p) is not valid pointer.", (exp)), 0)))

// the small scene-camera record this file's functions work with: Init fills it with
// its defaults; the class name and its other members are not recovered
struct XObjScnCamInitRec {
	u16 m_00;
	u16 m_02;
	u16 m_04;
	u16 m_06;
	u32 m_08;
	u32 m_0C;
	f32 m_10;
	u32 m_14;
	u32 m_18;
	u32 m_1C;
	u32 m_20;
	f32 m_24;
	f32 m_28;
	f32 m_2C;
	f32 m_30;
	f32 m_34;
	f32 m_38;
	f32 m_3C;
	f32 m_40;
	f32 m_44;
	u32 m_48;
	f32 m_4C;
	f32 m_50;
	f32 m_54;
	u32 m_58;
	f32 m_5C;
	f32 m_60;
	f32 m_64;
};

extern "C" void fn_800C659C(XObjScnCamInitRec* self, u32 arg)
{
	self->m_08 = arg;
	self->m_04 = 0;
	self->m_02 = 0;
	self->m_0C = 0;
	self->m_10 = NAN;
	self->m_20 = 0;
	self->m_24 = self->m_28 = NAN;
	self->m_2C = self->m_30 = 0.0f;
	self->m_34 = self->m_38 = 0.0f;
	self->m_3C              = 0.0f;
	self->m_40              = 0.0f;
	self->m_44              = 0.0f;
	self->m_4C              = 0.0f;
	self->m_50              = 0.0f;
	self->m_54              = 0.0f;
	self->m_5C              = 0.0f;
	self->m_60              = 0.0f;
	self->m_64              = 0.0f;
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
// NONMATCHING: the staged scene check is retail's shape (two flag values, the second read off the first before the ScnRoot construct); the
// residue is register choice (flag regs swapped, two saved regs instead of three), the placement of the FALSE return and the assert's
// string addressing
BOOL XObjScnCam::LoadFromPlacement(const CoordInfo& info)
{
	if (!XObj::LoadFromPlacement(info)) {
		return FALSE;
	}
	BOOL ready = FALSE;
	BOOL scn   = mObject.m_pScnMdl != NULL && mObject.m_resFile.HasResAnmScn();
	if (scn && mObject.m_resFile.GetResAnmScnNumEntries() != 0) {
		ready = TRUE;
	}
	if (ready) {
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
		((nw4r::g3d::ScnRoot*)mScene)->CalcWorld();
		((nw4r::g3d::ScnRoot*)mScene)->CalcView();
		((nw4r::g3d::ScnRoot*)mScene)->UpdateFrame();
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
			BOOL has = mObject.m_resFile.HasResAnmScn();
			((nw4r::g3d::ScnRoot*)mScene)->SetAnmScn(has ? mObject.m_resources.scn : NULL);
			((nw4r::g3d::ScnRoot*)mScene)->CalcWorld();
			((nw4r::g3d::ScnRoot*)mScene)->CalcView();
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
	u32* fields = (u32*)_1378;
	for (int i = 0; i < 2; i++, fields += 16) {
		for (int k = 0; k < 8; k++) {
			if (fields[k * 2] != 0) {
				fields[k * 2] = 0;
			}
		}
	}
}
