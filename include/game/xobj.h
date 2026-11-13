#ifndef _GAME_XOBJ_H
#define _GAME_XOBJ_H

#include "game/mapfloorhit.h"
#include "game/cobj.h"
#include "game/ccamera.h"
#include "game/coordinfo.h"
#include "game/evt.h"

// class names from RTTI and the vtable at 0x80225984, the virtuals past CXObjIntp's are unnamed.
// offsets are usa, japan's CJArchive is 8 bytes shorter so everything here sits 8 lower there
// and anything after mAnim, another CJArchive, 0x10 lower

class CShadow;

struct CAnmQueueEntry {
	s32 mId;    // _00, -1 when free
	u16 mFrame; // _04
	s16 mRate;  // _06
};

// anims waiting to play after the current one, name from __vt__9CAnmQueue
class CAnmQueue {
public:
	s32 mNext;                 // _00
	CAnmQueueEntry mEntry[16]; // _04
	// _84 vtable

	virtual ~CAnmQueue() { } // _08

	// todo: cleanup
	void Clear()
	{
		mNext = 0;
		for (u32 i = 0; i < 16; ++i) {
			mEntry[i].mId = -1;
		}
	}
};
typedef char CAnmQueue_size_check[sizeof(CAnmQueue) == 0x88 ? 1 : -1];

// one of an object's floors, a list of them ends at an id of -1
struct XObjFloor {
	s16 mId; // _00
#if defined(VERSION_R24J01)
	u8 mMaterial; // _02, ground types 13 and up count as none
	u8 mFlags;    // _03, bit 0 includes this floor in contact tests, bit 1 in screen bounds
#else
	s16 mType; // _02, ground type, 13 and up count as none
#endif
	s32 mCenterOffset[3]; // _04, in hundredths
	s32 mYawDegrees;      // _10
	s32 mExtent[3];       // _14, in hundredths
};
typedef char XObjFloor_size_check[sizeof(XObjFloor) == 0x20 ? 1 : -1];

// a floor's edge, offsets from the object's position
struct XObjEdge {
	Vec mContactOffset; // _00
	Vec mFrom;          // _0C
	Vec mTo;            // _18
};
typedef char XObjEdge_size_check[sizeof(XObjEdge) == 0x24 ? 1 : -1];

// a material color stepping towards a target, the two of them end up in CXObj's mColors
struct XObjColorStep {
	Vec mValue;  // _00
	Vec mTarget; // _0C
	Vec mSpeed;  // _18
};
typedef char XObjColorStep_size_check[sizeof(XObjColorStep) == 0x24 ? 1 : -1];

// the game's object, everything in a stage that has a model
class XObj : public CXObjIntp {
public:
	// _00-_228 = CXObjIntp
	CStatus64 mStatus;    // _228, 1 in use, 0x10000 drawn, 0x0040000000000000 while a read is pending
	CoordInfo mCoord;     // _238
	AttrInfo mAttr;       // _240
	XObjFloor* mFloors;   // _24C
	u32 mFloorMasks[2];   // _250, a bit per floor, one mask per group
	s32 mPoolIndex;       // _258, index in its pool
	s32 mPlacementParam0; // _25C, the placement entry's _28, copied on load; its meaning is up to the kind: the spider's variant (0..11), a
	                      // support's ledge height when HasObjectCustomLedgeHeight
	s32 mPlacementParam1; // _260, the placement entry's _2C, copied on load; per kind: the collection flag index of coins and dust
	                      // (XObjFlags::GetFlagIndex), the spider's height above mHome
	CVec mLastPos;        // _264, POS copies the position here
	s32 mHitFloorIndex;   // _274
	XObjEdge mEdges[32];  // _278, one per floor
	f32 mFloorXZExtent;   // _6F8
	f32 mFloorHeights[32];    // _6FC, a height per floor
	f32 mMaxFloorHeight;      // _77C
	f32 mSelectedFloorHeight; // _780, highest active floor, NaN when none is active
	s32 mSelectedFloorIndex;  // _784
	XObjMapHit mFloorHit;     // _788
	CVec mToppleOffset;  // _794, offset of the floors while the object topples off a ledge after a push: half its width sideways, half its
	                     // height down; its XZ is added to the position when it lands, then it is cleared
	CEvt mEvt;           // _7A4, the object's own script
	u32 mEventScript;    // _80C, the placement's stage script (GetStageScriptByIndex), 0 if none; returned by GetEventScript
	CAnmQueue mAnmQueue; // _810
	f32 mAnchorX;        // _898, NAN while there's no anchor to clamp around
	f32 mAnchorZ;        // _89C
	f32 mLimitPosX;      // _8A0
	f32 mLimitNegX;      // _8A4
	f32 mLimitPosZ;      // _8A8
	f32 mLimitNegZ;      // _8AC
	u32 mMarkerOverride; // _8B0, clamp flags from the event call -11, 0x80000000 while it's off
	XObjColorStep mColorStep[2]; // _8B4
	CJAnim mAnim;                // _8FC, anims from the object's second archive, ids from 1000 up

	virtual ~XObj() { }     // _08
	virtual void Release(); // _54
	virtual void SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend,
	                     f32 blendRate);   // _80
	virtual void ClearAnim();              // _84
	virtual void InitializeObject(u32 id); // _90, init
	virtual void ResetForNewGame();        // _94, back to the new-game state; run on every pooled object when a new game starts
	virtual void ResetObjectState();       // _98, resets the move state
	virtual BOOL LoadFromPlacement(const CoordInfo& info);           // _9C, called until it returns true
	virtual BOOL LoadAnimationArchiveAsync(const char* path);        // _A0, reads the second archive
	virtual void UpdateObject(u32 frames);                           // _A4, update
	virtual void StepColors(u32 frames);                             // _A8, steps mColorStep
	virtual void DrawObject();                                       // _AC, draws the object (and its parts) with the current camera
	virtual void DrawWithView(Mtx view);                             // _B0, draw
	virtual void DrawReflection(Mtx view);                           // _B4
	virtual void DrawGroundShadow();                                 // _B8
	virtual void DrawGroundShadowAt(const Vec* origin, f32 opacity); // _BC
	virtual void DrawObjectEffects();                                // _C0
	virtual void DrawCollisionShapes();                              // _C4, debug draw of the collision floors and edges
	virtual void DrawScreenOverlay();                         // _C8, screen-space pass: gauges, rings and panels drawn over the scene
	virtual void DrawDebugStatus();                           // _CC
	virtual void RegisterShadowObjects(CShadow* shadow);      // _D0
	virtual void ExpandShadowBounds(CShadow* shadow);         // _D4
	virtual void ApplyMaterialPresets(u32* flags);            // _D8, applies the stage pixel-engine presets to the materials
	virtual void SetDestinationAlpha(BOOL enabled, u8 value); // _DC, destination alpha for every material
	virtual void EnableLighting(f32 factor);                // _E0, turns lighting on for the model, scales the diffuse by the stage factor
	virtual void SetTranslucent(f32 alpha);                 // _E4, draws the model translucent with alpha
	virtual void SetOpaque();                               // _E8, back to opaque drawing
	virtual Vec GetCameraTarget();                          // _EC, where a camera looks at this object
	virtual HSD_AnimJoint* GetJointAnimation(s32 id);       // _F0, joint anim
	virtual HSD_MatAnimJoint* GetMaterialAnimation(s32 id); // _F4, material anim
	virtual HSD_ShapeAnimJoint* GetShapeAnimation(s32 id);  // _F8, shape anim
	virtual u32 TestFloorCollision(const Vec* from, const Vec* to, f32 radius, f32 height, Vec* push, Vec* contact, s32 arg6,
	                               s32 arg7);                         // _FC, collision test
	virtual s32 OnPlayerContact(s32 unused, s32 value);               // _100, the player touched or used the object
	virtual BOOL TestFloorCrossing(const Vec& start, const Vec& end); // _104
	virtual f32 GetShadowRadius();                                    // _108, shadow radius
	virtual void ProjectOffsetToScreen(const Vec* offset, f32* x, f32* y, f32* z,
	                                   CCamera* camera); // _10C, a point near the object on screen
	virtual f32 GetFloorXZExtent();                      // _110
	virtual f32 GetCollisionHeight();                    // _114
	virtual BOOL ProjectVisibleBounds(f32* width, f32* height, CCamera* camera, f32 radius, f32 objectHeight); // _118
	virtual BOOL IsOutsideCameraDepth(CCamera* camera);           // _11C, TRUE when the model lies outside the camera depth range
	virtual void SetCullMode(s32 mode);                           // _120, GX cull mode for every polygon
	virtual s32 HandleEventCommand(s32 count, s32* args);         // _124
	virtual const void* GetEventScript();                         // _128, the event script the object starts when used, NULL if none
	virtual void SetColor(s32 blend, u8 r, u8 g, u8 b, f32 time); // _12C, steps material color 0 (modulate) or 1 (blend) to r,g,b over time
	virtual void OnPushDisplacement(const Vec& displacement);     // _130
	virtual BOOL ReceivePlugPower(f32 amount);                    // _134, power fed through the plug each frame; FALSE stops the feed
	virtual void OnHitFromPosition(const Vec& position, f32 strength);   // _138, hit from a position
	virtual void OnSquirterHit(const Vec* contact, s32 value, s32 tint); // _13C, the squirter spray hit the object
	virtual BOOL OnItemUsed(const Vec* position, s32 value);             // _140, a held tool touched the object; TRUE when it reacted
	virtual void OnMicrophonePeak(s32 unused, s32 volume);               // _144, the microphone heard a sound louder than 1000

	// todo: cleanup
	s32 GetKind() const { return mAttr.mPlacement != NULL ? mAttr.mPlacement->mObjectId : -1; }

	// which anim archive an anim id is in, 0 for the model's own
	s32 GetAnimSet(s32 id) const
	{
		bool ret = false;
		if (id >= 1000 && mAnim.mArchive != NULL) {
			ret = true;
		}
		return ret ? id / 1000 : 0;
	}

	BOOL IsFloorOn(int i) const { return mFloorMasks[0] & (1 << i); }
	void ClearFloor(int i) { mFloorMasks[0] &= ~(1 << i); }

	BOOL HasQueuedAnim() const
	{
		for (int i = 0; i < 16; i++) {
			if (mAnmQueue.mEntry[(mAnmQueue.mNext + i) % 16].mId != -1) {
				return TRUE;
			}
		}
		return FALSE;
	}

	// Adds an animation to the first free slot from mNext.
	void PushAnim(s32 anim, u16 frame, s16 rate)
	{
		for (int i = 0; i < 16; i++) {
			int index = (mAnmQueue.mNext + i) % 16;
			if (mAnmQueue.mEntry[index].mId == -1) {
				mAnmQueue.mEntry[index].mId    = anim;
				mAnmQueue.mEntry[index].mFrame = frame;
				mAnmQueue.mEntry[index].mRate  = rate;
				return;
			}
		}
	}

	// takes the next anim off the queue
	void PopAnim(s32& id, u16& frame, s16& rate)
	{
		for (int i = 0; i < 16; i++) {
			s32 j = (mAnmQueue.mNext + i) % 16;
			if (mAnmQueue.mEntry[j].mId != -1) {
				id                      = mAnmQueue.mEntry[j].mId;
				frame                   = mAnmQueue.mEntry[j].mFrame;
				rate                    = mAnmQueue.mEntry[j].mRate;
				mAnmQueue.mEntry[j].mId = -1;
				mAnmQueue.mNext         = (j + 1) % 16;
				return;
			}
		}
	}

	s32 GetPool() const { return mAttr.mObjData != NULL ? GetObjDataPool(mAttr.mObjData) : -1; }

	// the kind record's category and item, s16 like the record's fields
	static s16 GetDataCategory(const UnkObjData* data)
	{
		if (data != NULL) {
			return data->mCategory;
		}
		return 0;
	}
	static s16 GetDataItem(const UnkObjData* data)
	{
		if (data != NULL) {
			return data->mItemIndex;
		}
		return 0;
	}
	s16 GetCategory() const
	{
		if (mAttr.mObjData != NULL) {
			return GetDataCategory(mAttr.mObjData);
		}
		return -1;
	}
	s16 GetItem() const
	{
		if (mAttr.mObjData != NULL) {
			return GetDataItem(mAttr.mObjData);
		}
		return -1;
	}

	s32 GetObjFlags() const { return mAttr.mObjData != NULL ? GetObjDataFlags(mAttr.mObjData) : -1; }
};
#if defined(VERSION_JP)
typedef char XObj_size_check[sizeof(XObj) == 0x970 ? 1 : -1];
#else
typedef char XObj_size_check[sizeof(XObj) == 0x980 ? 1 : -1];
#endif

// the object that plays camera animations
class XObjCamera : public XObj {
public:
	CCamera mCamera; // _980
	s32 mCameraAnim; // _B14, -1 when no anim is playing

	XObjCamera();
	virtual void Release();
	virtual void InitializeObject(u32 id);
	virtual void ResetObjectState();
	virtual BOOL LoadFromPlacement(const CoordInfo& info);
	virtual void UpdateObject(u32 frames);
};

class CJArchive;
class CXObj;
class CStatus;

// the object base functions, the units from 0x8008A000 to 0x80093000
extern "C" {
// ~XObj, the subclass dtors call it with 0
XObj* __dt__4XObjFv(XObj* object, s32 flags);
BOOL LoadJointAnimationAsync(CJAnim* anim, const char* path);
// the default OnHitFromPosition
void OnHitFromPosition__4XObjFRC3Vecf(XObj* object, const Vec* position, f32 strength);
CVec GetFloorContactPosition(XObj* object, s32 floor, const Vec* offset);
// the default SetColor
f32 GetFloorFacingAngle(XObj* object, const Vec* pos, s32 index, s32 requireSegment, Vec* contact, Vec* normalOut);
void UpdateObjectFloorHeights(XObj* object);
void UpdateObjectFloorXZExtent(XObj* object);
void DrawGroundShadowAt__4XObjFPC3Vecf(XObj* object, const Vec* origin, f32 opacity);
void DrawGroundShadow__4XObjFv(XObj* object);
void DrawWithView__4XObjFPA4_f(XObj* object, f32 (*view)[4]);
void UpdateObject__4XObjFUl(XObj* object, u32 frames);
// walk callback, sets up each material's pe and toon flags
void ApplyJointMaterialPEPresets(HSD_JObj* joint, f32** arguments, s32 type);
// the default LoadFromPlacement, reads the object's coord info
BOOL LoadFromPlacement__4XObjFRC9CoordInfo(XObj* object, const CoordInfo* info);
void ResetObjectState__4XObjFv(XObj* object);
void InitializeObject__4XObjFUl(XObj* object, u32 id);

// the object kinds from 0x800D4000 to 0x80108000
void SyncMoveFromJoint(CXObj* object);
void UpdateSpiderCoinBounces(void* context, u32 frames);
void ResetSpiderCoinBounces(void* context);
void ResetSpiderBossState(XObj* object);
void ResetActorMovementStep(void* object);
s32 HandleEventCommand__9XObjActorFlPl(CXObj* object, s32 index, s32* out);
void SetActorLookAtPosition(CXObj* object, const Vec* target, f32 rate);
void SetActorLookAngle(CXObj* object, f32 value, f32 rate);
void SetActorLookTarget(CXObj* object, const Vec* target, f32 rate);
void ResetActorLook(CXObj* object, f32 rate);

// pools and counters the object kinds share
extern XObj* gSpiderCarryModels[16];
extern s32 gSpiderCarrySlotOwners[16];
extern XObj* gSmallSpiderEffectObjects[12];
extern XObj* lbl_8065B2A0;
extern XObj* lbl_8065B2AC;
extern CStatus lbl_8065B2B0;
extern s32 gSpiderDrainFlashTimers[2];
extern u8 gSpiderDrainFlashClaimed;
extern s32 gAttachedSmallSpiderCount;
extern XObj* lbl_8065B2DC;
extern u8 lbl_8065B2E0;
extern u32 gSpiderBossCoinBounceContext;
extern s32 gSpiderBossVolleySlot;
extern s32 gActor3Kind15ActivationCounter;
}

extern "C" void HSD_MtxGetTranslate(Mtx mat, Vec* vec);

// the jobj's place in the world
inline Vec GetJObjPos(HSD_JObj* jobj)
{
	Vec v;
	HSD_MtxGetTranslate(HSD_JObjGetMtxPtr(jobj), &v);
	return v;
}

// what an object kind's objTable entry starts with, the archive its model is read from
struct XObjSource {
	const char* mName;     // _00
	CObjArchive* mArchive; // _04
	u32 _08;
	struct Cache {
		u32 address;
		u32 size;
	} mCache;
};

extern "C" XObjSource gObjectSources[];
extern "C" XObjSource* GetObjectSources();

#endif
