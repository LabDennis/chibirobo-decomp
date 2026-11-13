#ifndef _GAME_XOBJATTACHMENT_H
#define _GAME_XOBJATTACHMENT_H

#include "game/xobj.h"
#include "game/mapfloorhit.h"
#include "game/ccameraaction.h"

// the objects chibi robo carries around with him, class names from RTTI. XObjChr holds one
// of each, the plug and its cord first. offsets are usa, each XObj is 0x10 shorter in japan

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

// one point of the cord, it steps like a CObjVec but has no vtable of its own
struct XObjPlugCordPoint {
	XObjPlugCordPoint();

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
	f32 _3C;                    // _3C
	f32 _40;                    // _40
	u32 _44;                    // _44
	const f32* mEndHeight[3];   // _48, height-only constraints
	u8 _54;                     // _54
	u8 _55[0x58 - 0x55];        // _55
	f32 mWidth;                 // _58
	u32 mColor;                 // _5C
	u32 _60;                    // _60
	u8 mAlpha;                  // _64
	u8 _65[0x68 - 0x65];        // _65
	u32 mFlags;                 // _68
	CVec _6C;                   // _6C
	XObjMapHit mFloorHit;       // _7C
	u32 mMode;                  // _88
	XObjPlugCordNodes mNodes;   // _8C
};
STATIC_ASSERT(sizeof(XObjPlugCord) == 0x394);

class XObjPlug : public XObj {
public:
	XObjPlugCord mCord; // _980
	Vec mCordAnchor;    // _D14, source for the first cord endpoint
	Vec mCordJointPos;  // _D20
	Vec mCordJointTip;  // _D2C
	s32 _D38;           // _D38
	s32 _D3C;           // _D3C, XActLift's _54 with 0x10000 set
	s32 mLoadState;     // _D40
	u32 _D44;           // _D44
	XObjWireEff mWire;  // _D48

	XObjPlug();

	virtual ~XObjPlug();                                   // _08 (weak)
	virtual void Release();                                // _54
	virtual void InitializeObject(u32 id);                 // _90
	virtual void vf94();                                   // _94
	virtual void ResetObjectState();                       // _98
	virtual BOOL LoadFromPlacement(const CoordInfo& info); // _9C
	virtual void UpdateObject(u32 frames);                 // _A4
	virtual void DrawWithView(Mtx view);                   // _B0
	virtual void DrawReflection(Mtx view);                 // _B4
	virtual void DrawGroundShadow();                       // _B8
	virtual void DrawObjectEffects();                      // _C0
	virtual void vfDC(BOOL enabled, u8 value);             // _DC
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
	virtual void vf94();                                          // _94
	virtual void ResetObjectState();                              // _98
	virtual void vfAC();                                          // _AC
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

	virtual ~XObjRadar();                                  // _08 (weak)
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

extern "C" {
CVec GetPlugCordPointPosition(const XObjPlugCord* cord, s32 index);
void GetPlayerPlugCordAnchorTransform(void*, Vec* position, Vec* rotation, Vec* scale);
void UpdatePlugCordSimulation(XObjPlugCord* cord, u32 frames);
void ResetPlayerPlugState(XObjPlug* plug, BOOL resetJoint);
// the cord, the scale arguments go unused
void DrawPlugCordNodes(XObjPlugCord* cord, Mtx view, f32 scaleX, f32 scaleY, f32 scaleZ);
void DrawPlugCordSegments(XObjPlugCord* cord, Mtx view, f32 scaleX, f32 scaleY, f32 scaleZ);
void SetPlugCordAlphaMode(XObjPlugCord* cord, BOOL enabled, u8 alpha);
void SetPlugCordEndpointSource(XObjPlugCord* cord, u32 index, const Vec* source);
void ResetPlugCordPoints(XObjPlugCord* cord, f32 x, f32 y, f32 z);
f32 FindPlugCordFloor(XObjPlugCord* cord, Vec* pos, f32 radius, f32 height);
BOOL PushPlugCordPointFromObject(CVec* pos, XObj* object, f32 radius);
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

// follows the joint it hangs from
inline void CopyParentMtx(XObjAttachment* object)
{
	MtxPtr mtx = HSD_JObjGetMtxPtr(object->mParent);
	HSD_JObjCopyMtx(object->mJObj, mtx);
}

#endif
