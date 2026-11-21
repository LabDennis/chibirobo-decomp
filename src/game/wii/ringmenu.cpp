// The ring menu's file on Wii, .text 0x800E9FC4-: a separate object from xobj_chr.cpp
// (its .text ends at 0x800E9D30; retail links ringmenupointer.cpp between the two). The GameCube
// keeps all of it inside obja_chr.cpp; here the refresh rate is gRefreshRate instead of the
// IDLE_SECOND constant and the pointer input is the Wii Remote's.
#include "game/ringmenu.h"
#include "game/globals.h"
#include "game/gmath.h"
#include <math.h>

extern s32 gRefreshRate;

static inline void RaiseRingValue(f32& value, f32 target, f32 rate, u32 frames)
{
	const f32 next = value + rate * frames;
	value          = next;
	if (next >= target)
		value = target;
}

static inline void LowerRingValue(f32& value, f32 target, f32 rate, u32 frames)
{
	const f32 next = value - rate * frames;
	value          = next;
	if (next <= target)
		value = target;
}

static inline void ApproachItemScale(f32& value, f32 target, f32 step)
{
	if (value < target) {
		if ((value += step) > target)
			value = target;
	} else if (value > target) {
		if ((value -= step) < target)
			value = target;
	}
}

static inline void AddPulse(s32& red, s32& green, s32& blue, const OctaMenu* menu, s32 index)
{
	red += (s32)(25.0f * (f32)sin(1.2f * menu->mItems[index].mPulsePhase) + 25.0f);
	green += (s32)(25.0f * (f32)sin(1.2f * menu->mItems[index].mPulsePhase) + 25.0f);
	blue += (s32)(25.0f * (f32)sin(1.2f * menu->mItems[index].mPulsePhase) + 25.0f);
}

// the ring's fade, glow, item and label animation each frame; the Wii reads the refresh
// rate out of gRefreshRate where the GameCube had the IDLE_SECOND constant
/**
 * @note Address: 0x800E9FC4
 * @note Size: 0x974
 */
// NONMATCHING: register allocation: the highlight loop keeps its converted frames and the 1.0
// constant in f4/f5, retail swaps them
extern "C" void UpdateEquipmentRingAnimation(OctaMenu* ring, u32 frames)
{
	if (ring->mFocus != 0) {
		if (ring->mMenuType == 3) {
			if (ring->mFade < 1.0f)
				RaiseRingValue(ring->mFade, 1.0f, 1.0f / (0.18f * gRefreshRate), frames);
			else if (ring->mFade > 1.0f)
				LowerRingValue(ring->mFade, 1.0f, 1.0f / (0.18f * gRefreshRate), frames);
		} else {
			if (ring->mFade < 0.7f)
				RaiseRingValue(ring->mFade, 0.7f, 1.0f / (0.25f * gRefreshRate), frames);
			else if (ring->mFade > 0.7f)
				LowerRingValue(ring->mFade, 0.7f, 1.0f / (0.25f * gRefreshRate), frames);
		}
		if (ring->mGlow < 1.0f) {
			if (ring->mMenuType == 3)
				ring->mGlow = 1.0f;
			else
				RaiseRingValue(ring->mGlow, 1.0f, 1.0f / (0.35f * gRefreshRate), frames);
		}
	} else {
		if (ring->mOpen > 0) {
			if (ring->mFade < 1.0f)
				RaiseRingValue(ring->mFade, 1.0f, 1.0f / (0.18f * gRefreshRate), frames);
		} else if (ring->mOpen < 0) {
			if (ring->mFade > 0.0f) {
				if (ring->mMenuType == 3)
					LowerRingValue(ring->mFade, 0.0f, 1.0f / (0.18f * gRefreshRate), frames);
				else
					LowerRingValue(ring->mFade, 0.0f, 1.0f / (0.25f * gRefreshRate), frames);
			}
			if (ring->mFade == 0.0f)
				ring->FinishClose();
		}
		if (ring->mGlow > 0.0f) {
			if (ring->mMenuType == 3)
				ring->mGlow = 0.0f;
			else
				LowerRingValue(ring->mGlow, 0.0f, 1.0f / (0.35f * gRefreshRate), frames);
		}
	}
	if (ring->mFade > 0.0f) {
		for (s32 i = 0; i < 8; ++i) {
			if (ring->mFocus != 0) {
				if (ring->mMenuType == 3)
					ApproachItemScale(ring->mItems[i].mScale, 0.4f, 0.02f * frames);
				else
					ApproachItemScale(ring->mItems[i].mScale, 0.3f, 0.01f * frames);
			} else {
				ring->mItems[i].mPulsePhase += (DEG2RAD * 8.0f) * frames;
				ring->mItems[i]._0C += (DEG2RAD * 3.0f) * frames;
				if (ring->mMenuType == 3) {
					if (i == ring->mCursorIndex)
						ring->mItems[i].mScale = 0.5f;
					else
						ring->mItems[i].mScale = 0.4f;
				} else if (i == ring->mCursorIndex)
					ApproachItemScale(ring->mItems[i].mScale, 0.7f, 0.03f * frames);
				else
					ApproachItemScale(ring->mItems[i].mScale, 0.5f, 0.02f * frames);
			}
		}
		if ((ring->mPhase += (1.0f / (0.3f * gRefreshRate)) * frames) >= 2.0f)
			ring->mPhase = fmod(ring->mPhase, 2.0);
	}
	if (ring->mFade > 0.0f) {
		for (s32 i = 0; i < (s32)ARRAY_SIZE(ring->mLabelChars); ++i) {
			ring->mLabelChars[i].mAngle0 += (DEG2RAD * 5.0f) * frames;
			ring->mLabelChars[i].mAngle1 += (DEG2RAD * 3.0f) * frames;
		}
	}
	if (ring->mOpen > 0) {
		ring->mOpen = 2;
		if (ring->mFade >= 0.5f) {
			for (s32 i = 0; i < 8; ++i) {
				if (i == ring->mCursorIndex)
					RaiseRingValue(ring->mItems[i].mHighlight, 1.0f, 1.0f / (0.2f * gRefreshRate), frames);
				else if (ring->mMenuType == 3)
					ring->mItems[i].mHighlight = 0.0f;
				else
					LowerRingValue(ring->mItems[i].mHighlight, 0.0f, 1.0f / (0.25f * gRefreshRate), frames);
			}
		}
	} else if (ring->mOpen < 0) {
		for (s32 i = 0; i < 8; ++i)
			LowerRingValue(ring->mItems[i].mHighlight, 0.0f, 1.0f / (0.25f * gRefreshRate), frames);
	}
}

// marks the ring closed once its fade reached 0
/**
 * @note Address: 0x800EA938
 * @note Size: 0xC
 */
void OctaMenu::FinishClose()
{
	mOpen = 0;
}

// the item's color, pulsing while selected
/**
 * @note Address: 0x800EA944
 * @note Size: 0x144
 */
// NONMATCHING: regalloc and li scheduling, same wall as the GameCube's copy
u32 OctaMenu::GetItemColor(s32 index)
{
	s32 green, blue, red;
	if (mItems[index].mAvailable == 0) {
		blue  = 32;
		green = 32;
		red   = 32;
	} else if (mMenuType == 3) {
		red   = 50;
		green = 50;
		blue  = 200;
	} else if (index == mCursorIndex) {
		red   = 200;
		green = 100;
		blue  = 100;
	} else {
		red   = 50;
		green = 50;
		blue  = 200;
	}
	if (index == mCursorIndex)
		AddPulse(red, green, blue, this, index);
	return ((u32)red << 24) | ((u8)green << 16) | ((u8)blue << 8);
}

/**
 * @note Address: 0x800ECAC8
 * @note Size: 0x8
 */
s32 OctaMenu::GetItemIconTextureIndex(s32 index)
{
	return 0;
}

// the base menu equips nothing
/**
 * @note Address: 0x800ECAD0
 * @note Size: 0x8
 */
BOOL OctaMenu::IsItemEquipped(s32 index)
{
	return 0;
}

// the base menu has no labels; the Wii keeps an empty buffer where the GameCube returned ""
/**
 * @note Address: 0x800ECAD8
 * @note Size: 0x8
 */
const char* OctaMenu::GetItemLabel(s32 index)
{
	static char sLabel[8] = "";
	return sLabel;
}
