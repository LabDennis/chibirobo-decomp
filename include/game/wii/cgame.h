#ifndef GAME_WII_CGAME_H
#define GAME_WII_CGAME_H

#include "game/wii/cpad.h"
#include "game/wii/csound.h"
#include "game/cscrn.h"
#include "game/ut.h"
#include "game/inventory.h"

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
