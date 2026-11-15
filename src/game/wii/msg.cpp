// The message system. One file, .text 0x800A1938-0x800A8164:
// - the functions before it belong to evt.cpp: __dt__9CEvtTimerFv (0x800A14A8), __dt__8CEvtVarsFv
//   (0x800A14E8), __dt__9CEvtFlagsFv (0x800A1528) and __dt__11CEvtQueueExFv (0x800A18B8) pair to
//   game/evt.cpp in wii_gc_pairs.txt, and the CCameraMemento constructor (0x800A18F8) plus the
//   array destructor (0x800A191C) it registers sit between them and this file's first function
// - its jump tables sit in .rodata from 0x802FFAF8 on: jumptable_802FFAF8 (25 entries,
//   MsgCommandLength's MSG_CODE_* values 0..24) and jumptable_802FFB5C (CMsgData_Format)
// - the GameCube pairs (wii_gc_pairs.txt) land in the reverse of the GameCube's source order:
//   CMsg__cmd_SETFONT (0x800A7ED4) is the GameCube file's first function, CMsgData_Format and
//   CMsgData_Measure (0x800A1A20, 0x800A1FE8) are among its last
//
// NONMATCHING: one function has source so far: MsgCommandLength (0x800A1938). The rest of the
// unit is unwritten: CMsgData_Format (0x800A1A20), CMsgData_Measure (0x800A1FE8), fn_800A23B8,
// CMsgFont_Init (0x800A2524), ResetMessageState (0x800A2C5C), CMsgFont_Update (0x800A34C0),
// CMsg_Update (0x800A61C4), CMsg__cmd_WAIT (0x800A74C0), CMsg__cmd_SFX (0x800A7A68),
// CMsg__cmd_SETFONT (0x800A7ED4) and the small functions between them. The GameCube file's
// VERSION_JP branches are the reconstruction base; the Wii reads its language flag from
// gGame.mLanguage (_4160), where Japanese is 0.

#include "game/wii/msg.h"
#include "game/cfont.h"
#include "game/wii/cgame.h"

// the Wii keeps its own language codes: japanese is 0 (the GameCube tests for 6)
#define MsgHasSjisLead(c) (gGame.mLanguage == 0 && IsSjisLead(c))

/**
 * @note Address: 0x800A1938
 * @note Size: 0xE8
 */
extern "C" s32 MsgCommandLength(u8* p)
{
	switch (*p) {
	case MSG_CODE_END:
	case MSG_CODE_NEWLINE:
	case MSG_CODE_NEWLINE_VT:
	case MSG_CODE_STAY:
		return 1;
	case MSG_CODE_SPEED:
	case MSG_CODE_WAIT:
	case MSG_CODE_VOICE:
		return 2;
	case MSG_CODE_DEFAULT:
		return 6;
	case MSG_CODE_ANIM:
		return 8;
	case MSG_CODE_SFX:
		if (p[5] == MSG_SFX_FADE_OUT || p[5] == MSG_SFX_FADE_IN) {
			return 8;
		} else if (p[5] == MSG_SFX_FADE) {
			return 9;
		}
		return 6;
	case MSG_CODE_SIZE:
	case MSG_CODE_COLOR:
	case MSG_CODE_LAYOUT:
	case MSG_CODE_ICON:
	case MSG_CODE_CENTER:
		return 2;
	case MSG_CODE_ROTATE:
	case MSG_CODE_QUESTION:
		return 3;
	case MSG_CODE_SHAKE:
	case MSG_CODE_NUM_INPUT:
		return 4;
	case MSG_CODE_RGBA:
	case MSG_CODE_SCALE:
		return 5;
	default:
		if (MsgHasSjisLead(*p)) {
			return 2;
		}
		return 1;
	}
}
