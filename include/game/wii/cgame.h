#ifndef GAME_WII_CGAME_H
#define GAME_WII_CGAME_H

#include "game/wii/cpad.h"
#include "game/wii/csound.h"
#include "game/cscrn.h"
#include "game/ut.h"

// Verified Wii fields accessed by the controller, stage, and save code. The Wii CBase is larger than the
// GameCube one, so its vtable sits at 0x41E4, right behind its data.
class CBase {
public:
	u8 _0000[0x2298];
	CPadEx mPad;
	XSound mSound;
	u32 mLastRetrace;
	u32 mRetraceStep;
	CStatus64 mStatus;
	u8 _3400[0x4160 - 0x3400];
	u8 mLanguage;
	s32 mDvdStatus;
	s32 mPrevDvdStatus;
	BOOL mDiskOk;
	u8 _4170[0x41E4 - 0x4170];
	// _41E4 vtable

	void Start();
	virtual ~CBase(); // _08
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
	virtual u64 GetStatusFlags(u64 mask) const; // _40, the game's status flags under mask
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

// the Wii camera object behind gGame's camera member. A different layout than the GameCube CCamera,
// but the setters the save manager calls sit on its vtable at the offsets below
class CCamera {
public:
	virtual ~CCamera() { }                         // _08
	virtual void vf0C();                           // _0C
	virtual void vf10();                           // _10
	virtual void vf14();                           // _14
	virtual void SetFov(f32 fov);                  // _18
	virtual void vf1C();                           // _1C
	virtual void vf20();                           // _20
	virtual void vf24();                           // _24
	virtual void vf28();                           // _28
	virtual void vf2C();                           // _2C
	virtual void vf30();                           // _30
	virtual void vf34();                           // _34
	virtual void vf38();                           // _38
	virtual void vf3C();                           // _3C
	virtual void vf40();                           // _40
	virtual void SetDistance(f32 distance);        // _44
	virtual void SetEye(const Vec& eye);           // _48
	virtual void SetInterest(const Vec& interest); // _4C
	virtual void SetAngle(const Vec& angle);       // _50

	u8 _04[0x4];
	f32 mFov; // _08
	u8 _0C[0x90 - 0x0C];
	CVec mEye;      // _90
	CVec mInterest; // _A0
	CVec mAngle;    // _B0
	u8 _C0[0xD0 - 0xC0];
	f32 mDistance; // _D0
};

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
	u8 _0D4[0x234 - 0xD4];
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

// the Wii player object: a different, larger class than the GameCube XObjChr. These are the members
// the save manager reads and writes, at offsets verified against retail. The position lives behind a
// virtual getter (vtable +0x244 returns the Vec*), the restore calls vtable +0x90 with the saved
// position and +0x70 with the saved rotation.
struct WiiPlayerState {
	u8 _0000[0x64];
	f32 mRotY; // _0064, mMoveRotate.y
	u8 _0068[0x999C - 0x68];
	s32 mExp;        // _999C, mParams.mExperience
	s32 mLevel;      // _99A0
	s32 mRank;       // _99A4
	f32 mBattery;    // _99A8
	f32 mMaxBattery; // _99AC
	s32 mMoney;      // _99B0
	s32 mScrap;      // _99B4
	s32 mActive;     // _99B8, mParams._1C[0]
	u8 _99BC[0x99D8 - 0x99BC];
	u8 mItems[0x400]; // _99D8, mParams.mInventory.mItems
	u8 _99D8[0xDC54 - 0x9DD8];
	s32 mSuitChangeStep; // _DC54, a queued suit change
	s32 mSuit;           // _DC58, mSuits.mCurrent
	s32 mRequestedSuit;  // _DC5C, takes the saved suit
	s32 mSpecialSuit;    // _DC60, mSuits.mSpecial
	u8 _DC64[0x19E9C - 0xDC64];
	u8 mAttachments[0x48]; // _19E9C
	u8 _19EE4[0x1C994 - 0x19EE4];
	s32 mSquirterContent; // _1C994
	s32 mItemTint;        // _1C998
};

// the coin, item and dust collections each hold a pointer to their 0x80 flag words at +0x9A8
struct WiiFlagSource {
	u8 _000[0x9A8];
	u32* mFlags;
};

class CGame : public CBase {
public:
	s32 mLoadStep;
	u8 _41EC[0x42FC - 0x41EC];
	s32 mStage;
	s32 mPrevStage; // _4300
	u8 _4304[0x4B40 - 0x4304];
	CTpl mTpl;
	u8 _4D38[0x5078 - 0x4D38];
	CScrnRoboeyes mRoboeyes;
	u8 _511C[0xF70C - 0x511C];
	WiiPlayerState* mPlayer; // _F70C
	u8 _F710[0xF770 - 0xF710];
	WiiFlagSource* mCoins; // _F770
	u8 _F774[0xF784 - 0xF774];
	WiiFlagSource* mItems; // _F784
	u8 _F788[0xF874 - 0xF788];
	WiiFlagSource* mDust; // _F874
	u8 _F878[0xD7610 - 0xF878];
	WiiTimeState mTimeState;                   // _D7610
	CMotionControlCamera mMotionControlCamera; // _D7620
	WiiCameraActions mCameraActions;           // _D9B24
	u8 _D9B54[0xEC1A0 - 0xD9B54];
	f32 mPlayTime; // _EC1A0
	u8 _EC1A4[0xEE1DC - 0xEC1A4];
	u32 mMenuControls;
	u8 _EE1E0[0xEE530 - 0xEE1E0];
};
STATIC_ASSERT(sizeof(CGame) == 0xEE530);

extern "C" CGame gGame;
#endif
