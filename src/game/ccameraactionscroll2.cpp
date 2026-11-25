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

// how far each stage's room camera can pan, at its widest and its narrowest fov
static Scroll2Limits sScrollLimitsStage01 = { 0.0f, 0.0f, 0.0f, 0.0f, -252.42f, 235.19f, -112.42f, 138.56f, 60.0f, 15.0f, 33.0f };
static Scroll2Limits sScrollLimitsStage02 = { 0.0f, 0.0f, 0.0f, 0.0f, -174.25f, 174.25f, -316.0f, 260.0f, 60.0f, 15.0f, 60.0f };
static Scroll2Limits sScrollLimitsStage07 = { 0.0f, 0.0f, 0.0f, 0.0f, -214.98f, 214.98f, -293.72f, 293.72f, 60.0f, 15.0f, 60.0f };
static Scroll2Limits sScrollLimitsStage03 = { 0.0f, 0.0f, 0.0f, 0.0f, -125.0f, 125.0f, -110.0f, 110.0f, 60.0f, 15.0f, 60.0f };
static Scroll2Limits sScrollLimitsStage04 = { 0.0f, 0.0f, 0.0f, 0.0f, -200.0f, 200.0f, -200.0f, 200.0f, 60.0f, 15.0f, 60.0f };
static Scroll2Limits sScrollLimitsStage06 = { 0.0f, 0.0f, 0.0f, 0.0f, -200.0f, 200.0f, -200.0f, 200.0f, 60.0f, 15.0f, 33.0f };
static Scroll2Limits sScrollLimitsStage09 = { 0.0f, 0.0f, 0.0f, 0.0f, -130.43f, 149.38f, -268.49f, 439.07f, 60.0f, 15.0f, 60.0f };
static Scroll2Limits sScrollLimitsStage11 = { 0.0f, 0.0f, 0.0f, 0.0f, -160.0f, 160.0f, -80.0f, 80.0f, 40.0f, 15.0f, 27.07f };

#if defined(VERSION_PAL)
// europe keeps the limits, the caption outline and the marker positions in one record
ScrollRoomData gScrollRoomData = {
	{
	    NULL,
	    &sScrollLimitsStage01,
	    &sScrollLimitsStage02,
	    &sScrollLimitsStage03,
	    &sScrollLimitsStage04,
	    NULL,
	    &sScrollLimitsStage06,
	    &sScrollLimitsStage07,
	    NULL,
	    &sScrollLimitsStage09,
	    NULL,
	    &sScrollLimitsStage11,
	    NULL,
	    NULL,
	    &sScrollLimitsStage07,
	    NULL,
	    NULL,
	    NULL,
	    NULL,
	    NULL,
	},
	{ -1.0f, 1.0f, 1.0f, -1.0f },
	{ -1.0f, -1.0f, 1.0f, 1.0f },
	{ -1.0f, 1.0f, 1.0f, -1.0f },
	{ -1.0f, -1.0f, 1.0f, 1.0f },
};
#else
// the limits of each room's camera
Scroll2Limits* gScrollCameraLimitTables[20] = {
	NULL,
	&sScrollLimitsStage01,
	&sScrollLimitsStage02,
	&sScrollLimitsStage03,
	&sScrollLimitsStage04,
	NULL,
	&sScrollLimitsStage06,
	&sScrollLimitsStage07,
	NULL,
	&sScrollLimitsStage09,
	NULL,
	&sScrollLimitsStage11,
	NULL,
	NULL,
	&sScrollLimitsStage07,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

// the caption text's outline corner offsets
f32 gScrollCaptionOutlineX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 gScrollCaptionOutlineY[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
#endif

#define MARK_SCALE 0.65f

// the overlay caption: Japan's text table has it at 72, and its icon strip has 14 frames, not 17
#if defined(VERSION_JP)
#define CAPTION_TEXT       72
#define CAPTION_ICON_FRAME 0.071428575f
#else
#define CAPTION_TEXT       15
#define CAPTION_ICON_FRAME 0.05882353f
#endif

// the caption over the room camera: the frame of the icon strip it shows and its text
struct CaptionMsg {
	s32 mFrame;        // _00
	const char* mText; // _04
};

extern "C" {
const char* GetStageName(s32 stage);
void DrawTexturedWindowFrame(u32 color, u32 textureColor, f32 x, f32 y, f32 width, f32 height, f32 alpha, f32 scaleX, f32 scaleY);
}

// mSavedInfo while SaveCamInfo holds a camera info, NULL otherwise
u8* gSavedScrollCameraInfo;

// the objects hidden while this camera runs and what they were before
s32 gScrollCameraHiddenObjectIds[256];
s32 gScrollCameraHiddenObjectStates[256];

// draws one blinking marker centred on a screen point
static inline void DrawScrollMapMarker(s32 index, f32 x, f32 y, u32 color)
{
	CTexObj tex;
	f32 half = 0.5f;
	tex      = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[index - StatusTextureOffset], 0);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	f32 w  = MARK_SCALE * (1.1f * GXGetTexObjWidth(&tex));
	f32 h  = MARK_SCALE * GXGetTexObjHeight(&tex);
	f32 cx = w * half;
	f32 cy = -h * half;
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x + cx, y + cy, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
}

// the functions sit in reverse address order: -inline deferred emits a file's functions last to first
/**
 * @note Address: 0x801469A8
 * @note Size: 0x38
 */
BOOL CCameraActionScroll2::AddHideObj(s32 id)
{
	s32* slot = gScrollCameraHiddenObjectIds;
	for (s32 i = 0; i < 256; i++, slot++) {
		if (*slot == -1) {
			*slot = id;
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * @note Address: 0x801468FC
 * @note Size: 0xAC
 */
void CCameraActionScroll2::ClearHideList()
{
	for (int i = 0; i < 256; i++) {
		gScrollCameraHiddenObjectIds[i]    = -1;
		gScrollCameraHiddenObjectStates[i] = -1;
	}
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
		// 1: was drawn and is hidden now, 2: in use but not drawn, 0: missing or not in use
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
// NONMATCHING: regalloc
void CCameraActionScroll2::ShowObjs()
{
	s32* id    = gScrollCameraHiddenObjectIds;
	s32* state = gScrollCameraHiddenObjectStates;
	for (s32 i = 0; i < 256; i++) {
		if (*id == -1) {
			return;
		}
		// 0 and 2 were not drawn before HideObjs, they stay as they are
		if (*state != 0 && *state != 2) {
			gGame.mXObjs.mObjects[*id]->mStatus.mFlags |= 0x10000;
		}
		id++;
		state++;
	}
}

static inline void CalcLimits(CCameraActionScroll2* camera, const Scroll2Limits* limits)
{
	camera->mMaxFov = limits->mMaxFov;
	camera->mMinFov = limits->mMinFov;
	f32 xBlend      = (camera->mFov - camera->mMinFov) / (camera->mMaxFov - camera->mMinFov);
	// 0 keeps the narrow limits, 1 the wide ones; z is fully wide from mMidFov up
	f32 zBlend;
	if (camera->mFov > limits->mMidFov) {
		zBlend = 1.0f;
	} else {
		zBlend = (camera->mFov - camera->mMinFov) / (limits->mMidFov - camera->mMinFov);
	}
	camera->mMinX = xBlend * (limits->mWideMinX - limits->mNarrowMinX) + limits->mNarrowMinX;
	camera->mMaxX = xBlend * (limits->mWideMaxX - limits->mNarrowMaxX) + limits->mNarrowMaxX;
	camera->mMinZ = zBlend * (limits->mWideMinZ - limits->mNarrowMinZ) + limits->mNarrowMinZ;
	camera->mMaxZ = zBlend * (limits->mWideMaxZ - limits->mNarrowMaxZ) + limits->mNarrowMaxZ;
	camera->mMinX += camera->mStart.x;
	camera->mMaxX += camera->mStart.x;
	camera->mMinZ += camera->mStart.z;
	camera->mMaxZ += camera->mStart.z;
}

/**
 * @note Address: 0x80145FE8
 * @note Size: 0x750
 */
void CCameraActionScroll2::Reset()
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
	CVec front  = CVec(0.0f, 0.0f, 1.0f);
	mYaw        = atan2(mInterest.x - front.x, mInterest.z - front.z);
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
	// the stick works on the unturned camera: undo the quarter turn here, the end of Update turns it again
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
		CVec toCenter = Vec(mCenter);
		Vec origin    = eye;
		PSVECSubtract(&toCenter, &origin, &toCenter);
		PSVECNormalize(&toCenter, &toCenter);

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
		PSVECScale(&toCenter, &toCenter, 50.0f);
		Vec pan = move;
		PSVECAdd(&toCenter, &pan, &toCenter);
		PSVECNormalize(&toCenter, &toCenter);
		PSVECScale(&toCenter, &toCenter, mDistance);

		next = toCenter + eye;
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
	if (room < gGame.mRoomCount) {
		return ROOM_LIMITS[room];
	}
	return ROOM_LIMITS[7];
}

/**
 * @note Address: 0x80143A00
 * @note Size: 0x1C44
 */
void CCameraActionScroll2::Draw()
{
	static s32 sOverlayFrameCount = 0;

	f32 x;
	f32 y;

	sOverlayFrameCount++;
	f32 blink     = ABS((f32)sin(DEG2RAD * ((sOverlayFrameCount * 4) % 360)));
	f32 fastBlink = ABS((f32)sin(DEG2RAD * ((sOverlayFrameCount * 7) % 360)));

	u32 color = (u32)(255.0f * blink) | 0xC8C8C800;
	for (s32 i = 0; i < 288; i++) {
		XObj* obj = gGame.mXObjs.mPools[22].Get(i);
		if (!(obj->mStatus.mFlags & 0x10000)) {
			continue;
		}

		switch (obj->GetKind()) {
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
		case 0x115: {
			Vec ofs = { 0.0f, 0.0f, 0.0f };
			obj->ProjectOffsetToScreen(&ofs, &x, &y, NULL, 0);
			DrawScrollMapMarker(163, x, y, color);
			break;
		}
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
			Vec pos = { 0.0f, 0.0f, 0.0f };
			switch (obj->GetKind()) {
			case 0xD2:
			case 0x22C: {
				Vec alongZ = { 0.0f, 0.0f, -100.0f };
				pos        = alongZ;
				break;
			}
			case 0x70: {
				Vec high = { 10.0f, 170.0f, 12.0f };
				pos      = high;
				break;
			}
			}
			if (obj->GetKind() == 0x20) {
				Vec far = { -180.0f, 0.0f, -380.0f };
				pos     = far;
				// leaves the kind switch: this door gets no name when Reset found a saved camera info
				if (!mFromStage) {
					break;
				}
			}

			obj->ProjectOffsetToScreen(&pos, &x, &y, NULL, 0);

			s32 stage = 0;
			switch (gGame.mStage) {
			case 1:
				if (obj->GetKind() == 0xC) {
					stage = 7;
				} else {
					stage = 2;
				}
				// no break: room 1 runs room 2's door checks too, and they can override the stage above
			case 2:
				switch (obj->GetKind()) {
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
				if (obj->GetKind() == 0x75) {
					stage = 2;
				} else if (obj->GetKind() == 0x1B4) {
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
#if defined(VERSION_JP)
			u32 cells = strlen(name) / 2 + 1;
			if (cells < 4)
				cells = 4;
			DrawTexturedWindowFrame(0x141414A0, 0xFFFFFFFF, (cells - 1) * 18.0f * 0.5f + nameX, nameY - 18.0f, 18.0f * (cells - 3), 18.0f,
			                        1.0f, 1.0f, 1.0f);
#else
			f32 width = MeasureGameTextWidth(&gGame.mGs, name, 1, 18.0f) * 0.5f;
			DrawTexturedWindowFrame(0x141414A0, 0xFFFFFFFF, MeasureGameTextWidth(&gGame.mGs, name, 1, 18.0f) * 0.5f + nameX, nameY - 18.0f,
			                        width, 18.0f, 1.0f, 1.0f, 1.0f);
#endif
			DrawGsProportionalTextFormat(&gGame.mGs, -1, name, nameX, nameY - 18.0f, 0.0f, 18.0f, 18.0f);
			break;
		}
		}
	}

	// with its particle running, a leticker's marker and its label blink in turn
	u32 fastColor  = (u32)(255.0f * fastBlink) | 0xC8C8C800;
	u32 fadedAlpha = (u32)(255.0f * (1.0f - fastBlink));
#if defined(VERSION_PAL)
	u32 labelColor = fadedAlpha | 0xFFD52C00;
#else
	u32 labelColor = fadedAlpha | 0xC8C8C800;
#endif
	for (s32 i = 0; i < 12; i++) {
		XObjLeticker* obj = static_cast<XObjLeticker*>(gGame.mXObjs.mPools[9].Get(i));
		switch (obj->GetKind()) {
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
			DrawScrollMapMarker(163, x, y, fastColor);

			f32 markX = x;
			f32 markY = y;
#if defined(VERSION_PAL)
			for (s32 j = 0; j < 4; j++) {
				DrawGsProportionalTextFormat(&gGame.mGs, fadedAlpha, GetLocalizedInterfaceText(0x115), markX + gScrollRoomData.markerX[j],
				                             (-20.8f + (markY + gScrollRoomData.markerY[j])) - 11.0f, 0.0f, 22.0f, 22.0f);
			}
			DrawGsProportionalTextFormat(&gGame.mGs, labelColor, GetLocalizedInterfaceText(0x115), markX, (markY - 20.8f) - 11.0f, 0.0f,
			                             22.0f, 22.0f);
#else
			CTexObj tex;
			tex = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[161 - StatusTextureOffset], 0);
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			u16 w = GXGetTexObjWidth(&tex);
			u16 h = GXGetTexObjHeight(&tex);
			DrawGsTexturedQuad(&gGame.mGs, &tex, labelColor, labelColor, labelColor, labelColor, markX + 23.4f, markY + -20.8f, 0.0f,
			                   MARK_SCALE * (1.1f * w), MARK_SCALE * h, 0.0f, 0.0f, 0.0f);
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
		angle = PI + gGame.GetPlayer()->mMoveRotate.y;
		break;
	case 11:
		// the turned camera's room: the arrow only points left or right of the player
		if (HALF_PI + gGame.GetPlayer()->mMoveRotate.y > PI) {
			angle = -HALF_PI;
		} else {
			angle = HALF_PI;
		}
		break;
	default:
		angle = HALF_PI + gGame.GetPlayer()->mMoveRotate.y;
		break;
	}

	CTexObj tex;
	tex       = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[160 - StatusTextureOffset], 0);
	f32 pulse = sin(DEG2RAD * ((sOverlayFrameCount * 24) % 360));
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	tex        = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[166 - StatusTextureOffset], 0);
	f32 arrowW = 1.1f * GXGetTexObjWidth(&tex);
	u16 arrowH = GXGetTexObjHeight(&tex);
	f32 radius = arrowW / 3.0f + 3.0f * pulse;
	f32 arrowX = radius * (f32)sin(angle) + x;
	DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, arrowX, radius * (f32)cos(angle) + y, 0.0f, arrowW, arrowH, 0.0f, 0.0f,
	                   PI + angle);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);

	// the caption in the corner, with a dark outline
	CaptionMsg msg = { 7, NULL };
	msg.mText      = GetLocalizedInterfaceText(CAPTION_TEXT);
	f32 slide      = HALF_PI; // a slide in, always at its end here
	f32 captionX   = 100.0f * (1.0f - (f32)sin(slide)) + 576.0f;
	f32 fade       = 255.0f;
	u32 alpha      = (u32)fade;
	u32 white      = alpha | 0xFFFFFF00;
	f32 frame      = CAPTION_ICON_FRAME * msg.mFrame;
	captionX -= MeasureGameTextWidth(&gGame.mGs, msg.mText, 1, 20.0f);
	s32 line     = 0;
	f32 captionY = 24.0f * line + 62.0f;
	for (s32 i = 0; i < 4; i++) {
		DrawGsProportionalTextFormat(&gGame.mGs, alpha, msg.mText, captionX + CAPTION_OUTLINE_X[i], captionY + CAPTION_OUTLINE_Y[i], 0.0f,
		                             20.0f, 20.0f);
	}
	DrawGsProportionalTextFormat(&gGame.mGs, white, msg.mText, captionX, captionY, 0.0f, 20.0f, 20.0f);

	CTexObj icon;
	icon = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[119 - StatusTextureOffset], 0);
	PSMTXScale(icon.mMtx[1], CAPTION_ICON_FRAME, 1.0f, 1.0f);
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
		s32 count = gEventVars.mVars[0x603 + gGame.mStage];
		if (base > 0) {
			// shown as count/base: the hundreds go left of the slash, the low two digits right of it
			DrawSeparatedStatusValue(0xAB - StatusTextureOffset, base + count * 100, false, 56.0f, 378.0f, 1.0f);
		}
	}
}
/**
 * @note Address: 0x801439C0
 * @note Size: 0x40
 */
void CCameraActionScroll2::SaveCamInfo(const void* info)
{
	if (info == NULL) {
		gSavedScrollCameraInfo = NULL;
	} else {
		gSavedScrollCameraInfo = mSavedInfo;
		memmove(mSavedInfo, info, sizeof(mSavedInfo));
	}
}

u32 CCameraActionScroll2::GetCameraFlags()
{
	return CAMFLAG_EYE | CAMFLAG_INTEREST | CAMFLAG_FOV;
}

void CCameraActionScroll2::DrawDebug3d()
{
}
