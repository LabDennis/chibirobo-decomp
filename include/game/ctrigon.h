#ifndef _GAME_CTRIGON_H
#define _GAME_CTRIGON_H

#if defined(VERSION_R24J01)
#include "types.h"
#include "Dolphin/mtx.h"
#include "Dolphin/GX/GXTypes.h"

struct TRIGON_INFO;

// fills out[3] with world space corners
typedef void (*TrigonGetVtxFunc)(const TRIGON_INFO* tri, Vec* out);

// the Wii triangle: the GameCube one without the sort distance, plus the grid cell it
// sits in; the getters read the matrix at _10 and the fraction bits at _14
struct TRIGON_INFO {
	void* mVtx[3];            // _00, into the shape's position array
	TrigonGetVtxFunc mGetVtx; // _0C
	MtxP mMtx;                // _10, NULL when it's identity
	u16 mFrac;                // _14
	u16 mCell;                // _16, index into CTrigon::mCells
	u8 _18[8];                // _18, not recovered yet
};
STATIC_ASSERT(sizeof(TRIGON_INFO) == 0x20);

// a run of triangles in mTri that share a grid cell
struct TRIGON_CELL {
	TRIGON_INFO* mTri; // _00
	u32 mNum;          // _04
	u8 _08[8];         // _08, not recovered yet
};
STATIC_ASSERT(sizeof(TRIGON_CELL) == 0x10);

// Wii CTrigon: the GameCube pool plus a grid of cells over the floor. XObjMap keeps one
// at _9AC; its ctor clears mTri and stores the vtable at _9E8. The destructor
// (retail 0x800BEB0C) is the minimal body
class CTrigon {
public:
	TRIGON_INFO* mTri;   // _00
	u32 mNum;            // _04
	u32 mNumMax;         // _08
	u32 _0C;             // _0C, Create's counters, not recovered yet
	u32 _10;             // _10
	Vec mMin;            // _14
	Vec mMax;            // _20
	void* _2C;           // _2C, freed by Release, not recovered yet
	TRIGON_CELL* mCells; // _30
	u16 mNumCells;       // _34
	u16* mCellOrder;     // _38, the order Search walks the cells in
	// _3C vtable

	CTrigon()
	    : mTri(NULL)
	{
	}
	virtual ~CTrigon() { } // _08

	int GetVtxSize(GXVtxDescList* desc, GXVtxAttrFmtList* fmt, int* pos, int* nrm, int* tex, int* clr);
	void Release();
	int Check(Vec* pos, int hint, f32* floorY);
	int SearchNear(Vec* pos, int hint, f32* floorY);
	int Search(Vec* pos, f32* floorY);
};
STATIC_ASSERT(sizeof(CTrigon) == 0x40);
#else
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
	u16 _1A;                  // _1A, no access in any edition; the checker's struct copy (*out = checker) carries it along
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

#endif
