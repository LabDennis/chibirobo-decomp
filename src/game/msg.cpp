#include "game/xobjs.h"
#include "game/stagedata.h"
#include "game/ccameraaction.h"
#include "game/cfont.h"
#include "game/ctpl.h"
#include "game/cprimitive.h"
#include "game/cgs.h"
#include "game/evt.h"
#include "game/csound.h"
#include "game/msg.h"
#include "game/xchr.h"
#include "game/video.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <types.h>
#include "game/cgame.h"

extern "C" {
#include <dolphin/os.h>
#include <sysdolphin/aobj.h>
}

// the colours a message can switch its text to
u32 msgColorTable[] = {
	0xFFFFFFFF, 0x808080FF, 0x101010FF, 0x00FFFFFF, 0x00E000FF, 0x0000E0FF, 0xFF00FFFF, 0xE00000FF, 0xFFFF00FF, 0xFF8000FF,
};

extern "C" {
void CMsg_ResetPos(CMsg* msg);
void CMsgFont_Draw(void* p, f32 t);
void ClearGameSoundQueue(void* sound, s32 id);
}

#if defined(VERSION_JP)
#define MsgHasSjisLead(c) IsSjisLead(c)
#else
#define MsgHasSjisLead(c) (gGame.mLanguage == 6 && IsSjisLead(c))
#endif

// read before stepping, the script can point into the message's own bytes
static inline u32 ReadMsgU8(CMsg* msg)
{
	u32 value = *msg->mData.mCursor;
	msg->mData.mCursor++;
	return value;
}

// script operands are little endian, literal colors are rgba bytes
static inline s16 ReadMsgS16(CMsg* msg)
{
	u32 lo = ReadMsgU8(msg);
	u32 hi = ReadMsgU8(msg);
	return lo | (hi << 8);
}

/**
 * @note Address: 0x8007FFB4
 * @note Size: 0x2D0
 */
extern "C" s32 CMsg__cmd_SETFONT(CMsg* msg, u8 command)
{
	switch (command) {
	case 13: {
		u8 size = ReadMsgU8(msg);
		if (size == 255) {
			size = 22;
		}
		msg->mFont.mSize = size;
		break;
	}
	case 14: {
		u8 color = ReadMsgU8(msg);
		if (color == 255) {
			color = 0;
		}
		msg->mFont.mGlyphColor = msgColorTable[color];
		break;
	}
	case 15: {
		u32 r                  = ReadMsgU8(msg);
		u32 g                  = ReadMsgU8(msg);
		u32 b                  = ReadMsgU8(msg);
		u32 a                  = ReadMsgU8(msg);
		msg->mFont.mGlyphColor = (r << 24) | (g << 16) | (b << 8) | a;
		break;
	}
	case 16:
		msg->mFont._04_04 = ReadMsgU8(msg);
		break;
	case 19:
		msg->mFont.mFitLine = ReadMsgU8(msg);
		break;
	case 18: {
		u32 a              = ReadMsgU8(msg);
		u32 b              = ReadMsgU8(msg);
		u32 c              = ReadMsgU8(msg);
		msg->mFont.mEffect = a;
		msg->mFont.mAmount = b;
		msg->mFont.mSpeed  = c;
		break;
	}
	case 20:
		msg->mFont.mGlyphAngle = DEG2RAD * ReadMsgS16(msg);
		break;
	case 21: {
		f32 x                   = ReadMsgS16(msg) / 100.0f;
		f32 y                   = ReadMsgS16(msg) / 100.0f;
		msg->mFont.mGlyphScaleX = x;
		msg->mFont.mGlyphScaleY = y;
		break;
	}
	default:
#if defined(VERSION_JP)
		const s32 line = 2044;
#elif defined(VERSION_PAL)
		const s32 line = 2114;
#else
		const s32 line = 2097;
#endif
		OSPanic("msg.cpp", line, "%s%s", FormatToStaticBuffer("Unknown msg-setfont cmd - %d\n", command), "_cmd_SETFONT");
		break;
	}
	return 0;
}
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    clrlwi   r4, r4, 24
    stw      r0, 0x24(r1)
    subi     r0, r4, 0xd
    cmplwi   r0, 0x8
    bgt      .L_80080240
    lis      r5, jumptable_80225488@ha
    slwi     r0, r0, 2
    addi     r5, r5, jumptable_80225488@l
    lwzx     r0, r5, r0
    mtctr    r0
    bctr
.L_8007FFE8:
    lwz      r5, 0x804(r3)
    lbz      r4, 0x0(r5)
    addi     r0, r5, 0x1
    cmplwi   r4, 0xff
    stw      r0, 0x804(r3)
    mr       r0, r4
    bne      .L_80080008
    li       r0, 0x16
.L_80080008:
    clrlwi   r4, r0, 24
    lis      r0, 0x4330
    stw      r4, 0xc(r1)
    addis    r3, r3, 0x4
    lfd      f1, lbl_8065D048@sda21(r0)
    stw      r0, 0x8(r1)
    lfd      f0, 0x8(r1)
    fsubs    f0, f0, f1
    stfs     f0, 0x4570(r3)
    b        .L_80080270
.L_80080030:
    lwz      r5, 0x804(r3)
    lbz      r4, 0x0(r5)
    addi     r0, r5, 0x1
    cmplwi   r4, 0xff
    stw      r0, 0x804(r3)
    mr       r0, r4
    bne      .L_80080050
    li       r0, 0x0
.L_80080050:
    lis      r4, msgColorTable@ha
    clrlslwi r0, r0, 24, 2
    addi     r4, r4, msgColorTable@l
    addis    r3, r3, 0x4
    lwzx     r0, r4, r0
    stw      r0, 0x4578(r3)
    b        .L_80080270
.L_8008006C:
    lwz      r5, 0x804(r3)
    addis    r4, r3, 0x4
    lbz      r7, 0x0(r5)
    addi     r0, r5, 0x1
    stw      r0, 0x804(r3)
    lwz      r6, 0x804(r3)
    lbz      r5, 0x0(r6)
    addi     r0, r6, 0x1
    stw      r0, 0x804(r3)
    slwi     r0, r5, 16
    rlwimi   r0, r7, 24, 0, 7
    lwz      r5, 0x804(r3)
    lbz      r6, 0x0(r5)
    addi     r5, r5, 0x1
    stw      r5, 0x804(r3)
    rlwimi   r0, r6, 8, 16, 23
    lwz      r5, 0x804(r3)
    lbz      r6, 0x0(r5)
    addi     r5, r5, 0x1
    stw      r5, 0x804(r3)
    or       r0, r6, r0
    stw      r0, 0x4578(r4)
    b        .L_80080270
.L_800800C8:
    lwz      r5, 0x804(r3)
    lbz      r4, 0x0(r5)
    addi     r0, r5, 0x1
    stw      r0, 0x804(r3)
    lbz      r0, 0x8a4(r3)
    rlwimi   r0, r4, 2, 29, 29
    stb      r0, 0x8a4(r3)
    b        .L_80080270
.L_800800E8:
    lwz      r5, 0x804(r3)
    lbz      r4, 0x0(r5)
    addi     r0, r5, 0x1
    stw      r0, 0x804(r3)
    lbz      r0, 0x8a4(r3)
    rlwimi   r0, r4, 3, 28, 28
    stb      r0, 0x8a4(r3)
    b        .L_80080270
.L_80080108:
    lwz      r5, 0x804(r3)
    addis    r4, r3, 0x4
    lbz      r8, 0x0(r5)
    addi     r0, r5, 0x1
    stw      r0, 0x804(r3)
    lwz      r5, 0x804(r3)
    lbz      r7, 0x0(r5)
    addi     r0, r5, 0x1
    stw      r0, 0x804(r3)
    lwz      r6, 0x804(r3)
    lbz      r5, 0x0(r6)
    addi     r0, r6, 0x1
    stw      r0, 0x804(r3)
    stb      r8, 0x4588(r4)
    stb      r7, 0x4589(r4)
    stb      r5, 0x458a(r4)
    b        .L_80080270
.L_8008014C:
    lwz      r5, 0x804(r3)
    lis      r0, 0x4330
    stw      r0, 0x8(r1)
    addis    r4, r3, 0x4
    lbz      r7, 0x0(r5)
    addi     r0, r5, 0x1
    lfd      f1, lbl_8065D050@sda21(r0)
    stw      r0, 0x804(r3)
    lfs      f2, lbl_8065D030@sda21(r0)
    lwz      r6, 0x804(r3)
    lbz      r5, 0x0(r6)
    addi     r0, r6, 0x1
    rlwimi   r7, r5, 8, 16, 23
    stw      r0, 0x804(r3)
    extsh    r0, r7
    xoris    r0, r0, 0x8000
    stw      r0, 0xc(r1)
    lfd      f0, 0x8(r1)
    fsubs    f0, f0, f1
    fmuls    f0, f2, f0
    stfs     f0, 0x457c(r4)
    b        .L_80080270
.L_800801A4:
    lwz      r5, 0x804(r3)
    lis      r0, 0x4330
    stw      r0, 0x8(r1)
    addis    r4, r3, 0x4
    lbz      r7, 0x0(r5)
    addi     r5, r5, 0x1
    stw      r0, 0x10(r1)
    lfd      f3, lbl_8065D050@sda21(r0)
    stw      r5, 0x804(r3)
    lfs      f2, lbl_8065D034@sda21(r0)
    lwz      r6, 0x804(r3)
    lbz      r5, 0x0(r6)
    addi     r0, r6, 0x1
    stw      r0, 0x804(r3)
    rlwimi   r7, r5, 8, 16, 23
    extsh    r0, r7
    lwz      r5, 0x804(r3)
    xoris    r0, r0, 0x8000
    stw      r0, 0xc(r1)
    lbz      r7, 0x0(r5)
    addi     r0, r5, 0x1
    lfd      f0, 0x8(r1)
    stw      r0, 0x804(r3)
    fsubs    f0, f0, f3
    lwz      r6, 0x804(r3)
    lbz      r5, 0x0(r6)
    addi     r0, r6, 0x1
    fdivs    f1, f0, f2
    stw      r0, 0x804(r3)
    rlwimi   r7, r5, 8, 16, 23
    extsh    r0, r7
    xoris    r0, r0, 0x8000
    stw      r0, 0x14(r1)
    lfd      f0, 0x10(r1)
    stfs     f1, 0x4580(r4)
    fsubs    f0, f0, f3
    fdivs    f0, f0, f2
    stfs     f0, 0x4584(r4)
    b        .L_80080270
.L_80080240:
    lis      r3, sMessageUnknownFontCommand@ha
    addi     r3, r3, sMessageUnknownFontCommand@l
    crclr    cr1eq
    bl       FormatToStaticBuffer
    lis      r4, sMessageSetFontCommandName@ha
    mr       r6, r3
    addi     r7, r4, sMessageSetFontCommandName@l
    li       r3, lbl_8065D038@sda21
    li       r4, 0x831
    li       r5, lbl_80664158@sda21
    crclr    cr1eq
    bl       OSPanic
.L_80080270:
    lwz      r0, 0x24(r1)
    li       r3, 0x0
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: 0x80080284
 * @note Size: 0x2FC
 */
extern "C" s32 CMsg__cmd_SFX(CMsg* msg)
{
	s32 sfx = ReadMsgU8(msg);
	sfx |= ReadMsgU8(msg) << 8;
	sfx |= ReadMsgU8(msg) << 16;
	sfx |= ReadMsgU8(msg) << 24;
	s8 command = ReadMsgU8(msg);
	switch (command) {
	case 0:
		if (sfx == -2) {
			KeyOffGameSoundTrack(&gGame.mSound, -1);
		} else {
			StopGameSfx(&gGame.mSound, sfx);
		}
		break;
	case 1:
		PlayGameSound(&gGame.mSound, sfx, 255);
		break;
	case 2: {
		u16 frames = ReadMsgS16(msg);
		FadeOutGameSfx(&gGame.mSound, sfx, frames);
		break;
	}
	case 3: {
		u16 frames = ReadMsgS16(msg);
		FadeInGameSfx(&gGame.mSound, sfx, frames);
		break;
	}
	case 4: {
		u16 frames = ReadMsgS16(msg);
		u8 volume  = ReadMsgU8(msg);
		if (!IsGameSoundPlaying(&gGame.mSound, sfx)) {
			PlayGameSound(&gGame.mSound, sfx, volume);
		} else {
			SetGameSfxVolume(&gGame.mSound, sfx, volume, frames);
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
	case -1:
		if (IsGameSoundPlaying(&gGame.mSound, sfx)) {
#if defined(VERSION_PAL)
			msg->mData.mCursor -= 6;
#else
			msg->mData.mCursor -= 3;
#endif
			return 1;
		}
		break;
	}
	return 0;
}
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stmw     r27, 0xc(r1)
    mr       r30, r3
    lwz      r4, 0x804(r3)
    lbz      r31, 0x0(r4)
    addi     r0, r4, 0x1
    stw      r0, 0x804(r3)
    lwz      r4, 0x804(r30)
    lbz      r3, 0x0(r4)
    addi     r0, r4, 0x1
    stw      r0, 0x804(r30)
    rlwimi   r31, r3, 8, 16, 23
    lwz      r4, 0x804(r30)
    lbz      r3, 0x0(r4)
    addi     r0, r4, 0x1
    stw      r0, 0x804(r30)
    slwi     r0, r3, 16
    or       r31, r31, r0
    lwz      r4, 0x804(r30)
    lbz      r3, 0x0(r4)
    addi     r0, r4, 0x1
    stw      r0, 0x804(r30)
    slwi     r0, r3, 24
    or       r31, r31, r0
    lwz      r4, 0x804(r30)
    lbz      r3, 0x0(r4)
    addi     r0, r4, 0x1
    extsb    r3, r3
    stw      r0, 0x804(r30)
    addi     r0, r3, 0x1
    cmplwi   r0, 0x7
    bgt      .L_80080568
    lis      r3, jumptable_802254AC@ha
    slwi     r0, r0, 2
    addi     r3, r3, jumptable_802254AC@l
    lwzx     r0, r3, r0
    mtctr    r0
    bctr
.L_80080324:
    cmpwi    r31, -0x2
    bne      .L_80080348
    lis      r3, gGame@ha
    li       r4, -0x1
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x7e24
    bl       KeyOffGameSoundTrack
    b        .L_80080568
.L_80080348:
    lis      r3, gGame@ha
    mr       r4, r31
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x7e24
    bl       StopGameSfx
    b        .L_80080568
.L_80080364:
    lis      r3, gGame@ha
    mr       r4, r31
    addi     r3, r3, gGame@l
    li       r5, 0xff
    addis    r3, r3, 0xe
    addi     r3, r3, 0x7e24
    bl       PlayGameSound
    b        .L_80080568
.L_80080384:
    lwz      r5, 0x804(r30)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    mr       r4, r31
    lbz      r7, 0x0(r5)
    addi     r0, r5, 0x1
    addis    r3, r3, 0xe
    stw      r0, 0x804(r30)
    addi     r3, r3, 0x7e24
    lwz      r6, 0x804(r30)
    lbz      r5, 0x0(r6)
    addi     r0, r6, 0x1
    rlwimi   r7, r5, 8, 16, 23
    stw      r0, 0x804(r30)
    clrlwi   r5, r7, 16
    bl       FadeOutGameSfx
    b        .L_80080568
.L_800803C8:
    lwz      r5, 0x804(r30)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    mr       r4, r31
    lbz      r7, 0x0(r5)
    addi     r0, r5, 0x1
    addis    r3, r3, 0xe
    stw      r0, 0x804(r30)
    addi     r3, r3, 0x7e24
    lwz      r6, 0x804(r30)
    lbz      r5, 0x0(r6)
    addi     r0, r6, 0x1
    rlwimi   r7, r5, 8, 16, 23
    stw      r0, 0x804(r30)
    clrlwi   r5, r7, 16
    bl       FadeInGameSfx
    b        .L_80080568
.L_8008040C:
    lwz      r5, 0x804(r30)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    mr       r4, r31
    lbz      r7, 0x0(r5)
    addi     r0, r5, 0x1
    addis    r28, r3, 0xe
    stw      r0, 0x804(r30)
    addi     r28, r28, 0x7e24
    mr       r3, r28
    lwz      r6, 0x804(r30)
    lbz      r5, 0x0(r6)
    addi     r0, r6, 0x1
    stw      r0, 0x804(r30)
    rlwimi   r7, r5, 8, 16, 23
    clrlwi   r27, r7, 16
    lwz      r5, 0x804(r30)
    lbz      r29, 0x0(r5)
    addi     r0, r5, 0x1
    stw      r0, 0x804(r30)
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    bne      .L_8008047C
    mr       r3, r28
    mr       r4, r31
    mr       r5, r29
    bl       PlayGameSound
    b        .L_80080568
.L_8008047C:
    mr       r3, r28
    mr       r4, r31
    mr       r5, r29
    mr       r6, r27
    bl       SetGameSfxVolume
    b        .L_80080568
.L_80080494:
    cmpwi    r31, -0x2
    bne      .L_800804B8
    lis      r3, gGame@ha
    li       r4, -0x1
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x7e24
    bl       PauseGameSoundTrack
    b        .L_80080568
.L_800804B8:
    clrrwi   r3, r31, 16
    addis    r0, r3, 0x1
    cmplwi   r0, 0x0
    bne      .L_800804E8
    lis      r3, gGame@ha
    mr       r4, r31
    addi     r3, r3, gGame@l
    li       r5, 0x0
    addis    r3, r3, 0xe
    addi     r3, r3, 0x7e24
    bl       PauseGameMusic
    b        .L_80080568
.L_800804E8:
    mr       r3, r31
    bl       AXDriverPauseSfx
    b        .L_80080568
.L_800804F4:
    cmpwi    r31, -0x2
    bne      .L_80080518
    lis      r3, gGame@ha
    li       r4, -0x1
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x7e24
    bl       ResumeGameSoundTrack
    b        .L_80080568
.L_80080518:
    lis      r3, gGame@ha
    mr       r4, r31
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x7e24
    bl       ResumeGameSfx
    b        .L_80080568
.L_80080534:
    lis      r3, gGame@ha
    mr       r4, r31
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    addi     r3, r3, 0x7e24
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_80080568
    lwz      r4, 0x804(r30)
    li       r3, 0x1
    subi     r0, r4, 0x3
    stw      r0, 0x804(r30)
    b        .L_8008056C
.L_80080568:
    li       r3, 0x0
.L_8008056C:
    lmw      r27, 0xc(r1)
    lwz      r0, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: 0x80080580
 * @note Size: 0x364
 */
extern "C" s32 CMsg__cmd_ANIM(CMsg* msg)
{
	u8 flags = ReadMsgU8(msg);
	s16 id   = ReadMsgS16(msg);
	s32 anim = ReadMsgU8(msg);
	anim |= ReadMsgU8(msg) << 8;
	anim |= ReadMsgU8(msg) << 16;
	anim |= ReadMsgU8(msg) << 24;
	if (flags & 1) {
		anim = gEventVars.mVars[anim];
	}
	void* state = &gGame.mEvt.mData;
	XObj* obj   = *(void**)state != NULL ? ResolveEventObjectId(state, id) : NULL;
	if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		if (anim == -1) {
			HSD_AObj* aobj = GetAObj(obj->mJObj);
			if (!(aobj->flags & (AOBJ_NO_ANIM | AOBJ_REWINDED))) {
				msg->mData.mCursor -= 8;
				return 1;
			}
		} else if (!(flags & 0x10)) {
			obj->mAnmQueue.Clear();
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
	}
	return 0;
}
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stmw     r27, 0xc(r1)
    mr       r28, r3
    lwz      r3, 0x804(r3)
    lbz      r31, 0x0(r3)
    addi     r0, r3, 0x1
    stw      r0, 0x804(r28)
    clrlwi.  r0, r31, 31
    lwz      r3, 0x804(r28)
    lbz      r6, 0x0(r3)
    addi     r0, r3, 0x1
    stw      r0, 0x804(r28)
    lwz      r4, 0x804(r28)
    lbz      r3, 0x0(r4)
    addi     r0, r4, 0x1
    stw      r0, 0x804(r28)
    rlwimi   r6, r3, 8, 16, 23
    extsh    r4, r6
    lwz      r5, 0x804(r28)
    lbz      r29, 0x0(r5)
    addi     r0, r5, 0x1
    stw      r0, 0x804(r28)
    lwz      r5, 0x804(r28)
    lbz      r3, 0x0(r5)
    addi     r0, r5, 0x1
    stw      r0, 0x804(r28)
    rlwimi   r29, r3, 8, 16, 23
    lwz      r5, 0x804(r28)
    lbz      r3, 0x0(r5)
    addi     r0, r5, 0x1
    stw      r0, 0x804(r28)
    slwi     r0, r3, 16
    or       r29, r29, r0
    lwz      r3, 0x804(r28)
    lbz      r0, 0x0(r3)
    addi     r3, r3, 0x1
    slwi     r0, r0, 24
    stw      r3, 0x804(r28)
    or       r29, r29, r0
    beq      .L_80080638
    lis      r3, gEventVars@ha
    slwi     r0, r29, 2
    addi     r3, r3, gEventVars@l
    lwzx     r29, r3, r0
.L_80080638:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x6
    lwzu     r0, -0x2f80(r3)
    cmplwi   r0, 0x0
    beq      .L_8008065C
    bl       ResolveEventObjectId
    mr       r30, r3
    b        .L_80080660
.L_8008065C:
    li       r30, 0x0
.L_80080660:
    cmplwi   r30, 0x0
    beq      .L_800808CC
    lwz      r0, 0x228(r30)
    li       r5, 0x0
    lwz      r4, 0x22c(r30)
    li       r3, 0x1
    and      r0, r0, r5
    and      r3, r4, r3
    xor      r3, r3, r5
    xor      r0, r0, r5
    or.      r0, r3, r0
    beq      .L_800808CC
    cmpwi    r29, -0x1
    bne      .L_80080704
    lwz      r29, 0x0(r30)
    cmplwi   r29, 0x0
    beq      .L_800806E0
    mr       r3, r29
    bl       FindJointAnimationInTree
    cmplwi   r3, 0x0
    beq      .L_800806B8
    b        .L_800806E4
.L_800806B8:
    mr       r3, r29
    bl       FindMaterialOrTextureAnimationInJointTree
    cmplwi   r3, 0x0
    beq      .L_800806CC
    b        .L_800806E4
.L_800806CC:
    mr       r3, r29
    bl       FindPolygonAnimationInJointTree
    cmplwi   r3, 0x0
    beq      .L_800806E0
    b        .L_800806E4
.L_800806E0:
    li       r3, 0x0
.L_800806E4:
    lwz      r0, 0x0(r3)
    andis.   r0, r0, 0x4400
    bne      .L_800808CC
    lwz      r4, 0x804(r28)
    li       r3, 0x1
    subi     r0, r4, 0x8
    stw      r0, 0x804(r28)
    b        .L_800808D0
.L_80080704:
    rlwinm.  r0, r31, 0, 27, 27
    bne      .L_80080870
    stw      r5, 0x810(r30)
    li       r3, -0x1
    clrrwi.  r0, r29, 16
    stw      r3, 0x814(r30)
    stw      r3, 0x81c(r30)
    stw      r3, 0x824(r30)
    stw      r3, 0x82c(r30)
    stw      r3, 0x834(r30)
    stw      r3, 0x83c(r30)
    stw      r3, 0x844(r30)
    stw      r3, 0x84c(r30)
    stw      r3, 0x854(r30)
    stw      r3, 0x85c(r30)
    stw      r3, 0x864(r30)
    stw      r3, 0x86c(r30)
    stw      r3, 0x874(r30)
    stw      r3, 0x87c(r30)
    stw      r3, 0x884(r30)
    stw      r3, 0x88c(r30)
    beq      .L_8008081C
    mr       r3, r30
    clrlwi   r28, r29, 16
    lwz      r12, 0x34(r30)
    mr       r4, r28
    srawi    r29, r29, 16
    lwz      r12, 0xf0(r12)
    mtctr    r12
    bctrl
    mr       r0, r3
    mr       r3, r30
    lwz      r12, 0x34(r30)
    mr       r31, r0
    mr       r4, r29
    lwz      r12, 0xf4(r12)
    mtctr    r12
    bctrl
    mr       r0, r3
    mr       r3, r30
    lwz      r12, 0x34(r30)
    mr       r27, r0
    mr       r4, r28
    lwz      r12, 0xf8(r12)
    mtctr    r12
    bctrl
    lwz      r12, 0x34(r30)
    mr       r0, r3
    lfs      f1, lbl_8065D058@sda21(r0)
    mr       r3, r30
    lwz      r12, 0x80(r12)
    mr       r8, r0
    fmr      f3, f1
    mr       r6, r31
    mr       r7, r27
    lfs      f2, lbl_8065D05C@sda21(r0)
    li       r4, 0x0
    lfs      f4, lbl_8065D060@sda21(r0)
    li       r5, 0x1
    mtctr    r12
    bctrl
    cmpwi    r28, 0x3e8
    bge      .L_80080808
    cmpwi    r29, 0x3e8
    blt      .L_80080854
.L_80080808:
    lwz      r3, 0x228(r30)
    lis      r0, 0x8
    or       r0, r3, r0
    stw      r0, 0x228(r30)
    b        .L_80080854
.L_8008081C:
    mr       r3, r30
    lfs      f1, lbl_8065D058@sda21(r0)
    lwz      r12, 0x34(r30)
    mr       r4, r29
    fmr      f3, f1
    lfs      f2, lbl_8065D05C@sda21(r0)
    lwz      r12, 0x80(r12)
    li       r5, 0x0
    lfs      f4, lbl_8065D060@sda21(r0)
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    mtctr    r12
    bctrl
.L_80080854:
    mr       r3, r30
    li       r4, 0x1
    lwz      r12, 0x34(r30)
    lwz      r12, 0x88(r12)
    mtctr    r12
    bctrl
    b        .L_800808CC
.L_80080870:
    li       r0, 0x10
    mtctr    r0
.L_80080878:
    lwz      r0, 0x810(r30)
    add      r3, r0, r5
    slwi     r0, r3, 28
    srwi     r3, r3, 31
    subf     r0, r3, r0
    rotlwi   r0, r0, 4
    add      r0, r0, r3
    slwi     r3, r0, 3
    addi     r0, r3, 0x814
    lwzx     r0, r30, r0
    cmpwi    r0, -0x1
    bne      .L_800808C4
    add      r4, r30, r3
    li       r3, 0x0
    stw      r29, 0x814(r4)
    li       r0, 0x64
    sth      r3, 0x818(r4)
    sth      r0, 0x81a(r4)
    b        .L_800808CC
.L_800808C4:
    addi     r5, r5, 0x1
    bdnz     .L_80080878
.L_800808CC:
    li       r3, 0x0
.L_800808D0:
    lmw      r27, 0xc(r1)
    lwz      r0, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: 0x800808E4
 * @note Size: 0x2A4
 */
extern "C" s32 CMsg__cmd_WAIT(CMsg* msg)
{
	s32 command = ReadMsgU8(msg);
	// 252 to 255 wait for a button instead of a time
	if (!(command != 255 && command != 254 && command != 253 && command != 252)) {
		if (msg->_00 == 0x23) {
			if (!msg->mFont.mBlink) {
				s32 mode = -1;
				if (command == 255) {
					mode = 0;
				}
				if (command == 254) {
					mode = 1;
				}
				if (command == 253 || command == 252) {
					mode = 0;
				}
				msg->mFont.mBlink      = 1;
				msg->mFont._43C6C      = mode;
				msg->mFont.mBlinkPhase = 0.0f;
				msg->mData.mCursor -= 2;
			} else {
				BOOL done = FALSE;
				CPad pad(0);
				if (command == 253 || command == 252) {
					if ((pad.mPad->mTrigger & 0x400) && !(gGame.mStatus.mFlags & 0x20)) {
						done = TRUE;
						if (command == 253) {
							SelectPlayerEquipmentRing(gGame.GetPlayer(), 0);
						} else if (command == 252) {
							SelectPlayerEquipmentRing(gGame.GetPlayer(), 1);
						}
					}
				} else if (((pad.mPad->mTrigger & 0x100) || (pad.mPad->mTrigger & 0x200)) && !(gGame.mStatus.mFlags & 0x20)) {
					done = TRUE;
				}
				if (done) {
					msg->mFont.mBlink = 0;
					ClearGameSoundQueue(&gGame.mSound, msg->mGlyphSoundGroup);
				} else {
					msg->mData.mCursor -= 2;
				}
			}
		}
		return msg->mFont.mBlink;
	}
	// tenths of a second
	f32 frames = (command / 10) * FRAME_RATE;
	frames += (command % 10) * FRAME_RATE / 10.0f;
	msg->_445CC = frames;
	return 0;
}
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    stw      r0, 0x34(r1)
    stw      r31, 0x2c(r1)
    stw      r30, 0x28(r1)
    stw      r29, 0x24(r1)
    mr       r29, r3
    lwz      r3, 0x804(r3)
    lbz      r31, 0x0(r3)
    addi     r0, r3, 0x1
    cmpwi    r31, 0xff
    stw      r0, 0x804(r29)
    beq      .L_80080930
    cmpwi    r31, 0xfe
    beq      .L_80080930
    cmpwi    r31, 0xfd
    beq      .L_80080930
    cmpwi    r31, 0xfc
    bne      .L_80080AEC
.L_80080930:
    lbz      r0, 0x0(r29)
    cmplwi   r0, 0x23
    bne      .L_80080AE0
    lbz      r0, 0x8a4(r29)
    extrwi.  r0, r0, 1, 27
    bne      .L_800809A8
    cmpwi    r31, 0xff
    li       r4, -0x1
    bne      .L_80080958
    li       r4, 0x0
.L_80080958:
    cmpwi    r31, 0xfe
    bne      .L_80080964
    li       r4, 0x1
.L_80080964:
    cmpwi    r31, 0xfd
    beq      .L_80080974
    cmpwi    r31, 0xfc
    bne      .L_80080978
.L_80080974:
    li       r4, 0x0
.L_80080978:
    lbz      r0, 0x8a4(r29)
    li       r3, 0x1
    rlwimi   r0, r3, 4, 27, 27
    addis    r3, r29, 0x4
    stb      r0, 0x8a4(r29)
    lfs      f0, lbl_8065D058@sda21(r0)
    stw      r4, 0x450c(r3)
    stfs     f0, 0x4510(r3)
    lwz      r3, 0x804(r29)
    subi     r0, r3, 0x2
    stw      r0, 0x804(r29)
    b        .L_80080AE0
.L_800809A8:
    addi     r3, r1, 0x8
    li       r30, 0x0
    li       r4, 0x0
    bl       __ct__4CPadFUc
    cmpwi    r31, 0xfd
    beq      .L_800809C8
    cmpwi    r31, 0xfc
    bne      .L_80080A44
.L_800809C8:
    lwz      r3, 0x8(r1)
    lwz      r0, 0x14(r3)
    rlwinm.  r0, r0, 0, 21, 21
    beq      .L_80080A90
    lis      r4, gGame@ha
    li       r3, 0x20
    addi     r6, r4, gGame@l
    lwz      r5, 0x1b14(r6)
    li       r4, 0x0
    lwz      r0, 0x1b10(r6)
    and      r3, r5, r3
    and      r0, r0, r4
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    bne      .L_80080A90
    cmpwi    r31, 0xfd
    li       r30, 0x1
    bne      .L_80080A28
    addis    r3, r6, 0x1
    li       r4, 0x0
    lwz      r3, -0x6944(r3)
    bl       SelectPlayerEquipmentRing
    b        .L_80080A90
.L_80080A28:
    cmpwi    r31, 0xfc
    bne      .L_80080A90
    addis    r3, r6, 0x1
    li       r4, 0x1
    lwz      r3, -0x6944(r3)
    bl       SelectPlayerEquipmentRing
    b        .L_80080A90
.L_80080A44:
    lwz      r3, 0x8(r1)
    lwz      r3, 0x14(r3)
    rlwinm.  r0, r3, 0, 23, 23
    bne      .L_80080A5C
    rlwinm.  r0, r3, 0, 22, 22
    beq      .L_80080A90
.L_80080A5C:
    lis      r4, gGame@ha
    li       r3, 0x20
    addi     r5, r4, gGame@l
    lwz      r0, 0x1b10(r5)
    li       r4, 0x0
    lwz      r5, 0x1b14(r5)
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    bne      .L_80080A90
    li       r30, 0x1
.L_80080A90:
    cmpwi    r30, 0x0
    beq      .L_80080AC8
    lbz      r0, 0x8a4(r29)
    li       r3, 0x0
    rlwimi   r0, r3, 4, 27, 27
    lis      r3, gGame@ha
    stb      r0, 0x8a4(r29)
    addi     r3, r3, gGame@l
    addis    r4, r29, 0x4
    addis    r3, r3, 0xe
    lwz      r4, 0x45fc(r4)
    addi     r3, r3, 0x7e24
    bl       ClearGameSoundQueue
    b        .L_80080AD4
.L_80080AC8:
    lwz      r3, 0x804(r29)
    subi     r0, r3, 0x2
    stw      r0, 0x804(r29)
.L_80080AD4:
    lis      r3, __vt__4CPad@ha
    addi     r0, r3, __vt__4CPad@l
    stw      r0, 0xc(r1)
.L_80080AE0:
    lbz      r0, 0x8a4(r29)
    extrwi   r3, r0, 1, 27
    b        .L_80080B6C
.L_80080AEC:
    lis      r3, 0x6666
    lis      r0, 0x4330
    addi     r3, r3, 0x6667
    stw      r0, 0x18(r1)
    mulhw    r5, r3, r31
    lfd      f3, lbl_8065D050@sda21(r0)
    lfs      f4, lbl_8065D064@sda21(r0)
    addis    r4, r29, 0x4
    lfs      f0, lbl_8065D060@sda21(r0)
    li       r3, 0x0
    srawi    r6, r5, 2
    stw      r0, 0x10(r1)
    srawi    r0, r5, 2
    srwi     r5, r0, 31
    srwi     r7, r6, 31
    add      r0, r0, r5
    mulli    r0, r0, 0xa
    add      r5, r6, r7
    xoris    r5, r5, 0x8000
    subf     r0, r0, r31
    stw      r5, 0x14(r1)
    xoris    r0, r0, 0x8000
    stw      r0, 0x1c(r1)
    lfd      f2, 0x10(r1)
    lfd      f1, 0x18(r1)
    fsubs    f2, f2, f3
    fsubs    f1, f1, f3
    fmuls    f2, f4, f2
    fmuls    f1, f4, f1
    fdivs    f0, f1, f0
    fadds    f2, f2, f0
    stfs     f2, 0x45cc(r4)
.L_80080B6C:
    lwz      r0, 0x34(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    lwz      r29, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

static inline void MsgSetDigits(CMsg* msg, const s8* digits, s32 count, s32 first, s32 selected)
{
	for (s32 i = 0; i < 9; i += 3) {
		for (s32 j = 0; j < 3; j++) {
			msg->mFont.mDigits[i + j] = digits[i + j];
		}
	}
	msg->mFont.mDigitCount     = count;
	msg->mFont.mFirstEditDigit = first;
	msg->mFont.mOtherSelection = selected;
}

/**
 * @note Address: 0x80080B88
 * @note Size: 0x944
 */
extern "C" s32 CMsg__cmd_END(CMsg* msg)
{
	if (msg->mFont.mActive) {
		if (!msg->mFont.mFadeOut) {
			if (msg->_00 == 0x2B) {
				if (msg->_445D0 < 0) {
					msg->_445D0              = msg->_445D8;
					msg->mFont.mSelectedLine = msg->_445D0;
					msg->mFont.mCurLineY     = msg->mFont.mLineY[msg->mFont.mSelectedLine];
					msg->mFont.mCurLineWidth = 0.0f;
				} else if (!(gGame.mStatus.mFlags & 0x20)) {
					CPad pad(0);
					u32 up   = gGame.mStatus.mFlags & 0x80000000 ? 0x10000 : 0x10008;
					u32 down = gGame.mStatus.mFlags & 0x80000000 ? 0x20000 : 0x20004;
					if (pad.mPad->mRepeat & up) {
						if (--msg->_445D0 < 0) {
							msg->_445D0 = msg->mData.mLineCount - 1;
						}
						msg->mFont.mSelectedLine = msg->_445D0;
						PlayGameSound(&gGame.mSound, 0x87, 255);
					} else if (pad.mPad->mRepeat & down) {
						if (++msg->_445D0 >= msg->mData.mLineCount) {
							msg->_445D0 = 0;
						}
						msg->mFont.mSelectedLine = msg->_445D0;
						PlayGameSound(&gGame.mSound, 0x87, 255);
					} else if (pad.mPad->mTrigger & 0x100) {
						msg->_445D4              = msg->_445D0;
						msg->_445D0              = -1;
						msg->mFont.mFadeOut      = 1;
						msg->mFont.mSelectedLine = -1;
						PlayGameSound(&gGame.mSound, 0x88, 255);
					} else if (pad.mPad->mTrigger & 0x200) {
						msg->_445D4              = -1;
						msg->_445D0              = -1;
						msg->mFont.mFadeOut      = 1;
						msg->mFont.mSelectedLine = -1;
						PlayGameSound(&gGame.mSound, 0x86, 255);
					}
				}
			} else if (msg->_445E8 > 0) {
				if (msg->mFont.mOtherSelection < 0) {
					MsgSetDigits(msg, msg->_445DC, msg->_445E8, msg->_445EC, msg->_445F0);
				} else {
					CPad pad(0);
					u32 up       = gGame.mStatus.mFlags & 0x80000000 ? 0x10000 : 0x10008;
					u32 down     = gGame.mStatus.mFlags & 0x80000000 ? 0x20000 : 0x20004;
					u32 next     = gGame.mStatus.mFlags & 0x80000000 ? 0x40000 : 0x40001;
					u32 previous = gGame.mStatus.mFlags & 0x80000000 ? 0x80000 : 0x80002;
					if (pad.mPad->mRepeat & up) {
						if (++msg->_445DC[msg->_445F0] > 9) {
							msg->_445DC[msg->_445F0] = 0;
						}
						MsgSetDigits(msg, msg->_445DC, msg->_445E8, msg->_445EC, msg->_445F0);
						PlayGameSound(&gGame.mSound, 0x87, 255);
					} else if (pad.mPad->mRepeat & down) {
						if (--msg->_445DC[msg->_445F0] < 0) {
							msg->_445DC[msg->_445F0] = 9;
						}
						MsgSetDigits(msg, msg->_445DC, msg->_445E8, msg->_445EC, msg->_445F0);
						PlayGameSound(&gGame.mSound, 0x87, 255);
					} else if (pad.mPad->mRepeat & next) {
						if (++msg->_445F0 > msg->_445E8 - 1) {
							msg->_445F0 = msg->_445E8 - 1;
						} else {
							MsgSetDigits(msg, msg->_445DC, msg->_445E8, msg->_445EC, msg->_445F0);
							PlayGameSound(&gGame.mSound, 0x87, 255);
						}
					} else if (pad.mPad->mRepeat & previous) {
						if (--msg->_445F0 < msg->_445EC) {
							msg->_445F0 = msg->_445EC;
						} else {
							MsgSetDigits(msg, msg->_445DC, msg->_445E8, msg->_445EC, msg->_445F0);
							PlayGameSound(&gGame.mSound, 0x87, 255);
						}
					} else if (pad.mPad->mTrigger & 0x100) {
						msg->mFont.mFadeOut        = 1;
						msg->mFont.mOtherSelection = -1;
						PlayGameSound(&gGame.mSound, 0x88, 255);
						s32 result = 0;
						for (s32 i = 9; i >= 0; i--) {
							result = result * 10 + msg->_445DC[i];
						}
						gGame.mEvt._10 = result;
					} else if (pad.mPad->mTrigger & 0x200) {
						msg->mFont.mFadeOut        = 1;
						msg->mFont.mOtherSelection = -1;
						PlayGameSound(&gGame.mSound, 0x86, 255);
						gGame.mEvt._10 = -1;
					}
				}
			} else if (msg->_445F8 != 0) {
				if (msg->_445F8 == 1) {
					if (gGame.GetPlayer()->mOXMenu.mStarted < 0) {
						msg->_445F8++;
					}
				} else {
					msg->mFont.mFadeOut = 1;
				}
			} else {
				msg->mFont.mFadeOut = 1;
			}
		}
		msg->mData.mCursor--;
	} else {
		msg->mData.mCursor = NULL;
#if !defined(VERSION_JP)
		msg->mData.mIconColumns = 0;
#endif
	}
	return 1;
}
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stw      r31, 0x1c(r1)
    mr       r31, r3
    stw      r30, 0x18(r1)
    lbz      r3, 0x8a4(r3)
    extrwi.  r0, r3, 1, 24
    beq      .L_800814A4
    extrwi.  r0, r3, 1, 26
    bne      .L_80081494
    lbz      r0, 0x0(r31)
    cmplwi   r0, 0x2b
    bne      .L_80080DF8
    addis    r4, r31, 0x4
    lwz      r0, 0x45d0(r4)
    cmpwi    r0, 0x0
    bge      .L_80080C00
    lwz      r0, 0x45d8(r4)
    lfs      f0, lbl_8065D058@sda21(r0)
    stw      r0, 0x45d0(r4)
    lwz      r0, 0x45d0(r4)
    stw      r0, 0x458c(r4)
    lwz      r0, 0x458c(r4)
    slwi     r0, r0, 2
    add      r3, r4, r0
    lfs      f1, 0x4548(r3)
    stfs     f1, 0x4590(r4)
    stfs     f0, 0x4594(r4)
    b        .L_80081494
.L_80080C00:
    lis      r4, gGame@ha
    li       r3, 0x20
    addi     r30, r4, gGame@l
    lwz      r5, 0x1b14(r30)
    li       r4, 0x0
    lwz      r0, 0x1b10(r30)
    and      r3, r5, r3
    and      r0, r0, r4
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    bne      .L_80081494
    addi     r3, r1, 0x10
    li       r4, 0x0
    bl       __ct__4CPadFUc
    lwz      r4, 0x1b10(r30)
    li       r0, 0x0
    lwz      r5, 0x1b14(r30)
    lis      r3, 0x8000
    and      r6, r4, r0
    and      r5, r5, r3
    xor      r3, r5, r0
    xor      r0, r6, r0
    or.      r0, r3, r0
    beq      .L_80080C6C
    lis      r4, 0x1
    b        .L_80080C74
.L_80080C6C:
    lis      r3, 0x1
    addi     r4, r3, 0x8
.L_80080C74:
    li       r0, 0x0
    xor      r3, r5, r0
    xor      r0, r6, r0
    or.      r0, r3, r0
    beq      .L_80080C90
    lis      r6, 0x2
    b        .L_80080C98
.L_80080C90:
    lis      r3, 0x2
    addi     r6, r3, 0x4
.L_80080C98:
    lwz      r5, 0x10(r1)
    lwz      r3, 0x1c(r5)
    and.     r0, r3, r4
    beq      .L_80080CF4
    addis    r4, r31, 0x4
    lwz      r3, 0x45d0(r4)
    subic.   r0, r3, 0x1
    stw      r0, 0x45d0(r4)
    bge      .L_80080CC8
    lwz      r3, 0x808(r31)
    subi     r0, r3, 0x1
    stw      r0, 0x45d0(r4)
.L_80080CC8:
    addis    r5, r31, 0x4
    lis      r3, gGame@ha
    lwz      r0, 0x45d0(r5)
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    li       r4, 0x87
    stw      r0, 0x458c(r5)
    li       r5, 0xff
    addi     r3, r3, 0x7e24
    bl       PlayGameSound
    b        .L_80080DE8
.L_80080CF4:
    and.     r0, r3, r6
    beq      .L_80080D4C
    addis    r4, r31, 0x4
    lwz      r3, 0x45d0(r4)
    addi     r3, r3, 0x1
    stw      r3, 0x45d0(r4)
    lwz      r0, 0x808(r31)
    cmpw     r3, r0
    blt      .L_80080D20
    li       r0, 0x0
    stw      r0, 0x45d0(r4)
.L_80080D20:
    addis    r5, r31, 0x4
    lis      r3, gGame@ha
    lwz      r0, 0x45d0(r5)
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    li       r4, 0x87
    stw      r0, 0x458c(r5)
    li       r5, 0xff
    addi     r3, r3, 0x7e24
    bl       PlayGameSound
    b        .L_80080DE8
.L_80080D4C:
    lwz      r3, 0x14(r5)
    rlwinm.  r0, r3, 0, 23, 23
    beq      .L_80080DA0
    addis    r8, r31, 0x4
    lis      r3, gGame@ha
    lwz      r0, 0x45d0(r8)
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    li       r7, -0x1
    stw      r0, 0x45d4(r8)
    li       r6, 0x1
    li       r4, 0x88
    li       r5, 0xff
    stw      r7, 0x45d0(r8)
    addi     r3, r3, 0x7e24
    lbz      r0, 0x8a4(r31)
    rlwimi   r0, r6, 5, 26, 26
    stb      r0, 0x8a4(r31)
    stw      r7, 0x458c(r8)
    bl       PlayGameSound
    b        .L_80080DE8
.L_80080DA0:
    rlwinm.  r0, r3, 0, 22, 22
    beq      .L_80080DE8
    addis    r6, r31, 0x4
    li       r7, -0x1
    stw      r7, 0x45d4(r6)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    li       r5, 0x1
    stw      r7, 0x45d0(r6)
    addis    r3, r3, 0xe
    li       r4, 0x86
    lbz      r0, 0x8a4(r31)
    rlwimi   r0, r5, 5, 26, 26
    li       r5, 0xff
    addi     r3, r3, 0x7e24
    stb      r0, 0x8a4(r31)
    stw      r7, 0x458c(r6)
    bl       PlayGameSound
.L_80080DE8:
    lis      r3, __vt__4CPad@ha
    addi     r0, r3, __vt__4CPad@l
    stw      r0, 0x14(r1)
    b        .L_80081494
.L_80080DF8:
    addis    r5, r31, 0x4
    lwz      r3, 0x45e8(r5)
    cmpwi    r3, 0x0
    ble      .L_8008142C
    lwz      r0, 0x45ac(r5)
    cmpwi    r0, 0x0
    bge      .L_80080E84
    lwz      r6, 0x45f0(r5)
    addi     r7, r31, 0x3
    lwz      r4, 0x45ec(r5)
    addis    r8, r7, 0x4
    lbz      r0, 0x45dc(r5)
    addi     r7, r31, 0x6
    stb      r0, 0x4598(r5)
    lbz      r0, 0x45dd(r5)
    stb      r0, 0x4599(r5)
    lbz      r0, 0x45de(r5)
    stb      r0, 0x459a(r5)
    lbz      r0, 0x45dc(r8)
    stb      r0, 0x4598(r8)
    lbz      r0, 0x45dd(r8)
    stb      r0, 0x4599(r8)
    lbz      r0, 0x45de(r8)
    stb      r0, 0x459a(r8)
    addis    r8, r7, 0x4
    lbz      r0, 0x45dc(r8)
    stb      r0, 0x4598(r8)
    lbz      r0, 0x45dd(r8)
    stb      r0, 0x4599(r8)
    lbz      r0, 0x45de(r8)
    stb      r0, 0x459a(r8)
    stw      r3, 0x45a4(r5)
    stw      r4, 0x45a8(r5)
    stw      r6, 0x45ac(r5)
    b        .L_80081494
.L_80080E84:
    addi     r3, r1, 0x8
    li       r4, 0x0
    bl       __ct__4CPadFUc
    lis      r4, gGame@ha
    lis      r3, 0x8000
    addi     r5, r4, gGame@l
    li       r0, 0x0
    lwz      r4, 0x1b10(r5)
    lwz      r5, 0x1b14(r5)
    and      r6, r4, r0
    and      r5, r5, r3
    xor      r3, r5, r0
    xor      r0, r6, r0
    or.      r0, r3, r0
    beq      .L_80080EC8
    lis      r4, 0x1
    b        .L_80080ED0
.L_80080EC8:
    lis      r3, 0x1
    addi     r4, r3, 0x8
.L_80080ED0:
    li       r0, 0x0
    xor      r3, r5, r0
    xor      r0, r6, r0
    or.      r0, r3, r0
    beq      .L_80080EEC
    lis      r7, 0x2
    b        .L_80080EF4
.L_80080EEC:
    lis      r3, 0x2
    addi     r7, r3, 0x4
.L_80080EF4:
    li       r0, 0x0
    xor      r3, r5, r0
    xor      r0, r6, r0
    or.      r0, r3, r0
    beq      .L_80080F10
    lis      r8, 0x4
    b        .L_80080F18
.L_80080F10:
    lis      r3, 0x4
    addi     r8, r3, 0x1
.L_80080F18:
    li       r0, 0x0
    xor      r3, r5, r0
    xor      r0, r6, r0
    or.      r0, r3, r0
    beq      .L_80080F34
    lis      r6, 0x8
    b        .L_80080F3C
.L_80080F34:
    lis      r3, 0x8
    addi     r6, r3, 0x2
.L_80080F3C:
    lwz      r5, 0x8(r1)
    lwz      r3, 0x1c(r5)
    and.     r0, r3, r4
    beq      .L_80081014
    addis    r5, r31, 0x4
    lwz      r0, 0x45f0(r5)
    add      r4, r5, r0
    lbz      r3, 0x45dc(r4)
    addi     r3, r3, 0x1
    extsb    r0, r3
    stb      r3, 0x45dc(r4)
    cmpwi    r0, 0x9
    ble      .L_80080F80
    lwz      r0, 0x45f0(r5)
    li       r4, 0x0
    add      r3, r5, r0
    stb      r4, 0x45dc(r3)
.L_80080F80:
    addis    r6, r31, 0x4
    addi     r10, r31, 0x3
    lwz      r9, 0x45f0(r6)
    addis    r11, r10, 0x4
    lwz      r8, 0x45ec(r6)
    lis      r3, gGame@ha
    lwz      r7, 0x45e8(r6)
    addi     r3, r3, gGame@l
    lbz      r0, 0x45dc(r6)
    addis    r3, r3, 0xe
    addi     r10, r31, 0x6
    li       r4, 0x87
    stb      r0, 0x4598(r6)
    li       r5, 0xff
    addi     r3, r3, 0x7e24
    lbz      r0, 0x45dd(r6)
    stb      r0, 0x4599(r6)
    lbz      r0, 0x45de(r6)
    stb      r0, 0x459a(r6)
    lbz      r0, 0x45dc(r11)
    stb      r0, 0x4598(r11)
    lbz      r0, 0x45dd(r11)
    stb      r0, 0x4599(r11)
    lbz      r0, 0x45de(r11)
    stb      r0, 0x459a(r11)
    addis    r11, r10, 0x4
    lbz      r0, 0x45dc(r11)
    stb      r0, 0x4598(r11)
    lbz      r0, 0x45dd(r11)
    stb      r0, 0x4599(r11)
    lbz      r0, 0x45de(r11)
    stb      r0, 0x459a(r11)
    stw      r7, 0x45a4(r6)
    stw      r8, 0x45a8(r6)
    stw      r9, 0x45ac(r6)
    bl       PlayGameSound
    b        .L_8008141C
.L_80081014:
    and.     r0, r3, r7
    beq      .L_800810E0
    addis    r5, r31, 0x4
    lwz      r0, 0x45f0(r5)
    add      r4, r5, r0
    lbz      r3, 0x45dc(r4)
    subi     r3, r3, 0x1
    extsb.   r0, r3
    stb      r3, 0x45dc(r4)
    bge      .L_8008104C
    lwz      r0, 0x45f0(r5)
    li       r4, 0x9
    add      r3, r5, r0
    stb      r4, 0x45dc(r3)
.L_8008104C:
    addis    r6, r31, 0x4
    addi     r10, r31, 0x3
    lwz      r9, 0x45f0(r6)
    addis    r11, r10, 0x4
    lwz      r8, 0x45ec(r6)
    lis      r3, gGame@ha
    lwz      r7, 0x45e8(r6)
    addi     r3, r3, gGame@l
    lbz      r0, 0x45dc(r6)
    addis    r3, r3, 0xe
    addi     r10, r31, 0x6
    li       r4, 0x87
    stb      r0, 0x4598(r6)
    li       r5, 0xff
    addi     r3, r3, 0x7e24
    lbz      r0, 0x45dd(r6)
    stb      r0, 0x4599(r6)
    lbz      r0, 0x45de(r6)
    stb      r0, 0x459a(r6)
    lbz      r0, 0x45dc(r11)
    stb      r0, 0x4598(r11)
    lbz      r0, 0x45dd(r11)
    stb      r0, 0x4599(r11)
    lbz      r0, 0x45de(r11)
    stb      r0, 0x459a(r11)
    addis    r11, r10, 0x4
    lbz      r0, 0x45dc(r11)
    stb      r0, 0x4598(r11)
    lbz      r0, 0x45dd(r11)
    stb      r0, 0x4599(r11)
    lbz      r0, 0x45de(r11)
    stb      r0, 0x459a(r11)
    stw      r7, 0x45a4(r6)
    stw      r8, 0x45a8(r6)
    stw      r9, 0x45ac(r6)
    bl       PlayGameSound
    b        .L_8008141C
.L_800810E0:
    and.     r0, r3, r8
    beq      .L_8008119C
    addis    r7, r31, 0x4
    lwz      r3, 0x45f0(r7)
    addi     r3, r3, 0x1
    stw      r3, 0x45f0(r7)
    lwz      r6, 0x45e8(r7)
    subi     r0, r6, 0x1
    cmpw     r3, r0
    ble      .L_80081110
    stw      r0, 0x45f0(r7)
    b        .L_8008141C
.L_80081110:
    lwz      r9, 0x45f0(r7)
    addi     r10, r31, 0x3
    lwz      r8, 0x45ec(r7)
    addis    r11, r10, 0x4
    lbz      r0, 0x45dc(r7)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addi     r10, r31, 0x6
    stb      r0, 0x4598(r7)
    addis    r3, r3, 0xe
    li       r4, 0x87
    li       r5, 0xff
    lbz      r0, 0x45dd(r7)
    addi     r3, r3, 0x7e24
    stb      r0, 0x4599(r7)
    lbz      r0, 0x45de(r7)
    stb      r0, 0x459a(r7)
    lbz      r0, 0x45dc(r11)
    stb      r0, 0x4598(r11)
    lbz      r0, 0x45dd(r11)
    stb      r0, 0x4599(r11)
    lbz      r0, 0x45de(r11)
    stb      r0, 0x459a(r11)
    addis    r11, r10, 0x4
    lbz      r0, 0x45dc(r11)
    stb      r0, 0x4598(r11)
    lbz      r0, 0x45dd(r11)
    stb      r0, 0x4599(r11)
    lbz      r0, 0x45de(r11)
    stb      r0, 0x459a(r11)
    stw      r6, 0x45a4(r7)
    stw      r8, 0x45a8(r7)
    stw      r9, 0x45ac(r7)
    bl       PlayGameSound
    b        .L_8008141C
.L_8008119C:
    and.     r0, r3, r6
    beq      .L_80081254
    addis    r7, r31, 0x4
    lwz      r3, 0x45f0(r7)
    subi     r0, r3, 0x1
    stw      r0, 0x45f0(r7)
    lwz      r6, 0x45ec(r7)
    cmpw     r0, r6
    bge      .L_800811C8
    stw      r6, 0x45f0(r7)
    b        .L_8008141C
.L_800811C8:
    lwz      r9, 0x45f0(r7)
    addi     r10, r31, 0x3
    lwz      r8, 0x45e8(r7)
    addis    r11, r10, 0x4
    lbz      r0, 0x45dc(r7)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addi     r10, r31, 0x6
    stb      r0, 0x4598(r7)
    addis    r3, r3, 0xe
    li       r4, 0x87
    li       r5, 0xff
    lbz      r0, 0x45dd(r7)
    addi     r3, r3, 0x7e24
    stb      r0, 0x4599(r7)
    lbz      r0, 0x45de(r7)
    stb      r0, 0x459a(r7)
    lbz      r0, 0x45dc(r11)
    stb      r0, 0x4598(r11)
    lbz      r0, 0x45dd(r11)
    stb      r0, 0x4599(r11)
    lbz      r0, 0x45de(r11)
    stb      r0, 0x459a(r11)
    addis    r11, r10, 0x4
    lbz      r0, 0x45dc(r11)
    stb      r0, 0x4598(r11)
    lbz      r0, 0x45dd(r11)
    stb      r0, 0x4599(r11)
    lbz      r0, 0x45de(r11)
    stb      r0, 0x459a(r11)
    stw      r8, 0x45a4(r7)
    stw      r6, 0x45a8(r7)
    stw      r9, 0x45ac(r7)
    bl       PlayGameSound
    b        .L_8008141C
.L_80081254:
    lwz      r3, 0x14(r5)
    rlwinm.  r0, r3, 0, 23, 23
    beq      .L_800813C8
    lbz      r0, 0x8a4(r31)
    li       r3, 0x1
    rlwimi   r0, r3, 5, 26, 26
    addis    r4, r31, 0x4
    lis      r3, gGame@ha
    stb      r0, 0x8a4(r31)
    addi     r3, r3, gGame@l
    li       r0, -0x1
    addis    r3, r3, 0xe
    stw      r0, 0x45ac(r4)
    li       r4, 0x88
    li       r5, 0xff
    addi     r3, r3, 0x7e24
    bl       PlayGameSound
    li       r7, 0x9
    li       r0, 0x0
    addis    r6, r7, 0x4
    lis      r3, gGame@ha
    addi     r6, r6, 0x45dc
    li       r7, 0x8
    lbzx     r5, r31, r6
    mulli    r0, r0, 0xa
    addis    r6, r7, 0x4
    extsb    r4, r5
    add      r0, r4, r0
    addi     r6, r6, 0x45dc
    lbzx     r5, r31, r6
    mulli    r0, r0, 0xa
    li       r7, 0x7
    extsb    r4, r5
    addis    r6, r7, 0x4
    addi     r3, r3, gGame@l
    add      r0, r4, r0
    li       r7, 0x6
    addi     r6, r6, 0x45dc
    addis    r3, r3, 0x6
    lbzx     r5, r31, r6
    addis    r6, r7, 0x4
    mulli    r0, r0, 0xa
    li       r7, 0x5
    extsb    r4, r5
    addi     r6, r6, 0x45dc
    add      r0, r4, r0
    lbzx     r5, r31, r6
    addis    r6, r7, 0x4
    li       r7, 0x4
    mulli    r0, r0, 0xa
    extsb    r4, r5
    addi     r6, r6, 0x45dc
    lbzx     r5, r31, r6
    add      r0, r4, r0
    addis    r6, r7, 0x4
    mulli    r0, r0, 0xa
    extsb    r4, r5
    addi     r6, r6, 0x45dc
    lbzx     r5, r31, r6
    li       r7, 0x3
    add      r0, r4, r0
    addis    r6, r7, 0x4
    extsb    r4, r5
    mulli    r0, r0, 0xa
    li       r7, 0x2
    addi     r6, r6, 0x45dc
    lbzx     r5, r31, r6
    add      r0, r4, r0
    addis    r6, r7, 0x4
    mulli    r0, r0, 0xa
    extsb    r4, r5
    addi     r6, r6, 0x45dc
    lbzx     r5, r31, r6
    li       r7, 0x1
    add      r0, r4, r0
    addis    r6, r7, 0x4
    extsb    r4, r5
    mulli    r0, r0, 0xa
    li       r7, 0x0
    addi     r6, r6, 0x45dc
    lbzx     r5, r31, r6
    add      r0, r4, r0
    addis    r6, r7, 0x4
    mulli    r0, r0, 0xa
    extsb    r4, r5
    addi     r6, r6, 0x45dc
    lbzx     r5, r31, r6
    add      r0, r4, r0
    mulli    r0, r0, 0xa
    extsb    r4, r5
    add      r0, r4, r0
    stw      r0, -0x2f74(r3)
    b        .L_8008141C
.L_800813C8:
    rlwinm.  r0, r3, 0, 22, 22
    beq      .L_8008141C
    lbz      r0, 0x8a4(r31)
    li       r3, 0x1
    rlwimi   r0, r3, 5, 26, 26
    addis    r4, r31, 0x4
    lis      r3, gGame@ha
    stb      r0, 0x8a4(r31)
    addi     r3, r3, gGame@l
    li       r0, -0x1
    addis    r3, r3, 0xe
    stw      r0, 0x45ac(r4)
    li       r4, 0x86
    li       r5, 0xff
    addi     r3, r3, 0x7e24
    bl       PlayGameSound
    lis      r3, gGame@ha
    li       r0, -0x1
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x6
    stw      r0, -0x2f74(r3)
.L_8008141C:
    lis      r3, __vt__4CPad@ha
    addi     r0, r3, __vt__4CPad@l
    stw      r0, 0xc(r1)
    b        .L_80081494
.L_8008142C:
    lwz      r4, 0x45f8(r5)
    cmpwi    r4, 0x0
    beq      .L_80081484
    cmpwi    r4, 0x1
    bne      .L_80081470
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    addis    r3, r3, 0x1
    lwz      r0, 0x3394(r3)
    cmpwi    r0, 0x0
    beq      .L_80081494
    bge      .L_80081494
    addi     r0, r4, 0x1
    stw      r0, 0x45f8(r5)
    b        .L_80081494
.L_80081470:
    lbz      r0, 0x8a4(r31)
    li       r3, 0x1
    rlwimi   r0, r3, 5, 26, 26
    stb      r0, 0x8a4(r31)
    b        .L_80081494
.L_80081484:
    lbz      r0, 0x8a4(r31)
    li       r3, 0x1
    rlwimi   r0, r3, 5, 26, 26
    stb      r0, 0x8a4(r31)
.L_80081494:
    lwz      r3, 0x804(r31)
    subi     r0, r3, 0x1
    stw      r0, 0x804(r31)
    b        .L_800814B0
.L_800814A4:
    li       r0, 0x0
    stw      r0, 0x804(r31)
    stw      r0, 0x898(r31)
.L_800814B0:
    lwz      r0, 0x24(r1)
    li       r3, 0x1
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: 0x800814CC
 * @note Size: 0xD0
 */
extern "C" void CMsg_Draw(CMsg* msg)
{
	SetGsChannelEnabled(&gGame.mGs, 0, 0);
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 3, 0);
	if (msg->_445C8 == 0.0f) {
		CMsgFont_Draw(&msg->mFont, 1.0f);
	} else {
		f32 t = (msg->_445C8 - msg->_445C4) / msg->_445C8;
		if (t > 1.0f) {
			t = 1.0f;
		}
		if (t < 0.0f) {
			t = 0.0f;
		}
		CMsgFont_Draw(&msg->mFont, t);
	}
}

extern "C" {
s32 CMsg__cmd_END(CMsg* msg);
void CMsgFont_Update(CMsgFont* font, u32 step);
void CMsgFont_AddChar(CMsgFont* font, const char* text);
}

static inline void MsgNextLine(CMsg* msg)
{
	msg->mFont.mLine++;
	CMsgData* data      = msg->mFont.mData;
	f32 width           = msg->mFont.mFitLine ? data->mWidths[msg->mFont.mLine] : msg->mFont.mWidth;
	msg->mFont.mCursorX = -(width * 0.5f);
	msg->mFont.mCursorY += msg->mFont.mData->mHeights[msg->mFont.mLine - 1];
	msg->mFont.mLineY[msg->mFont.mLine] = msg->mFont.mCursorY + (msg->mFont.mData->mHeights[msg->mFont.mLine] - 22.0f) * 0.5f;
}

/**
 * @note Address: 0x8008159C
 * @note Size: 0x6D8
 */
extern "C" void CMsg_Update(CMsg* msg, u32 step)
{
	if (msg->mData.mCursor == NULL) {
		return;
	}
	s32 waiting = FALSE;
	while (!waiting) {
		if (msg->mFont.mFadeIn) {
			waiting = TRUE;
			continue;
		}
		if (msg->_445CC > 0.0f) {
			f32 time    = msg->_445CC - step;
			msg->_445CC = time;
			if (time < 0.0f) {
				msg->_445CC = 0.0f;
			} else {
				waiting = TRUE;
			}
			continue;
		}
		if (msg->_445C4 < 0.0f) {
			waiting = TRUE;
			continue;
		}
		if (msg->_445C4 > 0.0f) {
			f32 time    = msg->_445C4 - step;
			msg->_445C4 = time;
			if (time < 0.0f) {
				msg->_445C4 = 0.0f;
				if (msg->_445C8 > 0.0f && msg->_445C8 - step < 0.0f) {
					waiting = TRUE;
				}
			} else {
				waiting = TRUE;
			}
			continue;
		}
		u8 command = ReadMsgU8(msg);
		switch (command) {
		case 10:
			MsgNextLine(msg);
			waiting = FALSE;
			break;
		case 11:
			MsgNextLine(msg);
			waiting = FALSE;
			break;
		case 0:
			waiting = CMsg__cmd_END(msg);
			break;
		case 1: {
			s32 time = ReadMsgU8(msg);
			if (time == 255) {
				if (msg->_00 == 0x23) {
					time = 2;
				} else {
					time = 0;
				}
			}
			msg->_445C8 = time;
			waiting     = FALSE;
			break;
		}
		case 2:
			waiting = CMsg__cmd_WAIT(msg);
			break;
		case 3:
			waiting = CMsg__cmd_ANIM(msg);
			break;
		case 6: {
			u8 flags  = ReadMsgU8(msg);
			s32 value = ReadMsgU8(msg);
			value |= ReadMsgU8(msg) << 8;
			value |= ReadMsgU8(msg) << 16;
			value |= ReadMsgU8(msg) << 24;
			if (flags & 1) {
				value = gEventVars.mVars[value];
			}
			msg->_445D8 = value;
			waiting     = FALSE;
			break;
		}
		case 4:
			waiting = CMsg__cmd_SFX(msg);
			break;
		case 5:
			msg->mGlyphSoundGroup = ReadMsgU8(msg);
			if (msg->mGlyphSoundGroup == 255) {
				msg->mGlyphSoundGroup = -1;
			}
			waiting = FALSE;
			break;
		case 13:
			waiting = CMsg__cmd_SETFONT(msg, command);
			break;
		case 14:
			waiting = CMsg__cmd_SETFONT(msg, command);
			break;
		case 15:
			waiting = CMsg__cmd_SETFONT(msg, command);
			break;
		case 16:
			waiting = CMsg__cmd_SETFONT(msg, command);
			break;
		case 19:
			waiting = CMsg__cmd_SETFONT(msg, command);
			break;
		case 18:
			waiting = CMsg__cmd_SETFONT(msg, command);
			break;
		case 20:
			waiting = CMsg__cmd_SETFONT(msg, command);
			break;
		case 21:
			waiting = CMsg__cmd_SETFONT(msg, command);
			break;
		case 22:
			msg->_445E8 = ReadMsgU8(msg);
			msg->_445EC = ReadMsgU8(msg);
			msg->_445EC = msg->_445E8 - msg->_445EC;
			msg->_445F0 = ReadMsgU8(msg) - 1;
			if (msg->_445F0 > msg->_445E8 - 1) {
				msg->_445F0 = msg->_445E8 - 1;
			} else if (msg->_445F0 < msg->_445EC) {
				msg->_445F0 = msg->_445EC;
			}
			waiting = FALSE;
			break;
		case 23: {
			u32 arg1 = ReadMsgU8(msg);
			u32 arg2 = ReadMsgU8(msg);
			if (gGame.GetPlayer()->mOXMenu.mStarted == 0) {
				OpenPlayerYesNoMenu(gGame.GetPlayer(), arg1, arg2);
				msg->mData.mCursor -= 3;
				waiting = TRUE;
			} else {
				CPad pad(0);
				HandleOXMenuInput(&gGame.GetPlayer()->mOXMenu, &pad);
				if (pad.mPad->mTrigger & 0x100) {
					gGame.mEvt._10 = gGame.GetPlayer()->mOXMenu.mConfirmedIndex;
					waiting        = FALSE;
				} else {
					msg->mData.mCursor -= 3;
					waiting = TRUE;
				}
			}
			break;
		}
		case 24:
			msg->_445F8 = 1;
			waiting     = FALSE;
			break;
		default: {
			char text[3];
			if (MsgHasSjisLead(command)) {
				text[0] = command;
				text[1] = ReadMsgU8(msg);
				text[2] = 0;
			} else if (command == 17) {
				text[0] = command;
				text[1] = ReadMsgU8(msg);
				text[2] = 0;
			} else {
				text[0] = command;
				text[1] = 0;
			}
			CMsgFont_AddChar(&msg->mFont, text);
			msg->_445C4 = msg->_445C8;
			if (msg->mGlyphSoundGroup >= 0) {
				PlayMessageGlyphSound(&gGame.mSound, msg->mGlyphSoundGroup, msg->mFont.mSize / 22.0f, text[0], text[1]);
			}
			break;
		}
		}
	}
	CMsgFont_Update(&msg->mFont, step);
}
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    stw      r0, 0x44(r1)
    stmw     r25, 0x24(r1)
    mr       r31, r3
    mr       r30, r4
    lwz      r0, 0x804(r3)
    cmplwi   r0, 0x0
    beq      .L_80081C60
    lis      r3, gGame@ha
    li       r25, 0x0
    addi     r28, r3, gGame@l
    addis    r27, r28, 0xe
    addis    r29, r28, 0x1
    addis    r26, r28, 0x6
    addi     r27, r27, 0x7e24
    b        .L_80081C4C
.L_800815E0:
    lbz      r0, 0x8a4(r31)
    extrwi.  r0, r0, 1, 25
    beq      .L_800815F4
    li       r25, 0x1
    b        .L_80081C4C
.L_800815F4:
    addis    r3, r31, 0x4
    lfs      f2, lbl_8065D058@sda21(r0)
    lfs      f3, 0x45cc(r3)
    fcmpo    cr0, f3, f2
    ble      .L_80081640
    lis      r0, 0x4330
    stw      r30, 0x1c(r1)
    lfd      f1, lbl_8065D048@sda21(r0)
    stw      r0, 0x18(r1)
    lfd      f0, 0x18(r1)
    fsubs    f0, f0, f1
    fsubs    f0, f3, f0
    fcmpo    cr0, f0, f2
    stfs     f0, 0x45cc(r3)
    bge      .L_80081638
    stfs     f2, 0x45cc(r3)
    b        .L_80081C4C
.L_80081638:
    li       r25, 0x1
    b        .L_80081C4C
.L_80081640:
    lfs      f3, 0x45c4(r3)
    fcmpo    cr0, f3, f2
    bge      .L_80081654
    li       r25, 0x1
    b        .L_80081C4C
.L_80081654:
    ble      .L_800816BC
    lis      r0, 0x4330
    stw      r30, 0x1c(r1)
    lfd      f1, lbl_8065D048@sda21(r0)
    stw      r0, 0x18(r1)
    lfd      f0, 0x18(r1)
    fsubs    f0, f0, f1
    fsubs    f0, f3, f0
    fcmpo    cr0, f0, f2
    stfs     f0, 0x45c4(r3)
    bge      .L_800816B4
    stfs     f2, 0x45c4(r3)
    lfs      f3, 0x45c8(r3)
    fcmpo    cr0, f3, f2
    ble      .L_80081C4C
    stw      r30, 0x1c(r1)
    stw      r0, 0x18(r1)
    lfd      f0, 0x18(r1)
    fsubs    f0, f0, f1
    fsubs    f0, f3, f0
    fcmpo    cr0, f0, f2
    bge      .L_80081C4C
    li       r25, 0x1
    b        .L_80081C4C
.L_800816B4:
    li       r25, 0x1
    b        .L_80081C4C
.L_800816BC:
    lwz      r5, 0x804(r31)
    lbz      r4, 0x0(r5)
    addi     r0, r5, 0x1
    cmplwi   r4, 0x18
    stw      r0, 0x804(r31)
    bgt      .L_80081B68
    lis      r5, jumptable_802254CC@ha
    slwi     r0, r4, 2
    addi     r5, r5, jumptable_802254CC@l
    lwzx     r0, r5, r0
    mtctr    r0
    bctr
.L_800816EC:
    lwz      r4, 0x451c(r3)
    addi     r0, r4, 0x1
    stw      r0, 0x451c(r3)
    lbz      r0, 0x8a4(r31)
    extrwi.  r0, r0, 1, 28
    beq      .L_8008171C
    lwz      r0, 0x451c(r3)
    lwz      r4, 0x8a0(r31)
    slwi     r3, r0, 2
    addi     r0, r3, 0x830
    lfsx     f0, r4, r0
    b        .L_80081720
.L_8008171C:
    lfs      f0, 0x4538(r3)
.L_80081720:
    lfs      f3, lbl_8065D068@sda21(r0)
    addis    r5, r31, 0x4
    lfs      f1, lbl_8065D06C@sda21(r0)
    li       r25, 0x0
    fmuls    f0, f0, f3
    fneg     f0, f0
    stfs     f0, 0x4530(r5)
    lwz      r3, 0x451c(r5)
    lwz      r4, 0x8a0(r31)
    subi     r0, r3, 0x1
    lfs      f2, 0x453c(r5)
    slwi     r3, r0, 2
    addi     r0, r3, 0x858
    lfsx     f0, r4, r0
    fadds    f0, f2, f0
    stfs     f0, 0x453c(r5)
    lwz      r0, 0x451c(r5)
    lwz      r4, 0x8a0(r31)
    slwi     r3, r0, 2
    lfs      f0, 0x453c(r5)
    addi     r0, r3, 0x858
    lfsx     f2, r4, r0
    addis    r3, r3, 0x4
    addi     r3, r3, 0x4548
    fsubs    f1, f2, f1
    fmadds   f0, f1, f3, f0
    stfsx    f0, r31, r3
    b        .L_80081C4C
.L_80081790:
    lwz      r4, 0x451c(r3)
    addi     r0, r4, 0x1
    stw      r0, 0x451c(r3)
    lbz      r0, 0x8a4(r31)
    extrwi.  r0, r0, 1, 28
    beq      .L_800817C0
    lwz      r0, 0x451c(r3)
    lwz      r4, 0x8a0(r31)
    slwi     r3, r0, 2
    addi     r0, r3, 0x830
    lfsx     f0, r4, r0
    b        .L_800817C4
.L_800817C0:
    lfs      f0, 0x4538(r3)
.L_800817C4:
    lfs      f3, lbl_8065D068@sda21(r0)
    addis    r5, r31, 0x4
    lfs      f1, lbl_8065D06C@sda21(r0)
    li       r25, 0x0
    fmuls    f0, f0, f3
    fneg     f0, f0
    stfs     f0, 0x4530(r5)
    lwz      r3, 0x451c(r5)
    lwz      r4, 0x8a0(r31)
    subi     r0, r3, 0x1
    lfs      f2, 0x453c(r5)
    slwi     r3, r0, 2
    addi     r0, r3, 0x858
    lfsx     f0, r4, r0
    fadds    f0, f2, f0
    stfs     f0, 0x453c(r5)
    lwz      r0, 0x451c(r5)
    lwz      r4, 0x8a0(r31)
    slwi     r3, r0, 2
    lfs      f0, 0x453c(r5)
    addi     r0, r3, 0x858
    lfsx     f2, r4, r0
    addis    r3, r3, 0x4
    addi     r3, r3, 0x4548
    fsubs    f1, f2, f1
    fmadds   f0, f1, f3, f0
    stfsx    f0, r31, r3
    b        .L_80081C4C
.L_80081834:
    mr       r3, r31
    bl       CMsg__cmd_END
    mr       r25, r3
    b        .L_80081C4C
.L_80081844:
    lwz      r3, 0x804(r31)
    lbz      r4, 0x0(r3)
    addi     r0, r3, 0x1
    cmpwi    r4, 0xff
    stw      r0, 0x804(r31)
    bne      .L_80081874
    lbz      r0, 0x0(r31)
    cmplwi   r0, 0x23
    bne      .L_80081870
    li       r4, 0x2
    b        .L_80081874
.L_80081870:
    li       r4, 0x0
.L_80081874:
    xoris    r3, r4, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x1c(r1)
    addis    r3, r31, 0x4
    lfd      f1, lbl_8065D050@sda21(r0)
    li       r25, 0x0
    stw      r0, 0x18(r1)
    lfd      f0, 0x18(r1)
    fsubs    f0, f0, f1
    stfs     f0, 0x45c8(r3)
    b        .L_80081C4C
.L_800818A0:
    mr       r3, r31
    bl       CMsg__cmd_WAIT
    mr       r25, r3
    b        .L_80081C4C
.L_800818B0:
    mr       r3, r31
    bl       CMsg__cmd_ANIM
    mr       r25, r3
    b        .L_80081C4C
.L_800818C0:
    lwz      r4, 0x804(r31)
    lbz      r3, 0x0(r4)
    addi     r0, r4, 0x1
    stw      r0, 0x804(r31)
    clrlwi.  r0, r3, 31
    lwz      r3, 0x804(r31)
    lbz      r5, 0x0(r3)
    addi     r0, r3, 0x1
    stw      r0, 0x804(r31)
    lwz      r4, 0x804(r31)
    lbz      r3, 0x0(r4)
    addi     r0, r4, 0x1
    stw      r0, 0x804(r31)
    rlwimi   r5, r3, 8, 16, 23
    lwz      r4, 0x804(r31)
    lbz      r3, 0x0(r4)
    addi     r0, r4, 0x1
    stw      r0, 0x804(r31)
    slwi     r0, r3, 16
    or       r5, r5, r0
    lwz      r3, 0x804(r31)
    lbz      r0, 0x0(r3)
    addi     r3, r3, 0x1
    slwi     r0, r0, 24
    stw      r3, 0x804(r31)
    or       r5, r5, r0
    beq      .L_8008193C
    lis      r3, gEventVars@ha
    slwi     r0, r5, 2
    addi     r3, r3, gEventVars@l
    lwzx     r5, r3, r0
.L_8008193C:
    addis    r3, r31, 0x4
    li       r25, 0x0
    stw      r5, 0x45d8(r3)
    b        .L_80081C4C
.L_8008194C:
    mr       r3, r31
    bl       CMsg__cmd_SFX
    mr       r25, r3
    b        .L_80081C4C
.L_8008195C:
    lwz      r5, 0x804(r31)
    lbz      r4, 0x0(r5)
    addi     r0, r5, 0x1
    stw      r0, 0x804(r31)
    stw      r4, 0x45fc(r3)
    lwz      r0, 0x45fc(r3)
    cmpwi    r0, 0xff
    bne      .L_80081984
    li       r0, -0x1
    stw      r0, 0x45fc(r3)
.L_80081984:
    li       r25, 0x0
    b        .L_80081C4C
.L_8008198C:
    mr       r3, r31
    bl       CMsg__cmd_SETFONT
    mr       r25, r3
    b        .L_80081C4C
.L_8008199C:
    mr       r3, r31
    bl       CMsg__cmd_SETFONT
    mr       r25, r3
    b        .L_80081C4C
.L_800819AC:
    mr       r3, r31
    bl       CMsg__cmd_SETFONT
    mr       r25, r3
    b        .L_80081C4C
.L_800819BC:
    mr       r3, r31
    bl       CMsg__cmd_SETFONT
    mr       r25, r3
    b        .L_80081C4C
.L_800819CC:
    mr       r3, r31
    bl       CMsg__cmd_SETFONT
    mr       r25, r3
    b        .L_80081C4C
.L_800819DC:
    mr       r3, r31
    bl       CMsg__cmd_SETFONT
    mr       r25, r3
    b        .L_80081C4C
.L_800819EC:
    mr       r3, r31
    bl       CMsg__cmd_SETFONT
    mr       r25, r3
    b        .L_80081C4C
.L_800819FC:
    mr       r3, r31
    bl       CMsg__cmd_SETFONT
    mr       r25, r3
    b        .L_80081C4C
.L_80081A0C:
    lwz      r5, 0x804(r31)
    lbz      r4, 0x0(r5)
    addi     r0, r5, 0x1
    stw      r0, 0x804(r31)
    stw      r4, 0x45e8(r3)
    lwz      r5, 0x804(r31)
    lbz      r4, 0x0(r5)
    addi     r0, r5, 0x1
    stw      r0, 0x804(r31)
    stw      r4, 0x45ec(r3)
    lwz      r4, 0x45ec(r3)
    lwz      r0, 0x45e8(r3)
    subf     r0, r4, r0
    stw      r0, 0x45ec(r3)
    lwz      r5, 0x804(r31)
    lbz      r4, 0x0(r5)
    addi     r0, r5, 0x1
    stw      r0, 0x804(r31)
    subi     r0, r4, 0x1
    stw      r0, 0x45f0(r3)
    lwz      r4, 0x45e8(r3)
    lwz      r5, 0x45f0(r3)
    subi     r0, r4, 0x1
    cmpw     r5, r0
    ble      .L_80081A78
    stw      r0, 0x45f0(r3)
    b        .L_80081A88
.L_80081A78:
    lwz      r0, 0x45ec(r3)
    cmpw     r5, r0
    bge      .L_80081A88
    stw      r0, 0x45f0(r3)
.L_80081A88:
    li       r25, 0x0
    b        .L_80081C4C
.L_80081A90:
    lwz      r3, 0x804(r31)
    lbz      r4, 0x0(r3)
    addi     r0, r3, 0x1
    stw      r0, 0x804(r31)
    lwz      r3, 0x804(r31)
    lbz      r5, 0x0(r3)
    addi     r0, r3, 0x1
    stw      r0, 0x804(r31)
    lwz      r3, -0x6944(r29)
    addis    r6, r3, 0x1
    lwz      r0, 0x3394(r6)
    cmpwi    r0, 0x0
    bne      .L_80081ADC
    bl       OpenPlayerYesNoMenu
    lwz      r3, 0x804(r31)
    li       r5, 0x1
    subi     r0, r3, 0x3
    stw      r0, 0x804(r31)
    b        .L_80081B50
.L_80081ADC:
    addi     r3, r1, 0xc
    li       r4, 0x0
    bl       __ct__4CPadFUc
    lwz      r3, -0x6944(r29)
    addi     r4, r1, 0xc
    addis    r3, r3, 0x1
    addi     r3, r3, 0x3390
    bl       HandleOXMenuInput
    lwz      r3, 0xc(r1)
    lwz      r0, 0x14(r3)
    rlwinm.  r0, r0, 0, 23, 23
    beq      .L_80081B20
    lwz      r3, -0x6944(r29)
    addis    r3, r3, 0x1
    lwz      r0, 0x46d0(r3)
    stw      r0, -0x2f74(r26)
    b        .L_80081B40
.L_80081B20:
    lwz      r4, 0x804(r31)
    lis      r3, __vt__4CPad@ha
    addi     r0, r3, __vt__4CPad@l
    li       r5, 0x1
    subi     r3, r4, 0x3
    stw      r3, 0x804(r31)
    stw      r0, 0x10(r1)
    b        .L_80081B50
.L_80081B40:
    lis      r3, __vt__4CPad@ha
    li       r5, 0x0
    addi     r0, r3, __vt__4CPad@l
    stw      r0, 0x10(r1)
.L_80081B50:
    mr       r25, r5
    b        .L_80081C4C
.L_80081B58:
    li       r0, 0x1
    li       r25, 0x0
    stw      r0, 0x45f8(r3)
    b        .L_80081C4C
.L_80081B68:
    lbz      r0, 0x3318(r28)
    cmplwi   r0, 0x6
    bne      .L_80081BD0
    clrlwi   r0, r4, 24
    li       r3, 0x0
    cmplwi   r0, 0x81
    blt      .L_80081B8C
    cmplwi   r0, 0x9f
    ble      .L_80081BA0
.L_80081B8C:
    clrlwi   r0, r4, 24
    cmplwi   r0, 0xe0
    blt      .L_80081BA4
    cmplwi   r0, 0xfc
    bgt      .L_80081BA4
.L_80081BA0:
    li       r3, 0x1
.L_80081BA4:
    clrlwi.  r0, r3, 24
    beq      .L_80081BD0
    stb      r4, 0x8(r1)
    li       r0, 0x0
    lwz      r3, 0x804(r31)
    lbz      r4, 0x0(r3)
    addi     r3, r3, 0x1
    stw      r3, 0x804(r31)
    stb      r4, 0x9(r1)
    stb      r0, 0xa(r1)
    b        .L_80081C0C
.L_80081BD0:
    clrlwi   r0, r4, 24
    cmplwi   r0, 0x11
    bne      .L_80081C00
    stb      r4, 0x8(r1)
    li       r0, 0x0
    lwz      r3, 0x804(r31)
    lbz      r4, 0x0(r3)
    addi     r3, r3, 0x1
    stw      r3, 0x804(r31)
    stb      r4, 0x9(r1)
    stb      r0, 0xa(r1)
    b        .L_80081C0C
.L_80081C00:
    li       r0, 0x0
    stb      r4, 0x8(r1)
    stb      r0, 0x9(r1)
.L_80081C0C:
    addi     r3, r31, 0x8a0
    addi     r4, r1, 0x8
    bl       CMsgFont_AddChar
    addis    r3, r31, 0x4
    lfs      f0, 0x45c8(r3)
    stfs     f0, 0x45c4(r3)
    lwz      r4, 0x45fc(r3)
    cmpwi    r4, 0x0
    blt      .L_80081C4C
    lfs      f1, 0x4570(r3)
    mr       r3, r27
    lfs      f0, lbl_8065D06C@sda21(r0)
    lbz      r5, 0x8(r1)
    fdivs    f1, f1, f0
    lbz      r6, 0x9(r1)
    bl       PlayMessageGlyphSound
.L_80081C4C:
    cmpwi    r25, 0x0
    beq      .L_800815E0
    mr       r4, r30
    addi     r3, r31, 0x8a0
    bl       CMsgFont_Update
.L_80081C60:
    lmw      r25, 0x24(r1)
    lwz      r0, 0x44(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

static inline void ResetMsgState(CMsg* msg)
{
	msg->mData.mCursor = NULL;
#if !defined(VERSION_JP)
	msg->mData.mIconColumns = 0;
#endif
	msg->mFont.mActive    = 0;
	msg->mFont.mFadeIn    = 0;
	msg->mFont.mFadeOut   = 0;
	msg->mFont.mBlink     = 0;
	msg->mFont.mFitLine   = 1;
	msg->mFont._04_04     = 1;
	msg->mFont.mCharCount = 0;
	msg->mFont.mLine      = 0;
	msg->mFont.mPlacement = 0;
	Vec zero              = { 0.0f, 0.0f, 0.0f };
	msg->mFont.mPosition  = zero;
#if defined(VERSION_JP)
	msg->mFont.mSize = msg->mFont.mPrevSize = 22.0f;
#else
	msg->mFont.mSize     = 22.0f;
	msg->mFont.mPrevSize = NAN;
#endif
	msg->mFont.mGlyphColor     = msgColorTable[0];
	msg->mFont.mGlyphAngle     = 0.0f;
	msg->mFont.mGlyphScaleY    = 1.0f;
	msg->mFont.mGlyphScaleX    = 1.0f;
	msg->mFont.mEffect         = 0;
	msg->mFont.mSelectedLine   = -1;
	msg->mFont.mOtherSelection = -1;
	msg->mFont.mAlpha          = 0.0f;
	msg->mFont.mTextAlpha      = 1.0f;
	msg->mFont.mHighlightAlpha = 0.0f;
	msg->mFont._43C6C          = 0;
	msg->mFont.mBlinkPhase     = 0.0f;
	msg->mFont.mTexOffset      = 0.0f;
	for (int i = 0; i < 400; i++) {
		msg->mFont.mGlyphs[i].mPhaseX = msg->mFont.mGlyphs[i].mPhaseY = (i * 75) % 360;
	}
	msg->_445C0 = 1;
	msg->_445C8 = 0.0f;
	msg->_445C4 = 0.0f;
	msg->_445D4 = -1;
	msg->_445D0 = -1;
	msg->_445D8 = 0;
	for (int i = 0; i < 9; i += 3) {
		for (int j = 0; j < 3; j++) {
			msg->_445DC[i + j] = 0;
		}
	}
	msg->_445F0           = 0;
	msg->_445EC           = 0;
	msg->_445E8           = 0;
	msg->_445F8           = 0;
	msg->mGlyphSoundGroup = -1;
}

extern "C" void CMsgData_Format(u8* data, u8* script, MsgFormatArgs* args);
extern "C" void CMsgData_Measure(CMsgData* data);
extern "C" void CMsgFont_Init(CMsgFont* font, u8* data, f32 x, f32 y, s32 mode, s32 style, s32 align);

/**
 * @note Address: 0x80081C74
 * @note Size: 0x508
 */
extern "C" u8* CMsg_Open(CMsg* msg, u8 command, u8* script, MsgFormatArgs* args)
{
	ResetMsgState(msg);
	msg->_00       = command;
	s32 savedCount = args->count;
	CMsgData_Format(msg->mData.mText, script, args);
	CMsgData_Measure(&msg->mData);
	args->count        = savedCount;
	msg->mData.mCursor = msg->mData.mText;
#if !defined(VERSION_JP)
	if (msg->mData.mIconColumns == 0) {
		TPLPalette* palette     = (TPLPalette*)(&gGame.mTpl)->mBuffer;
		CTexObj* texture        = &(&gGame.mTpl)->mTex;
		CTexObj icon            = texture->Init(&palette->descriptorArray[119], 0);
		msg->mData.mIconColumns = (s32)GXGetTexObjWidth(&icon) / 24;
	}
#endif
	u8* cursor = msg->mData.mCursor;
	CMsgFont_Init(&msg->mFont, msg->mData.mText, msg->_44600, msg->_44604, msg->_44608, msg->mData.mAlignment, msg->mData.mStyle);
	msg->SetFontPosition(msg->mFontPosition.x, msg->mFontPosition.y, msg->mFontPosition.z);
	msg->_445C4 = 0.0f;
	msg->_445C8 = msg->_00 == 35 ? 2 : 0;
	msg->_445CC = 0.0f;
	return cursor;
}
/*
    stwu     r1, -0x150(r1)
    mflr     r0
    lis      r8, msgColorTable@ha
    li       r7, 0x0
    stw      r0, 0x154(r1)
    addi     r12, r8, msgColorTable@l
    lis      r10, sMessageFontOrigin@ha
    lfs      f2, lbl_8065D06C@sda21(r0)
    stmw     r23, 0x12c(r1)
    mr       r30, r3
    addi     r24, r10, sMessageFontOrigin@l
    lis      r9, __float_nan@ha
    addi     r23, r9, __float_nan@l
    mr       r10, r5
    mr       r31, r6
    li       r26, 0x1
    addis    r25, r30, 0x4
    lfs      f1, lbl_8065D058@sda21(r0)
    lfs      f0, lbl_8065D05C@sda21(r0)
    li       r11, -0x1
    mr       r8, r7
    mr       r9, r30
    stw      r7, 0x804(r3)
    stw      r7, 0x898(r3)
    lis      r3, 0xb60b
    addi     r6, r3, 0x60b7
    lbz      r0, 0x8a4(r30)
    rlwimi   r0, r7, 7, 24, 24
    stb      r0, 0x8a4(r30)
    lbz      r0, 0x8a4(r30)
    rlwimi   r0, r7, 6, 25, 25
    stb      r0, 0x8a4(r30)
    li       r0, 0x32
    lbz      r5, 0x8a4(r30)
    rlwimi   r5, r7, 5, 26, 26
    stb      r5, 0x8a4(r30)
    lbz      r3, 0x8a4(r30)
    rlwimi   r3, r7, 4, 27, 27
    stb      r3, 0x8a4(r30)
    lbz      r3, 0x8a4(r30)
    rlwimi   r3, r26, 3, 28, 28
    stb      r3, 0x8a4(r30)
    lbz      r3, 0x8a4(r30)
    rlwimi   r3, r26, 2, 29, 29
    stb      r3, 0x8a4(r30)
    stw      r7, 0x4518(r25)
    stw      r7, 0x451c(r25)
    stw      r7, 0x4520(r25)
    lwz      r3, 0x0(r24)
    lwz      r5, 0x4(r24)
    stw      r3, 0x8(r1)
    lwz      r3, 0x8(r24)
    stw      r5, 0xc(r1)
    lfs      f3, 0x8(r1)
    stw      r3, 0x10(r1)
    lfs      f4, 0xc(r1)
    stfs     f3, 0x4524(r25)
    lfs      f3, 0x10(r1)
    stfs     f4, 0x4528(r25)
    stfs     f3, 0x452c(r25)
    stfs     f2, 0x4570(r25)
    lfs      f2, 0x0(r23)
    stfs     f2, 0x4574(r25)
    lwz      r3, 0x0(r12)
    stw      r3, 0x4578(r25)
    stfs     f1, 0x457c(r25)
    stfs     f0, 0x4584(r25)
    stfs     f0, 0x4580(r25)
    stb      r7, 0x4588(r25)
    stw      r11, 0x458c(r25)
    stw      r11, 0x45ac(r25)
    stfs     f1, 0x45b0(r25)
    stfs     f0, 0x45b4(r25)
    stfs     f1, 0x45b8(r25)
    stw      r7, 0x450c(r25)
    stfs     f1, 0x4510(r25)
    stfs     f1, 0x4514(r25)
    mtctr    r0
.L_80081DAC:
    addi     r0, r7, 0x1
    addi     r5, r7, 0x2
    mulli    r0, r0, 0x4b
    addi     r3, r7, 0x3
    addi     r28, r7, 0x5
    addi     r12, r7, 0x6
    mulhw    r11, r6, r8
    addi     r27, r7, 0x4
    mulhw    r24, r6, r0
    add      r11, r11, r8
    srawi    r25, r11, 8
    addi     r11, r7, 0x7
    srwi     r23, r25, 31
    addi     r7, r7, 0x8
    add      r26, r24, r0
    add      r23, r25, r23
    mulli    r5, r5, 0x4b
    srawi    r26, r26, 8
    srwi     r24, r26, 31
    add      r24, r26, r24
    mulhw    r25, r6, r5
    mulli    r23, r23, 0x168
    add      r26, r25, r5
    subf     r23, r23, r8
    srawi    r29, r26, 8
    mulli    r3, r3, 0x4b
    sth      r23, 0x15f0(r9)
    srwi     r25, r29, 31
    sth      r23, 0x15ee(r9)
    addi     r8, r8, 0x258
    mulli    r24, r24, 0x168
    add      r25, r29, r25
    subf     r24, r24, r0
    mulhw    r26, r6, r3
    sth      r24, 0x189c(r9)
    sth      r24, 0x189a(r9)
    mulli    r0, r27, 0x4b
    add      r29, r26, r3
    mulli    r25, r25, 0x168
    srawi    r29, r29, 8
    srwi     r26, r29, 31
    subf     r5, r25, r5
    sth      r5, 0x1b48(r9)
    mulhw    r27, r6, r0
    add      r26, r29, r26
    sth      r5, 0x1b46(r9)
    mulli    r28, r28, 0x4b
    add      r29, r27, r0
    srawi    r29, r29, 8
    srwi     r27, r29, 31
    mulhw    r5, r6, r28
    add      r27, r29, r27
    mulli    r29, r26, 0x168
    add      r5, r5, r28
    subf     r3, r29, r3
    srawi    r5, r5, 8
    sth      r3, 0x1df4(r9)
    srwi     r29, r5, 31
    add      r29, r5, r29
    mulli    r5, r27, 0x168
    sth      r3, 0x1df2(r9)
    subf     r0, r5, r0
    mulli    r12, r12, 0x4b
    sth      r0, 0x20a0(r9)
    sth      r0, 0x209e(r9)
    mulhw    r3, r6, r12
    mulli    r5, r11, 0x4b
    add      r3, r3, r12
    srawi    r3, r3, 8
    srwi     r11, r3, 31
    mulhw    r0, r6, r5
    add      r3, r3, r11
    mulli    r11, r29, 0x168
    add      r0, r0, r5
    subf     r29, r11, r28
    srawi    r0, r0, 8
    mulli    r11, r3, 0x168
    sth      r29, 0x234c(r9)
    srwi     r3, r0, 31
    sth      r29, 0x234a(r9)
    subf     r11, r11, r12
    add      r0, r0, r3
    sth      r11, 0x25f8(r9)
    mulli    r0, r0, 0x168
    sth      r11, 0x25f6(r9)
    subf     r0, r0, r5
    sth      r0, 0x28a4(r9)
    sth      r0, 0x28a2(r9)
    addi     r9, r9, 0x1560
    bdnz     .L_80081DAC
    addis    r7, r30, 0x4
    li       r0, 0x1
    stw      r0, 0x45c0(r7)
    li       r6, 0x0
    lfs      f0, lbl_8065D058@sda21(r0)
    addis    r9, r6, 0x4
    addi     r9, r9, 0x45dc
    li       r0, -0x1
    stfs     f0, 0x45c8(r7)
    add      r9, r30, r9
    li       r8, 0x3
    mr       r5, r31
    stfs     f0, 0x45c4(r7)
    addi     r3, r30, 0x4
    stw      r0, 0x45d4(r7)
    stw      r0, 0x45d0(r7)
    stw      r6, 0x45d8(r7)
    stb      r6, 0x0(r9)
    stb      r6, 0x1(r9)
    stb      r6, 0x2(r9)
    addis    r9, r8, 0x4
    addi     r9, r9, 0x45dc
    li       r8, 0x6
    add      r9, r30, r9
    stb      r6, 0x0(r9)
    stb      r6, 0x1(r9)
    stb      r6, 0x2(r9)
    addis    r9, r8, 0x4
    addi     r9, r9, 0x45dc
    add      r9, r30, r9
    stb      r6, 0x0(r9)
    stb      r6, 0x1(r9)
    stb      r6, 0x2(r9)
    stw      r6, 0x45f0(r7)
    stw      r6, 0x45ec(r7)
    stw      r6, 0x45e8(r7)
    stw      r6, 0x45f8(r7)
    stw      r0, 0x45fc(r7)
    stb      r4, 0x0(r30)
    mr       r4, r10
    lwz      r23, 0x4(r31)
    bl       CMsgData_Format
    addi     r3, r30, 0x4
    bl       CMsgData_Measure
    stw      r23, 0x4(r31)
    addi     r0, r30, 0x4
    stw      r0, 0x804(r30)
    lwz      r0, 0x898(r30)
    cmpwi    r0, 0x0
    bne      .L_800820D8
    lis      r3, gGame@ha
    li       r5, 0x0
    addi     r3, r3, gGame@l
    lwz      r4, 0x3d4c(r3)
    addi     r3, r3, 0x3d6c
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x3b8
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r8, 0x0(r3)
    lis      r4, __vt__7CTexObj@ha
    lwz      r7, 0x4(r3)
    addi     r6, r4, __vt__7CTexObj@l
    li       r0, 0x18
    addi     r5, r1, 0x4c
    stw      r8, 0x14(r1)
    addi     r4, r3, 0x38
    stw      r7, 0x18(r1)
    lwz      r8, 0x8(r3)
    lwz      r7, 0xc(r3)
    stw      r8, 0x1c(r1)
    stw      r7, 0x20(r1)
    lwz      r8, 0x10(r3)
    lwz      r7, 0x14(r3)
    stw      r8, 0x24(r1)
    stw      r7, 0x28(r1)
    lwz      r8, 0x18(r3)
    lwz      r7, 0x1c(r3)
    stw      r8, 0x2c(r1)
    stw      r7, 0x30(r1)
    stw      r6, 0x110(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x34(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x38(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x3c(r1)
    stw      r6, 0x40(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x44(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x48(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x4c(r1)
    mtctr    r0
.L_80082090:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_80082090
    addi     r3, r1, 0x14
    bl       GXGetTexObjWidth
    lis      r5, 0x2aab
    lis      r4, __vt__7CTexObj@ha
    clrlwi   r3, r3, 16
    subi     r5, r5, 0x5555
    addi     r0, r4, __vt__7CTexObj@l
    mulhw    r3, r5, r3
    srawi    r3, r3, 2
    srwi     r4, r3, 31
    add      r3, r3, r4
    stw      r3, 0x898(r30)
    stw      r0, 0x110(r1)
.L_800820D8:
    addis    r5, r30, 0x4
    lwz      r23, 0x804(r30)
    lfs      f1, 0x4600(r5)
    addi     r3, r30, 0x8a0
    lfs      f2, 0x4604(r5)
    addi     r4, r30, 0x4
    lwz      r5, 0x4608(r5)
    lwz      r6, 0x894(r30)
    lwz      r7, 0x890(r30)
    bl       CMsgFont_Init
    addis    r7, r30, 0x4
    lis      r0, 0x4330
    lfs      f4, 0x4614(r7)
    li       r4, 0x2
    lfs      f3, 0x4610(r7)
    mr       r3, r23
    lfs      f0, 0x460c(r7)
    lfs      f2, lbl_8065D058@sda21(r0)
    stfs     f0, 0x4524(r7)
    lfd      f1, lbl_8065D050@sda21(r0)
    stfs     f3, 0x4528(r7)
    stfs     f4, 0x452c(r7)
    stfs     f2, 0x45c4(r7)
    lbz      r6, 0x0(r30)
    stw      r0, 0x118(r1)
    subi     r5, r6, 0x23
    subfic   r0, r6, 0x23
    nor      r0, r5, r0
    srawi    r0, r0, 31
    and      r0, r4, r0
    xoris    r0, r0, 0x8000
    stw      r0, 0x11c(r1)
    lfd      f0, 0x118(r1)
    fsubs    f0, f0, f1
    stfs     f0, 0x45c8(r7)
    stfs     f2, 0x45cc(r7)
    lmw      r23, 0x12c(r1)
    lwz      r0, 0x154(r1)
    mtlr     r0
    addi     r1, r1, 0x150
    blr
*/

/**
 * @note Address: 0x8008217C
 * @note Size: 0x60
 */
extern "C" void CMsg_ResetPos(CMsg* msg)
{
	msg->_44600          = 320.0f;
	msg->_44604          = 344.0f;
	msg->_44608          = 0;
	Vec zero             = { 0.0f, 0.0f, 0.0f };
	msg->mFontPosition.x = zero.x;
	msg->mFontPosition.y = zero.y;
	msg->mFontPosition.z = zero.z;
}

/**
 * @note Address: 0x800821DC
 * @note Size: 0x32C
 */
extern "C" void ResetMessageState(CMsg* msg)
{
	// NONMATCHING: regalloc
	ResetMsgState(msg);
}

/*
    stwu     r1, -0x30(r1)
    lis      r6, __float_nan@ha
    addi     r11, r6, __float_nan@l
    lis      r5, msgColorTable@ha
    stmw     r26, 0x18(r1)
    li       r4, 0x0
    lis      r7, sMessageFontOrigin@ha
    addi     r10, r5, msgColorTable@l
    addi     r26, r7, sMessageFontOrigin@l
    lis      r8, 0xb60b
    li       r12, 0x1
    addis    r27, r3, 0x4
    lfs      f2, lbl_8065D06C@sda21(r0)
    li       r9, -0x1
    lfs      f1, lbl_8065D058@sda21(r0)
    li       r7, 0x32
    lfs      f0, lbl_8065D05C@sda21(r0)
    mr       r5, r4
    mr       r6, r3
    stw      r4, 0x804(r3)
    stw      r4, 0x898(r3)
    lbz      r0, 0x8a4(r3)
    rlwimi   r0, r4, 7, 24, 24
    stb      r0, 0x8a4(r3)
    lbz      r0, 0x8a4(r3)
    rlwimi   r0, r4, 6, 25, 25
    stb      r0, 0x8a4(r3)
    addi     r0, r8, 0x60b7
    lbz      r28, 0x8a4(r3)
    rlwimi   r28, r4, 5, 26, 26
    stb      r28, 0x8a4(r3)
    lbz      r8, 0x8a4(r3)
    rlwimi   r8, r4, 4, 27, 27
    stb      r8, 0x8a4(r3)
    lbz      r8, 0x8a4(r3)
    rlwimi   r8, r12, 3, 28, 28
    stb      r8, 0x8a4(r3)
    lbz      r8, 0x8a4(r3)
    rlwimi   r8, r12, 2, 29, 29
    stb      r8, 0x8a4(r3)
    stw      r4, 0x4518(r27)
    stw      r4, 0x451c(r27)
    stw      r4, 0x4520(r27)
    lwz      r8, 0x0(r26)
    lwz      r12, 0x4(r26)
    stw      r8, 0x8(r1)
    lwz      r8, 0x8(r26)
    stw      r12, 0xc(r1)
    lfs      f3, 0x8(r1)
    stw      r8, 0x10(r1)
    lfs      f4, 0xc(r1)
    stfs     f3, 0x4524(r27)
    lfs      f3, 0x10(r1)
    stfs     f4, 0x4528(r27)
    stfs     f3, 0x452c(r27)
    stfs     f2, 0x4570(r27)
    lfs      f2, 0x0(r11)
    stfs     f2, 0x4574(r27)
    lwz      r8, 0x0(r10)
    stw      r8, 0x4578(r27)
    stfs     f1, 0x457c(r27)
    stfs     f0, 0x4584(r27)
    stfs     f0, 0x4580(r27)
    stb      r4, 0x4588(r27)
    stw      r9, 0x458c(r27)
    stw      r9, 0x45ac(r27)
    stfs     f1, 0x45b0(r27)
    stfs     f0, 0x45b4(r27)
    stfs     f1, 0x45b8(r27)
    stw      r4, 0x450c(r27)
    stfs     f1, 0x4510(r27)
    stfs     f1, 0x4514(r27)
    mtctr    r7
.L_80082300:
    addi     r7, r4, 0x1
    addi     r12, r4, 0x2
    mulli    r26, r7, 0x4b
    addi     r11, r4, 0x3
    addi     r10, r4, 0x4
    addi     r9, r4, 0x5
    mulhw    r8, r0, r5
    add      r7, r8, r5
    srawi    r28, r7, 8
    addi     r8, r4, 0x6
    srwi     r29, r28, 31
    addi     r7, r4, 0x7
    mulli    r31, r12, 0x4b
    addi     r4, r4, 0x8
    add      r29, r28, r29
    mulhw    r27, r0, r26
    add      r28, r27, r26
    srawi    r28, r28, 8
    srwi     r27, r28, 31
    add      r27, r28, r27
    mulli    r28, r29, 0x168
    subf     r30, r28, r5
    addi     r5, r5, 0x258
    mulhw    r12, r0, r31
    sth      r30, 0x15f0(r6)
    sth      r30, 0x15ee(r6)
    mulli    r28, r11, 0x4b
    add      r12, r12, r31
    mulhw    r11, r0, r28
    srawi    r12, r12, 8
    srwi     r29, r12, 31
    add      r30, r12, r29
    mulli    r12, r27, 0x168
    add      r11, r11, r28
    mulli    r29, r10, 0x4b
    srawi    r11, r11, 8
    subf     r27, r12, r26
    srwi     r12, r11, 31
    sth      r27, 0x189c(r6)
    mulhw    r10, r0, r29
    add      r12, r11, r12
    sth      r27, 0x189a(r6)
    mulli    r11, r30, 0x168
    add      r10, r10, r29
    subf     r30, r11, r31
    srawi    r10, r10, 8
    mulli    r31, r9, 0x4b
    sth      r30, 0x1b48(r6)
    srwi     r11, r10, 31
    sth      r30, 0x1b46(r6)
    add      r30, r10, r11
    mulhw    r9, r0, r31
    mulli    r10, r12, 0x168
    add      r9, r9, r31
    mulli    r11, r8, 0x4b
    srawi    r9, r9, 8
    subf     r12, r10, r28
    sth      r12, 0x1df4(r6)
    srwi     r10, r9, 31
    mulhw    r8, r0, r11
    sth      r12, 0x1df2(r6)
    add      r12, r9, r10
    mulli    r9, r30, 0x168
    add      r8, r8, r11
    subf     r30, r9, r29
    srawi    r8, r8, 8
    mulli    r9, r7, 0x4b
    sth      r30, 0x20a0(r6)
    srwi     r10, r8, 31
    sth      r30, 0x209e(r6)
    mulhw    r7, r0, r9
    add      r8, r8, r10
    mulli    r10, r12, 0x168
    add      r7, r7, r9
    subf     r12, r10, r31
    srawi    r7, r7, 8
    mulli    r10, r8, 0x168
    sth      r12, 0x234c(r6)
    srwi     r8, r7, 31
    sth      r12, 0x234a(r6)
    subf     r10, r10, r11
    add      r7, r7, r8
    sth      r10, 0x25f8(r6)
    mulli    r7, r7, 0x168
    sth      r10, 0x25f6(r6)
    subf     r7, r7, r9
    sth      r7, 0x28a4(r6)
    sth      r7, 0x28a2(r6)
    addi     r6, r6, 0x1560
    bdnz     .L_80082300
    addis    r5, r3, 0x4
    li       r0, 0x1
    stw      r0, 0x45c0(r5)
    li       r7, 0x0
    lfs      f0, lbl_8065D058@sda21(r0)
    addis    r6, r7, 0x4
    addi     r6, r6, 0x45dc
    li       r4, -0x1
    stfs     f0, 0x45c8(r5)
    li       r0, 0x0
    add      r6, r3, r6
    li       r7, 0x3
    stfs     f0, 0x45c4(r5)
    stw      r4, 0x45d4(r5)
    stw      r4, 0x45d0(r5)
    stw      r0, 0x45d8(r5)
    stb      r0, 0x0(r6)
    stb      r0, 0x1(r6)
    stb      r0, 0x2(r6)
    addis    r6, r7, 0x4
    addi     r6, r6, 0x45dc
    li       r7, 0x6
    add      r6, r3, r6
    stb      r0, 0x0(r6)
    stb      r0, 0x1(r6)
    stb      r0, 0x2(r6)
    addis    r6, r7, 0x4
    addi     r6, r6, 0x45dc
    add      r6, r3, r6
    stb      r0, 0x0(r6)
    stb      r0, 0x1(r6)
    stb      r0, 0x2(r6)
    stw      r0, 0x45f0(r5)
    stw      r0, 0x45ec(r5)
    stw      r0, 0x45e8(r5)
    stw      r0, 0x45f8(r5)
    stw      r4, 0x45fc(r5)
    lmw      r26, 0x18(r1)
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: 0x80082508
 * @note Size: 0xA0
 */
extern "C" BOOL LoadMessageCursorObject(CMsg* msg)
{
	void* spawn = GetSpawnData(13);
	XObj* obj   = &msg->mFont.mCursor[0].mCursor;
	if (obj->LoadFromPlacement(CoordInfo(spawn))) {
		obj->SetScale(3.0f, 3.0f, 3.0f, 0);
		return TRUE;
	}
	return FALSE;
}

extern "C" {
GXColor gMessageWindowDefaultColor = { 0, 0, 0, 0x80 };
}

/**
 * @note Address: 0x800825A8
 * @note Size: 0x3C8
 */
extern "C" void InitMessageWindow(CMsg* msg)
{
	ResetMsgState(msg);
	CMsg_ResetPos(msg);
	GXColor color       = gMessageWindowDefaultColor;
	msg->mFont.mColor.r = color.r;
	msg->mFont.mColor.g = color.g;
	msg->mFont.mColor.b = color.b;
	msg->mFont.mColor.a = color.a;
	msg->mFont.mCursor[0].mCursor.InitializeObject(0);
}
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    mr       r6, r3
    lis      r5, __float_nan@ha
    stw      r0, 0x44(r1)
    addi     r11, r5, __float_nan@l
    lis      r4, msgColorTable@ha
    li       r3, 0x0
    stmw     r26, 0x28(r1)
    lis      r7, sMessageFontOrigin@ha
    addi     r26, r7, sMessageFontOrigin@l
    addi     r10, r4, msgColorTable@l
    lis      r8, 0xb60b
    li       r12, 0x1
    addis    r27, r6, 0x4
    lfs      f2, lbl_8065D06C@sda21(r0)
    lfs      f1, lbl_8065D058@sda21(r0)
    li       r9, -0x1
    lfs      f0, lbl_8065D05C@sda21(r0)
    li       r7, 0x32
    mr       r4, r3
    mr       r5, r6
    stw      r3, 0x804(r6)
    stw      r3, 0x898(r6)
    lbz      r0, 0x8a4(r6)
    rlwimi   r0, r3, 7, 24, 24
    stb      r0, 0x8a4(r6)
    lbz      r0, 0x8a4(r6)
    rlwimi   r0, r3, 6, 25, 25
    stb      r0, 0x8a4(r6)
    addi     r0, r8, 0x60b7
    lbz      r28, 0x8a4(r6)
    rlwimi   r28, r3, 5, 26, 26
    stb      r28, 0x8a4(r6)
    lbz      r8, 0x8a4(r6)
    rlwimi   r8, r3, 4, 27, 27
    stb      r8, 0x8a4(r6)
    lbz      r8, 0x8a4(r6)
    rlwimi   r8, r12, 3, 28, 28
    stb      r8, 0x8a4(r6)
    lbz      r8, 0x8a4(r6)
    rlwimi   r8, r12, 2, 29, 29
    stb      r8, 0x8a4(r6)
    stw      r3, 0x4518(r27)
    stw      r3, 0x451c(r27)
    stw      r3, 0x4520(r27)
    lwz      r8, 0x0(r26)
    lwz      r12, 0x4(r26)
    stw      r8, 0xc(r1)
    lwz      r8, 0x8(r26)
    stw      r12, 0x10(r1)
    lfs      f3, 0xc(r1)
    stw      r8, 0x14(r1)
    lfs      f4, 0x10(r1)
    stfs     f3, 0x4524(r27)
    lfs      f3, 0x14(r1)
    stfs     f4, 0x4528(r27)
    stfs     f3, 0x452c(r27)
    stfs     f2, 0x4570(r27)
    lfs      f2, 0x0(r11)
    stfs     f2, 0x4574(r27)
    lwz      r8, 0x0(r10)
    stw      r8, 0x4578(r27)
    stfs     f1, 0x457c(r27)
    stfs     f0, 0x4584(r27)
    stfs     f0, 0x4580(r27)
    stb      r3, 0x4588(r27)
    stw      r9, 0x458c(r27)
    stw      r9, 0x45ac(r27)
    stfs     f1, 0x45b0(r27)
    stfs     f0, 0x45b4(r27)
    stfs     f1, 0x45b8(r27)
    stw      r3, 0x450c(r27)
    stfs     f1, 0x4510(r27)
    stfs     f1, 0x4514(r27)
    mtctr    r7
.L_800826D8:
    addi     r7, r3, 0x1
    addi     r12, r3, 0x2
    mulli    r26, r7, 0x4b
    addi     r11, r3, 0x3
    addi     r10, r3, 0x4
    addi     r9, r3, 0x5
    mulhw    r8, r0, r4
    add      r7, r8, r4
    srawi    r28, r7, 8
    addi     r8, r3, 0x6
    srwi     r29, r28, 31
    addi     r7, r3, 0x7
    mulli    r31, r12, 0x4b
    addi     r3, r3, 0x8
    add      r29, r28, r29
    mulhw    r27, r0, r26
    add      r28, r27, r26
    srawi    r28, r28, 8
    srwi     r27, r28, 31
    add      r27, r28, r27
    mulli    r28, r29, 0x168
    subf     r30, r28, r4
    addi     r4, r4, 0x258
    mulhw    r12, r0, r31
    sth      r30, 0x15f0(r5)
    sth      r30, 0x15ee(r5)
    mulli    r28, r11, 0x4b
    add      r12, r12, r31
    mulhw    r11, r0, r28
    srawi    r12, r12, 8
    srwi     r29, r12, 31
    add      r30, r12, r29
    mulli    r12, r27, 0x168
    add      r11, r11, r28
    mulli    r29, r10, 0x4b
    srawi    r11, r11, 8
    subf     r27, r12, r26
    srwi     r12, r11, 31
    sth      r27, 0x189c(r5)
    mulhw    r10, r0, r29
    add      r12, r11, r12
    sth      r27, 0x189a(r5)
    mulli    r11, r30, 0x168
    add      r10, r10, r29
    subf     r30, r11, r31
    srawi    r10, r10, 8
    mulli    r31, r9, 0x4b
    sth      r30, 0x1b48(r5)
    srwi     r11, r10, 31
    sth      r30, 0x1b46(r5)
    add      r30, r10, r11
    mulhw    r9, r0, r31
    mulli    r10, r12, 0x168
    add      r9, r9, r31
    mulli    r11, r8, 0x4b
    srawi    r9, r9, 8
    subf     r12, r10, r28
    sth      r12, 0x1df4(r5)
    srwi     r10, r9, 31
    mulhw    r8, r0, r11
    sth      r12, 0x1df2(r5)
    add      r12, r9, r10
    mulli    r9, r30, 0x168
    add      r8, r8, r11
    subf     r30, r9, r29
    srawi    r8, r8, 8
    mulli    r9, r7, 0x4b
    sth      r30, 0x20a0(r5)
    srwi     r10, r8, 31
    sth      r30, 0x209e(r5)
    mulhw    r7, r0, r9
    add      r8, r8, r10
    mulli    r10, r12, 0x168
    add      r7, r7, r9
    subf     r12, r10, r31
    srawi    r7, r7, 8
    mulli    r10, r8, 0x168
    sth      r12, 0x234c(r5)
    srwi     r8, r7, 31
    sth      r12, 0x234a(r5)
    subf     r10, r10, r11
    add      r7, r7, r8
    sth      r10, 0x25f8(r5)
    mulli    r7, r7, 0x168
    sth      r10, 0x25f6(r5)
    subf     r7, r7, r9
    sth      r7, 0x28a4(r5)
    sth      r7, 0x28a2(r5)
    addi     r5, r5, 0x1560
    bdnz     .L_800826D8
    addis    r8, r6, 0x4
    li       r0, 0x1
    stw      r0, 0x45c0(r8)
    li       r5, 0x0
    lfs      f0, lbl_8065D058@sda21(r0)
    addis    r10, r5, 0x4
    addi     r10, r10, 0x45dc
    lis      r3, sMessageDefaultFontPosition@ha
    stfs     f0, 0x45c8(r8)
    addi     r7, r3, sMessageDefaultFontPosition@l
    li       r0, -0x1
    add      r10, r6, r10
    stfs     f0, 0x45c4(r8)
    li       r9, 0x3
    lfs      f1, lbl_8065D070@sda21(r0)
    addi     r3, r6, 0x9c0
    stw      r0, 0x45d4(r8)
    li       r4, 0x0
    lfs      f0, lbl_8065D074@sda21(r0)
    stw      r0, 0x45d0(r8)
    stw      r5, 0x45d8(r8)
    stb      r5, 0x0(r10)
    stb      r5, 0x1(r10)
    stb      r5, 0x2(r10)
    addis    r10, r9, 0x4
    addi     r10, r10, 0x45dc
    li       r9, 0x6
    add      r10, r6, r10
    stb      r5, 0x0(r10)
    stb      r5, 0x1(r10)
    stb      r5, 0x2(r10)
    addis    r10, r9, 0x4
    addi     r10, r10, 0x45dc
    add      r10, r6, r10
    stb      r5, 0x0(r10)
    stb      r5, 0x1(r10)
    stb      r5, 0x2(r10)
    stw      r5, 0x45f0(r8)
    stw      r5, 0x45ec(r8)
    stw      r5, 0x45e8(r8)
    stw      r5, 0x45f8(r8)
    stw      r0, 0x45fc(r8)
    stfs     f1, 0x4600(r8)
    stfs     f0, 0x4604(r8)
    stw      r5, 0x4608(r8)
    lwz      r0, 0x0(r7)
    lwz      r5, 0x4(r7)
    stw      r0, 0x18(r1)
    lwz      r0, 0x8(r7)
    stw      r5, 0x1c(r1)
    lfs      f0, 0x18(r1)
    stw      r0, 0x20(r1)
    lfs      f1, 0x1c(r1)
    stfs     f0, 0x460c(r8)
    lfs      f0, 0x20(r1)
    stfs     f1, 0x4610(r8)
    stfs     f0, 0x4614(r8)
    lwz      r0, gMessageWindowDefaultColor@sda21(r0)
    stw      r0, 0x8(r1)
    lbz      r5, 0x8(r1)
    lbz      r0, 0x9(r1)
    stb      r5, 0x9b8(r6)
    lbz      r5, 0xa(r1)
    stb      r0, 0x9b9(r6)
    lbz      r0, 0xb(r1)
    stb      r5, 0x9ba(r6)
    stb      r0, 0x9bb(r6)
    lwz      r12, 0x9f4(r6)
    lwz      r12, 0x90(r12)
    mtctr    r12
    bctrl
    lmw      r26, 0x28(r1)
    lwz      r0, 0x44(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

extern "C" {
void SetTranslate__5CJObjFfffl(CJObj* object, f32 x, f32 y, f32 z, s32 force);
void SetScale__5CJObjFfffl(CJObj* object, f32 x, f32 y, f32 z, s32 force);
void SetRotationY__5CJObjFfl(CJObj* object, f32 yaw, s32 force);
void SetTranslate__5CJObjFRC3Vecl(CJObj* object, const Vec& position, s32 force);
void SetScale__5CJObjFRC3Vecl(CJObj* object, const Vec& scale, s32 force);
#if defined(VERSION_JP) || defined(VERSION_PAL)
void InvalidateMessageDrawState(u8* game);
#endif
}

#if defined(VERSION_JP)
#define MSG_OBJECT_OFFSET(offset) ((offset) - 8)
#else
#define MSG_OBJECT_OFFSET(offset) (offset)
#endif

#if defined(VERSION_PAL)
static inline void MsgRestoreScissor(u32 x, u32 y, u32 width, u32 height)
{
	f32 displayScale = gGame.mGs.mScreen._40;
	u32 scaledHeight = (u32)(height * displayScale);
	u32 scaledY      = (u32)(y * displayScale);
	GXSetScissor(x, scaledY, width, scaledHeight);
}
#endif

// one piece of the window frame, a texture window and where it goes
struct MsgWindowSlice {
	f32 u;      // _00
	f32 v;      // _04
	f32 scaleU; // _08
	f32 scaleV; // _0C
	f32 x;      // _10
	f32 y;      // _14
	f32 width;  // _18
	f32 height; // _1C
};

/**
 * @note Address: 0x80082970
 * @note Size: 0x2308
 */
extern "C" void CMsgFont_Draw(void* p, f32 lastAlpha)
{
	CMsgFont* font = (CMsgFont*)p;
	XObj* anchor   = (XObj*)font->mPlacement;
	if (anchor != NULL && (anchor->mStatus.mFlags & 1) && (anchor->mStatus.mFlags & 0x10000)) {
		anchor->ProjectOffsetToScreen(&font->mPosition, &font->mWindowX, &font->mWindowY, NULL, 0);
		font->mWindowY -= font->mHeight;
	}
	if (font->mWindowX - font->mWidth * 0.5f < 16.0f)
		font->mWindowX = 16.0f + font->mWidth * 0.5f;
	if (font->mWindowX + font->mWidth * 0.5f > 624.0f)
		font->mWindowX = 624.0f - font->mWidth * 0.5f;
	if (font->mWindowY < 34.0f)
		font->mWindowY = 34.0f;
	if (font->mWindowY + font->mHeight > 414.0f)
		font->mWindowY = 414.0f - font->mHeight;
	f32 portraitScale;
	if (gGame.mWipeTile1.mActive != 0)
		portraitScale = 0.846153855f;
	else
		portraitScale = font->mHeight / 52.0f;
	if (portraitScale > 1.1f)
		portraitScale = 1.1f;
	if (portraitScale < 0.5f)
		portraitScale = 0.5f;
	f32 portraitWidth = (font->mMode >= 0 && font->mMode < 1) ? 40.0f * portraitScale : 0.0f;
	f32 textOffset    = portraitWidth;
	f32 expandX       = 4.0f * (f32)sin(4.712389f * (1.0f - font->mAlpha));
	f32 expandY       = 4.0f * (f32)sin(4.712389f * (1.0f - font->mAlpha));

	if (font->mAlpha > 0.0f) {
		SetGsLineWidth(gGame.mGs.mCurRenderMode, 12, 5);
		f32 halfPortrait = portraitWidth * 0.5f;
		f32 left         = ((font->mWindowX - font->mWidth * 0.5f) - 40.0f - expandX) - halfPortrait;
		f32 right        = halfPortrait + (expandX + (40.0f + (font->mWindowX + font->mWidth * 0.5f)));
		f32 top          = (font->mWindowY - 8.0f) - expandY;
		f32 bottom       = expandY + (8.0f + (font->mWindowY + font->mHeight));
		f32 windowAlpha  = font->mColor.a != 0 ? font->mAlpha : 0.0f;
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
		f32 outerRight  = 9.0f + (8.0f + right);
		f32 outerBottom = 9.0f + (8.0f + bottom);
		DrawGsVertexQuad(&gGame.mGs, left - 8.0f, top - 8.0f, -1.0f, 0xFF000000, outerRight, top - 8.0f, -1.0f, 0xFF000000, outerRight,
		                 outerBottom, -1.0f, 0xFF000000, left - 8.0f, outerBottom, -1.0f, 0xFF000000);

		f32 innerLeft = 1.0f + left, innerRight = right - 1.0f;
		f32 innerTop = 1.0f + top, innerBottom = bottom - 1.0f;
		f32 roundLeft = 8.0f + innerLeft, roundRight = innerRight - 8.0f;
		f32 roundTop = 8.0f + innerTop, roundBottom = innerBottom - 8.0f;
		Vec fan[10] = { (Vec) { font->mWindowX, font->mWindowY, 0.0f },
			            (Vec) { innerLeft, roundTop, 0.0f },
			            (Vec) { roundLeft, innerTop, 0.0f },
			            (Vec) { roundRight, innerTop, 0.0f },
			            (Vec) { innerRight, roundTop, 0.0f },
			            (Vec) { innerRight, roundBottom, 0.0f },
			            (Vec) { roundRight, innerBottom, 0.0f },
			            (Vec) { roundLeft, innerBottom, 0.0f },
			            (Vec) { innerLeft, roundBottom, 0.0f },
			            (Vec) { innerLeft, roundTop, 0.0f } };
		u32 color
		    = ((u32)font->mColor.r << 24) | ((u32)font->mColor.g << 16) | ((u32)font->mColor.b << 8) | (u32)(font->mColor.a * font->mAlpha);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
		DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, fan, color, 10);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 1, 1);
		DrawGsCircle(&gGame.mGs, 16, color, color, roundLeft, roundTop, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
		DrawGsCircle(&gGame.mGs, 16, color, color, roundRight, roundTop, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
		DrawGsCircle(&gGame.mGs, 16, color, color, roundLeft, roundBottom, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
		DrawGsCircle(&gGame.mGs, 16, color, color, roundRight, roundBottom, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
		color = (u32)(64.0f * windowAlpha);
		for (u32 i = 0; i < 10; i++) {
			fan[i].x += 9.0f;
			fan[i].y += 9.0f;
		}
		DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, fan, color, 10);
		f32 shadowLeft = 8.0f + (1.0f + (9.0f + left));
		f32 shadowTop  = 8.0f + (1.0f + (9.0f + top));
		DrawGsCircle(&gGame.mGs, 16, color, color, shadowLeft, shadowTop, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
		f32 shadowRight = ((9.0f + right) - 1.0f) - 8.0f;
		DrawGsCircle(&gGame.mGs, 16, color, color, shadowRight, shadowTop, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
		f32 shadowBottom = ((9.0f + bottom) - 1.0f) - 8.0f;
		DrawGsCircle(&gGame.mGs, 16, color, color, shadowLeft, shadowBottom, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
		DrawGsCircle(&gGame.mGs, 16, color, color, shadowRight, shadowBottom, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);

		TPLPalette* palette    = (TPLPalette*)(&gGame.mTpl)->mBuffer;
		CTexObj* sharedTexture = &(&gGame.mTpl)->mTex;
#if defined(VERSION_JP)
		CTexObj border = sharedTexture->Init(&palette->descriptorArray[112], 0);
#else
		CTexObj border = sharedTexture->Init(&palette->descriptorArray[114], 0);
#endif
		f32 textureWidth  = GXGetTexObjWidth(&border);
		f32 textureHeight = GXGetTexObjHeight(&border);
		f32 stretchX      = (right - left) - 128.0f;
		if (stretchX < 0.0f)
			stretchX = 0.0f;
		f32 stretchY = (bottom - top) - 64.0f;
		if (stretchY < 0.0f)
			stretchY = 0.0f;
		MsgWindowSlice slices[8]
		    = { { 0, 0, 0.5f, 0.5f, 0, 0, 64, 32 },   { 0.5f, 0, 0, 0.5f, 0, 0, 0, 32 },       { 0.5f, 0, 0.5f, 0.5f, 0, 0, 64, 32 },
			    { 0, 0.5f, 0.5f, 0, 0, 0, 64, 0 },    { 0.5f, 0.5f, 0.5f, 0, 0, 0, 64, 0 },    { 0, 0.5f, 0.5f, 0.5f, 0, 0, 64, 32 },
			    { 0.5f, 0.5f, 0, 0.5f, 0, 0, 0, 32 }, { 0.5f, 0.5f, 0.5f, 0.5f, 0, 0, 64, 32 } };
		f32 edgeLeft = (32.0f + left) - 3.0f, edgeRight = 3.0f + (right - 32.0f);
		f32 edgeTop = 8.0f + ((16.0f + top) - 10.0f), edgeBottom = 10.0f + (bottom - 16.0f);
		slices[1].scaleU = 1.0f / textureWidth;
		slices[0].x      = edgeLeft;
		slices[0].y      = edgeTop;
		slices[1].x      = font->mWindowX;
		slices[1].y      = edgeTop;
		slices[1].width  = 6.0f + stretchX;
		slices[2].x      = edgeRight;
		slices[2].y      = edgeTop;
		slices[3].scaleV = 1.0f / textureHeight;
		slices[3].x      = edgeLeft;
		slices[3].y      = 4.0f + (font->mHeight * 0.5f + font->mWindowY);
		slices[3].height = (20.0f + stretchY) - 8.0f;
		slices[4].scaleV = 1.0f / textureHeight;
		slices[4].x      = edgeRight;
		slices[4].y      = 4.0f + (font->mHeight * 0.5f + font->mWindowY);
		slices[4].height = (20.0f + stretchY) - 8.0f;
		slices[5].x      = edgeLeft;
		slices[5].y      = edgeBottom;
		slices[6].scaleU = 1.0f / textureWidth;
		slices[6].x      = font->mWindowX;
		slices[6].y      = edgeBottom;
		slices[6].width  = 6.0f + stretchX;
		slices[7].x      = edgeRight;
		slices[7].y      = edgeBottom;
		color            = 0xFFFFFF00 | (u32)(255.0f * windowAlpha);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 1, 1);
		for (u32 i = 0; i < 8; i++) {
			PSMTXTrans(border.mMtx[0], slices[i].u, slices[i].v, 0.0f);
			border.mMtxDirty = TRUE;
			PSMTXScale(border.mMtx[1], slices[i].scaleU, slices[i].scaleV, 1.0f);
			border.mMtxDirty = TRUE;
			DrawGsTexturedQuad(&gGame.mGs, &border, color, color, color, color, slices[i].x, slices[i].y, 1.0f, slices[i].width,
			                   slices[i].height, 0.0f, 0.0f, 0.0f);
		}
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
		DrawGsVertexQuad(&gGame.mGs, left - 8.0f, top - 8.0f, -1.0f, 0xFF000000, outerRight, top - 8.0f, -1.0f, 0xFF000000, outerRight,
		                 outerBottom, -1.0f, 0xFF000000, left - 8.0f, outerBottom, -1.0f, 0xFF000000);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);

		if (font->mMode >= 0 && font->mMode < 1) {
			if (font->mColor.a != 0) {
#if defined(VERSION_PAL)
				f32 displayScale = gGame.mGs.mScreen._40;
				u32 clipHeight   = (u32)(displayScale * (u32)(2.0f * ((8.0f + expandY) - 1.0f) + font->mHeight));
#else
				u32 clipHeight = (u32)(2.0f * ((8.0f + expandY) - 1.0f) + font->mHeight);
#endif
				u32 clipWidth = (u32)(2.0f * ((40.0f + expandX) - 1.0f) + font->mWidth);
#if defined(VERSION_PAL)
				u32 clipY = (u32)(displayScale * (u32)(1.0f + ((font->mWindowY - 8.0f) - expandY)));
#else
				u32 clipY = (u32)(1.0f + ((font->mWindowY - 8.0f) - expandY));
#endif
				u32 clipX = (u32)(1.0f + (((font->mWindowX - font->mWidth * 0.5f) - 40.0f - expandX) - halfPortrait));
				GXSetScissor(clipX, clipY, clipWidth, clipHeight);
			}
			f32 portraitX   = 44.0f * portraitScale + ((font->mWindowX - font->mWidth * 0.5f) - 40.0f - halfPortrait);
			f32 portraitY   = 12.0f * portraitScale + (font->mHeight * 0.5f + font->mWindowY);
			XObj* portrait  = NULL;
			s32 objectCount = gGame.mXObjs.mPools[10].mUsed;
			s32 stride      = gGame.mXObjs.mPools[10].mStride;
			u8* objects     = gGame.mXObjs.mPools[10].mObjects;
			for (s32 i = 0; i < objectCount; i++) {
				XObj* obj = (XObj*)(objects + i * stride);
				if (obj->mStatus.mFlags & 1) {
					UnkObjData* kind = *(UnkObjData**)((u8*)obj + MSG_OBJECT_OFFSET(0x240));
					if ((s16)(kind != NULL ? kind->GetItem() : -1) == 10)
						portrait = obj;
				}
			}
			if (portrait != NULL) {
#if !defined(VERSION_JP)
				if (portrait->mJObj->flags & USER_DEF_MTX)
					textOffset = 0.0f;
				else {
#endif
					f32 scale       = (9.0f * portraitScale) * font->mAlpha;
					Vec oldPosition = (Vec)portrait->mMoveTranslate;
					Vec oldScale    = (Vec)portrait->mMoveScale;
					f32 oldYaw      = portrait->mMoveRotate.y;
					CJObj joint(portrait->mJObj);
					f32 depth = 80.0f * portraitScale;
					SetTranslate__5CJObjFfffl(&joint, portraitX, portraitY, depth, 1);
					SetScale__5CJObjFfffl(&joint, scale, -scale, scale, 1);
					SetRotationY__5CJObjFfl(&joint, 0.0f, 1);
					HSD_JObjSetupMatrix(joint.mJObj);
					s32 path[6] = { 0, 3, 0, 0, 0, 0 };
					f32 height  = CJObj_GetJointWorldPositionByPath((CJObj*)portrait, 6, path).y;
					SetTranslate__5CJObjFfffl(&joint, portraitX, portraitY + ((portraitY - height) - 12.0f), depth, 1);
					SetScale__5CJObjFfffl(&joint, scale, -scale, scale, 1);
					SetRotationY__5CJObjFfl(&joint, 0.0f, 1);
					HSD_JObjSetFlags(portrait->mJObj, 8);
					u32 oldColor                                      = *(u32*)((u8*)portrait + MSG_OBJECT_OFFSET(0x168));
					s32 oldMode                                       = *(s32*)((u8*)portrait + MSG_OBJECT_OFFSET(0x164));
					*(u32*)((u8*)portrait + MSG_OBJECT_OFFSET(0x168)) = 0xFFFFFF00;
					*(s32*)((u8*)portrait + MSG_OBJECT_OFFSET(0x164)) = 0;
					portrait->vfAC();
#if defined(VERSION_US)
					joint.CJObj::SetTranslate(oldPosition, 1);
					joint.CJObj::SetScale(oldScale, 1);
#else
				SetTranslate__5CJObjFRC3Vecl(&joint, oldPosition, 1);
				SetScale__5CJObjFRC3Vecl(&joint, oldScale, 1);
#endif
					SetRotationY__5CJObjFfl(&joint, oldYaw, 1);
					HSD_JObjClearFlags(portrait->mJObj, 8);
					*(u32*)((u8*)portrait + MSG_OBJECT_OFFSET(0x168)) = oldColor;
					*(s32*)((u8*)portrait + MSG_OBJECT_OFFSET(0x164)) = oldMode;
#if !defined(VERSION_JP)
				}
#endif
			} else {
				font->mCursor[0].mCursor.SetPos(portraitX, portraitY, 80.0f * portraitScale, 0);
				font->mCursor[0].mCursor.SetScale((9.0f * portraitScale) * font->mAlpha, 0);
				font->mCursor[0].mCursor.vfAC();
			}
#if defined(VERSION_JP) || defined(VERSION_PAL)
			InvalidateMessageDrawState((u8*)&gGame);
#else
			HSD_StateInvalidate(-1);
			if (gGame.mGs.mCurRenderMode != NULL)
				gGame.mGs.mCurRenderMode->Invalidate();
			if (gGame.mGs.mCurTevStage != NULL)
				gGame.mGs.mCurTevStage->Invalidate();
			if (gGame.mGs.mCurChannel != NULL && !gGame.mGs.mCurChannel->mDirty)
				gGame.mGs.mCurChannel->mDirty = TRUE;
#endif
			if (font->mColor.a != 0) {
#if defined(VERSION_PAL)
				MsgRestoreScissor(0, 0, 640, 448);
#else
				GXSetScissor(0, 0, 640, 448);
#endif
			}
		}
		if (font->mSelectedLine >= 0) {
			f32 x      = textOffset * 0.5f + font->mWindowX;
			f32 y      = font->mCurLineY + (8.0f + font->mWindowY);
			f32 width  = 24.0f + font->mCurLineWidth;
			f32 height = font->mData->mHeights[font->mSelectedLine];
			u32 color  = 0xC87F1600 | (u32)(200.0f * font->mHighlightAlpha);
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			CTexObj* highlight = &font->mTexs.mTex[MSG_TEX_ARROW];
			PSMTXScale(highlight->mMtx[1], 0.25f, 1.0f, 1.0f);
			highlight->mMtxDirty = TRUE;
			PSMTXTrans(highlight->mMtx[0], 0.25f, 0.0f, 0.0f);
			highlight->mMtxDirty = TRUE;
			y                    = 2.0f + y;
			DrawGsTexturedQuad(&gGame.mGs, highlight, color, color, color, color, x, y, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
			PSMTXTrans(highlight->mMtx[0], 0.0f, 0.0f, 0.0f);
			highlight->mMtxDirty = TRUE;
			f32 halfWidth        = width * 0.5f;
			DrawGsTexturedQuad(&gGame.mGs, highlight, color, color, color, color, (x - halfWidth) - 4.0f, y, 0.0f, 8.0f, height, 0.0f, 0.0f,
			                   0.0f);
			PSMTXTrans(highlight->mMtx[0], 0.5f, 0.0f, 0.0f);
			highlight->mMtxDirty = TRUE;
			DrawGsTexturedQuad(&gGame.mGs, highlight, color, color, color, color, 4.0f + (x + halfWidth), y, 0.0f, 8.0f, height, 0.0f, 0.0f,
			                   0.0f);
			CTexObj* line = &font->mTexs.mTex[MSG_TEX_LINE];
			f32 lineWidth = GXGetTexObjWidth(line);
			PSMTXScale(line->mMtx[1], 0.5f * (width / lineWidth), 0.5f, 1.0f);
			line->mMtxDirty = TRUE;
			color           = 0xFFFFFF00 | (u32)(40.0f * font->mHighlightAlpha);
			DrawGsTexturedQuad(&gGame.mGs, line, color, color, color, color, 2.0f + x, y, 0.0f, width, height - 4.0f, 0.0f, 0.0f, 0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		}
		if (font->mOtherSelection >= 0) {
			s32 line    = font->mGlyphs[font->mCharCount - 1].mLine;
			f32 center  = textOffset * 0.5f + font->mWindowX;
			f32 x       = ((font->mDigitCount * font->mSize) * 0.5f + center) - (font->mOtherSelection + 1) * font->mSize;
			f32 y       = 3.0f + ((font->mLineY[line] + (8.0f + font->mWindowY)) - font->mData->mHeights[line] * 0.5f);
			Vec quad[4] = { (Vec) { x, y, 0.0f }, (Vec) { x + font->mSize, y, 0.0f }, (Vec) { x + font->mSize, y + font->mSize, 0.0f },
				            (Vec) { x, y + font->mSize, 0.0f } };
			u32 color   = 0xFF00 | (u32)(200.0f * font->mHighlightAlpha);
			DrawGsVertexQuad(&gGame.mGs, quad[0].x, quad[0].y, quad[0].z, color, quad[1].x, quad[1].y, quad[1].z, color, quad[2].x,
			                 quad[2].y, quad[2].z, color, quad[3].x, quad[3].y, quad[3].z, color);
			for (s32 i = 0; i < font->mDigitCount; i++) {
				f32 digitX = ((font->mDigitCount * font->mSize) * 0.5f + center) - (i + 1) * font->mSize;
				u32 color;
				if (i < font->mFirstEditDigit)
					color = 0xFFFFFF00 | (u32)(255.0f * font->mTextAlpha);
				else
					color = 0x64FF6400 | (u32)(255.0f * font->mTextAlpha);
				char text[4];
#if defined(VERSION_JP)
				ConvertHalfWidthToFullWidthSJIS(FormatToStaticBuffer("%d", font->mDigits[i]), text);
#else
				sprintf(text, "%d", font->mDigits[i]);
#endif
				f32 width = MeasureGameTextWidth(&gGame.mGs, text, TRUE, font->mSize);
				digitX += (font->mSize - width) * 0.5f;
				DrawGsProportionalTextFormat(&gGame.mGs, color, text, digitX, y, 0.0f, font->mSize, font->mSize);
			}
		}
		SetGsLineWidth(gGame.mGs.mCurRenderMode, 12, 5);
	}

	if (font->mTextAlpha > 0.0f) {
		SetGsTevOrder(gGame.mGs.mCurTevStage, 0, 0, 4);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		gGame.mGs.mCurTevStage->SetNumTexGens(1);
		gGame.mGs.mCurTevStage->SetTexGen(0, 1, 4, 30);
		if (gGame.mGs.mCurRenderMode != NULL)
			ApplyGsRenderMode(gGame.mGs.mCurRenderMode, FALSE);
		if (gGame.mGs.mCurTevStage != NULL)
			ApplyGsTevStage(gGame.mGs.mCurTevStage, FALSE);
		if (gGame.mGs.mCurChannel != NULL)
			ApplyGsChannel(gGame.mGs.mCurChannel, FALSE);
		f32 halfOffset = textOffset * 0.5f;
		f32 alpha      = font->mTextAlpha;
		f32 last       = alpha * lastAlpha;
		Mtx translation, view;
		PSMTXTrans(translation, 2.0f + (font->mWindowX + halfOffset), 1.0f + font->mWindowY, 0.0f);
		PSMTXConcat(gGame.mGs.mCamera->mViewMtx, translation, view);
		for (s32 i = 0; i < font->mCharCount; i++) {
			MsgFontGlyph& glyph = font->mGlyphs[i];
			if (glyph.mLine == font->mSelectedLine) {
				u32 color = (u32)(150.0f * (i == font->mCharCount - 1 ? last : alpha));
				glyph.mSprite.SetBillboard(0);
				glyph.mSprite.Draw(view, GX_TEXMAP0, 30, GX_MTX2x4, color, color, color, color);
			}
		}
		PSMTXTrans(translation, font->mWindowX + halfOffset, font->mWindowY, 0.0f);
		PSMTXConcat(gGame.mGs.mCamera->mViewMtx, translation, view);
		for (s32 i = 0; i < font->mCharCount; i++) {
			MsgFontGlyph& glyph = font->mGlyphs[i];
			f32 opacity         = i == font->mCharCount - 1 ? last : alpha;
			u32 color           = (glyph.mColor & 0xFFFFFF00) | (u8)((u8)glyph.mColor * opacity);
			glyph.mSprite.SetBillboard(0);
			switch (glyph.mEffect & 0xF0) {
			case 0x10:
			case 0x20: {
				f32 angle       = glyph.mAngle;
				f32 a           = (glyph.mEffect & 1) ? glyph.mAmount * (f32)sin(DEG2RAD * glyph.mPhaseY) : 0.0f;
				f32 b           = (glyph.mEffect & 2) ? glyph.mAmount * (f32)sin(DEG2RAD * glyph.mPhaseX) : 0.0f;
				f32 quarterTurn = HALF_PI + angle;
				f32 sideSin     = sin(quarterTurn);
				f32 forwardSin  = sin(angle);
				f32 dx          = a * forwardSin + b * sideSin;
				f32 sideCos     = cos(quarterTurn);
				f32 forwardCos  = cos(angle);
				f32 dy          = a * forwardCos + b * sideCos;
				f32 scaleX = glyph.mScaleX, scaleY = glyph.mScaleY;
				if (glyph.mEffect & 4) {
					f32 wave  = sin(DEG2RAD * glyph.mPhaseW);
					f32 scale = 0.1f * wave + 1.0f;
					glyph.mSprite.SetScale(scaleX * scale, scaleY * scale, 1.0f);
				}
				if (glyph.mEffect & 8) {
					f32 wave = 0.52359879f * (f32)sin(DEG2RAD * glyph.mPhaseZ);
					glyph.mSprite.SetRot(0.0f, 0.0f, wave + (angle - wave * 0.5f));
				}
				Mtx offset, animatedView;
				PSMTXTrans(offset, dx, dy, 0.0f);
				PSMTXConcat(view, offset, animatedView);
				glyph.mSprite.Draw(animatedView, GX_TEXMAP0, 30, GX_MTX2x4, color, color, color, color);
				break;
			}
			default:
#if !defined(VERSION_JP)
				if (glyph.mAngle > 0.0f) {
					f32 wave  = sin(glyph.mAngle);
					f32 width = glyph.mWidthAdjust;
					Mtx offset, rotatedView;
					PSMTXTrans(offset, width, -width * wave, 0.0f);
					PSMTXConcat(view, offset, rotatedView);
					glyph.mSprite.Draw(rotatedView, GX_TEXMAP0, 30, GX_MTX2x4, color, color, color, color);
				} else
#endif
					glyph.mSprite.Draw(view, GX_TEXMAP0, 30, GX_MTX2x4, color, color, color, color);
				break;
			}
		}
		SetGsTevOrder(gGame.mGs.mCurTevStage, 255, 255, 4);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		gGame.mGs.mCurTevStage->SetNumTexGens(0);
	}
	if (font->mBlink && (s32)font->_43C6C >= 0) {
		SetGsTevOrder(gGame.mGs.mCurTevStage, 0, 0, 4);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		gGame.mGs.mCurTevStage->SetNumTexGens(1);
		gGame.mGs.mCurTevStage->SetTexGen(0, 1, 4, 30);
		s32 index = font->_43C6C;
		if (index >= 2)
			index = 2;
		CTexObj* cursor = &font->mTexs.mTex[MSG_TEX_CURSOR] + index;
		f32 width       = GXGetTexObjWidth(cursor);
		f32 height      = GXGetTexObjHeight(cursor);
		if ((s32)font->_43C6C >= 2) {
			width = (s32)GXGetTexObjWidth(cursor) / 11;
			PSMTXScale(cursor->mMtx[1], 1.0f / 11.0f, 1.0f, 1.0f);
			cursor->mMtxDirty = TRUE;
			PSMTXTrans(cursor->mMtx[0], (1.0f / 11.0f) * ((s32)font->_43C6C - 2), 0.0f, 0.0f);
			cursor->mMtxDirty = TRUE;
		}
		s32 line = font->mGlyphs[font->mCharCount - 1].mLine;
		f32 x    = width * 0.5f + (font->mCursorX + (textOffset * 0.5f + font->mWindowX));
		f32 y    = (font->mData->mHeights[line] * 0.5f + (font->mLineY[line] + (8.0f + font->mWindowY))) - height * 0.25f;
		f32 wave = sin(font->mBlinkPhase);
		if (wave < 0.0f)
			wave = 0.0f;
		u32 shadow = (u32)(200.0f * wave);
		u32 color  = 0xFFFFFF00 | shadow;
		DrawGsTexturedQuad(&gGame.mGs, cursor, shadow, shadow, shadow, shadow, 2.0f + x, 2.0f + y, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
		DrawGsTexturedQuad(&gGame.mGs, cursor, color, color, color, color, x, y, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
		SetGsTevOrder(gGame.mGs.mCurTevStage, 255, 255, 4);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		gGame.mGs.mCurTevStage->SetNumTexGens(0);
	}
}
/*
    stwu     r1, -0x660(r1)
    mflr     r0
    stw      r0, 0x664(r1)
    stfd     f31, 0x650(r1)
    psq_st   f31, 0x658(r1), 0, qr0
    stfd     f30, 0x640(r1)
    psq_st   f30, 0x648(r1), 0, qr0
    stfd     f29, 0x630(r1)
    psq_st   f29, 0x638(r1), 0, qr0
    stfd     f28, 0x620(r1)
    psq_st   f28, 0x628(r1), 0, qr0
    stfd     f27, 0x610(r1)
    psq_st   f27, 0x618(r1), 0, qr0
    stfd     f26, 0x600(r1)
    psq_st   f26, 0x608(r1), 0, qr0
    stfd     f25, 0x5f0(r1)
    psq_st   f25, 0x5f8(r1), 0, qr0
    stfd     f24, 0x5e0(r1)
    psq_st   f24, 0x5e8(r1), 0, qr0
    stfd     f23, 0x5d0(r1)
    psq_st   f23, 0x5d8(r1), 0, qr0
    stfd     f22, 0x5c0(r1)
    psq_st   f22, 0x5c8(r1), 0, qr0
    stfd     f21, 0x5b0(r1)
    psq_st   f21, 0x5b8(r1), 0, qr0
    stfd     f20, 0x5a0(r1)
    psq_st   f20, 0x5a8(r1), 0, qr0
    stfd     f19, 0x590(r1)
    psq_st   f19, 0x598(r1), 0, qr0
    stfd     f18, 0x580(r1)
    psq_st   f18, 0x588(r1), 0, qr0
    stfd     f17, 0x570(r1)
    psq_st   f17, 0x578(r1), 0, qr0
    stfd     f16, 0x560(r1)
    psq_st   f16, 0x568(r1), 0, qr0
    stfd     f15, 0x550(r1)
    psq_st   f15, 0x558(r1), 0, qr0
    stmw     r21, 0x524(r1)
    mr       r30, r3
    lis      r5, lbl_80208E88@ha
    addis    r4, r30, 0x4
    fmr      f31, f1
    lwz      r3, 0x3c80(r4)
    addi     r31, r5, lbl_80208E88@l
    cmplwi   r3, 0x0
    beq      .L_80082AA8
    lwz      r5, 0x228(r3)
    li       r7, 0x0
    lwz      r8, 0x22c(r3)
    li       r0, 0x1
    and      r6, r5, r7
    and      r0, r8, r0
    xor      r5, r0, r7
    xor      r0, r6, r7
    or.      r0, r5, r0
    beq      .L_80082AA8
    lis      r5, 0x1
    xor      r0, r6, r7
    and      r5, r8, r5
    xor      r5, r5, r7
    or.      r0, r5, r0
    beq      .L_80082AA8
    lwz      r12, 0x34(r3)
    mr       r5, r4
    mr       r6, r4
    li       r7, 0x0
    lwz      r12, 0x10c(r12)
    li       r8, 0x0
    mtctr    r12
    addi     r4, r4, 0x3c84
    addi     r5, r5, 0x3c94
    addi     r6, r6, 0x3ca0
    bctrl
    addis    r3, r30, 0x4
    lfs      f1, 0x3ca0(r3)
    lfs      f0, 0x3ca4(r3)
    fsubs    f0, f1, f0
    stfs     f0, 0x3ca0(r3)
.L_80082AA8:
    addis    r29, r30, 0x4
    lfs      f0, lbl_8065D068@sda21(r0)
    lfs      f1, 0x3c98(r29)
    lfs      f2, 0x3c94(r29)
    fmuls    f3, f1, f0
    lfs      f0, lbl_8065D094@sda21(r0)
    fsubs    f1, f2, f3
    fcmpo    cr0, f1, f0
    bge      .L_80082AD4
    fadds    f0, f0, f3
    stfs     f0, 0x3c94(r29)
.L_80082AD4:
    lfs      f1, 0x3c98(r29)
    lfs      f0, lbl_8065D068@sda21(r0)
    lfs      f2, 0x3c94(r29)
    fmuls    f3, f1, f0
    lfs      f0, lbl_8065D098@sda21(r0)
    fadds    f1, f2, f3
    fcmpo    cr0, f1, f0
    ble      .L_80082AFC
    fsubs    f0, f0, f3
    stfs     f0, 0x3c94(r29)
.L_80082AFC:
    lfs      f1, 0x3ca0(r29)
    lfs      f0, lbl_8065D09C@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_80082B10
    stfs     f0, 0x3ca0(r29)
.L_80082B10:
    lfs      f1, 0x3ca0(r29)
    lfs      f2, 0x3ca4(r29)
    lfs      f0, lbl_8065D0A0@sda21(r0)
    fadds    f1, f1, f2
    fcmpo    cr0, f1, f0
    ble      .L_80082B30
    fsubs    f0, f0, f2
    stfs     f0, 0x3ca0(r29)
.L_80082B30:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    lwz      r0, 0x4168(r3)
    cmpwi    r0, 0x0
    beq      .L_80082B4C
    lfs      f26, lbl_8065D0A4@sda21(r0)
    b        .L_80082B58
.L_80082B4C:
    lfs      f1, 0x3ca4(r29)
    lfs      f0, lbl_8065D0A8@sda21(r0)
    fdivs    f26, f1, f0
.L_80082B58:
    lfs      f0, lbl_8065D0AC@sda21(r0)
    fcmpo    cr0, f26, f0
    ble      .L_80082B68
    fmr      f26, f0
.L_80082B68:
    lfs      f0, lbl_8065D068@sda21(r0)
    fcmpo    cr0, f26, f0
    bge      .L_80082B78
    fmr      f26, f0
.L_80082B78:
    lwz      r3, 0xaa8(r30)
    li       r0, 0x0
    cmpwi    r3, 0x0
    blt      .L_80082B94
    cmpwi    r3, 0x1
    bge      .L_80082B94
    li       r0, 0x1
.L_80082B94:
    clrlwi.  r0, r0, 24
    beq      .L_80082BA8
    lfs      f0, lbl_8065D0B0@sda21(r0)
    fmuls    f19, f0, f26
    b        .L_80082BAC
.L_80082BA8:
    lfs      f19, lbl_8065D058@sda21(r0)
.L_80082BAC:
    addis    r3, r30, 0x4
    lfs      f1, lbl_8065D05C@sda21(r0)
    lfs      f0, 0x3d10(r3)
    fmr      f25, f19
    lfs      f2, lbl_8065D0B4@sda21(r0)
    fsubs    f0, f1, f0
    fmuls    f1, f2, f0
    bl       sin
    addis    r3, r30, 0x4
    lfs      f2, lbl_8065D05C@sda21(r0)
    lfs      f0, 0x3d10(r3)
    frsp     f4, f1
    lfs      f3, lbl_8065D0B8@sda21(r0)
    fsubs    f0, f2, f0
    lfs      f1, lbl_8065D0B4@sda21(r0)
    fmuls    f24, f3, f4
    fmuls    f1, f1, f0
    bl       sin
    addis    r3, r30, 0x4
    frsp     f3, f1
    lfs      f2, lbl_8065D0B8@sda21(r0)
    lfs      f1, 0x3d10(r3)
    lfs      f0, lbl_8065D058@sda21(r0)
    fmuls    f23, f2, f3
    fcmpo    cr0, f1, f0
    ble      .L_80084378
    lis      r3, gGame@ha
    li       r4, 0xc
    addi     r28, r3, gGame@l
    li       r5, 0x5
    lwz      r3, 0x290(r28)
    bl       SetGsLineWidth
    lfs      f0, lbl_8065D068@sda21(r0)
    lfs      f1, 0x3c98(r29)
    lbz      r0, 0x11b(r30)
    fmuls    f29, f19, f0
    fmuls    f0, f1, f0
    lfs      f1, 0x3c94(r29)
    lfs      f4, lbl_8065D0B0@sda21(r0)
    cmplwi   r0, 0x0
    lfs      f5, 0x3ca0(r29)
    fsubs    f3, f1, f0
    fadds    f1, f1, f0
    lfs      f0, 0x3ca4(r29)
    lfs      f2, lbl_8065D0BC@sda21(r0)
    fsubs    f3, f3, f4
    fadds    f1, f4, f1
    fadds    f0, f5, f0
    fsubs    f4, f3, f24
    fadds    f3, f24, f1
    fsubs    f1, f5, f2
    fadds    f0, f2, f0
    fsubs    f22, f4, f29
    fadds    f21, f29, f3
    fsubs    f20, f1, f23
    fadds    f19, f23, f0
    beq      .L_80082C9C
    addis    r3, r30, 0x4
    lfs      f30, 0x3d10(r3)
    b        .L_80082CA0
.L_80082C9C:
    lfs      f30, lbl_8065D058@sda21(r0)
.L_80082CA0:
    lwz      r3, 0x290(r28)
    li       r4, 0x1
    li       r5, 0x7
    li       r6, 0x1
    bl       SetGsDepthMode
    lfs      f4, lbl_8065D0BC@sda21(r0)
    lis      r3, gGame@ha
    lfs      f3, lbl_8065D0C4@sda21(r0)
    addi     r3, r3, gGame@l
    fadds    f2, f4, f19
    lfs      f5, lbl_8065D0C0@sda21(r0)
    fadds    f0, f4, f21
    stfs     f3, 0x8(r1)
    fsubs    f1, f22, f4
    lis      r4, 0xff00
    fadds    f28, f5, f2
    lis      r5, 0xff00
    fadds    f27, f5, f0
    stfs     f1, 0xc(r1)
    fsubs    f2, f20, f4
    lis      r6, 0xff00
    stfs     f28, 0x10(r1)
    fmr      f4, f27
    fmr      f5, f2
    lis      r7, 0xff00
    fmr      f6, f3
    stfs     f3, 0x14(r1)
    fmr      f7, f27
    fmr      f8, f28
    bl       DrawGsVertexQuad
    li       r0, 0xf
    addi     r5, r1, 0x288
    addi     r4, r31, 0xc8
    mtctr    r0
.L_80082D28:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_80082D28
    lfs      f0, lbl_8065D05C@sda21(r0)
    lwz      r21, 0x54(r31)
    fadds    f4, f0, f22
    lfs      f2, lbl_8065D0BC@sda21(r0)
    fadds    f5, f0, f20
    lwz      r12, 0x58(r31)
    fsubs    f1, f21, f0
    lwz      r0, 0x60(r31)
    fsubs    f0, f19, f0
    lwz      r11, 0x64(r31)
    lwz      r10, 0x6c(r31)
    fadds    f18, f2, f5
    lwz      r9, 0x70(r31)
    fadds    f16, f2, f4
    lwz      r8, 0x78(r31)
    fsubs    f17, f1, f2
    lwz      r5, 0x7c(r31)
    fsubs    f15, f0, f2
    lwz      r7, 0x84(r31)
    lwz      r6, 0x88(r31)
    lwz      r4, 0x90(r31)
    lwz      r3, 0x94(r31)
    stw      r0, 0xbc(r1)
    lwz      r0, 0x5c(r31)
    stw      r11, 0xc0(r1)
    lwz      r26, 0x68(r31)
    stw      r10, 0xb0(r1)
    lwz      r23, 0x74(r31)
    stw      r9, 0xb4(r1)
    lwz      r11, 0x80(r31)
    stw      r8, 0xa4(r1)
    lwz      r8, 0x8c(r31)
    stw      r5, 0xa8(r1)
    lwz      r5, 0x98(r31)
    stw      r7, 0x98(r1)
    lfs      f3, 0x3c94(r29)
    lfs      f2, 0x3ca0(r29)
    stw      r21, 0xc8(r1)
    stw      r12, 0xcc(r1)
    stw      r6, 0x9c(r1)
    stw      r4, 0x8c(r1)
    stw      r3, 0x90(r1)
    stfs     f3, 0xc8(r1)
    stfs     f2, 0xcc(r1)
    lwz      r21, 0xc8(r1)
    stfs     f4, 0xbc(r1)
    lwz      r27, 0xcc(r1)
    stfs     f18, 0xc0(r1)
    lwz      r25, 0xbc(r1)
    stfs     f16, 0xb0(r1)
    lwz      r24, 0xc0(r1)
    stfs     f5, 0xb4(r1)
    lwz      r22, 0xb0(r1)
    stfs     f17, 0xa4(r1)
    lwz      r12, 0xb4(r1)
    stfs     f5, 0xa8(r1)
    lwz      r10, 0xa4(r1)
    stfs     f1, 0x98(r1)
    lwz      r9, 0xa8(r1)
    stfs     f18, 0x9c(r1)
    lwz      r7, 0x98(r1)
    stfs     f1, 0x8c(r1)
    lwz      r6, 0x9c(r1)
    stfs     f15, 0x90(r1)
    lwz      r4, 0x8c(r1)
    lwz      r3, 0x90(r1)
    stw      r0, 0xd0(r1)
    stw      r21, 0x28c(r1)
    stw      r27, 0x290(r1)
    stw      r0, 0x294(r1)
    stw      r26, 0xc4(r1)
    stw      r25, 0x298(r1)
    stw      r24, 0x29c(r1)
    stw      r26, 0x2a0(r1)
    stw      r23, 0xb8(r1)
    stw      r22, 0x2a4(r1)
    stw      r12, 0x2a8(r1)
    stw      r23, 0x2ac(r1)
    stw      r11, 0xac(r1)
    stw      r10, 0x2b0(r1)
    stw      r9, 0x2b4(r1)
    stw      r11, 0x2b8(r1)
    stw      r8, 0xa0(r1)
    stw      r7, 0x2bc(r1)
    stw      r6, 0x2c0(r1)
    stw      r8, 0x2c4(r1)
    stw      r5, 0x94(r1)
    stw      r4, 0x2c8(r1)
    stw      r3, 0x2cc(r1)
    stw      r5, 0x2d0(r1)
    lwz      r23, 0x9c(r31)
    lis      r0, 0x4330
    lwz      r22, 0xa0(r31)
    addis    r3, r30, 0x4
    lwz      r11, 0xa8(r31)
    lwz      r10, 0xac(r31)
    lwz      r8, 0xb4(r31)
    lwz      r7, 0xb8(r31)
    lwz      r24, 0xa4(r31)
    lwz      r12, 0xb0(r31)
    lwz      r9, 0xbc(r31)
    lwz      r6, 0xc8(r31)
    lwz      r5, 0xc0(r31)
    lwz      r4, 0xc4(r31)
    stw      r23, 0x80(r1)
    lfd      f2, lbl_8065D048@sda21(r0)
    stw      r22, 0x84(r1)
    stw      r11, 0x74(r1)
    stw      r10, 0x78(r1)
    stw      r8, 0x68(r1)
    stw      r7, 0x6c(r1)
    stw      r5, 0x5c(r1)
    stw      r4, 0x60(r1)
    stfs     f17, 0x80(r1)
    stfs     f0, 0x84(r1)
    lwz      r23, 0x80(r1)
    stfs     f16, 0x74(r1)
    lwz      r22, 0x84(r1)
    stfs     f0, 0x78(r1)
    lwz      r11, 0x74(r1)
    stfs     f4, 0x68(r1)
    lwz      r10, 0x78(r1)
    stfs     f15, 0x6c(r1)
    lwz      r8, 0x68(r1)
    stfs     f4, 0x5c(r1)
    lwz      r7, 0x6c(r1)
    stfs     f18, 0x60(r1)
    lwz      r5, 0x5c(r1)
    lwz      r4, 0x60(r1)
    stw      r23, 0x2d4(r1)
    stw      r22, 0x2d8(r1)
    stw      r24, 0x2dc(r1)
    stw      r11, 0x2e0(r1)
    stw      r10, 0x2e4(r1)
    stw      r12, 0x2e8(r1)
    stw      r8, 0x2ec(r1)
    stw      r7, 0x2f0(r1)
    stw      r9, 0x2f4(r1)
    stw      r5, 0x2f8(r1)
    stw      r4, 0x2fc(r1)
    stw      r6, 0x300(r1)
    lbz      r4, 0x11b(r30)
    stw      r0, 0x508(r1)
    lfs      f0, 0x3d10(r3)
    stw      r4, 0x50c(r1)
    lfd      f1, 0x508(r1)
    stw      r24, 0x88(r1)
    fsubs    f1, f1, f2
    stw      r12, 0x7c(r1)
    fmuls    f1, f1, f0
    stw      r9, 0x70(r1)
    stw      r6, 0x64(r1)
    bl       __cvt_fp2unsigned
    lbz      r0, 0x119(r30)
    li       r4, 0x1
    lbz      r6, 0x118(r30)
    li       r5, 0x7
    slwi     r0, r0, 16
    lbz      r7, 0x11a(r30)
    rlwimi   r0, r6, 24, 0, 7
    li       r6, 0x1
    rlwimi   r0, r7, 8, 16, 23
    or       r21, r0, r3
    lwz      r3, 0x290(r28)
    bl       SetGsDepthMode
    lis      r3, gGame@ha
    mr       r7, r21
    addi     r22, r3, gGame@l
    addi     r6, r1, 0x28c
    lwz      r5, 0x44(r22)
    mr       r3, r22
    li       r4, 0xa0
    li       r8, 0xa
    addi     r5, r5, 0xe8
    bl       DrawGsTrianglePrimitive
    lwz      r3, 0x290(r28)
    li       r4, 0x1
    li       r5, 0x1
    li       r6, 0x1
    bl       SetGsDepthMode
    lfs      f3, lbl_8065D058@sda21(r0)
    lis      r3, gGame@ha
    lfs      f4, lbl_8065D094@sda21(r0)
    fmr      f1, f16
    fmr      f2, f18
    addi     r3, r3, gGame@l
    fmr      f5, f4
    mr       r5, r21
    fmr      f6, f3
    fmr      f7, f3
    fmr      f8, f3
    mr       r6, r21
    li       r4, 0x10
    bl       DrawGsCircle
    lfs      f3, lbl_8065D058@sda21(r0)
    lis      r3, gGame@ha
    lfs      f4, lbl_8065D094@sda21(r0)
    fmr      f1, f17
    fmr      f2, f18
    addi     r3, r3, gGame@l
    fmr      f5, f4
    mr       r5, r21
    fmr      f6, f3
    fmr      f7, f3
    fmr      f8, f3
    mr       r6, r21
    li       r4, 0x10
    bl       DrawGsCircle
    lfs      f3, lbl_8065D058@sda21(r0)
    lis      r3, gGame@ha
    lfs      f4, lbl_8065D094@sda21(r0)
    fmr      f1, f16
    fmr      f2, f15
    addi     r3, r3, gGame@l
    fmr      f5, f4
    mr       r5, r21
    fmr      f6, f3
    fmr      f7, f3
    fmr      f8, f3
    mr       r6, r21
    li       r4, 0x10
    bl       DrawGsCircle
    lfs      f3, lbl_8065D058@sda21(r0)
    lis      r3, gGame@ha
    lfs      f4, lbl_8065D094@sda21(r0)
    fmr      f1, f17
    fmr      f2, f15
    addi     r3, r3, gGame@l
    fmr      f5, f4
    mr       r5, r21
    fmr      f6, f3
    fmr      f7, f3
    fmr      f8, f3
    mr       r6, r21
    li       r4, 0x10
    bl       DrawGsCircle
    lfs      f0, lbl_8065D0C8@sda21(r0)
    fmuls    f1, f0, f30
    bl       __cvt_fp2unsigned
    lfs      f1, lbl_8065D0C0@sda21(r0)
    mr       r21, r3
    lfs      f5, 0x28c(r1)
    lis      r5, gGame@ha
    lfs      f3, 0x290(r1)
    addi     r3, r5, gGame@l
    fadds    f0, f5, f1
    lfs      f5, 0x298(r1)
    fadds    f12, f3, f1
    lfs      f3, 0x29c(r1)
    fadds    f11, f5, f1
    lfs      f5, 0x2a4(r1)
    fadds    f10, f3, f1
    lfs      f3, 0x2a8(r1)
    fadds    f9, f5, f1
    lfs      f5, 0x2b0(r1)
    fadds    f8, f3, f1
    lfs      f3, 0x2b4(r1)
    fadds    f7, f5, f1
    lfs      f5, 0x2bc(r1)
    fadds    f6, f3, f1
    lfs      f3, 0x2c0(r1)
    fadds    f4, f5, f1
    lfs      f5, 0x2c8(r1)
    fadds    f2, f3, f1
    stfs     f0, 0x28c(r1)
    fadds    f0, f5, f1
    lfs      f3, 0x2cc(r1)
    stfs     f12, 0x290(r1)
    mr       r7, r21
    fadds    f12, f3, f1
    lfs      f5, 0x2d4(r1)
    stfs     f11, 0x298(r1)
    addi     r6, r1, 0x28c
    fadds    f11, f5, f1
    lfs      f3, 0x2d8(r1)
    stfs     f10, 0x29c(r1)
    fadds    f10, f3, f1
    lwz      r4, 0x44(r22)
    li       r8, 0xa
    lfs      f5, 0x2e0(r1)
    stfs     f9, 0x2a4(r1)
    addi     r5, r4, 0xe8
    fadds    f9, f5, f1
    lfs      f3, 0x2e4(r1)
    stfs     f8, 0x2a8(r1)
    li       r4, 0xa0
    fadds    f8, f3, f1
    lfs      f5, 0x2ec(r1)
    stfs     f7, 0x2b0(r1)
    fadds    f7, f5, f1
    lfs      f3, 0x2f0(r1)
    stfs     f6, 0x2b4(r1)
    fadds    f6, f3, f1
    lfs      f5, 0x2f8(r1)
    stfs     f4, 0x2bc(r1)
    fadds    f4, f5, f1
    lfs      f3, 0x2fc(r1)
    stfs     f2, 0x2c0(r1)
    fadds    f2, f3, f1
    stfs     f0, 0x2c8(r1)
    stfs     f12, 0x2cc(r1)
    stfs     f11, 0x2d4(r1)
    stfs     f10, 0x2d8(r1)
    stfs     f9, 0x2e0(r1)
    stfs     f8, 0x2e4(r1)
    stfs     f7, 0x2ec(r1)
    stfs     f6, 0x2f0(r1)
    stfs     f4, 0x2f8(r1)
    stfs     f2, 0x2fc(r1)
    bl       DrawGsTrianglePrimitive
    lfs      f0, lbl_8065D0C0@sda21(r0)
    lis      r3, gGame@ha
    lfs      f3, lbl_8065D058@sda21(r0)
    addi     r3, r3, gGame@l
    fadds    f1, f0, f22
    lfs      f2, lbl_8065D05C@sda21(r0)
    fadds    f0, f0, f20
    lfs      f4, lbl_8065D094@sda21(r0)
    lfs      f7, lbl_8065D0BC@sda21(r0)
    fmr      f6, f3
    fadds    f1, f2, f1
    mr       r5, r21
    fadds    f0, f2, f0
    mr       r6, r21
    fmr      f5, f4
    li       r4, 0x10
    fadds    f15, f7, f1
    fadds    f16, f7, f0
    fmr      f7, f3
    fmr      f1, f15
    fmr      f2, f16
    fmr      f8, f3
    bl       DrawGsCircle
    lfs      f0, lbl_8065D0C0@sda21(r0)
    lis      r3, gGame@ha
    lfs      f3, lbl_8065D058@sda21(r0)
    fmr      f2, f16
    fadds    f5, f0, f21
    lfs      f1, lbl_8065D05C@sda21(r0)
    lfs      f0, lbl_8065D0BC@sda21(r0)
    fmr      f6, f3
    lfs      f4, lbl_8065D094@sda21(r0)
    fsubs    f1, f5, f1
    fmr      f5, f4
    addi     r3, r3, gGame@l
    fmr      f7, f3
    mr       r5, r21
    fsubs    f16, f1, f0
    fmr      f8, f3
    mr       r6, r21
    li       r4, 0x10
    fmr      f1, f16
    bl       DrawGsCircle
    lfs      f0, lbl_8065D0C0@sda21(r0)
    lis      r3, gGame@ha
    lfs      f3, lbl_8065D058@sda21(r0)
    fmr      f1, f15
    fadds    f5, f0, f19
    lfs      f2, lbl_8065D05C@sda21(r0)
    lfs      f0, lbl_8065D0BC@sda21(r0)
    fmr      f6, f3
    lfs      f4, lbl_8065D094@sda21(r0)
    fsubs    f2, f5, f2
    fmr      f5, f4
    addi     r3, r3, gGame@l
    fmr      f7, f3
    mr       r5, r21
    fsubs    f15, f2, f0
    fmr      f8, f3
    mr       r6, r21
    li       r4, 0x10
    fmr      f2, f15
    bl       DrawGsCircle
    lfs      f3, lbl_8065D058@sda21(r0)
    lis      r3, gGame@ha
    lfs      f4, lbl_8065D094@sda21(r0)
    fmr      f1, f16
    fmr      f2, f15
    addi     r3, r3, gGame@l
    fmr      f5, f4
    mr       r5, r21
    fmr      f6, f3
    fmr      f7, f3
    fmr      f8, f3
    mr       r6, r21
    li       r4, 0x10
    bl       DrawGsCircle
    lis      r3, gGame@ha
    li       r5, 0x0
    addi     r3, r3, gGame@l
    lwz      r4, 0x3d4c(r3)
    addi     r3, r3, 0x3d6c
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x390
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r5, 0x0(r3)
    lis      r4, __vt__7CTexObj@ha
    lwz      r7, 0x4(r3)
    addi     r25, r1, 0x440
    addi     r6, r4, __vt__7CTexObj@l
    li       r0, 0x18
    stw      r5, 0x404(r1)
    subi     r5, r25, 0x4
    addi     r4, r3, 0x38
    stw      r7, 0x408(r1)
    lwz      r8, 0x8(r3)
    lwz      r7, 0xc(r3)
    stw      r8, 0x40c(r1)
    stw      r7, 0x410(r1)
    lwz      r8, 0x10(r3)
    lwz      r7, 0x14(r3)
    stw      r8, 0x414(r1)
    stw      r7, 0x418(r1)
    lwz      r8, 0x18(r3)
    lwz      r7, 0x1c(r3)
    stw      r8, 0x41c(r1)
    stw      r7, 0x420(r1)
    stw      r6, 0x500(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x424(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x428(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x42c(r1)
    stw      r6, 0x430(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x434(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x438(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x43c(r1)
    mtctr    r0
.L_80083400:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_80083400
    addi     r3, r1, 0x404
    bl       GXGetTexObjWidth
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x50c(r1)
    addi     r3, r1, 0x404
    lfd      f1, lbl_8065D048@sda21(r0)
    stw      r0, 0x508(r1)
    lfd      f0, 0x508(r1)
    fsubs    f15, f0, f1
    bl       GXGetTexObjHeight
    fsubs    f1, f21, f22
    clrlwi   r3, r3, 16
    lfs      f0, lbl_8065D0CC@sda21(r0)
    lis      r0, 0x4330
    stw      r3, 0x514(r1)
    fsubs    f9, f1, f0
    stw      r0, 0x510(r1)
    lfs      f0, lbl_8065D058@sda21(r0)
    lfd      f2, lbl_8065D048@sda21(r0)
    lfd      f1, 0x510(r1)
    fcmpo    cr0, f9, f0
    fsubs    f5, f1, f2
    bge      .L_80083478
    fmr      f9, f0
.L_80083478:
    fsubs    f2, f19, f20
    lfs      f1, lbl_8065D0C8@sda21(r0)
    lfs      f0, lbl_8065D058@sda21(r0)
    fsubs    f10, f2, f1
    fcmpo    cr0, f10, f0
    bge      .L_80083494
    fmr      f10, f0
.L_80083494:
    li       r0, 0x20
    addi     r5, r1, 0x300
    addi     r4, r31, 0x140
    mtctr    r0
.L_800834A4:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_800834A4
    lfs      f7, lbl_8065D05C@sda21(r0)
    lfs      f2, lbl_8065D094@sda21(r0)
    fdivs    f8, f7, f15
    lfs      f13, lbl_8065D0D0@sda21(r0)
    lfs      f16, lbl_8065D0D4@sda21(r0)
    lfs      f12, lbl_8065D0D8@sda21(r0)
    lfs      f11, lbl_8065D0DC@sda21(r0)
    lfs      f1, lbl_8065D060@sda21(r0)
    fadds    f0, f2, f20
    lfs      f4, lbl_8065D078@sda21(r0)
    fadds    f6, f13, f22
    lfs      f3, lbl_8065D0BC@sda21(r0)
    fsubs    f15, f21, f13
    stfs     f8, 0x32c(r1)
    fsubs    f17, f0, f1
    lfs      f13, lbl_8065D068@sda21(r0)
    fsubs    f6, f6, f4
    lfs      f0, lbl_8065D0B8@sda21(r0)
    fdivs    f7, f7, f5
    stfs     f6, 0x314(r1)
    fadds    f5, f3, f17
    fadds    f12, f12, f10
    fsubs    f10, f19, f2
    stfs     f5, 0x318(r1)
    fadds    f2, f16, f9
    fadds    f15, f4, f15
    lfs      f9, 0x3c94(r29)
    fsubs    f12, f12, f3
    fadds    f10, f1, f10
    stfs     f9, 0x334(r1)
    fmuls    f1, f11, f30
    stfs     f5, 0x338(r1)
    stfs     f2, 0x33c(r1)
    stfs     f15, 0x354(r1)
    stfs     f5, 0x358(r1)
    stfs     f7, 0x370(r1)
    stfs     f6, 0x374(r1)
    lfs      f4, 0x3ca4(r29)
    lfs      f3, 0x3ca0(r29)
    fmadds   f3, f4, f13, f3
    stfs     f12, 0x380(r1)
    stfs     f7, 0x390(r1)
    fadds    f0, f0, f3
    stfs     f15, 0x394(r1)
    stfs     f0, 0x378(r1)
    stfs     f0, 0x398(r1)
    stfs     f12, 0x3a0(r1)
    stfs     f6, 0x3b4(r1)
    stfs     f10, 0x3b8(r1)
    stfs     f8, 0x3cc(r1)
    stfs     f9, 0x3d4(r1)
    stfs     f10, 0x3d8(r1)
    stfs     f2, 0x3dc(r1)
    stfs     f15, 0x3f4(r1)
    stfs     f10, 0x3f8(r1)
    bl       __cvt_fp2unsigned
    lis      r4, gGame@ha
    li       r0, -0x100
    addi     r27, r4, gGame@l
    li       r4, 0x0
    or       r24, r3, r0
    lwz      r3, 0x294(r27)
    bl       SetGsTevMode
    lwz      r3, 0x290(r28)
    li       r4, 0x1
    li       r5, 0x1
    li       r6, 0x1
    bl       SetGsDepthMode
    lis      r3, gGame@ha
    addi     r21, r1, 0x304
    addi     r23, r1, 0x470
    li       r26, 0x0
    addi     r22, r3, gGame@l
.L_800835DC:
    lfs      f1, 0x0(r21)
    mr       r3, r25
    lfs      f2, 0x4(r21)
    lfs      f3, lbl_8065D058@sda21(r0)
    bl       PSMTXTrans
    li       r0, 0x1
    lfs      f1, 0x8(r21)
    stw      r0, 0x43c(r1)
    mr       r3, r23
    lfs      f2, 0xc(r21)
    lfs      f3, lbl_8065D05C@sda21(r0)
    bl       PSMTXScale
    lfs      f6, lbl_8065D058@sda21(r0)
    li       r0, 0x1
    stw      r0, 0x43c(r1)
    mr       r3, r22
    fmr      f7, f6
    lfs      f1, 0x10(r21)
    fmr      f8, f6
    lfs      f2, 0x14(r21)
    lfs      f3, lbl_8065D05C@sda21(r0)
    mr       r5, r24
    lfs      f4, 0x18(r21)
    mr       r6, r24
    lfs      f5, 0x1c(r21)
    mr       r7, r24
    mr       r8, r24
    addi     r4, r1, 0x404
    bl       DrawGsTexturedQuad
    addi     r26, r26, 0x1
    addi     r21, r21, 0x20
    cmpwi    r26, 0x8
    blt      .L_800835DC
    lwz      r3, 0x294(r27)
    li       r4, 0x4
    bl       SetGsTevMode
    lwz      r3, 0x290(r28)
    li       r4, 0x1
    li       r5, 0x7
    li       r6, 0x1
    bl       SetGsDepthMode
    lfs      f0, lbl_8065D0BC@sda21(r0)
    lis      r3, gGame@ha
    lfs      f3, lbl_8065D0C4@sda21(r0)
    fmr      f4, f27
    fsubs    f1, f22, f0
    addi     r3, r3, gGame@l
    stfs     f3, 0x8(r1)
    fsubs    f2, f20, f0
    fmr      f6, f3
    stfs     f1, 0xc(r1)
    fmr      f5, f2
    lis      r4, 0xff00
    stfs     f28, 0x10(r1)
    fmr      f7, f27
    fmr      f8, f28
    lis      r5, 0xff00
    stfs     f3, 0x14(r1)
    lis      r6, 0xff00
    lis      r7, 0xff00
    bl       DrawGsVertexQuad
    lwz      r3, 0x290(r28)
    li       r4, 0x1
    li       r5, 0x3
    li       r6, 0x0
    bl       SetGsDepthMode
    lwz      r0, 0xaa8(r30)
    cmpwi    r0, 0x0
    blt      .L_80083D80
    cmpwi    r0, 0x1
    bge      .L_80083D80
    lbz      r0, 0x11b(r30)
    cmplwi   r0, 0x0
    beq      .L_800837A4
    lfs      f0, lbl_8065D0BC@sda21(r0)
    lfs      f1, lbl_8065D05C@sda21(r0)
    fadds    f2, f0, f23
    lfs      f3, lbl_8065D0E0@sda21(r0)
    lfs      f0, 0x3ca4(r29)
    lfs      f15, 0x3c98(r29)
    fsubs    f1, f2, f1
    fmadds   f1, f3, f1, f0
    bl       __cvt_fp2unsigned
    lfs      f1, lbl_8065D0B0@sda21(r0)
    mr       r24, r3
    lfs      f0, lbl_8065D05C@sda21(r0)
    fadds    f1, f1, f24
    lfs      f2, lbl_8065D0E0@sda21(r0)
    fsubs    f0, f1, f0
    fmadds   f1, f2, f0, f15
    bl       __cvt_fp2unsigned
    lfs      f1, 0x3ca0(r29)
    mr       r23, r3
    lfs      f0, lbl_8065D0BC@sda21(r0)
    lfs      f2, lbl_8065D05C@sda21(r0)
    fsubs    f0, f1, f0
    fsubs    f0, f0, f23
    fadds    f1, f2, f0
    bl       __cvt_fp2unsigned
    lfs      f2, lbl_8065D068@sda21(r0)
    mr       r22, r3
    lfs      f1, 0x3c94(r29)
    lfs      f0, lbl_8065D0B0@sda21(r0)
    fnmsubs  f1, f15, f2, f1
    lfs      f2, lbl_8065D05C@sda21(r0)
    fsubs    f0, f1, f0
    fsubs    f0, f0, f24
    fsubs    f0, f0, f29
    fadds    f1, f2, f0
    bl       __cvt_fp2unsigned
    mr       r4, r22
    mr       r5, r23
    mr       r6, r24
    bl       GXSetScissor
.L_800837A4:
    lfs      f1, 0x3c98(r29)
    lis      r3, gGame@ha
    lfs      f4, lbl_8065D068@sda21(r0)
    addi     r3, r3, gGame@l
    lfs      f0, 0x3c94(r29)
    addis    r3, r3, 0x1
    lfs      f2, lbl_8065D0B0@sda21(r0)
    li       r22, 0x0
    fnmsubs  f3, f1, f4, f0
    lfs      f1, 0x3ca4(r29)
    lfs      f0, 0x3ca0(r29)
    li       r7, 0x0
    lfs      f5, lbl_8065D0E4@sda21(r0)
    fsubs    f2, f3, f2
    fmadds   f0, f1, f4, f0
    lfs      f1, lbl_8065D0E8@sda21(r0)
    lwz      r0, -0x68a0(r3)
    fsubs    f2, f2, f29
    lwz      r9, -0x6894(r3)
    fmadds   f19, f1, f26, f0
    lwz      r8, -0x68a4(r3)
    fmadds   f18, f5, f26, f2
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_80083868
.L_80083808:
    add      r5, r8, r7
    li       r0, 0x1
    lwz      r3, 0x22c(r5)
    li       r6, 0x0
    lwz      r4, 0x228(r5)
    and      r3, r3, r0
    and      r0, r4, r6
    xor      r3, r3, r6
    xor      r0, r0, r6
    or.      r0, r3, r0
    beq      .L_80083860
    lwz      r3, 0x240(r5)
    cmplwi   r3, 0x0
    beq      .L_8008384C
    beq      .L_80083850
    lha      r6, 0x16(r3)
    b        .L_80083850
.L_8008384C:
    li       r6, -0x1
.L_80083850:
    extsh    r0, r6
    cmpwi    r0, 0xa
    bne      .L_80083860
    mr       r22, r5
.L_80083860:
    add      r7, r7, r9
    bdnz     .L_80083808
.L_80083868:
    cmplwi   r22, 0x0
    beq      .L_80083C20
    lwz      r4, 0x0(r22)
    lwz      r0, 0x14(r4)
    rlwinm.  r0, r0, 0, 8, 8
    beq      .L_80083888
    lfs      f25, lbl_8065D058@sda21(r0)
    b        .L_80083C88
.L_80083888:
    lfs      f0, lbl_8065D0C0@sda21(r0)
    addis    r3, r30, 0x4
    lwz      r9, 0xc8(r22)
    fmuls    f1, f0, f26
    lfs      f0, 0x3d10(r3)
    lwz      r8, 0xcc(r22)
    addi     r3, r1, 0x254
    lwz      r7, 0xd0(r22)
    lwz      r6, 0x128(r22)
    lwz      r5, 0x12c(r22)
    fmuls    f20, f1, f0
    lwz      r0, 0x130(r22)
    stw      r9, 0xe0(r1)
    lfs      f15, 0xfc(r22)
    stw      r8, 0xe4(r1)
    stw      r7, 0xe8(r1)
    stw      r6, 0xd4(r1)
    stw      r5, 0xd8(r1)
    stw      r0, 0xdc(r1)
    bl       __ct__5CJObjFP9_HSD_JObj
    lfs      f0, lbl_8065D0EC@sda21(r0)
    fmr      f1, f18
    fmr      f2, f19
    addi     r3, r1, 0x254
    fmuls    f16, f0, f26
    li       r4, 0x1
    fmr      f3, f16
    bl       SetTranslate__5CJObjFfffl
    fneg     f17, f20
    addi     r3, r1, 0x254
    fmr      f1, f20
    li       r4, 0x1
    fmr      f3, f20
    fmr      f2, f17
    bl       SetScale__5CJObjFfffl
    lfs      f1, lbl_8065D058@sda21(r0)
    addi     r3, r1, 0x254
    li       r4, 0x1
    bl       SetRotationY__5CJObjFfl
    lwz      r21, 0x254(r1)
    cmplwi   r21, 0x0
    beq      .L_80083970
    bne      .L_80083944
    li       r3, lbl_8065D07C@sda21
    li       r4, 0x25d
    li       r5, lbl_8065D084@sda21
    bl       __assert
.L_80083944:
    lwz      r4, 0x14(r21)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_80083960
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_80083960
    li       r3, 0x1
.L_80083960:
    clrlwi.  r0, r3, 24
    beq      .L_80083970
    mr       r3, r21
    bl       HSD_JObjSetupMatrixSub
.L_80083970:
    lwz      r11, 0x244(r31)
    mr       r4, r22
    lwz      r10, 0x248(r31)
    addi     r3, r1, 0x4c
    lwz      r9, 0x24c(r31)
    addi     r6, r1, 0xec
    lwz      r8, 0x250(r31)
    li       r5, 0x6
    lwz      r7, 0x254(r31)
    lwz      r0, 0x258(r31)
    stw      r11, 0xec(r1)
    stw      r10, 0xf0(r1)
    stw      r9, 0xf4(r1)
    stw      r8, 0xf8(r1)
    stw      r7, 0xfc(r1)
    stw      r0, 0x100(r1)
    bl       CJObj_GetJointWorldPositionByPath
    lfs      f1, 0x50(r1)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    lfs      f0, lbl_8065D0E8@sda21(r0)
    fsubs    f2, f19, f1
    stw      r0, 0x58(r1)
    fmr      f1, f18
    addi     r3, r1, 0x254
    fmr      f3, f16
    li       r4, 0x1
    fsubs    f0, f2, f0
    fadds    f2, f19, f0
    bl       SetTranslate__5CJObjFfffl
    fmr      f1, f20
    addi     r3, r1, 0x254
    fmr      f2, f17
    li       r4, 0x1
    fmr      f3, f20
    bl       SetScale__5CJObjFfffl
    lfs      f1, lbl_8065D058@sda21(r0)
    addi     r3, r1, 0x254
    li       r4, 0x1
    bl       SetRotationY__5CJObjFfl
    lwz      r3, 0x0(r22)
    li       r4, 0x8
    bl       HSD_JObjSetFlags
    lwz      r26, 0x168(r22)
    li       r4, -0x100
    lwz      r25, 0x164(r22)
    li       r0, 0x0
    mr       r3, r22
    stw      r4, 0x168(r22)
    stw      r0, 0x164(r22)
    lwz      r12, 0x34(r22)
    lwz      r12, 0xac(r12)
    mtctr    r12
    bctrl
    lwz      r21, 0x254(r1)
    addi     r24, r1, 0x258
    lfs      f2, 0xe0(r1)
    lfs      f1, 0xe4(r1)
    cmplwi   r21, 0x0
    lfs      f0, 0xe8(r1)
    stfs     f2, 0x258(r1)
    stfs     f1, 0x25c(r1)
    stfs     f0, 0x260(r1)
    bne      .L_80083A80
    li       r3, lbl_8065D07C@sda21
    li       r4, 0x3a9
    li       r5, lbl_8065D084@sda21
    bl       __assert
.L_80083A80:
    cmplwi   r24, 0x0
    bne      .L_80083A98
    addi     r5, r31, 0x2e8
    li       r3, lbl_8065D07C@sda21
    li       r4, 0x3aa
    bl       __assert
.L_80083A98:
    lfs      f0, 0x258(r1)
    stfs     f0, 0x38(r21)
    lfs      f0, 0x25c(r1)
    stfs     f0, 0x3c(r21)
    lfs      f0, 0x260(r1)
    stfs     f0, 0x40(r21)
    lwz      r0, 0x14(r21)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_80083B04
    cmplwi   r21, 0x0
    beq      .L_80083B04
    bne      .L_80083AD8
    li       r3, lbl_8065D07C@sda21
    li       r4, 0x25d
    li       r5, lbl_8065D084@sda21
    bl       __assert
.L_80083AD8:
    lwz      r4, 0x14(r21)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_80083AF4
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_80083AF4
    li       r3, 0x1
.L_80083AF4:
    clrlwi.  r0, r3, 24
    bne      .L_80083B04
    mr       r3, r21
    bl       HSD_JObjSetMtxDirtySub
.L_80083B04:
    lwz      r21, 0x254(r1)
    addi     r23, r1, 0x278
    lfs      f2, 0xd4(r1)
    lfs      f1, 0xd8(r1)
    cmplwi   r21, 0x0
    lfs      f0, 0xdc(r1)
    stfs     f2, 0x278(r1)
    stfs     f1, 0x27c(r1)
    stfs     f0, 0x280(r1)
    bne      .L_80083B3C
    li       r3, lbl_8065D07C@sda21
    li       r4, 0x316
    li       r5, lbl_8065D084@sda21
    bl       __assert
.L_80083B3C:
    cmplwi   r23, 0x0
    bne      .L_80083B54
    li       r3, lbl_8065D07C@sda21
    li       r4, 0x317
    li       r5, lbl_8065D08C@sda21
    bl       __assert
.L_80083B54:
    lfs      f0, 0x278(r1)
    stfs     f0, 0x2c(r21)
    lfs      f0, 0x27c(r1)
    stfs     f0, 0x30(r21)
    lfs      f0, 0x280(r1)
    stfs     f0, 0x34(r21)
    lwz      r0, 0x14(r21)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_80083BC0
    cmplwi   r21, 0x0
    beq      .L_80083BC0
    bne      .L_80083B94
    li       r3, lbl_8065D07C@sda21
    li       r4, 0x25d
    li       r5, lbl_8065D084@sda21
    bl       __assert
.L_80083B94:
    lwz      r4, 0x14(r21)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_80083BB0
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_80083BB0
    li       r3, 0x1
.L_80083BB0:
    clrlwi.  r0, r3, 24
    bne      .L_80083BC0
    mr       r3, r21
    bl       HSD_JObjSetMtxDirtySub
.L_80083BC0:
    fmr      f1, f15
    addi     r3, r1, 0x254
    li       r4, 0x1
    bl       SetRotationY__5CJObjFfl
    lwz      r3, 0x0(r22)
    li       r4, 0x8
    bl       HSD_JObjClearFlags
    stw      r26, 0x168(r22)
    lis      r3, __vt__5CJObj@ha
    cmplwi   r23, 0x0
    stw      r25, 0x164(r22)
    addi     r0, r3, __vt__5CJObj@l
    stw      r0, 0x288(r1)
    beq      .L_80083C04
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x284(r1)
.L_80083C04:
    lis      r3, __vt__4CVec@ha
    cmplwi   r24, 0x0
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x274(r1)
    beq      .L_80083C88
    stw      r0, 0x264(r1)
    b        .L_80083C88
.L_80083C20:
    addi     r3, r30, 0x120
    lfs      f0, lbl_8065D0EC@sda21(r0)
    lwz      r12, 0x154(r30)
    fmr      f1, f18
    fmr      f2, f19
    li       r4, 0x0
    lwz      r12, 0x68(r12)
    fmuls    f3, f0, f26
    mtctr    r12
    bctrl
    lfs      f0, lbl_8065D0C0@sda21(r0)
    addi     r3, r30, 0x120
    addis    r4, r30, 0x4
    lwz      r12, 0x154(r30)
    fmuls    f1, f0, f26
    lfs      f0, 0x3d10(r4)
    lwz      r12, 0x6c(r12)
    li       r4, 0x0
    fmuls    f1, f1, f0
    mtctr    r12
    bctrl
    addi     r3, r30, 0x120
    lwz      r12, 0x154(r30)
    lwz      r12, 0xac(r12)
    mtctr    r12
    bctrl
.L_80083C88:
    lis      r4, gGame@ha
    li       r3, -0x1
    addi     r26, r4, gGame@l
    bl       HSD_StateInvalidate
    lwz      r0, 0x290(r28)
    cmplwi   r0, 0x0
    beq      .L_80083CF8
    lwz      r3, 0x290(r26)
    lwz      r0, 0xc(r3)
    cmpwi    r0, 0x0
    bne      .L_80083CBC
    li       r0, 0x1
    stw      r0, 0xc(r3)
.L_80083CBC:
    lwz      r0, 0x10(r3)
    cmpwi    r0, 0x0
    bne      .L_80083CD0
    li       r0, 0x1
    stw      r0, 0x10(r3)
.L_80083CD0:
    lwz      r0, 0x14(r3)
    cmpwi    r0, 0x0
    bne      .L_80083CE4
    li       r0, 0x1
    stw      r0, 0x14(r3)
.L_80083CE4:
    lwz      r0, 0x18(r3)
    cmpwi    r0, 0x0
    bne      .L_80083CF8
    li       r0, 0x1
    stw      r0, 0x18(r3)
.L_80083CF8:
    lwz      r3, 0x294(r26)
    cmplwi   r3, 0x0
    beq      .L_80083D40
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_80083D18
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_80083D18:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80083D2C
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80083D2C:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_80083D40
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_80083D40:
    lwz      r3, 0x298(r26)
    cmplwi   r3, 0x0
    beq      .L_80083D60
    lwz      r0, 0x30(r3)
    cmpwi    r0, 0x0
    bne      .L_80083D60
    li       r0, 0x1
    stw      r0, 0x30(r3)
.L_80083D60:
    lbz      r0, 0x11b(r30)
    cmplwi   r0, 0x0
    beq      .L_80083D80
    li       r3, 0x0
    li       r4, 0x0
    li       r5, 0x280
    li       r6, 0x1c0
    bl       GXSetScissor
.L_80083D80:
    addis    r4, r30, 0x4
    lwz      r0, 0x3cec(r4)
    cmpwi    r0, 0x0
    blt      .L_80084000
    lfs      f1, lbl_8065D0BC@sda21(r0)
    slwi     r0, r0, 2
    lfs      f0, 0x3ca0(r29)
    lwz      r3, 0x0(r30)
    fadds    f0, f1, f0
    lfs      f4, 0x3cf0(r4)
    lfs      f5, lbl_8065D068@sda21(r0)
    add      r3, r3, r0
    lfs      f1, 0x3c94(r29)
    lfs      f3, lbl_8065D0F0@sda21(r0)
    lfs      f2, 0x3cf4(r4)
    fmadds   f16, f25, f5, f1
    lfs      f1, lbl_8065D0F4@sda21(r0)
    fadds    f15, f4, f0
    lfs      f0, 0x3d18(r4)
    fadds    f17, f3, f2
    lfs      f19, 0x858(r3)
    fmuls    f1, f1, f0
    bl       __cvt_fp2unsigned
    oris     r21, r3, 0xc87f
    lwz      r3, 0x294(r27)
    ori      r21, r21, 0x1600
    li       r4, 0x0
    bl       SetGsTevMode
    lfs      f2, lbl_8065D05C@sda21(r0)
    addis    r22, r30, 0x4
    addi     r3, r22, 0x3ad8
    lfs      f1, lbl_8065D0F8@sda21(r0)
    fmr      f3, f2
    addi     r22, r22, 0x3a6c
    bl       PSMTXScale
    lfs      f2, lbl_8065D058@sda21(r0)
    li       r0, 0x1
    stw      r0, 0x38(r22)
    addi     r3, r22, 0x3c
    fmr      f3, f2
    lfs      f1, lbl_8065D0F8@sda21(r0)
    bl       PSMTXTrans
    lfs      f0, lbl_8065D0E0@sda21(r0)
    li       r0, 0x1
    lfs      f3, lbl_8065D058@sda21(r0)
    lis      r3, gGame@ha
    fadds    f15, f0, f15
    stw      r0, 0x38(r22)
    fmr      f1, f16
    addi     r3, r3, gGame@l
    fmr      f4, f17
    mr       r4, r22
    fmr      f2, f15
    mr       r5, r21
    fmr      f5, f19
    mr       r6, r21
    fmr      f6, f3
    mr       r7, r21
    fmr      f7, f3
    mr       r8, r21
    fmr      f8, f3
    bl       DrawGsTexturedQuad
    lfs      f1, lbl_8065D058@sda21(r0)
    addi     r3, r22, 0x3c
    fmr      f2, f1
    fmr      f3, f1
    bl       PSMTXTrans
    lfs      f0, lbl_8065D068@sda21(r0)
    li       r0, 0x1
    lfs      f3, lbl_8065D058@sda21(r0)
    lis      r3, gGame@ha
    fmuls    f18, f17, f0
    lfs      f0, lbl_8065D0B8@sda21(r0)
    fmr      f2, f15
    stw      r0, 0x38(r22)
    fmr      f5, f19
    lfs      f4, lbl_8065D0BC@sda21(r0)
    fsubs    f1, f16, f18
    addi     r3, r3, gGame@l
    fmr      f6, f3
    mr       r4, r22
    fmr      f7, f3
    mr       r5, r21
    fmr      f8, f3
    mr       r6, r21
    fsubs    f1, f1, f0
    mr       r7, r21
    mr       r8, r21
    bl       DrawGsTexturedQuad
    lfs      f2, lbl_8065D058@sda21(r0)
    addi     r3, r22, 0x3c
    lfs      f1, lbl_8065D068@sda21(r0)
    fmr      f3, f2
    bl       PSMTXTrans
    fadds    f0, f16, f18
    lfs      f3, lbl_8065D058@sda21(r0)
    lfs      f1, lbl_8065D0B8@sda21(r0)
    li       r0, 0x1
    lis      r3, gGame@ha
    fmr      f2, f15
    fmr      f5, f19
    stw      r0, 0x38(r22)
    fmr      f6, f3
    lfs      f4, lbl_8065D0BC@sda21(r0)
    fmr      f7, f3
    addi     r3, r3, gGame@l
    fmr      f8, f3
    mr       r4, r22
    fadds    f1, f1, f0
    mr       r5, r21
    mr       r6, r21
    mr       r7, r21
    mr       r8, r21
    bl       DrawGsTexturedQuad
    addis    r21, r30, 0x4
    addi     r21, r21, 0x3b6c
    mr       r3, r21
    bl       GXGetTexObjWidth
    lis      r0, 0x4330
    clrlwi   r3, r3, 16
    stw      r3, 0x514(r1)
    addi     r3, r21, 0x6c
    lfd      f1, lbl_8065D048@sda21(r0)
    stw      r0, 0x510(r1)
    lfs      f2, lbl_8065D068@sda21(r0)
    lfd      f0, 0x510(r1)
    lfs      f3, lbl_8065D05C@sda21(r0)
    fsubs    f0, f0, f1
    fdivs    f0, f17, f0
    fmuls    f1, f2, f0
    bl       PSMTXScale
    li       r0, 0x1
    addis    r3, r30, 0x4
    stw      r0, 0x38(r21)
    lfs      f1, lbl_8065D0B0@sda21(r0)
    lfs      f0, 0x3d18(r3)
    fmuls    f1, f1, f0
    bl       __cvt_fp2unsigned
    lfs      f3, lbl_8065D058@sda21(r0)
    li       r0, -0x100
    lfs      f1, lbl_8065D0E0@sda21(r0)
    or       r5, r3, r0
    lfs      f0, lbl_8065D0B8@sda21(r0)
    fmr      f2, f15
    lis      r3, gGame@ha
    fmr      f4, f17
    fmr      f6, f3
    addi     r3, r3, gGame@l
    fmr      f7, f3
    fmr      f8, f3
    mr       r4, r21
    fadds    f1, f1, f16
    fsubs    f5, f19, f0
    mr       r6, r5
    mr       r7, r5
    mr       r8, r5
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r27)
    li       r4, 0x4
    bl       SetGsTevMode
.L_80084000:
    addis    r3, r30, 0x4
    lwz      r4, 0x3d0c(r3)
    cmpwi    r4, 0x0
    blt      .L_8008435C
    lwz      r0, 0x3d04(r3)
    lis      r8, 0x4330
    lwz      r5, 0x3c78(r3)
    addi     r4, r4, 0x1
    xoris    r0, r0, 0x8000
    lwz      r6, 0x25c(r31)
    mulli    r7, r5, 0x2ac
    stw      r0, 0x50c(r1)
    xoris    r9, r4, 0x8000
    lwz      r5, 0x260(r31)
    stw      r8, 0x508(r1)
    add      r4, r30, r7
    lwz      r0, 0xa8c(r4)
    lfd      f4, lbl_8065D050@sda21(r0)
    lfd      f0, 0x508(r1)
    slwi     r0, r0, 2
    add      r4, r3, r0
    lfs      f5, lbl_8065D068@sda21(r0)
    lfs      f1, 0x3c94(r29)
    fsubs    f0, f0, f4
    lfs      f6, 0x3cd0(r3)
    lwz      r7, 0x0(r30)
    fmadds   f16, f25, f5, f1
    fmuls    f2, f0, f6
    lfs      f1, lbl_8065D0BC@sda21(r0)
    lfs      f0, 0x3ca0(r29)
    add      r7, r7, r0
    stw      r9, 0x514(r1)
    fadds    f0, f1, f0
    stw      r8, 0x510(r1)
    fmadds   f2, f2, f5, f16
    lfs      f1, 0x3ca8(r4)
    lfd      f3, 0x510(r1)
    lwz      r4, 0x268(r31)
    fadds    f0, f1, f0
    fsubs    f3, f3, f4
    lfs      f1, 0x858(r7)
    lwz      r0, 0x26c(r31)
    lwz      r27, 0x28c(r31)
    fnmsubs  f0, f1, f5, f0
    fnmsubs  f4, f3, f6, f2
    lfs      f1, lbl_8065D078@sda21(r0)
    stw      r6, 0x40(r1)
    lwz      r26, 0x290(r31)
    fadds    f15, f1, f0
    stw      r5, 0x44(r1)
    fadds    f2, f4, f6
    lwz      r5, 0x294(r31)
    stw      r4, 0x34(r1)
    lwz      r25, 0x298(r31)
    stw      r0, 0x38(r1)
    lwz      r24, 0x29c(r31)
    stfs     f4, 0x40(r1)
    lwz      r0, 0x2a0(r31)
    stfs     f15, 0x44(r1)
    lwz      r23, 0x2a4(r31)
    stfs     f2, 0x34(r1)
    lwz      r22, 0x2a8(r31)
    stfs     f15, 0x38(r1)
    lwz      r12, 0x2ac(r31)
    lwz      r11, 0x2b0(r31)
    lwz      r10, 0x2b4(r31)
    lwz      r9, 0x2b8(r31)
    lwz      r8, 0x264(r31)
    stw      r5, 0x22c(r1)
    lwz      r5, 0x270(r31)
    stw      r0, 0x238(r1)
    lwz      r7, 0x40(r1)
    stw      r27, 0x224(r1)
    lwz      r6, 0x44(r1)
    stw      r26, 0x228(r1)
    lwz      r4, 0x34(r1)
    stw      r25, 0x230(r1)
    lwz      r0, 0x38(r1)
    stw      r24, 0x234(r1)
    stw      r23, 0x23c(r1)
    stw      r22, 0x240(r1)
    stw      r12, 0x244(r1)
    stw      r11, 0x248(r1)
    stw      r10, 0x24c(r1)
    stw      r9, 0x250(r1)
    stw      r8, 0x48(r1)
    stw      r7, 0x224(r1)
    stw      r6, 0x228(r1)
    stw      r8, 0x22c(r1)
    stw      r5, 0x3c(r1)
    stw      r4, 0x230(r1)
    stw      r0, 0x234(r1)
    stw      r5, 0x238(r1)
    lwz      r4, 0x274(r31)
    fadds    f3, f15, f6
    lwz      r6, 0x278(r31)
    lwz      r5, 0x280(r31)
    lwz      r0, 0x284(r31)
    stw      r4, 0x28(r1)
    lfs      f1, lbl_8065D0F4@sda21(r0)
    lwz      r7, 0x27c(r31)
    lwz      r4, 0x288(r31)
    stw      r6, 0x2c(r1)
    lfs      f0, 0x3d18(r3)
    stw      r5, 0x1c(r1)
    fmuls    f1, f1, f0
    stw      r0, 0x20(r1)
    stfs     f2, 0x28(r1)
    stfs     f3, 0x2c(r1)
    lwz      r6, 0x28(r1)
    stfs     f4, 0x1c(r1)
    lwz      r5, 0x2c(r1)
    stfs     f3, 0x20(r1)
    lwz      r3, 0x1c(r1)
    lwz      r0, 0x20(r1)
    stw      r7, 0x30(r1)
    stw      r6, 0x23c(r1)
    stw      r5, 0x240(r1)
    stw      r7, 0x244(r1)
    stw      r4, 0x24(r1)
    stw      r3, 0x248(r1)
    stw      r0, 0x24c(r1)
    stw      r4, 0x250(r1)
    bl       __cvt_fp2unsigned
    lfs      f1, 0x244(r1)
    ori      r4, r3, 0xff00
    lis      r3, gGame@ha
    lfs      f0, 0x248(r1)
    stfs     f1, 0x8(r1)
    addi     r3, r3, gGame@l
    lfs      f1, 0x24c(r1)
    mr       r5, r4
    stfs     f0, 0xc(r1)
    mr       r6, r4
    lfs      f0, 0x250(r1)
    mr       r7, r4
    stfs     f1, 0x10(r1)
    lfs      f1, 0x224(r1)
    stfs     f0, 0x14(r1)
    lfs      f2, 0x228(r1)
    lfs      f3, 0x22c(r1)
    lfs      f4, 0x230(r1)
    lfs      f5, 0x234(r1)
    lfs      f6, 0x238(r1)
    lfs      f7, 0x23c(r1)
    lfs      f8, 0x240(r1)
    bl       DrawGsVertexQuad
    lis      r3, gGame@ha
    lfd      f19, lbl_8065D050@sda21(r0)
    lfs      f17, lbl_8065D068@sda21(r0)
    addi     r22, r3, gGame@l
    addis    r25, r30, 0x4
    li       r21, 0x0
    lis      r24, 0x4330
    b        .L_80084350
.L_8008426C:
    xoris    r0, r0, 0x8000
    addi     r3, r21, 0x1
    stw      r0, 0x50c(r1)
    xoris    r3, r3, 0x8000
    lwz      r0, 0x3d08(r25)
    stw      r24, 0x508(r1)
    lfs      f2, 0x3cd0(r25)
    cmpw     r21, r0
    lfd      f0, 0x508(r1)
    stw      r3, 0x514(r1)
    fsubs    f0, f0, f19
    stw      r24, 0x510(r1)
    fmuls    f0, f0, f2
    lfd      f1, 0x510(r1)
    fsubs    f1, f1, f19
    fmadds   f0, f0, f17, f16
    fnmsubs  f18, f1, f2, f0
    bge      .L_800842D0
    lfs      f1, lbl_8065D0DC@sda21(r0)
    lfs      f0, 0x3d14(r25)
    fmuls    f1, f1, f0
    bl       __cvt_fp2unsigned
    li       r0, -0x100
    or       r23, r3, r0
    b        .L_800842E8
.L_800842D0:
    lfs      f1, lbl_8065D0DC@sda21(r0)
    lfs      f0, 0x3d14(r25)
    fmuls    f1, f1, f0
    bl       __cvt_fp2unsigned
    oris     r23, r3, 0x64ff
    ori      r23, r23, 0x6400
.L_800842E8:
    addis    r5, r21, 0x4
    addi     r3, r1, 0x18
    addi     r5, r5, 0x3cf8
    li       r4, lbl_8065D0FC@sda21
    lbzx     r5, r30, r5
    extsb    r5, r5
    crclr    cr1eq
    bl       sprintf
    lfs      f1, 0x3cd0(r25)
    mr       r3, r22
    addi     r4, r1, 0x18
    li       r5, 0x1
    bl       MeasureGameTextWidth
    lfs      f4, 0x3cd0(r25)
    fmr      f2, f15
    lfs      f3, lbl_8065D058@sda21(r0)
    mr       r3, r22
    fsubs    f0, f4, f1
    mr       r4, r23
    fmr      f5, f4
    addi     r5, r1, 0x18
    fmadds   f18, f0, f17, f18
    fmr      f1, f18
    crset    cr1eq
    bl       DrawGsProportionalTextFormat
    addi     r21, r21, 0x1
.L_80084350:
    lwz      r0, 0x3d04(r25)
    cmpw     r21, r0
    blt      .L_8008426C
.L_8008435C:
    lwz      r3, 0x290(r28)
    li       r4, 0xc
    li       r5, 0x5
    bl       SetGsLineWidth
    lis      r3, __vt__7CTexObj@ha
    addi     r0, r3, __vt__7CTexObj@l
    stw      r0, 0x500(r1)
.L_80084378:
    addis    r3, r30, 0x4
    lfs      f0, lbl_8065D058@sda21(r0)
    lfs      f1, 0x3d14(r3)
    fcmpo    cr0, f1, f0
    ble      .L_800848FC
    lis      r3, gGame@ha
    li       r4, 0x0
    addi     r28, r3, gGame@l
    li       r5, 0x0
    lwz      r3, 0x294(r28)
    li       r6, 0x4
    bl       SetGsTevOrder
    lwz      r3, 0x294(r28)
    li       r4, 0x0
    bl       SetGsTevMode
    lwz      r7, 0x294(r28)
    li       r6, 0x1
    lis      r3, gGame@ha
    li       r5, 0x0
    stb      r6, 0x80(r7)
    li       r4, 0x4
    li       r0, 0x1e
    addi     r22, r3, gGame@l
    stw      r6, 0x78(r7)
    lwz      r3, 0x294(r28)
    stw      r5, 0x84(r3)
    stw      r6, 0x88(r3)
    stw      r4, 0x8c(r3)
    stw      r0, 0x90(r3)
    stw      r6, 0x7c(r3)
    lwz      r3, 0x290(r22)
    cmplwi   r3, 0x0
    beq      .L_80084404
    li       r4, 0x0
    bl       ApplyGsRenderMode
.L_80084404:
    lwz      r3, 0x294(r22)
    cmplwi   r3, 0x0
    beq      .L_80084418
    li       r4, 0x0
    bl       ApplyGsTevStage
.L_80084418:
    lwz      r3, 0x298(r22)
    cmplwi   r3, 0x0
    beq      .L_8008442C
    li       r4, 0x0
    bl       ApplyGsChannel
.L_8008442C:
    lfs      f0, lbl_8065D068@sda21(r0)
    addis    r3, r30, 0x4
    lfs      f2, 0x3d14(r3)
    addi     r3, r1, 0x1f4
    fmuls    f17, f25, f0
    lfs      f0, 0x3c94(r29)
    lfs      f5, lbl_8065D0E0@sda21(r0)
    fmr      f15, f2
    lfs      f1, lbl_8065D05C@sda21(r0)
    fmuls    f16, f2, f31
    fadds    f4, f0, f17
    lfs      f0, 0x3ca0(r29)
    lfs      f3, lbl_8065D058@sda21(r0)
    fadds    f2, f1, f0
    fadds    f1, f5, f4
    bl       PSMTXTrans
    lis      r3, gGame@ha
    addi     r4, r1, 0x1f4
    addi     r23, r3, gGame@l
    addi     r5, r1, 0x1c4
    lwz      r3, 0x44(r23)
    addi     r3, r3, 0xe8
    bl       PSMTXConcat
    mr       r21, r30
    addis    r22, r30, 0x4
    li       r24, 0x0
    b        .L_8008450C
.L_80084498:
    lwz      r3, 0xd38(r21)
    lwz      r0, 0x3cec(r22)
    cmpw     r3, r0
    bne      .L_80084504
    subi     r0, r4, 0x1
    lfs      f1, lbl_8065D100@sda21(r0)
    cmpw     r24, r0
    bne      .L_800844C0
    fmr      f0, f16
    b        .L_800844C4
.L_800844C0:
    fmr      f0, f15
.L_800844C4:
    fmuls    f1, f1, f0
    bl       __cvt_fp2unsigned
    li       r0, 0x0
    mr       r8, r3
    stb      r0, 0xbb0(r21)
    li       r0, 0x1
    addi     r3, r21, 0xbac
    mr       r9, r8
    stw      r0, 0xbac(r21)
    mr       r10, r8
    addi     r4, r1, 0x1c4
    li       r5, 0x0
    stw      r8, 0x8(r1)
    li       r6, 0x1e
    li       r7, 0x1
    bl       Draw__7CSpriteFPA4_f11_GXTexMapIDUl13_GXTexMtxTypeUlUlUlUl
.L_80084504:
    addi     r21, r21, 0x2ac
    addi     r24, r24, 0x1
.L_8008450C:
    lwz      r4, 0x3c78(r22)
    cmpw     r24, r4
    blt      .L_80084498
    lfs      f0, 0x3c94(r29)
    addi     r3, r1, 0x1f4
    lfs      f2, 0x3ca0(r29)
    fadds    f1, f0, f17
    lfs      f3, lbl_8065D058@sda21(r0)
    bl       PSMTXTrans
    lwz      r3, 0x44(r23)
    addi     r4, r1, 0x1f4
    addi     r5, r1, 0x1c4
    addi     r3, r3, 0xe8
    bl       PSMTXConcat
    lfd      f19, lbl_8065D048@sda21(r0)
    mr       r25, r30
    addis    r24, r30, 0x4
    li       r22, 0x0
    lis      r27, 0x4330
    b        .L_800848BC
.L_8008455C:
    subi     r0, r3, 0x1
    cmpw     r22, r0
    bne      .L_80084570
    fmr      f1, f16
    b        .L_80084574
.L_80084570:
    fmr      f1, f15
.L_80084574:
    lwz      r5, 0xd34(r25)
    li       r3, 0x0
    stw      r27, 0x510(r1)
    li       r0, 0x1
    clrlwi   r4, r5, 24
    stw      r4, 0x514(r1)
    lfd      f0, 0x510(r1)
    stb      r3, 0xbb0(r25)
    fsubs    f0, f0, f19
    stw      r0, 0xbac(r25)
    fmuls    f0, f0, f1
    lbz      r3, 0xd4c(r25)
    rlwinm   r0, r3, 0, 24, 27
    fctiwz   f0, f0
    cmpwi    r0, 0x20
    stfd     f0, 0x508(r1)
    lwz      r23, 0x50c(r1)
    rlwimi   r23, r5, 0, 0, 23
    beq      .L_800845D0
    bge      .L_8008481C
    cmpwi    r0, 0x10
    beq      .L_800845D0
    b        .L_8008481C
.L_800845D0:
    clrlwi.  r0, r3, 31
    lfs      f17, 0xd3c(r25)
    beq      .L_80084630
    lha      r3, 0xd50(r25)
    lis      r0, 0x4330
    stw      r0, 0x510(r1)
    xoris    r0, r3, 0x8000
    lfd      f1, lbl_8065D050@sda21(r0)
    stw      r0, 0x514(r1)
    lfs      f2, lbl_8065D030@sda21(r0)
    lfd      f0, 0x510(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f2, f0
    bl       sin
    lbz      r3, 0xd4d(r25)
    lis      r0, 0x4330
    stw      r0, 0x508(r1)
    frsp     f2, f1
    lfd      f1, lbl_8065D048@sda21(r0)
    stw      r3, 0x50c(r1)
    lfd      f0, 0x508(r1)
    fsubs    f0, f0, f1
    fmuls    f20, f0, f2
    b        .L_80084634
.L_80084630:
    lfs      f20, lbl_8065D058@sda21(r0)
.L_80084634:
    lbz      r0, 0xd4c(r25)
    rlwinm.  r0, r0, 0, 30, 30
    beq      .L_80084694
    lha      r3, 0xd4e(r25)
    lis      r0, 0x4330
    stw      r0, 0x510(r1)
    xoris    r0, r3, 0x8000
    lfd      f1, lbl_8065D050@sda21(r0)
    stw      r0, 0x514(r1)
    lfs      f2, lbl_8065D030@sda21(r0)
    lfd      f0, 0x510(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f2, f0
    bl       sin
    lbz      r3, 0xd4d(r25)
    lis      r0, 0x4330
    stw      r0, 0x508(r1)
    frsp     f2, f1
    lfd      f1, lbl_8065D048@sda21(r0)
    stw      r3, 0x50c(r1)
    lfd      f0, 0x508(r1)
    fsubs    f0, f0, f1
    fmuls    f21, f0, f2
    b        .L_80084698
.L_80084694:
    lfs      f21, lbl_8065D058@sda21(r0)
.L_80084698:
    lfs      f0, lbl_8065D104@sda21(r0)
    fadds    f22, f0, f17
    fmr      f1, f22
    bl       sin
    frsp     f18, f1
    fmr      f1, f17
    bl       sin
    frsp     f2, f1
    fmuls    f0, f21, f18
    fmr      f1, f22
    fmadds   f18, f20, f2, f0
    bl       cos
    frsp     f22, f1
    fmr      f1, f17
    bl       cos
    fmuls    f0, f21, f22
    lbz      r0, 0xd4c(r25)
    frsp     f1, f1
    lfs      f21, 0xd40(r25)
    rlwinm.  r0, r0, 0, 29, 29
    lfs      f22, 0xd44(r25)
    fmadds   f20, f20, f1, f0
    beq      .L_80084748
    lha      r3, 0xd54(r25)
    lis      r0, 0x4330
    stw      r0, 0x510(r1)
    xoris    r0, r3, 0x8000
    lfd      f1, lbl_8065D050@sda21(r0)
    stw      r0, 0x514(r1)
    lfs      f2, lbl_8065D030@sda21(r0)
    lfd      f0, 0x510(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f2, f0
    bl       sin
    frsp     f1, f1
    li       r0, 0x1
    lfs      f3, lbl_8065D05C@sda21(r0)
    addi     r3, r25, 0xbe8
    lfs      f0, lbl_8065D108@sda21(r0)
    stw      r0, 0xbac(r25)
    fmadds   f0, f0, f1, f3
    fmuls    f1, f21, f0
    fmuls    f2, f22, f0
    bl       PSMTXScale
.L_80084748:
    lbz      r0, 0xd4c(r25)
    rlwinm.  r0, r0, 0, 28, 28
    beq      .L_800847CC
    lha      r3, 0xd52(r25)
    lis      r0, 0x4330
    stw      r0, 0x510(r1)
    xoris    r0, r3, 0x8000
    lfd      f1, lbl_8065D050@sda21(r0)
    stw      r0, 0x514(r1)
    lfs      f2, lbl_8065D030@sda21(r0)
    lfd      f0, 0x510(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f2, f0
    bl       sin
    frsp     f2, f1
    lfs      f0, lbl_8065D10C@sda21(r0)
    li       r0, 0x1
    lfs      f1, lbl_8065D058@sda21(r0)
    stw      r0, 0xbac(r25)
    addi     r3, r25, 0xc18
    fmuls    f21, f0, f2
    li       r4, 0x78
    bl       PSMTXRotRad
    lfs      f1, lbl_8065D058@sda21(r0)
    addi     r3, r25, 0xc48
    li       r4, 0x79
    bl       PSMTXRotRad
    lfs      f0, lbl_8065D068@sda21(r0)
    addi     r3, r25, 0xc78
    li       r4, 0x7a
    fnmsubs  f0, f21, f0, f17
    fadds    f1, f21, f0
    bl       PSMTXRotRad
.L_800847CC:
    fmr      f1, f18
    lfs      f3, lbl_8065D058@sda21(r0)
    fmr      f2, f20
    addi     r3, r1, 0x194
    bl       PSMTXTrans
    addi     r3, r1, 0x1c4
    addi     r4, r1, 0x194
    addi     r5, r1, 0x164
    bl       PSMTXConcat
    stw      r23, 0x8(r1)
    mr       r8, r23
    mr       r9, r23
    mr       r10, r23
    addi     r3, r25, 0xbac
    addi     r4, r1, 0x164
    li       r5, 0x0
    li       r6, 0x1e
    li       r7, 0x1
    bl       Draw__7CSpriteFPA4_f11_GXTexMapIDUl13_GXTexMtxTypeUlUlUlUl
    b        .L_800848B4
.L_8008481C:
    lfs      f1, 0xd3c(r25)
    lfs      f0, lbl_8065D058@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_8008488C
    bl       sin
    lfs      f4, 0xd48(r25)
    frsp     f2, f1
    lfs      f3, lbl_8065D058@sda21(r0)
    addi     r3, r1, 0x134
    fneg     f0, f4
    fmr      f1, f4
    fmuls    f2, f0, f2
    bl       PSMTXTrans
    addi     r3, r1, 0x1c4
    addi     r4, r1, 0x134
    addi     r5, r1, 0x104
    bl       PSMTXConcat
    stw      r23, 0x8(r1)
    mr       r8, r23
    mr       r9, r23
    mr       r10, r23
    addi     r3, r25, 0xbac
    addi     r4, r1, 0x104
    li       r5, 0x0
    li       r6, 0x1e
    li       r7, 0x1
    bl       Draw__7CSpriteFPA4_f11_GXTexMapIDUl13_GXTexMtxTypeUlUlUlUl
    b        .L_800848B4
.L_8008488C:
    stw      r23, 0x8(r1)
    mr       r8, r23
    mr       r9, r23
    mr       r10, r23
    addi     r3, r25, 0xbac
    addi     r4, r1, 0x1c4
    li       r5, 0x0
    li       r6, 0x1e
    li       r7, 0x1
    bl       Draw__7CSpriteFPA4_f11_GXTexMapIDUl13_GXTexMtxTypeUlUlUlUl
.L_800848B4:
    addi     r25, r25, 0x2ac
    addi     r22, r22, 0x1
.L_800848BC:
    lwz      r3, 0x3c78(r24)
    cmpw     r22, r3
    blt      .L_8008455C
    lwz      r3, 0x294(r28)
    li       r4, 0xff
    li       r5, 0xff
    li       r6, 0x4
    bl       SetGsTevOrder
    lwz      r3, 0x294(r28)
    li       r4, 0x4
    bl       SetGsTevMode
    lwz      r4, 0x294(r28)
    li       r3, 0x0
    li       r0, 0x1
    stb      r3, 0x80(r4)
    stw      r0, 0x78(r4)
.L_800848FC:
    lbz      r0, 0x4(r30)
    extrwi.  r0, r0, 1, 27
    beq      .L_80084BDC
    addis    r3, r30, 0x4
    lwz      r0, 0x3c6c(r3)
    cmpwi    r0, 0x0
    blt      .L_80084BDC
    lis      r3, gGame@ha
    li       r4, 0x0
    addi     r27, r3, gGame@l
    li       r5, 0x0
    lwz      r3, 0x294(r27)
    li       r6, 0x4
    bl       SetGsTevOrder
    lwz      r3, 0x294(r27)
    li       r4, 0x0
    bl       SetGsTevMode
    lwz      r7, 0x294(r27)
    li       r6, 0x1
    li       r5, 0x0
    li       r4, 0x4
    stb      r6, 0x80(r7)
    li       r0, 0x1e
    addis    r3, r30, 0x4
    stw      r6, 0x78(r7)
    lwz      r7, 0x294(r27)
    stw      r5, 0x84(r7)
    stw      r6, 0x88(r7)
    stw      r4, 0x8c(r7)
    stw      r0, 0x90(r7)
    stw      r6, 0x7c(r7)
    lwz      r0, 0x3c6c(r3)
    cmpwi    r0, 0x2
    blt      .L_80084988
    li       r0, 0x2
.L_80084988:
    slwi     r3, r0, 8
    addis    r22, r3, 0x4
    addi     r22, r22, 0x376c
    add      r22, r30, r22
    mr       r3, r22
    bl       GXGetTexObjWidth
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x514(r1)
    mr       r3, r22
    lfd      f1, lbl_8065D048@sda21(r0)
    stw      r0, 0x510(r1)
    lfd      f0, 0x510(r1)
    fsubs    f15, f0, f1
    bl       GXGetTexObjHeight
    clrlwi   r0, r3, 16
    lis      r4, 0x4330
    stw      r0, 0x50c(r1)
    addis    r3, r30, 0x4
    lwz      r0, 0x3c6c(r3)
    stw      r4, 0x508(r1)
    lfd      f1, lbl_8065D048@sda21(r0)
    cmpwi    r0, 0x2
    lfd      f0, 0x508(r1)
    fsubs    f16, f0, f1
    blt      .L_80084A94
    mr       r3, r22
    bl       GXGetTexObjWidth
    lis      r4, 0x2e8c
    clrlwi   r3, r3, 16
    subi     r4, r4, 0x5d17
    lis      r0, 0x4330
    mulhw    r4, r4, r3
    lfs      f2, lbl_8065D05C@sda21(r0)
    stw      r0, 0x510(r1)
    addi     r3, r22, 0x6c
    lfd      f4, lbl_8065D050@sda21(r0)
    fmr      f3, f2
    srawi    r0, r4, 1
    lfs      f1, lbl_8065D110@sda21(r0)
    srwi     r4, r0, 31
    add      r0, r0, r4
    xoris    r0, r0, 0x8000
    stw      r0, 0x514(r1)
    lfd      f0, 0x510(r1)
    fsubs    f15, f0, f4
    bl       PSMTXScale
    li       r3, 0x1
    lis      r0, 0x4330
    stw      r3, 0x38(r22)
    addis    r4, r30, 0x4
    lfs      f2, lbl_8065D058@sda21(r0)
    addi     r3, r22, 0x3c
    lwz      r4, 0x3c6c(r4)
    stw      r0, 0x508(r1)
    fmr      f3, f2
    subi     r0, r4, 0x2
    lfd      f1, lbl_8065D050@sda21(r0)
    xoris    r0, r0, 0x8000
    lfs      f4, lbl_8065D110@sda21(r0)
    stw      r0, 0x50c(r1)
    lfd      f0, 0x508(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f4, f0
    bl       PSMTXTrans
    li       r0, 0x1
    stw      r0, 0x38(r22)
.L_80084A94:
    addis    r5, r30, 0x4
    lfs      f1, lbl_8065D0BC@sda21(r0)
    lwz      r0, 0x3c78(r5)
    lfs      f0, 0x3ca0(r29)
    mulli    r0, r0, 0x2ac
    lfs      f5, lbl_8065D068@sda21(r0)
    lfs      f2, 0x3c94(r29)
    fadds    f0, f1, f0
    lwz      r4, 0x0(r30)
    add      r3, r30, r0
    lwz      r0, 0xa8c(r3)
    fmadds   f1, f25, f5, f2
    lfs      f2, 0x3c90(r5)
    slwi     r0, r0, 2
    lfs      f3, lbl_8065D0F8@sda21(r0)
    add      r3, r5, r0
    fadds    f4, f2, f1
    lfs      f1, 0x3ca8(r3)
    add      r3, r4, r0
    lfs      f2, 0x858(r3)
    fadds    f0, f1, f0
    lfs      f1, 0x3c70(r5)
    fmadds   f17, f15, f5, f4
    fmadds   f0, f2, f5, f0
    fnmsubs  f18, f16, f3, f0
    bl       sin
    frsp     f1, f1
    lfs      f0, lbl_8065D058@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_80084B10
    fmr      f1, f0
.L_80084B10:
    lfs      f0, lbl_8065D0F4@sda21(r0)
    fmuls    f1, f0, f1
    bl       __cvt_fp2unsigned
    lfs      f3, lbl_8065D058@sda21(r0)
    mr       r5, r3
    lfs      f0, lbl_8065D0E0@sda21(r0)
    lis      r3, gGame@ha
    fmr      f4, f15
    li       r0, -0x100
    fmr      f5, f16
    addi     r3, r3, gGame@l
    fmr      f6, f3
    mr       r4, r22
    fmr      f7, f3
    mr       r6, r5
    fmr      f8, f3
    mr       r7, r5
    fadds    f1, f0, f17
    mr       r8, r5
    fadds    f2, f0, f18
    or       r21, r5, r0
    bl       DrawGsTexturedQuad
    lfs      f3, lbl_8065D058@sda21(r0)
    lis      r3, gGame@ha
    fmr      f1, f17
    addi     r3, r3, gGame@l
    fmr      f2, f18
    mr       r4, r22
    fmr      f4, f15
    mr       r5, r21
    fmr      f5, f16
    mr       r6, r21
    fmr      f6, f3
    mr       r7, r21
    fmr      f7, f3
    mr       r8, r21
    fmr      f8, f3
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r27)
    li       r4, 0xff
    li       r5, 0xff
    li       r6, 0x4
    bl       SetGsTevOrder
    lwz      r3, 0x294(r27)
    li       r4, 0x4
    bl       SetGsTevMode
    lwz      r4, 0x294(r27)
    li       r3, 0x0
    li       r0, 0x1
    stb      r3, 0x80(r4)
    stw      r0, 0x78(r4)
.L_80084BDC:
    psq_l    f31, 0x658(r1), 0, qr0
    lfd      f31, 0x650(r1)
    psq_l    f30, 0x648(r1), 0, qr0
    lfd      f30, 0x640(r1)
    psq_l    f29, 0x638(r1), 0, qr0
    lfd      f29, 0x630(r1)
    psq_l    f28, 0x628(r1), 0, qr0
    lfd      f28, 0x620(r1)
    psq_l    f27, 0x618(r1), 0, qr0
    lfd      f27, 0x610(r1)
    psq_l    f26, 0x608(r1), 0, qr0
    lfd      f26, 0x600(r1)
    psq_l    f25, 0x5f8(r1), 0, qr0
    lfd      f25, 0x5f0(r1)
    psq_l    f24, 0x5e8(r1), 0, qr0
    lfd      f24, 0x5e0(r1)
    psq_l    f23, 0x5d8(r1), 0, qr0
    lfd      f23, 0x5d0(r1)
    psq_l    f22, 0x5c8(r1), 0, qr0
    lfd      f22, 0x5c0(r1)
    psq_l    f21, 0x5b8(r1), 0, qr0
    lfd      f21, 0x5b0(r1)
    psq_l    f20, 0x5a8(r1), 0, qr0
    lfd      f20, 0x5a0(r1)
    psq_l    f19, 0x598(r1), 0, qr0
    lfd      f19, 0x590(r1)
    psq_l    f18, 0x588(r1), 0, qr0
    lfd      f18, 0x580(r1)
    psq_l    f17, 0x578(r1), 0, qr0
    lfd      f17, 0x570(r1)
    psq_l    f16, 0x568(r1), 0, qr0
    lfd      f16, 0x560(r1)
    psq_l    f15, 0x558(r1), 0, qr0
    lfd      f15, 0x550(r1)
    lmw      r21, 0x524(r1)
    lwz      r0, 0x664(r1)
    mtlr     r0
    addi     r1, r1, 0x660
    blr
*/

/**
 * @note Address: 0x80084C78
 * @note Size: 0x148
 */
// a CJObj that starts out holding a jobj's current transform
#if defined(VERSION_JP)
#endif
CJObj::CJObj(HSD_JObj* jobj)
{
	Attach(jobj);
}
#if defined(VERSION_JP)
#endif

#if defined(VERSION_PAL)
#define MSG_FADE_RATE (1.0f / (0.3f * gVideoFramesPerSecond))
#else
#define MSG_FADE_RATE (1.0f / 18.0f)
#endif

/**
 * @note Address: 0x80084DC0
 * @note Size: 0x684
 */
extern "C" void CMsgFont_Update(CMsgFont* font, u32 step)
{
	if (font->mActive) {
		if (font->mFadeIn && font->mAlpha < 1.0f) {
			if (font->mColor.a == 0 && font->mMode < 0) {
				font->mAlpha  = 1.0f;
				font->mFadeIn = FALSE;
			}
			f32 alpha    = font->mAlpha + MSG_FADE_RATE * step;
			font->mAlpha = alpha;
			if (alpha >= 1.0f) {
				font->mAlpha  = 1.0f;
				font->mFadeIn = FALSE;
			}
		}
		if (font->mFadeOut) {
			if (font->mAlpha > 0.0f) {
				f32 alpha    = font->mAlpha - MSG_FADE_RATE * step;
				font->mAlpha = alpha;
				if (alpha <= 0.0f) {
					font->mAlpha   = 0.0f;
					font->mFadeOut = FALSE;
					font->mActive  = FALSE;
				}
			}
			font->mTextAlpha = font->mAlpha;
		}
		if (font->mSelectedLine >= 0 || font->mOtherSelection >= 0) {
			if (font->mHighlightAlpha < 1.0f) {
				f32 alpha             = font->mHighlightAlpha + MSG_FADE_RATE * step;
				font->mHighlightAlpha = alpha;
				if (alpha >= 1.0f) {
					font->mHighlightAlpha = 1.0f;
				}
			}
		} else if (font->mHighlightAlpha > 0.0f) {
			f32 alpha             = font->mHighlightAlpha - MSG_FADE_RATE * step;
			font->mHighlightAlpha = alpha;
			if (alpha <= 0.0f) {
				font->mHighlightAlpha = 0.0f;
			}
		}
		if (font->mSelectedLine >= 0) {
			f32 width = font->mFitLine ? font->mData->mWidths[font->mSelectedLine] : font->mData->mMaxWidth;
			font->mCurLineY += 0.8f * (font->mLineY[font->mSelectedLine] - font->mCurLineY);
			font->mCurLineWidth += 0.7f * (width - font->mCurLineWidth);
			font->mTexOffset -= 0.008f * step;
			PSMTXTrans(font->mTexs.mTex[MSG_TEX_LINE].mMtx[0], font->mTexOffset, 0.0f, 0.0f);
			font->mTexs.mTex[MSG_TEX_LINE].mMtxDirty = TRUE;
		}
		if (font->mBlink) {
			font->mBlinkPhase += 0.104719758f * step;
			font->mBlinkPhase = fmod(font->mBlinkPhase, (f64)PI);
		}
	}

	if (font->mActive) {
		for (s32 i = 0; i < 400; i++) {
			MsgFontGlyph& glyph = font->mGlyphs[i];
			switch (glyph.mEffect & 0xF0) {
			case 0x10:
				// this one wraps as an int before going to float
				glyph.mPhaseY = (s16)(f32)fmod((f32)(glyph.mPhaseY + glyph.mSpeed * step), 360.0);
				glyph.mPhaseX = (s16)(f32)fmod(glyph.mPhaseX + (0.5f * glyph.mSpeed) * step, 360.0);
				glyph.mPhaseZ = (s16)(f32)fmod(glyph.mPhaseZ + (0.7f * glyph.mSpeed) * step, 360.0);
				glyph.mPhaseW = (s16)(f32)fmod(glyph.mPhaseW + (0.8f * glyph.mSpeed) * step, 360.0);
				break;
			case 0x20:
				if (--glyph.mRefresh <= 0) {
					glyph.mPhaseY  = rand() % 360;
					glyph.mPhaseX  = rand() % 360;
					glyph.mPhaseZ  = rand() % 360;
					glyph.mPhaseW  = rand() % 360;
					glyph.mRefresh = glyph.mSpeed;
				}
				break;
			}
		}
	}
	font->mScrollX += 0.01f * step;
	font->mScrollY += 0.01f * step;
	PSMTXTrans(font->mFrame.mTex.mMtx[0], font->mScrollX, font->mScrollY, 0.0f);
	font->mFrame.mTex.mMtxDirty = TRUE;
	if (font->mMode >= 0 && font->mMode < 1 && (font->mCursor[0].mCursor.mStatus.mFlags & 1)) {
		font->mCursor[0].mCursor.UpdateObject(step);
	}
}

/**
 * @note Address: 0x80085444
 * @note Size: 0x6F0
 */
extern "C" void CMsgFont_AddChar(CMsgFont* font, const char* text)
{
	f32 width = MeasureGameTextWidth(&gGame.mGs, text, font->_04_04, font->mSize);
	f32 spacing;
#if defined(VERSION_JP)
	spacing = (font->mSize - font->mPrevSize) * 0.5f;
#else
	if (__fpclassifyf(font->mPrevSize) == FP_NAN) {
		spacing = 0.0f;
	} else {
		spacing = (font->mSize - font->mPrevSize) * 0.5f;
	}
#endif
	s32 textureWidth;
	if ((u8)text[0] == 17) {
#if defined(VERSION_JP)
		TPLPalette* palette = (TPLPalette*)(&gGame.mTpl)->mBuffer;
		CTexObj* texture    = &(&gGame.mTpl)->mTex;
		CTexObj icon        = texture->Init(&palette->descriptorArray[117], 0);
		PSMTXScale(icon.mMtx[1], 1.0f / 14.0f, 1.0f, 1.0f);
		icon.mMtxDirty = TRUE;
		PSMTXTrans(icon.mMtx[0], (1.0f / 14.0f) * (u8)text[1], 0.0f, 0.0f);
		icon.mMtxDirty = TRUE;
		textureWidth   = (s32)font->mGlyphs[font->mCharCount].mTexture.Set(icon) / 14;
#else
		TPLPalette* palette = (TPLPalette*)(&gGame.mTpl)->mBuffer;
		CTexObj* texture    = &(&gGame.mTpl)->mTex;
		CTexObj icon        = texture->Init(&palette->descriptorArray[119], 0);
		s32 index           = (u8)text[1];
		s32 columns         = font->mData->mIconColumns;
		if (index >= columns) {
			index = columns - 1;
		}
		f32 cellWidth = 1.0f / columns;
		PSMTXScale(icon.mMtx[1], cellWidth, 1.0f, 1.0f);
		icon.mMtxDirty = TRUE;
		PSMTXTrans(icon.mMtx[0], cellWidth * index, 0.0f, 0.0f);
		icon.mMtxDirty = TRUE;
		textureWidth   = (s32)font->mGlyphs[font->mCharCount].mTexture.Set(icon) / columns;
#endif
		width = textureWidth * (font->mSize / 22.0f);
	} else {
		textureWidth = font->mGlyphs[font->mCharCount].mSprite.mTex->SetChar(text);
	}
	f32 x = (width - textureWidth) * 0.5f + (11.0f + font->mCursorX + spacing);
	f32 y = font->mCursorY + font->mData->mHeights[font->mLine] * 0.5f;
#if defined(VERSION_JP)
	if ((u8)text[0] == 17) {
		font->mGlyphs[font->mCharCount].mSprite.SetTrans(x + 0.08f * textureWidth, y, 0.0f);
	} else {
		font->mGlyphs[font->mCharCount].mSprite.SetTrans(x, y, 0.0f);
	}
#else
	font->mGlyphs[font->mCharCount].mSprite.SetTrans(x, y, 0.0f);
#endif
	f32 sx, sy;
	if (font->mGlyphScaleX >= 0.0f && font->mGlyphScaleY >= 0.0f) {
		sx = font->mGlyphScaleX;
		sy = font->mGlyphScaleY;
	} else {
		f32 min = 100.0f * font->mGlyphScaleX;
		if (min < 0.0f)
			min = -min;
		f32 max = 100.0f * font->mGlyphScaleY;
		if (max < 0.0f)
			max = -max;
		f32 value = fmod((f32)rand(), max - min);
		f32 scale = font->mGlyphScaleX;
		if (scale < 0.0f)
			scale = -scale;
		sx = sy = scale + value / 100.0f;
	}
	f32 scaleX = (font->mSize * 0.5f) * sx;
	f32 scaleY = (-font->mSize * 0.5f) * sy;
	font->mGlyphs[font->mCharCount].mSprite.SetScale(scaleX, scaleY, 1.0f);
	f32 angle = font->mGlyphAngle;
	if (!(angle >= 0.0f)) {
		f32 range = 2.0f * angle;
		if (range < 0.0f)
			range = -range;
		f32 value = fmod((f32)rand(), RAD2DEG * range);
		angle     = DEG2RAD * value + font->mGlyphAngle;
	}
	font->mGlyphs[font->mCharCount].mSprite.SetRot(0.0f, 0.0f, angle);
	font->mGlyphs[font->mCharCount].mColor  = font->mGlyphColor;
	font->mGlyphs[font->mCharCount].mLine   = font->mLine;
	font->mGlyphs[font->mCharCount].mAngle  = angle;
	font->mGlyphs[font->mCharCount].mScaleX = scaleX;
	font->mGlyphs[font->mCharCount].mScaleY = scaleY;
#if !defined(VERSION_JP)
	if (!(angle <= PI)) {
		angle = TWO_PI - angle;
	}
	font->mGlyphs[font->mCharCount].mWidthAdjust = (angle / PI) * (width - font->mSize);
#endif
	font->mGlyphs[font->mCharCount].mEffect = font->mEffect;
	font->mGlyphs[font->mCharCount].mAmount = font->mAmount;
	font->mGlyphs[font->mCharCount].mPhaseZ = rand() % 360;
	font->mGlyphs[font->mCharCount].mPhaseW = rand() % 360;
	font->mGlyphs[font->mCharCount].mSpeed = font->mGlyphs[font->mCharCount].mRefresh = font->mSpeed;
	font->mCursorX += width + spacing;
	font->mPrevSize = font->mSize;
	font->mCharCount++;
}
/*
    stwu     r1, -0x190(r1)
    mflr     r0
    stw      r0, 0x194(r1)
    stfd     f31, 0x180(r1)
    psq_st   f31, 0x188(r1), 0, qr0
    stfd     f30, 0x170(r1)
    psq_st   f30, 0x178(r1), 0, qr0
    stfd     f29, 0x160(r1)
    psq_st   f29, 0x168(r1), 0, qr0
    stfd     f28, 0x150(r1)
    psq_st   f28, 0x158(r1), 0, qr0
    stfd     f27, 0x140(r1)
    psq_st   f27, 0x148(r1), 0, qr0
    stw      r31, 0x13c(r1)
    stw      r30, 0x138(r1)
    stw      r29, 0x134(r1)
    stw      r28, 0x130(r1)
    mr       r29, r3
    lis      r3, gGame@ha
    addis    r5, r29, 0x4
    lbz      r0, 0x4(r29)
    lfs      f1, 0x3cd0(r5)
    addi     r3, r3, gGame@l
    mr       r30, r4
    extrwi   r5, r0, 1, 29
    bl       MeasureGameTextWidth
    addis    r3, r29, 0x4
    lis      r0, 0x7f80
    lfs      f0, 0x3cd4(r3)
    fmr      f30, f1
    stfs     f0, 0x8(r1)
    lwz      r4, 0x8(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_800854E0
    bge      .L_80085510
    cmpwi    r3, 0x0
    beq      .L_800854F8
    b        .L_80085510
.L_800854E0:
    clrlwi.  r0, r4, 9
    beq      .L_800854F0
    li       r0, 0x1
    b        .L_80085514
.L_800854F0:
    li       r0, 0x2
    b        .L_80085514
.L_800854F8:
    clrlwi.  r0, r4, 9
    beq      .L_80085508
    li       r0, 0x5
    b        .L_80085514
.L_80085508:
    li       r0, 0x3
    b        .L_80085514
.L_80085510:
    li       r0, 0x4
.L_80085514:
    cmpwi    r0, 0x1
    bne      .L_80085524
    lfs      f31, lbl_8065D058@sda21(r0)
    b        .L_8008553C
.L_80085524:
    addis    r3, r29, 0x4
    lfs      f0, lbl_8065D068@sda21(r0)
    lfs      f2, 0x3cd0(r3)
    lfs      f1, 0x3cd4(r3)
    fsubs    f1, f2, f1
    fmuls    f31, f1, f0
.L_8008553C:
    lbz      r0, 0x0(r30)
    cmplwi   r0, 0x11
    bne      .L_80085700
    lis      r3, gGame@ha
    li       r5, 0x0
    addi     r3, r3, gGame@l
    lwz      r4, 0x3d4c(r3)
    addi     r3, r3, 0x3d6c
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x3b8
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r5, 0x0(r3)
    lis      r4, __vt__7CTexObj@ha
    lwz      r7, 0x4(r3)
    addi     r31, r1, 0x48
    addi     r6, r4, __vt__7CTexObj@l
    li       r0, 0x18
    stw      r5, 0xc(r1)
    subi     r5, r31, 0x4
    addi     r4, r3, 0x38
    stw      r7, 0x10(r1)
    lwz      r8, 0x8(r3)
    lwz      r7, 0xc(r3)
    stw      r8, 0x14(r1)
    stw      r7, 0x18(r1)
    lwz      r8, 0x10(r3)
    lwz      r7, 0x14(r3)
    stw      r8, 0x1c(r1)
    stw      r7, 0x20(r1)
    lwz      r8, 0x18(r3)
    lwz      r7, 0x1c(r3)
    stw      r8, 0x24(r1)
    stw      r7, 0x28(r1)
    stw      r6, 0x108(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x2c(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x30(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x34(r1)
    stw      r6, 0x38(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x3c(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x40(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x44(r1)
    mtctr    r0
.L_80085600:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_80085600
    lwz      r3, 0x0(r29)
    lbz      r28, 0x1(r30)
    lwz      r30, 0x894(r3)
    cmpw     r28, r30
    blt      .L_8008562C
    subi     r28, r30, 0x1
.L_8008562C:
    xoris    r3, r30, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x114(r1)
    addi     r3, r1, 0x78
    lfs      f2, lbl_8065D05C@sda21(r0)
    stw      r0, 0x110(r1)
    lfd      f1, lbl_8065D050@sda21(r0)
    fmr      f3, f2
    lfd      f0, 0x110(r1)
    fsubs    f0, f0, f1
    fdivs    f29, f2, f0
    fmr      f1, f29
    bl       PSMTXScale
    xoris    r3, r28, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x11c(r1)
    li       r4, 0x1
    lfs      f2, lbl_8065D058@sda21(r0)
    mr       r3, r31
    stw      r0, 0x118(r1)
    lfd      f1, lbl_8065D050@sda21(r0)
    fmr      f3, f2
    lfd      f0, 0x118(r1)
    stw      r4, 0x44(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f29, f0
    bl       PSMTXTrans
    li       r0, 0x1
    addis    r3, r29, 0x4
    stw      r0, 0x44(r1)
    addi     r4, r1, 0xc
    lwz      r0, 0x3c78(r3)
    mulli    r3, r0, 0x2ac
    addi     r3, r3, 0xaac
    add      r3, r29, r3
    bl       Set__7CTexObjFRC7CTexObj
    divw     r3, r3, r30
    addis    r4, r29, 0x4
    lis      r5, 0x4330
    lfs      f1, 0x3cd0(r4)
    lfs      f0, lbl_8065D06C@sda21(r0)
    lis      r4, __vt__7CTexObj@ha
    xoris    r0, r3, 0x8000
    fdivs    f0, f1, f0
    stw      r0, 0x124(r1)
    addi     r0, r4, __vt__7CTexObj@l
    lfd      f2, lbl_8065D050@sda21(r0)
    stw      r5, 0x120(r1)
    lfd      f1, 0x120(r1)
    fsubs    f1, f1, f2
    stw      r0, 0x108(r1)
    fmuls    f30, f1, f0
    b        .L_8008571C
.L_80085700:
    addis    r3, r29, 0x4
    mr       r4, r30
    lwz      r0, 0x3c78(r3)
    mulli    r3, r0, 0x2ac
    addi     r0, r3, 0xd14
    lwzx     r3, r29, r0
    bl       SetChar__7CTexObjFPCc
.L_8008571C:
    xoris    r0, r3, 0x8000
    lis      r4, 0x4330
    stw      r0, 0x124(r1)
    addis    r5, r29, 0x4
    lwz      r3, 0x3c7c(r5)
    li       r0, 0x1
    stw      r4, 0x120(r1)
    lfd      f2, lbl_8065D050@sda21(r0)
    slwi     r3, r3, 2
    lfd      f0, 0x120(r1)
    lwz      r4, 0x0(r29)
    fsubs    f2, f0, f2
    lfs      f1, lbl_8065D144@sda21(r0)
    lfs      f0, 0x3c90(r5)
    add      r4, r4, r3
    lwz      r3, 0x3c78(r5)
    fadds    f0, f1, f0
    mulli    r3, r3, 0x2ac
    fsubs    f2, f30, f2
    lfs      f4, lbl_8065D068@sda21(r0)
    fadds    f1, f0, f31
    lfs      f0, 0x3c9c(r5)
    fmuls    f3, f2, f4
    lfs      f2, 0x858(r4)
    addi     r3, r3, 0xbac
    add      r3, r29, r3
    fadds    f1, f3, f1
    stw      r0, 0x0(r3)
    fmadds   f2, f2, f4, f0
    lfs      f3, lbl_8065D058@sda21(r0)
    addi     r3, r3, 0xc
    bl       PSMTXTrans
    addis    r3, r29, 0x4
    lfs      f0, lbl_8065D058@sda21(r0)
    lfs      f2, 0x3ce0(r3)
    fcmpo    cr0, f2, f0
    cror     eq, gt, eq
    bne      .L_800857D0
    lfs      f1, 0x3ce4(r3)
    fcmpo    cr0, f1, f0
    cror     eq, gt, eq
    bne      .L_800857D0
    fmr      f4, f2
    fmr      f5, f1
    b        .L_8008585C
.L_800857D0:
    lfs      f1, lbl_8065D034@sda21(r0)
    lfs      f0, lbl_8065D058@sda21(r0)
    fmuls    f29, f1, f2
    fcmpo    cr0, f29, f0
    bge      .L_800857E8
    fneg     f29, f29
.L_800857E8:
    addis    r3, r29, 0x4
    lfs      f2, lbl_8065D034@sda21(r0)
    lfs      f1, 0x3ce4(r3)
    lfs      f0, lbl_8065D058@sda21(r0)
    fmuls    f28, f2, f1
    fcmpo    cr0, f28, f0
    bge      .L_80085808
    fneg     f28, f28
.L_80085808:
    bl       rand
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x124(r1)
    fsubs    f2, f28, f29
    lfd      f1, lbl_8065D050@sda21(r0)
    stw      r0, 0x120(r1)
    lfd      f0, 0x120(r1)
    fsubs    f1, f0, f1
    bl       fmod
    addis    r3, r29, 0x4
    lfs      f0, lbl_8065D058@sda21(r0)
    lfs      f2, 0x3ce0(r3)
    frsp     f1, f1
    fcmpo    cr0, f2, f0
    bge      .L_8008584C
    fneg     f2, f2
.L_8008584C:
    lfs      f0, lbl_8065D034@sda21(r0)
    fdivs    f0, f1, f0
    fadds    f5, f2, f0
    fmr      f4, f5
.L_8008585C:
    addis    r3, r29, 0x4
    lfs      f2, lbl_8065D068@sda21(r0)
    lfs      f1, 0x3cd0(r3)
    li       r0, 0x1
    lwz      r3, 0x3c78(r3)
    fneg     f0, f1
    lfs      f3, lbl_8065D05C@sda21(r0)
    fmuls    f1, f1, f2
    mulli    r3, r3, 0x2ac
    fmuls    f0, f0, f2
    fmuls    f29, f1, f4
    addi     r3, r3, 0xbac
    add      r3, r29, r3
    fmuls    f27, f0, f5
    stw      r0, 0x0(r3)
    fmr      f1, f29
    addi     r3, r3, 0x3c
    fmr      f2, f27
    bl       PSMTXScale
    addis    r3, r29, 0x4
    lfs      f1, lbl_8065D058@sda21(r0)
    lfs      f28, 0x3cdc(r3)
    fcmpo    cr0, f28, f1
    cror     eq, gt, eq
    bne      .L_800858C4
    b        .L_80085918
.L_800858C4:
    lfs      f0, lbl_8065D0E0@sda21(r0)
    fmuls    f28, f0, f28
    fcmpo    cr0, f28, f1
    bge      .L_800858D8
    fneg     f28, f28
.L_800858D8:
    bl       rand
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    lfs      f0, lbl_8065D148@sda21(r0)
    stw      r3, 0x124(r1)
    lfd      f1, lbl_8065D050@sda21(r0)
    fmuls    f2, f0, f28
    stw      r0, 0x120(r1)
    lfd      f0, 0x120(r1)
    fsubs    f1, f0, f1
    bl       fmod
    addis    r3, r29, 0x4
    frsp     f2, f1
    lfs      f1, lbl_8065D030@sda21(r0)
    lfs      f0, 0x3cdc(r3)
    fmadds   f28, f1, f2, f0
.L_80085918:
    addis    r3, r29, 0x4
    li       r0, 0x1
    lwz      r3, 0x3c78(r3)
    li       r4, 0x78
    lfs      f1, lbl_8065D058@sda21(r0)
    mulli    r3, r3, 0x2ac
    addi     r30, r3, 0xbac
    add      r30, r29, r30
    stw      r0, 0x0(r30)
    addi     r3, r30, 0x6c
    bl       PSMTXRotRad
    lfs      f1, lbl_8065D058@sda21(r0)
    addi     r3, r30, 0x9c
    li       r4, 0x79
    bl       PSMTXRotRad
    fmr      f1, f28
    addi     r3, r30, 0xcc
    li       r4, 0x7a
    bl       PSMTXRotRad
    addis    r5, r29, 0x4
    lfs      f0, lbl_8065D150@sda21(r0)
    lwz      r0, 0x3c78(r5)
    lwz      r4, 0x3cd8(r5)
    fcmpo    cr0, f28, f0
    mulli    r0, r0, 0x2ac
    add      r3, r29, r0
    stw      r4, 0xd34(r3)
    lwz      r0, 0x3c78(r5)
    lwz      r4, 0x3c7c(r5)
    mulli    r0, r0, 0x2ac
    add      r3, r29, r0
    stw      r4, 0xd38(r3)
    lwz      r0, 0x3c78(r5)
    mulli    r0, r0, 0x2ac
    add      r3, r29, r0
    stfs     f28, 0xd3c(r3)
    lwz      r0, 0x3c78(r5)
    mulli    r0, r0, 0x2ac
    add      r3, r29, r0
    stfs     f29, 0xd40(r3)
    lwz      r0, 0x3c78(r5)
    mulli    r0, r0, 0x2ac
    add      r3, r29, r0
    stfs     f27, 0xd44(r3)
    cror     eq, lt, eq
    bne      .L_800859D4
    b        .L_800859DC
.L_800859D4:
    lfs      f0, lbl_8065D14C@sda21(r0)
    fsubs    f28, f0, f28
.L_800859DC:
    lfs      f1, lbl_8065D150@sda21(r0)
    addis    r5, r29, 0x4
    lfs      f0, 0x3cd0(r5)
    fdivs    f1, f28, f1
    lwz      r0, 0x3c78(r5)
    mulli    r0, r0, 0x2ac
    add      r3, r29, r0
    fsubs    f0, f30, f0
    fmuls    f0, f1, f0
    stfs     f0, 0xd48(r3)
    lwz      r0, 0x3c78(r5)
    lbz      r4, 0x3ce8(r5)
    mulli    r0, r0, 0x2ac
    add      r3, r29, r0
    stb      r4, 0xd4c(r3)
    lwz      r0, 0x3c78(r5)
    lbz      r4, 0x3ce9(r5)
    mulli    r0, r0, 0x2ac
    add      r3, r29, r0
    stb      r4, 0xd4d(r3)
    bl       rand
    lis      r5, 0xb60b
    addis    r4, r29, 0x4
    addi     r5, r5, 0x60b7
    lwz      r0, 0x3c78(r4)
    mulhw    r4, r5, r3
    add      r4, r4, r3
    srawi    r4, r4, 8
    srwi     r5, r4, 31
    add      r4, r4, r5
    mulli    r4, r4, 0x168
    mulli    r0, r0, 0x2ac
    subf     r4, r4, r3
    add      r3, r29, r0
    sth      r4, 0xd52(r3)
    bl       rand
    lis      r4, 0xb60b
    addis    r5, r29, 0x4
    addi     r4, r4, 0x60b7
    lwz      r0, 0x3c78(r5)
    mulhw    r4, r4, r3
    fadds    f0, f30, f31
    add      r4, r4, r3
    srawi    r4, r4, 8
    srwi     r6, r4, 31
    add      r4, r4, r6
    mulli    r4, r4, 0x168
    mulli    r0, r0, 0x2ac
    subf     r4, r4, r3
    add      r3, r29, r0
    sth      r4, 0xd54(r3)
    lwz      r0, 0x3c78(r5)
    lbz      r4, 0x3cea(r5)
    mulli    r0, r0, 0x2ac
    add      r3, r29, r0
    stb      r4, 0xd57(r3)
    lwz      r0, 0x3c78(r5)
    mulli    r0, r0, 0x2ac
    add      r3, r29, r0
    stb      r4, 0xd56(r3)
    lfs      f1, 0x3c90(r5)
    fadds    f0, f1, f0
    stfs     f0, 0x3c90(r5)
    lfs      f0, 0x3cd0(r5)
    stfs     f0, 0x3cd4(r5)
    lwz      r3, 0x3c78(r5)
    addi     r0, r3, 0x1
    stw      r0, 0x3c78(r5)
    psq_l    f31, 0x188(r1), 0, qr0
    lfd      f31, 0x180(r1)
    psq_l    f30, 0x178(r1), 0, qr0
    lfd      f30, 0x170(r1)
    psq_l    f29, 0x168(r1), 0, qr0
    lfd      f29, 0x160(r1)
    psq_l    f28, 0x158(r1), 0, qr0
    lfd      f28, 0x150(r1)
    psq_l    f27, 0x148(r1), 0, qr0
    lfd      f27, 0x140(r1)
    lwz      r31, 0x13c(r1)
    lwz      r30, 0x138(r1)
    lwz      r29, 0x134(r1)
    lwz      r0, 0x194(r1)
    lwz      r28, 0x130(r1)
    mtlr     r0
    addi     r1, r1, 0x190
    blr
*/

extern "C" u16 GXGetTexObjHeight(const GXTexObj* obj);

/**
 * @note Address: 0x80085B34
 * @note Size: 0x950
 */
extern "C" void CMsgFont_Init(CMsgFont* window, u8* data, f32 x, f32 y, s32 placement, s32 fitLine, s32 mode)
{
	CMsgFont* font    = (CMsgFont*)window;
	font->mData       = (CMsgData*)data;
	font->mActive     = FALSE;
	font->mFadeIn     = FALSE;
	font->mFadeOut    = FALSE;
	font->mBlink      = FALSE;
	font->mFitLine    = TRUE;
	font->_04_04      = TRUE;
	font->mCharCount  = 0;
	font->mLine       = 0;
	font->mPlacement  = 0;
	Vec origin        = { 0.0f, 0.0f, 0.0f };
	font->mPosition.x = origin.x;
	font->mPosition.y = origin.y;
	font->mPosition.z = origin.z;
#if defined(VERSION_JP)
	font->mSize = font->mPrevSize = 22.0f;
#else
	font->mSize     = 22.0f;
	font->mPrevSize = NAN;
#endif
	font->mGlyphColor  = msgColorTable[0];
	font->mGlyphAngle  = 0.0f;
	font->mGlyphScaleX = font->mGlyphScaleY = 1.0f;
	font->mEffect                           = 0;
	font->mSelectedLine                     = -1;
	font->mOtherSelection                   = -1;
	font->mAlpha                            = 0.0f;
	font->mTextAlpha                        = 1.0f;
	font->mHighlightAlpha                   = 0.0f;
	font->_43C6C                            = 0;
	font->mBlinkPhase                       = 0.0f;
	font->mTexOffset                        = 0.0f;
	for (int i = 0; i < 400; i++) {
		font->mGlyphs[i].mPhaseX = font->mGlyphs[i].mPhaseY = (i * 75) % 360;
	}
	font->mActive  = TRUE;
	font->mFadeIn  = TRUE;
	font->mFadeOut = FALSE;
	font->mBlink   = FALSE;
	font->mFitLine = fitLine;
	font->_04_04   = TRUE;
	f32 width      = font->mData->mMaxWidth;
	f32 height     = font->mData->mTotalHeight;
#if defined(VERSION_JP)
	CTexObj* texture = &(&gGame.mTpl)->mTex;
#define MSG_INIT_DESCRIPTOR(i) ((i) - 2)
#else
	CTexObj* texture = &(&gGame.mTpl)->mTex;
#define MSG_INIT_DESCRIPTOR(i) (i)
#endif
	font->mFrame.mTex = texture->Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[MSG_INIT_DESCRIPTOR(138)], 0);
	font->mScrollX = font->mScrollY = 0.0f;
	PSMTXTrans(font->mFrame.mTex.mMtx[0], font->mScrollX, font->mScrollY, 0.0f);
	font->mFrame.mTex.mMtxDirty = TRUE;
	font->mScaleX               = width / GXGetTexObjWidth(&font->mFrame.mTex);
	font->mScaleY               = height / GXGetTexObjHeight(&font->mFrame.mTex);
	PSMTXScale(font->mFrame.mTex.mMtx[1], font->mScaleX, font->mScaleY, 1.0f);
	font->mFrame.mTex.mMtxDirty = TRUE;
	font->mMode                 = mode;
	for (s32 i = 0; i < 400; i++) {
		font->mGlyphs[i].mSprite.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
		font->mGlyphs[i].mSprite.mTex = &font->mGlyphs[i].mTexture;
		font->mGlyphs[i].mColor       = msgColorTable[0];
		font->mGlyphs[i].mLine        = 0;
	}
	font->mPlacement                 = placement;
	font->mCursorX                   = -((font->mFitLine ? font->mData->mWidths[font->mLine] : width) * 0.5f);
	font->mWindowX                   = x;
	font->mWidth                     = width;
	font->mCursorY                   = 0.0f;
	font->mWindowY                   = y;
	font->mHeight                    = height;
	font->mLineY[font->mLine]        = font->mCursorY + (font->mData->mHeights[0] - 22.0f) * 0.5f;
	font->mTexs.mTex[MSG_TEX_CURSOR] = texture->Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[MSG_INIT_DESCRIPTOR(115)], 0);
	font->mTexs.mTex[MSG_TEX_DIGIT]  = texture->Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[MSG_INIT_DESCRIPTOR(116)], 0);
	font->mTexs.mTex[MSG_TEX_ICON]   = texture->Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[MSG_INIT_DESCRIPTOR(119)], 0);
	font->mTexs.mTex[MSG_TEX_ARROW]  = texture->Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[MSG_INIT_DESCRIPTOR(117)], 0);
	font->mTexs.mTex[MSG_TEX_LINE]   = texture->Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[MSG_INIT_DESCRIPTOR(118)], 0);
#undef MSG_INIT_DESCRIPTOR
}
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
    stmw     r23, 0x3c(r1)
    stw      r4, 0x0(r3)
    mr       r27, r3
    lis      r10, sMessageFontOrigin@ha
    lis      r9, __float_nan@ha
    lbz      r0, 0x4(r3)
    li       r8, 0x0
    rlwimi   r0, r8, 7, 24, 24
    addi     r11, r9, __float_nan@l
    stb      r0, 0x4(r27)
    mr       r29, r7
    mr       r28, r5
    lis      r3, 0xb60b
    lbz      r0, 0x4(r27)
    rlwimi   r0, r8, 6, 25, 25
    li       r12, 0x1
    addis    r24, r27, 0x4
    stb      r0, 0x4(r27)
    addi     r23, r10, sMessageFontOrigin@l
    lis      r4, msgColorTable@ha
    fmr      f28, f1
    lbz      r0, 0x4(r27)
    rlwimi   r0, r8, 5, 26, 26
    addi     r10, r4, msgColorTable@l
    fmr      f29, f2
    stb      r0, 0x4(r27)
    lfs      f4, lbl_8065D06C@sda21(r0)
    li       r4, -0x1
    lbz      r5, 0x4(r27)
    rlwimi   r5, r8, 4, 27, 27
    lfs      f3, lbl_8065D058@sda21(r0)
    li       r0, 0x32
    stb      r5, 0x4(r27)
    addi     r5, r3, 0x60b7
    lfs      f0, lbl_8065D05C@sda21(r0)
    mr       r7, r8
    lbz      r3, 0x4(r27)
    rlwimi   r3, r12, 3, 28, 28
    mr       r9, r27
    stb      r3, 0x4(r27)
    lbz      r3, 0x4(r27)
    rlwimi   r3, r12, 2, 29, 29
    stb      r3, 0x4(r27)
    stw      r8, 0x3c78(r24)
    stw      r8, 0x3c7c(r24)
    stw      r8, 0x3c80(r24)
    lwz      r3, 0x0(r23)
    lwz      r12, 0x4(r23)
    stw      r3, 0x10(r1)
    lwz      r3, 0x8(r23)
    stw      r12, 0x14(r1)
    lfs      f1, 0x10(r1)
    stw      r3, 0x18(r1)
    lfs      f2, 0x14(r1)
    stfs     f1, 0x3c84(r24)
    lfs      f1, 0x18(r1)
    stfs     f2, 0x3c88(r24)
    stfs     f1, 0x3c8c(r24)
    stfs     f4, 0x3cd0(r24)
    lfs      f1, 0x0(r11)
    stfs     f1, 0x3cd4(r24)
    lwz      r3, 0x0(r10)
    stw      r3, 0x3cd8(r24)
    stfs     f3, 0x3cdc(r24)
    stfs     f0, 0x3ce4(r24)
    stfs     f0, 0x3ce0(r24)
    stb      r8, 0x3ce8(r24)
    stw      r4, 0x3cec(r24)
    stw      r4, 0x3d0c(r24)
    stfs     f3, 0x3d10(r24)
    stfs     f0, 0x3d14(r24)
    stfs     f3, 0x3d18(r24)
    stw      r8, 0x3c6c(r24)
    stfs     f3, 0x3c70(r24)
    stfs     f3, 0x3c74(r24)
    mtctr    r0
.L_80085C98:
    mulhw    r10, r5, r7
    addi     r0, r8, 0x1
    addi     r4, r8, 0x2
    addi     r3, r8, 0x3
    addi     r11, r8, 0x6
    addi     r26, r8, 0x4
    mulli    r0, r0, 0x4b
    add      r10, r10, r7
    addi     r12, r8, 0x5
    srawi    r25, r10, 8
    mulhw    r24, r5, r0
    addi     r10, r8, 0x7
    srwi     r23, r25, 31
    addi     r8, r8, 0x8
    add      r23, r25, r23
    mulli    r4, r4, 0x4b
    add      r30, r24, r0
    srawi    r30, r30, 8
    srwi     r24, r30, 31
    add      r24, r30, r24
    mulhw    r25, r5, r4
    add      r30, r25, r4
    mulli    r24, r24, 0x168
    srawi    r31, r30, 8
    mulli    r3, r3, 0x4b
    srwi     r25, r31, 31
    subf     r24, r24, r0
    add      r25, r31, r25
    mulli    r23, r23, 0x168
    subf     r23, r23, r7
    addi     r7, r7, 0x258
    mulhw    r30, r5, r3
    sth      r23, 0xd50(r9)
    sth      r23, 0xd4e(r9)
    sth      r24, 0xffc(r9)
    mulli    r0, r26, 0x4b
    add      r30, r30, r3
    sth      r24, 0xffa(r9)
    mulhw    r26, r5, r0
    srawi    r30, r30, 8
    srwi     r31, r30, 31
    add      r31, r30, r31
    mulli    r30, r25, 0x168
    add      r26, r26, r0
    subf     r4, r30, r4
    srawi    r30, r26, 8
    sth      r4, 0x12a8(r9)
    mulli    r26, r12, 0x4b
    srwi     r12, r30, 31
    sth      r4, 0x12a6(r9)
    mulhw    r4, r5, r26
    add      r30, r30, r12
    mulli    r12, r31, 0x168
    add      r4, r4, r26
    subf     r3, r12, r3
    srawi    r4, r4, 8
    sth      r3, 0x1554(r9)
    srwi     r12, r4, 31
    add      r12, r4, r12
    mulli    r4, r30, 0x168
    sth      r3, 0x1552(r9)
    subf     r0, r4, r0
    mulli    r11, r11, 0x4b
    sth      r0, 0x1800(r9)
    sth      r0, 0x17fe(r9)
    mulhw    r3, r5, r11
    mulli    r4, r10, 0x4b
    add      r3, r3, r11
    srawi    r3, r3, 8
    srwi     r10, r3, 31
    mulhw    r0, r5, r4
    add      r3, r3, r10
    mulli    r10, r12, 0x168
    add      r0, r0, r4
    subf     r12, r10, r26
    srawi    r0, r0, 8
    mulli    r10, r3, 0x168
    sth      r12, 0x1aac(r9)
    srwi     r3, r0, 31
    sth      r12, 0x1aaa(r9)
    subf     r10, r10, r11
    add      r0, r0, r3
    sth      r10, 0x1d58(r9)
    mulli    r0, r0, 0x168
    sth      r10, 0x1d56(r9)
    subf     r0, r0, r4
    sth      r0, 0x2004(r9)
    sth      r0, 0x2002(r9)
    addi     r9, r9, 0x1560
    bdnz     .L_80085C98
    lbz      r0, 0x4(r27)
    li       r8, 0x1
    rlwimi   r0, r8, 7, 24, 24
    lis      r3, gGame@ha
    stb      r0, 0x4(r27)
    addi     r31, r3, gGame@l
    addi     r30, r31, 0x3d6c
    li       r7, 0x0
    lbz      r0, 0x4(r27)
    rlwimi   r0, r8, 6, 25, 25
    mr       r3, r30
    li       r5, 0x0
    stb      r0, 0x4(r27)
    lbz      r0, 0x4(r27)
    rlwimi   r0, r7, 5, 26, 26
    stb      r0, 0x4(r27)
    lbz      r0, 0x4(r27)
    rlwimi   r0, r7, 4, 27, 27
    stb      r0, 0x4(r27)
    lbz      r0, 0x4(r27)
    rlwimi   r0, r6, 3, 28, 28
    stb      r0, 0x4(r27)
    lbz      r0, 0x4(r27)
    rlwimi   r0, r8, 2, 29, 29
    stb      r0, 0x4(r27)
    lwz      r4, 0x3d4c(r31)
    lwz      r6, 0x0(r27)
    lwz      r4, 0x8(r4)
    lfs      f31, 0x880(r6)
    lfs      f30, 0x888(r6)
    addi     r4, r4, 0x450
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    addi     r5, r27, 0x40
    addi     r4, r3, 0x38
    stw      r7, 0x8(r27)
    stw      r6, 0xc(r27)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x10(r27)
    stw      r6, 0x14(r27)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x18(r27)
    stw      r6, 0x1c(r27)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x20(r27)
    stw      r6, 0x24(r27)
    lwz      r6, 0x20(r3)
    stw      r6, 0x28(r27)
    lwz      r6, 0x24(r3)
    stw      r6, 0x2c(r27)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x30(r27)
    stw      r6, 0x34(r27)
    lwz      r6, 0x30(r3)
    stw      r6, 0x38(r27)
    lwz      r6, 0x34(r3)
    stw      r6, 0x3c(r27)
    lwz      r3, 0x38(r3)
    stw      r3, 0x40(r27)
    mtctr    r0
.L_80085F08:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_80085F08
    lfs      f3, lbl_8065D058@sda21(r0)
    addi     r3, r27, 0x44
    stfs     f3, 0x10c(r27)
    stfs     f3, 0x108(r27)
    lfs      f1, 0x108(r27)
    lfs      f2, 0x10c(r27)
    bl       PSMTXTrans
    li       r0, 0x1
    addi     r3, r27, 0x8
    stw      r0, 0x40(r27)
    bl       GXGetTexObjWidth
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x24(r1)
    addi     r3, r27, 0x8
    lfd      f1, lbl_8065D048@sda21(r0)
    stw      r0, 0x20(r1)
    lfd      f0, 0x20(r1)
    fsubs    f0, f0, f1
    fdivs    f0, f31, f0
    stfs     f0, 0x110(r27)
    bl       GXGetTexObjHeight
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x2c(r1)
    addi     r3, r27, 0x74
    lfd      f1, lbl_8065D048@sda21(r0)
    stw      r0, 0x28(r1)
    lfs      f3, lbl_8065D05C@sda21(r0)
    lfd      f0, 0x28(r1)
    fsubs    f0, f0, f1
    fdivs    f0, f30, f0
    stfs     f0, 0x114(r27)
    lfs      f1, 0x110(r27)
    lfs      f2, 0x114(r27)
    bl       PSMTXScale
    li       r0, 0x1
    lis      r3, msgColorTable@ha
    stw      r0, 0x40(r27)
    mr       r23, r27
    lfs      f27, lbl_8065D058@sda21(r0)
    addi     r26, r3, msgColorTable@l
    stw      r29, 0xaa8(r27)
    li       r24, 0x0
    li       r29, 0x0
.L_80085FD0:
    stfs     f27, 0x8(r1)
    addi     r3, r23, 0xbac
    lfs      f4, lbl_8065D05C@sda21(r0)
    fmr      f1, f27
    lwz      r12, 0xd10(r23)
    fmr      f2, f27
    fmr      f3, f27
    lwz      r12, 0xc(r12)
    fmr      f5, f4
    fmr      f6, f4
    fmr      f7, f27
    fmr      f8, f27
    mtctr    r12
    bctrl
    addi     r0, r23, 0xaac
    addi     r24, r24, 0x1
    stw      r0, 0xd14(r23)
    cmpwi    r24, 0x190
    lwz      r0, 0x0(r26)
    stw      r0, 0xd34(r23)
    stw      r29, 0xd38(r23)
    addi     r23, r23, 0x2ac
    blt      .L_80085FD0
    addis    r3, r27, 0x4
    stw      r28, 0x3c80(r3)
    lbz      r0, 0x4(r27)
    extrwi.  r0, r0, 1, 28
    beq      .L_80086058
    lwz      r0, 0x3c7c(r3)
    lwz      r3, 0x0(r27)
    slwi     r0, r0, 2
    add      r3, r3, r0
    lfs      f1, 0x830(r3)
    b        .L_8008605C
.L_80086058:
    fmr      f1, f31
.L_8008605C:
    lfs      f3, lbl_8065D068@sda21(r0)
    addis    r6, r27, 0x4
    lfs      f0, lbl_8065D058@sda21(r0)
    mr       r3, r30
    fmuls    f2, f1, f3
    lfs      f1, lbl_8065D06C@sda21(r0)
    li       r5, 0x0
    fneg     f2, f2
    stfs     f2, 0x3c90(r6)
    stfs     f28, 0x3c94(r6)
    stfs     f31, 0x3c98(r6)
    stfs     f0, 0x3c9c(r6)
    stfs     f29, 0x3ca0(r6)
    stfs     f30, 0x3ca4(r6)
    lwz      r4, 0x0(r27)
    lwz      r0, 0x3c7c(r6)
    lfs      f2, 0x858(r4)
    slwi     r0, r0, 2
    lfs      f0, 0x3c9c(r6)
    fsubs    f1, f2, f1
    add      r4, r6, r0
    fmadds   f0, f1, f3, f0
    stfs     f0, 0x3ca8(r4)
    lwz      r4, 0x3d4c(r31)
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x398
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r7, 0x0(r3)
    addis    r8, r27, 0x4
    lwz      r6, 0x4(r3)
    li       r0, 0x18
    addi     r5, r8, 0x37a4
    addi     r4, r3, 0x38
    stw      r7, 0x376c(r8)
    stw      r6, 0x3770(r8)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x3774(r8)
    stw      r6, 0x3778(r8)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x377c(r8)
    stw      r6, 0x3780(r8)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x3784(r8)
    stw      r6, 0x3788(r8)
    lwz      r6, 0x20(r3)
    stw      r6, 0x378c(r8)
    lwz      r6, 0x24(r3)
    stw      r6, 0x3790(r8)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x3794(r8)
    stw      r6, 0x3798(r8)
    lwz      r6, 0x30(r3)
    stw      r6, 0x379c(r8)
    lwz      r6, 0x34(r3)
    stw      r6, 0x37a0(r8)
    lwz      r3, 0x38(r3)
    stw      r3, 0x37a4(r8)
    mtctr    r0
.L_80086154:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_80086154
    lwz      r4, 0x3d4c(r31)
    mr       r3, r30
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x3a0
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r7, 0x0(r3)
    addis    r8, r27, 0x4
    lwz      r6, 0x4(r3)
    li       r0, 0x18
    addi     r5, r8, 0x38a4
    addi     r4, r3, 0x38
    stw      r7, 0x386c(r8)
    stw      r6, 0x3870(r8)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x3874(r8)
    stw      r6, 0x3878(r8)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x387c(r8)
    stw      r6, 0x3880(r8)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x3884(r8)
    stw      r6, 0x3888(r8)
    lwz      r6, 0x20(r3)
    stw      r6, 0x388c(r8)
    lwz      r6, 0x24(r3)
    stw      r6, 0x3890(r8)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x3894(r8)
    stw      r6, 0x3898(r8)
    lwz      r6, 0x30(r3)
    stw      r6, 0x389c(r8)
    lwz      r6, 0x34(r3)
    stw      r6, 0x38a0(r8)
    lwz      r3, 0x38(r3)
    stw      r3, 0x38a4(r8)
    mtctr    r0
.L_8008620C:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_8008620C
    lwz      r4, 0x3d4c(r31)
    mr       r3, r30
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x3b8
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r7, 0x0(r3)
    addis    r8, r27, 0x4
    lwz      r6, 0x4(r3)
    li       r0, 0x18
    addi     r5, r8, 0x39a4
    addi     r4, r3, 0x38
    stw      r7, 0x396c(r8)
    stw      r6, 0x3970(r8)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x3974(r8)
    stw      r6, 0x3978(r8)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x397c(r8)
    stw      r6, 0x3980(r8)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x3984(r8)
    stw      r6, 0x3988(r8)
    lwz      r6, 0x20(r3)
    stw      r6, 0x398c(r8)
    lwz      r6, 0x24(r3)
    stw      r6, 0x3990(r8)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x3994(r8)
    stw      r6, 0x3998(r8)
    lwz      r6, 0x30(r3)
    stw      r6, 0x399c(r8)
    lwz      r6, 0x34(r3)
    stw      r6, 0x39a0(r8)
    lwz      r3, 0x38(r3)
    stw      r3, 0x39a4(r8)
    mtctr    r0
.L_800862C4:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_800862C4
    lwz      r4, 0x3d4c(r31)
    mr       r3, r30
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x3a8
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r7, 0x0(r3)
    addis    r8, r27, 0x4
    lwz      r6, 0x4(r3)
    li       r0, 0x18
    addi     r5, r8, 0x3aa4
    addi     r4, r3, 0x38
    stw      r7, 0x3a6c(r8)
    stw      r6, 0x3a70(r8)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x3a74(r8)
    stw      r6, 0x3a78(r8)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x3a7c(r8)
    stw      r6, 0x3a80(r8)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x3a84(r8)
    stw      r6, 0x3a88(r8)
    lwz      r6, 0x20(r3)
    stw      r6, 0x3a8c(r8)
    lwz      r6, 0x24(r3)
    stw      r6, 0x3a90(r8)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x3a94(r8)
    stw      r6, 0x3a98(r8)
    lwz      r6, 0x30(r3)
    stw      r6, 0x3a9c(r8)
    lwz      r6, 0x34(r3)
    stw      r6, 0x3aa0(r8)
    lwz      r3, 0x38(r3)
    stw      r3, 0x3aa4(r8)
    mtctr    r0
.L_8008637C:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_8008637C
    lwz      r4, 0x3d4c(r31)
    mr       r3, r30
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x3b0
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r7, 0x0(r3)
    addis    r8, r27, 0x4
    lwz      r6, 0x4(r3)
    li       r0, 0x18
    addi     r5, r8, 0x3ba4
    addi     r4, r3, 0x38
    stw      r7, 0x3b6c(r8)
    stw      r6, 0x3b70(r8)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x3b74(r8)
    stw      r6, 0x3b78(r8)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x3b7c(r8)
    stw      r6, 0x3b80(r8)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x3b84(r8)
    stw      r6, 0x3b88(r8)
    lwz      r6, 0x20(r3)
    stw      r6, 0x3b8c(r8)
    lwz      r6, 0x24(r3)
    stw      r6, 0x3b90(r8)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x3b94(r8)
    stw      r6, 0x3b98(r8)
    lwz      r6, 0x30(r3)
    stw      r6, 0x3b9c(r8)
    lwz      r6, 0x34(r3)
    stw      r6, 0x3ba0(r8)
    lwz      r3, 0x38(r3)
    stw      r3, 0x3ba4(r8)
    mtctr    r0
.L_80086434:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_80086434
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
    lmw      r23, 0x3c(r1)
    lwz      r0, 0xb4(r1)
    mtlr     r0
    addi     r1, r1, 0xb0
    blr
*/

static inline s32 MsgCommandLength(u8* p)
{
	switch (*p) {
	case 0:
	case 10:
	case 11:
	case 24:
		return 1;
	case 1:
	case 2:
		return 2;
	case 6:
		return 6;
	case 3:
		return 8;
	case 4:
		if (p[5] == 2 || p[5] == 3) {
			return 8;
		} else if (p[5] == 4) {
			return 9;
		}
		return 6;
	case 5:
	case 13:
	case 14:
	case 16:
	case 17:
	case 19:
		return 2;
	case 20:
	case 23:
		return 3;
	case 18:
	case 22:
		return 4;
	case 15:
	case 21:
		return 5;
	default:
		if (MsgHasSjisLead(*p)) {
			return 2;
		}
		return 1;
	}
}

/**
 * @note Address: 0x80086484
 * @note Size: 0x528
 */
extern "C" void CMsgData_Measure(CMsgData* data)
{
	data->mLineCount                 = 0;
	data->mCounts[data->mLineCount]  = 0;
	data->mWidths[data->mLineCount]  = 0.0f;
	data->mHeights[data->mLineCount] = 0.0f;
	data->mStyle                     = -1;
	data->mAlignment                 = 1;
	f32 size                         = 22.0f;
	BOOL proportional                = TRUE;
	u8* p                            = data->mText;
	s32 done                         = 0;
	while (!done) {
		switch (*p) {
		case 10:
		case 11:
			if (data->mHeights[data->mLineCount] == 0.0f) {
				data->mHeights[data->mLineCount] = size;
			}
			data->mLineCount++;
			data->mCounts[data->mLineCount]  = 0;
			data->mWidths[data->mLineCount]  = 0.0f;
			data->mHeights[data->mLineCount] = size;
			break;
		case 0:
			if (data->mHeights[data->mLineCount] == 0.0f) {
				data->mHeights[data->mLineCount] = 22.0f;
			}
			data->mLineCount++;
			done = 1;
			break;
		case 13:
			size = (s32)p[1];
			if (size == 255.0f) {
				size = 22.0f;
			}
			if (size > data->mHeights[data->mLineCount]) {
				data->mHeights[data->mLineCount] = size;
			}
			break;
		case 16:
			proportional = p[1];
			break;
		case 19:
			data->mAlignment = p[1];
			break;
		case 5:
			data->mStyle = p[1];
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 6:
		case 14:
		case 15:
		case 18:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
			break;
		default:
			if (MsgHasSjisLead(*p)) {
				data->mCounts[data->mLineCount]++;
				char text[3] = "";
				text[0]      = p[0];
				text[1]      = p[1];
				data->mWidths[data->mLineCount] += MeasureGameTextWidth(&gGame.mGs, text, proportional, size);
			} else if (*p == 17) {
#if defined(VERSION_JP)
				TPLPalette* palette = (TPLPalette*)(&gGame.mTpl)->mBuffer;
				CTexObj* texture    = &(&gGame.mTpl)->mTex;
				CTexObj icon        = texture->Init(&palette->descriptorArray[117], 0);
				data->mWidths[data->mLineCount] += (size / 22.0f) * (GXGetTexObjWidth(&icon) / 14);
#else
				TPLPalette* palette = (TPLPalette*)(&gGame.mTpl)->mBuffer;
				CTexObj* texture    = &(&gGame.mTpl)->mTex;
				CTexObj icon        = texture->Init(&palette->descriptorArray[119], 0);
				data->mWidths[data->mLineCount] += 24.0f * (size / 22.0f);
#endif
			} else {
				data->mCounts[data->mLineCount]++;
				char text[2] = "";
				text[0]      = p[0];
				data->mWidths[data->mLineCount] += MeasureGameTextWidth(&gGame.mGs, text, proportional, size);
			}
			break;
		}
		p += MsgCommandLength(p);
	}
	data->mMaxWidth    = 0.0f;
	data->mMaxHeight   = 0.0f;
	data->mTotalHeight = 0.0f;
	for (s32 i = 0; i < data->mLineCount; i++) {
		if (data->mMaxWidth < data->mWidths[i]) {
			data->mMaxWidth = data->mWidths[i];
		}
		if (data->mMaxHeight < data->mHeights[i]) {
			data->mMaxHeight = data->mHeights[i];
		}
		data->mTotalHeight += data->mHeights[i];
	}
}
/*
    stwu     r1, -0x140(r1)
    mflr     r0
    stw      r0, 0x144(r1)
    stfd     f31, 0x130(r1)
    psq_st   f31, 0x138(r1), 0, qr0
    stmw     r26, 0x118(r1)
    mr       r26, r3
    li       r5, 0x0
    stw      r5, 0x804(r3)
    lis      r3, gGame@ha
    addi     r31, r3, gGame@l
    lfs      f0, lbl_8065D058@sda21(r0)
    lwz      r4, 0x804(r26)
    li       r3, -0x1
    li       r0, 0x1
    lfs      f31, lbl_8065D06C@sda21(r0)
    slwi     r4, r4, 2
    mr       r29, r26
    add      r4, r26, r4
    addi     r30, r31, 0x3d6c
    stw      r5, 0x808(r4)
    li       r28, 0x1
    li       r27, 0x0
    lwz      r4, 0x804(r26)
    slwi     r4, r4, 2
    add      r4, r26, r4
    stfs     f0, 0x830(r4)
    lwz      r4, 0x804(r26)
    slwi     r4, r4, 2
    add      r4, r26, r4
    stfs     f0, 0x858(r4)
    stw      r3, 0x88c(r26)
    stw      r0, 0x890(r26)
    b        .L_80086920
.L_8008650C:
    lbz      r4, 0x0(r29)
    cmplwi   r4, 0x18
    bgt      .L_80086640
    lis      r3, jumptable_80225594@ha
    slwi     r0, r4, 2
    addi     r3, r3, jumptable_80225594@l
    lwzx     r0, r3, r0
    mtctr    r0
    bctr
.L_80086530:
    lwz      r0, 0x804(r26)
    lfs      f1, lbl_8065D058@sda21(r0)
    slwi     r3, r0, 2
    addi     r0, r3, 0x858
    lfsx     f0, r26, r0
    fcmpu    cr0, f1, f0
    bne      .L_80086550
    stfsx    f31, r26, r0
.L_80086550:
    lwz      r3, 0x804(r26)
    li       r4, 0x0
    lfs      f0, lbl_8065D058@sda21(r0)
    addi     r0, r3, 0x1
    stw      r0, 0x804(r26)
    lwz      r0, 0x804(r26)
    slwi     r3, r0, 2
    addi     r0, r3, 0x808
    stwx     r4, r26, r0
    lwz      r0, 0x804(r26)
    slwi     r3, r0, 2
    addi     r0, r3, 0x830
    stfsx    f0, r26, r0
    lwz      r0, 0x804(r26)
    slwi     r3, r0, 2
    addi     r0, r3, 0x858
    stfsx    f31, r26, r0
    b        .L_8008683C
.L_80086598:
    lwz      r0, 0x804(r26)
    lfs      f1, lbl_8065D058@sda21(r0)
    slwi     r3, r0, 2
    addi     r0, r3, 0x858
    lfsx     f0, r26, r0
    fcmpu    cr0, f1, f0
    bne      .L_800865BC
    lfs      f0, lbl_8065D06C@sda21(r0)
    stfsx    f0, r26, r0
.L_800865BC:
    lwz      r3, 0x804(r26)
    li       r27, 0x1
    addi     r0, r3, 0x1
    stw      r0, 0x804(r26)
    b        .L_8008683C
.L_800865D0:
    lbz      r3, 0x1(r29)
    lis      r0, 0x4330
    stw      r0, 0x110(r1)
    xoris    r0, r3, 0x8000
    lfd      f2, lbl_8065D050@sda21(r0)
    stw      r0, 0x114(r1)
    lfs      f0, lbl_8065D0DC@sda21(r0)
    lfd      f1, 0x110(r1)
    fsubs    f31, f1, f2
    fcmpu    cr0, f0, f31
    bne      .L_80086600
    lfs      f31, lbl_8065D06C@sda21(r0)
.L_80086600:
    lwz      r0, 0x804(r26)
    slwi     r3, r0, 2
    addi     r0, r3, 0x858
    lfsx     f0, r26, r0
    fcmpo    cr0, f31, f0
    ble      .L_8008683C
    stfsx    f31, r26, r0
    b        .L_8008683C
.L_80086620:
    lbz      r28, 0x1(r29)
    b        .L_8008683C
.L_80086628:
    lbz      r0, 0x1(r29)
    stw      r0, 0x890(r26)
    b        .L_8008683C
.L_80086634:
    lbz      r0, 0x1(r29)
    stw      r0, 0x88c(r26)
    b        .L_8008683C
.L_80086640:
    lbz      r0, 0x3318(r31)
    cmplwi   r0, 0x6
    bne      .L_800866E8
    cmplwi   r4, 0x81
    li       r0, 0x0
    blt      .L_80086660
    cmplwi   r4, 0x9f
    ble      .L_80086670
.L_80086660:
    cmplwi   r4, 0xe0
    blt      .L_80086674
    cmplwi   r4, 0xfc
    bgt      .L_80086674
.L_80086670:
    li       r0, 0x1
.L_80086674:
    clrlwi.  r0, r0, 24
    beq      .L_800866E8
    lwz      r0, 0x804(r26)
    lis      r3, gGame@ha
    fmr      f1, f31
    addi     r3, r3, gGame@l
    slwi     r4, r0, 2
    mr       r5, r28
    addi     r7, r4, 0x808
    lwzx     r6, r26, r7
    addi     r4, r1, 0xc
    addi     r0, r6, 0x1
    stwx     r0, r26, r7
    lhz      r6, lbl_8065FFB0@sda21(r0)
    lbz      r0, lbl_8065FFB2@sda21(r0)
    sth      r6, 0xc(r1)
    stb      r0, 0xe(r1)
    lbz      r0, 0x0(r29)
    stb      r0, 0xc(r1)
    lbz      r0, 0x1(r29)
    stb      r0, 0xd(r1)
    bl       MeasureGameTextWidth
    lwz      r0, 0x804(r26)
    slwi     r3, r0, 2
    addi     r0, r3, 0x830
    lfsx     f0, r26, r0
    fadds    f0, f0, f1
    stfsx    f0, r26, r0
    b        .L_8008683C
.L_800866E8:
    cmplwi   r4, 0x11
    bne      .L_800867E4
    lwz      r4, 0x3d4c(r31)
    mr       r3, r30
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x3b8
    bl       Init__7CTexObjFP13TPLDescriptorUl
    lwz      r8, 0x0(r3)
    lis      r4, __vt__7CTexObj@ha
    lwz      r7, 0x4(r3)
    addi     r6, r4, __vt__7CTexObj@l
    li       r0, 0x18
    addi     r5, r1, 0x48
    stw      r8, 0x10(r1)
    addi     r4, r3, 0x38
    stw      r7, 0x14(r1)
    lwz      r8, 0x8(r3)
    lwz      r7, 0xc(r3)
    stw      r8, 0x18(r1)
    stw      r7, 0x1c(r1)
    lwz      r8, 0x10(r3)
    lwz      r7, 0x14(r3)
    stw      r8, 0x20(r1)
    stw      r7, 0x24(r1)
    lwz      r8, 0x18(r3)
    lwz      r7, 0x1c(r3)
    stw      r8, 0x28(r1)
    stw      r7, 0x2c(r1)
    stw      r6, 0x10c(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x30(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x34(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x38(r1)
    stw      r6, 0x3c(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x40(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x44(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x48(r1)
    mtctr    r0
.L_8008679C:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_8008679C
    lfs      f0, lbl_8065D06C@sda21(r0)
    lis      r3, __vt__7CTexObj@ha
    lwz      r4, 0x804(r26)
    addi     r0, r3, __vt__7CTexObj@l
    fdivs    f1, f31, f0
    lfs      f2, lbl_8065D0F0@sda21(r0)
    slwi     r3, r4, 2
    addi     r3, r3, 0x830
    lfsx     f0, r26, r3
    fmadds   f0, f2, f1, f0
    stfsx    f0, r26, r3
    stw      r0, 0x10c(r1)
    b        .L_8008683C
.L_800867E4:
    lwz      r0, 0x804(r26)
    lis      r3, gGame@ha
    fmr      f1, f31
    addi     r3, r3, gGame@l
    slwi     r4, r0, 2
    mr       r5, r28
    addi     r7, r4, 0x808
    lwzx     r6, r26, r7
    addi     r4, r1, 0x8
    addi     r0, r6, 0x1
    stwx     r0, r26, r7
    lhz      r0, lbl_8065FFB4@sda21(r0)
    sth      r0, 0x8(r1)
    lbz      r0, 0x0(r29)
    stb      r0, 0x8(r1)
    bl       MeasureGameTextWidth
    lwz      r0, 0x804(r26)
    slwi     r3, r0, 2
    addi     r0, r3, 0x830
    lfsx     f0, r26, r0
    fadds    f0, f0, f1
    stfsx    f0, r26, r0
.L_8008683C:
    lbz      r4, 0x0(r29)
    cmplwi   r4, 0x18
    bgt      .L_800868D4
    lis      r3, jumptable_80225530@ha
    slwi     r0, r4, 2
    addi     r3, r3, jumptable_80225530@l
    lwzx     r0, r3, r0
    mtctr    r0
    bctr
.L_80086860:
    li       r0, 0x1
    b        .L_8008691C
.L_80086868:
    li       r0, 0x2
    b        .L_8008691C
.L_80086870:
    li       r0, 0x6
    b        .L_8008691C
.L_80086878:
    li       r0, 0x8
    b        .L_8008691C
.L_80086880:
    lbz      r0, 0x5(r29)
    cmplwi   r0, 0x2
    beq      .L_80086894
    cmplwi   r0, 0x3
    bne      .L_8008689C
.L_80086894:
    li       r0, 0x8
    b        .L_8008691C
.L_8008689C:
    cmplwi   r0, 0x4
    bne      .L_800868AC
    li       r0, 0x9
    b        .L_8008691C
.L_800868AC:
    li       r0, 0x6
    b        .L_8008691C
.L_800868B4:
    li       r0, 0x2
    b        .L_8008691C
.L_800868BC:
    li       r0, 0x3
    b        .L_8008691C
.L_800868C4:
    li       r0, 0x4
    b        .L_8008691C
.L_800868CC:
    li       r0, 0x5
    b        .L_8008691C
.L_800868D4:
    lbz      r0, 0x3318(r31)
    cmplwi   r0, 0x6
    bne      .L_80086918
    cmplwi   r4, 0x81
    li       r0, 0x0
    blt      .L_800868F4
    cmplwi   r4, 0x9f
    ble      .L_80086904
.L_800868F4:
    cmplwi   r4, 0xe0
    blt      .L_80086908
    cmplwi   r4, 0xfc
    bgt      .L_80086908
.L_80086904:
    li       r0, 0x1
.L_80086908:
    clrlwi.  r0, r0, 24
    beq      .L_80086918
    li       r0, 0x2
    b        .L_8008691C
.L_80086918:
    li       r0, 0x1
.L_8008691C:
    add      r29, r29, r0
.L_80086920:
    cmpwi    r27, 0x0
    beq      .L_8008650C
    lfs      f0, lbl_8065D058@sda21(r0)
    mr       r3, r26
    li       r4, 0x0
    stfs     f0, 0x880(r26)
    stfs     f0, 0x884(r26)
    stfs     f0, 0x888(r26)
    b        .L_80086984
.L_80086944:
    lfs      f0, 0x880(r26)
    lfs      f1, 0x830(r3)
    fcmpo    cr0, f0, f1
    bge      .L_80086958
    stfs     f1, 0x880(r26)
.L_80086958:
    lfs      f0, 0x884(r26)
    lfs      f1, 0x858(r3)
    fcmpo    cr0, f0, f1
    bge      .L_8008696C
    stfs     f1, 0x884(r26)
.L_8008696C:
    lfs      f0, 0x858(r3)
    addi     r3, r3, 0x4
    lfs      f1, 0x888(r26)
    addi     r4, r4, 0x1
    fadds    f0, f1, f0
    stfs     f0, 0x888(r26)
.L_80086984:
    lwz      r0, 0x804(r26)
    cmpw     r4, r0
    blt      .L_80086944
    psq_l    f31, 0x138(r1), 0, qr0
    lfd      f31, 0x130(r1)
    lmw      r26, 0x118(r1)
    lwz      r0, 0x144(r1)
    mtlr     r0
    addi     r1, r1, 0x140
    blr
*/

#if defined(VERSION_JP) || defined(VERSION_PAL)
/**
 * @note Address: N/A
 * @note Size: ?
 */
// japan and europe only, the graphics state reset usa inlines into Draw
extern "C" void InvalidateMessageDrawState(u8* game)
{
	HSD_StateInvalidate(-1);
#if defined(VERSION_PAL)
	CGsRenderMode* mode = *(CGsRenderMode**)(game + 0x294);
	if (mode != NULL)
		mode->Invalidate();
	CGsTevStage* stage = *(CGsTevStage**)(game + 0x298);
	if (stage != NULL)
		stage->Invalidate();
	CGsChannel* channel = *(CGsChannel**)(game + 0x29C);
#else
	CGsRenderMode* mode = *(CGsRenderMode**)(game + 0x290);
	if (mode != NULL)
		mode->Invalidate();
	CGsTevStage* stage = *(CGsTevStage**)(game + 0x294);
	if (stage != NULL)
		stage->Invalidate();
	CGsChannel* channel = *(CGsChannel**)(game + 0x298);
#endif
	if (channel != NULL && !channel->mDirty)
		channel->mDirty = TRUE;
}
#endif
