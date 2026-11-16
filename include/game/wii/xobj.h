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
// object _04 (3 slots). MWCC appends a class's own virtuals behind the last
// part and reaches them through the first part's pointer, so the slot comments
// of CObj and XObj below count from the start of the whole table: CObj's own
// run from _2C to _148 (the last two are its overriders of IScnObjCallback's,
// which the second part reaches through this-adjusting thunks), XObj's from
// _14C (verified against InitializeObjectPools' virtual calls and against the
// calls inside the two classes' small virtuals at 0x800433FC-0x80044500).
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}

namespace g3d {

// the three hooks of nw4r's world-matrix pass; their parameters are not recovered
class ICalcWorldCallback {
public:
	virtual ~ICalcWorldCallback() { } // _08
	virtual void ExecCallbackA();     // _0C
	virtual void ExecCallbackB();     // _10
	virtual void ExecCallbackC();     // _14
};

class IScnObjCallback {
public:
	virtual ~IScnObjCallback() { } // _08
	// their names and parameters are not yet recovered
	virtual void _unk_00C();
	virtual void _unk_010();
};

// a node (bone) of a model resource, as far as the game reads it
struct ResNodeData {
	u8 _00[0x10];
	u32 mMtxID; // _10, the node's index in the model's world matrix array
	u8 _14[0x18];
	Vec mRotate;    // _2C, in degrees
	Vec mTranslate; // _38
};

// nw4r's handle to a node; the two checks carry g3d_resnode_ac.h's lines
class ResNode {
public:
	ResNodeData* mpData; // _00

	bool IsValid() const { return mpData != NULL; }
	ResNodeData* ptr() const { return mpData; }
	ResNodeData& ref() const
	{
		if (!IsValid()) {
			db::Panic("g3d_resnode_ac.h", 44, "%s::%s: Object not valid.", "ResNode", "ref");
		}
		return *mpData;
	}
	u32 GetMtxID() const
	{
		if (!IsValid()) {
			db::Panic("g3d_resnode_ac.h", 83, "NW4R:Failed assertion IsValid()");
		}
		if (IsValid()) {
			return ptr()->mMtxID;
		}
		return 0;
	}
};

// the scene model, as far as the game reads it
class ScnMdlSimple {
public:
	u8 _00[0xEC];
	Mtx* mpWorldMtxArray; // _EC, one per ResNodeData::mMtxID
};

// nw4r's handle to a model resource
class ResMdl {
public:
	void* mpData; // _00

	ResNode GetResNode(const char* name) const; // 0x8025CFC8
	ResNode GetResNode(u32 index) const;        // 0x8025D05C
	u32 GetResNodeNumEntries() const;           // 0x8025D104
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
// ccameraactioneye.h (which cgame.h includes) carries a second copy of the class for the camera
// code; until the two are one, a file that needs both headers includes that one first and takes it
#ifndef _GAME_CCAMERAACTIONEYE_H
class CObjVec : public TVec {
public:
	CObjVec();
	u8 Step(u32 frames); // 0x802A7FB4

	CVec mTarget; // _10
	CVec mSpeed;  // _20

	virtual ~CObjVec() { }
};
STATIC_ASSERT(sizeof(CObjVec) == 0x30);
#endif

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

	// the three run on the model members CObj still keeps as _E8.._13C: CJArchive's NW_Anm has
	// the same two cleared words 0x58 ahead of its vtable, so the class most likely starts there
	CVec GetNodeRotate(nw4r::g3d::ResNode node) const;    // 0x80043518, in radians
	CVec GetNodeTranslate(nw4r::g3d::ResNode node) const; // 0x80043988
	void ReleaseWork();                                   // 0x802B8674, frees what hangs at its _4C
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
	virtual void ExecCallbackA(); // _0C
	virtual void ExecCallbackB(); // _10
	virtual void ExecCallbackC(); // _14
	virtual void vf2C();
	virtual void vf30();
	virtual void vf34();
	virtual void vf38();
	virtual void vf3C();
	virtual void vf40();                                     // _40, XObj's is Release
	virtual u8 StepTranslate(u32 frames);                    // _44
	virtual u8 StepRotate(u32 frames);                       // _48
	virtual u8 StepScale(u32 frames);                        // _4C
	virtual nw4r::g3d::ResNode GetResNode(const char* name); // _50
	virtual nw4r::g3d::ResNode GetResNode(u32 index);        // _54
	virtual nw4r::g3d::ResNode GetResNode();                 // _58, the first node
	virtual nw4r::g3d::ResNode GetRootResNode();             // _5C
	virtual void SetTranslateX(f32 x);                       // _60
	virtual void SetTranslateY(f32 y);                       // _64
	virtual void SetTranslateZ(f32 z);                       // _68
	virtual void SetRotationX(f32 x);                        // _6C
	virtual void SetRotationY(f32 y);                        // _70
	virtual void SetRotationZ(f32 z);                        // _74
	virtual void SetScaleX(f32 x);                           // _78
	virtual void SetScaleY(f32 y);                           // _7C
	virtual void SetScaleZ(f32 z);                           // _80
	virtual void SetPos(f32 x, f32 y, f32 z);                // _84
	virtual void SetRot(f32 x, f32 y, f32 z);                // _88
	virtual void SetScale(f32 x, f32 y, f32 z);              // _8C
	virtual void SetPos(const Vec& pos);                     // _90
	virtual void SetRot(const Vec& rot);                     // _94
	virtual void SetScale(const Vec& scale);                 // _98
	virtual void vf9C();
	virtual MtxP GetWorldMtx(nw4r::g3d::ResNode node);      // _A0, NULL without a model
	virtual MtxP GetWorldMtx(const char* name);             // _A4
	virtual MtxP GetWorldMtx(u32 index);                    // _A8
	virtual MtxP GetWorldMtx();                             // _AC
	virtual CVec GetWorldPos(nw4r::g3d::ResNode node);      // _B0
	virtual CVec GetWorldPos(const char* name);             // _B4
	virtual CVec GetWorldPos(u32 index);                    // _B8
	virtual CVec GetWorldPos();                             // _BC
	virtual CVec GetNodeTranslate(nw4r::g3d::ResNode node); // _C0, the node's own translation
	virtual CVec GetNodeTranslate(const char* name);        // _C4
	virtual CVec GetNodeTranslate(u32 index);               // _C8
	virtual CVec GetNodeTranslate();                        // _CC
	virtual f32 GetNodeRotateX(nw4r::g3d::ResNode node);    // _D0, the node's own rotation, in radians
	virtual f32 GetNodeRotateX(const char* name);           // _D4
	virtual f32 GetNodeRotateX(u32 index);                  // _D8
	virtual f32 GetNodeRotateX();                           // _DC
	virtual f32 GetNodeRotateY(nw4r::g3d::ResNode node);    // _E0
	virtual f32 GetNodeRotateY(const char* name);           // _E4
	virtual f32 GetNodeRotateY(u32 index);                  // _E8
	virtual f32 GetNodeRotateY();                           // _EC
	virtual f32 GetNodeRotateZ(nw4r::g3d::ResNode node);    // _F0
	virtual f32 GetNodeRotateZ(const char* name);           // _F4
	virtual f32 GetNodeRotateZ(u32 index);                  // _F8
	virtual f32 GetNodeRotateZ();                           // _FC
	virtual void SetMoveDir(const Vec* dir, f32 speed);     // _100
	virtual void SetRotateY(f32 target, f32 speed);         // _104
	virtual void vf108();
	virtual void vf10C();
	virtual void vf110(); // _110, XObj's is ClearAnim
	virtual void vf114();
	virtual void vf118();
	virtual void vf11C();
	virtual f32 GetAnimRate(s32 index); // _120
	virtual f32 GetAnimRate();          // _124
	virtual f32 GetAnimFrame();         // _128
	virtual f32 GetAnimEndFrame();      // _12C
	virtual bool IsAnimEnd(f32 frames); // _130, the animation runs out within frames
	virtual void vf134();
	virtual void vf138();
	virtual void ReleaseModelWork(); // _13C
	virtual void vf140();
	virtual void _unk_00C(); // _144, IScnObjCallback's
	virtual void _unk_010(); // _148

	// the node's world matrix, NULL without a model. retail expands the same code in GetWorldMtx
	// (0x8001A3C0) and in GetWorldPos (0x80043B54), with the node copied first as an argument is
	MtxP GetNodeWorldMtx(nw4r::g3d::ResNode node) const { return mScnMdl != NULL ? mScnMdl->mpWorldMtxArray[node.GetMtxID()] : NULL; }

protected:
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
	u8 _C0[4];
	f32 mAnimRate[4]; // _C4
	CColor mColor0;   // _D4
	CColor mColor1;   // _DC
	u8 _E4[4];
	u32 _E8;
	u32 _EC;                          // the model's nw4r::g3d::ResMdl
	nw4r::g3d::ScnMdlSimple* mScnMdl; // _F0, NULL without a model
	u8 _F4[0x4C];
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

	virtual void ResetForNewGame();                        // _150
	virtual void ResetObjectState();                       // _154
	virtual BOOL LoadFromPlacement(const CoordInfo& info); // _158
	virtual void vf15C();
	virtual void vf160();
	virtual void UpdateObject(u32 frames); // _164
	virtual void vf168();
	virtual void vf16C();
	virtual void vf170();
	virtual void DrawObject();           // _174
	virtual void DrawWithView(Mtx view); // _178
	virtual void vf17C();
	virtual void vf180();
	virtual void vf184();
	virtual void vf188();
	virtual void DrawCollisionShapes(); // _18C
	virtual void vf190();
	virtual void DrawDebugStatus(); // _194
	virtual void vf198();
	virtual void vf19C();
	virtual void vf1A0();
	virtual void vf1A4();
	virtual void vf1A8();
	virtual void vf1AC();
	virtual Vec GetCameraTarget();    // _1B0, where a camera looks at this object
	virtual void SetBaseMtx(Mtx mtx); // _1B4, keeps a copy of mtx and points mBaseMtxPtr at it
	virtual void ClearBaseMtx();      // _1B8
	virtual void vf1BC();
	virtual void vf1C0();
	virtual void vf1C4();
	virtual u32 TestFloorCollision(const Vec* from, const Vec* to, f32 radius, f32 height, Vec* push, Vec* contact, s32 arg6,
	                               s32 arg7);                         // _1C8
	virtual s32 OnPlayerContact(s32 unused, s32 value);               // _1CC
	virtual BOOL TestFloorCrossing(const Vec& start, const Vec& end); // _1D0
	virtual void vf1D4();
	virtual void vf1D8();
	virtual void vf1DC();
	virtual void vf1E0();
	virtual void vf1E4();
	virtual s32 HandleEventCommand(s32 count, s32* args);                // _1E8
	virtual const void* GetEventScript();                                // _1EC
	virtual void SetColor(s32 blend, u8 r, u8 g, u8 b, f32 time);        // _1F0
	virtual void OnPushDisplacement(const Vec& displacement);            // _1F4
	virtual BOOL ReceivePlugPower(f32 amount);                           // _1F8
	virtual void OnHitFromPosition(const Vec& position, f32 strength);   // _1FC
	virtual void OnSquirterHit(const Vec* contact, s32 value, s32 tint); // _200
	virtual BOOL OnItemUsed(const Vec* position, s32 value);             // _204
	virtual void vf208();
	virtual void vf20C();
	virtual void vf210();
	virtual void OnMoveRequest();                                  // _214, runs ahead of the three requests below
	virtual void RequestRotationY(f32 y);                          // _218
	virtual void RequestRotateY(f32 target, f32 speed);            // _21C
	virtual void RequestMoveDir(const Vec* dir, f32 speed);        // _220
	virtual BOOL HasModel();                                       // _224
	virtual nw4r::g3d::ResNode GetResNodeOrFirst(u32 index);       // _228, the first node when index is out of range
	virtual nw4r::g3d::ResNode FindResNode(const char* name);      // _22C
	virtual CVec GetJointPos(nw4r::g3d::ResNode node);             // _230
	virtual CVec GetJointPos(const char* name);                    // _234
	virtual CVec GetJointPos(u32 index);                           // _238
	virtual CVec GetJointWorldPos(const char* name);               // _23C
	virtual CVec GetJointWorldPos(u32 index);                      // _240
	virtual CVec* GetPos();                                        // _244
	virtual CVec GetJointTranslate(const char* name);              // _248
	virtual CVec GetJointTranslate(u32 index);                     // _24C
	virtual CVec GetJointScale(nw4r::g3d::ResNode node);           // _250, the scale in the node's world matrix
	virtual void GetJointScale(nw4r::g3d::ResNode node, Vec* out); // _254
	virtual CVec* GetScale();                                      // _258
	virtual f32 GetPosX();                                         // _25C
	virtual f32 GetPosY();                                         // _260
	virtual f32 GetPosZ();                                         // _264
	virtual f32 GetRotX();                                         // _268
	virtual f32 GetRotY();                                         // _26C
	virtual f32 GetRotZ();                                         // _270
	virtual f32 GetScaleX();                                       // _274
	virtual f32 GetScaleY();                                       // _278
	virtual f32 GetScaleZ();                                       // _27C
	virtual void vf280();
	virtual void vf284(); // _284, calls vf280
	virtual void vf288();
	// CObj's
	virtual void ExecCallbackA();                            // _0C
	virtual void ExecCallbackB();                            // _10
	virtual void vf40();                                     // _40, Release
	virtual nw4r::g3d::ResNode GetResNode(const char* name); // _50
	virtual nw4r::g3d::ResNode GetResNode(u32 index);        // _54
	virtual nw4r::g3d::ResNode GetResNode();                 // _58
	virtual void vf108();                                    // _108
	virtual void vf110();                                    // _110, ClearAnim
	virtual void _unk_010();                                 // _148

private:
	CStatus64 mStatus; // _150
	CoordInfo mCoord;  // _160
	AttrInfo mAttr;    // _168
	u8 _174[0x28];
	CVec mLastPos; // _19C
	u8 _1AC[0x54C];
	CVec mToppleOffset; // _6F8
	u8 _708[0xC];
	CVec mVec714;             // _714
	CEvt mEvt;                // _724
	const void* mEventScript; // _790, the event script the object starts when used
	u8 _794[0x24];
	CAnmQueue mAnmQueue; // _7B8
	CJArchive mArchive;  // _820
	Mtx mBaseMtx;        // _968, SetBaseMtx's copy
	MtxP mBaseMtxPtr;    // _998, NULL or mBaseMtx
	u8 _99C[0xC];
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
