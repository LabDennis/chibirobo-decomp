#ifndef _GAME_CCAMERAACTION_H
#define _GAME_CCAMERAACTION_H

#include "Version.h"
#include "game/xobj.h"
#include "types.h"
#include "game/video.h"
#include "game/scrollroomdata.h"

// pal uses the current refresh rate, 50 or 60
#if defined(VERSION_PAL)
#define FRAME_TIME (1.0f / gVideoFramesPerSecond)
#else
#define FRAME_TIME (1.0f / 60.0f)
#endif

// class names from RTTI, the rest is guessed

// moves a vector from one value to another over time
class CInterfaceVectorInterpolation {
public:
	// _00 vtable

	virtual ~CInterfaceVectorInterpolation() { }        // _08
	virtual bool Start(Vec from, Vec to, f32 time) = 0; // _0C
	virtual bool Step(f32 dt)                      = 0; // _10, TRUE once it's done
	virtual bool Update(u32 step, f32 dt)          = 0; // _14, TRUE once it's done
	virtual Vec Get()                              = 0; // _18
};

// the same for one float
class CInterfaceInterpolation {
public:
	// _00 vtable

	virtual ~CInterfaceInterpolation() { }              // _08
	virtual bool Start(f32 from, f32 to, f32 time) = 0; // _0C
	virtual bool Step(f32 dt)                      = 0; // _10
	virtual bool Update(u32 step, f32 dt)          = 0; // _14
	virtual f32 Get()                              = 0; // _18
};

// constant speed
class CInterpolationLinear : public CInterfaceInterpolation {
public:
	CInterpolationLinear() { }
	virtual ~CInterpolationLinear() { }             // _08
	virtual bool Start(f32 from, f32 to, f32 time); // _0C
	virtual bool Step(f32 dt);                      // _10
	virtual bool Update(u32 step, f32 dt);          // _14
	virtual f32 Get();                              // _18

	// _00 vtable
	f32 mValue; // _04
	f32 mSpeed; // _08
	f32 mTime;  // _0C, time left
};
STATIC_ASSERT(sizeof(CInterpolationLinear) == 0x10);

// jumps straight to the end
class CInterpolationNone : public CInterfaceInterpolation {
public:
	CInterpolationNone() { }
	virtual ~CInterpolationNone() { }               // _08
	virtual bool Start(f32 from, f32 to, f32 time); // _0C
	virtual bool Step(f32 dt);                      // _10
	virtual bool Update(u32 step, f32 dt);          // _14
	virtual f32 Get();                              // _18

	// _00 vtable
	f32 mValue; // _04
	f32 mSpeed; // _08
	f32 mTime;  // _0C
};
STATIC_ASSERT(sizeof(CInterpolationNone) == 0x10);

class CInterpolationEaseInOut : public CInterfaceInterpolation {
public:
	CInterpolationEaseInOut() { }
	virtual ~CInterpolationEaseInOut() { }          // _08
	virtual bool Start(f32 from, f32 to, f32 time); // _0C
	virtual bool Step(f32 dt);                      // _10
	virtual bool Update(u32 step, f32 dt);          // _14
	virtual f32 Get();                              // _18

	// _00 vtable
	f32 mValue;    // _04
	f32 mTarget;   // _08
	f32 mTime;     // _0C, time left
	f32 mDuration; // _10
	f32 mSpeed;    // _14
	f32 mAccel;    // _18
};
STATIC_ASSERT(sizeof(CInterpolationEaseInOut) == 0x1C);

class CInterpolationEaseOut : public CInterfaceInterpolation {
public:
	CInterpolationEaseOut() { }
	virtual ~CInterpolationEaseOut() { }            // _08
	virtual bool Start(f32 from, f32 to, f32 time); // _0C
	virtual bool Step(f32 dt);                      // _10
	virtual bool Update(u32 step, f32 dt);          // _14
	virtual f32 Get();                              // _18

	// _00 vtable
	f32 mValue;    // _04
	f32 mTarget;   // _08
	f32 mTime;     // _0C, time left
	f32 mDuration; // _10
	f32 mSpeed;    // _14
	f32 mAccel;    // _18
	f32 mStart;    // _1C
};
STATIC_ASSERT(sizeof(CInterpolationEaseOut) == 0x20);

// moves 1/divisor of the way there every step, the dt is ignored
class CInterpolationEaseOutDivide : public CInterfaceInterpolation {
public:
	CInterpolationEaseOutDivide() { }
	virtual ~CInterpolationEaseOutDivide() { }         // _08
	virtual bool Start(f32 from, f32 to, f32 divisor); // _0C
	virtual bool Step(f32 dt);                         // _10, done once it moves less than 0.001
	virtual bool Update(u32 step, f32 dt);             // _14
	virtual f32 Get();                                 // _18

	// _00 vtable
	f32 mValue;   // _04
	f32 mTarget;  // _08
	f32 mDivisor; // _0C
};
STATIC_ASSERT(sizeof(CInterpolationEaseOutDivide) == 0x10);

// the vector ones run one scalar interpolation per axis
class CVectorLinearInterpolation : public CInterfaceVectorInterpolation {
public:
	virtual ~CVectorLinearInterpolation() { }       // _08
	virtual bool Start(Vec from, Vec to, f32 time); // _0C
	virtual bool Step(f32 dt);                      // _10
	virtual bool Update(u32 step, f32 dt);          // _14
	virtual Vec Get();                              // _18

	// _00 vtable
	CInterpolationLinear mAxes[3]; // _04
};
STATIC_ASSERT(sizeof(CVectorLinearInterpolation) == 0x34);

class CVectorEaseInOutInterpolation : public CInterfaceVectorInterpolation {
public:
	virtual ~CVectorEaseInOutInterpolation() { }    // _08
	virtual bool Start(Vec from, Vec to, f32 time); // _0C
	virtual bool Step(f32 dt);                      // _10
	virtual bool Update(u32 step, f32 dt);          // _14
	virtual Vec Get();                              // _18

	// _00 vtable
	CInterpolationEaseInOut mAxes[3]; // _04
	BOOL mXZDone;                     // _58, set once x and z are there
};
STATIC_ASSERT(sizeof(CVectorEaseInOutInterpolation) == 0x5C);

class CVectorEaseOutInterpolation : public CInterfaceVectorInterpolation {
public:
	virtual ~CVectorEaseOutInterpolation() { }      // _08
	virtual bool Start(Vec from, Vec to, f32 time); // _0C
	virtual bool Step(f32 dt);                      // _10
	virtual bool Update(u32 step, f32 dt);          // _14
	virtual Vec Get();                              // _18

	// _00 vtable
	CInterpolationEaseOut mAxes[3]; // _04
};
STATIC_ASSERT(sizeof(CVectorEaseOutInterpolation) == 0x64);

class CVectorNoneInterpolation : public CInterfaceVectorInterpolation {
public:
	virtual ~CVectorNoneInterpolation() { }         // _08
	virtual bool Start(Vec from, Vec to, f32 time); // _0C
	virtual bool Step(f32 dt);                      // _10
	virtual bool Update(u32 step, f32 dt);          // _14
	virtual Vec Get();                              // _18

	// _00 vtable
	CInterpolationNone mAxes[3]; // _04
};
STATIC_ASSERT(sizeof(CVectorNoneInterpolation) == 0x34);

class CVectorEaseOutDivideInterpolation : public CInterfaceVectorInterpolation {
public:
	virtual ~CVectorEaseOutDivideInterpolation() { }   // _08
	virtual bool Start(Vec from, Vec to, f32 divisor); // _0C
	virtual bool Step(f32 dt);                         // _10
	virtual bool Update(u32 step, f32 dt);             // _14
	virtual Vec Get();                                 // _18

	// _00 vtable
	CInterpolationEaseOutDivide mAxes[3]; // _04
	BOOL mXZDone;                         // _34, set once x and z are there
};
STATIC_ASSERT(sizeof(CVectorEaseOutDivideInterpolation) == 0x38);

class CInterfaceCameraAction {
public:
	CVec mEye;      // _00
	CVec mInterest; // _10
	CVec mAngle;    // _20
	f32 mDistance;  // _30
	f32 mFov;       // _34
	u32 _38;        // _38
	// _3C vtable

	virtual ~CInterfaceCameraAction() { }       // _08
	virtual void PreviousUpdate() { }           // _0C
	virtual void Update(u32 step) = 0;          // _10
	virtual void Reset() { }                    // _14
	virtual void SetCamera(CCamera* camera) { } // _18, takes eye, interest, angle, distance and fov
	virtual void Init() { }                     // _1C
	virtual void vf20() { }                     // _20
	virtual void vf24() { }                     // _24
	virtual void vf28() { }                     // _28
	virtual void SetEye(f32 x, f32 y, f32 z)    // _2C
	{
		mEye.x = x;
		mEye.y = y;
		mEye.z = z;
	}
	virtual void SetInterest(f32 x, f32 y, f32 z) // _30
	{
		mInterest.x = x;
		mInterest.y = y;
		mInterest.z = z;
	}
	virtual void SetAngle(f32 x, f32 y, f32 z) // _34
	{
		mAngle.x = x;
		mAngle.y = y;
		mAngle.z = z;
	}
	virtual void SetEye(const Vec& eye) { SetEye(eye.x, eye.y, eye.z); }                               // _38
	virtual void SetInterest(const Vec& interest) { SetInterest(interest.x, interest.y, interest.z); } // _3C
	virtual void SetAngle(const Vec& angle) { SetAngle(angle.x, angle.y, angle.z); }                   // _40
	virtual void SetDistance(f32 distance) { mDistance = distance; }                                   // _44
	virtual void SetFov(f32 fov) { mFov = fov; }                                                       // _48
	virtual void SetAngleZ(f32 z) { mAngle.z = z; }                                                    // _4C
	virtual u32 vf50() { return 0; }                                                                   // _50
	virtual u32 GetCameraFlags() { return _38; }                                                       // _54
	virtual f32 GetFollowDivisor();                                                                    // _58
	virtual u32 vf5C() { return 0; }                                                                   // _5C
	virtual XObj* GetTarget() { return NULL; }                                                         // _60

	// set the mode word along with the vector
	void SetEye(const Vec& eye, u32 mode)
	{
		_38 = mode;
		SetEye(eye);
	}
	void SetInterest(const Vec& interest, u32 mode)
	{
		_38 = mode;
		SetInterest(interest);
	}
};

// base of the area, map and reset actions. fields from _40 on differ per subclass
class CCameraActionAreaExM : public CInterfaceCameraAction {
public:
	virtual void vf64();          // _64
	virtual f32 vf68();           // _68
	virtual s32 vf6C();           // _6C
	virtual void vf70(s32 mode);  // _70
	virtual void vf74(Vec pos);   // _74
	virtual BOOL vf78(s32 id);    // _78, whether an area camera entry exists
	virtual BOOL vf7C(s32 index); // _7C
};

// an entry in CCameraActionMap's _570 and _9A0 lists
struct CamMapEntry {
	Vec _00; // _00, a position
	s32 _0C; // _0C
	s32 _10; // _10
	Vec _14; // _14
};

// the map action, CCameraActions holds it at _99C
class CCameraActionMap : public CCameraActionAreaExM {
public:
	s32 _40;                                           // _40
	XObj* mTarget;                                     // _44, the object the camera looks at
	CVec mPreviousInterest;                            // _48
	f32 _58;                                           // _58
	f32 _5C;                                           // _5C
	CVec _60;                                          // _60
	CVec _70;                                          // _70
	f32 _80;                                           // _80
	f32 _84;                                           // _84
	f32 _88;                                           // _88
	CObjVec _8C;                                       // _8C
	f32 _BC;                                           // _BC, turn speed
	f32 _C0;                                           // _C0, wrapped target yaw
	f32 _C4;                                           // _C4
	CInterpolationEaseOutDivide _C8;                   // _C8
	CVec mPreviousEye;                                 // _D8
	u8 _E8[0x4EC - 0xE8];                              // _E8
	s32 _4EC;                                          // _4EC
	s32 _4F0;                                          // _4F0
	s32 _4F4;                                          // _4F4
	s32 _4F8;                                          // _4F8
	u8 _4FC[0x570 - 0x4FC];                            // _4FC
	CamMapEntry mIntersectionEntries[32];              // _570
	s32 mIntersectionCount;                            // _970
	CamMapEntry* _974;                                 // _974
	s32 _978;                                          // _978
	CamMapEntry* _97C;                                 // _97C
	s32 _980;                                          // _980
	Vec _984;                                          // _984, one end of the picked side
	Vec _990;                                          // _990, the other end
	s32 _99C;                                          // _99C, 1 or -1, which way round they are
	CamMapEntry _9A0[32];                              // _9A0
	s32 _DA0;                                          // _DA0, count of _9A0
	s32 _DA4;                                          // _DA4
	s32 _DA8;                                          // _DA8
	s32 _DAC;                                          // _DAC
	s32 _DB0;                                          // _DB0
	CInterfaceVectorInterpolation* mTurnInterpolation; // _DB4
	CVectorEaseInOutInterpolation _DB8;                // _DB8
	CInterpolationEaseInOut _E14;                      // _E14
	CInterfaceInterpolation* _E30;                     // _E30, camera height
	CInterpolationEaseInOut _E34;                      // _E34
	CInterfaceInterpolation* _E50;                     // _E50
	f32 _E54;                                          // _E54
	s32 _E58;                                          // _E58
	s32 _E5C;                                          // _E5C
	f32 _E60;                                          // _E60, current camera height
	CInterfaceInterpolation* _E64;                     // _E64
	CInterpolationEaseOutDivide _E68;                  // _E68
	CInterfaceInterpolation* _E78;                     // _E78
	CInterpolationEaseInOut _E7C;                      // _E7C
	f32 _E98;                                          // _E98
	s32 _E9C;                                          // _E9C
	s32 _EA0;                                          // _EA0, manual camera input this frame
	s32 _EA4;                                          // _EA4, area IDs written to _EA8
	s32 _EA8[32];                                      // _EA8, areas containing the player
	CInterfaceVectorInterpolation* _F28;               // _F28
	CVectorEaseOutDivideInterpolation _F2C;            // _F2C
	f32 _F64;                                          // _F64, target-eye interpolation time
	CVec _F68;                                         // _F68
	CVec _F78;                                         // _F78
	CVec _F88;                                         // _F88
	CVec _F98;                                         // _F98
	CVec _FA8;                                         // _FA8
	CVec _FB8;                                         // _FB8
	u8 _FC8[0x11D8 - 0xFC8];                           // _FC8
	CStatus mStatus;                                   // _11D8, printed as CAMSTAT
	CStatus mPreviousStatus;                           // _11E0, printed as CAMSTATPRE
	BOOL mKeepPreviousEye;                             // _11E8

	CCameraActionMap();                      // in the dol, 0x8001A45C
	virtual ~CCameraActionMap();             // in the dol, 0x80009F20
	virtual void PreviousUpdate();           // _0C
	virtual void Update(u32 step);           // _10
	virtual void Reset();                    // _14
	virtual void SetCamera(CCamera* camera); // _18
	virtual void Init();                     // _1C
	virtual void vf24();                     // _24
	virtual void vf28();                     // _28
	virtual u32 GetCameraFlags();            // _54
	virtual f32 GetFollowDivisor();          // _58
	virtual u32 vf5C();                      // _5C
	virtual XObj* GetTarget();               // _60
};
STATIC_ASSERT(sizeof(CCameraActionMap) == 0x11EC);

// an entry in a CamAreaSlot list
struct CamAreaEntry {
	CVec _00; // _00, interest
	CVec _10; // _10, camera position
	CVec _20; // _20, an angle
	f32 _30;  // _30, field of view
	s32 _34;  // _34, entry ID
};
STATIC_ASSERT(sizeof(CamAreaEntry) == 0x38);

// a record in CamAreaState's _1C and _24 lists
struct CamAreaRecord {
	s32 _00;       // _00, -1 terminates the table
	s32 _04[4][2]; // _04, 4 corners as x z pairs, in hundredths
	s32 mTop;      // _24, upper height in hundredths
	s32 mBottom;   // _28, lower height in hundredths
	s16 _2C;       // _2C
	u16 _2E;       // _2E, flags. 4 marks a camera area, events toggle 0x80 to turn it off
	u8 _30[0x4];   // _30

	f32 GetTop() const { return mTop / 100.0f; }
	f32 GetBottom() const { return mBottom / 100.0f; }
};

// wraps a CamAreaRecord, name from RTTI
class CamAreaInfo {
public:
	CamAreaInfo(CamAreaRecord* record)
	    : _04(record)
	{
	}
	virtual ~CamAreaInfo() { } // _08

	CamAreaRecord* GetRecord() const { return _04; }
	bool Is_2C_1() const { return _04->_2C == 1; }
	bool IsCameraOff() const { return !(_04->_2E & 4); }
	bool IsCameraArea() const
	{
		u16 flags = _04->_2E & 4;
		return flags != 0;
	}
	u16 IsDisabled() const { return _04->_2E & 0x80; }
	f32 GetTop() const { return _04->mTop / 100.0f; }
	f32 GetBottom() const { return _04->mBottom / 100.0f; }

	// _00 vtable
	CamAreaRecord* _04; // _04
};

struct CamAreaSlot {
	CamAreaEntry* _00; // _00
	s32 _04;           // _04, group ID
	s32 _08;           // _08, entries in _00
	CVec _0C;          // _0C, representative camera position

	CamAreaSlot();
	~CamAreaSlot();
};
STATIC_ASSERT(sizeof(CamAreaSlot) == 0x1C);

struct CamInfoData;

// the area camera's state, name from RTTI, the vtable at 0x8022E554 and the DrawDebug3d strings
class CCameraActionAreaEx {
public:
	virtual ~CCameraActionAreaEx(); // _08

	// _00 vtable
	s32 _04;                 // _04
	s32 _08;                 // _08
	s32 _0C;                 // _0C
	f32 mBaseFov;            // _10
	s32 _14;                 // _14
	s32 _18;                 // _18
	CamAreaRecord* _1C;      // _1C
	s32 _20;                 // _20, records in _1C
	CamAreaRecord* _24;      // _24
	s32 _28;                 // _28, records in _24
	CamInfoData* _2C;        // _2C, stage camera placements
	CamAreaEntry* _30[2048]; // _30, entries in the shared placement pool
	s32 _2030;               // _2030, number of camera groups
	CamAreaSlot _2034[256];  // _2034, count unknown, 256 fills out the 0x3C34 object
};
STATIC_ASSERT(sizeof(CCameraActionAreaEx) == 0x3C34);
// todo: old name, the prototypes below still use it
typedef CCameraActionAreaEx CamAreaState;
extern "C" CCameraActionAreaEx gCameraAreaState;

// the area action, CCameraActions holds it at _40
class CCameraActionArea : public CCameraActionAreaExM {
public:
	f32 mTransitionTime;                                 // _40, transition time
	s32 mDirectionMode;                                  // _44, camera direction mode
	BOOL mUserSelected;                                  // _48, user-selected entry
	s32 mEntryIndex;                                     // _4C
	s32 mPrevEntryIndex;                                 // _50
	CamAreaEntry* mEntry;                                // _54
	CamAreaEntry* mPrevEntry;                            // _58
	s32 mControlEnabled;                                 // _5C
	BOOL mManualControl;                                 // _60, manual camera changes enabled
	XObj* mTarget;                                       // _64
	CInterfaceVectorInterpolation* mEyeInterp;           // _68
	CInterfaceInterpolation* mFovInterp;                 // _6C
	CVectorEaseInOutInterpolation mInterestEaseInOut;    // _70
	CVectorLinearInterpolation mEyeLinear;               // _CC
	CVectorEaseInOutInterpolation mEyeEaseInOut;         // _100
	CVectorEaseOutInterpolation mEyeEaseOut;             // _15C
	CVectorEaseOutDivideInterpolation mEyeEaseOutDivide; // _1C0
	CInterfaceVectorInterpolation* mEyeInterps[5];       // _1F8
	CInterpolationLinear mFovLinear;                     // _20C
	CInterpolationEaseInOut mFovEaseInOut;               // _21C
	CInterpolationEaseOut mFovEaseOut;                   // _238
	CInterpolationEaseOutDivide mFovEaseOutDivide;       // _258
	CInterfaceInterpolation* mFovInterps[5];             // _268
	CInterfaceVectorInterpolation* mInterestInterp;      // _27C
	CInterfaceVectorInterpolation* mInterestInterps[5];  // _280, by mode

	virtual void PreviousUpdate();           // _0C
	virtual void Update(u32 step);           // _10
	virtual void Reset();                    // _14
	virtual void SetCamera(CCamera* camera); // _18
	virtual void Init();                     // _1C
	virtual void vf24();                     // _24
	virtual void vf28();                     // _28
	virtual u32 GetCameraFlags();            // _54
	virtual u32 vf5C();                      // _5C
	virtual XObj* GetTarget();               // _60

	void SetEyeInterpolation(s32 mode, Vec from, Vec to, f32 time)
	{
		if (mode == 5) {
			mode = 4;
		}
		Vec target = to;
		if (mode == 3 || time <= 0.0f) {
			mEyeInterp = NULL;
			SetEye(target);
		} else {
			mEyeInterp = mEyeInterps[mode];
			if (mEyeInterp != NULL) {
				mEyeInterp->Start(from, target, time);
			}
		}
	}

	void SetEye_I(s32 mode, Vec from, Vec to, f32 time);

	void SetFovInterpolation(s32 mode, f32 from, f32 to, f32 time)
	{
		if (mode == 5) {
			mode = 4;
		}
		if (mode == 3 || time <= 0.0f) {
			mFovInterp = NULL;
			SetFov(to);
		} else {
			mFovInterp = mFovInterps[mode];
			if (mFovInterp != NULL) {
				mFovInterp->Start(from, to, time);
			}
		}
	}

	// the vectors are passed by value, hence the copies
	void SetInte_I(s32 mode, Vec from, Vec to, f32 time)
	{
		if (mode == 3 || time <= 0.0f) {
			mInterestInterp = NULL;
			SetInterest(to);
			return;
		}
		mInterestInterp = mInterestInterps[mode];
		if (mInterestInterp != NULL) {
			mInterestInterp->Start(from, to, time);
		}
	}
};
STATIC_ASSERT(sizeof(CCameraActionArea) == 0x294);

// puts the camera back behind the player, see ccameraactionreset.cpp
class CCameraActionReset : public CCameraActionAreaExM {
public:
	XObj* mTarget;                                 // _40
	s32 mMode;                                     // _44, picks the interpolators, 3 is none
	f32 mTime;                                     // _48
	BOOL mImmediate;                               // _4C
	s32 mNextCameraAction;                         // _50
	s32 mState;                                    // _54
	s32 mInterestMode;                             // _58
	CInterfaceVectorInterpolation* mEyeInterp;     // _5C
	CInterfaceInterpolation* mFovInterp;           // _60
	CVectorLinearInterpolation mEyeLinear;         // _64
	CVectorEaseInOutInterpolation mEyeEaseInOut;   // _98
	CVectorEaseOutInterpolation mEyeEaseOut;       // _F4
	CVectorNoneInterpolation mEyeNone;             // _158
	CInterfaceVectorInterpolation* mEyeInterps[4]; // _18C, by mode
	CInterpolationLinear mFovLinear;               // _19C
	CInterpolationEaseInOut mFovEaseInOut;         // _1AC
	CInterpolationEaseOut mFovEaseOut;             // _1C8
	CInterpolationNone mFovNone;                   // _1E8
	CInterfaceInterpolation* mFovInterps[4];       // _1F8

	virtual void PreviousUpdate();           // _0C
	virtual void Update(u32 step);           // _10
	virtual void SetCamera(CCamera* camera); // _18
	virtual void Init();                     // _1C
	virtual f32 GetFollowDivisor();          // _58
	virtual u32 vf5C();                      // _5C
	virtual XObj* GetTarget();               // _60

	void SetEye_Interp(s32 mode, const Vec& eye, f32 time)
	{
		if (mode == 3 || time <= 0.0f) {
			mEyeInterp = NULL;
			SetEye(eye);
			return;
		}
		mEyeInterp = mEyeInterps[mode];
		if (mEyeInterp != NULL) {
			mEyeInterp->Start(mEye, eye, time);
		}
	}
	void SetFov_Interp(s32 mode, f32 fov, f32 time)
	{
		if (mode == 3 || time <= 0.0f) {
			mFovInterp = NULL;
			SetFov(fov);
			return;
		}
		mFovInterp = mFovInterps[mode];
		if (mFovInterp != NULL) {
			mFovInterp->Start(mFov, fov, time);
		}
	}
};
STATIC_ASSERT(sizeof(CCameraActionReset) == 0x208);

// a camera that looks along the z axis at the player from a fixed distance, the dpad or
// c stick swaps between a near and a far one. see ccameraactionscroll.cpp
class CCameraActionScroll : public CInterfaceCameraAction {
public:
	BOOL mStart;                               // _40, the next update places the camera from scratch
	f32 mFarDistance;                          // _44
	f32 mNearDistance;                         // _48
	f32 mScrollFov;                            // _4C
	CInterfaceInterpolation* mDistanceInterp;  // _50
	CInterpolationEaseOutDivide mDistanceEase; // _54
	u32 mInterestMode;                         // _64
	s32 mZoom;                                 // _68, 0 near, 1 far

	virtual void Update(u32 step);               // _10
	virtual void Reset();                        // _14
	virtual void SetCamera(CCamera* camera);     // _18
	virtual u32 vf5C() { return mInterestMode; } // _5C

	void SetZoomDistance(f32 distance)
	{
		mDistanceInterp = NULL;
		SetDistance(distance);
	}
	void StartZoom(f32 distance)
	{
		mDistanceInterp = &mDistanceEase;
		if (mDistanceInterp != NULL) {
			mDistanceInterp->Start(mDistance, distance, 15.0f);
		}
	}
	virtual ~CCameraActionScroll() { }
};
STATIC_ASSERT(sizeof(CCameraActionScroll) == 0x6C);

// behind the player at an angle off its facing
class CCameraActionLadr : public CInterfaceCameraAction {
public:
	f32 mAngle; // _40, added to the player's y rotation

	virtual void Update(u32 step);              // _10
	virtual void Reset();                       // _14
	virtual u32 GetCameraFlags() { return 11; } // _54
	virtual Vec GetCameraTarget();              // _64, the player

	void CalcEye(Vec* eye, f32 angle);
	virtual ~CCameraActionLadr() { }
};
STATIC_ASSERT(sizeof(CCameraActionLadr) == 0x44);

// the camera on an XObjLeticker, the dpad turns it a quarter at a time inside a box
class CCameraActionLeticker : public CCameraActionLadr {
public:
	Vec* mTarget;    // _44
	f32 mTurnTo;     // _48
	f32 mTurnSpeed;  // _4C
	s32 mTurnFrames; // _50
	bool mBounded;   // _54
	f32 mMinX;       // _58
	f32 mMaxX;       // _5C
	f32 mMinZ;       // _60
	f32 mMaxZ;       // _64

	CCameraActionLeticker() { mTarget = NULL; }
	virtual ~CCameraActionLeticker() { }

	virtual void Update(u32 step);                  // _10
	virtual void Reset();                           // _14
	virtual u32 vf50() { return 0; }                // _50 (weak)
	virtual u32 GetCameraFlags() { return 11; }     // _54 (weak)
	virtual f32 GetFollowDivisor() { return 0.1f; } // _58 (weak)
	virtual Vec GetCameraTarget();                  // _64
};
STATIC_ASSERT(sizeof(CCameraActionLeticker) == 0x68);

// just looks at the player, the camera keeps everything else
class CCameraActionDefault : public CInterfaceCameraAction {
public:
	virtual void Update(u32 step); // _10
	virtual ~CCameraActionDefault() { }
};
STATIC_ASSERT(sizeof(CCameraActionDefault) == 0x40);

// follows the player from behind on a damped spring, the stick or dpad turns and zooms it
class CCameraActionChase : public CInterfaceCameraAction {
public:
	XObj* mTarget;     // _40
	f32 mRestDistance; // _44
	f32 mDamping;      // _48
	CVec mLastPos;     // _4C
	CVec mPos;         // _5C, the point behind the target the eye is pulled to
	u32 mMode;         // _6C

	virtual void Update(u32 step);           // _10
	virtual void Reset();                    // _14
	virtual void SetCamera(CCamera* camera); // _18
	virtual void Init();                     // _1C
	virtual u32 GetCameraFlags();            // _54
	virtual ~CCameraActionChase() { }
};
STATIC_ASSERT(sizeof(CCameraActionChase) == 0x70);

// the camera event scripts drive, every part eases to where the script says
class CCameraActionEvt : public CInterfaceCameraAction {
public:
	CCameraActionEvt() { }
	XObj* mTarget;                                      // _40, the interest follows it while nothing eases it
	CInterfaceVectorInterpolation* mEyeInterp;          // _44
	CInterfaceVectorInterpolation* mInterestInterp;     // _48
	CInterfaceInterpolation* mDistanceInterp;           // _4C
	CInterfaceInterpolation* mFovInterp;                // _50
	CInterfaceVectorInterpolation* mAngleInterp;        // _54
	CStatus mAxes;                                      // _58, which angles SetAngle_Interp moves
	u32 _60;                                            // _60
	CVectorLinearInterpolation mEyeLinear;              // _64
	CVectorEaseInOutInterpolation mEyeEaseInOut;        // _98
	CVectorEaseOutInterpolation mEyeEaseOut;            // _F4
	CVectorNoneInterpolation mEyeNone;                  // _158
	CInterfaceVectorInterpolation* mEyeInterps[4];      // _18C, by mode, 3 sets it at once
	CVectorLinearInterpolation mInterestLinear;         // _19C
	CVectorEaseInOutInterpolation mInterestEaseInOut;   // _1D0
	CVectorEaseOutInterpolation mInterestEaseOut;       // _22C
	CVectorNoneInterpolation mInterestNone;             // _290
	CInterfaceVectorInterpolation* mInterestInterps[4]; // _2C4
	CInterpolationLinear mFovLinear;                    // _2D4
	CInterpolationEaseInOut mFovEaseInOut;              // _2E4
	CInterpolationEaseOut mFovEaseOut;                  // _300
	CInterpolationNone mFovNone;                        // _320
	CInterfaceInterpolation* mFovInterps[4];            // _330
	CInterpolationLinear mDistanceLinear;               // _340
	CInterpolationEaseInOut mDistanceEaseInOut;         // _350
	CInterpolationEaseOut mDistanceEaseOut;             // _36C
	CInterpolationNone mDistanceNone;                   // _38C
	CInterfaceInterpolation* mDistanceInterps[4];       // _39C
	CVectorLinearInterpolation mAngleLinear;            // _3AC
	CVectorEaseInOutInterpolation mAngleEaseInOut;      // _3E0
	CVectorEaseOutInterpolation mAngleEaseOut;          // _43C
	CVectorNoneInterpolation mAngleNone;                // _4A0
	CInterfaceVectorInterpolation* mAngleInterps[4];    // _4D4

	virtual void Update(u32 step);           // _10
	virtual void SetCamera(CCamera* camera); // _18
	virtual void Init();                     // _1C

	void SetAngle_Interp(s32 mode, const Vec& angle, u32 axes, u32 type, BOOL relative, f32 time);
	void SetFov_Interp(s32 mode, f32 fov, f32 time);
	void SetInterest_Interp(s32 mode, const Vec& interest, f32 time);
	void SetInterest_Interp(s32 mode, XObj* target, f32 time);
	void SetEye_Interp(s32 mode, const Vec& eye, f32 time);
};
STATIC_ASSERT(sizeof(CCameraActionEvt) == 0x4E4);

// the room camera, it pans over the room with the stick and zooms with the c stick
class CCameraActionScroll2 : public CInterfaceCameraAction {
public:
	Vec* mCenterPtr;     // _40, always mCenter
	Vec mCenter;         // _44
	f32 mMinX;           // _50
	f32 mMaxX;           // _54
	f32 mMinZ;           // _58
	f32 mMaxZ;           // _5C
	f32 mMaxFov;         // _60
	f32 mMinFov;         // _64
	f32 mYaw;            // _68
	Vec mOffset;         // _6C, eye minus interest
	Vec mStart;          // _78, the interest it started from
	BOOL mTurned;        // _84, the camera is turned a quarter round x in room 11
	BOOL mFromStage;     // _88
	u8 mSavedInfo[0x20]; // _8C

	virtual void Update(u32 step); // _10
	virtual void Reset();          // _14
	virtual void vf20();           // _20, draws the markers over the room's objects
	virtual void vf28();           // _28
	virtual u32 GetCameraFlags();  // _54

	void SaveCamInfo(const void* info);
	Scroll2Limits* GetLimits(s32 room);
	void ShowObjs();
	void HideObjs();
	void ClearHideList();
	BOOL AddHideObj(s32 id);
	virtual ~CCameraActionScroll2() { }
};
STATIC_ASSERT(sizeof(CCameraActionScroll2) == 0xAC);

// plays a camera animation, copying an XObjCamera's camera every frame
class CCameraActionAnim : public CInterfaceCameraAction {
public:
	CCamera* mCamera; // _40
	XObjCamera* mObj; // _44

	virtual void Update(u32 step);           // _10
	virtual void SetCamera(CCamera* camera); // _18
	virtual void Init();                     // _1C

	void SetObj(XObjCamera* obj);
	virtual ~CCameraActionAnim() { }
};
STATIC_ASSERT(sizeof(CCameraActionAnim) == 0x48);

class CMotionControlCamera;

extern "C" {
// the area camera's state, ccameraactionareaex.cpp
f32 GetAreaCameraFovForHeightLevel(CamAreaState* state, s32 level);
s32 GetCameraAreaMode(CamAreaState* state);
void DrawDebug3d__19CCameraActionAreaExFv(CamAreaState* state);
void DrawMapCameraAreaDebug3D(CamAreaState* state, s32 mode);
s32 CollectCameraAreasContainingPoint(CamAreaState* state, Vec pos, s32* ids);
s32 FindFirstCameraAreaContainingPoint(CamAreaState* state, Vec pos);
s32 FindEnabledCameraAreaContainingPoint(CamAreaState* state, Vec pos);
void SetCameraAreaIndexAndBaseFov(CamAreaState* state, s32 index);
void UpdateCameraAreaIndexAndBaseFov(CamAreaState* state, s32 unused);
void LoadStageCameraAreaState(CamAreaState* state);
void InitCameraAreaEntries(CamAreaState* state);

// the area camera, ccameraactionarea.cpp
void SetAreaCameraHeightIndices(CCameraActionArea* area, s32 index);
void SelectAreaCameraEntryWithControl(CCameraActionArea* area, s32 slot, s32 entry, s32 mode, s32 control, f32 time);
void SelectAreaCameraEntryByViewDirection(CCameraActionArea* area, s32 mode, f32 time, s32 arg);
void ResumeAreaCameraControl(CCameraActionArea* area);

// the map camera, ccameraactionmap2.cpp
void StartMapCameraTargetHeightInterpolation(CCameraActionMap* map, f32 from, f32 to, f32 time);
f32 GetMapCameraHorizontalTargetDistance(CCameraActionMap* map);
void RefreshMapCameraArea(CCameraActionMap* map);
void GetMapCameraEyeBehindTarget(CCameraActionMap* map, Vec& out, s32 mode);
void SetMapCameraFovStep(CCameraActionMap* map, s32 index, f32 time);

void SetAreaCameraHeightIndex(CMotionControlCamera* camera, s32 mode);
}

#endif
