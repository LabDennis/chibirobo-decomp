#ifndef REL_ANA_H
#define REL_ANA_H

#include "game/hsd_jobj.h"
#include "game/ut.h"

class Ana {
public:
	Ana() { }
	~Ana() { }
	virtual bool IsActive();

	void SetJObj(s32 kind, s32 which)
	{
		mJObjKind  = kind;
		mJObjWhich = which;
	}

	Vec mPos;        // _04
	f32 mRotY;       // _10
	CStatus mStatus; // _14
	s32 mId;         // _1C
	s32 mJObjKind;   // _20
	s32 mJObjWhich;  // _24
};

inline bool Ana::IsActive()
{
	return mStatus.mFlags & 1;
}

class AnaNull : public Ana {
public:
	virtual bool IsActive() { return false; }
};

#define ANA_NUM 20

#endif
