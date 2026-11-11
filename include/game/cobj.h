#ifndef _GAME_COBJ_H
#define _GAME_COBJ_H

#include "Version.h"
#include "game/cjobj.h"
#include "game/cfile.h"
extern "C" {
#include <sysdolphin/archive.h>
#include <sysdolphin/mobj.h>
}

// class names from RTTI, ReadAsync/ReadCheck/Init from halt strings.
// offsets are usa, jp's CJArchive is 8 bytes shorter so CXObj fields after mArchive shift

// HSD archive plus the CFile it's read through, refcounted between CJArchives
class CObjArchive : public HSD_Archive {
public:
	// _00 HSD_Archive
	CFile mFile;   // _44
	s32 mRefCount; // _130
	// _134 vtable

	CObjArchive();
	virtual ~CObjArchive() { } // _08 (weak)

	// inlined:
	s32 Parse();
};
typedef char CObjArchive_size_check[sizeof(CObjArchive) == 0x138 ? 1 : -1];

// what the archive's public scene symbol points at
struct CJArchiveModel {
	HSD_Joint* mJoint;                    // _00
	HSD_AnimJoint** mAnimJoint;           // _04
	HSD_MatAnimJoint** mMatAnimJoint;     // _08
	HSD_ShapeAnimJoint** mShapeAnimJoint; // _0C
};

struct CJArchiveDesc {
	void* mObjectDesc;  // _00
	void** mAnimations; // _04, null terminated
};

struct CJArchiveScene {
	CJArchiveModel** mModels; // _00
	CJArchiveDesc* mCamera;   // _04
	CJArchiveDesc** mLights;  // _08, 8 of them
	CJArchiveDesc* _0C;       // _0C
};

// japan has one desc list fewer, the last one, which the usa SetupScene fills in wrong
#if defined(VERSION_JP)
#define CJARCHIVE_DESC_NUM 9
#else
#define CJARCHIVE_DESC_NUM 10
#endif

// a step machine that reads one CObjArchive in, then the model and anims out of it
class CJArchive {
public:
	CObjArchive* mArchive;                    // _00
	s32 mState;                               // _04
	u32 mFlags;                               // _08
	s32 mResult;                              // _0C
	u32 mAramAddr;                            // _10
	u32 mAramSize;                            // _14
	HSD_Joint* mJoint;                        // _18
	HSD_AnimJoint** mAnimJoint;               // _1C
	HSD_MatAnimJoint** mMatAnimJoint;         // _20
	HSD_ShapeAnimJoint** mShapeAnimJoint;     // _24
	CJArchiveDesc* mDesc[CJARCHIVE_DESC_NUM]; // _28
	s32 mNum[CJARCHIVE_DESC_NUM + 1];         // _50, the anim count, then one per mDesc list
	// _7C vtable

	virtual ~CJArchive() { }                                                                               // _08 (weak)
	virtual void Clear();                                                                                  // _0C
	virtual BOOL ReadAsync(CObjArchive* archive, const char* path, u32 flags, u32 aramAddr, u32 aramSize); // _10
	virtual BOOL ReadCheck();                                                                              // _14
	virtual void Release();                                                                                // _18

	void SetupScene(s32 model, s32 sceneIndex);
	HSD_AnimJoint* GetAnimJoint(s32 id, s32 depth, const s32* path);
	HSD_MatAnimJoint* GetMatAnimJoint(s32 id, s32 depth, const s32* path);
	HSD_ShapeAnimJoint* GetShapeAnimJoint(s32 id, s32 depth, const s32* path);

	// inlined:
	CJArchiveScene* GetScene(s32 index);
	HSD_JObj* LoadJObj();

	// anim ids past 1000 wrap around
	HSD_AnimJoint* GetAnimJointRoot(s32 id)
	{
		if (id >= 0) {
			s32 i = id % 1000;
			if (i < mNum[0]) {
				return mAnimJoint != NULL ? mAnimJoint[i] : NULL;
			}
		}
		return NULL;
	}

	HSD_MatAnimJoint* GetMatAnimJointRoot(s32 id)
	{
		if (id >= 0) {
			s32 i = id % 1000;
			if (i < mNum[0]) {
				return mMatAnimJoint != NULL ? mMatAnimJoint[i] : NULL;
			}
		}
		return NULL;
	}

	HSD_ShapeAnimJoint* GetShapeAnimJointRoot(s32 id)
	{
		if (id >= 0) {
			s32 i = id % 1000;
			if (i < mNum[0]) {
				return mShapeAnimJoint != NULL ? mShapeAnimJoint[i] : NULL;
			}
		}
		return NULL;
	}
};
#if defined(VERSION_JP)
typedef char CJArchive_size_check[sizeof(CJArchive) == 0x78 ? 1 : -1];
#else
typedef char CJArchive_size_check[sizeof(CJArchive) == 0x80 ? 1 : -1];
#endif

// a CJArchive for an object's second set of anims
class CJAnim : public CJArchive {
public:
	// _00-_80 = CJArchive
	s32 mLoading; // _80, LoadJointAnimationAsync's step

	virtual ~CJAnim() { }   // _08, inline, retail keeps a weak copy
	virtual void Clear();   // _0C
	virtual void Release(); // _18

	// anims past 1000 come from this archive instead of the model's
	void* GetAnimJoint(s32 id) { return mAnimJoint != NULL ? mAnimJoint[id % 1000] : NULL; }
	void* GetMatAnimJoint(s32 id) { return mMatAnimJoint != NULL ? mMatAnimJoint[id % 1000] : NULL; }
	void* GetShapeAnimJoint(s32 id) { return mShapeAnimJoint != NULL ? mShapeAnimJoint[id % 1000] : NULL; }
};

// a value with a target and a per frame speed to step it there
class CObjVec : public CVec {
public:
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

	// inlined:
	bool IsMoving() const;
};
typedef char CObjVec_size_check[sizeof(CObjVec) == 0x30 ? 1 : -1];

// a value waiting to be put on a jobj after the next anim step
struct CXObjQueueEntry {
	HSD_JObj* mJObj; // _00
	u32 mType;       // _04, which value, 1 to 0x100, or 0x200 and 0x400 for anim rates
	f32 mValue;      // _08
};

// the game's object, a CJObj with an archive, stepped transforms and a queue of joint values
class CXObj : public CJObj {
public:
	// _00 CJObj, vtable at _34
	u32 mId;                    // _38
	s32 mLoadState;             // _3C, 1 while the archive is being read
	u32 mModel;                 // _40
	u32 mLoadFlags;             // _44
	CJArchive mArchive;         // _48
	CObjVec mMoveTranslate;     // _C8
	CObjVec mMoveRotate;        // _F8
	CObjVec mMoveScale;         // _128
	s32 mAnimId;                // _158
	f32 mAnimRate;              // _15C
	f32 mLastAnimRate;          // _160
	u32 mColors[2];             // _164, the two colors the x_cobj materials blend in
	s32 mQueued;                // _16C
	CXObjQueueEntry mQueue[10]; // _170

	CXObj()
	{
		mMoveTranslate.Reset(0.0f, 0.0f, 0.0f);
		mMoveRotate.Reset(0.0f, 0.0f, 0.0f);
		mMoveScale.Reset(0.0f, 0.0f, 0.0f);
	}

	explicit CXObj(HSD_JObj* jobj);
	virtual ~CXObj() { } // _08

	// CJObj overrides, defined in another file
	virtual void SetTranslateX(f32 x, s32 force);                                                                     // _0C
	virtual void SetTranslateY(f32 y, s32 force);                                                                     // _10
	virtual void SetTranslateZ(f32 z, s32 force);                                                                     // _14
	virtual void SetScaleX(f32 x, s32 force);                                                                         // _18
	virtual void SetScaleY(f32 y, s32 force);                                                                         // _1C
	virtual void SetScaleZ(f32 z, s32 force);                                                                         // _20
	virtual void SetRotationX(f32 x, s32 force);                                                                      // _24
	virtual void SetRotationY(f32 y, s32 force);                                                                      // _28
	virtual void SetRotationZ(f32 z, s32 force);                                                                      // _2C
	virtual void SetScale(const Vec& v, s32 force);                                                                   // _38
	virtual void SetScale(f32 x, f32 y, f32 z, s32 force);                                                            // _3C
	virtual void SetAnimRate(f32 rate, u32 mask);                                                                     // _48
	virtual BOOL ReadAsync(CObjArchive* archive, const char* path, u32 model, u32 flags, u32 aramAddr, u32 aramSize); // _4C
	virtual BOOL ReadCheck();                                                                                         // _50
	virtual void Release();                                                                                           // _54
	virtual BOOL StepRotate(u32 frames);                                                                              // _58
	virtual BOOL StepTranslate(u32 frames);                                                                           // _5C
	virtual BOOL StepScale(u32 frames);                                                                               // _60
	virtual void SetPos(const Vec& pos, s32 force);                                                                   // _64
	virtual void SetPos(f32 x, f32 y, f32 z, s32 force);                                                              // _68
	virtual void SetScale(f32 scale, s32 force);                                                                      // _6C
	virtual void SetRot(const Vec& rot, s32 force);                                                                   // _70
	virtual void SetRot(f32 x, f32 y, f32 z, s32 force);                                                              // _74
	virtual void SetMoveDir(const Vec* dir, f32 speed);                                                               // _78
	virtual void SetRotateY(f32 target, f32 speed);                                                                   // _7C
	virtual void SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend,
	                     f32 blendRate); // _80
	virtual void ClearAnim();            // _84
	virtual void UpdateAnim(u32 speed);  // _88
	virtual void ApplyTransform();       // _8C

	// todo: cleanup
	void SetMoveDir(const Vec& dir, f32 speed) { SetMoveDir(&dir, speed); }

	void Init(u32 id);
	BOOL Load(CObjArchive* archive, const char* path, u32 model, u32 flags, u32 aramAddr, u32 aramSize);
	void Push(HSD_JObj* jobj, u32 type, f32 value);
	void Attach(HSD_JObj* jobj);
	f32 GetMObjAlpha(HSD_MObj* mobj);

	// inlined:
	HSD_JObj* FindJoint(s32 depth, s32* path);
	void ClearQueue();
};

#if defined(VERSION_JP)
typedef char CXObj_size_check[sizeof(CXObj) == 0x1E0 ? 1 : -1];
#else
typedef char CXObj_size_check[sizeof(CXObj) == 0x1E8 ? 1 : -1];
#endif

// a CXObj with a second jobj it blends into
class CXObjIntp : public CXObj {
public:
	CJObj mIntp;    // _1E8
	f32 mBlend;     // _220, below 1 while the two jobjs are being blended
	f32 mBlendRate; // _224

	virtual BOOL ReadCheck();               // _50
	virtual void Release();                 // _54
	virtual BOOL StepRotate(u32 frames);    // _58
	virtual BOOL StepTranslate(u32 frames); // _5C
	virtual BOOL StepScale(u32 frames);     // _60
	virtual void SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend,
	                     f32 blendRate); // _80
	virtual void UpdateAnim(u32 speed);  // _88
	virtual void ApplyTransform();       // _8C
};
#if defined(VERSION_JP)
typedef char CXObjIntp_size_check[sizeof(CXObjIntp) == 0x220 ? 1 : -1];
#else
typedef char CXObjIntp_size_check[sizeof(CXObjIntp) == 0x228 ? 1 : -1];
#endif

// turns one joint of a CXObj towards an angle, or towards another CXObj
class CJointRot {
public:
	s16 mMode;        // _00, 0 idle, 2 runs once, other values flip sign when the turn ends
	u16 mAxis;        // _02
	f32 mAngle;       // _04
	f32 mTarget;      // _08
	f32 mSpeed;       // _0C
	CXObj* mObj;      // _10
	HSD_JObj* mJoint; // _14
	CXObj* mLookAt;   // _18
	// _1C vtable

	virtual ~CJointRot() { } // _08 (weak)

	bool Update(u32 frames);
	void Init(CXObj* obj, s32 depth, s32* path, char axis);
};
typedef char CJointRot_size_check[sizeof(CJointRot) == 0x20 ? 1 : -1];

#endif
