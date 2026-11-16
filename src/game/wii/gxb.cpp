// Screen shakes, fades and the wipe family's shared code, ported to the Wii. The Wii added
// mScaleToWidescreen plus the SetDimensions/Start/StartTransition virtuals, the safe area's
// screen edges and a real refresh rate where the GameCube used FRAME_RATE.
#include "types.h"
#include "game/cscrn.h"
#include "game/screenwipegeometry.h"
#include "game/wii/cgame.h"
#include "game/wii/cgs.h"
#include "game/cprimitive.h"
#include "game/gmath.h"
#include "Dolphin/gx.h"
#include "Dolphin/mtx.h"
#include <math.h>
#include "game/wii/cpadact.h"
#include "game/wii/title.h"
#include "game/coordinfo.h"
#include "game/cptcl.h"
#include "game/ctpl.h"
#include "game/globals.h"
#include "JSystem/JMath/Inline.h"
#include "arithmetic.h"
#include "game/wii/cursor.h"
#include "game/logo.h"
#include <stdio.h>

extern "C" {
f64 fmax(f64, f64);
// the float forms of the Wii MSL (math_double.h), which the game's math.h declares as functions or not at all
inline f32 fmaxf(f32 x, f32 y)
{
	return fmax(x, y);
}
inline f32 sinf(f32 x)
{
	return sin(x);
}
inline f32 cosf(f32 x)
{
	return cos(x);
}
s32 GetItemIconTextureIndex(s32 id);
void DrawHudRoundedRectangle(u32 color, f32 x, f32 y, f32 w, f32 h, f32 rx, f32 ry);
void ApproachHudValue(f32* value, f32* target, f32 step);
void DrawRoboeyesItemLabels(CScrnRoboeyes* eyes);
}

// the screen layout globals wii/init.cpp fills in from the console's settings
extern s32 gRefreshRate;
extern s32 gScreenLeft;
extern s32 gScreenRight;
extern s32 gScreenTop;
extern s32 gScreenBottom;
extern f32 gScreenWidthScale; // against the GameCube's 640
extern s32 gPointerScreenWidth;
extern s32 gPointerScreenHeight;

// in the Wii's matrix unit (0x8018F304) but missing from its mtx.h
extern "C" void PSMTXMultVecArray(const Mtx m, const Vec* srcBase, Vec* dstBase, u32 count);

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

static inline bool SizeStopped(CScrnWipe* wipe)
{
	return 0.0f == wipe->mSpeedWidth && 0.0f == wipe->mSpeedHeight;
}

static inline bool HasColor(CScrnColor* color)
{
	return 0.0f != color->mColor.r || 0.0f != color->mColor.g || 0.0f != color->mColor.b || 0.0f != color->mColor.a;
}

static inline void CopyVector(CVec& destination, const Vec& source)
{
	destination.x = source.x;
	destination.y = source.y;
	destination.z = source.z;
}

// fills the wipe quad's corners from the frame's screen edges; z stays whatever the
// initializer left
static inline void SetWipeQuadScreenBounds(WipeQuad& quad)
{
	quad.vertices[0].x = (f32)gScreenLeft;
	quad.vertices[0].y = (f32)gScreenTop;
	quad.vertices[1].x = (f32)gScreenRight;
	quad.vertices[1].y = (f32)gScreenTop;
	quad.vertices[2].x = (f32)gScreenRight;
	quad.vertices[2].y = (f32)gScreenBottom;
	quad.vertices[3].x = (f32)gScreenLeft;
	quad.vertices[3].y = (f32)gScreenBottom;
}

static inline void BeginWipe(CGs* gs, s32 mode)
{
	if (mode) {
		static const Vec corners[4] = { { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 } };
		WipeQuad quad               = { { corners[0], corners[1], corners[2], corners[3] } };
		SetWipeQuadScreenBounds(quad);
		gs->mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
		DrawWipeQuad(gs, quad, 0);
	}
	gs->mCurRenderState->SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0xFF);
	SetGsChannelEnabled(gs, 0, 0);
	gs->mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gs->mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
}

static inline void FinishWipe(const CScrnWipe* wipe, CGs* gs, s32 mode)
{
	WipeQuad quad = { 0 };
	SetWipeQuadScreenBounds(quad);
	u32 color = ((u32)wipe->mColor.r << 24) | ((u32)wipe->mColor.g << 16) | ((u32)wipe->mColor.b << 8) | (u32)wipe->mColor.a;
	if (mode)
		gs->mCurRenderState->SetZMode(GX_TRUE, GX_LESS, GX_TRUE);
	else
		gs->mCurRenderState->SetZMode(GX_TRUE, GX_LESS, GX_FALSE);
	DrawWipeQuad(gs, quad, color);
	gs->mCurRenderState->SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_LEQUAL, 0xFF);
	gs->mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
}

static inline void BeginWipeMask(CGs* gs, f32 opacity)
{
	WipeQuad quad = { 0 };
	SetWipeQuadScreenBounds(quad);
	gs->mCurRenderState->SetDstAlpha(GX_FALSE, 0);
	gs->mCurRenderState->SetColorUpdate(GX_FALSE);
	gs->mCurRenderState->SetAlphaUpdate(GX_TRUE);
	gs->mCurRenderState->SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gs->mCurRenderState->SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0xFF);
	gs->mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
	gs->mCurTevStage->SetMode(GX_PASSCLR);
	DrawWipeQuad(gs, quad, 0xFF000000 | (u32)(255.0f - opacity));
	gs->mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gs->mCurRenderState->SetAlphaCompare(GX_GREATER, (u8)(s32)(255.0f - opacity), GX_AOP_AND, GX_LEQUAL, 0xFF);
}

static inline void EndWipeMask(CGs* gs)
{
	gs->mCurRenderState->SetColorUpdate(GX_TRUE);
	gs->mCurRenderState->SetAlphaUpdate(GX_FALSE);
	gs->mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_INVDSTALPHA, GX_BL_DSTALPHA, GX_LO_SET);
	gs->mCurRenderState->SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_LEQUAL, 0xFF);
	gs->mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
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
 * @note Address: 0x8003CEB0
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
 * @note Address: 0x8003CEE4
 * @note Size: 0x13C
 */
extern "C" void StartScreenShake(CScrnQuake* quake, f64 x0, f64 y0, f64 z0, f64 x1, f64 y1, f64 z1, f32 time)
{
	quake->mShake[1].Set(x1, y1, z1);
	quake->mShake[0].x = __fabsf(x0);
	quake->mShake[0].y = __fabsf(y0);
	quake->mShake[0].z = __fabsf(z0);
	quake->mShake[1].x = __fabsf(x1);
	quake->mShake[1].y = __fabsf(y1);
	quake->mShake[1].z = __fabsf(z1);
	if (time == 0.0f) {
		quake->mShake[2].Set(0.0f, 0.0f, 0.0f);
		CopyVector(quake->mShake[0], quake->mShake[1]);
	} else {
		CopyVector(quake->mShake[2], (quake->mShake[1] - quake->mShake[0]) / (gRefreshRate * time));
	}
	quake->mSign = 1;
}

/**
 * @note Address: 0x8003D020
 * @note Size: 0x70
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
 * @note Address: 0x8003D090
 * @note Size: 0x198
 */
extern "C" void UpdateScreenShake(CScrnQuake* quake, u32 frames)
{
	StepChannel(quake->mShake[0].x, quake->mShake[1].x, quake->mShake[2].x, frames);
	StepChannel(quake->mShake[0].y, quake->mShake[1].y, quake->mShake[2].y, frames);
	StepChannel(quake->mShake[0].z, quake->mShake[1].z, quake->mShake[2].z, frames);
	quake->mSign = -quake->mSign;
}

/**
 * @note Address: 0x8003D228
 * @note Size: 0x1DC
 */
// NONMATCHING: register choice only: retail has the green and blue speeds in f5/f7, this build in f6/f5.
// The speeds are computed alpha first, as retail orders its loads.
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
		f32 speedA      = ((f32)a1 - (f32)a0) / (gRefreshRate * time);
		f32 speedB      = ((f32)b1 - (f32)b0) / (gRefreshRate * time);
		f32 speedG      = ((f32)g1 - (f32)g0) / (gRefreshRate * time);
		f32 speedR      = ((f32)r1 - (f32)r0) / (gRefreshRate * time);
		color->mSpeed.r = speedR;
		color->mSpeed.g = speedG;
		color->mSpeed.b = speedB;
		color->mSpeed.a = speedA;
	}
}

/**
 * @note Address: 0x8003D404
 * @note Size: 0x178
 */
// NONMATCHING: register choice only: retail has the green and blue speeds in f5/f7, this build in f6/f5.
// The speeds are computed alpha first, as retail orders its loads.
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
		f32 speedA      = ((f32)a - color->mColor.a) / (gRefreshRate * time);
		f32 speedB      = ((f32)b - color->mColor.b) / (gRefreshRate * time);
		f32 speedG      = ((f32)g - color->mColor.g) / (gRefreshRate * time);
		f32 speedR      = ((f32)r - color->mColor.r) / (gRefreshRate * time);
		color->mSpeed.r = speedR;
		color->mSpeed.g = speedG;
		color->mSpeed.b = speedB;
		color->mSpeed.a = speedA;
	}
}

/**
 * @note Address: 0x8003D57C
 * @note Size: 0x1F8
 */
extern "C" void UpdateScreenFade(CScrnColor* color, u32 frames)
{
	StepChannel(color->mColor.r, color->mTarget.r, color->mSpeed.r, frames);
	StepChannel(color->mColor.g, color->mTarget.g, color->mSpeed.g, frames);
	StepChannel(color->mColor.b, color->mTarget.b, color->mSpeed.b, frames);
	StepChannel(color->mColor.a, color->mTarget.a, color->mSpeed.a, frames);
}

/**
 * @note Address: 0x8003D774
 * @note Size: 0x2B4
 */
extern "C" void DrawScreenColorOverlay(CScrnColor* color, CGs* gs)
{
	if (HasColor(color)) {
		WipeQuad quad = { 0 };
		SetWipeQuadScreenBounds(quad);
		u32 packed = ((u32)color->mColor.r << 24) | ((u32)color->mColor.g << 16) | ((u32)color->mColor.b << 8) | (u32)color->mColor.a;
		SetGsChannelEnabled(gs, 0, 0);
		gs->mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gs->mCurRenderState->SetZMode(GX_TRUE, GX_LESS, GX_FALSE);
		DrawWipeQuad(gs, quad, packed);
	}
}

/**
 * @note Address: 0x8003DA28
 * @note Size: 0x70
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
	mScaleToWidescreen         = TRUE;
}

/**
 * @note Address: 0x8003DA98
 * @note Size: 0xF4
 */
void CScrnWipe::SetDimensions(BOOL hideWhenDone, f32 x, f32 y, f32 width, f32 height, f32 targetWidth, f32 targetHeight, f32 time)
{
	mActive       = TRUE;
	mHideWhenDone = hideWhenDone;
	if (mScaleToWidescreen) {
		// the 4:3 picture's left edge on a 16:9 screen
		f32 offset = gGame.mAspectRatio == 1 ? 107.0f : 0.0f;
		if (gGame.mAspectRatio == 1) {
			x += offset;
			width *= gScreenWidthScale;
			height *= gScreenWidthScale;
			targetWidth *= gScreenWidthScale;
			targetHeight *= gScreenWidthScale;
		}
	}
	mX            = x;
	mY            = y;
	mWidth        = width;
	mHeight       = height;
	mTargetWidth  = targetWidth;
	mTargetHeight = targetHeight;
	if (0.0f == time) {
		mWidth      = targetWidth;
		mHeight     = targetHeight;
		mSpeedWidth = mSpeedHeight = 0.0f;
	} else {
		mSpeedWidth  = (targetWidth - width) / (gRefreshRate * time);
		mSpeedHeight = (targetHeight - height) / (gRefreshRate * time);
	}
}

/**
 * @note Address: 0x8003DB8C
 * @note Size: 0x164
 */
void CScrnWipe::Start(u8 r, u8 g, u8 b, u8 a, BOOL hideWhenDone, f32 x, f32 y, f32 width, f32 height, f32 targetWidth, f32 targetHeight,
                      f32 time)
{
	SetDimensions(hideWhenDone, x, y, width, height, targetWidth, targetHeight, time);
	mColor.r  = r;
	mColor.g  = g;
	mColor.b  = b;
	mColor.a  = a;
	mTarget.r = r;
	mTarget.g = g;
	mTarget.b = b;
	mTarget.a = a;
	mSpeed.r  = r;
	mSpeed.g  = g;
	mSpeed.b  = b;
	mSpeed.a  = a;
	if (mHideWhenDone && SizeStopped(this))
		mActive = FALSE;
}

/**
 * @note Address: 0x8003DCF0
 * @note Size: 0x284
 */
void CScrnWipe::StartTransition(u8 r, u8 g, u8 b, u8 a, u8 targetR, u8 targetG, u8 targetB, f32 x, f32 y, f32 width, f32 height,
                                f32 targetWidth, f32 targetHeight, f32 time, u8 targetA, BOOL hideWhenDone)
{
	SetDimensions(hideWhenDone, x, y, width, height, targetWidth, targetHeight, time);
	mColor.r  = r;
	mColor.g  = g;
	mColor.b  = b;
	mColor.a  = a;
	mTarget.r = targetR;
	mTarget.g = targetG;
	mTarget.b = targetB;
	mTarget.a = targetA;
	if (0.0f == time) {
		mColor.r = targetR;
		mColor.g = targetG;
		mColor.b = targetB;
		mColor.a = targetA;
		mSpeed.r = 0.0f;
		mSpeed.g = 0.0f;
		mSpeed.b = 0.0f;
		mSpeed.a = 0.0f;
	} else {
		f32 speedA = ((f32)targetA - (f32)a) / (gRefreshRate * time);
		f32 speedB = ((f32)targetB - (f32)b) / (gRefreshRate * time);
		f32 speedG = ((f32)targetG - (f32)g) / (gRefreshRate * time);
		f32 speedR = ((f32)targetR - (f32)r) / (gRefreshRate * time);
		mSpeed.r   = speedR;
		mSpeed.g   = speedG;
		mSpeed.b   = speedB;
		mSpeed.a   = speedA;
	}
	if (mHideWhenDone && SizeStopped(this))
		mActive = FALSE;
}

/**
 * @note Address: 0x8003DF74
 * @note Size: 0x394
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

// NONMATCHING: the four Draw methods share the wipe quad/state scaffolding below. Retail
// materializes an extra 48-byte stack copy of the quad (x/y copied, z written as 0) before
// some DrawGsVertexQuad calls; the source behind that copy is undetermined.
/**
 * @note Address: 0x8003E308
 * @note Size: 0xABC
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

/**
 * @note Address: 0x8003EDC4
 * @note Size: 0x50
 */
// a full screen tile keeps its aspect on widescreen by un-scaling the two heights
void CScrnWipeTile::SetDimensions(BOOL hideWhenDone, f32 x, f32 y, f32 width, f32 height, f32 targetWidth, f32 targetHeight, f32 time)
{
	if (mScaleToWidescreen && gGame.mAspectRatio == 1 && x == 320.0f && width == 640.0f && targetWidth == 640.0f) {
		height /= gScreenWidthScale;
		targetHeight /= gScreenWidthScale;
	}
	CScrnWipe::SetDimensions(hideWhenDone, x, y, width, height, targetWidth, targetHeight, time);
}

/**
 * @note Address: 0x8003EE14
 * @note Size: 0xC08
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
 * @note Address: 0x8003FA1C
 * @note Size: 0x934
 */
void CScrnWipeTex::Draw(CGs* gs, s32 mode)
{
	if (!mActive)
		return;
	BeginWipe(gs, mode);
	BeginWipeMask(gs, mColor.a);
	gs->mCurTevStage->SetMode(GX_MODULATE);
	gs->mCurRenderState->SetZCompLoc(GX_FALSE);
	DrawGsTexturedQuad(gs, &mTex, -1, -1, -1, -1, mX, mY, 0.0f, mWidth, mHeight, 0.0f, 0.0f, mRotation);
	gs->mCurRenderState->SetZCompLoc(GX_TRUE);
	EndWipeMask(gs);
	gs->mCurTevStage->SetMode(GX_PASSCLR);
	FinishWipe(this, gs, mode);
}

/**
 * @note Address: 0x80040350
 * @note Size: 0x70C
 */
// the grid of circles the custom wipe closes: the cell size comes from the pointer
// screen's width so the count survives widescreen
void CScrnWipeCustom::Draw(CGs* gs, s32 mode)
{
	if (!mActive)
		return;
	BeginWipe(gs, mode);
	f32 cell = (f32)gPointerScreenWidth / 16.0f;
	for (s32 row = 0; row < (f32)gPointerScreenHeight / cell; ++row) {
		f32 y = row * cell + cell * 0.5f;
		for (s32 col = 0; col < (f32)gPointerScreenWidth / cell; ++col) {
			f32 x = col * cell + cell * 0.5f;
			f32 extra;
			if (col < 16.0f * 0.5f)
				extra = 0.2f * col;
			else
				extra = 0.2f * ((f32)gPointerScreenWidth / cell - col);
			DrawGsCircle(gs, 16, 0, 0, x, y, 0.0f, mWidth + mWidth * extra, mHeight + mHeight * extra, 0.0f, 0.0f, 0.0f);
		}
	}
	FinishWipe(this, gs, mode);
}

/**
 * @note Address: 0x80040A5C
 * @note Size: 0xB4
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
	mScaleToWidescreen         = FALSE;
	mAngle                     = 0.0f;
	mPhase                     = 0.0f;
	ResetLensState(430.0f, 400.0f);
}

/**
 * @note Address: 0x80040B10
 * @note Size: 0x2F8
 */
void CScrnRoboeyes::StartTransition(u8 r, u8 g, u8 b, u8 a, u8 targetR, u8 targetG, u8 targetB, f32 x, f32 y, f32 width, f32 height,
                                    f32 targetWidth, f32 targetHeight, f32 time, u8 targetA, BOOL hideWhenDone)
{
	SetDimensions(hideWhenDone, x, y, width, height, targetWidth, targetHeight, time);
	mColor.r  = r;
	mColor.g  = g;
	mColor.b  = b;
	mColor.a  = a;
	mTarget.r = targetR;
	mTarget.g = targetG;
	mTarget.b = targetB;
	mTarget.a = targetA;
	if (0.0f == time) {
		mColor.r = targetR;
		mColor.g = targetG;
		mColor.b = targetB;
		mColor.a = targetA;
		mSpeed.r = 0.0f;
		mSpeed.g = 0.0f;
		mSpeed.b = 0.0f;
		mSpeed.a = 0.0f;
	} else {
		f32 speedA = ((f32)targetA - (f32)a) / (gRefreshRate * time);
		f32 speedB = ((f32)targetB - (f32)b) / (gRefreshRate * time);
		f32 speedG = ((f32)targetG - (f32)g) / (gRefreshRate * time);
		f32 speedR = ((f32)targetR - (f32)r) / (gRefreshRate * time);
		mSpeed.r   = speedR;
		mSpeed.g   = speedG;
		mSpeed.b   = speedB;
		mSpeed.a   = speedA;
	}
	if (mHideWhenDone && SizeStopped(this))
		mActive = FALSE;
	if (hideWhenDone)
		ResetLensState(width, height);
}

/**
 * @note Address: 0x80040E08
 * @note Size: 0x44
 */
// the lenses back to one eye of the given size, both open and centred
void CScrnRoboeyes::ResetLensState(f32 width, f32 height)
{
	mSeparation.value = mSeparation.target = 150.0f;
	mLeftWidth.value = mLeftWidth.target = width;
	mLeftHeight.value = mLeftHeight.target = height;
	mRightWidth.value = mRightWidth.target = width;
	mRightHeight.value = mRightHeight.target = height;
	mFocus.value = mFocus.target = 0.0f;
	mLensState                   = 0;
}

// moves a position by an offset passed by value. Retail's add is nw4r::math::VEC3Add, its three
// work registers show; nw4r/math.h does not compile under the game's include path (<cstdarg>),
// so the paired-single PSVECAdd the other Wii files use stands in and the two Updates differ in
// those registers
static inline void Move(CVec& pos, Vec offset)
{
	JMathInlineVEC::PSVECAdd(&pos, &offset, &pos);
}
// the item an object stands for, as the GameCube XObj::GetItem
static inline s32 GetItem(const WiiTitleObject* object)
{
	if (object->mKind != NULL) {
		return ((UnkObjData*)object->mKind)->GetItem();
	}
	return -1;
}

static inline BOOL ProjectRoboeyesItem(WiiTitleObject* object, f32* width, f32* height)
{
	f32 r = object->vf1D8() / 2.0f;
	if (r < 1.0f)
		r = 1.0f;
	f32 h = object->vf1DC();
	if (h < 1.0f)
		h = 1.0f;
	return object->vf1E0(width, height, NULL, r, h);
}

// the object's last drawn bounds touch the screen
static inline bool IsOnScreen(const WiiTitleObject* object)
{
	bool onScreen = false;
	if (object->mScreenBoundsValid && fpclassify(object->mScreenLeft) != FP_NAN && fpclassify(object->mScreenTop) != FP_NAN
	    && fpclassify(object->mScreenRight) != FP_NAN && fpclassify(object->mScreenBottom) != FP_NAN
	    && object->mScreenLeft <= (f32)gScreenRight && object->mScreenTop <= (f32)gScreenBottom && object->mScreenRight >= (f32)gScreenLeft
	    && object->mScreenBottom >= (f32)gScreenTop)
		onScreen = true;
	return onScreen;
}

/**
 * @note Address: 0x80040E4C
 * @note Size: 0x3DC
 */
// NONMATCHING: retail expands CScrnWipe::Update here. Its definition is in wii/gxb.cpp, the file this
// one continues; the call goes away once the two are one translation unit again.
void CScrnRoboeyes::Update(u32 frames)
{
	CScrnWipe::Update(frames);
	if (mActive) {
		mAngle += 0.02617994f * frames;
		mPhase += 0.10471976f * frames;
	}
}

/**
 * @note Address: 0x80041228
 * @note Size: 0x3DC
 */
void CScrnRoboeyes::Draw(CGs* gs, s32 mode)
{
	if (!mActive)
		return;
	// the scope follows the pointer while attachment 2 is out and rests at the middle otherwise
	if (!gGame.mHomeMenu.IsActive()) {
		if (gGame.GetPlayer()->GetAttachment() == 2) {
			mX = gWiiPointer.mPointer.x;
			mY = gWiiPointer.mPointer.y;
		} else {
			mX = gPointerScreenWidth / 2;
			mY = gPointerScreenHeight / 2;
		}
	}
	f32 x = mX, y = mY, width = mWidth, height = mHeight;
	u32 color = ((u32)mColor.r << 24) | ((u32)mColor.g << 16) | ((u32)mColor.b << 8) | (u32)mColor.a;
	SetGsChannelEnabled(gs, 0, 0);
	gs->mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gs->mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	gs->mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
	u32 alpha = color & 0xff;
	static f32 scanline;
	if (!gGame.mMenuActive) {
		if (++scanline >= 4.0f)
			scanline = 0;
	}
	gs->mCurRenderState->SetBlendMode(GX_BM_SUBTRACT, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gs->mCurRenderState->SetLineWidth(12, GX_TO_ONE);
	if (mWidth >= mTargetWidth) {
		u32 tint = 0x30303000 | alpha;
		// a variable in retail too: it stays in a register across the loop, a literal is loaded again for every line
		f32 zero = 0.0f;
		for (s32 i = 0; i < gPointerScreenHeight; i += 4.0f) {
			DrawGsLineSegment(gs, NULL, zero, i + scanline, zero, tint, gPointerScreenWidth, i + scanline, zero, tint);
		}
	}
	DrawRoboeyesItemLabels(this);
	DrawLenses(0, color, alpha, x, y, width, height);
}

/**
 * @note Address: 0x80041604
 * @note Size: 0xA74
 */
// NONMATCHING: math.h hands out the GameCube's inline __fpclassifyf where the Wii MSL has a function
// (0x8022A450), so the four NaN tests expand here. With them as calls 98.8 % is retail's; left over
// are register choices: radius in f31 above the hoisted constants, the object of the first loop in
// r21 above FSqrt's two strings, in-place fabs in the overlap test, and the stack slots of five
// spilled constants.
// The labels over the items the robo eyes see: a ring around each item, a line to a box with its
// icon and name, boxes pushed up where they would overlap.
extern "C" void DrawRoboeyesItemLabels(CScrnRoboeyes* eyes)
{
	if (!gGame.GetStatusFlags(0x100))
		return;
	if (gGame.mEvtData.mScript != NULL)
		return;
	struct Label {
		WiiTitleObject* object;
		f32 distance, left, y, width;
	};
	struct {
		Label entries[64];
		u32 used, visible;
	} work;
	work.used    = 0;
	work.visible = 0;
	for (s32 i = 0; i < gGame.mItemCount; ++i) {
		WiiTitleObject* object = (WiiTitleObject*)((u8*)gGame.mItems + i * gGame.mItemStride);
		if (!(object->mStatus & 1) || !(object->mStatus & 0x10000))
			continue;
		if (!GetItemFlagsMasked(GetItem(object), 2) || (object->mStatus & 0x0400000000000000ULL))
			continue;
		if (object->_998 != 0 || !(object->mStatus & 0x100000000ULL))
			continue;
		f32 radius, height;
		if (ProjectRoboeyesItem(object, &radius, &height)) {
			work.entries[work.visible].object = object;
			Vec offset                        = { 0, 0, 0 };
			f32 x, y;
			object->vf1D4(&offset, &x, &y, 0, 0);
			y -= radius / 2.0f;
			f32 dx = (f32)(gPointerScreenWidth / 2) - x, dy = (f32)(gPointerScreenHeight / 2) - y;
			work.entries[work.visible].distance = FSqrt(dx * dx + dy * dy);
			work.visible++;
		}
	}
	f32 size = 20.0f;
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
	const u32 gold = 0xffb628ff;
	f32 radius;
	for (s32 pass = 0; pass < 2; ++pass) {
		for (u32 i = 0; i < work.visible; ++i) {
			WiiTitleObject* object = work.entries[i].object;
			if (!IsOnScreen(object))
				continue;
			f32 boundsLeft = object->mScreenLeft, boundsRight = object->mScreenRight;
			f32 boundsTop = object->mScreenTop, boundsBottom = object->mScreenBottom;
			f32 boundsWidth = boundsRight - boundsLeft, boundsHeight = boundsBottom - boundsTop;
			f32 x, y, startX, startY, endX, right, endY, left, width;
			x = boundsLeft + boundsWidth / 2.0f;
			y = boundsTop + boundsHeight / 2.0f;
			// half the mean of the two sides
			radius           = (boundsWidth + (boundsHeight - boundsWidth) / 2.0f) / 2.0f;
			const char* name = (const char*)GetItemName(GetItem(object));
			width            = 32.0f + MeasureGameTextWidth(&gGame.mGs, name, TRUE, size);
			f32 angle        = 2.3561945f;
			startX           = x + radius * sinf(angle);
			startY           = y + radius * cosf(angle);
			endX             = x + 1.7f * radius * sinf(angle);
			endY             = y + 1.7f * radius * cosf(angle);
			left             = endX;
			right            = endX + width;
			for (u32 j = 0; j < work.used; ++j) {
				const Label& prior = work.entries[j];
				f32 dy = endY - prior.y, dx = left - prior.left;
				dy        = __fabsf(dy);
				dx        = __fabsf(dx);
				f32 reach = left > prior.left ? prior.width : width;
				if (dy < 32.0f && dx < reach) {
					endY = prior.y - 33.0f;
					if (left == right) {
						endX  = startX - __fabsf(endY - startY) / 2.0f;
						right = endX - width;
						left  = right;
					} else {
						endX  = startX + __fabsf(endY - startY) / 2.0f;
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
				// the ring breathes with mPhase; its teeth reach out a fifth further
				f32 segments = 16.0f;
				f32 inner    = radius + .1f * radius * sinf(eyes->mPhase);
				radius       = 1.2f * inner;
				for (s32 k = 0; (f32)k < segments; ++k) {
					f32 outerAngle = TWO_PI / segments * k + eyes->mAngle, innerAngle = TWO_PI / segments * (k + 1) + eyes->mAngle;
					f32 x0 = x + radius * sinf(outerAngle), y0 = y + radius * cosf(outerAngle);
					f32 x1 = x + inner * sinf(innerAngle), y1 = y + inner * cosf(innerAngle);
					DrawGsLineSegment(&gGame.mGs, NULL, x0, y0, 0, gold, x1, y1, 0, gold);
				}
				DrawGsLineSegment(&gGame.mGs, NULL, startX, startY, 0, gold, endX, endY, 0, gold);
			}
			if (pass == 1) {
				DrawHudRoundedRectangle(160, left + width / 2.0f, endY - 16.0f, width, 16.0f, 1.0f, 1.0f);
				DrawGsLineSegment(&gGame.mGs, NULL, endX, endY, 0, gold, right, endY, 0, gold);
				s32 icon  = GetItemIconTextureIndex(GetItem(object));
				CTpl& tpl = gGame.mTpl;
				tpl.mTex.Init(&tpl.mPalette->descriptorArray[icon], 0);
				CTexObj tex = tpl.mTex;
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, 16.0f + left, endY - 16.0f, 0, 32.0f, 32.0f, 0, 0, 0);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
				DrawGsProportionalTextFormat(&gGame.mGs, -1, name, 32.0f + left, (endY - size) - (32.0f - size) / 2.0f, 0, size, size);
			}
		}
		work.used = 0;
	}
	gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
}

/**
 * @note Address: 0x80042078
 * @note Size: 0xBDC
 */
// NONMATCHING: color and the color & ~0xff the tick loops share sit in r29 and r28 in retail and
// the other way round here; everything else is retail's
// mLensState 0: lenses apart at full size until attachment 2 is out, 1: they slide together, 2: the right
// lens shrinks to a half and the left to a sixth until it is put away, 3: back to full size, 4: apart again.
// single draws the left lens alone, already merged, without its trail
void CScrnRoboeyes::DrawLenses(u32 single, u32 color, u32 alpha, f32 x, f32 y, f32 width, f32 height)
{
	if (gGame.mMenuActive)
		return;
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
	static f32 trail[10];
	if (!gGame.mHomeMenu.IsActive()) {
		for (s32 i = 9; i > 0; --i)
			trail[i] = trail[i - 1];
		trail[0] = mRotation;
	}
	if (gGame.GetPlayer()->GetAttachment() == 2) {
		f32 fade;
		if (gGame.mScopeTimer > 0.0f) {
			if (gGame.mScopeTimer >= 5.0f)
				fade = 0.0f;
			else
				fade = sinf(HALF_PI * (1.0f - (f32)fmod(gGame.mScopeTimer, 1.0)));
		} else {
			fade = 1.0f;
		}
		alpha = (u32)((f32)alpha * fade);
		color = (color & ~0xffU) | alpha;
	}
	if (single == 1) {
		mLensState        = 2;
		mSeparation.value = mSeparation.target = 0.0f;
	}
	if (!gGame.mHomeMenu.IsActive()) {
		switch (mLensState) {
		case 0:
			mLeftWidth.target   = width;
			mLeftHeight.target  = height;
			mRightWidth.target  = width;
			mRightHeight.target = height;
			if (gGame.GetPlayer()->GetAttachment() == 2) {
				mLensState        = 1;
				mSeparation.value = mSeparation.target = 0.0f;
				mLeftWidth.value = mLeftWidth.target = width / 6.0f;
				mLeftHeight.value = mLeftHeight.target = height / 6.0f;
			}
			break;
		case 1:
			mSeparation.target = 0.0f;
			if (mSeparation.value <= 0.1f) {
				mLensState    = 2;
				mFocus.target = 120.0f;
			}
			break;
		case 2:
			mLeftWidth.target   = width / 6.0f;
			mLeftHeight.target  = height / 6.0f;
			mRightWidth.target  = width / 2.0f;
			mRightHeight.target = height / 2.0f;
			if (gGame.GetPlayer()->GetAttachment() != 2) {
				mLensState    = 3;
				mFocus.target = 0.0f;
			}
			break;
		case 3:
			mLeftWidth.target   = width;
			mLeftHeight.target  = height;
			mRightWidth.target  = width;
			mRightHeight.target = height;
			if (mLeftWidth.value >= width - 1.0f)
				mLensState = 4;
			break;
		case 4:
			mSeparation.target = 150.0f;
			if (mSeparation.value >= 149.0f) {
				mSeparation.value = 150.0f;
				mLensState        = 0;
			}
			break;
		}
		// the lenses never close slower than 12 a frame
		f32 step = __fabsf((mSeparation.target - mSeparation.value) / 2.0f);
		if (step < 12.0f)
			step = 12.0f;
		ApproachHudValue(&mSeparation.value, &mSeparation.target, step);
		ApproachHudValue(&mLeftWidth.value, &mLeftWidth.target, __fabsf((mLeftWidth.target - mLeftWidth.value) / 2.0f));
		ApproachHudValue(&mLeftHeight.value, &mLeftHeight.target, __fabsf((mLeftHeight.target - mLeftHeight.value) / 2.0f));
		ApproachHudValue(&mFocus.value, &mFocus.target, __fabsf((mFocus.target - mFocus.value) / 2.0f));
		ApproachHudValue(&mRightWidth.value, &mRightWidth.target, __fabsf((mRightWidth.target - mRightWidth.value) / 2.0f));
		ApproachHudValue(&mRightHeight.value, &mRightHeight.target, __fabsf((mRightHeight.target - mRightHeight.value) / 2.0f));
	}
	f32 offset = mSeparation.value;
	f32 lrx = .6f * (mLeftWidth.value / 2.0f), lry = .6f * (mLeftHeight.value / 2.0f);
	f32 rx = .6f * (mRightWidth.value / 2.0f), ry = .6f * (mRightHeight.value / 2.0f);
	Vec points[65];
	// the left lens: its ring, then the tick marks at the last rotations, each fainter than the one before
	f32 cx = x - offset;
	for (s32 i = 0; i < 65; ++i) {
		f32 sx = sinf(.09817477f * i), cy = cosf(.09817477f * i);
		points[i].x = sx * lrx + cx;
		points[i].y = cy * lry + y;
		points[i].z = 0;
	}
	DrawGsLineStrip(&gGame.mGs, points, color, 65);
	u32 trailColor = color;
	// a single lens shows the current rotation only
	s32 count = single == 0 ? 10 : 1;
	for (s32 i = 0; i < count; ++i) {
		for (s32 j = 0; j < 5; ++j) {
			f32 sx = sinf(HALF_PI * j + trail[i]), cy = cosf(HALF_PI * j + trail[i]);
			f32 x0 = sx * lrx + cx, x1 = sx * (lrx - 10.0f) + cx;
			f32 y0 = cy * lry + y, y1 = cy * (lry - 10.0f) + y;
			DrawGsLineSegment(&gGame.mGs, NULL, x0, y0, 0, trailColor, x1, y1, 0, trailColor);
		}
		trailColor = (color & ~0xffU) | (u8)(s32)(0.5f * ((f32)alpha * (10 - i) / 10.0f));
	}
	if (single == 0) {
		// the right lens turns the other way
		f32 rcx = x + offset;
		for (s32 i = 0; i < 65; ++i) {
			f32 sx = sinf(.09817477f * i), cy = cosf(.09817477f * i);
			points[i].x = sx * rx + rcx;
			points[i].y = cy * ry + y;
			points[i].z = 0;
		}
		DrawGsLineStrip(&gGame.mGs, points, color, 65);
		trailColor = color;
		for (s32 i = 0; i < 10; ++i) {
			for (s32 j = 0; j < 5; ++j) {
				f32 sx = sinf(HALF_PI * j + -trail[i]), cy = cosf(HALF_PI * j + -trail[i]);
				f32 x0 = sx * rx + rcx, x1 = sx * (rx - 10.0f) + rcx;
				f32 y0 = cy * ry + y, y1 = cy * (ry - 10.0f) + y;
				DrawGsLineSegment(&gGame.mGs, NULL, x0, y0, 0, trailColor, x1, y1, 0, trailColor);
			}
			trailColor = (color & ~0xffU) | (u8)(s32)(0.5f * ((f32)alpha * (10 - i) / 10.0f));
		}
		if (mLensState == 2) {
			CTexObj tex;
			static s32 frame;
			if (!gGame.mHomeMenu.IsActive())
				++frame;
			u32 tint = 0xffffff00 | alpha;
			for (s32 i = 0; i < 4; ++i) {
				f32 a      = HALF_PI * i;
				f32 pulse  = 3.0f * sinf(DEG2RAD * ((frame * 24) % 360));
				f32 radius = rx + pulse;
				f32 px     = radius * sinf(a) + x;
				f32 py     = radius * cosf(a) + y;
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				CTpl& tpl = gGame.mTpl;
				tpl.mTex.Init(&tpl.mPalette->descriptorArray[129], 0);
				tex    = tpl.mTex;
				width  = 1.4f * (1.1f * GXGetTexObjWidth(&tex));
				height = 1.4f * GXGetTexObjHeight(&tex);
				DrawGsTexturedQuad(&gGame.mGs, &tex, tint, tint, tint, tint, px, py, 0, width, height, 0, 0, a);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			}
		}
	}
	gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
}

/**
 * @note Address: 0x80042C54
 * @note Size: 0x16C
 */
// NONMATCHING: Move's add uses six float registers where retail's VEC3Add uses three, see Move
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
			mActive = false;
			mAlpha  = 0.0f;
			return;
		}
	}
	mVel.y -= 0.05f * (f32)frames;
	Move(mPos, (mVel * 1.0f) * (f32)frames);
	mAngle += mAngleSpeed;
}

/**
 * @note Address: 0x80042DC0
 * @note Size: 0x1C4
 */
void CPtclDust2::Draw(CGs* gs)
{
	if (!mActive) {
		return;
	}
	u32 color  = (mColor & 0xFFFFFF00) | (u8)(s32)((f32)(mColor & 0xFF) * mAlpha);
	f32 radius = 1.5f * fmaxf(mSize.x, mSize.y);
	if (mTex != NULL) {
		gs->mCurTevStage->SetMode(GX_MODULATE);
		Vec rot = { 0.0f, 0.0f, 0.0f };
		rot.z   = mAngle;
		DrawGsTexturedQuad(gs, mTex, color, color, color, color, mPos.x, mPos.y, mPos.z, radius, radius, rot.x, rot.y, rot.z);
		gs->mCurTevStage->SetMode(GX_PASSCLR);
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
 * @note Address: 0x80042F84
 * @note Size: 0x158
 */
// NONMATCHING: Move's add uses six float registers where retail's VEC3Add uses three, see Move
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
			mActive = false;
			mAlpha  = 0.0f;
			return;
		}
	}
	Move(mPos, (mVel * 1.0f) * (f32)frames);
	mAngle += mAngleSpeed;
}

/**
 * @note Address: 0x800430DC
 * @note Size: 0x1A0
 */
void CPtclDust3::Draw(CGs* gs)
{
	if (!mActive) {
		return;
	}
	f32 width     = 2.0f * (mSize.x * sinf(0.8f * (PI * mAlpha)));
	f32 height    = 2.0f * (mSize.y * sinf(0.8f * (PI * mAlpha)));
	u32 edgeColor = mColor & 0xFFFFFF00;
	u32 color     = (mColor & 0xFFFFFF00) | ((u32)(s32)((f32)(mColor & 0xFF) * mAlpha) & 0xFF);
	if (mTex != NULL) {
		gs->mCurTevStage->SetMode(GX_MODULATE);
		Vec rot = { 0.0f, 0.0f, 0.0f };
		rot.z   = mAngle;
		DrawGsTexturedQuad(gs, mTex, color, color, color, color, mPos.x, mPos.y, mPos.z, width, height, rot.x, rot.y, rot.z);
		gs->mCurTevStage->SetMode(GX_PASSCLR);
	} else {
		Vec rot = { 0.0f, 0.0f, 0.0f };
		rot.z   = mAngle;
		DrawGsCircle(gs, 8, color, edgeColor, mPos.x, mPos.y, mPos.z, width, height, rot.x, rot.y, rot.z);
	}
}

/**
 * @note Address: 0x8004327C
 * @note Size: 0x10
 */
void CPtclOne::Init()
{
	mActive = false;
	mTex    = NULL;
}

// The object classes' small virtuals and the boot logos (0x8004330C on). The destructors of CPtclDust3
// and CPtclDust2 (0x8004328C, 0x800432CC) come out with their vtables; the three this-adjusting thunks
// at 0x800444F0-0x80044507 come out with Cursor's vtable.
extern "C" {
void HSD_MtxGetScale(Mtx m, Vec* out);
}

// the nw4r::g3d::ResMdl handle is the NW_Obj's second word, still untyped in xobj.h
#define COBJ_RESMDL ((nw4r::g3d::ResMdl*)&mObject._04)
#define COBJ_NW_OBJ (&mObject)

// the three steppers: each vec, its target and its speed lie as one CObjVec, which xobj.h still
// spells as three members

// the scale a matrix carries. retail expands it in both GetJointScale: each keeps the vec
// HSD_MtxGetScale fills in and, behind it in the frame, an unnamed copy it reads the result from.
// the name is ours
static inline Vec GetMtxScale(MtxP mtx)
{
	Vec scale;
	HSD_MtxGetScale(mtx, &scale);
	return scale;
}

/**
 * @note Address: 0x8004330C
 * @note Size: 0x4
 */
void Cursor::InitializeObject()
{
	XObj::InitializeObject();
}

/**
 * @note Address: 0x80043310
 * @note Size: 0x38
 */
void Cursor::ResetObjectState()
{
	XObj::ResetObjectState();
	mAngleSpeed = 0.0f;
	mAngle      = 0.0f;
}

/**
 * @note Address: 0x80043348
 * @note Size: 0x2C
 */
BOOL Cursor::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info)) {
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80043374
 * @note Size: 0x88
 */
// Cursor's update, spins it
void Cursor::UpdateObject(u32 frames)
{
	mAngle = fmod(mAngleSpeed * (f32)frames + mAngle, TWO_PI);
	SetRotationZ(mAngle);
	XObj::UpdateObject(frames);
}

/**
 * @note Address: 0x800433FC
 * @note Size: 0xC
 */
void CObj::SetRotationZ(f32 z)
{
	mMoveRotate.z  = z;
	mMoveTarget1.z = z;
}

/**
 * @note Address: 0x80043408
 * @note Size: 0x14
 */
void Cursor::DrawObject()
{
	DrawWithView(NULL);
}

/**
 * @note Address: 0x8004341C
 * @note Size: 0x4
 */
void CObj::_unk_00C()
{
}

/**
 * @note Address: 0x80043420
 * @note Size: 0x4
 */
void CObj::ExecCallbackC()
{
}

/**
 * @note Address: 0x80043424
 * @note Size: 0x8
 */
void CObj::ReleaseModelWork()
{
	COBJ_NW_OBJ->ReleaseWork();
}

/**
 * @note Address: 0x8004342C
 * @note Size: 0x8C
 */
bool CObj::IsAnimEnd(f32 frames)
{
	f32 frame = GetAnimFrame();
	f32 end   = GetAnimEndFrame();
	return frame + mAnimRate[0] * frames >= end;
}

/**
 * @note Address: 0x800434B8
 * @note Size: 0x14
 */
f32 CObj::GetAnimRate()
{
	return GetAnimRate(0);
}

/**
 * @note Address: 0x800434CC
 * @note Size: 0x10
 */
f32 CObj::GetAnimRate(s32 index)
{
	return mAnimRate[index];
}

/**
 * @note Address: 0x800434DC
 * @note Size: 0x3C
 */
f32 CObj::GetNodeRotateZ(nw4r::g3d::ResNode node)
{
	return COBJ_NW_OBJ->GetNodeRotate(node).z;
}

/**
 * @note Address: 0x80043518
 * @note Size: 0xB8
 */
// NONMATCHING: retail builds the vec in a temporary of its own (z first) and copy-constructs the
// result from it; this build constructs the result in place, 0x1C bytes shorter. The source form
// that keeps the temporary is not found (a named local, a const local and an inline operator ruled out)
CVec NW_Obj::GetNodeRotate(nw4r::g3d::ResNode node) const
{
	const nw4r::g3d::ResNodeData& data = node.ref();
	CVec rotate(DEG2RAD * data.mRotate.x, DEG2RAD * data.mRotate.y, DEG2RAD * data.mRotate.z);
	return rotate;
}

/**
 * @note Address: 0x800435D0
 * @note Size: 0x54
 */
f32 CObj::GetNodeRotateZ(const char* name)
{
	return GetNodeRotateZ(GetResNode(name));
}

/**
 * @note Address: 0x80043624
 * @note Size: 0x8
 */
nw4r::g3d::ResNode CObj::GetResNode(const char* name)
{
	return COBJ_RESMDL->GetResNode(name);
}

/**
 * @note Address: 0x8004362C
 * @note Size: 0x54
 */
f32 CObj::GetNodeRotateZ(u32 index)
{
	return GetNodeRotateZ(GetResNode(index));
}

/**
 * @note Address: 0x80043680
 * @note Size: 0x8
 */
nw4r::g3d::ResNode CObj::GetResNode(u32 index)
{
	return COBJ_RESMDL->GetResNode(index);
}

/**
 * @note Address: 0x80043688
 * @note Size: 0x54
 */
f32 CObj::GetNodeRotateZ()
{
	return GetNodeRotateZ(GetResNode());
}

/**
 * @note Address: 0x800436DC
 * @note Size: 0xC
 */
nw4r::g3d::ResNode CObj::GetResNode()
{
	return COBJ_RESMDL->GetResNode((u32)0);
}

/**
 * @note Address: 0x800436E8
 * @note Size: 0x3C
 */
f32 CObj::GetNodeRotateY(nw4r::g3d::ResNode node)
{
	return COBJ_NW_OBJ->GetNodeRotate(node).y;
}

/**
 * @note Address: 0x80043724
 * @note Size: 0x54
 */
f32 CObj::GetNodeRotateY(const char* name)
{
	return GetNodeRotateY(GetResNode(name));
}

/**
 * @note Address: 0x80043778
 * @note Size: 0x54
 */
f32 CObj::GetNodeRotateY(u32 index)
{
	return GetNodeRotateY(GetResNode(index));
}

/**
 * @note Address: 0x800437CC
 * @note Size: 0x54
 */
f32 CObj::GetNodeRotateY()
{
	return GetNodeRotateY(GetResNode());
}

/**
 * @note Address: 0x80043820
 * @note Size: 0x3C
 */
f32 CObj::GetNodeRotateX(nw4r::g3d::ResNode node)
{
	return COBJ_NW_OBJ->GetNodeRotate(node).x;
}

/**
 * @note Address: 0x8004385C
 * @note Size: 0x54
 */
f32 CObj::GetNodeRotateX(const char* name)
{
	return GetNodeRotateX(GetResNode(name));
}

/**
 * @note Address: 0x800438B0
 * @note Size: 0x54
 */
f32 CObj::GetNodeRotateX(u32 index)
{
	return GetNodeRotateX(GetResNode(index));
}

/**
 * @note Address: 0x80043904
 * @note Size: 0x54
 */
f32 CObj::GetNodeRotateX()
{
	return GetNodeRotateX(GetResNode());
}

/**
 * @note Address: 0x80043958
 * @note Size: 0x30
 */
CVec CObj::GetNodeTranslate(nw4r::g3d::ResNode node)
{
	return COBJ_NW_OBJ->GetNodeTranslate(node);
}

/**
 * @note Address: 0x80043988
 * @note Size: 0x8C
 */
// NONMATCHING: same instructions, but retail stores the result's vtable ahead of loading the node's
// data pointer and this build behind it
CVec NW_Obj::GetNodeTranslate(nw4r::g3d::ResNode node) const
{
	const nw4r::g3d::ResNodeData& data = node.ref();
	return CVec(data.mTranslate.x, data.mTranslate.y, data.mTranslate.z);
}

/**
 * @note Address: 0x80043A14
 * @note Size: 0x6C
 */
CVec CObj::GetNodeTranslate(const char* name)
{
	return GetNodeTranslate(GetResNode(name));
}

/**
 * @note Address: 0x80043A80
 * @note Size: 0x6C
 */
CVec CObj::GetNodeTranslate(u32 index)
{
	return GetNodeTranslate(GetResNode(index));
}

/**
 * @note Address: 0x80043AEC
 * @note Size: 0x68
 */
CVec CObj::GetNodeTranslate()
{
	return GetNodeTranslate(GetResNode());
}

/**
 * @note Address: 0x80043B54
 * @note Size: 0xF4
 */
// NONMATCHING: retail reads mObject.m_pScnMdl and tests it ahead of saving the registers, and loads the three
// floats of the temporary CVec in the opposite registers (z in f0); the frame and the rest are retail's
// the translation column of the node's world matrix
CVec CObj::GetWorldPos(nw4r::g3d::ResNode node)
{
	MtxP mtx = GetNodeWorldMtx(node);
	Vec pos  = CVec(mtx[0][3], mtx[1][3], mtx[2][3]);
	return CVec(pos.x, pos.y, pos.z);
}

/**
 * @note Address: 0x80043C48
 * @note Size: 0x6C
 */
CVec CObj::GetWorldPos(const char* name)
{
	return GetWorldPos(GetResNode(name));
}

/**
 * @note Address: 0x80043CB4
 * @note Size: 0x6C
 */
CVec CObj::GetWorldPos(u32 index)
{
	return GetWorldPos(GetResNode(index));
}

/**
 * @note Address: 0x80043D20
 * @note Size: 0x68
 */
CVec CObj::GetWorldPos()
{
	return GetWorldPos(GetResNode());
}

/**
 * @note Address: 0x80043D88
 * @note Size: 0x68
 */
// NONMATCHING: one instruction out of place, retail reads mObject.m_pScnMdl ahead of saving r31 (an early
// return, a conditional expression and an inline test were tried)
MtxP CObj::GetWorldMtx(const char* name)
{
	if (mObject.m_pScnMdl != NULL) {
		return GetWorldMtx(GetResNode(name));
	}
	return NULL;
}

/**
 * @note Address: 0x80043DF0
 * @note Size: 0x68
 */
// NONMATCHING: one instruction out of place, retail reads mObject.m_pScnMdl ahead of saving r31 (an early
// return, a conditional expression and an inline test were tried)
MtxP CObj::GetWorldMtx(u32 index)
{
	if (mObject.m_pScnMdl != NULL) {
		return GetWorldMtx(GetResNode(index));
	}
	return NULL;
}

/**
 * @note Address: 0x80043E58
 * @note Size: 0x68
 */
// NONMATCHING: one instruction out of place, retail reads mObject.m_pScnMdl ahead of saving r31 (an early
// return, a conditional expression and an inline test were tried)
MtxP CObj::GetWorldMtx()
{
	if (mObject.m_pScnMdl != NULL) {
		return GetWorldMtx(GetResNode());
	}
	return NULL;
}

/**
 * @note Address: 0x80043EC0
 * @note Size: 0x1C
 */
void CObj::SetScale(const Vec& scale)
{
	SetScale(scale.x, scale.y, scale.z);
}

/**
 * @note Address: 0x80043EDC
 * @note Size: 0x1C
 */
void CObj::SetRot(f32 x, f32 y, f32 z)
{
	mMoveRotate.x  = x;
	mMoveRotate.y  = y;
	mMoveRotate.z  = z;
	mMoveTarget1.x = x;
	mMoveTarget1.y = y;
	mMoveTarget1.z = z;
}

/**
 * @note Address: 0x80043EF8
 * @note Size: 0x1C
 */
void CObj::SetRot(const Vec& rot)
{
	SetRot(rot.x, rot.y, rot.z);
}

/**
 * @note Address: 0x80043F14
 * @note Size: 0x1C
 */
void CObj::SetPos(const Vec& pos)
{
	SetPos(pos.x, pos.y, pos.z);
}

/**
 * @note Address: 0x80043F30
 * @note Size: 0xC
 */
void CObj::SetScaleZ(f32 z)
{
	mMoveScale.z   = z;
	mMoveTarget2.z = z;
}

/**
 * @note Address: 0x80043F3C
 * @note Size: 0xC
 */
void CObj::SetScaleY(f32 y)
{
	mMoveScale.y   = y;
	mMoveTarget2.y = y;
}

/**
 * @note Address: 0x80043F48
 * @note Size: 0xC
 */
void CObj::SetScaleX(f32 x)
{
	mMoveScale.x   = x;
	mMoveTarget2.x = x;
}

/**
 * @note Address: 0x80043F54
 * @note Size: 0xC
 */
void CObj::SetRotationY(f32 y)
{
	mMoveRotate.y  = y;
	mMoveTarget1.y = y;
}

/**
 * @note Address: 0x80043F60
 * @note Size: 0xC
 */
void CObj::SetRotationX(f32 x)
{
	mMoveRotate.x  = x;
	mMoveTarget1.x = x;
}

/**
 * @note Address: 0x80043F6C
 * @note Size: 0xC
 */
void CObj::SetTranslateZ(f32 z)
{
	mMoveTranslate.z = z;
	mMoveTarget0.z   = z;
}

/**
 * @note Address: 0x80043F78
 * @note Size: 0xC
 */
void CObj::SetTranslateY(f32 y)
{
	mMoveTranslate.y = y;
	mMoveTarget0.y   = y;
}

/**
 * @note Address: 0x80043F84
 * @note Size: 0xC
 */
void CObj::SetTranslateX(f32 x)
{
	mMoveTranslate.x = x;
	mMoveTarget0.x   = x;
}

/**
 * @note Address: 0x80043F90
 * @note Size: 0x10
 */
nw4r::g3d::ResNode CObj::GetRootResNode()
{
	return GetResNode();
}

/**
 * @note Address: 0x80043FA0
 * @note Size: 0x8
 */
u8 CObj::StepScale(u32 frames)
{
	return ((CObjVec*)&mMoveScale)->Step(frames);
}

/**
 * @note Address: 0x80043FA8
 * @note Size: 0x8
 */
u8 CObj::StepRotate(u32 frames)
{
	return ((CObjVec*)&mMoveRotate)->Step(frames);
}

/**
 * @note Address: 0x80043FB0
 * @note Size: 0x8
 */
u8 CObj::StepTranslate(u32 frames)
{
	return ((CObjVec*)&mMoveTranslate)->Step(frames);
}

/**
 * @note Address: 0x80043FB8
 * @note Size: 0x8
 */
nw4r::g3d::ResNode XObj::GetResNode(u32 index)
{
	return COBJ_RESMDL->GetResNode(index);
}

/**
 * @note Address: 0x80043FC0
 * @note Size: 0xC
 */
nw4r::g3d::ResNode XObj::GetResNode()
{
	return COBJ_RESMDL->GetResNode((u32)0);
}

/**
 * @note Address: 0x80043FCC
 * @note Size: 0x30
 */
f32 XObj::GetScaleZ()
{
	return GetScale()->z;
}

/**
 * @note Address: 0x80043FFC
 * @note Size: 0x8
 */
CVec* XObj::GetScale()
{
	return &mMoveScale;
}

/**
 * @note Address: 0x80044004
 * @note Size: 0x30
 */
f32 XObj::GetScaleY()
{
	return GetScale()->y;
}

/**
 * @note Address: 0x80044034
 * @note Size: 0x30
 */
f32 XObj::GetScaleX()
{
	return GetScale()->x;
}

/**
 * @note Address: 0x80044064
 * @note Size: 0x8
 */
f32 XObj::GetRotZ()
{
	return mMoveRotate.z;
}

/**
 * @note Address: 0x8004406C
 * @note Size: 0x8
 */
f32 XObj::GetRotY()
{
	return mMoveRotate.y;
}

/**
 * @note Address: 0x80044074
 * @note Size: 0x8
 */
f32 XObj::GetRotX()
{
	return mMoveRotate.x;
}

/**
 * @note Address: 0x8004407C
 * @note Size: 0x30
 */
f32 XObj::GetPosZ()
{
	return GetPos()->z;
}

/**
 * @note Address: 0x800440AC
 * @note Size: 0x30
 */
f32 XObj::GetPosY()
{
	return GetPos()->y;
}

/**
 * @note Address: 0x800440DC
 * @note Size: 0x30
 */
f32 XObj::GetPosX()
{
	return GetPos()->x;
}

/**
 * @note Address: 0x8004410C
 * @note Size: 0x7C
 */
void XObj::GetJointScale(nw4r::g3d::ResNode node, Vec* out)
{
	Vec scale = GetMtxScale(GetWorldMtx(node));
	out->x    = scale.x;
	out->y    = scale.y;
	out->z    = scale.z;
}

/**
 * @note Address: 0x80044188
 * @note Size: 0x8C
 */
CVec XObj::GetJointScale(nw4r::g3d::ResNode node)
{
	Vec scale = GetMtxScale(GetWorldMtx(node));
	return CVec(scale.x, scale.y, scale.z);
}

/**
 * @note Address: 0x80044214
 * @note Size: 0x10
 */
CVec XObj::GetJointTranslate(const char* name)
{
	return GetNodeTranslate(name);
}

/**
 * @note Address: 0x80044224
 * @note Size: 0x10
 */
CVec XObj::GetJointTranslate(u32 index)
{
	return GetNodeTranslate(index);
}

/**
 * @note Address: 0x80044234
 * @note Size: 0x10
 */
CVec XObj::GetJointWorldPos(const char* name)
{
	return GetWorldPos(name);
}

/**
 * @note Address: 0x80044244
 * @note Size: 0x10
 */
CVec XObj::GetJointWorldPos(u32 index)
{
	return GetWorldPos(index);
}

/**
 * @note Address: 0x80044254
 * @note Size: 0x10
 */
CVec XObj::GetJointPos(const char* name)
{
	return GetWorldPos(name);
}

/**
 * @note Address: 0x80044264
 * @note Size: 0x10
 */
CVec XObj::GetJointPos(u32 index)
{
	return GetWorldPos(index);
}

/**
 * @note Address: 0x80044274
 * @note Size: 0x38
 */
CVec XObj::GetJointPos(nw4r::g3d::ResNode node)
{
	return GetWorldPos(node);
}

/**
 * @note Address: 0x800442AC
 * @note Size: 0x14
 */
BOOL XObj::HasModel()
{
	return mObject.m_pScnMdl != NULL;
}

/**
 * @note Address: 0x800442C0
 * @note Size: 0x78
 */
nw4r::g3d::ResNode XObj::GetResNodeOrFirst(u32 index)
{
	if (index < COBJ_RESMDL->GetResNodeNumEntries()) {
		return GetResNode(index);
	}
	return GetResNode((u32)0);
}

/**
 * @note Address: 0x80044338
 * @note Size: 0x10
 */
nw4r::g3d::ResNode XObj::FindResNode(const char* name)
{
	return GetResNode(name);
}

/**
 * @note Address: 0x80044348
 * @note Size: 0x10
 */
void XObj::vf284()
{
	vf280();
}

/**
 * @note Address: 0x80044358
 * @note Size: 0x6C
 */
void XObj::RequestMoveDir(const Vec* dir, f32 speed)
{
	OnMoveRequest();
	SetMoveDir(dir, speed);
}

/**
 * @note Address: 0x800443C4
 * @note Size: 0x4
 */
void XObj::OnMoveRequest()
{
}

/**
 * @note Address: 0x800443C8
 * @note Size: 0x6C
 */
void XObj::RequestRotateY(f32 target, f32 speed)
{
	OnMoveRequest();
	SetRotateY(target, speed);
}

/**
 * @note Address: 0x80044434
 * @note Size: 0x5C
 */
void XObj::RequestRotationY(f32 y)
{
	OnMoveRequest();
	SetRotationY(y);
}

/**
 * @note Address: 0x80044490
 * @note Size: 0x4
 */
void XObj::OnPushDisplacement(const Vec& displacement)
{
}

/**
 * @note Address: 0x80044494
 * @note Size: 0x8
 */
const void* XObj::GetEventScript()
{
	return mEventScript;
}

/**
 * @note Address: 0x8004449C
 * @note Size: 0x8
 */
s32 XObj::HandleEventCommand(s32 count, s32* args)
{
	return 0;
}

/**
 * @note Address: 0x800444A4
 * @note Size: 0x8
 */
s32 XObj::OnPlayerContact(s32 unused, s32 value)
{
	return value;
}

/**
 * @note Address: 0x800444AC
 * @note Size: 0xC
 */
void XObj::ClearBaseMtx()
{
	mBaseMtxPtr = NULL;
}

/**
 * @note Address: 0x800444B8
 * @note Size: 0x14
 */
void XObj::SetBaseMtx(Mtx mtx)
{
	mBaseMtxPtr = mBaseMtx;
	PSMTXCopy(mtx, mBaseMtx);
}

/**
 * @note Address: 0x800444CC
 * @note Size: 0x1C
 */
Vec XObj::GetCameraTarget()
{
	return mMoveTranslate;
}

/**
 * @note Address: 0x800444E8
 * @note Size: 0x4
 */
void XObj::DrawDebugStatus()
{
}

/**
 * @note Address: 0x800444EC
 * @note Size: 0x4
 */
void XObj::ResetForNewGame()
{
}

/**
 * @note Address: 0x80044508
 * @note Size: 0x14
 */
void* CFileLogo::Alloc(u32 size)
{
	return (u8*)gGame.mMem1ArenaHi - size;
}

/**
 * @note Address: 0x8004451C
 * @note Size: 0x18
 */
void CFileLogo::Free()
{
	if (mBuffer != NULL) {
		mBuffer = NULL;
	}
}

/**
 * @note Address: 0x80044534
 * @note Size: 0x1C
 */
// Logo's init
extern "C" void ResetLogoFadeState(Logo* logo)
{
	logo->mState = 0;
	logo->ResetFade();
}

/**
 * @note Address: 0x80044550
 * @note Size: 0x33C
 */
// Logo::Update, reads the logos in, then fades the first in and out. it stays until a button
// other than HOME is pressed; the two logos behind it are skipped on the Wii
extern "C" void UpdateBootLogos(Logo* logo, u32 frames)
{
	switch (logo->mState) {
	case 1:
		logo->mState = 2;
	case 2: {
		char path[64];
		const char* suffix = "";
		sprintf(path, "tpl/logo%s%s.tpl", gGame.mAspectRatio == 1 ? "_w" : "", suffix);
		if (!logo->mFile.ReadEx(path, 0, 0)) {
			break;
		}
		logo->mTpl.Bind(logo->mFile.mBuffer);
		logo->mState = 3;
		logo->ResetFade();
	}
	case 3:
		if (logo->mAlpha < 1.0f && (logo->mAlpha += (1.0f / (0.5f * (f32)gRefreshRate)) * frames) > 1.0f) {
			logo->mAlpha = 1.0f;
		}
		if (logo->mAlpha != 1.0f) {
			break;
		}
		logo->mState = 4;
	case 4:
		logo->mTimer += frames;
		if (logo->mTimer >= (f32)gRefreshRate) {
			if ((logo->mAngle += (DEG2RAD * (180.0f / (f32)gRefreshRate)) * frames) >= 3.1415927f) {
				logo->mAngle = 0.0f;
			}
			BOOL done = logo->mTimer >= 20.0f * (f32)gRefreshRate
			              ? TRUE
			              : (gWiiPointer.mTrigger != 0 && !(gWiiPointer.mTrigger & ((u64)1 << RemoteHome)));
			if (!done) {
				break;
			}
		} else {
			break;
		}
		logo->mState = 5;
	case 5:
		if (logo->mAlpha > 0.0f && (logo->mAlpha -= (1.0f / (0.5f * (f32)gRefreshRate)) * frames) < 0.0f) {
			logo->mAlpha = 0.0f;
		}
		if (logo->mAlpha == 0.0f) {
			logo->mState = 6;
			logo->ResetFade();
		}
		break;
	case 6:
		logo->mState = 9;
	case 9:
		if (logo->mAlpha == 0.0f) {
			logo->mState = 12;
			logo->ResetFade();
			logo->mFile.Close();
			logo->mTpl.Close();
		}
		break;
	case 12:
		break;
	}
}

/**
 * @note Address: 0x8004488C
 * @note Size: 0x74C
 */
// NONMATCHING: float regalloc, as in the GameCube function: retail keeps width, height, x and y in
// f30, f31, f28, f29 for the first logo and in f31, f30, f29, f28 for the other two; this build in
// f31, f30, f29, f28 and f28, f29, f30, f31
// Logo::Draw, the current logo centred on the screen at its alpha
extern "C" void DrawCurrentLogo(Logo* logo)
{
	if (logo->IsActive() && ((CBase*)&gGame)->vf5C()) {
		if (logo->mAlpha > 0.0f) {
			SetGsChannelEnabled(&gGame.mGs, 0, 0);
			gGame.mGs.mCurRenderState->SetColorUpdate(GX_TRUE);
			gGame.mGs.mCurRenderState->SetAlphaUpdate(GX_FALSE);
			gGame.mGs.mCurRenderState->SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_LEQUAL, 255);
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			CTexObj tex;
			if (logo->mState == 3 || logo->mState == 4 || logo->mState == 5) {
				s32 index;
				switch (logo->mState) {
				case 3:
					index = 0;
					break;
				case 4:
				case 5:
					if (logo->mTimer < 5.0f * (f32)gRefreshRate) {
						index = 0;
					} else if (logo->mTimer < 10.0f * (f32)gRefreshRate) {
						index = 1;
					} else {
						index = !(logo->mTimer < 15.0f * (f32)gRefreshRate);
					}
					break;
				}
				logo->mTpl.mTex.Init(logo->GetDesc(index), 0);
				tex       = logo->mTpl.mTex;
				f32 w     = GXGetTexObjWidth(&tex);
				f32 h     = GXGetTexObjHeight(&tex);
				f32 x     = gPointerScreenWidth / 2;
				f32 y     = gPointerScreenHeight / 2;
				u32 color = (u32)(255.0f * logo->mAlpha) | 0xFFFFFF00;
				DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			}
			if (logo->mState == 6 || logo->mState == 7 || logo->mState == 8) {
				logo->mTpl.mTex.Init(logo->GetDesc(2), 0);
				tex       = logo->mTpl.mTex;
				f32 w     = GXGetTexObjWidth(&tex);
				f32 h     = GXGetTexObjHeight(&tex);
				f32 x     = gPointerScreenWidth / 2;
				f32 y     = h / 2.0f + 154.0f;
				u32 color = (u32)(255.0f * logo->mAlpha) | 0x0046FF00;
				DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			}
			if (logo->mState == 9 || logo->mState == 10 || logo->mState == 11) {
				logo->mTpl.mTex.Init(logo->GetDesc(3), 0);
				tex       = logo->mTpl.mTex;
				f32 w     = GXGetTexObjWidth(&tex);
				f32 h     = GXGetTexObjHeight(&tex);
				f32 x     = gPointerScreenWidth / 2;
				f32 y     = gPointerScreenHeight / 2;
				u32 color = (u32)(255.0f * logo->mAlpha) | 0xFFFFFF00;
				DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			}
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
		((CBase*)&gGame)->vf54();
	}
}
