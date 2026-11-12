#include "game/hsd_jobj.h"
extern "C" {
#include <sysdolphin/mtx_transform.h>
#include <sysdolphin/aobj.h>
#include <sysdolphin/rumble.h>
}
#include "game/clight.h"
#include "game/ccameraaction.h"
#include "game/ccameraactioneye.h"
#include "game/cmotioncontrolcamera.h"
#include "game/cparticle.h"
#include "game/ctpl.h"
#include "game/xobjs.h"
#include <string.h>
#include <stdlib.h>
#include "game/ut.h"
#include <stdio.h>
#include "game/evtqueue.h"
#include "game/msg.h"
#include "game/gmath.h"
#include "game/globals.h"
#include "types.h"
#include "Version.h"
#include "game/csound.h"
#include "game/globals.h"
#include "game/stagedata.h"
#include "game/xchr.h"
#include "game/cscrn.h"
#include "game/inventory.h"
#include "game/video.h"
#include "game/cgame.h"
#include "game/infoscrn.h"
#include "game/cmovie.h"

extern "C" {
extern GXColor sEvtWindowColorTemplate;
extern GXColor sEvtWindowDefaultColor;
}

class CJObj;

extern "C" void CEvt_EvalSet(CEvt* evt);
extern "C" s32 CEvt_Evaluate(CEvt* evt);
extern "C" void OSPanic(const char* file, int line, const char* msg, ...);

// the event globals, in the order the file's static initializer builds them
CEvtFlags gEventFlags;
CEvtVars gEventVars;
CEvtQueueEx gEventScriptQueue;
CEvtTimer gEventTimers;

// script names for objects
EvtObjIdTable gEventObjectIds = { 0 };

// the stream's beat callback latches the beat here for WAIT
EvtBeatLatch gEventBeatLatch;

// the handler of each command. Update looks an opcode up here; nothing reads the second byte
EvtCmdEntry evtCmdTable[] = {
	{ EVT_CMD_ABORT, 3, "ABORT", &CEvt::_cmd_ABORT },
	{ EVT_CMD_RETURN, 11, "RETURN", &CEvt::_cmd_RETURN },
	{ EVT_CMD_GOTO, 3, "GOTO", &CEvt::_cmd_GOTO },
	{ EVT_CMD_SET, 3, "SET", &CEvt::_cmd_SET },
	{ EVT_CMD_IF, 3, "IF", &CEvt::_cmd_IF },
	{ EVT_CMD_ELIF, 3, "ELIF", &CEvt::_cmd_IF },
	{ EVT_CMD_ENDIF, 3, "ENDIF", &CEvt::_cmd_GOTO },
	{ EVT_CMD_CASE, 3, "CASE", &CEvt::_cmd_IF },
	{ EVT_CMD_EXPR, 3, "EXPR", &CEvt::_cmd_IF },
	{ EVT_CMD_WHILE, 3, "WHILE", &CEvt::_cmd_IF },
	{ EVT_CMD_BREAK, 3, "BREAK", &CEvt::_cmd_GOTO },
	{ EVT_CMD_RUN, 7, "RUN", &CEvt::_cmd_RUN },
	{ EVT_CMD_LIB, 7, "LIB", &CEvt::_cmd_LIB },
	{ EVT_CMD_PUSHBP, 0, "PUSHBP", &CEvt::_cmd_PUSHBP },
	{ EVT_CMD_POPBP, 0, "POPBP", &CEvt::_cmd_POPBP },
	{ EVT_CMD_SETSP, 0, "SETSP", &CEvt::_cmd_SETSP },
	{ EVT_CMD_ANIM, 3, "ANIM", &CEvt::_cmd_ANIM },
	{ EVT_CMD_ANIM1, 3, "ANIM1", &CEvt::_cmd_ANIM },
	{ EVT_CMD_ANIM2, 3, "ANIM2", &CEvt::_cmd_ANIM2 },
	{ EVT_CMD_ATTACH, 1, "ATTACH", &CEvt::_cmd_ATTACH },
	{ EVT_CMD_BORN, 3, "BORN", &CEvt::_cmd_BORN },
	{ EVT_CMD_CALL, 3, "CALL", &CEvt::_cmd_CALL },
	{ EVT_CMD_CAMERA, 3, "CAMERA", &CEvt::_cmd_CAMERA },
	{ EVT_CMD_CHECK, 3, "CHECK", &CEvt::_cmd_WAIT },
	{ EVT_CMD_COLOR, 3, "COLOR", &CEvt::_cmd_COLOR },
	{ EVT_CMD_DETACH, 3, "DETACH", &CEvt::_cmd_ATTACH },
	{ EVT_CMD_DIR, 3, "DIR", &CEvt::_cmd_DIR },
	{ EVT_CMD_MDIR, 3, "MDIR", &CEvt::_cmd_DIR },
	{ EVT_CMD_DISP, 3, "DISP", &CEvt::_cmd_DISP },
	{ EVT_CMD_KILL, 3, "KILL", &CEvt::_cmd_KILL },
	{ EVT_CMD_LIGHT, 3, "LIGHT", &CEvt::_cmd_LIGHT },
	{ EVT_CMD_MENU, 3, "MENU", &CEvt::_cmd_MENU },
	{ EVT_CMD_MOVE, 3, "MOVE", &CEvt::_cmd_MOVE },
	{ EVT_CMD_MOVETO, 3, "MOVETO", &CEvt::_cmd_MOVETO },
	{ EVT_CMD_MOVIE, 3, "MOVIE", &CEvt::_cmd_MOVIE },
	{ EVT_CMD_MSG, 3, "MSG", &CEvt::_cmd_MSG },
	{ EVT_CMD_POS, 3, "POS", &CEvt::_cmd_POS },
	{ EVT_CMD_PRINTF, 1, "PRINTF", &CEvt::_cmd_PRINTF },
	{ EVT_CMD_PTCL, 3, "PTCL", &CEvt::_cmd_PTCL },
	{ EVT_CMD_READ, 3, "READ", &CEvt::_cmd_READ },
	{ EVT_CMD_SCALE, 3, "SCALE", &CEvt::_cmd_SCALE },
	{ EVT_CMD_MSCALE, 3, "MSCALE", &CEvt::_cmd_SCALE },
	{ EVT_CMD_SCRN, 3, "SCRN", &CEvt::_cmd_SCRN },
	{ EVT_CMD_SELECT, 3, "SELECT", &CEvt::_cmd_MSG },
	{ EVT_CMD_SFX, 3, "SFX", &CEvt::_cmd_SFX },
	{ EVT_CMD_TIMER, 3, "TIMER", &CEvt::_cmd_TIMER },
	{ EVT_CMD_WAIT, 3, "WAIT", &CEvt::_cmd_WAIT },
	{ EVT_CMD_WARP, 3, "WARP", &CEvt::_cmd_WARP },
	{ EVT_CMD_WIN, 3, "WIN", &CEvt::_cmd_WIN },
	{ 0 },
};

// the event system's own object, and the camera states scripts save and load
XObj gEventScriptObject;
CCameraMemento gEventCameraMementos[4];

// the game object

/**
 * @note Address: 0x8006FD8C
 * @note Size: 0x48C
 */
// CEvt::_cmd_WIN
/*
    stwu     r1, -0x60(r1)
    mflr     r0
    stw      r0, 0x64(r1)
    stfd     f31, 0x50(r1)
    psq_st   f31, 0x58(r1), 0, qr0
    stfd     f30, 0x40(r1)
    psq_st   f30, 0x48(r1), 0, qr0
    stw      r31, 0x3c(r1)
    stw      r30, 0x38(r1)
    stw      r29, 0x34(r1)
    stw      r28, 0x30(r1)
    mr       r29, r3
    bl       CEvt_Evaluate
    clrlwi   r0, r3, 16
    cmpwi    r0, 0xdb
    beq      .L_80070140
    bge      .L_8006FDF4
    cmpwi    r0, 0xd0
    beq      .L_8006FEB8
    bge      .L_8006FDE8
    cmpwi    r0, 0xcf
    bge      .L_8006FE00
    b        .L_800701E4
.L_8006FDE8:
    cmpwi    r0, 0xd2
    bge      .L_800701E4
    b        .L_800701A8
.L_8006FDF4:
    cmpwi    r0, 0xef
    beq      .L_8006FE7C
    b        .L_800701E4
.L_8006FE00:
    mr       r3, r29
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x14(r1)
    mr       r3, r29
    lfd      f1, lbl_8065CF88@sda21(r0)
    stw      r0, 0x10(r1)
    lfd      f0, 0x10(r1)
    fsubs    f31, f0, f1
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x1c(r1)
    lwz      r3, 0x54(r29)
    stw      r0, 0x18(r1)
    lfd      f1, lbl_8065CF88@sda21(r0)
    cmplwi   r3, 0x0
    lfd      f0, 0x18(r1)
    fsubs    f1, f0, f1
    beq      .L_800701E4
    addis    r3, r3, 0x4
    li       r0, 0x0
    stfs     f31, 0x4600(r3)
    lfs      f0, lbl_8065CF74@sda21(r0)
    stfs     f1, 0x4604(r3)
    stw      r0, 0x4608(r3)
    stfs     f0, 0x460c(r3)
    stfs     f0, 0x4610(r3)
    stfs     f0, 0x4614(r3)
    b        .L_800701E4
.L_8006FE7C:
    lwz      r3, 0x54(r29)
    cmplwi   r3, 0x0
    beq      .L_800701E4
    lfs      f0, lbl_8065CF78@sda21(r0)
    addis    r3, r3, 0x4
    lfs      f1, lbl_8065CF7C@sda21(r0)
    li       r0, 0x0
    stfs     f0, 0x4600(r3)
    lfs      f0, lbl_8065CF74@sda21(r0)
    stfs     f1, 0x4604(r3)
    stw      r0, 0x4608(r3)
    stfs     f0, 0x460c(r3)
    stfs     f0, 0x4610(r3)
    stfs     f0, 0x4614(r3)
    b        .L_800701E4
.L_8006FEB8:
    mr       r3, r29
    bl       CEvt_Evaluate
    extsh    r30, r3
    cmpwi    r30, 0x2710
    blt      .L_8006FF7C
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_8006FF64
.L_8006FEE4:
    lha      r0, 0x4(r3)
    cmpw     r0, r30
    bne      .L_8006FF58
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r31, -0x67a0(r3)
    cmplwi   r31, 0x0
    beq      .L_8006FF50
    lwz      r0, 0x228(r31)
    li       r4, 0x0
    lwz      r5, 0x22c(r31)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_8006FF50
    b        .L_8006FF68
.L_8006FF50:
    li       r31, 0x0
    b        .L_8006FF68
.L_8006FF58:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_8006FEE4
.L_8006FF64:
    li       r31, 0x0
.L_8006FF68:
    cmplwi   r31, 0x0
    bne      .L_8006FFB4
    li       r31, 0x0
    b        .L_8006FFB4
    b        .L_8006FFB4
.L_8006FF7C:
    extsh.   r0, r30
    blt      .L_8006FFA0
    lis      r3, gGame@ha
    slwi     r0, r30, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r31, -0x67a0(r3)
    b        .L_8006FFB4
.L_8006FFA0:
    cmpwi    r30, -0x1
    bne      .L_8006FFB0
    lwz      r31, 0x8(r29)
    b        .L_8006FFB4
.L_8006FFB0:
    mr       r31, r30
.L_8006FFB4:
    mr       r3, r29
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x1c(r1)
    mr       r3, r29
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x18(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x18(r1)
    fsubs    f1, f1, f2
    fdivs    f31, f1, f0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x14(r1)
    mr       r3, r29
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x10(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x10(r1)
    fsubs    f1, f1, f2
    fdivs    f30, f1, f0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x24(r1)
    cmplwi   r31, 0x0
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x20(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x20(r1)
    fsubs    f1, f1, f2
    fdivs    f0, f1, f0
    beq      .L_8007008C
    lwz      r0, 0x228(r31)
    li       r4, 0x0
    lwz      r5, 0x22c(r31)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_8007008C
    lwz      r3, 0x54(r29)
    cmplwi   r3, 0x0
    beq      .L_800701E4
    addis    r3, r3, 0x4
    stw      r31, 0x4608(r3)
    stfs     f31, 0x460c(r3)
    stfs     f30, 0x4610(r3)
    stfs     f0, 0x4614(r3)
    b        .L_800701E4
.L_8007008C:
    lis      r3, sEvtWinObjectKilled@ha
    mr       r4, r30
    addi     r3, r3, sEvtWinObjectKilled@l
    mr       r5, r30
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_800700F4
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800700E0
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800700E0
.L_800700D8:
    addi     r3, r3, 0x8
    bdnz     .L_800700D8
.L_800700E0:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800700F4
.L_800700F0:
    bdnz     .L_800700F0
.L_800700F4:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_800701E4
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80070128
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80070128
.L_80070120:
    addi     r3, r3, 0x8
    bdnz     .L_80070120
.L_80070128:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800701E4
.L_80070138:
    bdnz     .L_80070138
    b        .L_800701E4
.L_80070140:
    mr       r3, r29
    bl       CEvt_Evaluate
    clrlwi   r30, r3, 24
    mr       r3, r29
    bl       CEvt_Evaluate
    clrlwi   r31, r3, 24
    mr       r3, r29
    bl       CEvt_Evaluate
    clrlwi   r28, r3, 24
    mr       r3, r29
    bl       CEvt_Evaluate
    lwz      r4, 0x54(r29)
    clrlwi   r3, r3, 24
    cmplwi   r4, 0x0
    beq      .L_800701E4
    lwz      r0, sEvtWindowColorTemplate@sda21(r0)
    stb      r30, 0x9b8(r4)
    stb      r31, 0x9b9(r4)
    stw      r0, 0xc(r1)
    stb      r28, 0x9ba(r4)
    stb      r30, 0xc(r1)
    stb      r31, 0xd(r1)
    stb      r28, 0xe(r1)
    stb      r3, 0xf(r1)
    stb      r3, 0x9bb(r4)
    b        .L_800701E4
.L_800701A8:
    lwz      r3, 0x54(r29)
    cmplwi   r3, 0x0
    beq      .L_800701E4
    bl       CMsg_ResetPos
    lwz      r0, sEvtWindowDefaultColor@sda21(r0)
    lwz      r4, 0x54(r29)
    stw      r0, 0x8(r1)
    lbz      r3, 0x8(r1)
    lbz      r0, 0x9(r1)
    stb      r3, 0x9b8(r4)
    lbz      r3, 0xa(r1)
    stb      r0, 0x9b9(r4)
    lbz      r0, 0xb(r1)
    stb      r3, 0x9ba(r4)
    stb      r0, 0x9bb(r4)
.L_800701E4:
    li       r3, 0x0
    psq_l    f31, 0x58(r1), 0, qr0
    lfd      f31, 0x50(r1)
    psq_l    f30, 0x48(r1), 0, qr0
    lfd      f30, 0x40(r1)
    lwz      r31, 0x3c(r1)
    lwz      r30, 0x38(r1)
    lwz      r29, 0x34(r1)
    lwz      r0, 0x64(r1)
    lwz      r28, 0x30(r1)
    mtlr     r0
    addi     r1, r1, 0x60
    blr
*/

#define EVT_PLAYER (*gGame.GetPlayer())

extern "C" s32 GetStageNum(void);

// WARP, ABORT and RETURN all finish through this
static inline void CEvt_End(CEvt* evt)
{
	evt->mData.mScript = 0;
	if (evt->_54 != 0 && evt->_58 != 0) {
		evt->_58 = 0;
#if defined(VERSION_JP)
		*(s32*)(evt->_54 + 0x43F70) = 1;
#else
		*(s32*)(evt->_54 + 0x445C0) = 1;
#endif
	}
	Inventory* inventory = &EVT_PLAYER.mParams.mInventory;
	inventory->mCompact  = TRUE;
	CompactInventory(inventory);
	evt->mData.mScript = 0;
}

/**
 * @note Address: 0x80070304
 * @note Size: 0x1B9C
 */
// CEvt::_cmd_WAIT
/*
    stwu     r1, -0x60(r1)
    mflr     r0
    stw      r0, 0x64(r1)
    stfd     f31, 0x50(r1)
    psq_st   f31, 0x58(r1), 0, qr0
    stmw     r25, 0x34(r1)
    mr       r31, r3
    lis      r7, lbl_80207FA0@ha
    lwz      r6, 0x4(r3)
    mr       r30, r4
    mr       r25, r5
    addi     r29, r7, lbl_80207FA0@l
    subi     r28, r6, 0x1
    li       r27, 0x0
    bl       CEvt_Evaluate
    clrlwi   r3, r3, 16
    subi     r0, r3, 0xc8
    cmplwi   r0, 0x2e
    mr       r4, r3
    bgt      .L_80071E18
    lis      r3, jumptable_8022506C@ha
    slwi     r0, r0, 2
    addi     r3, r3, jumptable_8022506C@l
    lwzx     r0, r3, r0
    mtctr    r0
    bctr
.L_8007036C:
    mr       r3, r31
    bl       CEvt_Evaluate
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x2e
    bne      .L_80070448
    lwz      r4, 0x18(r31)
    cmpwi    r4, 0x0
    bne      .L_80070420
    lis      r4, 0x51ec
    lis      r0, 0x4330
    subi     r4, r4, 0x7ae1
    stw      r0, 0x18(r1)
    mulhw    r4, r4, r3
    lfd      f2, lbl_8065CF88@sda21(r0)
    lfs      f3, lbl_8065CF80@sda21(r0)
    li       r27, 0x1
    stw      r0, 0x20(r1)
    srawi    r5, r4, 5
    srawi    r0, r4, 5
    srwi     r4, r0, 31
    srwi     r6, r5, 31
    add      r0, r0, r4
    mulli    r0, r0, 0x64
    add      r4, r5, r6
    subf     r0, r0, r3
    mulli    r0, r0, 0x3c
    xoris    r0, r0, 0x8000
    stw      r0, 0x1c(r1)
    mulli    r0, r4, 0x1770
    lfd      f0, 0x18(r1)
    stw      r0, 0x18(r31)
    fsubs    f0, f0, f2
    lwz      r0, 0x18(r31)
    fdivs    f1, f0, f3
    xoris    r0, r0, 0x8000
    stw      r0, 0x24(r1)
    lfd      f0, 0x20(r1)
    fsubs    f0, f0, f2
    fmadds   f0, f3, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x28(r1)
    lwz      r0, 0x2c(r1)
    stw      r0, 0x18(r31)
    stw      r28, 0x4(r31)
    b        .L_80071E80
.L_80070420:
    mulli    r0, r25, 0x64
    subf.    r0, r0, r4
    stw      r0, 0x18(r31)
    bgt      .L_8007043C
    li       r0, 0x0
    stw      r0, 0x18(r31)
    b        .L_80071E80
.L_8007043C:
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_80070448:
    li       r0, 0x1
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_80070454:
    lis      r3, gGame@ha
    lfs      f1, lbl_8065CF74@sda21(r0)
    addi     r3, r3, gGame@l
    li       r0, 0x0
    lfs      f0, 0x3e90(r3)
    fcmpu    cr0, f1, f0
    bne      .L_80070498
    lfs      f0, 0x3e94(r3)
    fcmpu    cr0, f1, f0
    bne      .L_80070498
    lfs      f0, 0x3e98(r3)
    fcmpu    cr0, f1, f0
    bne      .L_80070498
    lfs      f0, 0x3e9c(r3)
    fcmpu    cr0, f1, f0
    bne      .L_80070498
    li       r0, 0x1
.L_80070498:
    clrlwi   r4, r0, 24
    clrlwi   r0, r30, 24
    neg      r3, r4
    or       r3, r3, r4
    cmplwi   r0, 0x2e
    srwi     r0, r3, 31
    bne      .L_800704C8
    cmpwi    r0, 0x0
    bne      .L_80071E80
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_800704C8:
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_800704D0:
    lis      r3, gGame@ha
    lfs      f1, lbl_8065CF74@sda21(r0)
    addi     r3, r3, gGame@l
    li       r0, 0x0
    lfs      f0, 0x46dc(r3)
    fcmpu    cr0, f1, f0
    bne      .L_80070514
    lfs      f0, 0x46e0(r3)
    fcmpu    cr0, f1, f0
    bne      .L_80070514
    lfs      f0, 0x46e4(r3)
    fcmpu    cr0, f1, f0
    bne      .L_80070514
    lfs      f0, 0x46e8(r3)
    fcmpu    cr0, f1, f0
    bne      .L_80070514
    li       r0, 0x1
.L_80070514:
    clrlwi   r4, r0, 24
    clrlwi   r0, r30, 24
    neg      r3, r4
    or       r3, r3, r4
    cmplwi   r0, 0x2e
    srwi     r0, r3, 31
    bne      .L_80070544
    cmpwi    r0, 0x0
    bne      .L_80071E80
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_80070544:
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_8007054C:
    lis      r3, gGame@ha
    li       r4, 0x0
    addi     r3, r3, gGame@l
    lfs      f1, lbl_8065CF74@sda21(r0)
    lfs      f0, 0x3ef8(r3)
    mr       r0, r4
    fcmpu    cr0, f1, f0
    bne      .L_8007057C
    lfs      f0, 0x3efc(r3)
    fcmpu    cr0, f1, f0
    bne      .L_8007057C
    li       r0, 0x1
.L_8007057C:
    clrlwi.  r0, r0, 24
    beq      .L_80070624
    lis      r3, gGame@ha
    lfs      f1, lbl_8065CF74@sda21(r0)
    addi     r3, r3, gGame@l
    li       r0, 0x0
    lfs      f0, 0x3f5c(r3)
    fcmpu    cr0, f1, f0
    bne      .L_800705B0
    lfs      f0, 0x3f60(r3)
    fcmpu    cr0, f1, f0
    bne      .L_800705B0
    li       r0, 0x1
.L_800705B0:
    clrlwi.  r0, r0, 24
    beq      .L_80070624
    lis      r3, gGame@ha
    lfs      f1, lbl_8065CF74@sda21(r0)
    addi     r3, r3, gGame@l
    li       r0, 0x0
    lfs      f0, 0x40c0(r3)
    fcmpu    cr0, f1, f0
    bne      .L_800705E4
    lfs      f0, 0x40c4(r3)
    fcmpu    cr0, f1, f0
    bne      .L_800705E4
    li       r0, 0x1
.L_800705E4:
    clrlwi.  r0, r0, 24
    beq      .L_80070624
    lis      r3, gGame@ha
    lfs      f1, lbl_8065CF74@sda21(r0)
    addi     r3, r3, gGame@l
    li       r0, 0x0
    lfs      f0, 0x4124(r3)
    fcmpu    cr0, f1, f0
    bne      .L_80070618
    lfs      f0, 0x4128(r3)
    fcmpu    cr0, f1, f0
    bne      .L_80070618
    li       r0, 0x1
.L_80070618:
    clrlwi.  r0, r0, 24
    beq      .L_80070624
    li       r4, 0x1
.L_80070624:
    clrlwi   r4, r4, 24
    clrlwi   r0, r30, 24
    neg      r3, r4
    or       r3, r3, r4
    cmplwi   r0, 0x2e
    srwi     r0, r3, 31
    bne      .L_80070654
    cmpwi    r0, 0x0
    bne      .L_80071E80
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_80070654:
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_8007065C:
    lis      r3, gGame@ha
    lfs      f1, lbl_8065CF74@sda21(r0)
    addi     r3, r3, gGame@l
    li       r0, 0x0
    lfs      f0, 0x4188(r3)
    fcmpu    cr0, f1, f0
    bne      .L_80070688
    lfs      f0, 0x418c(r3)
    fcmpu    cr0, f1, f0
    bne      .L_80070688
    li       r0, 0x1
.L_80070688:
    clrlwi   r4, r0, 24
    clrlwi   r0, r30, 24
    neg      r3, r4
    or       r3, r3, r4
    cmplwi   r0, 0x2e
    srwi     r0, r3, 31
    bne      .L_800706B8
    cmpwi    r0, 0x0
    bne      .L_80071E80
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_800706B8:
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_800706C0:
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x2e
    beq      .L_80071E80
    li       r0, 0x1
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_800706D8:
    mr       r3, r31
    bl       CEvt_Evaluate
    extsh    r25, r3
    cmpwi    r25, 0x2710
    blt      .L_8007079C
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80070784
.L_80070704:
    lha      r0, 0x4(r3)
    cmpw     r0, r25
    bne      .L_80070778
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r26, -0x67a0(r3)
    cmplwi   r26, 0x0
    beq      .L_80070770
    lwz      r0, 0x228(r26)
    li       r4, 0x0
    lwz      r5, 0x22c(r26)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_80070770
    b        .L_80070788
.L_80070770:
    li       r26, 0x0
    b        .L_80070788
.L_80070778:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_80070704
.L_80070784:
    li       r26, 0x0
.L_80070788:
    cmplwi   r26, 0x0
    bne      .L_800707D4
    li       r26, 0x0
    b        .L_800707D4
    b        .L_800707D4
.L_8007079C:
    extsh.   r0, r25
    blt      .L_800707C0
    lis      r3, gGame@ha
    slwi     r0, r25, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r26, -0x67a0(r3)
    b        .L_800707D4
.L_800707C0:
    cmpwi    r25, -0x1
    bne      .L_800707D0
    lwz      r26, 0x8(r31)
    b        .L_800707D4
.L_800707D0:
    mr       r26, r25
.L_800707D4:
    mr       r3, r31
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x2c(r1)
    cmplwi   r26, 0x0
    lfd      f1, lbl_8065CF88@sda21(r0)
    stw      r0, 0x28(r1)
    lfd      f0, 0x28(r1)
    fsubs    f31, f0, f1
    beq      .L_80070A50
    lwz      r0, 0x228(r26)
    li       r4, 0x0
    lwz      r5, 0x22c(r26)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_80070A50
    lwz      r0, 0x0(r26)
    cmplwi   r0, 0x0
    bne      .L_800708E0
    mr       r4, r25
    mr       r5, r25
    addi     r3, r29, 0x1b8
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80070898
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80070884
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80070884
.L_8007087C:
    addi     r3, r3, 0x8
    bdnz     .L_8007087C
.L_80070884:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80070898
.L_80070894:
    bdnz     .L_80070894
.L_80070898:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_800708E0
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800708CC
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800708CC
.L_800708C4:
    addi     r3, r3, 0x8
    bdnz     .L_800708C4
.L_800708CC:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800708E0
.L_800708DC:
    bdnz     .L_800708DC
.L_800708E0:
    lwz      r25, 0x0(r26)
    cmplwi   r25, 0x0
    beq      .L_80070928
    mr       r3, r25
    bl       FindJointAnimationInTree
    cmplwi   r3, 0x0
    beq      .L_80070900
    b        .L_8007092C
.L_80070900:
    mr       r3, r25
    bl       FindMaterialOrTextureAnimationInJointTree
    cmplwi   r3, 0x0
    beq      .L_80070914
    b        .L_8007092C
.L_80070914:
    mr       r3, r25
    bl       FindPolygonAnimationInJointTree
    cmplwi   r3, 0x0
    beq      .L_80070928
    b        .L_8007092C
.L_80070928:
    li       r3, 0x0
.L_8007092C:
    cmplwi   r3, 0x0
    beq      .L_80070A38
    lfs      f0, lbl_8065CF74@sda21(r0)
    fcmpo    cr0, f31, f0
    cror     eq, gt, eq
    bne      .L_800709FC
    lwz      r25, 0x0(r26)
    cmplwi   r25, 0x0
    beq      .L_80070998
    mr       r3, r25
    bl       FindJointAnimationInTree
    cmplwi   r3, 0x0
    beq      .L_80070968
    mr       r25, r3
    b        .L_8007099C
.L_80070968:
    mr       r3, r25
    bl       FindMaterialOrTextureAnimationInJointTree
    cmplwi   r3, 0x0
    beq      .L_80070980
    mr       r25, r3
    b        .L_8007099C
.L_80070980:
    mr       r3, r25
    bl       FindPolygonAnimationInJointTree
    cmplwi   r3, 0x0
    beq      .L_80070998
    mr       r25, r3
    b        .L_8007099C
.L_80070998:
    li       r25, 0x0
.L_8007099C:
    cmplwi   r25, 0x0
    beq      .L_800709C0
    bne      .L_800709B8
    li       r3, lbl_8065CF90@sda21
    li       r4, 0x95
    li       r5, lbl_8065CF98@sda21
    bl       __assert
.L_800709B8:
    lfs      f0, 0x4(r25)
    b        .L_800709C4
.L_800709C0:
    lfs      f0, lbl_8065CF74@sda21(r0)
.L_800709C4:
    fcmpo    cr0, f0, f31
    cror     eq, gt, eq
    mfcr     r3
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x2e
    extrwi   r0, r3, 1, 2
    bne      .L_800709F4
    cmpwi    r0, 0x0
    bne      .L_80071E80
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_800709F4:
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_800709FC:
    lwz      r3, 0x0(r3)
    clrlwi   r0, r30, 24
    andis.   r3, r3, 0x4400
    cmplwi   r0, 0x2e
    neg      r0, r3
    or       r0, r0, r3
    srwi     r0, r0, 31
    bne      .L_80070A30
    cmpwi    r0, 0x0
    bne      .L_80071E80
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_80070A30:
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_80070A38:
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x18
    bne      .L_80071E80
    li       r0, 0x1
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_80070A50:
    mr       r4, r25
    mr       r5, r25
    addi     r3, r29, 0x1e4
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80070AB4
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80070AA0
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80070AA0
.L_80070A98:
    addi     r3, r3, 0x8
    bdnz     .L_80070A98
.L_80070AA0:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80070AB4
.L_80070AB0:
    bdnz     .L_80070AB0
.L_80070AB4:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80070AFC
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80070AE8
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80070AE8
.L_80070AE0:
    addi     r3, r3, 0x8
    bdnz     .L_80070AE0
.L_80070AE8:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80070AFC
.L_80070AF8:
    bdnz     .L_80070AF8
.L_80070AFC:
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x18
    bne      .L_80071E80
    li       r0, 0x1
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_80070B14:
    mr       r3, r31
    bl       CEvt_Evaluate
    extsh    r4, r3
    cmpwi    r4, 0x2710
    blt      .L_80070BD8
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r6, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80070BC0
.L_80070B40:
    lha      r0, 0x4(r3)
    cmpw     r0, r4
    bne      .L_80070BB4
    lis      r3, gEventObjectIds@ha
    lis      r5, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r6, 2
    add      r3, r3, r0
    addi     r5, r5, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r5, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r26, -0x67a0(r3)
    cmplwi   r26, 0x0
    beq      .L_80070BAC
    lwz      r0, 0x228(r26)
    li       r5, 0x0
    lwz      r6, 0x22c(r26)
    li       r3, 0x1
    and      r0, r0, r5
    and      r3, r6, r3
    xor      r3, r3, r5
    xor      r0, r0, r5
    or.      r0, r3, r0
    beq      .L_80070BAC
    b        .L_80070BC4
.L_80070BAC:
    li       r26, 0x0
    b        .L_80070BC4
.L_80070BB4:
    addi     r3, r3, 0x4
    addi     r6, r6, 0x1
    bdnz     .L_80070B40
.L_80070BC0:
    li       r26, 0x0
.L_80070BC4:
    cmplwi   r26, 0x0
    bne      .L_80070C10
    li       r26, 0x0
    b        .L_80070C10
    b        .L_80070C10
.L_80070BD8:
    extsh.   r0, r4
    blt      .L_80070BFC
    lis      r3, gGame@ha
    slwi     r0, r4, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r26, -0x67a0(r3)
    b        .L_80070C10
.L_80070BFC:
    cmpwi    r4, -0x1
    bne      .L_80070C0C
    lwz      r26, 0x8(r31)
    b        .L_80070C10
.L_80070C0C:
    mr       r26, r4
.L_80070C10:
    cmplwi   r26, 0x0
    beq      .L_80070D30
    lwz      r0, 0x228(r26)
    li       r5, 0x0
    lwz      r6, 0x22c(r26)
    li       r3, 0x1
    and      r0, r0, r5
    and      r3, r6, r3
    xor      r3, r3, r5
    xor      r0, r0, r5
    or.      r0, r3, r0
    beq      .L_80070D30
    lwz      r0, 0x0(r26)
    cmplwi   r0, 0x0
    bne      .L_80070CF4
    mr       r5, r4
    addi     r3, r29, 0x20c
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80070CAC
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80070C98
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80070C98
.L_80070C90:
    addi     r3, r3, 0x8
    bdnz     .L_80070C90
.L_80070C98:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80070CAC
.L_80070CA8:
    bdnz     .L_80070CA8
.L_80070CAC:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80070CF4
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80070CE0
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80070CE0
.L_80070CD8:
    addi     r3, r3, 0x8
    bdnz     .L_80070CD8
.L_80070CE0:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80070CF4
.L_80070CF0:
    bdnz     .L_80070CF0
.L_80070CF4:
    lfs      f1, 0xfc(r26)
    lfs      f0, 0x10c(r26)
    fcmpu    cr0, f1, f0
    mfcr     r3
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x2e
    extrwi   r0, r3, 1, 2
    bne      .L_80070D28
    cmpwi    r0, 0x0
    bne      .L_80071E80
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_80070D28:
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_80070D30:
    mr       r5, r4
    addi     r3, r29, 0x238
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80070D90
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80070D7C
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80070D7C
.L_80070D74:
    addi     r3, r3, 0x8
    bdnz     .L_80070D74
.L_80070D7C:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80070D90
.L_80070D8C:
    bdnz     .L_80070D8C
.L_80070D90:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80070DD8
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80070DC4
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80070DC4
.L_80070DBC:
    addi     r3, r3, 0x8
    bdnz     .L_80070DBC
.L_80070DC4:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80070DD8
.L_80070DD4:
    bdnz     .L_80070DD4
.L_80070DD8:
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x18
    bne      .L_80071E80
    li       r0, 0x1
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_80070DF0:
    mr       r3, r31
    bl       CEvt_Evaluate
    extsh    r4, r3
    cmpwi    r4, 0x2710
    blt      .L_80070EB4
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r6, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80070E9C
.L_80070E1C:
    lha      r0, 0x4(r3)
    cmpw     r0, r4
    bne      .L_80070E90
    lis      r3, gEventObjectIds@ha
    lis      r5, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r6, 2
    add      r3, r3, r0
    addi     r5, r5, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r5, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r26, -0x67a0(r3)
    cmplwi   r26, 0x0
    beq      .L_80070E88
    lwz      r0, 0x228(r26)
    li       r5, 0x0
    lwz      r6, 0x22c(r26)
    li       r3, 0x1
    and      r0, r0, r5
    and      r3, r6, r3
    xor      r3, r3, r5
    xor      r0, r0, r5
    or.      r0, r3, r0
    beq      .L_80070E88
    b        .L_80070EA0
.L_80070E88:
    li       r26, 0x0
    b        .L_80070EA0
.L_80070E90:
    addi     r3, r3, 0x4
    addi     r6, r6, 0x1
    bdnz     .L_80070E1C
.L_80070E9C:
    li       r26, 0x0
.L_80070EA0:
    cmplwi   r26, 0x0
    bne      .L_80070EEC
    li       r26, 0x0
    b        .L_80070EEC
    b        .L_80070EEC
.L_80070EB4:
    extsh.   r0, r4
    blt      .L_80070ED8
    lis      r3, gGame@ha
    slwi     r0, r4, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r26, -0x67a0(r3)
    b        .L_80070EEC
.L_80070ED8:
    cmpwi    r4, -0x1
    bne      .L_80070EE8
    lwz      r26, 0x8(r31)
    b        .L_80070EEC
.L_80070EE8:
    mr       r26, r4
.L_80070EEC:
    cmplwi   r26, 0x0
    beq      .L_8007104C
    lwz      r0, 0x228(r26)
    li       r5, 0x0
    lwz      r6, 0x22c(r26)
    li       r3, 0x1
    and      r0, r0, r5
    and      r3, r6, r3
    xor      r3, r3, r5
    xor      r0, r0, r5
    or.      r0, r3, r0
    beq      .L_8007104C
    lwz      r0, 0x0(r26)
    cmplwi   r0, 0x0
    bne      .L_80070FD0
    mr       r5, r4
    addi     r3, r29, 0x25c
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80070F88
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80070F74
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80070F74
.L_80070F6C:
    addi     r3, r3, 0x8
    bdnz     .L_80070F6C
.L_80070F74:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80070F88
.L_80070F84:
    bdnz     .L_80070F84
.L_80070F88:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80070FD0
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80070FBC
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80070FBC
.L_80070FB4:
    addi     r3, r3, 0x8
    bdnz     .L_80070FB4
.L_80070FBC:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80070FD0
.L_80070FCC:
    bdnz     .L_80070FCC
.L_80070FD0:
    lwz      r0, 0xd8(r26)
    li       r3, 0x0
    lwz      r4, 0xdc(r26)
    stw      r0, 0x8(r1)
    lwz      r0, 0xe0(r26)
    lfs      f1, 0xc8(r26)
    lfs      f0, 0x8(r1)
    stw      r4, 0xc(r1)
    fcmpu    cr0, f1, f0
    stw      r0, 0x10(r1)
    bne      .L_80071020
    lfs      f1, 0xcc(r26)
    lfs      f0, 0xc(r1)
    fcmpu    cr0, f1, f0
    bne      .L_80071020
    lfs      f1, 0xd0(r26)
    lfs      f0, 0x10(r1)
    fcmpu    cr0, f1, f0
    bne      .L_80071020
    li       r3, 0x1
.L_80071020:
    clrlwi   r0, r30, 24
    clrlwi   r3, r3, 24
    cmplwi   r0, 0x2e
    bne      .L_80071044
    cmpwi    r3, 0x0
    bne      .L_80071E80
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_80071044:
    stw      r3, 0x10(r31)
    b        .L_80071E80
.L_8007104C:
    mr       r5, r4
    addi     r3, r29, 0x288
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_800710AC
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80071098
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80071098
.L_80071090:
    addi     r3, r3, 0x8
    bdnz     .L_80071090
.L_80071098:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800710AC
.L_800710A8:
    bdnz     .L_800710A8
.L_800710AC:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_800710F4
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800710E0
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800710E0
.L_800710D8:
    addi     r3, r3, 0x8
    bdnz     .L_800710D8
.L_800710E0:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800710F4
.L_800710F0:
    bdnz     .L_800710F0
.L_800710F4:
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x18
    bne      .L_80071E80
    li       r0, 0x1
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_8007110C:
    mr       r3, r31
    bl       CEvt_Evaluate
    extsh    r4, r3
    cmpwi    r4, 0x2710
    blt      .L_800711D0
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r6, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_800711B8
.L_80071138:
    lha      r0, 0x4(r3)
    cmpw     r0, r4
    bne      .L_800711AC
    lis      r3, gEventObjectIds@ha
    lis      r5, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r6, 2
    add      r3, r3, r0
    addi     r5, r5, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r5, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r26, -0x67a0(r3)
    cmplwi   r26, 0x0
    beq      .L_800711A4
    lwz      r0, 0x228(r26)
    li       r5, 0x0
    lwz      r6, 0x22c(r26)
    li       r3, 0x1
    and      r0, r0, r5
    and      r3, r6, r3
    xor      r3, r3, r5
    xor      r0, r0, r5
    or.      r0, r3, r0
    beq      .L_800711A4
    b        .L_800711BC
.L_800711A4:
    li       r26, 0x0
    b        .L_800711BC
.L_800711AC:
    addi     r3, r3, 0x4
    addi     r6, r6, 0x1
    bdnz     .L_80071138
.L_800711B8:
    li       r26, 0x0
.L_800711BC:
    cmplwi   r26, 0x0
    bne      .L_80071208
    li       r26, 0x0
    b        .L_80071208
    b        .L_80071208
.L_800711D0:
    extsh.   r0, r4
    blt      .L_800711F4
    lis      r3, gGame@ha
    slwi     r0, r4, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r26, -0x67a0(r3)
    b        .L_80071208
.L_800711F4:
    cmpwi    r4, -0x1
    bne      .L_80071204
    lwz      r26, 0x8(r31)
    b        .L_80071208
.L_80071204:
    mr       r26, r4
.L_80071208:
    cmplwi   r26, 0x0
    beq      .L_80071328
    lwz      r0, 0x228(r26)
    li       r5, 0x0
    lwz      r6, 0x22c(r26)
    li       r3, 0x1
    and      r0, r0, r5
    and      r3, r6, r3
    xor      r3, r3, r5
    xor      r0, r0, r5
    or.      r0, r3, r0
    beq      .L_80071328
    lwz      r0, 0x0(r26)
    cmplwi   r0, 0x0
    bne      .L_800712EC
    mr       r5, r4
    addi     r3, r29, 0x2b0
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_800712A4
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80071290
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80071290
.L_80071288:
    addi     r3, r3, 0x8
    bdnz     .L_80071288
.L_80071290:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800712A4
.L_800712A0:
    bdnz     .L_800712A0
.L_800712A4:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_800712EC
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800712D8
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800712D8
.L_800712D0:
    addi     r3, r3, 0x8
    bdnz     .L_800712D0
.L_800712D8:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800712EC
.L_800712E8:
    bdnz     .L_800712E8
.L_800712EC:
    lfs      f1, 0x12c(r26)
    lfs      f0, 0x13c(r26)
    fcmpu    cr0, f1, f0
    mfcr     r3
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x2e
    extrwi   r0, r3, 1, 2
    bne      .L_80071320
    cmpwi    r0, 0x0
    bne      .L_80071E80
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_80071320:
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_80071328:
    mr       r5, r4
    addi     r3, r29, 0x2dc
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80071388
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80071374
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80071374
.L_8007136C:
    addi     r3, r3, 0x8
    bdnz     .L_8007136C
.L_80071374:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80071388
.L_80071384:
    bdnz     .L_80071384
.L_80071388:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_800713D0
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800713BC
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800713BC
.L_800713B4:
    addi     r3, r3, 0x8
    bdnz     .L_800713B4
.L_800713BC:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800713D0
.L_800713CC:
    bdnz     .L_800713CC
.L_800713D0:
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x18
    bne      .L_80071E80
    li       r0, 0x1
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_800713E8:
    mr       r3, r31
    bl       CEvt_Evaluate
    lis      r4, gGame@ha
    addi     r5, r4, gGame@l
    mr       r4, r3
    addis    r3, r5, 0xe
    addi     r3, r3, 0x7e24
    bl       IsGameSoundPlaying
    clrlwi   r0, r30, 24
    cntlzw   r3, r3
    cmplwi   r0, 0x2e
    extrwi   r0, r3, 8, 19
    bne      .L_80071430
    cmpwi    r0, 0x0
    bne      .L_80071E80
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_80071430:
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_80071438:
    mr       r3, r31
    bl       CEvt_Evaluate
    extsh    r4, r3
    cmpwi    r4, 0x2710
    blt      .L_800714FC
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r6, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_800714E4
.L_80071464:
    lha      r0, 0x4(r3)
    cmpw     r0, r4
    bne      .L_800714D8
    lis      r3, gEventObjectIds@ha
    lis      r5, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r6, 2
    add      r3, r3, r0
    addi     r5, r5, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r5, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r26, -0x67a0(r3)
    cmplwi   r26, 0x0
    beq      .L_800714D0
    lwz      r0, 0x228(r26)
    li       r5, 0x0
    lwz      r6, 0x22c(r26)
    li       r3, 0x1
    and      r0, r0, r5
    and      r3, r6, r3
    xor      r3, r3, r5
    xor      r0, r0, r5
    or.      r0, r3, r0
    beq      .L_800714D0
    b        .L_800714E8
.L_800714D0:
    li       r26, 0x0
    b        .L_800714E8
.L_800714D8:
    addi     r3, r3, 0x4
    addi     r6, r6, 0x1
    bdnz     .L_80071464
.L_800714E4:
    li       r26, 0x0
.L_800714E8:
    cmplwi   r26, 0x0
    bne      .L_80071534
    li       r26, 0x0
    b        .L_80071534
    b        .L_80071534
.L_800714FC:
    extsh.   r0, r4
    blt      .L_80071520
    lis      r3, gGame@ha
    slwi     r0, r4, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r26, -0x67a0(r3)
    b        .L_80071534
.L_80071520:
    cmpwi    r4, -0x1
    bne      .L_80071530
    lwz      r26, 0x8(r31)
    b        .L_80071534
.L_80071530:
    mr       r26, r4
.L_80071534:
    cmplwi   r26, 0x0
    beq      .L_800716BC
    lwz      r0, 0x228(r26)
    li       r5, 0x0
    lwz      r6, 0x22c(r26)
    li       r3, 0x1
    and      r0, r0, r5
    and      r3, r6, r3
    xor      r3, r3, r5
    xor      r0, r0, r5
    or.      r0, r3, r0
    beq      .L_800716BC
    lwz      r0, 0x0(r26)
    cmplwi   r0, 0x0
    bne      .L_80071618
    mr       r5, r4
    addi     r3, r29, 0x304
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_800715D0
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800715BC
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800715BC
.L_800715B4:
    addi     r3, r3, 0x8
    bdnz     .L_800715B4
.L_800715BC:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800715D0
.L_800715CC:
    bdnz     .L_800715CC
.L_800715D0:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80071618
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80071604
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80071604
.L_800715FC:
    addi     r3, r3, 0x8
    bdnz     .L_800715FC
.L_80071604:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80071618
.L_80071614:
    bdnz     .L_80071614
.L_80071618:
    lfs      f1, lbl_8065CF74@sda21(r0)
    li       r3, 0x0
    lfs      f0, 0x8cc(r26)
    mr       r0, r3
    fcmpu    cr0, f1, f0
    bne      .L_8007164C
    lfs      f0, 0x8d0(r26)
    fcmpu    cr0, f1, f0
    bne      .L_8007164C
    lfs      f0, 0x8d4(r26)
    fcmpu    cr0, f1, f0
    bne      .L_8007164C
    li       r0, 0x1
.L_8007164C:
    clrlwi.  r0, r0, 24
    beq      .L_80071690
    lfs      f1, lbl_8065CF74@sda21(r0)
    li       r0, 0x0
    lfs      f0, 0x8f0(r26)
    fcmpu    cr0, f1, f0
    bne      .L_80071684
    lfs      f0, 0x8f4(r26)
    fcmpu    cr0, f1, f0
    bne      .L_80071684
    lfs      f0, 0x8f8(r26)
    fcmpu    cr0, f1, f0
    bne      .L_80071684
    li       r0, 0x1
.L_80071684:
    clrlwi.  r0, r0, 24
    beq      .L_80071690
    li       r3, 0x1
.L_80071690:
    clrlwi   r0, r30, 24
    clrlwi   r3, r3, 24
    cmplwi   r0, 0x2e
    bne      .L_800716B4
    cmpwi    r3, 0x0
    bne      .L_80071E80
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_800716B4:
    stw      r3, 0x10(r31)
    b        .L_80071E80
.L_800716BC:
    mr       r5, r4
    addi     r3, r29, 0x330
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_8007171C
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80071708
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80071708
.L_80071700:
    addi     r3, r3, 0x8
    bdnz     .L_80071700
.L_80071708:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007171C
.L_80071718:
    bdnz     .L_80071718
.L_8007171C:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80071764
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80071750
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80071750
.L_80071748:
    addi     r3, r3, 0x8
    bdnz     .L_80071748
.L_80071750:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80071764
.L_80071760:
    bdnz     .L_80071760
.L_80071764:
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x18
    bne      .L_80071E80
    li       r0, 0x1
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_8007177C:
    mr       r3, r31
    bl       CEvt_Evaluate
    cmpwi    r3, 0x1
    bne      .L_800717C8
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    subi     r3, r3, 0x70c8
    bl       InfoScrn_IsBatteryGaugeFull
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x2e
    bne      .L_800717C0
    cmpwi    r3, 0x0
    bne      .L_80071E80
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_800717C0:
    stw      r3, 0x10(r31)
    b        .L_80071E80
.L_800717C8:
    mr       r4, r3
    addi     r3, r29, 0x358
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvtCommandWAITName@ha
    mr       r6, r3
    addi     r7, r4, sEvtCommandWAITName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x1032
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_80071E80
.L_800717FC:
    lis      r3, gGame@ha
    li       r29, 0x1
    addi     r3, r3, gGame@l
    li       r4, 0x9
    addis    r26, r3, 0xe
    addi     r26, r26, 0x5d38
    mr       r3, r26
    bl       fn_80149674
    cmpwi    r3, 0x0
    beq      .L_80071828
    li       r29, 0x0
.L_80071828:
    mr       r3, r26
    li       r4, 0x1
    bl       fn_80149674
    cmpwi    r3, 0x0
    beq      .L_80071884
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    lwz      r3, 0x7df0(r3)
    lwz      r0, 0x68(r3)
    cmplwi   r0, 0x0
    bne      .L_80071870
    lwz      r0, 0x27c(r3)
    cmplwi   r0, 0x0
    bne      .L_80071870
    lwz      r0, 0x6c(r3)
    cmplwi   r0, 0x0
    beq      .L_80071878
.L_80071870:
    li       r0, 0x1
    b        .L_8007187C
.L_80071878:
    li       r0, 0x0
.L_8007187C:
    cntlzw   r0, r0
    extrwi   r29, r0, 8, 19
.L_80071884:
    mr       r3, r26
    li       r4, 0x3
    bl       fn_80149674
    cmpwi    r3, 0x0
    beq      .L_800718F8
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    lwz      r3, 0x7df8(r3)
    lwz      r0, 0x44(r3)
    cmplwi   r0, 0x0
    bne      .L_800718E4
    lwz      r0, 0x48(r3)
    cmplwi   r0, 0x0
    bne      .L_800718E4
    lwz      r0, 0x4c(r3)
    cmplwi   r0, 0x0
    bne      .L_800718E4
    lwz      r0, 0x50(r3)
    cmplwi   r0, 0x0
    bne      .L_800718E4
    lwz      r0, 0x54(r3)
    cmplwi   r0, 0x0
    beq      .L_800718EC
.L_800718E4:
    li       r0, 0x1
    b        .L_800718F0
.L_800718EC:
    li       r0, 0x0
.L_800718F0:
    cntlzw   r0, r0
    extrwi   r29, r0, 8, 19
.L_800718F8:
    mr       r3, r26
    li       r4, 0x6
    bl       fn_80149674
    cmpwi    r3, 0x0
    beq      .L_80071958
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x696c(r3)
    cmplwi   r3, 0x0
    beq      .L_80071958
    lwz      r0, 0x228(r3)
    li       r5, 0x0
    lwz      r6, 0x22c(r3)
    li       r4, 0x1
    and      r0, r0, r5
    and      r4, r6, r4
    xor      r4, r4, r5
    xor      r0, r0, r5
    or.      r0, r4, r0
    beq      .L_80071958
    bl       HasCameraAnimation
    cntlzw   r0, r3
    extrwi   r29, r0, 8, 19
.L_80071958:
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x2e
    bne      .L_80071978
    cmpwi    r29, 0x0
    bne      .L_80071E80
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_80071978:
    stw      r29, 0x10(r31)
    b        .L_80071E80
.L_80071980:
    mr       r3, r31
    bl       CEvt_Evaluate
    extsh    r4, r3
    cmpwi    r4, 0x2710
    blt      .L_80071A44
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r6, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80071A2C
.L_800719AC:
    lha      r0, 0x4(r3)
    cmpw     r0, r4
    bne      .L_80071A20
    lis      r3, gEventObjectIds@ha
    lis      r5, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r6, 2
    add      r3, r3, r0
    addi     r5, r5, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r5, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r26, -0x67a0(r3)
    cmplwi   r26, 0x0
    beq      .L_80071A18
    lwz      r0, 0x228(r26)
    li       r5, 0x0
    lwz      r6, 0x22c(r26)
    li       r3, 0x1
    and      r0, r0, r5
    and      r3, r6, r3
    xor      r3, r3, r5
    xor      r0, r0, r5
    or.      r0, r3, r0
    beq      .L_80071A18
    b        .L_80071A30
.L_80071A18:
    li       r26, 0x0
    b        .L_80071A30
.L_80071A20:
    addi     r3, r3, 0x4
    addi     r6, r6, 0x1
    bdnz     .L_800719AC
.L_80071A2C:
    li       r26, 0x0
.L_80071A30:
    cmplwi   r26, 0x0
    bne      .L_80071A7C
    li       r26, 0x0
    b        .L_80071A7C
    b        .L_80071A7C
.L_80071A44:
    extsh.   r0, r4
    blt      .L_80071A68
    lis      r3, gGame@ha
    slwi     r0, r4, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r26, -0x67a0(r3)
    b        .L_80071A7C
.L_80071A68:
    cmpwi    r4, -0x1
    bne      .L_80071A78
    lwz      r26, 0x8(r31)
    b        .L_80071A7C
.L_80071A78:
    mr       r26, r4
.L_80071A7C:
    cmplwi   r26, 0x0
    beq      .L_80071BCC
    lwz      r0, 0x228(r26)
    li       r5, 0x0
    lwz      r6, 0x22c(r26)
    li       r3, 0x1
    and      r0, r0, r5
    and      r3, r6, r3
    xor      r3, r3, r5
    xor      r0, r0, r5
    or.      r0, r3, r0
    beq      .L_80071BCC
    lwz      r0, 0x0(r26)
    cmplwi   r0, 0x0
    bne      .L_80071B60
    mr       r5, r4
    addi     r3, r29, 0x378
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80071B18
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80071B04
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80071B04
.L_80071AFC:
    addi     r3, r3, 0x8
    bdnz     .L_80071AFC
.L_80071B04:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80071B18
.L_80071B14:
    bdnz     .L_80071B14
.L_80071B18:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80071B60
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80071B4C
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80071B4C
.L_80071B44:
    addi     r3, r3, 0x8
    bdnz     .L_80071B44
.L_80071B4C:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80071B60
.L_80071B5C:
    bdnz     .L_80071B5C
.L_80071B60:
    lwz      r5, 0x228(r26)
    lis      r0, 0x40
    lwz      r3, 0x22c(r26)
    li       r4, 0x0
    and      r0, r5, r0
    and      r3, r3, r4
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    bne      .L_80071BA0
    lis      r0, 0x80
    and      r0, r5, r0
    xor      r0, r0, r4
    or.      r0, r3, r0
    bne      .L_80071BA0
    li       r4, 0x1
.L_80071BA0:
    clrlwi   r0, r30, 24
    clrlwi   r3, r4, 24
    cmplwi   r0, 0x2e
    bne      .L_80071BC4
    cmpwi    r3, 0x0
    bne      .L_80071E80
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_80071BC4:
    stw      r3, 0x10(r31)
    b        .L_80071E80
.L_80071BCC:
    mr       r5, r4
    addi     r3, r29, 0x3a4
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80071C2C
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80071C18
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80071C18
.L_80071C10:
    addi     r3, r3, 0x8
    bdnz     .L_80071C10
.L_80071C18:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80071C2C
.L_80071C28:
    bdnz     .L_80071C28
.L_80071C2C:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80071C74
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80071C60
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80071C60
.L_80071C58:
    addi     r3, r3, 0x8
    bdnz     .L_80071C58
.L_80071C60:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80071C74
.L_80071C70:
    bdnz     .L_80071C70
.L_80071C74:
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x18
    bne      .L_80071E80
    li       r0, 0x1
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_80071C8C:
    lis      r3, gGame@ha
    lfs      f1, lbl_8065CF74@sda21(r0)
    addi     r3, r3, gGame@l
    lfs      f0, 0x4298(r3)
    fcmpu    cr0, f1, f0
    mfcr     r3
    clrlwi   r0, r30, 24
    extrwi   r3, r3, 1, 2
    xori     r3, r3, 0x1
    cmplwi   r0, 0x2e
    clrlwi   r0, r3, 24
    cntlzw   r0, r0
    extrwi   r0, r0, 8, 19
    bne      .L_80071CD8
    cmpwi    r0, 0x0
    bne      .L_80071E80
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_80071CD8:
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_80071CE0:
    lis      r3, gGame@ha
    lfs      f1, lbl_8065CF74@sda21(r0)
    addi     r3, r3, gGame@l
    li       r0, 0x0
    addis    r3, r3, 0x1
    lfs      f0, -0x6984(r3)
    fcmpu    cr0, f1, f0
    bne      .L_80071D1C
    lfs      f0, -0x6980(r3)
    fcmpu    cr0, f1, f0
    bne      .L_80071D1C
    lfs      f0, -0x697c(r3)
    fcmpu    cr0, f1, f0
    bne      .L_80071D1C
    li       r0, 0x1
.L_80071D1C:
    clrlwi   r4, r0, 24
    clrlwi   r0, r30, 24
    neg      r3, r4
    or       r3, r3, r4
    cmplwi   r0, 0x2e
    srwi     r0, r3, 31
    bne      .L_80071D4C
    cmpwi    r0, 0x0
    bne      .L_80071E80
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_80071D4C:
    stw      r0, 0x10(r31)
    b        .L_80071E80
.L_80071D54:
    lwz      r3, gEventBeatLatch@sda21(r0)
    cmpwi    r3, 0x0
    beq      .L_80071D74
    li       r3, gEventBeatLatch@sda21
    li       r0, 0x0
    lwz      r3, 0x4(r3)
    stw      r0, gEventBeatLatch@sda21(r0)
    addi     r3, r3, 0x3e8
.L_80071D74:
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x2e
    bne      .L_80071D9C
    cmpwi    r3, 0x0
    bne      .L_80071D94
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_80071D94:
    stw      r3, 0x10(r31)
    b        .L_80071E80
.L_80071D9C:
    stw      r3, 0x10(r31)
    b        .L_80071E80
.L_80071DA4:
    mr       r3, r31
    bl       CEvt_Evaluate
    lis      r4, gGame@ha
    addi     r4, r4, gGame@l
    addis    r4, r4, 0x10
    lwz      r0, -0xc5c(r4)
    cntlzw   r0, r0
    extrwi.  r5, r0, 8, 19
    bne      .L_80071DE8
    cmpwi    r3, 0x0
    blt      .L_80071DE8
    lwz      r0, -0xbb0(r4)
    srwi     r4, r3, 31
    srawi    r5, r0, 31
    subfc    r0, r3, r0
    adde     r0, r5, r4
    clrlwi   r5, r0, 24
.L_80071DE8:
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x2e
    bne      .L_80071E10
    cmpwi    r5, 0x0
    bne      .L_80071E08
    stw      r28, 0x4(r31)
    li       r27, 0x1
    b        .L_80071E80
.L_80071E08:
    stw      r5, 0x10(r31)
    b        .L_80071E80
.L_80071E10:
    stw      r5, 0x10(r31)
    b        .L_80071E80
.L_80071E18:
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x2e
    bne      .L_80071E54
    addi     r3, r29, 0x3cc
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvtCommandWAITName@ha
    mr       r6, r3
    addi     r7, r4, sEvtCommandWAITName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x10c1
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_80071E80
.L_80071E54:
    addi     r3, r29, 0x3e8
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvtCommandWAITName@ha
    mr       r6, r3
    addi     r7, r4, sEvtCommandWAITName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x10c3
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
.L_80071E80:
    mr       r3, r27
    psq_l    f31, 0x58(r1), 0, qr0
    lfd      f31, 0x50(r1)
    lmw      r25, 0x34(r1)
    lwz      r0, 0x64(r1)
    mtlr     r0
    addi     r1, r1, 0x60
    blr
*/

#if defined(VERSION_PAL)
// the refresh rate, 50 or 60
#define FRAMES_PER_SEC gVideoFramesPerSecond
#else
#define FRAMES_PER_SEC 60
#endif

/**
 * @note Address: 0x80071EA0
 * @note Size: 0x98
 */
s32 CEvt::_cmd_TIMER(u8, s32)
{
	// first arg is in seconds
	s32 frames = CEvt_Evaluate(this) * FRAMES_PER_SEC;
	s32 value  = CEvt_Evaluate(this);
	if (frames == 0) {
		frames = 1;
	}
	for (s32 i = 0; i < 10; i++) {
		if (gEventTimers.mFrames[i] == 0) {
			gEventTimers.mFrames[i] = frames;
			gEventTimers.mValue[i]  = value;
			break;
		}
	}
	return 0;
}

extern "C" s32 HSD_AudioPStreamStartChParam(const char* path, u8 vol, s32 channel);
extern "C" BOOL AXDriverSetAllStreamParams(s32 a, s32 b);

// LatchEventBeat is the stream beat callback, it latches the beat here for WAIT
extern "C" void LatchEventBeat(s32 id, s32 beat, EvtBeatLatch* latch);

/**
 * @note Address: 0x80071F38
 * @note Size: 0x2DC
 */
s32 CEvt::_cmd_SFX(u8, s32)
{
	s32 sfx  = CEvt_Evaluate(this);
	s16 type = CEvt_Evaluate(this);

	switch (type) {
	case 0:
		if (sfx == -2) {
			KeyOffGameSoundTrack(&gGame.mSound, -1);
		} else {
			StopGameSfx(&gGame.mSound, sfx);
		}
		break;
	case 1:
		PlayGameSound(&gGame.mSound, sfx, 0xFF);
		gEventBeatLatch.mPending = 0;
		AXDriverSetAllStreamParams((s32)LatchEventBeat, (s32)&gEventBeatLatch);
		break;
	case 0xF5:
		// here the value is a stream path, not an id
		HSD_AudioPStreamStartChParam((const char*)sfx, 0xFF, 0x1F);
		gEventBeatLatch.mPending = 0;
		AXDriverSetAllStreamParams((s32)LatchEventBeat, (s32)&gEventBeatLatch);
		break;
	case 2:
		FadeOutGameSfx(&gGame.mSound, sfx, CEvt_Evaluate(this) * 2);
		break;
	case 3:
		FadeInGameSfx(&gGame.mSound, sfx, CEvt_Evaluate(this) * 2);
		break;
	case 4: {
		XSound* sound;
		u16 frames;
		u8 vol;
		frames = CEvt_Evaluate(this) * 2;
		vol    = CEvt_Evaluate(this);
		sound  = &gGame.mSound;
		if (!IsGameSoundPlaying(sound, sfx)) {
			PlayGameSound(sound, sfx, vol);
		} else {
			SetGameSfxVolume(sound, sfx, vol, frames);
		}
		break;
	}
	case 5:
		if (sfx == -2) {
			PauseGameSoundTrack(&gGame.mSound, -1);
		} else if ((sfx & 0xFFFF0000) == 0xFFFF0000) {
			PauseGameMusic(&gGame.mSound, sfx, 0);
		} else {
			AXDriverPauseSfx(sfx);
		}
		break;
	case 6:
		if (sfx == -2) {
			ResumeGameSoundTrack(&gGame.mSound, -1);
		} else {
			ResumeGameSfx(&gGame.mSound, sfx);
		}
		break;
	default:
#if defined(VERSION_PAL)
		OSPanic(__FILE__, 3820, "%s%s", FormatToStaticBuffer("sfx(%d,%d) - unresolved!\n", sfx, type), __FUNCTION__);
#else
		OSPanic(__FILE__, 3809, "%s%s", FormatToStaticBuffer("sfx(%d,%d) - unresolved!\n", sfx, type), __FUNCTION__);
#endif
		break;
	}

	return 0;
}

/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stw      r31, 0x1c(r1)
    stw      r30, 0x18(r1)
    stw      r29, 0x14(r1)
    stw      r28, 0x10(r1)
    mr       r28, r3
    bl       CEvt_Evaluate
    mr       r0, r3
    mr       r3, r28
    mr       r31, r0
    bl       CEvt_Evaluate
    extsh    r0, r3
    cmpwi    r0, 0x4
    mr       r5, r0
    beq      .L_800720B4
    bge      .L_80071FA4
    cmpwi    r0, 0x1
    beq      .L_80072000
    bge      .L_80071F98
    cmpwi    r0, 0x0
    bge      .L_80071FC0
    b        .L_800721BC
.L_80071F98:
    cmpwi    r0, 0x3
    bge      .L_8007208C
    b        .L_80072064
.L_80071FA4:
    cmpwi    r0, 0xf5
    beq      .L_80072038
    bge      .L_800721BC
    cmpwi    r0, 0x6
    beq      .L_8007217C
    bge      .L_800721BC
    b        .L_8007211C
.L_80071FC0:
    cmpwi    r31, -0x2
    bne      .L_80071FE4
    lis      r3, gGame@ha
    li       r4, -0x1
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x7e24
    bl       KeyOffGameSoundTrack
    b        .L_800721F0
.L_80071FE4:
    lis      r3, gGame@ha
    mr       r4, r31
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x7e24
    bl       StopGameSfx
    b        .L_800721F0
.L_80072000:
    lis      r3, gGame@ha
    mr       r4, r31
    addi     r3, r3, gGame@l
    li       r5, 0xff
    addis    r3, r3, 0xe
    addi     r3, r3, 0x7e24
    bl       PlayGameSound
    li       r0, 0x0
    lis      r3, LatchEventBeat@ha
    stw      r0, gEventBeatLatch@sda21(r0)
    addi     r3, r3, LatchEventBeat@l
    li       r4, gEventBeatLatch@sda21
    bl       AXDriverSetAllStreamParams
    b        .L_800721F0
.L_80072038:
    mr       r3, r31
    li       r4, 0xff
    li       r5, 0x1f
    bl       HSD_AudioPStreamStartChParam
    li       r0, 0x0
    lis      r3, LatchEventBeat@ha
    stw      r0, gEventBeatLatch@sda21(r0)
    addi     r3, r3, LatchEventBeat@l
    li       r4, gEventBeatLatch@sda21
    bl       AXDriverSetAllStreamParams
    b        .L_800721F0
.L_80072064:
    mr       r3, r28
    bl       CEvt_Evaluate
    lis      r4, gGame@ha
    clrlslwi r5, r3, 17, 1
    addi     r3, r4, gGame@l
    addis    r3, r3, 0xe
    mr       r4, r31
    addi     r3, r3, 0x7e24
    bl       FadeOutGameSfx
    b        .L_800721F0
.L_8007208C:
    mr       r3, r28
    bl       CEvt_Evaluate
    lis      r4, gGame@ha
    clrlslwi r5, r3, 17, 1
    addi     r3, r4, gGame@l
    addis    r3, r3, 0xe
    mr       r4, r31
    addi     r3, r3, 0x7e24
    bl       FadeInGameSfx
    b        .L_800721F0
.L_800720B4:
    mr       r3, r28
    bl       CEvt_Evaluate
    clrlslwi r29, r3, 17, 1
    mr       r3, r28
    bl       CEvt_Evaluate
    lis      r5, gGame@ha
    mr       r4, r31
    addi     r5, r5, gGame@l
    clrlwi   r28, r3, 24
    addis    r30, r5, 0xe
    addi     r30, r30, 0x7e24
    mr       r3, r30
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    bne      .L_80072104
    mr       r3, r30
    mr       r4, r31
    mr       r5, r28
    bl       PlayGameSound
    b        .L_800721F0
.L_80072104:
    mr       r3, r30
    mr       r4, r31
    mr       r5, r28
    mr       r6, r29
    bl       SetGameSfxVolume
    b        .L_800721F0
.L_8007211C:
    cmpwi    r31, -0x2
    bne      .L_80072140
    lis      r3, gGame@ha
    li       r4, -0x1
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x7e24
    bl       PauseGameSoundTrack
    b        .L_800721F0
.L_80072140:
    clrrwi   r3, r31, 16
    addis    r0, r3, 0x1
    cmplwi   r0, 0x0
    bne      .L_80072170
    lis      r3, gGame@ha
    mr       r4, r31
    addi     r3, r3, gGame@l
    li       r5, 0x0
    addis    r3, r3, 0xe
    addi     r3, r3, 0x7e24
    bl       PauseGameMusic
    b        .L_800721F0
.L_80072170:
    mr       r3, r31
    bl       AXDriverPauseSfx
    b        .L_800721F0
.L_8007217C:
    cmpwi    r31, -0x2
    bne      .L_800721A0
    lis      r3, gGame@ha
    li       r4, -0x1
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x7e24
    bl       ResumeGameSoundTrack
    b        .L_800721F0
.L_800721A0:
    lis      r3, gGame@ha
    mr       r4, r31
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x7e24
    bl       ResumeGameSfx
    b        .L_800721F0
.L_800721BC:
    lis      r3, sEvtSfxUnresolved@ha
    mr       r4, r31
    addi     r3, r3, sEvtSfxUnresolved@l
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvtCommandSFXName@ha
    mr       r6, r3
    addi     r7, r4, sEvtCommandSFXName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0xee1
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
.L_800721F0:
    lwz      r0, 0x24(r1)
    li       r3, 0x0
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    lwz      r28, 0x10(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: 0x80072214
 * @note Size: 0x10
 */
extern "C" void LatchEventBeat(s32 id, s32 beat, EvtBeatLatch* latch)
{
	latch->mPending = 1;
	latch->mBeat    = beat;
}

/**
 * @note Address: 0x80073768
 * @note Size: 0x4D0
 */
// CEvt::_cmd_READ
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    lis      r4, lbl_80207FA0@ha
    stw      r0, 0x24(r1)
    stw      r31, 0x1c(r1)
    addi     r31, r4, lbl_80207FA0@l
    stw      r30, 0x18(r1)
    stw      r29, 0x14(r1)
    stw      r28, 0x10(r1)
    mr       r28, r3
    bl       CEvt_Evaluate
    clrlwi   r0, r3, 16
    cmpwi    r0, 0xdd
    mr       r4, r0
    beq      .L_80073B48
    bge      .L_80073BE8
    cmpwi    r0, 0xcc
    beq      .L_800737B4
    b        .L_80073BE8
.L_800737B4:
    mr       r3, r28
    bl       CEvt_Evaluate
    extsh    r29, r3
    cmpwi    r29, 0x2710
    blt      .L_80073878
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80073860
.L_800737E0:
    lha      r0, 0x4(r3)
    cmpw     r0, r29
    bne      .L_80073854
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r30, -0x67a0(r3)
    cmplwi   r30, 0x0
    beq      .L_8007384C
    lwz      r0, 0x228(r30)
    li       r4, 0x0
    lwz      r5, 0x22c(r30)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_8007384C
    b        .L_80073864
.L_8007384C:
    li       r30, 0x0
    b        .L_80073864
.L_80073854:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_800737E0
.L_80073860:
    li       r30, 0x0
.L_80073864:
    cmplwi   r30, 0x0
    bne      .L_800738B0
    li       r30, 0x0
    b        .L_800738B0
    b        .L_800738B0
.L_80073878:
    extsh.   r0, r29
    blt      .L_8007389C
    lis      r3, gGame@ha
    slwi     r0, r29, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r30, -0x67a0(r3)
    b        .L_800738B0
.L_8007389C:
    cmpwi    r29, -0x1
    bne      .L_800738AC
    lwz      r30, 0x8(r28)
    b        .L_800738B0
.L_800738AC:
    mr       r30, r29
.L_800738B0:
    mr       r3, r28
    bl       CEvt_Evaluate
    cmplwi   r30, 0x0
    mr       r28, r3
    beq      .L_80073A98
    lwz      r0, 0x228(r30)
    li       r4, 0x0
    lwz      r5, 0x22c(r30)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_80073A98
    lwz      r0, 0x0(r30)
    cmplwi   r0, 0x0
    bne      .L_800739A8
    mr       r4, r29
    mr       r5, r28
    mr       r6, r29
    addi     r3, r31, 0x4d4
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80073960
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007394C
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007394C
.L_80073944:
    addi     r3, r3, 0x8
    bdnz     .L_80073944
.L_8007394C:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80073960
.L_8007395C:
    bdnz     .L_8007395C
.L_80073960:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_800739A8
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80073994
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80073994
.L_8007398C:
    addi     r3, r3, 0x8
    bdnz     .L_8007398C
.L_80073994:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800739A8
.L_800739A4:
    bdnz     .L_800739A4
.L_800739A8:
    lwz      r3, 0x228(r30)
    lis      r0, 0x8
    lwz      r5, 0x22c(r30)
    li       r4, 0x0
    and      r0, r3, r0
    and      r3, r5, r4
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_80073A88
    lwz      r29, 0x0(r30)
    cmplwi   r29, 0x0
    beq      .L_80073A18
    mr       r3, r29
    bl       FindJointAnimationInTree
    cmplwi   r3, 0x0
    beq      .L_800739F0
    b        .L_80073A1C
.L_800739F0:
    mr       r3, r29
    bl       FindMaterialOrTextureAnimationInJointTree
    cmplwi   r3, 0x0
    beq      .L_80073A04
    b        .L_80073A1C
.L_80073A04:
    mr       r3, r29
    bl       FindPolygonAnimationInJointTree
    cmplwi   r3, 0x0
    beq      .L_80073A18
    b        .L_80073A1C
.L_80073A18:
    li       r3, 0x0
.L_80073A1C:
    cmplwi   r3, 0x0
    beq      .L_80073A88
    lwz      r0, 0x0(r3)
    rlwinm.  r0, r0, 0, 1, 1
    bne      .L_80073A88
    lis      r3, 0x1062
    lwz      r9, 0x158(r30)
    addi     r0, r3, 0x4dd3
    mr       r3, r30
    mulhw    r0, r0, r9
    lwz      r12, 0x34(r30)
    lfs      f1, lbl_8065CF74@sda21(r0)
    li       r5, 0x1
    lwz      r12, 0x80(r12)
    li       r6, 0x0
    srawi    r0, r0, 6
    fmr      f3, f1
    srwi     r4, r0, 31
    lfs      f2, lbl_8065CFE0@sda21(r0)
    add      r0, r0, r4
    lfs      f4, lbl_8065CFE4@sda21(r0)
    mulli    r0, r0, 0x3e8
    li       r7, 0x0
    li       r8, 0x0
    subf     r4, r0, r9
    mtctr    r12
    bctrl
.L_80073A88:
    mr       r3, r30
    mr       r4, r28
    bl       QueueObjectAnimationRead
    b        .L_80073C14
.L_80073A98:
    mr       r4, r29
    mr       r5, r29
    addi     r3, r31, 0x504
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80073AFC
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80073AE8
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80073AE8
.L_80073AE0:
    addi     r3, r3, 0x8
    bdnz     .L_80073AE0
.L_80073AE8:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80073AFC
.L_80073AF8:
    bdnz     .L_80073AF8
.L_80073AFC:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80073C14
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80073B30
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80073B30
.L_80073B28:
    addi     r3, r3, 0x8
    bdnz     .L_80073B28
.L_80073B30:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80073C14
.L_80073B40:
    bdnz     .L_80073B40
    b        .L_80073C14
.L_80073B48:
    mr       r3, r28
    bl       CEvt_Evaluate
    extsh    r4, r3
    cmpwi    r4, 0x2710
    blt      .L_80073BC4
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80073BC4
.L_80073B74:
    lha      r0, 0x4(r3)
    cmpw     r0, r4
    bne      .L_80073BB8
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r3, -0x67a0(r3)
    cmplwi   r3, 0x0
    beq      .L_80073BC4
    b        .L_80073BC4
.L_80073BB8:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_80073B74
.L_80073BC4:
    mr       r3, r28
    bl       CEvt_Evaluate
    lis      r4, gGame@ha
    addi     r5, r4, gGame@l
    mr       r4, r3
    addis    r3, r5, 0xe
    addi     r3, r3, 0x7e24
    bl       RegisterGameSoundName
    b        .L_80073C14
.L_80073BE8:
    addi     r3, r31, 0x52c
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvtCommandREADName@ha
    mr       r6, r3
    addi     r7, r4, sEvtCommandREADName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0xd28
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
.L_80073C14:
    lwz      r0, 0x24(r1)
    li       r3, 0x0
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    lwz      r28, 0x10(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: 0x80074614
 * @note Size: 0x38
 */
s32 CEvt::_cmd_PRINTF(u8, s32)
{
	// the print itself is compiled out
	for (s32 i = mValues.mCount - 1; i >= 0; i--) { }

	u8 c;
	u8* str;
	do {
		str           = mData.mScript;
		c             = *str;
		mData.mScript = str + 1;
	} while (c != 0);

	return 0;
}

// maps object ids from 10000 up to an index in the game's object table
#if defined(VERSION_JP)
#define PLAYER_HELD_OBJ 0xA54
#define PLAYER_BE8      0xBD8
#else
#define PLAYER_HELD_OBJ 0xA64
#define PLAYER_BE8      0xBE8
#endif

// POS calls these after moving an object
extern "C" void CancelPlayerPushAction(u8* p);

static inline XObj* CEvt_FindObj(s16 id)
{
	for (s32 i = 0; i < gEventObjectIds.mCount; i++) {
		if (gEventObjectIds.mEntry[i].mId == id) {
			XObj* obj = gGame.mXObjs.mObjects[gEventObjectIds.mEntry[i].mIndex];
			if (obj != NULL && (obj->mStatus.mFlags & 1)) {
				return obj;
			}
			return NULL;
		}
	}
	return NULL;
}

/**
 * @note Address: 0x8006FCAC
 * @note Size: 0xE0
 */
extern "C" void RegisterEventObjectId(CEvt*, s16 id, s16 index)
{
	if (CEvt_FindObj(id) == NULL) {
		gEventObjectIds.mEntry[gEventObjectIds.mCount].mId    = id;
		gEventObjectIds.mEntry[gEventObjectIds.mCount].mIndex = index;
		++gEventObjectIds.mCount;
	}
}

// -1 is the script's own object, anything lower is a pointer
static inline XObj* CEvt_GetObj(CEvt* evt, s16 id)
{
	if (id >= 10000) {
		XObj* obj = CEvt_FindObj(id);
		// redundant, but it's in the asm
		if (obj == NULL) {
			return NULL;
		}
		return obj;
	}
	if (id >= 0) {
		return gGame.mXObjs.mObjects[id];
	}
	if (id == -1) {
		return (XObj*)evt->mData._04;
	}
	return (XObj*)id;
}

static inline s16 CEvt_FindObjIndex(s16 id)
{
	for (s32 i = 0; i < gEventObjectIds.mCount; i++) {
		if (gEventObjectIds.mEntry[i].mId == id) {
			return gEventObjectIds.mEntry[i].mIndex;
		}
	}
	return -1;
}

static inline void CEvt_ForgetObjId(s16 id)
{
	for (s32 i = 0; i < gEventObjectIds.mCount; i++) {
		if (gEventObjectIds.mEntry[i].mId == id) {
			for (s32 j = i; j < gEventObjectIds.mCount; j++) {
				gEventObjectIds.mEntry[j].mId    = gEventObjectIds.mEntry[j + 1].mId;
				gEventObjectIds.mEntry[j].mIndex = gEventObjectIds.mEntry[j + 1].mIndex;
			}
			gEventObjectIds.mCount--;
		}
	}
}

// the print is compiled out, only the length loops are left
static inline void CEvt_Warn(const char* msg)
{
	size_t len = strlen(msg) + 13;
	for (size_t i = 0; i < len; i++) { }
	for (size_t i = 0; i < len; i++) { }
}

/**
 * @note Address: 0x80073C38
 * @note Size: 0x9DC
 */
s32 CEvt::_cmd_PTCL(u8, s32)
{
	u32 code = CEvt_Evaluate(this);
	s16 type = CEvt_Evaluate(this);
	Vec translate, scale;
	if (type == EVT_ATOM_POS) {
		translate.x  = CEvt_Evaluate(this) / 100.0f;
		translate.y  = CEvt_Evaluate(this) / 100.0f;
		translate.z  = CEvt_Evaluate(this) / 100.0f;
		scale.x      = CEvt_Evaluate(this) / 100.0f;
		scale.y      = CEvt_Evaluate(this) / 100.0f;
		scale.z      = CEvt_Evaluate(this) / 100.0f;
		f32 angle    = CEvt_Evaluate(this);
		Vec rotation = { 0.0f, 0.0f, 0.0f };
		rotation.y   = DEG2RAD * angle;
		(&gGame.mParticle)->CreateSRT(code, 1, translate, scale, rotation, TRUE);
	} else if (type == EVT_ATOM_OBJ) {
		s16 id      = CEvt_Evaluate(this);
		XObj* obj   = CEvt_GetObj(this, id);
		translate.x = CEvt_Evaluate(this) / 100.0f;
		translate.y = CEvt_Evaluate(this) / 100.0f;
		translate.z = CEvt_Evaluate(this) / 100.0f;
		scale.x     = CEvt_Evaluate(this) / 100.0f;
		scale.y     = CEvt_Evaluate(this) / 100.0f;
		scale.z     = CEvt_Evaluate(this) / 100.0f;
		f32 angle   = CEvt_Evaluate(this);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			translate    = obj->mMoveTranslate + translate;
			Vec rotation = { 0.0f, 0.0f, 0.0f };
			rotation.y   = DEG2RAD * angle;
			(&gGame.mParticle)->CreateSRT(code, 1, translate, scale, rotation, TRUE);
		} else {
			CEvt_Warn(FormatToStaticBuffer("ptcl(..., @OBJ, %d, ...) obj(%d) killed!", id, id));
		}
	} else if (type == EVT_ATOM_LEAD) {
		s16 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, id);
		s32 depth = CEvt_Evaluate(this);
		if (depth > 16)
			depth = 16;
		s32 path[16];
		for (s32 i = 0; i < depth; i++) {
			path[i] = CEvt_Evaluate(this);
		}
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL) {
				CEvt_Warn(FormatToStaticBuffer("ptcl(%d, @LEAD, %d, ...) ... obj(%d) is collision!\n", code, id, id));
			}
			(&gGame.mParticle)->Create(code, 0, GetJointByPath(obj->mJObj, path, depth), TRUE);
		} else {
			CEvt_Warn(FormatToStaticBuffer("ptcl(..., @LEAD, %d, ...) obj(%d) killed!", id, id));
		}
		for (s32 i = 0; i < depth; i++) { }
	} else if (type == EVT_ATOM_KILL) {
		(&gGame.mParticle)->Kill((s32)code);
	} else {
#if defined(VERSION_PAL)
		const s32 line = 3315;
#else
		const s32 line = 3304;
#endif
		OSPanic("evt.cpp", line, "%s%s", FormatToStaticBuffer("ptcl(...) - invalid type - %d!\n", type), "_cmd_PTCL");
	}
	return 0;
}
/*
    stwu     r1, -0x110(r1)
    mflr     r0
    stw      r0, 0x114(r1)
    stfd     f31, 0x100(r1)
    psq_st   f31, 0x108(r1), 0, qr0
    stmw     r23, 0xdc(r1)
    lis      r4, lbl_80207FA0@ha
    mr       r25, r3
    addi     r31, r4, lbl_80207FA0@l
    bl       CEvt_Evaluate
    mr       r0, r3
    mr       r3, r25
    mr       r28, r0
    bl       CEvt_Evaluate
    extsh    r4, r3
    cmpwi    r4, 0xcf
    bne      .L_80073E14
    mr       r3, r25
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x9c(r1)
    mr       r3, r25
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x98(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x98(r1)
    fsubs    f1, f1, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x48(r1)
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xa4(r1)
    mr       r3, r25
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xa0(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xa0(r1)
    fsubs    f1, f1, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x4c(r1)
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xac(r1)
    mr       r3, r25
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xa8(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xa8(r1)
    fsubs    f1, f1, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x50(r1)
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xb4(r1)
    mr       r3, r25
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xb0(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xb0(r1)
    fsubs    f1, f1, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x3c(r1)
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xbc(r1)
    mr       r3, r25
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xb8(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xb8(r1)
    fsubs    f1, f1, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x40(r1)
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc4(r1)
    mr       r3, r25
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xc0(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xc0(r1)
    fsubs    f1, f1, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x44(r1)
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xcc(r1)
    lis      r3, gGame@ha
    lwz      r5, 0xc0(r31)
    addi     r3, r3, gGame@l
    stw      r0, 0xc8(r1)
    addis    r3, r3, 0x1
    lfd      f2, lbl_8065CF88@sda21(r0)
    mr       r4, r28
    lfd      f1, 0xc8(r1)
    addi     r6, r1, 0x48
    lfs      f0, lbl_8065CFE8@sda21(r0)
    addi     r7, r1, 0x3c
    fsubs    f1, f1, f2
    lwz      r9, 0xbc(r31)
    lwz      r0, 0xc4(r31)
    addi     r8, r1, 0x30
    stw      r5, 0x34(r1)
    li       r5, 0x1
    fmuls    f0, f0, f1
    stw      r9, 0x30(r1)
    li       r9, 0x1
    subi     r3, r3, 0x6eec
    stw      r0, 0x38(r1)
    stfs     f0, 0x34(r1)
    bl       CreateSRT__9CParticleFUllRC3VecRC3Vec3Veci
    b        .L_800745F4
.L_80073E14:
    cmpwi    r4, 0xd0
    bne      .L_800741DC
    mr       r3, r25
    bl       CEvt_Evaluate
    extsh    r29, r3
    cmpwi    r29, 0x2710
    blt      .L_80073EE0
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80073EC8
.L_80073E48:
    lha      r0, 0x4(r3)
    cmpw     r0, r29
    bne      .L_80073EBC
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r30, -0x67a0(r3)
    cmplwi   r30, 0x0
    beq      .L_80073EB4
    lwz      r0, 0x228(r30)
    li       r4, 0x0
    lwz      r5, 0x22c(r30)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_80073EB4
    b        .L_80073ECC
.L_80073EB4:
    li       r30, 0x0
    b        .L_80073ECC
.L_80073EBC:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_80073E48
.L_80073EC8:
    li       r30, 0x0
.L_80073ECC:
    cmplwi   r30, 0x0
    bne      .L_80073F18
    li       r30, 0x0
    b        .L_80073F18
    b        .L_80073F18
.L_80073EE0:
    extsh.   r0, r29
    blt      .L_80073F04
    lis      r3, gGame@ha
    slwi     r0, r29, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r30, -0x67a0(r3)
    b        .L_80073F18
.L_80073F04:
    cmpwi    r29, -0x1
    bne      .L_80073F14
    lwz      r30, 0x8(r25)
    b        .L_80073F18
.L_80073F14:
    mr       r30, r29
.L_80073F18:
    mr       r3, r25
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xcc(r1)
    mr       r3, r25
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xc8(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xc8(r1)
    fsubs    f1, f1, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x48(r1)
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc4(r1)
    mr       r3, r25
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xc0(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xc0(r1)
    fsubs    f1, f1, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x4c(r1)
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xbc(r1)
    mr       r3, r25
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xb8(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xb8(r1)
    fsubs    f1, f1, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x50(r1)
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xb4(r1)
    mr       r3, r25
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xb0(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xb0(r1)
    fsubs    f1, f1, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x3c(r1)
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xac(r1)
    mr       r3, r25
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xa8(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xa8(r1)
    fsubs    f1, f1, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x40(r1)
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xa4(r1)
    mr       r3, r25
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xa0(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xa0(r1)
    fsubs    f1, f1, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x44(r1)
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x9c(r1)
    cmplwi   r30, 0x0
    lfd      f1, lbl_8065CF88@sda21(r0)
    stw      r0, 0x98(r1)
    lfd      f0, 0x98(r1)
    fsubs    f31, f0, f1
    beq      .L_8007412C
    lwz      r0, 0x228(r30)
    li       r4, 0x0
    lwz      r5, 0x22c(r30)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_8007412C
    lwz      r7, 0x48(r1)
    addi     r3, r1, 0x14
    lwz      r6, 0x4c(r1)
    addi     r4, r30, 0xc8
    lwz      r0, 0x50(r1)
    addi     r5, r1, 0x24
    stw      r7, 0x24(r1)
    stw      r6, 0x28(r1)
    stw      r0, 0x2c(r1)
    bl       __pl__4CVecCF3Vec
    lfs      f0, lbl_8065CFE8@sda21(r0)
    lis      r4, __vt__4CVec@ha
    lwz      r0, 0xcc(r31)
    addi     r11, r4, __vt__4CVec@l
    fmuls    f0, f0, f31
    lis      r3, gGame@ha
    stw      r0, 0xc(r1)
    addi     r3, r3, gGame@l
    lfs      f3, 0x14(r1)
    addis    r3, r3, 0x1
    lfs      f2, 0x18(r1)
    mr       r4, r28
    lfs      f1, 0x1c(r1)
    addi     r6, r1, 0x48
    lwz      r10, 0xc8(r31)
    addi     r7, r1, 0x3c
    lwz      r0, 0xd0(r31)
    addi     r8, r1, 0x8
    stfs     f3, 0x48(r1)
    li       r5, 0x1
    li       r9, 0x1
    subi     r3, r3, 0x6eec
    stfs     f2, 0x4c(r1)
    stfs     f1, 0x50(r1)
    stw      r11, 0x20(r1)
    stw      r10, 0x8(r1)
    stw      r0, 0x10(r1)
    stfs     f0, 0xc(r1)
    bl       CreateSRT__9CParticleFUllRC3VecRC3Vec3Veci
    b        .L_800745F4
.L_8007412C:
    mr       r4, r29
    mr       r5, r29
    addi     r3, r31, 0x54c
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80074190
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007417C
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007417C
.L_80074174:
    addi     r3, r3, 0x8
    bdnz     .L_80074174
.L_8007417C:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80074190
.L_8007418C:
    bdnz     .L_8007418C
.L_80074190:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_800745F4
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800741C4
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800741C4
.L_800741BC:
    addi     r3, r3, 0x8
    bdnz     .L_800741BC
.L_800741C4:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800745F4
.L_800741D4:
    bdnz     .L_800741D4
    b        .L_800745F4
.L_800741DC:
    cmpwi    r4, 0xdc
    bne      .L_800745A4
    mr       r3, r25
    bl       CEvt_Evaluate
    extsh    r27, r3
    cmpwi    r27, 0x2710
    blt      .L_800742A8
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80074290
.L_80074210:
    lha      r0, 0x4(r3)
    cmpw     r0, r27
    bne      .L_80074284
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r30, -0x67a0(r3)
    cmplwi   r30, 0x0
    beq      .L_8007427C
    lwz      r0, 0x228(r30)
    li       r4, 0x0
    lwz      r5, 0x22c(r30)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_8007427C
    b        .L_80074294
.L_8007427C:
    li       r30, 0x0
    b        .L_80074294
.L_80074284:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_80074210
.L_80074290:
    li       r30, 0x0
.L_80074294:
    cmplwi   r30, 0x0
    bne      .L_800742E0
    li       r30, 0x0
    b        .L_800742E0
    b        .L_800742E0
.L_800742A8:
    extsh.   r0, r27
    blt      .L_800742CC
    lis      r3, gGame@ha
    slwi     r0, r27, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r30, -0x67a0(r3)
    b        .L_800742E0
.L_800742CC:
    cmpwi    r27, -0x1
    bne      .L_800742DC
    lwz      r30, 0x8(r25)
    b        .L_800742E0
.L_800742DC:
    mr       r30, r27
.L_800742E0:
    mr       r3, r25
    bl       CEvt_Evaluate
    mr       r26, r3
    cmpwi    r26, 0x10
    ble      .L_800742F8
    li       r26, 0x10
.L_800742F8:
    addi     r29, r1, 0x54
    li       r23, 0x0
    mr       r24, r29
    b        .L_8007431C
.L_80074308:
    mr       r3, r25
    bl       CEvt_Evaluate
    stw      r3, 0x0(r24)
    addi     r24, r24, 0x4
    addi     r23, r23, 0x1
.L_8007431C:
    cmpw     r23, r26
    blt      .L_80074308
    cmplwi   r30, 0x0
    beq      .L_800744AC
    lwz      r0, 0x228(r30)
    li       r4, 0x0
    lwz      r5, 0x22c(r30)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_800744AC
    lwz      r0, 0x0(r30)
    cmplwi   r0, 0x0
    bne      .L_80074410
    mr       r4, r28
    mr       r5, r27
    mr       r6, r27
    addi     r3, r31, 0x578
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_800743C8
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800743B4
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800743B4
.L_800743AC:
    addi     r3, r3, 0x8
    bdnz     .L_800743AC
.L_800743B4:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800743C8
.L_800743C4:
    bdnz     .L_800743C4
.L_800743C8:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80074410
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800743FC
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800743FC
.L_800743F4:
    addi     r3, r3, 0x8
    bdnz     .L_800743F4
.L_800743FC:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80074410
.L_8007440C:
    bdnz     .L_8007440C
.L_80074410:
    lwz      r6, 0x0(r30)
    cmplwi   r6, 0x0
    bne      .L_80074424
    li       r6, 0x0
    b        .L_80074488
.L_80074424:
    li       r3, 0x0
    b        .L_80074480
.L_8007442C:
    cmplwi   r6, 0x0
    bne      .L_8007443C
    li       r6, 0x0
    b        .L_80074440
.L_8007443C:
    lwz      r6, 0x10(r6)
.L_80074440:
    lwz      r0, 0x0(r29)
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80074478
.L_80074450:
    cmplwi   r6, 0x0
    bne      .L_80074460
    li       r6, 0x0
    b        .L_80074464
.L_80074460:
    lwz      r6, 0x8(r6)
.L_80074464:
    cmplwi   r6, 0x0
    bne      .L_80074474
    li       r6, 0x0
    b        .L_80074488
.L_80074474:
    bdnz     .L_80074450
.L_80074478:
    addi     r29, r29, 0x4
    addi     r3, r3, 0x1
.L_80074480:
    cmpw     r3, r26
    blt      .L_8007442C
.L_80074488:
    lis      r3, gGame@ha
    mr       r4, r28
    addi     r3, r3, gGame@l
    li       r5, 0x0
    addis    r3, r3, 0x1
    li       r7, 0x1
    subi     r3, r3, 0x6eec
    bl       Create__9CParticleFUlUcP9_HSD_JObji
    b        .L_80074558
.L_800744AC:
    mr       r4, r27
    mr       r5, r27
    addi     r3, r31, 0x5ac
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80074510
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800744FC
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800744FC
.L_800744F4:
    addi     r3, r3, 0x8
    bdnz     .L_800744F4
.L_800744FC:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80074510
.L_8007450C:
    bdnz     .L_8007450C
.L_80074510:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80074558
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80074544
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80074544
.L_8007453C:
    addi     r3, r3, 0x8
    bdnz     .L_8007453C
.L_80074544:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80074558
.L_80074554:
    bdnz     .L_80074554
.L_80074558:
    cmpwi    r26, 0x0
    li       r4, 0x0
    ble      .L_800745F4
    cmpwi    r26, 0x8
    subi     r3, r26, 0x8
    ble      .L_8007458C
    addi     r0, r3, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmpwi    r3, 0x0
    ble      .L_8007458C
.L_80074584:
    addi     r4, r4, 0x8
    bdnz     .L_80074584
.L_8007458C:
    subf     r0, r4, r26
    mtctr    r0
    cmpw     r4, r26
    bge      .L_800745F4
.L_8007459C:
    bdnz     .L_8007459C
    b        .L_800745F4
.L_800745A4:
    cmpwi    r4, 0xd2
    bne      .L_800745C8
    lis      r3, gGame@ha
    mr       r4, r28
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    subi     r3, r3, 0x6eec
    bl       Kill__9CParticleFl
    b        .L_800745F4
.L_800745C8:
    addi     r3, r31, 0x5d8
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvtCommandPTCLName@ha
    mr       r6, r3
    addi     r7, r4, sEvtCommandPTCLName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0xce8
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
.L_800745F4:
    li       r3, 0x0
    psq_l    f31, 0x108(r1), 0, qr0
    lfd      f31, 0x100(r1)
    lmw      r23, 0xdc(r1)
    lwz      r0, 0x114(r1)
    mtlr     r0
    addi     r1, r1, 0x110
    blr
*/

/**
 * @note Address: 0x8007311C
 * @note Size: 0x64C
 */
s32 CEvt::_cmd_SCALE(u8 cmd, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	f32 x     = CEvt_Evaluate(this) / 100.0f;
	f32 y     = CEvt_Evaluate(this) / 100.0f;
	f32 z     = CEvt_Evaluate(this) / 100.0f;
	if (cmd == EVT_CMD_SCALE) {
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			obj->SetScale(x, y, z, 0);
			UpdateObjectFloorHeights(obj);
			UpdateObjectFloorXZExtent(obj);
		} else {
			CEvt_Warn(FormatToStaticBuffer("scale(%d, ...) obj(%d) killed!", id, id));
		}
	} else {
		f32 time = CEvt_Evaluate(this) / 100.0f;
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL) {
				CEvt_Warn(FormatToStaticBuffer("mscale(%d, %d,%d,%d) ... obj(%d) is collision!", id, (s32)(100.0f * x), (s32)(100.0f * y),
				                               (s32)(100.0f * z), id));
			}
			if (time == 0.0f) {
				obj->SetScale(x, y, z, 0);
				UpdateObjectFloorHeights(obj);
				UpdateObjectFloorXZExtent(obj);
			} else {
#if defined(VERSION_PAL)
				CVec step = (CVec(x, y, z) - obj->mMoveScale) / (gVideoFramesPerSecond * time);
#else
				CVec step = (CVec(x, y, z) - obj->mMoveScale) / (60.0f * time);
#endif
				obj->mMoveScale.mTarget.Set(x, y, z);
				step.x = Abs(step.x);
				step.y = Abs(step.y);
				step.z = Abs(step.z);
				obj->mMoveScale.mSpeed.Set(step);
			}
		} else {
			CEvt_Warn(FormatToStaticBuffer("mscale(%d, ...) obj(%d) killed!", id, id));
		}
	}
	return 0;
}
/*
    stwu     r1, -0xd0(r1)
    mflr     r0
    stw      r0, 0xd4(r1)
    stfd     f31, 0xc0(r1)
    psq_st   f31, 0xc8(r1), 0, qr0
    stfd     f30, 0xb0(r1)
    psq_st   f30, 0xb8(r1), 0, qr0
    stfd     f29, 0xa0(r1)
    psq_st   f29, 0xa8(r1), 0, qr0
    stfd     f28, 0x90(r1)
    psq_st   f28, 0x98(r1), 0, qr0
    stmw     r27, 0x7c(r1)
    lis      r5, lbl_80207FA0@ha
    mr       r27, r3
    mr       r28, r4
    addi     r31, r5, lbl_80207FA0@l
    bl       CEvt_Evaluate
    extsh    r29, r3
    cmpwi    r29, 0x2710
    blt      .L_8007321C
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80073204
.L_80073184:
    lha      r0, 0x4(r3)
    cmpw     r0, r29
    bne      .L_800731F8
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r30, -0x67a0(r3)
    cmplwi   r30, 0x0
    beq      .L_800731F0
    lwz      r0, 0x228(r30)
    li       r4, 0x0
    lwz      r5, 0x22c(r30)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_800731F0
    b        .L_80073208
.L_800731F0:
    li       r30, 0x0
    b        .L_80073208
.L_800731F8:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_80073184
.L_80073204:
    li       r30, 0x0
.L_80073208:
    cmplwi   r30, 0x0
    bne      .L_80073254
    li       r30, 0x0
    b        .L_80073254
    b        .L_80073254
.L_8007321C:
    extsh.   r0, r29
    blt      .L_80073240
    lis      r3, gGame@ha
    slwi     r0, r29, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r30, -0x67a0(r3)
    b        .L_80073254
.L_80073240:
    cmpwi    r29, -0x1
    bne      .L_80073250
    lwz      r30, 0x8(r27)
    b        .L_80073254
.L_80073250:
    mr       r30, r29
.L_80073254:
    mr       r3, r27
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x5c(r1)
    mr       r3, r27
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x58(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x58(r1)
    fsubs    f1, f1, f2
    fdivs    f31, f1, f0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x64(r1)
    mr       r3, r27
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x60(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x60(r1)
    fsubs    f1, f1, f2
    fdivs    f30, f1, f0
    bl       CEvt_Evaluate
    xoris    r0, r3, 0x8000
    lis      r3, 0x4330
    stw      r0, 0x6c(r1)
    clrlwi   r0, r28, 24
    lfd      f2, lbl_8065CF88@sda21(r0)
    cmplwi   r0, 0x28
    stw      r3, 0x68(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x68(r1)
    fsubs    f1, f1, f2
    fdivs    f29, f1, f0
    bne      .L_800733FC
    cmplwi   r30, 0x0
    beq      .L_8007334C
    lwz      r0, 0x228(r30)
    li       r4, 0x0
    lwz      r5, 0x22c(r30)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_8007334C
    mr       r3, r30
    fmr      f1, f31
    lwz      r12, 0x34(r30)
    fmr      f2, f30
    fmr      f3, f29
    li       r4, 0x0
    lwz      r12, 0x3c(r12)
    mtctr    r12
    bctrl
    mr       r3, r30
    bl       UpdateObjectFloorHeights
    mr       r3, r30
    bl       UpdateObjectFloorXZExtent
    b        .L_80073730
.L_8007334C:
    mr       r4, r29
    mr       r5, r29
    addi     r3, r31, 0x464
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_800733B0
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007339C
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007339C
.L_80073394:
    addi     r3, r3, 0x8
    bdnz     .L_80073394
.L_8007339C:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800733B0
.L_800733AC:
    bdnz     .L_800733AC
.L_800733B0:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80073730
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800733E4
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800733E4
.L_800733DC:
    addi     r3, r3, 0x8
    bdnz     .L_800733DC
.L_800733E4:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80073730
.L_800733F4:
    bdnz     .L_800733F4
    b        .L_80073730
.L_800733FC:
    mr       r3, r27
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x6c(r1)
    cmplwi   r30, 0x0
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x68(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x68(r1)
    fsubs    f1, f1, f2
    fdivs    f28, f1, f0
    beq      .L_80073684
    lwz      r0, 0x228(r30)
    li       r4, 0x0
    lwz      r5, 0x22c(r30)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_80073684
    lwz      r0, 0x0(r30)
    cmplwi   r0, 0x0
    bne      .L_80073540
    fmuls    f2, f0, f31
    mr       r4, r29
    fmuls    f1, f0, f30
    mr       r8, r29
    fmuls    f0, f0, f29
    addi     r3, r31, 0x484
    fctiwz   f2, f2
    fctiwz   f1, f1
    fctiwz   f0, f0
    stfd     f2, 0x68(r1)
    stfd     f1, 0x60(r1)
    lwz      r5, 0x6c(r1)
    stfd     f0, 0x58(r1)
    lwz      r6, 0x64(r1)
    lwz      r7, 0x5c(r1)
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_800734F8
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800734E4
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800734E4
.L_800734DC:
    addi     r3, r3, 0x8
    bdnz     .L_800734DC
.L_800734E4:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800734F8
.L_800734F4:
    bdnz     .L_800734F4
.L_800734F8:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80073540
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007352C
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007352C
.L_80073524:
    addi     r3, r3, 0x8
    bdnz     .L_80073524
.L_8007352C:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80073540
.L_8007353C:
    bdnz     .L_8007353C
.L_80073540:
    lfs      f0, lbl_8065CF74@sda21(r0)
    fcmpu    cr0, f0, f28
    bne      .L_80073584
    mr       r3, r30
    fmr      f1, f31
    lwz      r12, 0x34(r30)
    fmr      f2, f30
    fmr      f3, f29
    li       r4, 0x0
    lwz      r12, 0x3c(r12)
    mtctr    r12
    bctrl
    mr       r3, r30
    bl       UpdateObjectFloorHeights
    mr       r3, r30
    bl       UpdateObjectFloorXZExtent
    b        .L_80073730
.L_80073584:
    lis      r3, __vt__4CVec@ha
    stfs     f31, 0x34(r1)
    addi     r0, r3, __vt__4CVec@l
    addi     r3, r1, 0x18
    stw      r0, 0x40(r1)
    addi     r4, r1, 0x34
    addi     r5, r1, 0x28
    stfs     f30, 0x38(r1)
    stfs     f29, 0x3c(r1)
    lwz      r6, 0x128(r30)
    lwz      r0, 0x12c(r30)
    stw      r6, 0x28(r1)
    stw      r0, 0x2c(r1)
    lwz      r0, 0x130(r30)
    stw      r0, 0x30(r1)
    bl       __mi__4CVecCF3Vec
    lfs      f0, lbl_8065CFC0@sda21(r0)
    addi     r3, r1, 0x8
    addi     r4, r1, 0x18
    fmuls    f1, f0, f28
    bl       fn_8002442C
    lis      r3, __vt__4CVec@ha
    lwz      r5, 0x8(r1)
    addi     r0, r3, __vt__4CVec@l
    lwz      r4, 0xc(r1)
    lwz      r3, 0x10(r1)
    stw      r5, 0x44(r1)
    lfs      f0, lbl_8065CF74@sda21(r0)
    lfs      f1, 0x44(r1)
    stw      r0, 0x14(r1)
    fcmpo    cr0, f1, f0
    stw      r0, 0x24(r1)
    stw      r0, 0x40(r1)
    stfs     f31, 0x138(r30)
    stfs     f30, 0x13c(r30)
    stw      r4, 0x48(r1)
    stw      r3, 0x4c(r1)
    stw      r0, 0x50(r1)
    stfs     f29, 0x140(r30)
    bge      .L_80073628
    fneg     f1, f1
.L_80073628:
    lfs      f2, 0x48(r1)
    lfs      f0, lbl_8065CF74@sda21(r0)
    stfs     f1, 0x44(r1)
    fcmpo    cr0, f2, f0
    bge      .L_80073640
    fneg     f2, f2
.L_80073640:
    lfs      f3, 0x4c(r1)
    lfs      f0, lbl_8065CF74@sda21(r0)
    stfs     f2, 0x48(r1)
    fcmpo    cr0, f3, f0
    bge      .L_80073658
    fneg     f3, f3
.L_80073658:
    lfs      f0, 0x44(r1)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    lfs      f1, 0x48(r1)
    stfs     f0, 0x148(r30)
    frsp     f0, f3
    stfs     f1, 0x14c(r30)
    stfs     f3, 0x4c(r1)
    stfs     f0, 0x150(r30)
    stw      r0, 0x50(r1)
    b        .L_80073730
.L_80073684:
    mr       r4, r29
    mr       r5, r29
    addi     r3, r31, 0x4b4
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_800736E8
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800736D4
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800736D4
.L_800736CC:
    addi     r3, r3, 0x8
    bdnz     .L_800736CC
.L_800736D4:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800736E8
.L_800736E4:
    bdnz     .L_800736E4
.L_800736E8:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80073730
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007371C
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007371C
.L_80073714:
    addi     r3, r3, 0x8
    bdnz     .L_80073714
.L_8007371C:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80073730
.L_8007372C:
    bdnz     .L_8007372C
.L_80073730:
    li       r3, 0x0
    psq_l    f31, 0xc8(r1), 0, qr0
    lfd      f31, 0xc0(r1)
    psq_l    f30, 0xb8(r1), 0, qr0
    lfd      f30, 0xb0(r1)
    psq_l    f29, 0xa8(r1), 0, qr0
    lfd      f29, 0xa0(r1)
    psq_l    f28, 0x98(r1), 0, qr0
    lfd      f28, 0x90(r1)
    lmw      r27, 0x7c(r1)
    lwz      r0, 0xd4(r1)
    mtlr     r0
    addi     r1, r1, 0xd0
    blr
*/

static inline XObj* CEvt_FindObjUnchecked(s16 id)
{
	for (s32 i = 0; i < gEventObjectIds.mCount; i++) {
		if (gEventObjectIds.mEntry[i].mId == id) {
			XObj* obj = gGame.mXObjs.mObjects[gEventObjectIds.mEntry[i].mIndex];
			if (obj != NULL) {
				return obj;
			}
			return NULL;
		}
	}
	return NULL;
}

/**
 * @note Address: 0x80073768
 * @note Size: 0x4D0
 */
s32 CEvt::_cmd_READ(u8, s32)
{
	u16 type = CEvt_Evaluate(this);
	switch (type) {
	case EVT_ATOM_ANIM: {
		s16 id     = CEvt_Evaluate(this);
		XObj* obj  = CEvt_GetObj(this, id);
		char* path = (char*)CEvt_Evaluate(this);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL) {
				CEvt_Warn(FormatToStaticBuffer("read(@ANIM, %d, %s) ... obj(%d) is collision!", id, path, id));
			}
			if (obj->mStatus.mFlags & 0x0008000000000000) {
				HSD_AObj* anim = GetAObj(obj->mJObj);
				if (anim != NULL && !(anim->flags & AOBJ_NO_ANIM)) {
					obj->SetAnim(obj->mAnimId % 1000, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				}
			}
			QueueObjectAnimationRead(obj, path);
		} else {
			CEvt_Warn(FormatToStaticBuffer("read(@ANIM, %d, ...) obj(%d) killed!", id, id));
		}
		break;
	}
	case EVT_ATOM_SFX: {
		s16 id = CEvt_Evaluate(this);
		if (id >= 10000) {
			CEvt_FindObjUnchecked(id);
		}
		const char* name = (const char*)CEvt_Evaluate(this);
		RegisterGameSoundName(&gGame.mSound, name);
		break;
	}
	default:
#if defined(VERSION_PAL)
		const s32 line = 3379;
#else
		const s32 line = 3368;
#endif
		OSPanic("evt.cpp", line, "%s%s", FormatToStaticBuffer("read(...) - invalid type - %d!\n", type), "_cmd_READ");
		break;
	}
	return 0;
}

// where WIN puts its message, a screen spot or a point on an object
struct EvtMsgPosition {
	f32 x;        // _00
	f32 y;        // _04
	XObj* object; // _08
	Vec offset;   // _0C
};

static inline EvtMsgPosition* CEvt_MsgPosition(u8* owner)
{
#if defined(VERSION_JP)
	return (EvtMsgPosition*)(owner + 0x43FB0);
#else
	return (EvtMsgPosition*)(owner + 0x44600);
#endif
}

extern "C" void CMsg_ResetPos(u8* owner);

/**
 * @note Address: 0x8006FD8C
 * @note Size: 0x48C
 */
s32 CEvt::_cmd_WIN(u8, s32)
{
	switch ((u16)CEvt_Evaluate(this)) {
	case EVT_ATOM_POS: {
		f32 x = CEvt_Evaluate(this);
		f32 y = CEvt_Evaluate(this);
		if (_54 != NULL) {
			EvtMsgPosition* pos = CEvt_MsgPosition(_54);
			pos->x              = x;
			pos->y              = y;
			pos->object         = NULL;
			pos->offset.x       = 0.0f;
			pos->offset.y       = 0.0f;
			pos->offset.z       = 0.0f;
		}
		break;
	}
	case EVT_ATOM_LETTERBOX:
		if (_54 != NULL) {
			EvtMsgPosition* pos = CEvt_MsgPosition(_54);
			pos->x              = 320.0f;
			pos->y              = 378.5f;
			pos->object         = NULL;
			pos->offset.x       = 0.0f;
			pos->offset.y       = 0.0f;
			pos->offset.z       = 0.0f;
		}
		break;
	case EVT_ATOM_OBJ: {
		s16 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, id);
		f32 x     = CEvt_Evaluate(this) / 100.0f;
		f32 y     = CEvt_Evaluate(this) / 100.0f;
		f32 z     = CEvt_Evaluate(this) / 100.0f;
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (_54 != NULL) {
				EvtMsgPosition* pos = CEvt_MsgPosition(_54);
				pos->object         = obj;
				pos->offset.x       = x;
				pos->offset.y       = y;
				pos->offset.z       = z;
			}
		} else {
			CEvt_Warn(FormatToStaticBuffer("win(@OBJ, %d, ...) obj(%d) killed!", id, id));
		}
		break;
	}
	case EVT_ATOM_COLOR: {
		u8 r = CEvt_Evaluate(this);
		u8 g = CEvt_Evaluate(this);
		u8 b = CEvt_Evaluate(this);
		u8 a = CEvt_Evaluate(this);
		if (_54 != NULL) {
			GXColor color            = sEvtWindowColorTemplate;
			color.r                  = r;
			color.g                  = g;
			color.b                  = b;
			color.a                  = a;
			*(GXColor*)(_54 + 0x9B8) = color;
		}
		break;
	}
	case EVT_ATOM_RESET:
		if (_54 != NULL) {
			CMsg_ResetPos(_54);
			GXColor color            = sEvtWindowDefaultColor;
			*(GXColor*)(_54 + 0x9B8) = color;
		}
		break;
	}
	return 0;
}

/**
 * @note Address: 0x80070218
 * @note Size: 0xEC
 */
s32 CEvt::_cmd_WARP(u8, s32)
{
	s32 stage = CEvt_Evaluate(this);
	s32 arg   = CEvt_Evaluate(this);
	// 10000 and 20000 are taken as is, 100 and up counts from the current stage
	if (stage != 10000 && stage != 20000 && stage >= 100) {
		stage += GetStageNum() - 99;
	}
	ChangeGameStage(&gGame, stage, arg);
	CEvt_End(this);
	return 1;
}

/**
 * @note Address: 0x8007464C
 * @note Size: 0x390
 */
s32 CEvt::_cmd_POS(u8, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	f32 x     = CEvt_Evaluate(this) / 100.0f;
	f32 y     = CEvt_Evaluate(this) / 100.0f;
	f32 z     = CEvt_Evaluate(this) / 100.0f;
	if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		obj->SetPos(x, y, z, 0);
		if (obj->mMoveTranslate.y != obj->_264.y) {
			obj->mStatus.mFlags |= 0x0010000000000000;
		}
		obj->_264.x = obj->mMoveTranslate.x;
		obj->_264.y = obj->mMoveTranslate.y;
		obj->_264.z = obj->mMoveTranslate.z;
		gGame.mMotionControlCamera.UpdateTarget(obj);
		u8* carry;
		u8* player = (u8*)gGame.GetPlayer();
		if (obj == (XObj*)player) {
			ResetPlayerHeadOffset((XObjChr*)player);
			ResetPlayerPlug((XObjChr*)player);
			carry = (u8*)&((XObjChr*)player)->mLift;
			if (*(XObj**)(carry + 0x20) != NULL) {
				UpdateHeldObject((XActLift*)carry, 1);
				(*(XObj**)(carry + 0x20))->UpdateObject(1);
			}
			CancelPlayerPushAction(player + PLAYER_BE8);
		}
	} else {
		CEvt_Warn(FormatToStaticBuffer("pos(%d, ...) obj(%d) killed!", id, id));
	}
	return 0;
}

// CEvt::_cmd_POS
/*
    stwu     r1, -0x50(r1)
    mflr     r0
    stw      r0, 0x54(r1)
    stfd     f31, 0x40(r1)
    psq_st   f31, 0x48(r1), 0, qr0
    stfd     f30, 0x30(r1)
    psq_st   f30, 0x38(r1), 0, qr0
    stw      r31, 0x2c(r1)
    stw      r30, 0x28(r1)
    stw      r29, 0x24(r1)
    mr       r29, r3
    bl       CEvt_Evaluate
    extsh    r30, r3
    cmpwi    r30, 0x2710
    blt      .L_80074738
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80074720
.L_800746A0:
    lha      r0, 0x4(r3)
    cmpw     r0, r30
    bne      .L_80074714
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r31, -0x67a0(r3)
    cmplwi   r31, 0x0
    beq      .L_8007470C
    lwz      r0, 0x228(r31)
    li       r4, 0x0
    lwz      r5, 0x22c(r31)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_8007470C
    b        .L_80074724
.L_8007470C:
    li       r31, 0x0
    b        .L_80074724
.L_80074714:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_800746A0
.L_80074720:
    li       r31, 0x0
.L_80074724:
    cmplwi   r31, 0x0
    bne      .L_80074770
    li       r31, 0x0
    b        .L_80074770
    b        .L_80074770
.L_80074738:
    extsh.   r0, r30
    blt      .L_8007475C
    lis      r3, gGame@ha
    slwi     r0, r30, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r31, -0x67a0(r3)
    b        .L_80074770
.L_8007475C:
    cmpwi    r30, -0x1
    bne      .L_8007476C
    lwz      r31, 0x8(r29)
    b        .L_80074770
.L_8007476C:
    mr       r31, r30
.L_80074770:
    mr       r3, r29
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc(r1)
    mr       r3, r29
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x8(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x8(r1)
    fsubs    f1, f1, f2
    fdivs    f31, f1, f0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x14(r1)
    mr       r3, r29
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x10(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x10(r1)
    fsubs    f1, f1, f2
    fdivs    f30, f1, f0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x1c(r1)
    cmplwi   r31, 0x0
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x18(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x18(r1)
    fsubs    f1, f1, f2
    fdivs    f3, f1, f0
    beq      .L_800748FC
    lwz      r0, 0x228(r31)
    li       r4, 0x0
    lwz      r5, 0x22c(r31)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_800748FC
    lwz      r12, 0x34(r31)
    fmr      f1, f31
    fmr      f2, f30
    mr       r3, r31
    lwz      r12, 0x68(r12)
    li       r4, 0x0
    mtctr    r12
    bctrl
    lfs      f1, 0xcc(r31)
    lfs      f0, 0x268(r31)
    fcmpu    cr0, f1, f0
    beq      .L_80074864
    lwz      r3, 0x228(r31)
    lis      r0, 0x10
    or       r0, r3, r0
    stw      r0, 0x228(r31)
.L_80074864:
    lfs      f0, 0xc8(r31)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    mr       r4, r31
    stfs     f0, 0x264(r31)
    addis    r3, r3, 0xe
    addi     r3, r3, 0x5d38
    lfs      f0, 0xcc(r31)
    stfs     f0, 0x268(r31)
    lfs      f0, 0xd0(r31)
    stfs     f0, 0x26c(r31)
    bl       fn_80148D6C
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r30, -0x6944(r3)
    cmplw    r31, r30
    bne      .L_800749AC
    mr       r3, r30
    bl       ResetPlayerHeadOffset
    mr       r3, r30
    bl       ResetPlayerPlug
    addi     r29, r30, 0xa44
    lwz      r0, 0xa64(r30)
    cmplwi   r0, 0x0
    beq      .L_800748F0
    mr       r3, r29
    li       r4, 0x1
    bl       fn_800CECFC
    lwz      r3, 0x20(r29)
    li       r4, 0x1
    lwz      r12, 0x34(r3)
    lwz      r12, 0xa4(r12)
    mtctr    r12
    bctrl
.L_800748F0:
    addi     r3, r30, 0xbe8
    bl       CancelPlayerPushAction
    b        .L_800749AC
.L_800748FC:
    lis      r3, sEvtPosKilled@ha
    mr       r4, r30
    addi     r3, r3, sEvtPosKilled@l
    mr       r5, r30
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80074964
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80074950
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80074950
.L_80074948:
    addi     r3, r3, 0x8
    bdnz     .L_80074948
.L_80074950:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80074964
.L_80074960:
    bdnz     .L_80074960
.L_80074964:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_800749AC
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80074998
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80074998
.L_80074990:
    addi     r3, r3, 0x8
    bdnz     .L_80074990
.L_80074998:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800749AC
.L_800749A8:
    bdnz     .L_800749A8
.L_800749AC:
    li       r3, 0x0
    psq_l    f31, 0x48(r1), 0, qr0
    lfd      f31, 0x40(r1)
    psq_l    f30, 0x38(r1), 0, qr0
    lfd      f30, 0x30(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r0, 0x54(r1)
    lwz      r29, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x50
    blr
*/

// script owner fields MSG and SELECT use, 0x650 lower in japan. _445C0 is raised by
// CEvt_End, _445F8 is nonzero while something waits on the owner, _445D4 is the SELECT answer
#if defined(VERSION_JP)
#define EVT_OWNER_445C0(owner) (*(s32*)((owner) + 0x43F70))
#define EVT_OWNER_445D4(owner) (*(s32*)((owner) + 0x43F84))
#define EVT_OWNER_445F8(owner) (*(s32*)((owner) + 0x43FA8))
#else
#define EVT_OWNER_445C0(owner) (*(s32*)((owner) + 0x445C0))
#define EVT_OWNER_445D4(owner) (*(s32*)((owner) + 0x445D4))
#define EVT_OWNER_445F8(owner) (*(s32*)((owner) + 0x445F8))
#endif

/**
 * @note Address: 0x800749DC
 * @note Size: 0x1E8
 */
s32 CEvt::_cmd_MSG(u8 cmd, s32)
{
	u8* owner = _54;
	if (owner != 0) {
		if (EVT_OWNER_445F8(owner) > 0) {
			if (_58 == 1) {
				EVT_OWNER_445F8(owner) = 2;
				_58++;
				mData.mScript--;
				return 1;
			}
			if (_58 == 2) {
				if (*(u32*)(owner + 0x804) != 0) {
					mData.mScript--;
					return 1;
				}
				_58                  = 0;
				EVT_OWNER_445C0(_54) = 1;
				if (mData.mScript[4] == 0) {
					mData.mScript = ResolveEventScriptOffset(mData.mScript);
					return 0;
				}
			}
		}
		owner = _54;
		if (EVT_OWNER_445C0(owner) != 0) {
			CMsg_Open((CMsg*)owner, cmd, mData.mScript + 4, (MsgFormatArgs*)&mValues.mBuf);
			EVT_OWNER_445C0(_54) = 0;
			mData.mScript--;
			if (cmd == EVT_CMD_SELECT) {
				PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			}
			return 1;
		}
		if (EVT_OWNER_445F8(owner) > 0) {
			_58           = 1;
			mData.mScript = ResolveEventScriptOffset(mData.mScript);
			return 0;
		}
		if (*(u32*)(owner + 0x804) != 0) {
			mData.mScript--;
			return 1;
		}
		if (cmd == EVT_CMD_SELECT) {
			_10 = EVT_OWNER_445D4(owner);
		}
		EVT_OWNER_445C0(_54) = 1;
		mData.mScript        = ResolveEventScriptOffset(mData.mScript);
		return 0;
	}
	mData.mScript = ResolveEventScriptOffset(mData.mScript);
	return 0;
}

extern "C" void StartGameMovie(u8* game, const char* path, BOOL loop, s32 arg);
extern "C" void StopGameMovie(u8* game);

/**
 * @note Address: 0x80074BC4
 * @note Size: 0x130
 */
s32 CEvt::_cmd_MOVIE(u8, s32)
{
	const char* path = (const char*)CEvt_Evaluate(this);
	if (path != NULL) {
#if defined(VERSION_PAL)
		// at 50hz europe plays its own 25 fps cut of the movie
		char buf[0x80];
		if (gVideoFramesPerSecond == 50) {
			sprintf(buf, "%s%s", path, "25");
			path = buf;
		}
#endif
		s32 x   = CEvt_Evaluate(this);
		s32 y   = CEvt_Evaluate(this);
		s32 w   = CEvt_Evaluate(this);
		s32 h   = CEvt_Evaluate(this);
		s32 arg = CEvt_Evaluate(this);
		// x and y are the center, the rect wants the corner
		x -= w / 2;
		y -= h / 2;
		StartGameMovie((u8*)&gGame, path, FALSE, arg);
		gGame.mMovieRect[0] = x;
		gGame.mMovieRect[1] = y;
		gGame.mMovieRect[2] = w;
		gGame.mMovieRect[3] = h;
	} else {
		StopGameMovie((u8*)&gGame);
	}
	return 0;
}

// CEvt::_cmd_MOVIE
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    stw      r0, 0x44(r1)
    stmw     r26, 0x28(r1)
    mr       r26, r3
    bl       CEvt_Evaluate
    mr.      r27, r3
    beq      .L_80074CD0
    mr       r3, r26
    bl       CEvt_Evaluate
    mr       r28, r3
    mr       r3, r26
    bl       CEvt_Evaluate
    mr       r29, r3
    mr       r3, r26
    bl       CEvt_Evaluate
    mr       r30, r3
    mr       r3, r26
    bl       CEvt_Evaluate
    mr       r31, r3
    mr       r3, r26
    bl       CEvt_Evaluate
    srwi     r4, r30, 31
    srwi     r0, r31, 31
    add      r4, r4, r30
    lis      r5, gGame@ha
    srawi    r8, r4, 1
    add      r0, r0, r31
    srawi    r7, r0, 1
    mr       r4, r27
    addi     r0, r5, gGame@l
    mr       r6, r3
    subf     r27, r8, r28
    subf     r28, r7, r29
    mr       r3, r0
    li       r5, 0x0
    bl       StartGameMovie
    lis      r6, 0x4330
    xoris    r0, r27, 0x8000
    stw      r0, 0xc(r1)
    xoris    r4, r28, 0x8000
    xoris    r3, r30, 0x8000
    lis      r5, gGame@ha
    stw      r6, 0x8(r1)
    addi     r5, r5, gGame@l
    lfd      f3, lbl_8065CF88@sda21(r0)
    xoris    r0, r31, 0x8000
    lfd      f0, 0x8(r1)
    addis    r5, r5, 0x10
    stw      r4, 0x14(r1)
    fsubs    f1, f0, f3
    stw      r6, 0x10(r1)
    lfd      f0, 0x10(r1)
    stw      r3, 0x1c(r1)
    fsubs    f2, f0, f3
    stw      r6, 0x18(r1)
    lfd      f0, 0x18(r1)
    stfs     f1, -0xba0(r5)
    fsubs    f1, f0, f3
    stfs     f2, -0xb9c(r5)
    stw      r0, 0x24(r1)
    stw      r6, 0x20(r1)
    lfd      f0, 0x20(r1)
    stfs     f1, -0xb98(r5)
    fsubs    f0, f0, f3
    stfs     f0, -0xb94(r5)
    b        .L_80074CDC
.L_80074CD0:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    bl       StopGameMovie
.L_80074CDC:
    lmw      r26, 0x28(r1)
    li       r3, 0x0
    lwz      r0, 0x44(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

// the y angle that faces from toward to
static inline f32 CEvt_Angle(const Vec& from, const Vec& to)
{
	return atan2(to.x - from.x, to.z - from.z);
}

/**
 * @note Address: 0x80074CF4
 * @note Size: 0x760
 */
s32 CEvt::_cmd_MOVETO(u8, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	f32 x     = CEvt_Evaluate(this) / 100.0f;
	f32 y     = CEvt_Evaluate(this) / 100.0f;
	f32 z     = CEvt_Evaluate(this) / 100.0f;
	f32 time  = CEvt_Evaluate(this) / 100.0f;
	s32 anim  = CEvt_Evaluate(this);
	s16 dir   = CEvt_Evaluate(this);
	if (obj != NULL && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
		if (obj->mJObj == NULL) {
			CEvt_Warn(FormatToStaticBuffer("moveto(%d, %d,%d,%d, %d, %d, %d) ... obj(%d) is collision!", id, (s32)(x * 100.0f),
			                               (s32)(y * 100.0f), (s32)(z * 100.0f), (s32)(time * 100.0f), anim, dir, id));
		}
		u8* player = (u8*)gGame.GetPlayer();
		if (obj == (XObj*)player) {
			CancelPlayerPushAction(player + PLAYER_BE8);
		}
		f32 distance;
		{
			CVec target(x, y, z);
			distance = PSVECDistance(&obj->mMoveTranslate, &target);
		}
		if (distance < 0.1f) {
			// already there
			obj->SetPos(x, y, z, 0);
		} else {
			if (time == 0.0f) {
				obj->SetMoveDir(CVec(x, y, z), distance);
			} else if (time > 0.0f) {
				obj->SetMoveDir(CVec(x, y, z), distance / (time * FRAMES_PER_SEC));
			} else {
				obj->SetMoveDir(CVec(x, y, z), -time);
			}
			if (dir != -1) {
				f32 yaw;
				// -2 faces where it's going
				if (dir == -2) {
					yaw = CEvt_Angle(obj->mMoveTranslate, CVec(x, y, z));
				} else {
					yaw = DEG2RAD * dir;
				}
				obj->SetRotationY(yaw, 0);
			}
			if (anim != -1) {
				if (anim & 0xFFFF0000) {
					s32 jointId     = anim & 0xFFFF;
					s32 matId       = anim >> 16;
					void* jointAnim = obj->GetJointAnimation(jointId);
					void* matAnim   = obj->GetMaterialAnimation(matId);
					void* shapeAnim = obj->GetShapeAnimation(jointId);
					obj->SetAnim(0, TRUE, jointAnim, matAnim, shapeAnim, 0.0f, 1.0f, 0.0f, 10.0f);
					if (jointId >= 1000 || matId >= 1000) {
						obj->mStatus.mFlags |= 0x0008000000000000;
					}
				} else {
					obj->SetAnim(anim, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				}
				obj->UpdateAnim(1);
			}
		}
	} else if (obj == NULL || !(obj->mStatus.mFlags & 1)) {
		CEvt_Warn(FormatToStaticBuffer("moveto(%d, ...) obj(%d) killed!", id, id));
	}
	return 0;
}
/*
    stwu     r1, -0xe0(r1)
    mflr     r0
    stw      r0, 0xe4(r1)
    stfd     f31, 0xd0(r1)
    psq_st   f31, 0xd8(r1), 0, qr0
    stfd     f30, 0xc0(r1)
    psq_st   f30, 0xc8(r1), 0, qr0
    stfd     f29, 0xb0(r1)
    psq_st   f29, 0xb8(r1), 0, qr0
    stfd     f28, 0xa0(r1)
    psq_st   f28, 0xa8(r1), 0, qr0
    stmw     r27, 0x8c(r1)
    mr       r28, r3
    bl       CEvt_Evaluate
    extsh    r30, r3
    cmpwi    r30, 0x2710
    blt      .L_80074DE8
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80074DD0
.L_80074D50:
    lha      r0, 0x4(r3)
    cmpw     r0, r30
    bne      .L_80074DC4
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r31, -0x67a0(r3)
    cmplwi   r31, 0x0
    beq      .L_80074DBC
    lwz      r0, 0x228(r31)
    li       r4, 0x0
    lwz      r5, 0x22c(r31)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_80074DBC
    b        .L_80074DD4
.L_80074DBC:
    li       r31, 0x0
    b        .L_80074DD4
.L_80074DC4:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_80074D50
.L_80074DD0:
    li       r31, 0x0
.L_80074DD4:
    cmplwi   r31, 0x0
    bne      .L_80074E20
    li       r31, 0x0
    b        .L_80074E20
    b        .L_80074E20
.L_80074DE8:
    extsh.   r0, r30
    blt      .L_80074E0C
    lis      r3, gGame@ha
    slwi     r0, r30, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r31, -0x67a0(r3)
    b        .L_80074E20
.L_80074E0C:
    cmpwi    r30, -0x1
    bne      .L_80074E1C
    lwz      r31, 0x8(r28)
    b        .L_80074E20
.L_80074E1C:
    mr       r31, r30
.L_80074E20:
    mr       r3, r28
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x64(r1)
    mr       r3, r28
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x60(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x60(r1)
    fsubs    f1, f1, f2
    fdivs    f31, f1, f0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x6c(r1)
    mr       r3, r28
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x68(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x68(r1)
    fsubs    f1, f1, f2
    fdivs    f30, f1, f0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x74(r1)
    mr       r3, r28
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x70(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x70(r1)
    fsubs    f1, f1, f2
    fdivs    f29, f1, f0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x7c(r1)
    mr       r3, r28
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x78(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x78(r1)
    fsubs    f1, f1, f2
    fdivs    f28, f1, f0
    bl       CEvt_Evaluate
    mr       r0, r3
    mr       r3, r28
    mr       r29, r0
    bl       CEvt_Evaluate
    cmplwi   r31, 0x0
    extsh    r28, r3
    beq      .L_8007533C
    lwz      r3, 0x228(r31)
    li       r5, 0x0
    lwz      r6, 0x22c(r31)
    li       r0, 0x1
    and      r4, r3, r5
    and      r0, r6, r0
    xor      r3, r0, r5
    xor      r0, r4, r5
    or.      r0, r3, r0
    beq      .L_8007533C
    lis      r3, 0x1
    xor      r0, r4, r5
    and      r3, r6, r3
    xor      r3, r3, r5
    or.      r0, r3, r0
    beq      .L_8007533C
    lwz      r0, 0x0(r31)
    cmplwi   r0, 0x0
    bne      .L_8007503C
    lfs      f0, lbl_8065CF80@sda21(r0)
    lis      r3, sEvtMoveToCollision@ha
    stw      r30, 0x8(r1)
    addi     r3, r3, sEvtMoveToCollision@l
    fmuls    f3, f0, f31
    mr       r4, r30
    fmuls    f2, f0, f30
    mr       r9, r29
    fmuls    f1, f0, f29
    mr       r10, r28
    fmuls    f0, f0, f28
    fctiwz   f3, f3
    fctiwz   f2, f2
    fctiwz   f1, f1
    fctiwz   f0, f0
    stfd     f3, 0x78(r1)
    stfd     f2, 0x70(r1)
    lwz      r5, 0x7c(r1)
    stfd     f1, 0x68(r1)
    lwz      r6, 0x74(r1)
    stfd     f0, 0x60(r1)
    lwz      r7, 0x6c(r1)
    lwz      r8, 0x64(r1)
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80074FF4
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80074FE0
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80074FE0
.L_80074FD8:
    addi     r3, r3, 0x8
    bdnz     .L_80074FD8
.L_80074FE0:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80074FF4
.L_80074FF0:
    bdnz     .L_80074FF0
.L_80074FF4:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_8007503C
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80075028
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80075028
.L_80075020:
    addi     r3, r3, 0x8
    bdnz     .L_80075020
.L_80075028:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007503C
.L_80075038:
    bdnz     .L_80075038
.L_8007503C:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    cmplw    r31, r3
    bne      .L_8007505C
    addi     r3, r3, 0xbe8
    bl       CancelPlayerPushAction
.L_8007505C:
    lis      r3, __vt__4CVec@ha
    stfs     f31, 0x50(r1)
    addi     r0, r3, __vt__4CVec@l
    addi     r3, r31, 0xc8
    stw      r0, 0x5c(r1)
    addi     r4, r1, 0x50
    stfs     f30, 0x54(r1)
    stfs     f29, 0x58(r1)
    bl       PSVECDistance
    lfs      f0, lbl_8065CFEC@sda21(r0)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    fcmpo    cr0, f1, f0
    stw      r0, 0x5c(r1)
    bge      .L_800750C0
    mr       r3, r31
    fmr      f1, f31
    lwz      r12, 0x34(r31)
    fmr      f2, f30
    fmr      f3, f29
    li       r4, 0x0
    lwz      r12, 0x68(r12)
    mtctr    r12
    bctrl
    b        .L_8007541C
.L_800750C0:
    lfs      f0, lbl_8065CF74@sda21(r0)
    fcmpu    cr0, f0, f28
    bne      .L_80075104
    stw      r0, 0x4c(r1)
    mr       r3, r31
    addi     r4, r1, 0x40
    stfs     f31, 0x40(r1)
    stfs     f30, 0x44(r1)
    stfs     f29, 0x48(r1)
    lwz      r12, 0x34(r31)
    lwz      r12, 0x78(r12)
    mtctr    r12
    bctrl
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x4c(r1)
    b        .L_80075188
.L_80075104:
    fcmpo    cr0, f28, f0
    ble      .L_80075150
    lfs      f0, lbl_8065CFC0@sda21(r0)
    mr       r3, r31
    stw      r0, 0x3c(r1)
    addi     r4, r1, 0x30
    fmuls    f0, f0, f28
    stfs     f31, 0x30(r1)
    fdivs    f1, f1, f0
    stfs     f30, 0x34(r1)
    stfs     f29, 0x38(r1)
    lwz      r12, 0x34(r31)
    lwz      r12, 0x78(r12)
    mtctr    r12
    bctrl
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x3c(r1)
    b        .L_80075188
.L_80075150:
    stw      r0, 0x2c(r1)
    mr       r3, r31
    fneg     f1, f28
    addi     r4, r1, 0x20
    stfs     f31, 0x20(r1)
    stfs     f30, 0x24(r1)
    stfs     f29, 0x28(r1)
    lwz      r12, 0x34(r31)
    lwz      r12, 0x78(r12)
    mtctr    r12
    bctrl
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x2c(r1)
.L_80075188:
    cmpwi    r28, -0x1
    beq      .L_80075218
    cmpwi    r28, -0x2
    bne      .L_800751DC
    lfs      f1, 0xc8(r31)
    lis      r3, __vt__4CVec@ha
    lfs      f0, 0xd0(r31)
    addi     r0, r3, __vt__4CVec@l
    fsubs    f1, f31, f1
    stw      r0, 0x1c(r1)
    fsubs    f2, f29, f0
    stfs     f31, 0x10(r1)
    stfs     f30, 0x14(r1)
    stfs     f29, 0x18(r1)
    bl       atan2
    frsp     f0, f1
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x1c(r1)
    fmr      f1, f0
    b        .L_80075200
.L_800751DC:
    xoris    r3, r28, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x7c(r1)
    lfd      f1, lbl_8065CF88@sda21(r0)
    stw      r0, 0x78(r1)
    lfs      f2, lbl_8065CFE8@sda21(r0)
    lfd      f0, 0x78(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f2, f0
.L_80075200:
    lwz      r12, 0x34(r31)
    mr       r3, r31
    li       r4, 0x0
    lwz      r12, 0x28(r12)
    mtctr    r12
    bctrl
.L_80075218:
    cmpwi    r29, -0x1
    beq      .L_8007541C
    clrrwi.  r0, r29, 16
    beq      .L_800752E8
    mr       r3, r31
    clrlwi   r28, r29, 16
    lwz      r12, 0x34(r31)
    mr       r4, r28
    srawi    r29, r29, 16
    lwz      r12, 0xf0(r12)
    mtctr    r12
    bctrl
    mr       r0, r3
    mr       r3, r31
    lwz      r12, 0x34(r31)
    mr       r30, r0
    mr       r4, r29
    lwz      r12, 0xf4(r12)
    mtctr    r12
    bctrl
    mr       r0, r3
    mr       r3, r31
    lwz      r12, 0x34(r31)
    mr       r27, r0
    mr       r4, r28
    lwz      r12, 0xf8(r12)
    mtctr    r12
    bctrl
    lwz      r12, 0x34(r31)
    mr       r0, r3
    lfs      f1, lbl_8065CF74@sda21(r0)
    mr       r3, r31
    lwz      r12, 0x80(r12)
    mr       r8, r0
    fmr      f3, f1
    mr       r6, r30
    mr       r7, r27
    lfs      f2, lbl_8065CFE0@sda21(r0)
    li       r4, 0x0
    lfs      f4, lbl_8065CFE4@sda21(r0)
    li       r5, 0x1
    mtctr    r12
    bctrl
    cmpwi    r28, 0x3e8
    bge      .L_800752D4
    cmpwi    r29, 0x3e8
    blt      .L_80075320
.L_800752D4:
    lwz      r3, 0x228(r31)
    lis      r0, 0x8
    or       r0, r3, r0
    stw      r0, 0x228(r31)
    b        .L_80075320
.L_800752E8:
    mr       r3, r31
    lfs      f1, lbl_8065CF74@sda21(r0)
    lwz      r12, 0x34(r31)
    mr       r4, r29
    fmr      f3, f1
    lfs      f2, lbl_8065CFE0@sda21(r0)
    lwz      r12, 0x80(r12)
    li       r5, 0x0
    lfs      f4, lbl_8065CFE4@sda21(r0)
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    mtctr    r12
    bctrl
.L_80075320:
    mr       r3, r31
    li       r4, 0x1
    lwz      r12, 0x34(r31)
    lwz      r12, 0x88(r12)
    mtctr    r12
    bctrl
    b        .L_8007541C
.L_8007533C:
    cmplwi   r31, 0x0
    beq      .L_8007536C
    lwz      r0, 0x228(r31)
    li       r4, 0x0
    lwz      r5, 0x22c(r31)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    bne      .L_8007541C
.L_8007536C:
    lis      r3, sEvtMoveToKilled@ha
    mr       r4, r30
    addi     r3, r3, sEvtMoveToKilled@l
    mr       r5, r30
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_800753D4
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800753C0
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800753C0
.L_800753B8:
    addi     r3, r3, 0x8
    bdnz     .L_800753B8
.L_800753C0:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800753D4
.L_800753D0:
    bdnz     .L_800753D0
.L_800753D4:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_8007541C
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80075408
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80075408
.L_80075400:
    addi     r3, r3, 0x8
    bdnz     .L_80075400
.L_80075408:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007541C
.L_80075418:
    bdnz     .L_80075418
.L_8007541C:
    li       r3, 0x0
    psq_l    f31, 0xd8(r1), 0, qr0
    lfd      f31, 0xd0(r1)
    psq_l    f30, 0xc8(r1), 0, qr0
    lfd      f30, 0xc0(r1)
    psq_l    f29, 0xb8(r1), 0, qr0
    lfd      f29, 0xb0(r1)
    psq_l    f28, 0xa8(r1), 0, qr0
    lfd      f28, 0xa0(r1)
    lmw      r27, 0x8c(r1)
    lwz      r0, 0xe4(r1)
    mtlr     r0
    addi     r1, r1, 0xe0
    blr
*/

/**
 * @note Address: 0x80075454
 * @note Size: 0x62C
 */
s32 CEvt::_cmd_MOVE(u8, s32)
{
	s16 id       = CEvt_Evaluate(this);
	XObj* obj    = CEvt_GetObj(this, id);
	f32 distance = CEvt_Evaluate(this) / 100.0f;
	f32 time     = CEvt_Evaluate(this) / 100.0f;
	s32 anim     = CEvt_Evaluate(this);
	s16 dir      = CEvt_Evaluate(this);
	if (obj != NULL && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
		if (obj->mJObj == NULL) {
			// retail's format says %d but passes a float
			CEvt_Warn(FormatToStaticBuffer("move(%d, %d, %d, %d, %d) ... obj(%d) is collision!", id, distance, (s32)(time * 100.0f), anim,
			                               dir, id));
		}
		u8* player = (u8*)gGame.GetPlayer();
		if (obj == (XObj*)player) {
			CancelPlayerPushAction(player + PLAYER_BE8);
		}
		Vec target = obj->mMoveTranslate;
		// -1 and -2 keep the way it faces
		f32 yaw = (dir == -1 || dir == -2) ? obj->mMoveRotate.y : DEG2RAD * dir;
		target.x += distance * (f32)sin(yaw);
		target.z += distance * (f32)cos(yaw);
		if (time == 0.0f) {
			obj->SetMoveDir(target, distance);
		} else if (time > 0.0f) {
			obj->SetMoveDir(target, distance / (time * FRAMES_PER_SEC));
		} else {
			obj->SetMoveDir(target, -time);
		}
		if (anim != -1) {
			if (anim & 0xFFFF0000) {
				s32 jointId     = anim & 0xFFFF;
				s32 matId       = anim >> 16;
				void* jointAnim = obj->GetJointAnimation(jointId);
				void* matAnim   = obj->GetMaterialAnimation(matId);
				void* shapeAnim = obj->GetShapeAnimation(jointId);
				obj->SetAnim(0, TRUE, jointAnim, matAnim, shapeAnim, 0.0f, 1.0f, 0.0f, 10.0f);
				if (jointId >= 1000 || matId >= 1000) {
					obj->mStatus.mFlags |= 0x0008000000000000;
				}
			} else {
				obj->SetAnim(anim, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
			obj->UpdateAnim(1);
		}
	} else if (obj == NULL || !(obj->mStatus.mFlags & 1)) {
		CEvt_Warn(FormatToStaticBuffer("move(%d, ...) obj(%d) killed!", id, id));
	}
	return 0;
}
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
    stmw     r27, 0x2c(r1)
    mr       r28, r3
    bl       CEvt_Evaluate
    extsh    r30, r3
    cmpwi    r30, 0x2710
    blt      .L_80075540
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80075528
.L_800754A8:
    lha      r0, 0x4(r3)
    cmpw     r0, r30
    bne      .L_8007551C
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r31, -0x67a0(r3)
    cmplwi   r31, 0x0
    beq      .L_80075514
    lwz      r0, 0x228(r31)
    li       r4, 0x0
    lwz      r5, 0x22c(r31)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_80075514
    b        .L_8007552C
.L_80075514:
    li       r31, 0x0
    b        .L_8007552C
.L_8007551C:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_800754A8
.L_80075528:
    li       r31, 0x0
.L_8007552C:
    cmplwi   r31, 0x0
    bne      .L_80075578
    li       r31, 0x0
    b        .L_80075578
    b        .L_80075578
.L_80075540:
    extsh.   r0, r30
    blt      .L_80075564
    lis      r3, gGame@ha
    slwi     r0, r30, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r31, -0x67a0(r3)
    b        .L_80075578
.L_80075564:
    cmpwi    r30, -0x1
    bne      .L_80075574
    lwz      r31, 0x8(r28)
    b        .L_80075578
.L_80075574:
    mr       r31, r30
.L_80075578:
    mr       r3, r28
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x1c(r1)
    mr       r3, r28
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x18(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x18(r1)
    fsubs    f1, f1, f2
    fdivs    f31, f1, f0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x24(r1)
    mr       r3, r28
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x20(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x20(r1)
    fsubs    f1, f1, f2
    fdivs    f29, f1, f0
    bl       CEvt_Evaluate
    mr       r0, r3
    mr       r3, r28
    mr       r29, r0
    bl       CEvt_Evaluate
    cmplwi   r31, 0x0
    extsh    r28, r3
    beq      .L_80075970
    lwz      r3, 0x228(r31)
    li       r5, 0x0
    lwz      r6, 0x22c(r31)
    li       r0, 0x1
    and      r4, r3, r5
    and      r0, r6, r0
    xor      r3, r0, r5
    xor      r0, r4, r5
    or.      r0, r3, r0
    beq      .L_80075970
    lis      r3, 0x1
    xor      r0, r4, r5
    and      r3, r6, r3
    xor      r3, r3, r5
    or.      r0, r3, r0
    beq      .L_80075970
    lwz      r0, 0x0(r31)
    cmplwi   r0, 0x0
    bne      .L_80075710
    lfs      f0, lbl_8065CF80@sda21(r0)
    lis      r3, sEvtMoveCollision@ha
    fmr      f1, f31
    addi     r3, r3, sEvtMoveCollision@l
    fmuls    f0, f0, f29
    mr       r4, r30
    mr       r6, r29
    mr       r7, r28
    fctiwz   f0, f0
    mr       r8, r30
    stfd     f0, 0x20(r1)
    lwz      r5, 0x24(r1)
    crset    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_800756C8
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800756B4
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800756B4
.L_800756AC:
    addi     r3, r3, 0x8
    bdnz     .L_800756AC
.L_800756B4:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800756C8
.L_800756C4:
    bdnz     .L_800756C4
.L_800756C8:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80075710
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800756FC
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800756FC
.L_800756F4:
    addi     r3, r3, 0x8
    bdnz     .L_800756F4
.L_800756FC:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80075710
.L_8007570C:
    bdnz     .L_8007570C
.L_80075710:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    cmplw    r31, r3
    bne      .L_80075730
    addi     r3, r3, 0xbe8
    bl       CancelPlayerPushAction
.L_80075730:
    addi     r0, r28, 0x2
    lwz      r5, 0xc8(r31)
    lwz      r4, 0xcc(r31)
    clrlwi   r3, r0, 16
    subfic   r0, r3, 0x1
    li       r6, 0x1
    stw      r5, 0x8(r1)
    orc      r3, r6, r3
    srwi     r0, r0, 1
    subf     r0, r0, r3
    stw      r4, 0xc(r1)
    srwi.    r0, r0, 31
    lwz      r3, 0xd0(r31)
    stw      r3, 0x10(r1)
    bne      .L_80075770
    li       r6, 0x0
.L_80075770:
    clrlwi.  r0, r6, 24
    beq      .L_80075780
    lfs      f30, 0xfc(r31)
    b        .L_800757A4
.L_80075780:
    xoris    r3, r28, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x24(r1)
    lfd      f1, lbl_8065CF88@sda21(r0)
    stw      r0, 0x20(r1)
    lfs      f2, lbl_8065CFE8@sda21(r0)
    lfd      f0, 0x20(r1)
    fsubs    f0, f0, f1
    fmuls    f30, f2, f0
.L_800757A4:
    fmr      f1, f30
    bl       sin
    frsp     f2, f1
    lfs      f0, 0x8(r1)
    fmr      f1, f30
    fmadds   f0, f31, f2, f0
    stfs     f0, 0x8(r1)
    bl       cos
    frsp     f2, f1
    lfs      f1, 0x10(r1)
    lfs      f0, lbl_8065CF74@sda21(r0)
    fmadds   f1, f31, f2, f1
    fcmpu    cr0, f0, f29
    stfs     f1, 0x10(r1)
    bne      .L_80075800
    mr       r3, r31
    fmr      f1, f31
    lwz      r12, 0x34(r31)
    addi     r4, r1, 0x8
    lwz      r12, 0x78(r12)
    mtctr    r12
    bctrl
    b        .L_8007584C
.L_80075800:
    fcmpo    cr0, f29, f0
    ble      .L_80075830
    lfs      f0, lbl_8065CFC0@sda21(r0)
    mr       r3, r31
    lwz      r12, 0x34(r31)
    addi     r4, r1, 0x8
    fmuls    f0, f0, f29
    lwz      r12, 0x78(r12)
    fdivs    f1, f31, f0
    mtctr    r12
    bctrl
    b        .L_8007584C
.L_80075830:
    mr       r3, r31
    fneg     f1, f29
    lwz      r12, 0x34(r31)
    addi     r4, r1, 0x8
    lwz      r12, 0x78(r12)
    mtctr    r12
    bctrl
.L_8007584C:
    cmpwi    r29, -0x1
    beq      .L_80075A50
    clrrwi.  r0, r29, 16
    beq      .L_8007591C
    mr       r3, r31
    clrlwi   r28, r29, 16
    lwz      r12, 0x34(r31)
    mr       r4, r28
    srawi    r29, r29, 16
    lwz      r12, 0xf0(r12)
    mtctr    r12
    bctrl
    mr       r0, r3
    mr       r3, r31
    lwz      r12, 0x34(r31)
    mr       r30, r0
    mr       r4, r29
    lwz      r12, 0xf4(r12)
    mtctr    r12
    bctrl
    mr       r0, r3
    mr       r3, r31
    lwz      r12, 0x34(r31)
    mr       r27, r0
    mr       r4, r28
    lwz      r12, 0xf8(r12)
    mtctr    r12
    bctrl
    lwz      r12, 0x34(r31)
    mr       r0, r3
    lfs      f1, lbl_8065CF74@sda21(r0)
    mr       r3, r31
    lwz      r12, 0x80(r12)
    mr       r8, r0
    fmr      f3, f1
    mr       r6, r30
    mr       r7, r27
    lfs      f2, lbl_8065CFE0@sda21(r0)
    li       r4, 0x0
    lfs      f4, lbl_8065CFE4@sda21(r0)
    li       r5, 0x1
    mtctr    r12
    bctrl
    cmpwi    r28, 0x3e8
    bge      .L_80075908
    cmpwi    r29, 0x3e8
    blt      .L_80075954
.L_80075908:
    lwz      r3, 0x228(r31)
    lis      r0, 0x8
    or       r0, r3, r0
    stw      r0, 0x228(r31)
    b        .L_80075954
.L_8007591C:
    mr       r3, r31
    lfs      f1, lbl_8065CF74@sda21(r0)
    lwz      r12, 0x34(r31)
    mr       r4, r29
    fmr      f3, f1
    lfs      f2, lbl_8065CFE0@sda21(r0)
    lwz      r12, 0x80(r12)
    li       r5, 0x0
    lfs      f4, lbl_8065CFE4@sda21(r0)
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    mtctr    r12
    bctrl
.L_80075954:
    mr       r3, r31
    li       r4, 0x1
    lwz      r12, 0x34(r31)
    lwz      r12, 0x88(r12)
    mtctr    r12
    bctrl
    b        .L_80075A50
.L_80075970:
    cmplwi   r31, 0x0
    beq      .L_800759A0
    lwz      r0, 0x228(r31)
    li       r4, 0x0
    lwz      r5, 0x22c(r31)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    bne      .L_80075A50
.L_800759A0:
    lis      r3, sEvtMoveKilled@ha
    mr       r4, r30
    addi     r3, r3, sEvtMoveKilled@l
    mr       r5, r30
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80075A08
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800759F4
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800759F4
.L_800759EC:
    addi     r3, r3, 0x8
    bdnz     .L_800759EC
.L_800759F4:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80075A08
.L_80075A04:
    bdnz     .L_80075A04
.L_80075A08:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80075A50
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80075A3C
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80075A3C
.L_80075A34:
    addi     r3, r3, 0x8
    bdnz     .L_80075A34
.L_80075A3C:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80075A50
.L_80075A4C:
    bdnz     .L_80075A4C
.L_80075A50:
    li       r3, 0x0
    psq_l    f31, 0x68(r1), 0, qr0
    lfd      f31, 0x60(r1)
    psq_l    f30, 0x58(r1), 0, qr0
    lfd      f30, 0x50(r1)
    psq_l    f29, 0x48(r1), 0, qr0
    lfd      f29, 0x40(r1)
    lmw      r27, 0x2c(r1)
    lwz      r0, 0x74(r1)
    mtlr     r0
    addi     r1, r1, 0x70
    blr
*/

// an entry in the game object's menu table
struct UnkEvtMenu {
	u8 _00[0x148]; // _00
	// _148 vtable

	virtual void vf08();
	virtual void vf0C();
	virtual void vf10();
	virtual void vf14();
	virtual void vf18();
	virtual void vf1C(); // _1C, MENU opens it with this
};

#if defined(VERSION_JP)
#define PLAYER_MENU_12E44 0x12D44
#define PLAYER_MENU_130D0 0x12FB8
#define PLAYER_MENU_130DC 0x12FC4
#define PLAYER_MENU_13368 0x13238
#else
#define PLAYER_MENU_12E44 0x12E44
#define PLAYER_MENU_130D0 0x130D0
#define PLAYER_MENU_130DC 0x130DC
#define PLAYER_MENU_13368 0x13368
#endif

extern "C" void ClosePlayerEquipmentMenu(u8* player, s32 arg);

/**
 * @note Address: 0x80075A80
 * @note Size: 0x19C
 */
s32 CEvt::_cmd_MENU(u8, s32)
{
	s32 ret = 0;
	// rewind to here until the menu closes
	u8* cmd = mData.mScript - 1;
	s32 id  = CEvt_Evaluate(this);
	if (_20 != 0) {
		if (gGame.mMenu.mActive != 0) {
			mData.mScript = cmd;
			ret           = 1;
		} else {
			_20 = 0;
		}
	} else if (id == 1000) {
		s32 arg = CEvt_Evaluate(this);
		if (arg >= 0) {
			SelectPlayerEquipmentRing(&EVT_PLAYER, arg);
		} else {
			u8* player = (u8*)gGame.GetPlayer();
			bool busy  = false;
			if (*(s32*)(player + PLAYER_MENU_12E44) != 0 || *(s32*)(player + PLAYER_MENU_130DC) != 0) {
				busy = true;
			}
			if (busy) {
				ClosePlayerEquipmentMenu(player, -1);
			}
		}
	} else if (id == 1001) {
		s32 a                                               = CEvt_Evaluate(this);
		s32 b                                               = CEvt_Evaluate(this);
		*(s32*)((u8*)gGame.GetPlayer() + PLAYER_MENU_130D0) = a;
		*(s32*)((u8*)gGame.GetPlayer() + PLAYER_MENU_13368) = b;
	} else {
		UnkEvtMenu* menu = (UnkEvtMenu*)gGame.mMenu.mMenus[id];
#if defined(VERSION_JP)
		// no null check in japan
		menu->vf1C();
		gGame.mMenu.mActive = 1;
#else
		if (menu != 0) {
			menu->vf1C();
			gGame.mMenu.mActive = 1;
		}
#endif
		_20           = 1;
		ret           = 1;
		mData.mScript = cmd;
	}
	return ret;
}

// walked through the lobj next links

/**
 * @note Address: 0x80075C1C
 * @note Size: 0x290
 */
s32 CEvt::_cmd_LIGHT(u8, s32)
{
	s32 index     = CEvt_Evaluate(this);
	CLight* light = gGame.mLightList;
	for (s32 i = 0; i < index; i++) {
		light = (CLight*)light->next;
		if (light == NULL) {
			break;
		}
	}

	u16 type = CEvt_Evaluate(this);
	switch (type) {
	case 0xDB: {
		u8 r            = CEvt_Evaluate(this);
		u8 g            = CEvt_Evaluate(this);
		u8 b            = CEvt_Evaluate(this);
		light->mColor.r = r;
		light->mColor.g = g;
		light->mColor.b = b;
		light->mColor.a = 255;
		HSD_LObjSetColor(light, light->mColor);
		break;
	}
	case 0xCF: {
		// positions come in hundredths
		f32 x = CEvt_Evaluate(this) / 100.0f;
		f32 y = CEvt_Evaluate(this) / 100.0f;
		light->SetPosition(x, y, CEvt_Evaluate(this) / 100.0f);
		break;
	}
	case 0xE3: {
		f32 x = CEvt_Evaluate(this) / 100.0f;
		f32 y = CEvt_Evaluate(this) / 100.0f;
		light->SetInterest(x, y, CEvt_Evaluate(this) / 100.0f);
		break;
	}
	default:
#if defined(VERSION_PAL)
		OSPanic(__FILE__, 2846, "%s%s", FormatToStaticBuffer("light(%d, %d,...) - unresolved!\n", index, type), __FUNCTION__);
#else
		OSPanic(__FILE__, 2842, "%s%s", FormatToStaticBuffer("light(%d, %d,...) - unresolved!\n", index, type), __FUNCTION__);
#endif
		break;
	}
	return 0;
}

/**
 * @note Address: 0x80075EAC
 * @note Size: 0x400
 */
s32 CEvt::_cmd_KILL(u8, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		XObj* held = *(XObj**)((u8*)gGame.GetPlayer() + PLAYER_HELD_OBJ);
		if (held != NULL && obj == held && (obj->mStatus.mFlags & 0x0000400000000000)) {
			// chibi is carrying it, can't kill that
			CEvt_Warn(FormatToStaticBuffer("kill(%d) ... obj(%d) lifted!", id, id));
		} else {
			obj->Release();
			CEvt_ForgetObjId(id);
		}
	} else {
		// not spawned yet, mark its stage entry so it never gets read
		s16 index;
		if (id >= 10000) {
			index = CEvt_FindObjIndex(id);
		} else {
			index = id;
		}
		CEvt_ForgetObjId(id);
		UnkStageObj* entry = GetStagePlacementByIndex(index);
		CoordInfo info(entry);
		entry->_34 |= 0x80000000ULL;
	}
	return 0;
}

/**
 * @note Address: 0x800762AC
 * @note Size: 0x358
 */
s32 CEvt::_cmd_DISP(u8, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	u8 on     = CEvt_Evaluate(this);
	if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		XObj* held = *(XObj**)((u8*)gGame.GetPlayer() + PLAYER_HELD_OBJ);
		if (held != NULL && obj == held && (obj->mStatus.mFlags & 0x0000400000000000)) {
			// chibi is carrying it, leave it be
			CEvt_Warn(FormatToStaticBuffer("disp(%d, %d) ... obj(%d) lifted!", id, on, id));
		} else if (on) {
			obj->mStatus.mFlags |= 0x10000;
		} else {
			obj->mStatus.mFlags &= ~0x10000;
		}
	} else {
		CEvt_Warn(FormatToStaticBuffer("disp(%d, %d) ... obj(%d) killed!", id, on, id));
	}
	return 0;
}

/**
 * @note Address: 0x80076604
 * @note Size: 0xC08
 */
s32 CEvt::_cmd_DIR(u8 command, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		u8* player = (u8*)gGame.GetPlayer();
		if ((u8*)obj == player)
			CancelPlayerPushAction(player + PLAYER_BE8);
	}
	if (command == EVT_CMD_DIR) {
		s16 direction = CEvt_Evaluate(this);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			obj->SetRotationY(DEG2RAD * direction, 0);
			gGame.mMotionControlCamera.UpdateTarget(obj);
			u8* player = (u8*)gGame.GetPlayer();
			if ((u8*)obj == player) {
				ResetPlayerHeadOffset((XObjChr*)player);
				XActLift* carry = &((XObjChr*)player)->mLift;
				if (carry->mTarget != NULL) {
					UpdateHeldObject(carry, 1);
					carry->mTarget->UpdateObject(1);
				}
			}
		}
	} else {
		u16 mode = CEvt_Evaluate(this);
		s32 direction;
		switch (mode) {
		case EVT_ATOM_DIR:
			direction = (s16)CEvt_Evaluate(this);
			break;
		case EVT_ATOM_POS: {
			f32 x = CEvt_Evaluate(this) / 100.0f;
			f32 z = CEvt_Evaluate(this) / 100.0f;
			if (obj != NULL && (obj->mStatus.mFlags & 1)) {
				direction = RAD2DEG * (f32)atan2(x - obj->mMoveTranslate.x, z - obj->mMoveTranslate.z);
			} else {
				CEvt_Warn(FormatToStaticBuffer("mdir(%d, @POS, ...): obj(%d) killed!", id, id));
			}
			break;
		}
		case EVT_ATOM_OBJ: {
			s16 targetId = CEvt_Evaluate(this);
			XObj* target = CEvt_GetObj(this, targetId);
			if (obj != NULL && (obj->mStatus.mFlags & 1)) {
				if (target != NULL && (target->mStatus.mFlags & 1)) {
					direction
					    = RAD2DEG
					    * (f32)atan2(target->mMoveTranslate.x - obj->mMoveTranslate.x, target->mMoveTranslate.z - obj->mMoveTranslate.z);
				} else {
					CEvt_Warn(FormatToStaticBuffer("mdir(%d, @OBJ, %d, ...): obj(%d) killed!", id, targetId, targetId));
					direction = RAD2DEG * obj->mMoveRotate.y;
				}
			} else {
				CEvt_Warn(FormatToStaticBuffer("mdir(%d, @OBJ, ...): obj(%d) killed!", id, id));
			}
			break;
		}
		case EVT_ATOM_CAM:
			if (obj != NULL && (obj->mStatus.mFlags & 1)) {
				CCamera* camera = &gGame.mMotionControlCamera.mCamera;
				direction       = RAD2DEG * (f32)atan2(camera->mEye.x - obj->mMoveTranslate.x, camera->mEye.z - obj->mMoveTranslate.z);
			} else {
				CEvt_Warn(FormatToStaticBuffer("mdir(%d, @CAM, ...): obj(%d) killed!", id, id));
			}
			break;
		default:
#if defined(VERSION_PAL)
			const s32 line = 2671;
#else
			const s32 line = 2667;
#endif
			OSPanic("evt.cpp", line, "%s%s", FormatToStaticBuffer("mdir(obj, %d,...) - unresolved!\n", mode), "_cmd_DIR");
			break;
		}
		f32 time = CEvt_Evaluate(this) / 100.0f;
		s32 anim = CEvt_Evaluate(this);
		if (obj != NULL && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
			if (obj->mJObj == NULL) {
				CEvt_Warn(FormatToStaticBuffer("mdir(%d, %d, %d, %d, %d) ... obj(%d) is collision!", id, mode, (s16)direction,
				                               (s32)(100.0f * time), anim, id));
			}
			f32 yaw        = DEG2RAD * (s16)direction;
			f32 difference = obj->mMoveRotate.y - yaw;
			while (difference < 0.0f)
				difference += TWO_PI;
			while (difference >= TWO_PI)
				difference -= TWO_PI;
			if (difference < 0.0017453292f || difference > 6.2814398f) {
				obj->SetRotationY(yaw, 0);
			} else {
				obj->SetRotateY(yaw, time);
				f32 angle = obj->mMoveRotate.mTarget.y - obj->mMoveRotate.y;
				if (angle > PI)
					angle = TWO_PI - angle;
				obj->mMoveRotate.mSpeed.x = 0.0f;
				if (angle < 0.0f)
					angle = -angle;
				obj->mMoveRotate.mSpeed.y = angle / (FRAMES_PER_SEC * time);
				obj->mMoveRotate.mSpeed.z = 0.0f;
				if (anim != -1) {
					if (anim & 0xFFFF0000) {
						s32 jointId     = anim & 0xFFFF;
						s32 matId       = anim >> 16;
						void* jointAnim = obj->GetJointAnimation(jointId);
						void* matAnim   = obj->GetMaterialAnimation(matId);
						void* shapeAnim = obj->GetShapeAnimation(jointId);
						obj->SetAnim(0, TRUE, jointAnim, matAnim, shapeAnim, 0.0f, 1.0f, 0.0f, 10.0f);
						if (jointId >= 1000 || matId >= 1000)
							obj->mStatus.mFlags |= 0x0008000000000000;
					} else {
						obj->SetAnim(anim, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
					}
					obj->UpdateAnim(1);
				}
			}
		}
	}
	return 0;
}
/*
    stwu     r1, -0x50(r1)
    mflr     r0
    stw      r0, 0x54(r1)
    stfd     f31, 0x40(r1)
    psq_st   f31, 0x48(r1), 0, qr0
    stmw     r25, 0x24(r1)
    lis      r5, lbl_80207FA0@ha
    mr       r25, r3
    mr       r26, r4
    addi     r31, r5, lbl_80207FA0@l
    bl       CEvt_Evaluate
    extsh    r28, r3
    cmpwi    r28, 0x2710
    blt      .L_800766EC
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_800766D4
.L_80076654:
    lha      r0, 0x4(r3)
    cmpw     r0, r28
    bne      .L_800766C8
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r30, -0x67a0(r3)
    cmplwi   r30, 0x0
    beq      .L_800766C0
    lwz      r0, 0x228(r30)
    li       r4, 0x0
    lwz      r5, 0x22c(r30)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_800766C0
    b        .L_800766D8
.L_800766C0:
    li       r30, 0x0
    b        .L_800766D8
.L_800766C8:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_80076654
.L_800766D4:
    li       r30, 0x0
.L_800766D8:
    cmplwi   r30, 0x0
    bne      .L_80076724
    li       r30, 0x0
    b        .L_80076724
    b        .L_80076724
.L_800766EC:
    extsh.   r0, r28
    blt      .L_80076710
    lis      r3, gGame@ha
    slwi     r0, r28, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r30, -0x67a0(r3)
    b        .L_80076724
.L_80076710:
    cmpwi    r28, -0x1
    bne      .L_80076720
    lwz      r30, 0x8(r25)
    b        .L_80076724
.L_80076720:
    mr       r30, r28
.L_80076724:
    cmplwi   r30, 0x0
    beq      .L_80076774
    lwz      r0, 0x228(r30)
    li       r4, 0x0
    lwz      r5, 0x22c(r30)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_80076774
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    cmplw    r30, r3
    bne      .L_80076774
    addi     r3, r3, 0xbe8
    bl       CancelPlayerPushAction
.L_80076774:
    clrlwi   r0, r26, 24
    cmplwi   r0, 0x1b
    bne      .L_80076868
    mr       r3, r25
    bl       CEvt_Evaluate
    cmplwi   r30, 0x0
    extsh    r6, r3
    beq      .L_800771EC
    lwz      r0, 0x228(r30)
    li       r4, 0x0
    lwz      r5, 0x22c(r30)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_800771EC
    xoris    r3, r6, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc(r1)
    mr       r3, r30
    lwz      r12, 0x34(r30)
    li       r4, 0x0
    stw      r0, 0x8(r1)
    lfd      f1, lbl_8065CF88@sda21(r0)
    lfd      f0, 0x8(r1)
    lfs      f2, lbl_8065CFE8@sda21(r0)
    fsubs    f0, f0, f1
    lwz      r12, 0x28(r12)
    fmuls    f1, f2, f0
    mtctr    r12
    bctrl
    lis      r3, gGame@ha
    mr       r4, r30
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x5d38
    bl       fn_80148D6C
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r25, -0x6944(r3)
    cmplw    r30, r25
    bne      .L_800771EC
    mr       r3, r25
    bl       ResetPlayerHeadOffset
    addi     r25, r25, 0xa44
    lwz      r0, 0x20(r25)
    cmplwi   r0, 0x0
    beq      .L_800771EC
    mr       r3, r25
    li       r4, 0x1
    bl       fn_800CECFC
    lwz      r3, 0x20(r25)
    li       r4, 0x1
    lwz      r12, 0x34(r3)
    lwz      r12, 0xa4(r12)
    mtctr    r12
    bctrl
    b        .L_800771EC
.L_80076868:
    mr       r3, r25
    bl       CEvt_Evaluate
    clrlwi   r0, r3, 16
    cmpwi    r0, 0xd0
    mr       r29, r0
    beq      .L_80076A20
    bge      .L_8007689C
    cmpwi    r0, 0xce
    beq      .L_80076E48
    bge      .L_800768B8
    cmpwi    r0, 0xcd
    bge      .L_800768A8
    b        .L_80076E48
.L_8007689C:
    cmpwi    r0, 0xe1
    beq      .L_80076D24
    b        .L_80076E48
.L_800768A8:
    mr       r3, r25
    bl       CEvt_Evaluate
    extsh    r27, r3
    b        .L_80076E78
.L_800768B8:
    mr       r3, r25
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc(r1)
    mr       r3, r25
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x8(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x8(r1)
    fsubs    f1, f1, f2
    fdivs    f31, f1, f0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x14(r1)
    cmplwi   r30, 0x0
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x10(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x10(r1)
    fsubs    f1, f1, f2
    fdivs    f2, f1, f0
    beq      .L_80076970
    lwz      r0, 0x228(r30)
    li       r4, 0x0
    lwz      r5, 0x22c(r30)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_80076970
    lfs      f1, 0xc8(r30)
    lfs      f0, 0xd0(r30)
    fsubs    f1, f31, f1
    fsubs    f2, f2, f0
    bl       atan2
    frsp     f1, f1
    lfs      f0, lbl_8065CFF0@sda21(r0)
    fmuls    f0, f0, f1
    fctiwz   f0, f0
    stfd     f0, 0x10(r1)
    lwz      r27, 0x14(r1)
    b        .L_80076E78
.L_80076970:
    mr       r4, r28
    mr       r5, r28
    addi     r3, r31, 0x788
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_800769D4
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800769C0
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800769C0
.L_800769B8:
    addi     r3, r3, 0x8
    bdnz     .L_800769B8
.L_800769C0:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800769D4
.L_800769D0:
    bdnz     .L_800769D0
.L_800769D4:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80076E78
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80076A08
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80076A08
.L_80076A00:
    addi     r3, r3, 0x8
    bdnz     .L_80076A00
.L_80076A08:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80076E78
.L_80076A18:
    bdnz     .L_80076A18
    b        .L_80076E78
.L_80076A20:
    mr       r3, r25
    bl       CEvt_Evaluate
    extsh    r5, r3
    cmpwi    r5, 0x2710
    blt      .L_80076AE4
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r6, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80076ACC
.L_80076A4C:
    lha      r0, 0x4(r3)
    cmpw     r0, r5
    bne      .L_80076AC0
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r6, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r3, -0x67a0(r3)
    cmplwi   r3, 0x0
    beq      .L_80076AB8
    lwz      r0, 0x228(r3)
    li       r6, 0x0
    lwz      r7, 0x22c(r3)
    li       r4, 0x1
    and      r0, r0, r6
    and      r4, r7, r4
    xor      r4, r4, r6
    xor      r0, r0, r6
    or.      r0, r4, r0
    beq      .L_80076AB8
    b        .L_80076AD0
.L_80076AB8:
    li       r3, 0x0
    b        .L_80076AD0
.L_80076AC0:
    addi     r3, r3, 0x4
    addi     r6, r6, 0x1
    bdnz     .L_80076A4C
.L_80076ACC:
    li       r3, 0x0
.L_80076AD0:
    cmplwi   r3, 0x0
    bne      .L_80076B1C
    li       r3, 0x0
    b        .L_80076B1C
    b        .L_80076B1C
.L_80076AE4:
    extsh.   r0, r5
    blt      .L_80076B08
    lis      r3, gGame@ha
    slwi     r0, r5, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r3, -0x67a0(r3)
    b        .L_80076B1C
.L_80076B08:
    cmpwi    r5, -0x1
    bne      .L_80076B18
    lwz      r3, 0x8(r25)
    b        .L_80076B1C
.L_80076B18:
    mr       r3, r5
.L_80076B1C:
    cmplwi   r30, 0x0
    beq      .L_80076C74
    lwz      r0, 0x228(r30)
    li       r6, 0x0
    lwz      r4, 0x22c(r30)
    li       r7, 0x1
    and      r0, r0, r6
    and      r4, r4, r7
    xor      r4, r4, r6
    xor      r0, r0, r6
    or.      r0, r4, r0
    beq      .L_80076C74
    cmplwi   r3, 0x0
    beq      .L_80076BAC
    lwz      r0, 0x228(r3)
    lwz      r4, 0x22c(r3)
    and      r0, r0, r6
    and      r4, r4, r7
    xor      r4, r4, r6
    xor      r0, r0, r6
    or.      r0, r4, r0
    beq      .L_80076BAC
    lfs      f3, 0xc8(r3)
    lfs      f1, 0xc8(r30)
    lfs      f2, 0xd0(r3)
    lfs      f0, 0xd0(r30)
    fsubs    f1, f3, f1
    fsubs    f2, f2, f0
    bl       atan2
    frsp     f1, f1
    lfs      f0, lbl_8065CFF0@sda21(r0)
    fmuls    f0, f0, f1
    fctiwz   f0, f0
    stfd     f0, 0x10(r1)
    lwz      r27, 0x14(r1)
    b        .L_80076E78
.L_80076BAC:
    mr       r4, r28
    mr       r6, r5
    addi     r3, r31, 0x7b0
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80076C10
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80076BFC
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80076BFC
.L_80076BF4:
    addi     r3, r3, 0x8
    bdnz     .L_80076BF4
.L_80076BFC:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80076C10
.L_80076C0C:
    bdnz     .L_80076C0C
.L_80076C10:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80076C58
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80076C44
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80076C44
.L_80076C3C:
    addi     r3, r3, 0x8
    bdnz     .L_80076C3C
.L_80076C44:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80076C58
.L_80076C54:
    bdnz     .L_80076C54
.L_80076C58:
    lfs      f1, lbl_8065CFF0@sda21(r0)
    lfs      f0, 0xfc(r30)
    fmuls    f0, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x10(r1)
    lwz      r27, 0x14(r1)
    b        .L_80076E78
.L_80076C74:
    mr       r4, r28
    mr       r5, r28
    addi     r3, r31, 0x7dc
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80076CD8
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80076CC4
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80076CC4
.L_80076CBC:
    addi     r3, r3, 0x8
    bdnz     .L_80076CBC
.L_80076CC4:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80076CD8
.L_80076CD4:
    bdnz     .L_80076CD4
.L_80076CD8:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80076E78
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80076D0C
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80076D0C
.L_80076D04:
    addi     r3, r3, 0x8
    bdnz     .L_80076D04
.L_80076D0C:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80076E78
.L_80076D1C:
    bdnz     .L_80076D1C
    b        .L_80076E78
.L_80076D24:
    cmplwi   r30, 0x0
    beq      .L_80076D98
    lwz      r0, 0x228(r30)
    li       r4, 0x0
    lwz      r5, 0x22c(r30)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_80076D98
    lis      r3, gGame@ha
    lfs      f1, 0xc8(r30)
    addi     r3, r3, gGame@l
    lfs      f0, 0xd0(r30)
    addis    r3, r3, 0xe
    lfs      f3, 0x5dcc(r3)
    lfs      f2, 0x5dd4(r3)
    fsubs    f1, f3, f1
    fsubs    f2, f2, f0
    bl       atan2
    frsp     f1, f1
    lfs      f0, lbl_8065CFF0@sda21(r0)
    fmuls    f0, f0, f1
    fctiwz   f0, f0
    stfd     f0, 0x10(r1)
    lwz      r27, 0x14(r1)
    b        .L_80076E78
.L_80076D98:
    mr       r4, r28
    mr       r5, r28
    addi     r3, r31, 0x804
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80076DFC
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80076DE8
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80076DE8
.L_80076DE0:
    addi     r3, r3, 0x8
    bdnz     .L_80076DE0
.L_80076DE8:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80076DFC
.L_80076DF8:
    bdnz     .L_80076DF8
.L_80076DFC:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80076E78
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80076E30
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80076E30
.L_80076E28:
    addi     r3, r3, 0x8
    bdnz     .L_80076E28
.L_80076E30:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80076E78
.L_80076E40:
    bdnz     .L_80076E40
    b        .L_80076E78
.L_80076E48:
    mr       r4, r29
    addi     r3, r31, 0x82c
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvtCommandDIRName@ha
    mr       r6, r3
    addi     r7, r4, sEvtCommandDIRName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0xa6b
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
.L_80076E78:
    mr       r3, r25
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x14(r1)
    mr       r3, r25
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x10(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x10(r1)
    fsubs    f1, f1, f2
    fdivs    f31, f1, f0
    bl       CEvt_Evaluate
    cmplwi   r30, 0x0
    mr       r26, r3
    beq      .L_800771EC
    lwz      r3, 0x228(r30)
    li       r5, 0x0
    lwz      r6, 0x22c(r30)
    li       r0, 0x1
    and      r4, r3, r5
    and      r0, r6, r0
    xor      r3, r0, r5
    xor      r0, r4, r5
    or.      r0, r3, r0
    beq      .L_800771EC
    lis      r3, 0x1
    xor      r0, r4, r5
    and      r3, r6, r3
    xor      r3, r3, r5
    or.      r0, r3, r0
    beq      .L_800771EC
    lwz      r0, 0x0(r30)
    cmplwi   r0, 0x0
    bne      .L_80076FD0
    lfs      f0, lbl_8065CF80@sda21(r0)
    mr       r4, r28
    mr       r5, r29
    mr       r8, r26
    fmuls    f0, f0, f31
    mr       r9, r28
    addi     r3, r31, 0x850
    extsh    r6, r27
    fctiwz   f0, f0
    stfd     f0, 0x10(r1)
    lwz      r7, 0x14(r1)
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80076F88
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80076F74
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80076F74
.L_80076F6C:
    addi     r3, r3, 0x8
    bdnz     .L_80076F6C
.L_80076F74:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80076F88
.L_80076F84:
    bdnz     .L_80076F84
.L_80076F88:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80076FD0
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80076FBC
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80076FBC
.L_80076FB4:
    addi     r3, r3, 0x8
    bdnz     .L_80076FB4
.L_80076FBC:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80076FD0
.L_80076FCC:
    bdnz     .L_80076FCC
.L_80076FD0:
    extsh    r3, r27
    lis      r0, 0x4330
    xoris    r3, r3, 0x8000
    stw      r0, 0x10(r1)
    lfd      f1, lbl_8065CF88@sda21(r0)
    stw      r3, 0x14(r1)
    lfs      f3, lbl_8065CFE8@sda21(r0)
    lfd      f0, 0x10(r1)
    lfs      f4, 0xfc(r30)
    fsubs    f1, f0, f1
    lfs      f2, lbl_8065CFF4@sda21(r0)
    lfs      f0, lbl_8065CF74@sda21(r0)
    fmuls    f1, f3, f1
    fsubs    f3, f4, f1
    b        .L_80077010
.L_8007700C:
    fadds    f3, f3, f2
.L_80077010:
    fcmpo    cr0, f3, f0
    blt      .L_8007700C
    lfs      f0, lbl_8065CFF4@sda21(r0)
    b        .L_80077024
.L_80077020:
    fsubs    f3, f3, f0
.L_80077024:
    fcmpo    cr0, f3, f0
    cror     eq, gt, eq
    beq      .L_80077020
    lfs      f0, lbl_8065CFF8@sda21(r0)
    fcmpo    cr0, f3, f0
    blt      .L_80077048
    lfs      f0, lbl_8065CFFC@sda21(r0)
    fcmpo    cr0, f3, f0
    ble      .L_80077064
.L_80077048:
    lwz      r12, 0x34(r30)
    mr       r3, r30
    li       r4, 0x0
    lwz      r12, 0x28(r12)
    mtctr    r12
    bctrl
    b        .L_800771EC
.L_80077064:
    lwz      r12, 0x34(r30)
    fmr      f2, f31
    mr       r3, r30
    lwz      r12, 0x7c(r12)
    mtctr    r12
    bctrl
    lfs      f2, 0x10c(r30)
    lfs      f1, 0xfc(r30)
    lfs      f0, lbl_8065D000@sda21(r0)
    fsubs    f1, f2, f1
    fcmpo    cr0, f1, f0
    ble      .L_8007709C
    lfs      f0, lbl_8065CFF4@sda21(r0)
    fsubs    f1, f0, f1
.L_8007709C:
    lfs      f0, lbl_8065CF74@sda21(r0)
    fcmpo    cr0, f1, f0
    stfs     f0, 0x118(r30)
    bge      .L_800770B4
    fneg     f2, f1
    b        .L_800770B8
.L_800770B4:
    fmr      f2, f1
.L_800770B8:
    lfs      f0, lbl_8065CFC0@sda21(r0)
    cmpwi    r26, -0x1
    lfs      f1, lbl_8065CF74@sda21(r0)
    fmuls    f0, f0, f31
    fdivs    f0, f2, f0
    stfs     f0, 0x11c(r30)
    stfs     f1, 0x120(r30)
    beq      .L_800771EC
    clrrwi.  r0, r26, 16
    beq      .L_800771A0
    mr       r3, r30
    clrlwi   r25, r26, 16
    lwz      r12, 0x34(r30)
    mr       r4, r25
    srawi    r26, r26, 16
    lwz      r12, 0xf0(r12)
    mtctr    r12
    bctrl
    mr       r0, r3
    mr       r3, r30
    lwz      r12, 0x34(r30)
    mr       r27, r0
    mr       r4, r26
    lwz      r12, 0xf4(r12)
    mtctr    r12
    bctrl
    mr       r0, r3
    mr       r3, r30
    lwz      r12, 0x34(r30)
    mr       r28, r0
    mr       r4, r25
    lwz      r12, 0xf8(r12)
    mtctr    r12
    bctrl
    lwz      r12, 0x34(r30)
    mr       r0, r3
    lfs      f1, lbl_8065CF74@sda21(r0)
    mr       r3, r30
    lwz      r12, 0x80(r12)
    mr       r8, r0
    fmr      f3, f1
    mr       r6, r27
    mr       r7, r28
    lfs      f2, lbl_8065CFE0@sda21(r0)
    li       r4, 0x0
    lfs      f4, lbl_8065CFE4@sda21(r0)
    li       r5, 0x1
    mtctr    r12
    bctrl
    cmpwi    r25, 0x3e8
    bge      .L_8007718C
    cmpwi    r26, 0x3e8
    blt      .L_800771D4
.L_8007718C:
    lwz      r3, 0x228(r30)
    lis      r0, 0x8
    or       r0, r3, r0
    stw      r0, 0x228(r30)
    b        .L_800771D4
.L_800771A0:
    lwz      r12, 0x34(r30)
    fmr      f3, f1
    mr       r3, r30
    mr       r4, r26
    lwz      r12, 0x80(r12)
    li       r5, 0x0
    lfs      f2, lbl_8065CFE0@sda21(r0)
    lfs      f4, lbl_8065CFE4@sda21(r0)
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    mtctr    r12
    bctrl
.L_800771D4:
    mr       r3, r30
    li       r4, 0x1
    lwz      r12, 0x34(r30)
    lwz      r12, 0x88(r12)
    mtctr    r12
    bctrl
.L_800771EC:
    li       r3, 0x0
    psq_l    f31, 0x48(r1), 0, qr0
    lfd      f31, 0x40(r1)
    lmw      r25, 0x24(r1)
    lwz      r0, 0x54(r1)
    mtlr     r0
    addi     r1, r1, 0x50
    blr
*/

/**
 * @note Address: 0x8007720C
 * @note Size: 0x64C
 */
s32 CEvt::_cmd_COLOR(u8, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	u16 mode  = CEvt_Evaluate(this);
	u8 r      = CEvt_Evaluate(this);
	u8 g      = CEvt_Evaluate(this);
	u8 b      = CEvt_Evaluate(this);
	f32 time  = CEvt_Evaluate(this) / 100.0f;
	if (obj == (XObj*)-2) {
		for (s32 i = 0; i < gGame.mXObjs.mObjectCount; i++) {
			XObj* current = gGame.mXObjs.mObjects[i];
			if ((current->mStatus.mFlags & 1) && current->mJObj != NULL) {
				if (mode == EVT_ATOM_MODULATE) {
					current->vf12C(0, r, g, b, time);
				} else if (mode == EVT_ATOM_BLEND) {
					current->vf12C(1, r, g, b, time);
				}
			}
		}
	} else if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		if (obj->mJObj == NULL) {
			if (mode == EVT_ATOM_MODULATE) {
				CEvt_Warn(FormatToStaticBuffer("color(%d, @MODULATE, %d,%d,%d, %d) ... obj(%d) is collision!", id, r, g, b,
				                               (s32)(100.0f * time), id));
			} else {
				CEvt_Warn(FormatToStaticBuffer("color(%d, @BLEND, %d,%d,%d, %d) ... obj(%d) is collision!", id, r, g, b,
				                               (s32)(100.0f * time), id));
			}
		}
		if (mode == EVT_ATOM_MODULATE) {
			obj->vf12C(0, r, g, b, time);
		} else if (mode == EVT_ATOM_BLEND) {
			obj->vf12C(1, r, g, b, time);
		}
	} else {
		if (mode == EVT_ATOM_MODULATE) {
			CEvt_Warn(
			    FormatToStaticBuffer("color(%d, @MODULATE, %d,%d,%d, %d) ... obj(%d) killed!", id, r, g, b, (s32)(100.0f * time), id));
		} else {
			CEvt_Warn(FormatToStaticBuffer("color(%d, @BLEND, %d,%d,%d, %d) ... obj(%d) killed!", id, r, g, b, (s32)(100.0f * time), id));
		}
	}
	return 0;
}
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    stw      r0, 0x44(r1)
    stfd     f31, 0x30(r1)
    psq_st   f31, 0x38(r1), 0, qr0
    stmw     r24, 0x10(r1)
    lis      r4, lbl_80207FA0@ha
    mr       r24, r3
    addi     r31, r4, lbl_80207FA0@l
    bl       CEvt_Evaluate
    extsh    r29, r3
    cmpwi    r29, 0x2710
    blt      .L_800772F0
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_800772D8
.L_80077258:
    lha      r0, 0x4(r3)
    cmpw     r0, r29
    bne      .L_800772CC
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r30, -0x67a0(r3)
    cmplwi   r30, 0x0
    beq      .L_800772C4
    lwz      r0, 0x228(r30)
    li       r4, 0x0
    lwz      r5, 0x22c(r30)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_800772C4
    b        .L_800772DC
.L_800772C4:
    li       r30, 0x0
    b        .L_800772DC
.L_800772CC:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_80077258
.L_800772D8:
    li       r30, 0x0
.L_800772DC:
    cmplwi   r30, 0x0
    bne      .L_80077328
    li       r30, 0x0
    b        .L_80077328
    b        .L_80077328
.L_800772F0:
    extsh.   r0, r29
    blt      .L_80077314
    lis      r3, gGame@ha
    slwi     r0, r29, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r30, -0x67a0(r3)
    b        .L_80077328
.L_80077314:
    cmpwi    r29, -0x1
    bne      .L_80077324
    lwz      r30, 0x8(r24)
    b        .L_80077328
.L_80077324:
    mr       r30, r29
.L_80077328:
    mr       r3, r24
    bl       CEvt_Evaluate
    clrlwi   r28, r3, 16
    mr       r3, r24
    bl       CEvt_Evaluate
    clrlwi   r27, r3, 24
    mr       r3, r24
    bl       CEvt_Evaluate
    clrlwi   r26, r3, 24
    mr       r3, r24
    bl       CEvt_Evaluate
    clrlwi   r25, r3, 24
    mr       r3, r24
    bl       CEvt_Evaluate
    xoris    r0, r3, 0x8000
    lis      r3, 0x4330
    stw      r0, 0xc(r1)
    addis    r0, r30, 0x1
    lfd      f2, lbl_8065CF88@sda21(r0)
    cmplwi   r0, 0xfffe
    stw      r3, 0x8(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x8(r1)
    fsubs    f1, f1, f2
    fdivs    f31, f1, f0
    bne      .L_80077454
    lis      r3, gGame@ha
    li       r29, 0x0
    addi     r30, r3, gGame@l
    addis    r31, r30, 0x1
    b        .L_80077444
.L_800773A4:
    addis    r3, r30, 0x1
    li       r4, 0x1
    lwz      r3, -0x67a0(r3)
    li       r5, 0x0
    lwz      r0, 0x228(r3)
    lwz      r6, 0x22c(r3)
    and      r0, r0, r5
    and      r4, r6, r4
    xor      r4, r4, r5
    xor      r0, r0, r5
    or.      r0, r4, r0
    beq      .L_8007743C
    lwz      r0, 0x0(r3)
    cmplwi   r0, 0x0
    beq      .L_8007743C
    cmplwi   r28, 0xde
    bne      .L_80077410
    lwz      r12, 0x34(r3)
    fmr      f1, f31
    mr       r5, r27
    mr       r6, r26
    lwz      r12, 0x12c(r12)
    mr       r7, r25
    li       r4, 0x0
    mtctr    r12
    bctrl
    b        .L_8007743C
.L_80077410:
    cmplwi   r28, 0xdf
    bne      .L_8007743C
    lwz      r12, 0x34(r3)
    fmr      f1, f31
    mr       r5, r27
    mr       r6, r26
    lwz      r12, 0x12c(r12)
    mr       r7, r25
    li       r4, 0x1
    mtctr    r12
    bctrl
.L_8007743C:
    addi     r30, r30, 0x4
    addi     r29, r29, 0x1
.L_80077444:
    lwz      r0, -0x57a0(r31)
    cmpw     r29, r0
    blt      .L_800773A4
    b        .L_80077838
.L_80077454:
    cmplwi   r30, 0x0
    beq      .L_80077694
    lwz      r0, 0x228(r30)
    li       r4, 0x0
    lwz      r5, 0x22c(r30)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_80077694
    lwz      r0, 0x0(r30)
    cmplwi   r0, 0x0
    bne      .L_8007762C
    cmplwi   r28, 0xde
    bne      .L_80077564
    fmuls    f0, f0, f31
    mr       r4, r29
    mr       r5, r27
    mr       r6, r26
    mr       r7, r25
    mr       r9, r29
    fctiwz   f0, f0
    addi     r3, r31, 0x884
    stfd     f0, 0x8(r1)
    lwz      r8, 0xc(r1)
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80077518
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80077504
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80077504
.L_800774FC:
    addi     r3, r3, 0x8
    bdnz     .L_800774FC
.L_80077504:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80077518
.L_80077514:
    bdnz     .L_80077514
.L_80077518:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_8007762C
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007754C
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007754C
.L_80077544:
    addi     r3, r3, 0x8
    bdnz     .L_80077544
.L_8007754C:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007762C
.L_8007755C:
    bdnz     .L_8007755C
    b        .L_8007762C
.L_80077564:
    fmuls    f0, f0, f31
    mr       r4, r29
    mr       r5, r27
    mr       r6, r26
    mr       r7, r25
    mr       r9, r29
    fctiwz   f0, f0
    addi     r3, r31, 0x8c4
    stfd     f0, 0x8(r1)
    lwz      r8, 0xc(r1)
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_800775E4
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800775D0
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800775D0
.L_800775C8:
    addi     r3, r3, 0x8
    bdnz     .L_800775C8
.L_800775D0:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800775E4
.L_800775E0:
    bdnz     .L_800775E0
.L_800775E4:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_8007762C
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80077618
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80077618
.L_80077610:
    addi     r3, r3, 0x8
    bdnz     .L_80077610
.L_80077618:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007762C
.L_80077628:
    bdnz     .L_80077628
.L_8007762C:
    cmplwi   r28, 0xde
    bne      .L_80077660
    mr       r3, r30
    fmr      f1, f31
    lwz      r12, 0x34(r30)
    mr       r5, r27
    mr       r6, r26
    mr       r7, r25
    lwz      r12, 0x12c(r12)
    li       r4, 0x0
    mtctr    r12
    bctrl
    b        .L_80077838
.L_80077660:
    cmplwi   r28, 0xdf
    bne      .L_80077838
    mr       r3, r30
    fmr      f1, f31
    lwz      r12, 0x34(r30)
    mr       r5, r27
    mr       r6, r26
    mr       r7, r25
    lwz      r12, 0x12c(r12)
    li       r4, 0x1
    mtctr    r12
    bctrl
    b        .L_80077838
.L_80077694:
    cmplwi   r28, 0xde
    bne      .L_8007776C
    lfs      f0, lbl_8065CF80@sda21(r0)
    mr       r4, r29
    mr       r5, r27
    mr       r6, r26
    fmuls    f0, f0, f31
    mr       r7, r25
    mr       r9, r29
    addi     r3, r31, 0x900
    fctiwz   f0, f0
    stfd     f0, 0x8(r1)
    lwz      r8, 0xc(r1)
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80077720
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007770C
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007770C
.L_80077704:
    addi     r3, r3, 0x8
    bdnz     .L_80077704
.L_8007770C:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80077720
.L_8007771C:
    bdnz     .L_8007771C
.L_80077720:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80077838
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80077754
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80077754
.L_8007774C:
    addi     r3, r3, 0x8
    bdnz     .L_8007774C
.L_80077754:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80077838
.L_80077764:
    bdnz     .L_80077764
    b        .L_80077838
.L_8007776C:
    lfs      f0, lbl_8065CF80@sda21(r0)
    mr       r4, r29
    mr       r5, r27
    mr       r6, r26
    fmuls    f0, f0, f31
    mr       r7, r25
    mr       r9, r29
    addi     r3, r31, 0x938
    fctiwz   f0, f0
    stfd     f0, 0x8(r1)
    lwz      r8, 0xc(r1)
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_800777F0
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_800777DC
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_800777DC
.L_800777D4:
    addi     r3, r3, 0x8
    bdnz     .L_800777D4
.L_800777DC:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_800777F0
.L_800777EC:
    bdnz     .L_800777EC
.L_800777F0:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_80077838
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80077824
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80077824
.L_8007781C:
    addi     r3, r3, 0x8
    bdnz     .L_8007781C
.L_80077824:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80077838
.L_80077834:
    bdnz     .L_80077834
.L_80077838:
    li       r3, 0x0
    psq_l    f31, 0x38(r1), 0, qr0
    lfd      f31, 0x30(r1)
    lmw      r24, 0x10(r1)
    lwz      r0, 0x44(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

extern "C" void SetStageCameraAnimation(XObj* obj, s32 anim, f32 frame);

#define EVT_CAMERA_ACTION(type) (((CInterfaceCameraAction**)&gGame.mCameraActions.mTable)[type])

/**
 * @note Address: 0x80077858
 * @note Size: 0xEFC
 */
s32 CEvt::_cmd_CAMERA(u8, s32)
{
	CCameraActionEvt* action   = (CCameraActionEvt*)EVT_CAMERA_ACTION(CAMACTION_EVT);
	CCameraActionAreaExM* area = (CCameraActionAreaExM*)EVT_CAMERA_ACTION(CAMACTION_AREA);
	CCameraActionReset* reset  = (CCameraActionReset*)EVT_CAMERA_ACTION(CAMACTION_RESET);
	u16 command                = CEvt_Evaluate(this);
	switch (command) {
	case 211: { // area camera
		s32 id        = CEvt_Evaluate(this);
		s32 direction = CEvt_Evaluate(this);
		s32 mode      = CEvt_Evaluate(this);
		f32 time      = CEvt_Evaluate(this) / 100.0f;
		if (GetStageCameraInfoRecords() != NULL && area->vf78(id) != 0) {
			CMotionControlCamera* camera = &gGame.mMotionControlCamera;
			camera->ChangeAction(CAMACTION_AREA);
			SelectAreaCameraEntryWithControl((CCameraActionArea*)area, id, direction, mode, 0, time);
			if (mode == 3) {
				camera->ApplyAction();
			}
		}
		break;
	}
	case EVT_ATOM_POS: {
		f32 x    = CEvt_Evaluate(this) / 100.0f;
		f32 y    = CEvt_Evaluate(this) / 100.0f;
		f32 z    = CEvt_Evaluate(this) / 100.0f;
		s32 mode = CEvt_Evaluate(this);
		f32 time = CEvt_Evaluate(this) / 100.0f;
		gGame.mMotionControlCamera.ChangeAction(CAMACTION_EVT);
		action->_38 = 11;
		Vec eye     = { 0.0f, 0.0f, 0.0f };
		eye.x       = x;
		eye.y       = y;
		eye.z       = z;
		Vec target  = eye;
		action->SetEye_Interp(mode, target, time);
		break;
	}
	case 240:
	case 243:
	case 251:
	case 252: {
		s32 axis      = CEvt_Evaluate(this);
		f32 degrees   = CEvt_Evaluate(this);
		s32 mode      = CEvt_Evaluate(this);
		f32 time      = CEvt_Evaluate(this) / 100.0f;
		u32 type      = command == 240 ? 2 : 1;
		BOOL relative = (u16)(command - 251) <= 1 ? true : false;
		gGame.mMotionControlCamera.ChangeAction(CAMACTION_EVT);
		switch (axis) {
		case 2: {
			Vec angle  = { 0.0f, 0.0f, 0.0f };
			angle.y    = DEG2RAD * degrees;
			Vec target = angle;
			action->SetAngle_Interp(mode, target, 2, type, relative, time);
			break;
		}
		case 3: {
			Vec angle  = { 0.0f, 0.0f, 0.0f };
			angle.x    = DEG2RAD * degrees;
			Vec target = angle;
			action->SetAngle_Interp(mode, target, 1, type, relative, time);
			break;
		}
		case 5: {
			Vec angle  = { 0.0f, 0.0f, 0.0f };
			angle.z    = DEG2RAD * degrees;
			Vec target = angle;
			action->SetAngle_Interp(mode, target, 4, type, relative, time);
			break;
		}
		}
		break;
	}
	case 227: { // interest position
		f32 x    = CEvt_Evaluate(this) / 100.0f;
		f32 y    = CEvt_Evaluate(this) / 100.0f;
		f32 z    = CEvt_Evaluate(this) / 100.0f;
		s32 mode = CEvt_Evaluate(this);
		f32 time = CEvt_Evaluate(this) / 100.0f;
		gGame.mMotionControlCamera.ChangeAction(CAMACTION_EVT);
		action->_38  = 11;
		Vec interest = { 0.0f, 0.0f, 0.0f };
		interest.x   = x;
		interest.y   = y;
		interest.z   = z;
		Vec target   = interest;
		action->SetInterest_Interp(mode, target, time);
		break;
	}
	case EVT_ATOM_OBJ: {
		s32 id    = CEvt_Evaluate(this);
		s32 mode  = CEvt_Evaluate(this);
		f32 time  = CEvt_Evaluate(this) / 100.0f;
		XObj* obj = CEvt_GetObj(this, (s16)id);
		if (obj != NULL) {
			Vec target = obj->GetCameraTarget();
			gGame.mMotionControlCamera.ChangeAction(CAMACTION_EVT);
			action->_38  = 11;
			Vec interest = target;
			action->SetInterest_Interp(mode, interest, time);
		} else {
			CEvt_Warn(FormatToStaticBuffer("cam(@OBJ, %d, ...): obj(%d) killed!", id, id));
		}
		break;
	}
	case EVT_ATOM_LEAD: {
		s32 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, (s16)id);
		if (obj != NULL) {
			gGame.mMotionControlCamera.ChangeAction(CAMACTION_EVT);
			action->_38 = 11;
			action->SetInterest_Interp(3, obj, 0.0f);
		} else {
			CEvt_Warn(FormatToStaticBuffer("cam(@LEAD, %d): obj(%d) killed!", id, id));
		}
		break;
	}
	case EVT_ATOM_DISTANCE:
		CEvt_Evaluate(this);
		CEvt_Evaluate(this);
		CEvt_Evaluate(this);
		break;
	case 229: { // field of view
		f32 fov  = CEvt_Evaluate(this);
		s32 mode = CEvt_Evaluate(this);
		f32 time = CEvt_Evaluate(this) / 100.0f;
		gGame.mMotionControlCamera.ChangeAction(CAMACTION_EVT);
		action->_38 = action->GetCameraFlags() | 8;
		action->SetFov_Interp(mode, fov, time);
		break;
	}
	case EVT_ATOM_ANIM: {
		s32 anim = CEvt_Evaluate(this);
		CEvt_Evaluate(this);
		CEvt_Evaluate(this);
		gGame.mMotionControlCamera.ChangeAction(CAMACTION_ANIM);
		XObj* obj = (XObj*)gGame.mXObjs.mPools[0].mObjects;
		if (obj != NULL && (obj->mStatus.mFlags & 1) != 0) {
			SetStageCameraAnimation(obj, anim, 0.0f);
		}
		break;
	}
	case EVT_ATOM_RESET: {
		s32 mode                     = CEvt_Evaluate(this);
		f32 time                     = CEvt_Evaluate(this) / 100.0f;
		CMotionControlCamera* camera = &gGame.mMotionControlCamera;
		if (camera->IsAction(CAMACTION_EVT)) {
			reset->mMode = mode;
			reset->mTime = time;
			reset->_4C   = FALSE;
			camera->ChangeAction(CAMACTION_RESET);
		}
		break;
	}
	case 230: { // finish the active scripted camera
		CMotionControlCamera* camera = &gGame.mMotionControlCamera;
		if (camera->IsAction(CAMACTION_AREA)) {
			ResumeAreaCameraControl((CCameraActionArea*)EVT_CAMERA_ACTION(CAMACTION_AREA));
		} else if (camera->IsAction(CAMACTION_EVT)) {
			reset->mMode = 0;
			reset->mTime = 0.0f;
			reset->_4C   = TRUE;
			camera->ChangeAction(CAMACTION_RESET);
		}
		break;
	}
	case 236: { // save camera
		CMotionControlCamera* camera = &gGame.mMotionControlCamera;
		camera->EndEye();
		s32 slot = CEvt_Evaluate(this);
		if (slot >= 0) {
			camera->CreateMemento(&gEventCameraMementos[slot]);
		} else {
			mCameraMemento = camera->CreateMemento(NULL);
		}
		break;
	}
	case 237: { // restore camera
		s32 slot = CEvt_Evaluate(this);
		if (slot >= 0) {
			gGame.mMotionControlCamera.SetMemento(&gEventCameraMementos[slot]);
		} else if (mCameraMemento != NULL) {
			gGame.mMotionControlCamera.SetMemento(mCameraMemento);
			mCameraMemento = NULL;
		}
		break;
	}
	case 238: {
		s32 mode = CEvt_Evaluate(this);
		if (mode <= 0) {
			s32 sign = mode >> 31;
			SetAreaCameraHeightIndex(&gGame.mMotionControlCamera, (mode ^ sign) - sign);
		}
		break;
	}
	case 232: // first-person controls
		switch (CEvt_Evaluate(this)) {
		case -1:
			gGame.mStatus.mFlags &= ~0x80ULL;
			break;
		case -2:
			gGame.mStatus.mFlags |= 0x80ULL;
			break;
		case 0:
			gGame.mMotionControlCamera.EndEye();
			break;
		case 1:
			gGame.mMotionControlCamera.StartEye();
			break;
		case 2: {
			f32 degrees                                                        = CEvt_Evaluate(this);
			((CCameraActionEye*)EVT_CAMERA_ACTION(CAMACTION_EYE))->mOverride.y = DEG2RAD * degrees;
			break;
		}
		case 3: {
			f32 degrees                                                        = CEvt_Evaluate(this);
			((CCameraActionEye*)EVT_CAMERA_ACTION(CAMACTION_EYE))->mOverride.x = DEG2RAD * degrees;
			break;
		}
		case 4: {
			f32 fov                                                            = CEvt_Evaluate(this);
			((CCameraActionEye*)EVT_CAMERA_ACTION(CAMACTION_EYE))->mOverride.z = fov;
			break;
		}
		}
		break;
	default:
#if defined(VERSION_PAL)
		OSPanic("evt.cpp", 2515, "%s%s", FormatToStaticBuffer("cam(%d,...) - unresolved!\n", command), "_cmd_CAMERA");
#else
		OSPanic("evt.cpp", 2511, "%s%s", FormatToStaticBuffer("cam(%d,...) - unresolved!\n", command), "_cmd_CAMERA");
#endif
		break;
	}
	return 0;
}

// CEvt::_cmd_CAMERA
/*
    stwu     r1, -0x130(r1)
    mflr     r0
    stw      r0, 0x134(r1)
    stfd     f31, 0x120(r1)
    psq_st   f31, 0x128(r1), 0, qr0
    stfd     f30, 0x110(r1)
    psq_st   f30, 0x118(r1), 0, qr0
    stfd     f29, 0x100(r1)
    psq_st   f29, 0x108(r1), 0, qr0
    stfd     f28, 0xf0(r1)
    psq_st   f28, 0xf8(r1), 0, qr0
    stmw     r25, 0xd4(r1)
    lis      r4, gGame@ha
    lis      r5, lbl_80207FA0@ha
    addi     r4, r4, gGame@l
    mr       r30, r3
    addis    r25, r4, 0xe
    addi     r28, r5, lbl_80207FA0@l
    lwz      r31, 0x7df8(r25)
    lwz      r29, 0x7df0(r25)
    lwz      r27, 0x7e10(r25)
    bl       CEvt_Evaluate
    clrlwi   r26, r3, 16
    subi     r0, r26, 0xcc
    cmplwi   r0, 0x30
    mr       r4, r26
    bgt      .L_800786F0
    lis      r3, jumptable_802251EC@ha
    slwi     r0, r0, 2
    addi     r3, r3, jumptable_802251EC@l
    lwzx     r0, r3, r0
    mtctr    r0
    bctr
.L_800778DC:
    mr       r3, r30
    bl       CEvt_Evaluate
    mr       r0, r3
    mr       r3, r30
    mr       r26, r0
    bl       CEvt_Evaluate
    mr       r0, r3
    mr       r3, r30
    mr       r27, r0
    bl       CEvt_Evaluate
    mr       r0, r3
    mr       r3, r30
    mr       r28, r0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xac(r1)
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xa8(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xa8(r1)
    fsubs    f1, f1, f2
    fdivs    f31, f1, f0
    bl       GetStageCameraInfoRecords
    cmplwi   r3, 0x0
    beq      .L_8007871C
    mr       r3, r29
    mr       r4, r26
    lwz      r12, 0x3c(r29)
    lwz      r12, 0x78(r12)
    mtctr    r12
    bctrl
    cmpwi    r3, 0x0
    beq      .L_8007871C
    lis      r3, gGame@ha
    li       r4, 0x1
    addi     r3, r3, gGame@l
    addis    r25, r3, 0xe
    addi     r25, r25, 0x5d38
    mr       r3, r25
    bl       fn_8014946C
    fmr      f1, f31
    mr       r3, r29
    mr       r4, r26
    mr       r5, r27
    mr       r6, r28
    li       r7, 0x0
    bl       SelectAreaCameraEntryWithControl
    cmpwi    r28, 0x3
    bne      .L_8007871C
    mr       r3, r25
    bl       fn_80148E30
    b        .L_8007871C
.L_800779B0:
    mr       r3, r30
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xac(r1)
    mr       r3, r30
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xa8(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xa8(r1)
    fsubs    f1, f1, f2
    fdivs    f31, f1, f0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xb4(r1)
    mr       r3, r30
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xb0(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xb0(r1)
    fsubs    f1, f1, f2
    fdivs    f30, f1, f0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xbc(r1)
    mr       r3, r30
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xb8(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xb8(r1)
    fsubs    f1, f1, f2
    fdivs    f29, f1, f0
    bl       CEvt_Evaluate
    mr       r0, r3
    mr       r3, r30
    mr       r25, r0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc4(r1)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xc0(r1)
    addis    r3, r3, 0xe
    lfs      f0, lbl_8065CF80@sda21(r0)
    li       r4, 0x3
    lfd      f1, 0xc0(r1)
    addi     r3, r3, 0x5d38
    fsubs    f1, f1, f2
    fdivs    f28, f1, f0
    bl       fn_8014946C
    li       r0, 0xb
    fmr      f1, f28
    stw      r0, 0x38(r31)
    mr       r3, r31
    mr       r4, r25
    addi     r5, r1, 0x8c
    lwz      r7, 0x80(r28)
    lwz      r6, 0x84(r28)
    lwz      r0, 0x88(r28)
    stw      r7, 0x80(r1)
    stw      r6, 0x84(r1)
    stw      r0, 0x88(r1)
    stfs     f31, 0x80(r1)
    stfs     f30, 0x84(r1)
    lwz      r7, 0x80(r1)
    stfs     f29, 0x88(r1)
    lwz      r6, 0x84(r1)
    lwz      r0, 0x88(r1)
    stw      r7, 0x8c(r1)
    stw      r6, 0x90(r1)
    stw      r0, 0x94(r1)
    bl       SetEye_Interp__16CCameraActionEvtFlRC3Vecf
    b        .L_8007871C
.L_80077AE4:
    mr       r3, r30
    bl       CEvt_Evaluate
    mr       r29, r3
    mr       r3, r30
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc4(r1)
    mr       r3, r30
    lfd      f1, lbl_8065CF88@sda21(r0)
    stw      r0, 0xc0(r1)
    lfd      f0, 0xc0(r1)
    fsubs    f28, f0, f1
    bl       CEvt_Evaluate
    mr       r0, r3
    mr       r3, r30
    mr       r27, r0
    bl       CEvt_Evaluate
    xoris    r0, r3, 0x8000
    lis      r3, 0x4330
    stw      r0, 0xbc(r1)
    subfic   r4, r26, 0xf0
    cntlzw   r4, r4
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r3, 0xb8(r1)
    subi     r0, r26, 0xfb
    clrlwi   r3, r0, 16
    srwi     r4, r4, 5
    lfd      f1, 0xb8(r1)
    subfic   r0, r3, 0x1
    lfs      f0, lbl_8065CF80@sda21(r0)
    li       r5, 0x1
    fsubs    f1, f1, f2
    orc      r3, r5, r3
    srwi     r0, r0, 1
    addi     r30, r4, 0x1
    subf     r0, r0, r3
    fdivs    f29, f1, f0
    srwi.    r0, r0, 31
    bne      .L_80077B88
    li       r5, 0x0
.L_80077B88:
    clrlwi   r5, r5, 24
    lis      r3, gGame@ha
    neg      r0, r5
    li       r4, 0x3
    addi     r3, r3, gGame@l
    or       r0, r0, r5
    addis    r3, r3, 0xe
    srwi     r25, r0, 31
    addi     r3, r3, 0x5d38
    bl       fn_8014946C
    cmpwi    r29, 0x4
    beq      .L_8007871C
    bge      .L_80077BCC
    cmpwi    r29, 0x2
    beq      .L_80077BD8
    bge      .L_80077C30
    b        .L_8007871C
.L_80077BCC:
    cmpwi    r29, 0x6
    bge      .L_8007871C
    b        .L_80077C88
.L_80077BD8:
    lfs      f0, lbl_8065CFE8@sda21(r0)
    fmr      f1, f29
    lwz      r0, 0x90(r28)
    mr       r3, r31
    fmuls    f0, f0, f28
    lwz      r10, 0x8c(r28)
    stw      r0, 0x6c(r1)
    lwz      r9, 0x94(r28)
    mr       r4, r27
    stfs     f0, 0x6c(r1)
    mr       r7, r30
    mr       r8, r25
    lwz      r0, 0x6c(r1)
    addi     r5, r1, 0x74
    stw      r10, 0x68(r1)
    li       r6, 0x2
    stw      r9, 0x70(r1)
    stw      r10, 0x74(r1)
    stw      r0, 0x78(r1)
    stw      r9, 0x7c(r1)
    bl       SetAngle_Interp__16CCameraActionEvtFlRC3VecUlUlif
    b        .L_8007871C
.L_80077C30:
    lfs      f0, lbl_8065CFE8@sda21(r0)
    fmr      f1, f29
    lwz      r0, 0x98(r28)
    mr       r3, r31
    fmuls    f0, f0, f28
    lwz      r10, 0x9c(r28)
    stw      r0, 0x50(r1)
    lwz      r9, 0xa0(r28)
    mr       r4, r27
    stfs     f0, 0x50(r1)
    mr       r7, r30
    mr       r8, r25
    lwz      r0, 0x50(r1)
    addi     r5, r1, 0x5c
    stw      r10, 0x54(r1)
    li       r6, 0x1
    stw      r9, 0x58(r1)
    stw      r0, 0x5c(r1)
    stw      r10, 0x60(r1)
    stw      r9, 0x64(r1)
    bl       SetAngle_Interp__16CCameraActionEvtFlRC3VecUlUlif
    b        .L_8007871C
.L_80077C88:
    lfs      f0, lbl_8065CFE8@sda21(r0)
    fmr      f1, f29
    lwz      r0, 0xac(r28)
    mr       r3, r31
    fmuls    f0, f0, f28
    lwz      r10, 0xa4(r28)
    stw      r0, 0x40(r1)
    lwz      r9, 0xa8(r28)
    mr       r4, r27
    stfs     f0, 0x40(r1)
    mr       r7, r30
    mr       r8, r25
    lwz      r0, 0x40(r1)
    addi     r5, r1, 0x44
    stw      r10, 0x38(r1)
    li       r6, 0x4
    stw      r9, 0x3c(r1)
    stw      r10, 0x44(r1)
    stw      r9, 0x48(r1)
    stw      r0, 0x4c(r1)
    bl       SetAngle_Interp__16CCameraActionEvtFlRC3VecUlUlif
    b        .L_8007871C
.L_80077CE0:
    mr       r3, r30
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc4(r1)
    mr       r3, r30
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xc0(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xc0(r1)
    fsubs    f1, f1, f2
    fdivs    f28, f1, f0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xbc(r1)
    mr       r3, r30
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xb8(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xb8(r1)
    fsubs    f1, f1, f2
    fdivs    f29, f1, f0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xb4(r1)
    mr       r3, r30
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xb0(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xb0(r1)
    fsubs    f1, f1, f2
    fdivs    f30, f1, f0
    bl       CEvt_Evaluate
    mr       r0, r3
    mr       r3, r30
    mr       r25, r0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xac(r1)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xa8(r1)
    addis    r3, r3, 0xe
    lfs      f0, lbl_8065CF80@sda21(r0)
    li       r4, 0x3
    lfd      f1, 0xa8(r1)
    addi     r3, r3, 0x5d38
    fsubs    f1, f1, f2
    fdivs    f31, f1, f0
    bl       fn_8014946C
    li       r0, 0xb
    fmr      f1, f31
    stw      r0, 0x38(r31)
    mr       r3, r31
    mr       r4, r25
    addi     r5, r1, 0x2c
    lwz      r7, 0xb0(r28)
    lwz      r6, 0xb4(r28)
    lwz      r0, 0xb8(r28)
    stw      r7, 0x20(r1)
    stw      r6, 0x24(r1)
    stw      r0, 0x28(r1)
    stfs     f28, 0x20(r1)
    stfs     f29, 0x24(r1)
    lwz      r7, 0x20(r1)
    stfs     f30, 0x28(r1)
    lwz      r6, 0x24(r1)
    lwz      r0, 0x28(r1)
    stw      r7, 0x2c(r1)
    stw      r6, 0x30(r1)
    stw      r0, 0x34(r1)
    bl       SetInterest_Interp__16CCameraActionEvtFlRC3Vecf
    b        .L_8007871C
.L_80077E14:
    mr       r3, r30
    bl       CEvt_Evaluate
    mr       r0, r3
    mr       r3, r30
    mr       r26, r0
    bl       CEvt_Evaluate
    mr       r0, r3
    mr       r3, r30
    mr       r27, r0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc4(r1)
    extsh    r4, r26
    lfd      f2, lbl_8065CF88@sda21(r0)
    cmpwi    r4, 0x2710
    stw      r0, 0xc0(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xc0(r1)
    fsubs    f1, f1, f2
    fdivs    f28, f1, f0
    blt      .L_80077F1C
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80077F04
.L_80077E84:
    lha      r0, 0x4(r3)
    cmpw     r0, r4
    bne      .L_80077EF8
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r4, -0x67a0(r3)
    cmplwi   r4, 0x0
    beq      .L_80077EF0
    lwz      r0, 0x228(r4)
    li       r5, 0x0
    lwz      r6, 0x22c(r4)
    li       r3, 0x1
    and      r0, r0, r5
    and      r3, r6, r3
    xor      r3, r3, r5
    xor      r0, r0, r5
    or.      r0, r3, r0
    beq      .L_80077EF0
    b        .L_80077F08
.L_80077EF0:
    li       r4, 0x0
    b        .L_80077F08
.L_80077EF8:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_80077E84
.L_80077F04:
    li       r4, 0x0
.L_80077F08:
    cmplwi   r4, 0x0
    bne      .L_80077F4C
    li       r4, 0x0
    b        .L_80077F4C
    b        .L_80077F4C
.L_80077F1C:
    extsh.   r0, r26
    blt      .L_80077F40
    lis      r3, gGame@ha
    slwi     r0, r4, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r4, -0x67a0(r3)
    b        .L_80077F4C
.L_80077F40:
    cmpwi    r4, -0x1
    bne      .L_80077F4C
    lwz      r4, 0x8(r30)
.L_80077F4C:
    cmplwi   r4, 0x0
    beq      .L_80077FD0
    lwz      r12, 0x34(r4)
    addi     r3, r1, 0x14
    lwz      r12, 0xec(r12)
    mtctr    r12
    bctrl
    lwz      r4, 0x14(r1)
    lis      r3, gGame@ha
    lwz      r5, 0x18(r1)
    addi     r3, r3, gGame@l
    lwz      r0, 0x1c(r1)
    addis    r3, r3, 0xe
    stw      r4, 0x98(r1)
    li       r4, 0x3
    addi     r3, r3, 0x5d38
    stw      r5, 0x9c(r1)
    stw      r0, 0xa0(r1)
    bl       fn_8014946C
    li       r0, 0xb
    lwz      r5, 0x98(r1)
    stw      r0, 0x38(r31)
    fmr      f1, f28
    lwz      r6, 0x9c(r1)
    mr       r3, r31
    lwz      r0, 0xa0(r1)
    mr       r4, r27
    stw      r5, 0x8(r1)
    addi     r5, r1, 0x8
    stw      r6, 0xc(r1)
    stw      r0, 0x10(r1)
    bl       SetInterest_Interp__16CCameraActionEvtFlRC3Vecf
    b        .L_8007871C
.L_80077FD0:
    mr       r4, r26
    mr       r5, r26
    addi     r3, r28, 0x96c
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80078034
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80078020
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80078020
.L_80078018:
    addi     r3, r3, 0x8
    bdnz     .L_80078018
.L_80078020:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80078034
.L_80078030:
    bdnz     .L_80078030
.L_80078034:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_8007871C
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80078068
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80078068
.L_80078060:
    addi     r3, r3, 0x8
    bdnz     .L_80078060
.L_80078068:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007871C
.L_80078078:
    bdnz     .L_80078078
    b        .L_8007871C
.L_80078080:
    mr       r3, r30
    bl       CEvt_Evaluate
    extsh    r29, r3
    cmpwi    r29, 0x2710
    blt      .L_80078144
    lis      r4, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r4)
    li       r6, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_8007812C
.L_800780AC:
    lha      r0, 0x4(r4)
    cmpw     r0, r29
    bne      .L_80078120
    lis      r4, gEventObjectIds@ha
    lis      r5, gGame@ha
    addi     r4, r4, gEventObjectIds@l
    slwi     r0, r6, 2
    add      r4, r4, r0
    addi     r5, r5, gGame@l
    lha      r0, 0x6(r4)
    addis    r4, r5, 0x1
    slwi     r0, r0, 2
    add      r4, r4, r0
    lwz      r29, -0x67a0(r4)
    cmplwi   r29, 0x0
    beq      .L_80078118
    lwz      r0, 0x228(r29)
    li       r5, 0x0
    lwz      r6, 0x22c(r29)
    li       r4, 0x1
    and      r0, r0, r5
    and      r4, r6, r4
    xor      r4, r4, r5
    xor      r0, r0, r5
    or.      r0, r4, r0
    beq      .L_80078118
    b        .L_80078130
.L_80078118:
    li       r29, 0x0
    b        .L_80078130
.L_80078120:
    addi     r4, r4, 0x4
    addi     r6, r6, 0x1
    bdnz     .L_800780AC
.L_8007812C:
    li       r29, 0x0
.L_80078130:
    cmplwi   r29, 0x0
    bne      .L_80078174
    li       r29, 0x0
    b        .L_80078174
    b        .L_80078174
.L_80078144:
    extsh.   r0, r3
    blt      .L_80078168
    lis      r4, gGame@ha
    slwi     r0, r29, 2
    addi     r4, r4, gGame@l
    addis    r4, r4, 0x1
    add      r4, r4, r0
    lwz      r29, -0x67a0(r4)
    b        .L_80078174
.L_80078168:
    cmpwi    r29, -0x1
    bne      .L_80078174
    lwz      r29, 0x8(r30)
.L_80078174:
    cmplwi   r29, 0x0
    beq      .L_800781B4
    lis      r3, gGame@ha
    li       r4, 0x3
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x5d38
    bl       fn_8014946C
    li       r0, 0xb
    lfs      f1, lbl_8065CF74@sda21(r0)
    stw      r0, 0x38(r31)
    mr       r3, r31
    mr       r5, r29
    li       r4, 0x3
    bl       SetInterest_Interp__16CCameraActionEvtFlP4XObjf
    b        .L_8007871C
.L_800781B4:
    mr       r4, r3
    mr       r5, r3
    addi     r3, r28, 0x990
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80078218
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80078204
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80078204
.L_800781FC:
    addi     r3, r3, 0x8
    bdnz     .L_800781FC
.L_80078204:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80078218
.L_80078214:
    bdnz     .L_80078214
.L_80078218:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_8007871C
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007824C
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007824C
.L_80078244:
    addi     r3, r3, 0x8
    bdnz     .L_80078244
.L_8007824C:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007871C
.L_8007825C:
    bdnz     .L_8007825C
    b        .L_8007871C
.L_80078264:
    mr       r3, r30
    bl       CEvt_Evaluate
    mr       r3, r30
    bl       CEvt_Evaluate
    mr       r3, r30
    bl       CEvt_Evaluate
    b        .L_8007871C
.L_80078280:
    mr       r3, r30
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc4(r1)
    mr       r3, r30
    lfd      f1, lbl_8065CF88@sda21(r0)
    stw      r0, 0xc0(r1)
    lfd      f0, 0xc0(r1)
    fsubs    f28, f0, f1
    bl       CEvt_Evaluate
    mr       r0, r3
    mr       r3, r30
    mr       r25, r0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xbc(r1)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xb8(r1)
    addis    r3, r3, 0xe
    lfs      f0, lbl_8065CF80@sda21(r0)
    li       r4, 0x3
    lfd      f1, 0xb8(r1)
    addi     r3, r3, 0x5d38
    fsubs    f1, f1, f2
    fdivs    f29, f1, f0
    bl       fn_8014946C
    mr       r3, r31
    lwz      r12, 0x3c(r31)
    lwz      r12, 0x54(r12)
    mtctr    r12
    bctrl
    ori      r0, r3, 0x8
    fmr      f1, f28
    fmr      f2, f29
    stw      r0, 0x38(r31)
    mr       r3, r31
    mr       r4, r25
    bl       SetFov_Interp__16CCameraActionEvtFlff
    b        .L_8007871C
.L_8007832C:
    mr       r3, r30
    bl       CEvt_Evaluate
    mr       r0, r3
    mr       r3, r30
    mr       r25, r0
    bl       CEvt_Evaluate
    mr       r3, r30
    bl       CEvt_Evaluate
    lis      r3, gGame@ha
    li       r4, 0x6
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x5d38
    bl       fn_8014946C
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x696c(r3)
    cmplwi   r3, 0x0
    beq      .L_8007871C
    lwz      r0, 0x228(r3)
    li       r5, 0x0
    lwz      r6, 0x22c(r3)
    li       r4, 0x1
    and      r0, r0, r5
    and      r4, r6, r4
    xor      r4, r4, r5
    xor      r0, r0, r5
    or.      r0, r4, r0
    beq      .L_8007871C
    lfs      f1, lbl_8065CF74@sda21(r0)
    mr       r4, r25
    bl       SetStageCameraAnimation
    b        .L_8007871C
.L_800783B4:
    mr       r3, r30
    bl       CEvt_Evaluate
    mr       r26, r3
    mr       r3, r30
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc4(r1)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xc0(r1)
    addis    r25, r3, 0xe
    addi     r25, r25, 0x5d38
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0xc0(r1)
    mr       r3, r25
    li       r4, 0x3
    fsubs    f1, f1, f2
    fdivs    f28, f1, f0
    bl       fn_80149674
    cmpwi    r3, 0x0
    beq      .L_8007871C
    stw      r26, 0x44(r27)
    li       r0, 0x0
    mr       r3, r25
    li       r4, 0x9
    stfs     f28, 0x48(r27)
    stw      r0, 0x4c(r27)
    bl       fn_8014946C
    b        .L_8007871C
.L_80078430:
    lis      r3, gGame@ha
    li       r4, 0x1
    addi     r3, r3, gGame@l
    addis    r26, r3, 0xe
    addi     r26, r26, 0x5d38
    mr       r3, r26
    bl       fn_80149674
    cmpwi    r3, 0x0
    beq      .L_80078460
    lwz      r3, 0x7df0(r25)
    bl       ResumeAreaCameraControl
    b        .L_8007871C
.L_80078460:
    mr       r3, r26
    li       r4, 0x3
    bl       fn_80149674
    cmpwi    r3, 0x0
    beq      .L_8007871C
    li       r0, 0x0
    lfs      f0, lbl_8065CF74@sda21(r0)
    stw      r0, 0x44(r27)
    li       r0, 0x1
    mr       r3, r26
    li       r4, 0x9
    stfs     f0, 0x48(r27)
    stw      r0, 0x4c(r27)
    bl       fn_8014946C
    b        .L_8007871C
.L_8007849C:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r25, r3, 0xe
    addi     r25, r25, 0x5d38
    mr       r3, r25
    bl       fn_80148B78
    mr       r3, r30
    bl       CEvt_Evaluate
    cmpwi    r3, 0x0
    blt      .L_800784E0
    lis      r4, gEventCameraMementos@ha
    slwi     r5, r3, 6
    addi     r0, r4, gEventCameraMementos@l
    mr       r3, r25
    add      r4, r0, r5
    bl       fn_80148B00
    b        .L_8007871C
.L_800784E0:
    mr       r3, r25
    li       r4, 0x0
    bl       fn_80148B00
    stw      r3, 0x60(r30)
    b        .L_8007871C
.L_800784F4:
    mr       r3, r30
    bl       CEvt_Evaluate
    cmpwi    r3, 0x0
    blt      .L_8007852C
    lis      r5, gGame@ha
    lis      r4, gEventCameraMementos@ha
    addi     r6, r5, gGame@l
    slwi     r5, r3, 6
    addis    r3, r6, 0xe
    addi     r0, r4, gEventCameraMementos@l
    add      r4, r0, r5
    addi     r3, r3, 0x5d38
    bl       fn_801487CC
    b        .L_8007871C
.L_8007852C:
    lwz      r4, 0x60(r30)
    cmplwi   r4, 0x0
    beq      .L_8007871C
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x5d38
    bl       fn_801487CC
    li       r0, 0x0
    stw      r0, 0x60(r30)
    b        .L_8007871C
.L_80078558:
    mr       r3, r30
    bl       CEvt_Evaluate
    mr.      r0, r3
    bgt      .L_8007871C
    lis      r3, gGame@ha
    srawi    r4, r0, 31
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    xor      r0, r4, r0
    subf     r4, r4, r0
    addi     r3, r3, 0x5d38
    bl       SetAreaCameraHeightIndex
    b        .L_8007871C
.L_8007858C:
    mr       r3, r30
    bl       CEvt_Evaluate
    addi     r0, r3, 0x2
    cmplwi   r0, 0x6
    bgt      .L_8007871C
    lis      r3, jumptable_802251D0@ha
    slwi     r0, r0, 2
    addi     r3, r3, jumptable_802251D0@l
    lwzx     r0, r3, r0
    mtctr    r0
    bctr
.L_800785B8:
    lis      r4, gGame@ha
    li       r3, -0x81
    addi     r6, r4, gGame@l
    li       r0, -0x1
    lwz      r4, 0x1b10(r6)
    lwz      r5, 0x1b14(r6)
    and      r0, r4, r0
    and      r3, r5, r3
    stw      r0, 0x1b10(r6)
    stw      r3, 0x1b14(r6)
    b        .L_8007871C
.L_800785E4:
    lis      r3, gGame@ha
    addi     r4, r3, gGame@l
    lwz      r0, 0x1b14(r4)
    ori      r0, r0, 0x80
    stw      r0, 0x1b14(r4)
    b        .L_8007871C
.L_800785FC:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x5d38
    bl       fn_80148B78
    b        .L_8007871C
.L_80078614:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x5d38
    bl       fn_80148C6C
    b        .L_8007871C
.L_8007862C:
    mr       r3, r30
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc4(r1)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    lfd      f1, lbl_8065CF88@sda21(r0)
    stw      r0, 0xc0(r1)
    addis    r3, r3, 0xe
    lfs      f2, lbl_8065CFE8@sda21(r0)
    lfd      f0, 0xc0(r1)
    lwz      r3, 0x7e00(r3)
    fsubs    f0, f0, f1
    fmuls    f0, f2, f0
    stfs     f0, 0x74(r3)
    b        .L_8007871C
.L_80078670:
    mr       r3, r30
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc4(r1)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    lfd      f1, lbl_8065CF88@sda21(r0)
    stw      r0, 0xc0(r1)
    addis    r3, r3, 0xe
    lfs      f2, lbl_8065CFE8@sda21(r0)
    lfd      f0, 0xc0(r1)
    lwz      r3, 0x7e00(r3)
    fsubs    f0, f0, f1
    fmuls    f0, f2, f0
    stfs     f0, 0x70(r3)
    b        .L_8007871C
.L_800786B4:
    mr       r3, r30
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc4(r1)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    lfd      f1, lbl_8065CF88@sda21(r0)
    stw      r0, 0xc0(r1)
    addis    r3, r3, 0xe
    lwz      r3, 0x7e00(r3)
    lfd      f0, 0xc0(r1)
    fsubs    f0, f0, f1
    stfs     f0, 0x78(r3)
    b        .L_8007871C
.L_800786F0:
    addi     r3, r28, 0x9b0
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvtCommandCAMERAName@ha
    mr       r6, r3
    addi     r7, r4, sEvtCommandCAMERAName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x9cf
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
.L_8007871C:
    li       r3, 0x0
    psq_l    f31, 0x128(r1), 0, qr0
    lfd      f31, 0x120(r1)
    psq_l    f30, 0x118(r1), 0, qr0
    lfd      f30, 0x110(r1)
    psq_l    f29, 0x108(r1), 0, qr0
    lfd      f29, 0x100(r1)
    psq_l    f28, 0xf8(r1), 0, qr0
    lfd      f28, 0xf0(r1)
    lmw      r25, 0xd4(r1)
    lwz      r0, 0x134(r1)
    mtlr     r0
    addi     r1, r1, 0x130
    blr
*/

/**
 * @note Address: 0x80078754
 * @note Size: 0x1D38
 */
// CEvt::_cmd_CALL
/*
    stwu     r1, -0x100(r1)
    mflr     r0
    lis      r4, lbl_80207FA0@ha
    stw      r0, 0x104(r1)
    stmw     r23, 0xdc(r1)
    addi     r29, r4, lbl_80207FA0@l
    mr       r31, r3
    li       r30, 0x0
    lwz      r7, 0x4(r3)
    lwz      r4, 0x4(r3)
    lbz      r5, 0x1(r7)
    subi     r28, r7, 0x1
    lbz      r6, 0x0(r7)
    addi     r0, r4, 0x2
    slwi     r4, r5, 8
    add      r4, r6, r4
    stw      r0, 0x4(r3)
    clrlwi   r0, r4, 16
    add      r23, r7, r0
    bl       CEvt_Evaluate
    extsh    r4, r3
    cmpwi    r4, 0x2710
    blt      .L_80078860
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80078848
.L_800787C8:
    lha      r0, 0x4(r3)
    cmpw     r0, r4
    bne      .L_8007883C
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r26, -0x67a0(r3)
    cmplwi   r26, 0x0
    beq      .L_80078834
    lwz      r0, 0x228(r26)
    li       r4, 0x0
    lwz      r5, 0x22c(r26)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_80078834
    b        .L_8007884C
.L_80078834:
    li       r26, 0x0
    b        .L_8007884C
.L_8007883C:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_800787C8
.L_80078848:
    li       r26, 0x0
.L_8007884C:
    cmplwi   r26, 0x0
    bne      .L_80078898
    li       r26, 0x0
    b        .L_80078898
    b        .L_80078898
.L_80078860:
    extsh.   r0, r4
    blt      .L_80078884
    lis      r3, gGame@ha
    slwi     r0, r4, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r26, -0x67a0(r3)
    b        .L_80078898
.L_80078884:
    cmpwi    r4, -0x1
    bne      .L_80078894
    lwz      r26, 0x8(r31)
    b        .L_80078898
.L_80078894:
    mr       r26, r4
.L_80078898:
    addi     r25, r1, 0x40
    li       r24, 0x0
    li       r27, 0x0
.L_800788A4:
    lwz      r0, 0x4(r31)
    cmplw    r0, r23
    bge      .L_800788D4
    cmpwi    r27, 0x18
    bge      .L_800788C8
    mr       r3, r31
    bl       CEvt_Evaluate
    stw      r3, 0x0(r25)
    addi     r24, r24, 0x1
.L_800788C8:
    addi     r25, r25, 0x4
    addi     r27, r27, 0x1
    b        .L_800788A4
.L_800788D4:
    cmpwi    r26, -0x1f
    beq      .L_80079FCC
    bge      .L_8007895C
    cmpwi    r26, -0x29
    beq      .L_80079FEC
    bge      .L_80078920
    cmpwi    r26, -0x2d
    beq      .L_8007A290
    bge      .L_80078910
    cmpwi    r26, -0x64
    beq      .L_8007A320
    bge      .L_8007A388
    cmpwi    r26, -0xc8
    beq      .L_8007A368
    b        .L_8007A388
.L_80078910:
    cmpwi    r26, -0x2b
    beq      .L_8007A05C
    bge      .L_8007A020
    b        .L_8007A228
.L_80078920:
    cmpwi    r26, -0x24
    beq      .L_8007A184
    bge      .L_80078944
    cmpwi    r26, -0x26
    beq      .L_8007A110
    bge      .L_8007A0F4
    cmpwi    r26, -0x27
    bge      .L_8007A2F8
    b        .L_8007A30C
.L_80078944:
    cmpwi    r26, -0x21
    beq      .L_8007A0C4
    bge      .L_8007A0A4
    cmpwi    r26, -0x22
    bge      .L_8007A140
    b        .L_8007A158
.L_8007895C:
    cmpwi    r26, -0x15
    beq      .L_80079384
    bge      .L_800789A4
    cmpwi    r26, -0x1a
    beq      .L_800796E4
    bge      .L_8007898C
    cmpwi    r26, -0x1c
    beq      .L_80079DA4
    bge      .L_80079AA0
    cmpwi    r26, -0x1d
    bge      .L_80079F9C
    b        .L_80079FB4
.L_8007898C:
    cmpwi    r26, -0x17
    beq      .L_800794BC
    bge      .L_80079474
    cmpwi    r26, -0x18
    bge      .L_800795AC
    b        .L_8007969C
.L_800789A4:
    cmpwi    r26, -0x10
    beq      .L_80078E90
    bge      .L_800789C8
    cmpwi    r26, -0x12
    beq      .L_8007915C
    bge      .L_8007906C
    cmpwi    r26, -0x13
    bge      .L_8007924C
    b        .L_80079294
.L_800789C8:
    cmpwi    r26, -0xa
    beq      .L_800789E0
    bge      .L_8007A388
    cmpwi    r26, -0xb
    bge      .L_80078B5C
    b        .L_80078D78
.L_800789E0:
    lwz      r0, 0x50(r1)
    lis      r5, 0x4330
    lwz      r4, 0x4c(r1)
    lwz      r3, 0x48(r1)
    xoris    r6, r0, 0x8000
    lwz      r0, 0x44(r1)
    xoris    r4, r4, 0x8000
    xoris    r3, r3, 0x8000
    lwz      r7, 0x40(r1)
    xoris    r0, r0, 0x8000
    stw      r6, 0xa4(r1)
    extsh    r6, r7
    lfd      f1, lbl_8065CF88@sda21(r0)
    stw      r5, 0xa0(r1)
    cmpwi    r6, 0x2710
    lfd      f0, 0xa0(r1)
    stw      r4, 0xac(r1)
    fsubs    f4, f0, f1
    stw      r5, 0xa8(r1)
    lfd      f0, 0xa8(r1)
    stw      r3, 0xb4(r1)
    fsubs    f3, f0, f1
    stw      r5, 0xb0(r1)
    lfd      f0, 0xb0(r1)
    stw      r0, 0xbc(r1)
    fsubs    f2, f0, f1
    stw      r5, 0xb8(r1)
    lfd      f0, 0xb8(r1)
    fsubs    f1, f0, f1
    blt      .L_80078B08
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80078AF0
.L_80078A70:
    lha      r0, 0x4(r3)
    cmpw     r0, r6
    bne      .L_80078AE4
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r6, -0x67a0(r3)
    cmplwi   r6, 0x0
    beq      .L_80078ADC
    lwz      r0, 0x228(r6)
    li       r4, 0x0
    lwz      r5, 0x22c(r6)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_80078ADC
    b        .L_80078AF4
.L_80078ADC:
    li       r6, 0x0
    b        .L_80078AF4
.L_80078AE4:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_80078A70
.L_80078AF0:
    li       r6, 0x0
.L_80078AF4:
    cmplwi   r6, 0x0
    bne      .L_80078B38
    li       r6, 0x0
    b        .L_80078B38
    b        .L_80078B38
.L_80078B08:
    extsh.   r0, r6
    blt      .L_80078B2C
    lis      r3, gGame@ha
    slwi     r0, r6, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r6, -0x67a0(r3)
    b        .L_80078B38
.L_80078B2C:
    cmpwi    r6, -0x1
    bne      .L_80078B38
    lwz      r6, 0x8(r31)
.L_80078B38:
    lfs      f0, 0xc8(r6)
    stfs     f0, 0x898(r6)
    lfs      f0, 0xd0(r6)
    stfs     f0, 0x89c(r6)
    stfs     f1, 0x8a0(r6)
    stfs     f2, 0x8a4(r6)
    stfs     f3, 0x8a8(r6)
    stfs     f4, 0x8ac(r6)
    b        .L_8007A474
.L_80078B5C:
    lwz      r0, 0x44(r1)
    lwz      r3, 0x40(r1)
    cmpwi    r0, -0x1
    extsh    r3, r3
    bne      .L_80078C70
    cmpwi    r3, 0x2710
    blt      .L_80078C28
    lis      r4, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r4)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80078C10
.L_80078B90:
    lha      r0, 0x4(r4)
    cmpw     r0, r3
    bne      .L_80078C04
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r3, -0x67a0(r3)
    cmplwi   r3, 0x0
    beq      .L_80078BFC
    lwz      r0, 0x228(r3)
    li       r5, 0x0
    lwz      r6, 0x22c(r3)
    li       r4, 0x1
    and      r0, r0, r5
    and      r4, r6, r4
    xor      r4, r4, r5
    xor      r0, r0, r5
    or.      r0, r4, r0
    beq      .L_80078BFC
    b        .L_80078C14
.L_80078BFC:
    li       r3, 0x0
    b        .L_80078C14
.L_80078C04:
    addi     r4, r4, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_80078B90
.L_80078C10:
    li       r3, 0x0
.L_80078C14:
    cmplwi   r3, 0x0
    bne      .L_80078C58
    li       r3, 0x0
    b        .L_80078C58
    b        .L_80078C58
.L_80078C28:
    extsh.   r0, r3
    blt      .L_80078C4C
    lis      r4, gGame@ha
    slwi     r0, r3, 2
    addi     r3, r4, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r3, -0x67a0(r3)
    b        .L_80078C58
.L_80078C4C:
    cmpwi    r3, -0x1
    bne      .L_80078C58
    lwz      r3, 0x8(r31)
.L_80078C58:
    li       r0, 0x0
    stw      r0, 0x8b0(r3)
    lwz      r0, 0x8b0(r3)
    oris     r0, r0, 0x8000
    stw      r0, 0x8b0(r3)
    b        .L_8007A474
.L_80078C70:
    cmpwi    r3, 0x2710
    blt      .L_80078D2C
    lis      r4, gEventObjectIds@ha
    li       r6, 0x0
    addi     r5, r4, gEventObjectIds@l
    lwz      r4, 0x0(r5)
    mtctr    r4
    cmpwi    r4, 0x0
    ble      .L_80078D14
.L_80078C94:
    lha      r4, 0x4(r5)
    cmpw     r4, r3
    bne      .L_80078D08
    lis      r3, gEventObjectIds@ha
    lis      r5, gGame@ha
    addi     r4, r3, gEventObjectIds@l
    slwi     r3, r6, 2
    add      r3, r4, r3
    addi     r4, r5, gGame@l
    lha      r3, 0x6(r3)
    addis    r4, r4, 0x1
    slwi     r3, r3, 2
    add      r3, r4, r3
    lwz      r3, -0x67a0(r3)
    cmplwi   r3, 0x0
    beq      .L_80078D00
    lwz      r4, 0x228(r3)
    li       r6, 0x0
    lwz      r7, 0x22c(r3)
    li       r5, 0x1
    and      r4, r4, r6
    and      r5, r7, r5
    xor      r5, r5, r6
    xor      r4, r4, r6
    or.      r4, r5, r4
    beq      .L_80078D00
    b        .L_80078D18
.L_80078D00:
    li       r3, 0x0
    b        .L_80078D18
.L_80078D08:
    addi     r5, r5, 0x4
    addi     r6, r6, 0x1
    bdnz     .L_80078C94
.L_80078D14:
    li       r3, 0x0
.L_80078D18:
    cmplwi   r3, 0x0
    bne      .L_80078D5C
    li       r3, 0x0
    b        .L_80078D5C
    b        .L_80078D5C
.L_80078D2C:
    extsh.   r4, r3
    blt      .L_80078D50
    lis      r4, gGame@ha
    slwi     r3, r3, 2
    addi     r4, r4, gGame@l
    addis    r4, r4, 0x1
    add      r3, r4, r3
    lwz      r3, -0x67a0(r3)
    b        .L_80078D5C
.L_80078D50:
    cmpwi    r3, -0x1
    bne      .L_80078D5C
    lwz      r3, 0x8(r31)
.L_80078D5C:
    lwz      r4, 0x8b0(r3)
    clrlwi   r4, r4, 1
    stw      r4, 0x8b0(r3)
    lwz      r4, 0x8b0(r3)
    or       r0, r4, r0
    stw      r0, 0x8b0(r3)
    b        .L_8007A474
.L_80078D78:
    lwz      r0, 0x1c(r31)
    cmpwi    r0, 0x0
    beq      .L_80078E14
    lis      r4, gGame@ha
    li       r3, 0x800
    addi     r6, r4, gGame@l
    lwz      r5, 0x1b14(r6)
    li       r4, 0x0
    lwz      r0, 0x1b10(r6)
    and      r3, r5, r3
    and      r0, r0, r4
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_80078DF8
    addi     r3, r1, 0x18
    li       r4, 0x0
    bl       __ct__4CPadFUc
    lis      r3, gGame@ha
    addi     r4, r1, 0x18
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    addi     r3, r3, 0x3390
    bl       HandleOXMenuInput
    lis      r3, __vt__4CPad@ha
    stw      r28, 0x4(r31)
    addi     r0, r3, __vt__4CPad@l
    li       r30, 0x1
    stw      r0, 0x1c(r1)
    b        .L_8007A474
.L_80078DF8:
    stw      r4, 0x1c(r31)
    addis    r3, r6, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lwz      r0, 0x46d0(r3)
    stw      r0, 0x10(r31)
    b        .L_8007A474
.L_80078E14:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r4, r3, 0x1
    lwz      r0, -0x6a2c(r4)
    cmpwi    r0, 0x0
    bge      .L_80078E84
    cmpwi    r26, -0xc
    li       r4, 0x2
    bne      .L_80078E48
    li       r4, 0x2
    b        .L_80078E74
.L_80078E48:
    cmpwi    r26, -0xd
    bne      .L_80078E58
    li       r4, 0x1
    b        .L_80078E74
.L_80078E58:
    cmpwi    r26, -0xe
    bne      .L_80078E68
    li       r4, 0x0
    b        .L_80078E74
.L_80078E68:
    cmpwi    r26, -0xf
    bne      .L_80078E74
    li       r4, 0x3
.L_80078E74:
    lwz      r5, 0x40(r1)
    bl       OpenPlayerYesNoMenu
    li       r0, 0x1
    stw      r0, 0x1c(r31)
.L_80078E84:
    stw      r28, 0x4(r31)
    li       r30, 0x1
    b        .L_8007A474
.L_80078E90:
    lwz      r0, 0x40(r1)
    cmpwi    r0, 0xe7
    bne      .L_80078F20
    lwz      r0, 0x44(r1)
    lis      r6, 0x4330
    lwz      r4, 0x48(r1)
    lis      r5, gGame@ha
    lwz      r3, 0x4c(r1)
    xoris    r7, r0, 0x8000
    lwz      r0, 0x50(r1)
    xoris    r4, r4, 0x8000
    xoris    r3, r3, 0x8000
    stw      r7, 0xbc(r1)
    xoris    r0, r0, 0x8000
    lfd      f4, lbl_8065CF88@sda21(r0)
    stw      r6, 0xb8(r1)
    addi     r5, r5, gGame@l
    lfd      f0, 0xb8(r1)
    stw      r4, 0xb4(r1)
    fsubs    f3, f0, f4
    stw      r6, 0xb0(r1)
    lfd      f0, 0xb0(r1)
    stw      r3, 0xac(r1)
    fsubs    f2, f0, f4
    stw      r6, 0xa8(r1)
    lfd      f0, 0xa8(r1)
    stw      r0, 0xa4(r1)
    fsubs    f1, f0, f4
    stw      r6, 0xa0(r1)
    lfd      f0, 0xa0(r1)
    stfs     f3, 0x43e0(r5)
    fsubs    f0, f0, f4
    stfs     f2, 0x43e4(r5)
    stfs     f1, 0x43e8(r5)
    stfs     f0, 0x43ec(r5)
    b        .L_8007A474
.L_80078F20:
    cmpwi    r0, 0xe1
    bne      .L_80079048
    lwz      r5, 0x44(r1)
    lis      r8, 0x4330
    lwz      r4, 0x48(r1)
    lis      r3, gGame@ha
    lwz      r0, 0x4c(r1)
    xoris    r5, r5, 0x8000
    xoris    r4, r4, 0x8000
    stw      r5, 0xbc(r1)
    xoris    r5, r0, 0x8000
    lwz      r0, 0x50(r1)
    stw      r8, 0xb8(r1)
    addi     r3, r3, gGame@l
    lfd      f7, lbl_8065CF88@sda21(r0)
    xoris    r7, r0, 0x8000
    lfd      f0, 0xb8(r1)
    addi     r3, r3, 0x42ac
    stw      r4, 0xb4(r1)
    addi     r4, r1, 0x20
    fsubs    f0, f0, f7
    lfs      f2, lbl_8065CF80@sda21(r0)
    stw      r8, 0xb0(r1)
    lwz      r0, 0x54(r1)
    lfd      f1, 0xb0(r1)
    fdivs    f4, f0, f2
    stw      r5, 0xac(r1)
    xoris    r6, r0, 0x8000
    lwz      r5, 0x58(r1)
    stw      r8, 0xa8(r1)
    lwz      r0, 0x5c(r1)
    lfd      f0, 0xa8(r1)
    fsubs    f1, f1, f7
    xoris    r5, r5, 0x8000
    xoris    r0, r0, 0x8000
    fsubs    f0, f0, f7
    lwz      r11, 0x60(r29)
    lwz      r10, 0x64(r29)
    lwz      r9, 0x68(r29)
    fdivs    f6, f1, f2
    stw      r11, 0x20(r1)
    lfs      f3, lbl_8065CFE8@sda21(r0)
    stw      r10, 0x24(r1)
    stw      r7, 0xa4(r1)
    stw      r8, 0xa0(r1)
    lfd      f1, 0xa0(r1)
    fdivs    f5, f0, f2
    stw      r9, 0x28(r1)
    stw      r6, 0xc4(r1)
    stw      r8, 0xc0(r1)
    lfd      f0, 0xc0(r1)
    stw      r5, 0xcc(r1)
    stw      r8, 0xc8(r1)
    fsubs    f1, f1, f7
    fsubs    f2, f0, f7
    lfd      f0, 0xc8(r1)
    stw      r0, 0xd4(r1)
    fmuls    f1, f3, f1
    fmuls    f2, f3, f2
    stw      r8, 0xd0(r1)
    fsubs    f3, f0, f7
    lfd      f0, 0xd0(r1)
    stfs     f4, 0x20(r1)
    fsubs    f4, f0, f7
    stfs     f6, 0x24(r1)
    stfs     f5, 0x28(r1)
    bl       ConfigureSubwindowCamera
    lis      r3, gGame@ha
    li       r4, 0x1
    addi     r3, r3, gGame@l
    li       r0, 0x0
    stw      r4, 0x43d0(r3)
    stw      r0, 0x43d4(r3)
    b        .L_8007A474
.L_80079048:
    cmpwi    r0, 0xd1
    beq      .L_80079058
    cmpwi    r0, 0xe6
    bne      .L_8007A474
.L_80079058:
    lis      r3, gGame@ha
    li       r0, 0x0
    addi     r3, r3, gGame@l
    stw      r0, 0x43d0(r3)
    b        .L_8007A474
.L_8007906C:
    lwz      r0, 0x40(r1)
    extsh.   r6, r0
    ble      .L_800790A8
    lis      r3, gGame@ha
    rlwinm   r4, r6, 29, 3, 29
    addi     r3, r3, gGame@l
    clrlwi   r0, r6, 27
    addis    r5, r3, 0x1
    lwz      r5, -0x68cc(r5)
    li       r3, 0x1
    slw      r0, r3, r0
    lwz      r5, 0x980(r5)
    lwzx     r3, r5, r4
    or       r0, r3, r0
    stwx     r0, r5, r4
.L_800790A8:
    cmpwi    r6, -0x2
    bne      .L_8007A474
    lis      r3, gGame@ha
    li       r0, 0x4
    addi     r4, r3, gGame@l
    addis    r4, r4, 0x1
    li       r3, -0x1
    lwz      r4, -0x68cc(r4)
    lwz      r4, 0x980(r4)
    mtctr    r0
.L_800790D0:
    stw      r3, 0x0(r4)
    stw      r3, 0x4(r4)
    stw      r3, 0x8(r4)
    stw      r3, 0xc(r4)
    stw      r3, 0x10(r4)
    stw      r3, 0x14(r4)
    stw      r3, 0x18(r4)
    stw      r3, 0x1c(r4)
    stw      r3, 0x20(r4)
    stw      r3, 0x24(r4)
    stw      r3, 0x28(r4)
    stw      r3, 0x2c(r4)
    stw      r3, 0x30(r4)
    stw      r3, 0x34(r4)
    stw      r3, 0x38(r4)
    stw      r3, 0x3c(r4)
    stw      r3, 0x40(r4)
    stw      r3, 0x44(r4)
    stw      r3, 0x48(r4)
    stw      r3, 0x4c(r4)
    stw      r3, 0x50(r4)
    stw      r3, 0x54(r4)
    stw      r3, 0x58(r4)
    stw      r3, 0x5c(r4)
    stw      r3, 0x60(r4)
    stw      r3, 0x64(r4)
    stw      r3, 0x68(r4)
    stw      r3, 0x6c(r4)
    stw      r3, 0x70(r4)
    stw      r3, 0x74(r4)
    stw      r3, 0x78(r4)
    stw      r3, 0x7c(r4)
    addi     r4, r4, 0x80
    bdnz     .L_800790D0
    b        .L_8007A474
.L_8007915C:
    lwz      r0, 0x40(r1)
    extsh.   r6, r0
    ble      .L_80079198
    lis      r3, gGame@ha
    rlwinm   r4, r6, 29, 3, 29
    addi     r3, r3, gGame@l
    clrlwi   r0, r6, 27
    addis    r5, r3, 0x1
    lwz      r5, -0x68cc(r5)
    li       r3, 0x1
    slw      r0, r3, r0
    lwz      r5, 0x980(r5)
    lwzx     r3, r5, r4
    andc     r0, r3, r0
    stwx     r0, r5, r4
.L_80079198:
    cmpwi    r6, -0x2
    bne      .L_8007A474
    lis      r3, gGame@ha
    li       r0, 0x4
    addi     r4, r3, gGame@l
    addis    r4, r4, 0x1
    li       r3, 0x0
    lwz      r4, -0x68cc(r4)
    lwz      r4, 0x980(r4)
    mtctr    r0
.L_800791C0:
    stw      r3, 0x0(r4)
    stw      r3, 0x4(r4)
    stw      r3, 0x8(r4)
    stw      r3, 0xc(r4)
    stw      r3, 0x10(r4)
    stw      r3, 0x14(r4)
    stw      r3, 0x18(r4)
    stw      r3, 0x1c(r4)
    stw      r3, 0x20(r4)
    stw      r3, 0x24(r4)
    stw      r3, 0x28(r4)
    stw      r3, 0x2c(r4)
    stw      r3, 0x30(r4)
    stw      r3, 0x34(r4)
    stw      r3, 0x38(r4)
    stw      r3, 0x3c(r4)
    stw      r3, 0x40(r4)
    stw      r3, 0x44(r4)
    stw      r3, 0x48(r4)
    stw      r3, 0x4c(r4)
    stw      r3, 0x50(r4)
    stw      r3, 0x54(r4)
    stw      r3, 0x58(r4)
    stw      r3, 0x5c(r4)
    stw      r3, 0x60(r4)
    stw      r3, 0x64(r4)
    stw      r3, 0x68(r4)
    stw      r3, 0x6c(r4)
    stw      r3, 0x70(r4)
    stw      r3, 0x74(r4)
    stw      r3, 0x78(r4)
    stw      r3, 0x7c(r4)
    addi     r4, r4, 0x80
    bdnz     .L_800791C0
    b        .L_8007A474
.L_8007924C:
    lis      r3, gGame@ha
    lwz      r0, 0x40(r1)
    addi     r3, r3, gGame@l
    li       r5, 0x1
    addis    r3, r3, 0x1
    extsh    r0, r0
    lwz      r3, -0x68cc(r3)
    clrlwi   r4, r0, 27
    rlwinm   r0, r0, 29, 3, 29
    lwz      r3, 0x980(r3)
    slw      r4, r5, r4
    lwzx     r0, r3, r0
    and      r3, r4, r0
    neg      r0, r3
    or       r0, r0, r3
    srwi     r0, r0, 31
    stw      r0, 0x10(r31)
    b        .L_8007A474
.L_80079294:
    lwz      r0, 0x40(r1)
    extsh.   r6, r0
    ble      .L_800792D0
    lis      r3, gGame@ha
    rlwinm   r4, r6, 29, 3, 29
    addi     r3, r3, gGame@l
    clrlwi   r0, r6, 27
    addis    r5, r3, 0x1
    lwz      r5, -0x68e0(r5)
    li       r3, 0x1
    slw      r0, r3, r0
    lwz      r5, 0x980(r5)
    lwzx     r3, r5, r4
    or       r0, r3, r0
    stwx     r0, r5, r4
.L_800792D0:
    cmpwi    r6, -0x2
    bne      .L_8007A474
    lis      r3, gGame@ha
    li       r0, 0x4
    addi     r4, r3, gGame@l
    addis    r4, r4, 0x1
    li       r3, -0x1
    lwz      r4, -0x68e0(r4)
    lwz      r4, 0x980(r4)
    mtctr    r0
.L_800792F8:
    stw      r3, 0x0(r4)
    stw      r3, 0x4(r4)
    stw      r3, 0x8(r4)
    stw      r3, 0xc(r4)
    stw      r3, 0x10(r4)
    stw      r3, 0x14(r4)
    stw      r3, 0x18(r4)
    stw      r3, 0x1c(r4)
    stw      r3, 0x20(r4)
    stw      r3, 0x24(r4)
    stw      r3, 0x28(r4)
    stw      r3, 0x2c(r4)
    stw      r3, 0x30(r4)
    stw      r3, 0x34(r4)
    stw      r3, 0x38(r4)
    stw      r3, 0x3c(r4)
    stw      r3, 0x40(r4)
    stw      r3, 0x44(r4)
    stw      r3, 0x48(r4)
    stw      r3, 0x4c(r4)
    stw      r3, 0x50(r4)
    stw      r3, 0x54(r4)
    stw      r3, 0x58(r4)
    stw      r3, 0x5c(r4)
    stw      r3, 0x60(r4)
    stw      r3, 0x64(r4)
    stw      r3, 0x68(r4)
    stw      r3, 0x6c(r4)
    stw      r3, 0x70(r4)
    stw      r3, 0x74(r4)
    stw      r3, 0x78(r4)
    stw      r3, 0x7c(r4)
    addi     r4, r4, 0x80
    bdnz     .L_800792F8
    b        .L_8007A474
.L_80079384:
    lwz      r0, 0x40(r1)
    extsh.   r6, r0
    ble      .L_800793C0
    lis      r3, gGame@ha
    rlwinm   r4, r6, 29, 3, 29
    addi     r3, r3, gGame@l
    clrlwi   r0, r6, 27
    addis    r5, r3, 0x1
    lwz      r5, -0x68e0(r5)
    li       r3, 0x1
    slw      r0, r3, r0
    lwz      r5, 0x980(r5)
    lwzx     r3, r5, r4
    andc     r0, r3, r0
    stwx     r0, r5, r4
.L_800793C0:
    cmpwi    r6, -0x2
    bne      .L_8007A474
    lis      r3, gGame@ha
    li       r0, 0x4
    addi     r4, r3, gGame@l
    addis    r4, r4, 0x1
    li       r3, 0x0
    lwz      r4, -0x68e0(r4)
    lwz      r4, 0x980(r4)
    mtctr    r0
.L_800793E8:
    stw      r3, 0x0(r4)
    stw      r3, 0x4(r4)
    stw      r3, 0x8(r4)
    stw      r3, 0xc(r4)
    stw      r3, 0x10(r4)
    stw      r3, 0x14(r4)
    stw      r3, 0x18(r4)
    stw      r3, 0x1c(r4)
    stw      r3, 0x20(r4)
    stw      r3, 0x24(r4)
    stw      r3, 0x28(r4)
    stw      r3, 0x2c(r4)
    stw      r3, 0x30(r4)
    stw      r3, 0x34(r4)
    stw      r3, 0x38(r4)
    stw      r3, 0x3c(r4)
    stw      r3, 0x40(r4)
    stw      r3, 0x44(r4)
    stw      r3, 0x48(r4)
    stw      r3, 0x4c(r4)
    stw      r3, 0x50(r4)
    stw      r3, 0x54(r4)
    stw      r3, 0x58(r4)
    stw      r3, 0x5c(r4)
    stw      r3, 0x60(r4)
    stw      r3, 0x64(r4)
    stw      r3, 0x68(r4)
    stw      r3, 0x6c(r4)
    stw      r3, 0x70(r4)
    stw      r3, 0x74(r4)
    stw      r3, 0x78(r4)
    stw      r3, 0x7c(r4)
    addi     r4, r4, 0x80
    bdnz     .L_800793E8
    b        .L_8007A474
.L_80079474:
    lis      r3, gGame@ha
    lwz      r0, 0x40(r1)
    addi     r3, r3, gGame@l
    li       r5, 0x1
    addis    r3, r3, 0x1
    extsh    r0, r0
    lwz      r3, -0x68e0(r3)
    clrlwi   r4, r0, 27
    rlwinm   r0, r0, 29, 3, 29
    lwz      r3, 0x980(r3)
    slw      r4, r5, r4
    lwzx     r0, r3, r0
    and      r3, r4, r0
    neg      r0, r3
    or       r0, r0, r3
    srwi     r0, r0, 31
    stw      r0, 0x10(r31)
    b        .L_8007A474
.L_800794BC:
    lwz      r0, 0x40(r1)
    extsh.   r6, r0
    ble      .L_800794F8
    lis      r3, gGame@ha
    rlwinm   r4, r6, 29, 3, 29
    addi     r3, r3, gGame@l
    clrlwi   r0, r6, 27
    addis    r5, r3, 0x1
    lwz      r5, -0x67dc(r5)
    li       r3, 0x1
    slw      r0, r3, r0
    lwz      r5, 0x980(r5)
    lwzx     r3, r5, r4
    or       r0, r3, r0
    stwx     r0, r5, r4
.L_800794F8:
    cmpwi    r6, -0x2
    bne      .L_8007A474
    lis      r3, gGame@ha
    li       r0, 0x4
    addi     r4, r3, gGame@l
    addis    r4, r4, 0x1
    li       r3, -0x1
    lwz      r4, -0x67dc(r4)
    lwz      r4, 0x980(r4)
    mtctr    r0
.L_80079520:
    stw      r3, 0x0(r4)
    stw      r3, 0x4(r4)
    stw      r3, 0x8(r4)
    stw      r3, 0xc(r4)
    stw      r3, 0x10(r4)
    stw      r3, 0x14(r4)
    stw      r3, 0x18(r4)
    stw      r3, 0x1c(r4)
    stw      r3, 0x20(r4)
    stw      r3, 0x24(r4)
    stw      r3, 0x28(r4)
    stw      r3, 0x2c(r4)
    stw      r3, 0x30(r4)
    stw      r3, 0x34(r4)
    stw      r3, 0x38(r4)
    stw      r3, 0x3c(r4)
    stw      r3, 0x40(r4)
    stw      r3, 0x44(r4)
    stw      r3, 0x48(r4)
    stw      r3, 0x4c(r4)
    stw      r3, 0x50(r4)
    stw      r3, 0x54(r4)
    stw      r3, 0x58(r4)
    stw      r3, 0x5c(r4)
    stw      r3, 0x60(r4)
    stw      r3, 0x64(r4)
    stw      r3, 0x68(r4)
    stw      r3, 0x6c(r4)
    stw      r3, 0x70(r4)
    stw      r3, 0x74(r4)
    stw      r3, 0x78(r4)
    stw      r3, 0x7c(r4)
    addi     r4, r4, 0x80
    bdnz     .L_80079520
    b        .L_8007A474
.L_800795AC:
    lwz      r0, 0x40(r1)
    extsh.   r6, r0
    ble      .L_800795E8
    lis      r3, gGame@ha
    rlwinm   r4, r6, 29, 3, 29
    addi     r3, r3, gGame@l
    clrlwi   r0, r6, 27
    addis    r5, r3, 0x1
    lwz      r5, -0x67dc(r5)
    li       r3, 0x1
    slw      r0, r3, r0
    lwz      r5, 0x980(r5)
    lwzx     r3, r5, r4
    andc     r0, r3, r0
    stwx     r0, r5, r4
.L_800795E8:
    cmpwi    r6, -0x2
    bne      .L_8007A474
    lis      r3, gGame@ha
    li       r0, 0x4
    addi     r4, r3, gGame@l
    addis    r4, r4, 0x1
    li       r3, 0x0
    lwz      r4, -0x67dc(r4)
    lwz      r4, 0x980(r4)
    mtctr    r0
.L_80079610:
    stw      r3, 0x0(r4)
    stw      r3, 0x4(r4)
    stw      r3, 0x8(r4)
    stw      r3, 0xc(r4)
    stw      r3, 0x10(r4)
    stw      r3, 0x14(r4)
    stw      r3, 0x18(r4)
    stw      r3, 0x1c(r4)
    stw      r3, 0x20(r4)
    stw      r3, 0x24(r4)
    stw      r3, 0x28(r4)
    stw      r3, 0x2c(r4)
    stw      r3, 0x30(r4)
    stw      r3, 0x34(r4)
    stw      r3, 0x38(r4)
    stw      r3, 0x3c(r4)
    stw      r3, 0x40(r4)
    stw      r3, 0x44(r4)
    stw      r3, 0x48(r4)
    stw      r3, 0x4c(r4)
    stw      r3, 0x50(r4)
    stw      r3, 0x54(r4)
    stw      r3, 0x58(r4)
    stw      r3, 0x5c(r4)
    stw      r3, 0x60(r4)
    stw      r3, 0x64(r4)
    stw      r3, 0x68(r4)
    stw      r3, 0x6c(r4)
    stw      r3, 0x70(r4)
    stw      r3, 0x74(r4)
    stw      r3, 0x78(r4)
    stw      r3, 0x7c(r4)
    addi     r4, r4, 0x80
    bdnz     .L_80079610
    b        .L_8007A474
.L_8007969C:
    lis      r3, gGame@ha
    lwz      r0, 0x40(r1)
    addi     r3, r3, gGame@l
    li       r5, 0x1
    addis    r3, r3, 0x1
    extsh    r0, r0
    lwz      r3, -0x67dc(r3)
    clrlwi   r4, r0, 27
    rlwinm   r0, r0, 29, 3, 29
    lwz      r3, 0x980(r3)
    slw      r4, r5, r4
    lwzx     r0, r3, r0
    and      r3, r4, r0
    neg      r0, r3
    or       r0, r0, r3
    srwi     r0, r0, 31
    stw      r0, 0x10(r31)
    b        .L_8007A474
.L_800796E4:
    lwz      r0, 0x40(r1)
    li       r3, 0x0
    lwz      r24, 0x44(r1)
    li       r4, 0x1
    extsh    r23, r0
    mr       r5, r24
    bl       __shl2i
    cmpwi    r23, 0x2710
    mr       r26, r4
    mr       r27, r3
    blt      .L_800797C0
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_800797A8
.L_80079728:
    lha      r0, 0x4(r3)
    cmpw     r0, r23
    bne      .L_8007979C
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r25, -0x67a0(r3)
    cmplwi   r25, 0x0
    beq      .L_80079794
    lwz      r0, 0x228(r25)
    li       r4, 0x0
    lwz      r5, 0x22c(r25)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_80079794
    b        .L_800797AC
.L_80079794:
    li       r25, 0x0
    b        .L_800797AC
.L_8007979C:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_80079728
.L_800797A8:
    li       r25, 0x0
.L_800797AC:
    cmplwi   r25, 0x0
    bne      .L_800797F8
    li       r25, 0x0
    b        .L_800797F8
    b        .L_800797F8
.L_800797C0:
    extsh.   r0, r23
    blt      .L_800797E4
    lis      r3, gGame@ha
    slwi     r0, r23, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r25, -0x67a0(r3)
    b        .L_800797F8
.L_800797E4:
    cmpwi    r23, -0x1
    bne      .L_800797F4
    lwz      r25, 0x8(r31)
    b        .L_800797F8
.L_800797F4:
    mr       r25, r23
.L_800797F8:
    cmplwi   r25, 0x0
    beq      .L_800799EC
    lwz      r3, 0x22c(r25)
    lis      r0, 0x2
    li       r5, 0x0
    lwz      r7, 0x228(r25)
    or       r6, r3, r26
    and      r4, r26, r0
    and      r3, r27, r5
    stw      r6, 0x22c(r25)
    or       r6, r7, r27
    xor      r4, r4, r5
    xor      r3, r3, r5
    stw      r6, 0x228(r25)
    or.      r3, r4, r3
    beq      .L_800798B4
    lwz      r4, 0x6c(r29)
    lis      r8, 0x80
    lwz      r12, 0x70(r29)
    lis      r7, 0x20
    lwz      r11, 0x74(r29)
    li       r6, 0x40
    lwz      r10, 0x78(r29)
    lis      r5, 0x40
    lwz      r9, 0x7c(r29)
    mr       r3, r25
    stw      r4, 0x2c(r1)
    addi     r4, r1, 0x2c
    stw      r12, 0x30(r1)
    stw      r11, 0x34(r1)
    stw      r10, 0x38(r1)
    stw      r9, 0x3c(r1)
    lwz      r10, 0x22c(r25)
    and      r9, r10, r8
    and      r8, r10, r7
    and      r7, r10, r6
    and      r6, r10, r0
    and      r0, r10, r5
    stw      r9, 0x2c(r1)
    stw      r8, 0x30(r1)
    stw      r7, 0x34(r1)
    stw      r6, 0x38(r1)
    stw      r0, 0x3c(r1)
    lwz      r12, 0x34(r25)
    lwz      r12, 0xd8(r12)
    mtctr    r12
    bctrl
.L_800798B4:
    li       r4, 0x0
    li       r0, 0x4000
    and      r3, r26, r4
    and      r0, r27, r0
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_8007A474
    lwz      r5, 0x240(r25)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    cmplwi   r5, 0x0
    lwz      r3, -0x6944(r3)
    beq      .L_800798FC
    beq      .L_80079900
    lha      r4, 0x14(r5)
    b        .L_80079900
.L_800798FC:
    li       r4, -0x1
.L_80079900:
    extsh    r0, r4
    cmpwi    r0, 0x8
    bne      .L_8007A474
    lwz      r0, 0xa64(r3)
    cmplw    r25, r0
    bne      .L_8007A474
    cmplwi   r5, 0x0
    beq      .L_80079934
    beq      .L_8007992C
    lha      r0, 0x16(r5)
    b        .L_80079938
.L_8007992C:
    li       r0, 0x0
    b        .L_80079938
.L_80079934:
    li       r0, -0x1
.L_80079938:
    extsh    r5, r0
    cmpwi    r5, 0x16
    beq      .L_8007998C
    bge      .L_80079960
    cmpwi    r5, 0x10
    beq      .L_8007996C
    blt      .L_800799A8
    cmpwi    r5, 0x15
    bge      .L_8007997C
    b        .L_800799A8
.L_80079960:
    cmpwi    r5, 0x38
    beq      .L_8007999C
    b        .L_800799A8
.L_8007996C:
    addis    r4, r3, 0x1
    li       r0, 0x4
    stw      r0, -0x6a30(r4)
    b        .L_800799A8
.L_8007997C:
    addis    r4, r3, 0x1
    li       r0, 0x5
    stw      r0, -0x6a30(r4)
    b        .L_800799A8
.L_8007998C:
    addis    r4, r3, 0x1
    li       r0, 0x6
    stw      r0, -0x6a30(r4)
    b        .L_800799A8
.L_8007999C:
    addis    r4, r3, 0x1
    li       r0, 0x7
    stw      r0, -0x6a30(r4)
.L_800799A8:
    cmpwi    r5, 0x17
    bge      .L_800799C8
    cmpwi    r5, 0x10
    beq      .L_800799D4
    blt      .L_8007A474
    cmpwi    r5, 0x15
    bge      .L_800799D4
    b        .L_8007A474
.L_800799C8:
    cmpwi    r5, 0x38
    beq      .L_800799D4
    b        .L_8007A474
.L_800799D4:
    bl       LoadPlayerAttachmentAnimation
    cmpwi    r3, 0x0
    bne      .L_8007A474
    stw      r28, 0x4(r31)
    li       r30, 0x1
    b        .L_8007A474
.L_800799EC:
    mr       r4, r23
    mr       r5, r24
    mr       r6, r23
    addi     r3, r29, 0x9cc
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80079A54
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80079A40
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80079A40
.L_80079A38:
    addi     r3, r3, 0x8
    bdnz     .L_80079A38
.L_80079A40:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80079A54
.L_80079A50:
    bdnz     .L_80079A50
.L_80079A54:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_8007A474
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80079A88
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80079A88
.L_80079A80:
    addi     r3, r3, 0x8
    bdnz     .L_80079A80
.L_80079A88:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007A474
.L_80079A98:
    bdnz     .L_80079A98
    b        .L_8007A474
.L_80079AA0:
    lwz      r0, 0x40(r1)
    li       r3, 0x0
    lwz      r24, 0x44(r1)
    li       r4, 0x1
    extsh    r26, r0
    mr       r5, r24
    bl       __shl2i
    cmpwi    r26, 0x2710
    mr       r27, r4
    mr       r28, r3
    blt      .L_80079B7C
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80079B64
.L_80079AE4:
    lha      r0, 0x4(r3)
    cmpw     r0, r26
    bne      .L_80079B58
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r25, -0x67a0(r3)
    cmplwi   r25, 0x0
    beq      .L_80079B50
    lwz      r0, 0x228(r25)
    li       r4, 0x0
    lwz      r5, 0x22c(r25)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_80079B50
    b        .L_80079B68
.L_80079B50:
    li       r25, 0x0
    b        .L_80079B68
.L_80079B58:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_80079AE4
.L_80079B64:
    li       r25, 0x0
.L_80079B68:
    cmplwi   r25, 0x0
    bne      .L_80079BB4
    li       r25, 0x0
    b        .L_80079BB4
    b        .L_80079BB4
.L_80079B7C:
    extsh.   r0, r26
    blt      .L_80079BA0
    lis      r3, gGame@ha
    slwi     r0, r26, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r25, -0x67a0(r3)
    b        .L_80079BB4
.L_80079BA0:
    cmpwi    r26, -0x1
    bne      .L_80079BB0
    lwz      r25, 0x8(r31)
    b        .L_80079BB4
.L_80079BB0:
    mr       r25, r26
.L_80079BB4:
    cmplwi   r25, 0x0
    beq      .L_80079CF0
    lwz      r6, 0x22c(r25)
    lis      r0, 0x2
    nor      r5, r27, r27
    li       r4, 0x0
    and      r3, r27, r0
    lwz      r7, 0x228(r25)
    and      r5, r6, r5
    and      r0, r28, r4
    nor      r6, r28, r28
    xor      r3, r3, r4
    xor      r0, r0, r4
    stw      r5, 0x22c(r25)
    or.      r0, r3, r0
    and      r3, r7, r6
    stw      r3, 0x228(r25)
    beq      .L_80079C18
    mr       r3, r25
    li       r4, 0x0
    lwz      r12, 0x34(r25)
    li       r5, 0x0
    lwz      r12, 0xdc(r12)
    mtctr    r12
    bctrl
.L_80079C18:
    li       r4, 0x0
    li       r0, 0x4000
    and      r3, r27, r4
    and      r0, r28, r0
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_8007A474
    lwz      r5, 0x240(r25)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    cmplwi   r5, 0x0
    lwz      r3, -0x6944(r3)
    beq      .L_80079C60
    beq      .L_80079C64
    lha      r4, 0x14(r5)
    b        .L_80079C64
.L_80079C60:
    li       r4, -0x1
.L_80079C64:
    extsh    r0, r4
    cmpwi    r0, 0x8
    bne      .L_8007A474
    lwz      r0, 0xa64(r3)
    cmplw    r25, r0
    bne      .L_8007A474
    cmplwi   r5, 0x0
    beq      .L_80079C98
    beq      .L_80079C90
    lha      r0, 0x16(r5)
    b        .L_80079C9C
.L_80079C90:
    li       r0, 0x0
    b        .L_80079C9C
.L_80079C98:
    li       r0, -0x1
.L_80079C9C:
    extsh    r0, r0
    cmpwi    r0, 0x17
    bge      .L_80079CC0
    cmpwi    r0, 0x10
    beq      .L_80079CCC
    blt      .L_8007A474
    cmpwi    r0, 0x15
    bge      .L_80079CCC
    b        .L_8007A474
.L_80079CC0:
    cmpwi    r0, 0x38
    beq      .L_80079CCC
    b        .L_8007A474
.L_80079CCC:
    addis    r3, r3, 0x1
    li       r0, 0x0
    stw      r0, -0x6a30(r3)
    lwz      r12, 0x5950(r3)
    lwz      r12, 0x18(r12)
    mtctr    r12
    addi     r3, r3, 0x58d4
    bctrl
    b        .L_8007A474
.L_80079CF0:
    mr       r4, r26
    mr       r5, r24
    mr       r6, r26
    addi     r3, r29, 0xa04
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80079D58
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80079D44
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80079D44
.L_80079D3C:
    addi     r3, r3, 0x8
    bdnz     .L_80079D3C
.L_80079D44:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80079D58
.L_80079D54:
    bdnz     .L_80079D54
.L_80079D58:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_8007A474
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80079D8C
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80079D8C
.L_80079D84:
    addi     r3, r3, 0x8
    bdnz     .L_80079D84
.L_80079D8C:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007A474
.L_80079D9C:
    bdnz     .L_80079D9C
    b        .L_8007A474
.L_80079DA4:
    lwz      r0, 0x40(r1)
    li       r3, 0x0
    lwz      r24, 0x44(r1)
    li       r4, 0x1
    extsh    r25, r0
    mr       r5, r24
    bl       __shl2i
    cmpwi    r25, 0x2710
    blt      .L_80079E78
    lis      r5, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r5)
    li       r7, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80079E60
.L_80079DE0:
    lha      r0, 0x4(r5)
    cmpw     r0, r25
    bne      .L_80079E54
    lis      r5, gEventObjectIds@ha
    lis      r6, gGame@ha
    addi     r5, r5, gEventObjectIds@l
    slwi     r0, r7, 2
    add      r5, r5, r0
    addi     r6, r6, gGame@l
    lha      r0, 0x6(r5)
    addis    r5, r6, 0x1
    slwi     r0, r0, 2
    add      r5, r5, r0
    lwz      r8, -0x67a0(r5)
    cmplwi   r8, 0x0
    beq      .L_80079E4C
    lwz      r0, 0x228(r8)
    li       r6, 0x0
    lwz      r7, 0x22c(r8)
    li       r5, 0x1
    and      r0, r0, r6
    and      r5, r7, r5
    xor      r5, r5, r6
    xor      r0, r0, r6
    or.      r0, r5, r0
    beq      .L_80079E4C
    b        .L_80079E64
.L_80079E4C:
    li       r8, 0x0
    b        .L_80079E64
.L_80079E54:
    addi     r5, r5, 0x4
    addi     r7, r7, 0x1
    bdnz     .L_80079DE0
.L_80079E60:
    li       r8, 0x0
.L_80079E64:
    cmplwi   r8, 0x0
    bne      .L_80079EB0
    li       r8, 0x0
    b        .L_80079EB0
    b        .L_80079EB0
.L_80079E78:
    extsh.   r0, r25
    blt      .L_80079E9C
    lis      r5, gGame@ha
    slwi     r0, r25, 2
    addi     r5, r5, gGame@l
    addis    r5, r5, 0x1
    add      r5, r5, r0
    lwz      r8, -0x67a0(r5)
    b        .L_80079EB0
.L_80079E9C:
    cmpwi    r25, -0x1
    bne      .L_80079EAC
    lwz      r8, 0x8(r31)
    b        .L_80079EB0
.L_80079EAC:
    mr       r8, r25
.L_80079EB0:
    cmplwi   r8, 0x0
    beq      .L_80079EE8
    lwz      r0, 0x228(r8)
    li       r5, 0x0
    lwz      r6, 0x22c(r8)
    and      r0, r0, r3
    and      r3, r6, r4
    xor      r3, r3, r5
    xor      r0, r0, r5
    or.      r0, r3, r0
    beq      .L_80079EE0
    li       r5, 0x1
.L_80079EE0:
    stw      r5, 0x10(r31)
    b        .L_8007A474
.L_80079EE8:
    mr       r4, r25
    mr       r5, r24
    mr       r6, r25
    addi     r3, r29, 0xa3c
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_80079F50
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80079F3C
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80079F3C
.L_80079F34:
    addi     r3, r3, 0x8
    bdnz     .L_80079F34
.L_80079F3C:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_80079F50
.L_80079F4C:
    bdnz     .L_80079F4C
.L_80079F50:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_8007A474
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_80079F84
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_80079F84
.L_80079F7C:
    addi     r3, r3, 0x8
    bdnz     .L_80079F7C
.L_80079F84:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007A474
.L_80079F94:
    bdnz     .L_80079F94
    b        .L_8007A474
.L_80079F9C:
    lwz      r3, 0x40(r1)
    lwz      r0, 0x44(r1)
    extsh    r3, r3
    clrlwi   r4, r0, 16
    bl       SetItemFlags
    b        .L_8007A474
.L_80079FB4:
    lwz      r3, 0x40(r1)
    lwz      r0, 0x44(r1)
    extsh    r3, r3
    clrlwi   r4, r0, 16
    bl       ClearItemFlags
    b        .L_8007A474
.L_80079FCC:
    lwz      r3, 0x40(r1)
    lwz      r0, 0x44(r1)
    extsh    r3, r3
    clrlwi   r4, r0, 16
    bl       GetItemFlagsMasked
    clrlwi   r0, r3, 16
    stw      r0, 0x10(r31)
    b        .L_8007A474
.L_80079FEC:
    lwz      r5, 0x40(r1)
    li       r3, 0x0
    li       r4, 0x1
    bl       __shl2i
    lis      r5, gGame@ha
    addi     r6, r5, gGame@l
    lwz      r0, 0x1b10(r6)
    lwz      r5, 0x1b14(r6)
    or       r0, r0, r3
    or       r3, r5, r4
    stw      r0, 0x1b10(r6)
    stw      r3, 0x1b14(r6)
    b        .L_8007A474
.L_8007A020:
    lwz      r5, 0x40(r1)
    li       r3, 0x0
    li       r4, 0x1
    bl       __shl2i
    lis      r5, gGame@ha
    nor      r6, r4, r4
    addi     r4, r5, gGame@l
    nor      r5, r3, r3
    lwz      r0, 0x1b10(r4)
    lwz      r3, 0x1b14(r4)
    and      r0, r0, r5
    and      r3, r3, r6
    stw      r0, 0x1b10(r4)
    stw      r3, 0x1b14(r4)
    b        .L_8007A474
.L_8007A05C:
    lwz      r5, 0x40(r1)
    li       r3, 0x0
    li       r4, 0x1
    bl       __shl2i
    lis      r6, gGame@ha
    li       r5, 0x0
    addi     r6, r6, gGame@l
    lwz      r0, 0x1b10(r6)
    lwz      r6, 0x1b14(r6)
    and      r0, r0, r3
    and      r3, r6, r4
    xor      r3, r3, r5
    xor      r0, r0, r5
    or.      r0, r3, r0
    beq      .L_8007A09C
    li       r5, 0x1
.L_8007A09C:
    stw      r5, 0x10(r31)
    b        .L_8007A474
.L_8007A0A4:
    lis      r4, gGame@ha
    lis      r3, 0x1
    addi     r5, r4, gGame@l
    lwz      r4, 0x1b10(r5)
    addi     r0, r3, -0x8000
    or       r0, r4, r0
    stw      r0, 0x1b10(r5)
    b        .L_8007A474
.L_8007A0C4:
    lis      r4, gGame@ha
    lis      r3, 0xffff
    addi     r7, r4, gGame@l
    li       r4, -0x1
    lwz      r6, 0x1b14(r7)
    addi     r0, r3, 0x7fff
    lwz      r5, 0x1b10(r7)
    and      r3, r6, r4
    and      r0, r5, r0
    stw      r3, 0x1b14(r7)
    stw      r0, 0x1b10(r7)
    b        .L_8007A474
.L_8007A0F4:
    lis      r3, gGame@ha
    lis      r0, 0x1
    addi     r5, r3, gGame@l
    lwz      r3, 0x1b10(r5)
    or       r0, r3, r0
    stw      r0, 0x1b10(r5)
    b        .L_8007A474
.L_8007A110:
    lis      r4, gGame@ha
    lis      r3, 0xffff
    addi     r7, r4, gGame@l
    li       r4, -0x1
    lwz      r6, 0x1b14(r7)
    subi     r0, r3, 0x1
    lwz      r5, 0x1b10(r7)
    and      r3, r6, r4
    and      r0, r5, r0
    stw      r3, 0x1b14(r7)
    stw      r0, 0x1b10(r7)
    b        .L_8007A474
.L_8007A140:
    lis      r3, gGame@ha
    addi     r4, r3, gGame@l
    lwz      r0, 0x1b10(r4)
    ori      r0, r0, 0x20
    stw      r0, 0x1b10(r4)
    b        .L_8007A474
.L_8007A158:
    lis      r4, gGame@ha
    li       r3, -0x1
    addi     r6, r4, gGame@l
    li       r0, -0x21
    lwz      r4, 0x1b10(r6)
    lwz      r5, 0x1b14(r6)
    and      r0, r4, r0
    and      r3, r5, r3
    stw      r0, 0x1b10(r6)
    stw      r3, 0x1b14(r6)
    b        .L_8007A474
.L_8007A184:
    lis      r3, gGame@ha
    li       r4, -0x1
    addi     r8, r3, gGame@l
    li       r0, -0x21
    addis    r3, r8, 0x1
    lwz      r5, 0x1b10(r8)
    lwz      r6, 0x1b14(r8)
    mr       r9, r8
    and      r0, r5, r0
    lwz      r5, -0x57a0(r3)
    and      r3, r6, r4
    stw      r0, 0x1b10(r8)
    stw      r3, 0x1b14(r8)
    mtctr    r5
    cmpwi    r5, 0x0
    ble      .L_8007A474
    lis      r4, 0x2000
    li       r7, 0x1
    li       r6, 0x0
.L_8007A1D0:
    addis    r3, r9, 0x1
    lwz      r3, -0x67a0(r3)
    lwz      r0, 0x228(r3)
    lwz      r10, 0x22c(r3)
    and      r5, r0, r6
    and      r0, r10, r7
    xor      r3, r0, r6
    xor      r0, r5, r6
    or.      r0, r3, r0
    beq      .L_8007A21C
    and      r3, r10, r4
    xor      r0, r5, r6
    xor      r3, r3, r6
    or.      r0, r3, r0
    beq      .L_8007A21C
    lwz      r0, 0x1b10(r8)
    ori      r0, r0, 0x20
    stw      r0, 0x1b10(r8)
    b        .L_8007A474
.L_8007A21C:
    addi     r9, r9, 0x4
    bdnz     .L_8007A1D0
    b        .L_8007A474
.L_8007A228:
    lis      r3, gCameraAreaState@ha
    lwz      r0, 0x40(r1)
    addi     r4, r3, gCameraAreaState@l
    lis      r3, __vt__11CamAreaInfo@ha
    lwz      r5, 0x20(r4)
    extsh    r6, r0
    addi     r3, r3, __vt__11CamAreaInfo@l
    lwz      r4, 0x1c(r4)
    mtctr    r5
    cmpwi    r5, 0x0
    ble      .L_8007A474
.L_8007A254:
    addic.   r0, r1, 0x10
    stw      r3, 0x10(r1)
    stw      r4, 0x14(r1)
    beq      .L_8007A268
    stw      r3, 0x10(r1)
.L_8007A268:
    lwz      r5, 0x14(r1)
    lha      r0, 0x2c(r5)
    cmpw     r6, r0
    bne      .L_8007A284
    lhz      r0, 0x2e(r5)
    rlwinm   r0, r0, 0, 25, 23
    sth      r0, 0x2e(r5)
.L_8007A284:
    addi     r4, r4, 0x34
    bdnz     .L_8007A254
    b        .L_8007A474
.L_8007A290:
    lis      r3, gCameraAreaState@ha
    lwz      r0, 0x40(r1)
    addi     r4, r3, gCameraAreaState@l
    lis      r3, __vt__11CamAreaInfo@ha
    lwz      r5, 0x20(r4)
    extsh    r6, r0
    addi     r3, r3, __vt__11CamAreaInfo@l
    lwz      r4, 0x1c(r4)
    mtctr    r5
    cmpwi    r5, 0x0
    ble      .L_8007A474
.L_8007A2BC:
    addic.   r0, r1, 0x8
    stw      r3, 0x8(r1)
    stw      r4, 0xc(r1)
    beq      .L_8007A2D0
    stw      r3, 0x8(r1)
.L_8007A2D0:
    lwz      r5, 0xc(r1)
    lha      r0, 0x2c(r5)
    cmpw     r6, r0
    bne      .L_8007A2EC
    lhz      r0, 0x2e(r5)
    ori      r0, r0, 0x80
    sth      r0, 0x2e(r5)
.L_8007A2EC:
    addi     r4, r4, 0x34
    bdnz     .L_8007A2BC
    b        .L_8007A474
.L_8007A2F8:
    lwz      r3, 0x40(r1)
    bl       fn_8006E2D4
    extsh    r0, r3
    stw      r0, 0x10(r31)
    b        .L_8007A474
.L_8007A30C:
    lwz      r3, 0x40(r1)
    bl       fn_8006E28C
    extsh    r0, r3
    stw      r0, 0x10(r31)
    b        .L_8007A474
.L_8007A320:
    cmpwi    r24, 0x0
    ble      .L_8007A344
    lwz      r3, 0x40(r1)
    subi     r4, r24, 0x1
    addi     r5, r1, 0x44
    addi     r3, r3, 0x7
    bl       CallStageModule
    stw      r3, 0x10(r31)
    b        .L_8007A474
.L_8007A344:
    lis      r3, sEvtCommandCALLName@ha
    addi     r6, r29, 0xa74
    addi     r7, r3, sEvtCommandCALLName@l
    li       r4, 0x86d
    li       r3, lbl_8065CFA0@sda21
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_8007A474
.L_8007A368:
    lis      r3, gGame@ha
    lwz      r4, 0x40(r1)
    addi     r3, r3, gGame@l
    lwz      r5, 0x44(r1)
    addis    r3, r3, 0x1
    lwz      r3, -0x7a7c(r3)
    bl       AddMainMenuExtraRow
    b        .L_8007A474
.L_8007A388:
    cmplwi   r26, 0x0
    beq      .L_8007A474
    lwz      r0, 0x228(r26)
    li       r4, 0x0
    lwz      r5, 0x22c(r26)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_8007A474
    mr       r3, r26
    mr       r4, r24
    lwz      r12, 0x34(r26)
    addi     r5, r1, 0x40
    lwz      r12, 0x124(r12)
    mtctr    r12
    bctrl
    lwz      r4, 0x240(r26)
    cmplwi   r4, 0x0
    beq      .L_8007A3F4
    beq      .L_8007A3EC
    lha      r0, 0x14(r4)
    b        .L_8007A3F8
.L_8007A3EC:
    li       r0, 0x0
    b        .L_8007A3F8
.L_8007A3F4:
    li       r0, -0x1
.L_8007A3F8:
    extsh    r0, r0
    cmpwi    r0, 0x2
    bne      .L_8007A428
    cmpwi    r3, -0x1
    bne      .L_8007A420
    lwz      r0, 0x40(r1)
    cmpwi    r0, 0x65
    beq      .L_8007A420
    li       r30, 0x1
    b        .L_8007A42C
.L_8007A420:
    stw      r3, 0x10(r31)
    b        .L_8007A42C
.L_8007A428:
    stw      r3, 0x10(r31)
.L_8007A42C:
    li       r4, 0x0
    cmpw     r4, r24
    bge      .L_8007A474
    cmpwi    r24, 0x8
    subi     r3, r24, 0x8
    ble      .L_8007A460
    addi     r0, r3, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmpwi    r3, 0x0
    ble      .L_8007A460
.L_8007A458:
    addi     r4, r4, 0x8
    bdnz     .L_8007A458
.L_8007A460:
    subf     r0, r4, r24
    mtctr    r0
    cmpw     r4, r24
    bge      .L_8007A474
.L_8007A470:
    bdnz     .L_8007A470
.L_8007A474:
    mr       r3, r30
    lmw      r23, 0xdc(r1)
    lwz      r0, 0x104(r1)
    mtlr     r0
    addi     r1, r1, 0x100
    blr
*/

#if defined(VERSION_JP)
#define XOBJ_80C 0x804
#else
#define XOBJ_80C 0x80C // BORN's last arg goes here
#endif

// null when the pool is full
static inline XObj* CEvt_Born(s32 index)
{
	CoordInfo info(GetSpawnData(index));
	return (XObj*)SpawnObjectFromPlacement(&gGame.mXObjs, &info, 1, FALSE);
}

/**
 * @note Address: 0x8007A48C
 * @note Size: 0x4C4
 */
s32 CEvt::_cmd_BORN(u8, s32)
{
	s16 id    = CEvt_Evaluate(this);
	s16 type  = CEvt_Evaluate(this);
	f32 x     = CEvt_Evaluate(this) / 100.0f;
	f32 y     = CEvt_Evaluate(this) / 100.0f;
	f32 z     = CEvt_Evaluate(this) / 100.0f;
	s16 rot   = CEvt_Evaluate(this);
	f32 sx    = CEvt_Evaluate(this) / 100.0f;
	f32 sy    = CEvt_Evaluate(this) / 100.0f;
	f32 sz    = CEvt_Evaluate(this) / 100.0f;
	s32 arg   = CEvt_Evaluate(this);
	XObj* obj = CEvt_FindObj(id);
	if (obj == NULL) {
		obj = CEvt_Born(type + 0x16);
		if (obj != NULL) {
			gGame.mStatus.mFlags |= 0x1000; // BORN raises it after a spawn
		}
	}
	if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		obj->SetPos(x, y, z, 0);
		obj->SetRotationY(DEG2RAD * rot, 0);
		obj->SetScale(sx, sy, sz, 0);
		UpdateObjectFloorHeights(obj);
		UpdateObjectFloorXZExtent(obj);
		*(s32*)((u8*)obj + XOBJ_80C) = arg;
		s16 index                    = FindStageObjectIndex(&gGame.mXObjs, (CXObj*)obj);
		if (CEvt_FindObj(id) == NULL) {
			gEventObjectIds.mEntry[gEventObjectIds.mCount].mId    = id;
			gEventObjectIds.mEntry[gEventObjectIds.mCount].mIndex = index;
			gEventObjectIds.mCount++;
		}
	} else {
		OSPanic(__FILE__, 1705, "%s%s", FormatToStaticBuffer("No more space for born(%d,...)-command!\n", id), __FUNCTION__);
	}
	return 0;
}

// CEvt::_cmd_BORN
/*
    stwu     r1, -0xc0(r1)
    mflr     r0
    stw      r0, 0xc4(r1)
    stfd     f31, 0xb0(r1)
    psq_st   f31, 0xb8(r1), 0, qr0
    stfd     f30, 0xa0(r1)
    psq_st   f30, 0xa8(r1), 0, qr0
    stfd     f29, 0x90(r1)
    psq_st   f29, 0x98(r1), 0, qr0
    stfd     f28, 0x80(r1)
    psq_st   f28, 0x88(r1), 0, qr0
    stfd     f27, 0x70(r1)
    psq_st   f27, 0x78(r1), 0, qr0
    stfd     f26, 0x60(r1)
    psq_st   f26, 0x68(r1), 0, qr0
    stmw     r26, 0x48(r1)
    mr       r26, r3
    bl       CEvt_Evaluate
    extsh    r29, r3
    mr       r3, r26
    bl       CEvt_Evaluate
    extsh    r28, r3
    mr       r3, r26
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x14(r1)
    mr       r3, r26
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x10(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x10(r1)
    fsubs    f1, f1, f2
    fdivs    f31, f1, f0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x1c(r1)
    mr       r3, r26
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x18(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x18(r1)
    fsubs    f1, f1, f2
    fdivs    f30, f1, f0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x24(r1)
    mr       r3, r26
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x20(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x20(r1)
    fsubs    f1, f1, f2
    fdivs    f29, f1, f0
    bl       CEvt_Evaluate
    extsh    r27, r3
    mr       r3, r26
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x2c(r1)
    mr       r3, r26
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x28(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x28(r1)
    fsubs    f1, f1, f2
    fdivs    f28, f1, f0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x34(r1)
    mr       r3, r26
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x30(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x30(r1)
    fsubs    f1, f1, f2
    fdivs    f27, f1, f0
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x3c(r1)
    mr       r3, r26
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x38(r1)
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x38(r1)
    fsubs    f1, f1, f2
    fdivs    f26, f1, f0
    bl       CEvt_Evaluate
    lis      r4, gEventObjectIds@ha
    mr       r31, r3
    addi     r30, r4, gEventObjectIds@l
    li       r5, 0x0
    lwz      r0, 0x0(r30)
    mr       r3, r30
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_8007A6A4
.L_8007A624:
    lha      r0, 0x4(r3)
    cmpw     r0, r29
    bne      .L_8007A698
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r6, -0x67a0(r3)
    cmplwi   r6, 0x0
    beq      .L_8007A690
    lwz      r0, 0x228(r6)
    li       r4, 0x0
    lwz      r5, 0x22c(r6)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_8007A690
    b        .L_8007A6A8
.L_8007A690:
    li       r6, 0x0
    b        .L_8007A6A8
.L_8007A698:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_8007A624
.L_8007A6A4:
    li       r6, 0x0
.L_8007A6A8:
    cmplwi   r6, 0x0
    mr       r26, r6
    bne      .L_8007A718
    addi     r3, r28, 0x16
    bl       GetSpawnData
    lis      r5, __vt__9CoordInfo@ha
    lis      r4, gGame@ha
    addi     r0, r5, __vt__9CoordInfo@l
    stw      r3, 0x8(r1)
    addi     r3, r4, gGame@l
    addi     r4, r1, 0x8
    addis    r3, r3, 0x1
    stw      r0, 0xc(r1)
    li       r5, 0x1
    li       r6, 0x0
    subi     r3, r3, 0x696c
    bl       SpawnObjectFromPlacement
    lis      r4, __vt__9CoordInfo@ha
    cmplwi   r3, 0x0
    addi     r0, r4, __vt__9CoordInfo@l
    mr       r26, r3
    stw      r0, 0xc(r1)
    beq      .L_8007A718
    lis      r3, gGame@ha
    addi     r4, r3, gGame@l
    lwz      r0, 0x1b14(r4)
    ori      r0, r0, 0x1000
    stw      r0, 0x1b14(r4)
.L_8007A718:
    cmplwi   r26, 0x0
    beq      .L_8007A8D4
    lwz      r0, 0x228(r26)
    li       r4, 0x0
    lwz      r5, 0x22c(r26)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_8007A8D4
    mr       r3, r26
    fmr      f1, f31
    lwz      r12, 0x34(r26)
    fmr      f2, f30
    fmr      f3, f29
    li       r4, 0x0
    lwz      r12, 0x68(r12)
    mtctr    r12
    bctrl
    xoris    r3, r27, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x3c(r1)
    mr       r3, r26
    lwz      r12, 0x34(r26)
    li       r4, 0x0
    stw      r0, 0x38(r1)
    lfd      f1, lbl_8065CF88@sda21(r0)
    lfd      f0, 0x38(r1)
    lfs      f2, lbl_8065CFE8@sda21(r0)
    fsubs    f0, f0, f1
    lwz      r12, 0x28(r12)
    fmuls    f1, f2, f0
    mtctr    r12
    bctrl
    mr       r3, r26
    fmr      f1, f28
    lwz      r12, 0x34(r26)
    fmr      f2, f27
    fmr      f3, f26
    li       r4, 0x0
    lwz      r12, 0x3c(r12)
    mtctr    r12
    bctrl
    mr       r3, r26
    bl       UpdateObjectFloorHeights
    mr       r3, r26
    bl       UpdateObjectFloorXZExtent
    lis      r3, gGame@ha
    stw      r31, 0x80c(r26)
    addi     r3, r3, gGame@l
    mr       r4, r26
    addis    r3, r3, 0x1
    subi     r3, r3, 0x696c
    bl       FindStageObjectIndex
    lis      r4, gEventObjectIds@ha
    extsh    r0, r3
    lwz      r8, gEventObjectIds@l(r4)
    li       r6, 0x0
    mtctr    r8
    cmpwi    r8, 0x0
    ble      .L_8007A894
.L_8007A814:
    lha      r3, 0x4(r30)
    cmpw     r3, r29
    bne      .L_8007A888
    lis      r3, gEventObjectIds@ha
    lis      r5, gGame@ha
    addi     r4, r3, gEventObjectIds@l
    slwi     r3, r6, 2
    add      r3, r4, r3
    addi     r4, r5, gGame@l
    lha      r3, 0x6(r3)
    addis    r4, r4, 0x1
    slwi     r3, r3, 2
    add      r3, r4, r3
    lwz      r7, -0x67a0(r3)
    cmplwi   r7, 0x0
    beq      .L_8007A880
    lwz      r3, 0x228(r7)
    li       r5, 0x0
    lwz      r6, 0x22c(r7)
    li       r4, 0x1
    and      r3, r3, r5
    and      r4, r6, r4
    xor      r4, r4, r5
    xor      r3, r3, r5
    or.      r3, r4, r3
    beq      .L_8007A880
    b        .L_8007A898
.L_8007A880:
    li       r7, 0x0
    b        .L_8007A898
.L_8007A888:
    addi     r30, r30, 0x4
    addi     r6, r6, 0x1
    bdnz     .L_8007A814
.L_8007A894:
    li       r7, 0x0
.L_8007A898:
    cmplwi   r7, 0x0
    bne      .L_8007A908
    lis      r4, gEventObjectIds@ha
    slwi     r3, r8, 2
    addi     r4, r4, gEventObjectIds@l
    add      r3, r4, r3
    sth      r29, 0x4(r3)
    lwz      r3, 0x0(r4)
    slwi     r3, r3, 2
    add      r3, r4, r3
    sth      r0, 0x6(r3)
    lwz      r3, 0x0(r4)
    addi     r0, r3, 0x1
    stw      r0, 0x0(r4)
    b        .L_8007A908
.L_8007A8D4:
    lis      r3, sEvtBornCapacity@ha
    mr       r4, r29
    addi     r3, r3, sEvtBornCapacity@l
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvtCommandBORNName@ha
    mr       r6, r3
    addi     r7, r4, sEvtCommandBORNName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x6a9
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
.L_8007A908:
    li       r3, 0x0
    psq_l    f31, 0xb8(r1), 0, qr0
    lfd      f31, 0xb0(r1)
    psq_l    f30, 0xa8(r1), 0, qr0
    lfd      f30, 0xa0(r1)
    psq_l    f29, 0x98(r1), 0, qr0
    lfd      f29, 0x90(r1)
    psq_l    f28, 0x88(r1), 0, qr0
    lfd      f28, 0x80(r1)
    psq_l    f27, 0x78(r1), 0, qr0
    lfd      f27, 0x70(r1)
    psq_l    f26, 0x68(r1), 0, qr0
    lfd      f26, 0x60(r1)
    lmw      r26, 0x48(r1)
    lwz      r0, 0xc4(r1)
    mtlr     r0
    addi     r1, r1, 0xc0
    blr
*/

static inline void EvtQueue_Push(u8* script, u32 owner, s16 useValue, s16 hitValue)
{
	s32 cursor = gEventScriptQueue.mRead;
	for (s32 i = 0; i < 128; i++, cursor++) {
		s32 index = cursor % 128;
		if (gEventScriptQueue.mScript[index] == NULL) {
			gEventScriptQueue.mScript[index]   = script;
			gEventScriptQueue.mContext[index]  = owner;
			gEventScriptQueue.mUseValue[index] = useValue;
			gEventScriptQueue.mHitValue[index] = hitValue;
			break;
		}
	}
}

static inline void CEvt_Attach(CEvt* evt, u8* script, u32 owner, s16 useValue = -1, s16 hitValue = -1)
{
	if (script == NULL) {
		return;
	}
	if (evt->mData.mScript != NULL && evt->_00 == 0) {
		EvtQueue_Push(script, owner, useValue, hitValue);
		return;
	}
	evt->mData.mScript  = NULL;
	evt->_18            = 0;
	evt->_1C            = 0;
	evt->_20            = 0;
	evt->mCalls.mCount  = 0;
	evt->mFrames.mCount = 0;
	evt->mValues.mCount = 0;
	if (evt->_00 == 0) {
		gEventScriptQueue.mRead = 0;
		for (u32 i = 0; i < 128; i++) {
			gEventScriptQueue.mScript[i] = NULL;
		}
	}
	evt->_10           = 0;
	evt->_14           = 0;
	evt->_58           = 0;
	evt->mUseValue     = -1;
	evt->mHitValue     = -1;
	evt->mData.mScript = script;
	if (evt->_00 != 0)
		owner = evt->_00;
	evt->mData._04 = owner;
	evt->mUseValue = useValue;
	evt->mHitValue = hitValue;
}

/**
 * @note Address: 0x8007A950
 * @note Size: 0x50C
 */
s32 CEvt::_cmd_ATTACH(u8 command, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	if (command == EVT_CMD_ATTACH) {
		u8* script = (u8*)CEvt_Evaluate(this);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			CEvt_Attach(&obj->mEvt, script, _00);
		} else {
			CEvt_Warn(FormatToStaticBuffer("attach(%d, ...): obj(%d) killed!", id, id));
		}
	} else if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		obj->mEvt.mData.mScript = NULL;
		if (obj->mEvt._54 != NULL && obj->mEvt._58 != 0) {
			obj->mEvt._58                  = 0;
			EVT_OWNER_445C0(obj->mEvt._54) = 1;
		}
		Inventory* inventory = &EVT_PLAYER.mParams.mInventory;
		inventory->mCompact  = TRUE;
		CompactInventory(inventory);
	} else {
		CEvt_Warn(FormatToStaticBuffer("detach(%d, ...): obj(%d) killed!", id, id));
	}
	return 0;
}
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stw      r31, 0x1c(r1)
    stw      r30, 0x18(r1)
    stw      r29, 0x14(r1)
    mr       r29, r4
    stw      r28, 0x10(r1)
    mr       r28, r3
    bl       CEvt_Evaluate
    extsh    r30, r3
    cmpwi    r30, 0x2710
    blt      .L_8007AA34
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_8007AA1C
.L_8007A99C:
    lha      r0, 0x4(r3)
    cmpw     r0, r30
    bne      .L_8007AA10
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r31, -0x67a0(r3)
    cmplwi   r31, 0x0
    beq      .L_8007AA08
    lwz      r0, 0x228(r31)
    li       r4, 0x0
    lwz      r5, 0x22c(r31)
    li       r3, 0x1
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_8007AA08
    b        .L_8007AA20
.L_8007AA08:
    li       r31, 0x0
    b        .L_8007AA20
.L_8007AA10:
    addi     r3, r3, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_8007A99C
.L_8007AA1C:
    li       r31, 0x0
.L_8007AA20:
    cmplwi   r31, 0x0
    bne      .L_8007AA6C
    li       r31, 0x0
    b        .L_8007AA6C
    b        .L_8007AA6C
.L_8007AA34:
    extsh.   r0, r30
    blt      .L_8007AA58
    lis      r3, gGame@ha
    slwi     r0, r30, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r31, -0x67a0(r3)
    b        .L_8007AA6C
.L_8007AA58:
    cmpwi    r30, -0x1
    bne      .L_8007AA68
    lwz      r31, 0x8(r28)
    b        .L_8007AA6C
.L_8007AA68:
    mr       r31, r30
.L_8007AA6C:
    clrlwi   r0, r29, 24
    cmplwi   r0, 0x14
    bne      .L_8007AD04
    mr       r3, r28
    bl       CEvt_Evaluate
    cmplwi   r31, 0x0
    beq      .L_8007AC50
    lwz      r0, 0x228(r31)
    li       r5, 0x0
    lwz      r6, 0x22c(r31)
    li       r4, 0x1
    and      r0, r0, r5
    and      r4, r6, r4
    xor      r4, r4, r5
    xor      r0, r0, r5
    or.      r0, r4, r0
    beq      .L_8007AC50
    cmplwi   r3, 0x0
    lwz      r6, 0x0(r28)
    beq      .L_8007AE38
    lwz      r0, 0x7a8(r31)
    cmplwi   r0, 0x0
    beq      .L_8007AB48
    lwz      r0, 0x7a4(r31)
    cmplwi   r0, 0x0
    bne      .L_8007AB48
    lis      r4, gEventScriptQueue@ha
    li       r0, 0x80
    addi     r5, r4, gEventScriptQueue@l
    lwz      r8, 0x0(r5)
    mtctr    r0
.L_8007AAE8:
    slwi     r0, r8, 25
    srwi     r4, r8, 31
    subf     r0, r4, r0
    rotlwi   r0, r0, 7
    add      r7, r0, r4
    slwi     r9, r7, 2
    add      r4, r5, r9
    lwz      r0, 0x4(r4)
    cmplwi   r0, 0x0
    bne      .L_8007AB3C
    lis      r4, gEventScriptQueue@ha
    slwi     r7, r7, 1
    addi     r5, r4, gEventScriptQueue@l
    li       r0, -0x1
    add      r4, r5, r9
    stw      r3, 0x4(r4)
    add      r3, r5, r7
    stw      r6, 0x208(r4)
    sth      r0, 0x408(r3)
    sth      r0, 0x508(r3)
    b        .L_8007AE38
.L_8007AB3C:
    addi     r8, r8, 0x1
    bdnz     .L_8007AAE8
    b        .L_8007AE38
.L_8007AB48:
    li       r5, 0x0
    stw      r5, 0x7a8(r31)
    stw      r5, 0x7bc(r31)
    stw      r5, 0x7c0(r31)
    stw      r5, 0x7c4(r31)
    stw      r5, 0x7cc(r31)
    stw      r5, 0x7dc(r31)
    stw      r5, 0x7ec(r31)
    lwz      r0, 0x7a4(r31)
    cmplwi   r0, 0x0
    bne      .L_8007AC08
    li       r0, 0x4
    lis      r4, gEventScriptQueue@ha
    stwu     r5, gEventScriptQueue@l(r4)
    mtctr    r0
.L_8007AB84:
    stw      r5, 0x4(r4)
    stw      r5, 0x8(r4)
    stw      r5, 0xc(r4)
    stw      r5, 0x10(r4)
    stw      r5, 0x14(r4)
    stw      r5, 0x18(r4)
    stw      r5, 0x1c(r4)
    stw      r5, 0x20(r4)
    stw      r5, 0x24(r4)
    stw      r5, 0x28(r4)
    stw      r5, 0x2c(r4)
    stw      r5, 0x30(r4)
    stw      r5, 0x34(r4)
    stw      r5, 0x38(r4)
    stw      r5, 0x3c(r4)
    stw      r5, 0x40(r4)
    stw      r5, 0x44(r4)
    stw      r5, 0x48(r4)
    stw      r5, 0x4c(r4)
    stw      r5, 0x50(r4)
    stw      r5, 0x54(r4)
    stw      r5, 0x58(r4)
    stw      r5, 0x5c(r4)
    stw      r5, 0x60(r4)
    stw      r5, 0x64(r4)
    stw      r5, 0x68(r4)
    stw      r5, 0x6c(r4)
    stw      r5, 0x70(r4)
    stw      r5, 0x74(r4)
    stw      r5, 0x78(r4)
    stw      r5, 0x7c(r4)
    stwu     r5, 0x80(r4)
    bdnz     .L_8007AB84
.L_8007AC08:
    li       r4, 0x0
    li       r0, -0x1
    stw      r4, 0x7b4(r31)
    stw      r4, 0x7b8(r31)
    stw      r4, 0x7fc(r31)
    sth      r0, 0x800(r31)
    sth      r0, 0x802(r31)
    stw      r3, 0x7a8(r31)
    lwz      r0, 0x7a4(r31)
    cmplwi   r0, 0x0
    beq      .L_8007AC38
    b        .L_8007AC3C
.L_8007AC38:
    mr       r0, r6
.L_8007AC3C:
    stw      r0, 0x7ac(r31)
    li       r0, -0x1
    sth      r0, 0x800(r31)
    sth      r0, 0x802(r31)
    b        .L_8007AE38
.L_8007AC50:
    lis      r3, sEvtAttachKilled@ha
    mr       r4, r30
    addi     r3, r3, sEvtAttachKilled@l
    mr       r5, r30
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_8007ACB8
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007ACA4
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007ACA4
.L_8007AC9C:
    addi     r3, r3, 0x8
    bdnz     .L_8007AC9C
.L_8007ACA4:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007ACB8
.L_8007ACB4:
    bdnz     .L_8007ACB4
.L_8007ACB8:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_8007AE38
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007ACEC
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007ACEC
.L_8007ACE4:
    addi     r3, r3, 0x8
    bdnz     .L_8007ACE4
.L_8007ACEC:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007AE38
.L_8007ACFC:
    bdnz     .L_8007ACFC
    b        .L_8007AE38
.L_8007AD04:
    cmplwi   r31, 0x0
    beq      .L_8007AD88
    lwz      r0, 0x228(r31)
    li       r4, 0x0
    lwz      r3, 0x22c(r31)
    li       r5, 0x1
    and      r0, r0, r4
    and      r3, r3, r5
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_8007AD88
    stw      r4, 0x7a8(r31)
    lwz      r0, 0x7f8(r31)
    cmplwi   r0, 0x0
    beq      .L_8007AD60
    lwz      r0, 0x7fc(r31)
    cmpwi    r0, 0x0
    beq      .L_8007AD60
    stw      r4, 0x7fc(r31)
    lwz      r3, 0x7f8(r31)
    addis    r3, r3, 0x4
    stw      r5, 0x45c0(r3)
.L_8007AD60:
    lis      r3, gGame@ha
    li       r0, 0x1
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    stw      r0, -0x6578(r3)
    subi     r3, r3, 0x6978
    bl       CompactInventory
    b        .L_8007AE38
.L_8007AD88:
    lis      r3, sEvtDetachKilled@ha
    mr       r4, r30
    addi     r3, r3, sEvtDetachKilled@l
    mr       r5, r30
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_8007ADF0
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007ADDC
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007ADDC
.L_8007ADD4:
    addi     r3, r3, 0x8
    bdnz     .L_8007ADD4
.L_8007ADDC:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007ADF0
.L_8007ADEC:
    bdnz     .L_8007ADEC
.L_8007ADF0:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_8007AE38
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007AE24
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007AE24
.L_8007AE1C:
    addi     r3, r3, 0x8
    bdnz     .L_8007AE1C
.L_8007AE24:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007AE38
.L_8007AE34:
    bdnz     .L_8007AE34
.L_8007AE38:
    lwz      r0, 0x24(r1)
    li       r3, 0x0
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    lwz      r28, 0x10(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

// puts an anim in the first free slot after the one playing
static inline void CEvt_QueueAnim(XObj* obj, s32 anim, u16 frame, s16 rate)
{
	for (int i = 0; i < 16; i++) {
		int index = (obj->mAnmQueue.mNext + i) % 16;
		if (obj->mAnmQueue.mEntry[index].mId == -1) {
			obj->mAnmQueue.mEntry[index].mId    = anim;
			obj->mAnmQueue.mEntry[index].mFrame = frame;
			obj->mAnmQueue.mEntry[index].mRate  = rate;
			return;
		}
	}
}

/**
 * @note Address: 0x8007AE5C
 * @note Size: 0x630
 */
s32 CEvt::_cmd_ANIM2(u8 command, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	for (s32 count = 0;; count++) {
		s32 anim = CEvt_Evaluate(this);
		if (anim == -1) {
			break;
		}
		u16 frame = CEvt_Evaluate(this);
		s16 rate  = CEvt_Evaluate(this);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL) {
				CEvt_Warn(FormatToStaticBuffer("anim2(%d, %d,%d,%d) ... obj(%d) is collision!", id, anim, frame, rate, id));
			} else if (count == 0) {
				obj->mAnmQueue.Clear();
				if (anim & 0xFFFF0000) {
					s32 jointId     = anim & 0xFFFF;
					s32 matId       = anim >> 16;
					void* jointAnim = obj->GetJointAnimation(jointId);
					void* matAnim   = obj->GetMaterialAnimation(matId);
					void* shapeAnim = obj->GetShapeAnimation(jointId);
					obj->SetAnim(0, TRUE, jointAnim, matAnim, shapeAnim, frame, rate / 100.0f, command == EVT_CMD_ANIM1 ? 1 : 0, 10.0f);
					if (jointId >= 1000 || matId >= 1000) {
						obj->mStatus.mFlags |= 0x0008000000000000;
					}
				} else {
					obj->SetAnim(anim, FALSE, NULL, NULL, NULL, frame, rate / 100.0f, 0.0f, 10.0f);
				}
				obj->UpdateAnim(1);
			} else {
				CEvt_QueueAnim(obj, anim, frame, rate);
			}
		} else {
			CEvt_Warn(FormatToStaticBuffer("anim2(%d, %d,%d,%d) ... obj(%d) killed!", id, anim, frame, rate, id));
		}
	}
	return 0;
}
/*
    stwu     r1, -0x50(r1)
    mflr     r0
    stw      r0, 0x54(r1)
    stmw     r21, 0x24(r1)
    mr       r30, r3
    mr       r31, r4
    bl       CEvt_Evaluate
    extsh    r0, r3
    cmpwi    r0, 0x2710
    blt      .L_8007AF38
    lis      r3, gEventObjectIds@ha
    li       r6, 0x0
    addi     r4, r3, gEventObjectIds@l
    lwz      r3, 0x0(r4)
    mtctr    r3
    cmpwi    r3, 0x0
    ble      .L_8007AF20
.L_8007AEA0:
    lha      r3, 0x4(r4)
    cmpw     r3, r0
    bne      .L_8007AF14
    lis      r3, gEventObjectIds@ha
    lis      r5, gGame@ha
    addi     r4, r3, gEventObjectIds@l
    slwi     r3, r6, 2
    add      r3, r4, r3
    addi     r4, r5, gGame@l
    lha      r3, 0x6(r3)
    addis    r4, r4, 0x1
    slwi     r3, r3, 2
    add      r3, r4, r3
    lwz      r29, -0x67a0(r3)
    cmplwi   r29, 0x0
    beq      .L_8007AF0C
    lwz      r3, 0x228(r29)
    li       r5, 0x0
    lwz      r6, 0x22c(r29)
    li       r4, 0x1
    and      r3, r3, r5
    and      r4, r6, r4
    xor      r4, r4, r5
    xor      r3, r3, r5
    or.      r3, r4, r3
    beq      .L_8007AF0C
    b        .L_8007AF24
.L_8007AF0C:
    li       r29, 0x0
    b        .L_8007AF24
.L_8007AF14:
    addi     r4, r4, 0x4
    addi     r6, r6, 0x1
    bdnz     .L_8007AEA0
.L_8007AF20:
    li       r29, 0x0
.L_8007AF24:
    cmplwi   r29, 0x0
    bne      .L_8007AF70
    li       r29, 0x0
    b        .L_8007AF70
    b        .L_8007AF70
.L_8007AF38:
    extsh.   r3, r0
    blt      .L_8007AF5C
    lis      r4, gGame@ha
    slwi     r3, r0, 2
    addi     r4, r4, gGame@l
    addis    r4, r4, 0x1
    add      r3, r4, r3
    lwz      r29, -0x67a0(r3)
    b        .L_8007AF70
.L_8007AF5C:
    cmpwi    r0, -0x1
    bne      .L_8007AF6C
    lwz      r29, 0x8(r30)
    b        .L_8007AF70
.L_8007AF6C:
    mr       r29, r0
.L_8007AF70:
    li       r27, 0x0
    mr       r28, r0
.L_8007AF78:
    mr       r3, r30
    bl       CEvt_Evaluate
    mr       r26, r3
    cmpwi    r26, -0x1
    beq      .L_8007B474
    mr       r3, r30
    bl       CEvt_Evaluate
    clrlwi   r25, r3, 16
    mr       r3, r30
    bl       CEvt_Evaluate
    cmplwi   r29, 0x0
    extsh    r24, r3
    beq      .L_8007B3A8
    lwz      r0, 0x228(r29)
    li       r3, 0x0
    lwz      r5, 0x22c(r29)
    li       r4, 0x1
    and      r0, r0, r3
    and      r4, r5, r4
    xor      r4, r4, r3
    xor      r0, r0, r3
    or.      r0, r4, r0
    beq      .L_8007B3A8
    lwz      r0, 0x0(r29)
    cmplwi   r0, 0x0
    bne      .L_8007B0A8
    lis      r3, sEvtAnim2Collision@ha
    mr       r4, r28
    addi     r3, r3, sEvtAnim2Collision@l
    mr       r5, r26
    mr       r6, r25
    mr       r7, r24
    mr       r8, r28
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_8007B058
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007B040
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007B040
.L_8007B038:
    addi     r3, r3, 0x8
    bdnz     .L_8007B038
.L_8007B040:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007B058
.L_8007B050:
    addi     r3, r3, 0x1
    bdnz     .L_8007B050
.L_8007B058:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_8007B46C
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007B08C
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007B08C
.L_8007B084:
    addi     r3, r3, 0x8
    bdnz     .L_8007B084
.L_8007B08C:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007B46C
.L_8007B09C:
    addi     r3, r3, 0x1
    bdnz     .L_8007B09C
    b        .L_8007B46C
.L_8007B0A8:
    cmpwi    r27, 0x0
    bne      .L_8007B294
    stw      r3, 0x810(r29)
    li       r3, -0x1
    clrrwi.  r0, r26, 16
    stw      r3, 0x814(r29)
    stw      r3, 0x81c(r29)
    stw      r3, 0x824(r29)
    stw      r3, 0x82c(r29)
    stw      r3, 0x834(r29)
    stw      r3, 0x83c(r29)
    stw      r3, 0x844(r29)
    stw      r3, 0x84c(r29)
    stw      r3, 0x854(r29)
    stw      r3, 0x85c(r29)
    stw      r3, 0x864(r29)
    stw      r3, 0x86c(r29)
    stw      r3, 0x874(r29)
    stw      r3, 0x87c(r29)
    stw      r3, 0x884(r29)
    stw      r3, 0x88c(r29)
    beq      .L_8007B210
    mr       r3, r29
    clrlwi   r23, r26, 16
    lwz      r12, 0x34(r29)
    mr       r4, r23
    srawi    r26, r26, 16
    lwz      r12, 0xf0(r12)
    mtctr    r12
    bctrl
    mr       r0, r3
    mr       r3, r29
    lwz      r12, 0x34(r29)
    mr       r22, r0
    mr       r4, r26
    lwz      r12, 0xf4(r12)
    mtctr    r12
    bctrl
    mr       r0, r3
    mr       r3, r29
    lwz      r12, 0x34(r29)
    mr       r21, r0
    mr       r4, r23
    lwz      r12, 0xf8(r12)
    mtctr    r12
    bctrl
    lis      r5, 0x4330
    xoris    r0, r24, 0x8000
    stw      r0, 0x14(r1)
    mr       r4, r3
    clrlwi   r0, r31, 24
    lwz      r12, 0x34(r29)
    stw      r5, 0x10(r1)
    subfic   r0, r0, 0x12
    lfd      f3, lbl_8065CF88@sda21(r0)
    cntlzw   r0, r0
    lfd      f1, 0x10(r1)
    srwi     r0, r0, 5
    lfs      f0, lbl_8065CF80@sda21(r0)
    xoris    r0, r0, 0x8000
    fsubs    f1, f1, f3
    mr       r8, r4
    stw      r25, 0xc(r1)
    mr       r3, r29
    lfd      f4, lbl_8065CFC8@sda21(r0)
    mr       r6, r22
    fdivs    f2, f1, f0
    stw      r5, 0x8(r1)
    lwz      r12, 0x80(r12)
    mr       r7, r21
    lfd      f0, 0x8(r1)
    li       r4, 0x0
    stw      r5, 0x18(r1)
    fsubs    f1, f0, f4
    lfs      f4, lbl_8065CFE4@sda21(r0)
    li       r5, 0x1
    stw      r0, 0x1c(r1)
    lfd      f0, 0x18(r1)
    fsubs    f3, f0, f3
    mtctr    r12
    bctrl
    cmpwi    r23, 0x3e8
    bge      .L_8007B1FC
    cmpwi    r26, 0x3e8
    blt      .L_8007B278
.L_8007B1FC:
    lwz      r3, 0x228(r29)
    lis      r0, 0x8
    or       r0, r3, r0
    stw      r0, 0x228(r29)
    b        .L_8007B278
.L_8007B210:
    lis      r9, 0x4330
    xoris    r0, r24, 0x8000
    stw      r0, 0x14(r1)
    mr       r3, r29
    lwz      r12, 0x34(r29)
    mr       r4, r26
    stw      r9, 0x10(r1)
    li       r5, 0x0
    lfd      f2, lbl_8065CF88@sda21(r0)
    li       r6, 0x0
    lfd      f1, 0x10(r1)
    li       r7, 0x0
    lfs      f0, lbl_8065CF80@sda21(r0)
    li       r8, 0x0
    fsubs    f1, f1, f2
    stw      r25, 0x1c(r1)
    lfd      f5, lbl_8065CFC8@sda21(r0)
    stw      r9, 0x18(r1)
    fdivs    f2, f1, f0
    lwz      r12, 0x80(r12)
    lfd      f0, 0x18(r1)
    lfs      f3, lbl_8065CF74@sda21(r0)
    lfs      f4, lbl_8065CFE4@sda21(r0)
    fsubs    f1, f0, f5
    mtctr    r12
    bctrl
.L_8007B278:
    mr       r3, r29
    li       r4, 0x1
    lwz      r12, 0x34(r29)
    lwz      r12, 0x88(r12)
    mtctr    r12
    bctrl
    b        .L_8007B46C
.L_8007B294:
    li       r0, 0x4
    mtctr    r0
.L_8007B29C:
    lwz      r5, 0x810(r29)
    add      r4, r5, r3
    slwi     r0, r4, 28
    srwi     r4, r4, 31
    subf     r0, r4, r0
    rotlwi   r0, r0, 4
    add      r0, r0, r4
    slwi     r0, r0, 3
    add      r4, r29, r0
    lwz      r0, 0x814(r4)
    cmpwi    r0, -0x1
    bne      .L_8007B2DC
    stw      r26, 0x814(r4)
    sth      r25, 0x818(r4)
    sth      r24, 0x81a(r4)
    b        .L_8007B46C
.L_8007B2DC:
    addi     r3, r3, 0x1
    add      r4, r5, r3
    slwi     r0, r4, 28
    srwi     r4, r4, 31
    subf     r0, r4, r0
    rotlwi   r0, r0, 4
    add      r0, r0, r4
    slwi     r0, r0, 3
    add      r4, r29, r0
    lwz      r0, 0x814(r4)
    cmpwi    r0, -0x1
    bne      .L_8007B31C
    stw      r26, 0x814(r4)
    sth      r25, 0x818(r4)
    sth      r24, 0x81a(r4)
    b        .L_8007B46C
.L_8007B31C:
    addi     r3, r3, 0x1
    add      r4, r5, r3
    slwi     r0, r4, 28
    srwi     r4, r4, 31
    subf     r0, r4, r0
    rotlwi   r0, r0, 4
    add      r0, r0, r4
    slwi     r0, r0, 3
    add      r4, r29, r0
    lwz      r0, 0x814(r4)
    cmpwi    r0, -0x1
    bne      .L_8007B35C
    stw      r26, 0x814(r4)
    sth      r25, 0x818(r4)
    sth      r24, 0x81a(r4)
    b        .L_8007B46C
.L_8007B35C:
    addi     r3, r3, 0x1
    add      r4, r5, r3
    slwi     r0, r4, 28
    srwi     r4, r4, 31
    subf     r0, r4, r0
    rotlwi   r0, r0, 4
    add      r0, r0, r4
    slwi     r0, r0, 3
    add      r4, r29, r0
    lwz      r0, 0x814(r4)
    cmpwi    r0, -0x1
    bne      .L_8007B39C
    stw      r26, 0x814(r4)
    sth      r25, 0x818(r4)
    sth      r24, 0x81a(r4)
    b        .L_8007B46C
.L_8007B39C:
    addi     r3, r3, 0x1
    bdnz     .L_8007B29C
    b        .L_8007B46C
.L_8007B3A8:
    lis      r3, sEvtAnim2Killed@ha
    mr       r4, r28
    addi     r3, r3, sEvtAnim2Killed@l
    mr       r5, r26
    mr       r6, r25
    mr       r7, r24
    mr       r8, r28
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_8007B420
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007B408
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007B408
.L_8007B400:
    addi     r3, r3, 0x8
    bdnz     .L_8007B400
.L_8007B408:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007B420
.L_8007B418:
    addi     r3, r3, 0x1
    bdnz     .L_8007B418
.L_8007B420:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_8007B46C
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007B454
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007B454
.L_8007B44C:
    addi     r3, r3, 0x8
    bdnz     .L_8007B44C
.L_8007B454:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007B46C
.L_8007B464:
    addi     r3, r3, 0x1
    bdnz     .L_8007B464
.L_8007B46C:
    addi     r27, r27, 0x1
    b        .L_8007AF78
.L_8007B474:
    lmw      r21, 0x24(r1)
    li       r3, 0x0
    lwz      r0, 0x54(r1)
    mtlr     r0
    addi     r1, r1, 0x50
    blr
*/

/**
 * @note Address: 0x8007B48C
 * @note Size: 0x5F0
 */
s32 CEvt::_cmd_ANIM(u8 command, s32)
{
	s16 id    = CEvt_Evaluate(this);
	XObj* obj = CEvt_GetObj(this, id);
	for (s32 count = 0;; count++) {
		s32 anim = CEvt_Evaluate(this);
		if (anim == -1) {
			break;
		}
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL) {
				CEvt_Warn(FormatToStaticBuffer("anim(%d, %d) ... obj(%d) is collision!", id, anim, id));
			} else if (count == 0) {
				obj->mAnmQueue.Clear();
				if (anim & 0xFFFF0000) {
					s32 jointId     = anim & 0xFFFF;
					s32 matId       = anim >> 16;
					void* jointAnim = obj->GetJointAnimation(jointId);
					void* matAnim   = obj->GetMaterialAnimation(matId);
					void* shapeAnim = obj->GetShapeAnimation(jointId);
					obj->SetAnim(0, TRUE, jointAnim, matAnim, shapeAnim, 0.0f, 1.0f, command == EVT_CMD_ANIM1 ? 1 : 0, 10.0f);
					if (jointId >= 1000 || matId >= 1000) {
						obj->mStatus.mFlags |= 0x0008000000000000;
					}
				} else {
					obj->SetAnim(anim, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, command == EVT_CMD_ANIM1 ? 1 : 0, 10.0f);
				}
				obj->UpdateAnim(1);
			} else {
				for (s32 i = 0; i < 16; i++) {
					s32 index = (obj->mAnmQueue.mNext + i) % 16;
					if (obj->mAnmQueue.mEntry[index].mId == -1) {
						obj->mAnmQueue.mEntry[index].mId    = anim;
						obj->mAnmQueue.mEntry[index].mFrame = 0;
						obj->mAnmQueue.mEntry[index].mRate  = 100;
						break;
					}
				}
			}
		} else {
			CEvt_Warn(FormatToStaticBuffer("anim(%d, %d) ... obj(%d) killed!", id, anim, id));
		}
	}
	return 0;
}
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    stw      r0, 0x44(r1)
    stmw     r23, 0x1c(r1)
    mr       r30, r3
    mr       r31, r4
    bl       CEvt_Evaluate
    extsh    r0, r3
    cmpwi    r0, 0x2710
    blt      .L_8007B568
    lis      r3, gEventObjectIds@ha
    li       r6, 0x0
    addi     r4, r3, gEventObjectIds@l
    lwz      r3, 0x0(r4)
    mtctr    r3
    cmpwi    r3, 0x0
    ble      .L_8007B550
.L_8007B4D0:
    lha      r3, 0x4(r4)
    cmpw     r3, r0
    bne      .L_8007B544
    lis      r3, gEventObjectIds@ha
    lis      r5, gGame@ha
    addi     r4, r3, gEventObjectIds@l
    slwi     r3, r6, 2
    add      r3, r4, r3
    addi     r4, r5, gGame@l
    lha      r3, 0x6(r3)
    addis    r4, r4, 0x1
    slwi     r3, r3, 2
    add      r3, r4, r3
    lwz      r29, -0x67a0(r3)
    cmplwi   r29, 0x0
    beq      .L_8007B53C
    lwz      r3, 0x228(r29)
    li       r5, 0x0
    lwz      r6, 0x22c(r29)
    li       r4, 0x1
    and      r3, r3, r5
    and      r4, r6, r4
    xor      r4, r4, r5
    xor      r3, r3, r5
    or.      r3, r4, r3
    beq      .L_8007B53C
    b        .L_8007B554
.L_8007B53C:
    li       r29, 0x0
    b        .L_8007B554
.L_8007B544:
    addi     r4, r4, 0x4
    addi     r6, r6, 0x1
    bdnz     .L_8007B4D0
.L_8007B550:
    li       r29, 0x0
.L_8007B554:
    cmplwi   r29, 0x0
    bne      .L_8007B5A0
    li       r29, 0x0
    b        .L_8007B5A0
    b        .L_8007B5A0
.L_8007B568:
    extsh.   r3, r0
    blt      .L_8007B58C
    lis      r4, gGame@ha
    slwi     r3, r0, 2
    addi     r4, r4, gGame@l
    addis    r4, r4, 0x1
    add      r3, r4, r3
    lwz      r29, -0x67a0(r3)
    b        .L_8007B5A0
.L_8007B58C:
    cmpwi    r0, -0x1
    bne      .L_8007B59C
    lwz      r29, 0x8(r30)
    b        .L_8007B5A0
.L_8007B59C:
    mr       r29, r0
.L_8007B5A0:
    li       r27, 0x0
    mr       r28, r0
.L_8007B5A8:
    mr       r3, r30
    bl       CEvt_Evaluate
    mr       r5, r3
    cmpwi    r5, -0x1
    beq      .L_8007BA64
    cmplwi   r29, 0x0
    beq      .L_8007B9A4
    lwz      r0, 0x228(r29)
    li       r6, 0x0
    lwz      r4, 0x22c(r29)
    li       r3, 0x1
    and      r0, r0, r6
    and      r3, r4, r3
    xor      r3, r3, r6
    xor      r0, r0, r6
    or.      r0, r3, r0
    beq      .L_8007B9A4
    lwz      r0, 0x0(r29)
    cmplwi   r0, 0x0
    bne      .L_8007B6B4
    lis      r3, sEvtAnimCollision@ha
    mr       r4, r28
    addi     r3, r3, sEvtAnimCollision@l
    mr       r6, r28
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_8007B664
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007B64C
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007B64C
.L_8007B644:
    addi     r3, r3, 0x8
    bdnz     .L_8007B644
.L_8007B64C:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007B664
.L_8007B65C:
    addi     r3, r3, 0x1
    bdnz     .L_8007B65C
.L_8007B664:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_8007BA5C
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007B698
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007B698
.L_8007B690:
    addi     r3, r3, 0x8
    bdnz     .L_8007B690
.L_8007B698:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007BA5C
.L_8007B6A8:
    addi     r3, r3, 0x1
    bdnz     .L_8007B6A8
    b        .L_8007BA5C
.L_8007B6B4:
    cmpwi    r27, 0x0
    bne      .L_8007B870
    stw      r6, 0x810(r29)
    li       r3, -0x1
    clrrwi.  r0, r5, 16
    stw      r3, 0x814(r29)
    stw      r3, 0x81c(r29)
    stw      r3, 0x824(r29)
    stw      r3, 0x82c(r29)
    stw      r3, 0x834(r29)
    stw      r3, 0x83c(r29)
    stw      r3, 0x844(r29)
    stw      r3, 0x84c(r29)
    stw      r3, 0x854(r29)
    stw      r3, 0x85c(r29)
    stw      r3, 0x864(r29)
    stw      r3, 0x86c(r29)
    stw      r3, 0x874(r29)
    stw      r3, 0x87c(r29)
    stw      r3, 0x884(r29)
    stw      r3, 0x88c(r29)
    beq      .L_8007B7F4
    mr       r3, r29
    clrlwi   r26, r5, 16
    lwz      r12, 0x34(r29)
    mr       r4, r26
    srawi    r25, r5, 16
    lwz      r12, 0xf0(r12)
    mtctr    r12
    bctrl
    mr       r0, r3
    mr       r3, r29
    lwz      r12, 0x34(r29)
    mr       r24, r0
    mr       r4, r25
    lwz      r12, 0xf4(r12)
    mtctr    r12
    bctrl
    mr       r0, r3
    mr       r3, r29
    lwz      r12, 0x34(r29)
    mr       r23, r0
    mr       r4, r26
    lwz      r12, 0xf8(r12)
    mtctr    r12
    bctrl
    clrlwi   r0, r31, 24
    mr       r5, r3
    subfic   r3, r0, 0x12
    lwz      r12, 0x34(r29)
    cntlzw   r4, r3
    lis      r0, 0x4330
    srwi     r4, r4, 5
    mr       r8, r5
    xoris    r4, r4, 0x8000
    stw      r0, 0x8(r1)
    lfd      f2, lbl_8065CF88@sda21(r0)
    mr       r3, r29
    stw      r4, 0xc(r1)
    mr       r6, r24
    lwz      r12, 0x80(r12)
    mr       r7, r23
    lfd      f0, 0x8(r1)
    li       r4, 0x0
    li       r5, 0x1
    lfs      f1, lbl_8065CF74@sda21(r0)
    fsubs    f3, f0, f2
    lfs      f2, lbl_8065CFE0@sda21(r0)
    lfs      f4, lbl_8065CFE4@sda21(r0)
    mtctr    r12
    bctrl
    cmpwi    r26, 0x3e8
    bge      .L_8007B7E0
    cmpwi    r25, 0x3e8
    blt      .L_8007B854
.L_8007B7E0:
    lwz      r3, 0x228(r29)
    lis      r0, 0x8
    or       r0, r3, r0
    stw      r0, 0x228(r29)
    b        .L_8007B854
.L_8007B7F4:
    clrlwi   r3, r31, 24
    lis      r0, 0x4330
    subfic   r4, r3, 0x12
    mr       r3, r29
    cntlzw   r4, r4
    lwz      r12, 0x34(r29)
    srwi     r4, r4, 5
    stw      r0, 0x8(r1)
    xoris    r0, r4, 0x8000
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0xc(r1)
    mr       r4, r5
    lwz      r12, 0x80(r12)
    li       r5, 0x0
    lfd      f0, 0x8(r1)
    li       r6, 0x0
    lfs      f1, lbl_8065CF74@sda21(r0)
    li       r7, 0x0
    fsubs    f3, f0, f2
    lfs      f2, lbl_8065CFE0@sda21(r0)
    lfs      f4, lbl_8065CFE4@sda21(r0)
    li       r8, 0x0
    mtctr    r12
    bctrl
.L_8007B854:
    mr       r3, r29
    li       r4, 0x1
    lwz      r12, 0x34(r29)
    lwz      r12, 0x88(r12)
    mtctr    r12
    bctrl
    b        .L_8007BA5C
.L_8007B870:
    li       r0, 0x4
    mtctr    r0
.L_8007B878:
    lwz      r4, 0x810(r29)
    add      r3, r4, r6
    slwi     r0, r3, 28
    srwi     r3, r3, 31
    subf     r0, r3, r0
    rotlwi   r0, r0, 4
    add      r0, r0, r3
    slwi     r0, r0, 3
    add      r7, r29, r0
    lwz      r0, 0x814(r7)
    cmpwi    r0, -0x1
    bne      .L_8007B8C0
    stw      r5, 0x814(r7)
    li       r3, 0x0
    li       r0, 0x64
    sth      r3, 0x818(r7)
    sth      r0, 0x81a(r7)
    b        .L_8007BA5C
.L_8007B8C0:
    addi     r6, r6, 0x1
    add      r3, r4, r6
    slwi     r0, r3, 28
    srwi     r3, r3, 31
    subf     r0, r3, r0
    rotlwi   r0, r0, 4
    add      r0, r0, r3
    slwi     r0, r0, 3
    add      r7, r29, r0
    lwz      r0, 0x814(r7)
    cmpwi    r0, -0x1
    bne      .L_8007B908
    stw      r5, 0x814(r7)
    li       r3, 0x0
    li       r0, 0x64
    sth      r3, 0x818(r7)
    sth      r0, 0x81a(r7)
    b        .L_8007BA5C
.L_8007B908:
    addi     r6, r6, 0x1
    add      r3, r4, r6
    slwi     r0, r3, 28
    srwi     r3, r3, 31
    subf     r0, r3, r0
    rotlwi   r0, r0, 4
    add      r0, r0, r3
    slwi     r0, r0, 3
    add      r7, r29, r0
    lwz      r0, 0x814(r7)
    cmpwi    r0, -0x1
    bne      .L_8007B950
    stw      r5, 0x814(r7)
    li       r3, 0x0
    li       r0, 0x64
    sth      r3, 0x818(r7)
    sth      r0, 0x81a(r7)
    b        .L_8007BA5C
.L_8007B950:
    addi     r6, r6, 0x1
    add      r3, r4, r6
    slwi     r0, r3, 28
    srwi     r3, r3, 31
    subf     r0, r3, r0
    rotlwi   r0, r0, 4
    add      r0, r0, r3
    slwi     r0, r0, 3
    add      r7, r29, r0
    lwz      r0, 0x814(r7)
    cmpwi    r0, -0x1
    bne      .L_8007B998
    stw      r5, 0x814(r7)
    li       r3, 0x0
    li       r0, 0x64
    sth      r3, 0x818(r7)
    sth      r0, 0x81a(r7)
    b        .L_8007BA5C
.L_8007B998:
    addi     r6, r6, 0x1
    bdnz     .L_8007B878
    b        .L_8007BA5C
.L_8007B9A4:
    lis      r3, sEvtAnimKilled@ha
    mr       r4, r28
    addi     r3, r3, sEvtAnimKilled@l
    mr       r6, r28
    crclr    cr1eq
    bl       FormatToStaticBuffer
    bl       strlen
    addi     r4, r3, 0xd
    li       r3, 0x0
    cmplwi   r4, 0x0
    ble      .L_8007BA10
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007B9F8
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007B9F8
.L_8007B9F0:
    addi     r3, r3, 0x8
    bdnz     .L_8007B9F0
.L_8007B9F8:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007BA10
.L_8007BA08:
    addi     r3, r3, 0x1
    bdnz     .L_8007BA08
.L_8007BA10:
    cmplwi   r4, 0x0
    li       r3, 0x0
    ble      .L_8007BA5C
    cmplwi   r4, 0x8
    subi     r5, r4, 0x8
    ble      .L_8007BA44
    addi     r0, r5, 0x7
    srwi     r0, r0, 3
    mtctr    r0
    cmplwi   r5, 0x0
    ble      .L_8007BA44
.L_8007BA3C:
    addi     r3, r3, 0x8
    bdnz     .L_8007BA3C
.L_8007BA44:
    subf     r0, r3, r4
    mtctr    r0
    cmplw    r3, r4
    bge      .L_8007BA5C
.L_8007BA54:
    addi     r3, r3, 0x1
    bdnz     .L_8007BA54
.L_8007BA5C:
    addi     r27, r27, 0x1
    b        .L_8007B5A8
.L_8007BA64:
    lmw      r23, 0x1c(r1)
    li       r3, 0x0
    lwz      r0, 0x44(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

/**
 * @note Address: 0x8007BA7C
 * @note Size: 0x10C
 */
s32 CEvt::_cmd_ABORT(u8, s32)
{
	if (_00 == 0) {
		CEvtQueueEx* queue  = &gEventScriptQueue;
		s32 idx             = queue->mRead;
		u8* script          = queue->mScript[idx];
		u32 context         = queue->mContext[idx];
		s16 useValue        = queue->mUseValue[idx];
		s16 hitValue        = queue->mHitValue[idx];
		queue->mScript[idx] = 0;
		queue->mRead        = (queue->mRead + 1) % 0x80;
		if (script != 0) {
			mData.mScript = script;
			mData._04     = context;
			mUseValue     = useValue;
			mHitValue     = hitValue;
			return 0;
		}
	}
	CEvt_End(this);
	return 1;
}

/**
 * @note Address: 0x8007BB88
 * @note Size: 0x138
 */
s32 CEvt::_cmd_RETURN(u8, s32)
{
	if (mCalls.mCount > 0) {
		mData.mScript = (u8*)mCalls.mBuf[--mCalls.mCount];
		return 0;
	}
	// nothing to return to, end the script like ABORT
	if (_00 == 0) {
		CEvtQueueEx* queue  = &gEventScriptQueue;
		s32 idx             = queue->mRead;
		u8* script          = queue->mScript[idx];
		u32 context         = queue->mContext[idx];
		s16 useValue        = queue->mUseValue[idx];
		s16 hitValue        = queue->mHitValue[idx];
		queue->mScript[idx] = 0;
		queue->mRead        = (queue->mRead + 1) % 0x80;
		if (script != 0) {
			mData.mScript = script;
			mData._04     = context;
			mUseValue     = useValue;
			mHitValue     = hitValue;
			return 0;
		}
	}
	CEvt_End(this);
	return 1;
}

/**
 * @note Address: 0x8007BCC0
 * @note Size: 0x74
 */
s32 CEvt::_cmd_LIB(u8, s32)
{
	// like RUN but the target is a 2 byte index into the library table
	mCalls.mBuf[mCalls.mCount++] = (s32)(mData.mScript + 2);
	u8* p                        = mData.mScript;
	s16 idx                      = p[0] + (p[1] << 8);
	mData.mScript                = p + 2;
	mData.mScript                = GetGlobalScriptByIndex(idx);
	return 0;
}

/**
 * @note Address: 0x8007BD34
 * @note Size: 0x58
 */
s32 CEvt::_cmd_RUN(u8, s32)
{
	mCalls.mBuf[mCalls.mCount++] = (s32)(mData.mScript + 4);
	mData.mScript                = ResolveEventScriptOffset(mData.mScript);
	return 0;
}

/**
 * @note Address: 0x8007BD8C
 * @note Size: 0x38
 */
s32 CEvt::_cmd_GOTO(u8, s32)
{
	mData.mScript = ResolveEventScriptOffset(mData.mScript);
	return 0;
}

/**
 * @note Address: 0x8007BDC4
 * @note Size: 0x48
 */
s32 CEvt::_cmd_SETSP(u8, s32)
{
	mValues.mBuf[mValues.mCount++] = CEvt_Evaluate(this);
	return 0;
}

/**
 * @note Address: 0x8007BE0C
 * @note Size: 0x24
 */
s32 CEvt::_cmd_POPBP(u8, s32)
{
	mValues.mCount = mFrames.mBuf[--mFrames.mCount];
	return 0;
}
/*
    lwz      r4, 0x38(r3)
    lwz      r5, 0x34(r3)
    subi     r0, r4, 0x1
    stw      r0, 0x38(r3)
    slwi     r0, r0, 2
    lwzx     r0, r5, r0
    stw      r0, 0x48(r3)
    li       r3, 0x0
    blr
*/

/**
 * @note Address: 0x8007BE30
 * @note Size: 0x24
 */
s32 CEvt::_cmd_PUSHBP(u8, s32)
{
	mFrames.mBuf[mFrames.mCount++] = mValues.mCount;
	return 0;
}

// any other op leaves result alone
static inline void Compare(BOOL& result, s32 opcode, s32 lhs, s32 rhs)
{
	u8 op = opcode;
	if (op == EVT_EXPR_EQUAL) {
		result = lhs == rhs;
	}
	if (op == EVT_EXPR_NOT_EQUAL) {
		result = lhs != rhs;
	}
	if (op == EVT_EXPR_LESS) {
		result = lhs < rhs;
	}
	if (op == EVT_EXPR_LESS_EQUAL) {
		result = lhs <= rhs;
	}
	if (op == EVT_EXPR_GREATER) {
		result = lhs > rhs;
	}
	if (op == EVT_EXPR_GREATER_EQUAL) {
		result = lhs >= rhs;
	}
}

// EvalSet inlines this too
static inline bool EvalCmp(CEvt* evt)
{
	BOOL result = false;
	u8 op       = *evt->mData.mScript;

	switch (op) {
	// anything that isn't a comparison is true when nonzero
	case EVT_EXPR_NOT:
	case EVT_EXPR_ADD:
	case EVT_EXPR_SUB:
	case EVT_EXPR_MUL:
	case EVT_EXPR_DIV:
	case EVT_EXPR_MOD:
	case EVT_EXPR_AND:
	case EVT_EXPR_OR:
	case EVT_EXPR_XOR:
	case EVT_EXPR_IMM16:
	case EVT_EXPR_IMM32:
	case EVT_EXPR_ADDRESS_OF:
	case EVT_EXPR_STACK:
	case EVT_EXPR_PARENT_STACK:
	case EVT_EXPR_FLAG:
	case EVT_EXPR_VAR:
	case EVT_EXPR_RESULT1:
	case EVT_EXPR_RESULT2:
	case EVT_EXPR_PAD:
	case EVT_EXPR_BATTERY:
	case EVT_EXPR_MONEY:
	case EVT_EXPR_ITEM:
	case EVT_EXPR_ATC:
	case EVT_EXPR_RANK:
	case EVT_EXPR_EXP:
	case EVT_EXPR_LEVEL:
	case EVT_EXPR_HOLD:
	case EVT_EXPR_MAP:
	case EVT_EXPR_ACTOR_NAME:
	case EVT_EXPR_ITEM_NAME:
	case EVT_EXPR_TIME:
	case EVT_EXPR_CURRENT_SUIT:
	case EVT_EXPR_SCRAP:
	case EVT_EXPR_CURRENT_ATC:
	case EVT_EXPR_USE:
	case EVT_EXPR_HIT:
	case EVT_EXPR_STICKER_NAME:
	case EVT_EXPR_OBJ:
	case EVT_EXPR_RANDOM:
	case EVT_EXPR_SIN:
	case EVT_EXPR_COS:
	case EVT_EXPR_ARRAY_ELEMENT:
		result = CEvt_Evaluate(evt) != 0;
		break;
	case EVT_EXPR_EQUAL:
	case EVT_EXPR_NOT_EQUAL:
	case EVT_EXPR_LESS:
	case EVT_EXPR_LESS_EQUAL:
	case EVT_EXPR_GREATER:
	case EVT_EXPR_GREATER_EQUAL: {
		evt->mData.mScript++;
		// the operand read second is the left side of the test
		s32 rhs = CEvt_Evaluate(evt);
		s32 lhs = CEvt_Evaluate(evt);
		Compare(result, op, lhs, rhs);
		break;
	}
	default:
		OSPanic(__FILE__, 1330, "%s%s", FormatToStaticBuffer("Unknown EvalCmp op - %d\n", op), __FUNCTION__);
		break;
	}

	return result;
}

/**
 * @note Address: 0x8007BE54
 * @note Size: 0x1DC
 */
s32 CEvt::_cmd_IF(u8, s32)
{
	// a true test skips the 4 byte jump offset, a false one takes it
	if (EvalCmp(this)) {
		mData.mScript += 4;
	} else {
		mData.mScript = ResolveEventScriptOffset(mData.mScript);
	}
	return 0;
}

// CEvt::_cmd_IF
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stw      r31, 0x1c(r1)
    stw      r30, 0x18(r1)
    stw      r29, 0x14(r1)
    mr       r29, r3
    stw      r28, 0x10(r1)
    li       r28, 0x0
    lwz      r5, 0x4(r3)
    lbz      r30, 0x0(r5)
    cmpwi    r30, 0x21
    mr       r4, r30
    bge      .L_8007BEB4
    cmpwi    r30, 0xf
    bge      .L_8007BEA8
    cmpwi    r30, 0x6
    bge      .L_8007BED8
    cmpwi    r30, 0x0
    bge      .L_8007BEF0
    b        .L_8007BFBC
.L_8007BEA8:
    cmpwi    r30, 0x17
    bge      .L_8007BED8
    b        .L_8007BFBC
.L_8007BEB4:
    cmpwi    r30, 0xc8
    bge      .L_8007BED0
    cmpwi    r30, 0x76
    bge      .L_8007BFBC
    cmpwi    r30, 0x64
    bge      .L_8007BED8
    b        .L_8007BFBC
.L_8007BED0:
    cmpwi    r30, 0xcd
    bge      .L_8007BFBC
.L_8007BED8:
    mr       r3, r29
    bl       CEvt_Evaluate
    neg      r0, r3
    or       r0, r0, r3
    srwi     r28, r0, 31
    b        .L_8007BFE8
.L_8007BEF0:
    addi     r0, r5, 0x1
    stw      r0, 0x4(r29)
    bl       CEvt_Evaluate
    mr       r31, r3
    mr       r3, r29
    bl       CEvt_Evaluate
    clrlwi.  r0, r30, 24
    bne      .L_8007BF1C
    subf     r0, r3, r31
    cntlzw   r0, r0
    extrwi   r28, r0, 8, 19
.L_8007BF1C:
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x1
    bne      .L_8007BF38
    subf     r4, r3, r31
    subf     r0, r31, r3
    or       r0, r4, r0
    srwi     r28, r0, 31
.L_8007BF38:
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x2
    bne      .L_8007BF58
    xor      r0, r31, r3
    srawi    r4, r0, 1
    and      r0, r0, r31
    subf     r0, r0, r4
    srwi     r28, r0, 31
.L_8007BF58:
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x3
    bne      .L_8007BF78
    srawi    r5, r31, 31
    srwi     r4, r3, 31
    subfc    r0, r3, r31
    adde     r0, r5, r4
    clrlwi   r28, r0, 24
.L_8007BF78:
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x4
    bne      .L_8007BF98
    xor      r0, r3, r31
    srawi    r4, r0, 1
    and      r0, r0, r3
    subf     r0, r0, r4
    srwi     r28, r0, 31
.L_8007BF98:
    clrlwi   r0, r30, 24
    cmplwi   r0, 0x5
    bne      .L_8007BFE8
    srawi    r5, r3, 31
    srwi     r4, r31, 31
    subfc    r0, r31, r3
    adde     r0, r5, r4
    clrlwi   r28, r0, 24
    b        .L_8007BFE8
.L_8007BFBC:
    lis      r3, sEvtUnknownComparison@ha
    addi     r3, r3, sEvtUnknownComparison@l
    crclr    cr1eq
    bl       FormatToStaticBuffer
    mr       r6, r3
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x532
    li       r5, lbl_8065CFA8@sda21
    li       r7, sEvtEvalCmpName@sda21
    crclr    cr1eq
    bl       OSPanic
.L_8007BFE8:
    cmpwi    r28, 0x0
    beq      .L_8007C000
    lwz      r3, 0x4(r29)
    addi     r0, r3, 0x4
    stw      r0, 0x4(r29)
    b        .L_8007C00C
.L_8007C000:
    lwz      r3, 0x4(r29)
    bl       ResolveEventScriptOffset
    stw      r3, 0x4(r29)
.L_8007C00C:
    lwz      r0, 0x24(r1)
    li       r3, 0x0
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    lwz      r28, 0x10(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: 0x8007C030
 * @note Size: 0x24
 */
s32 CEvt::_cmd_SET(u8, s32)
{
	CEvt_EvalSet(this);
	return 0;
}

extern "C" void SetAttachmentCountById(void* attachments, s32 index, s32 value);
#define EVT_PAD (&gGame.mPad.mPad[0])

/**
 * @note Address: 0x8007C054
 * @note Size: 0xA58
 */
extern "C" void CEvt_EvalSet(CEvt* evt)
{
	u8 op = *evt->mData.mScript;
	s32 value;
	switch (op) {
	case EVT_EXPR_EQUAL:
	case EVT_EXPR_NOT_EQUAL:
	case EVT_EXPR_LESS:
	case EVT_EXPR_LESS_EQUAL:
	case EVT_EXPR_GREATER:
	case EVT_EXPR_GREATER_EQUAL:
		value = EvalCmp(evt);
		break;
	case EVT_EXPR_ADD_ASSIGN:
	case EVT_EXPR_SUB_ASSIGN:
	case EVT_EXPR_MUL_ASSIGN:
	case EVT_EXPR_DIV_ASSIGN:
	case EVT_EXPR_MOD_ASSIGN:
	case EVT_EXPR_AND_ASSIGN:
	case EVT_EXPR_OR_ASSIGN:
	case EVT_EXPR_XOR_ASSIGN: {
		evt->mData.mScript++;
		s32 rhs         = CEvt_Evaluate(evt);
		u8* destination = evt->mData.mScript;
		value           = CEvt_Evaluate(evt);
		if (op == EVT_EXPR_ADD_ASSIGN)
			value += rhs;
		else if (op == EVT_EXPR_SUB_ASSIGN)
			value -= rhs;
		else if (op == EVT_EXPR_MUL_ASSIGN)
			value *= rhs;
		else if (op == EVT_EXPR_DIV_ASSIGN)
			value /= rhs;
		else if (op == EVT_EXPR_MOD_ASSIGN)
			value %= rhs;
		else if (op == EVT_EXPR_AND_ASSIGN)
			value &= rhs;
		else if (op == EVT_EXPR_OR_ASSIGN)
			value |= rhs;
		else if (op == EVT_EXPR_XOR_ASSIGN)
			value ^= rhs;
		evt->mData.mScript = destination;
		break;
	}
	default:
		value = CEvt_Evaluate(evt);
		break;
	}
	{
		s32 op = *evt->mData.mScript;
		evt->mData.mScript++;
		s32 index;
		switch (op) {
		case EVT_EXPR_STACK:
			index = *evt->mData.mScript;
			evt->mData.mScript++;
			evt->mValues.mBuf[index + evt->mFrames.mBuf[evt->mFrames.mCount - 1]] = value;
			break;
		case EVT_EXPR_FLAG:
			index = CEvt_Evaluate(evt);
			if (index >= 0 && index < 4096) {
				if (value != 0) {
					gEventFlags.mFlags[(u32)index >> 5] |= 1U << (index & 31);
				} else {
					gEventFlags.mFlags[(u32)index >> 5] &= ~(1U << (index & 31));
				}
			} else if (index != -1 && index != -2 && index != -3) {
				OSPanic("evt.cpp", 1107, "%s%s", FormatToStaticBuffer("Invalid EvalSet flag[%d] - 0~%d\n", index, 4096), "EvalSet");
			}
			break;
		case EVT_EXPR_VAR:
			index = CEvt_Evaluate(evt);
			if (index >= 0 && index < 2048) {
				gEventVars.mVars[index] = value;
			} else {
				OSPanic("evt.cpp", 1116, "%s%s", FormatToStaticBuffer("Invalid EvalSet var[%d] - 0~%d\n", index, 2048), "EvalSet");
			}
			break;
		case EVT_EXPR_RESULT1:
			evt->_10 = value;
			break;
		case EVT_EXPR_RESULT2:
			evt->_14 = value;
			break;
		case EVT_EXPR_PAD:
			index = CEvt_Evaluate(evt);
			if (index == 7) {
				if (gGame.mStatus.mFlags & 0x800000000ULL) {
					if (value == 1) {
						if (EVT_PAD->mConnected)
							HSD_PadRumbleOn(EVT_PAD->mPort);
					} else if (value == 0) {
						if (EVT_PAD->mConnected)
							HSD_PadRumbleOffN(EVT_PAD->mPort);
						if (EVT_PAD->mConnected)
							HSD_PadRumbleRemove(EVT_PAD->mPort);
					} else {
						if (EVT_PAD->mConnected)
							HSD_PadRumbleAdd(EVT_PAD->mPort, 0, -2, 128, (void*)value);
					}
				}
			} else {
				OSPanic("evt.cpp", 1144, "%s%s", FormatToStaticBuffer("Invalid EvalSet pad[%d] - %d\n", index, 7), "EvalSet");
			}
			break;
		case EVT_EXPR_BATTERY:
			index = CEvt_Evaluate(evt);
			if (index == 0) {
				EVT_PLAYER.mParams.SetBattery(value / 100.0f);
			} else if (index == 1) {
				Params* stats      = &EVT_PLAYER.mParams;
				f32 amount         = value / 100.0f;
				stats->mMaxBattery = amount > 0.0f ? amount : 0.0f;
				if (stats->mMaxBattery < 0.0f)
					stats->mMaxBattery = 0.0f;
				if (stats->mMaxBattery > 999.9f)
					stats->mMaxBattery = 999.9f;
				stats->SetBattery(stats->mBattery);
			} else {
				OSPanic("evt.cpp", 1157, "%s%s", FormatToStaticBuffer("Invalid EvalSet battery[%d] - 0~1\n", index), "EvalSet");
			}
			break;
		case EVT_EXPR_MONEY:
			AdjustPlayerMoney(&EVT_PLAYER.mParams, value - EVT_PLAYER.mParams.mMoney);
			break;
		case EVT_EXPR_SCRAP:
			AddPlayerScrap(&EVT_PLAYER.mParams, value - EVT_PLAYER.mParams.mScrap);
			break;
		case EVT_EXPR_ITEM:
			index = CEvt_Evaluate(evt);
			if (index >= 0 && index < 159) {
				SetOrAddInventoryItemCount(&EVT_PLAYER.mParams.mInventory, index, value, 0);
			} else {
				OSPanic("evt.cpp", 1176, "%s%s", FormatToStaticBuffer("Invalid EvalSet item[%d] - 0~%d\n", index, 159), "EvalSet");
			}
			break;
		case EVT_EXPR_ATC:
			index = CEvt_Evaluate(evt);
			if (index >= 0 && index < 9) {
				SetAttachmentCountById(&EVT_PLAYER.mAttachments, index, value);
			} else {
				OSPanic("evt.cpp", 1185, "%s%s", FormatToStaticBuffer("Invalid EvalSet atc[%d] - 0~%d\n", index, 9), "EvalSet");
			}
			break;
		case EVT_EXPR_CURRENT_SUIT: {
			XObjChr& player = EVT_PLAYER;
			if (player.mSuits.mChangeStep == FALSE) {
				player.mSuits.mRequested  = value;
				player.mSuits.mChangeStep = TRUE;
			}
			break;
		}
		case EVT_EXPR_EXP:
			AdjustPlayerExperience(&EVT_PLAYER.mParams, value - EVT_PLAYER.mParams.mExperience);
			break;
		case EVT_EXPR_LEVEL:
			AdjustPlayerLevel(&EVT_PLAYER.mParams, value - EVT_PLAYER.mParams.mLevel);
			break;
		case EVT_EXPR_RANK:
			AddPlayerRank(&EVT_PLAYER.mParams, value - EVT_PLAYER.mParams.mRank);
			break;
		case EVT_EXPR_CURRENT_ATC: {
			XObjChr& player = EVT_PLAYER;
			if (value != player.mAttachment) {
				player.mAttachmentRequest = value;
				player._95D8              = 0;
			}
			break;
		}
		case EVT_EXPR_TIME:
			index = CEvt_Evaluate(evt);
			if (index == 0) {
				gGame.mTimeState.mMode    = value;
				gGame.mTimeState.mTime    = 0.0f;
				gGame.mTimeState.mRunning = 0;
			} else if (index == 1) {
				gGame.mTimeState.mRunning = 0;
				gGame.mTimeState.mTime    = value / 100.0f;
			} else if (index == 2) {
				gGame.mTimeState.mDuration = value / 100.0f;
			} else {
				OSPanic("evt.cpp", 1234, "%s%s", FormatToStaticBuffer("Invalid EvalSet time[%d] - 0~2\n", index), "EvalSet");
			}
			break;
		case EVT_EXPR_HOLD:
		case EVT_EXPR_MAP:
		case EVT_EXPR_ACTOR_NAME:
		case EVT_EXPR_ITEM_NAME:
		case EVT_EXPR_USE:
		case EVT_EXPR_HIT:
		case EVT_EXPR_STICKER_NAME:
		case EVT_EXPR_OBJ:
		case EVT_EXPR_RANDOM:
		case EVT_EXPR_SIN:
		case EVT_EXPR_COS:
			break;
		default:
			OSPanic("evt.cpp", 1245, "%s%s", FormatToStaticBuffer("Unknown EvalSet op - %d\n", op), "EvalSet");
			break;
		}
	}
}
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    lis      r4, lbl_80207FA0@ha
    stw      r0, 0x34(r1)
    stmw     r26, 0x18(r1)
    addi     r30, r4, lbl_80207FA0@l
    mr       r28, r3
    lwz      r5, 0x4(r3)
    lbz      r29, 0x0(r5)
    clrlwi   r4, r29, 24
    cmpwi    r4, 0xf
    bge      .L_8007C098
    cmpwi    r4, 0x6
    bge      .L_8007C2C8
    cmpwi    r4, 0x0
    bge      .L_8007C0A4
    b        .L_8007C2C8
.L_8007C098:
    cmpwi    r4, 0x17
    bge      .L_8007C2C8
    b        .L_8007C218
.L_8007C0A4:
    cmpwi    r4, 0x21
    li       r27, 0x0
    bge      .L_8007C0D8
    cmpwi    r4, 0xf
    bge      .L_8007C0CC
    cmpwi    r4, 0x6
    bge      .L_8007C0FC
    cmpwi    r4, 0x0
    bge      .L_8007C114
    b        .L_8007C1E0
.L_8007C0CC:
    cmpwi    r4, 0x17
    bge      .L_8007C0FC
    b        .L_8007C1E0
.L_8007C0D8:
    cmpwi    r4, 0xc8
    bge      .L_8007C0F4
    cmpwi    r4, 0x76
    bge      .L_8007C1E0
    cmpwi    r4, 0x64
    bge      .L_8007C0FC
    b        .L_8007C1E0
.L_8007C0F4:
    cmpwi    r4, 0xcd
    bge      .L_8007C1E0
.L_8007C0FC:
    mr       r3, r28
    bl       CEvt_Evaluate
    neg      r0, r3
    or       r0, r0, r3
    srwi     r27, r0, 31
    b        .L_8007C208
.L_8007C114:
    addi     r0, r5, 0x1
    stw      r0, 0x4(r28)
    bl       CEvt_Evaluate
    mr       r31, r3
    mr       r3, r28
    bl       CEvt_Evaluate
    clrlwi.  r0, r29, 24
    bne      .L_8007C140
    subf     r0, r3, r31
    cntlzw   r0, r0
    extrwi   r27, r0, 8, 19
.L_8007C140:
    clrlwi   r0, r29, 24
    cmplwi   r0, 0x1
    bne      .L_8007C15C
    subf     r4, r3, r31
    subf     r0, r31, r3
    or       r0, r4, r0
    srwi     r27, r0, 31
.L_8007C15C:
    clrlwi   r0, r29, 24
    cmplwi   r0, 0x2
    bne      .L_8007C17C
    xor      r0, r31, r3
    srawi    r4, r0, 1
    and      r0, r0, r31
    subf     r0, r0, r4
    srwi     r27, r0, 31
.L_8007C17C:
    clrlwi   r0, r29, 24
    cmplwi   r0, 0x3
    bne      .L_8007C19C
    srawi    r5, r31, 31
    srwi     r4, r3, 31
    subfc    r0, r3, r31
    adde     r0, r5, r4
    clrlwi   r27, r0, 24
.L_8007C19C:
    clrlwi   r0, r29, 24
    cmplwi   r0, 0x4
    bne      .L_8007C1BC
    xor      r0, r3, r31
    srawi    r4, r0, 1
    and      r0, r0, r3
    subf     r0, r0, r4
    srwi     r27, r0, 31
.L_8007C1BC:
    clrlwi   r0, r29, 24
    cmplwi   r0, 0x5
    bne      .L_8007C208
    srawi    r5, r3, 31
    srwi     r4, r31, 31
    subfc    r0, r31, r3
    adde     r0, r5, r4
    clrlwi   r27, r0, 24
    b        .L_8007C208
.L_8007C1E0:
    addi     r3, r30, 0xba8
    crclr    cr1eq
    bl       FormatToStaticBuffer
    mr       r6, r3
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x532
    li       r5, lbl_8065CFA8@sda21
    li       r7, sEvtEvalCmpName@sda21
    crclr    cr1eq
    bl       OSPanic
.L_8007C208:
    neg      r0, r27
    or       r0, r0, r27
    srwi     r31, r0, 31
    b        .L_8007C2D4
.L_8007C218:
    addi     r0, r5, 0x1
    stw      r0, 0x4(r28)
    bl       CEvt_Evaluate
    lwz      r26, 0x4(r28)
    mr       r27, r3
    mr       r3, r28
    bl       CEvt_Evaluate
    clrlwi   r0, r29, 24
    mr       r31, r3
    cmplwi   r0, 0xf
    bne      .L_8007C24C
    add      r31, r31, r27
    b        .L_8007C2C0
.L_8007C24C:
    cmplwi   r0, 0x10
    bne      .L_8007C25C
    subf     r31, r27, r31
    b        .L_8007C2C0
.L_8007C25C:
    cmplwi   r0, 0x11
    bne      .L_8007C26C
    mullw    r31, r31, r27
    b        .L_8007C2C0
.L_8007C26C:
    cmplwi   r0, 0x12
    bne      .L_8007C27C
    divw     r31, r31, r27
    b        .L_8007C2C0
.L_8007C27C:
    cmplwi   r0, 0x13
    bne      .L_8007C294
    divw     r0, r31, r27
    mullw    r0, r0, r27
    subf     r31, r0, r31
    b        .L_8007C2C0
.L_8007C294:
    cmplwi   r0, 0x14
    bne      .L_8007C2A4
    and      r31, r31, r27
    b        .L_8007C2C0
.L_8007C2A4:
    cmplwi   r0, 0x15
    bne      .L_8007C2B4
    or       r31, r31, r27
    b        .L_8007C2C0
.L_8007C2B4:
    cmplwi   r0, 0x16
    bne      .L_8007C2C0
    xor      r31, r31, r27
.L_8007C2C0:
    stw      r26, 0x4(r28)
    b        .L_8007C2D4
.L_8007C2C8:
    mr       r3, r28
    bl       CEvt_Evaluate
    mr       r31, r3
.L_8007C2D4:
    lwz      r3, 0x4(r28)
    lbz      r4, 0x0(r3)
    addi     r0, r3, 0x1
    cmpwi    r4, 0x67
    stw      r0, 0x4(r28)
    beq      .L_8007C83C
    bge      .L_8007C344
    cmpwi    r4, 0x1f
    beq      .L_8007C4F8
    bge      .L_8007C320
    cmpwi    r4, 0x1c
    beq      .L_8007C3D8
    bge      .L_8007C314
    cmpwi    r4, 0x1a
    beq      .L_8007C3A0
    b        .L_8007CA70
.L_8007C314:
    cmpwi    r4, 0x1e
    bge      .L_8007C4F0
    b        .L_8007C490
.L_8007C320:
    cmpwi    r4, 0x64
    beq      .L_8007C5F0
    bge      .L_8007C338
    cmpwi    r4, 0x21
    bge      .L_8007CA70
    b        .L_8007C500
.L_8007C338:
    cmpwi    r4, 0x66
    bge      .L_8007C7C4
    b        .L_8007C774
.L_8007C344:
    cmpwi    r4, 0x71
    beq      .L_8007C79C
    bge      .L_8007C378
    cmpwi    r4, 0x6a
    beq      .L_8007C904
    bge      .L_8007C368
    cmpwi    r4, 0x69
    bge      .L_8007C8E0
    b        .L_8007C92C
.L_8007C368:
    cmpwi    r4, 0x6f
    beq      .L_8007C984
    bge      .L_8007C8B0
    b        .L_8007CA98
.L_8007C378:
    cmpwi    r4, 0xc8
    bge      .L_8007C394
    cmpwi    r4, 0x76
    bge      .L_8007CA70
    cmpwi    r4, 0x73
    bge      .L_8007CA98
    b        .L_8007C954
.L_8007C394:
    cmpwi    r4, 0xcc
    bge      .L_8007CA70
    b        .L_8007CA98
.L_8007C3A0:
    lwz      r3, 0x4(r28)
    lbz      r5, 0x0(r3)
    addi     r0, r3, 0x1
    stw      r0, 0x4(r28)
    lwz      r0, 0x38(r28)
    lwz      r3, 0x34(r28)
    slwi     r0, r0, 2
    lwz      r4, 0x44(r28)
    add      r3, r3, r0
    lwz      r0, -0x4(r3)
    add      r0, r5, r0
    slwi     r0, r0, 2
    stwx     r31, r4, r0
    b        .L_8007CA98
.L_8007C3D8:
    mr       r3, r28
    bl       CEvt_Evaluate
    cmpwi    r3, 0x0
    blt      .L_8007C448
    cmpwi    r3, 0x1000
    bge      .L_8007C448
    cmpwi    r31, 0x0
    beq      .L_8007C420
    lis      r4, gEventFlags@ha
    rlwinm   r6, r3, 29, 3, 29
    addi     r5, r4, gEventFlags@l
    clrlwi   r0, r3, 27
    li       r3, 0x1
    lwzx     r4, r5, r6
    slw      r0, r3, r0
    or       r0, r4, r0
    stwx     r0, r5, r6
    b        .L_8007CA98
.L_8007C420:
    lis      r4, gEventFlags@ha
    rlwinm   r6, r3, 29, 3, 29
    addi     r5, r4, gEventFlags@l
    clrlwi   r0, r3, 27
    li       r3, 0x1
    lwzx     r4, r5, r6
    slw      r0, r3, r0
    andc     r0, r4, r0
    stwx     r0, r5, r6
    b        .L_8007CA98
.L_8007C448:
    addi     r0, r3, 0x2
    cmplwi   r0, 0x1
    ble      .L_8007CA98
    cmpwi    r3, -0x3
    beq      .L_8007CA98
    mr       r4, r3
    addi     r3, r30, 0xbc4
    li       r5, 0x1000
    crclr    cr1eq
    bl       FormatToStaticBuffer
    mr       r6, r3
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x453
    li       r5, lbl_8065CFA8@sda21
    li       r7, sEvtEvalSetName@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_8007CA98
.L_8007C490:
    mr       r3, r28
    bl       CEvt_Evaluate
    cmpwi    r3, 0x0
    blt      .L_8007C4BC
    cmpwi    r3, 0x800
    bge      .L_8007C4BC
    lis      r4, gEventVars@ha
    slwi     r0, r3, 2
    addi     r3, r4, gEventVars@l
    stwx     r31, r3, r0
    b        .L_8007CA98
.L_8007C4BC:
    mr       r4, r3
    addi     r3, r30, 0xbe8
    li       r5, 0x800
    crclr    cr1eq
    bl       FormatToStaticBuffer
    mr       r6, r3
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x45c
    li       r5, lbl_8065CFA8@sda21
    li       r7, sEvtEvalSetName@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_8007CA98
.L_8007C4F0:
    stw      r31, 0x10(r28)
    b        .L_8007CA98
.L_8007C4F8:
    stw      r31, 0x14(r28)
    b        .L_8007CA98
.L_8007C500:
    mr       r3, r28
    bl       CEvt_Evaluate
    cmpwi    r3, 0x7
    bne      .L_8007C5BC
    lis      r3, gGame@ha
    li       r4, 0x0
    addi     r28, r3, gGame@l
    li       r0, 0x8
    lwz      r3, 0x1b10(r28)
    lwz      r5, 0x1b14(r28)
    and      r0, r3, r0
    and      r3, r5, r4
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_8007CA98
    cmpwi    r31, 0x1
    bne      .L_8007C560
    lwz      r0, 0x1a00(r28)
    cmpwi    r0, 0x0
    beq      .L_8007CA98
    lbz      r3, 0x19b4(r28)
    bl       HSD_PadRumbleOn
    b        .L_8007CA98
.L_8007C560:
    cmpwi    r31, 0x0
    bne      .L_8007C594
    lwz      r0, 0x1a00(r28)
    cmpwi    r0, 0x0
    beq      .L_8007C57C
    lbz      r3, 0x19b4(r28)
    bl       HSD_PadRumbleOffN
.L_8007C57C:
    lwz      r0, 0x1a00(r28)
    cmpwi    r0, 0x0
    beq      .L_8007CA98
    lbz      r3, 0x19b4(r28)
    bl       HSD_PadRumbleRemove
    b        .L_8007CA98
.L_8007C594:
    lwz      r0, 0x1a00(r28)
    cmpwi    r0, 0x0
    beq      .L_8007CA98
    lbz      r3, 0x19b4(r28)
    mr       r7, r31
    li       r4, 0x0
    li       r5, -0x2
    li       r6, 0x80
    bl       HSD_PadRumbleAdd
    b        .L_8007CA98
.L_8007C5BC:
    mr       r4, r3
    addi     r3, r30, 0xc08
    li       r5, 0x7
    crclr    cr1eq
    bl       FormatToStaticBuffer
    mr       r6, r3
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x478
    li       r5, lbl_8065CFA8@sda21
    li       r7, sEvtEvalSetName@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_8007CA98
.L_8007C5F0:
    mr       r3, r28
    bl       CEvt_Evaluate
    cmpwi    r3, 0x0
    bne      .L_8007C67C
    xoris    r3, r31, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc(r1)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x8(r1)
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    lfd      f0, 0x8(r1)
    lfs      f1, lbl_8065CF80@sda21(r0)
    addis    r3, r3, 0x1
    fsubs    f2, f0, f2
    lfs      f0, lbl_8065CF74@sda21(r0)
    fdivs    f1, f2, f1
    fcmpo    cr0, f1, f0
    ble      .L_8007C648
    b        .L_8007C64C
.L_8007C648:
    fmr      f1, f0
.L_8007C64C:
    stfs     f1, -0x69a8(r3)
    lfs      f0, lbl_8065CF74@sda21(r0)
    lfs      f1, -0x69a8(r3)
    fcmpo    cr0, f1, f0
    bge      .L_8007C664
    stfs     f0, -0x69a8(r3)
.L_8007C664:
    lfs      f0, -0x69a8(r3)
    lfs      f1, -0x69a4(r3)
    fcmpo    cr0, f0, f1
    ble      .L_8007CA98
    stfs     f1, -0x69a8(r3)
    b        .L_8007CA98
.L_8007C67C:
    cmpwi    r3, 0x1
    bne      .L_8007C744
    xoris    r3, r31, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc(r1)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x8(r1)
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    lfd      f0, 0x8(r1)
    lfs      f1, lbl_8065CF80@sda21(r0)
    addis    r3, r3, 0x1
    fsubs    f2, f0, f2
    lfs      f0, lbl_8065CF74@sda21(r0)
    fdivs    f1, f2, f1
    fcmpo    cr0, f1, f0
    ble      .L_8007C6CC
    b        .L_8007C6D0
.L_8007C6CC:
    fmr      f1, f0
.L_8007C6D0:
    stfs     f1, -0x69a4(r3)
    lfs      f0, lbl_8065CF74@sda21(r0)
    lfs      f1, -0x69a4(r3)
    fcmpo    cr0, f1, f0
    bge      .L_8007C6E8
    stfs     f0, -0x69a4(r3)
.L_8007C6E8:
    lfs      f1, -0x69a4(r3)
    lfs      f0, lbl_8065D004@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_8007C6FC
    stfs     f0, -0x69a4(r3)
.L_8007C6FC:
    lfs      f1, -0x69a8(r3)
    lfs      f0, lbl_8065CF74@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_8007C710
    b        .L_8007C714
.L_8007C710:
    fmr      f1, f0
.L_8007C714:
    stfs     f1, -0x69a8(r3)
    lfs      f0, lbl_8065CF74@sda21(r0)
    lfs      f1, -0x69a8(r3)
    fcmpo    cr0, f1, f0
    bge      .L_8007C72C
    stfs     f0, -0x69a8(r3)
.L_8007C72C:
    lfs      f0, -0x69a8(r3)
    lfs      f1, -0x69a4(r3)
    fcmpo    cr0, f0, f1
    ble      .L_8007CA98
    stfs     f1, -0x69a8(r3)
    b        .L_8007CA98
.L_8007C744:
    mr       r4, r3
    addi     r3, r30, 0xc28
    crclr    cr1eq
    bl       FormatToStaticBuffer
    mr       r6, r3
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x485
    li       r5, lbl_8065CFA8@sda21
    li       r7, sEvtEvalSetName@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_8007CA98
.L_8007C774:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lwz      r0, -0x69a0(r3)
    subi     r3, r3, 0x69b4
    subf     r4, r0, r31
    bl       AdjustPlayerMoney
    b        .L_8007CA98
.L_8007C79C:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lwz      r0, -0x699c(r3)
    subi     r3, r3, 0x69b4
    subf     r4, r0, r31
    bl       AddPlayerScrap
    b        .L_8007CA98
.L_8007C7C4:
    mr       r3, r28
    bl       CEvt_Evaluate
    cmpwi    r3, 0x0
    blt      .L_8007C808
    cmpwi    r3, 0x9f
    bge      .L_8007C808
    lis      r5, gGame@ha
    extsh    r4, r3
    addi     r3, r5, gGame@l
    extsh    r5, r31
    addis    r3, r3, 0x1
    li       r6, 0x0
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    subi     r3, r3, 0x6978
    bl       SetOrAddInventoryItemCount
    b        .L_8007CA98
.L_8007C808:
    mr       r4, r3
    addi     r3, r30, 0xc4c
    li       r5, 0x9f
    crclr    cr1eq
    bl       FormatToStaticBuffer
    mr       r6, r3
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x498
    li       r5, lbl_8065CFA8@sda21
    li       r7, sEvtEvalSetName@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_8007CA98
.L_8007C83C:
    mr       r3, r28
    bl       CEvt_Evaluate
    cmpwi    r3, 0x0
    blt      .L_8007C87C
    cmpwi    r3, 0x9
    bge      .L_8007C87C
    lis      r5, gGame@ha
    mr       r4, r3
    addi     r3, r5, gGame@l
    mr       r5, r31
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    addi     r3, r3, 0x2dfc
    bl       SetAttachmentCountById
    b        .L_8007CA98
.L_8007C87C:
    mr       r4, r3
    addi     r3, r30, 0xc70
    li       r5, 0x9
    crclr    cr1eq
    bl       FormatToStaticBuffer
    mr       r6, r3
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x4a1
    li       r5, lbl_8065CFA8@sda21
    li       r7, sEvtEvalSetName@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_8007CA98
.L_8007C8B0:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lwz      r0, -0x281c(r3)
    cmpwi    r0, 0x0
    bne      .L_8007CA98
    stw      r31, -0x2814(r3)
    li       r0, 0x1
    stw      r0, -0x281c(r3)
    b        .L_8007CA98
.L_8007C8E0:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lwzu     r0, -0x69b4(r3)
    subf     r4, r0, r31
    bl       AdjustPlayerExperience
    b        .L_8007CA98
.L_8007C904:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lwz      r0, -0x69b0(r3)
    subi     r3, r3, 0x69b4
    subf     r4, r0, r31
    bl       AdjustPlayerLevel
    b        .L_8007CA98
.L_8007C92C:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lwz      r0, -0x69ac(r3)
    subi     r3, r3, 0x69b4
    subf     r4, r0, r31
    bl       AddPlayerRank
    b        .L_8007CA98
.L_8007C954:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lwz      r0, -0x6a30(r3)
    cmpw     r31, r0
    beq      .L_8007CA98
    stw      r31, -0x6a2c(r3)
    li       r0, 0x0
    stw      r0, -0x6a28(r3)
    b        .L_8007CA98
.L_8007C984:
    mr       r3, r28
    bl       CEvt_Evaluate
    cmpwi    r3, 0x0
    bne      .L_8007C9B8
    lis      r3, gGame@ha
    lfs      f0, lbl_8065CF74@sda21(r0)
    addi     r3, r3, gGame@l
    li       r0, 0x0
    addis    r3, r3, 0xe
    stw      r31, 0x5d28(r3)
    stfs     f0, 0x5d2c(r3)
    stw      r0, 0x5d34(r3)
    b        .L_8007CA98
.L_8007C9B8:
    cmpwi    r3, 0x1
    bne      .L_8007CA00
    xoris    r3, r31, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc(r1)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x8(r1)
    addis    r3, r3, 0xe
    li       r0, 0x0
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x8(r1)
    stw      r0, 0x5d34(r3)
    fsubs    f1, f1, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x5d2c(r3)
    b        .L_8007CA98
.L_8007CA00:
    cmpwi    r3, 0x2
    bne      .L_8007CA40
    xoris    r3, r31, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc(r1)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    lfd      f2, lbl_8065CF88@sda21(r0)
    stw      r0, 0x8(r1)
    addis    r3, r3, 0xe
    lfs      f0, lbl_8065CF80@sda21(r0)
    lfd      f1, 0x8(r1)
    fsubs    f1, f1, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x5d30(r3)
    b        .L_8007CA98
.L_8007CA40:
    mr       r4, r3
    addi     r3, r30, 0xc90
    crclr    cr1eq
    bl       FormatToStaticBuffer
    mr       r6, r3
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x4d2
    li       r5, lbl_8065CFA8@sda21
    li       r7, sEvtEvalSetName@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_8007CA98
.L_8007CA70:
    addi     r3, r30, 0xcb0
    crclr    cr1eq
    bl       FormatToStaticBuffer
    mr       r6, r3
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x4dd
    li       r5, lbl_8065CFA8@sda21
    li       r7, sEvtEvalSetName@sda21
    crclr    cr1eq
    bl       OSPanic
.L_8007CA98:
    lmw      r26, 0x18(r1)
    lwz      r0, 0x34(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

extern "C" {
u8 GetPadRumbleStatus(u8 port);
}

static inline s32 EvtRead16(const u8* p)
{
	return p[0] + (p[1] << 8);
}

/**
 * @note Address: 0x8007CAAC
 * @note Size: 0x1D1C
 */
extern "C" s32 CEvt_Evaluate(CEvt* evt)
{
	s32 op = *evt->mData.mScript;
	evt->mData.mScript++;
	s32 result;
	switch (op) {
	case EVT_EXPR_EQUAL:
	case EVT_EXPR_NOT_EQUAL:
	case EVT_EXPR_LESS:
	case EVT_EXPR_LESS_EQUAL:
	case EVT_EXPR_GREATER:
	case EVT_EXPR_GREATER_EQUAL:
	case EVT_EXPR_ADD:
	case EVT_EXPR_SUB:
	case EVT_EXPR_MUL:
	case EVT_EXPR_DIV:
	case EVT_EXPR_MOD:
	case EVT_EXPR_AND:
	case EVT_EXPR_OR:
	case EVT_EXPR_XOR: {
		s32 rhs = CEvt_Evaluate(evt);
		s32 lhs = CEvt_Evaluate(evt);
		if ((u8)op == EVT_EXPR_EQUAL)
			result = lhs == rhs;
		if ((u8)op == EVT_EXPR_NOT_EQUAL)
			result = lhs != rhs;
		if ((u8)op == EVT_EXPR_LESS)
			result = lhs < rhs;
		if ((u8)op == EVT_EXPR_LESS_EQUAL)
			result = lhs <= rhs;
		if ((u8)op == EVT_EXPR_GREATER)
			result = lhs > rhs;
		if ((u8)op == EVT_EXPR_GREATER_EQUAL)
			result = lhs >= rhs;
		if ((u8)op == EVT_EXPR_ADD)
			result = lhs + rhs;
		if ((u8)op == EVT_EXPR_SUB)
			result = lhs - rhs;
		if ((u8)op == EVT_EXPR_MUL)
			result = lhs * rhs;
		if ((u8)op == EVT_EXPR_DIV)
			result = lhs / rhs;
		if ((u8)op == EVT_EXPR_MOD)
			result = lhs % rhs;
		if ((u8)op == EVT_EXPR_AND)
			result = lhs & rhs;
		if ((u8)op == EVT_EXPR_OR)
			result = lhs | rhs;
		if ((u8)op == EVT_EXPR_XOR)
			result = lhs ^ rhs;
		break;
	}
	case EVT_EXPR_NOT:
		result = !CEvt_Evaluate(evt);
		break;
	case EVT_EXPR_IMM16:
		result = (s16)EvtRead16(evt->mData.mScript);
		evt->mData.mScript += 2;
		break;
	case EVT_EXPR_IMM32:
		result = ReadResourceOffset(evt->mData.mScript);
		evt->mData.mScript += 4;
		break;
	case EVT_EXPR_ADDRESS_OF: {
		s32 offset = ReadResourceOffset(evt->mData.mScript);
		evt->mData.mScript += 4;
		s32 library = IsGlobalEventScript(evt->mData.mScript);
		if (library != 0)
			result = (s32)(GetGlobalScriptTable(library) + offset);
		else
			result = (s32)(GetStageDataBuffer() + offset);
		break;
	}
	case EVT_EXPR_STACK: {
		s32 index = *evt->mData.mScript;
		evt->mData.mScript++;
		result = evt->mValues.mBuf[index + evt->mFrames.mBuf[evt->mFrames.mCount - 1]];
		break;
	}
	case EVT_EXPR_PARENT_STACK: {
		s32 index = *evt->mData.mScript;
		evt->mData.mScript++;
		result = evt->mValues.mBuf[index + evt->mFrames.mBuf[evt->mFrames.mCount - 2]];
		break;
	}
	case EVT_EXPR_FLAG:
		result = CEvt_Evaluate(evt);
		if (result >= 0 && result < 4096) {
			result = (gEventFlags.mFlags[(u32)result >> 5] & (1U << (result & 31))) ? 1 : 0;
		} else if (result == -1)
			result = 0;
		else if (result == -2)
			result = (gGame.mStatus.mFlags & 0x2000) ? 1 : 0;
		else if (result == -3)
			result = (gGame.mStatus.mFlags & 0x8000000000000ULL) ? 1 : 0;
		else
			OSPanic("evt.cpp", 686, "%s%s", FormatToStaticBuffer("Invalid Evaluate flg[%d] - 0~%d\n", result, 4096), "Evaluate");
		break;
	case EVT_EXPR_VAR:
		result = CEvt_Evaluate(evt);
		if (result >= 0 && result < 2048)
			result = gEventVars.mVars[result];
		else
			OSPanic("evt.cpp", 694, "%s%s", FormatToStaticBuffer("Invalid Evaluate var[%d] - 0~%d\n", result, 2048), "Evaluate");
		break;
	case EVT_EXPR_RESULT1:
		result = evt->_10;
		break;
	case EVT_EXPR_RESULT2:
		result = evt->_14;
		break;
	case EVT_EXPR_PAD: {
		result = CEvt_Evaluate(evt);
		CPad pad(0);
		switch (result) {
		case 0:
			result = pad.mPad->mDirButton;
			break;
		case 1:
			result = pad.mPad->mDirTrigger;
			break;
		case 2:
			result = 100.0f * pad.mPad->mStickX;
			break;
		case 3:
			result = 100.0f * pad.mPad->mStickY;
			break;
		case 4:
			result = RAD2DEG * WrapAngle(-(f32)atan2(pad.mPad->mStickX, pad.mPad->mStickY));
			break;
		case 5: {
			f32 x  = pad.mPad->mStickX;
			f32 y  = pad.mPad->mStickY;
			result = 100.0f * sqrtf(x * x + y * y);
			break;
		}
		case 6: {
			f32 stickAngle = (f32)atan2(pad.mPad->mStickX, pad.mPad->mStickY);
			f32 cameraYaw  = gGame.mMotionControlCamera.mCamera.mAngle.y;
			f32 playerYaw  = gGame.GetPlayer()->mMoveRotate.y;
			result         = RAD2DEG * WrapAngle(-stickAngle + cameraYaw - playerYaw);
			break;
		}
		case 7:
			if (gGame.mStatus.mFlags & 0x800000000ULL) {
				u8 status = 0;
				if (pad.mPad->mConnected)
					status = GetPadRumbleStatus(pad.mPad->mPort);
				result = status == 2;
			} else
				result = 0;
			break;
		default:
			OSPanic("evt.cpp", 737, "%s%s", FormatToStaticBuffer("Invalid Evaluate pad[%d] - 0~7\n", result), "Evaluate");
			break;
		}
		break;
	}
	case EVT_EXPR_BATTERY:
		result = CEvt_Evaluate(evt);
		if (result == 0)
			result = 100.0f * EVT_PLAYER.mParams.mBattery;
		else if (result == 1)
			result = 100.0f * EVT_PLAYER.mParams.mMaxBattery;
		else
			OSPanic("evt.cpp", 750, "%s%s", FormatToStaticBuffer("Invalid Evaluate battery[%d] - 0~1\n", result), "Evaluate");
		break;
	case EVT_EXPR_MONEY:
		result = EVT_PLAYER.mParams.mMoney;
		break;
	case EVT_EXPR_SCRAP:
		result = EVT_PLAYER.mParams.mScrap;
		break;
	case EVT_EXPR_ITEM:
		result = CEvt_Evaluate(evt);
		if (result >= 0 && result < 159) {
			s32 index = FindOwnedInventoryItemIndex(EVT_PLAYER.mParams.mInventory.mItems, result);
			result    = index >= 0 ? EVT_PLAYER.mParams.mInventory.mItems[index].mCount : 0;
		} else
			OSPanic("evt.cpp", 769, "%s%s", FormatToStaticBuffer("Invalid Evaluate item[%d] - 0~%d\n", result, 159), "Evaluate");
		break;
	case EVT_EXPR_ATC:
		result = CEvt_Evaluate(evt);
		if (result >= 0 && result < 9) {
			result = GetAttachmentCountById(&EVT_PLAYER.mAttachments, result);
			if (result < 0)
				result = 0;
		} else
			OSPanic("evt.cpp", 778, "%s%s", FormatToStaticBuffer("Invalid Evaluate atc[%d] - 0~%d\n", result, 9), "Evaluate");
		break;
	case EVT_EXPR_CURRENT_SUIT:
		result = EVT_PLAYER.mSuits.mCurrent;
		break;
	case EVT_EXPR_EXP:
		result = EVT_PLAYER.mParams.mExperience;
		break;
	case EVT_EXPR_LEVEL:
		result = EVT_PLAYER.mParams.mLevel;
		break;
	case EVT_EXPR_RANK:
		result = EVT_PLAYER.mParams.mRank;
		break;
	case EVT_EXPR_HOLD: {
		XObj* held = *(XObj**)((u8*)gGame.GetPlayer() + PLAYER_HELD_OBJ);
		if (held != NULL) {
			if ((s16)held->GetPool() == 24)
				result = 1000;
			else if ((s16)held->GetPool() == 8)
				result = held->GetObjFlags();
			else
				result = -1;
		} else
			result = -1;
		break;
	}
	case EVT_EXPR_USE:
		result = evt->mUseValue;
		break;
	case EVT_EXPR_HIT:
		result = evt->mHitValue;
		break;
	case EVT_EXPR_CURRENT_ATC:
		result = EVT_PLAYER.GetAttachment();
		break;
	case EVT_EXPR_MAP:
		result = CEvt_Evaluate(evt);
		if (result >= 0 && result <= 1) {
			s32 stage = result == 0 ? gGame.mStage : gGame.mPrevStage;
			result    = stage;
			if (stage > GetStageNum())
				result = stage - GetStageNum() + 99;
		} else
			OSPanic("evt.cpp", 827, "%s%s", FormatToStaticBuffer("Invalid Evaluate map[%d] - 0~1\n", result), "Evaluate");
		break;
	case EVT_EXPR_ACTOR_NAME:
		result = CEvt_Evaluate(evt);
		if (result >= 20000)
			result -= 20000;
		result = (s32)GetActorName(result);
		break;
	case EVT_EXPR_ITEM_NAME:
		result = (s32)GetItemName(CEvt_Evaluate(evt));
		break;
	case EVT_EXPR_STICKER_NAME:
		result = (s32)GetStatusIconName(CEvt_Evaluate(evt));
		break;
	case EVT_EXPR_TIME:
		result = CEvt_Evaluate(evt);
		if (result == 0)
			result = gGame.mTimeState.mMode;
		else if (result == 1)
			result = 100.0f * gGame.mTimeState.mTime;
		else if (result == 2)
			result = 100.0f * gGame.mTimeState.mDuration;
		else
			OSPanic("evt.cpp", 854, "%s%s", FormatToStaticBuffer("Invalid Evaluate time[%d] - 0~2\n", result), "Evaluate");
		break;
	case EVT_EXPR_OBJ:
		result = CEvt_Evaluate(evt);
		switch (result) {
		case EVT_ATOM_ANIM:
		case EVT_ATOM_DIR:
		case EVT_ATOM_POSX:
		case EVT_ATOM_POSY:
		case EVT_ATOM_POSZ:
		case 235:
		case 247:
		case 248: {
			XObj* obj = CEvt_GetObj(evt, CEvt_Evaluate(evt));
			if (obj != NULL && (obj->mStatus.mFlags & 1)) {
				switch (result) {
				case EVT_ATOM_ANIM:
					result = obj->mAnimId;
					break;
				case EVT_ATOM_DIR:
					result = RAD2DEG * WrapAngle(obj->mMoveRotate.y);
					break;
				case EVT_ATOM_POSX:
					result = 100.0f * obj->mMoveTranslate.x;
					break;
				case EVT_ATOM_POSY:
					result = 100.0f * obj->mMoveTranslate.y;
					break;
				case EVT_ATOM_POSZ:
					result = 100.0f * obj->mMoveTranslate.z;
					break;
				case 235:
					result = (obj->mStatus.mFlags & 0x10000) ? 1 : 0;
					break;
				case 247: {
					Vec pos = { 0.0f, 0.0f, 0.0f };
					f32 x;
					obj->ProjectOffsetToScreen(&pos, &x, NULL, NULL, 0);
					result = 100.0f * x;
					break;
				}
				case 248: {
					Vec pos = { 0.0f, 0.0f, 0.0f };
					f32 y;
					obj->ProjectOffsetToScreen(&pos, NULL, &y, NULL, 0);
					result = 100.0f * y;
					break;
				}
				}
			} else
				result = -1;
			break;
		}
		case EVT_ATOM_BONEX:
		case EVT_ATOM_BONEY:
		case EVT_ATOM_BONEZ:
		case 249:
		case 250: {
			u8* data  = (u8*)CEvt_Evaluate(evt);
			XObj* obj = CEvt_GetObj(evt, (s16)EvtRead16(data));
			if (obj != NULL && (obj->mStatus.mFlags & 1)) {
				s32 depth = (s16)EvtRead16(data + 2);
				data += 4;
				s32 path[20];
				Vec pos;
				for (s32 i = 0; i < depth; i++, data += 2)
					path[i] = (s16)EvtRead16(data);
				switch (result) {
				case EVT_ATOM_BONEX:
					result = 100.0f * GetEventJointWorldPosition((CJObj*)obj, depth, path).x;
					break;
				case EVT_ATOM_BONEY:
					result = 100.0f * GetEventJointWorldPosition((CJObj*)obj, depth, path).y;
					break;
				case EVT_ATOM_BONEZ:
					result = 100.0f * GetEventJointWorldPosition((CJObj*)obj, depth, path).z;
					break;
				case 249: {
					pos = GetEventJointWorldPosition((CJObj*)obj, depth, path) - (Vec)obj->mMoveTranslate;
					f32 x;
					obj->ProjectOffsetToScreen(&pos, &x, NULL, NULL, 0);
					result = 100.0f * x;
					break;
				}
				case 250: {
					pos = GetEventJointWorldPosition((CJObj*)obj, depth, path) - (Vec)obj->mMoveTranslate;
					f32 y;
					obj->ProjectOffsetToScreen(&pos, NULL, &y, NULL, 0);
					result = 100.0f * y;
					break;
				}
				}
			} else
				result = -1;
			break;
		}
		case EVT_ATOM_DIRTO: {
			u8* data   = (u8*)CEvt_Evaluate(evt);
			XObj* from = CEvt_GetObj(evt, (s16)EvtRead16(data));
			XObj* to   = CEvt_GetObj(evt, (s16)EvtRead16(data + 2));
			if (from != NULL && (from->mStatus.mFlags & 1) && to != NULL && (to->mStatus.mFlags & 1)) {
				f32 angle = (f32)atan2(to->mMoveTranslate.x - from->mMoveTranslate.x, to->mMoveTranslate.z - from->mMoveTranslate.z);
				result    = RAD2DEG * WrapAngle(angle - from->mMoveRotate.y);
			} else
				result = -1;
			break;
		}
		case EVT_ATOM_DISTANCE: {
			u8* data   = (u8*)CEvt_Evaluate(evt);
			XObj* from = CEvt_GetObj(evt, (s16)EvtRead16(data));
			XObj* to   = CEvt_GetObj(evt, (s16)EvtRead16(data + 2));
			if (from != NULL && (from->mStatus.mFlags & 1) && to != NULL && (to->mStatus.mFlags & 1)) {
				result = 100.0f * PSVECDistance(&from->mMoveTranslate, &to->mMoveTranslate);
			} else
				result = -1;
			break;
		}
		default:
			OSPanic("evt.cpp", 977, "%s%s", FormatToStaticBuffer("Invalid Evaluate obj(...,%d)\n", result), "Evaluate");
			break;
		}
		break;
	case EVT_EXPR_RANDOM: {
		s32 max = CEvt_Evaluate(evt);
		result  = rand() % (max + 1);
		break;
	}
	case EVT_EXPR_SIN:
		result = 100.0f * (f32)sin(DEG2RAD * (CEvt_Evaluate(evt) / 100.0f));
		break;
	case EVT_EXPR_COS:
		result = 100.0f * (f32)cos(DEG2RAD * (CEvt_Evaluate(evt) / 100.0f));
		break;
	case EVT_EXPR_ARRAY_ELEMENT: {
		result     = (s8)CEvt_Evaluate(evt);
		s32 index  = CEvt_Evaluate(evt);
		s32 stride = result;
		if ((s8)result < 0)
			stride = -result;
		u8* p = (u8*)CEvt_Evaluate(evt) + index * stride;
		switch (result) {
		case 1:
			result = *p;
			break;
		case -1:
			result = *(s8*)p;
			break;
		case 2:
			result = (u16)EvtRead16(p);
			break;
		case -2:
			result = (s16)EvtRead16(p);
			break;
		case 4:
		case -4:
			result = ReadResourceOffset(p);
			break;
		default:
			result = 0;
			break;
		}
		break;
	}
	default:
		OSPanic("evt.cpp", 1023, "%s%s", FormatToStaticBuffer("Unknown Evaluate op - %d\n", op), "Evaluate");
		break;
	}
	return result;
}

/*
    stwu     r1, -0x140(r1)
    mflr     r0
    lis      r5, lbl_80207FA0@ha
    stw      r0, 0x144(r1)
    stw      r31, 0x13c(r1)
    stw      r30, 0x138(r1)
    mr       r30, r3
    stw      r29, 0x134(r1)
    addi     r29, r5, lbl_80207FA0@l
    stw      r28, 0x130(r1)
    lwz      r4, 0x4(r3)
    lbz      r28, 0x0(r4)
    addi     r0, r4, 0x1
    cmpwi    r28, 0x68
    stw      r0, 0x4(r3)
    beq      .L_8007D5A0
    bge      .L_8007CB84
    cmpwi    r28, 0x1c
    beq      .L_8007CEB0
    bge      .L_8007CB44
    cmpwi    r28, 0x17
    beq      .L_8007CD80
    bge      .L_8007CB2C
    cmpwi    r28, 0x6
    beq      .L_8007CD70
    bge      .L_8007CB20
    cmpwi    r28, 0x0
    bge      .L_8007CC14
    b        .L_8007E774
.L_8007CB20:
    cmpwi    r28, 0xf
    bge      .L_8007E774
    b        .L_8007CC14
.L_8007CB2C:
    cmpwi    r28, 0x1a
    beq      .L_8007CE40
    bge      .L_8007CE78
    cmpwi    r28, 0x19
    bge      .L_8007CDE4
    b        .L_8007CDA8
.L_8007CB44:
    cmpwi    r28, 0x64
    beq      .L_8007D35C
    bge      .L_8007CB74
    cmpwi    r28, 0x1f
    beq      .L_8007D038
    bge      .L_8007CB68
    cmpwi    r28, 0x1e
    bge      .L_8007D030
    b        .L_8007CFD0
.L_8007CB68:
    cmpwi    r28, 0x21
    bge      .L_8007E774
    b        .L_8007D040
.L_8007CB74:
    cmpwi    r28, 0x66
    beq      .L_8007D43C
    bge      .L_8007D4D0
    b        .L_8007D404
.L_8007CB84:
    cmpwi    r28, 0x73
    beq      .L_8007D674
    bge      .L_8007CBD8
    cmpwi    r28, 0x6e
    beq      .L_8007D754
    bge      .L_8007CBC0
    cmpwi    r28, 0x6b
    beq      .L_8007D5BC
    bge      .L_8007CBB4
    cmpwi    r28, 0x6a
    bge      .L_8007D584
    b        .L_8007D568
.L_8007CBB4:
    cmpwi    r28, 0x6d
    bge      .L_8007D730
    b        .L_8007D6B4
.L_8007CBC0:
    cmpwi    r28, 0x71
    beq      .L_8007D420
    bge      .L_8007D684
    cmpwi    r28, 0x70
    bge      .L_8007D54C
    b        .L_8007D774
.L_8007CBD8:
    cmpwi    r28, 0xc9
    beq      .L_8007E5DC
    bge      .L_8007CBFC
    cmpwi    r28, 0x75
    beq      .L_8007D764
    blt      .L_8007D67C
    cmpwi    r28, 0xc8
    bge      .L_8007D828
    b        .L_8007E774
.L_8007CBFC:
    cmpwi    r28, 0xcc
    beq      .L_8007E69C
    bge      .L_8007E774
    cmpwi    r28, 0xcb
    bge      .L_8007E64C
    b        .L_8007E5FC
.L_8007CC14:
    mr       r3, r30
    bl       CEvt_Evaluate
    mr       r29, r3
    mr       r3, r30
    bl       CEvt_Evaluate
    clrlwi.  r0, r28, 24
    bne      .L_8007CC3C
    subf     r0, r3, r29
    cntlzw   r0, r0
    extrwi   r31, r0, 8, 19
.L_8007CC3C:
    clrlwi   r0, r28, 24
    cmplwi   r0, 0x1
    bne      .L_8007CC5C
    subf     r4, r3, r29
    subf     r0, r29, r3
    or       r0, r4, r0
    srwi     r0, r0, 31
    mr       r31, r0
.L_8007CC5C:
    clrlwi   r0, r28, 24
    cmplwi   r0, 0x2
    bne      .L_8007CC80
    xor      r0, r29, r3
    srawi    r4, r0, 1
    and      r0, r0, r29
    subf     r0, r0, r4
    srwi     r0, r0, 31
    mr       r31, r0
.L_8007CC80:
    clrlwi   r0, r28, 24
    cmplwi   r0, 0x3
    bne      .L_8007CCA0
    srawi    r5, r29, 31
    srwi     r4, r3, 31
    subfc    r0, r3, r29
    adde     r0, r5, r4
    clrlwi   r31, r0, 24
.L_8007CCA0:
    clrlwi   r0, r28, 24
    cmplwi   r0, 0x4
    bne      .L_8007CCC4
    xor      r0, r3, r29
    srawi    r4, r0, 1
    and      r0, r0, r3
    subf     r0, r0, r4
    srwi     r0, r0, 31
    mr       r31, r0
.L_8007CCC4:
    clrlwi   r0, r28, 24
    cmplwi   r0, 0x5
    bne      .L_8007CCE4
    srawi    r5, r3, 31
    srwi     r4, r29, 31
    subfc    r0, r29, r3
    adde     r0, r5, r4
    clrlwi   r31, r0, 24
.L_8007CCE4:
    clrlwi   r0, r28, 24
    cmplwi   r0, 0x7
    bne      .L_8007CCF4
    add      r31, r3, r29
.L_8007CCF4:
    clrlwi   r0, r28, 24
    cmplwi   r0, 0x8
    bne      .L_8007CD04
    subf     r31, r29, r3
.L_8007CD04:
    clrlwi   r0, r28, 24
    cmplwi   r0, 0x9
    bne      .L_8007CD14
    mullw    r31, r3, r29
.L_8007CD14:
    clrlwi   r0, r28, 24
    cmplwi   r0, 0xa
    bne      .L_8007CD24
    divw     r31, r3, r29
.L_8007CD24:
    clrlwi   r0, r28, 24
    cmplwi   r0, 0xb
    bne      .L_8007CD3C
    divw     r0, r3, r29
    mullw    r0, r0, r29
    subf     r31, r0, r3
.L_8007CD3C:
    clrlwi   r0, r28, 24
    cmplwi   r0, 0xc
    bne      .L_8007CD4C
    and      r31, r3, r29
.L_8007CD4C:
    clrlwi   r0, r28, 24
    cmplwi   r0, 0xd
    bne      .L_8007CD5C
    or       r31, r3, r29
.L_8007CD5C:
    clrlwi   r0, r28, 24
    cmplwi   r0, 0xe
    bne      .L_8007E7A4
    xor      r31, r3, r29
    b        .L_8007E7A4
.L_8007CD70:
    bl       CEvt_Evaluate
    cntlzw   r0, r3
    extrwi   r31, r0, 8, 19
    b        .L_8007E7A4
.L_8007CD80:
    lwz      r4, 0x4(r30)
    lbz      r3, 0x1(r4)
    addi     r0, r4, 0x2
    lbz      r4, 0x0(r4)
    slwi     r3, r3, 8
    add      r3, r4, r3
    stw      r0, 0x4(r30)
    extsh    r0, r3
    mr       r31, r0
    b        .L_8007E7A4
.L_8007CDA8:
    lwz      r7, 0x4(r30)
    lbz      r4, 0x2(r7)
    addi     r0, r7, 0x4
    lbz      r3, 0x1(r7)
    lbz      r6, 0x3(r7)
    slwi     r5, r4, 16
    slwi     r3, r3, 8
    lbz      r4, 0x0(r7)
    slwi     r6, r6, 24
    add      r3, r3, r5
    stw      r0, 0x4(r30)
    add      r0, r3, r6
    add      r0, r4, r0
    mr       r31, r0
    b        .L_8007E7A4
.L_8007CDE4:
    lwz      r7, 0x4(r30)
    lbz      r4, 0x3(r7)
    addi     r0, r7, 0x4
    lbz      r5, 0x2(r7)
    lbz      r3, 0x1(r7)
    slwi     r6, r4, 24
    lbz      r4, 0x0(r7)
    slwi     r5, r5, 16
    slwi     r3, r3, 8
    stw      r0, 0x4(r30)
    add      r0, r3, r5
    add      r28, r0, r6
    lwz      r3, 0x4(r30)
    add      r28, r4, r28
    bl       IsGlobalEventScript
    cmpwi    r3, 0x0
    beq      .L_8007CE34
    bl       GetGlobalScriptTable
    add      r31, r3, r28
    b        .L_8007E7A4
.L_8007CE34:
    bl       GetStageDataBuffer
    add      r31, r3, r28
    b        .L_8007E7A4
.L_8007CE40:
    lwz      r3, 0x4(r30)
    lbz      r5, 0x0(r3)
    addi     r0, r3, 0x1
    stw      r0, 0x4(r30)
    lwz      r0, 0x38(r30)
    lwz      r3, 0x34(r30)
    slwi     r0, r0, 2
    lwz      r4, 0x44(r30)
    add      r3, r3, r0
    lwz      r0, -0x4(r3)
    add      r0, r5, r0
    slwi     r0, r0, 2
    lwzx     r31, r4, r0
    b        .L_8007E7A4
.L_8007CE78:
    lwz      r3, 0x4(r30)
    lbz      r5, 0x0(r3)
    addi     r0, r3, 0x1
    stw      r0, 0x4(r30)
    lwz      r3, 0x38(r30)
    lwz      r4, 0x34(r30)
    subi     r0, r3, 0x2
    lwz      r3, 0x44(r30)
    slwi     r0, r0, 2
    lwzx     r0, r4, r0
    add      r0, r5, r0
    slwi     r0, r0, 2
    lwzx     r31, r3, r0
    b        .L_8007E7A4
.L_8007CEB0:
    bl       CEvt_Evaluate
    mr.      r31, r3
    blt      .L_8007CEF8
    cmpwi    r31, 0x1000
    bge      .L_8007CEF8
    lis      r3, gEventFlags@ha
    rlwinm   r0, r31, 29, 3, 29
    addi     r3, r3, gEventFlags@l
    clrlwi   r4, r31, 27
    li       r5, 0x1
    lwzx     r0, r3, r0
    slw      r3, r5, r4
    and.     r0, r3, r0
    beq      .L_8007CEEC
    b        .L_8007CEF0
.L_8007CEEC:
    li       r5, 0x0
.L_8007CEF0:
    mr       r31, r5
    b        .L_8007E7A4
.L_8007CEF8:
    cmpwi    r31, -0x1
    bne      .L_8007CF08
    li       r31, 0x0
    b        .L_8007E7A4
.L_8007CF08:
    cmpwi    r31, -0x2
    bne      .L_8007CF50
    lis      r4, gGame@ha
    li       r3, 0x2000
    addi     r5, r4, gGame@l
    lwz      r0, 0x1b10(r5)
    li       r4, 0x0
    lwz      r5, 0x1b14(r5)
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_8007CF48
    li       r31, 0x1
    b        .L_8007E7A4
.L_8007CF48:
    li       r31, 0x0
    b        .L_8007E7A4
.L_8007CF50:
    cmpwi    r31, -0x3
    bne      .L_8007CF98
    lis      r3, gGame@ha
    li       r4, 0x0
    addi     r5, r3, gGame@l
    lis      r0, 0x8
    lwz      r3, 0x1b10(r5)
    lwz      r5, 0x1b14(r5)
    and      r0, r3, r0
    and      r3, r5, r4
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_8007CF90
    li       r31, 0x1
    b        .L_8007E7A4
.L_8007CF90:
    li       r31, 0x0
    b        .L_8007E7A4
.L_8007CF98:
    mr       r4, r31
    addi     r3, r29, 0xccc
    li       r5, 0x1000
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvaluateName@ha
    mr       r6, r3
    addi     r7, r4, sEvaluateName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x2ae
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_8007E7A4
.L_8007CFD0:
    bl       CEvt_Evaluate
    mr.      r31, r3
    blt      .L_8007CFF8
    cmpwi    r31, 0x800
    bge      .L_8007CFF8
    lis      r3, gEventVars@ha
    slwi     r0, r31, 2
    addi     r3, r3, gEventVars@l
    lwzx     r31, r3, r0
    b        .L_8007E7A4
.L_8007CFF8:
    mr       r4, r31
    addi     r3, r29, 0xcf0
    li       r5, 0x800
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvaluateName@ha
    mr       r6, r3
    addi     r7, r4, sEvaluateName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x2b6
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_8007E7A4
.L_8007D030:
    lwz      r31, 0x10(r30)
    b        .L_8007E7A4
.L_8007D038:
    lwz      r31, 0x14(r30)
    b        .L_8007E7A4
.L_8007D040:
    bl       CEvt_Evaluate
    mr       r0, r3
    addi     r3, r1, 0x1c
    mr       r31, r0
    li       r4, 0x0
    bl       __ct__4CPadFUc
    cmplwi   r31, 0x7
    bgt      .L_8007D31C
    lis      r3, jumptable_802252D4@ha
    slwi     r0, r31, 2
    addi     r3, r3, jumptable_802252D4@l
    lwzx     r0, r3, r0
    mtctr    r0
    bctr
.L_8007D078:
    lwz      r3, 0x1c(r1)
    lwz      r31, 0x8(r3)
    b        .L_8007D34C
.L_8007D084:
    lwz      r3, 0x1c(r1)
    lwz      r31, 0x18(r3)
    b        .L_8007D34C
.L_8007D090:
    lwz      r3, 0x1c(r1)
    lfs      f1, lbl_8065CF80@sda21(r0)
    lfs      f0, 0x34(r3)
    fmuls    f0, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007D34C
.L_8007D0B0:
    lwz      r3, 0x1c(r1)
    lfs      f1, lbl_8065CF80@sda21(r0)
    lfs      f0, 0x38(r3)
    fmuls    f0, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007D34C
.L_8007D0D0:
    lwz      r3, 0x1c(r1)
    lfs      f1, 0x34(r3)
    lfs      f2, 0x38(r3)
    bl       atan2
    frsp     f2, f1
    lfs      f1, lbl_8065CFF4@sda21(r0)
    lfs      f0, lbl_8065CF74@sda21(r0)
    fneg     f2, f2
    b        .L_8007D0F8
.L_8007D0F4:
    fadds    f2, f2, f1
.L_8007D0F8:
    fcmpo    cr0, f2, f0
    blt      .L_8007D0F4
    lfs      f0, lbl_8065CFF4@sda21(r0)
    b        .L_8007D10C
.L_8007D108:
    fsubs    f2, f2, f0
.L_8007D10C:
    fcmpo    cr0, f2, f0
    cror     eq, gt, eq
    beq      .L_8007D108
    lfs      f0, lbl_8065CFF0@sda21(r0)
    fmuls    f0, f0, f2
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007D34C
.L_8007D130:
    lwz      r3, 0x1c(r1)
    lfs      f0, lbl_8065CF74@sda21(r0)
    lfs      f1, 0x38(r3)
    lfs      f2, 0x34(r3)
    fmuls    f1, f1, f1
    fmadds   f4, f2, f2, f1
    fcmpo    cr0, f4, f0
    ble      .L_8007D198
    frsqrte  f1, f4
    lfd      f3, lbl_8065D008@sda21(r0)
    lfd      f2, lbl_8065D010@sda21(r0)
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f4, f0, f2
    fmul     f1, f1, f0
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f4, f0, f2
    fmul     f1, f1, f0
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f4, f0, f2
    fmul     f0, f1, f0
    fmul     f4, f4, f0
    frsp     f4, f4
    b        .L_8007D21C
.L_8007D198:
    lfd      f0, lbl_8065D018@sda21(r0)
    fcmpo    cr0, f4, f0
    bge      .L_8007D1B0
    lis      r3, __float_nan@ha
    lfs      f4, __float_nan@l(r3)
    b        .L_8007D21C
.L_8007D1B0:
    stfs     f4, 0x8(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x8(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_8007D1D8
    bge      .L_8007D208
    cmpwi    r3, 0x0
    beq      .L_8007D1F0
    b        .L_8007D208
.L_8007D1D8:
    clrlwi.  r0, r4, 9
    beq      .L_8007D1E8
    li       r0, 0x1
    b        .L_8007D20C
.L_8007D1E8:
    li       r0, 0x2
    b        .L_8007D20C
.L_8007D1F0:
    clrlwi.  r0, r4, 9
    beq      .L_8007D200
    li       r0, 0x5
    b        .L_8007D20C
.L_8007D200:
    li       r0, 0x3
    b        .L_8007D20C
.L_8007D208:
    li       r0, 0x4
.L_8007D20C:
    cmpwi    r0, 0x1
    bne      .L_8007D21C
    lis      r3, __float_nan@ha
    lfs      f4, __float_nan@l(r3)
.L_8007D21C:
    lfs      f0, lbl_8065CF80@sda21(r0)
    fmuls    f0, f0, f4
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007D34C
.L_8007D234:
    lwz      r3, 0x1c(r1)
    lfs      f1, 0x34(r3)
    lfs      f2, 0x38(r3)
    bl       atan2
    frsp     f2, f1
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    lfs      f1, lbl_8065CFF4@sda21(r0)
    addis    r4, r3, 0xe
    lfs      f0, lbl_8065CF74@sda21(r0)
    addis    r3, r3, 0x1
    fneg     f3, f2
    lfs      f2, 0x5df0(r4)
    lwz      r3, -0x6944(r3)
    fadds    f3, f3, f2
    lfs      f2, 0xfc(r3)
    fsubs    f2, f3, f2
    b        .L_8007D280
.L_8007D27C:
    fadds    f2, f2, f1
.L_8007D280:
    fcmpo    cr0, f2, f0
    blt      .L_8007D27C
    lfs      f0, lbl_8065CFF4@sda21(r0)
    b        .L_8007D294
.L_8007D290:
    fsubs    f2, f2, f0
.L_8007D294:
    fcmpo    cr0, f2, f0
    cror     eq, gt, eq
    beq      .L_8007D290
    lfs      f0, lbl_8065CFF0@sda21(r0)
    fmuls    f0, f0, f2
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007D34C
.L_8007D2B8:
    lis      r4, gGame@ha
    li       r3, 0x0
    addi     r5, r4, gGame@l
    li       r0, 0x8
    lwz      r4, 0x1b10(r5)
    lwz      r5, 0x1b14(r5)
    and      r0, r4, r0
    and      r4, r5, r3
    xor      r4, r4, r3
    xor      r0, r0, r3
    or.      r0, r4, r0
    beq      .L_8007D314
    lwz      r4, 0x1c(r1)
    lwz      r0, 0x4c(r4)
    cmpwi    r0, 0x0
    beq      .L_8007D300
    lbz      r3, 0x0(r4)
    bl       GetPadRumbleStatus
.L_8007D300:
    clrlwi   r0, r3, 24
    subfic   r0, r0, 0x2
    cntlzw   r0, r0
    srwi     r31, r0, 5
    b        .L_8007D34C
.L_8007D314:
    li       r31, 0x0
    b        .L_8007D34C
.L_8007D31C:
    mr       r4, r31
    addi     r3, r29, 0xd14
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvaluateName@ha
    mr       r6, r3
    addi     r7, r4, sEvaluateName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x2e1
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
.L_8007D34C:
    lis      r3, __vt__4CPad@ha
    addi     r0, r3, __vt__4CPad@l
    stw      r0, 0x20(r1)
    b        .L_8007E7A4
.L_8007D35C:
    bl       CEvt_Evaluate
    mr.      r31, r3
    bne      .L_8007D398
    lis      r3, gGame@ha
    lfs      f1, lbl_8065CF80@sda21(r0)
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lfs      f0, -0x69a8(r3)
    fmuls    f0, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007E7A4
.L_8007D398:
    cmpwi    r31, 0x1
    bne      .L_8007D3D0
    lis      r3, gGame@ha
    lfs      f1, lbl_8065CF80@sda21(r0)
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lfs      f0, -0x69a4(r3)
    fmuls    f0, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007E7A4
.L_8007D3D0:
    mr       r4, r31
    addi     r3, r29, 0xd34
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvaluateName@ha
    mr       r6, r3
    addi     r7, r4, sEvaluateName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x2ee
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_8007E7A4
.L_8007D404:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lwz      r31, -0x69a0(r3)
    b        .L_8007E7A4
.L_8007D420:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lwz      r31, -0x699c(r3)
    b        .L_8007E7A4
.L_8007D43C:
    bl       CEvt_Evaluate
    mr.      r31, r3
    blt      .L_8007D498
    cmpwi    r31, 0x9f
    bge      .L_8007D498
    lis      r3, gGame@ha
    extsh    r4, r31
    addi     r3, r3, gGame@l
    addis    r28, r3, 0x1
    lwz      r3, -0x6944(r28)
    addis    r3, r3, 0x1
    subi     r3, r3, 0x6978
    bl       FindOwnedInventoryItemIndex
    cmpwi    r3, 0x0
    blt      .L_8007D490
    lwz      r4, -0x6944(r28)
    slwi     r0, r3, 2
    addis    r3, r4, 0x1
    add      r3, r3, r0
    lha      r31, -0x6976(r3)
    b        .L_8007E7A4
.L_8007D490:
    li       r31, 0x0
    b        .L_8007E7A4
.L_8007D498:
    mr       r4, r31
    addi     r3, r29, 0xd58
    li       r5, 0x9f
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvaluateName@ha
    mr       r6, r3
    addi     r7, r4, sEvaluateName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x301
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_8007E7A4
.L_8007D4D0:
    bl       CEvt_Evaluate
    mr.      r31, r3
    blt      .L_8007D514
    cmpwi    r31, 0x9
    bge      .L_8007D514
    lis      r3, gGame@ha
    mr       r4, r31
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    addi     r3, r3, 0x2dfc
    bl       GetAttachmentCountById
    mr.      r31, r3
    bge      .L_8007E7A4
    li       r31, 0x0
    b        .L_8007E7A4
.L_8007D514:
    mr       r4, r31
    addi     r3, r29, 0xd7c
    li       r5, 0x9
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvaluateName@ha
    mr       r6, r3
    addi     r7, r4, sEvaluateName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x30a
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_8007E7A4
.L_8007D54C:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lwz      r31, -0x2818(r3)
    b        .L_8007E7A4
.L_8007D568:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lwz      r31, -0x69b4(r3)
    b        .L_8007E7A4
.L_8007D584:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lwz      r31, -0x69b0(r3)
    b        .L_8007E7A4
.L_8007D5A0:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lwz      r31, -0x69ac(r3)
    b        .L_8007E7A4
.L_8007D5BC:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    lwz      r3, 0xa64(r3)
    cmplwi   r3, 0x0
    beq      .L_8007D66C
    lwz      r3, 0x240(r3)
    cmplwi   r3, 0x0
    beq      .L_8007D5F8
    beq      .L_8007D5F0
    lha      r0, 0x14(r3)
    b        .L_8007D5FC
.L_8007D5F0:
    li       r0, 0x0
    b        .L_8007D5FC
.L_8007D5F8:
    li       r0, -0x1
.L_8007D5FC:
    extsh    r0, r0
    cmpwi    r0, 0x18
    bne      .L_8007D610
    li       r31, 0x3e8
    b        .L_8007E7A4
.L_8007D610:
    cmplwi   r3, 0x0
    beq      .L_8007D62C
    beq      .L_8007D624
    lha      r0, 0x14(r3)
    b        .L_8007D630
.L_8007D624:
    li       r0, 0x0
    b        .L_8007D630
.L_8007D62C:
    li       r0, -0x1
.L_8007D630:
    extsh    r0, r0
    cmpwi    r0, 0x8
    bne      .L_8007D664
    cmplwi   r3, 0x0
    beq      .L_8007D658
    beq      .L_8007D650
    lha      r0, 0x16(r3)
    b        .L_8007D65C
.L_8007D650:
    li       r0, 0x0
    b        .L_8007D65C
.L_8007D658:
    li       r0, -0x1
.L_8007D65C:
    extsh    r31, r0
    b        .L_8007E7A4
.L_8007D664:
    li       r31, -0x1
    b        .L_8007E7A4
.L_8007D66C:
    li       r31, -0x1
    b        .L_8007E7A4
.L_8007D674:
    lha      r31, 0x5c(r30)
    b        .L_8007E7A4
.L_8007D67C:
    lha      r31, 0x5e(r30)
    b        .L_8007E7A4
.L_8007D684:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lwz      r0, -0x6a2c(r3)
    cmpwi    r0, 0x0
    blt      .L_8007D6AC
    mr       r31, r0
    b        .L_8007E7A4
.L_8007D6AC:
    lwz      r31, -0x6a30(r3)
    b        .L_8007E7A4
.L_8007D6B4:
    bl       CEvt_Evaluate
    mr.      r31, r3
    blt      .L_8007D6FC
    cmpwi    r31, 0x1
    bgt      .L_8007D6FC
    lis      r3, gGame@ha
    slwi     r0, r31, 2
    addi     r3, r3, gGame@l
    add      r3, r3, r0
    lwz      r28, 0x3444(r3)
    mr       r31, r28
    bl       GetStageNum
    cmpw     r28, r3
    ble      .L_8007E7A4
    bl       GetStageNum
    subf     r3, r3, r28
    addi     r31, r3, 0x63
    b        .L_8007E7A4
.L_8007D6FC:
    mr       r4, r31
    addi     r3, r29, 0xda0
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvaluateName@ha
    mr       r6, r3
    addi     r7, r4, sEvaluateName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x33b
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_8007E7A4
.L_8007D730:
    bl       CEvt_Evaluate
    mr       r31, r3
    cmpwi    r31, 0x4e20
    blt      .L_8007D744
    subi     r31, r31, 0x4e20
.L_8007D744:
    mr       r3, r31
    bl       GetActorName
    mr       r31, r3
    b        .L_8007E7A4
.L_8007D754:
    bl       CEvt_Evaluate
    bl       GetItemName
    mr       r31, r3
    b        .L_8007E7A4
.L_8007D764:
    bl       CEvt_Evaluate
    bl       GetStatusIconName
    mr       r31, r3
    b        .L_8007E7A4
.L_8007D774:
    bl       CEvt_Evaluate
    mr.      r31, r3
    bne      .L_8007D794
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    lwz      r31, 0x5d28(r3)
    b        .L_8007E7A4
.L_8007D794:
    cmpwi    r31, 0x1
    bne      .L_8007D7C4
    lis      r3, gGame@ha
    lfs      f1, lbl_8065CF80@sda21(r0)
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    lfs      f0, 0x5d2c(r3)
    fmuls    f0, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007E7A4
.L_8007D7C4:
    cmpwi    r31, 0x2
    bne      .L_8007D7F4
    lis      r3, gGame@ha
    lfs      f1, lbl_8065CF80@sda21(r0)
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    lfs      f0, 0x5d30(r3)
    fmuls    f0, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007E7A4
.L_8007D7F4:
    mr       r4, r31
    addi     r3, r29, 0xdc0
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvaluateName@ha
    mr       r6, r3
    addi     r7, r4, sEvaluateName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x356
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_8007E7A4
.L_8007D828:
    bl       CEvt_Evaluate
    mr       r31, r3
    cmpwi    r31, 0xe4
    beq      .L_8007E324
    bge      .L_8007D874
    cmpwi    r31, 0xd7
    bge      .L_8007D864
    cmpwi    r31, 0xce
    bge      .L_8007D858
    cmpwi    r31, 0xcc
    bge      .L_8007D898
    b        .L_8007E5A8
.L_8007D858:
    cmpwi    r31, 0xd4
    bge      .L_8007D898
    b        .L_8007E5A8
.L_8007D864:
    cmpwi    r31, 0xda
    beq      .L_8007E054
    bge      .L_8007E5A8
    b        .L_8007DB9C
.L_8007D874:
    cmpwi    r31, 0xf7
    bge      .L_8007D888
    cmpwi    r31, 0xeb
    beq      .L_8007D898
    b        .L_8007E5A8
.L_8007D888:
    cmpwi    r31, 0xfb
    bge      .L_8007E5A8
    cmpwi    r31, 0xf9
    bge      .L_8007DB9C
.L_8007D898:
    mr       r3, r30
    bl       CEvt_Evaluate
    extsh    r3, r3
    cmpwi    r3, 0x2710
    blt      .L_8007D95C
    lis      r4, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r4)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_8007D944
.L_8007D8C4:
    lha      r0, 0x4(r4)
    cmpw     r0, r3
    bne      .L_8007D938
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r3, -0x67a0(r3)
    cmplwi   r3, 0x0
    beq      .L_8007D930
    lwz      r0, 0x228(r3)
    li       r5, 0x0
    lwz      r6, 0x22c(r3)
    li       r4, 0x1
    and      r0, r0, r5
    and      r4, r6, r4
    xor      r4, r4, r5
    xor      r0, r0, r5
    or.      r0, r4, r0
    beq      .L_8007D930
    b        .L_8007D948
.L_8007D930:
    li       r3, 0x0
    b        .L_8007D948
.L_8007D938:
    addi     r4, r4, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_8007D8C4
.L_8007D944:
    li       r3, 0x0
.L_8007D948:
    cmplwi   r3, 0x0
    bne      .L_8007D98C
    li       r3, 0x0
    b        .L_8007D98C
    b        .L_8007D98C
.L_8007D95C:
    extsh.   r0, r3
    blt      .L_8007D980
    lis      r4, gGame@ha
    slwi     r0, r3, 2
    addi     r3, r4, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r3, -0x67a0(r3)
    b        .L_8007D98C
.L_8007D980:
    cmpwi    r3, -0x1
    bne      .L_8007D98C
    lwz      r3, 0x8(r30)
.L_8007D98C:
    cmplwi   r3, 0x0
    beq      .L_8007DB94
    lwz      r4, 0x228(r3)
    li       r6, 0x0
    lwz      r7, 0x22c(r3)
    li       r0, 0x1
    and      r5, r4, r6
    and      r0, r7, r0
    xor      r4, r0, r6
    xor      r0, r5, r6
    or.      r0, r4, r0
    beq      .L_8007DB94
    cmpwi    r31, 0xd6
    beq      .L_8007DAA0
    bge      .L_8007D9F0
    cmpwi    r31, 0xcd
    beq      .L_8007DA1C
    bge      .L_8007D9E0
    cmpwi    r31, 0xcc
    bge      .L_8007DA14
    b        .L_8007E7A4
.L_8007D9E0:
    cmpwi    r31, 0xd4
    beq      .L_8007DA68
    bge      .L_8007DA84
    b        .L_8007E7A4
.L_8007D9F0:
    cmpwi    r31, 0xf7
    beq      .L_8007DAE4
    bge      .L_8007DA08
    cmpwi    r31, 0xeb
    beq      .L_8007DABC
    b        .L_8007E7A4
.L_8007DA08:
    cmpwi    r31, 0xf9
    bge      .L_8007E7A4
    b        .L_8007DB3C
.L_8007DA14:
    lwz      r31, 0x158(r3)
    b        .L_8007E7A4
.L_8007DA1C:
    lfs      f2, 0xfc(r3)
    lfs      f1, lbl_8065CFF4@sda21(r0)
    lfs      f0, lbl_8065CF74@sda21(r0)
    b        .L_8007DA30
.L_8007DA2C:
    fadds    f2, f2, f1
.L_8007DA30:
    fcmpo    cr0, f2, f0
    blt      .L_8007DA2C
    lfs      f0, lbl_8065CFF4@sda21(r0)
    b        .L_8007DA44
.L_8007DA40:
    fsubs    f2, f2, f0
.L_8007DA44:
    fcmpo    cr0, f2, f0
    cror     eq, gt, eq
    beq      .L_8007DA40
    lfs      f0, lbl_8065CFF0@sda21(r0)
    fmuls    f0, f0, f2
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007E7A4
.L_8007DA68:
    lfs      f1, lbl_8065CF80@sda21(r0)
    lfs      f0, 0xc8(r3)
    fmuls    f0, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007E7A4
.L_8007DA84:
    lfs      f1, lbl_8065CF80@sda21(r0)
    lfs      f0, 0xcc(r3)
    fmuls    f0, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007E7A4
.L_8007DAA0:
    lfs      f1, lbl_8065CF80@sda21(r0)
    lfs      f0, 0xd0(r3)
    fmuls    f0, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007E7A4
.L_8007DABC:
    lis      r3, 0x1
    xor      r0, r5, r6
    and      r3, r7, r3
    xor      r3, r3, r6
    or.      r0, r3, r0
    beq      .L_8007DADC
    li       r31, 0x1
    b        .L_8007E7A4
.L_8007DADC:
    li       r31, 0x0
    b        .L_8007E7A4
.L_8007DAE4:
    lwz      r6, 0x48(r29)
    addi     r4, r1, 0xb8
    lwz      r9, 0x4c(r29)
    addi     r5, r1, 0x18
    lwz      r0, 0x50(r29)
    li       r7, 0x0
    stw      r6, 0xb8(r1)
    li       r6, 0x0
    li       r8, 0x0
    stw      r9, 0xbc(r1)
    stw      r0, 0xc0(r1)
    lwz      r12, 0x34(r3)
    lwz      r12, 0x10c(r12)
    mtctr    r12
    bctrl
    lfs      f1, lbl_8065CF80@sda21(r0)
    lfs      f0, 0x18(r1)
    fmuls    f0, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007E7A4
.L_8007DB3C:
    lwz      r5, 0x54(r29)
    addi     r4, r1, 0xac
    lwz      r9, 0x58(r29)
    addi     r6, r1, 0x14
    lwz      r0, 0x5c(r29)
    li       r7, 0x0
    stw      r5, 0xac(r1)
    li       r5, 0x0
    li       r8, 0x0
    stw      r9, 0xb0(r1)
    stw      r0, 0xb4(r1)
    lwz      r12, 0x34(r3)
    lwz      r12, 0x10c(r12)
    mtctr    r12
    bctrl
    lfs      f1, lbl_8065CF80@sda21(r0)
    lfs      f0, 0x14(r1)
    fmuls    f0, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007E7A4
.L_8007DB94:
    li       r31, -0x1
    b        .L_8007E7A4
.L_8007DB9C:
    mr       r3, r30
    bl       CEvt_Evaluate
    lbz      r0, 0x1(r3)
    lbz      r4, 0x0(r3)
    slwi     r0, r0, 8
    add      r0, r4, r0
    extsh    r29, r0
    cmpwi    r29, 0x2710
    blt      .L_8007DC70
    lis      r4, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r4)
    li       r6, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_8007DC58
.L_8007DBD8:
    lha      r0, 0x4(r4)
    cmpw     r0, r29
    bne      .L_8007DC4C
    lis      r4, gEventObjectIds@ha
    lis      r5, gGame@ha
    addi     r4, r4, gEventObjectIds@l
    slwi     r0, r6, 2
    add      r4, r4, r0
    addi     r5, r5, gGame@l
    lha      r0, 0x6(r4)
    addis    r4, r5, 0x1
    slwi     r0, r0, 2
    add      r4, r4, r0
    lwz      r29, -0x67a0(r4)
    cmplwi   r29, 0x0
    beq      .L_8007DC44
    lwz      r0, 0x228(r29)
    li       r5, 0x0
    lwz      r6, 0x22c(r29)
    li       r4, 0x1
    and      r0, r0, r5
    and      r4, r6, r4
    xor      r4, r4, r5
    xor      r0, r0, r5
    or.      r0, r4, r0
    beq      .L_8007DC44
    b        .L_8007DC5C
.L_8007DC44:
    li       r29, 0x0
    b        .L_8007DC5C
.L_8007DC4C:
    addi     r4, r4, 0x4
    addi     r6, r6, 0x1
    bdnz     .L_8007DBD8
.L_8007DC58:
    li       r29, 0x0
.L_8007DC5C:
    cmplwi   r29, 0x0
    bne      .L_8007DCA0
    li       r29, 0x0
    b        .L_8007DCA0
    b        .L_8007DCA0
.L_8007DC70:
    extsh.   r0, r29
    blt      .L_8007DC94
    lis      r4, gGame@ha
    slwi     r0, r29, 2
    addi     r4, r4, gGame@l
    addis    r4, r4, 0x1
    add      r4, r4, r0
    lwz      r29, -0x67a0(r4)
    b        .L_8007DCA0
.L_8007DC94:
    cmpwi    r29, -0x1
    bne      .L_8007DCA0
    lwz      r29, 0x8(r30)
.L_8007DCA0:
    cmplwi   r29, 0x0
    beq      .L_8007E04C
    lwz      r0, 0x228(r29)
    li       r5, 0x0
    lwz      r6, 0x22c(r29)
    li       r4, 0x1
    and      r0, r0, r5
    and      r4, r6, r4
    xor      r4, r4, r5
    xor      r0, r0, r5
    or.      r0, r4, r0
    beq      .L_8007E04C
    lbz      r0, 0x3(r3)
    addi     r4, r3, 0x4
    lbz      r3, 0x2(r3)
    li       r6, 0x0
    slwi     r0, r0, 8
    add      r0, r3, r0
    extsh.   r3, r0
    mr       r5, r3
    ble      .L_8007DE28
    cmpwi    r3, 0x8
    subi     r7, r3, 0x8
    ble      .L_8007DDE8
    addi     r0, r7, 0x7
    addi     r3, r1, 0xd0
    srwi     r0, r0, 3
    mtctr    r0
    cmpwi    r7, 0x0
    ble      .L_8007DDE8
.L_8007DD18:
    lbz      r0, 0x1(r4)
    addi     r6, r6, 0x8
    lbz      r7, 0x0(r4)
    slwi     r0, r0, 8
    add      r0, r7, r0
    extsh    r0, r0
    stw      r0, 0x0(r3)
    lbz      r0, 0x3(r4)
    lbz      r7, 0x2(r4)
    slwi     r0, r0, 8
    add      r0, r7, r0
    extsh    r0, r0
    stw      r0, 0x4(r3)
    lbz      r0, 0x5(r4)
    lbz      r7, 0x4(r4)
    slwi     r0, r0, 8
    add      r0, r7, r0
    extsh    r0, r0
    stw      r0, 0x8(r3)
    lbz      r0, 0x7(r4)
    lbz      r7, 0x6(r4)
    slwi     r0, r0, 8
    add      r0, r7, r0
    extsh    r0, r0
    stw      r0, 0xc(r3)
    lbz      r0, 0x9(r4)
    lbz      r7, 0x8(r4)
    slwi     r0, r0, 8
    add      r0, r7, r0
    extsh    r0, r0
    stw      r0, 0x10(r3)
    lbz      r0, 0xb(r4)
    lbz      r7, 0xa(r4)
    slwi     r0, r0, 8
    add      r0, r7, r0
    extsh    r0, r0
    stw      r0, 0x14(r3)
    lbz      r0, 0xd(r4)
    lbz      r7, 0xc(r4)
    slwi     r0, r0, 8
    add      r0, r7, r0
    extsh    r0, r0
    stw      r0, 0x18(r3)
    lbz      r0, 0xf(r4)
    lbz      r7, 0xe(r4)
    addi     r4, r4, 0x10
    slwi     r0, r0, 8
    add      r0, r7, r0
    extsh    r0, r0
    stw      r0, 0x1c(r3)
    addi     r3, r3, 0x20
    bdnz     .L_8007DD18
.L_8007DDE8:
    slwi     r3, r6, 2
    addi     r7, r1, 0xd0
    subf     r0, r6, r5
    add      r7, r7, r3
    mtctr    r0
    cmpw     r6, r5
    bge      .L_8007DE28
.L_8007DE04:
    lbz      r0, 0x1(r4)
    lbz      r3, 0x0(r4)
    addi     r4, r4, 0x2
    slwi     r0, r0, 8
    add      r0, r3, r0
    extsh    r0, r0
    stw      r0, 0x0(r7)
    addi     r7, r7, 0x4
    bdnz     .L_8007DE04
.L_8007DE28:
    cmpwi    r31, 0xd9
    beq      .L_8007DECC
    bge      .L_8007DE44
    cmpwi    r31, 0xd7
    beq      .L_8007DE5C
    bge      .L_8007DE94
    b        .L_8007E7A4
.L_8007DE44:
    cmpwi    r31, 0xfa
    beq      .L_8007DFA8
    bge      .L_8007E7A4
    cmpwi    r31, 0xf9
    bge      .L_8007DF04
    b        .L_8007E7A4
.L_8007DE5C:
    mr       r4, r29
    addi     r3, r1, 0x9c
    addi     r6, r1, 0xd0
    bl       GetEventJointWorldPosition
    lfs      f1, lbl_8065CF80@sda21(r0)
    lis      r3, __vt__4CVec@ha
    lfs      f0, 0x9c(r1)
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0xa8(r1)
    fmuls    f0, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007E7A4
.L_8007DE94:
    mr       r4, r29
    addi     r3, r1, 0x8c
    addi     r6, r1, 0xd0
    bl       GetEventJointWorldPosition
    lfs      f1, lbl_8065CF80@sda21(r0)
    lis      r3, __vt__4CVec@ha
    lfs      f0, 0x90(r1)
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x98(r1)
    fmuls    f0, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007E7A4
.L_8007DECC:
    mr       r4, r29
    addi     r3, r1, 0x7c
    addi     r6, r1, 0xd0
    bl       GetEventJointWorldPosition
    lfs      f1, lbl_8065CF80@sda21(r0)
    lis      r3, __vt__4CVec@ha
    lfs      f0, 0x84(r1)
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x88(r1)
    fmuls    f0, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007E7A4
.L_8007DF04:
    mr       r4, r29
    addi     r3, r1, 0x6c
    addi     r6, r1, 0xd0
    bl       GetEventJointWorldPosition
    lwz      r6, 0xc8(r29)
    addi     r3, r1, 0x50
    lwz      r0, 0xcc(r29)
    addi     r4, r1, 0x6c
    addi     r5, r1, 0x60
    stw      r6, 0x60(r1)
    stw      r0, 0x64(r1)
    lwz      r0, 0xd0(r29)
    stw      r0, 0x68(r1)
    bl       __mi__4CVecCF3Vec
    lfs      f2, 0x50(r1)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    lfs      f1, 0x54(r1)
    lfs      f0, 0x58(r1)
    mr       r3, r29
    stfs     f2, 0xc4(r1)
    addi     r4, r1, 0xc4
    addi     r5, r1, 0x10
    li       r6, 0x0
    stfs     f1, 0xc8(r1)
    li       r7, 0x0
    li       r8, 0x0
    stfs     f0, 0xcc(r1)
    stw      r0, 0x5c(r1)
    stw      r0, 0x78(r1)
    lwz      r12, 0x34(r29)
    lwz      r12, 0x10c(r12)
    mtctr    r12
    bctrl
    lfs      f1, lbl_8065CF80@sda21(r0)
    lfs      f0, 0x10(r1)
    fmuls    f0, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007E7A4
.L_8007DFA8:
    mr       r4, r29
    addi     r3, r1, 0x40
    addi     r6, r1, 0xd0
    bl       GetEventJointWorldPosition
    lwz      r6, 0xc8(r29)
    addi     r3, r1, 0x24
    lwz      r0, 0xcc(r29)
    addi     r4, r1, 0x40
    addi     r5, r1, 0x34
    stw      r6, 0x34(r1)
    stw      r0, 0x38(r1)
    lwz      r0, 0xd0(r29)
    stw      r0, 0x3c(r1)
    bl       __mi__4CVecCF3Vec
    lfs      f2, 0x24(r1)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    lfs      f1, 0x28(r1)
    lfs      f0, 0x2c(r1)
    mr       r3, r29
    stfs     f2, 0xc4(r1)
    addi     r4, r1, 0xc4
    addi     r6, r1, 0xc
    li       r5, 0x0
    stfs     f1, 0xc8(r1)
    li       r7, 0x0
    li       r8, 0x0
    stfs     f0, 0xcc(r1)
    stw      r0, 0x30(r1)
    stw      r0, 0x4c(r1)
    lwz      r12, 0x34(r29)
    lwz      r12, 0x10c(r12)
    mtctr    r12
    bctrl
    lfs      f1, lbl_8065CF80@sda21(r0)
    lfs      f0, 0xc(r1)
    fmuls    f0, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007E7A4
.L_8007E04C:
    li       r31, -0x1
    b        .L_8007E7A4
.L_8007E054:
    mr       r3, r30
    bl       CEvt_Evaluate
    lbz      r0, 0x1(r3)
    lbz      r4, 0x0(r3)
    slwi     r0, r0, 8
    add      r0, r4, r0
    extsh    r29, r0
    cmpwi    r29, 0x2710
    blt      .L_8007E128
    lis      r4, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r4)
    li       r6, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_8007E110
.L_8007E090:
    lha      r0, 0x4(r4)
    cmpw     r0, r29
    bne      .L_8007E104
    lis      r4, gEventObjectIds@ha
    lis      r5, gGame@ha
    addi     r4, r4, gEventObjectIds@l
    slwi     r0, r6, 2
    add      r4, r4, r0
    addi     r5, r5, gGame@l
    lha      r0, 0x6(r4)
    addis    r4, r5, 0x1
    slwi     r0, r0, 2
    add      r4, r4, r0
    lwz      r29, -0x67a0(r4)
    cmplwi   r29, 0x0
    beq      .L_8007E0FC
    lwz      r0, 0x228(r29)
    li       r5, 0x0
    lwz      r6, 0x22c(r29)
    li       r4, 0x1
    and      r0, r0, r5
    and      r4, r6, r4
    xor      r4, r4, r5
    xor      r0, r0, r5
    or.      r0, r4, r0
    beq      .L_8007E0FC
    b        .L_8007E114
.L_8007E0FC:
    li       r29, 0x0
    b        .L_8007E114
.L_8007E104:
    addi     r4, r4, 0x4
    addi     r6, r6, 0x1
    bdnz     .L_8007E090
.L_8007E110:
    li       r29, 0x0
.L_8007E114:
    cmplwi   r29, 0x0
    bne      .L_8007E158
    li       r29, 0x0
    b        .L_8007E158
    b        .L_8007E158
.L_8007E128:
    extsh.   r0, r29
    blt      .L_8007E14C
    lis      r4, gGame@ha
    slwi     r0, r29, 2
    addi     r4, r4, gGame@l
    addis    r4, r4, 0x1
    add      r4, r4, r0
    lwz      r29, -0x67a0(r4)
    b        .L_8007E158
.L_8007E14C:
    cmpwi    r29, -0x1
    bne      .L_8007E158
    lwz      r29, 0x8(r30)
.L_8007E158:
    lbz      r0, 0x3(r3)
    lbz      r3, 0x2(r3)
    slwi     r0, r0, 8
    add      r0, r3, r0
    extsh    r3, r0
    cmpwi    r3, 0x2710
    blt      .L_8007E224
    lis      r4, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r4)
    li       r5, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_8007E20C
.L_8007E18C:
    lha      r0, 0x4(r4)
    cmpw     r0, r3
    bne      .L_8007E200
    lis      r3, gEventObjectIds@ha
    lis      r4, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r5, 2
    add      r3, r3, r0
    addi     r4, r4, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r4, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r3, -0x67a0(r3)
    cmplwi   r3, 0x0
    beq      .L_8007E1F8
    lwz      r0, 0x228(r3)
    li       r5, 0x0
    lwz      r6, 0x22c(r3)
    li       r4, 0x1
    and      r0, r0, r5
    and      r4, r6, r4
    xor      r4, r4, r5
    xor      r0, r0, r5
    or.      r0, r4, r0
    beq      .L_8007E1F8
    b        .L_8007E210
.L_8007E1F8:
    li       r3, 0x0
    b        .L_8007E210
.L_8007E200:
    addi     r4, r4, 0x4
    addi     r5, r5, 0x1
    bdnz     .L_8007E18C
.L_8007E20C:
    li       r3, 0x0
.L_8007E210:
    cmplwi   r3, 0x0
    bne      .L_8007E254
    li       r3, 0x0
    b        .L_8007E254
    b        .L_8007E254
.L_8007E224:
    extsh.   r0, r3
    blt      .L_8007E248
    lis      r4, gGame@ha
    slwi     r0, r3, 2
    addi     r3, r4, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r3, -0x67a0(r3)
    b        .L_8007E254
.L_8007E248:
    cmpwi    r3, -0x1
    bne      .L_8007E254
    lwz      r3, 0x8(r30)
.L_8007E254:
    cmplwi   r29, 0x0
    beq      .L_8007E31C
    lwz      r0, 0x228(r29)
    li       r5, 0x0
    lwz      r4, 0x22c(r29)
    li       r6, 0x1
    and      r0, r0, r5
    and      r4, r4, r6
    xor      r4, r4, r5
    xor      r0, r0, r5
    or.      r0, r4, r0
    beq      .L_8007E31C
    cmplwi   r3, 0x0
    beq      .L_8007E31C
    lwz      r0, 0x228(r3)
    lwz      r4, 0x22c(r3)
    and      r0, r0, r5
    and      r4, r4, r6
    xor      r4, r4, r5
    xor      r0, r0, r5
    or.      r0, r4, r0
    beq      .L_8007E31C
    lfs      f3, 0xc8(r3)
    lfs      f1, 0xc8(r29)
    lfs      f2, 0xd0(r3)
    lfs      f0, 0xd0(r29)
    fsubs    f1, f3, f1
    fsubs    f2, f2, f0
    bl       atan2
    frsp     f3, f1
    lfs      f2, 0xfc(r29)
    lfs      f1, lbl_8065CFF4@sda21(r0)
    lfs      f0, lbl_8065CF74@sda21(r0)
    fsubs    f2, f3, f2
    b        .L_8007E2E4
.L_8007E2E0:
    fadds    f2, f2, f1
.L_8007E2E4:
    fcmpo    cr0, f2, f0
    blt      .L_8007E2E0
    lfs      f0, lbl_8065CFF4@sda21(r0)
    b        .L_8007E2F8
.L_8007E2F4:
    fsubs    f2, f2, f0
.L_8007E2F8:
    fcmpo    cr0, f2, f0
    cror     eq, gt, eq
    beq      .L_8007E2F4
    lfs      f0, lbl_8065CFF0@sda21(r0)
    fmuls    f0, f0, f2
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007E7A4
.L_8007E31C:
    li       r31, -0x1
    b        .L_8007E7A4
.L_8007E324:
    mr       r3, r30
    bl       CEvt_Evaluate
    lbz      r0, 0x1(r3)
    lbz      r4, 0x0(r3)
    slwi     r0, r0, 8
    add      r0, r4, r0
    extsh    r4, r0
    cmpwi    r4, 0x2710
    blt      .L_8007E3F8
    lis      r5, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r5)
    li       r6, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_8007E3E0
.L_8007E360:
    lha      r0, 0x4(r5)
    cmpw     r0, r4
    bne      .L_8007E3D4
    lis      r4, gEventObjectIds@ha
    lis      r5, gGame@ha
    addi     r4, r4, gEventObjectIds@l
    slwi     r0, r6, 2
    add      r4, r4, r0
    addi     r5, r5, gGame@l
    lha      r0, 0x6(r4)
    addis    r4, r5, 0x1
    slwi     r0, r0, 2
    add      r4, r4, r0
    lwz      r4, -0x67a0(r4)
    cmplwi   r4, 0x0
    beq      .L_8007E3CC
    lwz      r0, 0x228(r4)
    li       r6, 0x0
    lwz      r7, 0x22c(r4)
    li       r5, 0x1
    and      r0, r0, r6
    and      r5, r7, r5
    xor      r5, r5, r6
    xor      r0, r0, r6
    or.      r0, r5, r0
    beq      .L_8007E3CC
    b        .L_8007E3E4
.L_8007E3CC:
    li       r4, 0x0
    b        .L_8007E3E4
.L_8007E3D4:
    addi     r5, r5, 0x4
    addi     r6, r6, 0x1
    bdnz     .L_8007E360
.L_8007E3E0:
    li       r4, 0x0
.L_8007E3E4:
    cmplwi   r4, 0x0
    bne      .L_8007E428
    li       r4, 0x0
    b        .L_8007E428
    b        .L_8007E428
.L_8007E3F8:
    extsh.   r0, r4
    blt      .L_8007E41C
    lis      r5, gGame@ha
    slwi     r0, r4, 2
    addi     r4, r5, gGame@l
    addis    r4, r4, 0x1
    add      r4, r4, r0
    lwz      r4, -0x67a0(r4)
    b        .L_8007E428
.L_8007E41C:
    cmpwi    r4, -0x1
    bne      .L_8007E428
    lwz      r4, 0x8(r30)
.L_8007E428:
    lbz      r0, 0x3(r3)
    lbz      r3, 0x2(r3)
    slwi     r0, r0, 8
    add      r0, r3, r0
    extsh    r5, r0
    cmpwi    r5, 0x2710
    blt      .L_8007E4F4
    lis      r3, gEventObjectIds@ha
    lwzu     r0, gEventObjectIds@l(r3)
    li       r6, 0x0
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_8007E4DC
.L_8007E45C:
    lha      r0, 0x4(r3)
    cmpw     r0, r5
    bne      .L_8007E4D0
    lis      r3, gEventObjectIds@ha
    lis      r5, gGame@ha
    addi     r3, r3, gEventObjectIds@l
    slwi     r0, r6, 2
    add      r3, r3, r0
    addi     r5, r5, gGame@l
    lha      r0, 0x6(r3)
    addis    r3, r5, 0x1
    slwi     r0, r0, 2
    add      r3, r3, r0
    lwz      r5, -0x67a0(r3)
    cmplwi   r5, 0x0
    beq      .L_8007E4C8
    lwz      r0, 0x228(r5)
    li       r6, 0x0
    lwz      r7, 0x22c(r5)
    li       r3, 0x1
    and      r0, r0, r6
    and      r3, r7, r3
    xor      r3, r3, r6
    xor      r0, r0, r6
    or.      r0, r3, r0
    beq      .L_8007E4C8
    b        .L_8007E4E0
.L_8007E4C8:
    li       r5, 0x0
    b        .L_8007E4E0
.L_8007E4D0:
    addi     r3, r3, 0x4
    addi     r6, r6, 0x1
    bdnz     .L_8007E45C
.L_8007E4DC:
    li       r5, 0x0
.L_8007E4E0:
    cmplwi   r5, 0x0
    bne      .L_8007E524
    li       r5, 0x0
    b        .L_8007E524
    b        .L_8007E524
.L_8007E4F4:
    extsh.   r0, r5
    blt      .L_8007E518
    lis      r3, gGame@ha
    slwi     r0, r5, 2
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    add      r3, r3, r0
    lwz      r5, -0x67a0(r3)
    b        .L_8007E524
.L_8007E518:
    cmpwi    r5, -0x1
    bne      .L_8007E524
    lwz      r5, 0x8(r30)
.L_8007E524:
    cmplwi   r4, 0x0
    beq      .L_8007E5A0
    lwz      r0, 0x228(r4)
    li       r6, 0x0
    lwz      r3, 0x22c(r4)
    li       r7, 0x1
    and      r0, r0, r6
    and      r3, r3, r7
    xor      r3, r3, r6
    xor      r0, r0, r6
    or.      r0, r3, r0
    beq      .L_8007E5A0
    cmplwi   r5, 0x0
    beq      .L_8007E5A0
    lwz      r0, 0x228(r5)
    lwz      r3, 0x22c(r5)
    and      r0, r0, r6
    and      r3, r3, r7
    xor      r3, r3, r6
    xor      r0, r0, r6
    or.      r0, r3, r0
    beq      .L_8007E5A0
    addi     r3, r4, 0xc8
    addi     r4, r5, 0xc8
    bl       PSVECDistance
    lfs      f0, lbl_8065CF80@sda21(r0)
    fmuls    f0, f0, f1
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007E7A4
.L_8007E5A0:
    li       r31, -0x1
    b        .L_8007E7A4
.L_8007E5A8:
    mr       r4, r31
    addi     r3, r29, 0xde4
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvaluateName@ha
    mr       r6, r3
    addi     r7, r4, sEvaluateName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x3d1
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
    b        .L_8007E7A4
.L_8007E5DC:
    bl       CEvt_Evaluate
    mr       r28, r3
    bl       rand
    addi     r4, r28, 0x1
    divw     r0, r3, r4
    mullw    r0, r0, r4
    subf     r31, r0, r3
    b        .L_8007E7A4
.L_8007E5FC:
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x124(r1)
    lfd      f3, lbl_8065CF88@sda21(r0)
    stw      r0, 0x120(r1)
    lfs      f1, lbl_8065CF80@sda21(r0)
    lfd      f2, 0x120(r1)
    lfs      f0, lbl_8065CFE8@sda21(r0)
    fsubs    f2, f2, f3
    fdivs    f1, f2, f1
    fmuls    f1, f0, f1
    bl       sin
    frsp     f1, f1
    lfs      f0, lbl_8065CF80@sda21(r0)
    fmuls    f0, f0, f1
    fctiwz   f0, f0
    stfd     f0, 0x128(r1)
    lwz      r31, 0x12c(r1)
    b        .L_8007E7A4
.L_8007E64C:
    bl       CEvt_Evaluate
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x12c(r1)
    lfd      f3, lbl_8065CF88@sda21(r0)
    stw      r0, 0x128(r1)
    lfs      f1, lbl_8065CF80@sda21(r0)
    lfd      f2, 0x128(r1)
    lfs      f0, lbl_8065CFE8@sda21(r0)
    fsubs    f2, f2, f3
    fdivs    f1, f2, f1
    fmuls    f1, f0, f1
    bl       cos
    frsp     f1, f1
    lfs      f0, lbl_8065CF80@sda21(r0)
    fmuls    f0, f0, f1
    fctiwz   f0, f0
    stfd     f0, 0x120(r1)
    lwz      r31, 0x124(r1)
    b        .L_8007E7A4
.L_8007E69C:
    bl       CEvt_Evaluate
    extsb    r31, r3
    mr       r3, r30
    bl       CEvt_Evaluate
    extsb.   r0, r31
    mr       r29, r3
    mr       r28, r31
    bge      .L_8007E6C0
    neg      r28, r31
.L_8007E6C0:
    mr       r3, r30
    bl       CEvt_Evaluate
    mullw    r4, r29, r28
    addi     r0, r31, 0x4
    cmplwi   r0, 0x8
    add      r6, r3, r4
    bgt      .L_8007E76C
    lis      r3, jumptable_802252B0@ha
    slwi     r0, r0, 2
    addi     r3, r3, jumptable_802252B0@l
    lwzx     r0, r3, r0
    mtctr    r0
    bctr
.L_8007E6F4:
    lbz      r31, 0x0(r6)
    b        .L_8007E7A4
.L_8007E6FC:
    lbz      r31, 0x0(r6)
    extsb    r31, r31
    b        .L_8007E7A4
.L_8007E708:
    lbz      r0, 0x1(r6)
    lbz      r3, 0x0(r6)
    slwi     r0, r0, 8
    add      r0, r3, r0
    clrlwi   r0, r0, 16
    mr       r31, r0
    b        .L_8007E7A4
.L_8007E724:
    lbz      r0, 0x1(r6)
    lbz      r3, 0x0(r6)
    slwi     r0, r0, 8
    add      r0, r3, r0
    extsh    r0, r0
    mr       r31, r0
    b        .L_8007E7A4
.L_8007E740:
    lbz      r3, 0x2(r6)
    lbz      r0, 0x1(r6)
    lbz      r5, 0x3(r6)
    slwi     r4, r3, 16
    slwi     r0, r0, 8
    lbz      r3, 0x0(r6)
    slwi     r5, r5, 24
    add      r0, r0, r4
    add      r31, r0, r5
    add      r31, r3, r31
    b        .L_8007E7A4
.L_8007E76C:
    li       r31, 0x0
    b        .L_8007E7A4
.L_8007E774:
    mr       r4, r28
    addi     r3, r29, 0xe04
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sEvaluateName@ha
    mr       r6, r3
    addi     r7, r4, sEvaluateName@l
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x3ff
    li       r5, lbl_8065CFA8@sda21
    crclr    cr1eq
    bl       OSPanic
.L_8007E7A4:
    lwz      r0, 0x144(r1)
    mr       r3, r31
    lwz      r31, 0x13c(r1)
    lwz      r30, 0x138(r1)
    lwz      r29, 0x134(r1)
    lwz      r28, 0x130(r1)
    mtlr     r0
    addi     r1, r1, 0x140
    blr
*/

// world position of the joint path leads to, path holds a sibling count per level
/**
 * @note Address: 0x8007E7C8
 * @note Size: 0x2C
 */
extern "C" CVec GetEventJointWorldPosition(CJObj* obj, s32 depth, s32* path)
{
	return CJObj_GetJointWorldPositionByPath(obj, depth, path);
}

extern "C" CVec GetEventJObjWorldPosition(CJObj* obj, HSD_JObj* jobj);

/**
 * @note Address: 0x8007E7F4
 * @note Size: 0xAC
 */
extern "C" CVec CJObj_GetJointWorldPositionByPath(CJObj* obj, s32 depth, s32* path)
{
	// CJObj::mJObj, game/cjobj.h would pull 0x24 bytes of .rodata into this file
#if defined(VERSION_JP)
	return GetJointPosition(FindJointByPath(*(HSD_JObj**)obj, path, depth));
#else
	return GetEventJObjWorldPosition(obj, GetJointByPath(*(HSD_JObj**)obj, path, depth));
#endif
}

/*
    stwu     r1, -0x10(r1)
    mflr     r0
    lwz      r7, 0x0(r4)
    stw      r0, 0x14(r1)
    cmplwi   r7, 0x0
    stw      r31, 0xc(r1)
    mr       r31, r3
    bne      .L_8007E81C
    li       r7, 0x0
    b        .L_8007E880
.L_8007E81C:
    li       r3, 0x0
    b        .L_8007E878
.L_8007E824:
    cmplwi   r7, 0x0
    bne      .L_8007E834
    li       r7, 0x0
    b        .L_8007E838
.L_8007E834:
    lwz      r7, 0x10(r7)
.L_8007E838:
    lwz      r0, 0x0(r6)
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_8007E870
.L_8007E848:
    cmplwi   r7, 0x0
    bne      .L_8007E858
    li       r7, 0x0
    b        .L_8007E85C
.L_8007E858:
    lwz      r7, 0x8(r7)
.L_8007E85C:
    cmplwi   r7, 0x0
    bne      .L_8007E86C
    li       r7, 0x0
    b        .L_8007E880
.L_8007E86C:
    bdnz     .L_8007E848
.L_8007E870:
    addi     r6, r6, 0x4
    addi     r3, r3, 0x1
.L_8007E878:
    cmpw     r3, r5
    blt      .L_8007E824
.L_8007E880:
    mr       r3, r31
    mr       r5, r7
    bl       GetEventJObjWorldPosition
    lwz      r0, 0x14(r1)
    lwz      r31, 0xc(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: 0x8007E8A0
 * @note Size: 0xD8
 */
extern "C" CVec GetEventJObjWorldPosition(CJObj* obj, HSD_JObj* jobj)
{
	return GetJointPosition(jobj);
}

/**
 * @note Address: 0x8007E978
 * @note Size: 0x4
 */
extern "C" void fn_8007E978(void)
{
}

extern "C" s32 InfoScrn_IsBatteryGaugeFull(void* hud);
extern "C" s32 HasCameraAnimation(XObj* obj);

static inline void EvtWaitFor(CEvt* evt, u8 command, u8* start, s32 ready, s32& yield)
{
	if (command == EVT_CMD_WAIT) {
		if (ready == 0) {
			evt->mData.mScript = start;
			yield              = 1;
		}
	} else
		evt->_10 = ready;
}

// a wipe is done once its size stops
static inline bool EvtZero2(const CScrnWipe& wipe)
{
	return 0.0f == wipe.mSpeedWidth && 0.0f == wipe.mSpeedHeight;
}

static inline bool EvtZero3(const f32* p)
{
	return 0.0f == p[0] && 0.0f == p[1] && 0.0f == p[2];
}

// a screen color is done once its speed is all zero
static inline bool EvtZero4(const ScrnColorF& speed)
{
	return 0.0f == speed.r && 0.0f == speed.g && 0.0f == speed.b && 0.0f == speed.a;
}

extern "C" {
void ConfigureSubwindowCamera(void* effect, Vec* pos, f32 pitch, f32 yaw, f32 arg1, f32 arg2);
void AddMainMenuExtraRow(void* object, s32 arg1, s32 arg2);
}

// todo: pools 4, 5 and 17 keep flag bits right after XObj, no idea what class
#define EVT_CALL_BITS(pool) (*(u32**)(gGame.mXObjs.mPools[pool + 3].mObjects + sizeof(XObj)))

/**
 * @note Address: 0x80078754
 * @note Size: 0x1D38
 */
s32 CEvt::_cmd_CALL(u8, s32)
{
	s32 yield  = 0;
	u8* start  = mData.mScript - 1;
	u8* begin  = mData.mScript;
	u16 length = EvtRead16(begin);
	mData.mScript += 2;
	u8* end      = begin + length;
	XObj* target = CEvt_GetObj(this, (s16)CEvt_Evaluate(this));
	s32 count    = 0;
	s32 args[24];
	for (s32 i = 0;; i++) {
		if (mData.mScript >= end)
			break;
		if (i < 24) {
			args[i] = CEvt_Evaluate(this);
			count++;
		}
	}

	switch ((s32)target) {
	case -10: {
		f32 d = args[4], c = args[3], b = args[2], a = args[1];
		XObj* obj       = CEvt_GetObj(this, (s16)args[0]);
		obj->mAnchorX   = obj->mMoveTranslate.x;
		obj->mAnchorZ   = obj->mMoveTranslate.z;
		obj->mLimitPosX = a;
		obj->mLimitNegX = b;
		obj->mLimitPosZ = c;
		obj->mLimitNegZ = d;
		break;
	}
	case -11: {
		s16 id    = args[0];
		s32 flags = args[1];
		if (flags == -1) {
			XObj* obj = CEvt_GetObj(this, id);
			obj->_8B0 = 0;
			obj->_8B0 |= 0x80000000;
		} else {
			XObj* obj = CEvt_GetObj(this, id);
			obj->_8B0 &= ~0x80000000;
			obj->_8B0 |= flags;
		}
		break;
	}
	case -12:
	case -13:
	case -14:
	case -15:
		if (_1C != 0) {
			if (gGame.mStatus.mFlags & 0x800) {
				CPad pad(0);
				HandleOXMenuInput(&gGame.GetPlayer()->mOXMenu, &pad);
				mData.mScript = start;
				yield         = 1;
			} else {
				_1C = 0;
				_10 = EVT_PLAYER.mOXMenu.mConfirmedIndex;
			}
		} else {
			if (EVT_PLAYER.mAttachmentRequest < 0) {
				s32 mode = 2;
				if ((s32)target == -12)
					mode = 2;
				else if ((s32)target == -13)
					mode = 1;
				else if ((s32)target == -14)
					mode = 0;
				else if ((s32)target == -15)
					mode = 3;
				OpenPlayerYesNoMenu(&EVT_PLAYER, mode, args[0]);
				_1C = 1;
			}
			mData.mScript = start;
			yield         = 1;
		}
		break;
	case -16:
		if (args[0] == 231) {
			gGame.mSubwin._134 = args[1];
			gGame.mSubwin._138 = args[2];
			gGame.mSubwin._13C = args[3];
			gGame.mSubwin._140 = args[4];
		} else if (args[0] == 225) {
			Vec pos = { 0.0f, 0.0f, 0.0f };
			pos.x   = args[1] / 100.0f;
			pos.y   = args[2] / 100.0f;
			pos.z   = args[3] / 100.0f;
			ConfigureSubwindowCamera(&gGame.mSubwin, &pos, DEG2RAD * args[4], DEG2RAD * args[5], args[6], args[7]);
			gGame.mSubwin.mEnabled      = 1;
			gGame.mSubwin.mFlags.mFlags = 0;
		} else if (args[0] == 209 || args[0] == 230) {
			gGame.mSubwin.mEnabled = 0;
		}
		break;
	case -17: {
		s16 id = args[0];
		if (id > 0) {
			u32* bits = EVT_CALL_BITS(5);
			bits[(u32)id >> 5] |= 1U << (id & 31);
		} else if (id == -2) {
			u32* bits = EVT_CALL_BITS(5);
			for (int i = 0; i < 128; i++)
				bits[i] = -1;
		}
		break;
	}
	case -18: {
		s16 id = args[0];
		if (id > 0) {
			u32* bits = EVT_CALL_BITS(5);
			bits[(u32)id >> 5] &= ~(1U << (id & 31));
		} else if (id == -2) {
			u32* bits = EVT_CALL_BITS(5);
			for (int i = 0; i < 128; i++)
				bits[i] = 0;
		}
		break;
	}
	case -19: {
		s16 id = args[0];
		_10    = (EVT_CALL_BITS(5)[(u32)id >> 5] & (1U << (id & 31))) != 0;
		break;
	}
	case -20: {
		s16 id = args[0];
		if (id > 0) {
			u32* bits = EVT_CALL_BITS(4);
			bits[(u32)id >> 5] |= 1U << (id & 31);
		} else if (id == -2) {
			u32* bits = EVT_CALL_BITS(4);
			for (int i = 0; i < 128; i++)
				bits[i] = -1;
		}
		break;
	}
	case -21: {
		s16 id = args[0];
		if (id > 0) {
			u32* bits = EVT_CALL_BITS(4);
			bits[(u32)id >> 5] &= ~(1U << (id & 31));
		} else if (id == -2) {
			u32* bits = EVT_CALL_BITS(4);
			for (int i = 0; i < 128; i++)
				bits[i] = 0;
		}
		break;
	}
	case -22: {
		s16 id = args[0];
		_10    = (EVT_CALL_BITS(4)[(u32)id >> 5] & (1U << (id & 31))) != 0;
		break;
	}
	case -23: {
		s16 id = args[0];
		if (id > 0) {
			u32* bits = EVT_CALL_BITS(17);
			bits[(u32)id >> 5] |= 1U << (id & 31);
		} else if (id == -2) {
			u32* bits = EVT_CALL_BITS(17);
			for (int i = 0; i < 128; i++)
				bits[i] = -1;
		}
		break;
	}
	case -24: {
		s16 id = args[0];
		if (id > 0) {
			u32* bits = EVT_CALL_BITS(17);
			bits[(u32)id >> 5] &= ~(1U << (id & 31));
		} else if (id == -2) {
			u32* bits = EVT_CALL_BITS(17);
			for (int i = 0; i < 128; i++)
				bits[i] = 0;
		}
		break;
	}
	case -25: {
		s16 id = args[0];
		_10    = (EVT_CALL_BITS(17)[(u32)id >> 5] & (1U << (id & 31))) != 0;
		break;
	}
	case -26: {
		s16 id    = args[0];
		s32 bit   = args[1];
		u64 mask  = (u64)1 << bit;
		XObj* obj = CEvt_GetObj(this, id);
		if (obj != NULL) {
			obj->mStatus.mFlags |= mask;
			if (mask & 0x20000) {
				u32 flags[5] = { 0, 0, 0, 0, 0 };
				flags[0]     = obj->mStatus.mFlags & 0x800000;
				flags[1]     = obj->mStatus.mFlags & 0x200000;
				flags[2]     = obj->mStatus.mFlags & 0x40;
				flags[3]     = obj->mStatus.mFlags & 0x20000;
				flags[4]     = obj->mStatus.mFlags & 0x400000;
				obj->vfD8(flags);
			}
			if (mask & ((u64)0x4000 << 32)) {
				XObjChr* player = gGame.GetPlayer();
				if ((s16)obj->GetPool() == 8 && obj == player->mLift.mTarget) {
					s16 item = obj->GetObjFlags();
					switch (item) {
					case 16:
						player->mAttachment = 4;
						break;
					case 21:
						player->mAttachment = 5;
						break;
					case 22:
						player->mAttachment = 6;
						break;
					case 56:
						player->mAttachment = 7;
						break;
					}
					switch (item) {
					case 16:
					case 21:
					case 22:
					case 56:
						if (!LoadPlayerAttachmentAnimation(player)) {
							mData.mScript = start;
							yield         = 1;
						}
						break;
					}
				}
			}
		} else
			CEvt_Warn(FormatToStaticBuffer("call(SYSCALL_SET_OBJSTAT, %d, 1<<%d): obj(%d) killed!", id, bit, id));
		break;
	}
	case -27: {
		s16 id    = args[0];
		s32 bit   = args[1];
		u64 mask  = (u64)1 << bit;
		XObj* obj = CEvt_GetObj(this, id);
		if (obj != NULL) {
			obj->mStatus.mFlags &= ~mask;
			if (mask & 0x20000)
				obj->vfDC(0, 0);
			if (mask & ((u64)0x4000 << 32)) {
				XObjChr* player = gGame.GetPlayer();
				if ((s16)obj->GetPool() == 8 && obj == player->mLift.mTarget) {
					s16 item = obj->GetObjFlags();
					switch (item) {
					case 16:
					case 21:
					case 22:
					case 56:
						player->mAttachment = 0;
						CJAnim* anim        = &player->mAttachmentAnim;
						anim->Release();
						break;
					}
				}
			}
		} else
			CEvt_Warn(FormatToStaticBuffer("call(SYSCALL_CLR_OBJSTAT, %d, 1<<%d): obj(%d) killed!", id, bit, id));
		break;
	}
	case -28: {
		s16 id    = args[0];
		s32 bit   = args[1];
		u64 mask  = (u64)1 << bit;
		XObj* obj = CEvt_GetObj(this, id);
		if (obj != NULL)
			_10 = (obj->mStatus.mFlags & mask) ? 1 : 0;
		else
			CEvt_Warn(FormatToStaticBuffer("call(SYSCALL_CHK_OBJSTAT, %d, 1<<%d): obj(%d) killed!", id, bit, id));
		break;
	}
	case -29:
		SetItemFlags((s16)args[0], (u16)args[1]);
		break;
	case -30:
		ClearItemFlags((s16)args[0], (u16)args[1]);
		break;
	case -31:
		_10 = (u16)GetItemFlagsMasked((s16)args[0], (u16)args[1]);
		break;
	case -41:
		gGame.mStatus.mFlags |= (u64)1 << args[0];
		break;
	case -42:
		gGame.mStatus.mFlags &= ~((u64)1 << args[0]);
		break;
	case -43:
		_10 = (gGame.mStatus.mFlags & ((u64)1 << args[0])) ? 1 : 0;
		break;
	case -32:
		gGame.mStatus.mFlags |= (u64)0x8000 << 32;
		break;
	case -33:
		gGame.mStatus.mFlags &= ~((u64)0x8000 << 32);
		break;
	case -37:
		gGame.mStatus.mFlags |= (u64)0x10000 << 32;
		break;
	case -38:
		gGame.mStatus.mFlags &= ~((u64)0x10000 << 32);
		break;
	case -34:
		gGame.mStatus.mFlags |= (u64)0x20 << 32;
		break;
	case -35:
		gGame.mStatus.mFlags &= ~((u64)0x20 << 32);
		break;
	case -36:
		gGame.mStatus.mFlags &= ~((u64)0x20 << 32);
		for (s32 i = 0; i < gGame.mXObjs.mObjectCount; i++) {
			XObj* obj = gGame.mXObjs.mObjects[i];
			if ((obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x20000000)) {
				gGame.mStatus.mFlags |= (u64)0x20 << 32;
				break;
			}
		}
		break;
	case -44: {
		s16 id                 = args[0];
		s32 areaCount          = gCameraAreaState._20;
		CamAreaRecord* records = gCameraAreaState._1C;
		for (s32 i = 0; i < areaCount; i++) {
			CamAreaRecord* record = CamAreaInfo(&records[i]).GetRecord();
			if (id == record->_2C)
				record->_2E &= ~0x80;
		}
		break;
	}
	case -45: {
		s16 id                 = args[0];
		s32 areaCount          = gCameraAreaState._20;
		CamAreaRecord* records = gCameraAreaState._1C;
		for (s32 i = 0; i < areaCount; i++) {
			CamAreaRecord* record = CamAreaInfo(&records[i]).GetRecord();
			if (id == record->_2C)
				record->_2E |= 0x80;
		}
		break;
	}
	case -39: {
		s16 result = fn_8006E2D4(args[0]);
#if defined(VERSION_PAL)
		if (result == 1)
			result = 2;
#endif
		_10 = result;
		break;
	}
	case -40:
		_10 = (s16)fn_8006E28C(args[0]);
		break;
	case -100:
		if (count > 0)
			_10 = CallStageModule(args[0] + 7, count - 1, &args[1]);
#if defined(VERSION_PAL)
		else
			OSPanic("evt.cpp", 2161, "%s%s", "SYSCALL_STGFUNC, noargs\n", "_cmd_CALL");
#else
		else
			OSPanic("evt.cpp", 2157, "%s%s", "SYSCALL_STGFUNC, noargs\n", "_cmd_CALL");
#endif
		break;
	case -200:
		AddMainMenuExtraRow(gGame.mMenu.mMenus[0], args[0], args[1]);
		break;
	default:
		if (target != NULL && (target->mStatus.mFlags & 1)) {
			s32 result = target->HandleEventCommand(count, args);
			if ((s16)target->GetPool() == 2) {
				if (result == -1 && args[0] != 101)
					yield = 1;
				else
					_10 = result;
			} else
				_10 = result;
			for (s32 i = 0; i < count; i++) { }
		}
		break;
	}
	return yield;
}

/**
 * @note Address: 0x80070304
 * @note Size: 0x1B9C
 */
s32 CEvt::_cmd_WAIT(u8 command, s32 step)
{
	u8* start = mData.mScript - 1;
	s32 yield = 0;
	u16 mode  = CEvt_Evaluate(this);
	switch (mode) {
	case EVT_ATOM_TIME: {
		s32 duration = CEvt_Evaluate(this);
		if (command == EVT_CMD_WAIT) {
			if (_18 == 0) {
				_18 = (duration / 100) * FRAMES_PER_SEC * 100;
				_18 += 100.0f * ((duration % 100 * FRAMES_PER_SEC) / 100.0f);
				mData.mScript = start;
				yield         = 1;
			} else {
				s32 remain = _18 - step * 100;
				_18        = remain;
				if (remain <= 0)
					_18 = 0;
				else {
					mData.mScript = start;
					yield         = 1;
				}
			}
		} else
			_10 = 1;
		break;
	}
	case EVT_ATOM_FADE:
		EvtWaitFor(this, command, start, EvtZero4(gGame.mFade.mSpeed) != 0, yield);
		break;
	case EVT_ATOM_MONO:
		EvtWaitFor(this, command, start, EvtZero4(gGame.mColor0.mColor.mSpeed) != 0, yield);
		break;
	case EVT_ATOM_WIPE:
		EvtWaitFor(this, command, start,
		           (EvtZero2(gGame.mWipeCircle) && EvtZero2(gGame.mWipeTex) && EvtZero2(gGame.mWipeCustom) && EvtZero2(gGame.mWipeTile0))
		               != 0,
		           yield);
		break;
	case EVT_ATOM_LETTERBOX:
		EvtWaitFor(this, command, start, EvtZero2(gGame.mWipeTile1) != 0, yield);
		break;
	case 203:
		if (command != EVT_CMD_WAIT)
			_10 = 1;
		break;
	case EVT_ATOM_ANIM: {
		s16 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, id);
		f32 frame = CEvt_Evaluate(this);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL)
				CEvt_Warn(FormatToStaticBuffer("wait(@ANIM, %d) ... obj(%d) is collision!", id, id));
			HSD_AObj* anim = GetAObj(obj->mJObj);
			if (anim != NULL) {
				if (frame >= 0.0f) {
					HSD_AObj* aobj = GetAObj(obj->mJObj);
					f32 current    = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
					EvtWaitFor(this, command, start, current >= frame, yield);
				} else
					EvtWaitFor(this, command, start, (anim->flags & 0x44000000) != 0, yield);
			} else if (command == EVT_CMD_CHECK)
				_10 = 1;
		} else {
			CEvt_Warn(FormatToStaticBuffer("wait(@ANIM, %d, ...) obj(%d) killed!", id, id));
			if (command == EVT_CMD_CHECK)
				_10 = 1;
		}
		break;
	}
	case EVT_ATOM_DIR: {
		s16 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, id);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL)
				CEvt_Warn(FormatToStaticBuffer("wait(@DIR, %d) ... obj(%d) is collision!", id, id));
			EvtWaitFor(this, command, start, obj->mMoveRotate.y == obj->mMoveRotate.mTarget.y, yield);
		} else {
			CEvt_Warn(FormatToStaticBuffer("wait(@DIR, %d, ...) obj(%d) killed!", id, id));
			if (command == EVT_CMD_CHECK)
				_10 = 1;
		}
		break;
	}
	case EVT_ATOM_MOVE: {
		s16 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, id);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL)
				CEvt_Warn(FormatToStaticBuffer("wait(@MOVE, %d) ... obj(%d) is collision!", id, id));
			const Vec& target = *(Vec*)((u8*)&obj->mMoveTranslate + sizeof(CVec));
			EvtWaitFor(this, command, start, !(obj->mMoveTranslate != target), yield);
		} else {
			CEvt_Warn(FormatToStaticBuffer("wait(@MOVE, %d, ...) obj(%d) killed!", id, id));
			if (command == EVT_CMD_CHECK)
				_10 = 1;
		}
		break;
	}
	case EVT_ATOM_SCALE: {
		s16 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, id);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL)
				CEvt_Warn(FormatToStaticBuffer("wait(@SCALE, %d) ... obj(%d) is collision!", id, id));
			EvtWaitFor(this, command, start, obj->mMoveScale.y == obj->mMoveScale.mTarget.y, yield);
		} else {
			CEvt_Warn(FormatToStaticBuffer("wait(@SCALE, %d, ...) obj(%d) killed!", id, id));
			if (command == EVT_CMD_CHECK)
				_10 = 1;
		}
		break;
	}
	case EVT_ATOM_SFX:
		EvtWaitFor(this, command, start, !IsGameSoundPlaying(&gGame.mSound, CEvt_Evaluate(this)), yield);
		break;
	case EVT_ATOM_COLOR: {
		s16 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, id);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL)
				CEvt_Warn(FormatToStaticBuffer("wait(@COLOR, %d) ... obj(%d) is collision!", id, id));
			EvtWaitFor(this, command, start, EvtZero3(&obj->mColorStep[0].mSpeed.x) && EvtZero3(&obj->mColorStep[1].mSpeed.x), yield);
		} else {
			CEvt_Warn(FormatToStaticBuffer("wait(@COLOR, %d, ...) obj(%d) killed!", id, id));
			if (command == EVT_CMD_CHECK)
				_10 = 1;
		}
		break;
	}
	case EVT_ATOM_REAL: {
		s32 index = CEvt_Evaluate(this);
		if (index == 1)
			EvtWaitFor(this, command, start, InfoScrn_IsBatteryGaugeFull(&gGame.mInfoScrn), yield);
		else {
#if defined(VERSION_PAL)
			OSPanic("evt.cpp", 4157, "%s%s", FormatToStaticBuffer("wait(@REAL, %d) - unresolved!\n", index), "_cmd_WAIT");
#else
			OSPanic("evt.cpp", 4146, "%s%s", FormatToStaticBuffer("wait(@REAL, %d) - unresolved!\n", index), "_cmd_WAIT");
#endif
		}
		break;
	}
	case EVT_ATOM_CAM: {
		s32 ready                    = 1;
		CMotionControlCamera* camera = &gGame.mMotionControlCamera;
		if (camera->IsAction(CAMACTION_RESET))
			ready = 0;
		if (camera->IsAction(CAMACTION_AREA)) {
			CCameraActionArea* area = (CCameraActionArea*)EVT_CAMERA_ACTION(CAMACTION_AREA);
			ready                   = !(area->mEyeInterp != NULL || area->mInterestInterp != NULL || area->mFovInterp != NULL);
		}
		if (camera->IsAction(CAMACTION_EVT)) {
			CCameraActionEvt* action = (CCameraActionEvt*)EVT_CAMERA_ACTION(CAMACTION_EVT);
			ready                    = !(action->mEyeInterp != NULL || action->mInterestInterp != NULL || action->mDistanceInterp != NULL
                      || action->mFovInterp != NULL || action->mAngleInterp != NULL);
		}
		if (camera->IsAction(CAMACTION_ANIM)) {
			XObj* obj = (XObj*)gGame.mXObjs.mPools[0].mObjects;
			if (obj != NULL && (obj->mStatus.mFlags & 1))
				ready = !HasCameraAnimation(obj);
		}
		EvtWaitFor(this, command, start, ready, yield);
		break;
	}
	case EVT_ATOM_READ: {
		s16 id    = CEvt_Evaluate(this);
		XObj* obj = CEvt_GetObj(this, id);
		if (obj != NULL && (obj->mStatus.mFlags & 1)) {
			if (obj->mJObj == NULL)
				CEvt_Warn(FormatToStaticBuffer("wait(@READ, %d) ... obj(%d) is collision!", id, id));
			EvtWaitFor(this, command, start, !(obj->mStatus.mFlags & 0x40000000000000ULL) && !(obj->mStatus.mFlags & 0x80000000000000ULL),
			           yield);
		} else {
			CEvt_Warn(FormatToStaticBuffer("wait(@READ, %d, ...) obj(%d) killed!", id, id));
			if (command == EVT_CMD_CHECK)
				_10 = 1;
		}
		break;
	}
	case EVT_ATOM_ZBLUR:
		EvtWaitFor(this, command, start, !(0.0f != gGame.mBlur.mSpeed), yield);
		break;
	case EVT_ATOM_SHAKE:
		EvtWaitFor(this, command, start, EvtZero3(&gGame.mQuake.mShake[2].x) != 0, yield);
		break;
	case EVT_ATOM_CUE: {
		s32 beat = gEventBeatLatch.mPending;
		if (beat != 0) {
			beat                     = gEventBeatLatch.mBeat;
			gEventBeatLatch.mPending = 0;
			beat += 1000;
		}
		if (command == EVT_CMD_WAIT && beat == 0) {
			mData.mScript = start;
			yield         = 1;
		} else
			_10 = beat;
		break;
	}
	case EVT_ATOM_MOVIE: {
		s32 frame = CEvt_Evaluate(this);
		s32 ready = gGame.mMovie.mMovie.mState == 0;
		if (!ready && frame >= 0)
			ready = gGame.mMovie.mMovie.mFrame >= frame;
		if (command == EVT_CMD_WAIT && ready == 0) {
			mData.mScript = start;
			yield         = 1;
		} else
			_10 = ready;
		break;
	}
	default:
		if (command == EVT_CMD_WAIT) {
#if defined(VERSION_PAL)
			OSPanic("evt.cpp", 4300, "%s%s", FormatToStaticBuffer("wait(%d,...) - unresolved!\n", mode), "_cmd_WAIT");
#else
			OSPanic("evt.cpp", 4289, "%s%s", FormatToStaticBuffer("wait(%d,...) - unresolved!\n", mode), "_cmd_WAIT");
#endif
		} else {
#if defined(VERSION_PAL)
			OSPanic("evt.cpp", 4302, "%s%s", FormatToStaticBuffer("check(%d,...) - unresolved!\n", mode), "_cmd_WAIT");
#else
			OSPanic("evt.cpp", 4291, "%s%s", FormatToStaticBuffer("check(%d,...) - unresolved!\n", mode), "_cmd_WAIT");
#endif
		}
		break;
	}
	return yield;
}

/**
 * @note Address: 0x8007E97C
 * @note Size: 0x528
 */
extern "C" void CEvt_Update(CEvt* evt, s32 step)
{
	if (evt->mData.mScript != NULL) {
		s32 result = 0;
		while (result == 0) {
			u8 command = *evt->mData.mScript;
			evt->mData.mScript++;
			for (s32 i = 1; i < 50; i++) {
				if (command == evtCmdTable[i].command) {
					result = (evt->*evtCmdTable[i].handler)(command, step);
					goto next;
				}
			}
			OSPanic("evt.cpp", 558, "%s%s", FormatToStaticBuffer("Unknown event cmd - %d\n", command), "Update");
		next:;
		}
	}
	if (evt->_00 == 0 && !(gGame.mStatus.mFlags & 0xC00)) {
		if (evt->mData.mScript == NULL) {
			for (u32 i = 0; i < 10; i++) {
				if (gEventTimers.mFrames[i] > 0) {
					if ((gEventTimers.mFrames[i] -= step) <= 0) {
						u8* script = (u8*)gEventTimers.mValue[i];
						s32 cursor = gEventScriptQueue.mRead;
						for (s32 j = 0; j < 128; j++, cursor++) {
							s32 index = cursor % 128;
							if (gEventScriptQueue.mScript[index] == NULL) {
								gEventScriptQueue.mScript[index] = script;
								break;
							}
						}
						gEventTimers.mFrames[i] = 0;
						gEventTimers.mValue[i]  = 0;
					}
				}
			}
		}
		if (evt->mData.mScript == NULL) {
			CEvtQueueEx* queue    = &gEventScriptQueue;
			s32 index             = queue->mRead;
			u8* script            = queue->mScript[index];
			u32 owner             = queue->mContext[index];
			s16 useValue          = queue->mUseValue[index];
			s16 hitValue          = queue->mHitValue[index];
			queue->mScript[index] = NULL;
			queue->mRead          = (queue->mRead + 1) % 128;
			if (script != NULL) {
				CEvt_Attach(evt, script, owner, useValue, hitValue);
			}
		}
	}
}
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stw      r31, 0x1c(r1)
    mr       r31, r3
    stw      r30, 0x18(r1)
    mr       r30, r4
    stw      r29, 0x14(r1)
    stw      r28, 0x10(r1)
    lwz      r0, 0x4(r3)
    cmplwi   r0, 0x0
    beq      .L_8007EA5C
    lis      r3, evtCmdTable@ha
    li       r28, 0x0
    addi     r29, r3, evtCmdTable@l
    b        .L_8007EA54
.L_8007E9BC:
    lwz      r3, 0x4(r31)
    li       r0, 0x31
    addi     r5, r29, 0x14
    li       r6, 0x1
    lbz      r4, 0x0(r3)
    addi     r3, r3, 0x1
    stw      r3, 0x4(r31)
    clrlwi   r3, r4, 24
    mtctr    r0
.L_8007E9E0:
    lbz      r0, 0x0(r5)
    cmplw    r3, r0
    bne      .L_8007EA18
    mulli    r6, r6, 0x14
    lis      r5, evtCmdTable@ha
    mr       r3, r31
    addi     r0, r5, evtCmdTable@l
    add      r6, r0, r6
    mr       r5, r30
    addi     r12, r6, 0x8
    bl       __ptmf_scall
    nop
    mr       r28, r3
    b        .L_8007EA54
.L_8007EA18:
    addi     r5, r5, 0x14
    addi     r6, r6, 0x1
    bdnz     .L_8007E9E0
    lis      r3, sEvtUnknownCommand@ha
    clrlwi   r4, r4, 24
    addi     r3, r3, sEvtUnknownCommand@l
    crclr    cr1eq
    bl       FormatToStaticBuffer
    mr       r6, r3
    li       r3, lbl_8065CFA0@sda21
    li       r4, 0x22e
    li       r5, lbl_8065CFA8@sda21
    li       r7, sEvtUpdateName@sda21
    crclr    cr1eq
    bl       OSPanic
.L_8007EA54:
    cmpwi    r28, 0x0
    beq      .L_8007E9BC
.L_8007EA5C:
    lwz      r0, 0x0(r31)
    cmplwi   r0, 0x0
    bne      .L_8007EE84
    lis      r4, gGame@ha
    li       r3, 0xc00
    addi     r4, r4, gGame@l
    li       r5, 0x0
    lwz      r0, 0x1b10(r4)
    lwz      r4, 0x1b14(r4)
    and      r0, r0, r5
    and      r3, r4, r3
    xor      r3, r3, r5
    xor      r0, r0, r5
    or.      r0, r3, r0
    bne      .L_8007EE84
    lwz      r0, 0x4(r31)
    cmplwi   r0, 0x0
    bne      .L_8007EBD4
    lis      r3, gEventTimers@ha
    addi     r9, r3, gEventTimers@l
.L_8007EAAC:
    lwz      r0, 0x0(r9)
    cmpwi    r0, 0x0
    ble      .L_8007EBC4
    subf.    r0, r30, r0
    stw      r0, 0x0(r9)
    bgt      .L_8007EBC4
    lis      r3, gEventScriptQueue@ha
    li       r0, 0x20
    addi     r4, r3, gEventScriptQueue@l
    lwz      r7, 0x28(r9)
    lwz      r8, 0x0(r4)
    li       r6, 0x0
    mtctr    r0
.L_8007EAE0:
    slwi     r0, r8, 25
    srwi     r3, r8, 31
    subf     r0, r3, r0
    rotlwi   r0, r0, 7
    add      r0, r0, r3
    slwi     r0, r0, 2
    add      r3, r4, r0
    lwzu     r0, 0x4(r3)
    cmplwi   r0, 0x0
    bne      .L_8007EB10
    stw      r7, 0x0(r3)
    b        .L_8007EBB8
.L_8007EB10:
    addi     r8, r8, 0x1
    slwi     r0, r8, 25
    srwi     r3, r8, 31
    subf     r0, r3, r0
    rotlwi   r0, r0, 7
    add      r0, r0, r3
    slwi     r0, r0, 2
    add      r3, r4, r0
    lwzu     r0, 0x4(r3)
    cmplwi   r0, 0x0
    bne      .L_8007EB44
    stw      r7, 0x0(r3)
    b        .L_8007EBB8
.L_8007EB44:
    addi     r8, r8, 0x1
    slwi     r0, r8, 25
    srwi     r3, r8, 31
    subf     r0, r3, r0
    rotlwi   r0, r0, 7
    add      r0, r0, r3
    slwi     r0, r0, 2
    add      r3, r4, r0
    lwzu     r0, 0x4(r3)
    cmplwi   r0, 0x0
    bne      .L_8007EB78
    stw      r7, 0x0(r3)
    b        .L_8007EBB8
.L_8007EB78:
    addi     r8, r8, 0x1
    slwi     r0, r8, 25
    srwi     r3, r8, 31
    subf     r0, r3, r0
    rotlwi   r0, r0, 7
    add      r0, r0, r3
    slwi     r0, r0, 2
    add      r3, r4, r0
    lwzu     r0, 0x4(r3)
    cmplwi   r0, 0x0
    bne      .L_8007EBAC
    stw      r7, 0x0(r3)
    b        .L_8007EBB8
.L_8007EBAC:
    addi     r8, r8, 0x1
    addi     r6, r6, 0x3
    bdnz     .L_8007EAE0
.L_8007EBB8:
    li       r0, 0x0
    stw      r0, 0x0(r9)
    stw      r0, 0x28(r9)
.L_8007EBC4:
    addi     r5, r5, 0x1
    addi     r9, r9, 0x4
    cmpwi    r5, 0xa
    blt      .L_8007EAAC
.L_8007EBD4:
    lwz      r0, 0x4(r31)
    cmplwi   r0, 0x0
    bne      .L_8007EE84
    lis      r4, gEventScriptQueue@ha
    li       r3, 0x0
    addi     r30, r4, gEventScriptQueue@l
    lwz      r0, 0x0(r30)
    addi     r4, r30, 0x4
    addi     r5, r30, 0x208
    addi     r6, r30, 0x408
    slwi     r11, r0, 2
    slwi     r10, r0, 1
    addi     r7, r30, 0x508
    lwzx     r0, r4, r11
    lwzx     r8, r5, r11
    lhax     r9, r6, r10
    cmplwi   r0, 0x0
    lhax     r10, r7, r10
    stwx     r3, r4, r11
    lwz      r11, 0x0(r30)
    addi     r12, r11, 0x1
    slwi     r11, r12, 25
    srwi     r12, r12, 31
    subf     r11, r12, r11
    rotlwi   r11, r11, 7
    add      r29, r11, r12
    stw      r29, 0x0(r30)
    beq      .L_8007EE84
    beq      .L_8007EE84
    lwz      r11, 0x4(r31)
    cmplwi   r11, 0x0
    beq      .L_8007ED84
    lwz      r11, 0x0(r31)
    cmplwi   r11, 0x0
    bne      .L_8007ED84
    li       r11, 0x20
    mtctr    r11
.L_8007EC68:
    slwi     r11, r29, 25
    srwi     r12, r29, 31
    subf     r11, r12, r11
    rotlwi   r11, r11, 7
    add      r12, r11, r12
    slwi     r31, r12, 2
    add      r11, r30, r31
    lwz      r11, 0x4(r11)
    cmplwi   r11, 0x0
    bne      .L_8007ECA8
    stwx     r0, r4, r31
    slwi     r0, r12, 1
    stwx     r8, r5, r31
    sthx     r9, r6, r0
    sthx     r10, r7, r0
    b        .L_8007EE84
.L_8007ECA8:
    addi     r29, r29, 0x1
    slwi     r11, r29, 25
    srwi     r12, r29, 31
    subf     r11, r12, r11
    rotlwi   r11, r11, 7
    add      r12, r11, r12
    slwi     r31, r12, 2
    add      r11, r30, r31
    lwz      r11, 0x4(r11)
    cmplwi   r11, 0x0
    bne      .L_8007ECEC
    stwx     r0, r4, r31
    slwi     r0, r12, 1
    stwx     r8, r5, r31
    sthx     r9, r6, r0
    sthx     r10, r7, r0
    b        .L_8007EE84
.L_8007ECEC:
    addi     r29, r29, 0x1
    slwi     r11, r29, 25
    srwi     r12, r29, 31
    subf     r11, r12, r11
    rotlwi   r11, r11, 7
    add      r12, r11, r12
    slwi     r31, r12, 2
    add      r11, r30, r31
    lwz      r11, 0x4(r11)
    cmplwi   r11, 0x0
    bne      .L_8007ED30
    stwx     r0, r4, r31
    slwi     r0, r12, 1
    stwx     r8, r5, r31
    sthx     r9, r6, r0
    sthx     r10, r7, r0
    b        .L_8007EE84
.L_8007ED30:
    addi     r29, r29, 0x1
    slwi     r11, r29, 25
    srwi     r12, r29, 31
    subf     r11, r12, r11
    rotlwi   r11, r11, 7
    add      r12, r11, r12
    slwi     r31, r12, 2
    add      r11, r30, r31
    lwz      r11, 0x4(r11)
    cmplwi   r11, 0x0
    bne      .L_8007ED74
    stwx     r0, r4, r31
    slwi     r0, r12, 1
    stwx     r8, r5, r31
    sthx     r9, r6, r0
    sthx     r10, r7, r0
    b        .L_8007EE84
.L_8007ED74:
    addi     r29, r29, 0x1
    addi     r3, r3, 0x3
    bdnz     .L_8007EC68
    b        .L_8007EE84
.L_8007ED84:
    li       r5, 0x0
    stw      r5, 0x4(r31)
    stw      r5, 0x18(r31)
    stw      r5, 0x1c(r31)
    stw      r5, 0x20(r31)
    stw      r5, 0x28(r31)
    stw      r5, 0x38(r31)
    stw      r5, 0x48(r31)
    lwz      r3, 0x0(r31)
    cmplwi   r3, 0x0
    bne      .L_8007EE44
    li       r3, 0x4
    lis      r4, gEventScriptQueue@ha
    stwu     r5, gEventScriptQueue@l(r4)
    mtctr    r3
.L_8007EDC0:
    stw      r5, 0x4(r4)
    stw      r5, 0x8(r4)
    stw      r5, 0xc(r4)
    stw      r5, 0x10(r4)
    stw      r5, 0x14(r4)
    stw      r5, 0x18(r4)
    stw      r5, 0x1c(r4)
    stw      r5, 0x20(r4)
    stw      r5, 0x24(r4)
    stw      r5, 0x28(r4)
    stw      r5, 0x2c(r4)
    stw      r5, 0x30(r4)
    stw      r5, 0x34(r4)
    stw      r5, 0x38(r4)
    stw      r5, 0x3c(r4)
    stw      r5, 0x40(r4)
    stw      r5, 0x44(r4)
    stw      r5, 0x48(r4)
    stw      r5, 0x4c(r4)
    stw      r5, 0x50(r4)
    stw      r5, 0x54(r4)
    stw      r5, 0x58(r4)
    stw      r5, 0x5c(r4)
    stw      r5, 0x60(r4)
    stw      r5, 0x64(r4)
    stw      r5, 0x68(r4)
    stw      r5, 0x6c(r4)
    stw      r5, 0x70(r4)
    stw      r5, 0x74(r4)
    stw      r5, 0x78(r4)
    stw      r5, 0x7c(r4)
    stwu     r5, 0x80(r4)
    bdnz     .L_8007EDC0
.L_8007EE44:
    li       r4, 0x0
    li       r3, -0x1
    stw      r4, 0x10(r31)
    stw      r4, 0x14(r31)
    stw      r4, 0x58(r31)
    sth      r3, 0x5c(r31)
    sth      r3, 0x5e(r31)
    stw      r0, 0x4(r31)
    lwz      r0, 0x0(r31)
    cmplwi   r0, 0x0
    beq      .L_8007EE74
    b        .L_8007EE78
.L_8007EE74:
    mr       r0, r8
.L_8007EE78:
    stw      r0, 0x8(r31)
    sth      r9, 0x5c(r31)
    sth      r10, 0x5e(r31)
.L_8007EE84:
    lwz      r0, 0x24(r1)
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    lwz      r28, 0x10(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

extern "C" {
void InfoScrn_ApplyBatteryHudCommand(InfoScrn* scrn, u8 show, s32 mode);
void InfoScrn_ApplyTimerHudCommand(InfoScrn* scrn, u8 show, s32 mode);
s32 InfoScrn_GetTimerHundredths(InfoScrn* scrn);
void InfoScrn_SetTimerHundredths(InfoScrn* scrn, s32 hundredths);
void InfoScrn_ShowTimer(InfoScrn* scrn);
void InfoScrn_HideTimer(InfoScrn* scrn);
}

struct EvtHudCommand {
	u8 enabled; // _00
	s32 mode;   // _04
};

/**
 * @note Address: 0x80072224
 * @note Size: 0xEF8
 */
s32 CEvt::_cmd_SCRN(u8, s32)
{
	u16 mode = CEvt_Evaluate(this);
	switch (mode) {
	case EVT_ATOM_FADE: {
		u8 r0    = CEvt_Evaluate(this);
		u8 g0    = CEvt_Evaluate(this);
		u8 b0    = CEvt_Evaluate(this);
		u8 a0    = CEvt_Evaluate(this);
		u8 r1    = CEvt_Evaluate(this);
		u8 g1    = CEvt_Evaluate(this);
		u8 b1    = CEvt_Evaluate(this);
		u8 a1    = CEvt_Evaluate(this);
		f32 time = CEvt_Evaluate(this) / 100.0f;
		StartScreenFade(&gGame.mFade, r0, g0, b0, a0, r1, g1, b1, a1, time);
		break;
	}
	case EVT_ATOM_MONO: {
		u8 r0    = CEvt_Evaluate(this);
		u8 g0    = CEvt_Evaluate(this);
		u8 b0    = CEvt_Evaluate(this);
		u8 a0    = CEvt_Evaluate(this);
		u8 r1    = CEvt_Evaluate(this);
		u8 g1    = CEvt_Evaluate(this);
		u8 b1    = CEvt_Evaluate(this);
		u8 a1    = CEvt_Evaluate(this);
		f32 time = CEvt_Evaluate(this) / 100.0f;
		StartScreenFade(&gGame.mColor0.mColor, r0, g0, b0, a0, r1, g1, b1, a1, time);
		break;
	}
	case EVT_ATOM_WIPE: {
		s32 type = CEvt_Evaluate(this);
		s32 open = CEvt_Evaluate(this);
		s32 x    = CEvt_Evaluate(this);
		s32 y    = CEvt_Evaluate(this);
		s32 w0   = CEvt_Evaluate(this);
		s32 h0   = CEvt_Evaluate(this);
		u8 r0    = CEvt_Evaluate(this);
		u8 g0    = CEvt_Evaluate(this);
		u8 b0    = CEvt_Evaluate(this);
		u8 a0    = CEvt_Evaluate(this);
		s32 w1   = CEvt_Evaluate(this);
		s32 h1   = CEvt_Evaluate(this);
		u8 r1    = CEvt_Evaluate(this);
		u8 g1    = CEvt_Evaluate(this);
		u8 b1    = CEvt_Evaluate(this);
		u8 a1    = CEvt_Evaluate(this);
		f32 time = CEvt_Evaluate(this) / 100.0f;
		switch (type) {
		case 0:
			StartScreenWipeColorTransition(&gGame.mWipeCircle, r0, g0, b0, a0, r1, g1, b1, x, y, w0, h0, w1, h1, time, a1, !open);
			break;
		case 1:
			StartScreenWipeColorTransition(&gGame.mWipeTile0, r0, g0, b0, a0, r1, g1, b1, x, y, w0, h0, w1, h1, time, a1, !open);
			break;
		default:
			if (type >= 2000) {
				if (open) {
					StartScreenWipeColorTransition(&gGame.mWipeCustom, r0, g0, b0, a0, r1, g1, b1, x, y, 57.0f, 57.0f, 0.0f, 0.0f, time, a1,
					                               !open);
				} else {
					StartScreenWipeColorTransition(&gGame.mWipeCustom, r0, g0, b0, a0, r1, g1, b1, x, y, 0.0f, 0.0f, 57.0f, 57.0f, time, a1,
					                               !open);
				}
			} else if (type >= 1000) {
				TPLPalette* palette = (TPLPalette*)(&gGame.mTpl)->mBuffer;
				CTexObj* texture    = &(&gGame.mTpl)->mTex;
				gGame.mWipeTex.mTex = texture->Init(&palette->descriptorArray[type - 1000], 0);
				StartScreenWipeColorTransition(&gGame.mWipeTex, r0, g0, b0, a0, r1, g1, b1, x, y, w0, h0, w1, h1, time, a1, !open);
			} else {
#if defined(VERSION_PAL)
				const s32 line = 3542;
#else
				const s32 line = 3531;
#endif
				OSPanic("evt.cpp", line, "%s%s", FormatToStaticBuffer("scrn(@WIPE, %d,...) - unresolved!\n", type), "_cmd_SCRN");
			}
			break;
		}
		break;
	}
	case EVT_ATOM_LETTERBOX: {
		s32 open    = CEvt_Evaluate(this);
		s32 height0 = open ? 448 : 314;
		u8 r0       = CEvt_Evaluate(this);
		u8 g0       = CEvt_Evaluate(this);
		u8 b0       = CEvt_Evaluate(this);
		u8 a0       = CEvt_Evaluate(this);
		s32 height1 = open ? 314 : 448;
		u8 r1       = CEvt_Evaluate(this);
		u8 g1       = CEvt_Evaluate(this);
		u8 b1       = CEvt_Evaluate(this);
		u8 a1       = CEvt_Evaluate(this);
		f32 time    = CEvt_Evaluate(this) / 100.0f;
		StartScreenWipeColorTransition(&gGame.mWipeTile1, r0, g0, b0, a0, r1, g1, b1, 320.0f, 200.0f, 640.0f, height0, 640.0f, height1,
		                               time, a1, !open);
		break;
	}
	case EVT_ATOM_HUD:
		switch (CEvt_Evaluate(this)) {
		case EVT_HUD_BATTERY: {
			s32 index                 = CEvt_Evaluate(this);
			EvtHudCommand commands[8] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 }, { 0, 2 }, { 1, 2 }, { 0, 3 }, { 1, 3 } };
			InfoScrn_ApplyBatteryHudCommand(&gGame.mInfoScrn, commands[index].enabled, commands[index].mode);
			break;
		}
		case EVT_HUD_CLOCK: {
			s32 index                 = CEvt_Evaluate(this);
			EvtHudCommand commands[8] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 }, { 1, 2 }, { 1, 3 }, { 0, 4 }, { 1, 4 } };
			InfoScrn_ApplyClockHudCommand(&gGame.mInfoScrn, commands[index].enabled, commands[index].mode, 0);
			break;
		}
		case EVT_HUD_SCORE: {
			s32 index                 = CEvt_Evaluate(this);
			EvtHudCommand commands[4] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
			InfoScrn_ApplyScoreHudCommand(&gGame.mInfoScrn, commands[index].enabled, commands[index].mode);
			break;
		}
		case 3:
			CEvt_Evaluate(this);
			CEvt_Evaluate(this);
			CEvt_Evaluate(this);
			CEvt_Evaluate(this);
			break;
		case EVT_HUD_TIMER: {
			s32 index = CEvt_Evaluate(this);
			if (index >= 0) {
				EvtHudCommand commands[4] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
				InfoScrn_ApplyTimerHudCommand(&gGame.mInfoScrn, commands[index].enabled, commands[index].mode);
			} else {
				switch (index) {
				case -1:
					InfoScrn_ShowTimer(&gGame.mInfoScrn);
					break;
				case -2:
					InfoScrn_HideTimer(&gGame.mInfoScrn);
					break;
				case -3:
					InfoScrn_SetTimerHundredths(&gGame.mInfoScrn, CEvt_Evaluate(this));
					break;
				case -4:
					_10 = InfoScrn_GetTimerHundredths(&gGame.mInfoScrn);
					break;
				}
			}
			break;
		}
		}
		break;
	case EVT_ATOM_ZBLUR: {
		u8 a0     = CEvt_Evaluate(this);
		u8 a1     = CEvt_Evaluate(this);
		s32 near  = CEvt_Evaluate(this);
		s32 far   = CEvt_Evaluate(this);
		f32 time  = CEvt_Evaluate(this) / 100.0f;
		f32 start = a0 / 255.0f;
		f32 end   = a1 / 255.0f;
		f32 step;
		if (time == 0.0f) {
			start = end;
			step  = 0.0f;
		} else {
			step = (end - start) / (FRAMES_PER_SEC * time);
		}
		if (start >= 0.0f)
			gGame.mBlur.mAlpha = start;
		if (end >= 0.0f)
			gGame.mBlur.mTarget = end;
		step                 = step < 0.0f ? -step : step;
		gGame.mBlur.mSpeed   = step;
		gGame.mBlur.mOffsetX = near;
		gGame.mBlur.mOffsetY = far;
		break;
	}
	case EVT_ATOM_SHAKE: {
		f32 x0   = CEvt_Evaluate(this) / 100.0f;
		f32 y0   = CEvt_Evaluate(this) / 100.0f;
		f32 z0   = CEvt_Evaluate(this) / 100.0f;
		f32 x1   = CEvt_Evaluate(this) / 100.0f;
		f32 y1   = CEvt_Evaluate(this) / 100.0f;
		f32 z1   = CEvt_Evaluate(this) / 100.0f;
		f32 time = CEvt_Evaluate(this) / 100.0f;
		StartScreenShake(&gGame.mQuake, x0, y0, z0, x1, y1, z1, time);
		break;
	}
	default:
#if defined(VERSION_PAL)
		const s32 line = 3737;
#else
		const s32 line = 3726;
#endif
		OSPanic("evt.cpp", line, "%s%s", FormatToStaticBuffer("scrn(%d,...) - unresolved!\n", mode), "_cmd_SCRN");
		break;
	}
	return 0;
}

static inline void ClearScript(CEvt* evt)
{
	evt->mData.mScript  = NULL;
	evt->_18            = 0;
	evt->_1C            = 0;
	evt->_20            = 0;
	evt->mCalls.mCount  = 0;
	evt->mFrames.mCount = 0;
	evt->mValues.mCount = 0;
}

static inline void ClearQueue()
{
	gEventScriptQueue.mRead = 0;
	for (u32 i = 0; i < 128; ++i) {
		gEventScriptQueue.mScript[i] = NULL;
	}
}

static inline void ClearResults(CEvt* evt)
{
	evt->_10       = 0;
	evt->_14       = 0;
	evt->_58       = 0;
	evt->mUseValue = -1;
	evt->mHitValue = -1;
}

/**
 * @note Address: 0x8007EEA4
 * @note Size: 0x194
 */
extern "C" void StartOrQueueEventScript(CEvt* evt, u8* script, u32 context, s16 useValue, s16 hitValue)
{
	if (script == NULL)
		return;
	if (evt->mData.mScript != NULL && evt->_00 == 0) {
		s32 cursor = gEventScriptQueue.mRead;
		for (s32 i = 0; i < 128; ++i) {
			s32 index = cursor % 128;
			if (gEventScriptQueue.mScript[index] == NULL) {
				gEventScriptQueue.mScript[index]   = script;
				gEventScriptQueue.mContext[index]  = context;
				gEventScriptQueue.mUseValue[index] = useValue;
				gEventScriptQueue.mHitValue[index] = hitValue;
				return;
			}
			++cursor;
		}
		return;
	}
	ClearScript(evt);
	if (evt->_00 == 0)
		ClearQueue();
	ClearResults(evt);
	evt->mData.mScript = script;
	evt->mData._04     = evt->_00 != 0 ? evt->_00 : context;
	evt->mUseValue     = useValue;
	evt->mHitValue     = hitValue;
}

/**
 * @note Address: 0x8007F038
 * @note Size: 0x8
 */
extern "C" BOOL fn_8007F038(CEvt* evt)
{
	return TRUE;
}

/**
 * @note Address: 0x8007F040
 * @note Size: 0x154
 */
extern "C" void ResetEventScriptState(CEvt* evt, BOOL clearTimers, BOOL clearGlobal)
{
	ClearScript(evt);
	if (evt->_00 == 0) {
		if (clearTimers) {
			for (u32 i = 0; i < 10; ++i) {
				gEventTimers.mFrames[i] = 0;
				gEventTimers.mValue[i]  = 0;
			}
		}
		ClearQueue();
		if (clearGlobal)
			gEventObjectIds.mCount = 0;
	}
	ClearResults(evt);
}

// allocates a stack's buffer
extern "C" void AllocateWordBuffer(CStack* stack, s32 capacity);

static inline void ClearTimers()
{
	for (u32 i = 0; i < 10; ++i) {
		gEventTimers.mFrames[i] = 0;
		gEventTimers.mValue[i]  = 0;
	}
}

/**
 * @note Address: 0x8007F194
 * @note Size: 0x3DC
 */
extern "C" void InitializeEventScriptContext(CEvt* evt, u8* context, void* owner)
{
	evt->_00 = (u32)owner;
	evt->_54 = context;
	if (evt->_00 == 0) {
		for (u32 i = 0; i < 128; ++i)
			gEventFlags.mFlags[i] = 0;
		for (u32 i = 0; i < 2048; ++i)
			gEventVars.mVars[i] = 0;
		ClearQueue();
		ClearTimers();
		InitializeObject__4XObjFUl(&gEventScriptObject, 0);
	}
	AllocateWordBuffer(&evt->mCalls, 32);
	AllocateWordBuffer(&evt->mFrames, 64);
	AllocateWordBuffer(&evt->mValues, 128);
	evt->mCameraMemento = NULL;
	evt->mData.mScript  = NULL;
	evt->_18            = 0;
	evt->_1C            = 0;
	evt->_20            = 0;
	evt->mCalls.mCount  = 0;
	evt->mFrames.mCount = 0;
	evt->mValues.mCount = 0;
	if (evt->_00 == 0) {
		ClearTimers();
		ClearQueue();
		gEventObjectIds.mCount = 0;
	}
	evt->_10       = 0;
	evt->_14       = 0;
	evt->_58       = 0;
	evt->mUseValue = -1;
	evt->mHitValue = -1;
}

// what ResolveEventObjectId gets as its context
struct EvtContext {
	u32 _00;       // _00
	XObj* mObject; // _04, the one running the script
};

static inline XObj* NamedObject(s16 id)
{
	for (s32 i = 0; i < gEventObjectIds.mCount; ++i) {
		if (gEventObjectIds.mEntry[i].mId == id) {
			XObj* object = gGame.GetObject(gEventObjectIds.mEntry[i].mIndex);
			if (object != NULL && (object->mStatus.mFlags & 1ULL))
				return object;
			return NULL;
		}
	}
	return NULL;
}

/**
 * @note Address: 0x8007F570
 * @note Size: 0xF4
 */
extern "C" XObj* ResolveEventObjectId(void* context, s16 id)
{
	if (id >= 10000) {
		XObj* object = NamedObject(id);
		if (object != NULL)
			return object;
		return NULL;
	}
	if (id >= 0)
		return gGame.GetObject(id);
	if (id == -1)
		return ((EvtContext*)context)->mObject;
	return (XObj*)(s32)id;
}
