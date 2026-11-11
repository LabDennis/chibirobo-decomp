#ifndef _GAME_CTRIGON_H
#define _GAME_CTRIGON_H

// the HSD headers have to come first, see game/hsd_jobj.h
#include "game/hsd_jobj.h"

// map floor collision. CTrigon takes one child joint of the stage model, keeps every
// upward facing triangle in its display lists as a TRIGON_INFO and sorts them by
// distance from the origin. class names from RTTI, file name from the wii port

struct TRIGON_INFO;

// fills out[3] with world space corners. needs the const or the readers can't schedule
// their loads ahead of the stores to out
typedef void (*TrigonGetVtxFunc)(const TRIGON_INFO* tri, Vec* out);

struct TRIGON_INFO {
	void* mVtx[3];            // _00, into the pobj's POS array
	TrigonGetVtxFunc mGetVtx; // _0C
	f32 mDist;                // _10, centroid distance from the origin, the sort key
	MtxPtr mMtx;              // _14, the joint's matrix, NULL when it's identity
	u16 mFrac;                // _18
	u16 _1A;                  // _1A
	u32 mColor;               // _1C, material diffuse without alpha, 0 means skip
};
typedef char TRIGON_INFO_size_check[sizeof(TRIGON_INFO) == 0x20 ? 1 : -1];

// throwaway triangle used while parsing, counts what it lets through
class _trigonChecker : public TRIGON_INFO {
public:
	_trigonChecker()
	    : mNumOk(0)
	    , mNumNg(0)
	{
	}
	virtual ~_trigonChecker() { } // _08

	bool Check();

	// _00 TRIGON_INFO
	// _20 vtable
	s32 mNumOk; // _24
	s32 mNumNg; // _28
};
typedef char _trigonChecker_size_check[sizeof(_trigonChecker) == 0x2C ? 1 : -1];

class CTrigon {
public:
	TRIGON_INFO* mTri; // _00
	u32 mNum;          // _04
	u32 mNumMax;       // _08
	f32 mMinY;         // _0C
	f32 mMaxY;         // _10
	HSD_JObj* mJObj;   // _14
	// _18 vtable

	CTrigon()
	    : mTri(NULL)
	{
	}
	virtual ~CTrigon() { } // _08

	TRIGON_INFO* Create(HSD_JObj* jobj);
	void Release();
	int Check(Vec* pos, f32* floorY, Vec* tri);
	int Search(Vec* pos, f32* floorY);
};
typedef char CTrigon_size_check[sizeof(CTrigon) == 0x1C ? 1 : -1];

#endif
