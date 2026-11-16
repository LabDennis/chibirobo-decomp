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

// the screen layout globals wii/init.cpp fills in from the console's settings
extern s32 gRefreshRate;
extern u32 gScreenLeft;
extern u32 gScreenRight;
extern u32 gScreenTop;
extern u32 gScreenBottom;
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
