#ifndef GAME_WII_XOBJ_H
#define GAME_WII_XOBJ_H

#include "types.h"

#include "Dolphin/gx.h"
#include "Dolphin/mtx.h"

#include "game/cfile.h"
#include "game/coordinfo.h"
#include "game/cstatus64.h"
#include "game/ctrigon.h"
#include "game/evt.h"
#include "game/ut.h"
#include "game/wii/cgs.h"

// the Wii XObj family, as far as xobj_ut.cpp's pool setup verifies it: names and
// sizes come from the retail RTTI strings and the element strides the pool
// constructors pass to __construct_new_array.
//
// XObj's vtable (retail __vt__4XObj at 0x80302438) has two parts, so XObj's
// chain carries two polymorphic bases: RTTI names them nw4r::g3d::
// ICalcWorldCallback at object _00 (4 slots) and nw4r::g3d::IScnObjCallback at
// object _04 (75 slots in __vt__4CObj at 0x80355EF8). MWCC appends the
// derived class's own virtuals to the end of the last vtable part, so XObj's
// own virtuals start at slot 75 - InitializeObject lands on +_14C (verified
// against InitializeObjectPools' virtual calls).
namespace nw4r {
namespace g3d {

class ICalcWorldCallback {
public:
	virtual ~ICalcWorldCallback() { } // _08
	// _0C.._14 not yet recovered
	virtual void _unk_00C();
	virtual void _unk_010();
	virtual void _unk_014();
};

class IScnObjCallback {
public:
	virtual ~IScnObjCallback() { } // _08
	// _0C.._148 not yet recovered
	virtual void _unk_00C();
	virtual void _unk_010();
	virtual void _unk_014();
	virtual void _unk_018();
	virtual void _unk_01C();
	virtual void _unk_020();
	virtual void _unk_024();
	virtual void _unk_028();
	virtual void _unk_02C();
	virtual void _unk_030();
	virtual void _unk_034();
	virtual void _unk_038();
	virtual void _unk_03C();
	virtual void _unk_040();
	virtual void _unk_044();
	virtual void _unk_048();
	virtual void _unk_04C();
	virtual void _unk_050();
	virtual void _unk_054();
	virtual void _unk_058();
	virtual void _unk_05C();
	virtual void _unk_060();
	virtual void _unk_064();
	virtual void _unk_068();
	virtual void _unk_06C();
	virtual void _unk_070();
	virtual void _unk_074();
	virtual void _unk_078();
	virtual void _unk_07C();
	virtual void _unk_080();
	virtual void _unk_084();
	virtual void _unk_088();
	virtual void _unk_08C();
	virtual void _unk_090();
	virtual void _unk_094();
	virtual void _unk_098();
	virtual void _unk_09C();
	virtual void _unk_0A0();
	virtual void _unk_0A4();
	virtual void _unk_0A8();
	virtual void _unk_0AC();
	virtual void _unk_0B0();
	virtual void _unk_0B4();
	virtual void _unk_0B8();
	virtual void _unk_0BC();
	virtual void _unk_0C0();
	virtual void _unk_0C4();
	virtual void _unk_0C8();
	virtual void _unk_0CC();
	virtual void _unk_0D0();
	virtual void _unk_0D4();
	virtual void _unk_0D8();
	virtual void _unk_0DC();
	virtual void _unk_0E0();
	virtual void _unk_0E4();
	virtual void _unk_0E8();
	virtual void _unk_0EC();
	virtual void _unk_0F0();
	virtual void _unk_0F4();
	virtual void _unk_0F8();
	virtual void _unk_0FC();
	virtual void _unk_100();
	virtual void _unk_104();
	virtual void _unk_108();
	virtual void _unk_10C();
	virtual void _unk_110();
	virtual void _unk_114();
	virtual void _unk_118();
	virtual void _unk_11C();
	virtual void _unk_120();
	virtual void _unk_124();
	virtual void _unk_128();
	virtual void _unk_12C();
	virtual void _unk_130();
};

} // namespace g3d
} // namespace nw4r

// the Wii's CVec sibling: same {x,y,z,vtable} shape with its own vtable, base of
// the Wii CObjVec (retail __ct__7CObjVecFv at 0x800BE7C0 stores the TVec vtable
// for its first member and CVec vtables for the two behind it)
class TVec : public CVec {
public:
	TVec() { }
	TVec(f32 x, f32 y, f32 z)
	    : CVec(x, y, z)
	{
	}
	virtual ~TVec() { } // _08
};

// same shape and role as the GameCube CObjVec: a current vec with the target it
// walks to and the step's speed, only the current vec is a TVec here
class CObjVec : public TVec {
public:
	CObjVec();

	CVec mTarget; // _10
	CVec mSpeed;  // _20

	virtual ~CObjVec() { }
};
STATIC_ASSERT(sizeof(CObjVec) == 0x30);

// small polymorphic members the aggregate constructors only tag with a vtable;
// their names come from the retail RTTI strings, their members are not yet
// recovered
class CResource {
public:
	virtual ~CResource() { }
};

// NW_Obj is the polymorphic base of NW_Anm (RTTI). both come out four bytes:
// CJArchive keeps the anm at _58 with CFile right behind it at _5C
class NW_Obj {
public:
	virtual ~NW_Obj() { }
};

class NW_Anm : public NW_Obj {
public:
	virtual ~NW_Anm() { }
};

// anims waiting to play after the current one. unlike the GameCube layout the
// vtable leads the object on the Wii (retail stores __vt__9CAnmQueue at _7B8
// inside XObj); the queue members behind it are not yet recovered
class CAnmQueue {
public:
	virtual ~CAnmQueue() { }

private:
	u8 _04[0x64];
};
STATIC_ASSERT(sizeof(CAnmQueue) == 0x68);

// the Wii object archive, replacing the GameCube CJArchive/CJAnim pair: the
// retail ctor (0x800BE7E0) clears two leading words, tags an NW_Anm and
// constructs the CFile the archive reads go through. XObj keeps one and
// XObjChr a second one
class CJArchive {
public:
	CJArchive();

	u32 _00;
	u32 _04;
	u8 _08[0x50];
	NW_Anm mAnm; // _58
	CFile mFile; // _5C
};
STATIC_ASSERT(sizeof(CJArchive) == 0x148);

// element of XObjMap's two 100-entry record lists: 0xC bytes with a ctor and
// dtor in the CPtcl unit (retail fn_800BEB4C/dtor_800BEB60 and
// fn_800BEBDC/fn_800BEBF0). their real names and members are not recovered yet
class XObjMapListA {
public:
	XObjMapListA();
	~XObjMapListA();

private:
	u8 _00[0xC];
};
STATIC_ASSERT(sizeof(XObjMapListA) == 0xC);

class XObjMapListB {
public:
	XObjMapListB();
	~XObjMapListB();

private:
	u8 _00[0xC];
};
STATIC_ASSERT(sizeof(XObjMapListB) == 0xC);

// the shared base of the game's scene objects: the two nw4r callback
// interfaces plus CObj's own members. the member list below is decoded from
// the stores in the retail aggregate constructors; the nine vec members come
// in three {TVec,CVec,CVec} groups matching the GameCube CObjVec shape, so
// they are named after the three move steppers the GameCube CXObj keeps in
// CObjVecs
class CObj : public nw4r::g3d::ICalcWorldCallback, public nw4r::g3d::IScnObjCallback {
public:
	// the retail aggregate constructors show all nine vecs zeroed while the
	// vec members' own ctors stay empty, so the zeros come from here
	CObj()
	{
		_E8 = 0;
		_EC = 0;
		mMoveTranslate.Set(0.0f, 0.0f, 0.0f);
		mMoveTarget0.Set(0.0f, 0.0f, 0.0f);
		mMoveSpeed0.Set(0.0f, 0.0f, 0.0f);
		mMoveRotate.Set(0.0f, 0.0f, 0.0f);
		mMoveTarget1.Set(0.0f, 0.0f, 0.0f);
		mMoveSpeed1.Set(0.0f, 0.0f, 0.0f);
		mMoveScale.Set(0.0f, 0.0f, 0.0f);
		mMoveTarget2.Set(0.0f, 0.0f, 0.0f);
		mMoveSpeed2.Set(0.0f, 0.0f, 0.0f);
	}
	virtual ~CObj() { }

private:
	u8 _08[0x24];
	CResource mResource; // _2C
	TVec mMoveTranslate; // _30
	CVec mMoveTarget0;   // _40
	CVec mMoveSpeed0;    // _50
	TVec mMoveRotate;    // _60
	CVec mMoveTarget1;   // _70
	CVec mMoveSpeed1;    // _80
	TVec mMoveScale;     // _90
	CVec mMoveTarget2;   // _A0
	CVec mMoveSpeed2;    // _B0
	u8 _C0[0x14];
	CColor mColor0; // _D4
	CColor mColor1; // _DC
	u8 _E4[4];
	u32 _E8;
	u32 _EC;
	u8 _F0[0x50];
	NW_Obj mObject; // _140
	u8 _144[0xC];
};
STATIC_ASSERT(sizeof(CObj) == 0x150);

class XObj : public CObj {
public:
	XObj();
	// the trivial body: retail's aggregate destructors inline it to nothing, the
	// standalone __dt__4XObjFv at 0x8002D57C is the deleting variant
	virtual ~XObj() { }
	virtual void InitializeObject(); // _14C, first of XObj's own virtuals; retail's out-of-line
	                                 // definition keeps the GameCube u32 parameter, but callers
	                                 // here emit no argument (the callee never reads it)

private:
	CStatus64 mStatus; // _150
	CoordInfo mCoord;  // _160
	AttrInfo mAttr;    // _168
	u8 _174[0x28];
	CVec mLastPos; // _19C
	u8 _1AC[0x54C];
	CVec mToppleOffset; // _6F8
	u8 _708[0xC];
	CVec mVec714; // _714
	CEvt mEvt;    // _724
	u8 _790[0x28];
	CAnmQueue mAnmQueue; // _7B8
	CJArchive mArchive;  // _820
	u8 _968[0x40];
};
STATIC_ASSERT(sizeof(XObj) == 0x9A8);

// subclass pool types: ctor/dtor are defined in their own units, the tail pad is
// the part of the stride the class adds on top of XObj
#define WII_XOBJ_DECL(name, size)      \
	class name : public XObj {         \
	public:                            \
		name();                        \
		virtual ~name();               \
                                       \
	private:                           \
		u8 _tail[size - sizeof(XObj)]; \
	};                                 \
	STATIC_ASSERT(sizeof(name) == size)

WII_XOBJ_DECL(XObjScnCam, 0x9C0);

// the light object: its dtor (0x800BE828) destroys CLight[8] at _9AC, the
// ctor (0x800BE364) is the XObj initialization plus that array
class XObjLight : public XObj {
public:
	XObjLight();
	virtual ~XObjLight();

private:
	u8 _9A8[4];
	CLight mLights[8]; // _9AC
	u8 _D0C[0x24];
};
STATIC_ASSERT(sizeof(XObjLight) == 0xD30);

// the map object: a word and a CTrigon behind the XObj members, then two
// 100-entry record lists the CPtcl unit's element types fill
class XObjMap : public XObj {
public:
	XObjMap();
	virtual ~XObjMap();

private:
	u8 _9A8[4];
	u32 _9AC;
	u8 _9B0[0x18];
	CTrigon mTrigon; // _9C8
	u8 _9EC[0x28];
	XObjMapListA mListA[0x64]; // _A14
	XObjMapListB mListB[0x64]; // _EC4
	u8 _1374[0x84];
};
STATIC_ASSERT(sizeof(XObjMap) == 0x13F8);
WII_XOBJ_DECL(XObjChr, 0x1CCA0);
WII_XOBJ_DECL(XObjActor3, 0x21C8);
WII_XOBJ_DECL(XObjSound, 0xA10);
WII_XOBJ_DECL(XObjCoin, 0xA30);
WII_XOBJ_DECL(XObjItem, 0x9B0);
WII_XOBJ_DECL(XObjLeticker, 0xAC8);
WII_XOBJ_DECL(XObjActorToy, 0x2208);
WII_XOBJ_DECL(XObjActorJenny, 0xD98);
WII_XOBJ_DECL(XObjActorMama, 0xD88);
WII_XOBJ_DECL(XObjActorPapa, 0xD90);
WII_XOBJ_DECL(XObjActorTao, 0xDF8);
WII_XOBJ_DECL(XObjActorDeca, 0xDA0);
WII_XOBJ_DECL(XObjArmy, 0xB30);
WII_XOBJ_DECL(XObjSpider, 0xDF8);
WII_XOBJ_DECL(XObjSpiderSmall, 0xB50);
WII_XOBJ_DECL(XObjSpiderBoss, 0xCE0);
WII_XOBJ_DECL(XObjDust, 0xA00);
WII_XOBJ_DECL(XObjHBox, 0x1368);
WII_XOBJ_DECL(XObjFree, 0x9B8);

// one of XOBJS's object pools, same layout as the GameCube one
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

	XObj* Get(s32 i) const { return (XObj*)(mObjects + i * mStride); }
};
STATIC_ASSERT(sizeof(XObjPool) == 0x14);

// the objects: a pool per kind and the table of the ones in use, same layout as
// the GameCube XOBJS
class XOBJS {
public:
	XObjPool mPools[0x17]; // _000
	XObj* mObjects[0x400]; // _1CC, the object table
	s32 mObjectCount;      // _11CC
	s32 mLoadObjectIndex;  // _11D0
	s32 mLoadState;        // _11D4, UpdateStageObjectLoading's state, 0 when idle
	// _11D8 vtable

	virtual ~XOBJS() { }
};
STATIC_ASSERT(sizeof(XOBJS) == 0x11DC);

// a small shadow helper the unit defines; only its trivial dtor is verified so
// far (retail 0x800BD66C, the deleting variant with no member calls)
class CustomShadow {
public:
	virtual ~CustomShadow();
};

// the Wii object readers: a thread-like prefix of 0x320 bytes (an OSThread plus
// the stack fields, like the GameCube CThread), then the vtable at _320. The
// queue and request members behind it are not yet recovered; the Wii knows only
// the all-list and single readers, no XObjReadAnm.
class XObjRead {
	u8 _000[0x320];
	// _320 vtable

public:
	virtual ~XObjRead() { }                                                                        // _08
	virtual BOOL Create(void* (*func)(void*), void* param, u32 stackSize, s32 priority, u16 attr); // _0C

private:
	u8 _324[0x3A0 - 0x324]; // vtable pad + CMsgQueue + requests, not yet recovered
};
STATIC_ASSERT(sizeof(XObjRead) == 0x3A0);

class XObjReadAll : public XObjRead { };
class XObjReadOne : public XObjRead { };

extern "C" {
void InitializeObjectPools(XOBJS* list);
void* ReadObjectListThread(void* arg);
void* ReadSingleObjectThread(void* arg);
extern XObjReadAll gAllObjectsReader;
extern XObjReadOne gSingleObjectReader;
}

#endif
