#ifndef GAME_WII_CGAME_H
#define GAME_WII_CGAME_H

#include "Dolphin/mtx.h"
#include "game/wii/cpad.h"
#include "game/wii/csound.h"
#include "game/wii/cgs.h"
#include "game/wii/homemenu.h"
#include "game/wii/saveslots.h"
#include "game/cscrn.h"
#include "game/cfile.h"
#include "game/cmovie.h"
#include "game/cmotioncontrolcamera.h"
#include "game/ccameraactioneye.h"
#include "game/ut.h"
#include "game/inventory.h"
#include "game/cstopwatch.h"
#include "game/cmovie.h"
#

class CCamera;

// one entry in the object reference table the renderer walks: the spawned object and a
// flags halfword (CMsgFont_Init sets 0x800 on its cursor's own status instead)
struct WiiObjectRef {
	void* mObj; // _00
	s16 mFlags; // _04
	s16 _06;
};

// Verified Wii fields accessed by the controller, stage, start-up and save code. The Wii CBase is larger
// than the GameCube one, so its vtable sits at 0x41E4, right behind its data.
class CBase {
public:
	u32 mTvFormat;            // _0000, 0 NTSC, 1 PAL, 2 M-PAL, 3 EURGB60
	u8 mProgressive;          // _0004
	CGs mGs;                  // _0008
	CPadEx mPad;              // _2298
	XSound mSound;            // _22A4
	u32 mLastRetrace;         // _33E4
	u32 mRetraceStep;         // _33E8
	CStatus64 mStatus;        // _33F0
	CTpl mIconTpl;            // _3400, the icons shown in the screen's corner; the first says the HOME Menu is not available
	u8 mCameras[2][0x1F8];    // _35F8, CCamera[2] by ~CBase, bytes while CCamera is defined below: the perspective and the screen camera
	CLight mLights[16];       // _39E8, the default set, the screen set starts at [8]
	CFog mFog;                // _40A8
	CStopwatch mStopwatch[2]; // _40C8, the whole frame, and the pad, BeforeUpdate and Draw
	void* mMem1Base;          // _4148, 0x80000000
	void* mMem1Start;         // _414C, the MEM1 arena's start before the heaps are made
	void* mMem1ArenaLo;       // _4150
	void* mMem1ArenaHi;       // _4154
	void* mMem2ArenaLo;       // _4158
	void* mMem2ArenaHi;       // _415C
	u8 mLanguage;             // _4160
	u8 mAspectRatio;          // _4161, SCGetAspectRatio: 0 is 4:3, 1 is 16:9
	s32 mDvdStatus;           // _4164
	s32 mPrevDvdStatus;       // _4168
	BOOL mDiskOk;             // _416C
	bool mDvdError;           // _4170, the drive stopped reading; cleared when it reads again
	DVDCommandBlock mDiskCheckBlock; // _4174, DVDCheckDiskAsync runs on it once, its userData is &mDvdStatus
	bool mDiskChecking;              // _41A4
	u8 _41A5[0x41A8 - 0x41A5];
	bool mResetRequested; // _41A8
	s32 mResetType;       // _41AC, 3 restarts the game, 4 returns to the Wii Menu
	u8 _41B0[0x41BC - 0x41B0];
	// _41BC, the interface text drawn over the frame, NULL when there is none, and its alpha at _41C8
	const char* mSystemMessage;
	u8 _41C0[0x41C8 - 0x41C0];
	f32 mSystemMessageAlpha;
	u8 _41CC[0x41E4 - 0x41CC];
	// _41E4 vtable

	virtual ~CBase() { }                            // _08
	virtual void Initialize(int argc, char** argv); // _0C
	virtual void Start();                           // _10, 0x80017D4C
	virtual void ResetNewGameState();               // _14, CBase's is empty
	virtual void vf18();
	virtual void BeforeUpdate();     // _1C
	virtual void Update(u32 frames); // _20
	// _24, _28 the scene through the main camera, _2C the screen-space layer, _30 retraces per frame
	virtual void Draw();
	virtual void DrawScene();
	virtual void DrawHud();
	virtual s32 FrameInterval();
	virtual void SetStatusFlags(u64 mask);   // _34, 0x8001D5B4
	virtual void ClearStatusFlags(u64 mask); // _38, 0x8001C78C
	virtual void ToggleStatusFlags(u64 mask);
	virtual u64 GetStatusFlags(u64 mask) const; // _40, the game's status flags under mask
	virtual CCamera* GetPerspectiveCamera();
	virtual CCamera* GetScreenCamera(); // _48, the orthographic camera the screen-space layer draws under
	virtual CCamera* GetCamera();       // _4C, the scene camera - CGame's is mMotionControlCamera
	virtual BOOL BeginCamera(CCamera* camera);
	virtual void vf54(); // _54, ends the current render pass
	virtual BOOL BeginSceneCamera();
	virtual BOOL vf5C(); // _5C, begins a render pass; the draws between it and vf54 run only when it holds
	// _60 mLights[0], _64 mLights[8], _68 GetDefaultLight unless overridden, _6C, _70 empty,
	// _74 SetLight(GetLight(), the camera's view, TRUE), _78 the screen light under the screen camera
	virtual CLight* GetDefaultLight();
	virtual CLight* GetScreenLight();
	virtual CLight* GetLight();
	virtual void SetLight(CLight* light, MtxP view, BOOL flag);
	virtual void vf70();
	virtual void SetCameraLight();
	virtual void SetScreenLight();
	virtual void vf7C();
	virtual void vf80();
	virtual void Reset(u32 code);

	void Run(int argc, char** argv); // 0x80017BD8, main's whole body
	void UpdateDvdStatus();          // 0x80019428
	void fn_80018364();              // 0x80018364, Update calls it first

	// asks for a reset unless one is already on its way
	void RequestReset(s32 type)
	{
		if (!mResetRequested) {
			mResetRequested = true;
			mResetType      = type;
		}
	}
};

// the time-of-day state, same fields as the GameCube EvtTimeState
struct WiiTimeState {
	s32 mMode;
	f32 mTime;
	f32 mDuration;
	s32 mRunning;
};
STATIC_ASSERT(sizeof(WiiTimeState) == 0x10);

// one letter of a name that pops in, the GameCube StageNameChar with two more words
struct StageNameChar {
	const char* mText; // _00
	f32 mScale;        // _04
	f32 mWait;         // _08
	f32 mBob;          // _0C
	f32 mTilt;         // _10
	s32 _14;           // _14, StartStageNameBubbles clears both
	f32 _18;           // _18
};
STATIC_ASSERT(sizeof(StageNameChar) == 0x1C);

// the stage name drawn a letter at a time
struct StageName {
	BOOL mActive;             // _00
	s32 mCount;               // _04
	u32 mColor;               // _08
	StageNameChar mChars[20]; // _0C
};
STATIC_ASSERT(sizeof(StageName) == 0x23C);

class CInterfaceCameraAction;

// the camera action table indices, same enum the GameCube camera uses
enum {
	CAMACTION_DEFAULT, // 0
	CAMACTION_AREA,    // 1
	CAMACTION_CHASE,   // 2
	CAMACTION_EVT,     // 3
	CAMACTION_SCROLL2, // 4
	CAMACTION_EYE,     // 5
	CAMACTION_ANIM,    // 6
	CAMACTION_MAP,     // 7
	CAMACTION_LADR,    // 8
	CAMACTION_RESET,   // 9
	CAMACTION_10,      // 10, always null
	CAMACTION_SCROLL,  // 11
	CAMACTION_NUM,
};

// where a camera looks from and at. The class has no RTTI of its own, so its name is made up
class CCameraView {
public:
	CVec mEye;      // _00
	CVec mInterest; // _10
	CVec mAngle;    // _20
	CVec mUp;       // _30
	f32 mDistance;  // _40
	u8 mDirty;      // _44, the view matrix is out of date

	void CalcEye();      // 0x802A855C
	void CalcInterest(); // 0x802A8644
	void CalcAngle();    // 0x802A8734
	void CalcUp();       // 0x802A88D0
	void CalcDistance(); // 0x802A896C, between the eye and the interest
};

// the Wii camera object behind gGame's camera member. A different layout than the GameCube CCamera,
// but the setters the start-up code and the save manager call sit on its vtable at the offsets below
class CCamera {
public:
	virtual ~CCamera() { }                                                                    // _08
	virtual void Init(f32 near, f32 far, f32 fov, f32 scale, const GXRenderModeObj* rmode);   // _0C
	virtual void vf10(f32 near, f32 far, f32 top, f32 bottom, f32 left, f32 right, f32 scale, // _10
	                  const GXRenderModeObj* rmode); // FbMirror::Init passes (0.1, 32768, ±11.5, 1)
	// an orthographic camera
	virtual void Init(f32 top, f32 bottom, f32 left, f32 right, f32 near, f32 far, f32 scale, const GXRenderModeObj* rmode); // _14
	virtual void SetFov(f32 fov)                                                                                             // _18
	{
		mFov             = fov;
		mProjectionDirty = TRUE;
	}
	virtual void vf1C(f32 near) // _1C, the mirror camera's near plane
	{
		mNear            = near;
		mProjectionDirty = TRUE;
	}
	virtual void vf20(f32 far) // _20, the far plane
	{
		mFar             = far;
		mProjectionDirty = TRUE;
	}
	virtual void vf24(f32 top, f32 bottom, f32 left, f32 right) // _24
	{
		mTop             = top;
		mBottom          = bottom;
		mLeft            = left;
		mRight           = right;
		mProjectionDirty = TRUE;
	}
	virtual void SetProjectionSource(MtxP projection); // _28, an external projection matrix; clearing it rebuilds ours
	virtual void vf2C();                               // _2C
	virtual MtxP GetProjectionSource();                // _30
	virtual void SetEye(f32 x, f32 y, f32 z);          // _34
	virtual void SetInterest(f32 x, f32 y, f32 z);     // _38
	virtual void SetAngle(f32 x, f32 y, f32 z)         // _3C
	{
		mView.mAngle.x = x;
		mView.mAngle.y = y;
		mView.mAngle.z = z;
		mView.mDirty   = TRUE;
	}
	virtual void SetUp(f32 x, f32 y, f32 z); // _40
	virtual void SetDistance(f32 distance)   // _44
	{
		mView.mDistance = distance;
		mView.mDirty    = TRUE;
	}
	virtual void SetEye(const Vec& eye) // _48
	{
		mView.mEye.Set(eye.x, eye.y, eye.z);
		mView.mDirty = TRUE;
	}
	virtual void SetInterest(const Vec& interest) // _4C
	{
		mView.mInterest.Set(interest.x, interest.y, interest.z);
		mView.mDirty = TRUE;
	}
	virtual void SetAngle(const Vec& angle); // _50
	virtual void vf54();                     // _54
	virtual void SetAngleY(f32 y)            // _58, ConfigureSubwindowCamera passes angleY here
	{
		mView.mAngle.y = y;
		mView.mDirty   = TRUE;
	}
	virtual void SetAngleX(f32 x) // _5C, then angleX here - same order as the GameCube CCamera
	{
		mView.mAngle.x = x;
		mView.mDirty   = TRUE;
	}
	virtual void SetAngleZ(f32 z); // _60
	virtual void vf64(CMtxP view)  // _64, points the camera at an external view matrix
	{
		mViewSource = view;
		if (view == NULL) {
			mView.mDirty = TRUE;
		}
	}
	virtual void vf68();                                             // _68
	virtual CMtxP GetViewSource();                                   // _6C
	virtual void CalcEye() { mView.CalcEye(); }                      // _70
	virtual void CalcInterest();                                     // _74, Update calls it where the GameCube called CalcInterest
	virtual void CalcAngle() { mView.CalcAngle(); }                  // _78
	virtual void CalcUp();                                           // _7C, Update calls it for the 0x40000 flag, the GameCube's CalcUp
	virtual void CalcDistance() { mView.CalcDistance(); }            // _80
	virtual void vf84();                                             // _84
	virtual void vf88(f32 x, f32 y, f32 w, f32 h, f32 near, f32 far) // _88, FbSubwin::Init passes (0, 0, w, h, 0, 1)
	{
		mViewport[0]   = x;
		mViewport[1]   = y;
		mViewport[2]   = w;
		mViewport[3]   = h;
		mViewport[4]   = near;
		mViewport[5]   = far;
		mViewportDirty = TRUE;
	}

	u8 mType;            // _04, 0 perspective, 1 frustum, 2 orthographic
	f32 mFov;            // _08
	f32 mAspect;         // _0C
	f32 mNear;           // _10
	f32 mFar;            // _14
	f32 mTop;            // _18, ortho projection bounds, fed to C_MTXOrtho
	f32 mBottom;         // _1C
	f32 mLeft;           // _20
	f32 mRight;          // _24
	u8 mProjectionDirty; // _28, SetFov sets it - the projection matrix is rebuilt lazily
	u8 _29[0x2C - 0x29];
	Mtx44 mProjection;      // _2C
	MtxP mProjectionSource; // _6C
	f32 mIsOrtho;           // _70, 1.0 for the orthographic camera
	f32 mProjParams[6];     // _74, the six entries of mProjection GXSetProjection takes
	f32 mScale;             // _8C
	CCameraView mView;      // _90
	Mtx44 mViewMtx;         // _D8, the computed view matrix the 2D draws multiply into (the fourth row stays identity)
	CMtxP mViewSource;      // _118, an external view matrix the camera follows; clearing it dirties the view
	f32 mViewport[6];       // _11C
	u8 mViewportDirty;      // _134
	Mtx44 mViewportMtx;     // _138, clip space to the viewport's pixels
	Mtx44 mScreenMtx;       // _178, viewport * projection * view
	Mtx44 mScreenInv;       // _1B8

	void InitView(const GXRenderModeObj* rmode); // 0x802A80FC, the view at its defaults and the full-screen viewport
	BOOL SetCurrent();                           // 0x802A82FC, rebuilds whatever matrix is out of date
	void EndCurrent();                           // 0x802A8558
};
STATIC_ASSERT(sizeof(CCamera) == 0x1F8);
#ifndef offsetof
#define offsetof(type, member) ((u32) & (((type*)0)->member))
#endif
STATIC_ASSERT(offsetof(CCamera, mProjectionDirty) == 0x28);

// what CreateMemento saves of the camera, the same class the GameCube version keeps
class CCameraMemento {
public:
	CCameraMemento() { }
	virtual ~CCameraMemento() { } // _08

	CVec mEye;                       // _04
	CVec mInterest;                  // _14
	CVec mAngle;                     // _24
	f32 mFov;                        // _34
	f32 mDistance;                   // _38
	CInterfaceCameraAction* mAction; // _3C

	void SetEye(f32 x, f32 y, f32 z)
	{
		mEye.x = x;
		mEye.y = y;
		mEye.z = z;
	}
	void SetInterest(f32 x, f32 y, f32 z)
	{
		mInterest.x = x;
		mInterest.y = y;
		mInterest.z = z;
	}
	void SetAngle(f32 x, f32 y, f32 z)
	{
		mAngle.x = x;
		mAngle.y = y;
		mAngle.z = z;
	}
	void SetEye(const Vec& eye) { SetEye(eye.x, eye.y, eye.z); }
	void SetInterest(const Vec& interest) { SetInterest(interest.x, interest.y, interest.z); }
	void SetAngle(const Vec& angle) { SetAngle(angle.x, angle.y, angle.z); }
};
STATIC_ASSERT(sizeof(CCameraMemento) == 0x40);

// the game's camera, a CCamera with whichever action drives it. CGame keeps it at +0xD7620.
// The member layout is the GameCube class's shifted by 0x64: there the camera was a member at
// _00 of size 0x194 and the derived fields started at _194, here CCamera is the base subobject
// of size 0x1F8 and they start at _1F8. Offsets verified in retail Init at 0x80167638.
class CMotionControlCamera : public CCamera {
public:
	CVec mEye;                             // _1F8, what the action wants this frame
	CVec mInterest;                        // _208
	CVec mAngle;                           // _218
	f32 mDistance;                         // _228
	f32 mFov;                              // _22C
	s32 mMode;                             // _230, the action's GetMode, 1 eases eye and interest in
	CInterfaceCameraAction* mAction;       // _234
	CInterfaceCameraAction* mPrevAction;   // _238
	CInterfaceCameraAction* mPushedAction; // _23C
	s32 _240;                              // _240, written -1 in Init, never read
	s32 _244;                              // _244, written -1 in Init, never read
	u32 mFlags;                            // _248, the action's GetCameraFlags, see CAMFLAG in cmotioncontrolcamera.h
	u8 _24C[0x250 - 0x24C];                // _24C, Init clears _248/_250/_254 but never this
	s32 mInterestMode;                     // _250, the action's GetInterestMode, 2 and 3 follow the interest in
	s32 mEyeMode;                          // _254, 2 and 3 follow the eye in
	u8 _258[0x25C - 0x258];
	CVec mFollowTarget;      // _25C, Follow steps mFollowValue towards it
	CVec mFollowValue;       // _26C
	CVec mFollowStep;        // _27C
	BOOL mFollowDone;        // _28C
	f32 mFollowDivide;       // _290, how many steps it takes, the action's GetFollowDivisor
	CCameraMemento mMemento; // _294
	u8 _2D4[0x2D8 - 0x2D4];

	void Init();
	void InitCamera();
	void Update(u32 step);
	void Interpolate(u32 step);
	void Draw();
	void fn_80169528(); // forwards the action's vf24, a second draw pass the GameCube never had
	void fn_80167728(); // clears the eye flag and drops every action pointer
	void ChangeAction(s32 type);
	void ChangeAction(CInterfaceCameraAction* action);
	void PushAction(CInterfaceCameraAction* action);
	void PopAction();
	void ApplyAction();
	bool IsAction(s32 type);
	s32 GetActionType();
	void UpdateTarget(XObj* obj);
	void StartEye();
	void EndEye();
	CCameraMemento* CreateMemento(CCameraMemento* memento);
	BOOL SetMemento(const CCameraMemento* memento);
	f32 GetMapDistance();
	void ReadAction();

	// what the action wants lives in mEye/mInterest/mAngle/mDistance/mFov above, Update copies
	// it there directly; unlike the GameCube class there are no target-writer inlines, they
	// would override CCamera's virtuals and the Wii vtable has none of its own functions
	// steps mFollowValue a fixed part of the way to the target
	void Follow(const Vec& target, const Vec& value)
	{
		mFollowTarget.x = target.x;
		mFollowTarget.y = target.y;
		mFollowTarget.z = target.z;
		mFollowValue.x  = value.x;
		mFollowValue.y  = value.y;
		mFollowValue.z  = value.z;
		mFollowStep     = (mFollowTarget - mFollowValue) / mFollowDivide;

		f32 step = PSVECMag(&mFollowStep);
		f32 dist = PSVECDistance(&mFollowTarget, &mFollowValue);
		if (dist <= step || dist < 0.05f) {
			mFollowValue.x = mFollowTarget.x;
			mFollowValue.y = mFollowTarget.y;
			mFollowValue.z = mFollowTarget.z;
			mFollowDone    = TRUE;
		} else {
			Vec step = mFollowStep;
			PSVECAdd(&mFollowValue, &step, &mFollowValue);
		}
	}

	// some actions start from the camera as it is, the rest from scratch
	void StartAction()
	{
		if (mAction != NULL) {
			if (mAction == GetAction(CAMACTION_CHASE) || mAction == GetAction(CAMACTION_10)) {
				mAction->Reset();
			} else if (mAction == GetAction(CAMACTION_EVT) || mAction == GetAction(CAMACTION_AREA) || mAction == GetAction(CAMACTION_MAP)
			           || mAction == GetAction(CAMACTION_RESET) || mAction == GetAction(CAMACTION_ANIM)) {
				mAction->SetCamera(this);
			} else {
				mAction->Reset();
			}
		}
	}

	// inline copy of ApplyAction
	void CopyAction()
	{
		if (mAction != NULL) {
			if (mFlags & CAMFLAG_EYE) {
				SetEye(mAction->mEye);
			}
			if (mFlags & CAMFLAG_INTEREST) {
				SetInterest(mAction->mInterest);
			}
			if (mFlags & CAMFLAG_DISTANCE) {
				SetDistance(mAction->mDistance);
			}
			if (mFlags & CAMFLAG_FOV) {
				SetFov(mAction->mFov);
			}
			if (mFlags & CAMFLAG_ANGLE) {
				SetAngle(mAction->mAngle);
			}
		}
	}

	static CInterfaceCameraAction* GetAction(s32 type);
	BOOL IsCurrent(s32 type) const { return mAction == GetAction(type) ? TRUE : FALSE; }
};
STATIC_ASSERT(sizeof(CMotionControlCamera) == 0x2D8);

// the Wii area action, 0x2FC against the GameCube's 0x294. Only the members CCameraActions' constructor
// (0x80034ACC) builds are placed: they sit at the GameCube class's offsets and carry its names. The plain
// fields between and behind them are not reconstructed, and neither are the virtuals it overrides
class CCameraActionArea : public CCameraActionAreaExM {
public:
	u8 _40[0x70 - 0x40];
	CVectorEaseInOutInterpolation mInterestEaseInOut;    // _70
	CVectorLinearInterpolation mEyeLinear;               // _CC
	CVectorEaseInOutInterpolation mEyeEaseInOut;         // _100
	CVectorEaseOutInterpolation mEyeEaseOut;             // _15C
	CVectorEaseOutDivideInterpolation mEyeEaseOutDivide; // _1C0
	u8 _1F8[0x20C - 0x1F8];
	CInterpolationLinear mFovLinear;               // _20C
	CInterpolationEaseInOut mFovEaseInOut;         // _21C
	CInterpolationEaseOut mFovEaseOut;             // _238
	CInterpolationEaseOutDivide mFovEaseOutDivide; // _258
	u8 _268[0x2FC - 0x268];

	virtual ~CCameraActionArea() { } // _08, 0x8002ECD0
	virtual void Update(u32 step);   // _10
};
STATIC_ASSERT(sizeof(CCameraActionArea) == 0x2FC);

// the Wii map action, 0x128C against the GameCube's 0x11EC, placed the same way: every member the constructor
// builds up to _11E0 is where the GameCube class has it, the vec group at _11FC is new
class CCameraActionMap : public CCameraActionAreaExM {
public:
	u8 _40[0x48 - 0x40];
	CVec mPreviousInterest; // _48
	u8 _58[0x60 - 0x58];
	CVec mSpringPrevTarget; // _60
	CVec mSpringTarget;     // _70
	u8 _80[0x8C - 0x80];
	// _8C and _11FC: the constructor (0x80034ACC) builds a CObjVec at each; typed, they tip ~CGame's inline budget
	// so that ~CCameraActions expands into it and the member destructors behind it are no longer emitted
	// (gamedtor.cpp, an incomplete CGame), so they stay bytes until the rest of CGame is typed
	u8 _8C[0x30];
	u8 _BC[0xC8 - 0xBC];
	CInterpolationEaseOutDivide mFovEaseOutDivide; // _C8
	CVec mPreviousEye;                             // _D8
	u8 _E8[0xDB8 - 0xE8];
	CVectorEaseInOutInterpolation mTurnEaseInOut; // _DB8
	CInterpolationEaseInOut mHeightEaseInOut;     // _E14
	u8 _E30[0xE34 - 0xE30];
	CInterpolationEaseInOut mAbsHeightEaseInOut; // _E34
	u8 _E50[0xE68 - 0xE50];
	CInterpolationEaseOutDivide mTargetHeightOffsetEase; // _E68
	u8 _E78[0xE7C - 0xE78];
	CInterpolationEaseInOut mEyeYOffsetEaseInOut; // _E7C
	u8 _E98[0xF2C - 0xE98];
	CVectorEaseOutDivideInterpolation mEyeEaseOutDivide; // _F2C
	u8 _F64[0xF68 - 0xF64];
	CVec mTargetEye;      // _F68
	CVec mPrevTargetEye;  // _F78
	CVec mOrbitEye;       // _F88
	CVec mOrbitAngle;     // _F98
	CVec mPrevOrbitEye;   // _FA8
	CVec mPrevOrbitAngle; // _FB8
	u8 _FC8[0x11D8 - 0xFC8];
	CStatus mStatus;         // _11D8
	CStatus mPreviousStatus; // _11E0
	u8 _11E8[0x11FC - 0x11E8];
	u8 _11FC[0x30];
	u8 _122C[0x128C - 0x122C];

	virtual ~CCameraActionMap() { } // _08, 0x8002E5C4
	virtual void Update(u32 step);  // _10
};
STATIC_ASSERT(sizeof(CCameraActionMap) == 0x128C);

// every camera action in one block, CGame keeps the block at +0xD78F8 and the table it fills
// in at +0xD9B24 as mCameraActions.
class CCameraActions {
public:
	CCameraActionDefault mDefault;                 // _00
	CCameraActionArea mArea;                       // _40
	CCameraActionChase mChase;                     // _33C
	CCameraActionEvt mEvt;                         // _3AC
	CCameraActionScroll2 mScroll2;                 // _890
	u8 _B78[0xB7C - 0xB78];                        // _B78
	CCameraActionEye mEye;                         // _B7C
	CCameraActionAnim mAnim;                       // _CA0
	CCameraActionMap mMap;                         // _CE8
	CCameraActionLadr mLadr;                       // _1F74
	CCameraActionReset mReset;                     // _1FB8
	CCameraActionScroll mScroll;                   // _21C0
	CInterfaceCameraAction* mTable[CAMACTION_NUM]; // _222C
	// _225C vtable: the constructor at 0x80034ACC stores the CCameraActions one (0x802E60C8) there, behind the table

	CCameraActions() { }
	virtual ~CCameraActions() { } // _08, 0x8002DD80

	void Init();
};
STATIC_ASSERT(sizeof(CCameraActions) == 0x2260);

// the action pointers indexed by CAMACTION_*, CGame keeps them at +0xD9B24; they are the tail
// of the CCameraActions block at +0xD78F8
struct WiiCameraActions {
	CInterfaceCameraAction* mTable[CAMACTION_NUM];
};
STATIC_ASSERT(sizeof(WiiCameraActions) == 0x30);

// chibi robo's numbers, watts, happy points and the items he holds -- same layout
// as the GameCube Params the Wii code was ported from
struct WiiPlayerParams {
	s32 mExperience;      // _00
	s32 mLevel;           // _04
	s32 mRank;            // _08
	f32 mBattery;         // _0C
	f32 mMaxBattery;      // _10
	s32 mMoney;           // _14
	s32 mScrap;           // _18
	u32 _1C[8];           // _1C, [0] above 0 is an unlimited battery; saved as mActive
	Inventory mInventory; // _3C
	// _440 vtable

	void SetBattery(f32 value)
	{
		mBattery = value > 0.0f ? value : 0.0f;
		if (mBattery < 0.0f)
			mBattery = 0.0f;
		if (mBattery > mMaxBattery)
			mBattery = mMaxBattery;
	}

	// money and scrap stay between 0 and 99999
	void SetMoney(s32 value)
	{
		mMoney = value > 0 ? value : 0;
		if (mMoney < 0)
			mMoney = 0;
		if (mMoney > 99999)
			mMoney = 99999;
	}
	void SetScrap(s32 value)
	{
		mScrap = value > 0 ? value : 0;
		if (mScrap < 0)
			mScrap = 0;
		if (mScrap > 99999)
			mScrap = 99999;
	}
};
STATIC_ASSERT(sizeof(WiiPlayerParams) == 0x440);

// the suit state fields the save manager touches, at the head of the PlayerSuits-shaped block
struct WiiPlayerSuits {
	s32 mLoadStep;   // _00
	s32 mChangeStep; // _04, a queued suit change
	s32 mCurrent;    // _08, the suit he's wearing
	s32 mRequested;  // _0C, takes the saved suit
	s32 mSpecial;    // _10
	                 // the suit objects beyond here are not reconstructed
};

struct WiiPlayerState;
// the vtable slots of the unreconstructed Wii player class that the save manager reaches:
// +0x70 takes the rotation, +0x90 a position Vec, +0x244 returns it
struct WiiPlayerVtable {
	void* _00[0x70 / 4];
	void (*SetRotation)(WiiPlayerState* chr, f32 rotY);
	void* _74[(0x90 - 0x74) / 4];
	void (*SetPosition)(WiiPlayerState* chr, const Vec* pos);
	void* _94[(0xAC - 0x94) / 4];
	// the camera actions read the object's world matrix through here; retail feeds the
	// result straight into PSMTXCopy (0x80160F7C / 0x80161160)
	MtxP (*GetMtxPtr)(WiiPlayerState* chr);
	void* _B0[(0x1B0 - 0xB0) / 4];
	// the camera actions reach the focus point here, the GameCube's XObj::GetCameraTarget
	Vec (*GetCameraTarget)(WiiPlayerState* chr);
	void* _1B4[(0x244 - 0x1B4) / 4];
	const Vec* (*GetPosition)(WiiPlayerState* chr);
	void* _248[(0x26C - 0x248) / 4];
	// the ladder camera adds the object's facing to its orbit angle
	f32 (*GetRotationY)(WiiPlayerState* chr);
	void* _270[(0x28C - 0x270) / 4];
	// the eye camera takes and releases the player's motion through here: 1 in StartEye, 0 in EndEye
	void (*vf28C)(WiiPlayerState* chr, s32 held);
};

// the cord wire object embedded in the player at +0xABC0, the GameCube's mPlug.mWire
// (XObjWireEff). Its vtable shares the unreconstructed Wii object layout where +0x90 takes a
// position Vec - the same slot the player's own vtable keeps it at
struct WiiCordWire {
	virtual void V_00(); // _00 through _8C unreconstructed
	virtual void V_04();
	virtual void V_08();
	virtual void V_0C();
	virtual void V_10();
	virtual void V_14();
	virtual void V_18();
	virtual void V_1C();
	virtual void V_20();
	virtual void V_24();
	virtual void V_28();
	virtual void V_2C();
	virtual void V_30();
	virtual void V_34();
	virtual void V_38();
	virtual void V_3C();
	virtual void V_40();
	virtual void V_44();
	virtual void V_48();
	virtual void V_4C();
	virtual void V_50();
	virtual void V_54();
	virtual void V_58();
	virtual void V_5C();
	virtual void V_60();
	virtual void V_64();
	virtual void V_68();
	virtual void V_6C();
	virtual void V_70();
	virtual void V_74();
	virtual void V_78();
	virtual void V_7C();
	virtual void V_80();
	virtual void V_84();
	virtual void SetPosition(const Vec* pos); // _90
};

// the yes/no menu CMsg__cmd_QUESTION opens on the player, the GameCube's mOXMenu. its input poll
// is HandleOXMenuInput at 0x800EF58C
struct WiiOXMenu {
	u8 _00[4];
	s32 mStarted; // _04
	u8 _08[0x1388];
	s32 mConfirmedIndex; // _1390
};

// one player action slot on the Wii player: only the state word at +4 is verified, the
// camera reads it to tell whether the look controls are free
struct WiiPlayerAction {
	u8 _00[4];
	s32 mState; // _04
};

// the attachment ring's menu on the player at _19EE8, the GameCube's mMenu.mAtc. The HUD reads the
// highlighted item's type; the item count is a bound, the menu only uses a handful
struct WiiAtcMenuItem {
	s32 mType; // _00
	u8 _04[0x24];
};
struct WiiAtcMenu {
	s32 mCursorIndex;            // _00
	u8 _04[0xC];                 // _04
	WiiAtcMenuItem mItems[0x20]; // _10
};

class CSound3DActor;
class XObj;

// one of the player's positioned sounds; ChangeGameStage stops both
struct WiiPlayerSound {
	CSound3DActor* mActor; // _00
	u8 _04[8];
};

// the Wii player object: a different, larger class than the GameCube XObjChr. These are the members
// the save manager reads and writes, at offsets verified against retail.
struct WiiPlayerState {
	WiiPlayerVtable* mVtable;
	u8 _0004[0x64 - 0x04];
	f32 mRotY; // _0064, mMoveRotate.y
	u8 _0068[0x9CC - 0x68];
	u32 mPlayerFlags; // _09CC, the player status word; bit 0x20000000 lets the eye camera open
	u8 _09D0[0xAA0 - 0x9D0];
	WiiPlayerAction mLift; // _0AA0, the carry action; its mState decides whether looking is free
	u8 _0AA8[0xAC0 - 0xAA8];
	XObj* mLiftTarget; // _0AC0, what he carries, the GameCube's mLift.mTarget
	u8 _0AC4[0xAF0 - 0xAC4];
	XObj* mLiftPlugTarget; // _0AF0, the GameCube's mLift.mPlugTarget
	u8 _0AF4[0xB50 - 0xAF4];
	WiiPlayerAction mHang; // _0B50
	u8 _0B58[0xBA0 - 0xB58];
	WiiPlayerAction mJump; // _0BA0
	u8 _0BA8[0xBF4 - 0xBA8];
	WiiPlayerAction mLadder; // _0BF4
	u8 _0BFC[0xC38 - 0xBFC];
	WiiPlayerAction mRope; // _0C38
	u8 _0C40[0xC78 - 0xC40];
	WiiPlayerAction mPush; // _0C78
	u8 _0C80[0xCD0 - 0xC80];
	WiiPlayerAction mTumble; // _0CD0
	u8 _0CD8[0x95BC - 0xCD8];
	WiiPlayerSound mSounds[2]; // _95BC
	u8 _95D4[0x9920 - 0x95D4];
	s32 mAttachment;        // _9920, the AtcMenu pick
	s32 mAttachmentRequest; // _9924, -1 when there's none
	s32 mAttachmentStep;    // _9928, how far the attachment change is
	u8 _992C[0x999C - 0x992C];
	WiiPlayerParams mParams; // _999C
	u8 _9DDC[0x9DF8 - 0x9DDC];
	f32 mHeldMoveAngle; // _9DF8, the heading the player snaps back to after a look
	f32 mMoveAngle;     // _9DFC
	u8 _9E00[0xABC0 - 0x9E00];
	WiiCordWire mCordWire; // _ABC0, the plug cord's spark wire, the GameCube's mPlug.mWire
	u8 _ABC4[0xDC50 - 0xABC4];
	WiiPlayerSuits mSuits; // _DC50
	u8 _DC64[0x19E9C - 0xDC64];
	u8 mAttachments[0x48]; // _19E9C
	u8 _19EE4[0x19EE8 - 0x19EE4];
	WiiAtcMenu mAtcMenu; // _19EE8
	u8 _1A3F8[0x1A420 - 0x1A3F8];
	WiiOXMenu mOXMenu; // _1A420
	u8 _1B7B4[0x1C994 - 0x1B7B4];
	s32 mSquirterContent; // _1C994
	s32 mItemTint;        // _1C998
	u8 _1C99C[0x1C9A4 - 0x1C99C];
	s32 mGroundType; // _1C9A4, the floor he walks on, picks the footstep sounds

	const Vec* GetPosition() { return mVtable->GetPosition(this); }
	MtxP GetMtxPtr() { return mVtable->GetMtxPtr(this); }
	Vec GetCameraTarget() { return mVtable->GetCameraTarget(this); }
	f32 GetRotationY() { return mVtable->GetRotationY(this); }
	void vf28C(s32 held) { mVtable->vf28C(this, held); }
	// the attachment chibi robo works with: the pending pick while the ring is closing, else the
	// current one, as AtcMenu::ConfirmSelection in atcmenuselection.cpp computes it
	s32 GetAttachment() const { return mAttachmentRequest >= 0 ? mAttachmentRequest : mAttachment; }
};
STATIC_ASSERT(offsetof(WiiPlayerState, mOXMenu) == 0x1A420);
STATIC_ASSERT(offsetof(WiiPlayerState, mGroundType) == 0x1C9A4);
STATIC_ASSERT(offsetof(WiiPlayerState, mSounds) == 0x95BC);

// the coin, item and dust collections each hold a pointer to their 0x80 flag words at +0x9A8
struct WiiFlagSource {
	u8 _000[0x9A8];
	u32* mFlags;
};

class WiiTitleObject;

// the running event's object resolver at CGame+0x4C8E4, the GameCube's CEvtData: the message
// commands test mScript, ResolveEventObjectId (0x8009453C) falls back to mContext for id -1
struct WiiEvtData {
	u8* mScript;              // _00
	WiiTitleObject* mContext; // _04
};

// a screen color with a texture to draw it through, CGame keeps it at _57CC; the GameCube struct of the same name
struct ScrnTex {
	f32 mAngle;        // _00
	CScrnColor mColor; // _04
	CTexObj mTex;      // _38
};
STATIC_ASSERT(sizeof(ScrnTex) == 0x13C);

// the two screen colors behind it, at _5908 and _593C. Each sits in a struct of its own as on the GameCube, but the
// Wii has a constructor for each (0x80033EC8, 0x80033F0C), so they are two types; the second one's name is ours
struct GameScrnColor {
	CScrnColor mColor; // _00
};
struct GameScrnColor1 {
	CScrnColor mColor; // _00
};

class XObj;

class CGame : public CBase {
public:
	virtual ~CGame(); // _08, 0x8002CFB8
	// the five mirrors CGame keeps at _ECD5C, 0x37C each; defined in game/fbtexeff.h behind its base FbMirror
	class FbMirrorEx;

	void Initialize(int argc, char** argv); // _0C override, parses /stage: /slot: (gameinit.cpp)
	CCamera* GetCamera();                   // _4C override, &mMotionControlCamera
	// _24, _28, _2C, _30 (the logos, title and menus run at 60, the game at 30), _68 mLightList, _84
	void Draw();
	void DrawScene();
	void DrawHud();
	s32 FrameInterval();
	CLight* GetLight();
	void Reset(u32 code);

	void ResetNewGameState();        // _14 override, 0x8001C37C
	void BeforeUpdate();             // _1C override, 0x8001C7B0
	void Update(u32 frames);         // _20 override, 0x8001E830
	void SetupStageCamera(s32 mode); // 0x8001DC88

	s32 mLoadStep;
	s32 mSetupStep;        // _41EC, how far FinalizeStageObjectSetup is, done at 5
	s32 _41F0;             // _41F0, ChangeGameStage sets it to 3
	CFileArchive mArchive; // _41F4
	s32 mStage;
	s32 mPrevStage; // _4300
	s32 mRoomCount; // _4304, CCameraActionScroll2::GetLimits bounds the room index with it
	u8 _4308[0x4B40 - 0x4308];
	CTpl mTpl;
	CScrnFade mFade;             // _4D38
	CScrnWipeCircle mWipeCircle; // _4D6C
	CScrnWipeTex mWipeTex;       // _4DD4
	CScrnWipeCustom mWipeCustom; // _4F40
	CScrnWipeTile mWipeTile;     // _4FA8
	CScrnWipeCine mWipeCine;     // _5010, the letterbox bars; the message window and the HUD read its mActive, mY and mHeight
	CScrnRoboeyes mRoboeyes;
	u8 _511C[0x52E4 - 0x511C];
	u8 mBlur[0x1A8];          // _52E4, FbBlur (fbeffect.h)
	u8 mSubwin[0x340];        // _548C, FbSubwin; fbtexeff.h needs this header, so it stays untyped here
	ScrnTex mScrnTex;         // _57CC
	GameScrnColor mColor0;    // _5908
	GameScrnColor1 mColor1;   // _593C
	s32 mOutlineWidthControl; // _5970
	f32 mOutlineWidth;        // _5974
	s32 mLogoState;           // _5978, the boot logos' step, as the GameCube mLogo.mState
	u8 _597C[0x5C70 - 0x597C];
	u8 mTitle[4];    // _5C70, where the Wii Title (title.h) starts, its vtable; mTitleState is its first member
	s32 mTitleState; // _5C74, the title screen's step, as the GameCube mTitle.mState
	u8 _5C78[0xAE00 - 0x5C78];
	BOOL mMenuActive; // _AE00, a menu is open, as the GameCube mMenu.mActive
	void* mMainMenu;  // _AE04, the menu group's first menu, as the GameCube mMenu.mMenus[0]
	u8 _AE08[0xAE34 - 0xAE08];
	s32 _AE34; // _AE34, DrawHud draws nothing while a menu is open and this is set
	u8 _AE38[0xAF58 - 0xAE38];
	// the object the InfoScrn_* HUD calls receive at +0xAF58; the Wii callbacks are
	// empty stubs, so only its address is verified, not its layout
	u8 mInfoScrn[0xB148 - 0xAF58]; // _AF58
	WiiHomeMenu mHomeMenu;         // _B148
	u8 _B7A8[0xBCA8 - 0xB7A8];     // _B7A8, the effect manager (ceffect.cpp), the GameCube's mParticle
	u8 mEmitters[0x80][0x74];      // _BCA8, its emitter handles
	u8 _F6A8[0xF6AC - 0xF6A8];
	CScrnQuake mQuake;              // _F6AC
	u8 mXObjPools[0xF70C - 0xF6E4]; // _F6E4, where the XOBJS block starts: its first two XObjPools
	WiiPlayerState* mPlayer;        // _F70C
	u8 _F710[0xF71C - 0xF710];
	s32 mPlayerStride; // _F71C, the size of one entry in the pool mPlayer points into
	u8 _F720[0xF770 - 0xF720];
	WiiFlagSource* mCoins; // _F770
	u8 _F774[0xF784 - 0xF774];
	WiiFlagSource* mItems; // _F784
	s32 mItemCount;        // _F788, the item pool's objects in use
	u8 _F78C[0xF794 - 0xF78C];
	s32 mItemStride; // _F794
	u8 _F798[0xF7AC - 0xF798];
	// the spawn pool CMsgFont_Draw scans for the portrait object (kind 10); sits just before mObjects
	u8* mObjPoolObjects; // _F7AC
	s32 mObjPoolCount;   // _F7B0
	u8 _F7B4[0xF7BC - 0xF7B4];
	s32 mObjPoolStride; // _F7BC
	u8 _F7C0[0xF874 - 0xF7C0];
	WiiFlagSource* mDust; // _F874
	u8 _F878[0xF8B0 - 0xF878];
	WiiTitleObject* mObjects[0x400]; // _F8B0, the object table ResolveEventObjectId indexes
	s32 mObjectCount;                // _108B0
	s32 mLoadObjectIndex;            // _108B4
	u8 _108B8[0x108C0 - 0x108B8];
	WiiObjectRef* mDrawRefs; // _108C0, the objects in draw order, BuildObjectDrawList fills it
	s32 mDrawRefCount;       // _108C4
	// pointers to spawned objects the renderer walks; entries are grouped in eights,
	// CMsgFont_Init pushes its two cursors here
	WiiObjectRef* mObjectRefs;        // _108C8
	s32 mObjectRefCount;              // _108CC
	WiiObjectRef* mSceneRefs;         // _108D0, the objects DrawScene draws
	s32 mSceneRefCount;               // _108D4
	WiiObjectRef* mEffectRefs;        // _108D8, the objects whose effects are drawn
	s32 mEffectRefCount;              // _108DC
	ResourceFile mObjArchives[0x400]; // _108E0, the pooled object archives, the GameCube's mObjArchives
	u8 _4C8E0[0x4C8E4 - 0x4C8E0];
	WiiEvtData mEvtData; // _4C8E4
	u8 _4C8EC[0x4C8F0 - 0x4C8EC];
	s32 mSelectResult; // _4C8F0, the yes/no answer CMsg__cmd_QUESTION stores, in mEvt on the GameCube
	u8 _4C8F4[0x4C950 - 0x4C8F4];
	u8 mMsgs[2][0x45660];                      // _4C950, CMsg[2]; msg.h needs this header, so they stay untyped here
	WiiTimeState mTimeState;                   // _D7610
	CMotionControlCamera mMotionControlCamera; // _D7620
	CCameraActions mCameraActions;             // _D78F8, mTable lands at _D9B24
	CLight* mLightList;                        // _D9B58, the lights the scene draws under, CBase's own set when the stage brings none
	s32 _D9B5C;                                // _D9B5C, ResetNewGameState clears both
	s32 _D9B60;                                // _D9B60
	u8 _D9B64[0xD9B80 - 0xD9B64];
	WiiSaveManager mSaveManager; // _D9B80
	u8 _D9B80[0xEC180 - 0xD9B80 - sizeof(WiiSaveManager)];
	BOOL mMemcardLoaded; // _EC180
	u8 _EC184[0xEC1A0 - 0xEC184];
	f32 mPlayTime;   // _EC1A0
	f32 _EC1A4;      // _EC1A4
	f32 _EC1A8;      // _EC1A8, a new game starts it at 1
	f32 mScopeTimer; // _EC1AC, the robo eyes' lenses pulse while it runs and hide from 5 on
	u8 _EC1B0[0xEC2B4 - 0xEC1B0];
	f32 _EC2B4; // _EC2B4, a new game starts it at NAN
	u8 _EC2B8[0xEC2C8 - 0xEC2B8];
	f32 _EC2C8; // _EC2C8, a new game starts it at -1
	u8 _EC2CC[0xECCE8 - 0xEC2CC];
	// _ECCE8 to _ECD24: one block FinalizeStageObjectSetup resets when the stage is up.
	// _ECCE8 is an object DrawHud draws through its vf19C at the two coordinates behind it, when there is one
	XObj* mHudObject;
	f32 mHudObjectX;
	f32 mHudObjectY;
	s32 _ECCF4;
	u8 _ECCF8[4];
	s32 _ECCFC; // -1
	s32 _ECD00; // -1
	s32 _ECD04;
	s32 _ECD08;
	s32 _ECD0C;
	s32 _ECD10;
	s32 _ECD14; // -1
	f32 _ECD18[4];
	u8 mFloorReflect;               // _ECD28, an object asked for the floor reflection
	Mtx mFloorReflectMtx;           // _ECD2C, PSMTXReflect about the floor
	u8 mMirrors[0xEDEC8 - 0xECD5C]; // _ECD5C, FbMirror[5]; fbtexeff.h needs this header, so they stay untyped here
	StageName mStageName;           // _EDEC8
	CMovie mMovie;                  // _EE104; AnimSfx::CheckStart starts no sound while its first word is set
	u32 mMovieFlags;                // _EE1C0, the Start flags
	f32 mMovieRect[4];              // _EE1C4, where the movie draws, NAN for its own size and the centre
	f32 mMovieVolume;               // _EE1D4
	bool mMovieOnThread;            // _EE1D8, the staff roll: UpdateMovie steps it on a thread of its own
	u8 _EE1D9[0xEE1DC - 0xEE1D9];
	u32 mMenuControls;
	CVec _EE1E0; // _EE1E0, a new game zeroes it
	CVec _EE1F0; // _EE1F0, and fills this one with NAN
	// _EE200 to _EE518: the shadow RenderSceneShadow casts from straight above, a Wii piece the GameCube had
	// as a CShadow on the light; the names are ours
	XObj* mShadowFocus;      // _EE200, what the shadow camera looks down on, the scene camera's interest without one
	f32 mShadowWidth;        // _EE204, 100 unless positive
	f32 mShadowHeight;       // _EE208, 100 unless positive
	f32 mShadowAlpha;        // _EE20C, 1 unless positive
	u8 mShadowCamera[0x1F8]; // _EE210, a CCamera: as a member it would join CGame's constructor and destructor
	u8 mShadowTex[0x104];    // _EE408, the CTexObj that shows it
	u16 mShadowTexWidth;     // _EE50C
	u16 mShadowTexHeight;    // _EE50E
	void* mShadowImage;      // _EE510, what the frame buffer is copied to
	f32 mShadowStrength;     // _EE514, the alpha the shadows draw at
	u8 _EE518[0xEE530 - 0xEE518];

	void SetStage(s32 stage)
	{
		mPrevStage = mStage;
		mStage     = stage;
	}

	// the checks OpenWiiHomeMenu makes. Retail keeps each one's result apart, so they are separate
	// inline functions; where one ends and the next begins, and all their names, are ours
	bool IsLoadStepRunning() const { return mLoadStep > 0 && mLoadStep < 30; }
	bool IsLogoActive() const { return mLogoState >= 1 && mLogoState < 12; }
	bool IsTitleActive() const { return mTitleState >= 1 && mTitleState < 23; }
	bool IsLogoShown() const { return IsLogoActive() && mLogoState > 2; }
	bool IsIntroShown() const { return IsLogoShown() || IsTitleActive(); }
	// loading with neither the logos nor the title on screen
	bool IsLoading() const { return IsLoadStepRunning() && !IsIntroShown(); }
	bool IsInterruptible() const { return !IsLoading() && mTitleState != 20; }
	bool IsMenuFree() const { return IsInterruptible() && !mMenuActive; }

	WiiPlayerState* GetPlayer() const { return mPlayer; }
	// 0x80037134, the sound code asks for player 0 through the pool
	WiiPlayerState* GetPlayer(s32 index) { return (WiiPlayerState*)((u8*)mPlayer + index * mPlayerStride); }
	s32 GetTimeMode() { return mTimeState.mMode; } // 0x800377B4
	bool CanOpenHomeMenu() const { return IsMenuFree() && !mHomeMenu.IsOpen(); }
	// interface text 3 is up over the frame (0x8002BBE0)
	bool IsSystemMessage3Shown();

	// 0x8002AEC8, projects a world position onto the screen under camera (the motion-control
	// camera when it is null); the GameCube had it on XObj
	void ProjectOffsetToScreen(const Vec* pos, f32* x, f32* y, f32* z, void* camera);
};
STATIC_ASSERT(sizeof(CGame) == 0xEE530);
STATIC_ASSERT(offsetof(CGame, mScrnTex) == 0x57CC);
STATIC_ASSERT(offsetof(CGame, mColor0) == 0x5908);
STATIC_ASSERT(offsetof(CGame, mOutlineWidth) == 0x5974);
STATIC_ASSERT(offsetof(CGame, mXObjPools) == 0xF6E4);
STATIC_ASSERT(offsetof(CGame, mPlayerStride) == 0xF71C);
STATIC_ASSERT(offsetof(CGame, mScopeTimer) == 0xEC1AC);
STATIC_ASSERT(offsetof(CGame, _D9B5C) == 0xD9B5C);
STATIC_ASSERT(offsetof(CGame, mMemcardLoaded) == 0xEC180);
STATIC_ASSERT(offsetof(CGame, _EC2B4) == 0xEC2B4);
STATIC_ASSERT(offsetof(CGame, _EC2C8) == 0xEC2C8);
STATIC_ASSERT(offsetof(CGame, mStageName) == 0xEDEC8);
STATIC_ASSERT(offsetof(CGame, mSetupStep) == 0x41EC);
STATIC_ASSERT(offsetof(CGame, mDrawRefs) == 0x108C0);
STATIC_ASSERT(offsetof(CGame, mLightList) == 0xD9B58);
STATIC_ASSERT(offsetof(CGame, mHudObject) == 0xECCE8);
STATIC_ASSERT(offsetof(CGame, mFloorReflectMtx) == 0xECD2C);
STATIC_ASSERT(offsetof(CGame, mMirrors) == 0xECD5C);
STATIC_ASSERT(offsetof(CGame, _EE1E0) == 0xEE1E0);
STATIC_ASSERT(offsetof(CGame, mSubwin) == 0x548C);
STATIC_ASSERT(offsetof(CGame, mTitleState) == 0x5C74);
STATIC_ASSERT(offsetof(CGame, mEmitters) == 0xBCA8);
STATIC_ASSERT(offsetof(CGame, mMsgs) == 0x4C950);
STATIC_ASSERT(offsetof(CGame, mShadowTexWidth) == 0xEE50C);

extern "C" CGame gGame;
#endif
