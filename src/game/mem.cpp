#include "game/mm.h"
#include <dolphin/os.h>
#include <string.h>

extern "C" {
void GetObjectHeapBounds(u32* size, void** top);
u32 GetObjectPoolCapacityBytes(void);
u32 GetObjectHeapRemainingBytes(void);
u32 GetObjectPoolUsedBytes(void);
u32 GetObjectPoolFreeBytes(void);
}

// one entry per block, used or freed. a freed block keeps its address in mFree so Alloc
// can hand it out again, the name is the tag the caller passed. MEMINFO is from the wii
// build's "no space for MEMINFO array" message
struct MEMINFO {
	u8* mAddr;        // _00
	u8* mFree;        // _04
	u32 mSize;        // _08
	char mName[0x80]; // _0C
	s32 mStatic;      // _8C, set blocks get reused whole instead of split
};
STATIC_ASSERT(sizeof(MEMINFO) == 0x90);

#define MEMINFO_MAX 0x400

static MEMINFO sInfo[MEMINFO_MAX];
static u8* sTop;
static u8* sCur;
static s32 sNum;

namespace mm {

static inline u32 GetAllocSize()
{
	BOOL enabled = OSDisableInterrupts();
	u32 size     = sTop - sCur; // the arena grows down from sTop
	OSRestoreInterrupts(enabled);
	return size;
}

// what's left of the arena once the game's blocks are taken out of it
static inline s32 GetRemainSize()
{
	BOOL enabled = OSDisableInterrupts();
	s32 remain   = GetObjectHeapRemainingBytes();
	s32 size;
	if (remain == -1) {
		size = 0;
	} else {
		size = remain - GetAllocSize();
	}
	OSRestoreInterrupts(enabled);
	return size;
}

// the arena left plus what HSD's object heap has freed
static inline s32 GetFreeSize()
{
	BOOL enabled = OSDisableInterrupts();
	u32 freed    = GetObjectPoolFreeBytes();
	s32 remain   = GetObjectHeapRemainingBytes();
	s32 size;
	if (remain == -1) {
		size = 0;
	} else {
		size = freed + remain - GetAllocSize();
	}
	OSRestoreInterrupts(enabled);
	return size;
}

} // namespace mm

#define KB(x) ((x) / 1024.0f)
#define MB(x) ((x) / 1048576.0f)

/**
 * @note Address: 0x801913D8
 * @note Size: 0x4EC
 */
void mm::Dump()
{
	OSReport("===============================================================================\n");

	u32 total = 0;
	for (int i = 0; i < MEMINFO_MAX; ++i) {
		MEMINFO* info = &sInfo[i];
		if (info->mAddr != 0) { // u lines are used blocks, f freed
			total += info->mSize;
			OSReport("-----(%4d) u %08X,%9d(%6.1f) %c %s\n", i, info->mAddr, info->mSize, KB(info->mSize),
			         (s8)(info->mStatic != 0 ? '*' : ' '), info->mName);
		} else if (info->mFree != 0) {
			total += info->mSize;
			OSReport("-----(%4d) f %08X,%9d(%6.1f) %c %s\n", i, info->mFree, info->mSize, KB(info->mSize),
			         (s8)(info->mStatic != 0 ? '*' : ' '), info->mName);
		}
	}
	OSReport("-----(%4d) * %08X,%9d(%6.1f)   <total>\n", sNum, sCur, total, KB(total));
	if (total != GetAllocSize()) {
		OSReport("mm::Dump>> total(%d) != GetAllocSize(%d)\n", total, GetAllocSize());
	}
	OSReport("\n");

	u32 size;
	GetObjectHeapBounds(&size, NULL);
	OSReport("ObjHeap   :%9d(%7.1fK)(%6.3fM)\n", size, KB(size), MB(size));
	OSReport(" remain   :%9d(%7.1fK)(%6.3fM)\n", GetObjectHeapRemainingBytes(), KB(GetObjectHeapRemainingBytes()),
	         MB(GetObjectHeapRemainingBytes()));
	OSReport(" alloc    :%9d(%7.1fK)(%6.3fM)\n", GetObjectPoolCapacityBytes(), KB(GetObjectPoolCapacityBytes()),
	         MB(GetObjectPoolCapacityBytes()));
	OSReport(" using    :%9d(%7.1fK)(%6.3fM)\n", GetObjectPoolUsedBytes(), KB(GetObjectPoolUsedBytes()), MB(GetObjectPoolUsedBytes()));
	OSReport(" freed    :%9d(%7.1fK)(%6.3fM)\n", GetObjectPoolFreeBytes(), KB(GetObjectPoolFreeBytes()), MB(GetObjectPoolFreeBytes()));
	OSReport("\n");

	size = GetFreeSize();
	OSReport("FreeHeap  :%9d(%7.1fK)(%6.3fM)\n", (s32)size, KB((s32)size), MB((s32)size));
	size = GetRemainSize();
	OSReport("RemainHeap:%9d(%7.1fK)(%6.3fM)\n", (s32)size, KB((s32)size), MB((s32)size));
	OSReport("\n");
}
/*
    stwu     r1, -0xa0(r1)
    mflr     r0
    stw      r0, 0xa4(r1)
    stfd     f31, 0x90(r1)
    psq_st   f31, 0x98(r1), 0, qr0
    stfd     f30, 0x80(r1)
    psq_st   f30, 0x88(r1), 0, qr0
    stmw     r27, 0x6c(r1)
    lis      r3, lbl_8020E508@ha
    addi     r31, r3, lbl_8020E508@l
    addi     r3, r31, 0xc
    crclr    cr1eq
    bl       OSReport
    lis      r3, sInfo@ha
    li       r30, 0x0
    addi     r27, r3, sInfo@l
    li       r28, 0x0
.L_8019141C:
    lwz      r5, 0x0(r27)
    cmplwi   r5, 0x0
    beq      .L_80191480
    lwz      r6, 0x8(r27)
    lis      r3, 0x4330
    stw      r3, 0x10(r1)
    mr       r4, r28
    lwz      r0, 0x8c(r27)
    add      r30, r30, r6
    stw      r6, 0x14(r1)
    addi     r3, r31, 0x60
    lfd      f2, lbl_8065F4D8@sda21(r0)
    cmpwi    r0, 0x0
    lfd      f1, 0x10(r1)
    li       r0, 0x20
    lfs      f0, lbl_8065F4C8@sda21(r0)
    fsubs    f1, f1, f2
    fmuls    f1, f1, f0
    beq      .L_8019146C
    li       r0, 0x2a
.L_8019146C:
    extsb    r7, r0
    addi     r8, r27, 0xc
    crset    cr1eq
    bl       OSReport
    b        .L_801914E0
.L_80191480:
    lwz      r5, 0x4(r27)
    cmplwi   r5, 0x0
    beq      .L_801914E0
    lwz      r6, 0x8(r27)
    lis      r3, 0x4330
    stw      r3, 0x10(r1)
    mr       r4, r28
    lwz      r0, 0x8c(r27)
    add      r30, r30, r6
    stw      r6, 0x14(r1)
    addi     r3, r31, 0x84
    lfd      f2, lbl_8065F4D8@sda21(r0)
    cmpwi    r0, 0x0
    lfd      f1, 0x10(r1)
    li       r0, 0x20
    lfs      f0, lbl_8065F4C8@sda21(r0)
    fsubs    f1, f1, f2
    fmuls    f1, f1, f0
    beq      .L_801914D0
    li       r0, 0x2a
.L_801914D0:
    extsb    r7, r0
    addi     r8, r27, 0xc
    crset    cr1eq
    bl       OSReport
.L_801914E0:
    addi     r28, r28, 0x1
    addi     r27, r27, 0x90
    cmpwi    r28, 0x400
    blt      .L_8019141C
    lis      r0, 0x4330
    stw      r30, 0x14(r1)
    lfd      f2, lbl_8065F4D8@sda21(r0)
    mr       r6, r30
    stw      r0, 0x10(r1)
    addi     r3, r31, 0xa8
    lfs      f0, lbl_8065F4C8@sda21(r0)
    lfd      f1, 0x10(r1)
    lwz      r4, sNum@sda21(r0)
    fsubs    f1, f1, f2
    lwz      r5, sCur@sda21(r0)
    fmuls    f1, f1, f0
    crset    cr1eq
    bl       OSReport
    bl       OSDisableInterrupts
    lwz      r4, sCur@sda21(r0)
    lwz      r0, sTop@sda21(r0)
    subf     r29, r4, r0
    bl       OSRestoreInterrupts
    cmplw    r30, r29
    beq      .L_8019156C
    bl       OSDisableInterrupts
    lwz      r4, sCur@sda21(r0)
    lwz      r0, sTop@sda21(r0)
    subf     r29, r4, r0
    bl       OSRestoreInterrupts
    mr       r4, r30
    mr       r5, r29
    addi     r3, r31, 0xd0
    crclr    cr1eq
    bl       OSReport
.L_8019156C:
    li       r3, lbl_8065F4CC@sda21
    crclr    cr1eq
    bl       OSReport
    addi     r3, r1, 0x8
    li       r4, 0x0
    bl       GetObjectHeapBounds
    lwz      r4, 0x8(r1)
    lis      r0, 0x4330
    stw      r0, 0x18(r1)
    addi     r3, r31, 0xfc
    lfd      f4, lbl_8065F4D8@sda21(r0)
    stw      r4, 0x1c(r1)
    lfs      f0, lbl_8065F4D0@sda21(r0)
    lfd      f1, 0x18(r1)
    stw      r4, 0x14(r1)
    fsubs    f1, f1, f4
    lfs      f3, lbl_8065F4C8@sda21(r0)
    stw      r0, 0x10(r1)
    fdivs    f2, f1, f0
    lfd      f0, 0x10(r1)
    fsubs    f0, f0, f4
    fmuls    f1, f0, f3
    crset    cr1eq
    bl       OSReport
    bl       GetObjectHeapRemainingBytes
    lis      r0, 0x4330
    stw      r3, 0x24(r1)
    lfd      f2, lbl_8065F4D8@sda21(r0)
    stw      r0, 0x20(r1)
    lfs      f0, lbl_8065F4D0@sda21(r0)
    lfd      f1, 0x20(r1)
    fsubs    f1, f1, f2
    fdivs    f31, f1, f0
    bl       GetObjectHeapRemainingBytes
    lis      r0, 0x4330
    stw      r3, 0x2c(r1)
    lfd      f2, lbl_8065F4D8@sda21(r0)
    stw      r0, 0x28(r1)
    lfs      f0, lbl_8065F4C8@sda21(r0)
    lfd      f1, 0x28(r1)
    fsubs    f1, f1, f2
    fmuls    f30, f1, f0
    bl       GetObjectHeapRemainingBytes
    fmr      f1, f30
    mr       r4, r3
    fmr      f2, f31
    addi     r3, r31, 0x11c
    crset    cr1eq
    bl       OSReport
    bl       GetObjectPoolCapacityBytes
    lis      r0, 0x4330
    stw      r3, 0x34(r1)
    lfd      f2, lbl_8065F4D8@sda21(r0)
    stw      r0, 0x30(r1)
    lfs      f0, lbl_8065F4D0@sda21(r0)
    lfd      f1, 0x30(r1)
    fsubs    f1, f1, f2
    fdivs    f31, f1, f0
    bl       GetObjectPoolCapacityBytes
    lis      r0, 0x4330
    stw      r3, 0x3c(r1)
    lfd      f2, lbl_8065F4D8@sda21(r0)
    stw      r0, 0x38(r1)
    lfs      f0, lbl_8065F4C8@sda21(r0)
    lfd      f1, 0x38(r1)
    fsubs    f1, f1, f2
    fmuls    f30, f1, f0
    bl       GetObjectPoolCapacityBytes
    fmr      f1, f30
    mr       r4, r3
    fmr      f2, f31
    addi     r3, r31, 0x13c
    crset    cr1eq
    bl       OSReport
    bl       GetObjectPoolUsedBytes
    lis      r0, 0x4330
    stw      r3, 0x44(r1)
    lfd      f2, lbl_8065F4D8@sda21(r0)
    stw      r0, 0x40(r1)
    lfs      f0, lbl_8065F4D0@sda21(r0)
    lfd      f1, 0x40(r1)
    fsubs    f1, f1, f2
    fdivs    f31, f1, f0
    bl       GetObjectPoolUsedBytes
    lis      r0, 0x4330
    stw      r3, 0x4c(r1)
    lfd      f2, lbl_8065F4D8@sda21(r0)
    stw      r0, 0x48(r1)
    lfs      f0, lbl_8065F4C8@sda21(r0)
    lfd      f1, 0x48(r1)
    fsubs    f1, f1, f2
    fmuls    f30, f1, f0
    bl       GetObjectPoolUsedBytes
    fmr      f1, f30
    mr       r4, r3
    fmr      f2, f31
    addi     r3, r31, 0x15c
    crset    cr1eq
    bl       OSReport
    bl       GetObjectPoolFreeBytes
    lis      r0, 0x4330
    stw      r3, 0x54(r1)
    lfd      f2, lbl_8065F4D8@sda21(r0)
    stw      r0, 0x50(r1)
    lfs      f0, lbl_8065F4D0@sda21(r0)
    lfd      f1, 0x50(r1)
    fsubs    f1, f1, f2
    fdivs    f30, f1, f0
    bl       GetObjectPoolFreeBytes
    lis      r0, 0x4330
    stw      r3, 0x5c(r1)
    lfd      f2, lbl_8065F4D8@sda21(r0)
    stw      r0, 0x58(r1)
    lfs      f0, lbl_8065F4C8@sda21(r0)
    lfd      f1, 0x58(r1)
    fsubs    f1, f1, f2
    fmuls    f31, f1, f0
    bl       GetObjectPoolFreeBytes
    fmr      f1, f31
    mr       r4, r3
    fmr      f2, f30
    addi     r3, r31, 0x17c
    crset    cr1eq
    bl       OSReport
    li       r3, lbl_8065F4CC@sda21
    crclr    cr1eq
    bl       OSReport
    bl       OSDisableInterrupts
    mr       r28, r3
    bl       GetObjectPoolFreeBytes
    mr       r29, r3
    bl       GetObjectHeapRemainingBytes
    mr       r30, r3
    cmpwi    r30, -0x1
    bne      .L_80191790
    li       r29, 0x0
    b        .L_801917AC
.L_80191790:
    bl       OSDisableInterrupts
    lwz      r4, sCur@sda21(r0)
    lwz      r0, sTop@sda21(r0)
    subf     r27, r4, r0
    bl       OSRestoreInterrupts
    add      r0, r29, r30
    subf     r29, r27, r0
.L_801917AC:
    mr       r3, r28
    bl       OSRestoreInterrupts
    xoris    r5, r29, 0x8000
    lis      r0, 0x4330
    stw      r5, 0x54(r1)
    mr       r4, r29
    lfd      f4, lbl_8065F4E0@sda21(r0)
    addi     r3, r31, 0x19c
    stw      r0, 0x50(r1)
    lfs      f0, lbl_8065F4D0@sda21(r0)
    lfd      f1, 0x50(r1)
    stw      r5, 0x5c(r1)
    fsubs    f1, f1, f4
    lfs      f3, lbl_8065F4C8@sda21(r0)
    stw      r0, 0x58(r1)
    fdivs    f2, f1, f0
    lfd      f0, 0x58(r1)
    stw      r29, 0x8(r1)
    fsubs    f0, f0, f4
    fmuls    f1, f0, f3
    crset    cr1eq
    bl       OSReport
    bl       OSDisableInterrupts
    mr       r29, r3
    bl       GetObjectHeapRemainingBytes
    mr       r30, r3
    cmpwi    r30, -0x1
    bne      .L_80191824
    li       r28, 0x0
    b        .L_8019183C
.L_80191824:
    bl       OSDisableInterrupts
    lwz      r4, sCur@sda21(r0)
    lwz      r0, sTop@sda21(r0)
    subf     r27, r4, r0
    bl       OSRestoreInterrupts
    subf     r28, r27, r30
.L_8019183C:
    mr       r3, r29
    bl       OSRestoreInterrupts
    xoris    r5, r28, 0x8000
    lis      r0, 0x4330
    stw      r5, 0x54(r1)
    mr       r4, r28
    lfd      f4, lbl_8065F4E0@sda21(r0)
    addi     r3, r31, 0x1bc
    stw      r0, 0x50(r1)
    lfs      f0, lbl_8065F4D0@sda21(r0)
    lfd      f1, 0x50(r1)
    stw      r5, 0x5c(r1)
    fsubs    f1, f1, f4
    lfs      f3, lbl_8065F4C8@sda21(r0)
    stw      r0, 0x58(r1)
    fdivs    f2, f1, f0
    lfd      f0, 0x58(r1)
    stw      r28, 0x8(r1)
    fsubs    f0, f0, f4
    fmuls    f1, f0, f3
    crset    cr1eq
    bl       OSReport
    li       r3, lbl_8065F4CC@sda21
    crclr    cr1eq
    bl       OSReport
    psq_l    f31, 0x98(r1), 0, qr0
    lfd      f31, 0x90(r1)
    psq_l    f30, 0x88(r1), 0, qr0
    lfd      f30, 0x80(r1)
    lmw      r27, 0x6c(r1)
    lwz      r0, 0xa4(r1)
    mtlr     r0
    addi     r1, r1, 0xa0
    blr
*/

/**
 * @note Address: 0x801918C4
 * @note Size: 0xB0
 */
void mm::SetName(void* ptr, const char* name)
{
	BOOL enabled = OSDisableInterrupts();
	for (int i = 0; i < sNum; i++) {
		if (sInfo[i].mAddr != 0 && sInfo[i].mAddr == ptr) {
			strcpy(sInfo[i].mName, name);
			OSRestoreInterrupts(enabled);
			return;
		}
	}
	OSRestoreInterrupts(enabled);
}
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stw      r31, 0x1c(r1)
    stw      r30, 0x18(r1)
    mr       r30, r4
    stw      r29, 0x14(r1)
    mr       r29, r3
    bl       OSDisableInterrupts
    lwz      r0, sNum@sda21(r0)
    lis      r4, sInfo@ha
    li       r5, 0x0
    mr       r31, r3
    addi     r4, r4, sInfo@l
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80191950
.L_80191908:
    lwz      r0, 0x0(r4)
    cmplwi   r0, 0x0
    beq      .L_80191944
    cmplw    r0, r29
    bne      .L_80191944
    mulli    r5, r5, 0x90
    lis      r3, sInfo@ha
    mr       r4, r30
    addi     r0, r3, sInfo@l
    add      r3, r0, r5
    addi     r3, r3, 0xc
    bl       strcpy
    mr       r3, r31
    bl       OSRestoreInterrupts
    b        .L_80191958
.L_80191944:
    addi     r4, r4, 0x90
    addi     r5, r5, 0x1
    bdnz     .L_80191908
.L_80191950:
    mr       r3, r31
    bl       OSRestoreInterrupts
.L_80191958:
    lwz      r0, 0x24(r1)
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: 0x80191974
 * @note Size: 0x94
 */
char* mm::GetName(void* ptr)
{
	BOOL enabled = OSDisableInterrupts();
	for (int i = 0; i < sNum; i++) {
		if (sInfo[i].mAddr != 0 && sInfo[i].mAddr == ptr) {
			OSRestoreInterrupts(enabled);
			return sInfo[i].mName;
		}
	}
	OSRestoreInterrupts(enabled);
	return NULL;
}
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    stw      r0, 0x14(r1)
    stw      r31, 0xc(r1)
    stw      r30, 0x8(r1)
    mr       r30, r3
    bl       OSDisableInterrupts
    lwz      r0, sNum@sda21(r0)
    lis      r4, sInfo@ha
    li       r31, 0x0
    addi     r4, r4, sInfo@l
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_801919E8
.L_801919AC:
    lwz      r0, 0x0(r4)
    cmplwi   r0, 0x0
    beq      .L_801919DC
    cmplw    r0, r30
    bne      .L_801919DC
    bl       OSRestoreInterrupts
    mulli    r4, r31, 0x90
    lis      r3, sInfo@ha
    addi     r0, r3, sInfo@l
    add      r3, r0, r4
    addi     r3, r3, 0xc
    b        .L_801919F0
.L_801919DC:
    addi     r4, r4, 0x90
    addi     r31, r31, 0x1
    bdnz     .L_801919AC
.L_801919E8:
    bl       OSRestoreInterrupts
    li       r3, 0x0
.L_801919F0:
    lwz      r0, 0x14(r1)
    lwz      r31, 0xc(r1)
    lwz      r30, 0x8(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: 0x80191A08
 * @note Size: 0x2F8
 */
void mm::Compact(BOOL dump)
{
	if (dump) {
		Dump();
	}

	BOOL enabled = OSDisableInterrupts();

	// glue each freed block to any freed block right after it
	for (int i = 0; i < sNum; i++) {
		if (sInfo[i].mAddr == 0 && sInfo[i].mFree != 0) {
			for (int j = 0; j < MEMINFO_MAX; j++) {
				if (i != j && sInfo[j].mAddr == 0 && sInfo[j].mFree != 0 && sInfo[i].mFree + sInfo[i].mSize == sInfo[j].mFree) {
					sInfo[i].mSize += sInfo[j].mSize;
					sInfo[j].mFree = 0;
					sInfo[j].mSize = 0;
				}
			}
		}
	}

	// find the lowest used block, every freed block below it goes back to the arena
	u8* low  = sTop;
	s32 last = 0;
	for (int i = 0; i < MEMINFO_MAX; i++) {
		if (sInfo[i].mAddr != 0) {
			if (sInfo[i].mAddr < low) {
				low = sInfo[i].mAddr;
			}
			last = i;
		}
	}
	for (int i = 0; i < MEMINFO_MAX; i++) {
		if (sInfo[i].mAddr == 0 && sInfo[i].mFree != 0) {
			if (sInfo[i].mFree < low) {
				sInfo[i].mFree = 0;
				sInfo[i].mSize = 0;
			}
			if (i > last) {
				last = i;
			}
		}
	}
	sCur = low;
	sNum = last + 1;

	OSRestoreInterrupts(enabled);
	if (dump) {
		Dump();
	}
}
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stw      r31, 0x1c(r1)
    mr.      r31, r3
    stw      r30, 0x18(r1)
    stw      r29, 0x14(r1)
    beq      .L_80191A2C
    bl       Dump__2mmFv
.L_80191A2C:
    bl       OSDisableInterrupts
    lis      r4, sInfo@ha
    li       r30, 0x0
    addi     r11, r4, sInfo@l
    lwz      r4, sNum@sda21(r0)
    mr       r10, r11
    b        .L_80191B28
.L_80191A48:
    lwz      r0, 0x0(r10)
    cmplwi   r0, 0x0
    bne      .L_80191B20
    lwz      r0, 0x4(r10)
    cmplwi   r0, 0x0
    beq      .L_80191B20
    li       r0, 0x200
    mr       r7, r11
    addi     r8, r10, 0x8
    addi     r9, r10, 0x4
    li       r29, 0x0
    li       r5, 0x0
    mtctr    r0
.L_80191A7C:
    cmpw     r30, r29
    beq      .L_80191AC4
    lwz      r0, 0x0(r7)
    cmplwi   r0, 0x0
    bne      .L_80191AC4
    lwz      r12, 0x4(r7)
    cmplwi   r12, 0x0
    beq      .L_80191AC4
    lwz      r0, 0x0(r9)
    lwz      r6, 0x0(r8)
    add      r0, r0, r6
    cmplw    r0, r12
    bne      .L_80191AC4
    lwz      r0, 0x8(r7)
    add      r0, r6, r0
    stw      r0, 0x0(r8)
    stw      r5, 0x4(r7)
    stw      r5, 0x8(r7)
.L_80191AC4:
    addi     r29, r29, 0x1
    addi     r7, r7, 0x90
    cmpw     r30, r29
    beq      .L_80191B14
    lwz      r0, 0x0(r7)
    cmplwi   r0, 0x0
    bne      .L_80191B14
    lwz      r12, 0x4(r7)
    cmplwi   r12, 0x0
    beq      .L_80191B14
    lwz      r0, 0x0(r9)
    lwz      r6, 0x0(r8)
    add      r0, r0, r6
    cmplw    r0, r12
    bne      .L_80191B14
    lwz      r0, 0x8(r7)
    add      r0, r6, r0
    stw      r0, 0x0(r8)
    stw      r5, 0x4(r7)
    stw      r5, 0x8(r7)
.L_80191B14:
    addi     r7, r7, 0x90
    addi     r29, r29, 0x1
    bdnz     .L_80191A7C
.L_80191B20:
    addi     r10, r10, 0x90
    addi     r30, r30, 0x1
.L_80191B28:
    cmpw     r30, r4
    blt      .L_80191A48
    li       r0, 0x100
    mr       r4, r11
    lwz      r5, sTop@sda21(r0)
    li       r6, 0x0
    li       r7, 0x0
    mtctr    r0
.L_80191B48:
    lwz      r0, 0x0(r4)
    cmplwi   r0, 0x0
    beq      .L_80191B64
    cmplw    r0, r5
    bge      .L_80191B60
    mr       r5, r0
.L_80191B60:
    mr       r6, r7
.L_80191B64:
    lwzu     r0, 0x90(r4)
    addi     r7, r7, 0x1
    cmplwi   r0, 0x0
    beq      .L_80191B84
    cmplw    r0, r5
    bge      .L_80191B80
    mr       r5, r0
.L_80191B80:
    mr       r6, r7
.L_80191B84:
    lwzu     r0, 0x90(r4)
    addi     r7, r7, 0x1
    cmplwi   r0, 0x0
    beq      .L_80191BA4
    cmplw    r0, r5
    bge      .L_80191BA0
    mr       r5, r0
.L_80191BA0:
    mr       r6, r7
.L_80191BA4:
    lwzu     r0, 0x90(r4)
    addi     r7, r7, 0x1
    cmplwi   r0, 0x0
    beq      .L_80191BC4
    cmplw    r0, r5
    bge      .L_80191BC0
    mr       r5, r0
.L_80191BC0:
    mr       r6, r7
.L_80191BC4:
    addi     r4, r4, 0x90
    addi     r7, r7, 0x1
    bdnz     .L_80191B48
    li       r0, 0x100
    li       r7, 0x0
    li       r4, 0x0
    mtctr    r0
.L_80191BE0:
    lwz      r0, 0x0(r11)
    cmplwi   r0, 0x0
    bne      .L_80191C14
    lwz      r0, 0x4(r11)
    cmplwi   r0, 0x0
    beq      .L_80191C14
    cmplw    r0, r5
    bge      .L_80191C08
    stw      r4, 0x4(r11)
    stw      r4, 0x8(r11)
.L_80191C08:
    cmpw     r7, r6
    ble      .L_80191C14
    mr       r6, r7
.L_80191C14:
    lwzu     r0, 0x90(r11)
    addi     r7, r7, 0x1
    cmplwi   r0, 0x0
    bne      .L_80191C4C
    lwz      r0, 0x4(r11)
    cmplwi   r0, 0x0
    beq      .L_80191C4C
    cmplw    r0, r5
    bge      .L_80191C40
    stw      r4, 0x4(r11)
    stw      r4, 0x8(r11)
.L_80191C40:
    cmpw     r7, r6
    ble      .L_80191C4C
    mr       r6, r7
.L_80191C4C:
    lwzu     r0, 0x90(r11)
    addi     r7, r7, 0x1
    cmplwi   r0, 0x0
    bne      .L_80191C84
    lwz      r0, 0x4(r11)
    cmplwi   r0, 0x0
    beq      .L_80191C84
    cmplw    r0, r5
    bge      .L_80191C78
    stw      r4, 0x4(r11)
    stw      r4, 0x8(r11)
.L_80191C78:
    cmpw     r7, r6
    ble      .L_80191C84
    mr       r6, r7
.L_80191C84:
    lwzu     r0, 0x90(r11)
    addi     r7, r7, 0x1
    cmplwi   r0, 0x0
    bne      .L_80191CBC
    lwz      r0, 0x4(r11)
    cmplwi   r0, 0x0
    beq      .L_80191CBC
    cmplw    r0, r5
    bge      .L_80191CB0
    stw      r4, 0x4(r11)
    stw      r4, 0x8(r11)
.L_80191CB0:
    cmpw     r7, r6
    ble      .L_80191CBC
    mr       r6, r7
.L_80191CBC:
    addi     r11, r11, 0x90
    addi     r7, r7, 0x1
    bdnz     .L_80191BE0
    addi     r0, r6, 0x1
    stw      r5, sCur@sda21(r0)
    stw      r0, sNum@sda21(r0)
    bl       OSRestoreInterrupts
    cmpwi    r31, 0x0
    beq      .L_80191CE4
    bl       Dump__2mmFv
.L_80191CE4:
    lwz      r0, 0x24(r1)
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: 0x80191D00
 * @note Size: 0x104
 */
void mm::Free(void* ptr, s32 flag)
{
	BOOL enabled = OSDisableInterrupts();
	for (int i = 0; i < sNum; i++) {
		if (sInfo[i].mAddr != 0 && sInfo[i].mAddr == ptr) {
			if (sInfo[i].mAddr == sCur) {
				// the lowest block, give it straight back to the arena
				// (sCur only ever points at the lowest used block)
				sCur           = sInfo[i].mAddr + sInfo[i].mSize;
				sInfo[i].mFree = 0;
				sInfo[i].mAddr = 0;
				sInfo[i].mSize = 0;
			} else {
				sInfo[i].mAddr = 0;
			}
			if (flag) { // also clears the static marker
				sInfo[i].mStatic = 0;
			}
			Compact(FALSE);
			OSRestoreInterrupts(enabled);
			return;
		}
	}
	// freeing a pointer it doesn't track dumps the table
	Dump();
	OSRestoreInterrupts(enabled);
}
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stw      r31, 0x1c(r1)
    stw      r30, 0x18(r1)
    mr       r30, r4
    stw      r29, 0x14(r1)
    mr       r29, r3
    bl       OSDisableInterrupts
    lwz      r0, sNum@sda21(r0)
    lis      r4, sInfo@ha
    li       r5, 0x0
    mr       r31, r3
    addi     r4, r4, sInfo@l
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80191DDC
.L_80191D44:
    lwz      r0, 0x0(r4)
    cmplwi   r0, 0x0
    beq      .L_80191DD0
    cmplw    r0, r29
    bne      .L_80191DD0
    mulli    r5, r5, 0x90
    lis      r3, sInfo@ha
    lwz      r0, sCur@sda21(r0)
    addi     r4, r3, sInfo@l
    lwzx     r6, r4, r5
    cmplw    r6, r0
    bne      .L_80191D98
    add      r7, r4, r5
    li       r0, 0x0
    lwz      r3, 0x8(r7)
    stw      r0, 0x4(r7)
    add      r3, r6, r3
    stwx     r0, r4, r5
    stw      r3, sCur@sda21(r0)
    stw      r0, 0x8(r7)
    b        .L_80191DA0
.L_80191D98:
    li       r0, 0x0
    stwx     r0, r4, r5
.L_80191DA0:
    cmpwi    r30, 0x0
    beq      .L_80191DBC
    lis      r3, sInfo@ha
    li       r4, 0x0
    addi     r0, r3, sInfo@l
    add      r3, r0, r5
    stw      r4, 0x8c(r3)
.L_80191DBC:
    li       r3, 0x0
    bl       Compact__2mmFi
    mr       r3, r31
    bl       OSRestoreInterrupts
    b        .L_80191DE8
.L_80191DD0:
    addi     r4, r4, 0x90
    addi     r5, r5, 0x1
    bdnz     .L_80191D44
.L_80191DDC:
    bl       Dump__2mmFv
    mr       r3, r31
    bl       OSRestoreInterrupts
.L_80191DE8:
    lwz      r0, 0x24(r1)
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: 0x80191E04
 * @note Size: 0x324
 */
void* mm::Alloc(u32 size, const char* name)
{
	BOOL enabled = OSDisableInterrupts();
	if (size & 31) { // blocks are 32-byte aligned
		OSReport("mm::Alloc>> alloc size must be 32byte alignment -- %s(%d)!\n", name, size);
		size = (size + 31) & ~31;
	}

	BOOL found = FALSE;
	if (size != 0) {
		int i;
		s32 num = sNum;
		u8* cur = sCur;
		for (i = 0; i < sNum; i++) {
			if (sInfo[i].mAddr == 0 && sInfo[i].mFree != 0 && sInfo[i].mSize >= size) {
				if (sInfo[i].mStatic != 0) { // a static block is reused whole
					sInfo[i].mAddr = sInfo[i].mFree;
					strcpy(sInfo[i].mName, name);
					found = TRUE;
					break;
				}
				s32 rest = sInfo[i].mSize - size;
				if (rest == 0) {
					sInfo[i].mAddr = sInfo[i].mFree;
					strcpy(sInfo[i].mName, name);
					found = TRUE;
					break;
				}
				if (rest > 0) {
					// split it, the low part stays free in a new entry
					for (int j = 0; j < MEMINFO_MAX; j++) {
						if (j != i && sInfo[j].mAddr == 0 && sInfo[j].mFree == 0) {
							sInfo[j].mAddr = 0;
							sInfo[j].mFree = sInfo[i].mFree;
							sInfo[j].mSize = rest;
							strcpy(sInfo[j].mName, "(subd)");
							if (sNum < j + 1) {
								sNum = j + 1;
							}
							sInfo[i].mAddr = sInfo[i].mFree + (sInfo[i].mSize - size);
							sInfo[i].mFree = sInfo[i].mAddr;
							sInfo[i].mSize = size;
							strcpy(sInfo[i].mName, name);
							found = TRUE;
							break;
						}
					}
				}
				break;
			}
		}

		if (!found) {
			// nothing reusable: carve a fresh block off the arena's bottom
			for (i = 0; i < MEMINFO_MAX; i++) {
				if (sInfo[i].mAddr == 0 && sInfo[i].mFree == 0) {
					sInfo[i].mFree = sCur - size;
					sInfo[i].mAddr = sCur - size;
					sInfo[i].mSize = size;
					strcpy(sInfo[i].mName, name);
					sCur -= size;
					if (sNum < i + 1) {
						sNum = i + 1;
					}
					found = TRUE;
					break;
				}
			}
		}

		if (found) {
			if (GetRemainSize() < 0) {
				// out of memory, undo it
				sCur           = cur;
				sInfo[i].mFree = 0;
				sInfo[i].mAddr = 0;
				sInfo[i].mSize = 0;
				sNum           = num;
			} else {
				OSRestoreInterrupts(enabled);
				return (void*)sInfo[i].mAddr;
			}
		}
	}

	OSRestoreInterrupts(enabled);
	return NULL;
}
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    stw      r0, 0x44(r1)
    stmw     r21, 0x14(r1)
    mr       r21, r3
    mr       r22, r4
    bl       OSDisableInterrupts
    clrlwi.  r0, r21, 27
    mr       r26, r3
    beq      .L_80191E4C
    lis      r3, lbl_8020E6E4@ha
    mr       r4, r22
    addi     r3, r3, lbl_8020E6E4@l
    mr       r5, r21
    crclr    cr1eq
    bl       OSReport
    addi     r0, r21, 0x1f
    clrrwi   r21, r0, 5
.L_80191E4C:
    cmplwi   r21, 0x0
    li       r0, 0x0
    beq      .L_80192108
    lis      r3, sInfo@ha
    lwz      r31, sNum@sda21(r0)
    addi     r29, r3, sInfo@l
    lwz      r24, sCur@sda21(r0)
    li       r25, 0x0
    mr       r4, r29
    mtctr    r31
    cmpwi    r31, 0x0
    ble      .L_80191FE0
.L_80191E7C:
    lwz      r3, 0x0(r4)
    cmplwi   r3, 0x0
    bne      .L_80191FD4
    lwz      r3, 0x4(r4)
    cmplwi   r3, 0x0
    beq      .L_80191FD4
    lwz      r3, 0x8(r4)
    cmplw    r3, r21
    blt      .L_80191FD4
    mulli    r30, r25, 0x90
    lis      r3, sInfo@ha
    addi     r4, r3, sInfo@l
    add      r5, r4, r30
    lwz      r3, 0x8c(r5)
    cmpwi    r3, 0x0
    beq      .L_80191ED8
    lwz      r0, 0x4(r5)
    mr       r4, r22
    addi     r3, r5, 0xc
    stw      r0, 0x0(r5)
    bl       strcpy
    li       r0, 0x1
    b        .L_80191FE0
.L_80191ED8:
    addi     r27, r4, 0x8
    lwzx     r3, r27, r30
    subf.    r7, r21, r3
    bne      .L_80191F04
    lwz      r0, 0x4(r5)
    mr       r4, r22
    addi     r3, r5, 0xc
    stw      r0, 0x0(r5)
    bl       strcpy
    li       r0, 0x1
    b        .L_80191FE0
.L_80191F04:
    cmpwi    r7, 0x0
    ble      .L_80191FE0
    li       r3, 0x400
    mr       r4, r29
    li       r23, 0x0
    mtctr    r3
.L_80191F1C:
    cmpw     r23, r25
    beq      .L_80191FC4
    lwz      r3, 0x0(r4)
    cmplwi   r3, 0x0
    bne      .L_80191FC4
    lwz      r3, 0x4(r4)
    cmplwi   r3, 0x0
    bne      .L_80191FC4
    mulli    r5, r23, 0x90
    lis      r3, sInfo@ha
    li       r0, 0x0
    addi     r3, r3, sInfo@l
    add      r6, r3, r5
    li       r4, lbl_8065F4E8@sda21
    stw      r0, 0x0(r6)
    addi     r28, r3, 0x4
    addi     r3, r6, 0xc
    lwzx     r0, r28, r30
    stwx     r0, r28, r5
    stwx     r7, r27, r5
    bl       strcpy
    lwz      r3, sNum@sda21(r0)
    addi     r0, r23, 0x1
    cmpw     r3, r0
    bge      .L_80191F84
    stw      r0, sNum@sda21(r0)
.L_80191F84:
    lwzx     r4, r27, r30
    lis      r3, sInfo@ha
    lwzx     r5, r28, r30
    addi     r0, r3, sInfo@l
    subf     r3, r21, r4
    mr       r4, r22
    add      r6, r0, r30
    add      r0, r5, r3
    stw      r0, 0x0(r6)
    addi     r3, r6, 0xc
    lwz      r0, 0x0(r6)
    stwx     r0, r28, r30
    stwx     r21, r27, r30
    bl       strcpy
    li       r0, 0x1
    b        .L_80191FE0
.L_80191FC4:
    addi     r4, r4, 0x90
    addi     r23, r23, 0x1
    bdnz     .L_80191F1C
    b        .L_80191FE0
.L_80191FD4:
    addi     r4, r4, 0x90
    addi     r25, r25, 0x1
    bdnz     .L_80191E7C
.L_80191FE0:
    cmpwi    r0, 0x0
    bne      .L_80192070
    li       r3, 0x400
    li       r25, 0x0
    mtctr    r3
.L_80191FF4:
    lwz      r3, 0x0(r29)
    cmplwi   r3, 0x0
    bne      .L_80192064
    lwz      r3, 0x4(r29)
    cmplwi   r3, 0x0
    bne      .L_80192064
    mulli    r5, r25, 0x90
    lwz      r0, sCur@sda21(r0)
    lis      r3, sInfo@ha
    subf     r6, r21, r0
    addi     r0, r3, sInfo@l
    mr       r4, r22
    add      r5, r0, r5
    stw      r6, 0x4(r5)
    addi     r3, r5, 0xc
    stw      r6, 0x0(r5)
    stw      r21, 0x8(r5)
    bl       strcpy
    lwz      r4, sCur@sda21(r0)
    addi     r0, r25, 0x1
    lwz      r3, sNum@sda21(r0)
    subf     r4, r21, r4
    cmpw     r3, r0
    stw      r4, sCur@sda21(r0)
    bge      .L_8019205C
    stw      r0, sNum@sda21(r0)
.L_8019205C:
    li       r0, 0x1
    b        .L_80192070
.L_80192064:
    addi     r29, r29, 0x90
    addi     r25, r25, 0x1
    bdnz     .L_80191FF4
.L_80192070:
    cmpwi    r0, 0x0
    beq      .L_80192108
    bl       OSDisableInterrupts
    mr       r22, r3
    bl       GetObjectHeapRemainingBytes
    mr       r21, r3
    cmpwi    r21, -0x1
    bne      .L_80192098
    li       r21, 0x0
    b        .L_801920B0
.L_80192098:
    bl       OSDisableInterrupts
    lwz      r4, sCur@sda21(r0)
    lwz      r0, sTop@sda21(r0)
    subf     r23, r4, r0
    bl       OSRestoreInterrupts
    subf     r21, r23, r21
.L_801920B0:
    mr       r3, r22
    bl       OSRestoreInterrupts
    cmpwi    r21, 0x0
    bge      .L_801920EC
    mulli    r5, r25, 0x90
    lis      r3, sInfo@ha
    li       r0, 0x0
    stw      r24, sCur@sda21(r0)
    addi     r4, r3, sInfo@l
    add      r3, r4, r5
    stw      r0, 0x4(r3)
    stwx     r0, r4, r5
    stw      r0, 0x8(r3)
    stw      r31, sNum@sda21(r0)
    b        .L_80192108
.L_801920EC:
    mr       r3, r26
    bl       OSRestoreInterrupts
    mulli    r0, r25, 0x90
    lis      r3, sInfo@ha
    addi     r3, r3, sInfo@l
    lwzx     r3, r3, r0
    b        .L_80192114
.L_80192108:
    mr       r3, r26
    bl       OSRestoreInterrupts
    li       r3, 0x0
.L_80192114:
    lmw      r21, 0x14(r1)
    lwz      r0, 0x44(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

/**
 * @note Address: 0x80192128
 * @note Size: 0xEC
 */
void mm::Init()
{
	for (int i = 0; i < MEMINFO_MAX; i++) {
		sInfo[i].mAddr    = 0;
		sInfo[i].mFree    = 0;
		sInfo[i].mSize    = 0;
		sInfo[i].mName[0] = '\0';
		sInfo[i].mStatic  = 0;
	}
	sTop = sCur = (u8*)OSGetArenaHi(); // the arena fills downward from here
	sNum        = 0;
}

// small container with a word buffer, class unknown
struct WordBuffer {
	u32* mBuf; // _00
	s32 _04;   // _04
	s32 mSize; // _08
};

/**
 * @note Address: 0x80192214
 * @note Size: 0x7C
 */
extern "C" void AllocateWordBuffer(WordBuffer* p, s32 n)
{
	if (p->mBuf != NULL) {
		if (p->mBuf != NULL) { // the inner check is redundant
			OSFreeToHeap(__OSCurrHeap, p->mBuf);
			p->mBuf = NULL;
		}
		p->mSize = 0;
		p->_04   = 0;
	}
	p->mBuf  = (u32*)OSAllocFromHeap(__OSCurrHeap, n * 4);
	p->_04   = 0;
	p->mSize = n;
}
