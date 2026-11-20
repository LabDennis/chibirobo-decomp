// The XObj core methods, .text 0x800AC07C-0x800B1558 (wii_units.md Nr. 45): the object's
// init/reset pair, its placement load, release, update, the draw dispatch virtuals, the
// material helpers, the animation set/clear pair and the floor-collision test. The Wii
// source is the GameCube's obja.cpp carried over onto the nw4r model and queue types.
#include "Version.h"
#if defined(VERSION_R24J01)
#include "game/wii/xobjflags.h" // first: its AnimSfx needs the real nw4r::math, ahead of xobj.h's shim
#include "game/wii/xobj.h"
#include "game/wii/cgame.h"
#include "game/evt.h"
#include "game/gmath.h"
#include "arithmetic.h"
#include "Dolphin/mtx.h"
#include <math.h>

using namespace nw4r::g3d;

// 0x800AC07C
void XObj::InitializeObject()
{
	Init();
	mStatus.mFlags    = 0;
	mCoord.mPlacement = NULL;
	mAttr.mObjData    = NULL;
	mAttr.mPlacement  = NULL;
	mFloors           = NULL;
	mPoolIndex        = 0;
	ResetObjectState();
	InitializeEventScriptContext(&mEvt, gGame.GetMsg(1), this);
	ClearAnimQueue();
}

// 0x800AC11C
void XObj::ResetObjectState()
{
	mEventScript        = NULL;
	mFloorHit.mTriangle = -1;
	mFloorHit.mTrigon   = 0;
	mFloorHit.mHeight   = 0.0f;
	mVec714.x = mVec714.y = mVec714.z = 0.0f;
	ClearAnimQueue();
	mLastPos           = mMoveTranslate;
	mLastRot           = mMoveRotate;
	mLastAnimFrame     = -1.0f;
	mAnmQueue.mAnchorX = mAnmQueue.mAnchorZ = NAN;
	mAnmQueue.mMarkerOverride               = 0x80000000;
	mAnmQueue.mColorStep[0].mValue.x = mAnmQueue.mColorStep[0].mValue.y = mAnmQueue.mColorStep[0].mValue.z = 255.0f;
	mAnmQueue.mColorStep[0].mTarget.x = mAnmQueue.mColorStep[0].mTarget.y = mAnmQueue.mColorStep[0].mTarget.z = 255.0f;
	mAnmQueue.mColorStep[0].mSpeed.x = mAnmQueue.mColorStep[0].mSpeed.y = mAnmQueue.mColorStep[0].mSpeed.z = 0.0f;
	mAnmQueue.mColorStep[1].mValue.x = mAnmQueue.mColorStep[1].mValue.y = mAnmQueue.mColorStep[1].mValue.z = 0.0f;
	mAnmQueue.mColorStep[1].mTarget.x = mAnmQueue.mColorStep[1].mTarget.y = mAnmQueue.mColorStep[1].mTarget.z = 0.0f;
	mAnmQueue.mColorStep[1].mSpeed.x = mAnmQueue.mColorStep[1].mSpeed.y = mAnmQueue.mColorStep[1].mSpeed.z = 0.0f;
	ClearBaseMtx();
	_99C = 0;
}

// 0x800AD870, flags[0] asks for dst alpha 8, flags[1] or nothing for 4, on every material's pix copy
void XObj::vf1A0(const u32* flags)
{
	u32 dstAlpha8 = flags[0];
	u32 dstAlpha4 = flags[1];
	for (u32 i = 0; i < mObject.m_resMdl.GetResMatNumEntries(); i++) {
		ResMat mat = mObject.m_resMdl.GetResMat(i);
		ScnMdl::CopiedMatAccess access(mObject.m_pScnMdl, i);
		ResMatPix pix;
		pix = access.GetResMatPix(false);
		if (!pix.IsValid()) {
			pix = mat.GetResMatPix();
		}
		if (dstAlpha8 != 0) {
			pix.GXSetDstAlpha(GX_TRUE, 8);
		} else if (dstAlpha4 != 0) {
			pix.GXSetDstAlpha(GX_TRUE, 4);
		} else {
			pix.GXSetDstAlpha(GX_TRUE, 4);
		}
		pix.DCStore(false);
		mat.DCStore(false);
	}
}

// 0x800AD9D0
void XObj::vf1A4()
{
}

// 0x800AD9D4
void XObj::vf1A8()
{
}

// 0x800AD9D8
void XObj::SetDiffuseColor(const GXColor& color)
{
	for (u32 i = 0; i < mObject.m_resMdl.GetResMatNumEntries(); i++) {
		ResMat mat = mObject.m_resMdl.GetResMat(i);
		ScnMdl::CopiedMatAccess access(mObject.m_pScnMdl, i);
		ResMatTevColor tevColor;
		tevColor = access.GetResMatTevColor(false);
		if (!tevColor.IsValid()) {
			tevColor = mat.GetResMatTevColor();
		}
		tevColor.GXSetTevKColor(GX_KCOLOR0, color);
		tevColor.DCStore(false);
		mat.DCStore(false);
	}
}

// 0x800AE448, queues the model on the scene's draw list when it has one and a nonzero scale
void XObj::vf170()
{
	if (mObject.m_pScnMdl == NULL) {
		return;
	}
	if (PSVECMag(&mMoveScale) <= 0.0f) {
		return;
	}
	WiiGsSceneObj* scene = gGame.mGs.mScene.mHead;
	scene->vf034(scene->mPending, mObject.m_pScnMdl);
}

// 0x800AE4B4
void XObj::DrawWithView(Mtx view)
{
	if (mObject.m_pScnMdl == NULL) {
		return;
	}
	if (mStatus.mFlags & 0x800) {
		return;
	}
	vf170();
}

// 0x800AE4E0
void XObj::DrawGroundShadow(f32 opacity)
{
	if (opacity == 0.0f) {
		return;
	}
	if (mObject.m_pScnMdl == NULL) {
		return;
	}
	if (mStatus.mFlags & 0x800) {
		return;
	}
	if (mStatus.mFlags & 0x4) {
		vf170();
	} else {
		DrawGroundShadowAt(&mMoveTranslate, 1.0f);
	}
}

// 0x800AE6A8
void XObj::DrawCollisionShapes()
{
}

// 0x800AE6AC, the screen-list twin of vf170, gated on both flag bits being set
void XObj::DrawScreenOverlay()
{
	if (mObject.m_pScnMdl == NULL) {
		return;
	}
	if ((mStatus.mFlags & 0x10001) != 0x10001) {
		return;
	}
	if (!(mStatus.mFlags & 0x800)) {
		return;
	}
	WiiGsSceneObj* scene = gGame.mGs.mScene.mHead;
	scene->vf034(scene->mPending, mObject.m_pScnMdl);
}

// 0x800AF4CC
void XObj::ClearAnim()
{
	CObj::ClearAnim();
	mStatus.mFlags &= ~0x0008000000000000ULL;
}

// 0x800AFF80
void XObj::vf1BC(s32 mode)
{
	if (mObject.m_pScnMdl == NULL) {
		return;
	}
	for (u32 i = 0; i < mObject.m_resMdl.GetResMatNumEntries(); i++) {
		ResMat mat = mObject.m_resMdl.GetResMat(i);
		ScnMdl::CopiedMatAccess access(mObject.m_pScnMdl, i);
		ResGenMode genMode;
		genMode = access.GetResGenMode(false);
		if (!genMode.IsValid()) {
			genMode = mat.GetResGenMode();
		}
		genMode.GXSetCullMode((GXCullMode)mode);
		mat.DCStore(false);
	}
}

// 0x800B008C, keeps each tev's old cull mode in the spare tag byte before forcing mode
void XObj::SetCullMode(s32 mode)
{
	if (mObject.m_pScnMdl == NULL) {
		return;
	}
	for (u32 i = 0; i < mObject.m_resMdl.GetResMatNumEntries(); i++) {
		ResMat mat = mObject.m_resMdl.GetResMat(i);
		ScnMdl::CopiedMatAccess access(mObject.m_pScnMdl, i);
		ResGenMode genMode;
		genMode = access.GetResGenMode(false);
		if (!genMode.IsValid()) {
			genMode = mat.GetResGenMode();
		}
		ResTev tev;
		tev = access.GetResTev(false);
		if (!tev.IsValid()) {
			tev = mat.GetResTev();
		}
		if (tev.ptr()->PADDING_0x18[5] == 0) {
			tev.ptr()->PADDING_0x18[5] = genMode.GXGetCullMode() + 1;
		}
		genMode.GXSetCullMode((GXCullMode)mode);
		mat.DCStore(false);
	}
}

// 0x800B0210
void XObj::ResetCullMode()
{
	if (mObject.m_pScnMdl == NULL) {
		return;
	}
	for (u32 i = 0; i < mObject.m_resMdl.GetResMatNumEntries(); i++) {
		ResMat mat = mObject.m_resMdl.GetResMat(i);
		ScnMdl::CopiedMatAccess access(mObject.m_pScnMdl, i);
		ResTev tev;
		tev = access.GetResTev(false);
		if (!tev.IsValid()) {
			tev = mat.GetResTev();
		}
		if (tev.ptr()->PADDING_0x18[5] == 0) {
			continue;
		}
		ResGenMode genMode;
		genMode = access.GetResGenMode(false);
		if (!genMode.IsValid()) {
			genMode = mat.GetResGenMode();
		}
		genMode.GXSetCullMode((GXCullMode)(tev.ptr()->PADDING_0x18[5] - 1));
		mat.DCStore(false);
	}
}

#endif
