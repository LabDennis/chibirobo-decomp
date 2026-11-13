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
f32 gTitleNameOutlineX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 gTitleNameOutlineY[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
f32 gTitleHelpOutlineX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 gTitleHelpOutlineY[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
}

extern "C" s32 GetItemIconTextureIndex(s32 id);
extern "C" void DrawHudRoundedRectangle(u32 color, f32 x, f32 y, f32 w, f32 h, f32 rx, f32 ry);

static inline BOOL ProjectRoboeyesItem(XObj* object, f32* width, f32* height)
{
	f32 r = object->GetFloorXZExtent() * .5f;
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
	work.used = work.visible = 0;
	XObjPool& pool           = gGame.mXObjs.mPools[8];
	for (s32 i = 0; i < pool.mUsed; ++i) {
		XObj* object = pool.Get(i);
		if (!(object->mStatus.mFlags & 1) || !(object->mStatus.mFlags & 0x10000))
			continue;
		if (!GetItemFlagsMasked(object->GetItem(), 2) || (object->mStatus.mFlags & 0x0400000000000000ULL))
			continue;
		f32 w, h;
		if (ProjectRoboeyesItem(object, &w, &h)) {
			work.entries[work.visible].object = object;
			Vec offset                        = { 0, 0, 0 };
			f32 x, y;
			object->ProjectOffsetToScreen(&offset, &x, &y, NULL, NULL);
			y -= .5f * w;
			f32 dx = 320.0f - x, dy = 224.0f - y;
			work.entries[work.visible++].distance = sqrtf(dx * dx + dy * dy);
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
			y -= radius * .5f;
			const char* name = (const char*)GetItemName(object->GetItem());
			f32 width        = 32.0f + MeasureGameTextWidth(gs, name, TRUE, 20);
			const f32 angle  = 2.3561945f;
			f32 startX = x + radius * (f32)sin(angle), startY = y + radius * (f32)cos(angle);
			f32 endX = x + 1.7f * radius * (f32)sin(angle), left = endX, right = endX + width;
			f32 endY = y + 1.7f * radius * (f32)cos(angle);
			for (u32 j = 0; j < work.used; ++j) {
				const Label& prior = work.entries[j];
				f32 dy = endY - prior.y, dx = left - prior.left;
				if (dy < 0)
					dy = -dy;
				if (dx < 0)
					dx = -dx;
				if (dy < 32 && dx < (left > prior.left ? prior.width : width)) {
					endY         = prior.y - 33;
					f32 distance = endY - startY;
					if (distance < 0)
						distance = -distance;
					if (left == right) {
						endX = startX - distance * .5f;
						left = right = endX - width;
					} else {
						endX  = startX + distance * .5f;
						left  = endX;
						right = endX + width;
					}
					j = -1;
				}
			}
			work.entries[work.used].y       = endY;
			work.entries[work.used].left    = left;
			work.entries[work.used++].width = width;
			if (pass == 0) {
				f32 inner = radius + .1f * radius * (f32)sin(eyes->mPhase), outer = 1.2f * inner;
				for (s32 k = 0; (f32)k < 16.0f; ++k) {
					f32 a = .3926991f * k + eyes->mAngle, b = .3926991f * (k + 1) + eyes->mAngle;
					f32 x0 = x + outer * (f32)sin(a), y0 = y + outer * (f32)cos(a);
					f32 x1 = x + inner * (f32)sin(b), y1 = y + inner * (f32)cos(b);
					DrawGsLineSegment(gs, NULL, x0, y0, 0, gold, x1, y1, 0, gold);
				}
				DrawGsLineSegment(gs, NULL, startX, startY, 0, gold, endX, endY, 0, gold);
			}
			if (pass == 1) {
				DrawHudRoundedRectangle(160, left + width * .5f, endY - 16, width, 16, 1, 1);
				DrawGsLineSegment(gs, NULL, endX, endY, 0, gold, right, endY, 0, gold);
				s32 icon    = GetItemIconTextureIndex(object->GetItem());
				CTpl& tpl   = gGame.mTpl;
				CTexObj tex = tpl.mTex.Init(&((TPLPalette*)tpl.mBuffer)->descriptorArray[icon], 0);
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
		f32 step = (target - value) * 0.5f;
		if (step < 0)
			step = -step;
		ApproachHudValue(&value, &target, step);
	}
};

static inline void DrawRoboeyesLens(CGs* gs, Vec* points, f32 x, f32 y, f32 rx, f32 ry, u32 color, const f32* angles, bool right)
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
		trailColor = (color & ~0xffU) | (u8)(s32)(0.5f * ((f32)(u8)color * (10 - i) / 10.0f));
	}
}

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
	static f32 scanline;
	if (!gGame.mMenu.mActive) {
		if (++scanline >= 4.0f)
			scanline = 0;
	}
	SetGsBlendMode(gs->mCurRenderMode, 3, 4, 5, 15);
	SetGsLineWidth(gs->mCurRenderMode, 12, 5);
	if (mWidth >= mTargetWidth) {
		u32 tint = 0x30303000 | (u8)color;
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
	static s32 state;
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
		rw.target = width * .5f;
		rh.target = height * .5f;
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
	f32 rx = .6f * (rw.value * .5f), ry = .6f * (rh.value * .5f);
	Vec points[65];
	DrawRoboeyesLens(gs, points, x - offset, y, .6f * (lw.value * .5f), .6f * (lh.value * .5f), color, trail, false);
	DrawRoboeyesLens(gs, points, x + offset, y, rx, ry, color, trail, true);
	if (state == 2) {
		CTexObj tex;
		static s32 frame;
		++frame;
		u32 tint = 0xffffff00 | (u8)color;
		for (s32 i = 0; i < 4; ++i) {
			f32 a      = HALF_PI * i;
			f32 radius = rx + 3.0f * (f32)sin(DEG2RAD * ((frame * 24) % 360));
			f32 sx = (f32)sin(a), cy = (f32)cos(a);
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			CTpl& tpl = gGame.mTpl;
			tex       = tpl.mTex.Init(&((TPLPalette*)tpl.mBuffer)->descriptorArray[128 - StatusTextureOffset], 0);
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
	DrawGsVertexQuad(gs, quad[0].x, quad[0].y, quad[0].z, color, quad[1].x, quad[1].y, quad[1].z, color, quad[2].x, quad[2].y, quad[2].z,
	                 color, quad[3].x, quad[3].y, quad[3].z, color);
}

static inline void BeginWipe(CGs* gs, s32 mode)
{
	if (mode) {
		static const Vec corners[4] = { { 0, 0, -1 }, { 640, 0, -1 }, { 640, 448, -1 }, { 0, 448, -1 } };
		WipeQuad quad               = { { corners[0], corners[1], corners[2], corners[3] } };
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
	static const Vec corners[4] = { { 0, 0, 0 }, { 640, 0, 0 }, { 640, 448, 0 }, { 0, 448, 0 } };
	WipeQuad quad               = { { corners[0], corners[1], corners[2], corners[3] } };
	u32 color = ((u32)wipe->mColor.r << 24) | ((u32)wipe->mColor.g << 16) | ((u32)wipe->mColor.b << 8) | (u32)wipe->mColor.a;
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
	static const Vec corners[4] = { { 0, 0, 0 }, { 640, 0, 0 }, { 640, 448, 0 }, { 0, 448, 0 } };
	WipeQuad quad               = { { corners[0], corners[1], corners[2], corners[3] } };
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
void CScrnWipeCustom::Draw(CGs* gs, s32 mode)
{
	if (!mActive)
		return;
	BeginWipe(gs, mode);
	for (s32 row = 0; row < 448.0f / 40.0f; ++row) {
		f32 y = (f32)row * 40.0f + 20.0f;
		for (s32 col = 0; col < 640.0f / 40.0f; ++col) {
			f32 x = (f32)col * 40.0f + 20.0f;
			f32 extra;
			if (col < 8.0f)
				extra = 0.2f * col;
			else
				extra = 0.2f * (16.0f - col);
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

static inline void SetTilePoint(Vec& v, f32 x, f32 y)
{
	v.x = x;
	v.y = y;
}

static inline void SetTileEdge(Vec* vertices, f32 x, f32 y, f32 outerX, f32 outerY, f32 nextX, f32 nextY)
{
	SetTilePoint(vertices[0], x, y);
	SetTilePoint(vertices[1], outerX, outerY);
	SetTilePoint(vertices[2], nextX, nextY);
	SetTilePoint(vertices[3], x, y);
}

/**
 * @note Address: 0x800220DC
 * @note Size: 0x7C0
 */
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
	const f32 dx = edgeWidth * 0.5f, dy = edgeHeight * 0.5f;
	f32 left = -mWidth * 0.5f, right = mWidth * 0.5f;
	f32 outerLeft = -mWidth * 0.5f - dx, outerRight = mWidth * 0.5f + dx;
	f32 top = -mHeight * 0.5f, bottom = mHeight * 0.5f;
	f32 outerTop = -mHeight * 0.5f - dy, outerBottom = mHeight * 0.5f + dy;
	SetTileEdge(vertices, left, top, outerLeft, outerTop, outerRight, outerTop);
	SetTileEdge(vertices + 4, right, top, outerRight, outerTop, outerRight, outerBottom);
	SetTileEdge(vertices + 8, right, bottom, outerRight, outerBottom, outerLeft, outerBottom);
	SetTileEdge(vertices + 12, left, bottom, outerLeft, outerBottom, outerLeft, outerTop);
	SetTilePoint(vertices[16], left, top);
	SetTilePoint(vertices[17], outerLeft, outerTop);
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
		v[0].pos.x    = mX - mWidth * 0.5f * (f32)sin(angle);
		v[0].pos.y    = mY + mHeight * 0.5f * (f32)cos(angle);
		v[0].pos.z    = 0.0f;
		v[0].color    = 255;
		v[1].pos.x    = mX - (mWidth + edgeWidth) * 0.5f * (f32)sin(angle);
		v[1].pos.y    = mY + (mHeight + edgeHeight) * 0.5f * (f32)cos(angle);
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
// NONMATCHING: regalloc
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
		time           = FRAME_RATE * time;
		wipe->mSpeed.r = ((f32)r1 - (f32)r0) / time;
		wipe->mSpeed.g = ((f32)g1 - (f32)g0) / time;
		wipe->mSpeed.b = ((f32)b1 - (f32)b0) / time;
		wipe->mSpeed.a = ((f32)a1 - (f32)a0) / time;
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
// NONMATCHING: retail loads the corners' address before it tests the color
extern "C" void DrawScreenColorOverlay(CScrnColor* color, CGs* gs)
{
	if (HasColor(color)) {
		static const Vec corners[4] = { { 0, 0, 0 }, { 640, 0, 0 }, { 640, 448, 0 }, { 0, 448, 0 } };
		WipeQuad quad               = { { corners[0], corners[1], corners[2], corners[3] } };
		u32 packed = ((u32)color->mColor.r << 24) | ((u32)color->mColor.g << 16) | ((u32)color->mColor.b << 8) | (u32)color->mColor.a;
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
// NONMATCHING: regalloc
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
		time            = FRAME_RATE * time;
		f32 sr          = ((f32)r - color->mColor.r) / time;
		f32 sg          = ((f32)g - color->mColor.g) / time;
		f32 sb          = ((f32)b - color->mColor.b) / time;
		f32 sa          = ((f32)a - color->mColor.a) / time;
		color->mSpeed.r = sr;
		color->mSpeed.g = sg;
		color->mSpeed.b = sb;
		color->mSpeed.a = sa;
	}
}

/**
 * @note Address: 0x80023EE0
 * @note Size: 0x20C
 */
// NONMATCHING: regalloc
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
		time            = FRAME_RATE * time;
		color->mSpeed.r = ((f32)r1 - (f32)r0) / time;
		color->mSpeed.g = ((f32)g1 - (f32)g0) / time;
		color->mSpeed.b = ((f32)b1 - (f32)b0) / time;
		color->mSpeed.a = ((f32)a1 - (f32)a0) / time;
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
	for (s32 i = 0; i < count; ++i) { }
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
			f32 shine  = sin(logo->mAngle);
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

/**
 * @note Address: 0x80027A98
 * @note Size: 0xD8
 */
// CXObj's SetScaleY
void CXObj::SetScaleY(f32 value, s32 force)
{
	mMoveScale.y         = value;
	mMoveScale.mTarget.y = value;
	if (mJObj != NULL) {
		CJObj::SetScaleY(value, force);
	}
}

/**
 * @note Address: 0x80027B70
 * @note Size: 0x18C
 */
// CXObj's SetScale(scale)
void CXObj::SetScale(f32 scale, s32 force)
{
	f32 x = scale;
	f32 y = scale;
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
 * @note Address: 0x80027CFC
 * @note Size: 0x18C
 */
// CXObj's SetScale(x, y, z)
void CXObj::SetScale(f32 x, f32 y, f32 z, s32 force)
{
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
 * @note Address: 0x80027E88
 * @note Size: 0x178
 */
// CXObj's SetScale(v)
void CXObj::SetScale(const Vec& scale, s32 force)
{
	mMoveScale.Set(scale);
	mMoveScale.mTarget.Set(scale);
	if (mJObj) {
		CJObj::SetScale(scale, force);
	}
}

/**
 * @note Address: 0x80028000
 * @note Size: 0xB4
 */
// XObjDemo's DrawObject, draws through its own camera and lights when it has them
void XObjDemo::DrawObject()
{
	if (mState != 0) {
		CCamera* camera = mArchive.mDesc[0] != NULL ? &mCamera : &gGame.mCameras[0];
		CLight* light   = mArchive.mDesc[1] != NULL ? &mLights[0] : &gGame.mLights[0];
		light->SetCurrent(camera);
		if (camera->SetCurrent() != NULL) {
			DrawWithView(NULL);
			camera->EndCurrent();
		}
	}
}

static inline bool FadeDone()
{
	return 0.0f == gGame.mFade.mSpeed.r && 0.0f == gGame.mFade.mSpeed.g && 0.0f == gGame.mFade.mSpeed.b && 0.0f == gGame.mFade.mSpeed.a;
}

/**
 * @note Address: 0x800280B4
 * @note Size: 0x244
 */
// XObjDemo's update, steps the camera and light anims with the model
void XObjDemo::UpdateObject(u32 frames)
{
	if (mState == 0) {
		return;
	}
	if (mState == 2 && FadeDone()) {
		mState = 0;
		return;
	}
	XObj::UpdateObject(frames);
	if (mArchive.mDesc[0] != NULL && mCameraAnim >= 0) {
		mCamera.SetAnimRate((f32)frames);
		mCamera.Anim();
	}
	for (s32 i = 0; i < mLightCount; ++i) {
		if (mArchive.mDesc[i + 1] != NULL && mLightAnims[i] >= 0) {
			mLights[i].SetAnimRate((f32)frames);
			mLights[i].Anim();
		}
	}
	if (mLoop) {
		if (GetAObj(mJObj)->flags & 0x44000000) {
			mState = 1;
			SetPos(mMoveTranslate, 0);
			SetAnim(0, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		}
	}
}

/**
 * @note Address: 0x80028368
 * @note Size: 0xA4
 */
// XObjDemo's Release
void XObjDemo::Release()
{
	if (mArchive.mDesc[0] != NULL) {
		mCamera.Release();
	}
	for (s32 i = 0; i < mLightCount; ++i) {
		if (mArchive.mDesc[i + 1] != NULL) {
			mLights[i].Release();
		}
	}
	XObj::Release();
}

/**
 * @note Address: 0x8002840C
 * @note Size: 0xD4
 */
// XObjDemo's LoadFromPlacement, builds the camera and the light list from the archive
BOOL XObjDemo::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info)) {
		if (mArchive.mDesc[0] != NULL) {
			mCamera.Init((HSD_CObjDesc*)mArchive.mDesc[0]->mObjectDesc);
#if defined(VERSION_PAL)
			mCamera.SetViewport(0.0f, 0.0f, &gGame.mGs.mScreen);
#endif
		}
		HSD_LObj* prev = NULL;
		for (s32 i = 0; i < 8; ++i) {
			if (mArchive.mDesc[i + 1] != NULL) {
				mLights[i].Init((HSD_LightDesc*)mArchive.mDesc[i + 1]->mObjectDesc, FALSE);
				mLightCount++;
				if (prev != NULL) {
					prev->next = &mLights[i];
				}
				prev = &mLights[i];
			}
		}
		if (prev != NULL) {
			prev->next = NULL;
		}
		return TRUE;
	}
	return FALSE;
}

// the title screen's background and text colors, the logo's textures and which of each is in use
const TitlePaletteEntry gTitlePalette[] = {
	{ { 0, 0, 128, 0 }, {} },     { { 128, 0, 0, 0 }, {} },     { { 0, 128, 0, 0 }, {} },
	{ { 200, 200, 200, 0 }, {} }, { { 128, 128, 128, 0 }, {} }, { { 0, 207, 233, 0 }, {} },
	{ { 255, 220, 133, 0 }, {} }, { { 255, 220, 0, 0 }, {} },   { {}, { 255, 255, 255, 0 } },
};
#if defined(VERSION_JP)
const s32 gTitleLogoTextures[3] = { 0, 3, 4 };
#else
const s32 gTitleLogoTextures[3] = {};
#endif
s32 gTitlePaletteIndex = 3;
s32 gTitleLogoIndex    = 1;

/**
 * @note Address: 0x800284E0
 * @note Size: 0x64
 */
// XObjDemo's ResetObjectState
void XObjDemo::ResetObjectState()
{
	XObj::ResetObjectState();
	mState      = 0;
	mLoop       = FALSE;
	mCameraAnim = -1;
	mLightCount = 0;
	for (s32 i = 0; i < 8; ++i) {
		mLightAnims[i] = -1;
	}
}

extern "C" void DrawTitleHelpText(Title* menu)
{
	f32 alpha   = menu->mMenuAlpha * menu->mHelpAlpha;
	f32 spacing = 30.0f;
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	CTexObj blob     = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[109 - StatusTextureOffset], 0);
	menu->mHelpCount = 0;
	const char* text = menu->mHelpText;
	if (text) {
		const int limit = 20;
		for (int i = 0; i < limit && *text; i++, text++) {
			menu->mBg[i].mText = text;
			menu->mHelpCount++;
			if (gGame.IsJapanese() && IsSjisLead(*text))
				text++;
		}
	}
	f32 zero    = 0.0f;
	f32 opacity = 255.0f * alpha;
	for (int i = 0; i < menu->mHelpCount; i++) {
		MenuHelpChar* glyph = &menu->mBg[i];
		s32 width;
		CFont font;
		CTexObj tex  = *GetFontGlyphTexture(&font, glyph->mText, &width);
		f32 bob      = 2.0f * (f32)sin(glyph->mAngle0);
		f32 rotation = sin(0.7f * glyph->mAngle1);
		f32 s1       = sin(3.1415927f * alpha);
		f32 s2       = sin(1.5707964f * alpha);
		f32 scale    = 0.2f * s1 + s2;
		f32 x        = 0.5f * spacing + (i * spacing + (320.0f - menu->mHelpCount * spacing * 0.5f));
		f32 y        = 384.0f + bob;
		rotation     = -(0.017453292f * (10.0f * rotation));
		u32 color    = (u32)opacity | 0xFFFFFF00;
		DrawGsTexturedQuad(&gGame.mGs, &blob, color, color, color, color, x, y, 1.0f, 30.0f * scale, 30.0f * scale, 0.0f, -zero, rotation);
		scale       = 30.0f * (0.8f * scale);
		f32 padding = HelpGlyphPadding(width, scale);
		DrawHelpLetter(tex, x, y, bob, scale, padding, rotation, zero, opacity, gTitleHelpOutlineX, gTitleHelpOutlineY);
	}
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
}

// the name's letters shrink away
static inline void HideName(Title* title)
{
	for (u32 i = 0; i < 20; i++) {
		title->mName.mChars[i].mWait = 0.0f;
	}
}

// the card was taken out: back to the card check with the save files cleared
static inline void OnCardRemoved(Title* title)
{
	title->mResult = -3;
	title->SetMode(9);
	title->mOffset = -32.0f;
	for (s32 i = 0; i < 3; i++) {
		ResetSaveSlotRecord(&gTitleSaveSlots[i]);
	}
}

// the card a new file was made on went away: back to the start
static inline void OnFormattedCardGone(Title* title)
{
	title->SetMode(25);
	title->mState = 11;
	PlayGameSound(&gGame.mSound, 0x88, 0xFF);
	title->mTargetPanelY = -200.0f;
	title->mOffset       = 0.0f;
	title->mAlpha        = 0.0f;
	HideName(title);
	title->mSelection     = 0;
	title->mSaveFound     = 0;
	title->mCardScanState = 0;
}

// moves the yes or no choice with left and right
static inline void MoveYesNo(Title* title, CPad& pad)
{
	if (pad.mPad->mRepeat & 0x40001) {
		if (title->mChoice > 0) {
			title->mChoice--;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		}
	} else if (pad.mPad->mRepeat & 0x80002) {
		if (title->mChoice < 1) {
			title->mChoice++;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		}
	}
}

/**
 * @note Address: 0x80028BAC
 * @note Size: 0x5BC
 */
// NONMATCHING: the loop's float registers are assigned in another order
// draws the title's name: a bubble per letter, then the letter with a shadow on four sides
extern "C" void DrawTitleNameBubbles(Title* title)
{
	if (title->mName.mActive) {
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		CTexObj bubble = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[108 - StatusTextureOffset], 0);
		f32 zero       = 0.0f;
		u32 color      = title->mName.mColor & 0xFFFFFF00;
		for (s32 i = 0; i < title->mName.mCount; i++) {
			f32 scale = title->mName.mChars[i].mScale;
			if (scale <= zero) {
				continue;
			}
#if !defined(VERSION_JP)
			if (strncmp(title->mName.mChars[i].mText, " ", 1) == 0) {
				continue;
			}
#endif
			CFont font;
			s32 width;
			CTexObj letter = *GetFontGlyphTexture(&font, title->mName.mChars[i].mText, &width);
			f32 bob;
			f32 tilt;
			f32 swell;
			f32 pop;
			f32 x;
			f32 rot;
			f32 y;
			bob   = 4.0f * (f32)sin(title->mName.mChars[i].mBob);
			tilt  = sin(0.7f * title->mName.mChars[i].mTilt);
			swell = sin(3.1415927f * scale);
			pop   = 0.2f * swell + (f32)sin(1.5707964f * scale);
#if defined(VERSION_PAL)
			x = 20.0f + (40.0f * i + (320.0f - 40.0f * title->mName.mCount / 2.0f));
#else
			x = 24.0f + (48.0f * i + (320.0f - 48.0f * title->mName.mCount / 2.0f));
#endif
			u32 alpha = 255.0f * scale;
			rot       = -(0.017453292f * (10.0f * tilt));
			y         = 64.0f + bob;
			DrawGsTexturedQuad(&gGame.mGs, &bubble, alpha | 0xFFFFFF00, alpha | 0xFFFFFF00, alpha | 0xFFFFFF00, alpha | 0xFFFFFF00, x, y,
			                   0.0f, 20.0f * (2.0f * pop), 20.0f * (2.0f * pop), zero, -zero, rot);
			f32 size   = 20.0f * (1.7f * pop);
			f32 shrink = width / 24.0f * size;
			f32 off    = size - shrink;
			for (s32 j = 0; j < 4; j++) {
				DrawGsTexturedQuad(&gGame.mGs, &letter, alpha, alpha, alpha, alpha, off / 2.0f + (x + gTitleNameOutlineX[j]),
				                   bob + (64.0f + gTitleNameOutlineY[j]), 0.0f, size, size, zero, -zero, rot);
			}
			DrawGsTexturedQuad(&gGame.mGs, &letter, color | alpha, color | alpha, color | alpha, color | alpha, off / 2.0f + x, y, 0.0f,
			                   size, size, zero, -zero, rot);
		}
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 7, 0);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
	}
}

/**
 * @note Address: 0x80029168
 * @note Size: 0x5F8
 */
// the options: up and down pick rumble, sound or back, A changes the line, B goes back. the
// rumble test runs the motor for 24 frames
extern "C" void UpdateTitleOptions(Title* title, u32 frames)
{
	CPad pad(0);

	title->mCursor.UpdateObject(frames);
	title->mHighlightDelay += frames;
	if (title->mHighlightDelay >= FRAME_RATE) {
		title->mHighlightDelay = FRAME_RATE;
		title->mPulseAngle     = fmod(title->mPulseAngle + frames * 9, 360.0);
	}
	if (!pad.mPad->mConnected && title->mRumble) {
		title->mRumble = FALSE;
		gGame.mStatus.mFlags &= ~0x800000000ULL;
	}

	switch (title->mOptionState) {
	case 0:
		if (pad.mPad->mRepeat & 0x10008) {
			if (title->mOptionLine > 0) {
				title->mOptionLine--;
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
				title->mPulseAngle = 45.0f;
			}
		} else if (pad.mPad->mRepeat & 0x20004) {
			if (title->mOptionLine < 2) {
				title->mOptionLine++;
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
				title->mPulseAngle = 45.0f;
			}
		}
		if (pad.mPad->mTrigger & 0x100) {
			switch (title->mOptionLine) {
			case 0:
				if (pad.mPad->mConnected) {
					if (title->mRumble) {
						title->mRumble = FALSE;
						gGame.mStatus.mFlags &= ~0x800000000ULL;
					} else {
						title->mRumble = TRUE;
						gGame.mStatus.mFlags |= 0x800000000ULL;
						if (pad.mPad->mConnected) {
							HSD_PadRumbleOn(pad.mPad->mPort);
						}
						title->mOptionState = 1;
						title->mRumbleTimer = 24;
					}
					PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				} else {
					PlayGameSound(&gGame.mSound, 0x85, 0xFF);
				}
				break;
			case 1:
				if (title->mMono) {
					title->mMono = FALSE;
					gGame.mStatus.mFlags &= ~0x1000000000ULL;
					SetGameSoundStereo(&gGame.mSound, FALSE);
				} else {
					title->mMono = TRUE;
					gGame.mStatus.mFlags |= 0x1000000000ULL;
					SetGameSoundStereo(&gGame.mSound, TRUE);
				}
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				break;
			case 2:
				title->mOptionState = 2;
				title->mCursor.SetAnim(1, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				title->mPulseAngle = 90.0f;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				break;
			}
		} else if (pad.mPad->mTrigger & 0x200) {
			title->mOptionState = 2;
			title->mCursor.SetAnim(1, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			title->mPulseAngle = 90.0f;
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
		}
		break;
	case 1:
		if (--title->mRumbleTimer < 0) {
			title->mOptionState = 0;
			if (pad.mPad->mConnected) {
				HSD_PadRumbleOffN(pad.mPad->mPort);
			}
			if (pad.mPad->mConnected) {
				HSD_PadRumbleRemove(pad.mPad->mPort);
			}
		}
		break;
	case 2:
		HSD_AObj* aobj = GetAObj(title->mCursor.mJObj);
		if ((aobj->flags & 0x44000000) || title->mCursor.mAnimId == 0 || title->mCursor.mArchive.mNum[0] < 2) {
			title->mCursor.SetAnim(0, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			title->mPulseAngle = 0.0f;
			title->mState      = 11;
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			title->mTargetOptionAlpha = 0.0f;
			title->mOptionState       = 0;
			title->mOptionLine        = 0;
			title->mTargetPanelY      = -200.0f;
			title->mOffset            = 0.0f;
			title->mAlpha             = 0.0f;
			HideName(title);
		}
		break;
	}
}

// how far the cursor's first animation is through, from 0 to 1
static inline f32 GetAnimProgress(Title* title)
{
	HSD_AObj* aobj = GetAObj(title->mCursor.mJObj);
	f32 frame      = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
	aobj           = GetAObj(title->mCursor.mJObj);
	return frame / (aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f);
}

// the options line is lit once the window is fully open
static inline bool IsLineLit(Title* title, s32 line)
{
	bool lit = false;
	if (title->mOptionLine == line && title->mHighlightDelay >= FRAME_RATE) {
		lit = true;
	}
	return lit;
}

MenuSaveSlot gTitleSaveSlots[3];

extern "C" {
void DrawMenuBannerPanels(const char*, f32, f32, f32, f32);
void DrawSaveSlotDetails(MenuSaveSlot*, u8, s16, u8, f32, f32, f32, f32, f32, f32, f32);
#if defined(VERSION_JP)
s32 DrawJapaneseSaveStatusMessage(s32, s32, f32, f32, f32, f32);
s32 DrawJapaneseCardErrorMessage(s32, s32, u8, f32, f32, f32, f32);
#else
// each save slot is this much wider than 154, and the three shift left by half of three times it
static f32 sSaveSlotExtraWidth = 10.0f;
s32 DrawSaveStatusMessage(s32, s32, f32, f32, f32, f32);
s32 DrawCardErrorMessage(s32, s32, u8, f32, f32, f32, f32);
#endif
}

static inline u32 LoadTextColor(f32 alpha)
{
	return 0x14141400 | (u32)(255.0f * alpha);
}
static inline f32 LoadPromptY(Title* title, f32 panelY)
{
	return ((8.0f + (356.0f - panelY)) - 6.0f) + title->mLoadOffset;
}
static inline s32 LoadStatusAt(Title* title, s32 id, f32 panelX, f32 panelY)
{
#if defined(VERSION_JP)
	return DrawJapaneseSaveStatusMessage(title->mChannel, id, panelX, panelY, 21.0f, title->mLoadAlpha);
#else
	return DrawSaveStatusMessage(title->mChannel, id, panelX, panelY, 21.0f, title->mLoadAlpha);
#endif
}
static inline s32 LoadStatusLines(Title* title, s32 id, f32 panelX, f32 panelY)
{
	return LoadStatusAt(title, id, 320.0f + panelX, LoadPromptY(title, panelY));
}
#if !defined(VERSION_JP)
static inline void DrawLoadText(const char* text, f32 center, f32 y, u32 color)
{
	f32 width = MeasureGameTextWidth(&gGame.mGs, text, 1, 21.0f);
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, center - width * 0.5f, y, 0.0f, 21.0f, 21.0f);
}
#endif
static inline const char* LoadPromptText(s32 western, s32 japanese)
{
#if defined(VERSION_JP)
	return GetTitleAndSaveMenuText(japanese);
#else
	return GetLocalizedInterfaceText(western);
#endif
}

#if defined(VERSION_JP)
static inline u32 LoadHalfWidth(u32 cells)
{
	return ((cells - 1) * 21) / 2;
}
#else
static inline void DrawLoadChoice(Title* title, s32 label, f32 center, f32 y)
{
	u32 color = LoadTextColor(title->mLoadAlpha);
	DrawLoadText(GetLocalizedInterfaceText(label), center, y, color);
}
#endif

extern "C" void DrawTitleLoadScreen(Title* title)
{
	switch (title->mMode) {
	case 2:
	case 3:
	case 8:
	case 15:
	case 24:
		for (s32 i = 0; i < 3; ++i) {
#if defined(VERSION_JP)
			f32 x = 49.0f + (194.0f * i + 77.0f), width = 154.0f;
#else
			f32 width = 154.0f + sSaveSlotExtraWidth;
			f32 x     = 49.0f + ((width * .5f) + i * (40.0f + width)) - (3.0f * sSaveSlotExtraWidth) * .5f;
#endif
			u8 selected = 0, empty = 0;
			if (title->mFile == i)
				selected = 1;
			if (title->mFreeBlocks >= 6 && title->mFreeEntries > 0)
				empty = 1;
			DrawSaveSlotDetails(&gTitleSaveSlots[i], empty, i, selected, x, 223.0f, width, 230.0f, title->mLoadAlpha, 1.0f, 1.0f);
		}
		break;
	}
	f32 panelX = title->mPanelX, panelY = title->mPanelY - 8.0f;
	DrawMenuBannerPanels(NULL, panelX, panelY, 1.0f, title->mLoadOffset);
	bool showMessage = false;
	s32 lines        = 2;
	char text[256];

	switch (title->mMode) {
	case 2:
		if (!title->mSelection) {
#if defined(VERSION_JP)
			sprintf(text, "%s", LoadPromptText(0x94, 0x15));
			u32 cells  = strlen(text) / 2;
			f32 center = 320.0f + panelX, top = 8.0f + (356.0f - panelY);
			f32 x = center - LoadHalfWidth(cells), y = (top - 6.0f) + title->mLoadOffset;
			DrawGsProportionalTextFormat(&gGame.mGs, LoadTextColor(title->mLoadAlpha), text, x, y, 0.0f, 21.0f, 21.0f);
			sprintf(text, "%s", LoadPromptText(0x95, 0x16));
			cells = strlen(text) / 2;
			DrawGsProportionalTextFormat(&gGame.mGs, LoadTextColor(title->mLoadAlpha), text, center - LoadHalfWidth(cells),
			                             (21.0f + top) + title->mLoadOffset, 0.0f, 21.0f, 21.0f);
#else
			f32 top   = 8.0f + (356.0f - panelY);
			u32 color = LoadTextColor(title->mLoadAlpha);
			DrawLoadText(LoadPromptText(0x94, 0), 320.0f, (top - 6.0f) + title->mLoadOffset, color);
			DrawLoadText(LoadPromptText(0x95, 0), 320.0f, (21.0f + top) + title->mLoadOffset, color);
#endif
		} else {
			sprintf(text, "%s", LoadPromptText(0xbc, 0x12));
			showMessage = true;
		}
		break;
	case 3:
		sprintf(text, "%s", LoadPromptText(0xa8, 1));
		showMessage = true;
		break;
	case 5:
		lines = LoadStatusLines(title, 10, panelX, panelY);
		break;
	case 6:
		lines = LoadStatusLines(title, 11, panelX, panelY);
		break;
	case 8:
		sprintf(text, "%s", LoadPromptText(0xc1, 0x32));
		showMessage = true;
		break;
	case 11: {
		f32 x = 320.0f + panelX, y = LoadPromptY(title, panelY);
		if (title->mPhase == 0)
			lines = LoadStatusAt(title, 5, x, y);
		else
			lines = LoadStatusAt(title, 6, x, y);
		break;
	}
	case 12:
		lines = LoadStatusLines(title, 7, panelX, panelY);
		break;
	case 13: {
		f32 x = 320.0f + panelX, y = LoadPromptY(title, panelY);
		if (title->mPhase == 0)
			lines = LoadStatusAt(title, 7, x, y);
		else
			lines = LoadStatusAt(title, 9, x, y);
		break;
	}
	case 9:
	case 14:
#if defined(VERSION_JP)
		lines = DrawJapaneseCardErrorMessage(title->mChannel, title->mResult, 1, 320.0f + panelX, LoadPromptY(title, panelY), 21.0f,
		                                     title->mLoadAlpha);
#else
		lines = DrawCardErrorMessage(title->mChannel, title->mResult, 1, 320.0f + panelX, LoadPromptY(title, panelY), 21.0f,
		                             title->mLoadAlpha);
#endif
		break;
	case 15:
		lines = LoadStatusLines(title, 4, panelX, panelY);
		break;
	case 18:
		lines = LoadStatusLines(title, 1, panelX, panelY);
		break;
	case 19:
#if defined(VERSION_JP)
		lines = LoadStatusLines(title, 12, panelX, panelY);
#else
	{
		f32 x = 320.0f + panelX, y = LoadPromptY(title, panelY);
		if (title->mFormatCancelled == 1)
			lines = LoadStatusAt(title, 16, x, y);
		else
			lines = LoadStatusAt(title, 12, x, y);
	}
#endif
		break;
	case 20:
		lines = LoadStatusLines(title, 3, panelX, panelY);
		break;
	case 21:
		lines = LoadStatusLines(title, 8, panelX, panelY);
		break;
	case 22:
		lines = LoadStatusLines(title, 14, panelX, panelY);
		break;
	case 24:
		if (title->mPhase == 0)
			lines = LoadStatusLines(title, 4, panelX, panelY);
		break;
	case 25:
		break;
	}
	if (lines <= 2)
		lines = 2;
	title->mOffset = -(lines - 2) * 21;
	if (showMessage) {
#if defined(VERSION_JP)
		u32 cells = strlen(text) / 2;
		DrawGsProportionalTextFormat(&gGame.mGs, LoadTextColor(title->mLoadAlpha), text, (320.0f + panelX) - LoadHalfWidth(cells),
		                             LoadPromptY(title, panelY), 0.0f, 21.0f, 21.0f);
#else
		DrawLoadText(text, 320.0f, LoadPromptY(title, panelY), LoadTextColor(title->mLoadAlpha));
#endif
	}

	switch (title->mMode) {
	case 3:
	case 4:
	case 5:
	case 6:
	case 11: {
		f32 extra = 0;
		if (lines > 1)
			extra = (lines - 2) * 21;
#if defined(VERSION_JP)
		sprintf(text, "%s", LoadPromptText(0, 0x33));
		u32 cells = strlen(text) / 2;
		f32 barX  = title->mChoice == 0 ? ((32.0f + (320.0f + panelX)) - LoadHalfWidth(cells)) - 2.0f
		                                : 60.0f + (10.0f + ((32.0f + (320.0f + panelX)) - LoadHalfWidth(cells)));
		f32 width = 56.699997f;
#else
		f32 barX = title->mChoice == 0 ? 281.25f : 358.75f, width = 61.5f;
#endif
		f32 base = (24.0f + (12.0f + (356.0f - panelY))) - 6.0f;
		{
			CTexObj tex;
			f32 alpha = title->mLoadAlpha, scroll = title->mLoadScroll;
			f32 barY     = 8.0f + (extra + (base + title->mLoadOffset));
			u32 barColor = 0x181CC600 | (u32)(200.0f * alpha);
			InitMenuSelectionBar(tex);
			DrawMenuSelectionBar(tex, barX, 2.0f + barY, width, 22.0f, alpha, scroll, barColor);
		}
		f32 y = extra + (base + title->mLoadOffset);
#if defined(VERSION_JP)
		DrawGsProportionalTextFormat(&gGame.mGs, LoadTextColor(title->mLoadAlpha), text, 10.0f + ((320.0f + panelX) - LoadHalfWidth(cells)),
		                             y, 0.0f, 21.0f, 21.0f);
#else
		if (gGame.IsJapanese()) {
			DrawLoadChoice(title, 0x65, 281.25f, y);
			DrawLoadChoice(title, 0x66, 358.75f, y);
		} else {
			DrawLoadChoice(title, 0x65, 283.25f, y - 1.0f);
			DrawLoadChoice(title, 0x66, 360.75f, y - 1.0f);
		}
#endif
		break;
	}
	}
	if (title->mOptionAlpha > 0.0f) {
#if defined(VERSION_JP)
		sprintf(text, "%s", LoadPromptText(0, 0x36));
		{
			u32 cells = strlen(text) / 2;
			DrawGsProportionalTextFormat(&gGame.mGs, LoadTextColor(title->mOptionAlpha), text, (320.0f + panelX) - LoadHalfWidth(cells),
			                             LoadPromptY(title, panelY), 0.0f, 21.0f, 21.0f);
		}
#else
		{
			u32 color = LoadTextColor(title->mOptionAlpha);
			DrawLoadText(GetLocalizedInterfaceText(0x61), 320.0f, LoadPromptY(title, panelY), color);
		}
#endif
	}
}

/**
 * @note Address: 0x80029760
 * @note Size: 0x69C
 */
// NONMATCHING: float regalloc
// draws the options: a window, its title and the three lines, the picked one lit once the
// window is open. the back line bobs while the cursor plays its anim
extern "C" void DrawTitleOptions(Title* title)
{
	f32 scale = title->mOptionAlpha;
	if (scale > 0.0f) {
		f32 alpha  = 255.0f * scale;
		f32 h      = 224.0f * scale;
		s32 a      = alpha;
		u32 window = (u32)alpha | 0xFFFFFF00;
		f32 top;
		DrawTexturedWindowFrame((u32)(160.0f * scale), window, 320.0f, top = 224.0f - h / 2.0f, 320.0f * scale, h, 1.0f, 1.0f, 1.0f);

		u32 lit    = (u8)a | 0xFFFFFF00;
		u32 normal = (u8)alpha | 0xFFFFFF00;
		const char* text;
#if !defined(VERSION_PAL)
#if defined(VERSION_JP)
		text = GetTitleAndSaveMenuText(0x35);
#else
		GetLocalizedInterfaceText(0x60);
#endif
		f32 titleSize = 24.0f * title->mOptionAlpha;
#if !defined(VERSION_JP)
		text = GetLocalizedInterfaceText(0x60);
#endif
		DrawGsProportionalTextFormat(&gGame.mGs, normal, text, 320.0f - MeasureGameTextWidth(&gGame.mGs, text, 1, titleSize) / 2.0f,
		                             titleSize + top, 0.0f, titleSize, titleSize);
#endif
		f32 size = 24.0f * title->mOptionAlpha;
#if defined(VERSION_PAL)
		top = (224.0f - (4.0f * (size * 0.5f) * 0.5f - size * 0.25f)) - h * 0.5f;
#endif
		u32 color = normal;
		if (IsLineLit(title, 0)) {
			color = lit;
		}
#if defined(VERSION_JP)
		text     = GetLocalizedInterfaceText(0x49);
		f32 half = size * 0.5f;
		f32 left = 320.0f - 9.0f * half;
		f32 y    = size + (4.0f * half + top);
		DrawGsProportionalTextFormat(&gGame.mGs, color, text, size + left, y, 0.0f, size, size);
		if (title->mRumble) {
			text = GetLocalizedInterfaceText(0x4a);
			DrawGsProportionalTextFormat(&gGame.mGs, normal, text, size + (5.0f * size + left), y, 0.0f, size, size);
		} else {
			text = GetLocalizedInterfaceText(0x4b);
			DrawGsProportionalTextFormat(&gGame.mGs, normal, text, half + (5.0f * size + left), y, 0.0f, size, size);
		}
		color = IsLineLit(title, 1) ? lit : normal;
		text  = GetLocalizedInterfaceText(0x4c);
		y     = size + (4.0f * (2.0f * half) + top);
		DrawGsProportionalTextFormat(&gGame.mGs, color, text, left, y, 0.0f, size, size);
		text = title->mMono ? GetLocalizedInterfaceText(0x4d) : GetLocalizedInterfaceText(0x4e);
		DrawGsProportionalTextFormat(&gGame.mGs, normal, text, 5.0f * size + left, y, 0.0f, size, size);
#else
		text  = GetLocalizedInterfaceText(0x10);
		f32 x = 320.0f - MeasureGameTextWidth(&gGame.mGs, text, 1, size) / 2.0f - 8.0f;
		f32 y;
		DrawGsProportionalTextFormat(&gGame.mGs, color, text, x - MeasureGameTextWidth(&gGame.mGs, text, 1, size) / 2.0f,
		                             y = size + (4.0f * (size / 2.0f) + top), 0.0f, size, size);
		text = title->mRumble ? GetLocalizedInterfaceText(0x11) : GetLocalizedInterfaceText(0x12);
		x    = 8.0f + (MeasureGameTextWidth(&gGame.mGs, text, 1, size) / 2.0f + 320.0f);
		DrawGsProportionalTextFormat(&gGame.mGs, normal, text, x - MeasureGameTextWidth(&gGame.mGs, text, 1, size) / 2.0f, y, 0.0f, size,
		                             size);

		color = normal;
		if (IsLineLit(title, 1)) {
			color = lit;
		}
		text = GetLocalizedInterfaceText(0x13);
		x    = 320.0f - MeasureGameTextWidth(&gGame.mGs, text, 1, size) / 2.0f - 8.0f;
		DrawGsProportionalTextFormat(&gGame.mGs, color, text, x - MeasureGameTextWidth(&gGame.mGs, text, 1, size) / 2.0f,
		                             y = size + (4.0f * (2.0f * (size / 2.0f)) + top), 0.0f, size, size);
		text = title->mMono ? GetLocalizedInterfaceText(0x14) : GetLocalizedInterfaceText(0x15);
		x    = 8.0f + (MeasureGameTextWidth(&gGame.mGs, text, 1, size) / 2.0f + 320.0f);
		DrawGsProportionalTextFormat(&gGame.mGs, normal, text, x - MeasureGameTextWidth(&gGame.mGs, text, 1, size) / 2.0f, y, 0.0f, size,
		                             size);
#endif

		f32 t        = title->mCursor.mAnimId == 1 ? GetAnimProgress(title) : 0.0f;
		f32 bob      = title->mOptionLine == 2 ? 2.0f * (f32)sin(4.712389f * t) : 0.0f;
		f32 backSize = 24.0f + bob;
		if (IsLineLit(title, 2)) {
			normal = lit;
		}
#if defined(VERSION_JP)
		text = GetLocalizedInterfaceText(0x4f);
		DrawGsProportionalTextFormat(&gGame.mGs, normal, text, 320.0f - 3.0f * half,
		                             (backSize - size) / 2.0f + ((2.0f * size + (4.0f * (3.0f * half) + top)) - backSize), 0.0f, size,
		                             backSize);
#else
		text = GetLocalizedInterfaceText(0x16);
		DrawGsProportionalTextFormat(&gGame.mGs, normal, text, 320.0f - MeasureGameTextWidth(&gGame.mGs, text, 1, size) / 2.0f,
		                             (backSize - size) / 2.0f + ((2.0f * size + (4.0f * (3.0f * (size / 2.0f)) + top)) - backSize), 0.0f,
		                             size, size);
#endif
	}
}

static inline void BeginTitleOverlay()
{
	CGs& gs = gGame.mGs;
	SetGsChannelEnabled(&gs, 4, 0);
	SetGsColorUpdate(gs.mCurRenderMode, 1);
	SetGsAlphaUpdate(gs.mCurRenderMode, 0);
	SetGsAlphaCompare(gs.mCurRenderMode, 4, 0, 0, 3, 255);
	SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gs.mCurRenderMode, 0, 7, 0);
	if (gs.mCurRenderMode)
		ApplyGsRenderMode(gs.mCurRenderMode, FALSE);
	if (gs.mCurTevStage)
		ApplyGsTevStage(gs.mCurTevStage, FALSE);
	if (gs.mCurChannel)
		ApplyGsChannel(gs.mCurChannel, FALSE);
}

static inline void DrawTitleCursors(Title* title, f32 halfWidth, f32 y)
{
#if defined(VERSION_JP)
	const f32 gap = 24.0f;
#else
	const f32 gap = 30.0f;
#endif
	title->mCursor.SetPos((320.0f - halfWidth) - gap, y, 10.0f, 0);
	title->mCursor.SetRotationY(0.2617994f, 0);
	title->mCursor.DrawObject();
	title->mCursor.SetPos(gap + (320.0f + halfWidth), y, 10.0f, 0);
	title->mCursor.SetRotationY(-0.2617994f, 0);
	title->mCursor.DrawObject();
}

static inline f32 DrawTitleEntry(Title* title, s32 entry, s32 textId, f32 y, f32 progress, u32 normal, u32 lit)
{
	const char* text = GetTitleAndSaveMenuText(textId);
	f32 size         = 24.0f + (title->mSelection == entry ? 2.0f * (f32)sin(4.712389f * progress) : 0.0f);
	f32 width        = MeasureGameTextWidth(&gGame.mGs, text, 1, 24.0f);
	if (entry == 1) {
		bool selected = false;
		if (title->mSelection == entry && (f32)title->mHighlightDelay >= FRAME_RATE)
			selected = true;
		u32 tint = normal;
		if (selected)
			tint = lit;
		if (title->mSaveFound)
			DrawGsProportionalTextFormat(&gGame.mGs, tint, text, 320.0f - width * .5f, (size - 24.0f) * .5f + (y - size), 0.0f, 24.0f,
			                             size);
	} else {
		f32 x = 320.0f - width * .5f, top = (size - 24.0f) * .5f + (y - size);
		bool selected = false;
		if (title->mSelection == entry && (f32)title->mHighlightDelay >= FRAME_RATE)
			selected = true;
		DrawGsProportionalTextFormat(&gGame.mGs, selected ? lit : normal, text, x, top, 0.0f, 24.0f, size);
	}
	return width;
}

#if !defined(VERSION_JP)
static inline f32 TitleOptionWidth(s32 first)
{
	f32 a       = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(first + 1), 1, 24.0f);
	f32 b       = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(first + 2), 1, 24.0f);
	f32 c       = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(first), 1, 24.0f);
	f32 largest = a > b ? a : b;
	return 2.0f * ((c > largest ? c : largest) + 8.0f);
}
#endif

extern "C" void DrawTitleScreen(Title* title)
{
#if defined(VERSION_JP)
	enum { dustTexture = 5, footerTexture = 2 };
#else
	enum { dustTexture = 2, footerTexture = 1 };
#endif
	bool active = false;
	if (title->mState >= 1 && title->mState < 23)
		active = true;
	if (!active)
		return;
	CGame& game       = gGame;
	CGs& gs           = game.mGs;
	gGame.mGs.mCamera = game.GetScreenCamera();
	if (BeginGsCamera(&game)) {
		if (title->mMenuAlpha > 0.0f) {
			BeginTitleOverlay();

			for (s32 i = 0; i < 16; ++i) {
				f32 alpha = title->mMenuAlpha;
				SetGsTevMode(gs.mCurTevStage, 0);
				TitleDust& dust = title->mDust[i];

				f32 w     = dust.mSpeedX * GXGetTexObjWidth(&title->mTex[dustTexture]);
				u16 h     = GXGetTexObjHeight(&title->mTex[dustTexture]);
				u32 color = 0xffffff00 | (u32)(255.0f * dust.mSpeedY * alpha);
				DrawGsTexturedQuad(&gGame.mGs, &title->mTex[dustTexture], color, color, color, color, dust.mX, dust.mY, 0.0f, w,
				                   dust.mSpeedX * h, 0, 0, 0);
				SetGsTevMode(gs.mCurTevStage, 4);
			}
			SetGsTevMode(gs.mCurTevStage, 0);
			u16 w     = GXGetTexObjWidth(&title->mTex[gTitleLogoTextures[gTitleLogoIndex]]);
			u16 h     = GXGetTexObjHeight(&title->mTex[gTitleLogoTextures[gTitleLogoIndex]]);
			u32 color = 0xffffff00 | (u32)(255.0f * title->mMenuAlpha);
			DrawGsTexturedQuad(&gGame.mGs, &title->mTex[gTitleLogoTextures[gTitleLogoIndex]], color, color, color, color, 320, 130, 0, w, h,
			                   0, 0, 0);
			SetGsTevMode(gs.mCurTevStage, 4);
		}
		EndGsCamera(&gGame.mGs);
	}
	if (title->mDemo.mState && (title->mDemo.mStatus.mFlags & 1)) {
		HSD_StateInvalidate(-1);
		if (gs.mCurRenderMode)
			gs.mCurRenderMode->Invalidate();
		if (gs.mCurTevStage)
			gs.mCurTevStage->Invalidate();
		if (gs.mCurChannel && !gs.mCurChannel->mDirty)
			gs.mCurChannel->mDirty = TRUE;
		title->mDemo.DrawObject();
	}
	gGame.mGs.mCamera = game.GetScreenCamera();
	if (!BeginGsCamera(&game))
		return;
	if (title->mMenuAlpha > 0.0f) {
		BeginTitleOverlay();
		SetGsTevMode(gs.mCurTevStage, 0);
		CTexObj& tex = title->mTex[footerTexture];
		u16 w = GXGetTexObjWidth(&tex), h = GXGetTexObjHeight(&tex);
		u32 color = (u32)(255.0f * title->mMenuAlpha);
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 320.0f, 398.0f, 0, .6f * w, .6f * h, 0, 0, 0);
		SetGsTevMode(gs.mCurTevStage, 4);
	}
	DrawPanelBackground<SavePanel>(title->mLoadAlpha + title->mOptionAlpha);
	DrawTitleHelpText(title);
	DrawTitleLoadScreen(title);
	DrawTitleOptions(title);
	DrawTitleNameBubbles(title);
	if (title->mState == 10 || title->mState == 11 || title->mState == 12) {
		f32 wave            = 127.0f * (f32)sin(DEG2RAD * title->mPulseAngle);
		f32 alpha           = title->mMenuAlpha;
		const GXColor& text = gTitlePalette[gTitlePaletteIndex].text;
		u32 rgb             = ((u32)text.r << 24) | ((u32)text.g << 16) | ((u32)text.b << 8);
		u32 lit             = rgb | (u8)(127.0f + alpha * wave);
		u32 normal          = rgb | (u8)(255.0f * alpha);
		f32 progress        = title->mCursor.mAnimId == 1 ? GetAnimProgress(title) : 0.0f;
#if defined(VERSION_JP)
		enum { first = 0x13, second = 0x14, third = 0x35 };
#else
		enum { first = 0, second = 1, third = 2 };
#endif
		f32 w0 = DrawTitleEntry(title, 0, first, 324.0f, progress, normal, lit);
		f32 w1 = DrawTitleEntry(title, 1, second, 352.0f, progress, normal, lit);
		f32 y  = 380.0f;
		f32 w2 = DrawTitleEntry(title, 2, third, y, progress, normal, lit);
		switch (title->mSelection) {
		case 0:
			y = 324.0f;
#if defined(VERSION_JP)
			w2 = w0;
#endif
			break;
		case 1:
			y = 352.0f;
#if defined(VERSION_JP)
			w2 = w1;
#endif
			break;
		case 2:
			y = 380.0f;
			break;
		}
#if !defined(VERSION_JP)
		f32 widest = w0 > w1 ? w0 : w1;
		w2         = w2 > widest ? w2 : widest;
#endif
		DrawTitleCursors(title, w2 * 0.5f, y);
	}
	if (title->mState == 21) {
		f32 y;
		u32 width;
		switch (title->mOptionLine) {
		case 0:
			y = 208;
#if defined(VERSION_PAL)
			y -= 18.0f;
#endif
#if defined(VERSION_JP)
			width = 192;
#else
			width = TitleOptionWidth(0x10);
#endif
			break;
		case 1:
			y = 256;
#if defined(VERSION_PAL)
			y -= 18.0f;
#endif
#if defined(VERSION_JP)
			width = 216;
#else
			width = TitleOptionWidth(0x13);
#endif
			break;
		case 2:
			y = 304;
#if defined(VERSION_PAL)
			y -= 18.0f;
#endif
#if defined(VERSION_JP)
			width = 72;
#else
			width = MeasureGameTextWidth(&gs, GetLocalizedInterfaceText(0x16), 1, 24.0f);
#endif
			break;
		}

		DrawTitleCursors(title, width / 2, y);
	}
	EndGsCamera(&gGame.mGs);
}

/**
 * @note Address: 0x8002BB7C
 * @note Size: 0x4
 */
void Title::State25(u32)
{
}

/**
 * @note Address: 0x8002BB80
 * @note Size: 0x148
 */
// waits out the delay, then fades the labels and leaves for the game
void Title::State24(u32 frames)
{
	switch (mPhase) {
	case 0:
		mDelay -= frames;
		if (mDelay < 0) {
			mDelay = 0;
			++mPhase;
		}
		break;
	case 1:
		HideName(this);
		mTargetPanelY = -200.0f;
		mOffset       = 0.0f;
		mAlpha        = 0.0f;
		mChoice       = 0;
		mState        = 13;
		SetMode(25);
		XSound* sound = &gGame.mSound;
		PlayGameSound(sound, 0x88, 255);
		StopGameSfx(sound, (s32)0xFFFF0003);
		gGame.mInfoScrn.mSelectedSaveSlot = mFile;
		break;
	}
}

/**
 * @note Address: 0x8002BCC8
 * @note Size: 0x4
 */
void Title::State23(u32)
{
}

/**
 * @note Address: 0x8002BCCC
 * @note Size: 0x20
 */
void Title::State22(u32 frames)
{
	State20(frames);
}

/**
 * @note Address: 0x8002BCEC
 * @note Size: 0x20
 */
void Title::State21(u32 frames)
{
	State20(frames);
}

/**
 * @note Address: 0x8002BD0C
 * @note Size: 0x4AC
 */
// the card check's message, it waits for A or for the card to change
void Title::State20(u32)
{
	CPad pad(0);
	s32 error = mResult;

	if (error == -3) {
		if (mSelection == 1) {
			if (mReady) {
				OnFormattedCardGone(this);
				return;
			}
		} else if (mReady == 1) {
			for (s32 i = 0; i < 3; i++) {
				ResetSaveSlotRecord(&gTitleSaveSlots[i]);
			}
			StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, mChannel, 1);
			SetMode(1);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			return;
		}
	} else if (mSelection == 1) {
		if (!mReady || !mSaveFound) {
			OnFormattedCardGone(this);
			return;
		}
	} else if (!mReady) {
		OnCardRemoved(this);
		return;
	}
	if (pad.mPad->mTrigger & 0x100) {
		if (mSelection == 1) {
			OnFormattedCardGone(this);
			return;
		}
		if (error == -3) {
			OnCardRemoved(this);
		} else if (error == -5) {
			SetMode(19);
			mShowMessage = 1;
		} else {
			SetMode(5);
			mOffset = -32.0f;
			mChoice = 1;
		}
		PlayGameSound(&gGame.mSound, 0x88, 0xFF);
	}
}

static inline void HandleUnavailableCard(Title* title)
{
	CPad pad(0);
	if (title->mReady == 0) {
		OnCardRemoved(title);
#if !defined(VERSION_JP)
		title->mFormatCancelled = 0;
#endif
		return;
	}
	if (pad.mPad->mTrigger & 0x100) {
		title->SetMode(5);
		title->mOffset = -32.0f;
		title->mChoice = 1;
		PlayGameSound(&gGame.mSound, 0x88, 255);
#if !defined(VERSION_JP)
		title->mFormatCancelled = 0;
#endif
	}
}

/**
 * @note Address: 0x8002C1B8
 * @note Size: 0x118
 */
void Title::State19(u32)
{
	HandleUnavailableCard(this);
}

/**
 * @note Address: 0x8002C2D0
 * @note Size: 0x118
 */
void Title::State18(u32)
{
	HandleUnavailableCard(this);
}

/**
 * @note Address: 0x8002C3E8
 * @note Size: 0x4
 */
void Title::State17(u32)
{
}

/**
 * @note Address: 0x8002C3EC
 * @note Size: 0x4
 */
void Title::State16(u32)
{
}

/**
 * @note Address: 0x8002C3F0
 * @note Size: 0x348
 */
// NONMATCHING: regalloc
// waits for the card to read the file picked, then shows what is in it
void Title::State15(u32 frames)
{
	Memcard* card = &gGame.mMemcard.mCard;
	s32 step      = card->mState;
	SavePreviewRecord* file;
	s32 result;

	mDelay -= frames;
	if (mDelay < 0) {
		mDelay = 0;
	}
	switch (step) {
	case 0:
		mFreeBlocks  = gGame.mMemcard.mCard.mFreeBlocks;
		mFreeEntries = gGame.mMemcard.mCard.mFreeEntries;
		file         = &card->mSaves[0][mFile];
		if (file->mState != 0) {
			gTitleSaveSlots[mFile].mUsed       = 1;
			gTitleSaveSlots[mFile].mLevel      = file->mHeader.mLevel;
			gTitleSaveSlots[mFile].mBattery    = file->mHeader.mBattery;
			gTitleSaveSlots[mFile].mMaxBattery = file->mHeader.mMaxBattery;
			gTitleSaveSlots[mFile].mMoney      = file->mHeader.mMoney;
			gTitleSaveSlots[mFile].mExp        = file->mHeader.mExp;
			gTitleSaveSlots[mFile].mScrap      = file->mHeader.mScrap;
			gTitleSaveSlots[mFile].mRank       = file->mHeader.mRank;
			gTitleSaveSlots[mFile].mHours      = file->mHeader.mFrames / 216000;
			gTitleSaveSlots[mFile].mMinutes    = (file->mHeader.mFrames / 3600) % 60;
			gTitleSaveSlots[mFile].mStage      = file->mHeader.mStage;
			gTitleSaveSlots[mFile].mTimeMode   = file->mHeader.mTimeMode;
			gTitleSaveSlots[mFile].mState      = 0;
			if (file->mHeader.mActive > 0) {
				gTitleSaveSlots[mFile].mActive = 1;
			} else {
				gTitleSaveSlots[mFile].mActive = 0;
			}
		}
		SetMode(24);
		break;
	case 4:
		break;
	case 1:
	case 2:
	case 3:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		result  = gGame.mMemcard.mCard.mCard.mResult;
		mResult = result;
		switch (result) {
		case -128:
		case -14:
		case -13:
		case -12:
		case -11:
		case -10:
		case -9:
		case -8:
		case -7:
		case -6:
		case -5:
		case -4:
		case -3:
		case -2:
		case -1:
		case 0:
			if (step == 2 && (result == -6 || result == -13)) {
				SetMode(11);
				mChoice      = 1;
				mShowMessage = 1;
			} else if (result == -3 || result == -5) {
				SetMode(20);
				mShowMessage = 1;
			} else {
				SetMode(14);
				mOffset = -32.0f;
			}
			mAlpha = 1.0f;
			break;
		default:
			SetMode(14);
			mAlpha  = 1.0f;
			mOffset = -32.0f;
			break;
		}
		break;
	}
}

/**
 * @note Address: 0x8002C738
 * @note Size: 0x408
 */
// shows what is wrong with the card until A is pressed or the card changes
void Title::State14(u32)
{
	CPad pad(0);
	s32 error = mResult;

	switch (error) {
	case -3:
		if (mWrongDevice == 1) {
			for (s32 i = 0; i < 3; i++) {
				ResetSaveSlotRecord(&gTitleSaveSlots[i]);
			}
			StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, mChannel, 1);
			SetMode(1);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			mOffset = 0.0f;
			return;
		}
		if (mReady == 1) {
			for (s32 i = 0; i < 3; i++) {
				ResetSaveSlotRecord(&gTitleSaveSlots[i]);
			}
			StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, mChannel, 1);
			SetMode(1);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			mOffset = 0.0f;
			return;
		}
		break;
	case -2:
		if (!mWrongDevice) {
			OnCardRemoved(this);
			return;
		}
		break;
	case -128:
		if ((!mReady || !mSaveFound) && mSelection == 1) {
			OnFormattedCardGone(this);
			return;
		}
		break;
	default:
		if (!mReady) {
			OnCardRemoved(this);
			return;
		}
		break;
	}
	if (pad.mPad->mTrigger & 0x100) {
		switch (error) {
		case -9:
			SetMode(18);
			mOffset = 0.0f;
			break;
		case -128:
			SetMode(6);
			mOffset = -32.0f;
			break;
		default:
			SetMode(5);
			mOffset = -32.0f;
			mChoice = 1;
			break;
		}
		PlayGameSound(&gGame.mSound, 0x88, 0xFF);
	}
}

/**
 * @note Address: 0x8002CB40
 * @note Size: 0x1B4
 */
void Title::State13(u32 frames)
{
	CPad pad(0);

	if (!mReady) {
		OnCardRemoved(this);
		return;
	}
	switch (mPhase) {
	case 0:
		mDelay -= frames;
		if (mDelay < 0) {
			mDelay = 0;
			mPhase++;
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
		}
		break;
	case 1:
		if (pad.mPad->mTrigger & 0x100) {
			for (s32 i = 0; i < 3; i++) {
				ResetSaveSlotRecord(&gTitleSaveSlots[i]);
			}
			StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, mChannel, 1);
			SetMode(1);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			return;
		}
		break;
	}
}

/**
 * @note Address: 0x8002CCF4
 * @note Size: 0x14C
 */
void Title::State12(u32 frames)
{
	mDelay -= frames;
	if (mDelay < 0) {
		mDelay = 0;
	}
	switch (gGame.mMemcard.mCard.mState) {
	case 0:
		mFreeBlocks  = gGame.mMemcard.mCard.mFreeBlocks;
		mFreeEntries = gGame.mMemcard.mCard.mFreeEntries;
		SetMode(13);
		mAlpha = 1.0f;
		break;
	case 1:
	case 2:
	case 3:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15: {
		s32 result = gGame.mMemcard.mCard.mCard.mResult;
		mResult    = result;
		switch (result) {
		case -128:
		case -14:
		case -13:
		case -12:
		case -11:
		case -10:
		case -9:
		case -8:
		case -7:
		case -6:
		case -5:
		case -4:
		case -3:
		case -2:
		case -1:
		case 0:
			if (result == -6 || result == -13) {
				SetMode(19);
				mShowMessage = 1;
			} else {
				SetMode(21);
				mShowMessage = 1;
			}
			mAlpha = 1.0f;
			break;
		default:
			SetMode(14);
			mAlpha  = 1.0f;
			mOffset = -32.0f;
			break;
		}
		break;
	}
	}
}

/**
 * @note Address: 0x8002CE40
 * @note Size: 0x348
 */
// asked twice whether to make a new save file
void Title::State11(u32)
{
	CPad pad(0);

	if (!mReady) {
		OnCardRemoved(this);
		return;
	}
	switch (mPhase) {
	case 0:
		MoveYesNo(this, pad);
		if (pad.mPad->mTrigger & 0x100) {
			if (mChoice == 0) {
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				mChoice = 1;
				mPhase++;
			} else {
				SetMode(19);
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
#if !defined(VERSION_JP)
				mFormatCancelled = 1;
#endif
			}
			return;
		}
		break;
	case 1:
		MoveYesNo(this, pad);
		if (pad.mPad->mTrigger & 0x100) {
			if (mChoice == 0) {
				StartMemoryCardFormatCheck(&gGame.mMemcard.mCard, mChannel, 1);
				SetMode(12);
				mDelay = 1.5f * FRAME_RATE;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			} else {
				SetMode(19);
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
#if !defined(VERSION_JP)
				mFormatCancelled = 1;
#endif
			}
			return;
		}
		break;
	}
}

/**
 * @note Address: 0x8002D188
 * @note Size: 0x4
 */
void Title::State10(u32)
{
}

/**
 * @note Address: 0x8002D18C
 * @note Size: 0x148
 */
void Title::State9(u32)
{
	CPad pad(0);
	if (mWrongDevice == 1 || mReady == 1) {
		for (s32 i = 0; i < 3; ++i) {
			ResetSaveSlotRecord(&gTitleSaveSlots[i]);
		}
		StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, mChannel, TRUE);
		SetMode(1);
		PlayGameSound(&gGame.mSound, 0x88, 255);
		mOffset = 0.0f;
		return;
	}
	if (pad.mPad->mTrigger & 0x100) {
		SetMode(5);
		mOffset = -32.0f;
		mChoice = 1;
		PlayGameSound(&gGame.mSound, 0x88, 255);
	}
}

/**
 * @note Address: 0x8002D2D4
 * @note Size: 0x218
 */
void Title::State8(u32)
{
	s32 cardState = gGame.mMemcard.mCard.mState;
	switch (cardState) {
	case 0:
		HideName(this);
		StopGameSfx(&gGame.mSound, (s32)0xFFFF0003);
		mTargetPanelY                     = -200.0f;
		mAlpha                            = 0.0f;
		gGame.mInfoScrn.mSelectedSaveSlot = mFile;
		ResumeScrapCounterPulse(&gGame.mInfoScrn);
		SetMode(25);
		mState   = 13;
		mTimeout = 20000;
		break;
	case 1:
	case 2:
	case 3:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15: {
		s32 result = gGame.mMemcard.mCard.mCard.mResult;
		mResult    = result;
		switch (result) {
		case -128:
		case -14:
		case -13:
		case -12:
		case -11:
		case -10:
		case -9:
		case -8:
		case -7:
		case -6:
		case -5:
		case -4:
		case -3:
		case -2:
		case -1:
		case 0:
			if (cardState == 2 && (result == -6 || result == -13)) {
				SetMode(11);
				mChoice      = 1;
				mShowMessage = 1;
			} else if (result == -3 || result == -5) {
				SetMode(22);
				mShowMessage = 1;
			} else {
				SetMode(14);
				mOffset = -32.0f;
			}
			mAlpha = 1.0f;
			break;
		default:
			SetMode(14);
			mAlpha  = 1.0f;
			mOffset = -32.0f;
			break;
		}
		break;
	}
	}
}

/**
 * @note Address: 0x8002D4EC
 * @note Size: 0x4
 */
void Title::State7(u32)
{
}

/**
 * @note Address: 0x8002D4F0
 * @note Size: 0x304
 */
// asked whether to save over the file, unless the card went away under it
void Title::State6(u32)
{
	CPad pad(0);

	if ((!mReady || !mSaveFound) && mSelection == 1) {
		SetMode(25);
		mState = 11;
		PlayGameSound(&gGame.mSound, 0x88, 0xFF);
		mTargetPanelY = -200.0f;
		mOffset       = 0.0f;
		mAlpha        = 0.0f;
		HideName(this);
		mSelection     = 0;
		mSaveFound     = 0;
		mCardScanState = 0;
		return;
	}
	if (!mReady) {
		OnCardRemoved(this);
		return;
	}
	if (pad.mPad->mRepeat & 0x40001) {
		if (mChoice > 0) {
			mChoice--;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			return;
		}
	} else if (pad.mPad->mRepeat & 0x80002) {
		if (mChoice < 1) {
			mChoice++;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			return;
		}
	}
	if (pad.mPad->mTrigger & 0x100) {
		switch (mChoice) {
		case 0:
			StartMemoryCardSaveWrite(&gGame.mMemcard.mCard, mChannel, mFile, 1);
			SetMode(15);
			mDelay = 1.5f * FRAME_RATE;
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			mOffset = -32.0f;
			break;
		case 1:
			SetMode(2);
			mChoice = 0;
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			break;
		}
	}
}

/**
 * @note Address: 0x8002D7F4
 * @note Size: 0x504
 */
// asked whether to go on without saving
void Title::State5(u32)
{
	CPad pad(0);

	switch (mResult) {
	case -3:
		if (mReady == 1) {
			for (s32 i = 0; i < 3; i++) {
				ResetSaveSlotRecord(&gTitleSaveSlots[i]);
			}
			StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, mChannel, 1);
			mOffset = 0.0f;
			SetMode(1);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			return;
		}
		break;
	case -2:
		if (!mWrongDevice) {
			OnCardRemoved(this);
			return;
		}
		break;
	default:
		if (!mReady) {
			OnCardRemoved(this);
			return;
		}
		break;
	}
	MoveYesNo(this, pad);
	if (pad.mPad->mTrigger & 0x100) {
		if (mChoice == 0 && (pad.mPad->mTrigger & 0x100)) {
			HideName(this);
			mTargetPanelY = -200.0f;
			mOffset       = 0.0f;
			mAlpha        = 0.0f;
			mChoice       = 0;
			mState        = 13;
			SetMode(25);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			StopGameSfx(&gGame.mSound, (s32)0xFFFF0003);
			gGame.mInfoScrn.mSelectedSaveSlot = mFile;
			return;
		}
		if (mChoice == 1 && (pad.mPad->mTrigger & 0x100)) {
			if (mShowMessage == 1) {
				SetMode(19);
				mOffset = 0.0f;
			} else if (mResult == 0) {
				SetMode(2);
				mChoice = 0;
			} else if (mResult == -3) {
				SetMode(25);
				mState = 11;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				mTargetPanelY = -200.0f;
				mAlpha        = 0.0f;
				mChoice       = 0;
				mOffset       = 0.0f;
				HideName(this);
			} else {
				for (s32 i = 0; i < 3; i++) {
					ResetSaveSlotRecord(&gTitleSaveSlots[i]);
				}
				StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, mChannel, 1);
				SetMode(1);
				mOffset = 0.0f;
			}
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			return;
		}
	}
}

/**
 * @note Address: 0x8002DCF8
 * @note Size: 0x4
 */
void Title::State4(u32)
{
}

/**
 * @note Address: 0x8002DCFC
 * @note Size: 0x240
 */
// the yes or no question, left and right pick, A takes it
void Title::State3(u32)
{
	CPad pad(0);
	CPadOne* state;

	if (!mReady) {
		OnCardRemoved(this);
		return;
	}
	state = pad.mPad;
	if (state->mRepeat & 0x40001) {
		if (mChoice > 0) {
			mChoice--;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			return;
		}
	} else if (state->mRepeat & 0x80002) {
		if (mChoice < 1) {
			mChoice++;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			return;
		}
	}
	if (state->mTrigger & 0x100) {
		switch (mChoice) {
		case 0:
			StartMemoryCardSaveWrite(&gGame.mMemcard.mCard, mChannel, mFile, 1);
			SetMode(15);
			mDelay = 1.5f * FRAME_RATE;
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			mOffset = -32.0f;
			return;
		case 1:
			SetMode(2);
			mChoice = 0;
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			return;
		}
	}
}

/**
 * @note Address: 0x8002DF3C
 * @note Size: 0x638
 */
// picks one of the three save files, A starts or loads it, B goes back
void Title::State2(u32)
{
	CPad pad(0);

	if ((!mReady || !mSaveFound) && mSelection == 1) {
		OnFormattedCardGone(this);
		return;
	}
	if (!mReady) {
		OnCardRemoved(this);
		return;
	}
	mOffset = 0.0f;
	if (pad.mPad->mRepeat & 0x40001) {
		mFile--;
		if (mFile < 0) {
			mFile = 2;
		}
		PlayGameSound(&gGame.mSound, 0x87, 0xFF);
	} else if (pad.mPad->mRepeat & 0x80002) {
		mFile++;
		if (mFile > 2) {
			mFile = 0;
		}
		PlayGameSound(&gGame.mSound, 0x87, 0xFF);
	}
	if (pad.mPad->mTrigger & 0x100) {
		switch (mSelection) {
		case 0:
			if (gTitleSaveSlots[mFile].mUsed == 1) {
				SetMode(3);
				mChoice = 1;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			} else if (mFreeBlocks >= 6 && mFreeEntries > 0) {
				StartMemoryCardSaveWrite(&gGame.mMemcard.mCard, mChannel, mFile, 1);
				SetMode(15);
				mDelay  = 1.5f * FRAME_RATE;
				mOffset = -32.0f;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			} else {
				mResult = -9;
				SetMode(14);
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				mOffset = -32.0f;
			}
			return;
		case 1:
			if (gTitleSaveSlots[mFile].mUsed == 1) {
				if (!IsSaveSlotIntact(&gTitleSaveSlots[mFile])) {
					mResult = -128;
					SetMode(14);
					PlayGameSound(&gGame.mSound, 0x88, 0xFF);
					mOffset = 0.0f;
					return;
				}
				if (gTitleSaveSlots[mFile].mStage == 0) {
					HideName(this);
					mTargetPanelY = -200.0f;
					mAlpha        = 0.0f;
					mChoice       = 0;
					mState        = 13;
					SetMode(25);
					PlayGameSound(&gGame.mSound, 0x88, 0xFF);
					StopGameSfx(&gGame.mSound, (s32)0xFFFF0003);
					gGame.mInfoScrn.mSelectedSaveSlot = mFile;
					return;
				}
				StartMemoryCardSaveRead(&gGame.mMemcard.mCard, mChannel, mFile, 1);
				SetMode(8);
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			} else {
				PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			}
			return;
		}
	} else if (pad.mPad->mTrigger & 0x200) {
		SetMode(25);
		mState = 11;
		PlayGameSound(&gGame.mSound, 0x88, 0xFF);
		mTargetPanelY = -200.0f;
		mOffset       = 0.0f;
		mAlpha        = 0.0f;
		HideName(this);
	}
}

/**
 * @note Address: 0x8002E574
 * @note Size: 0x358
 */
void Title::State1(u32 frames)
{
	Memcard* card = &gGame.mMemcard.mCard;
	s32 state     = card->mState;
	mDelay -= frames;
	if (mDelay < 0)
		mDelay = 0;
	switch (state) {
	case 0: {
		OSTime timestamps[3];
		for (s32 i = 0; i < 3; ++i) {
			SavePreviewRecord* save = &card->mSaves[0][i];
			if (save->mState) {
				OSCalendarTime calendar;
				OSTicksToCalendarTime(OSSecondsToTicks((OSTime)save->mStat.time), &calendar);
				s32 frames                     = save->mHeader.mFrames;
				s32 active                     = save->mHeader.mActive;
				gTitleSaveSlots[i].mUsed       = 1;
				gTitleSaveSlots[i].mLevel      = save->mHeader.mLevel;
				gTitleSaveSlots[i].mBattery    = save->mHeader.mBattery;
				gTitleSaveSlots[i].mMaxBattery = save->mHeader.mMaxBattery;
				gTitleSaveSlots[i].mMoney      = save->mHeader.mMoney;
				gTitleSaveSlots[i].mExp        = save->mHeader.mExp;
				gTitleSaveSlots[i].mScrap      = save->mHeader.mScrap;
				gTitleSaveSlots[i].mRank       = save->mHeader.mRank;
				gTitleSaveSlots[i].mHours      = frames / 216000;
				gTitleSaveSlots[i].mMinutes    = (frames / 3600) % 60;
				gTitleSaveSlots[i].mStage      = save->mHeader.mStage;
				gTitleSaveSlots[i].mTimeMode   = save->mHeader.mTimeMode;
				if (active > 0)
					gTitleSaveSlots[i].mActive = 1;
				else
					gTitleSaveSlots[i].mActive = 0;
				timestamps[i] = OSCalendarTimeToTicks(&calendar);
				if (save->mStatus != 0 || !IsValidSavePreview(save))
					gTitleSaveSlots[i].mState = 1;
				else
					gTitleSaveSlots[i].mState = 0;
			}
		}
		mFreeBlocks  = gGame.mMemcard.mCard.mFreeBlocks;
		mFreeEntries = gGame.mMemcard.mCard.mFreeEntries;
		SetMode(2);
		if (mSelection == 0)
			mFile = 0;
		else
			mFile = IndexOfLargestOfThree(timestamps[0], timestamps[1], timestamps[2]);
		mAlpha = 1.0f;
		break;
	}
	case 1:
	case 2:
	case 3:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15: {
		s32 result = gGame.mMemcard.mCard.mCard.mResult;
		mResult    = result;
		switch (result) {
		case -128:
		case -14:
		case -13:
		case -12:
		case -11:
		case -10:
		case -9:
		case -8:
		case -7:
		case -6:
		case -5:
		case -4:
		case -3:
		case -2:
		case -1:
		case 0:
			if (result == -6 || result == -13) {
				SetMode(11);
				mChoice = 1;
			} else if (result == -5) {
				SetMode(19);
				mShowMessage = 1;
			} else {
				SetMode(14);
				mOffset = -32.0f;
			}
			mAlpha = 1.0f;
			break;
		default:
			SetMode(14);
			mAlpha  = 1.0f;
			mOffset = -32.0f;
			break;
		}
		break;
	}
	}
}

/**
 * @note Address: 0x8002E8CC
 * @note Size: 0x4
 */
void Title::State0(u32)
{
}

static inline void ApproachTitleOffset(f32& value, f32& target)
{
	f32 speed = (target - value) * .25f;
	if (speed < 0)
		speed = -speed;
	if (speed <= 1)
		speed = 1;
	ApproachHudValue(&value, &target, speed);
}

extern "C" {
#if !defined(VERSION_JP)
extern const char* gTitleTextureSuffixes[5];
#endif
}

static inline void KeepTitleSoundPlaying(XSound* sound)
{
	if (!IsGameSoundPlaying(sound, (s32)0xffff0003))
		PlayGameSound(sound, (s32)0xffff0003, 255);
}

static inline void ShowTitleChoice(Title* title, s32 id)
{
	u32 tint             = 0xffffff00 | (u32)(255.f * title->mLoadAlpha);
	const char* text     = GetTitleAndSaveMenuText(id);
	title->mName.mActive = 1;
	title->mName.mCount  = 0;
	if (text != NULL) {
		title->mName.mColor = tint;
		for (s32 i = 0; i < 20 && *text; i++, text++) {
			title->mName.mChars[i].mText  = text;
			title->mName.mChars[i].mScale = -.2f * i;
			title->mName.mChars[i].mWait  = FRAME_RATE;
			title->mName.mChars[i].mBob   = DEG2RAD * (rand() % 360);
			title->mName.mChars[i].mTilt  = DEG2RAD * (rand() % 360);
			title->mName.mCount++;
			if (gGame.IsJapanese() && IsSjisLead(*text))
				text++;
		}
		for (s32 i = title->mName.mCount; i < 20; i++) {
			title->mName.mChars[i].mText  = NULL;
			title->mName.mChars[i].mScale = 0;
			title->mName.mChars[i].mWait  = 0;
		}
	}
}
static inline void UpdateTitleDust(Title* title, u32 frames)
{
	f32 fade   = .009f * frames;
	f32 growth = .018f * frames;
	for (s32 i = 0; i < (s32)ARRAY_SIZE(title->mDust); ++i) {
		TitleDust& dust = title->mDust[i];
		switch (dust.mState) {
		case 0:
			dust.mTimer = rand() % 120;
			++dust.mState;
			break;
		case 1:
			if ((s32)--dust.mTimer < 0) {
				dust.mSpeedY = 1;
				++dust.mState;
			}
			break;
		case 2:
			dust.mSpeedY -= fade;
			dust.mSpeedX += growth;
			if (dust.mSpeedX >= 2) {
				dust.mX      = rand() % 640;
				dust.mY      = rand() % 480;
				dust.mSpeedY = dust.mSpeedX = 0;
				dust.mState                 = 0;
			}
			break;
		}
	}
}

static inline void ApplyTitleControllerSetting(Title* title)
{
	if (gGame.mStatus.mFlags & 0x4000) {
		if (gGame.mPad.mPad[0].mConnected) {
			title->mRumble = 1;
			gGame.mStatus.mFlags |= 0x800000000ULL;
		} else {
			title->mRumble = 0;
			gGame.mStatus.mFlags &= ~0x800000000ULL;
		}
		gGame.mStatus.mFlags &= ~0x4000ULL;
	}
}

static inline void TitleSelectionMoved(Title* title)
{
	title->mPulseAngle = 45;
	PlayGameSound(&gGame.mSound, 0x87, 255);
}

static inline void TitleSelectionConfirmed(Title* title)
{
	PlayGameSound(&gGame.mSound, 0x88, 255);
	title->mState = 12;
	title->mCursor.SetAnim(1, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
	title->mPulseAngle = 90;
}

/**
 * @note Address: 0x8002E8D0
 */
// NONMATCHING: retail reaches the card's busy and found flags in two steps and turns the result of
// the shift-jis test into 0 or 1 before it tests it
extern "C" void UpdateTitleScreen(Title* title, u32 frames)
{

	ApproachTitleOffset(title->mPanelY, title->mTargetPanelY);
	ApproachHudValue(&title->mLoadAlpha, &title->mAlpha, .08f * frames);
	ApproachTitleOffset(title->mLoadOffset, title->mOffset);
	ApproachHudValue(&title->mOptionAlpha, &title->mTargetOptionAlpha, .08f * frames);
	title->mLoadScroll -= .008f * frames;
	if (title->mState != 0 && title->mState != 1 && title->mState != 23) {
		if (title->mName.mActive && title->mName.mActive > 0) {
			for (s32 i = 0; i < title->mName.mCount; ++i) {
				if (title->mName.mChars[i].mWait > 0) {
					if (title->mName.mChars[i].mScale < 1) {
						if ((title->mName.mChars[i].mScale += (1.f / (.3f * FRAME_RATE)) * frames) > 1)
							title->mName.mChars[i].mScale = 1;
					}
				} else if (title->mName.mChars[i].mScale > 0) {
					if ((title->mName.mChars[i].mScale -= (1.f / (.3f * FRAME_RATE)) * frames) < 0)
						title->mName.mChars[i].mScale = 0;
				}
			}
			s32 visible = 0;
			for (s32 i = 0; i < title->mName.mCount; ++i) {
				if (0 != title->mName.mChars[i].mScale) {
					++visible;
					title->mName.mChars[i].mBob += (5.f * DEG2RAD) * frames;
					title->mName.mChars[i].mTilt += (3.f * DEG2RAD) * frames;
				}
			}
			if (!visible)
				title->mName.mActive = 0;
		}
		switch (title->mHelpFadeState) {
		case 0:
			break;
		case 1:
			title->mHelpAlpha = 0;
			++title->mHelpFadeState;
			title->mHelpText = title->mNextHelpText;
			break;
		case 2:
			title->mHelpAlpha += .04f * frames;
			if (title->mHelpAlpha >= 1) {
				title->mHelpAlpha     = 1;
				title->mHelpFadeState = 0;
			}
			break;
		}
		if (title->mMenuAlpha > 0) {
			for (s32 i = 0; i < (s32)ARRAY_SIZE(title->mBg); ++i) {
				title->mBg[i].mAngle0 += (5.f * DEG2RAD) * frames;
				title->mBg[i].mAngle1 += (3.f * DEG2RAD) * frames;
			}
		}
		if (title->mMode != 1 && title->mMode != 8 && title->mMode != 12 && title->mMode != 15) {
			Memcard& card = gGame.mMemcard.mCard;
			s32 sector, capacity;
			title->mReady       = 0;
			title->mWrongDevice = 0;
			switch (ProbeMemoryCardCapacity(&card, title->mChannel, &capacity, &sector)) {
			case CARD_RESULT_WRONGDEVICE:
				title->mWrongDevice = 1;
				break;
			case CARD_RESULT_READY:
				if (sector == 8192)
					title->mReady = 1;
				break;
			case CARD_RESULT_NOCARD:
			case CARD_RESULT_FATAL_ERROR:
				break;
			}
			if (!title->mReady)
				title->mShowMessage = 0;
			if (!title->mReady) {
				if (title->mState != 20 && title->mSelection == 1)
					title->mSelection = 0;
				title->mSaveFound     = 0;
				title->mCardScanState = 0;
#if !defined(VERSION_JP)
				title->mFormatCancelled = 0;
#endif
			} else {
				switch (title->mCardScanState) {
				case 0:
					StartMemoryCardScan(&card, title->mChannel, FALSE);
					title->mCardScanState = 1;
					break;
				case 1:
					if (!title->mSaveFound && card.mFound)
						title->mSaveFound = 1;
					break;
				}
			}
		}
		bool running = title->mState >= 1 && title->mState < 23;
		if (!running)
			return;
		if (gGame.mMemcard.mCard.mBusy)
			return;
	}
	switch (title->mState) {
	case 1:
		title->mState = 2;
		StartMemoryCardScan(&gGame.mMemcard.mCard, title->mChannel, FALSE);
		StartScreenFade(&gGame.mFade, 0, 0, 0, 255, 0, 0, 0, 255, 0);
		break;
	case 2:
		if (gGame.mMemcard.mCard.mFound) {
			title->mSelection = 1;
			title->mSaveFound = 1;
			ApplyTitleControllerSetting(title);
		} else {
			title->mSelection     = 0;
			title->mSaveFound     = 0;
			title->mCardScanState = 0;
			ApplyTitleControllerSetting(title);
		}
		title->mState = 3;
		if (gCmdLineArgs.mTitleCursor < 0) {
			title->mSpawnIndex = rand() % 11;
		} else {
			title->mSpawnIndex = gCmdLineArgs.mTitleCursor % 11;
		}
		// fall through: loading starts on the same frame as cursor selection
	case 3:
		if (title->mCursor.LoadFromPlacement(CoordInfo(GetSpawnData(title->mSpawnIndex))))
			++title->mState;
		break;
	case 4:
		if (title->mDemo.LoadFromPlacement(CoordInfo(GetSpawnData(11))))
			++title->mState;
		break;
	case 5: {
#if defined(VERSION_JP)
		const char* path = "tpl/title.tpl";
#else
		char path[64];
		sprintf(path, "tpl/title%s.tpl", gTitleTextureSuffixes[gGame.mLanguage]);
#endif
		if (!title->mTpl.ReadEx(path, 0, 0))
			break;
		PlayGameSound(&gGame.mSound, (s32)0xffff0003, 255);
		title->mState   = 10;
		title->mTimeout = -1;
		title->mTpl.CloseHandle();
		title->mTex[0] = title->mTpl.mTex.Init(&((TPLPalette*)title->mTpl.mBuffer)->descriptorArray[0], 0);
		title->mTex[1] = title->mTpl.mTex.Init(&((TPLPalette*)title->mTpl.mBuffer)->descriptorArray[1], 0);
		title->mTex[2] = title->mTpl.mTex.Init(&((TPLPalette*)title->mTpl.mBuffer)->descriptorArray[2], 0);
#if defined(VERSION_JP)
		title->mTex[3] = title->mTpl.mTex.Init(&((TPLPalette*)title->mTpl.mBuffer)->descriptorArray[3], 0);
		title->mTex[4] = title->mTpl.mTex.Init(&((TPLPalette*)title->mTpl.mBuffer)->descriptorArray[4], 0);
		title->mTex[5] = title->mTpl.mTex.Init(&((TPLPalette*)title->mTpl.mBuffer)->descriptorArray[5], 0);
#endif
		title->mMenuAlpha = title->_14 = 0;
		StartScreenFade(&gGame.mFade, 0, 0, 0, 255, gTitlePalette[gTitlePaletteIndex].background.r,
		                gTitlePalette[gTitlePaletteIndex].background.g, gTitlePalette[gTitlePaletteIndex].background.b, 0, .5f);
#if defined(VERSION_PAL)
		Vec pos = { 0.0f, 4.0f, 0.0f };
#else
		Vec pos = { 0.0f, 8.0f, 0.0f };
#endif
		title->mDemo.mState = 1;
		title->mDemo.mLoop  = TRUE;
		title->mDemo.SetPos(pos, 0);
		title->mDemo.SetAnim(0, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
		// The first animation frame runs immediately after loading.
	}
	case 10: {

		if (title->mMenuAlpha < 1) {
			if ((title->mMenuAlpha += (1.f / (.5f * FRAME_RATE)) * frames) > 1)
				title->mMenuAlpha = 1;
		}
		if (title->_14 < 1) {
			if ((title->_14 += (1.f / FRAME_RATE) * frames) > 1)
				title->_14 = 1;
		}
		title->mCursor.UpdateObject(frames);
		title->mDemo.UpdateObject(frames);
		UpdateTitleDust(title, frames);
		if (title->mMenuAlpha == 1 && title->_14 == 1)
			title->mState = 11;
		break;
	}
	case 11: {
		title->mHighlightDelay += frames;
		if (title->mHighlightDelay >= FRAME_RATE) {
			title->mHighlightDelay = (s32)FRAME_RATE;
			title->mPulseAngle     = (f32)fmod(title->mPulseAngle + (f32)(frames * 9), 360.);
		}
		title->mCursor.UpdateObject(frames);
		title->mDemo.UpdateObject(frames);
		UpdateTitleDust(title, frames);
		KeepTitleSoundPlaying(&gGame.mSound);
		CPad pad(0);
		if (pad.mPad->mRepeat & 0x10008) {
			if (title->mSaveFound) {
				if (title->mSelection > 0) {
					--title->mSelection;
					TitleSelectionMoved(title);
				}
			} else if (title->mSelection != 0) {
				title->mSelection = 0;
				TitleSelectionMoved(title);
			}
		} else if (pad.mPad->mRepeat & 0x20004) {
			if (title->mSaveFound) {
				if (title->mSelection < 2) {
					++title->mSelection;
					TitleSelectionMoved(title);
				}
			} else if (title->mSelection != 2) {
				title->mSelection = 2;
				TitleSelectionMoved(title);
			}
		}
#if !defined(VERSION_PAL)
		else
#endif
		    if (pad.mPad->mTrigger & 0x100) {
			switch (title->mSelection) {
			case 0:
			case 1:
				for (s32 i = 0; i < 3; ++i)
					ResetSaveSlotRecord(&gTitleSaveSlots[i]);
				StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, title->mChannel, FALSE);
				title->SetMode(1);
				TitleSelectionConfirmed(title);
				break;
			case 2:
				TitleSelectionConfirmed(title);
				break;
			}
		}
		break;
	}
	case 12: {
		title->mCursor.UpdateObject(frames);
		title->mDemo.UpdateObject(frames);
		UpdateTitleDust(title, frames);
		HSD_AObj* aobj = GetAObj(title->mCursor.mJObj);
		if ((aobj->flags & 0x44000000) || title->mCursor.mAnimId == 0 || title->mCursor.mArchive.mNum[0] < 2) {
			title->mCursor.SetAnim(0, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
			title->mPulseAngle = 0;
			switch (title->mSelection) {
			case 0:
				title->mTargetPanelY = 0;
				title->mAlpha        = 1;
				title->mState        = 20;
#if defined(VERSION_JP)
				ShowTitleChoice(title, 19);
#else
				ShowTitleChoice(title, 0);
#endif
				break;
			case 1:
				title->mTargetPanelY = 0;
				title->mAlpha        = 1;
				title->mState        = 20;
				PlayGameSound(&gGame.mSound, 0x88, 255);
#if defined(VERSION_JP)
				ShowTitleChoice(title, 20);
#else
				ShowTitleChoice(title, 1);
#endif
				break;
			case 2:
				title->mTargetPanelY      = 0;
				title->mTargetOptionAlpha = 1;
				title->mState             = 21;
				title->mOptionState       = 0;
				title->mOptionLine        = 0;
				PlayGameSound(&gGame.mSound, 0x88, 255);
#if defined(VERSION_JP)
				ShowTitleChoice(title, 53);
#else
				ShowTitleChoice(title, 2);
#endif
				break;
			}
		}
		break;
	}
	case 20:
		title->mDemo.UpdateObject(frames);
		UpdateTitleDust(title, frames);
		KeepTitleSoundPlaying(&gGame.mSound);
		(title->*gTitleModeUpdates[title->mMode])(frames);
		break;
	case 21:
		title->mDemo.UpdateObject(frames);
		UpdateTitleDust(title, frames);
		KeepTitleSoundPlaying(&gGame.mSound);
		UpdateTitleOptions(title, frames);
		break;
	case 13: {
		title->mState = 14;
		StartScreenFade(&gGame.mFade, gTitlePalette[gTitlePaletteIndex].background.r, gTitlePalette[gTitlePaletteIndex].background.g,
		                gTitlePalette[gTitlePaletteIndex].background.b, 0, 0, 0, 0, 255, .5f);
		title->mDemo.mState = 0;
	}
	case 14: {

		if (title->mMenuAlpha > 0) {
			if ((title->mMenuAlpha -= (1.f / (.5f * FRAME_RATE)) * frames) < 0)
				title->mMenuAlpha = 0;
		}
		if (title->_14 > 0) {
			if ((title->_14 -= (1.f / (.5f * FRAME_RATE)) * frames) < 0)
				title->_14 = 0;
		}
		if (title->mMenuAlpha == 0) {
			bool faded = gGame.mFade.mSpeed.r == 0 && gGame.mFade.mSpeed.g == 0 && gGame.mFade.mSpeed.b == 0 && gGame.mFade.mSpeed.a == 0;
			if (faded) {
				GXColor black                   = { 0, 0, 0, 0 };
				title->mState                   = 22;
				HSD_VIData.current.vi.clear_clr = black;
			}
		}
		break;
	}
	case 22:
		title->mDemo.Release();
		title->mCursor.Release();
		title->mTpl.Close();
		if (title->mTimeout >= 0) {
			ChangeGameStage(&gGame, title->mTimeout, TRUE);
			title->mTimeout = -1;
		}
		title->mState = 23;
		break;
	case 23:
		break;
	}
	if (title->mState != 0 && title->mState != 22 && title->mState != 23) {
		GXColor color                   = gTitlePalette[gTitlePaletteIndex].background;
		HSD_VIData.current.vi.clear_clr = color;
	}
}

extern "C" {
s32 HSD_SynthSFXNewBank(s32 size);
s32 HSD_SynthSFXBankClear(s32 bank);
void HSD_JObjSetJSoundCallback(void (*callback)(s32));
extern "C" void PlayGameSfxAtFullVolume(s32 sfx);
void InitAudio(void* owner, u32 heapSize, u32 voices, u32 groups);
BOOL LoadSoundSampleMetadata(void* data);
s32 HSD_SynthSFXLoad(const char* name, s32 bank, void (*callback)(s32, s32), s32 arg);
}

// drops every sfx name waiting to load and empties the stage bank
static inline void ClearStageBank(XSound* sound)
{
	sound->mStageRequests.Clear();
	sound->mLoadRequestCount = 0;
	HSD_SynthSFXBankClear(sound->mStageBank);
}

// queues a name for the stage bank unless it is already there, sound.cpp has the same
static inline void RegisterStageSoundName(XSound* sound, const char* name)
{
	s32 bank = sound->mStageBank;
	if (name == NULL) {
		return;
	}
	for (u32 i = 0; i < 16; ++i) {
		if (sound->mStageRequests.entries[i].mName != NULL && strcmp(sound->mStageRequests.entries[i].mName, name) == 0) {
			return;
		}
	}
	for (u32 i = 0; i < 16; ++i) {
		if (sound->mStageRequests.entries[i].mName == NULL) {
			++sound->mLoadRequestCount;
			sound->mStageRequests.entries[i].mName = name;
			sound->mStageRequests.entries[i].mBank = bank;
#if !defined(VERSION_PAL)
			OSReport("Register:%s\n", name);
#endif
			return;
		}
	}
#if !defined(VERSION_PAL)
	OSReport("Ssm::Load xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
#endif
}

/**
 * @note Address: 0x8001F8D8
 * @note Size: 0xA4
 */
extern "C" void ClearStageSoundBank(XSound* sound)
{
	ClearStageBank(sound);
}

/**
 * @note Address: 0x8001F97C
 * @note Size: 0x2D8
 */
// NONMATCHING: the strings are in sound.cpp's rodata
// a negative stage loads the shared samples a step per call, TRUE once they're in
extern "C" BOOL LoadGameSoundBank(XSound* sound, s32 stage)
{
	if (stage < 0) {
		switch (sound->mLoadState) {
		case 0:
			sound->mSampleReady = 0;
			if (sound->mFile.ReadAsync("sfx_sample.sem", 0, 0, FALSE) >= 0) {
				++sound->mLoadState;
			} else {
#if defined(VERSION_JP)
				OSPanic("sound.cpp", 0xED, "%s%s", "", "Read");
#else
				OSPanic("sound.cpp", 0xF1, "%s%s", "", "Read");
#endif
			}
			break;
		case 1: {
			s32 result = sound->mFile.GetResult();
			if (result == 0) {
				++sound->mLoadState;
			} else if (result < 0) {
#if defined(VERSION_JP)
				OSPanic("sound.cpp", 0xF5, "%s%s", "", "Read");
#else
				OSPanic("sound.cpp", 0xF9, "%s%s", "", "Read");
#endif
			}
			break;
		}
		case 2:
			LoadSoundSampleMetadata(sound->mFile.mBuffer);
			HSD_SynthSFXLoad("sfx_sample.ssm", sound->mSampleBank, 0, 0);
			++sound->mLoadState;
			break;
		case 3:
			if (!HasPendingSoundBankLoad()) {
				sound->mLoadState   = 0;
				sound->mSampleReady = 1;
				return TRUE;
			}
			break;
		default:
			sound->mLoadState = 0;
			break;
		}
		return FALSE;
	}
	sound->mDefaultTrack = 0;
	ClearStageBank(sound);
#if !defined(VERSION_PAL)
	OSReport(">>>%s\n", stageSfxTable[stage]);
#endif
	RegisterStageSoundName(sound, stageSfxTable[stage]);
	return TRUE;
}

/**
 * @note Address: 0x8001FC54
 * @note Size: 0x68
 */
extern "C" void InitGameSound(XSound* sound)
{
	InitAudio(sound, 0x700000, 0x60, 0x20);
	sound->mSampleBank  = HSD_SynthSFXNewBank(0x480000);
	sound->mStageBank   = HSD_SynthSFXNewBank(0x280000);
	sound->mLoadState   = 0;
	sound->mSampleReady = 0;
	HSD_JObjSetJSoundCallback(PlayGameSfxAtFullVolume);
}

/**
 * @note Address: 0x8001FCBC
 * @note Size: 0x38
 */
// particle sounds from joint anims
extern "C" void PlayGameSfxAtFullVolume(s32 sfx)
{
	PlayGameSound(&gGame.mSound, sfx, 255);
}

/**
 * @note Address: 0x80030CE8
 * @note Size: 0x1FC
 */
extern "C" void ResetTitleScreenState(Title* title)
{
	title->mState          = 0;
	title->mMode           = 0;
	title->mTimeout        = -1;
	title->mMenuAlpha      = 0.0f;
	title->_14             = 0.0f;
	title->mPulseAngle     = 90.0f;
	title->mHighlightDelay = 0;
	title->mSpawnIndex     = 0;
	title->mCursor.InitializeObject(0);
	title->mCursor.mAngle      = 0.0f;
	title->mCursor.mAngleSpeed = 0.0f;
	title->mSelection          = 0;
	title->mFile               = 0;
	title->mChoice             = 0;
	title->mLoadAlpha          = 0.0f;
	title->mAlpha              = 0.0f;
	title->mPrevMode           = 0;
	title->mPhase              = 0;
	title->mDelay              = 0;
	title->mPanelX             = -10.0f;
	title->mPanelY             = -200.0f;
	title->mTargetPanelX       = -10.0f;
	title->mTargetPanelY       = -200.0f;
	title->_37C0               = 0.0f;
	title->mLoadOffset         = 0.0f;
	title->_37C8               = 0.0f;
	title->mOffset             = 0.0f;
	title->mOptionLine         = 0;
	title->mOptionState        = 0;
	title->mOptionAlpha        = 0.0f;
	title->mTargetOptionAlpha  = 0.0f;
	CPad pad(0);
	if (pad.mPad->mConnected != 0) {
		if (gGame.mStatus.mFlags & 0x800000000ULL)
			title->mRumble = 1;
		else
			title->mRumble = 0;
	} else {
		title->mRumble = 0;
	}
	if (gGame.mStatus.mFlags & 0x1000000000ULL)
		title->mMono = 1;
	else
		title->mMono = 0;
	title->mChannel = 0;
	for (s32 i = 0; i < 3; ++i)
		ResetSaveSlotRecord(&gTitleSaveSlots[i]);
	title->mLoadScroll    = 0.0f;
	title->mSaveFound     = 0;
	title->mCardScanState = 0;
	KeyOffGameSoundTrack(&gGame.mSound, 1);
#if !defined(VERSION_JP)
	title->mFormatCancelled = 0;
#endif
}

/**
 * @note Address: 0x80030EE4
 * @note Size: 0x120
 */
extern "C" void InitTitleScreen(Title* title)
{
	ResetTitleScreenState(title);
	title->mDemo.InitializeObject(0);
	for (s32 i = 0; i < 16; ++i) {
		title->mDust[i].mState  = 0;
		title->mDust[i].mTimer  = 0;
		title->mDust[i].mX      = rand() % 640;
		title->mDust[i].mY      = rand() % 480;
		title->mDust[i].mSpeedX = 0.0f;
		title->mDust[i].mSpeedY = 0.0f;
	}
#if !defined(VERSION_JP)
	title->mFormatCancelled = 0;
#endif
}
