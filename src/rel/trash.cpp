#include "rel/trash.h"
#include "game/hsd_jobj.h"
#include "game/animframe.h"
#include "game/xchr.h"
#include "game/xobjflags.h"
#include "game/cgame.h"
#include "game/gmath.h"

extern "C" double sin(double x);
extern "C" double cos(double x);
extern "C" double atan2(double y, double x);
extern "C" {
#include <dolphin/mtx/vec.h>
}

static XObj* trashTarget;
static s32 trashStackNum;
static f32 trashStackScale;
static XObj* trashObj;
static s32 trashState;
static Vec trashPos;
static Vec trashDest;
static Vec trashStep;
static f32 trashArc;
static s32 trashResult;
static u8 trashActive;
static s32 trashboxIndex;

/**
 * @note Address: stage04 .text 0x183C
 * @note Size: 0x5B8
 */
extern "C" void DrawTrashStack(void)
{
	// NONMATCHING: regalloc
	XObj* actor;
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
	if (actor == NULL || !(actor->mStatus.mFlags & 1)) {
		return;
	}
	if (!trashActive) {
		return;
	}

	count   = trashStackNum;
	Vec pos = actor->mMoveTranslate;
	angle   = 1.0471976f + actor->mMoveRotate.y;
	jobj    = actor->mJObj;
	if (count < 0) {
		count = 0;
	}
	if (count > 10) {
		count = 10;
	}

	for (i = 1; i < count; i++) {
		int step = i - 1;
		if (step % 4 == 0) {
			x = 0.0f;
			z = x;
			y = 2.0f * step / 4.0f + 2.0f;
		} else {
			a = DEG2RAD * (step % 4 * 120) + angle;
			x = 2.0f * (f32)sin(a);
			y = 2.0f * (i - 1) / 4.0f + 1.0f;
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

/**
 * @note Address: stage04 .text 0x1DF4
 * @note Size: 0x7E0
 */
extern "C" void UpdateTrash(u32 frames)
{
	// NONMATCHING: regalloc
	Vec impactPos;
	XObjChr* robot;

	if (trashTarget != NULL && trashActive) {
		CPad pad(0);
		robot = gGame.GetPlayer();
		switch (trashState) {
		case 0:
			trashObj = robot->mLift.mTarget;
			if (pad.mPad->mButton & 0x100) {
				robot->SetAnim(9, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				trashState++;
			} else if (pad.mPad->mTrigger & 0x200) {
				trashResult = 4;
				trashState  = 99;
			}
			break;
		case 1:
			if (robot->mMoveRotate.y == robot->mMoveRotate.mTarget.y) {
				robot->mLift.mState = 6;
				trashTarget->mStatus.mFlags |= 0x10000000000ULL;
				trashState++;
			}
			break;
		case 2:
			if (15.0f <= GetAnimFrame(robot->mJObj)) {
				robot->mLift.mTarget = NULL;
				trashObj->mStatus.mFlags &= ~0x400000000000000ULL;
				trashDest = trashTarget->mMoveTranslate;
				trashDest.y += 30.0f;
				trashPos    = trashObj->mMoveTranslate;
				trashStep.x = (trashDest.x - trashPos.x) / 36.0f;
				trashArc    = 0.0f;
				trashState++;
				trashStep.y = (trashDest.y - trashPos.y) / 36.0f;
				trashStep.z = (trashDest.z - trashPos.z) / 36.0f;
			}
			break;
		case 3: {
			if (30.0f <= GetAnimFrame(robot->mJObj) && GetAnimFrame(robot->mJObj) < 33.0f) {
				XSound* obj = &gGame.mSound;
				if (!IsGameSoundPlaying(obj, 0x1B9)) {
					PlayGameSound(obj, 0x1B9, 0xFF);
				}
			}
			trashArc += 0.08726646f * frames;
			if ((f32)sin(trashArc) < 0.0f) {
				trashResult = 2;
				if (trashObj->GetCategory() == 8) {
					static_cast<XObjItem*>(trashObj)->SetFlag();
				}
				trashObj->Release();
				trashObj   = NULL;
				trashState = 99;
			} else {
				Vec pos = trashObj->mMoveTranslate;
				pos.y   = trashTarget->mMoveTranslate.y;
				if (PSVECDistance(&trashTarget->mMoveTranslate, &pos) < 15.0f && trashObj->mMoveTranslate.y < 30.0f) {
					trashState = 10;
				} else {
					trashPos.x += trashStep.x * frames;
					trashPos.y += trashStep.y * frames;
					trashPos.z += trashStep.z * frames;
					pos = trashPos;
					pos.y += 10.0f * (f32)sin(trashArc);
					trashObj->SetMoveDir(&pos, 10.0f);
				}
			}
			break;
		}
		case 10: {
			f32 angle;
			trashResult = 3;
			impactPos   = trashObj->mMoveTranslate;
			angle       = atan2(impactPos.x - trashTarget->mMoveTranslate.x, impactPos.z - trashTarget->mMoveTranslate.z);
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
extern "C" void InitTrash(void)
{
	XObj* actor;
	CGame* game = &gGame;
	s32 count;
	s32 i;

	trashTarget   = NULL;
	trashObj      = NULL;
	trashboxIndex = -1;
	trashActive   = 0;
	count         = game->mXObjs.mObjectCount;
	for (i = 0; trashboxIndex < 0 && i < count; i++) {
		actor = game->mXObjs.mObjects[i];
		if (actor->mStatus.mFlags & 1) {
			if (actor->mCoord.mPlacement->mObjectId == 0x12D) {
				trashboxIndex = i;
			}
			if (actor->mCoord.mPlacement->mObjectId == 0x477) {
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
	if (11.5f < PSVECDistance(&a, &b)) {
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
	// NONMATCHING: register allocation
	CGame* game;
	XObjChr* robot;
	XObj* box;

	if (trashboxIndex < 0) {
		return 0;
	}
	game  = &gGame;
	robot = game->GetPlayer();
	if (trashboxIndex != robot->mFloorContacts[0].objectIndex) {
		return 0;
	}
	box = game->mXObjs.mObjects[trashboxIndex];
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
	XObjChr* robot = gGame.GetPlayer();

	if (robot->mLift.mTarget != NULL) {
		robot->mLift.mState = 6;
	}
	if (trashObj != NULL && (trashObj->mStatus.mFlags & 1)) {
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
	f32 scale       = args[2] / 100.0f;
	s32 idx         = args[0];
	s32 num         = args[1];
	trashTarget     = game->mXObjs.mObjects[idx];
	trashStackNum   = num;
	trashStackScale = scale;
	if (trashTarget != NULL) {
		trashResult = 0;
		trashState  = 0;
		trashActive = 1;
	}
	return 0;
}
