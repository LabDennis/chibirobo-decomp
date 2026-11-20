// The XObj core methods, .text 0x800AC07C-0x800B1558 (wii_units.md Nr. 45): the object's
// init/reset pair, its placement load, release, update, the draw dispatch virtuals, the
// material helpers, the animation set/clear pair and the floor-collision test. The Wii
// source is the GameCube's obja.cpp carried over onto the nw4r model and queue types.
#include "Version.h"
#if defined(VERSION_R24J01)
#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h" // the Wii MSL's out-of-line __fpclassifyf
#include "game/wii/xobjflags.h"                                     // first: its AnimSfx needs the real nw4r::math, ahead of xobj.h's shim
#include "game/wii/xobj.h"
#include "game/wii/cgame.h"
#include "game/wii/xobj_ut2.h"
#include "game/evt.h"
#include "game/gmath.h"
#include "arithmetic.h"
#include "Dolphin/mtx.h"
#include <math.h>

using namespace nw4r::g3d;

extern "C" void UpdateObjectFloorHeights(XObj* object);
extern "C" void UpdateObjectFloorXZExtent(XObj* object);
extern s32 gScreenLeft, gScreenRight, gScreenTop, gScreenBottom;
extern CustomShadow gCustomShadow;

static inline u32 ShadowSize(XObj* object)
{
	return object->mAttr.mObjData != NULL ? object->mAttr.mObjData->mShadowSize : 0;
}

static inline void StepColor(f32& current, f32& target, f32& speed, u32 frames)
{
	if (current != target) {
		current += speed * frames;
		if (speed > 0.0f && current >= target) {
			current = target;
			speed   = 0.0f;
		}
		if (speed < 0.0f && current <= target) {
			current = target;
			speed   = 0.0f;
		}
	}
}

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

// 0x800ADB20
void XObj::Release()
{
	if (mArchive.mAnm.IsLoaded()) {
		if (GetAnimId() >= 0) {
			XObjSource* source = NULL;
			if (mAttr.mObjData != NULL) {
				source = mAttr.mObjData->mSource;
			}
			if (source != NULL && source->mAnimNames != NULL
			    && mObject.m_resFile.GetResAnmChr(source->mAnimNames[GetAnimId()]).mpData == NULL) {
				SetAnim(0, TRUE, 0.0f, 15.0f, 15.0f, 1.0f);
			}
			SetAnim(GetAnimId(), TRUE, GetAnimFrame(), 15.0f, 15.0f, 1.0f);
		}
		ClearAnmSource();
		if (mArchive.mAnm.mFile.IsLoaded()) {
			mArchive.mAnm.Release(TRUE);
			mArchive.mAnm.mFile.Close();
		}
	}
	if (mEvt.mData.mScript != NULL) {
		ResetEventScriptState(&mEvt, FALSE, FALSE, FALSE, FALSE);
	}
	if (mStatus.mFlags & 1) {
		CObj::Release();
	}
	mAttr.mObjData   = NULL;
	mAttr.mPlacement = NULL;
	mFloors          = NULL;
	mStatus.mFlags &= ~0x10001ULL;
}

// 0x800ADD28
void XObj::UpdateObject(u32 frames)
{
	if (mObject.m_pScnMdl != NULL) {
		if (mReleaseCountdown > 0 && !(mStatus.mFlags & 0x10000ULL)) {
			if (--mReleaseCountdown == 0) {
				Release();
			}
			return;
		}
		vf168(frames);
		if (mStatus.mFlags & 0x1000000000000000ULL) { // toppling: sink onto the floor hit
			if (mMoveTranslate.y + mVec714.y > mFloorHit.mHeight) {
				mMoveTarget0.y = mFloorHit.mHeight;
				f32 speed      = 0.15f + mMoveSpeed0.y; // fall faster, capped at 10
				if (speed > 10.0f) {
					speed = 10.0f;
				}
				mMoveSpeed0.y = speed;
			} else { // landed: take over the offset and stand straight
				mMoveSpeed0.y = 0.0f;
				mMoveTranslate.x += mVec714.x;
				mMoveTranslate.z += mVec714.z;
				mMoveTranslate.y = mFloorHit.mHeight;
				mVec714.x = mVec714.y = mVec714.z = 0.0f;
				SetRotationX(0.0f);
				SetRotationZ(0.0f);
				mStatus.mFlags &= ~0x1000000000000000ULL;
				if ((s16)GetPool() == 0x15) {
					mStatus.mFlags |= 0x2000000000000000ULL;
				} else if ((s16)GetPool() == 0x16 && (mStatus.mFlags & 0x04000000ULL)) {
					mStatus.mFlags |= 0x2000000000000000ULL;
				}
			}
		}
		// move, scale and turn toward their targets, then animate
		((CObjVec*)&mMoveRotate)->Step((f32)frames);
		((CObjVec*)&mMoveTranslate)->Step((f32)frames);
		if (((CObjVec*)&mMoveScale)->Step((f32)frames)) {
			UpdateObjectFloorHeights(this);
			UpdateObjectFloorXZExtent(this);
		}
		if (mStatus.mFlags & 0x80000000000000ULL) { // the archive anm source's read is still running
			UpdateArchiveAnmSource(this);
		}
		UpdateAnim((f32)frames);
	}
	if ((mStatus.mFlags & 0x0010000000000000ULL) || mMoveTranslate.y != mLastPos.y) { // floor heights track the height
		UpdateObjectFloorHeights(this);
	}
	if (mEvt.mData.mScript != NULL) {
		CEvt_Update(&mEvt, frames);
	}
	if (mAnimEventCallback != NULL && !(mStatus.mFlags & 0x800ULL)) {
		mAnimEventCallback(this, GetAnimId(), mLastAnimFrame, GetAnimFrame());
	}
	if (PSVECMag(&mMoveScale) > 0.0f) {
		WiiObjectRef* ref = &gGame.mDrawLists[1].mRefs[gGame.mDrawLists[1].mCount];
		ref->mObj         = this;
		ref->mIndex       = 0;
		gGame.mDrawLists[1].mCount++;
	}
}

// 0x800AE170, steps the two queued material colors to their targets and flags the change
void XObj::vf168(u32 frames)
{
	if (mObject.m_pScnMdl != NULL) {
		for (s32 i = 0; i < 2; i++) {
			XObjColorStep* color = &mAnmQueue.mColorStep[i];
			StepColor(color->mValue.x, color->mTarget.x, color->mSpeed.x, frames);
			StepColor(color->mValue.y, color->mTarget.y, color->mSpeed.y, frames);
			StepColor(color->mValue.z, color->mTarget.z, color->mSpeed.z, frames);
		}
		mColor1.Set((u8)mAnmQueue.mColorStep[0].mValue.x, (u8)mAnmQueue.mColorStep[0].mValue.y, (u8)mAnmQueue.mColorStep[0].mValue.z, 0);
		mColorsChanged = TRUE;
		mColor0.Set((u8)mAnmQueue.mColorStep[1].mValue.x, (u8)mAnmQueue.mColorStep[1].mValue.y, (u8)mAnmQueue.mColorStep[1].mValue.z, 0);
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

// NONMATCHING: the body matches except two referent/scheduling details: retail loads
// mObjData twice (r6, then r7) where our build keeps the first load live; and our
// literals land on different .sdata2 slots while the unit is incomplete.
// 0x800AE53C, the blob shadow at a given spot: the model's scale times the kind's shadow
// size and a fixed 0.6 sets the blob, 170*opacity its alpha; the draw call takes both
// vectors by value, so the caller keeps its own copies
void XObj::DrawGroundShadowAt(const Vec* origin, f32 opacity)
{
	if (!(mStatus.mFlags & 0x8)) {
		return;
	}
	if (ShadowSize(this) == 0) {
		return;
	}
	Vec pos   = *origin;
	Vec scale = mMoveScale * (f32)ShadowSize(this) * 0.6f;
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gCustomShadow.Draw(pos, scale, (u32)(170.0f * opacity));
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

// NONMATCHING: same instruction stream (188 each side); retail schedules the flag test's
// lwz/and/xor. between the callee saves and the zero-stores between the render-state setup,
// here both run after. Register assignment and the sdata2 constant labels differ with it.
// 0x800AE700, frames the object's on-screen bounds a margin wider while it is pointer-hovered
void XObj::vf198(f32 margin, f32 alpha, u32 color)
{
	if ((mStatus.mFlags & 0x10001) != 0x10001) {
		return;
	}
	if (mStatus.mFlags & 0x1000000000800) {
		return;
	}
	BOOL onScreen = mScreenBoundsValid && __fpclassifyf(mScreenLeft) != FP_NAN && __fpclassifyf(mScreenTop) != FP_NAN
	             && __fpclassifyf(mScreenRight) != FP_NAN && __fpclassifyf(mScreenBottom) != FP_NAN && mScreenLeft <= gScreenRight
	             && mScreenTop <= gScreenBottom && mScreenRight >= gScreenLeft && mScreenBottom >= gScreenTop;
	if (onScreen && _6D8 != 0) {
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
		f32 l      = mScreenLeft - margin;
		f32 r      = mScreenRight + margin;
		f32 t      = mScreenTop - margin;
		f32 b      = mScreenBottom + margin;
		Vec pos[5] = { { l, t, 0.0f },   // top left
			           { r, t, 0.0f },   // top right
			           { r, b, 0.0f },   // bottom right
			           { l, b, 0.0f },   // bottom left
			           { l, t, 0.0f } }; // back to top left
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetLineWidth(0xC, GX_TO_ONE);
		DrawGsLineStrip(&gGame.mGs, pos, (color & 0xFFFFFF00) | (u8)((color & 0xFF) * alpha), 5);
	}
}

// 0x800AF2C8, the kind's packed anim id: the low halfword names the character animation, the high
// one a material animation; both index the source's own name table, each wrapping at 1000
void XObj::SetAnim(s32 id, BOOL force, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	if (id != GetAnimId()) {
		mLastAnimFrame = -1.0f;
	} else if (force && GetAnimFrame() != frame) {
		mLastAnimFrame = -1.0f;
	}
	if (mStatus.mFlags & 0x20000000000000ULL) {
		blend = 15.0f;
	}
	s32 animIndex      = (id & 0xFFFF) % 1000;
	XObjSource* source = NULL;
	if (mAttr.mObjData != NULL) {
		source = mAttr.mObjData->mSource;
	}
	if (source != NULL && source->mAnimNames != NULL) {
		if (animIndex >= source->mAnimCount) {
			animIndex = 0;
		}
		CObjAnim anim;
		anim.mId   = id;
		anim.mName = source->mAnimNames[animIndex];
		CObj::SetAnim(&anim, force, frame, rate, blend, blendRate);
		if ((id >> 16) != 0) {
			s32 matIndex = (id >> 16) % 1000;
			if (matIndex < source->mAnimCount) {
				CObjAnim matAnim;
				matAnim.mId   = id;
				matAnim.mName = source->mAnimNames[matIndex];
				SetMatAnim(&matAnim, frame);
			}
		}
	} else {
		CObj::SetAnim(id, force, frame, rate, blend, blendRate);
	}
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
