#ifndef _GAME_CGAME_H
#define _GAME_CGAME_H

#include "types.h"
#include "Version.h"
#include "game/cgs.h"
#include "game/cfont.h"
#include "game/ccamera.h"
#include "game/clight.h"
#include "game/cstatus64.h"
#include "game/cstopwatch.h"
#include "game/cfile.h"
#include "game/ctpl.h"
#include "game/ctoon.h"
#include "game/cscrn.h"
#include "game/fbeffect.h"
#include "game/fbtexeff.h"
#include "game/logo.h"
#include "game/title.h"
#include "game/menuex.h"
#include "game/infoscrn.h"
#include "game/cparticle.h"
#include "game/xobj.h"
#include "game/xobjflags.h"
#include "game/cobj.h"
#include "game/evt.h"
#include "game/msg.h"
#include "game/cmotioncontrolcamera.h"
#include "game/xsound.h"
#include "game/memcard.h"
#include "game/cmovie.h"
extern "C" {
#include <sysdolphin/controller.h>
}
#include "game/cpad.h"

class XObjChr;

// the time of day evt's TIME command drives, mode 1 is night
struct EvtTimeState {
	s32 mMode;     // _00
	f32 mTime;     // _04
	f32 mDuration; // _08
	s32 mRunning;  // _0C
};

// one of XOBJS's object pools, all its objects the same size in one block
struct XObjPool {
	u8* mObjects;      // _00
	s32 mUsed;         // _04, objects handed out so far
	s32 mCount;        // _08
	const char* mName; // _0C
	s32 mStride;       // _10, size of one object

	void Set(void* objects, s32 count, const char* name, s32 stride)
	{
		mObjects = (u8*)objects;
		mUsed    = 0;
		mCount   = count;
		mName    = name;
		mStride  = stride;
	}

	XObj* First() const { return (XObj*)mObjects; }
	XObj* Get(s32 i) const { return (XObj*)(mObjects + i * mStride); }

	// the first object whose flags match, -1 when there's none
	s32 Find(u64 mask, u64 value) const
	{
		for (s32 i = 0; i < mCount; i++) {
			if ((Get(i)->mStatus.mFlags & mask) == value) {
				return i;
			}
		}
		return -1;
	}
};
STATIC_ASSERT(sizeof(XObjPool) == 0x14);

// the objects: a pool per kind and the table of the ones in use, name from RTTI
class XOBJS {
public:
	enum CollectionPool {
		COINS = 7,
		ITEMS = 8,
		DUST  = 20,
	};

	XObjPool mPools[0x17]; // _000
	XObj* mObjects[0x400]; // _1CC, the object table
	s32 mObjectCount;      // _11CC
	s32 mLoadObjectIndex;  // _11D0
	s32 mLoadState;        // _11D4, UpdateStageObjectLoading's state, 0 when idle
	// _11D8 vtable

	virtual ~XOBJS() { } // _08

	CEvtFlags& GetCollectionFlags(CollectionPool pool) const { return *static_cast<XObjFlags*>(mPools[pool].First())->mFlags; }

	void ReadAll();
};
STATIC_ASSERT(sizeof(XOBJS) == 0x11DC);

// objects in draw order, CGame keeps three
struct XObjDrawList {
	XObj* mObjects[0x400]; // _0000
	s16 mIndices[0x400];   // _1000, each one's slot in XOBJS's mObjects
	s32 mCount;            // _1800

	void ClearObjects()
	{
		for (int i = 0; i < 1024; ++i)
			mObjects[i] = NULL;
		mCount = 0;
	}
};
STATIC_ASSERT(sizeof(XObjDrawList) == 0x1804);

// a screen color with a texture to draw it through
struct ScrnTex {
	f32 mAngle;        // _00
	CScrnColor mColor; // _04
	CTexObj mTex;      // _38
};
STATIC_ASSERT(sizeof(ScrnTex) == 0x138);

// the object archives loaded so far
struct ObjArchiveTable {
	CObjArchive mArchives[0x400]; // _00
};

// the movie, the card and two of the screen colors sit in structs of their own, CGame's dtor
// checks each twice
struct GameMovie {
	CMovie mMovie; // _00
};

struct GameMemcard {
	Memcard mCard; // _00
};

struct GameScrnColor {
	CScrnColor mColor; // _00
};

// the system half of the game object: the graphics state, font, pads, two cameras, six
// lights and the frame timers. class name from RTTI, layout from __ct__5CBaseFv. japan is
// 0x18 shorter from mFont on, europe is 4 longer from mFont and 8 from mStatus
class CBase {
public:
	CGs mGs;                  // _0000
	CFont mFont;              // _17C0, _17A8 in japan
	CPadEx mPad;              // _1900
	u32 mLastRetrace;         // _1B08, VIGetRetraceCount at the start of the frame
	u32 mRetraceStep;         // _1B0C, retraces since the last frame, at most 4
	CStatus64 mStatus;        // _1B10, _1AF8 in japan, _1B18 in europe
	CCamera mCameras[2];      // _1B20
	CLight mLights[6];        // _1E48
	CStopwatch mStopwatch[3]; // _3258, the whole frame, the update and the draw
#if !defined(VERSION_JP)
	u8 mLanguage; // _3318, 6 is japanese
#endif
	s32 mDvdStatus;     // _331C, _3300 in japan, DVDGetDriveStatus
	s32 mPrevDvdStatus; // _3320
	BOOL mDiskOk;       // _3324, DVDCheckDisk
	BOOL mDvdError;     // _3328, rumble is paused while set
	s32 mResetHold;     // _332C, frames the reset combo is held
	BOOL mReset;        // _3330, stops XOBJS::ReadAll when set
	// _3334 vtable

	CBase();
	virtual ~CBase();                               // _08
	virtual void Initialize(int argc, char** argv); // _0C
	virtual void Start();                           // _10
	virtual BOOL BeforeUpdate();                    // _14
	virtual void Update(u32 frames);                // _18
	virtual void Draw();                            // _1C
	virtual void vf20();                            // _20
	virtual void vf24();                            // _24
	virtual s32 FrameInterval();                    // _28, retraces per frame
	virtual u32 vf2C();                             // _2C
	virtual CCamera* vf30();                        // _30
	virtual CCamera* vf34();                        // _34
	virtual CCamera* GetCamera();                   // _38, vf30's unless overridden
	virtual CLight* vf3C();                         // _3C
	virtual CLight* vf40();                         // _40
	virtual CLight* GetLight();                     // _44, vf3C's unless overridden
	virtual void Reset(u32 code);                   // _48

	// the disc can't be read
	bool IsDiscError() const
	{
		bool error = false;
		if ((mDvdStatus != 0 || mPrevDvdStatus != 0) && !mDiskOk) {
			error = true;
		}
		return error;
	}

	// a stage is up and running, nothing stops it
	bool IsRunning() const
	{
		bool running = false;
		if ((mStatus.mFlags & 1) && !(mStatus.mFlags & 0x22)) {
			if (!IsDiscError()) {
				running = true;
			}
		}
		return running;
	}
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(CBase) == 0x3320);
#elif defined(VERSION_PAL)
STATIC_ASSERT(sizeof(CBase) == 0x3340);
#else
STATIC_ASSERT(sizeof(CBase) == 0x3338);
#endif

// the game: the title, the menus, the objects, the event scripts and the camera on top of
// CBase, its one instance is gGame. name from RTTI. japan is 0x20 shorter from mTpl,
// 0x48 from mInfoScrn, 0xCE8 after the messages and 0xE00 from mMemcard. europe matches usa
// from mTpl
class CGame : public CBase {
public:
	s32 _3338;             // _3338, _3320 in japan, _3340 in europe
	CFileArchive mArchive; // _333C
	s32 mStage;            // _3444, _342C in japan, _344C in europe
	s32 mPrevStage;        // _3448
	s32 _344C;             // _344C, room count
#if defined(VERSION_JP) || defined(VERSION_PAL)
	u8 _3450[8];
#else
	u8 _3450[16];
#endif
	ToonTexture mToonTexture; // _3460
	u8 _3C78[8];
	CTpl mTpl;                                 // _3C80, _3C60 in japan, the shared textures
	CScrnFade mFade;                           // _3E70
	CScrnWipeCircle mWipeCircle;               // _3EA4
	CScrnWipeTex mWipeTex;                     // _3F08
	CScrnWipeCustom mWipeCustom;               // _406C
	CScrnWipeTile mWipeTile0;                  // _40D0
	CScrnWipeTile mWipeTile1;                  // _4134
	CScrnRoboeyes mRoboeyes;                   // _4198
	CFbOutline mOutline;                       // _4204
	CFbBlur mBlur;                             // _4268
	FbSubwin mSubwin;                          // _42AC
	ScrnTex mScrnTex;                          // _4584
	GameScrnColor mColor0;                     // _46BC
	GameScrnColor mColor1;                     // _46F0
	s32 _4724;                                 // _4724
	f32 _4728;                                 // _4728, the outline's width, the c stick moves it
	Logo mLogo;                                // _472C
	Title mTitle;                              // _4930
	MenuEx mMenu;                              // _8580, _8538 in japan
	InfoScrn mInfoScrn;                        // _8F38, _8EF0 in japan
	CParticle mParticle;                       // _9114, _90CC in japan
	CScrnQuake mQuake;                         // _965C
	XOBJS mXObjs;                              // _9694
	XObjDrawList mDrawLists[3];                // _A870, _A828 in japan, the last one is what mirrors show
	ObjArchiveTable mObjArchives;              // _F07C
	CEvt mEvt;                                 // _5D07C
	CMsg mCMsgs[2];                            // _5D0E8
	EvtTimeState mTimeState;                   // _E5D28, _E5040 in japan
	CMotionControlCamera mMotionControlCamera; // _E5D38, _E5050 in japan
	CCameraActions mCameraActions;             // _E5FAC, _E52C4 in japan, the actions the camera picks from
	CLight* mLightList;                        // _E7E20, _E7138 in japan, LIGHT walks it
	XSound mSound;                             // _E7E24, _E713C in japan
#if defined(VERSION_JP)
	u8 _E8208[0xE8400 - 0xE8208 - 0x118]; // _E8208
#else
	u8 _E8208[0xE8400 - 0xE8208]; // _E8208
#endif
	GameMemcard mMemcard;         // _E8400, _E7600 in japan
	u8 _FEE24[0xFF000 - 0xFEE24]; // _FEE24
	BOOL mMemcardLoaded;          // _FF000, the stage came from the card
	u8 _FF004[0xFF200 - 0xFF004]; // _FF004
#if defined(VERSION_PAL)
	f32 mPlayTime; // _FF200, frames played, it stops just short of 100 hours
	f32 _FF204;
#else
	s32 mPlayTime; // _FF200, _FE400 in japan, frames played, it stops just short of 100 hours
	s32 _FF204;
#endif
	StageName mStageName; // _FF208
	GameMovie mMovie;     // _FF3A4
	u32 mMovieFlags;      // _FF45C, the Start flags
	f32 mMovieRect[4];    // _FF460, _FE660 in japan, where the movie draws

	CGame();
	virtual ~CGame();                               // _08
	virtual void Initialize(int argc, char** argv); // _0C
	virtual void Start();                           // _10
	virtual BOOL BeforeUpdate();                    // _14
	virtual void Update(u32 frames);                // _18
	virtual void Draw();                            // _1C
	virtual void vf20();                            // _20
	virtual void vf24();                            // _24
	virtual s32 FrameInterval();                    // _28
	virtual CCamera* GetCamera();                   // _38
	virtual CLight* GetLight();                     // _44
	virtual void Reset(u32 code);                   // _48
	void ResetNewGameState();

	XObj* GetObject(s32 index) const { return mXObjs.mObjects[index]; }
	CMsg* GetMsg(s32 index) { return &mCMsgs[index]; }
	// chibi robo, pool 2's one object
	XObjChr* GetPlayer() const { return (XObjChr*)mXObjs.mPools[2].mObjects; }

	void SetStage(s32 stage)
	{
		mPrevStage = mStage;
		mStage     = stage;
	}
	// the boot logo or the title screen is up
	u8 IsLogoActive() const { return (u8)(mLogo.mState >= 1 && mLogo.mState < 15); }
	u8 IsTitleActive() const { return (u8)(mTitle.mState >= 1 && mTitle.mState < 23); }
	u8 IsIntroActive() const
	{
		u8 active = 0;
		if (IsLogoActive() || IsTitleActive()) {
			active = 1;
		}
		return active;
	}
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(CGame) == 0xFE670);
#else
STATIC_ASSERT(sizeof(CGame) == 0xFF470);
#endif

extern "C" CGame gGame;

extern "C" {
void ChangeGameStage(CGame* game, s32 stage, BOOL showName);
s32 FindStageIndexByName(const char* name);
}

// the event's object with the given id
static inline XObj* FindObj(s16 id)
{
	return gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, id) : NULL;
}

// whether the stage up is the one with the name
static inline bool IsStage(const char* name)
{
	s32 stage = gGame.mStage;
	s32 id    = FindStageIndexByName(name);
	return id == stage;
}

#endif
