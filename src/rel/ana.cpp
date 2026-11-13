#include "Version.h"

// the holes the player digs and plants. linked into five stages. the functions are in the order they
// were written, retail's code runs the other way (-inline deferred)

#include "game/hsd_jobj.h"
#include "game/ut.h"
#include "game/cgame.h"
#include "game/xchr.h"
#include "game/xobjchr.h"
#include "game/xobjflags.h"

extern "C" {
void OSReport(const char* msg, ...);
}

class Ana {
public:
	Ana() { }
	~Ana() { }
	virtual bool IsActive() { return mStatus.mFlags & 1; }

	void Set(f32 x, f32 y, f32 z);

	void SetJObj(s32 kind, s32 which)
	{
		mJObjKind  = kind;
		mJObjWhich = which;
	}

	Vec mPos;        // _04
	f32 mRotY;       // _10
	CStatus mStatus; // _14
	s32 mId;         // _1C
	s32 mJObjKind;   // _20
	s32 mJObjWhich;  // _24
};

class AnaNull : public Ana {
public:
	virtual bool IsActive() { return false; }
};

#define ANA_NUM        20
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

s32 activeAnaNum;
static Ana anas[ANA_NUM];
static AnaNull anaNull;
XObj* tutiAnaObj;
XObj* tutiHutaObj;
XObj* flowerObjs[3];

Ana* GetNearestAna(f32 x, f32 y, f32 z);

inline Ana* GetAna(s32 id)
{
	if (id < 0 || id >= ARRAY_COUNT(anas)) {
		return &anaNull;
	}
	return &anas[id];
}

inline void ReleaseHeld(XObjChr* player)
{
	if (player->mLift.mTarget != NULL) {
		player->mLift.mTarget->mStatus.mFlags &= ~0x0400000000000000ULL;
		player->mLift.mTarget = NULL;
	}
	player->mLift.mState = 0;
}

inline void DropObj(XObj* obj)
{
	if (obj->GetCategory() == XOBJS::ITEMS) {
		static_cast<XObjItem*>(obj)->SetFlag();
	}
	obj->Release();
}

extern "C" HSD_JObj* GetAnaJObj(s32 kind, s32 which);

// defined further down
extern "C" void Ana_Draw(Ana* ana);
extern "C" s32 AddAna(s32 id, f32 x, f32 y, f32 z);

/**
 * @note Address: stage04 .text 0x1818
 * @note Size: 0x24
 */
// Ana::Ana
/**
 * @note Address: stage04 .text 0x17B8
 * @note Size: 0x60
 */
// Ana::~Ana

/**
 * @note Address: stage04 .text 0x1780
 * @note Size: 0x38
 */
// destroys anas at exit, compiler made

/**
 * @note Address: stage04 .text 0x1738
 * @note Size: 0x48
 */
// CStatus::~CStatus, inline in the class above

/**
 * @note Address: stage04 .text 0x16C8
 * @note Size: 0x70
 */
// AnaNull::~AnaNull, compiler made

/**
 * @note Address: stage04 .text 0x16C0
 * @note Size: 0x8
 */
// AnaNull::IsActive, inline in the class above

/**
 * @note Address: stage04 .text 0x1624
 * @note Size: 0x9C
 */
// static initializer for anas and anaNull

/**
 * @note Address: stage04 .text 0x1518
 * @note Size: 0x10C
 */
extern "C" s32 CmdAddAna(s32 cmd, s32 argc, s32* args)
{
	if (argc >= 4) {
		return AddAna(args[3], args[0] / 100.0f, args[1] / 100.0f, args[2] / 100.0f);
	}
	return AddAna(-1, args[0] / 100.0f, args[1] / 100.0f, args[2] / 100.0f);
}

/**
 * @note Address: stage04 .text 0x148C
 * @note Size: 0x8C
 */
extern "C" s32 CmdSetTutiAna(s32 cmd, s32 argc, s32* args)
{
	Ana* ana = GetAna(args[0]);
	if (!ana->IsActive()) {
		return 0;
	}
	ana->SetJObj(1, 0);
	return 1;
}

/**
 * @note Address: stage04 .text 0x124C
 * @note Size: 0x240
 */
extern "C" s32 CmdFindNearestAna(s32 cmd, s32 argc, s32* args)
{
	Ana* best = NULL;

	if (argc == 3) {
		f32 x = args[0] / 100.0f;
		f32 y = args[1] / 100.0f;
		f32 z = args[2] / 100.0f;
		best  = GetNearestAna(x, y, z);
	} else {
		XObj* contact = gGame.GetObject(gGame.GetPlayer()->mFloorContacts[2].objectIndex);
		if (contact != NULL) {
			best = GetNearestAna(contact->mMoveTranslate.x, contact->mMoveTranslate.y, contact->mMoveTranslate.z);
			// retail tests the contact's result here and again below
			if (best != NULL) {
				return best->mId;
			}
		}
	}
	if (best != NULL) {
		return best->mId;
	}
	return -1;
}

/**
 * @note Address: stage04 .text 0x1100
 * @note Size: 0x14C
 */
extern "C" s32 CmdPlantTane(s32 cmd, s32 argc, s32* args)
{
	XObj* obj;
	Ana* ana = GetAna(args[0]);
	if (!ana->IsActive()) {
		return 0;
	}
	ana->SetJObj(2, 0);
	obj = gGame.GetPlayer()->mLift.mTarget;
	if (obj != NULL) {
		ReleaseHeld(gGame.GetPlayer());
		DropObj(obj);
	}
	return 1;
}

/**
 * @note Address: stage04 .text 0x1068
 * @note Size: 0x98
 */
extern "C" s32 CmdSetAnaJObj(s32 cmd, s32 argc, s32* args)
{
	Ana* ana = GetAna(args[0]);
	if (!ana->IsActive()) {
		return 0;
	}
	ana->SetJObj(args[1], args[2]);
	return 1;
}

/**
 * @note Address: stage04 .text 0xFF4
 * @note Size: 0x74
 */
extern "C" s32 CmdSetAnaRotY(s32 cmd, s32 argc, s32* args)
{
	Ana* ana   = GetAna(args[0]);
	ana->mRotY = 57.295776f * args[1];
	return 1;
}

/**
 * @note Address: stage04 .text 0xEDC
 * @note Size: 0x118
 */
extern "C" s32 CmdDropHeldObj(s32 cmd, s32 argc, s32* args)
{
	XObj* obj = gGame.GetPlayer()->mLift.mTarget;
	if (obj != NULL) {
		obj->mStatus.mFlags &= ~0x10000ULL;
		ReleaseHeld(gGame.GetPlayer());
		DropObj(obj);
	}
	return 0;
}

// CmdDropHeldObj
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    lis      r3, gGame@ha
    stw      r0, 0x14(r1)
    addi     r3, r3, gGame@l
    addis    r5, r3, 0x1
    stw      r31, 0xc(r1)
    lwz      r3, -0x6944(r5)
    lwz      r31, 0xa64(r3)
    cmplwi   r31, 0x0
    beq      .L_00000FDC
    lis      r3, 0xffff
    lwz      r4, 0x22c(r31)
    subi     r0, r3, 0x1
    lwz      r3, 0x228(r31)
    and      r0, r4, r0
    li       r6, -0x1
    stw      r0, 0x22c(r31)
    and      r0, r3, r6
    stw      r0, 0x228(r31)
    lwz      r7, -0x6944(r5)
    lwz      r8, 0xa64(r7)
    cmplwi   r8, 0x0
    beq      .L_00000F64
    lwz      r0, 0x22c(r8)
    lis      r3, 0xfc00
    lwz      r5, 0x228(r8)
    subi     r4, r3, 0x1
    and      r3, r0, r6
    li       r0, 0x0
    stw      r3, 0x22c(r8)
    and      r3, r5, r4
    stw      r3, 0x228(r8)
    stw      r0, 0xa64(r7)
.L_00000F64:
    li       r0, 0x0
    stw      r0, 0xa48(r7)
    lwz      r3, 0x240(r31)
    cmplwi   r3, 0x0
    beq      .L_00000F84
    beq      .L_00000F88
    lha      r0, 0x14(r3)
    b        .L_00000F88
.L_00000F84:
    li       r0, -0x1
.L_00000F88:
    extsh    r0, r0
    cmpwi    r0, 0x8
    bne      .L_00000FC8
    mr       r3, r31
    lwz      r12, 0x34(r31)
    lwz      r12, 0x148(r12)
    mtctr    r12
    bctrl
    lwz      r6, 0x980(r31)
    rlwinm   r5, r3, 29, 3, 29
    clrlwi   r0, r3, 27
    li       r3, 0x1
    lwzx     r4, r6, r5
    slw      r0, r3, r0
    or       r0, r4, r0
    stwx     r0, r6, r5
.L_00000FC8:
    mr       r3, r31
    lwz      r12, 0x34(r31)
    lwz      r12, 0x54(r12)
    mtctr    r12
    bctrl
.L_00000FDC:
    lwz      r0, 0x14(r1)
    li       r3, 0x0
    lwz      r31, 0xc(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: stage04 .text 0xED0
 * @note Size: 0xC
 */
// Ana::IsActive, inline in the class above

/**
 * @note Address: stage04 .text 0xE48
 * @note Size: 0x88
 */
extern "C" s32 CmdAttachAnaToRobo(s32 cmd, s32 argc, s32* args)
{
	Ana* ana = GetAna(args[0]);
	if (!ana->IsActive()) {
		return 0;
	}
	ana->mStatus.mFlags |= 8;
	return 0;
}

/**
 * @note Address: stage04 .text 0xCFC
 * @note Size: 0x14C
 */
extern "C" void InitAnas(void)
{
	s32 num;
	s32 i;
	u32 j;

	tutiAnaObj  = NULL;
	tutiHutaObj = NULL;
	for (j = 0; j < ARRAY_COUNT(flowerObjs); j++) {
		flowerObjs[j] = NULL;
	}
	activeAnaNum = 0;
	for (j = 0; j < ANA_NUM; j++) {
		anas[j].mStatus.mFlags = 0;
	}
	num = gGame.mXObjs.mObjectCount;

	for (i = 0; i < num; i++) {
		XObj* obj = gGame.GetObject(i);
		if (obj->mStatus.mFlags & 1) {
			if (obj->mCoord._00->_00 == 0x2A9) {
				tutiAnaObj = obj;
			}
			if (obj->mCoord._00->_00 == 0x178) {
				tutiHutaObj = obj;
			}
			if (obj->mCoord._00->_00 == 0x2B7) {
				flowerObjs[0] = obj;
			}
			if (obj->mCoord._00->_00 == 0x2B8) {
				flowerObjs[1] = obj;
			}
			if (obj->mCoord._00->_00 == 0x2B9) {
				flowerObjs[2] = obj;
			}
		}
	}
}

// InitAnas
/*
    lis      r4, anas@ha
    li       r0, 0x0
    addi     r6, r4, anas@l
    lis      r3, gGame@ha
    stw      r0, 0x14(r6)
    addi     r4, r3, gGame@l
    addis    r5, r4, 0x1
    lis      r3, flowerObjs@ha
    stw      r0, 0x3c(r6)
    lis      r9, tutiAnaObj@ha
    lis      r8, tutiHutaObj@ha
    lis      r7, activeAnaNum@ha
    stw      r0, 0x64(r6)
    lwz      r5, -0x57a0(r5)
    stw      r0, 0x8c(r6)
    stw      r0, 0xb4(r6)
    stw      r0, 0xdc(r6)
    stw      r0, 0x104(r6)
    stw      r0, 0x12c(r6)
    stw      r0, 0x154(r6)
    stw      r0, 0x17c(r6)
    stw      r0, 0x1a4(r6)
    stw      r0, 0x1cc(r6)
    stw      r0, 0x1f4(r6)
    stw      r0, 0x21c(r6)
    stw      r0, 0x244(r6)
    stw      r0, 0x26c(r6)
    stw      r0, 0x294(r6)
    stw      r0, 0x2bc(r6)
    stwu     r0, flowerObjs@l(r3)
    stw      r0, 0x2e4(r6)
    stw      r0, 0x30c(r6)
    stw      r0, tutiAnaObj@l(r9)
    stw      r0, tutiHutaObj@l(r8)
    stw      r0, 0x4(r3)
    stw      r0, 0x8(r3)
    stw      r0, activeAnaNum@l(r7)
    mtctr    r5
    cmpwi    r5, 0x0
    blelr
.L_00000D9C:
    addis    r6, r4, 0x1
    li       r5, 0x1
    lwz      r8, -0x67a0(r6)
    li       r6, 0x0
    lwz      r0, 0x228(r8)
    lwz      r7, 0x22c(r8)
    and      r0, r0, r6
    and      r5, r7, r5
    xor      r5, r5, r6
    xor      r0, r0, r6
    or.      r0, r5, r0
    beq      .L_00000E3C
    lwz      r5, 0x238(r8)
    lwz      r0, 0x0(r5)
    cmpwi    r0, 0x2a9
    bne      .L_00000DE4
    lis      r5, tutiAnaObj@ha
    stw      r8, tutiAnaObj@l(r5)
.L_00000DE4:
    lwz      r5, 0x238(r8)
    lwz      r0, 0x0(r5)
    cmpwi    r0, 0x178
    bne      .L_00000DFC
    lis      r5, tutiHutaObj@ha
    stw      r8, tutiHutaObj@l(r5)
.L_00000DFC:
    lwz      r5, 0x238(r8)
    lwz      r0, 0x0(r5)
    cmpwi    r0, 0x2b7
    bne      .L_00000E14
    lis      r5, flowerObjs@ha
    stw      r8, flowerObjs@l(r5)
.L_00000E14:
    lwz      r5, 0x238(r8)
    lwz      r0, 0x0(r5)
    cmpwi    r0, 0x2b8
    bne      .L_00000E28
    stw      r8, 0x4(r3)
.L_00000E28:
    lwz      r5, 0x238(r8)
    lwz      r0, 0x0(r5)
    cmpwi    r0, 0x2b9
    bne      .L_00000E3C
    stw      r8, 0x8(r3)
.L_00000E3C:
    addi     r4, r4, 0x4
    bdnz     .L_00000D9C
    blr
*/

/**
 * @note Address: stage04 .text 0xCF8
 * @note Size: 0x4
 */
extern "C" void UpdateAnas(void)
{
}

/**
 * @note Address: stage04 .text 0xC9C
 * @note Size: 0x5C
 */
extern "C" void DrawAnas(void)
{
	u32 i;
	for (i = 0; i < ANA_NUM; i++) {
		if (anas[i].mStatus.mFlags & 1) {
			Ana_Draw(&anas[i]);
		}
	}
}

/**
 * @note Address: stage04 .text 0xA4C
 * @note Size: 0x250
 */
extern "C" s32 AddAna(s32 id, f32 x, f32 y, f32 z)
{
	CVec pos(x, y, z);
	u32 i;

	if (id >= 0 && id < ARRAY_COUNT(anas)) {
#if defined(VERSION_GGTP01)
		// europe has no prints here and keeps the entry's address; indexing the table again does not give its code
		Ana* ana = &anas[id];
		if (!(ana->mStatus.mFlags & 1)) {
			activeAnaNum++;
		}
		ana->mStatus.mFlags |= 1;
		ana->mPos.x = x;
		ana->mPos.y = y;
		ana->mPos.z = z;
		ana->mId    = id;
#else
		if (!(anas[id].mStatus.mFlags & 1)) {
			activeAnaNum++;
		}
		OSReport("id=%d,%4.2f,%4.2f,%4.2f\n", id, x, y, z);
		anas[id].Set(x, y, z);
		anas[id].mId = id;
#endif
		return id;
	}

	for (i = 0; i < ARRAY_COUNT(anas); i++) {
		if ((anas[i].mStatus.mFlags & 1) && PSVECDistance(&pos, &anas[i].mPos) < 5.0f) {
			return -1;
		}
	}
	for (i = 0; i < ARRAY_COUNT(anas); i++) {
		if (!(anas[i].mStatus.mFlags & 1)) {
			activeAnaNum++;
			anas[i].Set(x, y, z);
			anas[i].mId = i;
			return i;
		}
	}
	return -1;
}

// AddAna
/*
    stwu     r1, -0x70(r1)
    mflr     r0
    stw      r0, 0x74(r1)
    stfd     f31, 0x60(r1)
    psq_st   f31, 0x68(r1), 0, qr0
    stfd     f30, 0x50(r1)
    psq_st   f30, 0x58(r1), 0, qr0
    stfd     f29, 0x40(r1)
    psq_st   f29, 0x48(r1), 0, qr0
    stfd     f28, 0x30(r1)
    psq_st   f28, 0x38(r1), 0, qr0
    stw      r31, 0x2c(r1)
    stw      r30, 0x28(r1)
    stw      r29, 0x24(r1)
    fmr      f28, f1
    mr.      r29, r3
    fmr      f29, f2
    lis      r3, __vt__4CVec@ha
    fmr      f30, f3
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x14(r1)
    stfs     f28, 0x8(r1)
    stfs     f29, 0xc(r1)
    stfs     f30, 0x10(r1)
    blt      .L_00000B60
    cmplwi   r29, 0x14
    bge      .L_00000B60
    mulli    r30, r29, 0x28
    lis      r3, anas@ha
    addi     r0, r3, anas@l
    add      r31, r0, r30
    lwzu     r0, 0x14(r31)
    clrlwi.  r0, r0, 31
    bne      .L_00000AE8
    lis      r3, activeAnaNum@ha
    addi     r4, r3, activeAnaNum@l
    lwz      r3, 0x0(r4)
    addi     r0, r3, 0x1
    stw      r0, 0x0(r4)
.L_00000AE8:
    fmr      f1, f28
    lis      r3, sAnaIdPositionFormat@ha
    fmr      f2, f29
    addi     r3, r3, sAnaIdPositionFormat@l
    fmr      f3, f30
    mr       r4, r29
    crset    cr1eq
    bl       OSReport
    fmr      f1, f28
    lis      r3, lbl_7_rodata_F0@ha
    fmr      f2, f29
    addi     r3, r3, lbl_7_rodata_F0@l
    fmr      f3, f30
    crset    cr1eq
    bl       OSReport
    lwz      r5, 0x0(r31)
    lis      r3, anas@ha
    addi     r0, r3, anas@l
    lis      r4, __vt__4CVec@ha
    ori      r5, r5, 0x1
    mr       r3, r29
    stw      r5, 0x0(r31)
    add      r5, r0, r30
    addi     r0, r4, __vt__4CVec@l
    stfs     f28, 0x4(r5)
    stfs     f29, 0x8(r5)
    stfs     f30, 0xc(r5)
    stw      r29, 0x1c(r5)
    stw      r0, 0x14(r1)
    b        .L_00000C60
.L_00000B60:
    lis      r3, sAnaMinimumSpacing@ha
    lis      r4, anas@ha
    addi     r31, r4, anas@l
    lfs      f31, sAnaMinimumSpacing@l(r3)
    li       r30, 0x0
    mr       r29, r31
.L_00000B78:
    lwz      r0, 0x14(r29)
    clrlwi.  r0, r0, 31
    beq      .L_00000BAC
    addi     r3, r1, 0x8
    addi     r4, r29, 0x4
    bl       PSVECDistance
    fcmpo    cr0, f1, f31
    bge      .L_00000BAC
    lis      r4, __vt__4CVec@ha
    li       r3, -0x1
    addi     r0, r4, __vt__4CVec@l
    stw      r0, 0x14(r1)
    b        .L_00000C60
.L_00000BAC:
    addi     r30, r30, 0x1
    addi     r29, r29, 0x28
    cmplwi   r30, 0x14
    blt      .L_00000B78
    li       r0, 0x14
    li       r30, 0x0
    mtctr    r0
.L_00000BC8:
    lwz      r0, 0x14(r31)
    clrlwi.  r0, r0, 31
    bne      .L_00000C44
    lis      r4, activeAnaNum@ha
    lis      r3, lbl_7_rodata_F0@ha
    addi     r5, r4, activeAnaNum@l
    fmr      f1, f28
    lwz      r4, 0x0(r5)
    fmr      f2, f29
    fmr      f3, f30
    addi     r3, r3, lbl_7_rodata_F0@l
    addi     r0, r4, 0x1
    stw      r0, 0x0(r5)
    crset    cr1eq
    bl       OSReport
    mulli    r5, r30, 0x28
    lis      r4, anas@ha
    lis      r3, __vt__4CVec@ha
    addi     r0, r4, anas@l
    add      r5, r0, r5
    addi     r0, r3, __vt__4CVec@l
    lwz      r4, 0x14(r5)
    mr       r3, r30
    stw      r0, 0x14(r1)
    ori      r0, r4, 0x1
    stw      r0, 0x14(r5)
    stfs     f28, 0x4(r5)
    stfs     f29, 0x8(r5)
    stfs     f30, 0xc(r5)
    stw      r30, 0x1c(r5)
    b        .L_00000C60
.L_00000C44:
    addi     r31, r31, 0x28
    addi     r30, r30, 0x1
    bdnz     .L_00000BC8
    lis      r4, __vt__4CVec@ha
    li       r3, -0x1
    addi     r0, r4, __vt__4CVec@l
    stw      r0, 0x14(r1)
.L_00000C60:
    psq_l    f31, 0x68(r1), 0, qr0
    lfd      f31, 0x60(r1)
    psq_l    f30, 0x58(r1), 0, qr0
    lfd      f30, 0x50(r1)
    psq_l    f29, 0x48(r1), 0, qr0
    lfd      f29, 0x40(r1)
    psq_l    f28, 0x38(r1), 0, qr0
    lfd      f28, 0x30(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r0, 0x74(r1)
    lwz      r29, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x70
    blr
*/

// no stage has this function's code: both calls are inlined and the linker dropped the rest. its
// string lies between Ana::Set's and AddAna's in retail, so it was compiled here
Ana* GetNearestAna(f32 x, f32 y, f32 z)
{
	Ana* nearest = NULL;
	u32 i;
	CVec pos(x, y, z);
#if !defined(VERSION_GGTP01)
	OSReport("%4.2f %4.2f %4.2f\n", x, y, z);
#endif
	for (i = 0; i < ARRAY_COUNT(anas); i++) {
		if (anas[i].mStatus.mFlags & 1) {
			f32 dist = PSVECDistance(&pos, &anas[i].mPos);
			if (nearest == NULL || dist < PSVECDistance(&pos, &nearest->mPos)) {
				nearest = &anas[i];
			}
		}
	}
	return nearest;
}

// takes the hole into use at a place. only its inlined copies are left, like GetNearestAna's
void Ana::Set(f32 x, f32 y, f32 z)
{
#if !defined(VERSION_GGTP01)
	OSReport("Ana::%4.2f, %4.2f, %4.2f\n", x, y, z);
#endif
	mStatus.mFlags |= 1;
	mPos.x = x;
	mPos.y = y;
	mPos.z = z;
}

/**
 * @note Address: stage04 .text 0x5EC
 * @note Size: 0x460
 */
extern "C" HSD_JObj* GetAnaJObj(s32 kind, s32 which)
{
	HSD_JObj* jobj0;
	HSD_JObj* jobj1;
	XObj* obj;

	if (tutiAnaObj == NULL) {
		return NULL;
	}
	jobj0 = tutiAnaObj->mJObj;
	jobj1 = NULL;
	if (tutiHutaObj != NULL) {
		jobj1 = tutiHutaObj->mJObj;
	}
	if (which < 0 || which >= ARRAY_COUNT(flowerObjs)) {
		which = 0;
	}
	obj = flowerObjs[which];
	switch (kind) {
	case 1:
		return jobj0;
	case 2:
		return jobj1;
	case 3: {
		if (obj == NULL) {
			return NULL;
		}
		s32 path[2] = { 0, 0 };
		return GetJointByPath(obj->mJObj, path, 1);
	}
	case 4: {
		if (obj == NULL) {
			return NULL;
		}
		s32 path[2] = { 1, 0 };
		return GetJointByPath(obj->mJObj, path, 1);
	}
	case 5: {
		if (obj == NULL) {
			return NULL;
		}
		s32 path[2] = { 2, 0 };
		return GetJointByPath(obj->mJObj, path, 1);
	}
	case 6:
		if (obj == NULL) {
			return NULL;
		}
		if (which == 0) {
			s32 path[2] = { 4, 0 };
			return GetJointByPath(obj->mJObj, path, 1);
		} else {
			s32 path[2] = { 3, 0 };
			return GetJointByPath(obj->mJObj, path, 1);
		}
	case 7:
		if (obj == NULL) {
			return NULL;
		}
		if (which == 0) {
			s32 path[2] = { 3, 0 };
			return GetJointByPath(obj->mJObj, path, 1);
		} else {
			s32 path[2] = { 4, 0 };
			return GetJointByPath(obj->mJObj, path, 1);
		}
	}
	return NULL;
}

/**
 * @note Address: stage04 .text 0x2F0
 * @note Size: 0x2FC
 */
extern "C" void Ana_Draw(Ana* ana)
{
	HSD_JObj* jobj = GetAnaJObj(ana->mJObjKind, ana->mJObjWhich);
	f32 x, y, z;

	if (jobj == NULL) {
		return;
	}
	if (ana->mStatus.mFlags & 8) {
		const CVec& pos = GetPlayerCarryJointWorldPosition(gGame.GetPlayer());
		x               = pos.x;
		y               = pos.y;
		z               = pos.z;
	} else {
		x = ana->mPos.x;
		y = ana->mPos.y;
		z = ana->mPos.z;
	}
	HSD_JObjSetTranslateX(jobj, x);
	HSD_JObjSetTranslateY(jobj, y);
	HSD_JObjSetTranslateZ(jobj, z);
	HSD_JObjSetRotationY(jobj, ana->mRotY);
	HSD_JObjDispAll(jobj, NULL, 7, 0);
}

/**
 * @note Address: stage04 .text 0x2A8
 * @note Size: 0x48
 */
// CVec::~CVec
