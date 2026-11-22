#include "game/ccameraaction.h"
extern "C" {
#include "Dolphin/mtx.h"
}
#include "game/caminfo.h"
#include "game/scrollcameravisibility.h"
#include "game/scrollroomdata.h"
#include "game/ctpl.h"
#include "game/gmath.h"
#include "Version.h"
#include <math.h>
#include <string.h>
#include "game/globals.h"
#include "game/stagedata.h"
#include "game/evt.h"
#include "game/wii/cgame.h"
#include "game/wii/title.h"
#include "game/wii/cgs.h"
#include "game/cscrn.h"
#include "game/cmotioncontrolcamera.h"
#include "game/wii/cpadact.h"
#include "JSystem/JMath/Inline.h"

// the Wii build inlines the five JMathInlineVEC helpers as paired-single code;
// the longer SDK calls (PSVECNormalize, PSVECDistance) stay out of line
#define PSVECAdd        JMathInlineVEC::PSVECAdd
#define PSVECSubtract   JMathInlineVEC::PSVECSubtract
#define PSVECScale      JMathInlineVEC::PSVECScale
#define PSVECDotProduct JMathInlineVEC::PSVECDotProduct
#define PSVECSquareMag  JMathInlineVEC::PSVECSquareMag

#define ROOM_LIMITS gScrollCameraLimitTables
#define PLAYER      gGame.GetPlayer()
#define MARK_SCALE  0.65f

// how far each stage's room camera can pan, at its widest and its narrowest fov
static Scroll2Limits sScrollLimitsStage01 = { 0.0f, 0.0f, 0.0f, 0.0f, -252.42f, 235.19f, -112.42f, 138.56f, 60.0f, 15.0f, 33.0f };
static Scroll2Limits sScrollLimitsStage02 = { 0.0f, 0.0f, 0.0f, 0.0f, -174.25f, 174.25f, -316.0f, 260.0f, 60.0f, 15.0f, 60.0f };
static Scroll2Limits sScrollLimitsStage07 = { 0.0f, 0.0f, 0.0f, 0.0f, -214.98f, 214.98f, -293.72f, 293.72f, 60.0f, 15.0f, 60.0f };
static Scroll2Limits sScrollLimitsStage03 = { 0.0f, 0.0f, 0.0f, 0.0f, -125.0f, 125.0f, -110.0f, 110.0f, 60.0f, 15.0f, 60.0f };
static Scroll2Limits sScrollLimitsStage04 = { 0.0f, 0.0f, 0.0f, 0.0f, -200.0f, 200.0f, -200.0f, 200.0f, 60.0f, 15.0f, 60.0f };
static Scroll2Limits sScrollLimitsStage06 = { 0.0f, 0.0f, 0.0f, 0.0f, -200.0f, 200.0f, -200.0f, 200.0f, 60.0f, 15.0f, 33.0f };
static Scroll2Limits sScrollLimitsStage09 = { 0.0f, 0.0f, 0.0f, 0.0f, -130.43f, 149.38f, -268.49f, 439.07f, 60.0f, 15.0f, 60.0f };
static Scroll2Limits sScrollLimitsStage11 = { 0.0f, 0.0f, 0.0f, 0.0f, -160.0f, 160.0f, -80.0f, 80.0f, 40.0f, 15.0f, 27.07f };

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

extern "C" {
const char* GetStageName(s32 stage);
u16 GXGetTexObjWidth(const GXTexObj* obj);
void PSMTXTrans(Mtx m, f32 x, f32 y, f32 z);
u16 GXGetTexObjHeight(const GXTexObj* obj);
void DrawTexturedWindowFrame(u32 color, s32 a, f32 x, f32 y, f32 w, f32 h, f32 sx, f32 sy, f32 sz);
void PSMTXRotRad(Mtx m, char axis, f32 rad);
extern s32 gPointerScreenWidth, gPointerScreenHeight;
extern f32 gFrameRateScale;
void DrawSeparatedStatusValue(s32 icon, s32 value, bool selected, f32 x, f32 y, f32 alpha);
extern s32 gScreenSafeLeft;
extern s32 gScreenSafeBottom;
extern s32 gScreenSafeHeight;
}

// mSavedInfo while SaveCamInfo holds a camera info, NULL otherwise
void* gSavedScrollCameraInfo;

// the objects hidden while this camera runs and what they were before
s32 gScrollCameraHiddenObjectIds[256];
s32 gScrollCameraHiddenObjectStates[256];

static const s32 sRoomBaseCounts[10] = { 0, 9, 11, 4, 7, 0, 12, 9, 0, 8 };

// the drag frame's accumulated impulse, written on press and while dragging
static Vec sDragImpulse;
static f32 sDragImpulseW;
// the damping Update drops the drag vec to once the button is released
static f32 sDragDampingReleased = 0.7f;
// the screen-space scale of a drag step, x and y
static f32 sDragScaleX = 0.02f;
static f32 sDragScaleY = 0.02f;
// the damping while the drag is held
static f32 sDragDampingHeld = 0.7f;
// the pan speed multiplier the drag vec is scaled by
static f32 sDragPanSpeed = 1.03f;

/**
 * @note Address: 0x80161FFC
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
 * @note Address: 0x80162034
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
 * @note Address: 0x801620E0
 * @note Size: 0x134
 */
// NONMATCHING: the objects walk through WiiTitleObject on the Wii; the ids index
// gGame.mXObjs.mObjects and the drawn bit lives in a 64-bit mask at _150
void CCameraActionScroll2::HideObjs()
{
	s32* id    = gScrollCameraHiddenObjectIds;
	s32* state = gScrollCameraHiddenObjectStates;
	for (s32 i = 0; i < 256; i++) {
		if (*id == -1) {
			return;
		}
		WiiTitleObject* obj = (WiiTitleObject*)gGame.mXObjs.mObjects[*id];
		// 1: was drawn and is hidden now, 2: in use but not drawn, 0: missing or not in use
		if (obj != NULL && (obj->mStatus & 1)) {
			if (obj->mStatus & 0x10000) {
				*state = 1;
				obj->mStatus &= ~0x10000;
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
 * @note Address: 0x80162214
 * @note Size: 0xD0
 */
void CCameraActionScroll2::ShowObjs()
{
	s32* id    = gScrollCameraHiddenObjectIds;
	s32* state = gScrollCameraHiddenObjectStates;
	for (s32 i = 0; i < 256; i++) {
		if (*id == -1) {
			return;
		}
		// 0 and 2 were not drawn before HideObjs, they stay as they are
		if (*state == 0 || *state == 2) {
			id++;
			state++;
		} else {
			((WiiTitleObject*)gGame.mXObjs.mObjects[*id])->mStatus |= 0x10000;
			id++;
			state++;
		}
	}
}

static void InitQuads(CCameraActionScroll2* camera);
static void CheckQuads(CCameraActionScroll2* camera, u32 step, BOOL dragging, Vec* unused);
static void CalcLimits(CCameraActionScroll2* camera, Scroll2Limits* limits);

/**
 * @note Address: 0x801622E4
 * @note Size: 0x14
 */
void CCameraActionScroll2::Init()
{
	mWipe.Reset();
}

/**
 * @note Address: 0x801622F8
 * @note Size: 0x5E4
 */
// NONMATCHING: the Wii additions (quad tex init at _E0, mMtx at _120, the flag at _11C and the
// drag fields) are not fully decoded yet; the GameCube flow plus drag init follows
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

	mStart.x     = mInterest.x;
	mStart.y     = mInterest.y;
	mStart.z     = mInterest.z;
	CVec offset  = mEye - mInterest;
	mOffset.x    = offset.x;
	mOffset.y    = offset.y;
	mOffset.z    = offset.z;
	CVec center  = mInterest;
	mCenter.x    = center.x;
	mCenter.y    = center.y;
	mCenter.z    = center.z;
	mCenterPtr   = &mCenter;
	mDragging    = FALSE;
	mDragDamping = 0.85f;
	mDragVec.x   = 0.0f;
	mDragVec.y   = 0.0f;
	mDragVec.z   = 0.0f;
	CalcLimits(this, GetLimits(gGame.mStage));
	InitQuads(this);
	Update(1);
}

/**
 * @note Address: 0x801628DC
 * @note Size: 0x888
 */
// NONMATCHING: regalloc/scheduling; the drag math matches retail structurally
void CCameraActionScroll2::Update(u32 step)
{
	mWipe.Update(step);

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

	f32 zoom = gWiiPointer.mStick.y;
	if (fabs(zoom) != 0.0f) {
		zoom *= gFrameRateScale;
	}
	if (zoom != 0.0f) {
		f32 fov = mFov - 0.3f * zoom * step;
		if (fov < mMinFov) {
			fov = mMinFov;
		}
		if (fov > mMaxFov) {
			fov = mMaxFov;
		}
		SetFov(fov);
		CalcLimits(this, GetLimits(gGame.mStage));
	}

	CVec unused(0.0f, 0.0f, 0.0f);
	BOOL pressed = FALSE;
	if (gWiiPointer.mPointerValid) {
		BOOL released = FALSE;
		if (gWiiPointer.mButton & 0x10) {
			if (!mDragging) {
				mDragging = TRUE;
				pressed   = TRUE;
			}
		} else {
			if (mDragging) {
				mDragging = FALSE;
				released  = TRUE;
			}
		}
		if (pressed) {
			mDragStart.x   = gWiiPointer.mPointer.x;
			mDragStart.y   = gWiiPointer.mPointer.y;
			mDragStart.z   = gWiiPointer.mPointer.z;
			mDragLast.x    = gWiiPointer.mPointer.x;
			mDragLast.y    = gWiiPointer.mPointer.y;
			mDragLast.z    = gWiiPointer.mPointer.z;
			sDragImpulse.x = 0.0f;
			sDragImpulse.y = 0.0f;
			sDragImpulse.z = 0.0f;
			sDragImpulseW  = 0.0f;
		}
		if (released) {
			mDragDamping = sDragDampingReleased;
		}
		if (mDragging) {
			mDragDamping   = sDragDampingHeld;
			sDragImpulse.x = gWiiPointer.mPointer.x - mDragLast.x;
			sDragImpulse.y = gWiiPointer.mPointer.y - mDragLast.y;
			mDragVec.x += (gWiiPointer.mPointer.x - mDragStart.x) * sDragScaleX * -1.0f;
			mDragVec.y += (gWiiPointer.mPointer.y - mDragLast.y) * sDragScaleY * -1.0f;
			mDragLast.x = gWiiPointer.mPointer.x;
			mDragLast.y = gWiiPointer.mPointer.y;
			mDragLast.z = gWiiPointer.mPointer.z;
		}
	} else {
		mDragging = FALSE;
	}

	PSVECScale(&mDragVec, &mDragVec, mDragDamping);
	f32 dragX = 0.0f;
	f32 dragY = 0.0f;
	if (mDragVec.x != 0.0f) {
		dragX = mDragVec.x;
	}
	if (mDragVec.y != 0.0f) {
		dragY = mDragVec.y;
	}
	CheckQuads(this, step, gWiiPointer.mPointerValid && mDragging, &unused);
	if (dragX != 0.0f || dragY != 0.0f) {
		CVec eye(mEye);
		CVec toCenter(mCenter);
		PSVECSubtract(&toCenter, &eye, &toCenter);
		{
			CVec start(mEye); // never read, retail builds it here
			PSVECNormalize(&toCenter, &toCenter);

			CVec move(-dragX, 0.0f, dragY);
			Mtx rot;
			PSMTXRotRad(rot, 'y', atan2(mInterest.x - mEye.x, mInterest.z - mEye.z));
			PSMTXMultVec(rot, &move, &move);
			f32 min   = (f32)sin(0.2617993950843811);
			f32 max   = (f32)sin(1.0471975803375244);
			f32 blend = ((f32)sin(DEG2RAD * mFov) - min) / (max - min);
			f32 speed = 2.2f * (min + blend * (max - min)) * sDragPanSpeed;
			PSVECScale(&move, &move, speed);
			PSVECScale(&toCenter, &toCenter, 50.0f);
			PSVECAdd(&toCenter, &move, &toCenter);
			PSVECNormalize(&toCenter, &toCenter);
			PSVECScale(&toCenter, &toCenter, mDistance);
		}

		CVec next = toCenter + eye;
		if (gSavedScrollCameraInfo != NULL) {
			next.y = mInterest.y;
		} else {
			next.y = 0.0f;
		}
		mCenterPtr->x = next.x;
		mCenterPtr->y = next.y;
		mCenterPtr->z = next.z;
	}

	{
		CVec center(*mCenterPtr);
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
		mCenterPtr->x = center.x;
		mCenterPtr->y = center.y;
		mCenterPtr->z = center.z;
	}

	SetInterest(*mCenterPtr);
	SetEye(mOffset.x + mCenterPtr->x, mOffset.y + mCenterPtr->y, mOffset.z + mCenterPtr->z);

	if (mTurned) {
		Vec eye(mEye);
		Mtx turn;
		PSMTXRotRad(turn, 'x', HALF_PI);
		PSMTXMultVec(turn, &eye, &eye);
		SetEye(eye);
		Vec interest(mInterest);
		PSMTXMultVec(turn, &interest, &interest);
		SetInterest(interest);
	}
}

/**
 * @note Address: 0x80163164
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
 * @note Address: 0x8016319C
 * @note Size: 0xB4
 */
// NONMATCHING: out of line on the Wii; retail folds the fov blend differently
static void CalcLimits(CCameraActionScroll2* camera, Scroll2Limits* limits)
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
 * @note Address: 0x80163250
 * @note Size: 0xD08
 */
// NONMATCHING: the object and status passes are verified in retail; our object loops recompute the blink values,
// and register allocation and scheduling differ
void CCameraActionScroll2::Draw()
{
	static s32 sOverlayFrameCount = 0;

	f32 x;
	f32 y;

	sOverlayFrameCount++;
	f32 blink     = ABS(std::sinf(DEG2RAD * ((sOverlayFrameCount * 4) % 360)));
	f32 fastBlink = ABS(std::sinf(DEG2RAD * ((sOverlayFrameCount * 7) % 360)));

	Vec pos = { 0.0f, 0.0f, 0.0f };
	for (s32 i = 0; i < gGame.mXObjs.mPools[10].mUsed; i++) {
		WiiTitleObject* obj = (WiiTitleObject*)(gGame.mXObjs.mPools[10].mObjects + i * gGame.mXObjs.mPools[10].mStride);
		if (!(obj->mStatus & 0x10000)) {
			continue;
		}

		switch (*(s32*)obj->mSpawnData) {
		case 0x10E: {
			Vec ofs = { 0.0f, 0.0f, 0.0f };
			obj->vf1D4(&ofs, &x, &y, 0, 0);
			DrawMapMarker(163, 0x000000FF, x, y, blink, MARK_SCALE);
			break;
		}
		case 0x9D: {
			Vec ofs = { 0.0f, 0.0f, 0.0f };
			obj->vf1D4(&ofs, &x, &y, 0, 0);
			DrawMapMarker(166, 0x0000C8FF, x, y, blink, MARK_SCALE);
			break;
		}
		case 0x115:
			if (obj->mStatus & 0x10000) {
				Vec ofs = { 0.0f, 0.0f, 0.0f };
				obj->vf1D4(&ofs, &x, &y, 0, 0);
				DrawMapMarker(164, 0x0000C8FF, x, y, blink, MARK_SCALE);
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
			switch (*(s32*)obj->mSpawnData) {
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
			if (*(s32*)obj->mSpawnData == 0x20) {
				Vec far = { -180.0f, 0.0f, -380.0f };
				pos     = far;
				// leaves the kind switch: this door gets no name when Reset found a saved camera info
				if (!mFromStage) {
					break;
				}
			}

			Vec at = { 0.0f, 0.0f, 0.0f };
			at     = pos;
			obj->vf1D4(&at, &x, &y, 0, 0);
			obj->vf1D8(); // the GameCube's GetFloorXZExtent
			obj->vf1DC(); // the GameCube's GetCollisionHeight

			s32 stage = 0;
			switch (gGame.mStage) {
			case 1:
				if (*(s32*)obj->mSpawnData == 0xC) {
					stage = 7;
				} else {
					stage = 2;
				}
				// no break: room 1 runs room 2's door checks too, and they can override the stage above
			case 2:
				switch (*(s32*)obj->mSpawnData) {
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
				if (*(s32*)obj->mSpawnData == 0x75) {
					stage = 2;
				} else if (*(s32*)obj->mSpawnData == 0x1B4) {
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
			// 135 degrees, the results go unused
			sin(2.356194496154785);
			cos(2.356194496154785);
			u32 cells = strlen(name) / 2 + 1;
			if (cells < 4) {
				cells = 4;
			}
			DrawTexturedWindowFrame(0x141414A0, -1, (cells - 1) * 18.0f * 0.5f + nameX, nameY - 18.0f, 18.0f * (cells - 3), 18.0f, 1.0f,
			                        1.0f, 1.0f);
			DrawGsProportionalTextFormat(&gGame.mGs, -1, name, nameX, nameY - 18.0f, 0.0f, 18.0f, 18.0f);
			break;
		}
		}
	}

	for (s32 i = 0; i < gGame.mXObjs.mPools[10].mUsed; i++) {
		WiiTitleObject* obj = (WiiTitleObject*)(gGame.mXObjs.mPools[10].mObjects + i * gGame.mXObjs.mPools[10].mStride);
		switch (*(s32*)obj->mSpawnData) {
		case 0xD3:
		case 0x115:
		case 0x116:
			break;
		default:
			continue;
		}
		if (!(obj->mStatus & 0x10000)) {
			continue;
		}

		Vec ofs = { 0.0f, 0.0f, 0.0f };
		obj->vf1D4(&ofs, &x, &y, 0, 0);
		if (((u8*)obj)[0xA54]) { // _A54, the leticker's running particle flag
			DrawMapMarker(164, 0x0000C8FF, x, y, fastBlink, MARK_SCALE);
			DrawMapLabel(x, y, 1.0f - fastBlink, MARK_SCALE);
		} else {
			DrawMapMarker(164, 0x0000C8FF, x, y, blink, MARK_SCALE);
		}
	}

	{
		Vec ofs = { 0.0f, 0.0f, 0.0f };
		((WiiTitleObject*)gGame.GetPlayer())->vf1D4(&ofs, &x, &y, 0, 0);
		DrawMapMarker(165, 0xC80000FF, x, y, blink, MARK_SCALE);
	}

	// the arrow round the player points the way it faces
	f32 angle;
	switch (gGame.mStage) {
	case 3:
		angle = PI + PLAYER->mMoveRotate.y;
		break;
	case 11:
		// the turned camera's room: the arrow only points left or right of the player
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

	f32 pulse = std::sinf(DEG2RAD * ((sOverlayFrameCount * 24) % 360));
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	gGame.mTpl.mTex.Init(&(&gGame.mTpl)->mPalette->descriptorArray[166 - StatusTextureOffset], 0);
	CTexObj tex = gGame.mTpl.mTex;
	f32 zoom    = 1.0f;
	f32 arrowW  = zoom * (1.1f * GXGetTexObjWidth(&tex)) / zoom;
	u16 arrowH  = GXGetTexObjHeight(&tex);
	f32 radius  = arrowW / 3.0f + 3.0f * pulse * zoom / zoom;
	f32 arrowX  = radius * std::sinf(angle) + x;
	DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, arrowX, radius * std::cosf(angle) + y, 0.0f, arrowW, zoom * arrowH / zoom, 0.0f,
	                   0.0f, PI + angle);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);

	// the wipe sits over the room view, the edge quads round the drag frame
	DrawQuads();
	mWipe.Draw(&gGame.mGs, 0);
	gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	if (gGame.mStage >= 0 && gGame.mStage < 10) {
		s32 base  = sRoomBaseCounts[gGame.mStage];
		s32 count = gEventVars.mVars[0x603 + gGame.mStage];
		s32 value = base + count * 100;
		if (base > 0) {
			DrawSeparatedStatusValue(0xAC, value, false, gScreenSafeLeft + 40, gScreenSafeBottom - 40, 1.0f);
		}
	}
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
}

// draws one blinking marker centred on a screen point; the colour the callers pass goes unused.
// retail converts the alpha once per corner colour, and both helpers stay calls in Draw
/**
 * @note Address: 0x80163F58
 * @note Size: 0x2A4
 */
// zoom is the same 1.0f scale Draw's player arrow multiplies and divides by (retail fmuls and fdivs by one
// register there, which only a variable gives); retail divides the sizes by it here too (fdivs at 0x801640DC)
void CCameraActionScroll2::DrawMapMarker(s32 index, u32 color, f32 x, f32 y, f32 alpha, f32 scale)
{
	CTexObj tex;
	f32 zoom = 1.0f;
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[index - StatusTextureOffset], 0);
	tex = gGame.mTpl.mTex;
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	f32 cx, cy;
	u16 texW = GXGetTexObjWidth(&tex);
	f32 w    = scale * (1.1f * texW) / zoom;
	u16 texH = GXGetTexObjHeight(&tex);
	f32 h    = scale * texH / zoom;
	cx       = w / 2.0f;
	cy       = -h / 2.0f;
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
	                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, x + cx, y + cy, 0.0f, w, h, 0.0f, 0.0f,
	                   0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

// the leticker's label, up and right of its marker
/**
 * @note Address: 0x801641FC
 * @note Size: 0x2B8
 */
// the sizes and offsets divided by the same zoom (fdivs at 0x80164384)
void CCameraActionScroll2::DrawMapLabel(f32 x, f32 y, f32 alpha, f32 scale)
{
	CTexObj tex;
	f32 zoom = 1.0f;
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[162 - StatusTextureOffset], 0);
	tex = gGame.mTpl.mTex;
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	f32 ox, oy;
	u16 texW = GXGetTexObjWidth(&tex);
	f32 w    = scale * (1.1f * texW) / zoom;
	u16 texH = GXGetTexObjHeight(&tex);
	f32 h    = scale * texH / zoom;
	ox       = 36.0f * scale / zoom;
	oy       = -32.0f * scale / zoom;
	DrawGsTexturedQuad(&gGame.mGs, &tex, (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800,
	                   (u32)(255.0f * alpha) | 0xC8C8C800, (u32)(255.0f * alpha) | 0xC8C8C800, x + ox, y + oy, 0.0f, w, h, 0.0f, 0.0f,
	                   0.0f);
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

/**
 * @note Address: 0x801644B4
 * @note Size: 0x28
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

/**
 * @note Address: 0x801644DC
 * @note Size: 0x4
 */
void CCameraActionScroll2::DrawDebug3d()
{
}

/**
 * @note Address: 0x801644E0
 * @note Size: 0x4
 */
void CCameraActionScroll2::DrawDebug2d()
{
}

// one of the drag frame's edges: marks it blocked when the centre sits at its limit
/**
 * @note Address: 0x801644E4
 * @note Size: 0x294
 */
// NONMATCHING: index-table pick and limit margin guessed from retail
static void CheckQuads(CCameraActionScroll2* camera, u32 step, BOOL dragging, Vec* unused)
{
	static const s32 sEdgeOrder[3][4] = {
		{ 3, 2, 0, 1 },
		{ 1, 0, 3, 2 },
		{ 0, 1, 2, 3 },
	};
	if (!dragging) {
		for (s32 i = 0; i < 4; i++) {
			camera->mQuads[i].mActive = TRUE;
			camera->mQuads[i].mUnk24  = FALSE;
		}
		return;
	}
	CVec dir;
	dir.x = camera->mOffset.x;
	dir.y = camera->mOffset.y;
	dir.z = camera->mOffset.z;
	PSVECNormalize(&dir, &dir);
	const s32* order;
	if (dir.z == 0.0f && dir.x < 0.0f) {
		order = sEdgeOrder[0];
	} else if (dir.x == 0.0f && dir.z < 0.0f) {
		order = sEdgeOrder[1];
	} else if (dir.x == 0.0f && dir.z > 0.0f) {
		order = sEdgeOrder[2];
	} else {
		order = sEdgeOrder[0];
	}
	for (s32 i = 0; i < 4; i++) {
		camera->mQuads[i].mActive = TRUE;
		camera->mQuads[i].mUnk24  = TRUE;
	}
	if (camera->mCenter.x <= camera->mMinX + 0.5f) {
		camera->mQuads[order[0]].mUnk24 = FALSE;
	}
	if (camera->mCenter.x >= camera->mMaxX - 0.5f) {
		camera->mQuads[order[1]].mUnk24 = FALSE;
	}
	if (camera->mCenter.z <= camera->mMinZ + 0.5f) {
		camera->mQuads[order[2]].mUnk24 = FALSE;
	}
	if (camera->mCenter.z >= camera->mMaxZ - 0.5f) {
		camera->mQuads[order[3]].mUnk24 = FALSE;
	}
}

// sets the four edge quads round the screen centre; the widescreen flag picks their half offsets
/**
 * @note Address: 0x80164778
 * @note Size: 0x238
 */
// NONMATCHING: the field initialization and unrolled loop match; table setup scheduling and saved registers differ
static void InitQuads(CCameraActionScroll2* camera)
{
	s32 center          = gGame.mAspectRatio ? 0x26C : 0x232;
	s32 x               = center / 2 - 20;
	s32 y               = gScreenSafeHeight / 2 - 15;
	f32 positions[4][2] = {
		{ x, 0.0f },
		{ -x, 0.0f },
		{ 0.0f, y },
		{ 0.0f, -y },
	};
	const f32 rot[4]    = { 0.0f, PI, -HALF_PI, HALF_PI };
	const f32 dir[4][2] = {
		{ 0.0f, 1.0f },
		{ 0.0f, -1.0f },
		{ 1.0f, 0.0f },
		{ -1.0f, 0.0f },
	};
	for (s32 i = 0; i < 4; i++) {
		ScrollEdgeQuad* quad = &camera->mQuads[i];
		quad->mTex           = camera->_E0;
		quad->mActive        = FALSE;
		quad->mUnk24         = FALSE;
		quad->mX             = positions[i][0];
		quad->mY             = positions[i][1];
		quad->mUnk0C         = dir[i][0];
		quad->mUnk10         = dir[i][1];
		quad->mScale         = 1.0f;
		quad->mRot           = rot[i];
		quad->mUnk1C         = i;
	}
}

// draws the four drag-frame edges that are active
/**
 * @note Address: 0x801649B0
 * @note Size: 0x228
 */
// NONMATCHING: render-state pokes and the colour path guessed from retail
void CCameraActionScroll2::DrawQuads()
{
	gGame.mGs.mCurRenderState->mZMode.mEnable = GX_FALSE;
	gGame.mGs.mCurRenderState->mZMode.mFunc   = GX_ALWAYS;
	gGame.mGs.mCurRenderState->mZMode.mUpdate = GX_FALSE;
	GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
	GXSetAlphaUpdate(GX_FALSE);
	for (s32 i = 0; i < 4; i++) {
		ScrollEdgeQuad* quad = &mQuads[i];
		if (!quad->mActive) {
			continue;
		}
		f32 alpha = quad->mUnk24 ? 1.0f : 0.5f;
		u32 color = (u32)(255.0f * alpha) | 0xFFFFFF00;
		f32 w     = quad->mScale * GXGetTexObjWidth((GXTexObj*)quad->mTex);
		f32 h     = quad->mScale * GXGetTexObjHeight((GXTexObj*)quad->mTex);
		DrawGsTexturedQuad(&gGame.mGs, (CTexObj*)quad->mTex, color, color, color, color, gPointerScreenWidth * 0.5f + quad->mX,
		                   gPointerScreenHeight * 0.5f + quad->mY, 0.0f, w, h, 0.0f, 0.0f, quad->mRot);
	}
	// the GameCube code pokes the tev ops after the quad loop; on the Wii they're structs
	gGame.mGs.mCurTevStage->mColorOp.mOp = (GXTevOp)4;
	gGame.mGs.mCurTevStage->mAlphaOp.mOp = (GXTevOp)0;
}

/**
 * @note Address: 0x80164BD8
 * @note Size: 0x1C
 */
BOOL CCameraActionScroll2::IsFovNotMax()
{
	return mFov != mMaxFov;
}

/**
 * @note Address: 0x80164BF4
 * @note Size: 0x8
 */
u32 CCameraActionScroll2::GetCameraFlags()
{
	return CAMFLAG_EYE | CAMFLAG_INTEREST | CAMFLAG_FOV;
}
