#ifndef _GAME_XOBJATTACHMENT_H
#define _GAME_XOBJATTACHMENT_H

#include "Version.h"
#if defined(VERSION_R24J01)
#include "Dolphin/mtx.h"
#include "game/ut.h"
#include "game/wii/xobj.h"
class WiiTitleObject;
class XObjPlug;
#else
#include "game/xobj.h"
#endif
#include "game/mapfloorhit.h"
#include "game/ccameraaction.h"

// the objects chibi robo carries around with him, class names from RTTI. XObjChr holds one
// of each, the plug and its cord first. offsets are usa, each XObj is 0x10 shorter in japan

#if !defined(VERSION_R24J01)
// the wire effect along the plug's cord
class XObjWireEff : public XObj {
public:
	s32 mAnim;                 // _980, -1 when off
	f32 mRate;                 // _984
	s32 mEffect;               // _988, -1 when off
	HSD_Generator* mGenerator; // _98C
	s32 mSparkMode;            // _990
	s32 mSparkTimer;           // _994

	XObjWireEff();

	virtual ~XObjWireEff() { }             // _08 (weak)
	virtual void Release();                // _54
	virtual void ResetObjectState();       // _98
	virtual void UpdateObject(u32 frames); // _A4
	virtual void DrawWithView(Mtx view);   // _B0
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjWireEff) == 0x988);
#else
STATIC_ASSERT(sizeof(XObjWireEff) == 0x998);
#endif
#endif

// one point of the cord, it steps like a CObjVec but has no vtable of its own. On the
// GameCube the ctor and dtor are implicit: the compiler emits both after InitPlugCord,
// whose function-scope static array needs them
struct XObjPlugCordPoint {
#if defined(VERSION_R24J01)
	XObjPlugCordPoint();
	~XObjPlugCordPoint();
#endif

	CVec mPos;    // _00
	CVec mTarget; // _10
	CVec mSpeed;  // _20
};
STATIC_ASSERT(sizeof(XObjPlugCordPoint) == 0x30);

// a spark that runs along the cord
struct XObjPlugCordNode {
	s32 mIndex;                    // _00, the point it's on
	CVec mPos;                     // _04
	CInterpolationEaseInOut mStep; // _14
	f32 mDistance;                 // _30, along the current segment
	CVec mDir;                     // _34
	s32 mState;                    // _44, 1 while running
	s32 mMode;                     // _48, 1 starts at the far end, -1 at point 0

	XObjPlugCordNode();
	~XObjPlugCordNode();
};
STATIC_ASSERT(sizeof(XObjPlugCordNode) == 0x4C);

struct XObjPlugCordNodes {
	XObjPlugCordNode mNode[10]; // _00
	u32 mColor;                 // _2F8
	u32 mCount;                 // _2FC
	u32 mMode;                  // _300, handed to each new node
	BOOL mEnabled;              // _304
};
STATIC_ASSERT(sizeof(XObjPlugCordNodes) == 0x308);

struct XObjPlugCord {
	s32 mCount;                 // _00
	XObjPlugCordPoint* mPoints; // _04, one shared buffer of 128
	f32 mSpeed;                 // _08
	Vec mEnd[3];                // _0C, the plug end first, then the last point
	const Vec* mEndSource[3];   // _30, followed every frame when set
	f32 _3C;                    // _3C, set to 0.0f in InitPlugCord, never read
	f32 _40;                    // _40, set to 0.0f in InitPlugCord, never read
	u32 _44;                    // _44, never accessed
	const f32* mEndHeight[3];   // _48, height-only constraints
	u8 _54;                     // _54, set to 6 in InitPlugCord, never read
	u8 _55[0x58 - 0x55];        // _55
	f32 mWidth;                 // _58
	u32 mColor;                 // _5C
	u32 _60;   // _60, always written with the same value as mColor (InitPlugCord, InitializeObject, UpdateObject, DrawGroundShadow), never
	           // read
	u8 mAlpha; // _64
	u8 _65[0x68 - 0x65]; // _65
	u32 mFlags;          // _68
	CVec mPull; // _6C, the cord's pull on its pinned first end, the first segment's correction negated; drags the player while the plug is
	            // connected
	XObjMapHit mFloorHit;     // _7C
	s32 mMode;                // _88
	XObjPlugCordNodes mNodes; // _8C
};
STATIC_ASSERT(sizeof(XObjPlugCord) == 0x394);

#if !defined(VERSION_R24J01)
class XObjPlug : public XObj {
public:
	XObjPlugCord mCord;  // _980
	Vec mCordAnchor;     // _D14, source for the first cord endpoint
	Vec mCordJointPos;   // _D20
	Vec mCordJointTip;   // _D2C
	s32 mCordEndsPinned; // _D38, 1 while cord ends 1 and 2 follow mCordJointPos and mCordJointTip (plug held or connected), 0 once they are
	                     // let go
	s32 mConnection;     // _D3C, XActLift's _54 with 0x10000 set
	s32 mLoadState;      // _D40
	u32 _D44;            // _D44, never accessed
	XObjWireEff mWire;   // _D48

	XObjPlug();

	virtual ~XObjPlug() { }                                   // _08 (weak)
	virtual void Release();                                   // _54
	virtual void InitializeObject(u32 id);                    // _90
	virtual void ResetForNewGame();                           // _94
	virtual void ResetObjectState();                          // _98
	virtual BOOL LoadFromPlacement(const CoordInfo& info);    // _9C
	virtual void UpdateObject(u32 frames);                    // _A4
	virtual void DrawWithView(Mtx view);                      // _B0
	virtual void DrawReflection(Mtx view);                    // _B4
	virtual void DrawGroundShadow();                          // _B8
	virtual void DrawObjectEffects();                         // _C0
	virtual void SetDestinationAlpha(BOOL enabled, u8 value); // _DC
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjPlug) == 0x16C0);
#else
STATIC_ASSERT(sizeof(XObjPlug) == 0x16E0);
#endif

// the base of the copter, the blaster, the radar and the ox marks
class XObjAttachment : public XObj {
public:
	HSD_JObj* mParent; // _980, the joint it copies its matrix from
	BOOL mVisible;     // _984, cleared when it's put away

	XObjAttachment();

	virtual ~XObjAttachment() { }                                 // _08 (weak)
	virtual void Release();                                       // _54
	virtual void InitializeObject(u32 id);                        // _90
	virtual void ResetForNewGame();                               // _94
	virtual void ResetObjectState();                              // _98
	virtual void DrawObject();                                    // _AC
	virtual void DrawWithView(Mtx view);                          // _B0
	virtual void Show();                                          // _148
	virtual void Hide();                                          // _14C
	virtual void SetParent(HSD_JObj* parent, BOOL fromPlacement); // _150
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjAttachment) == 0x978);
#else
STATIC_ASSERT(sizeof(XObjAttachment) == 0x988);
#endif

class XObjCopter : public XObjAttachment {
public:
	XObjCopter();

	virtual ~XObjCopter() { }              // _08 (weak)
	virtual void UpdateObject(u32 frames); // _A4
};

class XObjPopper : public XObjAttachment {
public:
	XObjPopper();

	virtual ~XObjPopper() { }              // _08 (weak)
	virtual void UpdateObject(u32 frames); // _A4
};

class XObjRadar : public XObjAttachment {
public:
	s32 mState;             // _988
	f32 mYaw;               // _98C, NAN until the first aim
	f32 mPitch;             // _990
	u8 _994[0x998 - 0x994]; // _994

	XObjRadar();

	virtual ~XObjRadar() { }                               // _08 (weak)
	virtual void InitializeObject(u32 id);                 // _90
	virtual BOOL LoadFromPlacement(const CoordInfo& info); // _9C
	virtual void UpdateObject(u32 frames);                 // _A4
};

class XObjOX : public XObjAttachment {
public:
	XObjOX();

	virtual ~XObjOX() { }                  // _08 (weak)
	virtual void UpdateObject(u32 frames); // _A4
};
#endif

#if defined(VERSION_R24J01)
// the Wii base of the copter, the blaster, the radar and the OX marks: the same
// class as the GameCube's below, but XObj runs to _9A8 on the Wii and the parent
// handle is a plain matrix pointer the callback copies
class XObjAttachment : public XObj {
public:
	MtxP mParent;  // _9A8, the owner's matrix it copies its transform from, NULL while detached
	BOOL mVisible; // _9AC, cleared when it's put away

	XObjAttachment();

	// the GameCube header's own shape: the dtor stays non-inline so the vtable
	// lands with it in xobjmember.cpp like retail's
	virtual ~XObjAttachment();                                                                                                  // _08
	virtual void ExecCallbackB(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj); // _10
	virtual void Release();                                                                                                     // _40
	virtual void InitializeObject();                                                                                            // _14C
	virtual void ResetForNewGame();                                                                                             // _150
	virtual void ResetObjectState();                                                                                            // _154
	virtual void DrawWithView(Mtx view);                                                                                        // _178
	virtual void Show();                                                                                                        // _28C
	virtual void Hide();                                                                                                        // _290
	virtual void SetParent(MtxP parent, BOOL fromPlacement);                                                                    // _294
};
STATIC_ASSERT(sizeof(XObjAttachment) == 0x9B0);

class XObjCopter : public XObjAttachment {
public:
	XObjCopter();

	virtual ~XObjCopter() { }              // _08 (weak)
	virtual void UpdateObject(u32 frames); // _164
};

class XObjPopper : public XObjAttachment {
public:
	XObjPopper();

	virtual ~XObjPopper() { } // _08 (weak)
	virtual void ExecCallbackB(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj); // _10
	virtual void UpdateObject(u32 frames);                                                                                      // _164
};

class XObjRadar : public XObjAttachment {
public:
	s32 mState;             // _9B0
	f32 mYaw;               // _9B4, the dish's turning inside the scan
	f32 mPitch;             // _9B8
	u8 _9BC[0x9C0 - 0x9BC]; // _9BC

	XObjRadar();

	virtual ~XObjRadar() { } // _08 (weak)
	virtual void ExecCallbackB(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj); // _10
	virtual void InitializeObject();                                                                                            // _14C
	virtual BOOL LoadFromPlacement(const CoordInfo& info);                                                                      // _158
	virtual void UpdateObject(u32 frames);                                                                                      // _164
};
STATIC_ASSERT(sizeof(XObjRadar) == 0x9C0);

class XObjOX : public XObjAttachment {
public:
	XObjOX();

	virtual ~XObjOX() { }                  // _08 (weak)
	virtual void UpdateObject(u32 frames); // _164
};

// the plug at the end of chibi robo's cord: same members as the GameCube class,
// the wire effect at _DA0, and a joint matrix + roll the callback maintains
// instead of the GameCube's HSD_JObj parenting
class XObjPlug : public XObj {
public:
	XObjPlugCord mCord;  // _9A8
	Vec mCordAnchor;     // _D3C, source for the first cord endpoint
	Vec mCordJointPos;   // _D48
	Vec mCordJointTip;   // _D54
	s32 mCordEndsPinned; // _D60, 1 while cord ends 1 and 2 follow mCordJointPos and mCordJointTip (plug held or connected), 0 once
	                     // they are let go
	s32 mConnection;     // _D64, nonzero while the plug sits in a socket
	Mtx mPlugMtx;        // _D68, the transform ExecCallbackB hands the plug joint
	f32 mPlugRot;        // _D98, the joint's Z roll while carried
	s32 mLoadState;      // _D9C
	XObjWireEff mWire;   // _DA0

	XObjPlug();

	virtual ~XObjPlug() { } // _08 (weak)
	virtual void ExecCallbackB(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj); // _10
	virtual void Release();                                                                                                     // _40
	virtual void InitializeObject();                                                                                            // _14C
	virtual void ResetForNewGame();                                                                                             // _150
	virtual void ResetObjectState();                                                                                            // _154
	virtual BOOL LoadFromPlacement(const CoordInfo& info);                                                                      // _158
	virtual void UpdateObject(u32 frames);                                                                                      // _164
	virtual void DrawWithView(Mtx view);                                                                                        // _178
	virtual void DrawReflection(Mtx view);                                                                                      // _17C
	virtual void DrawGroundShadow(f32 opacity);                                                                                 // _180
	virtual void DrawObjectEffects();                                                                                           // _188
	virtual void SetDestinationAlpha(BOOL enabled, u8 value); // _1A4, the base's vf1A4 slot
};
STATIC_ASSERT(sizeof(XObjPlug) == 0x1760);
STATIC_ASSERT(offsetof(XObjPlug, mWire) == 0xDA0);
#endif

extern "C" {
CVec GetPlugCordPointPosition(const XObjPlugCord* cord, s32 index);
void GetPlayerPlugCordAnchorTransform(void*, Vec* position, Vec* rotation, Vec* scale);
void UpdatePlugCordSimulation(XObjPlugCord* cord, u32 frames);
void ResetPlayerPlugState(XObjPlug* plug, BOOL resetJoint);
// the cord, the scale arguments go unused; the Wii splits the GS-state entry
// (DrawPlugCordNodes) from the node loop (DrawPlugCordSparks), the GameCube did both
#if defined(VERSION_R24J01)
void DrawPlugCordNodes(XObjPlugCord* cord, Mtx view, f32 scaleX, f32 scaleY, f32 scaleZ);
void DrawPlugCordSparks(XObjPlugCordNodes* nodes, Mtx view, f32 scaleX, f32 scaleY, f32 scaleZ);
#else
void DrawPlugCordNodes(XObjPlugCord* cord, Mtx view, f32 scaleX, f32 scaleY, f32 scaleZ);
#endif
void DrawPlugCordSegments(XObjPlugCord* cord, Mtx view, f32 scaleX, f32 scaleY, f32 scaleZ);
void SetPlugCordAlphaMode(XObjPlugCord* cord, BOOL enabled, u8 alpha);
void SetPlugCordEndpointSource(XObjPlugCord* cord, u32 index, const Vec* source);
void ResetPlugCordPoints(XObjPlugCord* cord, f32 x, f32 y, f32 z);
f32 FindPlugCordFloor(XObjPlugCord* cord, Vec* pos, f32 radius, f32 height);
// the Wii object class is not reconstructed; the cord passes the table's WiiTitleObject
#if defined(VERSION_R24J01)
BOOL PushPlugCordPointFromObject(CVec* pos, WiiTitleObject* object, f32 radius);
#else
BOOL PushPlugCordPointFromObject(CVec* pos, XObj* object, f32 radius);
#endif
void InitPlugCord(XObjPlugCord* cord, s32 count, f32 x, f32 y, f32 z, f32 speed);
void UpdatePlugCordNodes(XObjPlugCordNodes* nodes, XObjPlugCordPoint* points, s32 count, u32 frames);
void UpdatePlugCordSpark(XObjPlugCordNode* node, XObjPlugCordPoint* points, s32 count, f32 value);
void StartPlugCordNode(XObjPlugCordNode* node, XObjPlugCordPoint* points, s32 count, f32 time, s32 mode);

// the copter's anims, game/objb_attach.cpp
void PlayCopterLandingAnimation(XObjCopter* copter);
void PlayCopterDescentAnimation(XObjCopter* copter);
void PlayCopterStowAnimation(XObjCopter* copter);
void ShowAndDeployCopter(XObjCopter* copter);
}

#if !defined(VERSION_R24J01)
// follows the joint it hangs from
inline void CopyParentMtx(XObjAttachment* object)
{
	MtxPtr mtx = HSD_JObjGetMtxPtr(object->mParent);
	HSD_JObjCopyMtx(object->mJObj, mtx);
}
#endif

#endif
