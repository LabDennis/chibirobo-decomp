#include "game/hsd_jobj.h"
#include "game/cgame.h"
#include "game/cgs.h"
#include "game/cscrn.h"
#include "game/globals.h"
#include "game/screentransition.h"
#include "game/screenwipegeometry.h"
#include "game/video.h"
#include "game/xobjs.h"
#include "game/xchr.h"

#include <math.h>

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

/**
 * @note Address: 0x8001FCF4
 * @note Size: 0xAD4
 */
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
		f32 radius, height;
		if (ProjectRoboeyesItem(object, &radius, &height)) {
			work.entries[work.visible].object = object;
			Vec offset                        = { 0, 0, 0 };
			f32 x, y;
			object->ProjectOffsetToScreen(&offset, &x, &y, NULL, NULL);
			y -= .5f * radius;
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
			const char* name = GetItemName(object->GetItem());
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
					// moved above that label, so test against every placed label again
					j = -1;
				}
			}
			work.entries[work.used].y       = endY;
			work.entries[work.used].left    = left;
			work.entries[work.used++].width = width;
			if (pass == 0) {
				f32 inner = radius + .1f * radius * (f32)sin(eyes->mPhase), outer = 1.2f * inner;
				for (s32 k = 0; (f32)k < 16.0f; ++k) {
					f32 outerAngle = .3926991f * k + eyes->mAngle, innerAngle = .3926991f * (k + 1) + eyes->mAngle;
					f32 x0 = x + outer * (f32)sin(outerAngle), y0 = y + outer * (f32)cos(outerAngle);
					f32 x1 = x + inner * (f32)sin(innerAngle), y1 = y + inner * (f32)cos(innerAngle);
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
	// 0: lenses apart at full size until attachment 2 is out, 1: they slide together, 2: the right lens
	// shrinks to a half and the left to a sixth until it is put away, 3: back to full size, 4: apart again
	switch (state) {
	case 0:
		lw.target = width;
		lh.target = height;
		rw.target = width;
		rh.target = height;
		if (gGame.GetPlayer()->GetAttachment() == AttachmentPopper)
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
	StepScreenTransition(mWidth, mTargetWidth, mSpeedWidth, frames);
	StepScreenTransition(mHeight, mTargetHeight, mSpeedHeight, frames);
	StepScreenTransition(mColor.r, mTarget.r, mSpeed.r, frames);
	StepScreenTransition(mColor.g, mTarget.g, mSpeed.g, frames);
	StepScreenTransition(mColor.b, mTarget.b, mSpeed.b, frames);
	StepScreenTransition(mColor.a, mTarget.a, mSpeed.a, frames);
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
	StepScreenTransition(color->mColor.r, color->mTarget.r, color->mSpeed.r, frames);
	StepScreenTransition(color->mColor.g, color->mTarget.g, color->mSpeed.g, frames);
	StepScreenTransition(color->mColor.b, color->mTarget.b, color->mSpeed.b, frames);
	StepScreenTransition(color->mColor.a, color->mTarget.a, color->mSpeed.a, frames);
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
