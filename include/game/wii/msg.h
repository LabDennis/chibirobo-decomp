#ifndef _GAME_WII_MSG_H
#define _GAME_WII_MSG_H

#include "types.h"

// control codes inside message text. the enum is the GameCube msg.h's, checked against the
// jump table of the Wii's MsgCommandLength at 0x802FFAF8 (25 entries, codes 0..24)
enum MsgCode {
	MSG_CODE_END        = 0,  // no args. also where select, number input and stay wait for the player
	MSG_CODE_SPEED      = 1,  // u8 frames per char, 0xFF resets, to 2 in msg but 0 in select
	MSG_CODE_WAIT       = 2,  // u8 tenths of a second, 0xFC to 0xFF wait for a button instead
	MSG_CODE_ANIM       = 3,  // u8 flags, s16 obj, s32 anim
	MSG_CODE_SFX        = 4,  // u32 id, s8 MsgSfxType, then 0, 2 or 3 more bytes
	MSG_CODE_VOICE      = 5,  // u8 voice, 0..33, 0xFF for none
	MSG_CODE_DEFAULT    = 6,  // u8 flags, s32 index, the cursor select() starts on
	MSG_CODE_NEWLINE    = 10, // no args
	MSG_CODE_NEWLINE_VT = 11, // no args, same code as NEWLINE
	MSG_CODE_FORMAT     = 12, // printf spec up to the next 12, expanded when the message opens
	MSG_CODE_SIZE       = 13, // u8, 0xFF resets to 22
	MSG_CODE_COLOR      = 14, // u8 index into the 10 color presets, 0xFF resets to white
	MSG_CODE_RGBA       = 15, // u32 big endian
	MSG_CODE_LAYOUT     = 16, // u8, bit 0
	MSG_CODE_ICON       = 17, // u8 icon, drawn as a glyph, not a SETFONT code
	MSG_CODE_SHAKE      = 18, // u8 flags, u8 strength, u8 speed
	MSG_CODE_CENTER     = 19, // u8, bit 0, on by default
	MSG_CODE_ROTATE     = 20, // s16 degrees
	MSG_CODE_SCALE      = 21, // s16 x, s16 y, percent
	MSG_CODE_NUM_INPUT  = 22, // u8 digits, u8 editable, u8 selected
	MSG_CODE_QUESTION   = 23, // u8 flags, u8 default
	MSG_CODE_STAY       = 24, // no args, leaves the window up after msg returns
};

// the sfx subcommand of MSG_CODE_SFX, one byte at p[5]
enum MsgSfxType {
	MSG_SFX_WAIT     = -1, // no extra bytes
	MSG_SFX_STOP     = 0,  // no extra bytes
	MSG_SFX_PLAY     = 1,  // no extra bytes
	MSG_SFX_FADE_OUT = 2,  // u16 frames
	MSG_SFX_FADE_IN  = 3,  // u16 frames
	MSG_SFX_FADE     = 4,  // u16 frames, u8 volume
	MSG_SFX_PAUSE    = 5,  // no extra bytes
	MSG_SFX_RESUME   = 6,  // no extra bytes
};

#endif
