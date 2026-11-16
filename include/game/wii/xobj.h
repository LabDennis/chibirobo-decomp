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

struct XObjFloor;

// the Wii XObj family, as far as xobj_ut.cpp's pool setup and the ported
// object code verify it: names and sizes come from the retail RTTI strings,
// the element strides the pool constructors pass to __construct_new_array and
// the members the ported code reaches.
//
// XObj's vtable (retail __vt__4XObj at 0x80302438) has two parts, so the chain
// carries two polymorphic bases: RTTI names them nw4r::g3d::ICalcWorldCallback
// at object _00 (4 slots, the primary part) and nw4r::g3d::IScnObjCallback at
// object _04 (6 slots, offset-to-top -4). MWCC appends every class's own
// virtuals to the last vtable part, so the secondary part then carries CObj's
// 69 own virtuals at slots 0x38..0x148 and XObj's 80 own virtuals at
// 0x14C..0x288 (InitializeObject lands on +_14C, verified against
// InitializeObjectPools' virtual calls). Calls through a complete-object
// pointer load the +0 vptr and index the parts by absolute offset. Virtual
// names below come from retail symbols where they exist; vfNNN names the byte
// offset in the table where the slot is still anonymous.
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
	// _0C.._1C not yet recovered; retail keeps the two first slots thunks to
	// XObj implementations and the last three the interface's empty defaults
	virtual void _unk_00C();
	virtual void _unk_010();
	virtual void _unk_014();
	virtual void _unk_018();
	virtual void _unk_01C();
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

	u8 Step(u32 frames);

	// puts it at a value with nowhere to go
	void Reset(f32 x, f32 y, f32 z)
	{
		this->x   = x;
		this->y   = y;
		this->z   = z;
		mTarget.x = x;
		mTarget.y = y;
		mTarget.z = z;
		mSpeed.x  = 0.0f;
		mSpeed.y  = 0.0f;
		mSpeed.z  = 0.0f;
	}
	void Reset(const Vec& v) { Reset(v.x, v.y, v.z); }
	void SetTarget(f32 x, f32 y, f32 z)
	{
		mTarget.x = x;
		mTarget.y = y;
		mTarget.z = z;
	}
	void SetTarget(const Vec& v) { SetTarget(v.x, v.y, v.z); }
	// puts it at a value with its target there too, the speed stays
	void SetNow(const Vec& v)
	{
		Set(v);
		SetTarget(v);
	}
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
// fn_800BEBDC/dtor_800BEBF0). their real names and members are not recovered yet
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

	// CObj's own virtuals, secondary part slots 0x38..0x148
	virtual void vf038();                                // _38
	virtual void vf03C();                                // _3C
	virtual void Release();                              // _40, releases the carried object
	virtual void vf044();                                // _44
	virtual void vf048();                                // _48
	virtual void vf04C();                                // _4C
	virtual void vf050();                                // _50
	virtual void vf054();                                // _54
	virtual void vf058();                                // _58
	virtual void vf05C();                                // _5C
	virtual void vf060();                                // _60
	virtual void vf064();                                // _64
	virtual void vf068();                                // _68
	virtual void vf06C();                                // _6C
	virtual void vf070(f32 rot);                         // _70, sets the pose rotation
	virtual void vf074();                                // _74
	virtual void vf078();                                // _78
	virtual void vf07C();                                // _7C
	virtual void vf080();                                // _80
	virtual void vf084();                                // _84
	virtual void vf088();                                // _88
	virtual void vf08C();                                // _8C
	virtual void vf090(const Vec& pos);                  // _90, copies the owner's pose position
	virtual void vf094();                                // _94
	virtual void vf098();                                // _98
	virtual void vf09C(f32 scale);                       // _9C, scales the pose
	virtual void vf0A0();                                // _A0
	virtual void vf0A4();                                // _A4
	virtual void vf0A8();                                // _A8
	virtual void vf0AC();                                // _AC
	virtual void vf0B0();                                // _B0
	virtual void vf0B4();                                // _B4
	virtual void vf0B8();                                // _B8
	virtual void vf0BC();                                // _BC
	virtual void vf0C0();                                // _C0
	virtual void vf0C4();                                // _C4
	virtual void vf0C8();                                // _C8
	virtual void vf0CC();                                // _CC
	virtual void vf0D0();                                // _D0
	virtual void vf0D4();                                // _D4
	virtual void vf0D8();                                // _D8
	virtual void vf0DC();                                // _DC
	virtual void vf0E0();                                // _E0
	virtual void vf0E4();                                // _E4
	virtual void vf0E8();                                // _E8
	virtual void vf0EC();                                // _EC
	virtual void vf0F0();                                // _F0
	virtual void vf0F4();                                // _F4
	virtual void vf0F8();                                // _F8
	virtual void vf0FC();                                // _FC
	virtual void SetMoveDir(const Vec* dir, f32 speed);  // _100, CXObj impl
	virtual void SetRotateY(f32 a, f32 b);               // _104, CXObj impl
	virtual void SetAnim(s32 id, BOOL force, f32 frame, f32 rate, f32 blend,
	                     f32 duration);                  // _108
	virtual void vf10C();                                // _10C
	virtual void ClearAnim();                            // _110
	virtual void vf114(f32 rate);                        // _114, sets the anim rate
	virtual void vf118();                                // _118
	virtual s32 vf11C();                                 // _11C, current anim id
	virtual void vf120();                                // _120
	virtual void vf124();                                // _124
	virtual void vf128();                                // _128
	virtual void vf12C();                                // _12C
	virtual void vf130();                                // _130
	virtual s32 vf134();                                 // _134, state query, callers test it
	virtual void vf138();                                // _138
	virtual void vf13C();                                // _13C
	virtual void vf140();                                // _140
	virtual void vf144();                                // _144
	virtual void vf148();                                // _148

	u8 _08[0x24];
	CResource mResource; // _2C
	TVec mMoveTranslate; // _30, the object's pose position
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

	// XObj's own virtuals, secondary part slots 0x14C..0x288
	// retail's out-of-line definition keeps the GameCube u32 parameter, but
	// callers here emit no argument (the callee never reads it)
	virtual void InitializeObject();                       // _14C
	virtual void vf150();                                  // _150
	virtual s32 ResetObjectState();                        // _154
	virtual BOOL LoadFromPlacement(const CoordInfo& info); // _158
	virtual void vf15C();                                  // _15C
	virtual void vf160();                                  // _160
	virtual void UpdateObject(u32 frames);                 // _164
	virtual void vf168();                                  // _168
	virtual void vf16C();                                  // _16C
	virtual void vf170();                                  // _170
	virtual void vf174();                                  // _174
	virtual s32 vf178(const CoordInfo& info);              // _178
	virtual void vf17C();                                  // _17C
	virtual void vf180();                                  // _180
	virtual void vf184(u32 frames);                        // _184
	virtual void vf188();                                  // _188
	virtual void DrawCollisionShapes();                    // _18C
	virtual void vf190();                                  // _190
	virtual void DrawDebugStatus();                        // _194
	virtual void vf198();                                  // _198
	virtual void vf19C();                                  // _19C
	virtual void vf1A0();                                  // _1A0
	virtual void vf1A4();                                  // _1A4
	virtual void vf1A8();                                  // _1A8
	virtual void vf1AC();                                  // _1AC
	virtual void vf1B0();                                  // _1B0
	virtual void vf1B4(const Mtx mtx);                     // _1B4, copies the pose matrix
	virtual void vf1B8();                                  // _1B8
	virtual void vf1BC();                                  // _1BC
	virtual void vf1C0();                                  // _1C0
	virtual void vf1C4();                                  // _1C4
	virtual u32 TestFloorCollision(const Vec* from, const Vec* to, f32 radius, f32 height,
	                               Vec* push, Vec* contact, s32 arg6, s32 arg7); // _1C8
	virtual s32 OnPlayerContact(s32 unused, s32 value);                          // _1CC
	virtual BOOL TestFloorCrossing(const Vec& start, const Vec& end);            // _1D0
	virtual void vf1D4();                                                        // _1D4
	virtual void vf1D8();                                                        // _1D8
	virtual void vf1DC();                                                        // _1DC
	virtual void vf1E0();                                                        // _1E0
	virtual void vf1E4();                                                        // _1E4
	virtual void vf1E8();                                                        // _1E8
	virtual const void* GetEventScript();                                        // _1EC
	virtual void SetColor(s32 blend, u8 r, u8 g, u8 b, f32 time);                // _1F0
	virtual void OnPushDisplacement(const Vec& displacement);                    // _1F4
	virtual BOOL ReceivePlugPower(f32 amount);                                   // _1F8
	virtual void OnHitFromPosition(const Vec& position, f32 strength);           // _1FC
	virtual void OnSquirterHit(const Vec* contact, s32 value, s32 tint);         // _200
	virtual BOOL OnItemUsed(const Vec* position, s32 value);                     // _204
	virtual void vf208();                                                        // _208
	virtual void vf20C();                                                        // _20C
	virtual void vf210();                                                        // _210
	virtual void vf214();                                                        // _214
	virtual void vf218();                                                        // _218
	virtual void vf21C();                                                        // _21C
	virtual void vf220();                                                        // _220
	virtual void vf224();                                                        // _224
	virtual void vf228();                                                        // _228
	virtual void* vf22C(const char* name); // _22C, finds a named joint in the model
	virtual Vec vf230(void* joint);        // _230, reads a joint's world position
	virtual void vf234();                  // _234
	virtual void vf238();                  // _238
	virtual void vf23C();                  // _23C
	virtual void vf240();                  // _240
	virtual CObjVec* vf244();              // _244, the object's pose position channel
	virtual void vf248();                  // _248
	virtual void vf24C();                  // _24C
	virtual void vf250();                  // _250
	virtual void vf254();                  // _254
	virtual CObjVec* vf258();              // _258, the object's pose scale channel
	virtual void vf25C();                  // _25C
	virtual void vf260();                  // _260
	virtual void vf264();                  // _264
	virtual void vf268();                  // _268
	virtual void vf26C();                  // _26C
	virtual void vf270();                  // _270
	virtual void vf274();                  // _274
	virtual void vf278();                  // _278
	virtual void vf27C();                  // _27C
	virtual void vf280();                  // _280
	virtual void vf284();                  // _284
	virtual void vf288();                  // _288

	CStatus64 mStatus; // _150, 64 bit state flags
	CoordInfo mCoord;  // _160, the object's placement record
	AttrInfo mAttr;    // _168
	XObjFloor* mFloors; // _174, the object's collision floor list
	u32 mFloorMasks[2]; // _178, per-floor collision enables
	u8 _180[0x1C];
	CVec mLastPos; // _19C
	u8 _1AC[0x54C];
	CVec mToppleOffset; // _6F8
	u8 _708[0xC];
	CVec mVec714; // _714
	CEvt mEvt;    // _724
	u32 mEventScript; // _790, event script started on use, NULL if none
	u8 _794[0x24];
	CAnmQueue mAnmQueue; // _7B8
	CJArchive mArchive;  // _820
	u8 _968[0x40];
};
STATIC_ASSERT(sizeof(XObj) == 0x9A8);

// the base of the people and toys, the GameCube's members shifted by +0x28
class XObjActor : public XObj {
public:
	virtual void OnPushDisplacement(const Vec& displacement); // _1F4 override

	f32 mHeading;   // _9A8, the way it walks
	f32 mMoveSpeed; // _9AC, move speed
	u8 _9B0[0x9B8 - 0x9B0];
	s32 mKindStep;    // _9B8, the kind's own step
	u32 mKindCounter; // _9BC, cleared with mKindStep

	Vec GetCameraTarget();
};

// the plain object an XObjHBox carries
struct XObjHBoxPart {
	XObj mObj;              // _00
	f32 mHit;               // _9A8, set to 1 when chibi robo hits the box
	u8 _9AC[0x9B0 - 0x9AC]; // _9AC
	f32 mRange;             // _9B0
	s32 mSfx;               // _9B4, keyed off on a hit
};
STATIC_ASSERT(sizeof(XObjHBoxPart) == 0x9B8);

// a box that reads a second object (placement 36) and draws it inside itself
class XObjHBox : public XObj {
public:
	virtual void Release();                              // _40 override
	virtual void InitializeObject();                     // _14C override, fn_801164D8
	virtual void vf150();                                // _150 override, fn_80116518
	virtual s32 ResetObjectState();                      // _154 override
	virtual BOOL LoadFromPlacement(const CoordInfo& info); // _158 override
	virtual void UpdateObject(u32 frames);                 // _164 override
	virtual s32 vf178(const CoordInfo& info);              // _178 override
	virtual void vf184(u32 frames);                        // _184 override
	virtual void DrawScreenOverlay();                      // _190 override
	virtual void OnHitFromPosition(const Vec& position, f32 strength); // _1FC override

	s32 mLoadState;     // _9A8
	f32 mRing;          // _9AC, draws the two rings while above 0
	XObjHBoxPart mPart; // _9B0
};
STATIC_ASSERT(sizeof(XObjHBox) == 0x1368);

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
