#ifndef GAME_WII_CGAME_H
#define GAME_WII_CGAME_H

#include "Dolphin/mtx.h"
#include "JSystem/JMath/Inline.h"
#include "game/attachmentvalues.h"
#include "game/wii/cpad.h"
#include "game/wii/ceffect.h"
#include "game/wii/csound.h"
#include "game/wii/cgs.h"
#include "game/wii/homemenu.h"
#include "game/wii/saveslots.h"
#include "game/wii/fbeffect.h"
#include "game/wii/infoscrn.h"
#include "game/logo.h"
#include "game/evt.h"
#include "game/evttime.h"
#include "game/cptcl.h"
#include "game/wii/msg.h"
#include "game/cscrn.h"
#include "game/gamescrencolor.h"
#include "game/cfile.h"
#include "game/cmovie.h"
#include "game/cmotioncontrolcamera.h"
#include "game/ccameraactioneye.h"
#include "game/ut.h"
#include "game/inventory.h"
#include "game/cstopwatch.h"
#include "game/cmovie.h"
#include "game/wii/xobjs.h"
#include "game/wii/xobj.h"
#include "game/wii/xobjcamera.h"
#include "game/wii/xchrmenu.h"
#include "game/playeranimationdata.h"
#include "game/wii/menuutil.h"
#include "game/menuhelpchar.h"
#include "game/menudata.h"
#include "game/xactdata.h"
#include "game/cameraactionids.h"
#include "game/ccameramemento.h"
#

class CCamera;

namespace nw4r {
namespace ef {
class EffectProject;
class TextureProject;
} // namespace ef
} // namespace nw4r

// one entry in the object reference table the renderer walks: the spawned object and a
// flags halfword (CMsgFont_Init sets 0x800 on its cursor's own status instead)
struct WiiObjectRef {
	void* mObj; // _00
	s16 mIndex; // _04, the object's slot in mObjects
	s16 _06;
};

// objects in draw order, the GameCube's XObjDrawList; the Wii list points at its entries
struct XObjDrawList {
	WiiObjectRef* mRefs; // _00
	s32 mCount;          // _04

	void Initialize(); // 0x800C5CD4, allocates the 1024-entry ref table off MEM1
	void PauseSounds(f32 fade);
	void ResumeSounds(f32 fade);
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
	                  const GXRenderModeObj* rmode); // FbMirror::Init passes (0.1, 32768, +/-11.5, 1)
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
	virtual void SetUp(const Vec& up);       // _54, xobj_camera.cpp's copy into mView.mUp
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

	union {
		struct {
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
		};
	};
	CCameraView mView; // _90
	Mtx44 mViewMtx;    // _D8, the computed view matrix the 2D draws multiply into (the fourth row stays identity)
	CMtxP mViewSource; // _118, an external view matrix the camera follows; clearing it dirties the view
	union {
		struct {
			f32 mViewport[6];  // _11C
			u8 mViewportDirty; // _134
			u8 _135[0x138 - 0x135];
			Mtx44 mViewportMtx; // _138, clip space to the viewport's pixels
		};
	};
	Mtx44 mScreenMtx; // _178, viewport * projection * view
	Mtx44 mScreenInv; // _1B8

	void InitView(const GXRenderModeObj* rmode); // 0x802A80FC, the view at its defaults and the full-screen viewport
	BOOL SetCurrent();                           // 0x802A82FC, rebuilds whatever matrix is out of date
	void EndCurrent();                           // 0x802A8558
	void Project(const Vec& pos, Vec* out) const
	{
		f32 sx;
		GXProject(pos.x, pos.y, pos.z, mViewMtx, &mIsOrtho, mViewport, &sx, &out->y, &out->z);
		out->x = sx * mScale;
	}
};
STATIC_ASSERT(sizeof(CCamera) == 0x1F8);
#ifndef offsetof
#define offsetof(type, member) ((u32) & (((type*)0)->member))
#endif
STATIC_ASSERT(offsetof(CCamera, mProjectionDirty) == 0x28);

// Verified Wii fields accessed by the controller, stage, start-up and save code. The Wii CBase is larger
// than the GameCube one, so its vtable sits at 0x41E4, right behind its data.
class CBase {
public:
	u32 mTvFormat;                   // _0000, 0 NTSC, 1 PAL, 2 M-PAL, 3 EURGB60
	u8 mProgressive;                 // _0004
	CGs mGs;                         // _0008
	CPadEx mPad;                     // _2298
	XSound mSound;                   // _22A4
	u32 mLastRetrace;                // _33E4
	u32 mRetraceStep;                // _33E8
	CStatus64 mStatus;               // _33F0
	CTpl mIconTpl;                   // _3400, the icons shown in the screen's corner; the first says the HOME Menu is not available
	CCamera mCameras[2];             // _35F8, the perspective and the screen camera
	CLight mLights[16];              // _39E8, the default set, the screen set starts at [8]
	CFog mFog;                       // _40A8
	CStopwatch mStopwatch[2];        // _40C8, the whole frame, and the pad, BeforeUpdate and Draw
	void* mMem1Base;                 // _4148, 0x80000000
	void* mMem1Start;                // _414C, the MEM1 arena's start before the heaps are made
	void* mMem1ArenaLo;              // _4150
	void* mMem1ArenaHi;              // _4154
	void* mMem2ArenaLo;              // _4158
	void* mMem2ArenaHi;              // _415C
	u8 mLanguage;                    // _4160
	u8 mAspectRatio;                 // _4161, SCGetAspectRatio: 0 is 4:3, 1 is 16:9
	s32 mDvdStatus;                  // _4164
	s32 mPrevDvdStatus;              // _4168
	BOOL mDiskOk;                    // _416C
	bool mDvdError;                  // _4170, the drive stopped reading; cleared when it reads again
	DVDCommandBlock mDiskCheckBlock; // _4174, DVDCheckDiskAsync runs on it once, its userData is &mDvdStatus
	bool mDiskChecking;              // _41A4
	u8 _41A5[0x41A8 - 0x41A5];
	bool mResetRequested; // _41A8
	s32 mResetType;       // _41AC, 3 restarts the game, 4 returns to the Wii Menu
	u32 _41B0;            // _41B0, BeforeUpdate sets the low three bits once the loading screen may show
	u32 mLastDeviceType;  // _41B4, pad 0's last mDeviceType the corner message saw
	s32 mLastProbe;       // _41B8, and its last mProbeResult, -1 before the first frame
	// _41BC, the interface text drawn over the frame, NULL when there is none, and its alpha at _41C8
	const char* mSystemMessage;
	u8 _41C0;
	u8 _41C1;
	u8 _41C2;
	u8 _41C3;
	bool mBatteryLow; // _41C4, the remote's battery is down to its last bar
	u8 _41C5[0x41C8 - 0x41C5];
	f32 mSystemMessageAlpha;
	f32 mBatteryAlpha;        // _41CC, the low-battery icon fades in and out with mBatteryLow
	f32 mBatteryBlink;        // _41D0, counts down from 1 while the icon is fully shown
	f32 mSystemMessageBlink;  // _41D4, the same for the message
	f32 mSystemMessageTarget; // _41D8, nonzero while the message should be up
	f32 mNoRemoteFrames;      // _41DC, frames pad 0 has reported no remote, up to 10
	CTexObj* _41E0;           // _41E0, BeforeUpdate points it at CGame's copy of misc.tpl's texture 261
	// _41E4 vtable

	virtual ~CBase() { }                            // _08
	virtual void Initialize(int argc, char** argv); // _0C
	virtual void Start();                           // _10, 0x80017D4C
	virtual void ResetNewGameState();               // _14, CBase's is empty
	virtual void InitFont(const char* path); // _18, CGame::Start passes the font file named on the command line or NULL; the name is ours
	virtual BOOL BeforeUpdate();             // _1C
	virtual void Update(u32 frames);         // _20
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
	virtual void DrawDiscError(); // _80, the drive's error message over the frame while the disc cannot be read
	virtual void Reset(u32 code);

	void Run(int argc, char** argv); // 0x80017BD8, main's whole body
	void UpdateDvdStatus();          // 0x80019428
	void UpdateSystemMessage();      // 0x80018364, Update calls it first: the corner message and the battery icon
	void DrawSystemMessage();        // 0x800187B0

	// the drive reports trouble and the disc is not back yet
	BOOL IsDiscError() const { return (mDvdStatus != 0 || mPrevDvdStatus != 0) && !mDiskOk; }

	// asks for a reset unless one is already on its way
	void RequestReset(s32 type)
	{
		if (!mResetRequested) {
			mResetRequested = true;
			mResetType      = type;
		}
	}
};

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

// the game's camera, a CCamera with whichever action drives it. CGame keeps it at +0xD7620.
// The member layout is the GameCube class's shifted by 0x64: the camera member is size 0x1F8
// here and the motion fields start at _1F8. Offsets verified in retail Init at 0x80167638.
// CMotionControlCamera's own vtable sits at _258, behind mEyeMode; its RTTI at 0x805A0900
// has no base-class link, while the CCamera member keeps CCamera's vtable at _00.
class CMotionControlCamera {
public:
	CCamera mCamera;                       // _00
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
	CStatus32 mFlags;                      // _248, the action's GetCameraFlags, see CAMFLAG in cmotioncontrolcamera.h
	s32 mInterestMode;                     // _250, the action's GetInterestMode, 2 and 3 follow the interest in
	s32 mEyeMode;                          // _254, 2 and 3 follow the eye in
	// _258 vtable

	virtual ~CMotionControlCamera() { } // _08

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
			JMathInlineVEC::PSVECAdd(&mFollowValue, &step, &mFollowValue);
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
				mAction->SetCamera(&mCamera);
			} else {
				mAction->Reset();
			}
		}
	}

	// inline copy of ApplyAction
	void CopyAction()
	{
		if (mAction != NULL) {
			if (mFlags.mFlags & CAMFLAG_EYE) {
				mCamera.SetEye(mAction->mEye);
			}
			if (mFlags.mFlags & CAMFLAG_INTEREST) {
				mCamera.SetInterest(mAction->mInterest);
			}
			if (mFlags.mFlags & CAMFLAG_DISTANCE) {
				mCamera.SetDistance(mAction->mDistance);
			}
			if (mFlags.mFlags & CAMFLAG_FOV) {
				mCamera.SetFov(mAction->mFov);
			}
			if (mFlags.mFlags & CAMFLAG_ANGLE) {
				mCamera.SetAngle(mAction->mAngle);
			}
		}
	}

	static CInterfaceCameraAction* GetAction(s32 type);
	BOOL IsCurrent(s32 type) const { return mAction == GetAction(type) ? TRUE : FALSE; }
};
STATIC_ASSERT(sizeof(CMotionControlCamera) == 0x2D8);

// CamEdgeArrow lives in ccameraaction.h (the leticker's action class carries a pair too)

// the Wii area action, 0x2FC against the GameCube's 0x294. Only the members CCameraActions' constructor
// (0x80034ACC) builds are placed: they sit at the GameCube class's offsets and carry its names. The plain
// fields between and behind them are not reconstructed, and neither are the virtuals it overrides
class CCameraActionArea : public CCameraActionAreaExM {
public:
	virtual void vf24();        // _24, 0x801569B4, the edge arrows
	virtual void DrawDebug3d(); // _28, 0x80156E58

	f32 mTransitionTime;                                 // _40
	s32 mKeepViewDirection;                              // _44, set by SetCamera: pick entries by the current view until one is applied
	BOOL mForceApply;                                    // _48, apply mEntry even if its id is unchanged
	s32 mEntryIndex;                                     // _4C
	s32 mPrevEntryIndex;                                 // _50
	CamAreaEntry* mEntry;                                // _54
	CamAreaEntry* mPrevEntry;                            // _58
	s32 mControlEnabled;                                 // _5C, with _2D0 set the area action keeps the pointer's cursor up
	BOOL mPadControlEnabled;                             // _60
	XObj* mTarget;                                       // _64
	CInterfaceVectorInterpolation* mEyeInterp;           // _68
	CInterfaceInterpolation* mFovInterp;                 // _6C
	CVectorEaseInOutInterpolation mInterestEaseInOut;    // _70
	CVectorLinearInterpolation mEyeLinear;               // _CC
	CVectorEaseInOutInterpolation mEyeEaseInOut;         // _100
	CVectorEaseOutInterpolation mEyeEaseOut;             // _15C
	CVectorEaseOutDivideInterpolation mEyeEaseOutDivide; // _1C0
	CInterfaceVectorInterpolation* mEyeInterps[5];       // _1F8
	CInterpolationLinear mFovLinear;                     // _20C
	CInterpolationEaseInOut mFovEaseInOut;               // _21C
	CInterpolationEaseOut mFovEaseOut;                   // _238
	CInterpolationEaseOutDivide mFovEaseOutDivide;       // _258
	CInterfaceInterpolation* mFovInterps[5];             // _268
	CInterfaceVectorInterpolation* mInterestInterp;      // _27C
	CInterfaceVectorInterpolation* mInterestInterps[5];  // _280, by mode
	// _294 to _2FC: the Wii's pointer controls, Reset and SetCamera clear them; the names are not recovered
	CamEdgeArrow mArrows[2]; // _294, left and right
	f32 _2C4;
	f32 _2C8;
	f32 _2CC;
	u8 _2D0;
	u8 _2D1;
	u8 _2D2;
	Vec _2D4;
	u8 _2E0[0x2E8 - 0x2E0];
	s32 _2E8;
	s32 _2EC;
	u32 _2F0;
	u32 _2F4;
	s32 _2F8;

	virtual ~CCameraActionArea() { }         // _08, 0x8002ECD0
	virtual void PreviousUpdate();           // _0C
	virtual void Update(u32 step);           // _10
	virtual void Reset();                    // _14
	virtual void SetCamera(CCamera* camera); // _18
	virtual void Init();                     // _1C
	virtual void DrawDebug2d();              // _28

	s32 FindFacing(s32 slotIndex, const Vec& dir);

	void SetEye_I(s32 mode, Vec from, Vec to, f32 time);
	void SetEyeInterpolation(s32 mode, Vec from, Vec to, f32 time)
	{
		if (mode == 5) {
			mode = 4;
		}
		Vec target = to;
		if (mode == 3 || time <= 0.0f) {
			mEyeInterp = NULL;
			SetEye(target);
		} else {
			mEyeInterp = mEyeInterps[mode];
			if (mEyeInterp != NULL) {
				mEyeInterp->Start(from, target, time);
			}
		}
	}
	void SetFovInterpolation(s32 mode, f32 from, f32 to, f32 time)
	{
		s32 kind = mode;
		if (mode == 5) {
			kind = 4;
		}
		if (kind == 3 || time <= 0.0f) {
			mFovInterp = NULL;
			SetFov(to);
		} else {
			mFovInterp = mFovInterps[kind];
			if (mFovInterp != NULL) {
				mFovInterp->Start(from, to, time);
			}
		}
	}

	// the vectors are passed by value, hence the copies
	void SetInte_I(s32 mode, Vec from, Vec to, f32 time)
	{
		if (mode == 3 || time <= 0.0f) {
			mInterestInterp = NULL;
			SetInterest(to);
			return;
		}
		mInterestInterp = mInterestInterps[mode];
		if (mInterestInterp != NULL) {
			mInterestInterp->Start(from, to, time);
		}
	}
	virtual u32 GetCameraFlags();  // _58
	virtual u32 GetInterestMode(); // _60
	virtual XObj* GetTarget();     // _64
};
STATIC_ASSERT(sizeof(CCameraActionArea) == 0x2FC);

// the Wii map action, 0x128C against the GameCube's 0x11EC, placed the same way: every member the constructor
// builds up to _11E0 is where the GameCube class has it, the vec group at _11FC is new
class CCameraActionMap : public CCameraActionAreaExM {
public:
	// NONMATCHING: retail stores each CStatus member's flags before its vtable; the weak array copy reverses that order.
	CCameraActionMap() { }
	virtual void DrawDebug3d(); // _28, 0x8015F34C

	s32 mInterestMode;       // _40, what GetInterestMode returns
	XObj* mTarget;           // _44, the object the camera looks at
	CVec mPreviousInterest;  // _48
	f32 mRestDistance;       // _58
	f32 mSpringDamping;      // _5C
	CVec mSpringPrevTarget;  // _60
	CVec mSpringTarget;      // _70
	f32 mTargetRestDistance; // _80, the rest distance mRestDistance eases toward
	f32 mFlatDistance;       // _84
	f32 mOrbitRadius;        // _88
	// _8C and _11FC: the constructor (0x80034ACC) builds a CObjVec at each
	CObjVec _8C;
	f32 mTurnSpeed;                                // _BC, turn speed
	f32 mTargetYaw;                                // _C0, wrapped target yaw
	f32 _C4;                                       // _C4
	CInterpolationEaseOutDivide mFovEaseOutDivide; // _C8
	CVec mPreviousEye;                             // _D8
	u8 _E8[0x4EC - 0xE8];
	s32 mInsideCllCount;                                 // _4EC
	s32 mInsideCll0;                                     // _4F0
	s32 mInsideCll1;                                     // _4F4
	s32 mInsideCll2;                                     // _4F8
	u8 _4FC[0x570 - 0x4FC];                              // _4FC
	CamMapEntry mIntersectionEntries[32];                // _570
	s32 mIntersectionCount;                              // _970
	CamMapEntry* mNearestTargetEntry;                    // _974
	s32 mNearestTargetIndex;                             // _978
	CamMapEntry* mNearestEyeEntry;                       // _97C
	s32 mNearestEyeIndex;                                // _980
	Vec mSideStart;                                      // _984, one end of the picked side
	Vec mSideEnd;                                        // _990, the other end
	s32 mSideSign;                                       // _99C, 1 or -1, which way round they are
	CamMapEntry mAreaIntersections[32];                  // _9A0
	s32 mAreaIntersectionCount;                          // _DA0, count of mAreaIntersections
	s32 mPrevMapArea;                                    // _DA4
	s32 mMapArea;                                        // _DA8
	s32 mPadControlEnabled;                              // _DAC
	s32 _DB0;                                            // _DB0
	CInterfaceVectorInterpolation* mTurnInterpolation;   // _DB4
	CVectorEaseInOutInterpolation mTurnEaseInOut;        // _DB8
	CInterpolationEaseInOut mHeightEaseInOut;            // _E14
	CInterfaceInterpolation* mHeightInterp;              // _E30, camera height above the target
	CInterpolationEaseInOut mAbsHeightEaseInOut;         // _E34
	CInterfaceInterpolation* mAbsHeightInterp;           // _E50, absolute camera height
	f32 mTargetHeight;                                   // _E54, the height kept once no area is picked
	s32 mHeightMode;                                     // _E58, 0 height above the target, 1 absolute
	s32 mUseAreaHeight;                                  // _E5C, 1 keeps the area's height outside it
	f32 mCurrentHeight;                                  // _E60, current camera height
	CInterfaceInterpolation* mTargetHeightOffsetInterp;  // _E64
	CInterpolationEaseOutDivide mTargetHeightOffsetEase; // _E68
	CInterfaceInterpolation* mEyeYOffsetInterp;          // _E78
	CInterpolationEaseInOut mEyeYOffsetEaseInOut;        // _E7C
	f32 mEyeYOffset;                                     // _E98
	s32 mTurnBlocked;                                    // _E9C, 1 while area corners keep the camera from turning
	s32 mManualInput;                                    // _EA0, manual camera input this frame
	s32 mInsideAreaCount;                                // _EA4, area IDs written to mInsideAreas
	s32 mInsideAreas[32];                                // _EA8, areas containing the player
	CInterfaceVectorInterpolation* mEyeInterp;           // _F28
	CVectorEaseOutDivideInterpolation mEyeEaseOutDivide; // _F2C
	f32 mEyeInterpTime;                                  // _F64, target-eye interpolation time
	CVec mTargetEye;                                     // _F68
	CVec mPrevTargetEye;                                 // _F78
	CVec mOrbitEye;                                      // _F88
	CVec mOrbitAngle;                                    // _F98
	CVec mPrevOrbitEye;                                  // _FA8
	CVec mPrevOrbitAngle;                                // _FB8
	u8 _FC8[0x11D8 - 0xFC8];
	CStatus mStatus;         // _11D8
	CStatus mPreviousStatus; // _11E0
	BOOL mKeepPreviousEye;   // _11E8
	f32 mOrbitInput;         // _11EC, the pad's orbit input, shaped by the curve at _126C (name inferred)
	f32 _11F0;               // _11F0, second orbit axis, only cleared by the pad controls
	f32 mPointerHoldTime;    // _11F4, frames the pointer button has been held (name inferred)
	u8 mPointerHeld;         // _11F8, set while the map action wants the pointer's cursor
	u8 _11F9[0x11FC - 0x11F9];
	CObjVec _11FC;
	u8 mOrbiting;    // _122C, 1 while the orbit keeps placing the eye (name inferred)
	u8 mOrbitReseed; // _122D, reseeds the orbit target from the camera angle on the next input (name inferred)
	u8 _122E[0x1230 - 0x122E];
	Vec mOrbitPointer;       // _1230, the pointer position clamped to the screen limits (name inferred)
	CamEdgeArrow mArrows[2]; // _123C, left and right
	f32 _126C;               // _126C
	f32 _1270;               // _1270
	f32 _1274;               // _1274
	f32 _1278;               // _1278
	f32 _127C;               // _127C
	f32 _1280;               // _1280
	f32 _1284;               // _1284
	f32 _1288;               // _1288

	virtual ~CCameraActionMap() { }          // _08, 0x8002E5C4
	virtual void PreviousUpdate();           // _0C
	virtual void Update(u32 step);           // _10
	virtual void Reset();                    // _14
	virtual void SetCamera(CCamera* camera); // _18, takes eye, interest, angle, distance and fov
	virtual void Init();                     // _1C
	virtual void vf24();                     // _24, draws the turn arrows at the screen edges (fn_80160788 in retail)
	virtual void DrawDebug2d();              // _28
	virtual u32 GetCameraFlags();            // _58
	virtual f32 GetFollowDivisor();          // _5C
	virtual u32 GetInterestMode();           // _60

	s32 CalcIntersectVec(const Vec& from, const Vec& to, CamMapEntry* out, s32 type);
	void Reset2(); // 0x80157384, resets eye and interest behind the target; called directly, not virtual
};
STATIC_ASSERT(sizeof(CCameraActionMap) == 0x128C);
STATIC_ASSERT(offsetof(CCameraActionMap, mTurnSpeed) == 0xBC);
STATIC_ASSERT(offsetof(CCameraActionMap, mPrevMapArea) == 0xDA4);

// Wii-only stage camera action returned by fn_80169674; holds a pointer-active byte at _74
class CCameraActionPointer : public CInterfaceCameraAction {
public:
	u8 _40[0x34];
	u8 mPointerHeld; // _74
};
STATIC_ASSERT(offsetof(CCameraActionPointer, mPointerHeld) == 0x74);

// every camera action in one block, CGame keeps the block at +0xD78F8 and the table it fills
// in at +0xD9B24 as mCameraActions.
class CCameraActions {
public:
	CCameraActionDefault mDefault;                 // _00
	CCameraActionArea mArea;                       // _40
	CCameraActionChase mChase;                     // _33C
	CCameraActionEvt mEvt;                         // _3AC
	CCameraActionScroll2 mScroll2;                 // _890
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
// as the GameCube Params the Wii code was ported from; the Wii class keeps the
// GameCube name in the symbols and vtable
struct Params {
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

	Params();
	virtual ~Params(); // _08

	void SetBattery(f32 value)
	{
		mBattery = value > 0.0f ? value : 0.0f;
		if (mBattery < 0.0f)
			mBattery = 0.0f;
		if (mBattery > mMaxBattery)
			mBattery = mMaxBattery;
	}

	// the battery's size stays below 999.9 watts
	void SetMaxBattery(f32 value)
	{
		mMaxBattery = value > 0.0f ? value : 0.0f;
		if (mMaxBattery < 0.0f)
			mMaxBattery = 0.0f;
		if (mMaxBattery > 999.9f)
			mMaxBattery = 999.9f;
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
STATIC_ASSERT(sizeof(Params) == 0x444);
typedef Params WiiPlayerParams;

// the objects behind a suit, the GameCube's PlayerSuits.mObjects: XObjs with two virtuals of their own that the
// player forwards to the one he wears. The class name is RTTI.
class XObjDress : public XObj {
public:
	XObjDress()
	{
		u32* p = _9AC;
		do {
			*p++ = 0;
		} while (p < _9AC + 7);
	}
	virtual ~XObjDress() { }
	virtual void vf28C(BOOL on); // _28C
	virtual void vf290(BOOL on); // _290

	XObj* mOwner; // _9A8
	u32 _9AC[7];  // retail's ctor clears these seven words
	u8 m_9C8;     // _9C8, vf28C hides the head nodes while it is set
	u8 m_9C9;     // _9C9, vf290 hides the left-arm nodes
	u8 _9CA[0x9D0 - 0x9CA];
};
STATIC_ASSERT(sizeof(XObjDress) == 0x9D0);
typedef XObjDress WiiSuitObject;

// the extra model suits 3 and 6 bring, updated and drawn with the suit; the class name is RTTI.
// Its ExecCallbackB grafts the owner's head matrix onto the callback result.
class XObjDressOpt : public XObj {
public:
	XObjDressOpt() { }
	virtual ~XObjDressOpt() { }

	XObj* mOwner; // _9A8, the object whose head joint this part follows
	u8 _9AC[0x9B0 - 0x9AC];
};
STATIC_ASSERT(sizeof(XObjDressOpt) == 0x9B0);
typedef XObjDressOpt WiiSuitPart;

// his suits, the GameCube's PlayerSuits: the save manager touches the state at the head. The name follows the
// ctor/dtor names in symbols.txt (0x800C08B8, 0x800C0E18); retail's XObjChrBase RTTI is polymorphic and this has
// no vptr, so the name is unconfirmed
struct XObjChrBase {
	XObjChrBase() { }
	~XObjChrBase() { }

	s32 mLoadStep;          // _00
	s32 mChangeStep;        // _04, a queued suit change
	s32 mCurrent;           // _08, the suit he's wearing
	s32 mRequested;         // _0C, takes the saved suit
	s32 mSpecial;           // _10
	u32 _14;                // _14
	XObjDress mObjects[9];  // _18
	XObjDressOpt mParts[9]; // _5868
};
STATIC_ASSERT(sizeof(XObjChrBase) == 0xAF98);
typedef XObjChrBase WiiPlayerSuits;

// the plug object embedded in the player at +0x9E20, the GameCube's mPlug (XObjPlug): ResetPlayerPlug
// reaches its XObj status and virtuals. The anchor transform takes the whole block; of its own members only
// the connection word the cord simulation reads is verified
struct WiiPlayerPlug : public XObj {
	u8 _09A8[0xD64 - 0x9A8];
	s32 mConnection; // _D64, XActLift's mPlugConnection with 0x10000 set
	u8 _D68[0xD98 - 0xD68];
	f32 mEndRotationZ; // _D98, the plug end's pitch while it is pulled out
	u8 _D9C[0xDA0 - 0xD9C];
};
STATIC_ASSERT(sizeof(WiiPlayerPlug) == 0xDA0);

// the yes/no menu CMsg__cmd_QUESTION opens on the player, the GameCube's mOXMenu. its input poll
// is HandleOXMenuInput at 0x800EF58C
struct WiiOXMenuMarkObj {
	u8 _0[0x9A8]; // the mark's own XObj (retail __destroy_arr walks mMark[2] in
	              // 0x800C1348); kept opaque so WiiOXMenu stays POD
};
struct WiiOXMenuMark {
	f32 mSpin;             // _00
	f32 mTilt;             // _04
	f32 mScale;            // _08
	u32 _0C;               // _0C, the GameCube's dropped random angle
	WiiOXMenuMarkObj mObj; // _10, the GameCube's XObj mObj
};
STATIC_ASSERT(sizeof(WiiOXMenuMark) == 0x9B8);
STATIC_ASSERT(offsetof(WiiOXMenuMark, mObj) == 0x10);
struct WiiOXMenu {
	s32 mLoadStep;          // _00, the GameCube's
	s32 mStarted;           // _04
	s32 mCrossMask;         // _08
	s32 mSelectedIndex;     // _0C
	s32 _10;                // _10, -1 while nothing is picked
	f32 mFade;              // _14, cleared on reset
	WiiOXMenuMark mMark[2]; // _18
	XObj* mChr;             // _1388
	f32 mStartRot;          // _138C, the chr's y rotation when it opened
	s32 mConfirmedIndex;    // _1390
};

// one of chibi robo's actions on the Wii player, the GameCube's XAction (RTTI) with the same head: the camera reads
// mState to tell whether the look controls are free, the action accessors of xobj_chr.cpp reach the rest
struct WiiPlayerState;
struct EffectInstance; // game/wii/ceffect.h
#include "game/xactionbase.h"

#include "game/xactclasses.h"

class CSound3DActor;
class XObj;

// where chibi robo looks and the marker over it, the GameCube's XObjChrLook with the same head
struct XObjChrLook {
	BOOL mActive;     // _00
	s32 mMode;        // _04
	XObj* mObject;    // _08
	Vec mOffset;      // _0C
	Vec mTarget;      // _18
	f32 mFade;        // _24, the marker is shown while it is above 0
	f32 mPhase;       // _28
	f32 mPulse;       // _2C
	BOOL _30;         // _30, set with mActive, cleared on its own
	s32 mMarkerCount; // _34, the greatest number of action markers found this frame
};
STATIC_ASSERT(sizeof(XObjChrLook) == 0x38);

// one of the player's positioned sounds; ChangeGameStage stops both
// the Wii player object: a different, larger class than the GameCube XObjChr, on the Wii XObj. These are the
// members the player code reaches, at offsets verified against retail; its own virtuals start at _28C.
// todo: it is the same class as xobj.h's XObjChr, whose pool still declares it with a padded tail
struct WiiPlayerState : public XObj {
	WiiPlayerState* mPlayer; // _9A8, the player object these head joints belong to
	// the player's head bones, fetched out of the loaded model by name in LoadFromPlacement:
	// second_null, head_chn, head_cap and both eyelid pairs, down then up
	nw4r::g3d::ResNode mHeadNodes[7]; // _9AC
	u8 m_9C8;                         // _9C8, DrawWithView skips the transition suit part while it is set
	u8 m_9C9;                         // _9C9, the same for the popper mount, fn_800E3C50 sets it
	u8 _09CA[0x9CC - 0x9CA];
	u32 mPlayerFlags; // _09CC, the player status word; bit 0x20000000 lets the eye camera open
	u8 _09D0[0x9D4 - 0x9D0];
	CObjVec mMotion; // _09D4, how he moves this frame
	f32 mMoveSpeed;  // _0A04, the magnitude of his requested movement
	f32 mStick[4];
	s32 mHitObjectIndex;         // _0A18, the closest object under his interaction probe
	s32 mActionObjectIndex;      // _0A1C, the closest one offering an action
	s32 mFloorContactObjects[3]; // _0A20, current, previous and event floor contacts, -1 for none
	XObj* mFloorSupport;         // _0A2C, the object or map supporting him
	s32 mPreviousContactObject;  // _0A30, the object his automatic approach last used
	CVec mContactPos;            // _0A34, the automatic approach target's position
	s32 mLastContactFloorIndex;  // _0A44, the automatic approach target's floor
	CVec mEdgePos;               // _0A48, the automatic approach target's edge position
	f32 mNearDistance;           // _0A58, 2.0 while nothing is near
	s32 mAutoActionFrames;       // _0A5C, frames spent on the current automatic approach
	Vec mAutoActionStart;        // _0A60, where that approach began
	s32 mLetickerIndex;          // _0A6C, the clear leticker's object index, -1 for none
	s32 _0A70;                   // _0A70, cleared on reset
	u32 mLiftPlugFlags;          // _0A74, the GameCube's word; bit 0x80 is the lifted plug's
	s32 mAPressed;               // _0A78
	s32 mPressTimer;             // _0A7C, frames A has been held
	s32 mAttachmentActionType;   // _0A80
	s32 mToolActionType;         // _0A84
	s32 mIdleLoops;              // _0A88, idle anim loops before the next idle move
	f32 mIdleTimer;              // _0A8C, the GameCube XObjChri's: 0 while he moves, counts up standing still
	s32 mFlashTimer;             // _0A90
	s32 mFlashCount;             // _0A94, frames the flash has run
	f32 mHitStrength;            // _0A98, the last hit's strength
	f32 mMoveSpeedScale;         // _0A9C
	XAction mLift;               // _0AA0, the carry action; its mState decides whether looking is free, mTarget is what he carries
	u8 _0ACC[0xAF0 - 0xACC];
	XObj* mLiftPlugTarget; // _0AF0, the GameCube's mLift.mPlugTarget
	u8 _0AF4[0xAF8 - 0xAF4];
	s32 _0AF8; // _0AF8, of the carry action; while it is set he carries at his own joint, not the suit's
	u8 _0AFC[0xB50 - 0xAFC];
	XActHang mHang; // _0B50

	XAction mJump; // _0BA0
	u8 _0BCC[0xBF4 - 0xBCC];
	XAction mLadder; // _0BF4
	u8 _0C20[0xC38 - 0xC20];
	XAction mRope; // _0C38
	u8 _0C64[0xC78 - 0xC64];
	XAction mPush; // _0C78
	u8 _0CA4[0xCD0 - 0xCA4];
	XActTumble mTumble; // _0CD0
	XActCopter mCopter; // _0D18
	XAction mPopper;    // _0D50, the GameCube's mPopper; its first word is the owner
	u8 _0D7C[0x95AC - 0xD7C];
	// his sounds: AnimSfx's head (game/wii/animsfx.h, which needs this header) with two actors, as XObjCoin's;
	// only the two are reached and the radar action follows at _95D4
	u8 mSfx[0x28];  // _95AC
	XAction mRadar; // _95D4
	u8 _9600[0x9694 - 0x9600];
	FrameEventTime mFrameTime;        // _9694, where the running anim's frame events are
	s32 _969C;                        // _969C, cleared on reset; maybe the GameCube's mIdleJointCooldown
	s32 mBlinkWait;                   // _96A0, frames until he blinks again
	s32 mBlinkFrame;                  // _96A4, where the eyelids are in a blink, -1 between blinks
	s8 mBlinkAnims[0x168];            // _96A8, by anim id % 1000, set where he may blink
	XObjScnCamInitRec mEyeAimRecord;  // _9810, a camera aim record fn_800C659C fills; the eye aim
	XObjScnCamInitRec mHeadAimRecord; // _987C, the head aim; the menus snap it and ExecCallbackB applies it to head_chn
	XObjChrLook mLook;                // _98E8
	s32 mAttachment;                  // _9920, the AtcMenu pick
	s32 mAttachmentRequest;           // _9924, -1 when there's none
	s32 mAttachmentStep;              // _9928, how far the attachment change is
	s32 mScriptAttach;                // _992C, an attachment the running script asks for
	s32 mNextClimbType;               // _9930, the climb queued for the next grab
	s32 mClimbType;                   // _9934, the climb he is on
	Vec mFloorTriangle[3];            // _9938, the map triangle under him
	CObjVec mSlopeMotion;             // _995C, the downhill drift
	CVec mFloorSlope;                 // _998C, the GameCube's mFloorSlope
	Params mParams;                   // _999C
	f32 mFallDistance;                // _9DE0
	f32 mCameraTarget[3];             // _9DE4, NANed while the camera keeps no target point
	f32 mCameraTargetMargin;          // _9DF0
	f32 mCameraTargetMarginSetting;   // _9DF4
	f32 mHeldMoveAngle;               // _9DF8, the heading the player snaps back to after a look
	f32 mMoveAngle;                   // _9DFC
	f32 mHeldStickAngle;              // _9E00, NANed while no stick direction is held
	s32 mBatteryWarningFrames;        // _9E04
	f32 mBatteryWarningFade;          // _9E08
	f32 mBatteryWarningAngle;         // _9E0C
	XObj* mGaugeTarget;               // _9E10, the object the battery gauge sits over
	f32 mGaugeTime;                   // _9E14
	s32 mLoadStep;                    // _9E18, the step of LoadFromPlacement's stage load to resume at
	u8 _9E1C[0x9E20 - 0x9E1C];
	WiiPlayerPlug mPlug;   // _9E20, the GameCube's mPlug (XObjPlug)
	XObjWireEff mCordWire; // _ABC0, the plug cord's spark wire, the GameCube's mPlug.mWire
	u8 mCopterObj[0x9B0];  // _B580, the copter attachment object, the GameCube's mCopterObj; its class is unmapped, the code reaches it
	                       // through XObj's vtable
	u8 mPopperObj[0x9B0];  // _BF30, the popper attachment object, the GameCube's mPopperObj
	u8 mRadarObj[0x9C0];   // _C8E0, the radar attachment object, the GameCube's mRadarObj
	u8 mOXObj[0x9B0];      // _D2A0, the OX attachment object, the GameCube's mOXObj
	XObjChrBase mSuits;    // _DC50, the suit objects at _18 and their extra parts at _5868
	CPadOne mPad;          // _18BE8, the pad he reads, the GameCube's mPad
	BOOL mPadReset;        // _19E90, the GameCube's mPad.mReset, moved behind the Wii pad
	u8 _19E94[4];          // _19E94
	f32 mWalkHeading;      // _19E98, NANed until he walks
	AttachmentValues mAttachments; // _19E9C, goes in the save as is
	XObjChrMenu mMenu;             // _19EE4, the attachment and costume rings
	WiiOXMenu mOXMenu;             // _1A420
	u8 _1B7B4[0x1B7B8 - 0x1B7B4];
	// nonzero while he stands on the floor: GetHeight reads 0 and the movement's floor snap skips; only bit 0's
	// writers are decoded so far, the word reads as a whole
	u32 mOnGround;          // _1B7B8
	DustParticlePool mDust; // _1B7BC, the puff pool objects burst into, XObjChr's mDustPool
	u8 _1C770[0x1C774 - 0x1C770];
	CTexObj mTex[2];       // _1C774, the two dust textures
	s32 _1C97C;            // _1C97C, cleared in ResetForNewGame
	s16 mSavedTarget;      // _1C980, the carried object's item index while a release drops it
	u8 _1C982[2];          // _1C982
	s32 mSavedTargetAnim;  // _1C984, its playing anim
	s32 mSavedTargetState; // _1C988, mLift's target state at the release
	XObj* mPendingTarget;  // _1C98C, the respawned carry object RestorePlayerHeldItem waits on
	BOOL mBatteryDrains;   // _1C990, TRUE while his battery runs down and an empty battery stops him
	s32 mSquirterContent;  // _1C994
	union {
		GXColor mItemTint; // _1C998, tints the held item
		s32 mItemTintWord; // _1C998, the save copies it as a word
	};
	f32 mHeadOffset;          // _1C99C
	BOOL mHeadOffsetReady;    // _1C9A0
	s32 mGroundType;          // _1C9A4, the floor he walks on, picks the footstep sounds
	Vec _1C9A8;               // _1C9A8, the GameCube's _158C4; the spider boss zeroes it while flag 0x400 is up
	u8 _1C9B4[4];             // _1C9B4
	f32 _1C9B8[3];            // _1C9B8, read by index; the movement code tests [2] above 0
	f32 _1C9C4[2];            // _1C9C4, two more of the same block, cleared with it on reset
	CVec mEyeAim;             // _1C9CC, the pitch and yaw the eye camera aims shots with
	CVec mEyeAimTarget;       // _1C9DC, where the eye camera eases the aim toward
	CVec mEyeAimStep;         // _1C9EC, the per-frame aim step while the eye camera runs
	CJArchive mAttachmentAnm; // _1C9FC, the attachment's own animation file, the GameCube's mAttachmentAnim
	CJArchive mSuitAnm;       // _1CB44, the worn suit's animation file, the GameCube's mSuitAnim

	// the eye camera takes and releases the player's motion through here: 1 in StartEye, 0 in EndEye
	virtual void vf28C(s32 held); // _28C, the first of XObjChr's own virtuals
	virtual void vf290(BOOL on);  // _290, hides the popper mount while the popper is out
	// snaps his heading back unless a hang, rope, ladder or push action holds it
	void ResetCameraValue()
	{
		if (mHang.mState != 5 && mRope.mState != 5 && mLadder.mState != 5 && mPush.mState != 2) {
			mHeldMoveAngle = mMoveAngle;
		}
	}
	// the attachment chibi robo works with: the pending pick while the ring is closing, else the
	// current one, as AtcMenu::ConfirmSelection in atcmenuselection.cpp computes it
	s32 GetAttachment() const { return mAttachmentRequest >= 0 ? mAttachmentRequest : mAttachment; }

	void CaptureControl()
	{
		mPlayerFlags = 0;
		mPlayerFlags |= 0x80000000;
	}

	void LockControl()
	{
		mPlayerFlags = 0;
		mPlayerFlags |= 0x40000000;
		mPadReset = FALSE;
	}

	void RestoreControl()
	{
		mPlayerFlags = 0;
		mPlayerFlags |= 0x20000000;
		mPlayerFlags |= 7;
		mPadReset = TRUE;
	}
	// the GameCube's GetHeight with the flag inverted: _1B7B8 is up while he stands on the floor, so the
	// height above it only counts while it's clear
	f32 GetHeight() const { return mOnGround ? 0.0f : mMoveTranslate.y - mFloorHit.mHeight; }
	// a copy of the aim the eye camera steers with; the popper's muzzle math takes it by value
	Vec GetEyeAim() const { return mEyeAim; }
};
STATIC_ASSERT(offsetof(WiiPlayerState, mOXMenu) == 0x1A420);
STATIC_ASSERT(offsetof(WiiPlayerState, mGroundType) == 0x1C9A4);
STATIC_ASSERT(offsetof(WiiPlayerState, mHeadNodes) == 0x9AC);
STATIC_ASSERT(offsetof(WiiPlayerState, mLoadStep) == 0x9E18);
STATIC_ASSERT(offsetof(WiiPlayerState, mSfx) == 0x95AC);
STATIC_ASSERT(offsetof(WiiPlayerState, mLook) == 0x98E8);
STATIC_ASSERT(offsetof(WiiPlayerState, mEyeAimRecord) == 0x9810);
STATIC_ASSERT(offsetof(WiiPlayerState, mHeadAimRecord) == 0x987C);
STATIC_ASSERT(offsetof(WiiPlayerState, mStick) == 0x0A08);
STATIC_ASSERT(offsetof(WiiPlayerState, mNearDistance) == 0x0A58);
STATIC_ASSERT(offsetof(WiiPlayerState, mLiftPlugFlags) == 0x0A74);
STATIC_ASSERT(offsetof(WiiPlayerState, mFrameTime) == 0x9694);
STATIC_ASSERT(offsetof(WiiPlayerState, mBlinkAnims) == 0x96A8);
STATIC_ASSERT(offsetof(WiiPlayerState, mScriptAttach) == 0x992C);
STATIC_ASSERT(offsetof(WiiPlayerState, mCameraTarget) == 0x9DE4);
STATIC_ASSERT(offsetof(WiiPlayerState, mHeldStickAngle) == 0x9E00);
STATIC_ASSERT(offsetof(WiiPlayerState, mWalkHeading) == 0x19E98);
STATIC_ASSERT(offsetof(WiiPlayerState, mPad) == 0x18BE8);
STATIC_ASSERT(offsetof(WiiPlayerState, mMenu) == 0x19EE4);

// the coin, item and dust collections each hold a pointer to their 0x80 flag words at +0x9A8
struct WiiFlagSource {
	u8 _000[0x9A8];
	u32* mFlags;
};

class WiiTitleObject;

// a screen color with a texture to draw it through, CGame keeps it at _57CC; the GameCube struct of the same name
struct ScrnTex {
	f32 mAngle;        // _00
	CScrnColor mColor; // _04
	CTexObj mTex;      // _38
};
STATIC_ASSERT(sizeof(ScrnTex) == 0x13C);

// the two screen colors behind it, at _5908 and _593C. Each sits in a struct of its own as on the GameCube, but the
// Wii has a constructor for each (0x80033EC8, 0x80033F0C), so they are two types; the second one's name is ours
struct GameScrnColor1 {
	CScrnColor mColor; // _00
};

class XObj;

// the frame a menu draws behind itself, the GameCube struct one word longer; CMenu::Init sets what is named
struct MenuWindow {
	BOOL mOpen;   // _00
	f32 mX;       // _04, center
	f32 mY;       // _08
	f32 mWidth;   // _0C
	f32 mHeight;  // _10
	CTexObj mTex; // _14
	u8 _118[0x128 - 0x118];
	f32 mDrawAlpha; // _128

	void SetSize(f32 width, f32 height)
	{
		mWidth  = width;
		mHeight = height;
	}
	void SetPos(f32 x, f32 y)
	{
		mX = x;
		mY = y;
	}
};
STATIC_ASSERT(sizeof(MenuWindow) == 0x12C);

// the hand that points at a menu's row, the cursor the menus share (MenuEx's); on the Wii a picture from the
// game's palette, and only its blink is left (UpdateOpenMenus, 0x8008492C)
class MenuCursor {
public:
	s32 mTimer; // _00
	s32 mFrame; // _04, 0 to 7, which of the cursor pictures (palette entry 248 on)
};

// one of the game's menus, the GameCube class of the same name with two more virtuals. Its state leads
// the object and the vtable sits behind the members, at _14C; the menus MenuEx keeps derive from it.
// Reset comes before LoadTexture here: MenuMain's vtable has its Reset at _0C and CMenu::Init calls _0C
class CMenu {
public:
	s32 mState;          // _00, 0 while closed
	f32 mAlpha;          // _04
	s32 mSelection;      // _08, the row the cursor is on
	MenuWindow mWindow;  // _0C
	MenuCursor* mCursor; // _138, MenuEx's
	BOOL mSuspended;     // _13C, another menu is on top
	BOOL mSuspendFade;   // _140, fade out while suspended
	f32 mFade;           // _144
	f32 mSuspendAlpha;   // _148
	// _14C vtable

	~CMenu() { }

	virtual void Init();             // _08
	virtual void Reset();            // _0C
	virtual BOOL LoadTexture();      // _10
	virtual BOOL Update(u32 frames); // _14
	virtual BOOL Draw();             // _18
	virtual void Open();             // _1C
	virtual void Close(s32 state);   // _20, stores state as mState (0x80050D9C)
	virtual BOOL IsOpen() { return mState != 0; }
	virtual void Suspend(BOOL fade); // _28
	virtual void Resume();           // _2C
	virtual void vf28();
	virtual void vf2C();
	virtual void vf38();
};

// a menu of text rows, the GameCube class of the same name
class Menu : public CMenu {
public:
	virtual void InitRows() { }

	f32 mFontSize; // _150
	f32 mMaxWidth; // _154, of the widest row
	f32 mRowCount; // _158
};

class MenuMain : public Menu {
public:
	struct ExtraRow {
		char* mText; // _00
		s32 mAction; // _04
	};

	s32 mBaseRows;          // _15C
	char* mText[14];        // _160
	f32 mTextWidth[14];     // _198
	s32 mChannel;           // _1D0
	ExtraRow mExtraRows[5]; // _1D4

	virtual void Init();
	virtual void Reset();
	virtual BOOL Update(u32 frames);
	virtual BOOL Draw();
	virtual void Open();
	virtual void Close(s32 state);
	virtual void Resume();
};

class MenuStageSelect : public Menu {
public:
	virtual void Init();
	virtual BOOL Update(u32 frames);
	virtual BOOL Draw();
	virtual void Open();

	s32 mCount;            // _15C
	char mLabels[100][64]; // _160, "\x01" marks a stage that can't be picked
	f32 mLabelWidth[100];  // _1A60
};
STATIC_ASSERT(offsetof(MenuMain, mChannel) == 0x1D0);
STATIC_ASSERT(offsetof(MenuStageSelect, mLabelWidth) == 0x1A60);

// the save menu, the GameCube class of the same name. Its RTTI has the bases MenuPad and CContainCallback at
// _15C, as the item and status menus have them; from mResult on the members sit 0x2C behind the GameCube's, the bigger save
// records, and between mFile and mLastFile come the stick's copies of the file and yes/no choice, as Title has them
class MenuSave : public Menu, public MenuPad, public CContainCallback {
public:
	f32 mPanelX;               // _160
	f32 mPanelY;               // _164
	f32 mTargetPanelX;         // _168
	f32 mTargetPanelY;         // _16C
	f32 _170;                  // _170
	f32 mBannerSlide;          // _174
	f32 _178;                  // _178
	f32 mOffset;               // _17C
	WiiMenuSaveSlot mSlots[3]; // _180, the three save files
	u32 mFreeBlocks;           // _27C
	u32 mFreeEntries;          // _280
	s32 mResult;               // _284, the save operation's result
	s32 mMessage;              // _288
	u8 mReady;                 // _28C
	u8 mShowMessage;           // _28D
	u8 mWrongDevice;           // _28E
	u8 mSaved;                 // _28F
	f32 mDrawAlpha;            // _290
	f32 mTargetAlpha;          // _294, mDrawAlpha goes to it
	s32 mMode;                 // _298, the state the table runs
	s32 mPrevMode;             // _29C
	u32 _2A0;                  // _2A0
	s32 mPhase;                // _2A4
	f32 mTime;                 // _2A8
	s32 mDelay;                // _2AC
	s32 mFile;                 // _2B0, the file picked, -1 with the pointer on none
	s32 mStickFile;            // _2B4, the file the stick is on, mFile again when the pointer goes
	s32 mNewestFile;           // _2B8
	s32 mChoice;               // _2BC, yes or no, -1 with the pointer on neither
	s32 mStickChoice;          // _2C0
	s32 mLastFile;             // _2C4, -1 for none
	s32 mChannel;              // _2C8, the GameCube's card slot
	s32 mShowHelp;             // _2CC
	f32 mHelpAlpha;            // _2D0
	const char* mHelpText;     // _2D4
	const char* mNextHelpText; // _2D8
	f32 mTexScroll;            // _2DC
	MenuTitle mTitle;          // _2E0
	s32 mHelpCount;            // _47C
	MenuHelpChar mBg[20];      // _480
	u8 mResetOnClose;          // _570, closing asks the game for reset type 5
	CbPane mPanes[5];          // _574, the three files, yes and no
	f32 mFileAlpha[3];         // _704, each file's highlight

	void SetMode(s32 mode)
	{
		mPrevMode = mMode;
		mMode     = mode;
		mPhase    = 0;
	}

	virtual void Init();
	virtual void Reset();
	virtual BOOL Update(u32 frames);
	virtual BOOL Draw();
	virtual void Open();

	virtual void vf08(CbPane* pane);
	virtual void vf0C(CbPane* pane);
	virtual void vf10(CbPane* pane);
	virtual void vf18(nw4r::math::VEC2* pos);
	virtual void vf1C(nw4r::math::VEC2* pos);

	void State0(u32 frames);
	void State1(u32 frames);
	void State2(u32 frames);
	void State3(u32 frames);
	void State4(u32 frames);
	void State5(u32 frames);
	void State6(u32 frames);
	void State7(u32 frames);
	void State8(u32 frames);
	void State9(u32 frames);
	void State10(u32 frames);
	void State11(u32 frames);
	void State12(u32 frames);
	void State13(u32 frames);
	void State14(u32 frames);
	void State15(u32 frames);
	void State16(u32 frames);
	void State17(u32 frames);
	void State18(u32 frames);
	void State19(u32 frames);
};
STATIC_ASSERT(offsetof(MenuSave, mPanelX) == 0x160);
STATIC_ASSERT(offsetof(MenuSave, mLastFile) == 0x2C4);
STATIC_ASSERT(offsetof(MenuSave, mResetOnClose) == 0x570);
STATIC_ASSERT(sizeof(MenuSave) == 0x710);

// the save file test menus, Wii only (RTTI); the first lists the save slots, the second what to do with one,
// the third shows a save error
// one line of a save file test menu
struct MenuTextRow {
	char* mText; // _00
	f32 mWidth;  // _04

	BOOL IsSeparator() const { return mText != NULL && mText[0] == 1; } // "\x01" rows can't be picked
};

// the save file test menus' common base, named by retail's RTTI: Init lays out the rows through InitRows
class MenuUtil : public Menu {
public:
	virtual void Init();     // _08, 0x80053410
	virtual ~MenuUtil() { }  // _38
	virtual void InitRows(); // _3C, 0x800534D4
};

class MenuSFTest : public MenuUtil {
public:
	MenuSFTest()
	    : mRows(NULL)
	{
	}
	virtual void Init();
	virtual BOOL Update(u32 frames);
	virtual BOOL Draw();
	virtual void Open();
	virtual ~MenuSFTest() { }
	virtual void InitRows(); // _3C, 0x80052F98

	void BuildRows();               // 0x80052FF8, the slot lines and the free space
	void StartOperation(s32 which); // 0x80053540, 0 none, 1 scan, 2 save, 4 load, 6 delete

	MenuTextRow* mRows; // _15C, twelve of them
	s32 mRowTotal;      // _160
	s32 mOperation;     // _164, the save manager job running, 0 when none
	s32 _168;           // _168, -1 on opening
};
STATIC_ASSERT(sizeof(MenuSFTest) == 0x16C);

// Sub and Err keep MenuUtil's Init; its row draw (0x80050314) is still a free function here
class MenuSFTestSub : public MenuUtil {
public:
	MenuSFTestSub()
	    : mRows(NULL)
	{
	}
	virtual BOOL Update(u32 frames);
	virtual BOOL Draw();
	virtual void Open();
	virtual ~MenuSFTestSub() { }
	virtual void InitRows(); // _3C, 0x800548AC

	MenuTextRow* mRows; // _15C, four of them
	s32 mCount;         // _160
};
STATIC_ASSERT(sizeof(MenuSFTestSub) == 0x164);

class MenuSFTestErr : public MenuUtil {
public:
	MenuSFTestErr()
	    : mRows(NULL)
	{
	}
	virtual BOOL Update(u32 frames);
	virtual BOOL Draw();
	virtual void Open();
	virtual ~MenuSFTestErr() { }
	virtual void InitRows(); // _3C, 0x80055318

	MenuTextRow* mRows; // _15C, the one error line
	s32 mCount;         // _160
};
STATIC_ASSERT(sizeof(MenuSFTestErr) == 0x164);

// a pointer pane whose picture fades in while the pane can be picked and out while it can't; the name is ours
struct MenuPaneFade {
	CbPane* mPane; // _00
	f32 mAlpha;    // _04
	u8 mShown;     // _08, the pane's mEnabled as of the last update
	u8 mHovered;   // _09, pointer is over this pane

	void Update(u32 frames)
	{
		mShown    = mPane->mEnabled;
		f32 alpha = mAlpha;
		if (mShown) {
			alpha += 0.2f * frames;
			if (alpha > 1.0f)
				alpha = 1.0f;
		} else {
			alpha -= 0.2f * frames;
			if (alpha < 0.0f)
				alpha = 0.0f;
		}
		mAlpha = alpha;
	}
};

// the two fading panes the item and status menus keep; the name is ours
struct MenuPaneFadePair {
	MenuPaneFade mFades[2]; // _00

	void Update(u32 frames); // 0x8005A048
};

// the item menu, the GameCube class of the same name
class MenuItem : public Menu, public MenuPad, public CContainCallback {
public:
	virtual void Init();
	virtual BOOL Update(u32 frames);
	virtual BOOL Draw();
	virtual void Open();

	virtual void vf08(CbPane* pane);
	virtual void vf0C(CbPane* pane);
	virtual void vf10(CbPane* pane);
	virtual void vf14(CbPane* pane);
	virtual void vf18(nw4r::math::VEC2* pos);
	virtual void vf1C(nw4r::math::VEC2* pos);

	s32 mMode;         // _160, the item menu's phase
	s32 mDelay;        // _164
	f32 mGridX;        // _168
	f32 mGridY;        // _16C
	f32 mTargetGridX;  // _170
	f32 mTargetGridY;  // _174
	f32 mPanelX;       // _178
	f32 mPanelY;       // _17C
	f32 mTargetPanelX; // _180
	f32 mTargetPanelY; // _184
	u8 _188[0x198 - 0x188];
	f32 _198; // _198
	f32 _19C; // _19C
	u8 _1A0[0x1A4 - 0x1A0];
	s32 mPrevSelection;   // _1A4
	s32 mStickSelection;  // _1A8, the row the stick is on, mSelection again when the pointer goes
	s32 mShowHelp;        // _1AC
	f32 mNameAlpha;       // _1B0
	u8* mName;            // _1B4
	u8* mHelp;            // _1B8
	f32 mItemPhase;       // _1BC
	MenuTitle mTitle;     // _1C0
	s32 mHelpCount;       // _35C, set by Open, read by Draw
	MenuHelpChar mBg[20]; // _360
	CbPane mPanes[10];    // _450, 8 item cells plus previous/next item page
	u8 mHovered;          // _770
	u8 _771[0x774 - 0x771];
	MenuPaneFadePair mPaneFades; // _774
};
STATIC_ASSERT(sizeof(MenuItem) == 0x78C);

// the status and PC menus, the GameCube classes of the same names; only their empty states have source yet
class MenuStatus : public Menu, public MenuPad, public CContainCallback {
public:
	void Init();
	BOOL Update(u32 frames);
	void State0(u32 frames);
	void State1(u32 frames);
	void State2(u32 frames);
	void State3(u32 frames);
	void State4(u32 frames);
	void State5(u32 frames);
	void State8(u32 frames);
	void State9(u32 frames);
	void State6(u32 frames);
	void State7(u32 frames);

	virtual BOOL Draw();
	virtual void Open();

	virtual void vf08(CbPane* pane);
	virtual void vf0C(CbPane* pane);
	virtual void vf10(CbPane* pane);
	virtual void vf14(CbPane* pane);
	virtual void vf18(nw4r::math::VEC2* pos);
	virtual void vf1C(nw4r::math::VEC2* pos);

	void ClearSelections();
	void RestoreSelections();

	u8 _160[0x518 - 0x160];
	s32 mMode;                   // _518
	s32 mPrevMode;               // _51C
	s32 mNextMode;               // _520, where a wait mode goes after
	s32 mOverviewSelection;      // _524
	s32 mPrevOverviewSelection;  // _528
	s32 mStickOverviewSelection; // _52C
	s32 mIconSelection;          // _530
	s32 mPrevIconSelection;      // _534
	s32 mStickIconSelection;     // _538
	s32 mChoice;                 // _53C, yes or no when discarding items
	s32 mCount;                  // _540, number to discard
	s32 mStickSelection;         // _544
	s32 mPrevSelection;          // _548
	f32 mOffsetX;                // _54C
	f32 mOffsetY;                // _550
	f32 mTargetOffsetX;          // _554
	f32 mTargetOffsetY;          // _558
	f32 mPanelX;                 // _55C
	f32 mPanelY;                 // _560
	f32 mTargetPanelX;           // _564
	f32 mTargetPanelY;           // _568
	u8 _56C[0x57C - 0x56C];
	f32 mItemPanelX;          // _57C
	f32 mItemPanelY;          // _580
	f32 mTargetItemPanelX;    // _584
	f32 mTargetItemPanelY;    // _588
	f32 mDiscardPanelX;       // _58C
	f32 mDiscardPanelY;       // _590
	f32 mTargetDiscardPanelX; // _594
	f32 mTargetDiscardPanelY; // _598
	u8 _59C[0x5AC - 0x59C];
	MenuStatusTab mTabs[4];  // _5AC
	f32 mX;                  // _5EC
	f32 mY;                  // _5F0
	f32 mTargetX;            // _5F4
	f32 mTargetY;            // _5F8
	f32 mOverviewX;          // _5FC
	f32 mOverviewY;          // _600
	f32 mTargetOverviewX;    // _604
	f32 mTargetOverviewY;    // _608
	f32 mGridX;              // _60C
	f32 mGridY;              // _610
	f32 mTargetGridX;        // _614
	f32 mTargetGridY;        // _618
	f32 mDiagramX;           // _61C
	f32 mDiagramY;           // _620
	f32 mTargetDiagramX;     // _624
	f32 mTargetDiagramY;     // _628
	MenuStatusTab mTabs2[4]; // _62C
	s32 mPhase;              // _66C
	f32 mDelay;              // _670
	s32 mSavedBgm;           // _674
	f32 mSavedBgmVolume;     // _678
	f32 mDetailScale;        // _67C
	f32 mTargetDetailScale;  // _680
	u8 mHasFourthTab;        // _684
	u8 _685[0x689 - 0x685];
	u8 mShowGauge; // _689
	u8 _68A[0x68C - 0x68A];
	u8 mIconLit; // _68C
	u8 _68D[0x690 - 0x68D];
	f32 mGaugeValue;       // _690
	s32 mShowHelp;         // _694
	f32 mHelpAlpha;        // _698
	const char* mHelpText; // _69C
	const char* mHelp;     // _6A0
	f32 mPulsePhase;       // _6A4
	s16 _6A8;
	s16 _6AA;
	s32 mItemSelection;          // _6AC
	s32 mPrevItemSelection;      // _6B0
	s32 mStickItemSelection;     // _6B4
	MenuTitle mTitle;            // _6B8
	s32 mHelpCount;              // _854
	MenuHelpChar mHelpChars[20]; // _858
	CbPane mPanes[51];           // _948, tabs, grid cells and detail controls
	u8 mHovered;                 // _1938
	u8 _1939[0x193C - 0x1939];
	MenuPaneFadePair mPaneFades; // _193C
	s32 _1954;
};
STATIC_ASSERT(sizeof(MenuStatus) == 0x1958);

// the chibi-pc shop, the GameCube class of the same name; its members sit four bytes further on because
// Menu is a word longer here (State4, State11, ResetPCCharacters)
class MenuPC : public Menu {
public:
	void State0(u32 frames);
	void State1(u32 frames);
	void State2(u32 frames);
	void State3(u32 frames);
	void State4(u32 frames);
	void State5(u32 frames);
	void State6(u32 frames);
	void State7(u32 frames);
	void State8(u32 frames);
	void State9(u32 frames);
	void State10(u32 frames);
	void State11(u32 frames);

	virtual void Init();
	virtual BOOL Update(u32 frames);
	virtual BOOL Draw();
	virtual void Open();

	s32 mMode;              // _15C
	s32 mPrevMode;          // _160
	u32 _164;               // _164
	s32 mPhase;             // _168
	f32 mSpeed;             // _16C
	s32 mTextPhase;         // _170
	s32 mChoice;            // _174
	s32 mItemShopSelection; // _178
	s32 _17C;
	s32 _180;
	s32 mYesNo;              // _184
	s32 mMoneyDigit;         // _188
	s32 mScrapShopSelection; // _18C
	s32 mScrapDigit;         // _190
	f32 mProgress;           // _194
	f32 mTargetProgress;     // _198
	f32 _19C;                // _19C, eased toward _1A4
	f32 mFrameHeight;        // _1A0, the frame quad grows to its target on opening and closes back to 2
	f32 _1A4;                // _1A4
	f32 mTargetFrameHeight;  // _1A8
	u8 _1AC[0x1BC - 0x1AC];
	s32 mCharIndex;          // _1BC
	u32 mGreetingVariant;    // _1C0
	f32 mTextX;              // _1C4
	f32 mTextY;              // _1C8
	f32 mTargetX;            // _1CC
	f32 mTargetY;            // _1D0
	char mChars[80];         // _1D4, forty two byte sjis characters
	u8 mTerminator;          // _224
	f32 mCharX[40];          // _228
	u8 mCharFlags[40];       // _2C8
	char mCharCopies[40][3]; // _2F0, each character again as a string of its own
	u32 mGlyphFrames;        // _368
	u32 mNextGlyph;          // _36C
	s32 mKeeperAnimStep;     // _370, the shopkeeper's anim step
	s32 mKeeperAnimId;       // _374, the anim for it
	u8 mPlayKeeperAnim;      // _378, set the anim
	u32 mNumberOffset;       // _37C
	u8 mInsertsNumber;       // _380
	s32 mAmount;             // _384
	s32 mDepositBalance;     // _388
	s32 mItemShopBase;       // _38C
	s32 mItemShopCount;      // _390
	f32 mPanelAlpha;         // _394
	f32 mPanelTarget;        // _398
	f32 mListAlpha;          // _39C
	f32 mTargetListAlpha;    // _3A0
	u8 mShopOnly;            // _3A4, opens straight into the item list and B closes
	u8 _3A5[0x3A8 - 0x3A5];
	u32 mDialogueNumber;          // _3A8
	f32 mRankingOffsets[5];       // _3AC
	PCRankingRow mRankingRows[5]; // _3C0
	s32 mRankingEnd;              // _3FC
	f32 mScroll;                  // _400
	f32 mScrollTarget;            // _404
	s32 mScrapShopBase;           // _408
	s32 mScrapShopCount;          // _40C
	PCAmount mMoneyDisplay;       // _410
	PCAmount mScrapDisplay;       // _418
	PCAmount mDepositDisplay;     // _420
	f32 _428;                     // _428, eased toward _42C
	f32 _42C;                     // _42C
	f32 mScrapRate;               // _430
	s32 mExitState;               // _434, the game state it hands back
	u8 mCanSelect;                // _438
	u8 _439[0x43C - 0x439];
	s32 mChoices[4];  // _43C
	s32 mChoiceCount; // _44C
	f32 mTexScroll;   // _450
};
STATIC_ASSERT(offsetof(MenuPC, mYesNo) == 0x184);
STATIC_ASSERT(offsetof(MenuPC, mCharCopies) == 0x2F0);
STATIC_ASSERT(offsetof(MenuPC, mNumberOffset) == 0x37C);
STATIC_ASSERT(offsetof(MenuPC, mShopOnly) == 0x3A4);
STATIC_ASSERT(offsetof(MenuPC, mDepositDisplay) == 0x420);
STATIC_ASSERT(offsetof(MenuPC, mExitState) == 0x434);
STATIC_ASSERT(offsetof(MenuPC, mTexScroll) == 0x450);

// frame buffer effects that draw through a copied texture; the Wii base keeps the bigger
// WiiFbTexture and drops mActive (WiiFbTexture::mOwnsImage took its slot), the vtable sits
// at _124 right behind the texture.
class CFbTexEff {
public:
	WiiFbTexture mTexture; // _00
	// _124 vtable

	virtual void Init();             // _08
	virtual void Reset();            // _0C
	virtual void Copy();             // _10
	virtual void Update(u32 frames); // _14
	virtual void Draw();             // _18
};

// Wii FbSubwin: CFbTexEff with the fade state and its own perspective camera at _148.
class FbSubwin : public CFbTexEff {
public:
	BOOL mEnabled;    // _128, fades in while set, out while not
	CStatus32 mFlags; // _12C, Draw tests bit 2
	f32 mAmount;      // _134, 0 to 1
	f32 mRectX;       // _138, centre x of the window's rect on screen
	f32 mRectY;       // _13C, centre y of the window's rect on screen
	f32 mRectWidth;   // _140, window width, NaN (with mRectHeight) picks the round frame
	f32 mRectHeight;  // _144, window height, NaN picks the round frame
	CCamera mCamera;  // _148

	virtual void Init();
	virtual void Reset();
	virtual void Copy();
	virtual void Update(u32 frames);
	virtual void Draw();
};
STATIC_ASSERT(offsetof(FbSubwin, mEnabled) == 0x128);
STATIC_ASSERT(offsetof(FbSubwin, mAmount) == 0x134);
STATIC_ASSERT(offsetof(FbSubwin, mCamera) == 0x148);
STATIC_ASSERT(sizeof(FbSubwin) == 0x340);

// the model object a mirror binds to: a stage object's scene carrier. Only what the retail
// functions prove is filled in - the vtable's _50 finds a node by name ("mirror" is the
// mirror surface), _A0 returns a node's matrix, _EC is the model's nw4r resource handle and
// _F0 a related data object. The name is ours.
class CMirrorObj {
public:
	virtual void vf08();
	virtual void vf0C();
	virtual void vf10();
	virtual void vf14();
	virtual void vf18();
	virtual void vf1C();
	virtual void vf20();
	virtual void vf24();
	virtual void vf28();
	virtual void vf2C();
	virtual void vf30();
	virtual void vf34();
	virtual void vf38();
	virtual void vf3C();
	virtual void vf40();
	virtual void vf44();
	virtual void vf48();
	virtual void vf4C();
	virtual nw4r::g3d::ResNode vf50(const char* name); // _50
	virtual void vf54();
	virtual void vf58();
	virtual void vf5C();
	virtual void vf60();
	virtual void vf64();
	virtual void vf68();
	virtual void vf6C();
	virtual void vf70();
	virtual void vf74();
	virtual void vf78();
	virtual void vf7C();
	virtual void vf80();
	virtual void vf84();
	virtual void vf88();
	virtual void vf8C();
	virtual void vf90();
	virtual void vf94();
	virtual void vf98();
	virtual void vf9C();
	virtual MtxP vfA0(nw4r::g3d::ResNode node); // _A0

	u8 _04[0xE8];
	nw4r::g3d::ResMdl mMdl;       // _EC, the model's nw4r ResMdl handle
	nw4r::g3d::ScnMdlSimple* mF0; // _F0, the model the node matrices live in
	u8 _F4[0x150 - 0xF4];
	u64 mStatus; // _150, the object's status flags
};

// a mirror, renders the scene again through a camera reflected in a named node's plane;
// the Wii version keeps its own 256-square texture and binds the model's "mirror_mat"
// material animation so the mirror shows the copy
class FbMirror : public CFbTexEff {
public:
	CMirrorObj* mJoint;     // _128
	GXTexObj* mJointAnm;    // _12C, the ResAnmTexPat bound to "mirror_mat"
	GXTexObj mJointAnmCopy; // _130, a copy of the anm with the mirror's texture patched in
	CCamera mCamera;        // _150
	BOOL mVisible;          // _348, the eye is in front of the mirror
	Mtx mView;              // _34C

	virtual void Init();             // _08, Init__8FbMirrorFv
	virtual void Reset();            // _0C, Reset__8FbMirrorFv
	virtual void Copy();             // _10, Copy__8FbMirrorFv
	virtual void Update(u32 frames); // _14, Update__8FbMirrorFUl
	virtual void Draw();             // _18, Draw__8FbMirrorFv

	void SetMirror(CMirrorObj* obj); // fn_80019F24
	bool IsUsed() const { return mJoint ? true : false; }
};
STATIC_ASSERT(sizeof(FbMirror) == 0x37C);

// the effect manager (ceffect.cpp), CGame's at _B7A8, the GameCube's mParticle; Update hands it the camera each frame.
// Class name from RTTI, the constructor in 0x800332D8 stores its vtable at _4FC and builds the two sets at _08
class CEffectSystem {
public:
	// one set of effect resources, with two CFiles at _84 (constructor 0x8003467C, destructor 0x8002F5D4); the name is ours
	struct Resource {
		s32 mState;                           // _00
		char mName[0x80];                     // _04
		CFile mFiles[2];                      // _84
		nw4r::ef::EffectProject* mEffectProj; // _25C
		nw4r::ef::TextureProject* mTexProj;   // _260
		u32 mNumEmitter;                      // _264
	};

	void* mMemoryPool;      // _00
	void* mMemoryManager;   // _04
	Resource mResources[2]; // _08
	u32 mLightMask;         // _4D8
	GXColor mLightColor;    // _4DC
	GXColor _4E0;           // _4E0
	u32 _4E4;               // _4E4
	f32 _4E8;               // _4E8
	f32 _4EC;               // _4EC
	f32 _4F0;               // _4F0
	f32 _4F4;               // _4F4
	GXColor _4F8;           // _4F8
	// _4FC vtable

	virtual ~CEffectSystem() { }
	void Init();
	BOOL Load(const char* name, s32 idx);
	BOOL UpdateLoad(s32 idx);
	void Unload(s32 idx);
	void Update(const Vec* camPos, MtxP camMtx, MtxP projMtx, u32 steps, s32 groupID, f32 f1, f32 f2);
	void Draw(MtxP viewMtx, s32 groupID);
	void RetireAll(u32 groupID, BOOL retireParticles);
	void UpdateLights(CLight* light);
	void SetFog(u32 param, const GXColor* color, f32 f1, f32 f2, f32 f3, f32 f4);
};
STATIC_ASSERT(sizeof(CEffectSystem) == 0x500);

// the event particles, CGame's at _BCA8 (class name from RTTI, vtable at _3A00 by the constructor in 0x800332D8):
// the emitter handles, stepped with the camera like the global at 0x8044EE4C
class CEvtPtcl {
public:
	EffectInstance mEffects[0x80]; // _00
	// _3A00 vtable

	virtual ~CEvtPtcl() { }
};
STATIC_ASSERT(sizeof(CEvtPtcl) == 0x3A04);

// the pooled object archives, the GameCube struct of the same name; CGame's destructor calls its
// destructor (0x8002F2E8) on _108E0
struct ObjArchiveTable {
	ResourceFile mArchives[0x400]; // _00
};

// the menus and what they share, the GameCube class of the same name (RTTI); the constructor in 0x800332D8 stores
// its vtable at _30, ahead of the Wii's further members, and builds the CTexObj at _50

class MenuEx {
public:
	BOOL mActive;         // _00, a menu is open
	CMenu* mMenus[9];     // _04, [0] is the main menu
	MenuCursor mCursor;   // _28
	virtual ~MenuEx() { } // _30 vtable
	s32 _34;              // _34, DrawHud draws nothing while a menu is open and this is set
	u8 _38[0x50 - 0x38];
	CTexObj mTex; // _50
	u8 _154[0x158 - 0x154];
};
STATIC_ASSERT(sizeof(MenuEx) == 0x158);

// the pointer's cursor and what hangs off it, a struct of its own: CGame's destructor calls one destructor
// (0x8002D438) on _EC1A8 and the constructor builds it in 0x800351CC, with the Cursor (RTTI) at _128. The name is ours
struct GamePointer {
	f32 mAlpha;      // _000, fades the cursor; a new game starts it at 1
	f32 mScopeTimer; // _004, the robo eyes' lenses pulse while it runs and hide from 5 on
	CTexObj mTex;    // _008, a copy of misc.tpl's texture 261
	f32 _10C;        // _10C, a new game starts it at NAN
	CTexObj* _110;
	f32 _114;
	f32 _118;
	u32 _11C;
	f32 _120; // _120, a new game starts it at -1
	u8 _124[0x128 - 0x124];
	u8 mCursor[0xB40 - 0x128]; // _128, the Cursor BeforeUpdate places at spawn 12; xobj.h needs this header
	// _B40 to _B7C: one block FinalizeStageObjectSetup resets when the stage is up.
	// _B40 is an object DrawHud draws through its vf19C at the two coordinates behind it, when there is one
	XObj* mHudObject;
	f32 mHudObjectX;
	f32 mHudObjectY;
	// _B4C on: the object the roboeyes' scope picks, nearest to the camera among those the pointer is on
	XObj* mScopeTarget;
	f32 mScopeSpeed;      // _B50, how fast the pointer moved when it picked it
	s32 mScopeMarker;     // _B54, the target's _18C, -1 when it is an item
	s32 mScopeItem;       // _B58, the target's item, -1 when it has a marker
	s32 mScopeTicks;      // _B5C, marker sounds since the last long one
	BOOL mScopeRareItem;  // _B60, the item is one of a few special ones
	s32 mScopeLevel;      // _B64, 1 to 3 by the speed, picks the row of the stage's sounds
	s32 mScopeSoundIndex; // _B68
	s32 mScopeSound;      // _B6C, the sound last played for the target, -1 for none
	f32 mScopeZoom;       // _B70, eases back to 1
	f32 mScopeAngle;      // _B74, turns while the zoom is up
	f32 mScopeFade;       // _B78
	f32 mScopeFade2;      // _B7C
	u8 mFloorReflect;     // _B80, an object asked for the floor reflection
	Mtx mFloorReflectMtx; // _B84, PSMTXReflect about the floor
};
STATIC_ASSERT(sizeof(GamePointer) == 0xBB4);

// the point the 3D sound listens from, a Wii piece in a struct of its own: CGame's destructor calls one destructor
// (0x8002D288) for both vectors and the constructor in 0x800332D8 builds them together. The names are ours
struct GameListener {
	CVec mPos;   // _00, a new game zeroes it
	CVec mPoint; // _10, a new game fills it with NAN; the listener stands 50 in front of it, towards the camera
};

// the shadow camera of RenderSceneShadow and the texture it renders to, a struct of its own like GameListener
// (destructor 0x8002D21C, built by 0x80035464 in the constructor); the name is ours
struct GameShadow {
	CCamera mCamera; // _000
	CTexObj mTex;    // _1F8, shows it
	u16 mTexWidth;   // _2FC
	u16 mTexHeight;  // _2FE
	void* mImage;    // _300, what the frame buffer is copied to
	f32 mStrength;   // _304, the alpha the shadows draw at
	u8 _308[0x320 - 0x308];
};
STATIC_ASSERT(sizeof(GameShadow) == 0x320);

class CGame : public CBase {
public:
	virtual ~CGame(); // _08, 0x8002CFB8
	// the mirrors CGame holds. RTTI names the class "CGame::FbMirrorEx"; its vtable (0x802E5FF4) has FbMirror's five
	// functions and it adds no members, the constructor at 0x80035418 is the compiler's
	class FbMirrorEx : public FbMirror { };

	void Initialize(int argc, char** argv); // _0C override, parses /stage: /slot: (gameinit.cpp)
	void Start();                           // _10 override, 0x8001BC60 (gameinit.cpp)
	CCamera* GetCamera();                   // _4C override, &mMotionControlCamera
	// _24, _28, _2C, _30 (the logos, title and menus run at 60, the game at 30), _68 mLightList, _84
	void Draw();
	void DrawScene();
	void DrawHud();
	s32 FrameInterval();
	CLight* GetLight();
	void Reset(u32 code);

	void ResetNewGameState();        // _14 override, 0x8001C37C
	BOOL BeforeUpdate();             // _1C override, 0x8001C7B0
	void Update(u32 frames);         // _20 override, 0x8001E830
	void SetupStageCamera(s32 mode); // 0x8001DC88

	s32 mLoadStep;
	s32 mSetupStep; // _41EC, how far FinalizeStageObjectSetup is, done at 5
	s32 _41F0; // _41F0, frames until the stage's objects are read again (BeforeUpdate's steps 20 to 22 wait for it); ChangeGameStage sets
	           // it to 3
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
	FbEffect mOutline;        // _511C
	FbBlur mBlur;             // _52E4
	FbSubwin mSubwin;         // _548C
	ScrnTex mScrnTex;         // _57CC
	GameScrnColor mColor0;    // _5908
	GameScrnColor1 mColor1;   // _593C
	s32 mOutlineWidthControl; // _5970
	f32 mOutlineWidth;        // _5974
	Logo mLogo;               // _5978
	Title mTitle;             // _5C70
	MenuEx mMenu;             // _AE00
	InfoScrn mInfoScrn;       // _AF58
	WiiHomeMenu mHomeMenu;    // _B148
	CEffectSystem mEffects;   // _B7A8
	CEvtPtcl mEvtPtcl;        // _BCA8
	CScrnQuake mQuake;        // _F6AC
	// the objects, as on the GameCube: pool 0 is the scene camera, 2 chibi robo, 3 the stage map, 7 the coins, 8 the items,
	// 10 the spawn pool CMsgFont_Draw scans for the portrait object, 20 the dust; mObjects is the table
	// ResolveEventObjectId indexes. The coin, item and dust pools each lead with a WiiFlagSource
	XOBJS mXObjs; // _F6E4
	// [0] the objects in draw order, BuildObjectDrawList fills it; [1] the spawned objects the renderer walks, entries
	// grouped in eights, CMsgFont_Init pushes its two cursors here; [2] the objects DrawScene draws and [3] the ones whose
	// effects are drawn (the ones the mirrors show), BuildSceneDrawLists fills both each frame
	XObjDrawList mDrawLists[4];   // _108C0
	ObjArchiveTable mObjArchives; // _108E0
	CEvt mEvt;                    // _4C8E0
	u8 _4C94C[0x4C950 - 0x4C94C];
	CMsg mMsgs[2];                             // _4C950
	EvtTimeState mTimeState;                   // _D7610
	CMotionControlCamera mMotionControlCamera; // _D7620
	CCameraActions mCameraActions;             // _D78F8, mTable lands at _D9B24
	CLight* mLightList;                        // _D9B58, the lights the scene draws under, CBase's own set when the stage brings none
	s32 _D9B5C;                                // _D9B5C, ResetNewGameState clears both
	s32 _D9B60;                                // _D9B60
	u8 _D9B64[0xD9B80 - 0xD9B64];
	CSaveFile mSaveManager; // _D9B80
	u8 _D9B80[0xEC180 - 0xD9B80 - sizeof(CSaveFile)];
	BOOL mMemcardLoaded; // _EC180
	u8 _EC184[0xEC1A0 - 0xEC184];
	f32 mPlayTime;          // _EC1A0
	f32 _EC1A4;             // _EC1A4
	GamePointer mPointer;   // _EC1A8
	FbMirrorEx mMirrors[5]; // _ECD5C
	StageName mStageName;   // _EDEC8
	CMovie mMovie;          // _EE104; AnimSfx::CheckStart starts no sound while its first word is set
	u32 mMovieFlags;        // _EE1C0, the Start flags
	f32 mMovieRect[4];      // _EE1C4, where the movie draws, NAN for its own size and the centre
	f32 mMovieVolume;       // _EE1D4
	bool mMovieOnThread;    // _EE1D8, the staff roll: UpdateMovie steps it on a thread of its own
	u8 _EE1D9[0xEE1DC - 0xEE1D9];
	u32 mMenuControls;
	GameListener mListener; // _EE1E0
	// _EE200 to _EE518: the shadow RenderSceneShadow casts from straight above, a Wii piece the GameCube had
	// as a CShadow on the light; the names are ours
	XObj* mShadowFocus; // _EE200, BeforeUpdate's step 27 resets the four; what the shadow camera looks down on, the scene camera's interest
	                    // without one
	f32 mShadowWidth;   // _EE204, 100 unless positive
	f32 mShadowHeight;  // _EE208, 100 unless positive
	f32 mShadowAlpha;   // _EE20C, 1 unless positive
	GameShadow mShadow; // _EE210

	void SetStage(s32 stage)
	{
		mPrevStage = mStage;
		mStage     = stage;
	}

	// the checks OpenWiiHomeMenu makes. Retail keeps each one's result apart, so they are separate
	// inline functions; where one ends and the next begins, and all their names, are ours
	bool IsLoadStepRunning() const { return mLoadStep > 0 && mLoadStep < 30; }
	bool IsLogoActive() const { return mLogo.mState >= 1 && mLogo.mState < 12; }
	bool IsTitleActive() const { return mTitle.mState >= 1 && mTitle.mState < 23; }
	bool IsLogoShown() const { return IsLogoActive() && mLogo.mState > 2; }
	bool IsIntroShown() const { return IsLogoShown() || IsTitleActive(); }
	// loading with neither the logos nor the title on screen
	bool IsLoading() const { return IsLoadStepRunning() && !IsIntroShown(); }
	bool IsInterruptible() const { return !IsLoading() && mTitle.mState != 20; }
	bool IsMenuFree() const { return IsInterruptible() && !mMenu.mActive; }
	// the stage runs: no suspend, no menu, the disc readable and no reset coming
	bool IsRunning() const
	{
		return GetStatusFlags(1) && !GetStatusFlags(0x200000000000022ULL) && !mMenu.mActive && !IsDiscError() && !mResetRequested;
	}

	CMsg* GetMsg(s32 index) { return &mMsgs[index]; }
	WiiPlayerState* GetPlayer() const { return (WiiPlayerState*)mXObjs.mPools[2].mObjects; }
	// 0x80037134, the sound code asks for player 0 through the pool
	WiiPlayerState* GetPlayer(s32 index) { return (WiiPlayerState*)(mXObjs.mPools[2].mObjects + index * mXObjs.mPools[2].mStride); }
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
STATIC_ASSERT(offsetof(CGame, mXObjs) == 0xF6E4);
STATIC_ASSERT(offsetof(CGame, mPointer) == 0xEC1A8);
STATIC_ASSERT(offsetof(CGame, _D9B5C) == 0xD9B5C);
STATIC_ASSERT(offsetof(CGame, mMemcardLoaded) == 0xEC180);
STATIC_ASSERT(offsetof(CGame, mStageName) == 0xEDEC8);
STATIC_ASSERT(offsetof(CGame, mSetupStep) == 0x41EC);
STATIC_ASSERT(offsetof(CGame, mDrawLists) == 0x108C0);
STATIC_ASSERT(offsetof(CGame, mLightList) == 0xD9B58);
STATIC_ASSERT(offsetof(WiiPlayerState, mPopper) == 0xD50);
STATIC_ASSERT(offsetof(CGame, mMirrors) == 0xECD5C);
STATIC_ASSERT(offsetof(CGame, mListener) == 0xEE1E0);
STATIC_ASSERT(offsetof(CGame, mSubwin) == 0x548C);
STATIC_ASSERT(offsetof(CGame, mEvtPtcl) == 0xBCA8);
STATIC_ASSERT(offsetof(CGame, mEffects) == 0xB7A8);
STATIC_ASSERT(offsetof(CGame, mInfoScrn) == 0xAF58);
STATIC_ASSERT(offsetof(CGame, mObjArchives) == 0x108E0);
STATIC_ASSERT(offsetof(CGame, mMsgs) == 0x4C950);
STATIC_ASSERT(offsetof(CGame, mShadow) == 0xEE210);
STATIC_ASSERT(offsetof(CGame, _41F0) == 0x41F0);
STATIC_ASSERT(offsetof(CGame, mOutline) == 0x511C);
STATIC_ASSERT(offsetof(CGame, mSaveManager) == 0xD9B80);
STATIC_ASSERT(offsetof(CGame, mShadowFocus) == 0xEE200);
STATIC_ASSERT(offsetof(CBase, _41E0) == 0x41E0);
STATIC_ASSERT(offsetof(CGame, mBlur) == 0x52E4);
STATIC_ASSERT(offsetof(CGame, mMenu) == 0xAE00);
STATIC_ASSERT(offsetof(CGame, mTitle) == 0x5C70);
STATIC_ASSERT(sizeof(Title) == 0xAE00 - 0x5C70);
STATIC_ASSERT(offsetof(CGame, mLogo) == 0x5978);
STATIC_ASSERT(offsetof(CGame, mEvt) == 0x4C8E0);

extern "C" CGame gGame;

extern "C" s32 FindStageIndexByName(const char* name);

// whether the stage up is the one with the name
static inline bool IsStage(const char* name)
{
	s32 stage = gGame.mStage;
	s32 id    = FindStageIndexByName(name);
	return id == stage;
}
#endif
