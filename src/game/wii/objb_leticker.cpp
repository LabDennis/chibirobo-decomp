#include "game/wii/xobjleticker.h" // first: its AnimSfx needs the real nw4r::math
#include "game/wii/xobj.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include "game/ccameraaction.h"
#include "game/gmath.h"
#include "game/evt.h"
#include "arithmetic.h"
#include <math.h>

// the rising lifts and their camera action. The GameCube's objb_leticker.cpp carries
// the same code; the Wii keeps the member layout shifted 0x28 behind the bigger XObj
// and turns the camera with extra helpers living in the units above (fn_8016A81C,
// fn_8016AA78), so the pointer work in the action's Update goes through them

extern "C" f32 GetSoundVolumeAtPosition(XSound* sound, const Vec* pos, s32 which);
extern "C" void fn_800A12F8(void* effects, u32 id, BOOL retireParticles); // retires the event effect with that id
extern "C" void UpdateObjectFloorXZExtent(XObj* object);
extern "C" void UpdateObjectFloorHeights(XObj* object);
extern "C" void SelectHighestActiveFloor(XObj* object);
extern "C" void PSMTXMultVecArray(const Mtx m, const Vec* src, Vec* dst, u32 count);
extern "C" void fn_8016AA74(const f32* params);
extern "C" f32 fn_8016A81C(f32 value, f32 min, f32 max);
extern "C" void fn_8016AA78(f32* out, f32* in, f32* limits);
extern "C" f32 fn_8016A8D4(f32 from, f32 to, f32 t, f32 scale);
extern "C" CVec GetCornerIconPosition(CGame* game);
extern s32 gScreenSafeWidth;
extern s32 gScreenSafeBottom;
extern s32 gPointerScreenWidth;
extern s32 gPointerScreenHeight;
extern f32 lbl_805A1188[2];
extern f32 lbl_805A1288[2];
extern f32 lbl_805A1290[2];
extern CCameraActionLeticker gLetickerCameraAction;

// this unit's own pointer limits, which Update clamps to and the debug overlay draws;
// the area camera keeps the same numbers in its own copy
static const f32 sDebugParams[4] = { 1.1f, 0.9f, 1.0f, 0.6f };

// which of the leticker's four sides angle is closest to, counting quarter turns
// counterclockwise; the GameCube reads Cos/Sin off a table, the Wii uses MSL cosf/sinf
static inline s32 GetSide(f32 angle)
{
	s32 side;
	if (std::cosf(0.7853982f) <= std::cosf(angle)) { // 45 degrees
		side = 0;
	} else if (std::cosf(angle) <= std::cosf(3.9269907f)) { // 225 degrees
		side = 2;
	} else if (0.0f < std::sinf(angle)) {
		side = 1;
	} else {
		side = 3;
	}
	return side;
}

// the floor boxes; the GameCube tables carry the same numbers minus mFlags
static XObjFloor sLetickerLiftFloor0[2] = {
	{ 2, 0, 1, { 0, 0, 0 }, 0, { 1200, 5000, 1200 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLiftFloor1[2] = {
	{ 2, 0, 3, { 0, 517, 0 }, 0, { 1216, 878, 1216 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel10Floors[5] = {
	{ 2, 0, 1, { 0, 1000, -4650 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -13800 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -22750 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -28600 }, 0, { 600, 150, 2650 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel9Floors[4] = {
	{ 2, 0, 1, { 0, 1000, -4650 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -13800 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -22750 }, 0, { 600, 150, 7650 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel8Floors[4] = {
	{ 2, 0, 1, { 0, 1000, -4650 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -13800 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -21300 }, 0, { 600, 150, 5000 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel7Floors[4] = {
	{ 2, 0, 1, { 0, 1000, -4650 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -13800 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -19600 }, 0, { 600, 150, 2650 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel6Floors[3] = {
	{ 2, 0, 1, { 0, 1000, -4650 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -13800 }, 0, { 600, 150, 7650 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel5Floors[3] = {
	{ 2, 0, 1, { 0, 1000, -4650 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -12350 }, 0, { 600, 150, 5000 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel4Floors[3] = {
	{ 2, 0, 1, { 0, 1000, -4650 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -10650 }, 0, { 600, 150, 2650 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel3Floors[2] = {
	{ 2, 0, 1, { 0, 1000, -4650 }, 0, { 600, 150, 7650 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel2Floors[2] = {
	{ 2, 0, 1, { 0, 1000, -3200 }, 0, { 600, 150, 5000 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel1Floors[2] = {
	{ 2, 0, 1, { 0, 1000, -1500 }, 0, { 600, 150, 2650 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel0Floors[2] = {
	{ 2, 0, 1, { 0, 2564, 6 }, 0, { 604, 2827, 121 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

// the floor boxes active at each level, picked by mCurrentLevel
static XObjFloor* sLetickerFloorsByLevel[] = {
	sLetickerLevel0Floors, sLetickerLevel1Floors, sLetickerLevel2Floors,  sLetickerLevel3Floors,
	sLetickerLevel4Floors, sLetickerLevel5Floors, sLetickerLevel6Floors,  sLetickerLevel7Floors,
	sLetickerLevel8Floors, sLetickerLevel9Floors, sLetickerLevel10Floors,
};

// anim 7 plays sound 0x1FD at frame 10
static AnimSfxEvent sLetickerAnimSfxEvents[] = {
	{ 7, 10.0f, -1.0f, 1, 5, 0x1FD, 0xFF, 0, 0, 0 },
	{ -1 },
};

static XObjFloor* sLetickerLiftFloors[2] = { sLetickerLiftFloor0, sLetickerLiftFloor1 };

// sets the object's floor boxes, they take effect at once
static inline void SetFloors(XObj* obj, XObjFloor* floors)
{
	obj->mFloors        = floors;
	obj->mFloorMasks[1] = 0;
	obj->mFloorMasks[0] = 0;
	UpdateObjectFloorXZExtent(obj);
	UpdateObjectFloorHeights(obj);
}

// the turn-cross edges for this frame: Update pulls them out of
// gWiiPointer.mFlags at its top and hands them back at its end, so the cross
// still reaches the actions that run behind it in the same frame
static BOOL sTurnLeftEdge;  // .sbss 0x805A1C80, bit 32 turns mTurnTo a quarter down
static BOOL sTurnRightEdge; // .sbss 0x805A1C84, bit 33 turns it a quarter up

// whether the camera's eye still sits inside the lift's bounds when it faces angle;
// ReceivePlugPower turns it a quarter turn at a time until it fits
static inline bool CanTurn(CCameraActionLeticker* camera, f32 angle)
{
	if (!camera->mBounded) {
		return true;
	}
	Vec eye;
	camera->CalcEye(&eye, angle);
	if (eye.x < camera->mMinX || camera->mMaxX < eye.x || eye.z < camera->mMinZ || camera->mMaxZ < eye.z) {
		return false;
	}
	return true;
}

// starts one of the ride's two scripted animations: both objects move, the player
// is pinned to the lift and its rumble loop swaps over. the GameCube stopped
// sound 0xB0 and played 0xAE/0xAF; the Wii plays 0x11B/0x11C over a stopped 0x11D
static inline void StartLetickerRide(XObjLeticker* lift, XObj* part, WiiPlayerState* player, s32 anim, s32 partAnim, s32 sound)
{
	lift->SetAnim(anim, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	part->mStatus.mFlags |= 0x10000ULL;
	part->SetAnim(partAnim, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	part->SetPos(*lift->GetPos());
	player->mPlayerFlags = 0;
	player->mPlayerFlags |= 0x40000000;
	player->mPadReset = FALSE;
	StopWiiSfx(&gGame.mSound, 0x11D, 0.0f);
	PlayWiiSoundEffect(&gGame.mSound, sound, 1.0f, 0.0f);
	lift->mStep = 10;
}

/**
 * @note Address: 0x80119220
 * @note Size: 0xCC
 */
void CCameraActionLeticker::Init()
{
	// the turn arrows sit 30 pixels in from the safe area's edges, the right one turned round
	nw4r::math::_VEC2 pos[2] = { 0 };
	pos[0].x                 = gScreenSafeWidth / 2 - 30;
	pos[1].x                 = -(gScreenSafeWidth / 2 - 30);
	f32 rot[2]               = { 0.0f, PI };
	mArrows[0].mPos.x        = pos[0].x;
	mArrows[0].mPos.y        = pos[0].y;
	mArrows[0].mPos.z        = rot[0];
	mArrows[0].mShown        = 0;
	mArrows[0]._0D           = 0;
	mArrows[0]._10           = 0.0f;
	mArrows[0]._14           = 0.0f;
	mArrows[1].mPos.x        = pos[1].x;
	mArrows[1].mPos.y        = pos[1].y;
	mArrows[1].mPos.z        = rot[1];
	mArrows[1].mShown        = 0;
	mArrows[1]._0D           = 0;
	mArrows[1]._10           = 0.0f;
	mArrows[1]._14           = 0.0f;
}

/**
 * @note Address: 0x801192EC
 * @note Size: 0xAC
 */
void CCameraActionLeticker::Reset()
{
	CCameraActionLadr::Reset();
	mTurnFrames = 0;
	mTurnTo     = 0.0f;
	mBounded    = FALSE;
	gGame.SetStatusFlags(0x80);
	_76 = _75 = _74 = 0;
	_A4 = _A0 = 0;
	Vec v     = { 0.0f, 0.0f, 0.0f };
	_94       = v;
	_A8 = _90 = _8C = 0;
}

// NONMATCHING: first draft — retail frames the locals at 0x120 (ours 0xc0):
// the pointer Vec is copied word-for-word behind a second set of locals we
// don't have, its fn_8016AA78 limits argument is the unit's own {1.1,0.9,1,0.6}
// table at .rodata 0x802DAEA8 (sDebugParams, still waiting on the unit's
// .rodata split, retail's RTTI strings sit right behind it), and the callee
// register set is f30/f31 where we use only f31
/**
 * @note Address: 0x80119398
 * @note Size: 0xABC
 */
void CCameraActionLeticker::Update(u32 step)
{
	sTurnLeftEdge  = (gWiiPointer.mFlags & 0x100000000ULL) != 0;
	sTurnRightEdge = (gWiiPointer.mFlags & 0x200000000ULL) != 0;
	gWiiPointer.mFlags &= ~0x100000000ULL;
	gWiiPointer.mFlags &= ~0x200000000ULL;
	_68 = 0.0f;
	_6C = 0.0f;

	if (gWiiPointer.mButton & 4) {
		if (_70 > lbl_805A1288[0]) {
			if (_74 == 0) {
				_74 = 1;
			}
		} else {
			_70 += step;
		}
	} else {
		_74      = 0;
		_70      = 0.0f;
		Vec zero = { 0.0f, 0.0f, 0.0f };
		_78      = zero;
	}

	CVec input(0.0f, 0.0f, 0.0f);
	if (gWiiPointer.mPointerValid) {
		Vec pointer;
		pointer.x = 2.0f * (gWiiPointer.mPointer.x / gPointerScreenWidth) - 1.0f;
		pointer.y = 2.0f * (gWiiPointer.mPointer.y / gPointerScreenHeight) - 1.0f;
		pointer.z = gWiiPointer.mPointer.z;
		fn_8016AA78((f32*)&input, (f32*)&pointer, (f32*)sDebugParams);
		_78 = input;
	} else if (!(gGame._41B0 & 8)) {
		input = _78;
	}

	CVec direction(0.0f, 0.0f, 0.0f);
	if (gWiiPointer.mButton & 2) {
		direction.y = 1.0f;
	} else if (gWiiPointer.mButton & 1) {
		direction.y = -1.0f;
	}

	if (_74) {
		if (gWiiPointer.mPointerValid) {
			f32 y = fn_8016A81C(2.0f * (gWiiPointer.mPointer.y / gPointerScreenHeight) - 1.0f, -sDebugParams[0], sDebugParams[0]);
			f32 x = fn_8016A81C(2.0f * (gWiiPointer.mPointer.x / gPointerScreenWidth) - 1.0f, -sDebugParams[0], sDebugParams[0]);
			if (nw4r::math::FAbs(y) < sDebugParams[2] && nw4r::math::FAbs(x) < sDebugParams[3]) {
				_A8 = 1;
			}
		}
		if (_A8) {
			_68 = input.y;
		}
	} else {
		_A8 = 0;
		_68 = direction.y;
	}

	if (_68 > 0.0f) {
		_8C += step;
	} else {
		_8C = 0;
	}
	if (_68 < 0.0f) {
		_90 += step;
	} else {
		_90 = 0;
	}

	// a twist edge is injected back into mFlags so the turn runs below;
	// holding past the threshold keeps re-arming it on the repeat counters
	BOOL changed = FALSE;
	if (_68 > 0.0f) {
		if (_A0 == 0) {
			_A0     = 1;
			changed = TRUE;
		}
	} else {
		_A0 = 0;
	}
	if (changed) {
		gWiiPointer.mFlags |= 0x100000000ULL;
		if (_A0 == 0) {
			_A0 = 1;
		}
	}
	if (_A0 != 0 && _8C >= 30 && _8C >= 60) {
		gWiiPointer.mFlags |= 0x100000000ULL;
		_8C = 30;
	}

	changed = FALSE;
	if (_68 < 0.0f) {
		if (_A4 == 0) {
			_A4     = 1;
			changed = TRUE;
		}
	} else if (_A4 != 0) {
		_A4 = 0;
	}
	if (changed) {
		gWiiPointer.mFlags |= 0x200000000ULL;
		if (_A4 == 0) {
			_A4 = 1;
		}
	}
	if (_A4 != 0 && _90 >= 30 && _90 >= 60) {
		gWiiPointer.mFlags |= 0x200000000ULL;
		_90 = 30;
	}

	mArrows[0]._0D    = 0;
	mArrows[1]._0D    = 0;
	mArrows[0].mShown = 0;
	mArrows[1].mShown = 0;
	if (_74) {
		mArrows[0]._0D = 1;
		mArrows[1]._0D = 1;
		if (_68 > 0.0f) {
			mArrows[0].mShown = 1;
		} else if (_68 < 0.0f) {
			mArrows[1].mShown = 1;
		}
	} else {
		if (_68 > 0.0f) {
			mArrows[0]._0D    = 1;
			mArrows[0].mShown = 1;
		} else if (_68 < 0.0f) {
			mArrows[1]._0D    = 1;
			mArrows[1].mShown = 1;
		}
	}

	if (mTurnFrames == 0) {
		if (!CanTurn(this, mTurnTo - HALF_PI)) {
			mArrows[0]._0D = 0;
		}
		if (!CanTurn(this, HALF_PI + mTurnTo)) {
			mArrows[1]._0D = 0;
		}
	}
	if (mArrows[0]._0D) {
		if (mArrows[0]._14 < 1.0f) {
			mArrows[0]._14 += 0.1f * lbl_805A1290[0] * step;
			if (mArrows[0]._14 > 1.0f) {
				mArrows[0]._14 = 1.0f;
			}
		}
	} else if (mArrows[0]._14 > 0.0f) {
		mArrows[0]._14 -= 0.1f * lbl_805A1290[1] * step;
		if (mArrows[0]._14 < 0.0f) {
			mArrows[0]._14 = 0.0f;
		}
	}
	if (mArrows[1]._0D) {
		if (mArrows[1]._14 < 1.0f) {
			mArrows[1]._14 += 0.1f * lbl_805A1290[0] * step;
			if (mArrows[1]._14 > 1.0f) {
				mArrows[1]._14 = 1.0f;
			}
		}
	} else if (mArrows[1]._14 > 0.0f) {
		mArrows[1]._14 -= 0.1f * lbl_805A1290[1] * step;
		if (mArrows[1]._14 < 0.0f) {
			mArrows[1]._14 = 0.0f;
		}
	}

	if (mTurnFrames == 0) {
		if ((gWiiPointer.mFlags & 0x100000000ULL) != 0) {
			if (CanTurn(this, mTurnTo - HALF_PI)) {
				mTurnTo -= HALF_PI;
				mTurnSpeed  = -0.08726646f; // five degrees a frame, a quarter in 18
				mTurnFrames = 18;
			}
		} else if ((gWiiPointer.mFlags & 0x200000000ULL) != 0) {
			if (CanTurn(this, HALF_PI + mTurnTo)) {
				mTurnTo += HALF_PI;
				mTurnSpeed  = 0.08726646f;
				mTurnFrames = 18;
			}
		}
	} else {
		mTurnFrames -= step;
		if (mTurnFrames <= 0) {
			mAngle      = mTurnTo;
			mTurnFrames = 0;
		} else {
			mAngle += mTurnSpeed * step;
		}
	}
	CCameraActionLadr::Update(step);

	if (sTurnRightEdge) {
		gWiiPointer.mFlags |= 0x200000000ULL;
	} else {
		gWiiPointer.mFlags &= ~0x200000000ULL;
	}
	if (sTurnLeftEdge) {
		gWiiPointer.mFlags |= 0x100000000ULL;
	} else {
		gWiiPointer.mFlags &= ~0x100000000ULL;
	}
}

// NONMATCHING: first draft — retail's frame is 0x210 (ours 0x220), the two
// f-loads into the scale expression swap a gGame member against a pool literal,
// and the _0D/mShown test folds differently (beq vs our ble after cror)
/**
 * @note Address: 0x80119E54
 * @note Size: 0x494
 */
void CCameraActionLeticker::vf24()
{
	CVec corner = GetCornerIconPosition(&gGame);
	f32 limit   = gGame.mWipeCine.mY + 0.5f * gGame.mWipeCine.mHeight;
	if (limit >= gScreenSafeBottom) {
		limit = gScreenSafeBottom;
	}
	f32 bottom = corner.y;
	if (bottom > limit) {
		bottom = gGame.mWipeCine.mY + 0.5f * gGame.mWipeCine.mHeight;
		if (bottom >= gScreenSafeBottom) {
			bottom = gScreenSafeBottom;
		}
	}
	f32 maxY = ((bottom - 58.0f) - 40.0f) - 50.0f;
	for (s32 i = 0; i < 2; i++) {
		CamEdgeArrow* arrow = &mArrows[i];
		f32 scale;
		if (arrow->_0D) {
			arrow->_10 = 0.5f;
			if (arrow->mShown) {
				arrow->_10 = 1.0f;
			}
			scale = arrow->_10 * fn_8016A8D4(0.0f, 1.0f, arrow->_14, lbl_805A1188[0]);
		} else {
			scale = arrow->_10 * fn_8016A8D4(0.0f, 1.0f, arrow->_14, lbl_805A1188[0]);
		}
		if (scale <= 0.0f) {
			continue;
		}
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
		gGame.mGs.mCurRenderState->SetAlphaUpdate(GX_FALSE);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		if (i == 1) {
			gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_NONE);
		}
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[271], 0);
		CTexObj tex = gGame.mTpl.mTex;
		PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.0f, 0.0f);
		tex.mTexMtx.mDirty = true;
		GXColor color      = { 0xFF, 0xFF, 0xFF, 0xFF };
		color.a            = color.a * scale;
		f32 x              = arrow->mPos.x + 0.5f * gPointerScreenWidth;
		f32 y              = arrow->mPos.y + 0.5f * gPointerScreenHeight;
		if (y > maxY) {
			y = maxY;
		}
		f32 width  = GXGetTexObjWidth(&tex);
		f32 height = GXGetTexObjHeight(&tex);
		DrawGsTexturedQuad(&gGame.mGs, &tex, *(u32*)&color, *(u32*)&color, *(u32*)&color, *(u32*)&color, x, y, 0.0f, width, height, 0.0f,
		                   arrow->mPos.z, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_BACK);
	}
}

/**
 * @note Address: 0x8011A328
 * @note Size: 0xC
 */
void CCameraActionLeticker::DrawDebug2d()
{
	fn_8016AA74(sDebugParams);
}

/**
 * @note Address: 0x8011A334
 * @note Size: 0x9C
 */
void XObjLeticker::ResetObjectState()
{
	XObj::ResetObjectState();
	mLinkedCount   = 0;
	mFunc          = NULL;
	mCurrentLevel  = 0;
	mFuncTime      = 0;
	mLevelProgress = 0.0f;
	mHeight        = 0.0f;
	mPtcl          = TRUE;
	mPowered       = FALSE;
	mPowerTimer    = 0;
	mState         = 0;
	mHeading       = 0.0f;
	mBox[0].mId    = -1;
	mBox[1].mId    = -1;
	mSfx.Init((WiiPlayerState*)this, sLetickerAnimSfxEvents);
}

/**
 * @note Address: 0x8011A3D0
 * @note Size: 0x238
 */
void XObjLeticker::UpdateObject(u32 frames)
{
	if (GetAnimId() == 1) {
		if (GetAnimEndFrame() <= frames + GetAnimFrame()) {
			SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
	} else if (GetAnimId() == 3) {
		if (GetAnimEndFrame() <= frames + GetAnimFrame()) {
			SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
	}
	XObj::UpdateObject(frames);
	mFuncTime += frames;
	if (mFunc) {
		(this->*mFunc)();
	}
	f32 volume = GetSoundVolumeAtPosition(&gGame.mSound, GetPos(), 0);
	mSfx.Update(GetAnimId(), GetAnimFrame(), volume);
}

/**
 * @note Address: 0x8011A608
 * @note Size: 0x3D4
 */
void XObjLeticker::StateLift()
{
	XObj* base = gGame.mXObjs.mObjects[mLinked[0]];
	XObj* top  = gGame.mXObjs.mObjects[mLinked[1]];
	if (base == NULL || top == NULL) {
		return;
	}
	f32 height = 4.0f * (s32)(mLevelProgress / 3.0f) + 0.5f;
	if (mHeight == height) {
		mReachedTargetHeight = TRUE;
	} else if (mHeight < height) {
		mHeight += 0.5f;
		if (height < mHeight) {
			mHeight = height;
		}
	} else if (height < mHeight && 0.0f < mHeight) {
		mHeight -= 0.5f;
		if (mHeight < 0.0f) {
			mHeight = 0.0f;
		}
	}
	if (mHeight <= 2.5f) {
		mHeight = 2.5f;
	}
	s32 step = mHeight / 4.0f; // each level is four units of height
	if (mCurrentLevel != step) {
		if (!IsWiiSfxPlaying(&gGame.mSound, 0x115)) {
			PlayWiiSoundEffect(&gGame.mSound, 0x115, 1.0f, 0.0f);
		}
		SetGameSfxPitch(&gGame.mSound, 0x115, (s16)(600.0f * (mSpeedScale - 1.0f)), 0.0f);
		mCurrentLevel        = step;
		mReachedTargetHeight = FALSE;
	}
	top->SetTranslateY(9.0f + (mHeight + GetPos()->y));
	mStackTopPosition = *top->GetPos();
	if (mStatus.mFlags & 0x0800000000000000ULL) {
		f32 h;
		if (mHeight < 10.0f) {
			h = 19.0f;
		} else {
			h = 5.0f + (9.0f + mHeight) - 2.0f;
		}
		sLetickerLiftFloors[0]->mExtent[1]       = (s32)h * 100;
		sLetickerLiftFloors[0]->mCenterOffset[1] = (s32)(h / FSqrt(3.0f)) * 100;
		SetFloors(this, sLetickerLiftFloors[0]);
	} else {
		mBox[0].mCenterOffset[1] = (s32)((mHeight - 12.5f) / 4.0f) * 0xA9 + 0x654;
		mBox[0].mExtent[1]       = (s32)((mHeight - 12.5f) / 4.0f) * 400 + 1000;
		base->mFloors            = mBox;
		base->mFloorMasks[1]     = 0;
		base->mFloorMasks[0]     = 0;
		UpdateObjectFloorHeights(base);
		SetFloors(this, sLetickerLiftFloors[1]);
		SetColor(1, 0, 0, 0, 0.5f);
		SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	}
}

// NONMATCHING: regalloc, retail loads mCurrentLevel into r3 and the 1 for mUnwindWatch into r0;
// ours swaps the two
/**
 * @note Address: 0x8011A9DC
 * @note Size: 0x444
 */
void XObjLeticker::StateCount()
{
	XObj* part;
	s32 count = (s32)mLevelProgress / 10;
	if (mMaxLevel < count) {
		count = 10;
	}
	part = gGame.mXObjs.mObjects[mLinked[0]];
	if (mUnwindWatch && mUnwindAnim != part->GetAnimId()) { // another anim cut in ahead of the queued unwind
		mUnwindSuperseded = TRUE;
		mUnwindWatch      = FALSE;
	}
	if (mCurrentLevel != count) {
		if (part->GetAnimEndFrame() + part->GetAnimRate(1) <= part->GetAnimFrame()) {
			if (mCurrentLevel < count) {
				mCurrentLevel++;
				f32 rate = 1.0f;
				if (!mPowered) {
					rate = 8.0f;
				}
				part->SetAnim(mCurrentLevel, FALSE, 0.0f, rate, 0.0f, 15.0f);
			} else if (mPowered) { // a powered unwind steps back down at double rate
				mCurrentLevel--;
				part->SetAnim(24 - mCurrentLevel, FALSE, 0.0f, 2.0f, 0.0f, 15.0f);
				if (mCurrentLevel == 0) {
					part->PushAnim(0, 0, 100);
					mUnwindWatch = TRUE;
					mUnwindAnim  = 24 - mCurrentLevel;
				}
			} else {
				mLevelProgress = (mCurrentLevel - 1) * 10;
			}
			if (mPowered) {
				PlayWiiSoundEffect(&gGame.mSound, 0x119, 1.0f, 0.0f);
				SetGameSfxPitch(&gGame.mSound, 0x119, (s16)(600.0f * (mSpeedScale - 1.0f)), 0.0f);
			}
		}
		SetFloors(part, sLetickerFloorsByLevel[mCurrentLevel]);
	}
	if (!(mStatus.mFlags & 0x0800000000000000ULL)) {
		SetColor(1, 0, 0, 0, 0.5f);
		SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	}
}

/**
 * @note Address: 0x8011AE20
 * @note Size: 0xE4
 */
void XObjLeticker::StateRide()
{
	WiiPlayerState* player  = gGame.GetPlayer();
	nw4r::g3d::ResNode node = GetResNodeOrFirst(0xD);
	switch (mStep) {
	case 0:
		break;
	case 10: {
		Vec pos;
		pos.x = pos.z = 0.0f;
		pos.y         = 0.5f;
		PSMTXMultVec(GetWorldMtx(node), &pos, &pos);
		player->SetPos(pos);
		player->SetRotationY(player->mMoveRotate.y - 0.5235988f);
		break;
	}
	}
}

/**
 * @note Address: 0x8011AF04
 * @note Size: 0xE4
 */
void XObjLeticker::DrawObject()
{
	DrawWithView(NULL);
	GetPos(); // unused: retail keeps the vcall, the GameCube's segment-draw loop that read it is gone
	switch (mState) {
	case 1:
		gGame.mXObjs.mObjects[mLinked[0]]->GetResNodeOrFirst(7); // unused, as GetPos above
		break;
	case 2:
		gGame.mXObjs.mObjects[mLinked[0]]->mStatus.mFlags &= ~0x80000000ULL;
		break;
	case 3:
		break;
	}
}

// NONMATCHING: register coloring residue — ours gives i r29 and the drain pointer r27,
// retail i r27 / drain r29; the mState guard also folds to a single bne where retail
// keeps beq into the body + b to the exit, and two float pool referents differ
/**
 * @note Address: 0x8011AFE8
 * @note Size: 0x1B8
 */
void XObjLeticker::DrawLate()
{
	if (mState != 1) {
		return;
	}
	// the lift's stack: the linked part is redrawn at each 20-unit step of the
	// height, its position yanked back once the pass is done
	s32 i;
	XObj* part = gGame.mXObjs.mObjects[mLinked[0]];
	f32 y      = part->GetPos()->y;
	WiiGsSceneObj* scene;
	for (i = 0; 0.0f < mHeight - 20.0f * i; i++) {
		part->SetTranslateY(y + mHeight - 20.0f * i);
		gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, part->mObject.m_pScnMdl);
		((nw4r::g3d::ScnRoot*)gGame.mGs.mScene.mHead)->UpdateFrame();
		((nw4r::g3d::ScnRoot*)gGame.mGs.mScene.mHead)->CalcWorld();
		((nw4r::g3d::ScnRoot*)gGame.mGs.mScene.mHead)->CalcView();
		((nw4r::g3d::ScnRoot*)gGame.mGs.mScene.mHead)->GatherDrawScnObj();
		((nw4r::g3d::ScnRoot*)gGame.mGs.mScene.mHead)->DrawOpa();
		scene = gGame.mGs.mScene.mHead;
		while (scene->mPending != 0) {
			scene->PopPending();
		}
	}
	part->SetTranslateY(y);
}

// NONMATCHING: ours keeps one more float in a saved register (f21, a 0x10 bigger frame);
// retail's int-to-float conversion carries an xoris ours lacks
/**
 * @note Address: 0x8011B1A0
 * @note Size: 0x3D0
 */
void XObjLeticker::DrawObjectEffects()
{
	XObj* part = gGame.mXObjs.mObjects[mLinked[0]];
	switch (mState) {
	case 3:
		if (!(0.0f < mLevelProgress) || (!mPowered && part->GetAnimId() != 1)) {
			break;
		}
		Mtx mtx;
		u32 colors[50];
		Vec pts[50];
		Vec pos = *GetPos();
		PSMTXTrans(mtx, pos.x, pos.y, pos.z);
		f32 twist = 6.0f * (mLevelProgress / mMaxLevelProgress) * (f32)sin(DEG2RAD * (mFuncTime * 5)) + 72.0f;
		u32 i;
		for (i = 0; i < 50; i++) { // a helix of points rising and twisting with progress
			f32 angle = DEG2RAD * mFuncTime + DEG2RAD * (twist * (s32)i);
			f32 y     = 12.0f + 0.2f * (24.0f * (s32)i / 50.0f) * mLevelProgress / mMaxLevelProgress;
			pts[i].x  = 7.1f * (f32)sin(angle);
			pts[i].y  = y;
			pts[i].z  = 7.1f * (f32)cos(angle);
			colors[i] = ((48 - (s32)i) * 32 / 48 + 32) | 0x40F000; // the low channel fades along the strip
		}
		pts[49].x = pts[44].x;
		pts[49].y = pts[44].y;
		pts[49].z = pts[44].z;
		PSMTXMultVecArray(mtx, pts, pts, 50);
		DrawGsColoredLineStrip(&gGame.mGs, pts, colors, 50);
		if (!IsWiiSfxPlaying(&gGame.mSound, 0x11D)) {
			PlayWiiSoundEffect(&gGame.mSound, 0x11D, GetSoundVolumeAtPosition(&gGame.mSound, part->GetPos(), 0), 0.0f);
			SetGameSfxPitch(&gGame.mSound, 0x11D, (s16)(600.0f * (mSpeedScale - 1.0f)), 0.0f);
		} else {
			SetWiiSfxVolume(&gGame.mSound, 0x11D, GetSoundVolumeAtPosition(&gGame.mSound, part->GetPos(), 0), 0.0f);
		}
		break;
	}
}

extern "C" {

// forward, GetClearLetickerApproachSide runs ahead of it in retail .text
bool IsLetickerJointClearOfObjects(XObj* obj, nw4r::g3d::ResNode node);
// forwards, ReceivePlugPower calls them, they follow OnPushDisplacement in retail .text
BOOL TurnLetickerAndCheckClearance(XObjLeticker* lift, f32 angle);
BOOL TurnLetickerLinkedPartAndTrackJoint(XObjLeticker* leticker, f32 angle);

/**
 * @note Address: 0x8011B570
 * @note Size: 0x224
 */
s32 GetClearLetickerApproachSide(XObj* obj, const Vec* pos)
{
	const Vec* objPos = obj->GetPos();
	f32 angle         = WrapAngle((f32)atan2(pos->x - objPos->x, pos->z - objPos->z) - obj->mMoveRotate.y);
	if (3.0f + obj->GetPos()->y < gGame.GetPlayer()->GetPos()->y) {
		return 0;
	}
	if ((f32)cos(0.7853982) < (f32)cos(angle)) { // 45 degrees
		if (IsLetickerJointClearOfObjects(obj, obj->GetResNodeOrFirst(6))) {
			return -1;
		}
	} else if ((f32)cos(angle) < (f32)cos(3.9269907)) { // 225 degrees
		if (IsLetickerJointClearOfObjects(obj, obj->GetResNodeOrFirst(10))) {
			return -2;
		}
	} else if (0.0f < (f32)sin(angle)) {
		if (IsLetickerJointClearOfObjects(obj, obj->GetResNodeOrFirst(8))) {
			return -3;
		}
	} else {
		if (IsLetickerJointClearOfObjects(obj, obj->GetResNodeOrFirst(12))) {
			return -4;
		}
	}
	return 0;
}

// whether the spot under the joint is free, nothing near it in the way
// NONMATCHING: regalloc, target and loop walker swap r26/r27 and an f1/f2 temp swaps against retail
/**
 * @note Address: 0x8011B7BC
 * @note Size: 0x244
 */
bool IsLetickerJointClearOfObjects(XObj* obj, nw4r::g3d::ResNode node)
{
	CVec pos;
	MtxP mtx = obj->GetWorldMtx(node);
	pos.x = pos.y = pos.z = 0.0f;
	PSMTXMultVec(mtx, &pos, &pos);
	pos.y -= 6.0f;
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		if (!(gGame.mXObjs.mObjects[i]->mStatus.mFlags & 1) || !(gGame.mXObjs.mObjects[i]->mStatus.mFlags & 0x10000)) {
			continue;
		}
		if (obj == gGame.mXObjs.mObjects[i]) {
			continue;
		}
		if ((s16)gGame.mXObjs.mObjects[i]->GetPool() == 2) {
			continue;
		}
		XObj* target    = gGame.mXObjs.mObjects[i];
		CVec* targetPos = target->GetPos();
		f32 z           = targetPos->z - pos.z;
		f32 x           = targetPos->x - pos.x;
		f32 dist        = FSqrt(x * x + z * z);
		if (dist > 100.0f + (5.0f + target->mFloorXZExtent[0])) {
			continue;
		}
		CVec hit, normal;
		if (target->TestFloorCollision(&pos, &pos, 5.0f, 12.0f, &hit, &normal, 0, 0) & 2) {
			return false;
		}
	}
	return true;
}

} // extern "C"

// NONMATCHING: retail hoists the unit's generated-data base into r30 and reads the three
// StateLift/StateCount/StateRide PTMF records through it; ours references each record with its
// own lis/lwzu and saves one GPR less (r27-r31 vs r26-r31). Same source matches on GameCube.

/**
 * @note Address: 0x8011BA00
 * @note Size: 0xB00
 */
s32 XObjLeticker::HandleEventCommand(s32 count, s32* args)
{
	WiiPlayerState* player = gGame.GetPlayer();
	switch (args[0]) {
	case 0:
		mLinkedCount = 0;
		mMaxLevel    = args[1];
		for (s32 i = 0; i < 4; ++i)
			mLevelBySide[i] = mMaxLevel;
		for (s32 i = 2; i < count && (u32)mLinkedCount < 2; ++i) {
			mLinked[mLinkedCount] = args[i];
			++mLinkedCount;
		}
		mStackTopPosition = *GetPos();
		mSpeedScale       = 1.0f;
		for (s32 i = 0; i < mLinkedCount; ++i) // claims the parts, the GameCube had no word for it
			gGame.mXObjs.mObjects[mLinked[i]]->_99C = 1;
		if (mLinkedCount > 0) {
			XObj* part = gGame.mXObjs.mObjects[mLinked[0]];
			switch (part->mCoord.mPlacement->mObjectId) {
			case 0xEE: {
				mState = 1;
				mFunc  = &XObjLeticker::StateLift;
				if (mMaxLevel > 30)
					mMaxLevel = 30;
				mBox[0].mId              = 2;
				mBox[0].mMaterial        = 0;
				mBox[0].mFlags           = 3;
				mBox[0].mCenterOffset[0] = 0;
				mBox[0].mCenterOffset[1] = 1620;
				mBox[0].mCenterOffset[2] = 0;
				mBox[0].mYawDegrees      = 0;
				mBox[0].mExtent[0]       = 400;
				mBox[0].mExtent[1]       = 1000;
				mBox[0].mExtent[2]       = 200; // the base's floor box, 16.2 units up
				part->GetResNodeOrFirst(1);     // unused, as GetPos in DrawObject: the GameCube hid the joint
				break;
			}
			case 0x114:
				mState = 2;
				mFunc  = &XObjLeticker::StateCount;
				if (mMaxLevel > 10)
					mMaxLevel = 10;
				break;
			case 0x266:
				mState = 3;
				mFunc  = NULL;
				for (s32 i = 0; mFloors[i].mId != -1; ++i)
					if (mFloors[i].mFlags & 1)
						mFloorMasks[0] |= 1U << i;
				SelectHighestActiveFloor(this);
				break;
			}
		}
		mMaxLevelProgress    = 10.0f * mMaxLevel;
		mHeight              = 0.0f;
		mReachedTargetHeight = FALSE;
		mCameraBounded       = FALSE;
		mCurrentLevel        = 0;
		break;
	case 1: {
		if (count < 2 || args[1] == 0)
			mLevelProgress = 0.0f;
		else
			mLevelProgress = 10.0f * args[1];
		if (count < 3 || args[2] == 0)
			mHeading = 0.0f;
		else
			mHeading = DEG2RAD * args[2];
		bool setHeight;
		if (count < 4 || args[3] == 0)
			setHeight = false;
		else
			setHeight = true;
		if (mLinkedCount == 0) {
			break; // the GameCube reported an uninitialized leticker, the Wii just leaves
		}
		switch (mState) {
		case 1: {
			XObj* base = gGame.mXObjs.mObjects[mLinked[0]];
			XObj* top  = gGame.mXObjs.mObjects[mLinked[1]];
			if (setHeight) {
				mHeight       = 4.0f * (s32)(mLevelProgress / 3.0f) + 0.5f;
				mCurrentLevel = mHeight / 4.0f;
			}
			base->SetRotateY(mHeading + GetRotY(), 0.1f);
			top->SetRotateY(mHeading + GetRotY(), 0.1f);
			break;
		}
		case 2: {
			XObj* part    = gGame.mXObjs.mObjects[mLinked[0]];
			mCurrentLevel = 0;
			part->SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			part->SetRotateY(mHeading + GetRotY(), 0.1f);
			break;
		}
		case 3:
			SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mLevelProgress = 0.0f;
			mStep          = 0;
			break;
		}
		break;
	}
	case 2:
		// the dust particle flag; the GameCube created the effect here, the Wii
		// retires it from the powered code paths instead
		if (args[1]) {
			mPtcl = TRUE;
		} else {
			mPtcl = FALSE;
		}
		break;
	case 3:
		// script queries: level, facing degrees, at max, powered, speed percent
		switch (args[1]) {
		case 0:
			return (s32)((9.0f + mLevelProgress) / 10.0f);
		case 1: // GetSide's quarters in degrees
			if ((f32)cos(0.7853982) <= (f32)cos(mHeading)) {
				return 0;
			} else if ((f32)cos(mHeading) <= (f32)cos(3.9269907)) {
				return 180;
			} else if (0.0f < (f32)sin(mHeading)) {
				return 90;
			} else {
				return 270;
			}
		case 2:
			if (mLevelProgress < mMaxLevelProgress)
				return FALSE;
			return TRUE;
		case 3:
			return mPowered;
		case 4:
			return (s32)(100.0f * mSpeedScale);
		}
		// no break: a query the list doesn't have runs on into the settings
	case 4:
		switch (args[1]) {
		case 0:
			mSpeedScale = args[2] / 100.0f;
			break;
		case 1:
			for (s32 i = 0; i < 4; ++i)
				mLevelBySide[i] = args[i + 2];
			break;
		case 2:
			mCameraBounded = TRUE;
			mCameraMinX    = args[2] / 100.0f;
			mCameraMaxX    = args[3] / 100.0f;
			mCameraMinZ    = args[4] / 100.0f;
			mCameraMaxZ    = args[5] / 100.0f;
			break;
		}
		break;
	case 5: // the ride's script commands, state 3 only
		if (mState == 3) {
			XObj* part = gGame.mXObjs.mObjects[mLinked[0]];
			switch (args[1]) {
			case 0:
				mLevelProgress = 0.0f;
				SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				part->SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				player->mPlayerFlags = 0;
				player->mPlayerFlags |= 0x20000000;
				player->mPlayerFlags |= 7;
				player->mPadReset = TRUE;
				player->SetRotationY(mMoveRotate.y);
				mStep = 0;
				break;
			case 1:
				StartLetickerRide(this, part, player, 4, 2, 0x11B);
				break;
			case 2:
				StartLetickerRide(this, part, player, 5, 3, 0x11C);
				break;
			case 3:
				if (count <= 2 || args[2]) {
					player->mLift.mState = 1;
					player->mLift.SetTarget(&player->mPlug);
					player->mLift.mTargetState = 0;
				} else if (player->mLift.mTarget) {
					player->mLift.mState = 6;
				}
				break;
			}
			mFunc = &XObjLeticker::StateRide;
		}
		break;
	}
	return 0;
}

/**
 * @note Address: 0x8011C500
 * @note Size: 0x4
 */
void XObjLeticker::OnHitFromPosition(const Vec& position, f32 strength)
{
}

// NONMATCHING: retail fetches the player's position before the lift's for the heading and
// re-reads mObjects[mLinked[0]] for each call in the ride case; ours keeps the part in a register
// and inlines the bounds setter's loads in a different order
/**
 * @note Address: 0x8011C504
 * @note Size: 0x95C
 */
BOOL XObjLeticker::ReceivePlugPower(f32 amount)
{
	WiiPlayerState* player = gGame.GetPlayer();
	if (player->GetPos()->y < GetPos()->y - 5.0f) {
		return FALSE;
	}
	CPad pad(0);
	f32 heading = (f32)atan2(player->GetPos()->x - GetPos()->x, player->GetPos()->z - GetPos()->z);
	f32 angle   = heading - GetRotY();
	// the pushed direction snaps the target rotation to a quarter turn
	s32 side = GetSide(angle);
	switch (side) {
	case 0:
		angle = 0.0f;
		break;
	case 1:
		angle = HALF_PI;
		break;
	case 2:
		angle = PI;
		break;
	case 3:
		angle = -HALF_PI;
		break;
	}
	if (!mPowered) {
		mPowered = TRUE;
		mWaiting = FALSE;
		if (mState != 3) {
			gGame.mMotionControlCamera.PushAction(&gLetickerCameraAction);
			gLetickerCameraAction.mTarget = &mStackTopPosition;
			if (mCameraBounded) {
				gLetickerCameraAction.SetBounds(mCameraMinX, mCameraMaxX, mCameraMinZ, mCameraMaxZ);
				for (s32 i = 0; i < 4; ++i) {
					if (CanTurn(&gLetickerCameraAction, gLetickerCameraAction.mAngle)) {
						break;
					}
					gLetickerCameraAction.mTurnTo = gLetickerCameraAction.mAngle += HALF_PI;
				}
			}
		} else {
			XObj* part = gGame.mXObjs.mObjects[mLinked[0]];
			part->mStatus.mFlags &= ~0x10000ULL;
			part->SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		if (mState == 2) {
			mMaxLevel         = mLevelBySide[side];
			mMaxLevelProgress = 10.0f * mMaxLevel;
			mUnwindSuperseded = FALSE;
			mUnwindWatch      = FALSE;
			mUnwindAnim       = 0;
		}
		if (mPtcl) {
			fn_800A12F8(&gGame.mEvtPtcl, 0xA5, FALSE);
			mPtcl = FALSE;
		}
	}
	if (mWaiting) {
		if (!(pad.mPad->mButton & (1ULL << (NunchukStick + 2)))) {
			mPowerTimer -= 1;
			if (mPowerTimer <= 0) {
				return FALSE;
			}
			return TRUE;
		}
		mWaiting = FALSE;
	}
	amount *= mSpeedScale;
	++mPowerTimer;
	bool starting = mPowerTimer < 40;
	switch (mState) {
	case 1: // the lift
		if (!TurnLetickerAndCheckClearance(this, angle)) {
			if (mHeight == 4.0f * (s32)(mLevelProgress / 3.0f) + 0.5f) {
				XObj* part = gGame.mXObjs.mObjects[mLinked[1]];
				PlayWiiSoundEffect(&gGame.mSound, 0x1FE, 1.0f, 0.0f);
				SetColor(1, 0xFF, 0, 0, 0.5f);
				SetAnim(4, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				part->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				part->PushAnim(0, 0, 100);
				mPowerTimer = 30;
				mWaiting    = TRUE;
			}
			return TRUE;
		}
		break;
	case 2: // the counter
		if (!TurnLetickerLinkedPartAndTrackJoint(this, angle)) {
			return starting;
		}
		break;
	case 3: // the ride
		mStackTopPosition = *GetPos();
		gGame.mXObjs.mObjects[mLinked[0]]->SetPos(*GetPos());
		gGame.mXObjs.mObjects[mLinked[0]]->SetRotateY(mMoveRotate.y, 1.0f);
		if (mMaxLevelProgress <= mLevelProgress + amount) {
			XObj* part = gGame.mXObjs.mObjects[mLinked[0]];
			part->mStatus.mFlags |= 0x10000ULL;
			part->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		} else {
			XObj* part = gGame.mXObjs.mObjects[mLinked[0]];
			if (part->GetAnimId() != 0) {
				part->SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			}
		}
		break;
	}
	if (player->mParams.mBattery <= 0.0f) {
		return FALSE;
	}
	if (pad.mPad->mButton & (1ULL << (NunchukStick + 2))) {
		mLevelProgress -= amount;
		if (mLevelProgress < 0.0f) {
			mLevelProgress = 0.0f;
			return starting;
		}
	} else {
		mLevelProgress += amount;
		if (mMaxLevelProgress <= mLevelProgress) {
			mLevelProgress = mMaxLevelProgress;
			return starting;
		}
	}
	if (GetAnimId() != 1 && GetAnimId() != 2) {
		SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	}
	return TRUE;
}

/**
 * @note Address: 0x8011CE60
 * @note Size: 0x1D0
 */
void XObjLeticker::OnUnplugged()
{
	if (mState != 3) {
		gGame.ClearStatusFlags(0x80);
		gGame.mMotionControlCamera.PopAction();
	}
	mPowered    = FALSE;
	mPowerTimer = 0;
	if (mState == 2) {
		mMaxLevel         = mLevelBySide[GetSide(mHeading)];
		mMaxLevelProgress = 10.0f * mMaxLevel;
		mLevelProgress    = 10.0f * mCurrentLevel;
	}
	SetColor(1, 0, 0, 0, 0.5f);
	SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
}

// NONMATCHING: retail loads GetPos()->x/z straight from r3 before moving the part into r3;
// ours copies the result to r4 first, and the loop registers r28-r30 rotate
/**
 * @note Address: 0x8011D030
 * @note Size: 0x1B8
 */
void XObjLeticker::OnPushDisplacement(const Vec& displacement)
{
	for (s32 i = 0; i < mLinkedCount; i++) {
		XObj* part = gGame.mXObjs.mObjects[mLinked[i]];
		part->SetTranslateX(GetPos()->x);
		part->SetTranslateZ(GetPos()->z);
	}
	if (0.0f < PSVECMag(&displacement)) {
		SetColor(1, 0, 0, 0, 0.5f);
		SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	} else {
		SetColor(1, 0xFF, 0, 0, 1.0f);
		SetAnim(7, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	}
	if (mPtcl) {
		fn_800A12F8(&gGame.mEvtPtcl, 0xA5, FALSE);
		mPtcl = FALSE;
	}
}

// the leticker's world callback: after the base handling it rotates joint 2's
// world matrix to face the player. the GameCube did this in UpdateObject with
// HSD_JObjSetRotationY; on the Wii it rides the calc-world chain
/**
 * @note Address: 0x8011D1E8
 * @note Size: 0x164
 */
void XObjLeticker::ExecCallbackB(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
	XObj::ExecCallbackB(pResult, mdl, pFuncObj);
	if (pFuncObj->GetCallbackNodeID() == 0) {
		nw4r::g3d::ResNode node = GetResNode(2);
		if (node.IsValid()) {
			pFuncObj->SetCallbackNodeID(node.GetID());
		}
		return;
	}
	Mtx m;
	PSMTXCopy(pResult->mpMtx, m);
	const CVec* playerPos = gGame.GetPlayer()->GetPos();
	const CVec* pos       = GetPos();
	f32 angle             = (f32)atan2(playerPos->x - pos->x, playerPos->z - pos->z);
	Mtx rot;
	PSMTXRotRad(rot, 'y', angle);
	// retail re-writes the translation column it just copied before concatenating
	PSMTXTrans(m, m[0][3], m[1][3], m[2][3]);
	PSMTXConcat(m, rot, m);
	PSMTXCopy(m, pResult->mpMtx);
}

extern "C" {

// NONMATCHING: retail calls the Wii MSL's out-of-line __fpclassifyf where our math.h inlines it,
// and copies the CVec with float loads where ours copies words
/**
 * @note Address: 0x8011D34C
 * @note Size: 0x3A0
 */
BOOL TurnLetickerAndCheckClearance(XObjLeticker* lift, f32 angle)
{
	CPad pad(0);
	XObj* first    = gGame.mXObjs.mObjects[lift->mLinked[0]];
	XObj* second   = gGame.mXObjs.mObjects[lift->mLinked[1]];
	lift->mHeading = angle;
	first->SetRotateY(angle + lift->GetRotY(), 0.1f);
	second->SetRotateY(angle + lift->GetRotY(), 0.1f);
	if (pad.mPad->mButton & (1ULL << (NunchukStick + 2))) {
		return TRUE;
	}

	f32 offset = 4.0f * (s32)(lift->mLevelProgress / 3.0f) + 0.5f;
	CVec pos(*second->GetPos());
	pos.y = 6.0f + (3.0f + (9.0f + (offset + lift->GetPosY())));
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* target = gGame.mXObjs.mObjects[i];
		if (!(target->mStatus.mFlags & 1) || !(target->mStatus.mFlags & 0x10000)) {
			continue;
		}
		if (target == lift || target == first || target == second) {
			continue;
		}
		XObjSource* source = target->mAttr.mObjData != NULL ? target->mAttr.mObjData->mSource : NULL;
		if (source == NULL) {
			continue;
		}
		const Vec* targetPos = target->GetPos();
		f32 dx               = targetPos->x - pos.x;
		f32 dz               = targetPos->z - pos.z;
		if (FSqrt(dx * dx + dz * dz) > 100.0f + (5.0f + target->mFloorXZExtent[0])) {
			continue;
		}
		CVec hit, normal;
		u32 flags = target->TestFloorCollision(&pos, &pos, 5.5f, 6.0f, &hit, &normal, 0, 0);
		if (flags & 2) {
			return FALSE;
		}
		if (flags & 1) {
			f32 floor = target->mFloorHeights[target->mHitFloorIndex];
			if (__fpclassifyf(floor) == FP_NAN) {
				return FALSE;
			}
			if (pos.y <= floor) {
				return FALSE;
			}
		}
	}
	return TRUE;
}

// NONMATCHING: retail adds angle and the lift's rotation once into f31; ours keeps the rotation
// and adds again, and the zero Vec's stores are scheduled later
/**
 * @note Address: 0x8011D6EC
 * @note Size: 0x1B0
 */
BOOL TurnLetickerLinkedPartAndTrackJoint(XObjLeticker* leticker, f32 angle)
{
	XObj* part = gGame.mXObjs.mObjects[leticker->mLinked[0]];
	f32 my     = leticker->GetRotY();
	f32 c      = cos(part->GetRotY() - (angle + my));
	if (c < (f32)cos(0.08726646f)) { // the part is more than five degrees off
		leticker->mLevelProgress = 0.0f;
		leticker->mPowerTimer    = 0;
		if (part->GetAnimId() == 0) {
			part->SetRotateY(angle + leticker->GetRotY(), 0.1f);
			leticker->mHeading = angle;
		}
	}
	if (!leticker->mUnwindSuperseded) {
		if (part->GetAnimFrame() < part->GetAnimEndFrame()) {
			return TRUE;
		}
	}
	if (leticker->mUnwindSuperseded) {
		leticker->mUnwindSuperseded = FALSE;
	}
	Vec pos;
	pos.x = pos.y = pos.z = 0.0f;
	PSMTXMultVec(part->GetWorldMtx(leticker->mCurrentLevel + 1), &pos, &leticker->mStackTopPosition);
	return TRUE;
}

} // extern "C"

/**
 * @note Address: 0x8011D89C
 * @note Size: 0x14
 */
void XObjLeticker::ResumeSounds(f32 fade)
{
	mSfx.Resume(fade);
}

/**
 * @note Address: 0x8011D8B0
 * @note Size: 0x14
 */
void XObjLeticker::PauseSounds(f32 fade)
{
	mSfx.Pause(fade);
}

/**
 * @note Address: 0x8011D8C4
 * @note Size: 0x8
 */
f32 CCameraActionLeticker::GetFollowDivisor()
{
	return 0.1f;
}

/**
 * @note Address: 0x8011D8CC
 * @note Size: 0x8
 */
u32 CCameraActionLeticker::GetCameraFlags()
{
	return 11;
}

/**
 * @note Address: 0x8011D8DC - the deleting destructor sits between GetCameraFlags
 * and GetMode in retail, the compiler picks its own spot
 */

/**
 * @note Address: 0x8011D8D4
 * @note Size: 0x8
 */
u32 CCameraActionLeticker::GetMode()
{
	return 0;
}

/**
 * @note Address: 0x8011D91C
 * @note Size: 0x20
 */
Vec CCameraActionLeticker::GetCameraTarget()
{
	return *mTarget;
}

// the action ReceivePlugPower pushes while the leticker's camera is bounded;
// retail's __sinit constructs it in this unit
CCameraActionLeticker gLetickerCameraAction;
