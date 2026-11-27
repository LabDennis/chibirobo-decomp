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
	case MSG_SFX_WAIT:
		if (IsGameSoundPlaying(&gGame.mSound, sfx)) {
			// back onto the code byte to run it again: code, u32 id and type are 6 bytes, so the US and JP 3 lands inside the id
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
	if (flags & MSG_ANIM_VARIABLE) {
		anim = gEventVars.mVars[anim];
	}
	XObj* obj = FindObj(id);
	if (obj != NULL && (obj->mStatus.mFlags & 1)) {
		if (anim == -1) {
			HSD_AObj* aobj = GetAObj(obj->mJObj);
			if (!(aobj->flags & (AOBJ_NO_ANIM | AOBJ_REWINDED))) {
				msg->mData.mCursor -= 8;
				return 1;
			}
		} else if (!(flags & MSG_ANIM_QUEUE)) {
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
/**
 * @note Address: 0x800808E4
 * @note Size: 0x2A4
 */
extern "C" s32 CMsg__cmd_WAIT(CMsg* msg)
{
	s32 command = ReadMsgU8(msg);
	// 252 to 255 wait for a button instead of a time
	if (!(command != 255 && command != 254 && command != 253 && command != 252)) {
		if (msg->mCommand == EVT_CMD_MSG) {
			if (!msg->mFont.mBlink) {
				s32 icon = -1;
				if (command == 255) {
					icon = 0;
				}
				if (command == 254) {
					icon = 1;
				}
				if (command == 253 || command == 252) {
					icon = 0;
				}
				msg->mFont.mBlink      = 1;
				msg->mFont.mBlinkIcon  = icon;
				msg->mFont.mBlinkPhase = 0.0f;
				msg->mData.mCursor -= 2;
			} else {
				BOOL done = FALSE;
				CPad pad(0);
				if (command == 253 || command == 252) {
					if ((pad.mPad->mTrigger & PAD_BUTTON_X) && !(gGame.mStatus.mFlags & 0x20)) {
						done = TRUE;
						if (command == 253) {
							SelectPlayerEquipmentRing(gGame.GetPlayer(), 0);
						} else if (command == 252) {
							SelectPlayerEquipmentRing(gGame.GetPlayer(), 1);
						}
					}
				} else if (((pad.mPad->mTrigger & PAD_BUTTON_A) || (pad.mPad->mTrigger & PAD_BUTTON_B)) && !(gGame.mStatus.mFlags & 0x20)) {
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
	msg->mWaitTimer = frames;
	return 0;
}
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
			if (msg->mCommand == EVT_CMD_SELECT) {
				if (msg->mSelectedLine < 0) {
					msg->mSelectedLine       = msg->mDefaultLine;
					msg->mFont.mSelectedLine = msg->mSelectedLine;
					msg->mFont.mCurLineY     = msg->mFont.mLineY[msg->mFont.mSelectedLine];
					msg->mFont.mCurLineWidth = 0.0f;
				} else if (!(gGame.mStatus.mFlags & 0x20)) {
					CPad pad(0);
					u32 up   = gGame.mStatus.mFlags & 0x80000000 ? HSD_PAD_STICK_UP : (HSD_PAD_STICK_UP | HSD_PAD_DPADUP);
					u32 down = gGame.mStatus.mFlags & 0x80000000 ? HSD_PAD_STICK_DOWN : (HSD_PAD_STICK_DOWN | HSD_PAD_DPADDOWN);
					if (pad.mPad->mRepeat & up) {
						if (--msg->mSelectedLine < 0) {
							msg->mSelectedLine = msg->mData.mLineCount - 1;
						}
						msg->mFont.mSelectedLine = msg->mSelectedLine;
						PlayGameSound(&gGame.mSound, 0x87, 255);
					} else if (pad.mPad->mRepeat & down) {
						if (++msg->mSelectedLine >= msg->mData.mLineCount) {
							msg->mSelectedLine = 0;
						}
						msg->mFont.mSelectedLine = msg->mSelectedLine;
						PlayGameSound(&gGame.mSound, 0x87, 255);
					} else if (pad.mPad->mTrigger & PAD_BUTTON_A) {
						msg->mSelectResult       = msg->mSelectedLine;
						msg->mSelectedLine       = -1;
						msg->mFont.mFadeOut      = 1;
						msg->mFont.mSelectedLine = -1;
						PlayGameSound(&gGame.mSound, 0x88, 255);
					} else if (pad.mPad->mTrigger & PAD_BUTTON_B) {
						msg->mSelectResult       = -1;
						msg->mSelectedLine       = -1;
						msg->mFont.mFadeOut      = 1;
						msg->mFont.mSelectedLine = -1;
						PlayGameSound(&gGame.mSound, 0x86, 255);
					}
				}
			} else if (msg->mDigitCount > 0) {
				if (msg->mFont.mOtherSelection < 0) {
					MsgSetDigits(msg, msg->mDigits, msg->mDigitCount, msg->mFirstEditDigit, msg->mSelectedDigit);
				} else {
					CPad pad(0);
					// mDigits[0] is the ones digit (A reads them from [9] down), so left moves to the next higher digit
					u32 up       = gGame.mStatus.mFlags & 0x80000000 ? HSD_PAD_STICK_UP : (HSD_PAD_STICK_UP | HSD_PAD_DPADUP);
					u32 down     = gGame.mStatus.mFlags & 0x80000000 ? HSD_PAD_STICK_DOWN : (HSD_PAD_STICK_DOWN | HSD_PAD_DPADDOWN);
					u32 next     = gGame.mStatus.mFlags & 0x80000000 ? HSD_PAD_STICK_LEFT : (HSD_PAD_STICK_LEFT | HSD_PAD_DPADLEFT);
					u32 previous = gGame.mStatus.mFlags & 0x80000000 ? HSD_PAD_STICK_RIGHT : (HSD_PAD_STICK_RIGHT | HSD_PAD_DPADRIGHT);
					if (pad.mPad->mRepeat & up) {
						if (++msg->mDigits[msg->mSelectedDigit] > 9) {
							msg->mDigits[msg->mSelectedDigit] = 0;
						}
						MsgSetDigits(msg, msg->mDigits, msg->mDigitCount, msg->mFirstEditDigit, msg->mSelectedDigit);
						PlayGameSound(&gGame.mSound, 0x87, 255);
					} else if (pad.mPad->mRepeat & down) {
						if (--msg->mDigits[msg->mSelectedDigit] < 0) {
							msg->mDigits[msg->mSelectedDigit] = 9;
						}
						MsgSetDigits(msg, msg->mDigits, msg->mDigitCount, msg->mFirstEditDigit, msg->mSelectedDigit);
						PlayGameSound(&gGame.mSound, 0x87, 255);
					} else if (pad.mPad->mRepeat & next) {
						if (++msg->mSelectedDigit > msg->mDigitCount - 1) {
							msg->mSelectedDigit = msg->mDigitCount - 1;
						} else {
							MsgSetDigits(msg, msg->mDigits, msg->mDigitCount, msg->mFirstEditDigit, msg->mSelectedDigit);
							PlayGameSound(&gGame.mSound, 0x87, 255);
						}
					} else if (pad.mPad->mRepeat & previous) {
						if (--msg->mSelectedDigit < msg->mFirstEditDigit) {
							msg->mSelectedDigit = msg->mFirstEditDigit;
						} else {
							MsgSetDigits(msg, msg->mDigits, msg->mDigitCount, msg->mFirstEditDigit, msg->mSelectedDigit);
							PlayGameSound(&gGame.mSound, 0x87, 255);
						}
					} else if (pad.mPad->mTrigger & PAD_BUTTON_A) {
						msg->mFont.mFadeOut        = 1;
						msg->mFont.mOtherSelection = -1;
						PlayGameSound(&gGame.mSound, 0x88, 255);
						s32 result = 0;
						for (s32 i = 9; i >= 0; i--) {
							result = result * 10 + msg->mDigits[i];
						}
						gGame.mEvt.mSelectResult = result;
					} else if (pad.mPad->mTrigger & PAD_BUTTON_B) {
						msg->mFont.mFadeOut        = 1;
						msg->mFont.mOtherSelection = -1;
						PlayGameSound(&gGame.mSound, 0x86, 255);
						gGame.mEvt.mSelectResult = -1;
					}
				}
			} else if (msg->mStay != 0) {
				if (msg->mStay == 1) {
					if (gGame.GetPlayer()->mOXMenu.mStarted < 0) {
						msg->mStay++;
					}
				} else {
					msg->mFont.mFadeOut = 1;
				}
			} else {
				msg->mFont.mFadeOut = 1;
			}
		}
		// stay on END while the window is up, CMsgFont_Update clears mActive once the fade out ends
		msg->mData.mCursor--;
	} else {
		msg->mData.mCursor = NULL;
#if !defined(VERSION_JP)
		msg->mData.mIconColumns = 0;
#endif
	}
	return 1;
}
/**
 * @note Address: 0x800814CC
 * @note Size: 0xD0
 */
extern "C" void CMsg_Draw(CMsg* msg)
{
	SetGsChannelEnabled(&gGame.mGs, 0, 0);
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 3, 0);
	if (msg->mCharDelay == 0.0f) {
		CMsgFont_Draw(&msg->mFont, 1.0f);
	} else {
		f32 t = (msg->mCharDelay - msg->mCharTimer) / msg->mCharDelay;
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
		if (msg->mWaitTimer > 0.0f) {
			f32 time        = msg->mWaitTimer - step;
			msg->mWaitTimer = time;
			if (time < 0.0f) {
				msg->mWaitTimer = 0.0f;
			} else {
				waiting = TRUE;
			}
			continue;
		}
		if (msg->mCharTimer < 0.0f) {
			waiting = TRUE;
			continue;
		}
		if (msg->mCharTimer > 0.0f) {
			f32 time        = msg->mCharTimer - step;
			msg->mCharTimer = time;
			if (time < 0.0f) {
				msg->mCharTimer = 0.0f;
				if (msg->mCharDelay > 0.0f && msg->mCharDelay - step < 0.0f) {
					waiting = TRUE;
				}
			} else {
				waiting = TRUE;
			}
			continue;
		}
		u8 command = ReadMsgU8(msg);
		switch (command) {
		case MSG_CODE_NEWLINE:
			MsgNextLine(msg);
			waiting = FALSE;
			break;
		case MSG_CODE_NEWLINE_VT:
			MsgNextLine(msg);
			waiting = FALSE;
			break;
		case MSG_CODE_END:
			waiting = CMsg__cmd_END(msg);
			break;
		case MSG_CODE_SPEED: {
			s32 delay = ReadMsgU8(msg);
			if (delay == 255) {
				if (msg->mCommand == EVT_CMD_MSG) {
					delay = 2;
				} else {
					delay = 0;
				}
			}
			msg->mCharDelay = delay;
			waiting         = FALSE;
			break;
		}
		case MSG_CODE_WAIT:
			waiting = CMsg__cmd_WAIT(msg);
			break;
		case MSG_CODE_ANIM:
			waiting = CMsg__cmd_ANIM(msg);
			break;
		case MSG_CODE_DEFAULT: {
			u8 flags  = ReadMsgU8(msg);
			s32 value = ReadMsgU8(msg);
			value |= ReadMsgU8(msg) << 8;
			value |= ReadMsgU8(msg) << 16;
			value |= ReadMsgU8(msg) << 24;
			if (flags & 1) {
				value = gEventVars.mVars[value];
			}
			msg->mDefaultLine = value;
			waiting           = FALSE;
			break;
		}
		case MSG_CODE_SFX:
			waiting = CMsg__cmd_SFX(msg);
			break;
		case MSG_CODE_VOICE:
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
		case MSG_CODE_NUM_INPUT:
			msg->mDigitCount     = ReadMsgU8(msg);
			msg->mFirstEditDigit = ReadMsgU8(msg);
			msg->mFirstEditDigit = msg->mDigitCount - msg->mFirstEditDigit;
			msg->mSelectedDigit  = ReadMsgU8(msg) - 1;
			if (msg->mSelectedDigit > msg->mDigitCount - 1) {
				msg->mSelectedDigit = msg->mDigitCount - 1;
			} else if (msg->mSelectedDigit < msg->mFirstEditDigit) {
				msg->mSelectedDigit = msg->mFirstEditDigit;
			}
			waiting = FALSE;
			break;
		case MSG_CODE_QUESTION: {
			u32 crossMask     = ReadMsgU8(msg);
			u32 selectedIndex = ReadMsgU8(msg);
			if (gGame.GetPlayer()->mOXMenu.mStarted == 0) {
				OpenPlayerYesNoMenu(gGame.GetPlayer(), crossMask, selectedIndex);
				msg->mData.mCursor -= 3;
				waiting = TRUE;
			} else {
				CPad pad(0);
				HandleOXMenuInput(&gGame.GetPlayer()->mOXMenu, &pad);
				if (pad.mPad->mTrigger & PAD_BUTTON_A) {
					gGame.mEvt.mSelectResult = gGame.GetPlayer()->mOXMenu.mConfirmedIndex;
					waiting                  = FALSE;
				} else {
					msg->mData.mCursor -= 3;
					waiting = TRUE;
				}
			}
			break;
		}
		case MSG_CODE_STAY:
			msg->mStay = 1;
			waiting    = FALSE;
			break;
		default: {
			char text[3];
			if (MsgHasSjisLead(command)) {
				text[0] = command;
				text[1] = ReadMsgU8(msg);
				text[2] = 0;
			} else if (command == MSG_CODE_ICON) {
				text[0] = command;
				text[1] = ReadMsgU8(msg);
				text[2] = 0;
			} else {
				text[0] = command;
				text[1] = 0;
			}
			CMsgFont_AddChar(&msg->mFont, text);
			msg->mCharTimer = msg->mCharDelay;
			if (msg->mGlyphSoundGroup >= 0) {
				PlayMessageGlyphSound(&gGame.mSound, msg->mGlyphSoundGroup, msg->mFont.mSize / 22.0f, text[0], text[1]);
			}
			break;
		}
		}
	}
	CMsgFont_Update(&msg->mFont, step);
}
// the Wii build keeps this as its own function, shared with the message reset
static inline void ResetMsgFont(CMsgFont* font)
{
	font->mActive    = FALSE;
	font->mFadeIn    = FALSE;
	font->mFadeOut   = FALSE;
	font->mBlink     = FALSE;
	font->mFitLine   = TRUE;
	font->_04_04     = TRUE;
	font->mCharCount = 0;
	font->mLine      = 0;
	font->mPlacement = NULL;
	Vec zero         = { 0.0f, 0.0f, 0.0f };
	font->mPosition  = zero;
#if defined(VERSION_JP)
	font->mSize = font->mPrevSize = 22.0f;
#else
	font->mSize     = 22.0f;
	font->mPrevSize = NAN;
#endif
	font->mGlyphColor     = msgColorTable[0];
	font->mGlyphAngle     = 0.0f;
	font->mGlyphScaleY    = 1.0f;
	font->mGlyphScaleX    = 1.0f;
	font->mEffect         = 0;
	font->mSelectedLine   = -1;
	font->mOtherSelection = -1;
	font->mAlpha          = 0.0f;
	font->mTextAlpha      = 1.0f;
	font->mHighlightAlpha = 0.0f;
	font->mBlinkIcon      = 0;
	font->mBlinkPhase     = 0.0f;
	font->mTexOffset      = 0.0f;
	for (int i = 0; i < 400; i++) {
		font->mGlyphs[i].mPhaseX = font->mGlyphs[i].mPhaseY = (i * 75) % 360;
	}
}

static inline void ResetMsgState(CMsg* msg)
{
	msg->mData.mCursor = NULL;
#if !defined(VERSION_JP)
	msg->mData.mIconColumns = 0;
#endif
	ResetMsgFont(&msg->mFont);
	msg->mIdle         = 1;
	msg->mCharDelay    = 0.0f;
	msg->mCharTimer    = 0.0f;
	msg->mSelectResult = -1;
	msg->mSelectedLine = -1;
	msg->mDefaultLine  = 0;
	for (int i = 0; i < 9; i += 3) {
		for (int j = 0; j < 3; j++) {
			msg->mDigits[i + j] = 0;
		}
	}
	msg->mSelectedDigit   = 0;
	msg->mFirstEditDigit  = 0;
	msg->mDigitCount      = 0;
	msg->mStay            = 0;
	msg->mGlyphSoundGroup = -1;
}

extern "C" void CMsgFont_Init(CMsgFont* font, u8* data, f32 x, f32 y, XObj* placement, s32 fitLine, s32 mode);

/**
 * @note Address: 0x80081C74
 * @note Size: 0x508
 */
extern "C" u8* CMsg_Open(CMsg* msg, u8 command, u8* script, MsgFormatArgs* args)
{
	ResetMsgState(msg);
	msg->mCommand  = command;
	s32 savedCount = args->count;
	msg->mData.Format(script, args);
	msg->mData.Measure();
	args->count        = savedCount;
	msg->mData.mCursor = msg->mData.mText;
#if !defined(VERSION_JP)
	if (msg->mData.mIconColumns == 0) {
		// how many 24 pixel icons the icon strip holds, CMsgFont_AddChar clamps MSG_CODE_ICON's index to it
		TPLPalette* palette     = (TPLPalette*)(&gGame.mTpl)->mBuffer;
		CTexObj* texture        = &(&gGame.mTpl)->mTex;
		CTexObj icon            = texture->Init(&palette->descriptorArray[119], 0);
		msg->mData.mIconColumns = (s32)GXGetTexObjWidth(&icon) / 24;
	}
#endif
	u8* cursor = msg->mData.mCursor;
	CMsgFont_Init(&msg->mFont, msg->mData.mText, msg->mWindowX, msg->mWindowY, msg->mPlacement, msg->mData.mAlignment, msg->mData.mStyle);
	msg->SetFontPosition(msg->mFontPosition.x, msg->mFontPosition.y, msg->mFontPosition.z);
	msg->mCharTimer = 0.0f;
	msg->mCharDelay = msg->mCommand == EVT_CMD_MSG ? 2 : 0;
	msg->mWaitTimer = 0.0f;
	return cursor;
}
/**
 * @note Address: 0x8008217C
 * @note Size: 0x60
 */
extern "C" void CMsg_ResetPos(CMsg* msg)
{
	msg->mWindowX        = 320.0f;
	msg->mWindowY        = 344.0f;
	msg->mPlacement      = 0;
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

/**
 * @note Address: 0x80082508
 * @note Size: 0xA0
 */
extern "C" BOOL LoadMessageCursorObject(CMsg* msg)
{
	StagePlacement* spawn = GetSpawnData(13);
	XObj* obj             = &msg->mFont.mCursor[0].mCursor;
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
	ResetMessageState(msg);
	CMsg_ResetPos(msg);
	GXColor color       = gMessageWindowDefaultColor;
	msg->mFont.mColor.r = color.r;
	msg->mFont.mColor.g = color.g;
	msg->mFont.mColor.b = color.b;
	msg->mFont.mColor.a = color.a;
	msg->mFont.mCursor[0].mCursor.InitializeObject(0);
}
extern "C" {
#if defined(VERSION_JP) || defined(VERSION_PAL)
void InvalidateMessageDrawState(CGs* gs);
#endif
}

#if defined(VERSION_PAL)
static inline void MsgRestoreScissor(u32 x, u32 y, u32 width, u32 height)
{
	f32 displayScale = gGame.mGs.mScreen.mHeightScale;
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
	XObj* anchor   = font->mPlacement;
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
	// only mode 0 has the portrait, and the text moves right by half its width
	f32 portraitWidth = (font->mMode >= 0 && font->mMode < 1) ? 40.0f * portraitScale : 0.0f;
	f32 textOffset    = portraitWidth;
	// 4.712389 is 3 pi / 2: as mAlpha rises from 0 to 1 the frame starts 4 pixels in, swells 4 out, then settles
	f32 expandX = 4.0f * (f32)sin(4.712389f * (1.0f - font->mAlpha));
	f32 expandY = 4.0f * (f32)sin(4.712389f * (1.0f - font->mAlpha));

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
		DrawGsVertexQuad(&gGame.mGs, left - 8.0f, top - 8.0f, -1.0f, outerRight, top - 8.0f, -1.0f, outerRight, outerBottom, -1.0f,
		                 left - 8.0f, outerBottom, -1.0f, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000);

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
		DrawGsVertexQuad(&gGame.mGs, left - 8.0f, top - 8.0f, -1.0f, outerRight, top - 8.0f, -1.0f, outerRight, outerBottom, -1.0f,
		                 left - 8.0f, outerBottom, -1.0f, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);

		if (font->mMode >= 0 && font->mMode < 1) {
			if (font->mColor.a != 0) {
#if defined(VERSION_PAL)
				f32 displayScale = gGame.mGs.mScreen.mHeightScale;
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
				if ((obj->mStatus.mFlags & 1) && obj->GetItem() == 10) {
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
					joint.CJObj::SetTranslate(portraitX, portraitY, depth, 1);
					joint.CJObj::SetScale(scale, -scale, scale, 1);
					joint.CJObj::SetRotationY(0.0f, 1);
					HSD_JObjSetupMatrix(joint.mJObj);
					s32 path[6] = { 0, 3, 0, 0, 0, 0 };
					f32 height  = CJObj_GetJointWorldPositionByPath(portrait, 6, path).y;
					joint.CJObj::SetTranslate(portraitX, portraitY + ((portraitY - height) - 12.0f), depth, 1);
					joint.CJObj::SetScale(scale, -scale, scale, 1);
					joint.CJObj::SetRotationY(0.0f, 1);
					HSD_JObjSetFlags(portrait->mJObj, 8);
					u32 oldModulateColor = portrait->mColors[1];
					u32 oldBlendColor    = portrait->mColors[0];
					portrait->mColors[1] = 0xFFFFFF00;
					portrait->mColors[0] = 0;
					portrait->DrawObject();
					joint.CJObj::SetTranslate(oldPosition, 1);
					joint.CJObj::SetScale(oldScale, 1);
					joint.CJObj::SetRotationY(oldYaw, 1);
					HSD_JObjClearFlags(portrait->mJObj, 8);
					portrait->mColors[1] = oldModulateColor;
					portrait->mColors[0] = oldBlendColor;
#if !defined(VERSION_JP)
				}
#endif
			} else {
				font->mCursor[0].mCursor.SetPos(portraitX, portraitY, 80.0f * portraitScale, 0);
				font->mCursor[0].mCursor.SetScale((9.0f * portraitScale) * font->mAlpha, 0);
				font->mCursor[0].mCursor.DrawObject();
			}
#if defined(VERSION_JP) || defined(VERSION_PAL)
			InvalidateMessageDrawState(&gGame.mGs);
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
			DrawGsVertexQuad(&gGame.mGs, quad[0].x, quad[0].y, quad[0].z, quad[1].x, quad[1].y, quad[1].z, quad[2].x, quad[2].y,
			                 quad[2].z, quad[3].x, quad[3].y, quad[3].z, color, color, color, color);
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
				f32 shakeY      = (glyph.mEffect & 1) ? glyph.mAmount * (f32)sin(DEG2RAD * glyph.mPhaseY) : 0.0f;
				f32 shakeX      = (glyph.mEffect & 2) ? glyph.mAmount * (f32)sin(DEG2RAD * glyph.mPhaseX) : 0.0f;
				f32 quarterTurn = HALF_PI + angle;
				f32 sideSin     = sin(quarterTurn);
				f32 forwardSin  = sin(angle);
				f32 dx          = shakeY * forwardSin + shakeX * sideSin;
				f32 sideCos     = cos(quarterTurn);
				f32 forwardCos  = cos(angle);
				f32 dy          = shakeY * forwardCos + shakeX * sideCos;
				f32 scaleX = glyph.mScaleX, scaleY = glyph.mScaleY;
				if (glyph.mEffect & 4) {
					f32 wave  = sin(DEG2RAD * glyph.mPhaseW);
					f32 scale = 0.1f * wave + 1.0f;
					glyph.mSprite.SetScale(scaleX * scale, scaleY * scale, 1.0f);
				}
				if (glyph.mEffect & 8) {
					// 0.52359879 is 30 degrees and SetRot gets angle + wave / 2, so the glyph rocks up to 15 degrees either way
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
	if (font->mBlink && (s32)font->mBlinkIcon >= 0) {
		SetGsTevOrder(gGame.mGs.mCurTevStage, 0, 0, 4);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		gGame.mGs.mCurTevStage->SetNumTexGens(1);
		gGame.mGs.mCurTevStage->SetTexGen(0, 1, 4, 30);
		s32 index = font->mBlinkIcon;
		if (index >= 2)
			index = 2;
		CTexObj* cursor = &font->mTexs.mTex[MSG_TEX_CURSOR] + index;
		f32 width       = GXGetTexObjWidth(cursor);
		f32 height      = GXGetTexObjHeight(cursor);
		if ((s32)font->mBlinkIcon >= 2) {
			width = (s32)GXGetTexObjWidth(cursor) / 11;
			PSMTXScale(cursor->mMtx[1], 1.0f / 11.0f, 1.0f, 1.0f);
			cursor->mMtxDirty = TRUE;
			PSMTXTrans(cursor->mMtx[0], (1.0f / 11.0f) * ((s32)font->mBlinkIcon - 2), 0.0f, 0.0f);
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
			// zero-alpha tint and no voice code (mMode stays -1, see CMsgData::Measure): no fade in
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
		// a select() line or a number input digit is picked: fade the highlight in, else out
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
			// CMsgFont_Draw fades the button prompt with sin(mBlinkPhase): 6 degrees a frame, wrapped at PI, a pulse every 30 frames
			font->mBlinkPhase += DEG2RAD * 6.0f * step;
			font->mBlinkPhase = fmod(font->mBlinkPhase, (f64)PI);
		}
	}

	if (font->mActive) {
		for (s32 i = 0; i < 400; i++) {
			MsgFontGlyph& glyph = font->mGlyphs[i];
			// CMsgFont_Draw: bits 1 and 2 shift the glyph, 4 pulses its scale, 8 rocks it; the high nibble sets how the phases move
			switch (glyph.mEffect & 0xF0) {
			case 0x10:
				// smooth: every phase turns by its share of mSpeed degrees a frame
				// this one adds in u32 (step is u32) before going to float, so a sum below 0 wraps
				glyph.mPhaseY = (s16)(f32)fmod((f32)(glyph.mPhaseY + glyph.mSpeed * step), 360.0);
				glyph.mPhaseX = (s16)(f32)fmod(glyph.mPhaseX + (0.5f * glyph.mSpeed) * step, 360.0);
				glyph.mPhaseZ = (s16)(f32)fmod(glyph.mPhaseZ + (0.7f * glyph.mSpeed) * step, 360.0);
				glyph.mPhaseW = (s16)(f32)fmod(glyph.mPhaseW + (0.8f * glyph.mSpeed) * step, 360.0);
				break;
			case 0x20:
				// jitter: fresh random phases every mSpeed updates
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
	// only mode 0 has the portrait, CMsgFont_Draw leaves 40 * scale of room for it
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
	// half the size change since the previous glyph; outside japan mPrevSize is NAN until the first one (CMsgFont_Init)
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
	if ((u8)text[0] == MSG_CODE_ICON) {
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
	if ((u8)text[0] == MSG_CODE_ICON) {
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
		// a negative MSG_CODE_SCALE: each glyph gets |x| plus a random part of the gap up to |y|
		f32 min = 100.0f * font->mGlyphScaleX;
		if (min < 0.0f)
			min = -min;
		f32 max = 100.0f * font->mGlyphScaleY;
		if (max < 0.0f)
			max = -max;
		f32 extra = fmod((f32)rand(), max - min);
		f32 scale = font->mGlyphScaleX;
		if (scale < 0.0f)
			scale = -scale;
		sx = sy = scale + extra / 100.0f;
	}
	f32 scaleX = (font->mSize * 0.5f) * sx;
	f32 scaleY = (-font->mSize * 0.5f) * sy;
	font->mGlyphs[font->mCharCount].mSprite.SetScale(scaleX, scaleY, 1.0f);
	f32 angle = font->mGlyphAngle;
	// a negative MSG_CODE_ROTATE angle: each glyph gets a random tilt between angle and -angle
	if (!(angle >= 0.0f)) {
		f32 range = 2.0f * angle;
		if (range < 0.0f)
			range = -range;
		f32 delta = fmod((f32)rand(), RAD2DEG * range);
		angle     = DEG2RAD * delta + font->mGlyphAngle;
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
extern "C" u16 GXGetTexObjHeight(const GXTexObj* obj);

/**
 * @note Address: 0x80085B34
 * @note Size: 0x950
 */
extern "C" void CMsgFont_Init(CMsgFont* font, u8* data, f32 x, f32 y, XObj* placement, s32 fitLine, s32 mode)
{
	font->mData = (CMsgData*)data;
	ResetMsgFont(font);
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
	font->mCursorX                   = -((font->mFitLine ? font->mData->mWidths[font->mLine] : width) / 2.0f);
	font->mWindowX                   = x;
	font->mWidth                     = width;
	font->mCursorY                   = 0.0f;
	font->mWindowY                   = y;
	font->mHeight                    = height;
	font->mLineY[font->mLine]        = font->mCursorY + (font->mData->mHeights[0] - 22.0f) / 2.0f;
	font->mTexs.mTex[MSG_TEX_CURSOR] = texture->Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[MSG_INIT_DESCRIPTOR(115)], 0);
	font->mTexs.mTex[MSG_TEX_DIGIT]  = texture->Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[MSG_INIT_DESCRIPTOR(116)], 0);
	font->mTexs.mTex[MSG_TEX_ICON]   = texture->Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[MSG_INIT_DESCRIPTOR(119)], 0);
	font->mTexs.mTex[MSG_TEX_ARROW]  = texture->Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[MSG_INIT_DESCRIPTOR(117)], 0);
	font->mTexs.mTex[MSG_TEX_LINE]   = texture->Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[MSG_INIT_DESCRIPTOR(118)], 0);
#undef MSG_INIT_DESCRIPTOR
}
// bytes a code with its arguments, or a plain character, takes: how far CMsgData::Measure steps past it
static inline s32 MsgCommandLength(u8* p)
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

/**
 * @note Address: 0x80086484
 * @note Size: 0x528
 */
void CMsgData::Measure()
{
	mLineCount           = 0;
	mCounts[mLineCount]  = 0;
	mWidths[mLineCount]  = 0.0f;
	mHeights[mLineCount] = 0.0f;
	mStyle               = -1;
	mAlignment           = 1;
	f32 size             = 22.0f;
	u8* p                = mText;
	BOOL proportional    = TRUE;
	s32 done             = 0;
	while (!done) {
		switch (*p) {
		case MSG_CODE_NEWLINE:
		case MSG_CODE_NEWLINE_VT:
			if (mHeights[mLineCount] == 0.0f) {
				mHeights[mLineCount] = size;
			}
			mLineCount++;
			mCounts[mLineCount]  = 0;
			mWidths[mLineCount]  = 0.0f;
			mHeights[mLineCount] = size;
			break;
		case MSG_CODE_END:
			if (mHeights[mLineCount] == 0.0f) {
				mHeights[mLineCount] = 22.0f;
			}
			mLineCount++;
			done = 1;
			break;
		case MSG_CODE_SIZE:
			size = (s32)p[1];
			if (size == 255.0f) {
				size = 22.0f;
			}
			if (size > mHeights[mLineCount]) {
				mHeights[mLineCount] = size;
			}
			break;
		case MSG_CODE_LAYOUT:
			proportional = p[1];
			break;
		case MSG_CODE_CENTER:
			mAlignment = p[1];
			break;
		case MSG_CODE_VOICE:
			// the voice byte doubles as the window's mode, CMsg_Open passes mStyle to CMsgFont_Init
			mStyle = p[1];
			break;
		case MSG_CODE_SPEED:
		case MSG_CODE_WAIT:
		case MSG_CODE_ANIM:
		case MSG_CODE_SFX:
		case MSG_CODE_DEFAULT:
		case MSG_CODE_COLOR:
		case MSG_CODE_RGBA:
		case MSG_CODE_SHAKE:
		case MSG_CODE_ROTATE:
		case MSG_CODE_SCALE:
		case MSG_CODE_NUM_INPUT:
		case MSG_CODE_QUESTION:
		case MSG_CODE_STAY:
			break;
		default:
			if (MsgHasSjisLead(*p)) {
				mCounts[mLineCount]++;
				char text[3] = "";
				text[0]      = p[0];
				text[1]      = p[1];
				mWidths[mLineCount] += MeasureGameTextWidth(&gGame.mGs, text, proportional, size);
			} else if (*p == MSG_CODE_ICON) {
#if defined(VERSION_JP)
				TPLPalette* palette = (TPLPalette*)(&gGame.mTpl)->mBuffer;
				CTexObj* texture    = &(&gGame.mTpl)->mTex;
				CTexObj icon        = texture->Init(&palette->descriptorArray[117], 0);
				mWidths[mLineCount] += (size / 22.0f) * (GXGetTexObjWidth(&icon) / 14);
#else
				TPLPalette* palette = (TPLPalette*)(&gGame.mTpl)->mBuffer;
				CTexObj* texture    = &(&gGame.mTpl)->mTex;
				CTexObj icon        = texture->Init(&palette->descriptorArray[119], 0);
				// a 24 pixel cell, the width CMsg_Open divides the icon strip by for mIconColumns
				mWidths[mLineCount] += 24.0f * (size / 22.0f);
#endif
			} else {
				mCounts[mLineCount]++;
				char text[2] = "";
				text[0]      = p[0];
				mWidths[mLineCount] += MeasureGameTextWidth(&gGame.mGs, text, proportional, size);
			}
			break;
		}
		p += MsgCommandLength(p);
	}
	mMaxWidth    = 0.0f;
	mMaxHeight   = 0.0f;
	mTotalHeight = 0.0f;
	for (s32 i = 0; i < mLineCount; i++) {
		if (mMaxWidth < mWidths[i]) {
			mMaxWidth = mWidths[i];
		}
		if (mMaxHeight < mHeights[i]) {
			mMaxHeight = mHeights[i];
		}
		mTotalHeight += mHeights[i];
	}
}
#if defined(VERSION_JP) || defined(VERSION_PAL)
/**
 * @note Address: N/A
 * @note Size: ?
 */
// japan and europe only, the graphics state reset usa inlines into Draw
extern "C" void InvalidateMessageDrawState(CGs* gs)
{
	HSD_StateInvalidate(-1);
	CGsRenderMode* mode = gs->mCurRenderMode;
	if (mode != NULL)
		mode->Invalidate();
	CGsTevStage* stage = gs->mCurTevStage;
	if (stage != NULL)
		stage->Invalidate();
	CGsChannel* channel = gs->mCurChannel;
	if (channel != NULL && !channel->mDirty)
		channel->mDirty = TRUE;
}
#endif
