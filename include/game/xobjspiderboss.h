#ifndef _GAME_XOBJSPIDERBOSS_H
#define _GAME_XOBJSPIDERBOSS_H

#include "game/xobjjointpart.h"
#include "game/csound.h"

// the big spider, npc_spider_3. class name from RTTI and the vtable at 0x8022BC18, the units
// from 0x80100264 to 0x80106A14. offsets are usa, everything past XObj sits 0x10 lower in japan

class XObjSpiderBoss : public XObj {
public:
	typedef void (XObjSpiderBoss::*StateFunc)(u32 frames);

	// _00-_980 = XObj
	f32 mHeading;             // _980
	f32 mSpeed;               // _984
	f32 mTargetRotY;          // _988
	f32 mRotSpeed;            // _98C
	u8 _990[0x994 - 0x990];   // _990
	f32 _994;                 // _994
	f32 mRadius;              // _998
	f32 mSpeedY;              // _99C
	f32 _9A0;                 // _9A0
	f32 _9A4;                 // _9A4
	s32 mState;               // _9A8, an index into gSpiderBossStateHandlers
	s32 mStep;                // _9AC
	s32 mTimer;               // _9B0
	CStatus mSpiderStatus;    // _9B4, 1 hit, 2 knocked over, 8 airborne, 0x2000000 frozen
	s32 mPrevState;           // _9BC
	u32 mActionTimer;         // _9C0, countdown the states pace their moves on
	f32 _9C4;                 // _9C4
	f32 _9C8;                 // _9C8
	Vec mHitPos;              // _9CC
	Vec mBasePos;             // _9D8
	f32 mHitStrength;         // _9E4
	s32 mHits;                // _9E8, hits left; each hit takes one and launches parts as it falls
	s32 mMaxHits;             // _9EC, 23, mHits starts here
	Vec mPatrolTarget;        // _9F0
	Vec mAreaPos;             // _9FC, corner of the area it walks in
	f32 mAreaWidth;           // _A08
	f32 mAreaDepth;           // _A0C
	Vec _A10;                 // _A10
	f32 _A1C;                 // _A1C
	s32 mPatrolSelection;     // _A20
	s32 mAltAnimId;           // _A24, the anim the sounds follow while 0x40000000 is set
	s32 mGrabCooldown;        // _A28
	s32 mCommand;             // _A2C
	s32 mArgs[5];             // _A30
	u32 _A44;                 // _A44
	f32 _A48;                 // _A48
	f32 mEscapeInputStrength; // _A4C
	s32 mDrainTimer;          // _A50
	f32 mEscapeRemaining;     // _A54
	Vec _A58;                 // _A58
	s32 mGrabIntervalFrames;  // _A64, a grab is available after three seconds
	u32 mDamageStep;          // _A68, 0 watching for a hit, 1 blinking it off
	s32 mBlinkTimer;          // _A6C
	u8 _A70[0xA95 - 0xA70];   // _A70
	u8 mPatrolEffectActive;   // _A95
	u8 _A96[0xA98 - 0xA96];   // _A96
	AnimSfx mAnimSfx;         // _A98
	XObjJointPart mParts[8];  // _AA8

	XObjSpiderBoss();

	void State0(u32 frames);
	void State1(u32 frames);
	void State2(u32 frames);
	void State3(u32 frames);
	void State4(u32 frames);

	virtual ~XObjSpiderBoss();                                         // _08
	virtual void InitializeObject(u32 id);                             // _90
	virtual void vf94();                                               // _94
	virtual void ResetObjectState();                                   // _98
	virtual BOOL LoadFromPlacement(const CoordInfo& info);             // _9C
	virtual void UpdateObject(u32 frames);                             // _A4
	virtual void vfAC();                                               // _AC
	virtual void DrawGroundShadow();                                   // _B8
	virtual void DrawGroundShadowAt(const Vec* origin, f32 opacity);   // _BC
	virtual void DrawObjectEffects();                                  // _C0
	virtual void vfC8();                                               // _C8
	virtual void DrawDebugStatus();                                    // _CC
	virtual s32 vf100(s32 unused, s32 value);                          // _100
	virtual s32 HandleEventCommand(s32 count, s32* args);              // _124
	virtual void OnHitFromPosition(const Vec& position, f32 strength); // _138
	virtual void UpdateAnimationSounds();                              // _148, anim sounds
};

extern "C" {
XObjSpiderBoss* __dt__14XObjSpiderBossFv(XObjSpiderBoss* spider, s32 flags);
void UpdateSpiderBossAttachments(XObjSpiderBoss* spider, u32 frames);
void UpdateSpiderBossDamage(XObjSpiderBoss* spider, u32 frames);
void ApplySpiderBossPlacement(XObjSpiderBoss* spider);

// the states, copied from lbl_8022BB40 at startup
extern XObjSpiderBoss::StateFunc gSpiderBossStateHandlers[5];
}

#endif
