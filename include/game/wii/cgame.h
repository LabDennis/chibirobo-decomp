#ifndef GAME_WII_CGAME_H
#define GAME_WII_CGAME_H

#include "game/wii/cpad.h"
#include "game/wii/csound.h"
#include "game/cscrn.h"

class CGame;

// Only this slot of the Wii CBase vtable is used here. It is shared with CGame.
struct WiiGameVTable {
	void* _00[0x40 / sizeof(void*)];
	u64 (*getFlags)(const CGame*, u64 mask);
};

// Verified Wii fields accessed by the controller, stage, and save code.
class CGame {
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
	const WiiGameVTable* mVTable;
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
