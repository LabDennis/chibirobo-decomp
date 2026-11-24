// The ring menu's file on Wii, .text 0x800E9FC4-: a separate object from xobj_chr.cpp
// (its .text ends at 0x800E9D30; retail links ringmenupointer.cpp between the two). The GameCube
// keeps all of it inside obja_chr.cpp; here the refresh rate is gRefreshRate instead of the
// IDLE_SECOND constant and the pointer input is the Wii Remote's.
#include "game/attachmentvalues.h"
#include "game/ringmenu.h"
#include "game/inventory.h"
#include "game/wii/cgame.h"
#include "game/cprimitive.h"
#include "game/cfont.h"
#include "game/ctpl.h"
#include "game/wii/xobj.h"
#include "game/globals.h"
#include "game/gmath.h"
#include <math.h>
#include <string.h>

extern s32 gRefreshRate;

extern "C" {
BOOL IsCarryingMug(XAction* action);
BOOL IsCarryingSquirter(XAction* action);
BOOL IsCarryingSpoon(XAction* action);
BOOL IsCarryingToothbrush(XAction* action);
}

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

// the Wii sizes the ring to the framebuffer: the first-person center is the
// pointer's screen, the row menu's the ring's own screen rectangle
extern s32 gPointerScreenWidth;
extern s32 gScreenSafeBottom;
extern s32 gRingScreenWidth;
extern s32 gRingScreenHeight;

// NONMATCHING: the row table and the icon size cache trail gPointParticlePool in .bss
// (pool + 0x3F38/0x3F58/0x3F78); retail keeps the pool base in a register and folds the
// three arrays' addresses into it, we load each one's address separately
static f32 gEquipmentRowY[8];
static f32 gEquipmentIconWidth[8];
static f32 gEquipmentIconHeight[8];

static const f32 gEquipmentRowX[8]     = { -3, -2, -1, 0, 1, 2, 3, 4 };
static const f32 gEquipmentRingY[8]    = { -1, -1, -1, 0, 1, 1, 1, 0 };
static const f32 gEquipmentRingX[8]    = { -1, 0, 1, 1, 1, 0, -1, -1 };
static const f32 gEquipmentOutlineX[4] = { -1, 1, 1, -1 };
static const f32 gEquipmentOutlineY[4] = { -1, -1, 1, 1 };

static inline CTexObj& RingTexture(OctaMenu* menu, s32 index)
{
	s32 id = menu->GetItemIconTextureIndex(index);
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[id], 0);
	return gGame.mTpl.mTex;
}

// the Wii's text is Japanese with the language setting at 0
static inline bool HelpTextIsJapanese()
{
	return gGame.mLanguage == 0;
}

static inline void DrawRingBandLines(f32 x0, f32 x1, f32 x2, f32 x3, f32 y0, f32 y1, u32 ink)
{
	DrawGsLineSegment(&gGame.mGs, NULL, x1, y0, 0, ink, x2, y1, 0, ink);
	DrawGsLineSegment(&gGame.mGs, NULL, x0, y0, 0, 0xffffff00, x1, y1, 0, ink);
	DrawGsLineSegment(&gGame.mGs, NULL, x2, y0, 0, ink, x3, y1, 0, 0xffffff00);
}

static inline void DrawRingQuad(CGs& gs, CTexObj& tex, u32 color, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rotZ = 0, f32 rotY = 0)
{
	DrawGsTexturedQuad(&gs, &tex, color, color, color, color, x, y, z, w, h, 0, rotY, rotZ);
}

// NONMATCHING: ~80 %: the three .bss scratch arrays are folded into the
// gPointParticlePool base register in retail (pool + 0x3F38/0x3F58/0x3F78) which no
// source spelling produces, and the icon/label loops' register allocation and
// scheduling run long; also open: retail reads mLastPointedItem for the cursor slot
// and gWiiPointer inside the icon loop — their exact expressions are not recovered yet
/**
 * @note Address: 0x800EAA88
 * @note Size: 0x2040
 */
extern "C" void DrawEquipmentRing(OctaMenu* menu)
{
	if (!(menu->mFade > 0))
		return;
	f32 originX, originY;
	f32 slotY[8], slotX[8];
	// in first person the ring sits below the screen center
	if (gGame.GetStatusFlags(0x100)) {
		originX = gPointerScreenWidth / 2;
		originY = gScreenSafeBottom - 0x8e;
	} else if (menu->mMenuType == 3) {
		originX = gRingScreenWidth / 2;
		originY = gRingScreenHeight / 2 - 0xC;
	} else {
		Vec offset = { 0, 8.8f, 0 };
		menu->mChr->ProjectOffsetToScreen(&offset, &originX, &originY, 0, NULL);
	}
	f32 fade = menu->mFade;
	f32 scale;
	// menu type 3 puts the items in a row that scrolls sideways, type 0 on a square, the others on a circle
	if (menu->mMenuType == 3) {
		for (s32 i = 0; i < 8; ++i) {
			s32 slot = i - menu->mFirstVisibleItem;
			if (slot < 0)
				slot += 7;
			slotY[i]                          = menu->mRowOffset + gEquipmentRowY[slot];
			menu->mItems[i].mTargetSlotOffset = menu->mFade * gEquipmentRowX[slot];
			if (menu->mFade < 1.f) {
				menu->mItems[i].mSlotOffset = menu->mItems[i].mTargetSlotOffset;
			} else {
				if (menu->mItems[i].mSlotOffset < 0 && menu->mItems[i].mTargetSlotOffset == 3)
					menu->mItems[i].mSlotOffset = 4;
				else if (menu->mItems[i].mSlotOffset > 0 && menu->mItems[i].mTargetSlotOffset == -3)
					menu->mItems[i].mSlotOffset = -4;
				f32 step = menu->mItems[i].mTargetSlotOffset - menu->mItems[i].mSlotOffset;
				if (step > 1) {
					step                        = 1;
					menu->mItems[i].mSlotOffset = menu->mItems[i].mTargetSlotOffset - 1;
				}
				if (step < -1) {
					step                        = -1;
					menu->mItems[i].mSlotOffset = menu->mItems[i].mTargetSlotOffset + 1;
				}
				menu->mItems[i].mSlotOffset += .4f * step;
			}
			slotX[i] = menu->mItems[i].mSlotOffset;
		}
		scale = 1.1f;
	} else if (menu->mMenuType == 0) {
		for (int i = 0; i < 8; ++i) {
			slotY[i] = gEquipmentRingY[i];
			slotX[i] = gEquipmentRingX[i];
		}
		scale = 1.1f;
	} else {
		for (s32 j = -1, i = 0; i < 8; ++j, ++i) {
			f32 angle = (PI / 4.f) * j;
			slotY[i]  = -1.f * std::cosf(angle);
			slotX[i]  = std::sinf(angle);
		}
		scale = 1.35f;
	}

	for (s32 i = 0; i < 8; ++i) {
		CTexObj tex = RingTexture(menu, i);
		u16 w = GXGetTexObjWidth(&tex), h = GXGetTexObjHeight(&tex);
		gEquipmentIconWidth[i]  = 100.f;
		gEquipmentIconHeight[i] = 100.f * ((f32)h / w);
	}
	f32 spread = menu->mGlow > .5f ? 0 : 100;
	s32 count  = menu->mMenuType == 3 ? 7 : 8;
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gGame.mGs.mCurRenderState->SetLineWidth(12, (GXTexOffset)5);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
	if (menu->mMenuType == 3) {
		s32 start = menu->mFirstVisibleItem, last = (start + 6) % 7;
		f32 itemSize = menu->mItems[start].mScale * std::sinf((PI * .5f) * fade);
		if (itemSize > .4f)
			itemSize = .4f;
		f32 radius = 60.f + 20.f * itemSize;
		f32 y0     = originY + (scale * slotY[start]) * radius;
		f32 left   = originX + (-3.f * scale) * radius;
		f32 y1     = originY + (scale * slotY[last]) * radius;
		f32 right  = originX + (3.f * scale) * radius;
		f32 half   = .7f * (itemSize * gEquipmentIconHeight[start]);

		f32 labelY    = y0 + 24.f;
		u32 fill      = (u32)(128.f * fade);
		f32 highlight = 0;
		for (s32 i = 0; i < count; ++i) {
			if (menu->mItems[i].mHighlight > highlight)
				highlight = menu->mItems[i].mHighlight;
		}
		// The vertex buffer also contains the strip below the icons for their labels.
		u32 labelAlpha      = (u32)(128.f * highlight);
		u32 clear           = 0;
		PrimVertex band[24] = { { (Vec) { ((left - half) - 2.f * half), (y0 - half), 0 }, clear },
			                    { (Vec) { (left - half), (y0 - half), 0 }, fill },
			                    { (Vec) { (left - half), (y0 + half), 0 }, fill },
			                    { (Vec) { ((left - half) - 2.f * half), (y0 + half), 0 }, clear },
			                    { (Vec) { (left - half), (y0 - half), 0 }, fill },
			                    { (Vec) { (right + half), (y0 - half), 0 }, fill },
			                    { (Vec) { (right + half), (y0 + half), 0 }, fill },
			                    { (Vec) { (left - half), (y0 + half), 0 }, fill },
			                    { (Vec) { (right + half), (y0 - half), 0 }, fill },
			                    { (Vec) { ((right + half) + 2.f * half), (y0 - half), 0 }, clear },
			                    { (Vec) { ((right + half) + 2.f * half), (y0 + half), 0 }, clear },
			                    { (Vec) { (right + half), (y0 + half), 0 }, fill },
			                    { (Vec) { ((left - half) - 2.f * half), labelY, 0 }, clear },
			                    { (Vec) { (left - half), labelY, 0 }, labelAlpha },
			                    { (Vec) { (left - half), labelY + half, 0 }, labelAlpha },
			                    { (Vec) { ((left - half) - 2.f * half), labelY + half, 0 }, clear },
			                    { (Vec) { (left - half), labelY, 0 }, labelAlpha },
			                    { (Vec) { (right + half), labelY, 0 }, labelAlpha },
			                    { (Vec) { (right + half), labelY + half, 0 }, labelAlpha },
			                    { (Vec) { (left - half), labelY + half, 0 }, labelAlpha },
			                    { (Vec) { (right + half), labelY, 0 }, labelAlpha },
			                    { (Vec) { ((right + half) + 2.f * half), labelY, 0 }, clear },
			                    { (Vec) { ((right + half) + 2.f * half), labelY + half, 0 }, clear },
			                    { (Vec) { (right + half), labelY + half, 0 }, labelAlpha } };
		DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, band, 4);
		DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, band + 4, 4);
		DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, band + 8, 4);
		u32 ink = 0xffffff00 | (u32)(255.f * fade);
		DrawRingBandLines((left - half) - 2.f * half, left - half, right + half, (right + half) + 2.f * half, y0 - half, y1 - half, ink);
		DrawRingBandLines((left - half) - 2.f * half, left - half, right + half, (right + half) + 2.f * half, y0 + half, y1 + half, ink);
	}
	CTexObj background = RingTexture(menu, -1);
	for (s32 i = 0; i < count; ++i) {
		CTexObj tex = RingTexture(menu, i);
		f32 width = gEquipmentIconWidth[i], height = gEquipmentIconHeight[i];
		std::sinf(menu->mItems[i].mPulsePhase);
		std::sinf(menu->mItems[i]._0C);
		std::sinf(.5f * menu->mItems[i]._0C);
		std::sinf(.7f * menu->mItems[i]._0C);
		f32 pulse;
		if (menu->mMenuType == 3)
			pulse = menu->mItems[i].mScale * std::sinf((PI * .5f) * fade);
		else
			pulse = menu->mItems[i].mScale * std::sinf(2.0943952f * fade);
		f32 x, y;

		if (menu->mMenuType == 3) {
			y        = originY + 68.f * (scale * slotY[i]);
			x        = originX + 68.f * (scale * slotX[i]);
			s32 slot = i - menu->mFirstVisibleItem;
			if (slot < 0)
				slot += 7;
			if (slot == menu->mLastPointedItem) {
				menu->mCursorY = y;
				menu->mCursorX = menu->mFade < 1.f ? x : (f32)(gRingScreenWidth / 2);
			}
		} else {
			f32 radius = 60.f + 20.f * menu->mItems[i].mScale;
			y          = 10.f + ((originY + fade * (radius * (scale * slotY[i]))) - (.5f * width) * .5f);
			x          = originX + fade * (radius * (scale * slotX[i]));
		}
		if (menu->mFocus && menu->mMenuType != 3) {
			if (menu->mMenuType == 0) {
				y += 80.f * (slotY[i] * std::sinf(PI * menu->mGlow));
				x += 80.f * (slotX[i] * std::sinf(PI * menu->mGlow));
			} else {
				y += spread * (slotY[i] * std::sinf(PI * menu->mGlow));
				x += spread * (slotX[i] * std::sinf(PI * menu->mGlow));
			}
		}
		f32 slot = menu->mItems[i].mSlotOffset;
		f32 alpha;
		if (slot >= 3.02f)
			alpha = Abs(4.f - slot);
		else if (slot <= -3.02f)
			alpha = Abs(4.f + slot);
		else
			alpha = 1.f;
		menu->GetItemColor(i);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		f32 iconAlpha = alpha;
		if (menu->IsItemEquipped(i))
			iconAlpha = alpha * .25f;
		u32 color = 0xffffff00 | (u32)(255.f * alpha);
		f32 size  = 1.2f * pulse;
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
		gGame.mGs.mCurRenderState->SetZCompLoc(GX_FALSE);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		DrawRingQuad(gGame.mGs, background, color, x, y, 0, width * size, height * size);
		DrawRingQuad(gGame.mGs, tex, 0xffffff00 | (u32)(255.f * iconAlpha), x, y, 0, width * size, height * size);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetZCompLoc(GX_TRUE);
		if (i == menu->mCursorIndex) {
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCCOL, GX_BL_SRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_EQUAL, GX_FALSE);
			f32 glow = fade * (.2f * std::sinf(PI * menu->mPhase) + .2f);
			if (glow < 0)
				glow = 0;
			u32 glowColor = 0xffffff00 | (u32)(255.f * glow);
			DrawGsQuad(&gGame.mGs, glowColor, glowColor, glowColor, glowColor, x, y, 0, width * size, height * size, 0, 0, 0);
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
		}
		// an item scrolling off one end of the row also shows at the other end
		if (menu->mMenuType == 3 && (menu->mItems[i].mSlotOffset >= 3.02f || menu->mItems[i].mSlotOffset <= -3.02f)) {
			f32 span = 7.f * scale;
			if (menu->mItems[i].mSlotOffset >= 3.02f) {
				x     = x - span * (20.f * menu->mItems[i].mScale + 60.f);
				alpha = Abs(menu->mItems[i].mSlotOffset - 3.f);
			} else if (menu->mItems[i].mSlotOffset <= -3.02f) {
				x     = x + span * (20.f * menu->mItems[i].mScale + 60.f);
				alpha = Abs(3.f + menu->mItems[i].mSlotOffset);
			} else {
				alpha = 1.f;
			}
			menu->GetItemColor(i);
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			u32 wrapColor = 0xffffff00 | (u32)((255.f * fade) * alpha);
			gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			DrawRingQuad(gGame.mGs, background, wrapColor, x, y, 0, size * width, size * height);
			DrawRingQuad(gGame.mGs, tex, wrapColor, x, y, 0, size * width, size * height);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
	}
	gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_BACK);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	SetGsChannelEnabled(&gGame.mGs, 0, 0);
	gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
	f32 zero = 0.0f;
	for (s32 i = 0; i < count; ++i) {
		OctaMenuItem& item = menu->mItems[i];
		if (item.mHighlight == zero)
			continue;
		f32 width = gEquipmentIconWidth[i];
		std::sinf(2.0943952f * fade);
		f32 x, y;
		if (menu->mMenuType == 3) {
			y = originY + 68.f * (scale * slotY[i]);
			x = originX + 68.f * (scale * slotX[i]);
		} else {
			f32 radius = 60.f + 20.f * item.mScale;
			y          = 10.f + ((originY + fade * (radius * (scale * slotY[i]))) - (.5f * width) * .5f);
			x          = originX + fade * (radius * (scale * slotX[i]));
		}
		f32 highlight      = item.mHighlight;
		const char* source = menu->GetItemLabel(i);
		char label[128];
		strcpy(label, source);
		MeasureGameTextWidth(&gGame.mGs, label, TRUE, 22.f);
		f32 baseline = 52.f + y;
		f32 tileSize = 30.f;
		f32 spacing  = tileSize;
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		CTexObj tile       = RingTexture(menu, -2);
		menu->mLabelLength = 0;
		const char* p      = label;
		for (s32 k = 0; k < (s32)ARRAY_SIZE(menu->mLabelChars) && *p; ++k, ++p) {
			menu->mLabelChars[k].mText = p;
			++menu->mLabelLength;
			if (HelpTextIsJapanese() && (IsSjisLead((u8)*p) & 1))
				++p;
		}
		f32 halfSpacing = 0.5f * spacing;
		for (s32 k = 0; k < menu->mLabelLength; ++k) {
			MenuHelpChar& ch = menu->mLabelChars[k];

			if (!HelpTextIsJapanese() && strncmp(ch.mText, " ", 1) == 0)
				continue;
			CFont font;
			s32 width;
			CTexObj glyph = *GetFontGlyphTexture(&font, ch.mText, 0, &width);
			f32 bob       = 2.f * std::sinf(ch.mAngle0);
			f32 spin      = std::sinf(.7f * ch.mAngle1);
			f32 glow      = std::sinf(PI * highlight);
			f32 wave      = std::sinf((PI * .5f) * highlight);
			f32 cy        = baseline + bob;
			f32 scale     = .2f * glow + wave;
			f32 cx        = halfSpacing + (k * spacing + (x - (menu->mLabelLength * spacing) / 2.f));
			f32 rotation  = -(DEG2RAD * (10.f * spin));

			DrawRingQuad(gGame.mGs, tile, 0xffffff00 | (u32)(255.f * highlight), cx, cy, 1, tileSize * scale, tileSize * scale, rotation,
			             -zero);
			f32 glyphSize  = tileSize * (.8f * scale);
			f32 glyphWidth = (width / 24.0f) * glyphSize;
			f32 padding    = glyphSize - glyphWidth;

			for (s32 j = 0; j < 4; ++j)
				DrawRingQuad(gGame.mGs, glyph, (u32)(255.f * highlight), padding / 2.0f + (cx + gEquipmentOutlineX[j]),
				             bob + (baseline + gEquipmentOutlineY[j]), 1, glyphSize, glyphSize, rotation, -zero);
			DrawRingQuad(gGame.mGs, glyph, 0xfffffb00 | (u32)(255.f * highlight), padding / 2.0f + cx, cy, 1, glyphSize, glyphSize,
			             rotation, -zero);
		}
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	}
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
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

// the base menu has no labels; the Wii keeps an empty buffer where the GameCube returned "":
// retail reads lbl_805A1108, eight zero bytes in writable .sdata, not the unit's pooled strings
/**
 * @note Address: 0x800ECAD8
 * @note Size: 0x8
 */
const char* OctaMenu::GetItemLabel(s32 index)
{
	static char sLabel[8] = "";
	return sLabel;
}

// one of his tools is out
static inline BOOL IsToolOut(WiiPlayerState* chr, s16 item)
{
	switch (item) {
	case 0x10:
		return IsCarryingToothbrush(&chr->mLift);
	case 0x16:
		return IsCarryingMug(&chr->mLift);
	case 0x15:
		return IsCarryingSpoon(&chr->mLift);
	default:
		return IsCarryingSquirter(&chr->mLift);
	}
}

// he can use the tool, he has it or it is out
static inline s32 CanUseTool(WiiPlayerState* chr, s16 item)
{
	s32 has = 1;
	if (FindOwnedInventoryItemIndex(chr->mParams.mInventory.mItems, item) < 0 && !IsToolOut(chr, item))
		has = 0;
	return has;
}

// which of his tools he can use, he has them or one of them is going; the same
// helper xobj_chr.cpp inlines into GetAttachmentCountById
static inline void UpdateAttachmentAvailability(AttachmentValues* values)
{
	WiiPlayerState* chr = gGame.GetPlayer();
	values->SetCount(4, CanUseTool(chr, 0x10));
	values->SetCount(6, CanUseTool(chr, 0x16));
	values->SetCount(5, CanUseTool(chr, 0x15));
	values->SetCount(7, CanUseTool(chr, 0x38));
	values->SetCount(8, FALSE);
}

// how many of an attachment chibi robo has, the GameCube's helper of the same
// name; on the Wii the availability refresh is inlined in the count lookup
static inline s32 GetPlayerAttachmentCount(WiiPlayerState* player, s32 type)
{
	s32 index = player->mAttachments.Find(type);
	if (index >= 0) {
		UpdateAttachmentAvailability(&player->mAttachments);
		return player->mAttachments.mEntries[index].mValue;
	}
	return -1;
}

// AtcMenu::Open, OctaMenu's plus the item counts. The row menu's start item is
// not scrolled to on the Wii, the equipped one's row slot goes to
// mLastPointedItem instead, pending or found; the keep flag clears on the way out
/**
 * @note Address: 0x800ECAE0
 * @note Size: 0x8D8
 */
// NONMATCHING: register allocation only: identical instruction stream, retail keeps the
// CanUseTool result in r26 and the slot-8 FALSE constant in r21, we put them in r21/r22 and
// shift this/focus/i/item one register up
void AtcMenu::Open(s32 focus)
{
	for (s32 i = 0; i < 8; ++i) {
		if (mMenuType == 2)
			mItems[i].mAvailable = 1;
		else
			mItems[i].mAvailable = GetPlayerAttachmentCount((WiiPlayerState*)mChr, mItems[i].mType);
	}
	OctaMenu::Open(focus);
	if (mMenuType == 3) {
		if (mPendingFirstVisibleItem >= 0) {
			mLastPointedItem = mPendingFirstVisibleItem - 1;
			if (mLastPointedItem < 0)
				mLastPointedItem += 7;
			mPendingFirstVisibleItem = -1;
		} else if (((WiiPlayerState*)mChr)->GetAttachment() != 0 && !mKeepPointedItemOnOpen) {
			for (s32 i = 0; i < 8; ++i) {
				if (mItems[i].mType == ((WiiPlayerState*)mChr)->GetAttachment()) {
					mLastPointedItem = i - mFirstVisibleItem;
					if (mLastPointedItem < 0)
						mLastPointedItem += 7;
				}
			}
		}
	}
	mKeepPointedItemOnOpen = 0;
}
