#ifndef GAME_WII_CGAME_H
#define GAME_WII_CGAME_H

#include "game/wii/cpad.h"
#include "game/wii/csound.h"
#include "game/cscrn.h"

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

class CGame : public CBase {
public:
	s32 mLoadStep;
	u8 _41EC[0x42FC - 0x41EC];
	s32 mStage;
	u8 _4300[0x4B40 - 0x4300];
	CTpl mTpl;
	u8 _4D38[0x5078 - 0x4D38];
	CScrnRoboeyes mRoboeyes;
	u8 _511C[0xEE1DC - 0x511C];
	u32 mMenuControls;
	u8 _EE1E0[0xEE530 - 0xEE1E0];
};
STATIC_ASSERT(sizeof(CGame) == 0xEE530);

extern "C" CGame gGame;
#endif
