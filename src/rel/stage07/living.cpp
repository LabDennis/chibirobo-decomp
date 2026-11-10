#include "Version.h"
#include "game/cjobj.h"
#include "game/cjobjset.h"

extern "C" f32 PSVECDistance(Vec* a, Vec* b);

// unused, but they sit at the start of this file's rodata
static const f32 sStage07VectorTemplates[2][9] = {
	{ 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
	{ 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
};

// CGame
extern "C" u8 gGame[];

#define MAINGAME_AT(type, offset) (*(type*)(gGame + (offset)))

#if defined(VERSION_JP)
#define STAGE07_FLAG_LIST   MAINGAME_AT(void*, 0xE7114)
#define STAGE07_SOUND       (&MAINGAME_AT(u8, 0xE713C))
#define STAGE07_FIELD_E5DDC (&MAINGAME_AT(Vec, 0xE50F4))
#else
#define STAGE07_FLAG_LIST   MAINGAME_AT(void*, 0xE7DFC)
#define STAGE07_SOUND       (&MAINGAME_AT(u8, 0xE7E24))
#define STAGE07_FIELD_E5DDC (&MAINGAME_AT(Vec, 0xE5DDC))
#endif

extern "C" void fn_801469A8(void* obj, s32 value);
extern "C" s32 IsGameSoundPlaying(void* obj, s32 id);
extern "C" s32 GetSoundVolumeAtPosition(void* obj, Vec* pos, s32 arg);
extern "C" void SetGameSfxVolume(void* obj, s32 id, s32 arg, s32 arg2);

// in rel/trash.cpp and rel/fafa.cpp
extern "C" void DrawTrashStack(void* arg);
extern "C" void UpdateTrash(void* arg);
extern "C" void InitTrash(void* arg);
extern "C" s32 CmdIsNearTrashbox(s32 cmd, s32 arg, s32* arg2);
extern "C" s32 CmdCancelTrash(s32 cmd, s32 arg, s32* arg2);
extern "C" s32 CmdEndTrash(s32 cmd, s32 arg, s32* arg2);
extern "C" s32 CmdGetTrashResult(s32 cmd, s32 arg, s32* arg2);
extern "C" s32 CmdStartTrash(s32 cmd, s32 arg, s32* arg2);
extern "C" void SetFafaPartVisibility(s32 mode);

// one child step then path[i] sibling steps per level
static inline HSD_JObj* GetJObjByPath(HSD_JObj* jobj, s32* path, s32 depth)
{
	s32 i;
	s32 j;

	if (jobj == NULL) {
		return NULL;
	}
	for (i = 0; i < depth; i++, path++) {
		jobj = jobj == NULL ? NULL : jobj->child;
		for (j = 0; j < *path; j++) {
			jobj = jobj == NULL ? NULL : jobj->next;
			if (jobj == NULL) {
				return NULL;
			}
		}
	}
	return jobj;
}

struct Stage07Actor {
#if defined(VERSION_JP)
	u8 _00[0xC0]; // _00
#else
	u8 _00[0xC8]; // _00
#endif
	Vec _C8;       // _C8 (JP: _C0)
	u8 _D4[0x154]; // _D4 (JP: _CC)
	u64 _228;      // _228 (JP: _220)
	u8 _230[0x8];  // _230 (JP: _228)
	s32* _238;     // _238 (JP: _230)
};

struct Stage07Game {
#if defined(VERSION_JP)
	u8 _00[0x9804]; // _00
#else
	u8 _00[0x984C]; // _00
#endif
	u8* mPool;                    // _984C (JP: _9804)
	u8 _9850[0xC];                // _9850 (JP: _9808)
	s32 mPoolEntrySize;           // _985C (JP: _9814)
	Stage07Actor* mActors[0x400]; // _9860 (JP: _9818)
	s32 mActorCount;              // _A860 (JP: _A818)

	Stage07Actor* GetActor(s32 index) const { return mActors[index]; }
};

#define STAGE07_GAME ((Stage07Game*)gGame)

struct Stage07PoolEntry {
	HSD_JObj* jobj; // _00
#if defined(VERSION_JP)
	u8 _04[0x238]; // _04
#else
	u8 _04[0x240]; // _04
#endif
	s32* _244; // _244 (JP: _23C)
};

extern "C" s32 CmdAddObjFlagList(s32 cmd, s32 count, s32* values);
extern "C" void SetObj37PartVisibility(s32 mode);
extern "C" s32 CmdSetObj37PartVisibility(s32 cmd, s32 arg, s32* arg2);
extern "C" s32 CmdSetFafaPartVisibility(s32 cmd, s32 arg, s32* arg2);
extern "C" s32 CmdNop(s32 cmd, s32 arg, s32* arg2);
extern "C" void DrawStage(void* arg);
extern "C" void UpdateStage(void* arg);
extern "C" void InitStage(void* arg);

typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 arg, s32* arg2);

extern "C" {
RelSlotFunc _prolog_slots[7] = { InitStage, UpdateStage, DrawStage };
RelCmdFunc _prolog_cmds[]    = {
    CmdNop,
    CmdSetFafaPartVisibility,
    CmdSetObj37PartVisibility,
    CmdStartTrash,
    CmdGetTrashResult,
    CmdEndTrash,
    (RelCmdFunc)CmdAddObjFlagList,
    CmdCancelTrash,
    CmdIsNearTrashbox,
};
}

/**
 * @note Address: stage07 .text 0x0
 * @note Size: 0x78
 */
extern "C" s32 CmdAddObjFlagList(s32 cmd, s32 count, s32* values)
{
	s32 i;

	for (i = 0; i < count; i++) {
		fn_801469A8(STAGE07_FLAG_LIST, values[i]);
	}
	return 0;
}

/**
 * @note Address: stage07 .text 0x78
 * @note Size: 0xF10
 */
extern "C" void SetObj37PartVisibility(s32 mode)
{
	CJObj jobj;
	s32 size = STAGE07_GAME->mPoolEntrySize;
	u8* pool = STAGE07_GAME->mPool;
	s32 i;

	for (i = 0; i < 0x120; i++) {
		Stage07PoolEntry* entry = (Stage07PoolEntry*)(pool + i * size);
		s32* type               = entry->_244;
		if ((type != NULL ? *type : -1) != 0x37) {
			continue;
		}
		switch (mode) {
		case 0: {
			s32 path0[2] = { 0, 1 };
			jobj.Attach(GetJObjByPath(entry->jobj, path0, 2));
			HSD_JObjSetFlags(jobj.mJObj, HIDDEN);
			s32 path1[2] = { 0, 2 };
			jobj.Attach(GetJObjByPath(entry->jobj, path1, 2));
			HSD_JObjSetFlags(jobj.mJObj, HIDDEN);
			break;
		}
		case 1: {
			s32 path0[2] = { 0, 1 };
			jobj.Attach(GetJObjByPath(entry->jobj, path0, 2));
			HSD_JObjClearFlags(jobj.mJObj, HIDDEN);
			s32 path1[2] = { 0, 2 };
			jobj.Attach(GetJObjByPath(entry->jobj, path1, 2));
			HSD_JObjSetFlags(jobj.mJObj, HIDDEN);
			break;
		}
		case 2: {
			s32 path0[2] = { 0, 1 };
			jobj.Attach(GetJObjByPath(entry->jobj, path0, 2));
			HSD_JObjSetFlags(jobj.mJObj, HIDDEN);
			s32 path1[2] = { 0, 2 };
			jobj.Attach(GetJObjByPath(entry->jobj, path1, 2));
			HSD_JObjClearFlags(jobj.mJObj, HIDDEN);
			break;
		}
		case 3: {
			s32 path0[2] = { 0, 1 };
			jobj.Attach(GetJObjByPath(entry->jobj, path0, 2));
			HSD_JObjClearFlags(jobj.mJObj, HIDDEN);
			s32 path1[2] = { 0, 2 };
			jobj.Attach(GetJObjByPath(entry->jobj, path1, 2));
			HSD_JObjClearFlags(jobj.mJObj, HIDDEN);
			break;
		}
		}
		break;
	}
}

/**
 * @note Address: stage07 .text 0xF88
 * @note Size: 0x88
 */
// CJObj::~CJObj, inline

/**
 * @note Address: stage07 .text 0x1010
 * @note Size: 0x48
 */
// CVec::~CVec, inline

/**
 * @note Address: stage07 .text 0x1058
 * @note Size: 0x28
 */
extern "C" s32 CmdSetObj37PartVisibility(s32 cmd, s32 arg, s32* arg2)
{
	SetObj37PartVisibility(arg2[0]);
	return 0;
}

/**
 * @note Address: stage07 .text 0x1080
 * @note Size: 0x28
 */
extern "C" s32 CmdSetFafaPartVisibility(s32 cmd, s32 arg, s32* arg2)
{
	SetFafaPartVisibility(arg2[0]);
	return 0;
}

/**
 * @note Address: stage07 .text 0x10A8
 * @note Size: 0x8
 */
extern "C" s32 CmdNop(s32 cmd, s32 arg, s32* arg2)
{
	return 0;
}

/**
 * @note Address: stage07 .text 0x10B0
 * @note Size: 0x20
 */
extern "C" void DrawStage(void* arg)
{
	DrawTrashStack(arg);
}

/**
 * @note Address: stage07 .text 0x10D0
 * @note Size: 0x110
 */
extern "C" void UpdateStage(void* arg)
{
	Stage07Actor* actor;
	void* obj = STAGE07_SOUND;
	s32 i;

	if (IsGameSoundPlaying(obj, 0x1000D)) {
		for (i = 0; i < STAGE07_GAME->mActorCount; i++) {
			actor = STAGE07_GAME->GetActor(i);
			if ((actor->_228 & 1) && (actor->_228 & 0x10000) && *actor->_238 == 0xD0) {
				PSVECDistance(STAGE07_FIELD_E5DDC, &actor->_C8);
				SetGameSfxVolume(obj, 0x1000D, GetSoundVolumeAtPosition(obj, &actor->_C8, 1), 0);
			}
		}
	}
	UpdateTrash(arg);
}
// UpdateStage
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    lis      r5, gGame@ha
    lis      r4, 0x1
    stw      r0, 0x34(r1)
    addi     r5, r5, gGame@l
    addi     r4, r4, 0xd
    stmw     r25, 0x14(r1)
    addis    r27, r5, 0xe
    addi     r27, r27, 0x7e24
    mr       r25, r3
    mr       r3, r27
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_000011C4
    lis      r3, gGame@ha
    li       r26, 0x0
    addi     r30, r3, gGame@l
    addis    r29, r30, 0xe
    addis    r28, r30, 0x1
    b        .L_000011B8
.L_00001124:
    addis    r3, r30, 0x1
    li       r0, 0x1
    lwz      r31, -0x67a0(r3)
    li       r5, 0x0
    lwz      r3, 0x228(r31)
    lwz      r6, 0x22c(r31)
    and      r4, r3, r5
    and      r0, r6, r0
    xor      r3, r0, r5
    xor      r0, r4, r5
    or.      r0, r3, r0
    beq      .L_000011B0
    lis      r3, 0x1
    xor      r0, r4, r5
    and      r3, r6, r3
    xor      r3, r3, r5
    or.      r0, r3, r0
    beq      .L_000011B0
    lwz      r3, 0x238(r31)
    lwz      r0, 0x0(r3)
    cmpwi    r0, 0xd0
    bne      .L_000011B0
    addi     r3, r29, 0x5ddc
    addi     r4, r31, 0xc8
    bl       PSVECDistance
    mr       r3, r27
    addi     r4, r31, 0xc8
    li       r5, 0x1
    bl       GetSoundVolumeAtPosition
    lis      r4, 0x1
    mr       r5, r3
    mr       r3, r27
    li       r6, 0x0
    addi     r4, r4, 0xd
    bl       SetGameSfxVolume
.L_000011B0:
    addi     r30, r30, 0x4
    addi     r26, r26, 0x1
.L_000011B8:
    lwz      r0, -0x57a0(r28)
    cmpw     r26, r0
    blt      .L_00001124
.L_000011C4:
    mr       r3, r25
    bl       UpdateTrash
    lmw      r25, 0x14(r1)
    lwz      r0, 0x34(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: stage07 .text 0x11E0
 * @note Size: 0x20
 */
extern "C" void InitStage(void* arg)
{
	InitTrash(arg);
}

/**
 * @note Address: stage07 .text 0x1200
 * @note Size: 0xD4
 */
// CJObj::SetTranslateX, inline

/**
 * @note Address: stage07 .text 0x12D4
 * @note Size: 0xD4
 */
// CJObj::SetTranslateY, inline

/**
 * @note Address: stage07 .text 0x13A8
 * @note Size: 0xD4
 */
// CJObj::SetTranslateZ, inline

/**
 * @note Address: stage07 .text 0x147C
 * @note Size: 0xD4
 */
// CJObj::SetScaleX, inline

/**
 * @note Address: stage07 .text 0x1550
 * @note Size: 0xD4
 */
// CJObj::SetScaleY, inline

/**
 * @note Address: stage07 .text 0x1624
 * @note Size: 0xD4
 */
// CJObj::SetScaleZ, inline

/**
 * @note Address: stage07 .text 0x16F8
 * @note Size: 0x1A8
 */
// CJObj::SetRotationX, inline
// SetRotationX
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    stw      r0, 0x44(r1)
    stfd     f31, 0x30(r1)
    psq_st   f31, 0x38(r1), 0, qr0
    stw      r31, 0x2c(r1)
    stw      r30, 0x28(r1)
    stw      r29, 0x24(r1)
    fmr      f31, f1
    cmpwi    r4, 0x0
    lis      r4, sStage07VectorTemplates@ha
    mr       r29, r3
    addi     r31, r4, sStage07VectorTemplates@l
    bne      .L_0000173C
    lfs      f0, 0x14(r29)
    fcmpu    cr0, f31, f0
    beq      .L_0000187C
.L_0000173C:
    stfs     f31, 0x14(r29)
    lwz      r30, 0x0(r29)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_000017F0
    addi     r3, r29, 0x14
    addi     r4, r1, 0x8
    bl       EulerToQuat
    lwz      r30, 0x0(r29)
    cmplwi   r30, 0x0
    bne      .L_00001778
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x290
    bl       __assert
.L_00001778:
    lfs      f0, 0x8(r1)
    stfs     f0, 0x1c(r30)
    lfs      f0, 0xc(r1)
    stfs     f0, 0x20(r30)
    lfs      f0, 0x10(r1)
    stfs     f0, 0x24(r30)
    lfs      f0, 0x14(r1)
    stfs     f0, 0x28(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_0000187C
    cmplwi   r30, 0x0
    beq      .L_0000187C
    bne      .L_000017C0
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x25d
    bl       __assert
.L_000017C0:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_000017DC
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_000017DC
    li       r3, 0x1
.L_000017DC:
    clrlwi.  r0, r3, 24
    bne      .L_0000187C
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
    b        .L_0000187C
.L_000017F0:
    cmplwi   r30, 0x0
    bne      .L_00001808
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x2a4
    bl       __assert
.L_00001808:
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_00001824
    addi     r3, r31, 0x8c
    addi     r5, r31, 0xcc
    li       r4, 0x2a5
    bl       __assert
.L_00001824:
    stfs     f31, 0x1c(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_0000187C
    cmplwi   r30, 0x0
    beq      .L_0000187C
    bne      .L_00001850
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x25d
    bl       __assert
.L_00001850:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_0000186C
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_0000186C
    li       r3, 0x1
.L_0000186C:
    clrlwi.  r0, r3, 24
    bne      .L_0000187C
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_0000187C:
    psq_l    f31, 0x38(r1), 0, qr0
    lwz      r0, 0x44(r1)
    lfd      f31, 0x30(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r29, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

/**
 * @note Address: stage07 .text 0x18A0
 * @note Size: 0x1A8
 */
// CJObj::SetRotationY, inline
// SetRotationY
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    stw      r0, 0x44(r1)
    stfd     f31, 0x30(r1)
    psq_st   f31, 0x38(r1), 0, qr0
    stw      r31, 0x2c(r1)
    stw      r30, 0x28(r1)
    stw      r29, 0x24(r1)
    fmr      f31, f1
    cmpwi    r4, 0x0
    lis      r4, sStage07VectorTemplates@ha
    mr       r29, r3
    addi     r31, r4, sStage07VectorTemplates@l
    bne      .L_000018E4
    lfs      f0, 0x18(r29)
    fcmpu    cr0, f31, f0
    beq      .L_00001A24
.L_000018E4:
    stfs     f31, 0x18(r29)
    lwz      r30, 0x0(r29)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_00001998
    addi     r3, r29, 0x14
    addi     r4, r1, 0x8
    bl       EulerToQuat
    lwz      r30, 0x0(r29)
    cmplwi   r30, 0x0
    bne      .L_00001920
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x290
    bl       __assert
.L_00001920:
    lfs      f0, 0x8(r1)
    stfs     f0, 0x1c(r30)
    lfs      f0, 0xc(r1)
    stfs     f0, 0x20(r30)
    lfs      f0, 0x10(r1)
    stfs     f0, 0x24(r30)
    lfs      f0, 0x14(r1)
    stfs     f0, 0x28(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_00001A24
    cmplwi   r30, 0x0
    beq      .L_00001A24
    bne      .L_00001968
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x25d
    bl       __assert
.L_00001968:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00001984
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00001984
    li       r3, 0x1
.L_00001984:
    clrlwi.  r0, r3, 24
    bne      .L_00001A24
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
    b        .L_00001A24
.L_00001998:
    cmplwi   r30, 0x0
    bne      .L_000019B0
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x2b8
    bl       __assert
.L_000019B0:
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_000019CC
    addi     r3, r31, 0x8c
    addi     r5, r31, 0xcc
    li       r4, 0x2b9
    bl       __assert
.L_000019CC:
    stfs     f31, 0x20(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_00001A24
    cmplwi   r30, 0x0
    beq      .L_00001A24
    bne      .L_000019F8
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x25d
    bl       __assert
.L_000019F8:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00001A14
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00001A14
    li       r3, 0x1
.L_00001A14:
    clrlwi.  r0, r3, 24
    bne      .L_00001A24
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_00001A24:
    psq_l    f31, 0x38(r1), 0, qr0
    lwz      r0, 0x44(r1)
    lfd      f31, 0x30(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r29, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

/**
 * @note Address: stage07 .text 0x1A48
 * @note Size: 0x1A8
 */
// CJObj::SetRotationZ, inline
// SetRotationZ
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    stw      r0, 0x44(r1)
    stfd     f31, 0x30(r1)
    psq_st   f31, 0x38(r1), 0, qr0
    stw      r31, 0x2c(r1)
    stw      r30, 0x28(r1)
    stw      r29, 0x24(r1)
    fmr      f31, f1
    cmpwi    r4, 0x0
    lis      r4, sStage07VectorTemplates@ha
    mr       r29, r3
    addi     r31, r4, sStage07VectorTemplates@l
    bne      .L_00001A8C
    lfs      f0, 0x1c(r29)
    fcmpu    cr0, f31, f0
    beq      .L_00001BCC
.L_00001A8C:
    stfs     f31, 0x1c(r29)
    lwz      r30, 0x0(r29)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_00001B40
    addi     r3, r29, 0x14
    addi     r4, r1, 0x8
    bl       EulerToQuat
    lwz      r30, 0x0(r29)
    cmplwi   r30, 0x0
    bne      .L_00001AC8
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x290
    bl       __assert
.L_00001AC8:
    lfs      f0, 0x8(r1)
    stfs     f0, 0x1c(r30)
    lfs      f0, 0xc(r1)
    stfs     f0, 0x20(r30)
    lfs      f0, 0x10(r1)
    stfs     f0, 0x24(r30)
    lfs      f0, 0x14(r1)
    stfs     f0, 0x28(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_00001BCC
    cmplwi   r30, 0x0
    beq      .L_00001BCC
    bne      .L_00001B10
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x25d
    bl       __assert
.L_00001B10:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00001B2C
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00001B2C
    li       r3, 0x1
.L_00001B2C:
    clrlwi.  r0, r3, 24
    bne      .L_00001BCC
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
    b        .L_00001BCC
.L_00001B40:
    cmplwi   r30, 0x0
    bne      .L_00001B58
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x2cc
    bl       __assert
.L_00001B58:
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_00001B74
    addi     r3, r31, 0x8c
    addi     r5, r31, 0xcc
    li       r4, 0x2cd
    bl       __assert
.L_00001B74:
    stfs     f31, 0x24(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_00001BCC
    cmplwi   r30, 0x0
    beq      .L_00001BCC
    bne      .L_00001BA0
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x25d
    bl       __assert
.L_00001BA0:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00001BBC
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00001BBC
    li       r3, 0x1
.L_00001BBC:
    clrlwi.  r0, r3, 24
    bne      .L_00001BCC
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_00001BCC:
    psq_l    f31, 0x38(r1), 0, qr0
    lwz      r0, 0x44(r1)
    lfd      f31, 0x30(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r29, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

/**
 * @note Address: stage07 .text 0x1BF0
 * @note Size: 0x178
 */
// CJObj::SetTranslate, the three float one, inline
// SetTranslate
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    cmpwi    r4, 0x0
    lis      r4, sStage07VectorTemplates@ha
    stw      r0, 0x34(r1)
    stw      r31, 0x2c(r1)
    addi     r31, r4, sStage07VectorTemplates@l
    stw      r30, 0x28(r1)
    stw      r29, 0x24(r1)
    mr       r29, r3
    bne      .L_00001CA0
    lis      r3, gJointTranslateZero@ha
    lfs      f4, 0x4(r29)
    addi     r4, r3, gJointTranslateZero@l
    li       r0, 0x0
    lwz      r3, 0x0(r4)
    lwz      r5, 0x4(r4)
    stw      r3, 0x8(r1)
    lwz      r4, 0x8(r4)
    stfs     f1, 0x8(r1)
    lwz      r3, 0x8(r1)
    stw      r5, 0xc(r1)
    stw      r4, 0x10(r1)
    stw      r3, 0x14(r1)
    stfs     f2, 0xc(r1)
    lfs      f0, 0x14(r1)
    stfs     f3, 0x10(r1)
    lwz      r4, 0xc(r1)
    fcmpu    cr0, f4, f0
    lwz      r3, 0x10(r1)
    stw      r4, 0x18(r1)
    stw      r3, 0x1c(r1)
    bne      .L_00001C94
    lfs      f4, 0x8(r29)
    lfs      f0, 0x18(r1)
    fcmpu    cr0, f4, f0
    bne      .L_00001C94
    lfs      f4, 0xc(r29)
    lfs      f0, 0x1c(r1)
    fcmpu    cr0, f4, f0
    beq      .L_00001C98
.L_00001C94:
    li       r0, 0x1
.L_00001C98:
    clrlwi.  r0, r0, 24
    beq      .L_00001D4C
.L_00001CA0:
    stfs     f1, 0x4(r29)
    stfs     f2, 0x8(r29)
    stfs     f3, 0xc(r29)
    lwz      r30, 0x0(r29)
    cmplwi   r30, 0x0
    bne      .L_00001CC8
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x3a9
    bl       __assert
.L_00001CC8:
    addic.   r0, r29, 0x4
    bne      .L_00001CE0
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x9c
    li       r4, 0x3aa
    bl       __assert
.L_00001CE0:
    lfs      f0, 0x4(r29)
    stfs     f0, 0x38(r30)
    lfs      f0, 0x8(r29)
    stfs     f0, 0x3c(r30)
    lfs      f0, 0xc(r29)
    stfs     f0, 0x40(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_00001D4C
    cmplwi   r30, 0x0
    beq      .L_00001D4C
    bne      .L_00001D20
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x25d
    bl       __assert
.L_00001D20:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00001D3C
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00001D3C
    li       r3, 0x1
.L_00001D3C:
    clrlwi.  r0, r3, 24
    bne      .L_00001D4C
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_00001D4C:
    lwz      r0, 0x34(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r29, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: stage07 .text 0x1D68
 * @note Size: 0x158
 */
// CJObj::SetTranslate, the vector one, inline
// SetTranslate
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    cmpwi    r5, 0x0
    lis      r5, sStage07VectorTemplates@ha
    stw      r0, 0x34(r1)
    stw      r31, 0x2c(r1)
    addi     r31, r5, sStage07VectorTemplates@l
    stw      r30, 0x28(r1)
    stw      r29, 0x24(r1)
    mr       r29, r3
    bne      .L_00001DEC
    lwz      r3, 0x0(r4)
    li       r0, 0x0
    lwz      r5, 0x4(r4)
    stw      r3, 0x8(r1)
    lwz      r3, 0x8(r4)
    lfs      f1, 0x4(r29)
    lfs      f0, 0x8(r1)
    stw      r5, 0xc(r1)
    fcmpu    cr0, f1, f0
    stw      r3, 0x10(r1)
    bne      .L_00001DE0
    lfs      f1, 0x8(r29)
    lfs      f0, 0xc(r1)
    fcmpu    cr0, f1, f0
    bne      .L_00001DE0
    lfs      f1, 0xc(r29)
    lfs      f0, 0x10(r1)
    fcmpu    cr0, f1, f0
    beq      .L_00001DE4
.L_00001DE0:
    li       r0, 0x1
.L_00001DE4:
    clrlwi.  r0, r0, 24
    beq      .L_00001EA4
.L_00001DEC:
    lfs      f0, 0x0(r4)
    lfs      f1, 0x4(r4)
    stfs     f0, 0x4(r29)
    lfs      f0, 0x8(r4)
    stfs     f1, 0x8(r29)
    stfs     f0, 0xc(r29)
    lwz      r30, 0x0(r29)
    cmplwi   r30, 0x0
    bne      .L_00001E20
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x3a9
    bl       __assert
.L_00001E20:
    addic.   r0, r29, 0x4
    bne      .L_00001E38
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x9c
    li       r4, 0x3aa
    bl       __assert
.L_00001E38:
    lfs      f0, 0x4(r29)
    stfs     f0, 0x38(r30)
    lfs      f0, 0x8(r29)
    stfs     f0, 0x3c(r30)
    lfs      f0, 0xc(r29)
    stfs     f0, 0x40(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_00001EA4
    cmplwi   r30, 0x0
    beq      .L_00001EA4
    bne      .L_00001E78
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x25d
    bl       __assert
.L_00001E78:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00001E94
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00001E94
    li       r3, 0x1
.L_00001E94:
    clrlwi.  r0, r3, 24
    bne      .L_00001EA4
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_00001EA4:
    lwz      r0, 0x34(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r29, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: stage07 .text 0x1EC0
 * @note Size: 0x178
 */
// CJObj::SetScale, the three float one, inline
// SetScale
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    cmpwi    r4, 0x0
    lis      r4, sStage07VectorTemplates@ha
    stw      r0, 0x34(r1)
    stw      r31, 0x2c(r1)
    addi     r31, r4, sStage07VectorTemplates@l
    stw      r30, 0x28(r1)
    stw      r29, 0x24(r1)
    mr       r29, r3
    bne      .L_00001F70
    lis      r3, gJointScaleZero@ha
    lfs      f4, 0x24(r29)
    addi     r4, r3, gJointScaleZero@l
    li       r0, 0x0
    lwz      r3, 0x0(r4)
    lwz      r5, 0x4(r4)
    stw      r3, 0x8(r1)
    lwz      r4, 0x8(r4)
    stfs     f1, 0x8(r1)
    lwz      r3, 0x8(r1)
    stw      r5, 0xc(r1)
    stw      r4, 0x10(r1)
    stw      r3, 0x14(r1)
    stfs     f2, 0xc(r1)
    lfs      f0, 0x14(r1)
    stfs     f3, 0x10(r1)
    lwz      r4, 0xc(r1)
    fcmpu    cr0, f4, f0
    lwz      r3, 0x10(r1)
    stw      r4, 0x18(r1)
    stw      r3, 0x1c(r1)
    bne      .L_00001F64
    lfs      f4, 0x28(r29)
    lfs      f0, 0x18(r1)
    fcmpu    cr0, f4, f0
    bne      .L_00001F64
    lfs      f4, 0x2c(r29)
    lfs      f0, 0x1c(r1)
    fcmpu    cr0, f4, f0
    beq      .L_00001F68
.L_00001F64:
    li       r0, 0x1
.L_00001F68:
    clrlwi.  r0, r0, 24
    beq      .L_0000201C
.L_00001F70:
    stfs     f1, 0x24(r29)
    stfs     f2, 0x28(r29)
    stfs     f3, 0x2c(r29)
    lwz      r30, 0x0(r29)
    cmplwi   r30, 0x0
    bne      .L_00001F98
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x316
    bl       __assert
.L_00001F98:
    addic.   r0, r29, 0x24
    bne      .L_00001FB0
    addi     r3, r31, 0x8c
    addi     r5, r31, 0xb0
    li       r4, 0x317
    bl       __assert
.L_00001FB0:
    lfs      f0, 0x24(r29)
    stfs     f0, 0x2c(r30)
    lfs      f0, 0x28(r29)
    stfs     f0, 0x30(r30)
    lfs      f0, 0x2c(r29)
    stfs     f0, 0x34(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_0000201C
    cmplwi   r30, 0x0
    beq      .L_0000201C
    bne      .L_00001FF0
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x25d
    bl       __assert
.L_00001FF0:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_0000200C
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_0000200C
    li       r3, 0x1
.L_0000200C:
    clrlwi.  r0, r3, 24
    bne      .L_0000201C
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_0000201C:
    lwz      r0, 0x34(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r29, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: stage07 .text 0x2038
 * @note Size: 0x158
 */
// CJObj::SetScale, the vector one, inline
// SetScale
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    cmpwi    r5, 0x0
    lis      r5, sStage07VectorTemplates@ha
    stw      r0, 0x34(r1)
    stw      r31, 0x2c(r1)
    addi     r31, r5, sStage07VectorTemplates@l
    stw      r30, 0x28(r1)
    stw      r29, 0x24(r1)
    mr       r29, r3
    bne      .L_000020BC
    lwz      r3, 0x0(r4)
    li       r0, 0x0
    lwz      r5, 0x4(r4)
    stw      r3, 0x8(r1)
    lwz      r3, 0x8(r4)
    lfs      f1, 0x24(r29)
    lfs      f0, 0x8(r1)
    stw      r5, 0xc(r1)
    fcmpu    cr0, f1, f0
    stw      r3, 0x10(r1)
    bne      .L_000020B0
    lfs      f1, 0x28(r29)
    lfs      f0, 0xc(r1)
    fcmpu    cr0, f1, f0
    bne      .L_000020B0
    lfs      f1, 0x2c(r29)
    lfs      f0, 0x10(r1)
    fcmpu    cr0, f1, f0
    beq      .L_000020B4
.L_000020B0:
    li       r0, 0x1
.L_000020B4:
    clrlwi.  r0, r0, 24
    beq      .L_00002174
.L_000020BC:
    lfs      f0, 0x0(r4)
    lfs      f1, 0x4(r4)
    stfs     f0, 0x24(r29)
    lfs      f0, 0x8(r4)
    stfs     f1, 0x28(r29)
    stfs     f0, 0x2c(r29)
    lwz      r30, 0x0(r29)
    cmplwi   r30, 0x0
    bne      .L_000020F0
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x316
    bl       __assert
.L_000020F0:
    addic.   r0, r29, 0x24
    bne      .L_00002108
    addi     r3, r31, 0x8c
    addi     r5, r31, 0xb0
    li       r4, 0x317
    bl       __assert
.L_00002108:
    lfs      f0, 0x24(r29)
    stfs     f0, 0x2c(r30)
    lfs      f0, 0x28(r29)
    stfs     f0, 0x30(r30)
    lfs      f0, 0x2c(r29)
    stfs     f0, 0x34(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_00002174
    cmplwi   r30, 0x0
    beq      .L_00002174
    bne      .L_00002148
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x25d
    bl       __assert
.L_00002148:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00002164
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00002164
    li       r3, 0x1
.L_00002164:
    clrlwi.  r0, r3, 24
    bne      .L_00002174
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_00002174:
    lwz      r0, 0x34(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r29, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: stage07 .text 0x2190
 * @note Size: 0x22C
 */
// CJObj::SetRotation, the three float one, inline
// SetRotation
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    cmpwi    r4, 0x0
    lis      r4, sStage07VectorTemplates@ha
    stw      r0, 0x44(r1)
    stw      r31, 0x3c(r1)
    addi     r31, r4, sStage07VectorTemplates@l
    stw      r30, 0x38(r1)
    stw      r29, 0x34(r1)
    mr       r29, r3
    bne      .L_00002240
    lis      r3, gJointRotationZero@ha
    lfs      f4, 0x4(r29)
    addi     r4, r3, gJointRotationZero@l
    li       r0, 0x0
    lwz      r3, 0x0(r4)
    lwz      r5, 0x4(r4)
    stw      r3, 0x8(r1)
    lwz      r4, 0x8(r4)
    stfs     f1, 0x8(r1)
    lwz      r3, 0x8(r1)
    stw      r5, 0xc(r1)
    stw      r4, 0x10(r1)
    stw      r3, 0x14(r1)
    stfs     f2, 0xc(r1)
    lfs      f0, 0x14(r1)
    stfs     f3, 0x10(r1)
    lwz      r4, 0xc(r1)
    fcmpu    cr0, f4, f0
    lwz      r3, 0x10(r1)
    stw      r4, 0x18(r1)
    stw      r3, 0x1c(r1)
    bne      .L_00002234
    lfs      f4, 0x8(r29)
    lfs      f0, 0x18(r1)
    fcmpu    cr0, f4, f0
    bne      .L_00002234
    lfs      f4, 0xc(r29)
    lfs      f0, 0x1c(r1)
    fcmpu    cr0, f4, f0
    beq      .L_00002238
.L_00002234:
    li       r0, 0x1
.L_00002238:
    clrlwi.  r0, r0, 24
    beq      .L_000023A0
.L_00002240:
    stfs     f1, 0x14(r29)
    stfs     f2, 0x18(r29)
    stfs     f3, 0x1c(r29)
    lwz      r30, 0x0(r29)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_000022FC
    addi     r3, r29, 0x14
    addi     r4, r1, 0x20
    bl       EulerToQuat
    lwz      r30, 0x0(r29)
    cmplwi   r30, 0x0
    bne      .L_00002284
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x290
    bl       __assert
.L_00002284:
    lfs      f0, 0x20(r1)
    stfs     f0, 0x1c(r30)
    lfs      f0, 0x24(r1)
    stfs     f0, 0x20(r30)
    lfs      f0, 0x28(r1)
    stfs     f0, 0x24(r30)
    lfs      f0, 0x2c(r1)
    stfs     f0, 0x28(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_000023A0
    cmplwi   r30, 0x0
    beq      .L_000023A0
    bne      .L_000022CC
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x25d
    bl       __assert
.L_000022CC:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_000022E8
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_000022E8
    li       r3, 0x1
.L_000022E8:
    clrlwi.  r0, r3, 24
    bne      .L_000023A0
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
    b        .L_000023A0
.L_000022FC:
    cmplwi   r30, 0x0
    bne      .L_00002314
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x290
    bl       __assert
.L_00002314:
    addic.   r0, r29, 0x14
    bne      .L_0000232C
    addi     r3, r31, 0x8c
    addi     r5, r31, 0xa8
    li       r4, 0x291
    bl       __assert
.L_0000232C:
    lfs      f0, 0x14(r29)
    stfs     f0, 0x1c(r30)
    lfs      f0, 0x18(r29)
    stfs     f0, 0x20(r30)
    lfs      f0, 0x1c(r29)
    stfs     f0, 0x24(r30)
    lfs      f0, 0x20(r29)
    stfs     f0, 0x28(r30)
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_000023A0
    cmplwi   r30, 0x0
    beq      .L_000023A0
    bne      .L_00002374
    addi     r3, r31, 0x8c
    addi     r5, r31, 0x94
    li       r4, 0x25d
    bl       __assert
.L_00002374:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00002390
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00002390
    li       r3, 0x1
.L_00002390:
    clrlwi.  r0, r3, 24
    bne      .L_000023A0
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_000023A0:
    lwz      r0, 0x44(r1)
    lwz      r31, 0x3c(r1)
    lwz      r30, 0x38(r1)
    lwz      r29, 0x34(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

/**
 * @note Address: stage07 .text 0x23BC
 * @note Size: 0x20C
 */
// CJObj::SetRotation, the vector one, inline

/**
 * @note Address: stage07 .text 0x25C8
 * @note Size: 0x80
 */
// CJObj::SetAnimRate, inline
