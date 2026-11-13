#ifndef _GAME_STAGE_H
#define _GAME_STAGE_H

#include "game/cmodule.h"
#include "game/coordinfo.h"
#include "Version.h"
#include "game/stagedata.h"

struct StageDesc {
	const char* fileName;
#if defined(VERSION_PAL)
	const char* moduleName;
#endif
	s32 message;
	u32 flags;
};
extern "C" StageDesc stageTable[];

// the stage file's environment block, class name from RTTI
class EnvInfo {
public:
	u8* mData; // _00
	// _04 vtable

	EnvInfo(u8* data)
	    : mData(data)
	{
	}
	virtual ~EnvInfo() { } // _08
};
STATIC_ASSERT(sizeof(EnvInfo) == 8);

typedef s32 (*StageFunction)(s32 function, s32 count, s32* args);

// the loaded stage, its file and rel, class name from RTTI
class Stage {
public:
	Stage()
	    : mEnvironment(NULL)
	    , mPlacements(NULL)
	{
		mState = 0;
	}

	char mDataPath[0x40];       // _00
	char mModulePath[0x40];     // _40
	s32 mState;                 // _80
	CFile mDataFile;            // _84
	EnvInfo mEnvironment;       // _170
	CoordInfo mPlacements;      // _178
	u8* mScriptOffsets;         // _180
	u8* mSetupScript;           // _184
	u8* mEntryScript;           // _188
	u8* mBatteryDepletedScript; // _18C
	u8* mZButtonScript;         // _190
	u8* mTimerExpiredScript;    // _194
	void* _198; // _198, the stage file's resource at header offset 0x20, NULL when absent; written in the stage loader, never read
	const s32* mEventObjectPairs; // _19C
	void* mCameraAreaRecords;     // _1A0
	void* mCameraInfoRecords;     // _1A4
	void* mCameraAreaExRecords;   // _1A8
	CModule mModule;              // _1AC
	// _2F0 vtable

	virtual ~Stage() { } // _08

	// calls into the stage's rel, -1 without one
	s32 CallModule(s32 function, s32 count, s32* args)
	{
		if (mModule.IsLoaded())
			return ((StageFunction)mModule.mModule->prolog)(function, count, args);
		return -1;
	}
};
STATIC_ASSERT(sizeof(Stage) == 0x2F4);
extern "C" Stage gStageResources;

static inline bool IsModuleLoaded(CModule* module)
{
	return module->IsLoaded();
}

#endif
