#include "game/xobjattachment.h"
#include "game/wii/cgame.h"
#include "game/wii/ceffect.h"
#include "game/wii/xobj.h"
#include "game/stagedata.h"
#include "game/ut.h"
#include "game/gmath.h"
#include "arithmetic.h"
#include <math.h>
#include <stdlib.h>

// the plug object and its cord effects, the GameCube's xobjplugload.cpp and
// objb_plug.cpp in one unit. The Wii build keeps a joint matrix and a roll for
// the carried plug where the GameCube re-parented a HSD_JObj.

extern "C" {
// the Wii build's base SetDestinationAlpha, the _1A4 slot
void fn_800AD9D0(XObj* object, BOOL enabled, u8 value);
// the player's world matrix, xobj_chr.cpp
MtxP fn_800E59DC(WiiPlayerState* chr);
// the cord's spark pass, the GameCube ran it inside UpdateObject
void fn_801105D4(XObjPlug* plug, u32 frames);
// the event emitter pool's free-slot allocation, xactradar.cpp uses the same
EffectInstance* fn_800A11A4(EffectInstance* effects, u32 id, s32 group);
// the Wii variant of HSD_MtxSRT, ut.cpp: no parent_scale argument
void HSD_MtxSRT(Mtx m, Vec* scale, Vec* rotation, Vec* translation);
void HSD_MtxGetScale(Mtx m, Vec* dst);
}

/**
 * @note Address: 0x8010F944
 * @note Size: 0x7C
 */
void XObjPlug::InitializeObject(u32 id)
{
	XObj::InitializeObject();
	InitPlugCord(&mCord, 20, 0.0f, 0.0f, 0.0f, 0.5f);
	mCord.mColor = 0x141414FF;
	mCord._60    = 0x141414FF;
	mCord.mWidth = 0.2f;
	mLoadState   = 0;
	mWire.InitializeObject();
}

/**
 * @note Address: 0x8010F9C0
 * @note Size: 0x8
 */
extern "C" void SetPlugCordSpeed(XObjPlug* plug, f32 speed)
{
	plug->mCord.mSpeed = speed;
}

/**
 * @note Address: 0x8010F9C8
 * @note Size: 0x28
 */
void XObjPlug::ResetForNewGame()
{
	mStatus.mFlags &= ~0x0400000000000000ULL;
}

/**
 * @note Address: 0x8010F9F0
 * @note Size: 0xE8
 */
void XObjPlug::ResetObjectState()
{
	XObj::ResetObjectState();
	mConnection = 0;
	if (mStatus.mFlags & 1) {
		Vec pos = *gGame.GetPlayer()->GetPos();
		ResetPlugCordPoints(&mCord, pos.x, pos.y, pos.z);
		mPlugRot = 0.0f;
	}
	SetPlugCordEndpointSource(&mCord, 0, &mCordAnchor);
	SetPlugCordEndpointSource(&mCord, 1, NULL);
	SetPlugCordEndpointSource(&mCord, 2, NULL);
	mCordEndsPinned = 0;
	mWire.ResetObjectState();
	mWire.mSparkMode = -1;
}

/**
 * @note Address: 0x8010FAD8
 * @note Size: 0x104
 */
BOOL XObjPlug::LoadFromPlacement(const CoordInfo& info)
{
	switch (mLoadState) {
	case 0:
		if (!XObj::LoadFromPlacement(info)) {
			break;
		}
		mLoadState++;
		// fall through
	case 1: {
		CoordInfo info2(GetSpawnData(16));
		if (mWire.LoadFromPlacement(info2)) {
			Vec pos = *gGame.GetPlayer()->GetPos();
			ResetPlugCordPoints(&mCord, pos.x, pos.y, pos.z);
			mLoadState++;
		}
		break;
	}
	}
	return mLoadState > 1;
}

/**
 * @note Address: 0x8010FBDC
 * @note Size: 0x4C
 */
void XObjPlug::Release()
{
	mWire.Release();
	XObj::Release();
	mLoadState = 0;
}

/**
 * @note Address: 0x8010FC28
 * @note Size: 0x154
 */
extern "C" void GetPlayerPlugCordAnchorTransform(void* object, Vec* position, Vec* rotation, Vec* scale)
{
	Mtx matrix;
	nw4r::g3d::ResNode node = gGame.GetPlayer()->FindResNode("code_cause_model");
	if (node.IsValid()) {
		PSMTXCopy(gGame.GetPlayer()->GetWorldMtx(node), matrix);
	}
	CVec unused; // never read, the dol still constructs it
	static CVec offset(0.0f, 0.0f, -0.4f);
	if (position != NULL) {
		PSMTXMultVec(matrix, &offset, position);
	}
	if (rotation != NULL) {
		Vec zero  = { 0.0f, 0.0f, 0.0f };
		*rotation = zero;
	}
	if (scale != NULL) {
		HSD_MtxGetScale(matrix, scale);
	}
}

// the GameCube's UpdateObject: the carried and the connected plug pin both cord ends to
// the "cord_connect" joint, the free plug hangs from the cord's tip
/**
 * @note Address: 0x8010FD7C
 * @note Size: 0x5EC
 */
void XObjPlug::UpdateObject(u32 frames)
{
	if (mStatus.mFlags & 1) {
		GetPlayerPlugCordAnchorTransform(this, &mCordAnchor, NULL, NULL);
		if (mStatus.mFlags & 0x0400000000000000ULL) {
			if (mCordEndsPinned == 0) {
				SetPlugCordEndpointSource(&mCord, 1, &mCordJointPos);
				SetPlugCordEndpointSource(&mCord, 2, &mCordJointTip);
				mCordEndsPinned = 1;
			}
			Mtx matrix;
			nw4r::g3d::ResNode node = FindResNode("cord_connect");
			PSMTXCopy(GetWorldMtx(node), matrix);
			mCordJointPos = CVec(matrix[0][3], matrix[1][3], matrix[2][3]);
			CVec tip(0.0f, 0.0f, 5.0f);
			PSMTXMultVec(matrix, &tip, &tip);
			mCordJointTip = tip;
		} else if (mConnection != 0) {
			if (mCordEndsPinned == 0) {
				SetPlugCordEndpointSource(&mCord, 1, &mCordJointPos);
				SetPlugCordEndpointSource(&mCord, 2, &mCordJointTip);
				mCordEndsPinned = 1;
			}
			Mtx matrix;
			nw4r::g3d::ResNode node = FindResNode("cord_connect");
			PSMTXCopy(GetWorldMtx(node), matrix);
			mCordJointPos = CVec(matrix[0][3], matrix[1][3], matrix[2][3]);
			CVec tip(0.0f, 0.0f, 5.0f);
			PSMTXMultVec(matrix, &tip, &tip);
			mCordJointTip = tip;
		} else {
			if (mCordEndsPinned == 1) {
				SetPlugCordEndpointSource(&mCord, 1, NULL);
				SetPlugCordEndpointSource(&mCord, 2, NULL);
				mCordEndsPinned = 0;
			}
		}
		const Vec& tint = mAnmQueue.mColorStep[1].mValue;
		u32 color       = ((u32)tint.x << 24) | ((u32)tint.y << 16) | ((u32)tint.z << 8) | 0xff;
		mCord.mColor    = color;
		mCord._60       = color;
		UpdatePlugCordSimulation(&mCord, frames);
	} else {
		ResetObjectState();
	}
	mWire.UpdateObject(frames);
	if (mStatus.mFlags & 1) {
		if (mStatus.mFlags & 0x0400000000000000ULL) {
			XObj::UpdateObject(frames);
			fn_801105D4(this, frames);
		} else if (mConnection != 0) {
			XObj::UpdateObject(frames);
			fn_801105D4(this, frames);
		} else {
			CVec pos = GetPlugCordPointPosition(&mCord, 18);
			SetPos(pos);
			CVec tip = GetPlugCordPointPosition(&mCord, 19);
			CVec base = GetPlugCordPointPosition(&mCord, 18);
			CVec dir, rot;
			rot.y = atan2(tip.x - base.x, tip.z - base.z);
			f32 pitch;
			f32 dy = tip.y - base.y;
			pitch  = atan2(FSqrt((tip.x - base.x) * (tip.x - base.x) + (tip.z - base.z) * (tip.z - base.z)), FSqrt(dy * dy));
			rot.x  = dy < 0.0f ? HALF_PI - pitch : pitch - HALF_PI;
			rot.z = 0.0f;
			SetRot(rot);
			static Vec hang = { 0, -0.2f, 0 };
			Mtx m;
			HSD_MtxSRT(m, (Vec*)&mMoveScale, &rot, &pos);
			Vec offset = hang;
			PSMTXMultVec(m, &offset, &offset);
			SetPos(offset);
			XObj::UpdateObject(frames);
			fn_801105D4(this, frames);
		}
	}
}

/**
 * @note Address: 0x80110368
 * @note Size: 0x34
 */
extern "C" CVec GetPlugCordPointPosition(const XObjPlugCord* cord, s32 index)
{
	return cord->mPoints[index].mPos;
}

/**
 * @note Address: 0x8011039C
 * @note Size: 0x74
 */
void XObjPlug::DrawWithView(Mtx view)
{
	XObj::DrawWithView(view);
	mWire.DrawWithView(view);
	if (mObject.m_pScnMdl != NULL) {
		DrawPlugCordSegments(&mCord, view, 1.0f, 1.0f, 1.0f);
	}
}

/**
 * @note Address: 0x80110410
 * @note Size: 0x68
 */
void XObjPlug::DrawReflection(Mtx view)
{
	DrawWithView(view);
	if (mObject.m_pScnMdl != NULL) {
		DrawPlugCordSegments(&mCord, view, 1.0f, 1.0f, 1.0f);
	}
}

/**
 * @note Address: 0x80110478
 * @note Size: 0xC4
 */
void XObjPlug::DrawGroundShadow(f32 opacity)
{
	mCord.mColor = 0x14141440;
	mCord._60    = 0x14141440;
	mCord.mWidth = 0.15f;
	if (mObject.m_pScnMdl != NULL) {
		if (gGame.mGs.mCurRenderState != NULL) {
			gGame.mGs.mCurRenderState->Apply();
		}
		if (gGame.mGs.mCurTevStage != NULL) {
			gGame.mGs.mCurTevStage->Apply();
		}
		if (gGame.mGs.mCurChannel != NULL) {
			gGame.mGs.mCurChannel->Apply();
		}
		DrawPlugCordSegments(&mCord, NULL, 1.0f, 1.0f, 1.0f);
	}
	mCord.mColor = 0x141414FF;
	mCord._60    = 0x141414FF;
	mCord.mWidth = 0.2f;
}

/**
 * @note Address: 0x8011053C
 * @note Size: 0x34
 */
void XObjPlug::DrawObjectEffects()
{
	if (mObject.m_pScnMdl != NULL) {
		DrawPlugCordNodes(&mCord, gGame.mGs.mCamera->mViewMtx, 1.0f, 1.0f, 1.0f);
	}
}

/**
 * @note Address: 0x80110570
 * @note Size: 0x54
 */
void XObjPlug::SetDestinationAlpha(BOOL enabled, u8 value)
{
	fn_800AD9D0(this, enabled, value);
	SetPlugCordAlphaMode(&mCord, enabled, value);
}

/**
 * @note Address: 0x801105C4
 * @note Size: 0x10
 */
extern "C" void fn_801105C4(XObjPlug* plug, s32 mode)
{
	plug->mWire.mSparkMode  = mode;
	plug->mWire.mSparkTimer = -1;
}

// the GameCube's spark block inside UpdateObject: when the wire sparks it fires a
// one-shot sizzle effect and plays the two crackle sounds alternately
/**
 * @note Address: 0x801105D4
 * @note Size: 0x1D8
 */
extern "C" void fn_801105D4(XObjPlug* plug, u32 frames)
{
	if (plug->mWire.mSparkMode == 0) {
		if (plug->mWire.mSparkTimer < 0) {
			Vec pos, scale;
			pos                   = *plug->GetPos();
			CVec cvec(0.5f, 0.5f, 0.5f);
			scale                 = cvec;
			Vec rotation          = { 0, 0, 0 };
			EffectInstance* inst = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x1a, 1);
			if (inst != NULL) {
				inst->pos   = pos;
				inst->rot   = rotation;
				inst->scale = scale;
				UpdateEffectMtx(inst);
			}
			plug->mWire.mSparkTimer += rand() % 10 + 16;
		}
		if (!IsWiiSfxPlaying(&gGame.mSound, 0x168) && !IsWiiSfxPlaying(&gGame.mSound, 0x169)) {
			if (plug->mWire.mSparkTimer % 2 != 0) {
				PlayWiiSoundEffect(&gGame.mSound, 0x168, 1.0f, 0.0f);
			} else {
				PlayWiiSoundEffect(&gGame.mSound, 0x169, 1.0f, 0.0f);
			}
		}
		plug->mWire.mSparkTimer -= frames;
	}
}

/**
 * @note Address: 0x801107AC
 * @note Size: 0xC
 */
extern "C" void fn_801107AC(XObjPlug* plug)
{
	plug->mWire.mSparkMode = -1;
}

// the GameCube's ExecCallbackB: while carried the plug joint follows the player's
// world matrix rolled by mPlugRot, while connected it reuses the stored matrix
/**
 * @note Address: 0x801107B8
 * @note Size: 0x150
 */
void XObjPlug::ExecCallbackB(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
	XObj::ExecCallbackB(pResult, mdl, pFuncObj);
	if (pFuncObj->GetCallbackNodeID() == 0) {
		nw4r::g3d::ResNode node = GetResNode("plug_model");
		if (node.IsValid()) {
			pFuncObj->mNodeID = node.GetID();
		}
	} else {
		if (mStatus.mFlags & 0x0400000000000000ULL) {
			Mtx playerMtx, rot;
			PSMTXCopy(fn_800E59DC(gGame.GetPlayer()), playerMtx);
			PSMTXRotRad(rot, 'z', mPlugRot);
			PSMTXConcat(playerMtx, rot, playerMtx);
			PSMTXCopy(playerMtx, pResult->mpMtx);
			PSMTXCopy(playerMtx, mPlugMtx);
		} else if (mConnection != 0) {
			if (mPlugMtx != NULL) {
				PSMTXCopy(mPlugMtx, pResult->mpMtx);
			} else {
				PSMTXIdentity(pResult->mpMtx);
			}
		}
	}
}

namespace {

// retires the instance's effect when there is one
static inline void KillInstance(EffectInstance* inst)
{
	if (inst != NULL) {
		RetireEffect(&inst->handle, FALSE);
	}
}

// the GameCube's KillGen: on the Wii a generator is an event-effect instance, so the
// retail code shows the caller's own guard in front of the guarded call
static inline void KillGen(EffectInstance*& gen)
{
	if (gen != NULL) {
		KillInstance(gen);
		gen = NULL;
	}
}

}

/**
 * @note Address: 0x80110908
 * @note Size: 0x60
 */
void XObjWireEff::ResetObjectState()
{
	XObj::ResetObjectState();
	mStatus.mFlags &= ~0x10000ULL;
	mAnim      = -1;
	mEffect    = -1;
	mGenerator = NULL;
}

/**
 * @note Address: 0x80110968
 * @note Size: 0x80
 */
void XObjWireEff::Release()
{
	mStatus.mFlags &= ~0x10000ULL;
	mAnim   = -1;
	mEffect = -1;
	KillGen(mGenerator);
	XObj::Release();
}

/**
 * @note Address: 0x801109E8
 * @note Size: 0x2C
 */
void XObjWireEff::UpdateObject(u32 frames)
{
	if ((mStatus.mFlags & 1) && (mAnim >= 0 || mEffect >= 0)) {
		XObj::UpdateObject(frames);
	}
}

/**
 * @note Address: 0x80110A14
 * @note Size: 0x1C
 */
void XObjWireEff::DrawWithView(Mtx view)
{
	if ((mStatus.mFlags & 1) && (mStatus.mFlags & 0x10000)) {
		XObj::DrawWithView(view);
	}
}

// the GameCube's ConfigureWireEffect: an anim index, an event effect index, a
// rate and a packed color; either side may stay off with -1
/**
 * @note Address: 0x80110A30
 * @note Size: 0x174
 */
extern "C" void ConfigurePlugWireEffect(XObjWireEff* object, s32 anim, s32 effect, f32 rate, u32 color)
{
	f64 r = rate;
	object->mAnim   = anim;
	object->mEffect = effect;
	object->mRate   = rate;
	if (anim >= 0) {
		object->mStatus.mFlags |= 0x10000ULL;
		object->SetAnim(anim, FALSE, 0.0f, r, 0.0f, 15.0f);
	} else {
		object->mStatus.mFlags &= ~0x10000ULL;
	}
	if (object->mEffect >= 0) {
		KillGen(object->mGenerator);
		MtxP worldMtx        = object->GetWorldMtx();
		EffectInstance* inst = fn_800A11A4((EffectInstance*)gGame.mEvtPtcl.mEmitters, 0x52, 1);
		if (inst != NULL) {
			inst->mtx = worldMtx;
			UpdateEffectMtx(inst);
		}
		object->mGenerator = inst;
	} else {
		KillGen(object->mGenerator);
	}
	object->SetColor(0, color >> 24, color >> 16, color >> 8, 0.0f);
}

/**
 * @note Address: 0x80110BA4
 * @note Size: 0x78
 */
extern "C" void ClearWireEffect(XObjWireEff* object)
{
	object->mStatus.mFlags &= ~0x10000ULL;
	object->mAnim   = -1;
	object->mEffect = -1;
	KillGen(object->mGenerator);
}
