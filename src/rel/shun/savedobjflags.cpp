#include "types.h"
#include "Version.h"

extern "C" u8 gGame[];

struct ShunObj {
#if defined(VERSION_GGTJ01_01)
	u8 _00[0x220]; // _00
#else
	u8 _00[0x228]; // _00
#endif
	u64 mFlags; // _228, JP _220
};
#if defined(VERSION_GGTJ01_01)
typedef char ShunObjViewSizeCheck[sizeof(ShunObj) == 0x228 ? 1 : -1];
#else
typedef char ShunObjViewSizeCheck[sizeof(ShunObj) == 0x230 ? 1 : -1];
#endif

struct ShunObjList {
	void* _00; // _00
};

struct SavedObjFlag {
	s32 mId;  // _00
	u8 mFlag; // _04
};

struct SavedObjFlagList {
	void Apply(s32 mode);

	SavedObjFlag mEntries[0x100]; // _00
	s32 mCount;                   // _800
};

extern "C" void SetJunkUnkoFlag(u8 on);
extern "C" ShunObj* ResolveEventObjectId(ShunObjList* list, s16 id);

#if defined(VERSION_GGTJ01_01)
#define SHUN_OBJ_LIST() ((ShunObjList*)(gGame + 0x5D038))
#else
#define SHUN_OBJ_LIST() ((ShunObjList*)(gGame + 0x5D080))
#endif

static SavedObjFlagList savedObjFlags;

inline ShunObj* ShunFindObj(s16 id)
{
	ShunObjList* list = SHUN_OBJ_LIST();
	if (list->_00 != NULL) {
		return ResolveEventObjectId(list, id);
	}
	return NULL;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
inline void SavedObjFlagList::Apply(s32 mode)
{
	s32 i;
	ShunObj* obj;

	if (mode != 0) {
		if (mCount > 0) {
			s32 j;
			for (j = 0; j < mCount; j++) {
				if (mEntries[j].mId == -1) {
					break;
				}
				obj = ShunFindObj(mEntries[j].mId);
				if (mEntries[j].mFlag & 1) {
					obj->mFlags |= 0x10000;
				}
			}
		}
	} else {
		if (mCount > 0) {
			for (i = 0; i < mCount; i++) {
				if (mEntries[i].mId == -1) {
					break;
				}
				obj = ShunFindObj(mEntries[i].mId);
				obj->mFlags &= ~0x10000ULL;
			}
		}
	}
}

/**
 * @note Address: shun .text 0x0
 * @note Size: 0x40
 */
extern "C" s32 CmdSetJunkUnkoFlag(s32 cmd, s32 count, s32* args)
{
	if (args[0] != 0) {
		SetJunkUnkoFlag(1);
	} else {
		SetJunkUnkoFlag(0);
	}
	return 0;
}

/**
 * @note Address: shun .text 0x40
 * @note Size: 0x158
 */
extern "C" s32 CmdApplySavedObjFlags(s32 cmd, s32 count, s32* args)
{
	savedObjFlags.Apply(args[0]);
	return 0;
}
// CmdApplySavedObjFlags
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stmw     r27, 0xc(r1)
    lwz      r0, 0x0(r5)
    cmpwi    r0, 0x0
    beq      .L_000000EC
    lis      r3, savedObjFlags@ha
    addi     r30, r3, savedObjFlags@l
    lwz      r0, 0x800(r30)
    cmpwi    r0, 0x0
    ble      .L_00000180
    lis      r3, gGame@ha
    mr       r29, r30
    addi     r3, r3, gGame@l
    li       r31, 0x0
    addis    r28, r3, 0x6
    subi     r28, r28, 0x2f80
    b        .L_000000DC
.L_0000008C:
    lwz      r3, 0x0(r29)
    cmpwi    r3, -0x1
    beq      .L_00000180
    lwz      r0, 0x0(r28)
    extsh    r4, r3
    cmplwi   r0, 0x0
    beq      .L_000000B4
    mr       r3, r28
    bl       ResolveEventObjectId
    b        .L_000000B8
.L_000000B4:
    li       r3, 0x0
.L_000000B8:
    lbz      r0, 0x4(r29)
    clrlwi.  r0, r0, 31
    beq      .L_000000D4
    lwz      r5, 0x22c(r3)
    lis      r0, 0x1
    or       r0, r5, r0
    stw      r0, 0x22c(r3)
.L_000000D4:
    addi     r29, r29, 0x8
    addi     r31, r31, 0x1
.L_000000DC:
    lwz      r0, 0x800(r30)
    cmpw     r31, r0
    blt      .L_0000008C
    b        .L_00000180
.L_000000EC:
    lis      r3, savedObjFlags@ha
    addi     r30, r3, savedObjFlags@l
    lwz      r0, 0x800(r30)
    cmpwi    r0, 0x0
    ble      .L_00000180
    lis      r4, gGame@ha
    lis      r3, 0xffff
    addi     r4, r4, gGame@l
    mr       r28, r30
    addis    r27, r4, 0x6
    subi     r31, r3, 0x1
    li       r29, 0x0
    subi     r27, r27, 0x2f80
    b        .L_00000174
.L_00000124:
    lwz      r3, 0x0(r28)
    cmpwi    r3, -0x1
    beq      .L_00000180
    lwz      r0, 0x0(r27)
    extsh    r4, r3
    cmplwi   r0, 0x0
    beq      .L_0000014C
    mr       r3, r27
    bl       ResolveEventObjectId
    b        .L_00000150
.L_0000014C:
    li       r3, 0x0
.L_00000150:
    lwz      r0, 0x22c(r3)
    li       r4, -0x1
    lwz      r5, 0x228(r3)
    addi     r28, r28, 0x8
    and      r0, r0, r31
    addi     r29, r29, 0x1
    stw      r0, 0x22c(r3)
    and      r0, r5, r4
    stw      r0, 0x228(r3)
.L_00000174:
    lwz      r0, 0x800(r30)
    cmpw     r29, r0
    blt      .L_00000124
.L_00000180:
    lmw      r27, 0xc(r1)
    li       r3, 0x0
    lwz      r0, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: shun .text 0x198
 * @note Size: 0xF0
 */
extern "C" s32 CmdSaveObjFlag(s32 cmd, s32 count, s32* args)
{
	s32 id = args[0];
	ShunObj* obj;

	if (savedObjFlags.mCount < 0x100) {
		savedObjFlags.mEntries[savedObjFlags.mCount].mId = id;
		obj                                              = ShunFindObj(savedObjFlags.mEntries[savedObjFlags.mCount].mId);
		if (obj != NULL && (obj->mFlags & 1)) {
			if (obj->mFlags & 0x10000) {
				savedObjFlags.mEntries[savedObjFlags.mCount].mFlag |= 1;
			}
			savedObjFlags.mCount++;
		}
	}
	return 0;
}

/**
 * @note Address: shun .text 0x288
 * @note Size: 0xB0
 */
extern "C" s32 CmdClearSavedObjFlags(s32 cmd, s32 count, s32* args)
{
	s32 i;

	savedObjFlags.mCount = 0;
	for (i = 0; i < ARRAY_SIZE(savedObjFlags.mEntries); i++) {
		savedObjFlags.mEntries[i].mId   = -1;
		savedObjFlags.mEntries[i].mFlag = 0;
	}
	return 0;
}
