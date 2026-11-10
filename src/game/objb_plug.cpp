#include "game/xchr.h"
#include "game/cgame.h"
#include "game/gmath.h"
#include "game/csound.h"
#include "game/cgs.h"
#include "game/texturecopy.h"
#include "game/ccamera.h"
#include "game/cparticle.h"
#include "game/stagedata.h"
#include "game/cjobjinlineattach.h"
extern "C" {
#include <sysdolphin/mtx.h>
#include <sysdolphin/random.h>
}

extern "C" {

static inline void AttachPlugCord(XObjPlug* plug)
{
	if (plug->_D38 == 0) {
		SetPlugCordEndpointSource(&plug->mCord, 1, &plug->mCordJointPos);
		SetPlugCordEndpointSource(&plug->mCord, 2, &plug->mCordJointTip);
		plug->_D38 = 1;
	}
}

static inline void UpdateCordJoint(XObjPlug* plug, CJObj& joint)
{
	Mtx matrix;
	PSMTXCopy(joint.GetMtx(), matrix);
	plug->mCordJointPos = CVec(matrix[0][3], matrix[1][3], matrix[2][3]);
	CVec tip(0, 0, 5);
	PSMTXMultVec(matrix, &tip, &tip);
	plug->mCordJointTip = tip;
}

#pragma push
#pragma pool_data off
void UpdateObject__8XObjPlugFUl(XObjPlug* plug, u32 frames)
{
	UpdateObject__4XObjFUl(plug, frames);
	if (plug->mStatus.mFlags & 1) {
		GetPlayerPlugCordAnchorTransform(plug, &plug->mCordAnchor, NULL, NULL);
		if (plug->mStatus.mFlags & 0x0400000000000000ULL) {
			AttachPlugCord(plug);
			s32 path[] = { 0, 0 };
			CJObj joint(FindJointByPath(plug->mJObj, path, 2));
			UpdateCordJoint(plug, joint);
		} else if (plug->_D3C != 0) {
			AttachPlugCord(plug);
			s32 path[] = { 0, 0 };
			CJObj joint(FindJointByPath(plug->mJObj, path, 2));
			UpdateCordJoint(plug, joint);
		} else if (plug->_D38 == 1) {
			SetPlugCordEndpointSource(&plug->mCord, 1, NULL);
			SetPlugCordEndpointSource(&plug->mCord, 2, NULL);
			plug->_D38 = 0;
		}
		const Vec& tint    = plug->mColorStep[1].mValue;
		plug->mCord.mColor = plug->mCord._60 = ((u32)tint.x << 24) | ((u32)tint.y << 16) | ((u32)tint.z << 8) | 0xff;
		UpdatePlugCordSimulation(&plug->mCord, frames);
	} else {
		plug->ResetObjectState();
	}
	plug->mWire.UpdateObject(frames);
	if ((plug->mStatus.mFlags & 1) && !(plug->mStatus.mFlags & 0x0400000000000000ULL) && plug->_D3C == 0) {
		Vec pos = GetPlugCordPointPosition(&plug->mCord, 18);
		plug->SetPos(pos, 0);
		CVec tip, base, rot;
		tip    = GetPlugCordPointPosition(&plug->mCord, 19);
		base   = GetPlugCordPointPosition(&plug->mCord, 18);
		rot.y  = atan2(tip.x - base.x, tip.z - base.z);
		f32 dx = tip.x - base.x, dy = tip.y - base.y, dz = tip.z - base.z;
		f32 pitch = atan2(sqrtf(dx * dx + dz * dz), sqrtf(dy * dy));
		rot.x     = dy < 0 ? HALF_PI - pitch : pitch - HALF_PI;
		rot.z     = 0;
		plug->SetRot(rot, 0);
		Mtx matrix;
		PSMTXCopy(HSD_JObjGetMtxPtr(plug->mJObj), matrix);
		Vec offset = { 0, -0.2f, 0 };
		PSMTXMultVec(matrix, &offset, &offset);
		plug->SetPos(offset, 0);
		if (plug->mWire.mSparkMode == 0) {
			if (plug->mWire.mSparkTimer < 0) {
				CVec pos(plug->mMoveTranslate);
				CVec scale(0.5f, 0.5f, 0.5f);
				Vec rotation = { 0, 0, 0 };
				gGame.mParticle.CreateSRT(0x1001a, 1, pos, scale, rotation, FALSE);
				plug->mWire.mSparkTimer += HSD_Randi(10) + 16;
			}
			XSound* sound = &gGame.mSound;
			if (IsGameSoundPlaying(sound, 0xfb) != 1 && IsGameSoundPlaying(sound, 0xfc) != 1) {
				if (plug->mWire.mSparkTimer % 2)
					PlayGameSound(sound, 0xfb, 255);
				else
					PlayGameSound(sound, 0xfc, 255);
			}
			plug->mWire.mSparkTimer -= frames;
		}
	}
}
#pragma pop

/**
 * @note Address: 0x800D9068
 * @note Size: 0x34
 */
CVec GetPlugCordPointPosition(const XObjPlugCord* cord, s32 index)
{
	return cord->mPoints[index].mPos;
}

/**
 * @note Address: 0x800D909C
 * @note Size: 0x32C
 */
void GetPlayerPlugCordAnchorTransform(void*, Vec* position, Vec* rotation, Vec* scale)
{
	Mtx matrix;
	s32 path[] = { 0, 1, 0, 1, 1 };
	CJObj joint(FindJointByPath(gGame.GetPlayer()->mJObj, path, 5));
	if (joint.mJObj != NULL) {
		PSMTXCopy(joint.GetMtx(), matrix);
	}

	CVec unused; // never read, the dol still constructs it
	static CVec offset(0.0f, 0.0f, -0.4f);
	if (position != NULL) {
		PSMTXMultVec(matrix, &offset, position);
	}
	if (rotation != NULL) {
		HSD_MtxGetRotation(matrix, rotation);
	}
	if (scale != NULL) {
		HSD_MtxGetScale(matrix, scale);
	}
}

} // extern "C"
