#include "game/ccameraaction.h"
extern "C" {
#include <dolphin/mtx/vec.h>
}
#include "game/caminfo.h"
#include "game/scrollcameravisibility.h"
#include "game/ctpl.h"
#include "game/gmath.h"
#include "Version.h"
#include <math.h>
#include <string.h>
#include "game/globals.h"
#include "game/stagedata.h"
#include "game/evt.h"
#include "game/cgame.h"
#include "game/xchr.h"
#include "game/xobjleticker.h"
#include "game/cgs.h"
#include "game/stg.h"

#if defined(VERSION_PAL)
#define ROOM_LIMITS       gScrollRoomData.limits
#define CAPTION_OUTLINE_X gScrollRoomData.captionX
#define CAPTION_OUTLINE_Y gScrollRoomData.captionY
#else
#define ROOM_LIMITS       gScrollCameraLimitTables
#define CAPTION_OUTLINE_X gScrollCaptionOutlineX
#define CAPTION_OUTLINE_Y gScrollCaptionOutlineY
#endif

#define PLAYER     gGame.GetPlayer()
#define MARK_SCALE 0.65f

// a message id with its text, the caption over the room camera
struct CaptionMsg {
	s32 mId;           // _00
	const char* mText; // _04
};

extern "C" {
const char* GetStageName(s32 stage);
u16 GXGetTexObjWidth(const GXTexObj* obj);
void PSMTXTrans(Mtx m, f32 x, f32 y, f32 z);
u16 GXGetTexObjHeight(const GXTexObj* obj);
void DrawTexturedWindowFrame(u32 color, s32 a, f32 x, f32 y, f32 w, f32 h, f32 sx, f32 sy, f32 sz);
void PSMTXRotRad(Mtx m, char axis, f32 rad);
}

void* gSavedScrollCameraInfo;
// draws one blinking marker centred on a screen point
// todo: cleanup
/**
 * @note Address: N/A
 * @note Size: ?
 */
inline void DrawScrollMapMarker(s32 index, f32 x, f32 y, u32 color)
{
	CTexObj tex;
	f32 half = 0.5f;
	tex      = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[index - StatusTextureOffset], 0);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	f32 zoom = 1.0f; // never anything else, but it stops the divide being folded away
	f32 w    = MARK_SCALE * (1.1f * GXGetTexObjWidth(&tex)) / zoom;
	f32 h    = MARK_SCALE * GXGetTexObjHeight(&tex) / zoom;
	f32 cx   = w * half;
	f32 cy   = -h * half;
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x + cx, y + cy, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
}

/**
 * @note Address: 0x80146820
 * @note Size: 0xDC
 */
void CCameraActionScroll2::HideObjs()
{
	s32* id    = gScrollCameraHiddenObjectIds;
	s32* state = gScrollCameraHiddenObjectStates;
	for (s32 i = 0; i < 256; i++) {
		if (*id == -1) {
			return;
		}
		XObj* obj = gGame.GetObject(*id);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mStatus.mFlags & 0x10000) {
				*state = 1;
				obj->mStatus.mFlags &= ~0x10000;
			} else {
				*state = 2;
			}
		} else {
			*state = 0;
		}
		id++;
		state++;
	}
}

/**
 * @note Address: 0x80146738
 * @note Size: 0xE8
 */
void CCameraActionScroll2::ShowObjs()
{
	s32* id    = gScrollCameraHiddenObjectIds;
	s32* state = gScrollCameraHiddenObjectStates;
	for (s32 i = 0; i < 256; i++) {
		if (*id == -1) {
			return;
		}
		if (*state == 0 || *state == 2) {
			id++;
			state++;
		} else {
			gGame.mXObjs.mObjects[*id]->mStatus.mFlags |= 0x10000;
			id++;
			state++;
		}
	}
}

// todo: cleanup
/**
 * @note Address: N/A
 * @note Size: ?
 */
inline void CalcLimits(CCameraActionScroll2* camera, Scroll2Limits* limits)
{
	camera->mMaxFov = limits->mMaxFov;
	camera->mMinFov = limits->mMinFov;
	f32 x           = (camera->mFov - camera->mMinFov) / (camera->mMaxFov - camera->mMinFov);
	f32 z;
	if (camera->mFov > limits->mMidFov) {
		z = 1.0f;
	} else {
		z = (camera->mFov - camera->mMinFov) / (limits->mMidFov - camera->mMinFov);
	}
	camera->mMinX = x * (limits->mWideMinX - limits->mNarrowMinX) + limits->mNarrowMinX;
	camera->mMaxX = x * (limits->mWideMaxX - limits->mNarrowMaxX) + limits->mNarrowMaxX;
	camera->mMinZ = z * (limits->mWideMinZ - limits->mNarrowMinZ) + limits->mNarrowMinZ;
	camera->mMaxZ = z * (limits->mWideMaxZ - limits->mNarrowMaxZ) + limits->mNarrowMaxZ;
	camera->mMinX += camera->mStart.x;
	camera->mMaxX += camera->mStart.x;
	camera->mMinZ += camera->mStart.z;
	camera->mMaxZ += camera->mStart.z;
}

/**
 * @note Address: 0x80145FE8
 * @note Size: 0x750
 */
void CCameraActionScroll2::vf14()
{
	if (gGame.mStage == 11) {
		mTurned = TRUE;
	} else {
		mTurned = FALSE;
	}

	if (gSavedScrollCameraInfo != NULL) {
		mFromStage        = FALSE;
		CamInfoData* data = GetStageCameraInfoRecords();
		if (data != NULL) {
			Vec interest = CamInfo(data).GetInterest();
			interest.y += 170.0f;
			SetInterest(interest);
			Vec eye = CamInfo(data).GetEye();
			eye.y += 170.0f;
			SetEye(eye);
			SetDistance(PSVECDistance(&mEye, &mInterest));
			SetFov(CamInfo(data).GetFov());
		}
	} else {
		mFromStage        = TRUE;
		CamInfoData* data = GetStageCameraInfoRecords();
		if (data != NULL) {
			SetInterest(CamInfo(data).GetInterest());
			SetEye(CamInfo(data).GetEye());
			SetDistance(PSVECDistance(&mEye, &mInterest));
			SetFov(CamInfo(data).GetFov());
		}
	}

	mStart.x    = mInterest.x;
	mStart.y    = mInterest.y;
	mStart.z    = mInterest.z;
	CVec offset = mEye - mInterest;
	mOffset.x   = offset.x;
	mOffset.y   = offset.y;
	mOffset.z   = offset.z;
	CVec center = mInterest;
	mCenter.x   = center.x;
	mCenter.y   = center.y;
	mCenter.z   = center.z;
	mCenterPtr  = &mCenter;
	mYaw        = atan2(mInterest.x - mEye.x, mInterest.z - mEye.z);
	CalcLimits(this, GetLimits(gGame.mStage));
	Update(1);
}

static inline Vec GetPanTarget(const CCameraActionScroll2* camera)
{
	return *camera->mCenterPtr;
}

static inline void SetPanCenter(CCameraActionScroll2* camera, Vec position)
{
	camera->mCenter.x = position.x;
	camera->mCenter.y = position.y;
	camera->mCenter.z = position.z;
}

/**
 * @note Address: 0x8014567C
 * @note Size: 0x96C
 */
void CCameraActionScroll2::Update(u32 step)
{
	if (mTurned) {
		Mtx turn;
		PSMTXRotRad(turn, 'x', -HALF_PI);
		Vec eye(mEye);
		PSMTXMultVec(turn, &eye, &eye);
		SetEye(eye);
		Vec interest(mInterest);
		PSMTXMultVec(turn, &interest, &interest);
		SetInterest(interest);
	}

	CPad pad(0);
	if (pad.mPad->mSubStickY != 0.0f) {
		f32 fov = mFov - 0.3f * pad.mPad->mSubStickY * step;
		if (fov < mMinFov) {
			fov = mMinFov;
		}
		if (fov > mMaxFov) {
			fov = mMaxFov;
		}
		SetFov(fov);
		CalcLimits(this, GetLimits(gGame.mStage));
	}

	CVec next;
	f32 stickX = pad.mPad->mStickX;
	f32 stickY = pad.mPad->mStickY;
	if (stickX != 0.0f || stickY != 0.0f) {
		CVec eye(mEye);
		CVec center = Vec(mCenter);
		Vec origin  = eye;
		PSVECSubtract(&center, &origin, &center);
		PSVECNormalize(&center, &center);

		CVec move = CVec(-stickX, 0.0f, stickY);
		Mtx rot;
		PSMTXIdentity(rot);
		PSMTXRotRad(rot, 'y', atan2(mInterest.x - mEye.x, mInterest.z - mEye.z));
		PSMTXMultVec(rot, &move, &move);
		f32 speed = 2.2f * (f32)sin(DEG2RAD * mFov);
		if (speed > 1.5f) {
			speed = 1.5f;
		}
		PSVECScale(&move, &move, speed);

		Mtx tilt;
		PSMTXIdentity(tilt);
		f32 dx    = mInterest.x - mEye.x;
		f32 dz    = mInterest.z - mEye.z;
		f32 dy    = mInterest.y - mEye.y;
		f32 pitch = atan2(sqrtf(dx * dx + dz * dz), sqrtf(dy * dy));
		if (dy < 0.0f) {
			pitch = HALF_PI - pitch;
		} else {
			pitch = pitch - HALF_PI;
		}
		PSMTXRotRad(tilt, 'x', pitch);
		PSMTXRotRad(tilt, 'z', 0.0f);
		PSMTXMultVec(tilt, &move, &move);
		PSVECScale(&center, &center, 50.0f);
		Vec pan = move;
		PSVECAdd(&center, &pan, &center);
		PSVECNormalize(&center, &center);
		PSVECScale(&center, &center, mDistance);

		next = center + eye;
		if (gSavedScrollCameraInfo != NULL) {
			next.y = mInterest.y;
		} else {
			next.y = 0.0f;
		}
		SetPanCenter(this, next);
	}

	CVec center = Vec(mCenter);
	if (center.x < mMinX) {
		center.x = mMinX;
	} else if (center.x > mMaxX) {
		center.x = mMaxX;
	}
	if (center.z < mMinZ) {
		center.z = mMinZ;
	} else if (center.z > mMaxZ) {
		center.z = mMaxZ;
	}
	SetPanCenter(this, center);

	SetInterest(GetPanTarget(this));
	SetEye(mOffset.x + GetPanTarget(this).x, mOffset.y + GetPanTarget(this).y, mOffset.z + GetPanTarget(this).z);

	if (mTurned) {
		Mtx turn;
		PSMTXRotRad(turn, 'x', HALF_PI);
		Vec eye(mEye);
		PSMTXMultVec(turn, &eye, &eye);
		SetEye(eye);
		Vec interest(mInterest);
		PSMTXMultVec(turn, &interest, &interest);
		SetInterest(interest);
	}
}

/**
 * @note Address: 0x80145644
 * @note Size: 0x38
 */
Scroll2Limits* CCameraActionScroll2::GetLimits(s32 room)
{
	if (room < gGame._344C) {
		return ROOM_LIMITS[room];
	}
	return ROOM_LIMITS[7];
}

/**
 * @note Address: 0x80143A00
 * @note Size: 0x1C44
 */
void CCameraActionScroll2::vf20()
{
	static s32 sOverlayFrameCount = 0;

	f32 x;
	f32 y;

	sOverlayFrameCount++;
	f32 blink     = ABS((f32)sin(DEG2RAD * ((sOverlayFrameCount * 4) % 360)));
	f32 slowBlink = ABS((f32)sin(DEG2RAD * ((sOverlayFrameCount * 7) % 360)));

	Vec pos   = { 0.0f, 0.0f, 0.0f };
	u32 color = (u32)(255.0f * blink) | 0xC8C8C800;
	for (s32 i = 0; i < 288; i++) {
		XObj* obj = gGame.mXObjs.mPools[22].Get(i);
		if (!(obj->mStatus.mFlags & 0x10000)) {
			continue;
		}

		switch ((obj)->GetKind()) {
		case 0x10E: {
			Vec ofs = { 0.0f, 0.0f, 0.0f };
			obj->ProjectOffsetToScreen(&ofs, &x, &y, NULL, 0);
			DrawScrollMapMarker(162, x, y, color);
			break;
		}
		case 0x9D: {
			Vec ofs = { 0.0f, 0.0f, 0.0f };
			obj->ProjectOffsetToScreen(&ofs, &x, &y, NULL, 0);
			DrawScrollMapMarker(165, x, y, color);
			break;
		}
		case 0x115:
			if (obj->mStatus.mFlags & 0x10000) {
				Vec ofs = { 0.0f, 0.0f, 0.0f };
				obj->ProjectOffsetToScreen(&ofs, &x, &y, NULL, 0);
				DrawScrollMapMarker(163, x, y, color);
			}
			break;
		case 0xC:
		case 0x19:
		case 0x1A:
		case 0x1B:
		case 0x20:
		case 0x51:
		case 0x70:
		case 0x75:
		case 0x76:
		case 0xD2:
		case 0x165:
		case 0x1B4:
		case 0x1E5:
		case 0x22C:
		case 0x238: {
			// the doors, each says which stage it goes to
			Vec door = { 0.0f, 0.0f, 0.0f };
			pos      = door;
			switch ((obj)->GetKind()) {
			case 0xD2:
			case 0x22C: {
				Vec high = { 0.0f, 0.0f, -100.0f };
				pos      = high;
				break;
			}
			case 0x70: {
				Vec side = { 10.0f, 170.0f, 12.0f };
				pos      = side;
				break;
			}
			}
			if ((obj)->GetKind() == 0x20) {
				Vec far = { -180.0f, 0.0f, -380.0f };
				pos     = far;
				if (!mFromStage) {
					break;
				}
			}

			Vec at = { 0.0f, 0.0f, 0.0f };
			at     = pos;
			obj->ProjectOffsetToScreen(&at, &x, &y, NULL, 0);
			obj->GetFloorXZExtent();
			obj->GetCollisionHeight();

			s32 stage = 0;
			switch (gGame.mStage) {
			case 1:
				if ((obj)->GetKind() == 0xC) {
					stage = 7;
				} else {
					stage = 2;
				}
				// room 2's doors get visited here too
			case 2:
				switch ((obj)->GetKind()) {
				case 0x19:
					stage = 4;
					break;
				case 0x1A:
					stage = 1;
					break;
				case 0x1B:
				case 0x238:
					stage = 6;
					break;
				case 0x20:
					stage = 3;
					break;
				case 0x76:
					stage = 7;
					break;
				}
				break;
			case 3:
				stage = 2;
				break;
			case 4:
				stage = 2;
				break;
			case 5:
				stage = 7;
				break;
			case 6:
				stage = 2;
				break;
			case 7:
				if ((obj)->GetKind() == 0x75) {
					stage = 2;
				} else if ((obj)->GetKind() == 0x1B4) {
					stage = 1;
				} else {
					stage = 9;
				}
				break;
			case 9:
				stage = 7;
				break;
			}

			const char* name = GetStageName(stage);
			MeasureGameTextWidth(&gGame.mGs, name, 1, 16.0f);
			f32 nameX = x;
			f32 nameY = y;
			sin(2.356194496154785);
			cos(2.356194496154785);
#if defined(VERSION_JP)
			u32 cells = strlen(name) / 2 + 1;
			if (cells < 4)
				cells = 4;
			DrawTexturedWindowFrame(0x141414A0, -1, (cells - 1) * 18.0f * 0.5f + nameX, nameY - 18.0f, 18.0f * (cells - 3), 18.0f, 1.0f,
			                        1.0f, 1.0f);
#else
			f32 width = MeasureGameTextWidth(&gGame.mGs, name, 1, 18.0f) * 0.5f;
			DrawTexturedWindowFrame(0x141414A0, -1, MeasureGameTextWidth(&gGame.mGs, name, 1, 18.0f) * 0.5f + nameX, nameY - 18.0f, width,
			                        18.0f, 1.0f, 1.0f, 1.0f);
#endif
			DrawGsProportionalTextFormat(&gGame.mGs, -1, name, nameX, nameY - 18.0f, 0.0f, 18.0f, 18.0f);
			break;
		}
		}
	}

	u32 color2     = (u32)(255.0f * slowBlink) | 0xC8C8C800;
	u32 fadedAlpha = (u32)(255.0f * (1.0f - slowBlink));
#if defined(VERSION_PAL)
	u32 markerColor = fadedAlpha | 0xFFD52C00;
#else
	u32 color2Off = fadedAlpha | 0xC8C8C800;
#endif
	for (s32 i = 0; i < 12; i++) {
		XObjLeticker* obj = static_cast<XObjLeticker*>(gGame.mXObjs.mPools[9].Get(i));
		switch ((obj)->GetKind()) {
		case 0xD3:
		case 0x115:
		case 0x116:
			break;
		default:
			continue;
		}
		if (!(obj->mStatus.mFlags & 0x10000)) {
			continue;
		}

		Vec ofs = { 0.0f, 0.0f, 0.0f };
		obj->ProjectOffsetToScreen(&ofs, &x, &y, NULL, 0);
		if (obj->mPtcl) {
			DrawScrollMapMarker(163, x, y, color2);

			f32 markX = x;
			f32 markY = y;
#if defined(VERSION_PAL)
			for (s32 j = 0; j < 4; j++) {
				DrawGsProportionalTextFormat(&gGame.mGs, fadedAlpha, GetLocalizedInterfaceText(0x115), markX + gScrollRoomData.markerX[j],
				                             (-20.8f + (markY + gScrollRoomData.markerY[j])) - 11.0f, 0.0f, 22.0f, 22.0f);
			}
			DrawGsProportionalTextFormat(&gGame.mGs, markerColor, GetLocalizedInterfaceText(0x115), markX, (markY - 20.8f) - 11.0f, 0.0f,
			                             22.0f, 22.0f);
#else
			CTexObj tex;
			tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[161 - StatusTextureOffset], 0);
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			u16 w    = GXGetTexObjWidth(&tex);
			u16 h    = GXGetTexObjHeight(&tex);
			f32 zoom = 1.0f;
			DrawGsTexturedQuad(&gGame.mGs, &tex, color2Off, color2Off, color2Off, color2Off, markX + 23.4f, markY + -20.8f, 0.0f,
			                   MARK_SCALE * (1.1f * w) / zoom, MARK_SCALE * h / zoom, 0.0f, 0.0f, 0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
#endif
		} else {
			DrawScrollMapMarker(163, x, y, color);
		}
	}

	{
		Vec ofs = { 0.0f, 0.0f, 0.0f };
		gGame.GetPlayer()->ProjectOffsetToScreen(&ofs, &x, &y, NULL, 0);
		DrawScrollMapMarker(164, x, y, color);
	}

	// the arrow round the player points the way it faces
	f32 angle;
	switch (gGame.mStage) {
	case 3:
		angle = PI + PLAYER->mMoveRotate.y;
		break;
	case 11:
		if (HALF_PI + PLAYER->mMoveRotate.y > PI) {
			angle = -HALF_PI;
		} else {
			angle = HALF_PI;
		}
		break;
	default:
		angle = HALF_PI + PLAYER->mMoveRotate.y;
		break;
	}

	CTexObj tex;
	tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[160 - StatusTextureOffset], 0);
	GXGetTexObjWidth(&tex);
	GXGetTexObjHeight(&tex);
	sin(angle);
	cos(angle);
	f32 pulse = sin(DEG2RAD * ((sOverlayFrameCount * 24) % 360));
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	tex        = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[166 - StatusTextureOffset], 0);
	f32 zoom   = 1.0f;
	f32 arrowW = zoom * (1.1f * GXGetTexObjWidth(&tex)) / zoom;
	u16 arrowH = GXGetTexObjHeight(&tex);
	f32 radius = arrowW / 3.0f + 3.0f * pulse * zoom / zoom;
	f32 arrowX = radius * (f32)sin(angle) + x;
	DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, arrowX, radius * (f32)cos(angle) + y, 0.0f, arrowW, zoom * arrowH / zoom, 0.0f,
	                   0.0f, PI + angle);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);

	// the caption in the corner, with a dark outline
	CaptionMsg msg = { 7, NULL };
	msg.mText      = GetLocalizedInterfaceText(15);
	f32 slide      = HALF_PI; // a slide in, always at its end here
	f32 captionX   = 100.0f * (1.0f - (f32)sin(slide)) + 576.0f;
	f32 fade       = 255.0f;
	u32 alpha      = (u32)fade;
	u32 white      = alpha | 0xFFFFFF00;
	f32 frame      = 0.05882353f * msg.mId;
	captionX -= MeasureGameTextWidth(&gGame.mGs, msg.mText, 1, 20.0f);
	s32 line     = 0;
	f32 captionY = 24.0f * line + 62.0f;
	for (s32 i = 0; i < 4; i++) {
		DrawGsProportionalTextFormat(&gGame.mGs, alpha, msg.mText, captionX + CAPTION_OUTLINE_X[i], captionY + CAPTION_OUTLINE_Y[i], 0.0f,
		                             20.0f, 20.0f);
	}
	DrawGsProportionalTextFormat(&gGame.mGs, white, msg.mText, captionX, captionY, 0.0f, 20.0f, 20.0f);

	CTexObj icon;
	icon = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[119 - StatusTextureOffset], 0);
	PSMTXScale(icon.mMtx[1], 0.05882353f, 1.0f, 1.0f);
	icon.mMtxDirty = TRUE;
	PSMTXTrans(icon.mMtx[0], frame, 0.0f, 0.0f);
	icon.mMtxDirty = TRUE;
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	DrawGsTexturedQuad(&gGame.mGs, &icon, white, white, white, white, captionX - 12.0f, captionY + 10.0f, 0.0f, 24.0f, 24.0f, 0.0f, 0.0f,
	                   0.0f);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);

	if (gGame.mStage >= 0 && gGame.mStage < 10) {
		static const s32 sRoomBaseCounts[10] = { 0, 9, 11, 4, 7, 0, 12, 9, 0, 8 };
		s32 base                             = sRoomBaseCounts[gGame.mStage];
		// script variables from 0x603 on hold a count per room
		s32 count = gEventVariables[0x603 + gGame.mStage];
		if (base > 0) {
			DrawSeparatedStatusValue(0xAB - StatusTextureOffset, base + count * 100, false, 56.0f, 378.0f, 1.0f);
		}
	}
}
/*
    stwu     r1, -0xa70(r1)
    mflr     r0
    stw      r0, 0xa74(r1)
    stfd     f31, 0xa60(r1)
    psq_st   f31, -0x598(r1), 0, qr0
    stfd     f30, 0xa50(r1)
    psq_st   f30, -0x5a8(r1), 0, qr0
    stfd     f29, 0xa40(r1)
    psq_st   f29, -0x5b8(r1), 0, qr0
    stfd     f28, 0xa30(r1)
    psq_st   f28, -0x5c8(r1), 0, qr0
    stfd     f27, 0xa20(r1)
    psq_st   f27, -0x5d8(r1), 0, qr0
    stfd     f26, 0xa10(r1)
    psq_st   f26, -0x5e8(r1), 0, qr0
    stfd     f25, 0xa00(r1)
    psq_st   f25, -0x5f8(r1), 0, qr0
    stmw     r16, 0x9c0(r1)
    lbz      r0, sScrollOverlayFrameCountInitialized@sda21(r0)
    lis      r4, lbl_8020CE70@ha
    mr       r24, r3
    extsb.   r0, r0
    addi     r31, r4, lbl_8020CE70@l
    bne      .L_80143A70
    li       r3, 0x0
    li       r0, 0x1
    stw      r3, sScrollOverlayFrameCount@sda21(r0)
    stb      r0, sScrollOverlayFrameCountInitialized@sda21(r0)
.L_80143A70:
    lwz      r4, sScrollOverlayFrameCount@sda21(r0)
    lis      r3, 0xb60b
    lis      r0, 0x4330
    lfd      f1, lbl_8065EB78@sda21(r0)
    addi     r5, r4, 0x1
    addi     r3, r3, 0x60b7
    slwi     r4, r5, 2
    stw      r0, 0x9a0(r1)
    mulhw    r0, r3, r4
    lfs      f2, lbl_8065EB24@sda21(r0)
    stw      r5, sScrollOverlayFrameCount@sda21(r0)
    add      r0, r0, r4
    srawi    r0, r0, 8
    srwi     r3, r0, 31
    add      r0, r0, r3
    mulli    r0, r0, 0x168
    subf     r0, r0, r4
    xoris    r0, r0, 0x8000
    stw      r0, 0x9a4(r1)
    lfd      f0, 0x9a0(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f2, f0
    bl       sin
    frsp     f1, f1
    lfs      f0, lbl_8065EB0C@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_80143B3C
    lwz      r4, sScrollOverlayFrameCount@sda21(r0)
    lis      r3, 0xb60b
    lis      r0, 0x4330
    lfd      f1, lbl_8065EB78@sda21(r0)
    slwi     r4, r4, 2
    addi     r3, r3, 0x60b7
    mulhw    r3, r3, r4
    stw      r0, 0x9a0(r1)
    lfs      f2, lbl_8065EB24@sda21(r0)
    add      r0, r3, r4
    srawi    r0, r0, 8
    srwi     r3, r0, 31
    add      r0, r0, r3
    mulli    r0, r0, 0x168
    subf     r0, r0, r4
    xoris    r0, r0, 0x8000
    stw      r0, 0x9a4(r1)
    lfd      f0, 0x9a0(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f2, f0
    bl       sin
    frsp     f0, f1
    fneg     f27, f0
    b        .L_80143B94
.L_80143B3C:
    lwz      r4, sScrollOverlayFrameCount@sda21(r0)
    lis      r3, 0xb60b
    lis      r0, 0x4330
    lfd      f1, lbl_8065EB78@sda21(r0)
    slwi     r4, r4, 2
    addi     r3, r3, 0x60b7
    mulhw    r3, r3, r4
    stw      r0, 0x9a0(r1)
    lfs      f2, lbl_8065EB24@sda21(r0)
    add      r0, r3, r4
    srawi    r0, r0, 8
    srwi     r3, r0, 31
    add      r0, r0, r3
    mulli    r0, r0, 0x168
    subf     r0, r0, r4
    xoris    r0, r0, 0x8000
    stw      r0, 0x9a4(r1)
    lfd      f0, 0x9a0(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f2, f0
    bl       sin
    frsp     f27, f1
.L_80143B94:
    lwz      r4, sScrollOverlayFrameCount@sda21(r0)
    lis      r3, 0xb60b
    lis      r0, 0x4330
    lfd      f1, lbl_8065EB78@sda21(r0)
    mulli    r4, r4, 0x7
    addi     r3, r3, 0x60b7
    stw      r0, 0x9a0(r1)
    lfs      f2, lbl_8065EB24@sda21(r0)
    mulhw    r0, r3, r4
    add      r0, r0, r4
    srawi    r0, r0, 8
    srwi     r3, r0, 31
    add      r0, r0, r3
    mulli    r0, r0, 0x168
    subf     r0, r0, r4
    xoris    r0, r0, 0x8000
    stw      r0, 0x9a4(r1)
    lfd      f0, 0x9a0(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f2, f0
    bl       sin
    frsp     f1, f1
    lfs      f0, lbl_8065EB0C@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_80143C58
    lwz      r4, sScrollOverlayFrameCount@sda21(r0)
    lis      r3, 0xb60b
    lis      r0, 0x4330
    lfd      f1, lbl_8065EB78@sda21(r0)
    mulli    r4, r4, 0x7
    addi     r3, r3, 0x60b7
    stw      r0, 0x9a0(r1)
    lfs      f2, lbl_8065EB24@sda21(r0)
    mulhw    r0, r3, r4
    add      r0, r0, r4
    srawi    r0, r0, 8
    srwi     r3, r0, 31
    add      r0, r0, r3
    mulli    r0, r0, 0x168
    subf     r0, r0, r4
    xoris    r0, r0, 0x8000
    stw      r0, 0x9a4(r1)
    lfd      f0, 0x9a0(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f2, f0
    bl       sin
    frsp     f0, f1
    fneg     f29, f0
    b        .L_80143CB0
.L_80143C58:
    lwz      r4, sScrollOverlayFrameCount@sda21(r0)
    lis      r3, 0xb60b
    lis      r0, 0x4330
    lfd      f1, lbl_8065EB78@sda21(r0)
    mulli    r4, r4, 0x7
    addi     r3, r3, 0x60b7
    stw      r0, 0x9a0(r1)
    lfs      f2, lbl_8065EB24@sda21(r0)
    mulhw    r0, r3, r4
    add      r0, r0, r4
    srawi    r0, r0, 8
    srwi     r3, r0, 31
    add      r0, r0, r3
    mulli    r0, r0, 0x168
    subf     r0, r0, r4
    xoris    r0, r0, 0x8000
    stw      r0, 0x9a4(r1)
    lfd      f0, 0x9a0(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f2, f0
    bl       sin
    frsp     f29, f1
.L_80143CB0:
    lfs      f0, lbl_8065EB04@sda21(r0)
    lis      r3, gGame@ha
    addi     r29, r3, gGame@l
    lwz      r4, 0x70(r31)
    lwz      r3, 0x74(r31)
    fmuls    f1, f0, f27
    lwz      r0, 0x78(r31)
    addi     r28, r29, 0x3d6c
    stw      r4, 0x90(r1)
    addis    r27, r29, 0x1
    li       r26, 0x0
    stw      r3, 0x94(r1)
    stw      r0, 0x98(r1)
    bl       __cvt_fp2unsigned
    oris     r30, r3, 0xc8c8
    lis      r3, gGame@ha
    ori      r30, r30, 0xc800
    lis      r18, 0x1
    addi     r25, r3, gGame@l
.L_80143CFC:
    lwz      r3, -0x67a4(r27)
    li       r0, 0x0
    lwz      r4, -0x67b4(r27)
    mullw    r3, r26, r3
    add      r16, r4, r3
    lwz      r3, 0x228(r16)
    lwz      r4, 0x22c(r16)
    and      r5, r3, r0
    and      r4, r4, r18
    xor      r3, r4, r0
    xor      r0, r5, r0
    or.      r0, r3, r0
    beq      .L_801447B0
    lwz      r3, 0x244(r16)
    cmplwi   r3, 0x0
    beq      .L_80143D44
    lwz      r0, 0x0(r3)
    b        .L_80143D48
.L_80143D44:
    li       r0, -0x1
.L_80143D48:
    cmpwi    r0, 0xd2
    beq      .L_801443E4
    bge      .L_80143DB8
    cmpwi    r0, 0x51
    beq      .L_801443E4
    bge      .L_80143D8C
    cmpwi    r0, 0x1c
    bge      .L_80143D80
    cmpwi    r0, 0xc
    beq      .L_801443E4
    blt      .L_801447B0
    cmpwi    r0, 0x19
    bge      .L_801443E4
    b        .L_801447B0
.L_80143D80:
    cmpwi    r0, 0x20
    beq      .L_801443E4
    b        .L_801447B0
.L_80143D8C:
    cmpwi    r0, 0x77
    bge      .L_80143DAC
    cmpwi    r0, 0x70
    beq      .L_801443E4
    blt      .L_801447B0
    cmpwi    r0, 0x75
    bge      .L_801443E4
    b        .L_801447B0
.L_80143DAC:
    cmpwi    r0, 0x9d
    beq      .L_80143FF8
    b        .L_801447B0
.L_80143DB8:
    cmpwi    r0, 0x1b4
    beq      .L_801443E4
    bge      .L_80143DE8
    cmpwi    r0, 0x115
    beq      .L_801441E4
    bge      .L_80143DDC
    cmpwi    r0, 0x10e
    beq      .L_80143E0C
    b        .L_801447B0
.L_80143DDC:
    cmpwi    r0, 0x165
    beq      .L_801443E4
    b        .L_801447B0
.L_80143DE8:
    cmpwi    r0, 0x22c
    beq      .L_801443E4
    bge      .L_80143E00
    cmpwi    r0, 0x1e5
    beq      .L_801443E4
    b        .L_801447B0
.L_80143E00:
    cmpwi    r0, 0x238
    beq      .L_801443E4
    b        .L_801447B0
.L_80143E0C:
    lwz      r6, 0x7c(r31)
    mr       r3, r16
    lwz      r9, 0x80(r31)
    addi     r4, r1, 0x84
    lwz      r0, 0x84(r31)
    addi     r5, r1, 0xc
    stw      r6, 0x84(r1)
    addi     r6, r1, 0x8
    li       r7, 0x0
    li       r8, 0x0
    stw      r9, 0x88(r1)
    stw      r0, 0x8c(r1)
    lwz      r12, 0x34(r16)
    lwz      r12, 0x10c(r12)
    mtctr    r12
    bctrl
    lis      r3, __vt__7CTexObj@ha
    lwz      r4, 0x3d4c(r29)
    addi     r0, r3, __vt__7CTexObj@l
    lfs      f30, 0x8(r1)
    stw      r0, 0x798(r1)
    mr       r3, r28
    lfs      f31, 0xc(r1)
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x510
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    addi     r5, r1, 0x6d4
    addi     r4, r3, 0x38
    stw      r7, 0x69c(r1)
    stw      r6, 0x6a0(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x6a4(r1)
    stw      r6, 0x6a8(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x6ac(r1)
    stw      r6, 0x6b0(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x6b4(r1)
    stw      r6, 0x6b8(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x6bc(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x6c0(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x6c4(r1)
    stw      r6, 0x6c8(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x6cc(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x6d0(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x6d4(r1)
    mtctr    r0
.L_80143F00:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_80143F00
    lwz      r3, 0x294(r29)
    li       r4, 0x0
    bl       SetGsTevMode
    addi     r3, r1, 0x69c
    bl       GXGetTexObjWidth
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x9a4(r1)
    addi     r3, r1, 0x69c
    lfd      f1, lbl_8065EB18@sda21(r0)
    stw      r0, 0x9a0(r1)
    lfs      f2, lbl_8065EB10@sda21(r0)
    lfd      f0, 0x9a0(r1)
    lfs      f3, lbl_8065EB28@sda21(r0)
    fsubs    f1, f0, f1
    lfs      f0, lbl_8065EB08@sda21(r0)
    fmuls    f1, f2, f1
    fmuls    f1, f3, f1
    fdivs    f27, f1, f0
    bl       GXGetTexObjHeight
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x9ac(r1)
    lis      r3, gGame@ha
    lfs      f3, lbl_8065EB0C@sda21(r0)
    fmr      f4, f27
    stw      r0, 0x9a8(r1)
    addi     r3, r3, gGame@l
    lfd      f1, lbl_8065EB18@sda21(r0)
    fmr      f6, f3
    lfd      f0, 0x9a8(r1)
    lfs      f9, lbl_8065EB28@sda21(r0)
    fmr      f7, f3
    fsubs    f1, f0, f1
    lfs      f2, lbl_8065EB20@sda21(r0)
    lfs      f5, lbl_8065EB08@sda21(r0)
    fmr      f8, f3
    fmuls    f0, f27, f2
    fmuls    f9, f9, f1
    mr       r5, r30
    mr       r6, r30
    fadds    f1, f31, f0
    mr       r7, r30
    fdivs    f5, f9, f5
    mr       r8, r30
    addi     r4, r1, 0x69c
    fneg     f0, f5
    fmuls    f0, f0, f2
    fadds    f2, f30, f0
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r29)
    li       r4, 0x4
    bl       SetGsTevMode
    lis      r3, __vt__7CTexObj@ha
    addi     r0, r3, __vt__7CTexObj@l
    stw      r0, 0x798(r1)
    b        .L_801447B0
.L_80143FF8:
    lwz      r6, 0x88(r31)
    mr       r3, r16
    lwz      r9, 0x8c(r31)
    addi     r4, r1, 0x78
    lwz      r0, 0x90(r31)
    addi     r5, r1, 0xc
    stw      r6, 0x78(r1)
    addi     r6, r1, 0x8
    li       r7, 0x0
    li       r8, 0x0
    stw      r9, 0x7c(r1)
    stw      r0, 0x80(r1)
    lwz      r12, 0x34(r16)
    lwz      r12, 0x10c(r12)
    mtctr    r12
    bctrl
    lis      r3, __vt__7CTexObj@ha
    lwz      r4, 0x3d4c(r29)
    addi     r0, r3, __vt__7CTexObj@l
    lfs      f31, 0x8(r1)
    stw      r0, 0x698(r1)
    mr       r3, r28
    lfs      f30, 0xc(r1)
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x528
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    addi     r5, r1, 0x5d4
    addi     r4, r3, 0x38
    stw      r7, 0x59c(r1)
    stw      r6, 0x5a0(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x5a4(r1)
    stw      r6, 0x5a8(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x5ac(r1)
    stw      r6, 0x5b0(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x5b4(r1)
    stw      r6, 0x5b8(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x5bc(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x5c0(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x5c4(r1)
    stw      r6, 0x5c8(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x5cc(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x5d0(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x5d4(r1)
    mtctr    r0
.L_801440EC:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_801440EC
    lwz      r3, 0x294(r29)
    li       r4, 0x0
    bl       SetGsTevMode
    addi     r3, r1, 0x59c
    bl       GXGetTexObjWidth
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x9ac(r1)
    addi     r3, r1, 0x59c
    lfd      f1, lbl_8065EB18@sda21(r0)
    stw      r0, 0x9a8(r1)
    lfs      f2, lbl_8065EB10@sda21(r0)
    lfd      f0, 0x9a8(r1)
    lfs      f3, lbl_8065EB28@sda21(r0)
    fsubs    f1, f0, f1
    lfs      f0, lbl_8065EB08@sda21(r0)
    fmuls    f1, f2, f1
    fmuls    f1, f3, f1
    fdivs    f27, f1, f0
    bl       GXGetTexObjHeight
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x9a4(r1)
    lis      r3, gGame@ha
    lfs      f3, lbl_8065EB0C@sda21(r0)
    fmr      f4, f27
    stw      r0, 0x9a0(r1)
    addi     r3, r3, gGame@l
    lfd      f1, lbl_8065EB18@sda21(r0)
    fmr      f6, f3
    lfd      f0, 0x9a0(r1)
    lfs      f9, lbl_8065EB28@sda21(r0)
    fmr      f7, f3
    fsubs    f1, f0, f1
    lfs      f2, lbl_8065EB20@sda21(r0)
    lfs      f5, lbl_8065EB08@sda21(r0)
    fmr      f8, f3
    fmuls    f0, f27, f2
    fmuls    f9, f9, f1
    mr       r5, r30
    mr       r6, r30
    fadds    f1, f30, f0
    mr       r7, r30
    fdivs    f5, f9, f5
    mr       r8, r30
    addi     r4, r1, 0x59c
    fneg     f0, f5
    fmuls    f0, f0, f2
    fadds    f2, f31, f0
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r29)
    li       r4, 0x4
    bl       SetGsTevMode
    lis      r3, __vt__7CTexObj@ha
    addi     r0, r3, __vt__7CTexObj@l
    stw      r0, 0x698(r1)
    b        .L_801447B0
.L_801441E4:
    li       r0, 0x0
    xor      r3, r4, r0
    xor      r0, r5, r0
    or.      r0, r3, r0
    beq      .L_801447B0
    lwz      r6, 0x94(r31)
    mr       r3, r16
    lwz      r9, 0x98(r31)
    addi     r4, r1, 0x6c
    lwz      r0, 0x9c(r31)
    addi     r5, r1, 0xc
    stw      r6, 0x6c(r1)
    addi     r6, r1, 0x8
    li       r7, 0x0
    li       r8, 0x0
    stw      r9, 0x70(r1)
    stw      r0, 0x74(r1)
    lwz      r12, 0x34(r16)
    lwz      r12, 0x10c(r12)
    mtctr    r12
    bctrl
    lis      r3, __vt__7CTexObj@ha
    lwz      r4, 0x3d4c(r29)
    addi     r0, r3, __vt__7CTexObj@l
    lfs      f31, 0x8(r1)
    stw      r0, 0x598(r1)
    mr       r3, r28
    lfs      f30, 0xc(r1)
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x518
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    addi     r5, r1, 0x4d4
    addi     r4, r3, 0x38
    stw      r7, 0x49c(r1)
    stw      r6, 0x4a0(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x4a4(r1)
    stw      r6, 0x4a8(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x4ac(r1)
    stw      r6, 0x4b0(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x4b4(r1)
    stw      r6, 0x4b8(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x4bc(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x4c0(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x4c4(r1)
    stw      r6, 0x4c8(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x4cc(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x4d0(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x4d4(r1)
    mtctr    r0
.L_801442EC:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_801442EC
    lwz      r3, 0x294(r29)
    li       r4, 0x0
    bl       SetGsTevMode
    addi     r3, r1, 0x49c
    bl       GXGetTexObjWidth
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x9ac(r1)
    addi     r3, r1, 0x49c
    lfd      f1, lbl_8065EB18@sda21(r0)
    stw      r0, 0x9a8(r1)
    lfs      f2, lbl_8065EB10@sda21(r0)
    lfd      f0, 0x9a8(r1)
    lfs      f3, lbl_8065EB28@sda21(r0)
    fsubs    f1, f0, f1
    lfs      f0, lbl_8065EB08@sda21(r0)
    fmuls    f1, f2, f1
    fmuls    f1, f3, f1
    fdivs    f27, f1, f0
    bl       GXGetTexObjHeight
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x9a4(r1)
    lis      r3, gGame@ha
    lfs      f3, lbl_8065EB0C@sda21(r0)
    fmr      f4, f27
    stw      r0, 0x9a0(r1)
    addi     r3, r3, gGame@l
    lfd      f1, lbl_8065EB18@sda21(r0)
    fmr      f6, f3
    lfd      f0, 0x9a0(r1)
    lfs      f9, lbl_8065EB28@sda21(r0)
    fmr      f7, f3
    fsubs    f1, f0, f1
    lfs      f2, lbl_8065EB20@sda21(r0)
    lfs      f5, lbl_8065EB08@sda21(r0)
    fmr      f8, f3
    fmuls    f0, f27, f2
    fmuls    f9, f9, f1
    mr       r5, r30
    mr       r6, r30
    fadds    f1, f30, f0
    mr       r7, r30
    fdivs    f5, f9, f5
    mr       r8, r30
    addi     r4, r1, 0x49c
    fneg     f0, f5
    fmuls    f0, f0, f2
    fadds    f2, f31, f0
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r29)
    li       r4, 0x4
    bl       SetGsTevMode
    lis      r3, __vt__7CTexObj@ha
    addi     r0, r3, __vt__7CTexObj@l
    stw      r0, 0x598(r1)
    b        .L_801447B0
.L_801443E4:
    lwz      r4, 0xa0(r31)
    lwz      r3, 0xa4(r31)
    lwz      r0, 0xa8(r31)
    stw      r4, 0x60(r1)
    lwz      r4, 0x244(r16)
    stw      r3, 0x64(r1)
    lfs      f2, 0x60(r1)
    cmplwi   r4, 0x0
    stw      r0, 0x68(r1)
    lfs      f1, 0x64(r1)
    lfs      f0, 0x68(r1)
    stfs     f2, 0x90(r1)
    stfs     f1, 0x94(r1)
    stfs     f0, 0x98(r1)
    beq      .L_80144428
    lwz      r0, 0x0(r4)
    b        .L_8014442C
.L_80144428:
    li       r0, -0x1
.L_8014442C:
    cmpwi    r0, 0xd2
    beq      .L_80144450
    bge      .L_80144444
    cmpwi    r0, 0x70
    beq      .L_80144484
    b        .L_801444B4
.L_80144444:
    cmpwi    r0, 0x22c
    beq      .L_80144450
    b        .L_801444B4
.L_80144450:
    lwz      r4, 0xac(r31)
    lwz      r3, 0xb0(r31)
    lwz      r0, 0xb4(r31)
    stw      r4, 0x54(r1)
    stw      r3, 0x58(r1)
    lfs      f2, 0x54(r1)
    stw      r0, 0x5c(r1)
    lfs      f1, 0x58(r1)
    lfs      f0, 0x5c(r1)
    stfs     f2, 0x90(r1)
    stfs     f1, 0x94(r1)
    stfs     f0, 0x98(r1)
    b        .L_801444B4
.L_80144484:
    lwz      r4, 0xb8(r31)
    lwz      r3, 0xbc(r31)
    lwz      r0, 0xc0(r31)
    stw      r4, 0x48(r1)
    stw      r3, 0x4c(r1)
    lfs      f2, 0x48(r1)
    stw      r0, 0x50(r1)
    lfs      f1, 0x4c(r1)
    lfs      f0, 0x50(r1)
    stfs     f2, 0x90(r1)
    stfs     f1, 0x94(r1)
    stfs     f0, 0x98(r1)
.L_801444B4:
    lwz      r3, 0x244(r16)
    cmplwi   r3, 0x0
    beq      .L_801444C8
    lwz      r0, 0x0(r3)
    b        .L_801444CC
.L_801444C8:
    li       r0, -0x1
.L_801444CC:
    cmpwi    r0, 0x20
    bne      .L_80144510
    lwz      r0, 0xc4(r31)
    lwz      r4, 0xc8(r31)
    lwz      r3, 0xcc(r31)
    stw      r0, 0x3c(r1)
    lwz      r0, 0x88(r24)
    stw      r4, 0x40(r1)
    lfs      f2, 0x3c(r1)
    cmpwi    r0, 0x0
    stw      r3, 0x44(r1)
    lfs      f1, 0x40(r1)
    lfs      f0, 0x44(r1)
    stfs     f2, 0x90(r1)
    stfs     f1, 0x94(r1)
    stfs     f0, 0x98(r1)
    beq      .L_801447B0
.L_80144510:
    lwz      r6, 0xd0(r31)
    mr       r3, r16
    lwz      r8, 0xd4(r31)
    addi     r4, r1, 0x30
    lwz      r0, 0xd8(r31)
    addi     r5, r1, 0xc
    stw      r6, 0x30(r1)
    addi     r6, r1, 0x8
    lfs      f2, 0x90(r1)
    li       r7, 0x0
    stw      r8, 0x34(r1)
    li       r8, 0x0
    lfs      f1, 0x94(r1)
    stw      r0, 0x38(r1)
    lfs      f0, 0x98(r1)
    stfs     f2, 0x30(r1)
    stfs     f1, 0x34(r1)
    stfs     f0, 0x38(r1)
    lwz      r12, 0x34(r16)
    lwz      r12, 0x10c(r12)
    mtctr    r12
    bctrl
    mr       r3, r16
    lwz      r12, 0x34(r16)
    lwz      r12, 0x110(r12)
    mtctr    r12
    bctrl
    mr       r3, r16
    lwz      r12, 0x34(r16)
    lwz      r12, 0x114(r12)
    mtctr    r12
    bctrl
    lwz      r0, 0x3444(r25)
    li       r3, 0x0
    cmplwi   r0, 0x9
    bgt      .L_801446E4
    lis      r4, jumptable_8022EA18@ha
    slwi     r0, r0, 2
    addi     r4, r4, jumptable_8022EA18@l
    lwzx     r0, r4, r0
    mtctr    r0
    bctr
.L_801445B8:
    lwz      r3, 0x244(r16)
    cmplwi   r3, 0x0
    beq      .L_801445CC
    lwz      r0, 0x0(r3)
    b        .L_801445D0
.L_801445CC:
    li       r0, -0x1
.L_801445D0:
    cmpwi    r0, 0xc
    bne      .L_801445E0
    li       r3, 0x7
    b        .L_801445E4
.L_801445E0:
    li       r3, 0x2
.L_801445E4:
    lwz      r4, 0x244(r16)
    cmplwi   r4, 0x0
    beq      .L_801445F8
    lwz      r0, 0x0(r4)
    b        .L_801445FC
.L_801445F8:
    li       r0, -0x1
.L_801445FC:
    cmpwi    r0, 0x20
    beq      .L_8014465C
    bge      .L_8014462C
    cmpwi    r0, 0x1a
    beq      .L_8014464C
    bge      .L_80144620
    cmpwi    r0, 0x19
    bge      .L_80144644
    b        .L_801446E4
.L_80144620:
    cmpwi    r0, 0x1c
    bge      .L_801446E4
    b        .L_80144654
.L_8014462C:
    cmpwi    r0, 0x238
    beq      .L_80144654
    bge      .L_801446E4
    cmpwi    r0, 0x76
    beq      .L_80144664
    b        .L_801446E4
.L_80144644:
    li       r3, 0x4
    b        .L_801446E4
.L_8014464C:
    li       r3, 0x1
    b        .L_801446E4
.L_80144654:
    li       r3, 0x6
    b        .L_801446E4
.L_8014465C:
    li       r3, 0x3
    b        .L_801446E4
.L_80144664:
    li       r3, 0x7
    b        .L_801446E4
.L_8014466C:
    li       r3, 0x2
    b        .L_801446E4
.L_80144674:
    li       r3, 0x2
    b        .L_801446E4
.L_8014467C:
    li       r3, 0x7
    b        .L_801446E4
.L_80144684:
    li       r3, 0x2
    b        .L_801446E4
.L_8014468C:
    lwz      r3, 0x244(r16)
    cmplwi   r3, 0x0
    beq      .L_801446A0
    lwz      r0, 0x0(r3)
    b        .L_801446A4
.L_801446A0:
    li       r0, -0x1
.L_801446A4:
    cmpwi    r0, 0x75
    bne      .L_801446B4
    li       r3, 0x2
    b        .L_801446E4
.L_801446B4:
    cmplwi   r3, 0x0
    beq      .L_801446C4
    lwz      r0, 0x0(r3)
    b        .L_801446C8
.L_801446C4:
    li       r0, -0x1
.L_801446C8:
    cmpwi    r0, 0x1b4
    bne      .L_801446D8
    li       r3, 0x1
    b        .L_801446E4
.L_801446D8:
    li       r3, 0x9
    b        .L_801446E4
.L_801446E0:
    li       r3, 0x7
.L_801446E4:
    bl       GetStageName
    lis      r4, gGame@ha
    mr       r16, r3
    addi     r3, r4, gGame@l
    lfs      f1, lbl_8065EB2C@sda21(r0)
    li       r5, 0x1
    mr       r4, r16
    bl       MeasureGameTextWidth
    lfs      f26, 0xc(r1)
    lfs      f28, 0x8(r1)
    lfd      f1, lbl_8065EB30@sda21(r0)
    bl       sin
    lfd      f1, lbl_8065EB30@sda21(r0)
    bl       cos
    lis      r3, gGame@ha
    lfs      f1, lbl_8065EB38@sda21(r0)
    addi     r3, r3, gGame@l
    mr       r4, r16
    li       r5, 0x1
    bl       MeasureGameTextWidth
    lfs      f0, lbl_8065EB20@sda21(r0)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    mr       r4, r16
    fmuls    f27, f1, f0
    lfs      f1, lbl_8065EB38@sda21(r0)
    li       r5, 0x1
    bl       MeasureGameTextWidth
    lfs      f5, lbl_8065EB08@sda21(r0)
    lis      r3, 0x1414
    lfs      f0, lbl_8065EB20@sda21(r0)
    fmr      f3, f27
    lfs      f4, lbl_8065EB38@sda21(r0)
    fmr      f6, f5
    fmadds   f1, f1, f0, f26
    addi     r3, r3, 0x14a0
    fmr      f7, f5
    fsubs    f2, f28, f4
    li       r4, -0x1
    bl       DrawTexturedWindowFrame
    lfs      f4, lbl_8065EB38@sda21(r0)
    lis      r3, gGame@ha
    fmr      f1, f26
    lfs      f3, lbl_8065EB0C@sda21(r0)
    fmr      f5, f4
    addi     r3, r3, gGame@l
    fsubs    f2, f28, f4
    mr       r5, r16
    li       r4, -0x1
    crset    cr1eq
    bl       DrawGsProportionalTextFormat
.L_801447B0:
    addi     r26, r26, 0x1
    cmpwi    r26, 0x120
    blt      .L_80143CFC
    lfs      f0, lbl_8065EB04@sda21(r0)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    li       r20, 0x0
    fmuls    f1, f0, f29
    addis    r17, r3, 0x1
    bl       __cvt_fp2unsigned
    lfs      f0, lbl_8065EB08@sda21(r0)
    oris     r18, r3, 0xc8c8
    lfs      f1, lbl_8065EB04@sda21(r0)
    ori      r18, r18, 0xc800
    fsubs    f0, f0, f29
    fmuls    f1, f1, f0
    bl       __cvt_fp2unsigned
    oris     r19, r3, 0xc8c8
    ori      r19, r19, 0xc800
.L_801447FC:
    lwz      r0, -0x68a8(r17)
    lwz      r3, -0x68b8(r17)
    mullw    r0, r20, r0
    add      r16, r3, r0
    lwz      r3, 0x244(r16)
    cmplwi   r3, 0x0
    beq      .L_80144820
    lwz      r0, 0x0(r3)
    b        .L_80144824
.L_80144820:
    li       r0, -0x1
.L_80144824:
    cmpwi    r0, 0x115
    bge      .L_80144838
    cmpwi    r0, 0xd3
    beq      .L_80144840
    b        .L_80144D90
.L_80144838:
    cmpwi    r0, 0x117
    bge      .L_80144D90
.L_80144840:
    lwz      r0, 0x228(r16)
    li       r4, 0x0
    lwz      r5, 0x22c(r16)
    lis      r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_80144D90
    lwz      r6, 0xdc(r31)
    mr       r3, r16
    lwz      r9, 0xe0(r31)
    addi     r4, r1, 0x24
    lwz      r0, 0xe4(r31)
    addi     r5, r1, 0xc
    stw      r6, 0x24(r1)
    addi     r6, r1, 0x8
    li       r7, 0x0
    li       r8, 0x0
    stw      r9, 0x28(r1)
    stw      r0, 0x2c(r1)
    lwz      r12, 0x34(r16)
    lwz      r12, 0x10c(r12)
    mtctr    r12
    bctrl
    lbz      r0, 0xa2c(r16)
    cmplwi   r0, 0x0
    beq      .L_80144BE8
    lis      r3, __vt__7CTexObj@ha
    lwz      r4, 0x3d4c(r29)
    addi     r0, r3, __vt__7CTexObj@l
    lfs      f31, 0x8(r1)
    stw      r0, 0x498(r1)
    mr       r3, r28
    lfs      f30, 0xc(r1)
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x518
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    addi     r5, r1, 0x3d4
    addi     r4, r3, 0x38
    stw      r7, 0x39c(r1)
    stw      r6, 0x3a0(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x3a4(r1)
    stw      r6, 0x3a8(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x3ac(r1)
    stw      r6, 0x3b0(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x3b4(r1)
    stw      r6, 0x3b8(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x3bc(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x3c0(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x3c4(r1)
    stw      r6, 0x3c8(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x3cc(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x3d0(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x3d4(r1)
    mtctr    r0
.L_80144968:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_80144968
    lwz      r3, 0x294(r29)
    li       r4, 0x0
    bl       SetGsTevMode
    addi     r3, r1, 0x39c
    bl       GXGetTexObjWidth
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x9ac(r1)
    addi     r3, r1, 0x39c
    lfd      f1, lbl_8065EB18@sda21(r0)
    stw      r0, 0x9a8(r1)
    lfs      f2, lbl_8065EB10@sda21(r0)
    lfd      f0, 0x9a8(r1)
    lfs      f3, lbl_8065EB28@sda21(r0)
    fsubs    f1, f0, f1
    lfs      f0, lbl_8065EB08@sda21(r0)
    fmuls    f1, f2, f1
    fmuls    f1, f3, f1
    fdivs    f27, f1, f0
    bl       GXGetTexObjHeight
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x9a4(r1)
    lis      r3, gGame@ha
    lfs      f3, lbl_8065EB0C@sda21(r0)
    fmr      f4, f27
    stw      r0, 0x9a0(r1)
    addi     r3, r3, gGame@l
    lfd      f1, lbl_8065EB18@sda21(r0)
    fmr      f6, f3
    lfd      f0, 0x9a0(r1)
    lfs      f9, lbl_8065EB28@sda21(r0)
    fmr      f7, f3
    fsubs    f1, f0, f1
    lfs      f2, lbl_8065EB20@sda21(r0)
    lfs      f5, lbl_8065EB08@sda21(r0)
    fmr      f8, f3
    fmuls    f0, f27, f2
    fmuls    f9, f9, f1
    mr       r5, r18
    mr       r6, r18
    fadds    f1, f30, f0
    mr       r7, r18
    fdivs    f5, f9, f5
    mr       r8, r18
    addi     r4, r1, 0x39c
    fneg     f0, f5
    fmuls    f0, f0, f2
    fadds    f2, f31, f0
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r29)
    li       r4, 0x4
    bl       SetGsTevMode
    lis      r3, __vt__7CTexObj@ha
    lwz      r4, 0x3d4c(r29)
    addi     r0, r3, __vt__7CTexObj@l
    lfs      f28, 0x8(r1)
    stw      r0, 0x498(r1)
    mr       r3, r28
    lfs      f27, 0xc(r1)
    li       r5, 0x0
    stw      r0, 0x398(r1)
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x508
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    addi     r5, r1, 0x2d4
    addi     r4, r3, 0x38
    stw      r7, 0x29c(r1)
    stw      r6, 0x2a0(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x2a4(r1)
    stw      r6, 0x2a8(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x2ac(r1)
    stw      r6, 0x2b0(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x2b4(r1)
    stw      r6, 0x2b8(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x2bc(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x2c0(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x2c4(r1)
    stw      r6, 0x2c8(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x2cc(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x2d0(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x2d4(r1)
    mtctr    r0
.L_80144B08:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_80144B08
    lwz      r3, 0x294(r29)
    li       r4, 0x0
    bl       SetGsTevMode
    addi     r3, r1, 0x29c
    bl       GXGetTexObjWidth
    mr       r16, r3
    addi     r3, r1, 0x29c
    bl       GXGetTexObjHeight
    lis      r4, 0x4330
    clrlwi   r0, r16, 16
    stw      r0, 0x9b4(r1)
    clrlwi   r0, r3, 16
    lfs      f3, lbl_8065EB0C@sda21(r0)
    lis      r3, gGame@ha
    stw      r4, 0x9b0(r1)
    addi     r3, r3, gGame@l
    lfd      f4, lbl_8065EB18@sda21(r0)
    fmr      f6, f3
    lfd      f0, 0x9b0(r1)
    fmr      f7, f3
    stw      r4, 0x9b8(r1)
    fmr      f8, f3
    fsubs    f1, f0, f4
    stw      r0, 0x9bc(r1)
    mr       r5, r19
    lfs      f2, lbl_8065EB10@sda21(r0)
    mr       r6, r19
    lfd      f0, 0x9b8(r1)
    fmuls    f2, f2, f1
    lfs      f9, lbl_8065EB28@sda21(r0)
    fsubs    f0, f0, f4
    lfs      f5, lbl_8065EB08@sda21(r0)
    lfs      f1, lbl_8065EB3C@sda21(r0)
    mr       r7, r19
    fmuls    f4, f9, f2
    lfs      f10, lbl_8065EB40@sda21(r0)
    fmuls    f0, f9, f0
    mr       r8, r19
    fadds    f1, f27, f1
    addi     r4, r1, 0x29c
    fdivs    f4, f4, f5
    fdivs    f5, f0, f5
    fadds    f2, f28, f10
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r29)
    li       r4, 0x4
    bl       SetGsTevMode
    lis      r3, __vt__7CTexObj@ha
    addi     r0, r3, __vt__7CTexObj@l
    stw      r0, 0x398(r1)
    b        .L_80144D90
.L_80144BE8:
    lis      r3, __vt__7CTexObj@ha
    lwz      r4, 0x3d4c(r29)
    addi     r0, r3, __vt__7CTexObj@l
    lfs      f31, 0x8(r1)
    stw      r0, 0x298(r1)
    mr       r3, r28
    lfs      f30, 0xc(r1)
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x518
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    addi     r5, r1, 0x1d4
    addi     r4, r3, 0x38
    stw      r7, 0x19c(r1)
    stw      r6, 0x1a0(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x1a4(r1)
    stw      r6, 0x1a8(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x1ac(r1)
    stw      r6, 0x1b0(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x1b4(r1)
    stw      r6, 0x1b8(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x1bc(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x1c0(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x1c4(r1)
    stw      r6, 0x1c8(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x1cc(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x1d0(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x1d4(r1)
    mtctr    r0
.L_80144C9C:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_80144C9C
    lwz      r3, 0x294(r29)
    li       r4, 0x0
    bl       SetGsTevMode
    addi     r3, r1, 0x19c
    bl       GXGetTexObjWidth
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x9bc(r1)
    addi     r3, r1, 0x19c
    lfd      f1, lbl_8065EB18@sda21(r0)
    stw      r0, 0x9b8(r1)
    lfs      f2, lbl_8065EB10@sda21(r0)
    lfd      f0, 0x9b8(r1)
    lfs      f3, lbl_8065EB28@sda21(r0)
    fsubs    f1, f0, f1
    lfs      f0, lbl_8065EB08@sda21(r0)
    fmuls    f1, f2, f1
    fmuls    f1, f3, f1
    fdivs    f27, f1, f0
    bl       GXGetTexObjHeight
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x9b4(r1)
    lis      r3, gGame@ha
    lfs      f3, lbl_8065EB0C@sda21(r0)
    fmr      f4, f27
    stw      r0, 0x9b0(r1)
    addi     r3, r3, gGame@l
    lfd      f1, lbl_8065EB18@sda21(r0)
    fmr      f6, f3
    lfd      f0, 0x9b0(r1)
    lfs      f9, lbl_8065EB28@sda21(r0)
    fmr      f7, f3
    fsubs    f1, f0, f1
    lfs      f2, lbl_8065EB20@sda21(r0)
    lfs      f5, lbl_8065EB08@sda21(r0)
    fmr      f8, f3
    fmuls    f0, f27, f2
    fmuls    f9, f9, f1
    mr       r5, r30
    mr       r6, r30
    fadds    f1, f30, f0
    mr       r7, r30
    fdivs    f5, f9, f5
    mr       r8, r30
    addi     r4, r1, 0x19c
    fneg     f0, f5
    fmuls    f0, f0, f2
    fadds    f2, f31, f0
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r29)
    li       r4, 0x4
    bl       SetGsTevMode
    lis      r3, __vt__7CTexObj@ha
    addi     r0, r3, __vt__7CTexObj@l
    stw      r0, 0x298(r1)
.L_80144D90:
    addi     r20, r20, 0x1
    cmpwi    r20, 0xc
    blt      .L_801447FC
    lis      r3, gGame@ha
    lwz      r4, 0xe8(r31)
    addi     r3, r3, gGame@l
    lwz      r6, 0xec(r31)
    lwz      r0, 0xf0(r31)
    addis    r16, r3, 0x1
    stw      r4, 0x18(r1)
    addi     r4, r1, 0x18
    lwz      r3, -0x6944(r16)
    addi     r5, r1, 0xc
    stw      r6, 0x1c(r1)
    addi     r6, r1, 0x8
    li       r7, 0x0
    li       r8, 0x0
    stw      r0, 0x20(r1)
    lwz      r12, 0x34(r3)
    lwz      r12, 0x10c(r12)
    mtctr    r12
    bctrl
    lis      r3, __vt__7CTexObj@ha
    lwz      r4, 0x3d4c(r29)
    addi     r0, r3, __vt__7CTexObj@l
    lfs      f31, 0x8(r1)
    stw      r0, 0x198(r1)
    mr       r3, r28
    lfs      f30, 0xc(r1)
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x520
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    addi     r5, r1, 0xd4
    addi     r4, r3, 0x38
    stw      r7, 0x9c(r1)
    stw      r6, 0xa0(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0xa4(r1)
    stw      r6, 0xa8(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0xac(r1)
    stw      r6, 0xb0(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0xb4(r1)
    stw      r6, 0xb8(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0xbc(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0xc0(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0xc4(r1)
    stw      r6, 0xc8(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0xcc(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0xd0(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0xd4(r1)
    mtctr    r0
.L_80144E9C:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_80144E9C
    lwz      r3, 0x294(r29)
    li       r4, 0x0
    bl       SetGsTevMode
    addi     r3, r1, 0x9c
    bl       GXGetTexObjWidth
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x9bc(r1)
    addi     r3, r1, 0x9c
    lfd      f1, lbl_8065EB18@sda21(r0)
    stw      r0, 0x9b8(r1)
    lfs      f2, lbl_8065EB10@sda21(r0)
    lfd      f0, 0x9b8(r1)
    lfs      f3, lbl_8065EB28@sda21(r0)
    fsubs    f1, f0, f1
    lfs      f0, lbl_8065EB08@sda21(r0)
    fmuls    f1, f2, f1
    fmuls    f1, f3, f1
    fdivs    f27, f1, f0
    bl       GXGetTexObjHeight
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x9b4(r1)
    lis      r3, gGame@ha
    lfs      f3, lbl_8065EB0C@sda21(r0)
    fmr      f4, f27
    stw      r0, 0x9b0(r1)
    addi     r3, r3, gGame@l
    lfd      f1, lbl_8065EB18@sda21(r0)
    fmr      f6, f3
    lfd      f0, 0x9b0(r1)
    lfs      f9, lbl_8065EB28@sda21(r0)
    fmr      f7, f3
    fsubs    f1, f0, f1
    lfs      f2, lbl_8065EB20@sda21(r0)
    lfs      f5, lbl_8065EB08@sda21(r0)
    fmr      f8, f3
    fmuls    f0, f27, f2
    fmuls    f9, f9, f1
    mr       r5, r30
    mr       r6, r30
    fadds    f1, f30, f0
    mr       r7, r30
    fdivs    f5, f9, f5
    mr       r8, r30
    addi     r4, r1, 0x9c
    fneg     f0, f5
    fmuls    f0, f0, f2
    fadds    f2, f31, f0
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r29)
    li       r4, 0x4
    bl       SetGsTevMode
    lwz      r0, 0x3444(r25)
    lis      r3, __vt__7CTexObj@ha
    addi     r3, r3, __vt__7CTexObj@l
    cmpwi    r0, 0xb
    stw      r3, 0x198(r1)
    beq      .L_80144FC0
    bge      .L_80144FEC
    cmpwi    r0, 0x3
    beq      .L_80144FAC
    b        .L_80144FEC
.L_80144FAC:
    lwz      r3, -0x6944(r16)
    lfs      f1, lbl_8065EB44@sda21(r0)
    lfs      f0, 0xfc(r3)
    fadds    f29, f1, f0
    b        .L_80144FFC
.L_80144FC0:
    lwz      r3, -0x6944(r16)
    lfs      f2, lbl_8065EB4C@sda21(r0)
    lfs      f1, 0xfc(r3)
    lfs      f0, lbl_8065EB44@sda21(r0)
    fadds    f1, f2, f1
    fcmpo    cr0, f1, f0
    ble      .L_80144FE4
    lfs      f29, lbl_8065EB48@sda21(r0)
    b        .L_80144FFC
.L_80144FE4:
    fmr      f29, f2
    b        .L_80144FFC
.L_80144FEC:
    lwz      r3, -0x6944(r16)
    lfs      f1, lbl_8065EB4C@sda21(r0)
    lfs      f0, 0xfc(r3)
    fadds    f29, f1, f0
.L_80144FFC:
    lis      r3, __vt__7CTexObj@ha
    lwz      r4, 0x3d4c(r29)
    addi     r0, r3, __vt__7CTexObj@l
    mr       r3, r28
    stw      r0, 0x998(r1)
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x500
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    addi     r5, r1, 0x8d4
    addi     r4, r3, 0x38
    stw      r7, 0x89c(r1)
    stw      r6, 0x8a0(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x8a4(r1)
    stw      r6, 0x8a8(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x8ac(r1)
    stw      r6, 0x8b0(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x8b4(r1)
    stw      r6, 0x8b8(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x8bc(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x8c0(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x8c4(r1)
    stw      r6, 0x8c8(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x8cc(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x8d0(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x8d4(r1)
    mtctr    r0
.L_801450A8:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_801450A8
    addi     r3, r1, 0x89c
    bl       GXGetTexObjWidth
    addi     r3, r1, 0x89c
    bl       GXGetTexObjHeight
    fmr      f1, f29
    bl       sin
    fmr      f1, f29
    bl       cos
    lwz      r4, sScrollOverlayFrameCount@sda21(r0)
    lis      r3, 0xb60b
    lis      r0, 0x4330
    lfd      f1, lbl_8065EB78@sda21(r0)
    mulli    r4, r4, 0x18
    addi     r3, r3, 0x60b7
    stw      r0, 0x9b8(r1)
    lfs      f2, lbl_8065EB24@sda21(r0)
    mulhw    r0, r3, r4
    add      r0, r0, r4
    srawi    r0, r0, 8
    srwi     r3, r0, 31
    add      r0, r0, r3
    mulli    r0, r0, 0x168
    subf     r0, r0, r4
    xoris    r0, r0, 0x8000
    stw      r0, 0x9bc(r1)
    lfd      f0, 0x9b8(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f2, f0
    bl       sin
    frsp     f27, f1
    lwz      r3, 0x294(r29)
    li       r4, 0x0
    bl       SetGsTevMode
    lwz      r4, 0x3d4c(r29)
    mr       r3, r28
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x530
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    addi     r5, r1, 0x8d4
    addi     r4, r3, 0x38
    stw      r7, 0x89c(r1)
    stw      r6, 0x8a0(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x8a4(r1)
    stw      r6, 0x8a8(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x8ac(r1)
    stw      r6, 0x8b0(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x8b4(r1)
    stw      r6, 0x8b8(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x8bc(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x8c0(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x8c4(r1)
    stw      r6, 0x8c8(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x8cc(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x8d0(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x8d4(r1)
    mtctr    r0
.L_801451E0:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_801451E0
    addi     r3, r1, 0x89c
    bl       GXGetTexObjWidth
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x9b4(r1)
    addi     r3, r1, 0x89c
    lfd      f1, lbl_8065EB18@sda21(r0)
    stw      r0, 0x9b0(r1)
    lfs      f2, lbl_8065EB10@sda21(r0)
    lfd      f0, 0x9b0(r1)
    lfs      f3, lbl_8065EB08@sda21(r0)
    fsubs    f0, f0, f1
    fmuls    f0, f2, f0
    fmuls    f0, f3, f0
    fdivs    f28, f0, f3
    bl       GXGetTexObjHeight
    lfs      f3, lbl_8065EB50@sda21(r0)
    fmr      f1, f29
    lfs      f2, lbl_8065EB08@sda21(r0)
    mr       r16, r3
    fmuls    f0, f3, f27
    fdivs    f3, f28, f3
    fmuls    f0, f0, f2
    fdivs    f0, f0, f2
    fadds    f26, f3, f0
    bl       sin
    frsp     f2, f1
    lfs      f0, 0xc(r1)
    fmr      f1, f29
    fmadds   f27, f26, f2, f0
    bl       cos
    clrlwi   r3, r16, 16
    lis      r0, 0x4330
    stw      r3, 0x9ac(r1)
    frsp     f10, f1
    lfs      f3, lbl_8065EB0C@sda21(r0)
    lis      r3, gGame@ha
    stw      r0, 0x9a8(r1)
    fmr      f1, f27
    lfd      f2, lbl_8065EB18@sda21(r0)
    lfd      f0, 0x9a8(r1)
    fmr      f4, f28
    lfs      f8, lbl_8065EB08@sda21(r0)
    fmr      f6, f3
    fsubs    f2, f0, f2
    lfs      f9, 0x8(r1)
    lfs      f0, lbl_8065EB44@sda21(r0)
    fmr      f7, f3
    addi     r3, r3, gGame@l
    fmuls    f5, f8, f2
    fmadds   f2, f26, f10, f9
    addi     r4, r1, 0x89c
    li       r5, -0x1
    fdivs    f5, f5, f8
    li       r6, -0x1
    li       r7, -0x1
    li       r8, -0x1
    fadds    f8, f0, f29
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r29)
    li       r4, 0x4
    bl       SetGsTevMode
    lwz      r4, lbl_8065EAF8@sda21(r0)
    li       r3, 0xf
    lwz      r0, lbl_8065EAFC@sda21(r0)
    stw      r4, 0x10(r1)
    stw      r0, 0x14(r1)
    bl       GetLocalizedInterfaceText
    stw      r3, 0x14(r1)
    lfs      f1, lbl_8065EB4C@sda21(r0)
    bl       sin
    frsp     f2, f1
    lfs      f1, lbl_8065EB08@sda21(r0)
    lfs      f3, lbl_8065EB58@sda21(r0)
    li       r16, 0x0
    lfs      f0, lbl_8065EB54@sda21(r0)
    fsubs    f2, f1, f2
    lwz      r26, 0x14(r1)
    lfs      f1, lbl_8065EB04@sda21(r0)
    fmadds   f25, f3, f2, f0
    bl       __cvt_fp2unsigned
    lwz      r0, 0x10(r1)
    lis      r18, 0x4330
    lis      r4, gGame@ha
    lis      r7, gScrollCaptionOutlineY@ha
    xoris    r0, r0, 0x8000
    mr       r8, r3
    stw      r0, 0x9a4(r1)
    addi     r21, r4, gGame@l
    li       r0, -0x100
    lis      r6, gScrollCaptionOutlineX@ha
    stw      r18, 0x9a0(r1)
    lis      r5, __vt__7CTexObj@ha
    lfd      f27, lbl_8065EB78@sda21(r0)
    addi     r22, r5, __vt__7CTexObj@l
    lfd      f0, 0x9a0(r1)
    mr       r24, r8
    lfs      f1, lbl_8065EB5C@sda21(r0)
    mr       r23, r21
    fsubs    f0, f0, f27
    lfs      f28, lbl_8065EB68@sda21(r0)
    lfs      f31, lbl_8065EB64@sda21(r0)
    mr       r3, r21
    lfs      f30, lbl_8065EB00@sda21(r0)
    mr       r4, r26
    fmuls    f26, f1, f0
    lfs      f29, lbl_8065EB6C@sda21(r0)
    lfs      f1, lbl_8065EB60@sda21(r0)
    or       r27, r8, r0
    addi     r30, r1, 0x7d8
    addi     r17, r1, 0x808
    addi     r20, r7, gScrollCaptionOutlineY@l
    addi     r19, r6, gScrollCaptionOutlineX@l
    li       r5, 0x1
    bl       MeasureGameTextWidth
    xoris    r0, r16, 0x8000
    stw      r18, 0x9b8(r1)
    fsubs    f25, f25, f1
    li       r18, 0x0
    stw      r0, 0x9bc(r1)
    lfd      f0, 0x9b8(r1)
    fsubs    f0, f0, f27
    fmadds   f27, f0, f28, f31
.L_801453E0:
    lfs      f1, 0x0(r19)
    mr       r3, r21
    lfs      f0, 0x0(r20)
    mr       r4, r24
    lfs      f4, lbl_8065EB60@sda21(r0)
    fadds    f1, f25, f1
    fadds    f2, f27, f0
    lfs      f3, lbl_8065EB0C@sda21(r0)
    fmr      f5, f4
    mr       r5, r26
    crset    cr1eq
    bl       DrawGsProportionalTextFormat
    addi     r18, r18, 0x1
    addi     r19, r19, 0x4
    cmpwi    r18, 0x4
    addi     r20, r20, 0x4
    blt      .L_801453E0
    lfs      f4, lbl_8065EB60@sda21(r0)
    fmr      f1, f25
    fmr      f2, f27
    lfs      f3, lbl_8065EB0C@sda21(r0)
    fmr      f5, f4
    mr       r3, r21
    mr       r4, r27
    mr       r5, r26
    crset    cr1eq
    bl       DrawGsProportionalTextFormat
    lwz      r4, 0x3d4c(r29)
    mr       r3, r28
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x3b8
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    subi     r5, r30, 0x4
    addi     r4, r3, 0x38
    stw      r7, 0x79c(r1)
    stw      r6, 0x7a0(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x7a4(r1)
    stw      r6, 0x7a8(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x7ac(r1)
    stw      r6, 0x7b0(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x7b4(r1)
    stw      r6, 0x7b8(r1)
    stw      r22, 0x898(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x7bc(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x7c0(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x7c4(r1)
    stw      r6, 0x7c8(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x7cc(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x7d0(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x7d4(r1)
    mtctr    r0
.L_801454F0:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_801454F0
    lfs      f2, lbl_8065EB08@sda21(r0)
    mr       r3, r17
    lfs      f1, lbl_8065EB5C@sda21(r0)
    fmr      f3, f2
    bl       PSMTXScale
    lfs      f2, lbl_8065EB0C@sda21(r0)
    li       r0, 0x1
    fmr      f1, f26
    stw      r0, 0x7d4(r1)
    fmr      f3, f2
    mr       r3, r30
    bl       PSMTXTrans
    li       r0, 0x1
    lwz      r3, 0x294(r29)
    stw      r0, 0x7d4(r1)
    li       r4, 0x0
    bl       SetGsTevMode
    lfs      f3, lbl_8065EB0C@sda21(r0)
    fsubs    f1, f25, f30
    lfs      f4, lbl_8065EB68@sda21(r0)
    fadds    f2, f27, f29
    fmr      f6, f3
    mr       r3, r23
    fmr      f5, f4
    fmr      f7, f3
    mr       r5, r27
    fmr      f8, f3
    mr       r6, r27
    mr       r7, r27
    mr       r8, r27
    addi     r4, r1, 0x79c
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r29)
    li       r4, 0x4
    bl       SetGsTevMode
    lwz      r0, 0x3444(r25)
    stw      r22, 0x898(r1)
    cmpwi    r0, 0x0
    blt      .L_801455EC
    cmpwi    r0, 0xa
    bge      .L_801455EC
    lis      r3, gEventVariables@ha
    slwi     r5, r0, 2
    addi     r0, r3, gEventVariables@l
    addi     r3, r31, 0x48
    add      r4, r0, r5
    lwzx     r3, r3, r5
    lwz      r0, 0x180c(r4)
    cmpwi    r3, 0x0
    mulli    r0, r0, 0x64
    add      r4, r3, r0
    ble      .L_801455EC
    lfs      f1, lbl_8065EB70@sda21(r0)
    li       r3, 0xab
    lfs      f2, lbl_8065EB74@sda21(r0)
    li       r5, 0x0
    lfs      f3, lbl_8065EB08@sda21(r0)
    bl       fn_80050FE8
.L_801455EC:
    lis      r3, __vt__7CTexObj@ha
    addi     r0, r3, __vt__7CTexObj@l
    stw      r0, 0x998(r1)
    psq_l    f31, -0x598(r1), 0, qr0
    lfd      f31, 0xa60(r1)
    psq_l    f30, -0x5a8(r1), 0, qr0
    lfd      f30, 0xa50(r1)
    psq_l    f29, -0x5b8(r1), 0, qr0
    lfd      f29, 0xa40(r1)
    psq_l    f28, -0x5c8(r1), 0, qr0
    lfd      f28, 0xa30(r1)
    psq_l    f27, -0x5d8(r1), 0, qr0
    lfd      f27, 0xa20(r1)
    psq_l    f26, -0x5e8(r1), 0, qr0
    lfd      f26, 0xa10(r1)
    psq_l    f25, -0x5f8(r1), 0, qr0
    lfd      f25, 0xa00(r1)
    lmw      r16, 0x9c0(r1)
    lwz      r0, 0xa74(r1)
    mtlr     r0
    addi     r1, r1, 0xa70
    blr
*/
