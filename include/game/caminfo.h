#ifndef _GAME_CAMINFO_H
#define _GAME_CAMINFO_H

#include "types.h"
#include <dolphin/mtx/mtxtypes.h>

// split from ccameraaction.h since the getters' Vec literals land in the .rodata of every
// file that includes them

// a stage's camera placement, positions and fov in hundredths
struct CamInfoData {
	s32 mGroupId;     // _00, camera area group the entry belongs to, a group's entries are consecutive; -1 ends the table
	s32 mEye[3];      // _04
	s32 mInterest[3]; // _10
	s32 mFov;         // _1C
};

// wraps a CamInfoData, name from RTTI
class CamInfo {
public:
	virtual ~CamInfo() { } // _08, its one copy is in ccameraactionarea.cpp

	CamInfoData* mData; // _04

	CamInfo(CamInfoData* data) { mData = data; }
	Vec GetEye()
	{
		Vec eye = { mData->mEye[0] / 100.0f, mData->mEye[1] / 100.0f, mData->mEye[2] / 100.0f };
		return eye;
	}
	Vec GetInterest()
	{
		Vec interest = { mData->mInterest[0] / 100.0f, mData->mInterest[1] / 100.0f, mData->mInterest[2] / 100.0f };
		return interest;
	}
	f32 GetFov() { return mData->mFov / 100.0f; }
};

#endif
