#include "game/soundtracks.h"
#include "game/vistate.h"
#include "game/cmdline.h"
#include "game/stagedata.h"
#include "game/drawhelptext.h"
#include "types.h"
#include "Version.h"
#include "game/hsd_jobj.h"
extern "C" {
#include <dolphin/mtx/vec.h>
}
#include "game/cjobjset.h"
#include "game/cgame.h"
#include "game/cgameutil.h"
#include "game/cgs.h"
#include "game/cptcl.h"
#include "game/cscrn.h"
#include "game/screenwipegeometry.h"
#include "game/csound.h"
#include "game/globals.h"
#include "game/gmath.h"
#include "game/logo.h"
#include "game/memcard.h"
#include "game/menusave.h"
#include "game/menuselectionbar.h"
#include "game/menupanelbackground.h"
#include "game/title.h"
#include "game/video.h"
#include "game/xobjdemo.h"
#include "game/xobjs.h"
#include "game/xchr.h"
#include "Dolphin/OS/OSUtil.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Screen wipes and fades, dust particles, logos and the title screen.

extern "C" {
char* GetTitleAndSaveMenuText(s32 id);
void PSMTXMultVecArray(const Mtx m, const Vec* src, Vec* dst, u32 count);
void SetGameSoundStereo(XSound* sound, BOOL mono);
void HSD_PadRumbleOn(u8 no);
void HSD_PadRumbleOffN(u8 no);
void HSD_PadRumbleRemove(u8 no);
// a window frame, see the menus
void DrawTexturedWindowFrame(u32 color0, u32 color1, f32 x, f32 y, f32 w, f32 h, f32 sx, f32 sy, f32 sz);
void ResumeScrapCounterPulse(void* info);
s32 IndexOfLargestOfThree(OSTime first, OSTime second, OSTime third);
}

extern "C" s32 GetItemIconTextureIndex(s32 id);
extern "C" void DrawHudRoundedRectangle(u32 color, f32 x, f32 y, f32 w, f32 h, f32 rx, f32 ry);

static inline BOOL ProjectRoboeyesItem(XObj* object, f32* width, f32* height)
{
	f32 r = object->GetFloorXZExtent() / 2.0f;
	if (r < 1.0f)
		r = 1.0f;
	f32 h = object->GetCollisionHeight();
	if (h < 1.0f)
		h = 1.0f;
	return object->ProjectVisibleBounds(width, height, NULL, r, h);
}

extern "C" void DrawRoboeyesItemLabels(CScrnRoboeyes* eyes, CBase* base)
{
	if (!(gGame.mStatus.mFlags & 0x100))
		return;
	struct Label {
		XObj* object;
		f32 distance, left, y, width;
	};
	struct {
		Label entries[64];
		u32 used, visible;
	} work;
	work.visible = work.used = 0;
	XObjPool& pool           = gGame.mXObjs.mPools[8];
	for (s32 i = 0; i < pool.mUsed; ++i) {
		XObj* object = pool.Get(i);
		if (!(object->mStatus.mFlags & 1) || !(object->mStatus.mFlags & 0x10000))
			continue;
		if (!GetItemFlagsMasked(object->GetItem(), 2) || (object->mStatus.mFlags & 0x0400000000000000ULL))
			continue;
		f32 radius, height;
		if (ProjectRoboeyesItem(object, &radius, &height)) {
			work.entries[work.visible].object = object;
			Vec offset                        = { 0, 0, 0 };
			f32 x, y;
			object->ProjectOffsetToScreen(&offset, &x, &y, NULL, NULL);
			y -= radius / 2.0f;
			f32 dx = 320.0f - x, dy = 224.0f - y;
			work.entries[work.visible].distance = sqrtf(dx * dx + dy * dy);
			work.visible++;
		}
	}
	CGs* gs = &base->mGs;
	SetGsBlendMode(gs->mCurRenderMode, 1, 4, 5, 15);
	SetGsLineWidth(gs->mCurRenderMode, 12, 5);
	const u32 gold = 0xffb628ff;
	for (s32 pass = 0; pass < 2; ++pass) {
		for (u32 i = 0; i < work.visible; ++i) {
			XObj* object = work.entries[i].object;
			f32 radius, height;
			if (!ProjectRoboeyesItem(object, &radius, &height))
				continue;
			if (radius < 4)
				radius = 4;
			if (height < 4)
				height = 4;
			if (radius > height)
				radius = height;
			Vec offset = { 0, 0, 0 };
			f32 x, y, z;
			object->ProjectOffsetToScreen(&offset, &x, &y, &z, NULL);
			y -= radius / 2.0f;
			const char* name = (const char*)GetItemName(object->GetItem());
			f32 width        = 32.0f + MeasureGameTextWidth(gs, name, TRUE, 20);
			f32 angle        = 2.3561945f;
			f32 startX = x + radius * std::sinf(angle), startY = y + radius * std::cosf(angle);
			f32 endX = x + 1.7f * radius * std::sinf(angle), endY = y + 1.7f * radius * std::cosf(angle);
			f32 left = endX, right = endX + width;
			for (u32 j = 0; j < work.used; ++j) {
				const Label& prior = work.entries[j];
				f32 dy             = endY - prior.y;
				if (dy < 0)
					dy = -dy;
				f32 dx = left - prior.left;
				if (dx < 0)
					dx = -dx;
				f32 room = left > prior.left ? prior.width : width;
				if (dy < 32 && dx < room) {
					endY = prior.y - 33;
					if (left == right) {
						endX = startX - Abs(endY - startY) / 2.0f;
						left = right = endX - width;
					} else {
						endX  = startX + Abs(endY - startY) / 2.0f;
						left  = endX;
						right = endX + width;
					}
					// moved above that label, so test against every placed label again
					j = -1;
				}
			}
			work.entries[work.used].y     = endY;
			work.entries[work.used].left  = left;
			work.entries[work.used].width = width;
			work.used++;
			if (pass == 0) {
				f32 pulse = std::sinf(eyes->mPhase);
				f32 inner = radius + .1f * radius * pulse, outer = 1.2f * inner;
				for (s32 k = 0; (f32)k < 16.0f; ++k) {
					f32 outerAngle = .3926991f * k + eyes->mAngle, innerAngle = .3926991f * (k + 1) + eyes->mAngle;
					f32 x0 = x + outer * std::sinf(outerAngle), y0 = y + outer * std::cosf(outerAngle);
					f32 x1 = x + inner * std::sinf(innerAngle), y1 = y + inner * std::cosf(innerAngle);
					DrawGsLineSegment(gs, NULL, x0, y0, 0, gold, x1, y1, 0, gold);
				}
				DrawGsLineSegment(gs, NULL, startX, startY, 0, gold, endX, endY, 0, gold);
			}
			if (pass == 1) {
				DrawHudRoundedRectangle(160, left + width / 2.0f, endY - 16, width, 16, 1, 1);
				DrawGsLineSegment(gs, NULL, endX, endY, 0, gold, right, endY, 0, gold);
				s32 icon    = GetItemIconTextureIndex(object->GetItem());
				CTexObj tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[icon], 0);
				SetGsTevMode(gGame.mGs.mCurTevStage, 0);
				DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, left + 16, endY - 16, 0, 32, 32, 0, 0, 0);
				SetGsTevMode(gGame.mGs.mCurTevStage, 4);
				DrawGsProportionalTextFormat(&base->mGs, -1, name, left + 32, (endY - 20) - 6, 0, 20, 20);
			}
		}
		work.used = 0;
	}
	SetGsLineWidth(gs->mCurRenderMode, 12, 5);
}

extern "C" void ApproachHudValue(f32*, f32*, f32);

struct RoboeyesValue {
	f32 value, target;
	void Update()
	{
		f32 step = (target - value) / 2.0f;
		if (step < 0)
			step = -step;
		ApproachHudValue(&value, &target, step);
	}
};

static inline void DrawRoboeyesLens(CGs* gs, Vec* points, f32 x, f32 y, f32 rx, f32 ry, u32 color, u32 alpha, const f32* angles, bool right)
{
	for (s32 i = 0; i < 65; ++i) {
		f32 a  = .09817477f * i;
		f32 sx = (f32)sin(a), cy = (f32)cos(a);
		points[i].x = sx * rx + x;
		points[i].y = cy * ry + y;
		points[i].z = 0;
	}
	DrawGsLineStrip(gs, points, color, 65);
	u32 trailColor = color;
	for (s32 i = 0; i < 10; ++i) {
		for (s32 j = 0; j < 5; ++j) {
			f32 a  = HALF_PI * j;
			f32 sx = (f32)sin(right ? a - angles[i] : a + angles[i]);
			f32 cy = (f32)cos(right ? a - angles[i] : a + angles[i]);
			DrawGsLineSegment(gs, NULL, sx * rx + x, cy * ry + y, 0, trailColor, sx * (rx - 10.0f) + x, cy * (ry - 10.0f) + y, 0,
			                  trailColor);
		}
		trailColor = (color & ~0xffU) | (u8)(s32)(0.5f * ((f32)alpha * (10 - i) / 10.0f));
	}
}

// NONMATCHING: float registers only (retail keeps width and height in f19/f20, this build in f15/f16)
void CScrnRoboeyes::Draw(CGs* gs, s32 mode)
{
	if (!mActive)
		return;
	f32 x = mX, y = mY, width = mWidth, height = mHeight;
	u32 color = ((u32)mColor.r << 24) | ((u32)mColor.g << 16) | ((u32)mColor.b << 8) | (u32)mColor.a;
	SetGsChannelEnabled(gs, 0, 0);
	SetGsBlendMode(gs->mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gs->mCurRenderMode, 1, 7, 1);
	SetGsDepthMode(gs->mCurRenderMode, 1, 7, 0);
	u32 alpha           = color & 0xff;
	static f32 scanline = 0.0f;
	if (!gGame.mMenu.mActive) {
		if (++scanline >= 4.0f)
			scanline = 0;
	}
	SetGsBlendMode(gs->mCurRenderMode, 3, 4, 5, 15);
	SetGsLineWidth(gs->mCurRenderMode, 12, 5);
	if (mWidth >= mTargetWidth) {
		u32 tint = 0x30303000 | alpha;
		for (s32 i = 0; i < 448; i += 4.0f) {
			f32 line = i + scanline;
			DrawGsLineSegment(gs, NULL, 0, line, 0, tint, 640, line, 0, tint);
		}
	}
	DrawRoboeyesItemLabels(this, (CBase*)gs);
	SetGsBlendMode(gs->mCurRenderMode, 1, 4, 5, 15);
	SetGsLineWidth(gs->mCurRenderMode, 12, 5);
	static f32 trail[10];
	for (s32 i = 9; i > 0; --i)
		trail[i] = trail[i - 1];
	trail[0]                        = mRotation;
	static RoboeyesValue separation = { 150, 150 }, lw = { width, width }, lh = { height, height }, rw = { width, width },
	                     rh = { height, height }, focus = { 0, 0 };
	static s32 state = 0;
	// 0: lenses apart at full size until attachment 2 is out, 1: they slide together, 2: the right lens
	// shrinks to a half and the left to a sixth until it is put away, 3: back to full size, 4: apart again
	switch (state) {
	case 0:
		lw.target = width;
		lh.target = height;
		rw.target = width;
		rh.target = height;
		if (gGame.GetPlayer()->GetAttachment() == 2)
			state = 1;
		break;
	case 1:
		separation.target = 0;
		if (separation.value <= 0.1f) {
			state        = 2;
			focus.target = 120;
		}
		break;
	case 2:
		lw.target = width / 6.0f;
		lh.target = height / 6.0f;
		rw.target = width / 2.0f;
		rh.target = height / 2.0f;
		if (gGame.GetPlayer()->GetAttachment() != 2) {
			focus.target = 0;
			state        = 3;
		}
		break;
	case 3:
		lw.target = width;
		lh.target = height;
		rw.target = width;
		rh.target = height;
		if (lw.value >= width - 1.0f)
			state = 4;
		break;
	case 4:
		separation.target = 150;
		if (separation.value >= 149) {
			separation.value = 150;
			state            = 0;
		}
		break;
	}
	separation.Update();
	lw.Update();
	lh.Update();
	focus.Update();
	rw.Update();
	rh.Update();
	f32 offset = separation.value;
	f32 rx = .6f * (rw.value / 2.0f), ry = .6f * (rh.value / 2.0f);
	Vec points[65];
	DrawRoboeyesLens(gs, points, x - offset, y, .6f * (lw.value / 2.0f), .6f * (lh.value / 2.0f), color, alpha, trail, false);
	DrawRoboeyesLens(gs, points, x + offset, y, rx, ry, color, alpha, trail, true);
	if (state == 2) {
		CTexObj tex;
		static s32 frame = 0;
		++frame;
		u32 tint = 0xffffff00 | alpha;
		for (s32 i = 0; i < 4; ++i) {
			f32 a      = HALF_PI * i;
			f32 pulse  = 3.0f * std::sinf(DEG2RAD * ((frame * 24) % 360));
			f32 radius = rx + pulse;
			f32 sx = std::sinf(a), cy = std::cosf(a);
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			tex   = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[128 - StatusTextureOffset], 0);
			u16 w = GXGetTexObjWidth(&tex), h = GXGetTexObjHeight(&tex);
			DrawGsTexturedQuad(&gGame.mGs, &tex, tint, tint, tint, tint, radius * sx + x, radius * cy + y, 0, 1.4f * (1.1f * w), 1.4f * h,
			                   0, 0, a);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		}
	}
	SetGsLineWidth(gs->mCurRenderMode, 12, 5);
}

/**
 * @note Address: 0x80021474
 * @note Size: 0x8C
 */
void CScrnRoboeyes::Update(u32 frames)
{
	CScrnWipe::Update(frames);
	if (mActive) {
		mAngle += 0.02617994f * frames;
		mPhase += 0.10471976f * frames;
	}
}

/**
 * @note Address: 0x80021500
 * @note Size: 0x70
 */
void CScrnRoboeyes::Reset()
{
	mActive = FALSE;
	mX = mY = 0.0f;
	mWidth = mHeight = 0.0f;
	mTargetWidth = mTargetHeight = 0.0f;
	mSpeedWidth = mSpeedHeight = 0.0f;
	mColor.r                   = 0.0f;
	mColor.g                   = 0.0f;
	mColor.b                   = 0.0f;
	mColor.a                   = 0.0f;
	mTarget.r                  = 0.0f;
	mTarget.g                  = 0.0f;
	mTarget.b                  = 0.0f;
	mTarget.a                  = 0.0f;
	mSpeed.r                   = 0.0f;
	mSpeed.g                   = 0.0f;
	mSpeed.b                   = 0.0f;
	mSpeed.a                   = 0.0f;
	mRotation = mSpeedRotation = 0.0f;
	mAngle                     = 0.0f;
	mPhase                     = 0.0f;
}

// steps a value towards its target, it stops there
static inline void StepChannel(f32& current, f32& target, f32& speed, u32 frames)
{
	if (current != target) {
		current += speed * frames;
		if (speed > 0.0f && current >= target) {
			current = target;
			speed   = 0.0f;
		}
		if (speed < 0.0f && current <= target) {
			current = target;
			speed   = 0.0f;
		}
	}
}

static inline void DrawWipeQuad(CGs* gs, const WipeQuad& shape, u32 color)
{
	const Vec* quad = shape.vertices;
	DrawGsVertexQuad(gs, quad[0].x, quad[0].y, quad[0].z, quad[1].x, quad[1].y, quad[1].z, quad[2].x, quad[2].y, quad[2].z, quad[3].x,
	                 quad[3].y, quad[3].z, color, color, color, color);
}

static inline void BeginWipe(CGs* gs, s32 mode)
{
	if (mode) {
		static const Vec p0 = { 0, 0, -1 }, p1 = { 640, 0, -1 }, p2 = { 640, 448, -1 }, p3 = { 0, 448, -1 };
		WipeQuad quad = { { p0, p1, p2, p3 } };
		SetGsDepthMode(gs->mCurRenderMode, 1, 7, 1);
		DrawWipeQuad(gs, quad, 0);
	}
	SetGsAlphaCompare(gs->mCurRenderMode, 7, 0, 0, 7, 0xFF);
	SetGsChannelEnabled(gs, 0, 0);
	SetGsBlendMode(gs->mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gs->mCurRenderMode, 1, 7, 1);
}

static inline void FinishWipe(const CScrnWipe* wipe, CGs* gs, s32 mode)
{
	static const Vec p0 = { 0, 0, 0 }, p1 = { 640, 0, 0 }, p2 = { 640, 448, 0 }, p3 = { 0, 448, 0 };
	WipeQuad quad = { { p0, p1, p2, p3 } };
	u32 color     = ((u32)wipe->mColor.r << 24) | ((u32)wipe->mColor.g << 16) | ((u32)wipe->mColor.b << 8) | (u32)wipe->mColor.a;
	if (mode)
		SetGsDepthMode(gs->mCurRenderMode, 1, 1, 1);
	else
		SetGsDepthMode(gs->mCurRenderMode, 1, 1, 0);
	DrawWipeQuad(gs, quad, color);
	SetGsAlphaCompare(gs->mCurRenderMode, 4, 0, 0, 3, 0xFF);
	SetGsDepthMode(gs->mCurRenderMode, 1, 3, 0);
}

static inline void BeginWipeMask(CGs* gs, f32 opacity)
{
	static const Vec p0 = { 0, 0, 0 }, p1 = { 640, 0, 0 }, p2 = { 640, 448, 0 }, p3 = { 0, 448, 0 };
	WipeQuad quad = { { p0, p1, p2, p3 } };
	SetGsDestinationAlpha(gs->mCurRenderMode, 0, 0);
	SetGsColorUpdate(gs->mCurRenderMode, 0);
	SetGsAlphaUpdate(gs->mCurRenderMode, 1);
	SetGsBlendMode(gs->mCurRenderMode, 0, 4, 5, 15);
	SetGsAlphaCompare(gs->mCurRenderMode, 7, 0, 0, 7, 255);
	SetGsDepthMode(gs->mCurRenderMode, 0, 7, 0);
	SetGsTevMode(gs->mCurTevStage, 4);
	f32 alpha = 255.0f - opacity;
	DrawWipeQuad(gs, quad, 0xFF000000 | (u32)alpha);
	SetGsBlendMode(gs->mCurRenderMode, 1, 4, 5, 15);
	SetGsAlphaCompare(gs->mCurRenderMode, 4, (s32)alpha, 0, 3, 255);
}

static inline void EndWipeMask(CGs* gs)
{
	SetGsColorUpdate(gs->mCurRenderMode, 1);
	SetGsAlphaUpdate(gs->mCurRenderMode, 0);
	SetGsBlendMode(gs->mCurRenderMode, 1, 7, 6, 15);
	SetGsAlphaCompare(gs->mCurRenderMode, 4, 0, 0, 3, 255);
	SetGsDepthMode(gs->mCurRenderMode, 1, 7, 1);
}

/**
 * @note Address: 0x80021570
 * @note Size: 0x528
 */
// NONMATCHING: retail adds the row's half cell after a separate multiply (fmuls, fadds) where this build
// fuses it (fmadds), one instruction short. The cell size is a variable as on the Wii, where it comes from
// the pointer screen's width.
void CScrnWipeCustom::Draw(CGs* gs, s32 mode)
{
	if (!mActive)
		return;
	BeginWipe(gs, mode);
	f32 cell = 640.0f / 16.0f;
	for (s32 row = 0; row < 448.0f / cell; ++row) {
		f32 y = row * cell + cell * 0.5f;
		for (s32 col = 0; col < 640.0f / cell; ++col) {
			f32 x = col * cell + cell * 0.5f;
			f32 extra;
			if (col < 16.0f * 0.5f)
				extra = 0.2f * col;
			else
				extra = 0.2f * (640.0f / cell - col);
			DrawGsCircle(gs, 16, 0, 0, x, y, 0.0f, mWidth + mWidth * extra, mHeight + mHeight * extra, 0.0f, 0.0f, 0.0f);
		}
	}
	FinishWipe(this, gs, mode);
}

/**
 * @note Address: 0x80021A98
 * @note Size: 0x644
 */
void CScrnWipeTex::Draw(CGs* gs, s32 mode)
{
	if (!mActive)
		return;
	BeginWipe(gs, mode);
	BeginWipeMask(gs, mColor.a);
	SetGsTevMode(gs->mCurTevStage, 0);
	SetGsDepthCompareBeforeTexture(gs->mCurRenderMode, 0);
	DrawGsTexturedQuad(gs, &mTex, -1, -1, -1, -1, mX, mY, 0.0f, mWidth, mHeight, 0.0f, 0.0f, mRotation);
	SetGsDepthCompareBeforeTexture(gs->mCurRenderMode, 1);
	EndWipeMask(gs);
	SetGsTevMode(gs->mCurTevStage, 4);
	FinishWipe(this, gs, mode);
}

/**
 * @note Address: 0x800220DC
 * @note Size: 0x7C0
 */
// NONMATCHING: stack offsets of BeginWipe/Mtx locals (+0x30) and .rodata offsets; all instructions and registers match
void CScrnWipeTile::Draw(CGs* gs, s32 mode)
{
	if (!mActive)
		return;
	BeginWipe(gs, mode);
	BeginWipeMask(gs, mColor.a);
	f32 edgeWidth = 24.0f;
	if (edgeWidth > mWidth)
		edgeWidth = mWidth;
	f32 edgeHeight = 24.0f;
	if (edgeHeight > mHeight)
		edgeHeight = mHeight;
	DrawGsQuad(gs, 255, 255, 255, 255, mX, mY, 0.0f, mWidth, mHeight, 0.0f, 0.0f, mRotation);

	Vec vertices[18] = {};
	const f32 dx = edgeWidth / 2.0f, dy = edgeHeight / 2.0f;
	vertices[0].x  = -mWidth / 2.0f;
	vertices[0].y  = -mHeight / 2.0f;
	vertices[1].x  = -mWidth / 2.0f - dx;
	vertices[1].y  = -mHeight / 2.0f - dy;
	vertices[2].x  = mWidth / 2.0f + dx;
	vertices[2].y  = -mHeight / 2.0f - dy;
	vertices[3].x  = -mWidth / 2.0f;
	vertices[3].y  = -mHeight / 2.0f;
	vertices[4].x  = mWidth / 2.0f;
	vertices[4].y  = -mHeight / 2.0f;
	vertices[5].x  = mWidth / 2.0f + dx;
	vertices[5].y  = -mHeight / 2.0f - dy;
	vertices[6].x  = mWidth / 2.0f + dx;
	vertices[6].y  = mHeight / 2.0f + dy;
	vertices[7].x  = mWidth / 2.0f;
	vertices[7].y  = -mHeight / 2.0f;
	vertices[8].x  = mWidth / 2.0f;
	vertices[8].y  = mHeight / 2.0f;
	vertices[9].x  = mWidth / 2.0f + dx;
	vertices[9].y  = mHeight / 2.0f + dy;
	vertices[10].x = -mWidth / 2.0f - dx;
	vertices[10].y = mHeight / 2.0f + dy;
	vertices[11].x = mWidth / 2.0f;
	vertices[11].y = mHeight / 2.0f;
	vertices[12].x = -mWidth / 2.0f;
	vertices[12].y = mHeight / 2.0f;
	vertices[13].x = -mWidth / 2.0f - dx;
	vertices[13].y = mHeight / 2.0f + dy;
	vertices[14].x = -mWidth / 2.0f - dx;
	vertices[14].y = -mHeight / 2.0f - dy;
	vertices[15].x = -mWidth / 2.0f;
	vertices[15].y = mHeight / 2.0f;
	vertices[16].x = -mWidth / 2.0f;
	vertices[16].y = -mHeight / 2.0f;
	vertices[17].x = -mWidth / 2.0f - dx;
	vertices[17].y = -mHeight / 2.0f - dy;
	u32 colors[18] = { 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0 };
	Mtx translation, rotation, transform;
	PSMTXTrans(translation, mX, mY, 0.0f);
	PSMTXRotRad(rotation, 'z', -mRotation);
	PSMTXConcat(translation, rotation, transform);
	PSMTXMultVecArray(transform, vertices, vertices, 18);
	DrawGsColoredTrianglePrimitive(gs, GX_TRIANGLESTRIP, gs->mCamera->mViewMtx, vertices, colors, 18);
	EndWipeMask(gs);
	FinishWipe(this, gs, mode);
}

/**
 * @note Address: 0x8002289C
 * @note Size: 0x794
 */
void CScrnWipe::Draw(CGs* gs, s32 mode)
{
	if (!mActive)
		return;
	BeginWipe(gs, mode);
	BeginWipeMask(gs, mColor.a);
	f32 edgeWidth = 32.0f;
	if (edgeWidth > mWidth)
		edgeWidth = mWidth;
	f32 edgeHeight = 32.0f * (mHeight / mWidth);
	if (edgeHeight > mHeight)
		edgeHeight = mHeight;

	DrawGsCircle(gs, 64, 255, 255, mX, mY, 0.0f, mWidth, mHeight, 0.0f, 0.0f, 0.0f);
	PrimVertex vertices[130];
	for (s32 i = 0; i < 65; i++) {
		f32 angle     = (TWO_PI / 64.0f) * i;
		PrimVertex* v = &vertices[2 * i];
		v[0].pos.x    = mX - mWidth / 2.0f * std::sinf(angle);
		v[0].pos.y    = mY + mHeight / 2.0f * std::cosf(angle);
		v[0].pos.z    = 0.0f;
		v[0].color    = 255;
		v[1].pos.x    = mX - (mWidth + edgeWidth) / 2.0f * std::sinf(angle);
		v[1].pos.y    = mY + (mHeight + edgeHeight) / 2.0f * std::cosf(angle);
		v[1].pos.z    = 0.0f;
		v[1].color    = 0;
	}
	DrawGsTriangleVertices(gs, GX_TRIANGLESTRIP, gs->mCamera->mViewMtx, vertices, 130);
	EndWipeMask(gs);
	FinishWipe(this, gs, mode);
}

static inline bool SizeStopped(CScrnWipe* wipe)
{
	return 0.0f == wipe->mSpeedWidth && 0.0f == wipe->mSpeedHeight;
}

/**
 * @note Address: 0x80023030
 * @note Size: 0x408
 */
// a wipe's update: it turns, sizes its hole and fades its color, and goes off once the size
// stops when it was asked to
void CScrnWipe::Update(u32 frames)
{
	if (!mActive) {
		return;
	}
	if (0.0f != mSpeedRotation) {
		mRotation = fmod(mSpeedRotation * frames + mRotation, TWO_PI);
	}
	StepChannel(mWidth, mTargetWidth, mSpeedWidth, frames);
	StepChannel(mHeight, mTargetHeight, mSpeedHeight, frames);
	StepChannel(mColor.r, mTarget.r, mSpeed.r, frames);
	StepChannel(mColor.g, mTarget.g, mSpeed.g, frames);
	StepChannel(mColor.b, mTarget.b, mSpeed.b, frames);
	StepChannel(mColor.a, mTarget.a, mSpeed.a, frames);
	if (mHideWhenDone && SizeStopped(this)) {
		mActive = FALSE;
	}
}

/**
 * @note Address: 0x80023438
 * @note Size: 0x2B4
 */
// starts a wipe, the color and the hole size both go from the first values to the second
extern "C" void StartScreenWipeColorTransition(CScrnWipe* wipe, u8 r0, u8 g0, u8 b0, u8 a0, u8 r1, u8 g1, u8 b1, f32 x, f32 y, f32 width0,
                                               f32 height0, f32 width1, f32 height1, f32 time, u8 a1, BOOL hideWhenDone)
{
	wipe->mActive       = TRUE;
	wipe->mHideWhenDone = hideWhenDone;
	wipe->mX            = x;
	wipe->mY            = y;
	wipe->mWidth        = width0;
	wipe->mHeight       = height0;
	wipe->mTargetWidth  = width1;
	wipe->mTargetHeight = height1;
	if (0.0f == time) {
		wipe->mWidth  = width1;
		wipe->mHeight = height1;
	} else {
		wipe->mSpeedWidth  = (width1 - width0) / (FRAME_RATE * time);
		wipe->mSpeedHeight = (height1 - height0) / (FRAME_RATE * time);
	}
	wipe->mColor.r  = r0;
	wipe->mColor.g  = g0;
	wipe->mColor.b  = b0;
	wipe->mColor.a  = a0;
	wipe->mTarget.r = r1;
	wipe->mTarget.g = g1;
	wipe->mTarget.b = b1;
	wipe->mTarget.a = a1;
	if (0.0f == time) {
		wipe->mColor.r = r1;
		wipe->mColor.g = g1;
		wipe->mColor.b = b1;
		wipe->mColor.a = a1;
		wipe->mSpeed.r = 0.0f;
		wipe->mSpeed.g = 0.0f;
		wipe->mSpeed.b = 0.0f;
		wipe->mSpeed.a = 0.0f;
	} else {
		wipe->mSpeed.Set(((f32)r1 - (f32)r0) / (FRAME_RATE * time), ((f32)g1 - (f32)g0) / (FRAME_RATE * time),
		                 ((f32)b1 - (f32)b0) / (FRAME_RATE * time), ((f32)a1 - (f32)a0) / (FRAME_RATE * time));
	}
	if (wipe->mHideWhenDone && SizeStopped(wipe))
		wipe->mActive = FALSE;
}

/**
 * @note Address: 0x800236EC
 * @note Size: 0x1B8
 */
// the same with one color
extern "C" void StartScreenWipe(CScrnWipe* wipe, u8 r, u8 g, u8 b, u8 a, BOOL hideWhenDone, f32 x, f32 y, f32 width0, f32 height0,
                                f32 width1, f32 height1, f32 time)
{
	wipe->mActive       = TRUE;
	wipe->mHideWhenDone = hideWhenDone;
	wipe->mX            = x;
	wipe->mY            = y;
	wipe->mWidth        = width0;
	wipe->mHeight       = height0;
	wipe->mTargetWidth  = width1;
	wipe->mTargetHeight = height1;
	if (0.0f == time) {
		wipe->mWidth      = width1;
		wipe->mHeight     = height1;
		wipe->mSpeedWidth = wipe->mSpeedHeight = 0.0f;
	} else {
		wipe->mSpeedWidth  = (width1 - width0) / (FRAME_RATE * time);
		wipe->mSpeedHeight = (height1 - height0) / (FRAME_RATE * time);
	}
	wipe->mColor.r  = r;
	wipe->mColor.g  = g;
	wipe->mColor.b  = b;
	wipe->mColor.a  = a;
	wipe->mTarget.r = r;
	wipe->mTarget.g = g;
	wipe->mTarget.b = b;
	wipe->mTarget.a = a;
	wipe->mSpeed.r  = r;
	wipe->mSpeed.g  = g;
	wipe->mSpeed.b  = b;
	wipe->mSpeed.a  = a;
	if (wipe->mHideWhenDone && SizeStopped(wipe))
		wipe->mActive = FALSE;
}

/**
 * @note Address: 0x800238A4
 * @note Size: 0x68
 */
void CScrnWipe::Reset()
{
	mActive = FALSE;
	mX = mY = 0.0f;
	mWidth = mHeight = 0.0f;
	mTargetWidth = mTargetHeight = 0.0f;
	mSpeedWidth = mSpeedHeight = 0.0f;
	mColor.r                   = 0.0f;
	mColor.g                   = 0.0f;
	mColor.b                   = 0.0f;
	mColor.a                   = 0.0f;
	mTarget.r                  = 0.0f;
	mTarget.g                  = 0.0f;
	mTarget.b                  = 0.0f;
	mTarget.a                  = 0.0f;
	mSpeed.r                   = 0.0f;
	mSpeed.g                   = 0.0f;
	mSpeed.b                   = 0.0f;
	mSpeed.a                   = 0.0f;
	mRotation = mSpeedRotation = 0.0f;
}

static inline bool HasColor(CScrnColor* color)
{
	return 0.0f != color->mColor.r || 0.0f != color->mColor.g || 0.0f != color->mColor.b || 0.0f != color->mColor.a;
}

/**
 * @note Address: 0x8002390C
 * @note Size: 0x22C
 */
// NONMATCHING: the code is retail's, the corners sit 0x210 further down in .rodata than retail has them
extern "C" void DrawScreenColorOverlay(CScrnColor* color, CGs* gs)
{
	if (HasColor(color)) {
		static const Vec p0 = { 0, 0, 0 }, p1 = { 640, 0, 0 }, p2 = { 640, 448, 0 }, p3 = { 0, 448, 0 };
		WipeQuad quad = { { p0, p1, p2, p3 } };
		u32 packed    = ((u32)color->mColor.r << 24) | ((u32)color->mColor.g << 16) | ((u32)color->mColor.b << 8) | (u32)color->mColor.a;
		SetGsChannelEnabled(gs, 0, 0);
		SetGsBlendMode(gs->mCurRenderMode, 1, 4, 5, 15);
		SetGsDepthMode(gs->mCurRenderMode, 1, 1, 0);
		DrawWipeQuad(gs, quad, packed);
	}
}

/**
 * @note Address: 0x80023B38
 * @note Size: 0x23C
 */
extern "C" void UpdateScreenFade(CScrnColor* color, u32 frames)
{
	StepChannel(color->mColor.r, color->mTarget.r, color->mSpeed.r, frames);
	StepChannel(color->mColor.g, color->mTarget.g, color->mSpeed.g, frames);
	StepChannel(color->mColor.b, color->mTarget.b, color->mSpeed.b, frames);
	StepChannel(color->mColor.a, color->mTarget.a, color->mSpeed.a, frames);
}

/**
 * @note Address: 0x80023D74
 * @note Size: 0x16C
 */
extern "C" void SetScreenFadeTargetColor(CScrnColor* color, u8 r, u8 g, u8 b, u8 a, f32 time)
{
	color->mTarget.r = r;
	color->mTarget.g = g;
	color->mTarget.b = b;
	color->mTarget.a = a;
	if (0.0f == time) {
		color->mColor.r = r;
		color->mColor.g = g;
		color->mColor.b = b;
		color->mColor.a = a;
		color->mSpeed.r = 0.0f;
		color->mSpeed.g = 0.0f;
		color->mSpeed.b = 0.0f;
		color->mSpeed.a = 0.0f;
	} else {
		color->mSpeed.Set(((f32)r - color->mColor.r) / (FRAME_RATE * time), ((f32)g - color->mColor.g) / (FRAME_RATE * time),
		                  ((f32)b - color->mColor.b) / (FRAME_RATE * time), ((f32)a - color->mColor.a) / (FRAME_RATE * time));
	}
}

/**
 * @note Address: 0x80023EE0
 * @note Size: 0x20C
 */
extern "C" void StartScreenFade(CScrnColor* color, u8 r0, u8 g0, u8 b0, u8 a0, u8 r1, u8 g1, u8 b1, u8 a1, f32 time)
{
	color->mColor.r  = r0;
	color->mColor.g  = g0;
	color->mColor.b  = b0;
	color->mColor.a  = a0;
	color->mTarget.r = r1;
	color->mTarget.g = g1;
	color->mTarget.b = b1;
	color->mTarget.a = a1;
	if (0.0f == time) {
		color->mColor.r = r1;
		color->mColor.g = g1;
		color->mColor.b = b1;
		color->mColor.a = a1;
		color->mSpeed.r = 0.0f;
		color->mSpeed.g = 0.0f;
		color->mSpeed.b = 0.0f;
		color->mSpeed.a = 0.0f;
	} else {
		color->mSpeed.Set(((f32)r1 - (f32)r0) / (FRAME_RATE * time), ((f32)g1 - (f32)g0) / (FRAME_RATE * time),
		                  ((f32)b1 - (f32)b0) / (FRAME_RATE * time), ((f32)a1 - (f32)a0) / (FRAME_RATE * time));
	}
}

/**
 * @note Address: 0x800240EC
 * @note Size: 0x1BC
 */
extern "C" void UpdateScreenShake(CScrnQuake* quake, u32 frames)
{
	StepChannel(quake->mShake[0].x, quake->mShake[1].x, quake->mShake[2].x, frames);
	StepChannel(quake->mShake[0].y, quake->mShake[1].y, quake->mShake[2].y, frames);
	StepChannel(quake->mShake[0].z, quake->mShake[1].z, quake->mShake[2].z, frames);
	quake->mSign = -quake->mSign;
}

static inline void CopyVector(CVec& destination, const Vec& source)
{
	destination.x = source.x;
	destination.y = source.y;
	destination.z = source.z;
}

/**
 * @note Address: 0x800242A8
 * @note Size: 0x184
 */
extern "C" void StartScreenShake(CScrnQuake* quake, f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1, f32 time)
{
	quake->mShake[0].Set(x0, y0, z0);
	quake->mShake[0].x = Abs(quake->mShake[0].x);
	quake->mShake[0].y = Abs(quake->mShake[0].y);
	quake->mShake[0].z = Abs(quake->mShake[0].z);
	quake->mShake[1].Set(x1, y1, z1);
	quake->mShake[1].x = Abs(quake->mShake[1].x);
	quake->mShake[1].y = Abs(quake->mShake[1].y);
	quake->mShake[1].z = Abs(quake->mShake[1].z);
	if (time == 0.0f) {
		CopyVector(quake->mShake[0], quake->mShake[1]);
		quake->mShake[2].Set(0.0f, 0.0f, 0.0f);
	} else {
		CopyVector(quake->mShake[2], (quake->mShake[1] - quake->mShake[0]) / (FRAME_RATE * time));
	}
	quake->mSign = 1;
}

/**
 * @note Address: 0x8002442C
 * @note Size: 0x74
 */
CVec CVec::operator/(f32 divisor) const
{
	CVec result = *this;
	result.x /= divisor;
	result.y /= divisor;
	result.z /= divisor;
	return result;
}

/**
 * @note Address: 0x800244A0
 * @note Size: 0x84
 */
CVec CVec::operator-(Vec other) const
{
	CVec result = *this;
	PSVECSubtract(&result, &other, &result);
	return result;
}

/**
 * @note Address: 0x80024524
 * @note Size: 0x34
 */
extern "C" void ResetScreenShake(CScrnQuake* quake)
{
	quake->mShake[0].Set(0.0f, 0.0f, 0.0f);
	quake->mShake[1].Set(0.0f, 0.0f, 0.0f);
	quake->mShake[2].Set(0.0f, 0.0f, 0.0f);
	quake->mSign = 1;
}

/**
 * @note Address: 0x800245F0
 * @note Size: 0x10
 */
void CPtclOne::Init()
{
	mActive = FALSE;
	mTex    = NULL;
}

/**
 * @note Address: 0x80024600
 * @note Size: 0x1D8
 */
// NONMATCHING: regalloc (rot copy uses r4/r6/r0 instead of r5/r4/r0, scheduling mTex load earlier)
void CPtclDust3::Draw(CGs* gs)
{
	if (!mActive) {
		return;
	}
	f32 width = mSize.x * (f32)sin(0.8f * (PI * mAlpha));
	width *= 2.0f;
	f32 height = mSize.y * (f32)sin(0.8f * (PI * mAlpha));
	height *= 2.0f;
	u32 edgeColor = mColor & 0xFFFFFF00;
	u32 color     = (mColor & 0xFFFFFF00) | ((u32)(s32)((f32)(mColor & 0xFF) * mAlpha) & 0xFF);
	if (mTex != NULL) {
		SetGsTevMode(gs->mCurTevStage, 0);
		Vec rot = { 0.0f, 0.0f, 0.0f };
		rot.z   = mAngle;
		DrawGsTexturedQuad(gs, mTex, color, color, color, color, mPos.x, mPos.y, mPos.z, width, height, rot.x, rot.y, rot.z);
		SetGsTevMode(gs->mCurTevStage, 4);
	} else {
		Vec rot = { 0.0f, 0.0f, 0.0f };
		rot.z   = mAngle;
		DrawGsCircle(gs, 8, color, edgeColor, mPos.x, mPos.y, mPos.z, width, height, rot.x, rot.y, rot.z);
	}
}

// moves a position by an offset passed by value
static inline void Move(CVec& pos, Vec offset)
{
	PSVECAdd(&pos, &offset, &pos);
}

/**
 * @note Address: 0x800247D8
 * @note Size: 0x15C
 */
void CPtclDust3::Update(u32 frames)
{
	if (!mActive) {
		return;
	}
	if (mAlphaStep < 0.0f) {
		if ((mAlpha -= mAlphaStep * (f32)frames) >= 1.0f) {
			mAlphaStep = -mAlphaStep;
		}
	}
	if (mAlphaStep > 0.0f) {
		if ((mAlpha -= mAlphaStep * (f32)frames) <= 0.0f) {
			mActive = FALSE;
			mAlpha  = 0.0f;
			return;
		}
	}
	Move(mPos, (mVel * 1.0f) * (f32)frames);
	mAngle += mAngleSpeed;
}

/**
 * @note Address: 0x80024934
 * @note Size: 0x1E8
 */
// NONMATCHING: regalloc (rot copy uses r4/r6/r0 instead of r5/r4/r0, scheduling mTex load earlier)
void CPtclDust2::Draw(CGs* gs)
{
	if (!mActive) {
		return;
	}
	u32 color  = (mColor & 0xFFFFFF00) | ((u32)(s32)((f32)(mColor & 0xFF) * mAlpha) & 0xFF);
	f32 radius = 1.5f * (mSize.x > mSize.y ? mSize.x : mSize.y);
	if (mTex != NULL) {
		SetGsTevMode(gs->mCurTevStage, 0);
		Vec rot = { 0.0f, 0.0f, 0.0f };
		rot.z   = mAngle;
		DrawGsTexturedQuad(gs, mTex, color, color, color, color, mPos.x, mPos.y, mPos.z, radius, radius, rot.x, rot.y, rot.z);
		SetGsTevMode(gs->mCurTevStage, 4);
	} else {
		// two triangles make a star
		Vec first = { 0.0f, 0.0f, 0.0f };
		first.z   = mAngle;
		DrawGsCircle(gs, 3, color, color, mPos.x, mPos.y, mPos.z, radius, radius, first.x, first.y, first.z);
		Vec second = { 0.0f, 0.0f, 0.0f };
		second.z   = THIRD_PI + mAngle;
		DrawGsCircle(gs, 3, color, color, mPos.x, mPos.y, mPos.z, radius, radius, second.x, second.y, second.z);
	}
}

/**
 * @note Address: 0x80024B1C
 * @note Size: 0x184
 */
void CPtclDust2::Update(u32 frames)
{
	if (!mActive) {
		return;
	}
	if (mAlphaStep < 0.0f) {
		if ((mAlpha -= mAlphaStep * (f32)frames) >= 1.0f) {
			mAlphaStep = -mAlphaStep;
		}
	}
	if (mAlphaStep > 0.0f) {
		if ((mAlpha -= mAlphaStep * (f32)frames) <= 0.0f) {
			mActive = FALSE;
			mAlpha  = 0.0f;
			return;
		}
	}
	mVel.y -= 0.05f * (f32)frames;
	Move(mPos, (mVel * 1.0f) * (f32)frames);
	mAngle += mAngleSpeed;
}

/**
 * @note Address: 0x80024CA0
 * @note Size: 0xE0
 */
extern "C" void DrawParticlePool(CPtcl* ptcl)
{
	CGs* gs = &gGame.mGs;
	SetGsChannelEnabled(gs, 0, 0);
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
	gGame.mGs.mBillboardEnabled = 1;
	for (s32 i = 0; i < ptcl->mCount; ++i) {
		CPtclOne* one = ptcl->Get(i);
		if (one->mActive != FALSE) {
			one->Draw(&gGame.mGs);
		}
	}
	gGame.mGs.mBillboardEnabled = 0;
}

/**
 * @note Address: 0x80024D80
 * @note Size: 0x4
 */
void CPtclOne::Draw(CGs* gs)
{
}

/**
 * @note Address: 0x80024D84
 * @note Size: 0x84
 */
extern "C" void UpdateParticlePool(CPtcl* ptcl, u32 frames)
{
	for (s32 i = 0; i < ptcl->mCount; ++i) {
		CPtclOne* one = ptcl->Get(i);
		if (one->mActive != FALSE) {
			one->Update(frames);
		}
	}
}

/**
 * @note Address: 0x80024E08
 * @note Size: 0x4
 */
void CPtclOne::Update(u32 frames)
{
}

/**
 * @note Address: 0x80024E0C
 * @note Size: 0x44
 */
extern "C" void ClearParticlePool(CPtcl* ptcl)
{
	for (s32 i = 0; i < ptcl->mCount; ++i) {
		CPtclOne* one = ptcl->Get(i);
		if (one->mActive != FALSE) {
			one->mActive = FALSE;
			one->mTex    = NULL;
		}
	}
}

/**
 * @note Address: 0x80024E50
 * @note Size: 0x15C
 */
extern "C" CPtclOne* SpawnPoolParticle(CPtcl* ptcl, const Vec* pos, const Vec* size, const Vec* vel, u32 color)
{
	for (s32 i = 0; i < ptcl->mCount; ++i) {
		CPtclOne* one = ptcl->Get(i);
		if (one->mActive == FALSE) {
			one->mActive     = TRUE;
			one->mPos        = CVec(*pos);
			one->mVel        = CVec(*vel);
			one->mSize       = CVec(*size);
			one->mAngle      = (f32)fmod((f32)rand(), PI);
			one->mAngleSpeed = 0.0f;
			one->mColor      = color;
			one->mAlpha      = 1.0f;
			one->mAlphaStep  = 0.02f;
			one->mTex        = NULL;
			return one;
		}
	}
	return NULL;
}

/**
 * @note Address: 0x80024FAC
 * @note Size: 0x84
 */
extern "C" void InitParticlePool(CPtcl* ptcl, void* ptcls, s32 stride, s32 count)
{
	ptcl->mPtcls   = (u8*)ptcls;
	ptcl->mStride  = stride;
	ptcl->mCount   = count;
	ptcl->mCounter = 0;
	for (s32 i = 0; i < count; ++i) {
		ptcl->Get(i)->Init();
	}
}

/**
 * @note Address: 0x80025150
 * @note Size: 0xD8
 */
// Cursor's SetScaleY
void Cursor::SetScaleY(f32 y, s32 force)
{
	y                    = -y;
	mMoveScale.y         = y;
	mMoveScale.mTarget.y = y;
	if (mJObj != NULL) {
		CJObj::SetScaleY(y, force);
	}
}

/**
 * @note Address: 0x80025228
 * @note Size: 0x190
 */
// Cursor's SetScale(scale)
void Cursor::SetScale(f32 scale, s32 force)
{
	f32 x = scale;
	f32 y = -scale;
	f32 z = scale;

	mMoveScale.x         = x;
	mMoveScale.y         = y;
	mMoveScale.z         = z;
	mMoveScale.mTarget.x = x;
	mMoveScale.mTarget.y = y;
	mMoveScale.mTarget.z = z;
	if (mJObj) {
		if (force == 0) {
			Vec v = { x, y, z };
			if (!(mScale != v)) {
				return;
			}
		}
		mScale.x = x;
		mScale.y = y;
		mScale.z = z;
		HSD_JObjSetScale(mJObj, &mScale);
	}
}

/**
 * @note Address: 0x800253B8
 * @note Size: 0x190
 */
// Cursor's SetScale(x, y, z)
void Cursor::SetScale(f32 x, f32 y, f32 z, s32 force)
{
	y = -y;

	mMoveScale.x         = x;
	mMoveScale.y         = y;
	mMoveScale.z         = z;
	mMoveScale.mTarget.x = x;
	mMoveScale.mTarget.y = y;
	mMoveScale.mTarget.z = z;
	if (mJObj) {
		if (force == 0) {
			Vec v = { x, y, z };
			if (!(mScale != v)) {
				return;
			}
		}
		mScale.x = x;
		mScale.y = y;
		mScale.z = z;
		HSD_JObjSetScale(mJObj, &mScale);
	}
}

/**
 * @note Address: 0x80025548
 * @note Size: 0x19C
 */
// Cursor's SetScale(v)
void Cursor::SetScale(const Vec& scale, s32 force)
{
	f32 x = scale.x;
	f32 y = -scale.y;
	f32 z = scale.z;

	mMoveScale.x         = x;
	mMoveScale.y         = y;
	mMoveScale.z         = z;
	mMoveScale.mTarget.x = x;
	mMoveScale.mTarget.y = y;
	mMoveScale.mTarget.z = z;
	if (mJObj) {
		if (force == 0) {
			Vec v = { x, y, z };
			if (!(mScale != v)) {
				return;
			}
		}
		mScale.x = x;
		mScale.y = y;
		mScale.z = z;
		HSD_JObjSetScale(mJObj, &mScale);
	}
}

// animation ids from 1000 up are in the object's own anim archive once that is read, the
// rest are in its model archive. the last three digits pick the animation
static inline s32 AnimationGroup(XObj* object, s32 id)
{
	bool extra = id >= 1000;
	if (extra) {
		extra = object->mAnim.mArchive != NULL;
	}
	return extra ? id / 1000 : 0;
}

/**
 * @note Address: 0x800256E4
 * @note Size: 0x38
 */
// Cursor's SetAnim, keeps the classical scale
void Cursor::SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	XObj::SetAnim(id, force, jointAnim, matAnim, shapeAnim, frame, rate, blend, blendRate);
	HSD_JObjSetFlags(mJObj, CLASSICAL_SCALE);
}

/**
 * @note Address: 0x8002571C
 * @note Size: 0x4
 */
// XObj's ResetForNewGame
void XObj::ResetForNewGame()
{
}

/**
 * @note Address: 0x80025720
 * @note Size: 0x24
 */
// XObj's LoadAnimationArchiveAsync
BOOL XObj::LoadAnimationArchiveAsync(const char* path)
{
	return LoadJointAnimationAsync(&mAnim, path);
}

/**
 * @note Address: 0x80025744
 * @note Size: 0x4
 */
// XObj's DrawDebugStatus
void XObj::DrawDebugStatus()
{
}

/**
 * @note Address: 0x80025748
 * @note Size: 0x1C
 */
// XObj's GetCameraTarget
Vec XObj::GetCameraTarget()
{
	return mMoveTranslate;
}

/**
 * @note Address: 0x80025764
 * @note Size: 0xF4
 */
// XObj's GetJointAnimation
HSD_AnimJoint* XObj::GetJointAnimation(s32 id)
{
	if (AnimationGroup(this, id)) {
		HSD_AnimJoint** animations = mAnim.mAnimJoint;
		return animations != NULL ? animations[(id % 1000) % 1000] : NULL;
	}
	HSD_AnimJoint** animations = mArchive.mAnimJoint;
	return animations != NULL ? animations[id % 1000] : NULL;
}

/**
 * @note Address: 0x80025858
 * @note Size: 0xF4
 */
// XObj's GetMaterialAnimation
HSD_MatAnimJoint* XObj::GetMaterialAnimation(s32 id)
{
	if (AnimationGroup(this, id)) {
		HSD_MatAnimJoint** animations = mAnim.mMatAnimJoint;
		return animations != NULL ? animations[(id % 1000) % 1000] : NULL;
	}
	HSD_MatAnimJoint** animations = mArchive.mMatAnimJoint;
	return animations != NULL ? animations[id % 1000] : NULL;
}

/**
 * @note Address: 0x8002594C
 * @note Size: 0xF4
 */
// XObj's GetShapeAnimation
HSD_ShapeAnimJoint* XObj::GetShapeAnimation(s32 id)
{
	if (AnimationGroup(this, id)) {
		HSD_ShapeAnimJoint** animations = mAnim.mShapeAnimJoint;
		return animations != NULL ? animations[(id % 1000) % 1000] : NULL;
	}
	HSD_ShapeAnimJoint** animations = mArchive.mShapeAnimJoint;
	return animations != NULL ? animations[id % 1000] : NULL;
}

/**
 * @note Address: 0x80025A40
 * @note Size: 0x8
 */
// XObj's OnPlayerContact
s32 XObj::OnPlayerContact(s32 unused, s32 value)
{
	return value;
}

/**
 * @note Address: 0x80025A48
 * @note Size: 0x50
 */
s32 XObj::HandleEventCommand(s32 count, s32* args)
{
	for (int i = 0; i < count; i++) { }
	return 0;
}

/**
 * @note Address: 0x80025A98
 * @note Size: 0x8
 */
// XObj's GetEventScript
extern "C" s32 GetEventScript__4XObjFv(XObj* object)
{
	return object->mEventScript;
}

/**
 * @note Address: 0x80025AA0
 * @note Size: 0x4
 */
void XObj::OnPushDisplacement(const Vec& displacement)
{
}

/**
 * @note Address: 0x80025AA4
 * @note Size: 0xD8
 */
// CXObj's SetTranslateX
void CXObj::SetTranslateX(f32 x, s32 force)
{
	mMoveTranslate.x         = x;
	mMoveTranslate.mTarget.x = x;
	if (mJObj != NULL) {
		CJObj::SetTranslateX(x, force);
	}
}

/**
 * @note Address: 0x80025B7C
 * @note Size: 0xD8
 */
// CXObj's SetTranslateY
void CXObj::SetTranslateY(f32 y, s32 force)
{
	mMoveTranslate.y         = y;
	mMoveTranslate.mTarget.y = y;
	if (mJObj != NULL) {
		CJObj::SetTranslateY(y, force);
	}
}

/**
 * @note Address: 0x80025C54
 * @note Size: 0xD8
 */
// CXObj's SetTranslateZ
void CXObj::SetTranslateZ(f32 z, s32 force)
{
	mMoveTranslate.z         = z;
	mMoveTranslate.mTarget.z = z;
	if (mJObj != NULL) {
		CJObj::SetTranslateZ(z, force);
	}
}

/**
 * @note Address: 0x80025D2C
 * @note Size: 0xD8
 */
// CXObj's SetScaleX
void CXObj::SetScaleX(f32 x, s32 force)
{
	mMoveScale.x         = x;
	mMoveScale.mTarget.x = x;
	if (mJObj != NULL) {
		CJObj::SetScaleX(x, force);
	}
}

/**
 * @note Address: 0x80025E04
 * @note Size: 0xD8
 */
// CXObj's SetScaleZ
void CXObj::SetScaleZ(f32 z, s32 force)
{
	mMoveScale.z         = z;
	mMoveScale.mTarget.z = z;
	if (mJObj != NULL) {
		CJObj::SetScaleZ(z, force);
	}
}

/**
 * @note Address: 0x80025EDC
 * @note Size: 0x1B0
 */
// CXObj's SetRotationX
void CXObj::SetRotationX(f32 x, s32 force)
{
	mMoveRotate.x         = x;
	mMoveRotate.mTarget.x = x;
	if (mJObj != NULL) {
		CJObj::SetRotationX(x, force);
	}
}

/**
 * @note Address: 0x8002608C
 * @note Size: 0x1B0
 */
// CXObj's SetRotationY
void CXObj::SetRotationY(f32 y, s32 force)
{
	mMoveRotate.y         = y;
	mMoveRotate.mTarget.y = y;
	if (mJObj != NULL) {
		CJObj::SetRotationY(y, force);
	}
}

/**
 * @note Address: 0x8002623C
 * @note Size: 0x1B0
 */
// CXObj's SetRotationZ
void CXObj::SetRotationZ(f32 z, s32 force)
{
	mMoveRotate.z         = z;
	mMoveRotate.mTarget.z = z;
	if (mJObj != NULL) {
		CJObj::SetRotationZ(z, force);
	}
}

/**
 * @note Address: 0x800263EC
 * @note Size: 0x190
 */
// CXObj's SetPos(x, y, z)
void CXObj::SetPos(f32 x, f32 y, f32 z, s32 force)
{
	mMoveTranslate.x         = x;
	mMoveTranslate.y         = y;
	mMoveTranslate.z         = z;
	mMoveTranslate.mTarget.x = x;
	mMoveTranslate.mTarget.y = y;
	mMoveTranslate.mTarget.z = z;
	if (mJObj) {
		if (force == 0) {
			Vec v = { x, y, z };
			if (!(mTranslate != v)) {
				return;
			}
		}
		mTranslate.x = x;
		mTranslate.y = y;
		mTranslate.z = z;
		HSD_JObjSetTranslate(mJObj, &mTranslate);
	}
}

/**
 * @note Address: 0x8002657C
 * @note Size: 0x17C
 */
// CXObj's SetPos(pos)
void CXObj::SetPos(const Vec& v, s32 force)
{
	mMoveTranslate.Set(v);
	mMoveTranslate.mTarget.Set(v);
	if (mJObj) {
		CJObj::SetTranslate(v, force);
	}
}

/**
 * @note Address: 0x800266F8
 * @note Size: 0x240
 */
// CXObj's SetRot(x, y, z)
void CXObj::SetRot(f32 x, f32 y, f32 z, s32 force)
{
	mMoveRotate.x         = x;
	mMoveRotate.y         = y;
	mMoveRotate.z         = z;
	mMoveRotate.mTarget.x = x;
	mMoveRotate.mTarget.y = y;
	mMoveRotate.mTarget.z = z;
	if (mJObj) {
		if (force == 0) {
			Vec v = { x, y, z };
			// tests the cached translation, as CJObj's does
			if (!(mTranslate != v)) {
				return;
			}
		}
		mRotate.x = x;
		mRotate.y = y;
		mRotate.z = z;
		UpdateRotation();
	}
}

/**
 * @note Address: 0x80026938
 * @note Size: 0x22C
 */
// CXObj's SetRot(rot)
void CXObj::SetRot(const Vec& v, s32 force)
{
	mMoveRotate.Set(v);
	mMoveRotate.mTarget.Set(v);
	if (mJObj) {
		CJObj::SetRotation(v, force);
	}
}

/**
 * @note Address: 0x80026B64
 * @note Size: 0x38
 */
// Cursor's DrawObject
void Cursor::DrawObject()
{
	if (mJObj != NULL) {
		HSD_JObjDispAll(mJObj, NULL, 7, 0);
	}
}

/**
 * @note Address: 0x80026B9C
 * @note Size: 0x90
 */
// Cursor's update, spins it
void Cursor::UpdateObject(u32 frames)
{
	mAngle = fmod(mAngleSpeed * (f32)frames + mAngle, TWO_PI);
	SetRotationZ(mAngle, 0);
	XObj::UpdateObject(frames);
}

/**
 * @note Address: 0x80026C2C
 * @note Size: 0x4C
 */
BOOL Cursor::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info)) {
		HSD_JObjSetFlags(mJObj, CLASSICAL_SCALE);
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80026C78
 * @note Size: 0x38
 */
void Cursor::InitializeObject(u32 id)
{
	XObj::InitializeObject(id);
	mAngleSpeed = 0.0f;
	mAngle      = 0.0f;
}

/**
 * @note Address: 0x80026CB0
 * @note Size: 0x878
 */
// NONMATCHING: float regalloc
// Logo::Draw, the current logo centred on the screen at its alpha. the first one has a shine
// that comes and goes with the angle
extern "C" void DrawCurrentLogo(Logo* logo)
{
	if (logo->IsActive() && logo->mAlpha > 0.0f) {
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		SetGsColorUpdate(gGame.mGs.mCurRenderMode, 1);
		SetGsAlphaUpdate(gGame.mGs.mCurRenderMode, 0);
		SetGsAlphaCompare(gGame.mGs.mCurRenderMode, 4, 0, 0, 3, 255);
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 7, 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		CTexObj tex;
		if (logo->mState == 3 || logo->mState == 4 || logo->mState == 5) {
			tex       = logo->mTpl.mTex.Init(logo->GetDesc(3), 0);
			u16 w     = GXGetTexObjWidth(&tex);
			f32 h     = GXGetTexObjHeight(&tex);
			u32 color = (u32)(255.0f * logo->mAlpha) | 0xFFFFFF00;
			DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 320.0f, 224.0f, 0.0f, w, h, 0.0f, 0.0f, 0.0f);

			tex        = logo->mTpl.mTex.Init(logo->GetDesc(4), 0);
			f32 shineW = GXGetTexObjWidth(&tex);
			f32 shineH = GXGetTexObjHeight(&tex);
			f32 y      = shineH / 2.0f + 359.0f;
			f32 shine  = std::sinf(logo->mAngle);
			if (shine < 0.0f) {
				shine = 0.0f;
			}
			color = (u32)(255.0f * logo->mAlpha * shine) | 0xFFFFFF00;
			DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 320.0f, y, 0.0f, shineW, shineH, 0.0f, 0.0f, 0.0f);
		}
		if (logo->mState == 6 || logo->mState == 7 || logo->mState == 8) {
			tex   = logo->mTpl.mTex.Init(logo->GetDesc(0), 0);
			u16 w = GXGetTexObjWidth(&tex);
			f32 h = GXGetTexObjHeight(&tex);
#if defined(VERSION_JP)
			u32 color = (u32)(255.0f * logo->mAlpha) | 0x0046FF00;
#else
			u32 color = (u32)(255.0f * logo->mAlpha) | 0xDC000000;
#endif
			DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 320.0f, h / 2.0f + 154.0f, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
		}
		if (logo->mState == 9 || logo->mState == 10 || logo->mState == 11) {
			tex       = logo->mTpl.mTex.Init(logo->GetDesc(1), 0);
			u16 w     = GXGetTexObjWidth(&tex);
			u16 h     = GXGetTexObjHeight(&tex);
			u32 color = (u32)(255.0f * logo->mAlpha) | 0xFFFFFF00;
			DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 320.0f, 224.0f, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
		}
		if (logo->mState == 12 || logo->mState == 13 || logo->mState == 14) {
			tex       = logo->mTpl.mTex.Init(logo->GetDesc(2), 0);
			u16 w     = GXGetTexObjWidth(&tex);
			u16 h     = GXGetTexObjHeight(&tex);
			u32 color = (u32)(255.0f * logo->mAlpha) | 0xFFFFFF00;
			DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 320.0f, 224.0f, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
		}
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	}
}

/**
 * @note Address: 0x80027528
 * @note Size: 0x554
 */
// NONMATCHING: sprintf's format and buffer are set up in another order
// Logo::Update, reads the logos in, then fades each of them in and out
extern "C" void UpdateBootLogos(Logo* logo, u32 frames)
{
#if !defined(VERSION_JP)
	char path[64];
#endif
	switch (logo->mState) {
	case 1:
		logo->mState = 2;
	case 2: {
#if defined(VERSION_PAL)
		static const char* const suffix[] = { "_en", "_ge", "_fr", "_sp", "_it" };
		sprintf(path, "tpl/logo%s.tpl", suffix[gGame.mLanguage]);
#elif defined(VERSION_US)
		const char* suffix = "_us";
		sprintf(path, "tpl/logo%s.tpl", suffix);
#else
		const char* path = "tpl/logo.tpl";
#endif
		if (!logo->mTpl.ReadEx(path, 0, 0)) {
			break;
		}
		logo->mTpl.CloseHandle();
		if ((s32)((TPLPalette*)logo->mTpl.mBuffer)->numDescriptors > 3) {
			logo->mState = 3;
		} else {
			logo->mState = 6;
		}
		logo->ResetFade();
	}
	case 3:
		if (logo->mAlpha < 1.0f && (logo->mAlpha += (1.0f / (0.5f * FRAME_RATE)) * frames) > 1.0f) {
			logo->mAlpha = 1.0f;
		}
		if (logo->mAlpha != 1.0f) {
			break;
		}
		logo->mState = 4;
	case 4:
		logo->mTimer += frames;
		if (logo->mTimer >= 1.5f * FRAME_RATE) {
			if ((logo->mAngle += (DEG2RAD * (180.0f / FRAME_RATE)) * frames) >= 3.1415927f) {
				logo->mAngle = 0.0f;
			}
			BOOL done = logo->mTimer >= 60.0f * FRAME_RATE ? TRUE : (&gGame.mPad.mPad[0])->mTrigger != 0;
			if (!done) {
				break;
			}
		} else {
			break;
		}
		logo->mState = 5;
	case 5:
		if (logo->mAlpha > 0.0f && (logo->mAlpha -= (1.0f / (0.5f * FRAME_RATE)) * frames) < 0.0f) {
			logo->mAlpha = 0.0f;
		}
		if (logo->mAlpha == 0.0f) {
			logo->mState = 6;
			logo->ResetFade();
		}
		break;
	case 6:
		if (logo->mAlpha < 1.0f && (logo->mAlpha += (1.0f / (0.5f * FRAME_RATE)) * frames) > 1.0f) {
			logo->mAlpha = 1.0f;
		}
		if (logo->mAlpha != 1.0f) {
			break;
		}
		logo->mState = 7;
	case 7:
		if ((logo->mTimer += frames) >= FRAME_RATE) {
			logo->mState = 8;
		} else {
			break;
		}
	case 8:
		if (logo->mAlpha > 0.0f && (logo->mAlpha -= (1.0f / (0.5f * FRAME_RATE)) * frames) < 0.0f) {
			logo->mAlpha = 0.0f;
		}
		if (logo->mAlpha == 0.0f) {
			logo->mState = 9;
			logo->ResetFade();
		}
		break;
	case 9:
		if (logo->mAlpha < 1.0f && (logo->mAlpha += (1.0f / (0.5f * FRAME_RATE)) * frames) > 1.0f) {
			logo->mAlpha = 1.0f;
		}
		if (logo->mAlpha != 1.0f) {
			break;
		}
		logo->mState = 10;
	case 10:
		if ((logo->mTimer += frames) >= FRAME_RATE) {
			logo->mState = 11;
		} else {
			break;
		}
	case 11:
		if (logo->mAlpha > 0.0f && (logo->mAlpha -= (1.0f / (0.5f * FRAME_RATE)) * frames) < 0.0f) {
			logo->mAlpha = 0.0f;
		}
		if (logo->mAlpha == 0.0f) {
			logo->mState = 12;
			logo->ResetFade();
		}
		break;
	case 12:
		if (logo->mAlpha < 1.0f && (logo->mAlpha += (1.0f / (0.5f * FRAME_RATE)) * frames) > 1.0f) {
			logo->mAlpha = 1.0f;
		}
		if (logo->mAlpha != 1.0f) {
			break;
		}
		logo->mState = 13;
	case 13:
		if ((logo->mTimer += frames) >= FRAME_RATE) {
			logo->mState = 14;
		} else {
			break;
		}
	case 14:
		if (logo->mAlpha > 0.0f && (logo->mAlpha -= (1.0f / (0.5f * FRAME_RATE)) * frames) < 0.0f) {
			logo->mAlpha = 0.0f;
		}
		if (logo->mAlpha == 0.0f) {
			logo->mState = 15;
			logo->ResetFade();
			logo->mTpl.Close();
		}
		break;
	case 15:
		break;
	}
}

/**
 * @note Address: 0x80027A7C
 * @note Size: 0x1C
 */
// Logo's init
extern "C" void ResetLogoFadeState(Logo* logo)
{
	logo->mState = 0;
	logo->ResetFade();
}
