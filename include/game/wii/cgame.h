#ifndef GAME_WII_CGAME_H
#define GAME_WII_CGAME_H

#include "Dolphin/mtx.h"
#include "game/wii/cpad.h"
#include "game/wii/csound.h"
#include "game/wii/cgs.h"
#include "game/wii/homemenu.h"
#include "game/cscrn.h"
#include "game/ut.h"
#include "game/inventory.h"

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
	u32 mTvFormat;             // _0000, 0 NTSC, 1 PAL, 2 M-PAL, 3 EURGB60
	u8 mProgressive;           // _0004
	CGs mGs;                   // _0008
	CPadEx mPad;               // _2298
	XSound mSound;             // _22A4
	u32 mLastRetrace;          // _33E4
	u32 mRetraceStep;          // _33E8
	CStatus64 mStatus;         // _33F0
	CTpl mIconTpl;             // _3400, the icons shown in the screen's corner; the first says the HOME Menu is not available
	u8 _35F8[0x4148 - 0x35F8]; // the perspective and the screen camera, 0x1F8 each, start here
	void* mMem1Base;           // _4148, 0x80000000
	void* mMem1Start;          // _414C, the MEM1 arena's start before the heaps are made
	void* mMem1ArenaLo;        // _4150
	void* mMem1ArenaHi;        // _4154
	void* mMem2ArenaLo;        // _4158
	void* mMem2ArenaHi;        // _415C
	u8 mLanguage;              // _4160
	u8 mAspectRatio;           // _4161, SCGetAspectRatio: 0 is 4:3, 1 is 16:9
	s32 mDvdStatus;            // _4164
	s32 mPrevDvdStatus;        // _4168
	BOOL mDiskOk;              // _416C
	u8 _4170[0x41A8 - 0x4170];
	bool mResetRequested; // _41A8
	s32 mResetType;       // _41AC, 3 restarts the game, 4 returns to the Wii Menu
	u8 _41B0[0x41E4 - 0x41B0];
	// _41E4 vtable

	virtual ~CBase();                               // _08
	virtual void Initialize(int argc, char** argv); // _0C
	virtual void Start();                           // _10, 0x80017D4C
	virtual void vf14();
	virtual void vf18();
	virtual void vf1C();
	virtual void vf20();
	virtual void vf24();
	virtual void vf28();
	virtual void vf2C();
	virtual void vf30();
	virtual void SetStatusFlags(u64 mask);   // _34, 0x8001D5B4
	virtual void ClearStatusFlags(u64 mask); // _38, 0x8001C78C
	virtual void vf3C();
	virtual u64 GetStatusFlags(u64 mask) const; // _40, the game's status flags under mask
	virtual void vf44();
	virtual CCamera* GetScreenCamera(); // _48, the orthographic camera the screen-space layer draws under

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
	CVec _30;       // _30
	f32 mDistance;  // _40
	u8 mDirty;      // _44, the view matrix is out of date

	void CalcEye();      // 0x802A855C
	void CalcAngle();    // 0x802A8734
	void CalcDistance(); // 0x802A896C, between the eye and the interest
};

// the Wii camera object behind gGame's camera member. A different layout than the GameCube CCamera,
// but the setters the start-up code and the save manager call sit on its vtable at the offsets below
class CCamera {
public:
	virtual ~CCamera() { }                                                                  // _08
	virtual void Init(f32 near, f32 far, f32 fov, f32 scale, const GXRenderModeObj* rmode); // _0C
	virtual void vf10();                                                                    // _10
	// an orthographic camera
	virtual void Init(f32 top, f32 bottom, f32 left, f32 right, f32 near, f32 far, f32 scale, const GXRenderModeObj* rmode); // _14
	virtual void SetFov(f32 fov);                                                                                            // _18
	virtual void vf1C();                                                                                                     // _1C
	virtual void vf20();                                                                                                     // _20
	virtual void vf24();                                                                                                     // _24
	virtual void vf28();                                                                                                     // _28
	virtual void vf2C();                                                                                                     // _2C
	virtual void vf30();                                                                                                     // _30
	virtual void SetEye(f32 x, f32 y, f32 z);                                                                                // _34
	virtual void SetInterest(f32 x, f32 y, f32 z);                                                                           // _38
	virtual void SetAngle(f32 x, f32 y, f32 z)                                                                               // _3C
	{
		mView.mAngle.x = x;
		mView.mAngle.y = y;
		mView.mAngle.z = z;
		mView.mDirty   = TRUE;
	}
	virtual void vf40();                                  // _40
	virtual void SetDistance(f32 distance);               // _44
	virtual void SetEye(const Vec& eye);                  // _48
	virtual void SetInterest(const Vec& interest);        // _4C
	virtual void SetAngle(const Vec& angle);              // _50
	virtual void vf54();                                  // _54
	virtual void SetAngleY(f32 y);                        // _58, ConfigureSubwindowCamera passes angleY here
	virtual void SetAngleX(f32 x);                        // _5C, then angleX here - same order as the GameCube CCamera
	virtual void vf60();                                  // _60
	virtual void vf64();                                  // _64
	virtual void vf68();                                  // _68
	virtual void vf6C();                                  // _6C
	virtual void CalcEye();                               // _70
	virtual void vf74();                                  // _74
	virtual void CalcAngle() { mView.CalcAngle(); }       // _78
	virtual void vf7C();                                  // _7C
	virtual void CalcDistance() { mView.CalcDistance(); } // _80
	virtual void vf84();                                  // _84
	virtual void vf88(f32, f32, f32, f32, f32, f32);      // _88, FbSubwin::Init passes (0, 0, w, h, 0, 1)

	u8 _04[0x4];
	f32 mFov; // _08
	u8 _0C[0x18 - 0x0C];
	f32 mTop;    // _18, ortho projection bounds, fed to C_MTXOrtho
	f32 mBottom; // _1C
	u8 _20[0x2C - 0x20];
	Mtx44 mProjection; // _2C
	u8 _6C[0x90 - 0x6C];
	CCameraView mView; // _90
	Mtx mViewMtx;      // _D8, the computed view matrix the 2D draws multiply into
	u8 _108[0x1F8 - 0x108];
};
STATIC_ASSERT(sizeof(CCamera) == 0x1F8);

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

// the Wii motion control camera, a CCamera object with the action interface on top. The restore
// uses the base setters; ChangeAction, GetActionType and SetMemento are named in the binary
class CMotionControlCamera : public CCamera {
public:
	u8 _1F8[0x234 - 0x1F8];
	CInterfaceCameraAction* mAction; // _234, the running action, GetActionType finds it in the table
	u8 _238[0x2504 - 0x238];

	s32 GetActionType();          // 0x80169548, mTable index of mAction
	void ChangeAction(long type); // 0x80169768
	BOOL SetMemento(const CCameraMemento* memento);
};

// the action pointers indexed by CAMACTION_*, CGame keeps them at +0xD9B24
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
	void* _94[(0x244 - 0x94) / 4];
	const Vec* (*GetPosition)(WiiPlayerState* chr);
};

// the yes/no menu CMsg__cmd_QUESTION opens on the player, the GameCube's mOXMenu. its input poll
// is HandleOXMenuInput at 0x800EF58C
struct WiiOXMenu {
	u8 _00[4];
	s32 mStarted; // _04
	u8 _08[0x1388];
	s32 mConfirmedIndex; // _1390
};

// the Wii player object: a different, larger class than the GameCube XObjChr. These are the members
// the save manager reads and writes, at offsets verified against retail.
struct WiiPlayerState {
	WiiPlayerVtable* mVtable;
	u8 _0004[0x64 - 0x04];
	f32 mRotY; // _0064, mMoveRotate.y
	u8 _0068[0x9920 - 0x68];
	s32 mAttachment;        // _9920, the AtcMenu pick
	s32 mAttachmentRequest; // _9924, -1 when there's none
	s32 mAttachmentStep;    // _9928, how far the attachment change is
	u8 _992C[0x999C - 0x992C];
	WiiPlayerParams mParams; // _999C
	u8 _9DDC[0xDC50 - 0x9DDC];
	WiiPlayerSuits mSuits; // _DC50
	u8 _DC64[0x19E9C - 0xDC64];
	u8 mAttachments[0x48]; // _19E9C
	u8 _19EE4[0x1A420 - 0x19EE4];
	WiiOXMenu mOXMenu; // _1A420
	u8 _1B7B4[0x1C994 - 0x1B7B4];
	s32 mSquirterContent; // _1C994
	s32 mItemTint;        // _1C998

	const Vec* GetPosition() { return mVtable->GetPosition(this); }
};

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

class CGame : public CBase {
public:
	void Initialize(int argc, char** argv); // _0C override, parses /stage: /slot: (gameinit.cpp)

	s32 mLoadStep;
	u8 _41EC[0x42FC - 0x41EC];
	s32 mStage;
	s32 mPrevStage; // _4300
	u8 _4304[0x4B40 - 0x4304];
	CTpl mTpl;
	CScrnFade mFade; // _4D38
	u8 _4D6C[0x5044 - 0x4D6C];
	u32 mWipeTileActive; // _5044, the screen wipe's active flag, the GameCube's mWipeTile1.mActive
	u8 _5048[0x5078 - 0x5048];
	CScrnRoboeyes mRoboeyes;
	u8 _511C[0x5978 - 0x511C];
	s32 mLogoState; // _5978, the boot logos' step, as the GameCube mLogo.mState
	u8 _597C[0x5C74 - 0x597C];
	s32 mTitleState; // _5C74, the title screen's step, as the GameCube mTitle.mState
	u8 _5C78[0xAE00 - 0x5C78];
	BOOL mMenuActive; // _AE00, a menu is open, as the GameCube mMenu.mActive
	u8 _AE04[0xAF58 - 0xAE04];
	// the object the InfoScrn_* HUD calls receive at +0xAF58; the Wii callbacks are
	// empty stubs, so only its address is verified, not its layout
	u8 mInfoScrn[0xB148 - 0xAF58]; // _AF58
	WiiHomeMenu mHomeMenu;         // _B148
	u8 _B7A8[0xF70C - 0xB7A8];
	WiiPlayerState* mPlayer; // _F70C
	u8 _F710[0xF770 - 0xF710];
	WiiFlagSource* mCoins; // _F770
	u8 _F774[0xF784 - 0xF774];
	WiiFlagSource* mItems; // _F784
	u8 _F788[0xF7AC - 0xF788];
	// the spawn pool CMsgFont_Draw scans for the portrait object (kind 10); sits just before mObjects
	u8* mObjPoolObjects; // _F7AC
	s32 mObjPoolCount;   // _F7B0
	u8 _F7B4[0xF7BC - 0xF7B4];
	s32 mObjPoolStride; // _F7BC
	u8 _F7C0[0xF874 - 0xF7C0];
	WiiFlagSource* mDust; // _F874
	u8 _F878[0xF8B0 - 0xF878];
	WiiTitleObject* mObjects[0x400]; // _F8B0, the object table ResolveEventObjectId indexes
	u8 _108B0[0x108C8 - 0x108B0];
	// pointers to spawned objects the renderer walks; entries are grouped in eights,
	// CMsgFont_Init pushes its two cursors here
	WiiObjectRef* mObjectRefs; // _108C8
	s32 mObjectRefCount;       // _108CC
	u8 _108D0[0x4C8E4 - 0x108D0];
	WiiEvtData mEvtData; // _4C8E4
	u8 _4C8EC[0x4C8F0 - 0x4C8EC];
	s32 mSelectResult; // _4C8F0, the yes/no answer CMsg__cmd_QUESTION stores, in mEvt on the GameCube
	u8 _4C8F4[0xD7610 - 0x4C8F4];
	WiiTimeState mTimeState;                   // _D7610
	CMotionControlCamera mMotionControlCamera; // _D7620
	WiiCameraActions mCameraActions;           // _D9B24
	u8 _D9B54[0xEC1A0 - 0xD9B54];
	f32 mPlayTime; // _EC1A0
	u8 _EC1A4[0xEE1DC - 0xEC1A4];
	u32 mMenuControls;
	u8 _EE1E0[0xEE530 - 0xEE1E0];

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
	bool CanOpenHomeMenu() const { return IsMenuFree() && !mHomeMenu.IsOpen(); }
};
STATIC_ASSERT(sizeof(CGame) == 0xEE530);

extern "C" CGame gGame;
#endif
