#include "game/fighter.h"
#include "game/hsd_jobj.h"
#include "game/cgame.h"
#include "game/csound.h"
#include "game/globals.h"
#include "game/gmath.h"
#include "game/xchr.h"
#include "game/xobjactor.h"

#include <Dolphin/pad.h>
#include <dolphin/mtx/mtx.h>
extern "C" {
#include <sysdolphin/random.h>
}

extern "C" void GXProject(f32 x, f32 y, f32 z, const Mtx view, const f32* proj, const f32* viewport, f32* sx, f32* sy, f32* sz);

inline Vec MakeVec(f32 x, f32 y, f32 z)
{
	Vec v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

namespace fighter {

class CCameraActionFighter : public CInterfaceCameraAction {
public:
	CCameraActionFighter()
	{
		mTargetMtx        = NULL;
		mInterestOffset.x = mInterestOffset.y = mInterestOffset.z = 0.0f;
		mEyeOffset.x = mEyeOffset.y = mEyeOffset.z = 0.0f;
		mCenterView                                = true;
	}
	virtual ~CCameraActionFighter() { }

	void SetView(Vec eye, Vec interest, MtxPtr mtx)
	{
		mCenterView     = false;
		mEyeOffset      = eye;
		mInterestOffset = interest;
		mTargetMtx      = mtx;
	}

	virtual void Update(u32 step);
	virtual void Init() { mCenterView = false; }
	virtual u32 GetMode() { return 0; }
	virtual u32 GetCameraFlags() { return 11; }
	virtual f32 GetFollowDivisor() { return 0.1f; }

	MtxPtr mTargetMtx; // _40, matrix the eye and interest offsets are carried by each frame; the fighter's model matrix (fighterWork._0C);
	                   // Update does nothing while NULL
	Vec mInterestOffset; // _44, interest point local to mTargetMtx, (0, 0, 120) ahead of the fighter
	Vec mEyeOffset;      // _50, eye point local to mTargetMtx, (0, 40, -180) behind and above the fighter
	u8 _5C[0xC];         // _5C
	bool mCenterView; // _68, true: eye on the world's vertical axis (x = z = 0) at the height of a point 80 ahead of the fighter, looking
	                  // at that point; false: chase view from mEyeOffset/mInterestOffset. Toggled by PAD_BUTTON_Y (0x800) trigger
};

} // namespace fighter

// one of the things dropped into the stage
struct Mato {
	Mato()
	{
		mObj         = NULL;
		mId          = -1;
		mMarkerColor = 0xFFFFFFC8;
	}

	CVec GetPos() const { return mObj->mMoveTranslate; }
	s32 GetPoints() const;

	s32 mId;           // _00, id the stage script passed to AddMato; -1 when unused; returned by TakeDestroyedMatoId
	XObj* mObj;        // _04, the dol actor this target is (XObj, an XObj); its mStatus bit 0x10000 marks it live
	f32 mScreenX;      // _08, screen x of the target's marker, from XObj::ProjectOffsetToScreen
	f32 mScreenY;      // _0C, screen y of the target's marker, raised by half the projected size
	f32 mScreenZ;      // _10, screen z (depth) from ProjectOffsetToScreen; written, never read
	u32 mMarkerColor;  // _14, RGBA colour word: 0xFFFFFFC8 at construction, each frame while on screen 0xFF0000C8 (red) if mOnScreenTime
	                   // >= 15 else 0xFFFFFF50; written, never read
	u8 mOnScreen;      // _18, set while XObj::ProjectVisibleBounds says the target is visible
	u8 mPending;       // _19, set when the shot that takes mShotsLeft to 0 is fired, cleared on AddMato and when a landing shot leaves
	                   // mHitsLeft > 0; while set the HUD skips the marker and the aim cannot lock on
	u8 _1A[0x2];       // _1A
	s32 mOnScreenTime; // _1C, zeroed on AddMato and every frame the target is off screen; compared >= 15 to pick the red colour in
	                   // mMarkerColor, but nothing ever increments it
	f32 mMarkerPhase;  // _20, f32 seeded with HSD_Rand() in AddMato, +1.0f each frame while on screen; never read otherwise
	s32 mShotsLeft;    // _24, shots still to fire at it; 1 for object kind 0x14D, 3 for 0x2EA; at 0 it is no longer a target
	s32 mHitsLeft;     // _28, hits still to land; when a landing shot takes it to 0 the target counts as destroyed and scores
	s32 mKind;         // _2C, 0 for object kind 0x14D, 1 for 0x2EA; picks the points in MatoList::mKindPoints
	struct MatoList* mList; // _30, the MatoList this target belongs to (AddMato sets &matoList); the shot that destroys it posts
	                        // mDestroyed and adds mKindPoints to its mScore
};

struct MatoList {
	s32 mCount;         // _00, number of targets used in _2C
	Vec mAimPos;        // _04, copy of the aim point (fighterWork.mAim.mPos, local to the fighter matrix) made every HUD draw; never read
	s32 mScore;         // _10, points scored so far
	s32 mFrameCount;    // _14, frame counter: zeroed at init, +1 every update; never read
	f32 mProgress;      // _18, flight progress 0..1, fighterWork._A0 / _A8
	u32 mTimeLeft;      // _1C, time left in seconds, fighterWork._BC / 60; 6000 at init
	s32 mKindPoints[2]; // _20, points per target kind, indexed by Mato::_2C; words 0x24 and 0x28 of the fighter params
	Mato* mDestroyed;   // _28, last target destroyed, until the script takes its id
	Mato mMatos[0x50];  // _2C, the targets, 0x50 slots, first _00 used
};

inline s32 Mato::GetPoints() const
{
	return mList->mKindPoints[mKind];
}

struct Shot {
	Shot() { }
	~Shot() { }

	void Start(Vec from, Vec to, Mato* item)
	{
		u32 i;
		mActive     = true;
		mStartPos   = from;
		mEndPos     = to;
		mPos        = from;
		mTarget     = item;
		mTimer      = 60;
		mVelocity.x = (mEndPos.x - mStartPos.x) / mTimer;
		mVelocity.y = (mEndPos.y - mStartPos.y) / mTimer;
		mVelocity.z = (mEndPos.z - mStartPos.z) / mTimer;
		for (i = 0; i < 10; i++) {
			mTrail[i] = mPos;
		}
	}
	void Update(u32 dt);

	u8 mActive;      // _00, shot in flight: set by Start, cleared when mTimer runs out, free slots are the ones with it false
	u8 _01[0x3];     // _01
	Vec mStartPos;   // _04, where the shot starts (Start's from); mPos begins here
	Vec mEndPos;     // _10, where the shot ends (Start's to); mPos reaches it after mTimer frames
	Vec mPos;        // _1C, current position, moved by mVelocity * dt each update and drawn as the shot's head
	Vec mVelocity;   // _28, step per frame from mStartPos to mEndPos
	s32 mTimer;      // _34, frames left in flight, starts at 60; the shot ends when it reaches 0
	Vec mTrail[10];  // _38, last 10 positions, newest first, drawn as fading circles behind the shot
	Mato* mTarget;   // _B0, the Mato the shot was fired at (FighterAim mTarget at firing), NULL when nothing was locked on
	CStatus mResult; // _B4, result of the shot when it ends: bit 1 = it had a target (hit), bit 2 = no target (miss); read and cleared each
	                 // ShotList_Update
};

// where the player is aiming and what's under it
struct FighterAim {
	FighterAim() { mTarget = NULL; }

	Vec mPos; // _00, aim point in the fighter's local space (stage params x, y, z / 100, default 0, 20, 170); transformed by the fighter
	          // matrix to find targets and fire
	f32 mScreenX;  // _0C, screen x of mPos from GXProject, used as the reticle centre when there is no target
	f32 mScreenY;  // _10, screen y of mPos from GXProject, used as the reticle centre when there is no target
	f32 mScreenZ;  // _14, screen z (depth) of mPos, written by GXProject next to mScreenX/mScreenY; never read
	Mato* mTarget; // _18, the Mato locked on: the nearest live one within range of the aim point, NULL when none
};

struct ShotList {
	void Add(Vec from, Vec to, Mato* item)
	{
		u32 i;
		for (i = 0; i < 5; i++) {
			if (!mShots[i].mActive) {
				break;
			}
		}
		if (i < 5) {
			mShots[i].Start(from, to, item);
			PlayGameSound(&gGame.mSound, 11, 0xFF);
		}
	}

	Shot mShots[5];  // _00, the 5 shot slots
	s32 mHitStreak;  // _3AC, hits in a row; at 5 it raises shot event bit 4 (GetShotEvent 3) and resets
	s32 mMissStreak; // _3B0, misses in a row; at 5 it raises shot event bit 8 (GetShotEvent 4) and resets
};

struct Fighter {
	~Fighter() { }

	XObj* mBody;  // _00, first object passed to StartFighter, drawn; its jobj takes mBodyMtx and the player is seated on its joint {0};
	              // NULL while no fighter is running
	XObj* mPath;  // _04, hidden object whose animation is the flight: its joint {0,0} gives mBodyMtx, its end frame is mEndFrame,
	              // mFrame is played on it
	XObj* mExtra; // _08, optional third object from StartFighter (may be NULL), placed at mBodyMtx moved by (0, 4.11, -1)
	              // through mExtraMtx; what it shows is unknown
	Mtx mBodyMtx; // _0C, world matrix of the fighter: mPath's joint {0,0} scaled by 0.5, copied to mBody's jobj; aim point, shot direction
	              // and camera use it
	Mtx mPlayerMtx;  // _3C, the player's matrix: mBody's joint {0} * trans(0,-1.89,-7.16) * rotY(mPlayerRotY), copied to the player's jobj
	                 // (first object of XOBJS pool 2)
	Mtx mExtraMtx;   // _6C, matrix of the optional object mExtra: mBodyMtx moved by (0, 4.11, -1); only built when mExtra is set
	f32 mPlayerRotY; // _9C, player's turn around Y in radians, -80 degrees times the stick's x
	f32 mFrame; // _A0, current frame of the flight animation on mPath, advanced by mSpeed times the rate table; also drives the pilot and
	            // wing
	f32 mPrevFrame;  // _A4, mFrame before this update; sound cues fire for frames in (mPrevFrame, mFrame]
	f32 mEndFrame;   // _A8, end frame of mPath's animation; mFrame / mEndFrame is the progress
	f32 mSpeed;      // _AC, frames advanced per update before the rate table: grows by mAccel per second up to mMaxSpeed, each shot takes
	                 // mFireSlowdown down to mMinSpeed; starts at 1
	u8* mSpeedRates; // _B0, script table of percent rates indexed by frame, clamped to 1..2.5 times; NULL means rate 0
	u8* mParams;   // _B4, script parameter block of little endian words: aim point x/y/z, lock range, then min/max speed, accel, slowdown,
	               // time (all /100 except time)
	u32 _B8;       // _B8, set to 0 in Fighter_Init, never read
	u32 mTimeLeft; // _BC, frames left, starts at mTimeLimit and counts down by dt; GetTimeLeft reports it in seconds
	u8 _C0[0x8];   // _C0
	Vec mPos;      // _C8, position of the fighter (origin of mBodyMtx), written each update but never read
	u8 _D4[0xC];   // _D4
	f32 mMinSpeed; // _E0, floor for mSpeed after a shot slows it, param word 0x10 / 100
	f32 mMaxSpeed; // _E4, cap for mSpeed while it accelerates, param word 0x14 / 100
	f32 mAccel;    // _E8, mSpeed gained per second, param word 0x18 / 100
	f32 mFireSlowdown; // _EC, mSpeed lost per shot, param word 0x1C / 100
	s32 mTimeLimit;    // _F0, time limit in frames: param word 0x20 seconds * 60 + 59; mTimeLeft starts from it
	FighterAim mAim;   // _F4, aim point in fighter space and the target locked under it
	Mato* mLastTarget; // _110, compared with a newly locked target to play the lock sound only on a change; only ever set to NULL, so the
	                   // test always passes
	ShotList mShots;   // _114, the five shots in flight
};

struct PilotNwing {
	XObj* mCarriedObj;   // _00, first object from CmdRegisterFighterLink, drawn (status 0x10000 set); its jobj gets mCarriedMtx, the
	                     // joint 0/0 of mPathObj at half scale
	XObj* mPathObj;      // _04, hidden object (XObj status 0x10000 'drawn' cleared) whose animation 0 is played at mAnimFrame; its joint
	                     // 0/0 carries mCarriedObj
	XObj* mAttachedObj0; // _08, optional third object, drawn; placed at (0, -1.89, -1) from mCarriedMtx
	XObj* mAttachedObj1; // _0C, optional fourth object, drawn; placed at (0, -1.89, -7.16) from mCarriedMtx, the offset Fighter_Update
	                     // seats the player at
	Mtx mCarriedMtx;     // _10, mPathObj's joint 0/0 matrix scaled by 0.5, copied to mCarriedObj and the base for both attachments
	Mtx mAttachedMtx0;   // _40, mCarriedMtx moved by (0, -1.89, -1), copied to mAttachedObj0
	Mtx mAttachedMtx1;   // _70, mCarriedMtx moved by (0, -1.89, -7.16), copied to mAttachedObj1
	f32 mAnimFrame;      // _A0, frame mPathObj's animation is set to, taken from the fighter's frame (fighterWork._A0) each update
	f32 _A4; // _A4, set to 1.0f in InitPilotNwing and never read; Fighter's matching field (_AC, also 1.0f at init) is its frame step
};

struct FighterPanels {
	s32 mLowerState;  // _00, lower panel open/close state: 0 idle, 1 opening, 2 closing; back to 0 when mLowerOpen reaches 1 or 0
	s32 mCenterState; // _04, center panel open/close state: 0 idle, 1 opening, 2 closing; back to 0 when mCenterOpen reaches 1 or 0
	f32 mLowerOpen;   // _08, how far the lower panel is open, 0..1; scales its drawn height, nothing is drawn at 0
	f32 mCenterOpen;  // _0C, how far the center panel is open, 0..1; scales its drawn height, nothing is drawn at 0
	s32 mLowerFrame;  // _10, lower panel animation frame 0..8, picks texture 25 + frame; restarts at 0 when the panel is shown
	s32 mCenterFrame; // _14, center panel animation frame, 0..5 for variant 0 (textures 12+) or 0..6 otherwise (textures 18+); restarts at
	                  // 0 when shown
	s32 mLowerFrameTimer;  // _18, frames until mLowerFrame advances, reloaded with 6
	s32 mCenterFrameTimer; // _1C, frames until mCenterFrame advances, reloaded with 6
	s32 mCenterVariant;    // _20, which center panel animation plays, the script's second argument: 0 uses the 6 textures from 12, anything
	                       // else the 7 from 18
};

// shadow offsets for the hud captions
f32 captionShadowX[4] = { -2.0f, 2.0f, 2.0f, -2.0f };
f32 captionShadowY[4] = { -2.0f, -2.0f, 2.0f, 2.0f };

u8 lbl_5_bss_0;
CStatus shotEvents;
f32 reticleSpin[10]; // the reticle's last 10 spin phases for the spoke trail, [0] newest; FighterAim_Draw writes [0] = 0
                     // every draw, so they only trail zero
extern fighter::CCameraActionFighter fighterCamera;
extern MatoList matoList;
extern Fighter fighterWork;
extern PilotNwing pilotNwing;
extern FighterPanels panels;
extern s32 fireHoldFrames;
extern u8* soundCueTable[2];

extern "C" void DrawScoreHud(u32 score, u32 time, u32 gauge);
extern "C" void Fighter_Fire(Fighter* work, s32 batteryCost);
extern "C" void Fighter_Update(Fighter* work, u32 dt);
extern "C" void PilotNwing_Update(PilotNwing* link, u32 dt, f32 animFrame);
extern "C" void Fighter_Init(Fighter* work, XObj* body, XObj* path, XObj* extra);
extern "C" void FighterPanels_Draw(FighterPanels* fade);
extern "C" void FighterPanels_Update(FighterPanels* fade, u32 dt);
extern "C" void FighterAim_Draw(FighterAim* aim, CCamera* camera, Mtx fighterMtx);
extern "C" void ShotList_Update(ShotList* shots, u32 dt);

/**
 * @note Address: stage02 .text 0x0
 * @note Size: 0xC
 */
// inline

/**
 * @note Address: stage02 .text 0xC
 * @note Size: 0x98
 */
// generated dtor

/**
 * @note Address: stage02 .text 0xA4
 * @note Size: 0x8
 */
// inline

/**
 * @note Address: stage02 .text 0xAC
 * @note Size: 0x8
 */
// inline

/**
 * @note Address: stage02 .text 0xB4
 * @note Size: 0xC
 */
// inline

/**
 * @note Address: stage02 .text 0xC0
 * @note Size: 0x4
 */
// inline

/**
 * @note Address: stage02 .text 0xC4
 * @note Size: 0x4
 */
// inline

/**
 * @note Address: stage02 .text 0xC8
 * @note Size: 0x4
 */
// inline

/**
 * @note Address: stage02 .text 0xCC
 * @note Size: 0x4
 */
// inline

/**
 * @note Address: stage02 .text 0xD0
 * @note Size: 0x4
 */
// inline

/**
 * @note Address: stage02 .text 0xD4
 * @note Size: 0x4
 */
// inline

/**
 * @note Address: stage02 .text 0xD8
 * @note Size: 0x38
 */
// SetEye

/**
 * @note Address: stage02 .text 0x110
 * @note Size: 0x10
 */
// inline

/**
 * @note Address: stage02 .text 0x120
 * @note Size: 0x38
 */
// SetInterest

/**
 * @note Address: stage02 .text 0x158
 * @note Size: 0x10
 */
// inline

/**
 * @note Address: stage02 .text 0x168
 * @note Size: 0x38
 */
// CInterfaceCameraAction::SetAngle(const Vec&)

/**
 * @note Address: stage02 .text 0x1A0
 * @note Size: 0x10
 */
// inline

/**
 * @note Address: stage02 .text 0x1B0
 * @note Size: 0x8
 */
// inline

/**
 * @note Address: stage02 .text 0x1B8
 * @note Size: 0x8
 */
// inline

/**
 * @note Address: stage02 .text 0x1C0
 * @note Size: 0x8
 */
// inline

/**
 * @note Address: stage02 .text 0x1C8
 * @note Size: 0x8
 */
// inline

/**
 * @note Address: stage02 .text 0x1D0
 * @note Size: 0x8
 */
// inline

/**
 * @note Address: stage02 .text 0x1D8
 * @note Size: 0x16D4
 */
// draws a caption with a black shadow on every side
inline void DrawCaption(s32 id, f32 x, f32 y, f32 scale)
{
	s32 i;
	f32 cx;
	f32 cy;

	DrawGsQuad(&gGame.mGs, 0xFF, 0xFF, 0xFF, 0xFF, x, y, 0.0f,
	           scale * (MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(id), 1, 20.0f) - 4.0f), 12.0f, 0.0f, 0.0f, 0.0f);
	cx = x - MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(id), 1, 20.0f) / 2.0f;
	cy = y - 13.0f;
	for (i = 0; i < 4; i++) {
		DrawGsProportionalTextFormat(&gGame.mGs, 0xFF, GetLocalizedInterfaceText(id), cx + captionShadowX[i], cy + captionShadowY[i], 0.0f,
		                             20.0f, 20.0f);
	}
	DrawGsProportionalTextFormat(&gGame.mGs, -1, GetLocalizedInterfaceText(id), cx, cy, 0.0f, 20.0f, 20.0f);
}

extern "C" void DrawScoreHud(u32 score, u32 time, u32 gauge)
{
	CTexObj tex;
	f32 scale = 1.0f;
	f32 alpha = 255.0f;
	s32 i;
	s32 d;
	f32 x;
	f32 left;
	f32 digW;
	f32 digH;
	f32 w;
	u32 base;
	s32 hi;
	s32 lo;
	u16 tw;
	u16 th;
	s32 stage;
	s32 stage02;

	if (time >= 100) {
		time = 99;
	}
	stage   = gGame.mStage;
	stage02 = FindStageIndexByName("stage02");
	if (stage02 == stage) {
		for (i = 0; i < 48; i++) {
			XObj* obj = gGame.mXObjs.mPools[5].Get(i);
			if (obj->GetKind() != 0x149) {
				continue;
			}
			CTpl& tpl = static_cast<XObjActor3*>(obj)->mPanelTpl;
			if (!tpl.mBound) {
				break;
			}

			// score
			tex = tpl.mTex.Init(&static_cast<TPLPalette*>(tpl.mBuffer)->descriptorArray[3], 0);
			tw  = tex.GetWidth();
			w   = scale * tw;
			th  = tex.GetHeight();
			x   = 640.0f - w / 2.0f - 40.0f;
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, x, 40.0f, 0.0f, w, scale * th, 0.0f, 0.0f, 0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
#if !defined(VERSION_JP)
			DrawCaption(0x5A, x, 40.0f, scale);
#endif
			tex  = tpl.mTex.Init(&static_cast<TPLPalette*>(tpl.mBuffer)->descriptorArray[1], 0);
			left = 20.0f + (x - 12.0f - 88.0f);
			tw   = tex.GetWidth();
			digW = 0.8f * (scale * (tw / 11));
			th   = tex.GetHeight();
			digH = 0.8f * (scale * th);
			PSMTXScale(tex.mMtx[1], 0.09090909f, 1.0f, 1.0f);
			tex.mMtxDirty = 1;
			d             = 0;
			base          = alpha;
			hi            = base | 0xFAE0CF00;
			lo            = base | 0xE9813A00;
			for (; d < 4; d++) {
				f32 div = 1.0f;
				s32 k;
				s32 digit;
				if (d > 0) {
					for (k = 0; k < d; k++) {
						div *= 10.0f;
					}
				}
				digit = score / div;
				if (d == 0 || digit != 0) {
					digit %= 10;
					PSMTXTrans(tex.mMtx[0], digit * 0.09090909f, 0.0f, 0.0f);
					tex.mMtxDirty = 1;
					SetGsTevMode(gGame.mGs.mCurTevStage, 0);
					DrawGsTexturedQuad(&gGame.mGs, &tex, hi, hi, lo, lo, 7.0f * digW + (left - d * digW) - 32.0f, 68.0f, 0.0f, digW, digH,
					                   0.0f, 0.0f, 0.0f);
					SetGsTevMode(gGame.mGs.mCurTevStage, 4);
				}
			}

			// time
			tex = tpl.mTex.Init(&static_cast<TPLPalette*>(tpl.mBuffer)->descriptorArray[4], 0);
			tw  = tex.GetWidth();
			th  = tex.GetHeight();
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, 320.0f, 40.0f, 0.0f, scale * tw, scale * th, 0.0f, 0.0f, 0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
#if !defined(VERSION_JP)
			DrawCaption(0x5B, 320.0f, 40.0f, scale);
#endif
			tex  = tpl.mTex.Init(&static_cast<TPLPalette*>(tpl.mBuffer)->descriptorArray[1], 0);
			tw   = tex.GetWidth();
			digW = 1.4f * (scale * (tw / 11));
			th   = tex.GetHeight();
			digH = 1.4f * (scale * th);
#if defined(VERSION_JP)
			left = 110.0f;
#else
			left = 110.0f + 11.0f;
#endif
			PSMTXScale(tex.mMtx[1], 0.09090909f, 1.0f, 1.0f);
			d             = 0;
			base          = alpha;
			hi            = base | 0x1F33FF00;
			lo            = base | 0xFFFFFF00;
			tex.mMtxDirty = 1;
			for (; d < 2; d++) {
				f32 div = 1.0f;
				s32 k;
				s32 digit;
				if (d > 0) {
					for (k = 0; k < d; k++) {
						div *= 10.0f;
					}
				}
				digit = time / div;
				if (d == 0 || digit != 0) {
					digit %= 10;
					PSMTXTrans(tex.mMtx[0], digit * 0.09090909f, 0.0f, 0.0f);
					tex.mMtxDirty = 1;
					SetGsTevMode(gGame.mGs.mCurTevStage, 0);
					DrawGsTexturedQuad(&gGame.mGs, &tex, lo, lo, hi, hi, 7.0f * digW + (left - d * (digW - 8.0f)) - 32.0f, 76.0f, 0.0f,
					                   digW, digH, 0.0f, 0.0f, 0.0f);
					SetGsTevMode(gGame.mGs.mCurTevStage, 4);
				}
			}
			tw   = tex.GetWidth();
			digW = 1.4f * (scale * (tw / 11));
			th   = tex.GetHeight();
			digH = 1.4f * (scale * th);
#if defined(VERSION_JP)
			left = 110.0f;
#else
			left = 110.0f + 11.0f;
#endif
			PSMTXTrans(tex.mMtx[0], 0.90909094f, 0.0f, 0.0f);
			tex.mMtxDirty = 1;
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			s32 lead = 2;
			if (time < 10)
				lead = 1;
			DrawGsTexturedQuad(&gGame.mGs, &tex, lo, lo, hi, hi, 7.0f * digW + (left - lead * (digW - 8.0f)) - 32.0f, 76.0f, 0.0f, digW,
			                   digH, 0.0f, 0.0f, 0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);

			// gauge
			tex = tpl.mTex.Init(&static_cast<TPLPalette*>(tpl.mBuffer)->descriptorArray[5], 0);
			tw  = tex.GetWidth();
			th  = tex.GetHeight();
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, 100.0f, 44.0f, 0.0f, scale * tw, scale * th, 0.0f, 0.0f, 0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
#if !defined(VERSION_JP)
			DrawCaption(0x5C, 100.0f, 44.0f, scale);
#endif
			tex  = tpl.mTex.Init(&static_cast<TPLPalette*>(tpl.mBuffer)->descriptorArray[6], 0);
			tw   = tex.GetWidth();
			w    = scale * tw;
			th   = tex.GetHeight();
			digH = scale * th;
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, 100.0f, 72.0f, 0.0f, w, digH, 0.0f, 0.0f, 0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
			tex = tpl.mTex.Init(&static_cast<TPLPalette*>(tpl.mBuffer)->descriptorArray[7], 0);
			tw  = tex.GetWidth();
			th  = tex.GetHeight();
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, 100.0f - w / 2.0f + gauge * w / 100.0f, 72.0f, 0.0f, scale * tw,
			                   scale * th, 0.0f, 0.0f, 0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
			break;
		}
	}
}

// DrawScoreHud

/**
 * @note Address: stage02 .text 0x18AC
 * @note Size: 0x48
 */
// generated dtor

/**
 * @note Address: stage02 .text 0x18F4
 * @note Size: 0x244
 */
extern "C" void DrawFighterHud(void)
{
	MatoList* targets;
	s32 i;
	f32 projH;
	f32 projW;

	if (fighterWork.mBody == NULL) {
		return;
	}
	targets            = &matoList;
	targets->mAimPos.x = fighterWork.mAim.mPos.x;
	targets->mAimPos.y = fighterWork.mAim.mPos.y;
	targets->mAimPos.z = fighterWork.mAim.mPos.z;
	for (i = 0; i < targets->mCount; i++) {
		Mato* item = &targets->mMatos[i];
		XObj* obj  = item->mObj;
		f32 halfW;
		f32 height;
		if (obj == NULL || !(obj->mStatus.mFlags & 1) || item->mPending) {
			continue;
		}
		halfW = obj->GetFloorXZExtent() / 2.0f;
		if (halfW < 1.0f) {
			halfW = 1.0f;
		}
		height = item->mObj->GetCollisionHeight();
		if (height < 1.0f) {
			height = 1.0f;
		}
		if (item->mObj->ProjectVisibleBounds(&projW, &projH, NULL, halfW, height)) {
			item->mOnScreen = true;
			if (projW > projH) {
				projW = projH;
			}
			Vec zero = { 0.0f, 0.0f, 0.0f };
			item->mObj->ProjectOffsetToScreen(&zero, &item->mScreenX, &item->mScreenY, &item->mScreenZ, NULL);
			item->mScreenY -= projW / 2.0f; // raise the marker by half the smaller projected extent
		} else {
			item->mOnScreen = false;
		}
	}
	DrawScoreHud(matoList.mScore, matoList.mTimeLeft, 100.0f * matoList.mProgress);
	FighterAim_Draw(&fighterWork.mAim, &gGame.mMotionControlCamera.mCamera, fighterWork.mBodyMtx);
	FighterPanels_Draw(&panels);
}

// DrawFighterHud

/**
 * @note Address: stage02 .text 0x1B38
 * @note Size: 0x2FC
 */
extern "C" void DrawShots(void)
{
	u32 i;
	s32 j;

	if (fighterWork.mBody == NULL) {
		return;
	}
	Shot* entry = fighterWork.mShots.mShots;
	for (i = 0; i < 5; i++, entry++) {
		if (!entry->mActive) {
			continue;
		}
		gGame.mGs.mBillboardEnabled = 1; // the circles draw as billboards
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		for (j = 0; j < 10; j++) { // the trail as circles, older positions dimmer
			Vec* pos  = &entry->mTrail[j];
			Vec up    = { 0.0f, 1.0f, 0.0f };
			u32 alpha = 96.0f * (1.0f - 0.1f * j);
			DrawGsCircle(&gGame.mGs, 16, alpha | 0xFFFFFF00, 0xFFFFFF00, pos->x, pos->y, pos->z, 4.0f, 4.0f, up.x, up.y, up.z);
		}
		// the head: two white circles under a green one
		Vec* pos = &entry->mPos;
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 2, 4, 15);
		{
			Vec up = { 0.0f, 1.0f, 0.0f };
			DrawGsCircle(&gGame.mGs, 16, 0xFFFFFF80, 0xFFFFFF00, pos->x, pos->y, pos->z, 4.0f, 4.0f, up.x, up.y, up.z);
		}
		{
			Vec up = { 0.0f, 1.0f, 0.0f };
			DrawGsCircle(&gGame.mGs, 16, 0xFFFFFF80, 0xFFFFFF00, pos->x, pos->y, pos->z, 5.0f, 5.0f, up.x, up.y, up.z);
		}
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		{
			Vec up = { 0.0f, 1.0f, 0.0f };
			DrawGsCircle(&gGame.mGs, 16, 0x64FF64C8, 0x64FF6400, pos->x, pos->y, pos->z, 5.0f, 5.0f, up.x, up.y, up.z);
		}
		gGame.mGs.mBillboardEnabled = 0;
	}
}

// DrawShots

/**
 * @note Address: stage02 .text 0x1E34
 * @note Size: 0x10
 */
extern "C" void DrawFighterStage(void)
{
	if (fighterWork.mBody == NULL) {
		return;
	}
}

/**
 * @note Address: stage02 .text 0x1E44
 * @note Size: 0x514
 */
extern "C" void UpdateFighter(u32 dt)
{
	if (fighterWork.mBody == NULL) {
		return;
	}
	shotEvents.mFlags = 0;
	CPad pad(0);
	// every press fires a shot costing 1; releasing after holding A 20+ frames fires another costing 10
	if (pad.mPad->mButton & PAD_BUTTON_A) {
		if (pad.mPad->mTrigger & PAD_BUTTON_A) {
			Fighter_Fire(&fighterWork, 1);
		}
		fireHoldFrames++;
	} else {
		if (fireHoldFrames >= 20) {
			Fighter_Fire(&fighterWork, 10);
		}
		fireHoldFrames = 0;
	}
	fighterWork.mPlayerRotY = -80.0f * DEG2RAD * pad.mPad->mStickX;
	Fighter_Update(&fighterWork, dt);
	PilotNwing_Update(&pilotNwing, dt, fighterWork.mFrame);
	matoList.mFrameCount++;
	matoList.mProgress = fighterWork.mFrame / fighterWork.mEndFrame;
	matoList.mTimeLeft = fighterWork.mTimeLeft / 60;
	for (s32 i = 0; i < matoList.mCount; i++) {
		Mato* item = &matoList.mMatos[i];
		if (!item->mOnScreen) {
			item->mOnScreenTime = 0;
		} else {
			if (item->mOnScreenTime >= 15) {
				item->mMarkerColor = 0xFF0000C8;
			} else {
				item->mMarkerColor = 0xFFFFFF50;
			}
			item->mMarkerPhase += 1.0f;
		}
	}
	// re-lock the aim only once the flight is past its first percent and the lock-on sound 0x20006 isn't playing
	if (IsGameSoundPlaying(&gGame.mSound, 0x20006) == 0 && 0.01f < fighterWork.mFrame / fighterWork.mEndFrame) {
		Fighter* work = &fighterWork;
		Vec pos;
		Mato* best = NULL;
		f32 bestDist;
		work->mAim.mTarget = NULL;
		pos                = work->mAim.mPos;
		PSMTXMultVec(work->mBodyMtx, &pos, &pos);
		for (s32 i = 0; i < matoList.mCount; i++) { // the nearest live target within lock range
			Mato* item = &matoList.mMatos[i];
			f32 dist;
			f32 range;
			if (!(item->mObj->mStatus.mFlags & 0x10000) || item->mShotsLeft <= 0) {
				continue;
			}
			CVec targetPos = item->GetPos();
			dist           = PSVECDistance(&targetPos, &pos);
			// the lock range: param word 0xC / 100, default 75
			if (fighterWork.mParams == NULL) {
				range = 75.0f;
			} else {
				range = (s32)ReadResourceOffset(fighterWork.mParams + 0xC) / 100.0f;
			}
			if (range < dist) {
				continue;
			}
			if (best == NULL || dist < bestDist) {
				best     = item;
				bestDist = dist;
			}
		}
		if (best != NULL && !best->mPending) {
			if (work->mLastTarget != best) {
				PlayGameSound(&gGame.mSound, 0x20006, 0xFF); // the lock-on sound; mLastTarget is always NULL, so it plays on every lock
			}
			work->mAim.mTarget = best;
		}
	}
	FighterPanels_Update(&panels, dt);
	// cue records {frame, sound id}; a cue fires when the flight frame crosses its frame
	for (s32 i = 0; i < (s32)ReadResourceOffset(soundCueTable[0] + i * 8); i++) {
		s32 frame = ReadResourceOffset(soundCueTable[0] + i * 8);
		s32 id    = ReadResourceOffset(soundCueTable[0] + i * 8 + 4);
		if (fighterWork.mPrevFrame < (f32)frame && (f32)frame <= fighterWork.mFrame) {
			PlayGameSound(&gGame.mSound, id, 0xFF);
		}
	}
}

// UpdateFighter

/**
 * @note Address: stage02 .text 0x2358
 * @note Size: 0x48
 */
// generated dtor

/**
 * @note Address: stage02 .text 0x23A0
 * @note Size: 0x2C
 */
// inline

/**
 * @note Address: stage02 .text 0x23CC
 * @note Size: 0x48
 */
// generated dtor

/**
 * @note Address: stage02 .text 0x2414
 * @note Size: 0x3C
 */
extern "C" void ShowCenterPanel(bool on, s32 variant)
{
	if (on) {
		panels.mCenterState   = 1;
		panels.mCenterFrame   = 0;
		panels.mCenterVariant = variant;
	} else {
		panels.mCenterState = 2;
	}
}

/**
 * @note Address: stage02 .text 0x2450
 * @note Size: 0x30
 */
extern "C" void ShowLowerPanel(bool on)
{
	if (on) {
		panels.mLowerState = 1;
		panels.mLowerFrame = 0;
	} else {
		panels.mLowerState = 2;
	}
}

/**
 * @note Address: stage02 .text 0x2480
 * @note Size: 0xA0
 */
extern "C" void InitPilotNwing(XObj* carried, XObj* path, XObj* attached0, XObj* attached1)
{
	if (carried == NULL || path == NULL) {
		return;
	}
	pilotNwing.mCarriedObj   = carried;
	pilotNwing.mPathObj      = path;
	pilotNwing.mAttachedObj0 = attached0;
	pilotNwing.mAttachedObj1 = attached1;
	carried->mStatus.mFlags |= 0x10000;
	pilotNwing.mPathObj->mStatus.mFlags &= ~0x10000ULL;
	pilotNwing.mAttachedObj0->mStatus.mFlags |= 0x10000;
	pilotNwing.mAttachedObj1->mStatus.mFlags |= 0x10000;
	pilotNwing.mAnimFrame = 0.0f;
	pilotNwing._A4        = 1.0f;
}

/**
 * @note Address: stage02 .text 0x2520
 * @note Size: 0x50
 */
extern "C" s32 GetShotEvent(void)
{
	s32 ret   = 0;
	u32 flags = shotEvents.mFlags;
	// the pending shot event for the stage script: 1 a target destroyed,
	// 2 never raised (nothing sets bit 2), 3 five-hit streak, 4 five-miss streak
	if (flags & 4) {
		ret = 3;
	} else if (flags & 8) {
		ret = 4;
	} else if (flags & 1) {
		ret = 1;
	} else if (flags & 2) {
		ret = 2;
	}
	return ret;
}

/**
 * @note Address: stage02 .text 0x2570
 * @note Size: 0x448
 */
extern "C" void EndFighter(void)
{
	if (fighterWork.mBody == NULL) {
		return;
	}
	gGame.GetPlayer()->RestoreControl();
	HSD_JObjClearMtxFlags(gGame.GetPlayer()->mJObj);
	// back to the game's own camera action, and stop sound 0x20004 if it is still playing
	gGame.mMotionControlCamera.ChangeAction(gGame.mMotionControlCamera.mPrevAction);
	if (IsGameSoundPlaying(&gGame.mSound, 0x20004)) {
		StopGameSfx(&gGame.mSound, 0x20004);
	}
	if (fighterWork.mBody != NULL) {
		HSD_JObjClearMtxFlags(fighterWork.mBody->mJObj);
		if (fighterWork.mExtra != NULL) {
			HSD_JObjClearMtxFlags(fighterWork.mExtra->mJObj);
		}
	}
	HSD_JObjClearMtxFlags(pilotNwing.mCarriedObj->mJObj);
	if (pilotNwing.mAttachedObj0 != NULL) {
		HSD_JObjClearMtxFlags(pilotNwing.mAttachedObj0->mJObj);
	}
	if (pilotNwing.mAttachedObj1 != NULL) {
		HSD_JObjClearMtxFlags(pilotNwing.mAttachedObj1->mJObj);
	}
	fighterWork.mBody = fighterWork.mPath = fighterWork.mExtra = NULL;
	pilotNwing.mCarriedObj = pilotNwing.mPathObj = NULL;
}

// EndFighter

/**
 * @note Address: stage02 .text 0x29B8
 * @note Size: 0x2C
 */
extern "C" s32 TakeDestroyedMatoId(void)
{
	s32 ret             = matoList.mDestroyed != NULL ? matoList.mDestroyed->mId : -1;
	matoList.mDestroyed = NULL;
	return ret;
}

/**
 * @note Address: stage02 .text 0x29E4
 * @note Size: 0x10
 */
extern "C" s32 GetScore(void)
{
	return matoList.mScore;
}

/**
 * @note Address: stage02 .text 0x29F4
 * @note Size: 0x10
 */
extern "C" u32 GetTimeLeft(void)
{
	return matoList.mTimeLeft;
}

/**
 * @note Address: stage02 .text 0x2A04
 * @note Size: 0x10
 */
extern "C" f32 GetFighterProgress(void)
{
	return matoList.mProgress;
}

/**
 * @note Address: stage02 .text 0x2A14
 * @note Size: 0xC
 */
extern "C" void SetSoundCueTable(u8* data)
{
	soundCueTable[0] = data;
}

/**
 * @note Address: stage02 .text 0x2A20
 * @note Size: 0x1D4
 */
extern "C" void SetFighterParams(u8* data)
{
	fighterWork.mParams       = data;
	fighterWork.mMinSpeed     = (s32)ReadResourceOffset(data + 0x10) / 100.0f;
	fighterWork.mMaxSpeed     = (s32)ReadResourceOffset(data + 0x14) / 100.0f;
	fighterWork.mAccel        = (s32)ReadResourceOffset(data + 0x18) / 100.0f;
	fighterWork.mFireSlowdown = (s32)ReadResourceOffset(data + 0x1C) / 100.0f;
	fighterWork.mTimeLimit    = ReadResourceOffset(data + 0x20) * 60 + 59;
	matoList.mKindPoints[0]   = ReadResourceOffset(data + 0x24);
	matoList.mKindPoints[1]   = ReadResourceOffset(data + 0x28);
}

/**
 * @note Address: stage02 .text 0x2BF4
 * @note Size: 0x10
 */
extern "C" void SetSpeedRateTable(u8* rates)
{
	fighterWork.mSpeedRates = rates;
}

/**
 * @note Address: stage02 .text 0x2C04
 * @note Size: 0x108
 */
extern "C" void AddMato(XObj* obj, s32 id)
{
	Mato* item;
	s32 kind;

	if (matoList.mCount >= ARRAY_SIZE(matoList.mMatos)) {
		return;
	}
	item                = &matoList.mMatos[matoList.mCount++];
	item->mObj          = obj;
	item->mId           = id;
	item->mOnScreenTime = 0;
	item->mPending      = false;
	item->mMarkerPhase  = HSD_Rand();
	item->mObj->mStatus.mFlags |= 0x10000;
	kind = item->mObj->GetKind();
	switch (kind) {
	case 0x14D: // the one-shot targets
		item->mShotsLeft = 1;
		item->mHitsLeft  = 1;
		item->mKind      = 0;
		break;
	case 0x2EA: // the three-hit targets
		item->mShotsLeft = 3;
		item->mHitsLeft  = 3;
		item->mKind      = 1;
		break;
	}
	item->mList = &matoList;
}

// AddMato

/**
 * @note Address: stage02 .text 0x2D0C
 * @note Size: 0x1BC
 */
extern "C" void StartFighter(XObj* body, XObj* path, XObj* extra)
{
	if (body == NULL || path == NULL) {
		return;
	}
	shotEvents.mFlags = 0;
	Fighter_Init(&fighterWork, body, path, extra);
	matoList.mCount      = 0;
	matoList.mScore      = 0;
	matoList.mFrameCount = 0;
	matoList.mProgress   = 0.0f;
	matoList.mTimeLeft   = 6000;
	matoList.mDestroyed  = NULL;
	gGame.GetPlayer()->LockControl();
	gGame.mMotionControlCamera.ChangeAction(&fighterCamera);
	fighterCamera.SetView(MakeVec(0.0f, 40.0f, -180.0f), MakeVec(0.0f, 0.0f, 120.0f), fighterWork.mBodyMtx);
	fireHoldFrames           = 0;
	lbl_5_bss_0              = 0;
	panels.mLowerState       = 0;
	panels.mCenterState      = 0;
	panels.mLowerOpen        = 0.0f;
	panels.mCenterOpen       = 0.0f;
	panels.mLowerFrame       = 0;
	panels.mCenterFrame      = 0;
	panels.mLowerFrameTimer  = 6;
	panels.mCenterFrameTimer = 6;
	panels.mCenterVariant    = 0;
}

// StartFighter

/**
 * @note Address: stage02 .text 0x2EC8
 * @note Size: 0x2C
 */
extern "C" void ClearFighterObjects(void)
{
	fighterWork.mBody = fighterWork.mPath = fighterWork.mExtra = NULL;
	pilotNwing.mCarriedObj = pilotNwing.mPathObj = NULL;
}

/**
 * @note Address: stage02 .text 0x2EF4
 * @note Size: 0x448
 */
extern "C" void PilotNwing_Update(PilotNwing* link, u32 dt, f32 animFrame)
{
	Mtx scale;
	Mtx attach;
	HSD_JObj* jobj;

	PSMTXScale(scale, 0.5f, 0.5f, 0.5f);
	jobj = GetJointByPath(link->mPathObj->mJObj, (s32[2]) { 0, 0 }, 2);
	PSMTXConcat(HSD_JObjGetMtxPtr(jobj), scale, link->mCarriedMtx);
	HSD_JObjCopyMtx(link->mCarriedObj->mJObj, link->mCarriedMtx);
	link->mAnimFrame = animFrame;
	link->mPathObj->SetAnim(0, TRUE, NULL, NULL, NULL, link->mAnimFrame, 1.0f, 0.0f, 10.0f);
	if (link->mAttachedObj0 != NULL) {
		PSMTXTrans(attach, 0.0f, -1.89f, -1.0f);
		PSMTXConcat(link->mCarriedMtx, attach, link->mAttachedMtx0);
		HSD_JObjCopyMtx(link->mAttachedObj0->mJObj, link->mAttachedMtx0);
	}
	if (link->mAttachedObj1 != NULL) {
		PSMTXTrans(attach, 0.0f, -1.89f, -7.16f);
		PSMTXConcat(link->mCarriedMtx, attach, link->mAttachedMtx1);
		HSD_JObjCopyMtx(link->mAttachedObj1->mJObj, link->mAttachedMtx1);
	}
}

// PilotNwing_Update

/**
 * @note Address: stage02 .text 0x333C
 * @note Size: 0x874
 */
extern "C" void Fighter_Fire(Fighter* work, s32 batteryCost)
{
	Mato* item = work->mAim.mTarget;
	Vec offset = { 0.0f, 10.0f, 0.0f };
	Vec origin;

	PSMTXMultVec(work->mBodyMtx, &offset, &origin);
	if (item == NULL) {
		Vec destination;
		PSMTXMultVec(work->mBodyMtx, &work->mAim.mPos, &destination);
		work->mShots.Add(origin, destination, NULL);
	} else {
		item->mShotsLeft--;
		if (item->mShotsLeft <= 0) {
			item->mPending = true;
		}
		work->mLastTarget = NULL;
		work->mShots.Add(origin, item->GetPos(), item);
	}
	Params& params = gGame.GetPlayer()->mParams;
	params.SetBattery(params.mBattery - batteryCost);
	work->mSpeed -= work->mFireSlowdown;
	if (work->mSpeed < work->mMinSpeed) {
		work->mSpeed = work->mMinSpeed;
	}
}

// Fighter_Fire

/**
 * @note Address: stage02 .text 0x3BB0
 * @note Size: 0x664
 */
extern "C" void Fighter_Update(Fighter* work, u32 dt)
{
	Mtx rot;
	Mtx scale;
	Mtx offset;
	Vec zero;
	HSD_JObj* jobj;
	f32 rate;

	work->mTimeLeft -= dt;
	PSMTXScale(scale, 0.5f, 0.5f, 0.5f);
	jobj = GetJointByPath(work->mPath->mJObj, (s32[2]) { 0, 0 }, 2);
	PSMTXConcat(HSD_JObjGetMtxPtr(jobj), scale, work->mBodyMtx);
	HSD_JObjCopyMtx(work->mBody->mJObj, work->mBodyMtx);
	zero.x = zero.y = zero.z = 0.0f;
	PSMTXMultVec(work->mBodyMtx, &zero, &work->mPos);
	PSMTXRotRad(rot, 'Y', work->mPlayerRotY);
	PSMTXTrans(offset, 0.0f, -1.89f, -7.16f);
	PSMTXConcat(offset, rot, offset);
	jobj = GetJointByPath(work->mBody->mJObj, (s32[1]) { 0 }, 1);
	PSMTXConcat(HSD_JObjGetMtxPtr(jobj), offset, work->mPlayerMtx);
	HSD_JObjCopyMtx(gGame.GetPlayer()->mJObj, work->mPlayerMtx);
	if (work->mExtra != NULL) {
		PSMTXTrans(offset, 0.0f, 4.11f, -1.0f);
		PSMTXConcat(work->mBodyMtx, offset, work->mExtraMtx);
		HSD_JObjCopyMtx(work->mExtra->mJObj, work->mExtraMtx);
	}
	work->mSpeed += dt * work->mAccel / 60.0f;
	if (work->mMaxSpeed < work->mSpeed) {
		work->mSpeed = work->mMaxSpeed;
	}
	work->mPrevFrame = work->mFrame;
	if (work->mSpeedRates == NULL) {
		rate = 0.0f;
	} else {
		// the script's percent rate for this frame: non-positive becomes 1, capped at 2.5
		rate = work->mSpeedRates[(s32)work->mFrame] / 100.0f;
		if (rate <= 0.0f) {
			rate = 1.0f;
		}
		if (2.5f < rate) {
			rate = 2.5f;
		}
	}
	work->mFrame += work->mSpeed * rate;
	work->mPath->SetAnim(0, TRUE, NULL, NULL, NULL, work->mFrame, 1.0f, 0.0f, 10.0f);
	ShotList_Update(&work->mShots, dt);
}

// Fighter_Update

/**
 * @note Address: stage02 .text 0x4214
 * @note Size: 0x2DC
 */
extern "C" void Fighter_Init(Fighter* work, XObj* body, XObj* path, XObj* extra)
{
	HSD_AObj* pathAnim;
	f32 aimX;
	f32 aimY;
	f32 aimZ;
	u32 i;

	work->mBody  = body;
	work->mPath  = path;
	work->mExtra = extra;
	work->mBody->mStatus.mFlags |= 0x10000;
	work->mBody->mStatus.mFlags |= 0x40000;
	work->mPath->mStatus.mFlags &= ~0x10000ULL;
	work->mFrame       = 0.0f;
	pathAnim           = GetAObj(work->mPath->mJObj);
	work->mEndFrame    = pathAnim != NULL ? HSD_AObjGetEndFrame(pathAnim) : 0.0f;
	work->mSpeed       = 1.0f;
	work->_B8          = 0;
	work->mTimeLeft    = work->mTimeLimit;
	aimZ               = work->mParams == NULL ? 170.0f : (s32)ReadResourceOffset(work->mParams + 8) / 100.0f;
	aimY               = work->mParams == NULL ? 20.0f : (s32)ReadResourceOffset(work->mParams + 4) / 100.0f;
	aimX               = work->mParams == NULL ? 0.0f : (s32)ReadResourceOffset(work->mParams) / 100.0f;
	work->mAim.mPos.x  = aimX;
	work->mAim.mPos.y  = aimY;
	work->mAim.mPos.z  = aimZ;
	work->mAim.mTarget = NULL;
	work->mLastTarget  = NULL;
	for (i = 0; i < 5; i++) {
		work->mShots.mShots[i].mActive        = false;
		work->mShots.mShots[i].mResult.mFlags = 0;
	}
	work->mShots.mHitStreak = work->mShots.mMissStreak = 0;
}

// Fighter_Init

/**
 * @note Address: stage02 .text 0x44F0
 * @note Size: 0xCB4
 */
extern "C" void FighterPanels_Draw(FighterPanels* fade)
{
	CTexObj tex;
	f32 scale = 1.0f;
	s32 i;
	u16 tw;
	u16 th;
	s32 stage;
	s32 stage02;

	stage   = gGame.mStage;
	stage02 = FindStageIndexByName("stage02");
	if (stage02 == stage) {
		for (i = 0; i < 48; i++) {
			XObj* obj = gGame.mXObjs.mPools[5].Get(i);
			f32 y;
			f32 half;
			f32 top;
			f32 w;
			f32 h;
			// Kind 0x149 carries the stage02 panel textures.
			if (obj->GetKind() != 0x149) {
				continue;
			}
			CTpl& tpl = static_cast<XObjActor3*>(obj)->mPanelTpl;
			if (!tpl.mBound) {
				continue;
			}
			if (fade->mLowerOpen > 0.0f) {
				// the open panel: a fill, the current animation frame, and the same strip above and below the opening
				tex  = tpl.mTex.Init(&static_cast<TPLPalette*>(tpl.mBuffer)->descriptorArray[10], 0);
				tw   = tex.GetWidth();
				w    = 1.1f * (scale * tw);
				th   = tex.GetHeight();
				half = fade->mLowerOpen * (scale * th);
				y    = 320.0f;
#if !defined(VERSION_JP)
				y -= 32.0f;
#endif
				SetGsTevMode(gGame.mGs.mCurTevStage, 0);
				DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, 100.0f, y, 0.0f, w, half, 0.0f, 0.0f, 0.0f);
				SetGsTevMode(gGame.mGs.mCurTevStage, 4);
				tex = tpl.mTex.Init(&static_cast<TPLPalette*>(tpl.mBuffer)->descriptorArray[fade->mLowerFrame + 25], 0);
				tw  = tex.GetWidth();
				w   = 1.1f * (scale * tw);
				th  = tex.GetHeight();
				h   = fade->mLowerOpen * (scale * th);
				SetGsTevMode(gGame.mGs.mCurTevStage, 0);
				DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, 100.0f, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
				SetGsTevMode(gGame.mGs.mCurTevStage, 4);
				tex = tpl.mTex.Init(&static_cast<TPLPalette*>(tpl.mBuffer)->descriptorArray[11], 0);
				half *= 0.5f;
				top = y;
				top -= half;
				tw = tex.GetWidth();
				w  = 1.1f * (scale * tw);
				th = tex.GetHeight();
				h  = scale * th;
				SetGsTevMode(gGame.mGs.mCurTevStage, 0);
				DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, 100.0f, top, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
				SetGsTevMode(gGame.mGs.mCurTevStage, 4);
				SetGsTevMode(gGame.mGs.mCurTevStage, 0);
				DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, 100.0f, y + half, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
				SetGsTevMode(gGame.mGs.mCurTevStage, 4);
			}
			if (fade->mCenterOpen > 0.0f) {
				// the open panel: a fill, the current animation frame, and the same strip above and below the opening
				tex  = tpl.mTex.Init(&static_cast<TPLPalette*>(tpl.mBuffer)->descriptorArray[8], 0);
				tw   = tex.GetWidth();
				th   = tex.GetHeight();
				half = fade->mCenterOpen * (scale * th);
				SetGsTevMode(gGame.mGs.mCurTevStage, 0);
				DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, 100.0f, 180.0f, 0.0f, 1.1f * (scale * tw), half, 0.0f, 0.0f, 0.0f);
				SetGsTevMode(gGame.mGs.mCurTevStage, 4);
				if (fade->mCenterVariant == 0) {
					tex = tpl.mTex.Init(&static_cast<TPLPalette*>(tpl.mBuffer)->descriptorArray[fade->mCenterFrame + 12], 0);
				} else {
					tex = tpl.mTex.Init(&static_cast<TPLPalette*>(tpl.mBuffer)->descriptorArray[fade->mCenterFrame + 18], 0);
				}
				tw = tex.GetWidth();
				w  = 1.1f * (scale * tw);
				th = tex.GetHeight();
				h  = fade->mCenterOpen * (scale * th);
				SetGsTevMode(gGame.mGs.mCurTevStage, 0);
				DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, 100.0f, 180.0f, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
				SetGsTevMode(gGame.mGs.mCurTevStage, 4);
				tex = tpl.mTex.Init(&static_cast<TPLPalette*>(tpl.mBuffer)->descriptorArray[9], 0);
				half *= 0.5f;
				top = 180.0f;
				top -= half;
				tw = tex.GetWidth();
				w  = 1.1f * (scale * tw);
				th = tex.GetHeight();
				h  = scale * th;
				SetGsTevMode(gGame.mGs.mCurTevStage, 0);
				DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, 100.0f, top, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
				SetGsTevMode(gGame.mGs.mCurTevStage, 4);
				SetGsTevMode(gGame.mGs.mCurTevStage, 0);
				DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, 100.0f, 180.0f + half, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
				SetGsTevMode(gGame.mGs.mCurTevStage, 4);
			}
		}
	}
}

// FighterPanels_Draw

/**
 * @note Address: stage02 .text 0x51A4
 * @note Size: 0x240
 */
extern "C" void FighterPanels_Update(FighterPanels* fade, u32 dt)
{
	switch (fade->mLowerState) {
	case 0:
		break;
	case 1:
		fade->mLowerOpen += 0.06f * dt;
		if (fade->mLowerOpen > 1.0f) {
			fade->mLowerOpen  = 1.0f;
			fade->mLowerState = 0;
		}
		break;
	case 2:
		fade->mLowerOpen -= 0.06f * dt;
		if (fade->mLowerOpen < 0.0f) {
			fade->mLowerOpen  = 0.0f;
			fade->mLowerState = 0;
		}
		break;
	}
	fade->mLowerFrameTimer -= dt;
	if (--fade->mLowerFrameTimer < 0) {
		fade->mLowerFrame++;
		if (fade->mLowerFrame > 8) {
			fade->mLowerFrame = 0;
		}
		fade->mLowerFrameTimer = 6;
	}
	switch (fade->mCenterState) {
	case 0:
		break;
	case 1:
		fade->mCenterOpen += 0.06f * dt;
		if (fade->mCenterOpen > 1.0f) {
			fade->mCenterOpen  = 1.0f;
			fade->mCenterState = 0;
		}
		break;
	case 2:
		fade->mCenterOpen -= 0.06f * dt;
		if (fade->mCenterOpen < 0.0f) {
			fade->mCenterOpen  = 0.0f;
			fade->mCenterState = 0;
		}
		break;
	}
	fade->mCenterFrameTimer -= dt;
	if (--fade->mCenterFrameTimer < 0) {
		fade->mCenterFrame++;
		if (fade->mCenterFrame >= (fade->mCenterVariant == 0 ? 6 : 7)) {
			fade->mCenterFrame = 0;
		}
		fade->mCenterFrameTimer = 6;
	}
}

/**
 * @note Address: stage02 .text 0x53E4
 * @note Size: 0x664
 */
extern "C" void FighterAim_Draw(FighterAim* aim, CCamera* cam, Mtx fighterMtx)
{
	Vec pos;
	Vec ring[65];
	f32 cx;
	f32 cy;
	s32 k;
	s32 i;
	s32 j;

	// the reticle only draws while the lock-on sound plays or a target is locked
	if (IsGameSoundPlaying(&gGame.mSound, 0x20006) == 0 && aim->mTarget == NULL) {
		return;
	}
	PSMTXMultVec(fighterMtx, &aim->mPos, &pos);
	GXProject(pos.x, pos.y, pos.z, cam->mViewMtx, cam->mProjection, cam->mViewport, &aim->mScreenX, &aim->mScreenY, &aim->mScreenZ);
	if (aim->mTarget == NULL) {
		cx = aim->mScreenX;
		cy = aim->mScreenY;
	} else {
		cx = aim->mTarget->mScreenX;
		cy = aim->mTarget->mScreenY;
	}
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	SetGsLineWidth(gGame.mGs.mCurRenderMode, 12, 5);
	for (i = 9; i > 0; i--) {
		reticleSpin[i] = reticleSpin[i - 1];
	}
	reticleSpin[0] = 0.0f;
	for (k = 0; k < 2; k++) { // an inner and an outer ring
		f32 radius = k == 0 ? 10.0f : 30.000002f;
		f32 innerRadius;
		s32 spokeColor;
		for (i = 0; i < 65; i++) { // the ring as 64 segments plus the closing point
			f32 angle = 0.09817477f * i;
			f32 s     = sin(angle);
			f32 c     = cos(angle);
			ring[i].x = s * radius + cx;
			ring[i].y = c * radius + cy;
			ring[i].z = 0.0f;
		}
		DrawGsLineStrip(&gGame.mGs, ring, -1, 65);
		innerRadius = radius - 10.0f;
		spokeColor  = -1;
		for (j = 0; j < 10; j++) { // ten spoke sets, each dimmer than the last
			for (i = 0; i < 5; i++) {
				f32 angle = HALF_PI * i;
				f32 s     = sin(angle + reticleSpin[j]);
				f32 c     = cos(angle + reticleSpin[j]);
				DrawGsLineSegment(&gGame.mGs, NULL, s * radius + cx, c * radius + cy, 0.0f, spokeColor, s * innerRadius + cx,
				                  c * innerRadius + cy, 0.0f, spokeColor);
			}
			spokeColor = 0xFFFFFF00 | (u8)(s32)(0.5f * (255.0f * (10 - j) / 10.0f));
		}
	}
	{
		CTexObj tex;
		static s32 frame = 0;
		frame++;
		for (i = 0; i < 4; i++) { // four icons 90 degrees apart, radius wobbling +/-3 as frame turns
			f32 angle = HALF_PI * i;
			f32 ds    = sin(DEG2RAD * ((frame * 24) % 360));
			f32 dist  = ds * 3.0f;
			dist += 30.000002f;
			f32 s = sin(angle);
			f32 c = cos(angle);
			u16 w;
			u16 h;
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			tex = gGame.mTpl.mTex.Init(&static_cast<TPLPalette*>(gGame.mTpl.mBuffer)->descriptorArray[128 - StatusTextureOffset], 0);
			w   = GXGetTexObjWidth(&tex);
			h   = GXGetTexObjHeight(&tex);
			DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, dist * s + cx, dist * c + cy, 0.0f, 1.1f * w, h, 0.0f, 0.0f, angle);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		}
	}
}

// FighterAim_Draw

fighter::CCameraActionFighter fighterCamera;
MatoList matoList;
Fighter fighterWork;
PilotNwing pilotNwing;
FighterPanels panels;
s32 fireHoldFrames;
u8* soundCueTable[2];

inline void Shot::Update(u32 dt)
{
	s32 j;

	if (mActive) {
		mTimer -= dt;
		if (mTimer <= 0) {
			Mato* item;
			mActive = false;
			item    = mTarget;
			if (item != NULL) {
				if (--item->mHitsLeft > 0) {
					item->mPending = false;
				} else if (item->mObj != NULL && (item->mObj->mStatus.mFlags & 0x10000)) {
					item->mList->mDestroyed = item;
					item->mList->mScore += item->GetPoints();
					shotEvents.mFlags |= 1;
				}
				mResult.mFlags |= 1;
			} else {
				mResult.mFlags |= 2;
			}
		} else {
			mPos.x = mVelocity.x * dt + mPos.x;
			mPos.y = mVelocity.y * dt + mPos.y;
			mPos.z = mVelocity.z * dt + mPos.z;
			for (j = 9; j > 0; j--) {
				mTrail[j] = mTrail[j - 1];
			}
			mTrail[0] = mPos;
		}
	}
}

/**
 * @note Address: stage02 .text 0x5A48
 * @note Size: 0x2A0
 */
extern "C" void ShotList_Update(ShotList* list, u32 dt)
{
	u32 i;

	for (i = 0; i < 5; i++) {
		Shot* entry = &list->mShots[i];
		if (!entry->mActive) {
			continue;
		}
		entry->Update(dt);
		if (entry->mResult.mFlags & 1) {
			list->mHitStreak++;
			list->mMissStreak = 0;
			if (list->mHitStreak >= 5) {
				shotEvents.mFlags |= 4;
				list->mHitStreak = 0;
			}
		} else if (entry->mResult.mFlags & 2) {
			list->mHitStreak = 0;
			list->mMissStreak++;
			if (list->mMissStreak >= 5) {
				shotEvents.mFlags |= 8;
				list->mMissStreak = 0;
			}
		}
		entry->mResult.mFlags = 0;
	}
}

// ShotList_Update

/**
 * @note Address: stage02 .text 0x5CE8
 * @note Size: 0x170
 */
void fighter::CCameraActionFighter::Update(u32 step)
{
	Vec v;

	if (mTargetMtx == NULL) {
		return;
	}
	CPad pad(0);
	if (pad.mPad->mTrigger & PAD_BUTTON_Y) { // Y toggles the centred and chase views
		mCenterView = !mCenterView;
	}
	SetFov(40.0f);
	if (mCenterView) {
		v.x = v.y = v.z = 0.0f;
		v.z             = 80.0f;
		PSMTXMultVec(mTargetMtx, &v, &v);
		SetInterest(v);
		v.x = v.z = 0.0f;
		SetEye(v);
	} else {
		PSMTXMultVec(mTargetMtx, &mEyeOffset, &v);
		SetEye(v);
		PSMTXMultVec(mTargetMtx, &mInterestOffset, &v);
		SetInterest(v);
	}
}

/**
 * @note Address: stage02 .text 0x5E58
 * @note Size: 0x88
 */
// generated dtor

/**
 * @note Address: stage02 .text 0x5EE0
 * @note Size: 0x4
 */
// inline

/**
 * @note Address: stage02 .text 0x5EE4
 * @note Size: 0x8
 */
// inline

/**
 * @note Address: stage02 .text 0x5EEC
 * @note Size: 0x8
 */
// inline

/**
 * @note Address: stage02 .text 0x5EF4
 * @note Size: 0xC
 */
// inline

/**
 * @note Address: stage02 .text 0x5F00
 * @note Size: 0x134
 */
// fn_5_5F00

/**
 * @note Address: stage02 .text 0x6034
 * @note Size: 0x6C
 */
// generated dtor

/**
 * @note Address: stage02 .text 0x60A0
 * @note Size: 0x54
 */
// generated dtor

/**
 * @note Address: stage02 .text 0x60F4
 * @note Size: 0x18
 */
// inline ctor

/**
 * @note Address: stage02 .text 0x610C
 * @note Size: 0x1C
 */
// inline ctor

/**
 * @note Address: stage02 .text 0x6128
 * @note Size: 0x48
 */
// generated dtor
