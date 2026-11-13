#include "Version.h"
#include "game/hsd_jobj.h"
#include "game/ut.h"
#include "game/cgame.h"
#include "game/gmath.h"

extern "C" double sin(double x);
extern "C" double cos(double x);
extern "C" double atan2(double y, double x);
extern "C" f32 PSVECDistance(Vec* a, Vec* b);

// first aobj on the jobj or below it
static inline HSD_AObj* FindAnim(HSD_JObj* jobj)
{
	if (jobj != NULL) {
		HSD_AObj* aobj = FindJointAnimationInTree(jobj);
		if (aobj != NULL) {
			return aobj;
		}
		aobj = FindMaterialOrTextureAnimationInJointTree(jobj);
		if (aobj != NULL) {
			return aobj;
		}
		aobj = FindPolygonAnimationInJointTree(jobj);
		if (aobj != NULL) {
			return aobj;
		}
	}
	return NULL;
}

static inline f32 GetAnimFrame(HSD_JObj* jobj)
{
	HSD_AObj* aobj = FindAnim(jobj);
	if (aobj != NULL) {
		return HSD_AObjGetCurrentFrame(aobj);
	}
	return 0.0f;
}

struct TrashObj;

struct UnkKindData {
	u8 _00[0x14];  // _00
	s16 mCategory; // _14, UnkObjData::mCategory, the object's category
};

struct TrashObj {
	HSD_JObj* jobj; // _00
	u8 _04[0x30];   // _04
	// _34 vtable
	virtual void vf08();                                       // _08
	virtual void SetTranslateX();                              // _0C
	virtual void SetTranslateY();                              // _10
	virtual void SetTranslateZ();                              // _14
	virtual void SetScaleX();                                  // _18
	virtual void SetScaleY();                                  // _1C
	virtual void SetScaleZ();                                  // _20
	virtual void SetRotationX();                               // _24
	virtual void SetRotationY();                               // _28
	virtual void SetRotationZ();                               // _2C
	virtual void SetTranslate(const Vec& v, s32 force);        // _30
	virtual void SetTranslate(f32 x, f32 y, f32 z, s32 force); // _34
	virtual void SetScale(const Vec& v, s32 force);            // _38
	virtual void SetScale(f32 x, f32 y, f32 z, s32 force);     // _3C
	virtual void SetRotation(const Vec& v, s32 force);         // _40
	virtual void SetRotation(f32 x, f32 y, f32 z, s32 force);  // _44
	virtual void SetAnimRate();                                // _48
	virtual void ReadAsync();                                  // _4C
	virtual void ReadCheck();                                  // _50
	virtual void Release();                                    // _54
	virtual void StepRotate();                                 // _58
	virtual void StepTranslate();                              // _5C
	virtual void StepScale();                                  // _60
	virtual void SetPos(const Vec& pos, s32 force);            // _64
	virtual void SetPos(f32 x, f32 y, f32 z, s32 force);       // _68
	virtual void SetScale(f32 scale, s32 force);               // _6C
	virtual void SetRot(const Vec& rot, s32 force);            // _70
	virtual void SetRot(f32 x, f32 y, f32 z, s32 force);       // _74

	virtual void SetMoveDir(const Vec* dir, f32 speed); // _78

	virtual void SetRotateY(); // _7C

	virtual void SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend,
	                     f32 blendRate); // _80

	virtual void ClearAnim();                 // _84
	virtual void UpdateAnim();                // _88
	virtual void ApplyTransform();            // _8C
	virtual void InitializeObject();          // _90
	virtual void ResetForNewGame();           // _94
	virtual void ResetObjectState();          // _98
	virtual void LoadFromPlacement();         // _9C
	virtual void LoadAnimationArchiveAsync(); // _A0
	virtual void UpdateObject();              // _A4
	virtual void StepColors();                // _A8
	virtual void DrawObject();                // _AC
	virtual void DrawWithView();              // _B0
	virtual void DrawReflection();            // _B4
	virtual void DrawGroundShadow();          // _B8
	virtual void DrawGroundShadowAt();        // _BC
	virtual void DrawObjectEffects();         // _C0
	virtual void DrawCollisionShapes();       // _C4
	virtual void DrawScreenOverlay();         // _C8
	virtual void DrawDebugStatus();           // _CC
	virtual void RegisterShadowObjects();     // _D0
	virtual void ExpandShadowBounds();        // _D4
	virtual void ApplyMaterialPresets();      // _D8
	virtual void SetDestinationAlpha();       // _DC
	virtual void EnableLighting();            // _E0
	virtual void SetTranslucent();            // _E4
	virtual void SetOpaque();                 // _E8
	virtual void GetCameraTarget();           // _EC
	virtual void GetJointAnimation();         // _F0
	virtual void GetMaterialAnimation();      // _F4
	virtual void GetShapeAnimation();         // _F8
	virtual void TestFloorCollision();        // _FC
	virtual void OnPlayerContact();           // _100
	virtual void TestFloorCrossing();         // _104
	virtual void GetShadowRadius();           // _108
	virtual void ProjectOffsetToScreen();     // _10C
	virtual void GetFloorXZExtent();          // _110
	virtual void GetCollisionHeight();        // _114
	virtual void ProjectVisibleBounds();      // _118
	virtual void IsOutsideCameraDepth();      // _11C
	virtual void SetCullMode();               // _120
	virtual void HandleEventCommand();        // _124
	virtual void GetEventScript();            // _128
	virtual void SetColor();                  // _12C
	virtual void OnPushDisplacement();        // _130
	virtual void ReceivePlugPower();          // _134
	virtual void OnHitFromPosition();         // _138
	virtual void OnSquirterHit();             // _13C
	virtual void OnItemUsed();                // _140
	virtual void OnMicrophonePeak();          // _144
	virtual u32 vf148();                      // _148
#if defined(VERSION_JP)
	u8 _38[0xC0 - 0x38]; // _38
#else
	u8 _38[0xC8 - 0x38]; // _38
#endif
	Vec mMoveTranslate;     // _C8 in Japan _C0
	u8 _D4[0x28];           // _D4 in Japan _CC
	f32 mMoveRotateY;       // _FC in Japan _F4
	u8 _100[0xC];           // _100 in Japan _F8
	f32 mMoveRotateTargetY; // _10C in Japan _104
	u8 _110[0x118];         // _110 in Japan _108
	u64 mStatus;            // _228 in Japan _220
	u8 _230[0x8];           // _230 in Japan _228
	s32* mCoord;            // _238 in Japan _230
	u8 _23C[0x4];           // _23C in Japan _234
	UnkKindData* mAttr;     // _240 in Japan _238
#if defined(VERSION_JP)
	u8 _244[0x970 - 0x23C]; // _244 in Japan _23C
#else
	u8 _244[0x980 - 0x244]; // _244
#endif
	u32* mFlags;           // _980 in Japan _970
	u8 _984[0x50];         // _984 in Japan _974
	s32 mFloorContacts;    // _9D4 in Japan _9C4, the contacted object's index (compared against the trashbox's)
	u8 _9D8[0x70];         // _9D8 in Japan _9C8
	s32 mLiftState;        // _A48 in Japan _A38
	u8 _A4C[0x18];         // _A4C in Japan _A3C
	TrashObj* mLiftTarget; // _A64 in Japan _A54

	Vec* GetPos() { return &mMoveTranslate; }
};

#if defined(VERSION_JP)
typedef char TrashXObjViewSizeCheck[sizeof(TrashObj) == 0xA58 ? 1 : -1];
#else
typedef char TrashXObjViewSizeCheck[sizeof(TrashObj) == 0xA68 ? 1 : -1];
#endif

static TrashObj* trashTarget;
static s32 trashStackNum;
static f32 trashStackScale;
static TrashObj* trashObj;
static s32 trashState;
static Vec trashPos;
static Vec trashDest;
static Vec trashStep;
static f32 trashArc;
// 0 while it runs, 2 dropped in flight, 3 reached the box, 4 cancelled on B
static s32 trashResult;
static u8 trashActive;
static s32 trashboxIndex;

extern "C" void DrawTrashStack(void* arg);
extern "C" void UpdateTrash(u32 frames);
extern "C" void InitTrash(void* arg);
extern "C" s32 CmdIsNearTrashbox(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdCancelTrash(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdEndTrash(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdGetTrashResult(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdStartTrash(s32 cmd, s32 count, s32* args);

/**
 * @note Address: stage04 .text 0x183C
 * @note Size: 0x5B8
 */
extern "C" void DrawTrashStack(void* arg)
{
	// NONMATCHING: regalloc
	TrashObj* actor;
	HSD_JObj* jobj;
	f32 a;
	f32 angle;
	f32 x;
	f32 y;
	f32 z;
	s32 count;
	s32 i;

	if (trashTarget == NULL) {
		return;
	}
	actor = trashObj;
	if (actor == NULL || !(actor->mStatus & 1)) {
		return;
	}
	if (!trashActive) {
		return;
	}

	count   = trashStackNum;
	Vec pos = actor->mMoveTranslate;
	angle   = THIRD_PI + actor->mMoveRotateY;
	jobj    = actor->jobj;
	if (count < 0) {
		count = 0;
	}
	if (count > 10) {
		count = 10;
	}

	for (i = 1; i < count; i++) {
		int step = i - 1;
		if (step % 4 == 0) { // every fourth piece sits on the axis
			x = 0.0f;
			z = x;
			y = 2.0f * step / 4.0f + 2.0f;
		} else {
			a = DEG2RAD * (step % 4 * 120) + angle; // the other three at 120 degrees
			x = 2.0f * (f32)sin(a);
			y = 2.0f * (i - 1) / 4.0f + 1.0f; // half a unit higher each step
			z = 2.0f * (f32)cos(a);
		}
		HSD_JObjSetTranslateX(jobj, x * trashStackScale + pos.x);
		HSD_JObjSetTranslateY(jobj, y * trashStackScale + pos.y);
		HSD_JObjSetTranslateZ(jobj, z * trashStackScale + pos.z);
		HSD_JObjDispAll(jobj, NULL, 7, 0);
	}
	HSD_JObjSetTranslateX(jobj, pos.x);
	HSD_JObjSetTranslateY(jobj, pos.y);
	HSD_JObjSetTranslateZ(jobj, pos.z);
}
// DrawTrashStack
/*
    stwu     r1, -0xb0(r1)
    mflr     r0
    stw      r0, 0xb4(r1)
    stfd     f31, 0xa0(r1)
    psq_st   f31, 0xa8(r1), 0, qr0
    stfd     f30, 0x90(r1)
    psq_st   f30, 0x98(r1), 0, qr0
    stfd     f29, 0x80(r1)
    psq_st   f29, 0x88(r1), 0, qr0
    stfd     f28, 0x70(r1)
    psq_st   f28, 0x78(r1), 0, qr0
    stfd     f27, 0x60(r1)
    psq_st   f27, 0x68(r1), 0, qr0
    stfd     f26, 0x50(r1)
    psq_st   f26, 0x58(r1), 0, qr0
    stfd     f25, 0x40(r1)
    psq_st   f25, 0x48(r1), 0, qr0
    stw      r31, 0x3c(r1)
    stw      r30, 0x38(r1)
    stw      r29, 0x34(r1)
    stw      r28, 0x30(r1)
    lis      r3, trashTarget@ha
    lwz      r0, trashTarget@l(r3)
    cmplwi   r0, 0x0
    beq      .L_00001D9C
    lis      r3, trashObj@ha
    lwz      r6, trashObj@l(r3)
    cmplwi   r6, 0x0
    beq      .L_00001D9C
    lwz      r0, 0x228(r6)
    li       r4, 0x0
    lwz      r5, 0x22c(r6)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    bne      .L_000018DC
    b        .L_00001D9C
.L_000018DC:
    lis      r3, trashActive@ha
    lbz      r0, trashActive@l(r3)
    cmplwi   r0, 0x0
    beq      .L_00001D9C
    lis      r4, trashStackNum@ha
    lis      r3, lbl_7_rodata_170@ha
    lwz      r7, trashStackNum@l(r4)
    lwz      r5, 0xc8(r6)
    lwz      r4, 0xcc(r6)
    cmpwi    r7, 0x0
    lwz      r0, 0xd0(r6)
    mr       r29, r7
    lfs      f1, lbl_7_rodata_170@l(r3)
    lfs      f0, 0xfc(r6)
    lwz      r31, 0x0(r6)
    fadds    f29, f1, f0
    stw      r5, 0x8(r1)
    stw      r4, 0xc(r1)
    stw      r0, 0x10(r1)
    bge      .L_00001930
    li       r29, 0x0
.L_00001930:
    cmpwi    r29, 0xa
    ble      .L_0000193C
    li       r29, 0xa
.L_0000193C:
    lis      r3, trashStackScale@ha
    lfs      f31, 0xc(r1)
    lfs      f30, 0x10(r1)
    addi     r30, r3, trashStackScale@l
    lfs      f28, 0x8(r1)
    li       r28, 0x1
    b        .L_00001C10
.L_00001958:
    subi     r4, r28, 0x1
    slwi     r0, r4, 30
    srwi     r3, r4, 31
    subf     r0, r3, r0
    rotlwi   r0, r0, 2
    add.     r0, r0, r3
    bne      .L_000019C0
    xoris    r3, r4, 0x8000
    lis      r0, 0x4330
    lis      r4, sTrashSignedConversionBias@ha
    stw      r3, 0x1c(r1)
    addi     r3, r4, sTrashSignedConversionBias@l
    lis      r5, lbl_7_rodata_168@ha
    stw      r0, 0x18(r1)
    lis      r4, lbl_7_rodata_174@ha
    lfd      f1, 0x0(r3)
    lis      r3, lbl_7_rodata_178@ha
    lfd      f0, 0x18(r1)
    lfs      f2, lbl_7_rodata_174@l(r4)
    fsubs    f1, f0, f1
    lfs      f27, lbl_7_rodata_168@l(r5)
    lfs      f0, lbl_7_rodata_178@l(r3)
    fmr      f25, f27
    fmuls    f1, f2, f1
    fmadds   f26, f1, f0, f2
    b        .L_00001A60
.L_000019C0:
    mulli    r4, r0, 0x78
    lis      r0, 0x4330
    stw      r0, 0x18(r1)
    lis      r3, sTrashSignedConversionBias@ha
    lis      r5, lbl_7_rodata_17C@ha
    lfd      f1, sTrashSignedConversionBias@l(r3)
    xoris    r0, r4, 0x8000
    lfs      f2, lbl_7_rodata_17C@l(r5)
    stw      r0, 0x1c(r1)
    lfd      f0, 0x18(r1)
    fsubs    f0, f0, f1
    fmadds   f27, f2, f0, f29
    fmr      f1, f27
    bl       sin
    subi     r3, r28, 0x1
    lis      r0, 0x4330
    xoris    r3, r3, 0x8000
    lis      r5, lbl_7_rodata_174@ha
    lis      r4, sTrashSignedConversionBias@ha
    stw      r3, 0x24(r1)
    addi     r3, r4, sTrashSignedConversionBias@l
    frsp     f5, f1
    stw      r0, 0x20(r1)
    lis      r4, lbl_7_rodata_178@ha
    lfd      f2, 0x0(r3)
    lis      r3, sTrashScalePercent@ha
    lfd      f0, 0x20(r1)
    lfs      f4, lbl_7_rodata_174@l(r5)
    fmr      f1, f27
    fsubs    f3, f0, f2
    lfs      f2, lbl_7_rodata_178@l(r4)
    lfs      f0, sTrashScalePercent@l(r3)
    fmuls    f27, f4, f5
    fmuls    f3, f4, f3
    fmadds   f26, f3, f2, f0
    bl       cos
    lis      r3, lbl_7_rodata_174@ha
    frsp     f1, f1
    lfs      f0, lbl_7_rodata_174@l(r3)
    fmuls    f25, f0, f1
.L_00001A60:
    lfs      f0, 0x0(r30)
    cmplwi   r31, 0x0
    fmadds   f27, f27, f0, f28
    bne      .L_00001A88
    lis      r3, lbl_7_rodata_184@ha
    lis      r5, lbl_7_rodata_18C@ha
    addi     r3, r3, lbl_7_rodata_184@l
    li       r4, 0x3b8
    addi     r5, r5, lbl_7_rodata_18C@l
    bl       __assert
.L_00001A88:
    stfs     f27, 0x38(r31)
    lwz      r0, 0x14(r31)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_00001AE8
    cmplwi   r31, 0x0
    beq      .L_00001AE8
    bne      .L_00001ABC
    lis      r3, lbl_7_rodata_184@ha
    lis      r5, lbl_7_rodata_18C@ha
    addi     r3, r3, lbl_7_rodata_184@l
    li       r4, 0x25d
    addi     r5, r5, lbl_7_rodata_18C@l
    bl       __assert
.L_00001ABC:
    lwz      r4, 0x14(r31)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00001AD8
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00001AD8
    li       r3, 0x1
.L_00001AD8:
    clrlwi.  r0, r3, 24
    bne      .L_00001AE8
    mr       r3, r31
    bl       HSD_JObjSetMtxDirtySub
.L_00001AE8:
    lfs      f0, 0x0(r30)
    cmplwi   r31, 0x0
    fmadds   f27, f26, f0, f31
    bne      .L_00001B10
    lis      r3, lbl_7_rodata_184@ha
    lis      r5, lbl_7_rodata_18C@ha
    addi     r3, r3, lbl_7_rodata_184@l
    li       r4, 0x3c6
    addi     r5, r5, lbl_7_rodata_18C@l
    bl       __assert
.L_00001B10:
    stfs     f27, 0x3c(r31)
    lwz      r0, 0x14(r31)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_00001B70
    cmplwi   r31, 0x0
    beq      .L_00001B70
    bne      .L_00001B44
    lis      r3, lbl_7_rodata_184@ha
    lis      r5, lbl_7_rodata_18C@ha
    addi     r3, r3, lbl_7_rodata_184@l
    li       r4, 0x25d
    addi     r5, r5, lbl_7_rodata_18C@l
    bl       __assert
.L_00001B44:
    lwz      r4, 0x14(r31)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00001B60
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00001B60
    li       r3, 0x1
.L_00001B60:
    clrlwi.  r0, r3, 24
    bne      .L_00001B70
    mr       r3, r31
    bl       HSD_JObjSetMtxDirtySub
.L_00001B70:
    lfs      f0, 0x0(r30)
    cmplwi   r31, 0x0
    fmadds   f27, f25, f0, f30
    bne      .L_00001B98
    lis      r3, lbl_7_rodata_184@ha
    lis      r5, lbl_7_rodata_18C@ha
    addi     r3, r3, lbl_7_rodata_184@l
    li       r4, 0x3d4
    addi     r5, r5, lbl_7_rodata_18C@l
    bl       __assert
.L_00001B98:
    stfs     f27, 0x40(r31)
    lwz      r0, 0x14(r31)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_00001BF8
    cmplwi   r31, 0x0
    beq      .L_00001BF8
    bne      .L_00001BCC
    lis      r3, lbl_7_rodata_184@ha
    lis      r5, lbl_7_rodata_18C@ha
    addi     r3, r3, lbl_7_rodata_184@l
    li       r4, 0x25d
    addi     r5, r5, lbl_7_rodata_18C@l
    bl       __assert
.L_00001BCC:
    lwz      r4, 0x14(r31)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00001BE8
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00001BE8
    li       r3, 0x1
.L_00001BE8:
    clrlwi.  r0, r3, 24
    bne      .L_00001BF8
    mr       r3, r31
    bl       HSD_JObjSetMtxDirtySub
.L_00001BF8:
    mr       r3, r31
    li       r4, 0x0
    li       r5, 0x7
    li       r6, 0x0
    bl       HSD_JObjDispAll
    addi     r28, r28, 0x1
.L_00001C10:
    cmpw     r28, r29
    blt      .L_00001958
    cmplwi   r31, 0x0
    bne      .L_00001C38
    lis      r3, lbl_7_rodata_184@ha
    lis      r5, lbl_7_rodata_18C@ha
    addi     r3, r3, lbl_7_rodata_184@l
    li       r4, 0x3b8
    addi     r5, r5, lbl_7_rodata_18C@l
    bl       __assert
.L_00001C38:
    lfs      f0, 0x8(r1)
    stfs     f0, 0x38(r31)
    lwz      r0, 0x14(r31)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_00001C9C
    cmplwi   r31, 0x0
    beq      .L_00001C9C
    bne      .L_00001C70
    lis      r3, lbl_7_rodata_184@ha
    lis      r5, lbl_7_rodata_18C@ha
    addi     r3, r3, lbl_7_rodata_184@l
    li       r4, 0x25d
    addi     r5, r5, lbl_7_rodata_18C@l
    bl       __assert
.L_00001C70:
    lwz      r4, 0x14(r31)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00001C8C
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00001C8C
    li       r3, 0x1
.L_00001C8C:
    clrlwi.  r0, r3, 24
    bne      .L_00001C9C
    mr       r3, r31
    bl       HSD_JObjSetMtxDirtySub
.L_00001C9C:
    cmplwi   r31, 0x0
    bne      .L_00001CBC
    lis      r3, lbl_7_rodata_184@ha
    lis      r5, lbl_7_rodata_18C@ha
    addi     r3, r3, lbl_7_rodata_184@l
    li       r4, 0x3c6
    addi     r5, r5, lbl_7_rodata_18C@l
    bl       __assert
.L_00001CBC:
    stfs     f31, 0x3c(r31)
    lwz      r0, 0x14(r31)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_00001D1C
    cmplwi   r31, 0x0
    beq      .L_00001D1C
    bne      .L_00001CF0
    lis      r3, lbl_7_rodata_184@ha
    lis      r5, lbl_7_rodata_18C@ha
    addi     r3, r3, lbl_7_rodata_184@l
    li       r4, 0x25d
    addi     r5, r5, lbl_7_rodata_18C@l
    bl       __assert
.L_00001CF0:
    lwz      r4, 0x14(r31)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00001D0C
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00001D0C
    li       r3, 0x1
.L_00001D0C:
    clrlwi.  r0, r3, 24
    bne      .L_00001D1C
    mr       r3, r31
    bl       HSD_JObjSetMtxDirtySub
.L_00001D1C:
    cmplwi   r31, 0x0
    bne      .L_00001D3C
    lis      r3, lbl_7_rodata_184@ha
    lis      r5, lbl_7_rodata_18C@ha
    addi     r3, r3, lbl_7_rodata_184@l
    li       r4, 0x3d4
    addi     r5, r5, lbl_7_rodata_18C@l
    bl       __assert
.L_00001D3C:
    stfs     f30, 0x40(r31)
    lwz      r0, 0x14(r31)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_00001D9C
    cmplwi   r31, 0x0
    beq      .L_00001D9C
    bne      .L_00001D70
    lis      r3, lbl_7_rodata_184@ha
    lis      r5, lbl_7_rodata_18C@ha
    addi     r3, r3, lbl_7_rodata_184@l
    li       r4, 0x25d
    addi     r5, r5, lbl_7_rodata_18C@l
    bl       __assert
.L_00001D70:
    lwz      r4, 0x14(r31)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00001D8C
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00001D8C
    li       r3, 0x1
.L_00001D8C:
    clrlwi.  r0, r3, 24
    bne      .L_00001D9C
    mr       r3, r31
    bl       HSD_JObjSetMtxDirtySub
.L_00001D9C:
    psq_l    f31, 0xa8(r1), 0, qr0
    lfd      f31, 0xa0(r1)
    psq_l    f30, 0x98(r1), 0, qr0
    lfd      f30, 0x90(r1)
    psq_l    f29, 0x88(r1), 0, qr0
    lfd      f29, 0x80(r1)
    psq_l    f28, 0x78(r1), 0, qr0
    lfd      f28, 0x70(r1)
    psq_l    f27, 0x68(r1), 0, qr0
    lfd      f27, 0x60(r1)
    psq_l    f26, 0x58(r1), 0, qr0
    lfd      f26, 0x50(r1)
    psq_l    f25, 0x48(r1), 0, qr0
    lfd      f25, 0x40(r1)
    lwz      r31, 0x3c(r1)
    lwz      r30, 0x38(r1)
    lwz      r29, 0x34(r1)
    lwz      r0, 0xb4(r1)
    lwz      r28, 0x30(r1)
    mtlr     r0
    addi     r1, r1, 0xb0
    blr
*/

// the kind id, -1 without kind data
static inline s16 GetKindId(TrashObj* obj)
{
	return obj->mAttr != NULL ? (obj->mAttr != NULL ? obj->mAttr->mCategory : 0) : -1;
}

/**
 * @note Address: stage04 .text 0x1DF4
 * @note Size: 0x7E0
 */
extern "C" void UpdateTrash(u32 frames)
{
	// NONMATCHING: regalloc
	Vec impactPos;
	TrashObj* robot;

	if (trashTarget != NULL && trashActive) {
		CPad pad(0);
		robot = (TrashObj*)gGame.GetPlayer();
		switch (trashState) {
		case 0:
			trashObj = robot->mLiftTarget;   // the lifted object is the one to throw
			if (pad.mPad->mButton & 0x100) { // A throws
				robot->SetAnim(9, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				trashState++;
			} else if (pad.mPad->mTrigger & 0x200) { // B cancels
				trashResult = 4;
				trashState  = 99;
			}
			break;
		case 1:
			// wait for the robot's turn
			if (robot->mMoveRotateY == robot->mMoveRotateTargetY) {
				robot->mLiftState = 6;
				trashTarget->mStatus |= 0x10000000000ULL;
				trashState++;
			}
			break;
		case 2:
			if (15.0f <= GetAnimFrame(robot->jobj)) { // the toss anim's release frame
				robot->mLiftTarget = NULL;
				trashObj->mStatus &= ~0x400000000000000ULL; // no longer carried
				trashDest.x = trashTarget->GetPos()->x;
				trashDest.y = trashTarget->GetPos()->y;
				trashDest.z = trashTarget->GetPos()->z;
				trashDest.y += 30.0f; // aim above the box
				trashPos.x  = trashObj->GetPos()->x;
				trashPos.y  = trashObj->GetPos()->y;
				trashPos.z  = trashObj->GetPos()->z;
				trashStep.x = (trashDest.x - trashPos.x) / 36.0f; // 36 frames of flight
				trashArc    = 0.0f;
				trashState++;
				trashStep.y = (trashDest.y - trashPos.y) / 36.0f;
				trashStep.z = (trashDest.z - trashPos.z) / 36.0f;
			}
			break;
		case 3: {
			if (30.0f <= GetAnimFrame(robot->jobj) && GetAnimFrame(robot->jobj) < 33.0f) {
				// the toss sound's window
				XSound* obj = &gGame.mSound;
				if (!IsGameSoundPlaying(obj, 0x1B9)) {
					PlayGameSound(obj, 0x1B9, 0xFF);
				}
			}
			trashArc += DEG2RAD * 5 * frames;
			if ((f32)sin(trashArc) < 0.0f) { // past the arc's peak: it drops where it is
				trashResult    = 2;
				TrashObj* held = trashObj;
				if (GetKindId(held) == 8) { // category-8 pieces get their flag set
					u32 bit = held->vf148();
					held->mFlags[bit >> 5] |= 1 << (bit & 31);
				}
				trashObj->Release();
				trashObj   = NULL;
				trashState = 99;
			} else {
				Vec pos;
				pos.x = trashObj->GetPos()->x;
				pos.y = trashObj->GetPos()->y;
				pos.z = trashObj->GetPos()->z;
				pos.y = trashTarget->GetPos()->y;
				if (PSVECDistance(&trashTarget->mMoveTranslate, &pos) < 15.0f && trashObj->GetPos()->y < 30.0f) {
					trashState = 10; // close and low enough: it counts
				} else {
					trashPos.x += trashStep.x * frames;
					trashPos.y += trashStep.y * frames;
					trashPos.z += trashStep.z * frames;
					pos.x = trashPos.x;
					pos.y = trashPos.y;
					pos.z = trashPos.z;
					pos.y += 10.0f * (f32)sin(trashArc); // the arc's height
					trashObj->SetMoveDir(&pos, 10.0f);
				}
			}
			break;
		}
		case 10: {
			f32 angle;
			trashResult = 3;
			// it lands on the ground 2 units from the box, along the throw's line
			impactPos.x = trashObj->GetPos()->x;
			impactPos.y = trashObj->GetPos()->y;
			impactPos.z = trashObj->GetPos()->z;
			angle       = atan2(impactPos.x - trashTarget->GetPos()->x, impactPos.z - trashTarget->GetPos()->z);
			impactPos.x += 2.0f * (f32)sin(angle);
			impactPos.y = 0.0f;
			impactPos.z += 2.0f * (f32)cos(angle);
			trashObj->SetMoveDir(&impactPos, 1.0f);
			trashState = 99;
			break;
		}
		case 99:
			break;
		}
	}
}
// UpdateTrash
/*
    stwu     r1, -0x60(r1)
    mflr     r0
    stw      r0, 0x64(r1)
    stfd     f31, 0x50(r1)
    psq_st   f31, 0x58(r1), 0, qr0
    stw      r31, 0x4c(r1)
    stw      r30, 0x48(r1)
    stw      r29, 0x44(r1)
    lis      r4, trashTarget@ha
    mr       r30, r3
    lwz      r0, trashTarget@l(r4)
    cmplwi   r0, 0x0
    beq      .L_000025B0
    lis      r3, trashActive@ha
    lbz      r0, trashActive@l(r3)
    cmplwi   r0, 0x0
    beq      .L_000025B0
    addi     r3, r1, 0x8
    li       r4, 0x0
    bl       __ct__4CPadFUc
    lis      r3, trashState@ha
    lis      r4, gGame@ha
    addi     r5, r3, trashState@l
    lwz      r0, 0x0(r5)
    addi     r3, r4, gGame@l
    addis    r3, r3, 0x1
    cmpwi    r0, 0x3
    lwz      r31, -0x6944(r3)
    beq      .L_00002104
    bge      .L_00001E84
    cmpwi    r0, 0x1
    beq      .L_00001F38
    bge      .L_00001F74
    cmpwi    r0, 0x0
    bge      .L_00001E9C
    b        .L_000025A4
.L_00001E84:
    cmpwi    r0, 0x63
    beq      .L_000025A4
    bge      .L_000025A4
    cmpwi    r0, 0xa
    beq      .L_000024C8
    b        .L_000025A4
.L_00001E9C:
    lwz      r0, 0xa64(r31)
    lis      r3, trashObj@ha
    lwz      r4, 0x8(r1)
    stw      r0, trashObj@l(r3)
    lwz      r0, 0x4(r4)
    rlwinm.  r0, r0, 0, 23, 23
    beq      .L_00001F14
    lis      r4, lbl_7_rodata_168@ha
    lwz      r12, 0x34(r31)
    lfs      f1, lbl_7_rodata_168@l(r4)
    lis      r5, sTrashScalePercent@ha
    lwz      r12, 0x80(r12)
    lis      r4, lbl_7_rodata_1A0@ha
    fmr      f3, f1
    lfs      f2, sTrashScalePercent@l(r5)
    lfs      f4, lbl_7_rodata_1A0@l(r4)
    mr       r3, r31
    li       r4, 0x9
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    mtctr    r12
    bctrl
    lis      r3, trashState@ha
    addi     r4, r3, trashState@l
    lwz      r3, 0x0(r4)
    addi     r0, r3, 0x1
    stw      r0, 0x0(r4)
    b        .L_000025A4
.L_00001F14:
    lwz      r0, 0x14(r4)
    rlwinm.  r0, r0, 0, 22, 22
    beq      .L_000025A4
    li       r0, 0x63
    lis      r3, trashResult@ha
    li       r4, 0x4
    stw      r0, 0x0(r5)
    stw      r4, trashResult@l(r3)
    b        .L_000025A4
.L_00001F38:
    lfs      f1, 0xfc(r31)
    lfs      f0, 0x10c(r31)
    fcmpu    cr0, f1, f0
    bne      .L_000025A4
    li       r0, 0x6
    lis      r3, trashTarget@ha
    stw      r0, 0xa48(r31)
    lwz      r4, trashTarget@l(r3)
    lwz      r0, 0x228(r4)
    ori      r0, r0, 0x100
    stw      r0, 0x228(r4)
    lwz      r3, 0x0(r5)
    addi     r0, r3, 0x1
    stw      r0, 0x0(r5)
    b        .L_000025A4
.L_00001F74:
    lwz      r30, 0x0(r31)
    cmplwi   r30, 0x0
    beq      .L_00001FC8
    mr       r3, r30
    bl       FindJointAnimationInTree
    cmplwi   r3, 0x0
    beq      .L_00001F98
    mr       r30, r3
    b        .L_00001FCC
.L_00001F98:
    mr       r3, r30
    bl       FindMaterialOrTextureAnimationInJointTree
    cmplwi   r3, 0x0
    beq      .L_00001FB0
    mr       r30, r3
    b        .L_00001FCC
.L_00001FB0:
    mr       r3, r30
    bl       FindPolygonAnimationInJointTree
    cmplwi   r3, 0x0
    beq      .L_00001FC8
    mr       r30, r3
    b        .L_00001FCC
.L_00001FC8:
    li       r30, 0x0
.L_00001FCC:
    cmplwi   r30, 0x0
    beq      .L_00001FF8
    bne      .L_00001FF0
    lis      r3, sStage04PathAobjH@ha
    lis      r5, lbl_7_rodata_1AC@ha
    addi     r3, r3, sStage04PathAobjH@l
    li       r4, 0x95
    addi     r5, r5, lbl_7_rodata_1AC@l
    bl       __assert
.L_00001FF0:
    lfs      f1, 0x4(r30)
    b        .L_00002000
.L_00001FF8:
    lis      r3, lbl_7_rodata_168@ha
    lfs      f1, lbl_7_rodata_168@l(r3)
.L_00002000:
    lis      r3, lbl_7_rodata_1B4@ha
    lfs      f0, lbl_7_rodata_1B4@l(r3)
    fcmpo    cr0, f0, f1
    cror     eq, lt, eq
    bne      .L_000025A4
    li       r0, 0x0
    lis      r5, 0xfc00
    stw      r0, 0xa64(r31)
    lis      r3, trashObj@ha
    lis      r4, lbl_7_rodata_1B8@ha
    li       r0, -0x1
    lwzu     r12, trashObj@l(r3)
    addi     r11, r4, lbl_7_rodata_1B8@l
    subi     r5, r5, 0x1
    lis      r7, trashTarget@ha
    lwz      r9, 0x22c(r12)
    lis      r6, trashDest@ha
    lwz      r8, 0x228(r12)
    lis      r10, trashPos@ha
    and      r0, r9, r0
    lfs      f0, 0x0(r11)
    stw      r0, 0x22c(r12)
    and      r0, r8, r5
    lis      r5, lbl_7_rodata_1BC@ha
    lis      r4, trashState@ha
    stw      r0, 0x228(r12)
    addi     r9, r5, lbl_7_rodata_1BC@l
    addi     r12, r6, trashDest@l
    addi     r5, r4, trashState@l
    lwz      r30, trashTarget@l(r7)
    lis      r4, lbl_7_rodata_168@ha
    addi     r7, r4, lbl_7_rodata_168@l
    lwz      r4, 0x0(r5)
    lfs      f2, 0xc8(r30)
    lis      r6, trashArc@ha
    lwz      r3, 0x0(r3)
    lis      r8, trashStep@ha
    stfs     f2, 0x0(r12)
    addi     r0, r4, 0x1
    lfs      f3, 0x0(r9)
    lfs      f1, 0xcc(r30)
    stfs     f1, 0x4(r12)
    fadds    f6, f1, f0
    lfs      f0, 0x0(r7)
    lfs      f7, 0xd0(r30)
    stfs     f7, 0x8(r12)
    stfs     f6, 0x4(r12)
    lfs      f1, 0xc8(r3)
    stfsu    f1, trashPos@l(r10)
    fsubs    f1, f2, f1
    lfs      f5, 0xcc(r3)
    fdivs    f2, f1, f3
    stfs     f5, 0x4(r10)
    lfs      f4, 0xd0(r3)
    stfs     f0, trashArc@l(r6)
    stfs     f4, 0x8(r10)
    stw      r0, 0x0(r5)
    fsubs    f1, f6, f5
    stfsu    f2, trashStep@l(r8)
    fsubs    f0, f7, f4
    fdivs    f1, f1, f3
    fdivs    f0, f0, f3
    stfs     f1, 0x4(r8)
    stfs     f0, 0x8(r8)
    b        .L_000025A4
.L_00002104:
    lwz      r29, 0x0(r31)
    cmplwi   r29, 0x0
    beq      .L_00002158
    mr       r3, r29
    bl       FindJointAnimationInTree
    cmplwi   r3, 0x0
    beq      .L_00002128
    mr       r29, r3
    b        .L_0000215C
.L_00002128:
    mr       r3, r29
    bl       FindMaterialOrTextureAnimationInJointTree
    cmplwi   r3, 0x0
    beq      .L_00002140
    mr       r29, r3
    b        .L_0000215C
.L_00002140:
    mr       r3, r29
    bl       FindPolygonAnimationInJointTree
    cmplwi   r3, 0x0
    beq      .L_00002158
    mr       r29, r3
    b        .L_0000215C
.L_00002158:
    li       r29, 0x0
.L_0000215C:
    cmplwi   r29, 0x0
    beq      .L_00002188
    bne      .L_00002180
    lis      r3, sStage04PathAobjH@ha
    lis      r5, lbl_7_rodata_1AC@ha
    addi     r3, r3, sStage04PathAobjH@l
    li       r4, 0x95
    addi     r5, r5, lbl_7_rodata_1AC@l
    bl       __assert
.L_00002180:
    lfs      f1, 0x4(r29)
    b        .L_00002190
.L_00002188:
    lis      r3, lbl_7_rodata_168@ha
    lfs      f1, lbl_7_rodata_168@l(r3)
.L_00002190:
    lis      r3, lbl_7_rodata_1B8@ha
    lfs      f0, lbl_7_rodata_1B8@l(r3)
    fcmpo    cr0, f0, f1
    cror     eq, lt, eq
    bne      .L_00002274
    lwz      r29, 0x0(r31)
    cmplwi   r29, 0x0
    beq      .L_000021F8
    mr       r3, r29
    bl       FindJointAnimationInTree
    cmplwi   r3, 0x0
    beq      .L_000021C8
    mr       r29, r3
    b        .L_000021FC
.L_000021C8:
    mr       r3, r29
    bl       FindMaterialOrTextureAnimationInJointTree
    cmplwi   r3, 0x0
    beq      .L_000021E0
    mr       r29, r3
    b        .L_000021FC
.L_000021E0:
    mr       r3, r29
    bl       FindPolygonAnimationInJointTree
    cmplwi   r3, 0x0
    beq      .L_000021F8
    mr       r29, r3
    b        .L_000021FC
.L_000021F8:
    li       r29, 0x0
.L_000021FC:
    cmplwi   r29, 0x0
    beq      .L_00002228
    bne      .L_00002220
    lis      r3, sStage04PathAobjH@ha
    lis      r5, lbl_7_rodata_1AC@ha
    addi     r3, r3, sStage04PathAobjH@l
    li       r4, 0x95
    addi     r5, r5, lbl_7_rodata_1AC@l
    bl       __assert
.L_00002220:
    lfs      f1, 0x4(r29)
    b        .L_00002230
.L_00002228:
    lis      r3, lbl_7_rodata_168@ha
    lfs      f1, lbl_7_rodata_168@l(r3)
.L_00002230:
    lis      r3, lbl_7_rodata_1C0@ha
    lfs      f0, lbl_7_rodata_1C0@l(r3)
    fcmpo    cr0, f1, f0
    bge      .L_00002274
    lis      r3, gGame@ha
    li       r4, 0x1b9
    addi     r3, r3, gGame@l
    addis    r29, r3, 0xe
    addi     r29, r29, 0x7e24
    mr       r3, r29
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    bne      .L_00002274
    mr       r3, r29
    li       r4, 0x1b9
    li       r5, 0xff
    bl       PlayGameSound
.L_00002274:
    lis      r0, 0x4330
    lis      r3, lbl_7_rodata_1C8@ha
    stw      r30, 0x2c(r1)
    addi     r4, r3, lbl_7_rodata_1C8@l
    lis      r5, lbl_7_rodata_1C4@ha
    lfd      f1, 0x0(r4)
    stw      r0, 0x28(r1)
    lis      r3, trashArc@ha
    lfs      f2, lbl_7_rodata_1C4@l(r5)
    lfd      f0, 0x28(r1)
    fsubs    f1, f0, f1
    lfsu     f0, trashArc@l(r3)
    fmadds   f1, f2, f1, f0
    stfs     f1, 0x0(r3)
    bl       sin
    lis      r3, lbl_7_rodata_168@ha
    frsp     f1, f1
    lfs      f0, lbl_7_rodata_168@l(r3)
    fcmpo    cr0, f1, f0
    bge      .L_00002374
    lis      r3, trashObj@ha
    lis      r4, trashResult@ha
    li       r0, 0x2
    lwz      r29, trashObj@l(r3)
    stw      r0, trashResult@l(r4)
    lwz      r3, 0x240(r29)
    cmplwi   r3, 0x0
    beq      .L_000022F8
    beq      .L_000022F0
    lha      r0, 0x14(r3)
    b        .L_000022FC
.L_000022F0:
    li       r0, 0x0
    b        .L_000022FC
.L_000022F8:
    li       r0, -0x1
.L_000022FC:
    extsh    r0, r0
    cmpwi    r0, 0x8
    bne      .L_0000233C
    mr       r3, r29
    lwz      r12, 0x34(r29)
    lwz      r12, 0x148(r12)
    mtctr    r12
    bctrl
    lwz      r6, 0x980(r29)
    rlwinm   r5, r3, 29, 3, 29
    clrlwi   r0, r3, 27
    li       r3, 0x1
    lwzx     r4, r6, r5
    slw      r0, r3, r0
    or       r0, r4, r0
    stwx     r0, r6, r5
.L_0000233C:
    lis      r3, trashObj@ha
    addi     r3, r3, trashObj@l
    lwz      r3, 0x0(r3)
    lwz      r12, 0x34(r3)
    lwz      r12, 0x54(r12)
    mtctr    r12
    bctrl
    lis      r4, trashObj@ha
    li       r5, 0x0
    lis      r3, trashState@ha
    li       r0, 0x63
    stw      r5, trashObj@l(r4)
    stw      r0, trashState@l(r3)
    b        .L_000025A4
.L_00002374:
    lis      r4, trashObj@ha
    lis      r3, trashTarget@ha
    addi     r5, r4, trashObj@l
    lwz      r5, 0x0(r5)
    addi     r3, r3, trashTarget@l
    lwz      r3, 0x0(r3)
    addi     r4, r1, 0x10
    lfs      f0, 0xc8(r5)
    addi     r3, r3, 0xc8
    stfs     f0, 0x10(r1)
    lfs      f0, 0xcc(r5)
    stfs     f0, 0x14(r1)
    lfs      f0, 0xd0(r5)
    stfs     f0, 0x18(r1)
    lfs      f0, 0x4(r3)
    stfs     f0, 0x14(r1)
    bl       PSVECDistance
    lis      r3, lbl_7_rodata_1B4@ha
    lfs      f0, lbl_7_rodata_1B4@l(r3)
    fcmpo    cr0, f1, f0
    bge      .L_000023F8
    lis      r4, trashObj@ha
    lis      r3, lbl_7_rodata_1B8@ha
    addi     r4, r4, trashObj@l
    lfs      f0, lbl_7_rodata_1B8@l(r3)
    lwz      r4, 0x0(r4)
    lfs      f1, 0xcc(r4)
    fcmpo    cr0, f1, f0
    bge      .L_000023F8
    lis      r3, trashState@ha
    li       r0, 0xa
    stw      r0, trashState@l(r3)
    b        .L_000025A4
.L_000023F8:
    lis      r0, 0x4330
    lis      r3, lbl_7_rodata_1C8@ha
    stw      r30, 0x2c(r1)
    addi     r4, r3, lbl_7_rodata_1C8@l
    lis      r5, trashStep@ha
    lfd      f6, 0x0(r4)
    stw      r0, 0x28(r1)
    lis      r3, trashPos@ha
    addi     r4, r3, trashPos@l
    lfsu     f3, trashStep@l(r5)
    lfd      f0, 0x28(r1)
    lis      r3, trashArc@ha
    stw      r30, 0x34(r1)
    fsubs    f2, f0, f6
    lfs      f1, 0x0(r4)
    stw      r0, 0x30(r1)
    fmadds   f5, f3, f2, f1
    lfd      f0, 0x30(r1)
    stw      r30, 0x3c(r1)
    fsubs    f2, f0, f6
    lfs      f3, 0x4(r5)
    stw      r0, 0x38(r1)
    lfs      f1, 0x4(r4)
    lfd      f0, 0x38(r1)
    fmadds   f4, f3, f2, f1
    lfs      f3, 0x8(r5)
    fsubs    f2, f0, f6
    lfs      f0, 0x8(r4)
    stfs     f5, 0x0(r4)
    lfs      f1, trashArc@l(r3)
    fmadds   f0, f3, f2, f0
    stfs     f4, 0x4(r4)
    stfs     f5, 0x10(r1)
    stfs     f0, 0x8(r4)
    stfs     f4, 0x14(r1)
    stfs     f0, 0x18(r1)
    bl       sin
    lis      r4, lbl_7_rodata_1A0@ha
    frsp     f2, f1
    lfs      f1, lbl_7_rodata_1A0@l(r4)
    lis      r3, trashObj@ha
    lfs      f0, 0x14(r1)
    addi     r3, r3, trashObj@l
    lwz      r3, 0x0(r3)
    fmadds   f0, f1, f2, f0
    addi     r4, r1, 0x10
    stfs     f0, 0x14(r1)
    lwz      r12, 0x34(r3)
    lwz      r12, 0x78(r12)
    mtctr    r12
    bctrl
    b        .L_000025A4
.L_000024C8:
    lis      r4, trashResult@ha
    lis      r3, trashObj@ha
    addi     r5, r4, trashResult@l
    li       r0, 0x3
    lwz      r4, trashObj@l(r3)
    lis      r3, trashTarget@ha
    stw      r0, 0x0(r5)
    addi     r3, r3, trashTarget@l
    lwz      r3, 0x0(r3)
    lfs      f3, 0xc8(r4)
    stfs     f3, 0x1c(r1)
    lfs      f0, 0xcc(r4)
    stfs     f0, 0x20(r1)
    lfs      f2, 0xd0(r4)
    stfs     f2, 0x24(r1)
    lfs      f1, 0xc8(r3)
    lfs      f0, 0xd0(r3)
    fsubs    f1, f3, f1
    fsubs    f2, f2, f0
    bl       atan2
    frsp     f31, f1
    fmr      f1, f31
    bl       sin
    lis      r3, lbl_7_rodata_174@ha
    frsp     f4, f1
    addi     r4, r3, lbl_7_rodata_174@l
    lfs      f2, 0x1c(r1)
    lfs      f3, 0x0(r4)
    lis      r3, lbl_7_rodata_168@ha
    lfs      f0, lbl_7_rodata_168@l(r3)
    fmadds   f2, f3, f4, f2
    fmr      f1, f31
    stfs     f0, 0x20(r1)
    stfs     f2, 0x1c(r1)
    bl       cos
    lis      r4, lbl_7_rodata_174@ha
    frsp     f2, f1
    lfs      f1, lbl_7_rodata_174@l(r4)
    lis      r3, trashObj@ha
    lfs      f0, 0x24(r1)
    addi     r4, r3, trashObj@l
    lis      r3, sTrashScalePercent@ha
    fmadds   f0, f1, f2, f0
    addi     r5, r3, sTrashScalePercent@l
    lwz      r3, 0x0(r4)
    addi     r4, r1, 0x1c
    lfs      f1, 0x0(r5)
    stfs     f0, 0x24(r1)
    lwz      r12, 0x34(r3)
    lwz      r12, 0x78(r12)
    mtctr    r12
    bctrl
    lis      r3, trashState@ha
    li       r0, 0x63
    stw      r0, trashState@l(r3)
.L_000025A4:
    lis      r3, __vt__4CPad@ha
    addi     r0, r3, __vt__4CPad@l
    stw      r0, 0xc(r1)
.L_000025B0:
    psq_l    f31, 0x58(r1), 0, qr0
    lwz      r0, 0x64(r1)
    lfd      f31, 0x50(r1)
    lwz      r31, 0x4c(r1)
    lwz      r30, 0x48(r1)
    lwz      r29, 0x44(r1)
    mtlr     r0
    addi     r1, r1, 0x60
    blr
*/

/**
 * @note Address: stage04 .text 0x25D4
 * @note Size: 0x48
 */
// CPad::~CPad

/**
 * @note Address: stage04 .text 0x261C
 * @note Size: 0xCC
 */
extern "C" void InitTrash(void* arg)
{
	TrashObj* actor;
	CGame* game = &gGame;
	s32 count;
	s32 i;

	trashTarget   = NULL;
	trashObj      = NULL;
	trashboxIndex = -1;
	trashActive   = 0;
	count         = game->mXObjs.mObjectCount;
	for (i = 0; trashboxIndex < 0 && i < count; i++) {
		actor = (TrashObj*)game->mXObjs.mObjects[i];
		if (actor->mStatus & 1) { // live objects only
			// the trashbox's object ids
			if (*actor->mCoord == 0x12D) {
				trashboxIndex = i;
			}
			if (*actor->mCoord == 0x477) {
				trashboxIndex = i;
			}
		}
	}
}

// needed as an inline for the stack layout of the two CVec copies
static inline s32 IsNearXZ(const Vec& p, const Vec& q)
{
	CVec a, b;
	a.x = p.x;
	a.y = p.y;
	a.z = p.z;
	b.x = q.x;
	b.y = q.y;
	b.z = q.z;
	b.y = 0.0f;
	a.y = 0.0f;
	if (11.5f < PSVECDistance(&a, &b)) { // the y fields are zeroed: XZ only
		return 0;
	}
	return 1;
}

/**
 * @note Address: stage04 .text 0x26E8
 * @note Size: 0x100
 */
extern "C" s32 CmdIsNearTrashbox(s32 cmd, s32 count, s32* args)
{
	// NONMATCHING: count regs — the instruction sequence is identical, but trashboxIndex,
	// the gGame+0x10000 base, robot and the CVec vtable land one slot over.
	CGame* game;
	TrashObj* robot;
	TrashObj* box;

	if (trashboxIndex < 0) {
		return 0;
	}
	game  = &gGame;
	robot = (TrashObj*)game->GetPlayer();
	if (trashboxIndex != robot->mFloorContacts) { // the box must be the contacted object
		return 0;
	}
	box = (TrashObj*)game->mXObjs.mObjects[trashboxIndex];
	return IsNearXZ(robot->mMoveTranslate, box->mMoveTranslate);
}
// CmdIsNearTrashbox
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    lis      r3, trashboxIndex@ha
    stw      r0, 0x34(r1)
    lwz      r5, trashboxIndex@l(r3)
    cmpwi    r5, 0x0
    bge      .L_0000270C
    li       r3, 0x0
    b        .L_000027D8
.L_0000270C:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r4, r3, 0x1
    lwz      r7, -0x6944(r4)
    lwz      r0, 0x9d4(r7)
    cmpw     r5, r0
    beq      .L_00002730
    li       r3, 0x0
    b        .L_000027D8
.L_00002730:
    lis      r3, __vt__4CVec@ha
    slwi     r0, r5, 2
    addi     r6, r3, __vt__4CVec@l
    lis      r3, lbl_7_rodata_168@ha
    stw      r6, 0x14(r1)
    add      r5, r4, r0
    addi     r4, r3, lbl_7_rodata_168@l
    lwz      r5, -0x67a0(r5)
    stw      r6, 0x24(r1)
    addi     r3, r1, 0x8
    lfs      f0, 0x0(r4)
    addi     r4, r1, 0x18
    lfs      f1, 0xc8(r7)
    stfs     f1, 0x8(r1)
    lfs      f1, 0xcc(r7)
    stfs     f1, 0xc(r1)
    lfs      f1, 0xd0(r7)
    stfs     f1, 0x10(r1)
    lfs      f1, 0xc8(r5)
    stfs     f1, 0x18(r1)
    lfs      f1, 0xcc(r5)
    stfs     f1, 0x1c(r1)
    lfs      f1, 0xd0(r5)
    stfs     f1, 0x20(r1)
    stfs     f0, 0x1c(r1)
    stfs     f0, 0xc(r1)
    bl       PSVECDistance
    lis      r3, lbl_7_rodata_16C@ha
    lfs      f0, lbl_7_rodata_16C@l(r3)
    fcmpo    cr0, f0, f1
    bge      .L_000027C4
    lis      r4, __vt__4CVec@ha
    li       r3, 0x0
    addi     r0, r4, __vt__4CVec@l
    stw      r0, 0x24(r1)
    stw      r0, 0x14(r1)
    b        .L_000027D8
.L_000027C4:
    lis      r4, __vt__4CVec@ha
    li       r3, 0x1
    addi     r0, r4, __vt__4CVec@l
    stw      r0, 0x24(r1)
    stw      r0, 0x14(r1)
.L_000027D8:
    lwz      r0, 0x34(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: stage04 .text 0x27E8
 * @note Size: 0x9C
 */
extern "C" s32 CmdCancelTrash(s32 cmd, s32 count, s32* args)
{
	TrashObj* robot = (TrashObj*)gGame.GetPlayer();

	if (robot->mLiftTarget != NULL) {
		robot->mLiftState = 6;
	}
	if (trashObj != NULL && (trashObj->mStatus & 1)) {
		trashObj->Release();
		trashObj = NULL;
	}
	return 0;
}

/**
 * @note Address: stage04 .text 0x2884
 * @note Size: 0x2C
 */
extern "C" s32 CmdEndTrash(s32 cmd, s32 count, s32* args)
{
	if (trashTarget != NULL) {
		trashObj    = NULL;
		trashActive = 0;
	}
	return 0;
}

/**
 * @note Address: stage04 .text 0x28B0
 * @note Size: 0x10
 */
extern "C" s32 CmdGetTrashResult(s32 cmd, s32 count, s32* args)
{
	return trashResult;
}

/**
 * @note Address: stage04 .text 0x28C0
 * @note Size: 0xA8
 */
extern "C" s32 CmdStartTrash(s32 cmd, s32 count, s32* args)
{
	CGame* game     = &gGame;
	f32 scale       = args[2] / 100.0f; // percent
	s32 idx         = args[0];          // the trashbox's object index
	s32 num         = args[1];          // pieces in the stack
	trashTarget     = (TrashObj*)game->mXObjs.mObjects[idx];
	trashStackNum   = num;
	trashStackScale = scale;
	if (trashTarget != NULL) {
		trashResult = 0;
		trashState  = 0;
		trashActive = 1;
	}
	return 0;
}
