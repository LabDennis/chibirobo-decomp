// The robo eyes screen (the scope chibi robo looks through) and the dust particles: the part of
// the GameCube gxb.cpp that follows the wipes. Retail has this unit and wii/gxb.cpp in one file,
// CScrnRoboeyes::Update expands CScrnWipe::Update from there.
//
// Where the GameCube source halves with * 0.5f and folds its absolute values into branches, the
// Wii code divides by 2.0f (the operand order of the multiply shows it) and has fabs, written
// __fabsf here as in wii/gxb.cpp.
#include "types.h"
#include "game/wii/cgame.h"
#include "game/wii/cgs.h"
#include "game/wii/cpadact.h"
#include "game/wii/title.h"
#include "game/coordinfo.h"
#include "game/cptcl.h"
#include "game/cscrn.h"
#include "game/ctpl.h"
#include "game/globals.h"
#include "game/gmath.h"
#include <math.h>
extern "C" {
#include "Dolphin/mtx.h"
}
#include "JSystem/JMath/Inline.h"
#include "arithmetic.h"

// the Wii lays a file's functions out backwards, so they are written here from the highest
// address down

// the screen layout globals wii/init.cpp fills in from the console's settings
extern s32 gScreenLeft;
extern s32 gScreenRight;
extern s32 gScreenTop;
extern s32 gScreenBottom;
extern s32 gPointerScreenWidth;
extern s32 gPointerScreenHeight;

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

/**
 * @note Address: 0x8004327C
 * @note Size: 0x10
 */
void CPtclOne::Init()
{
	mActive = false;
	mTex    = NULL;
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

// moves a position by an offset passed by value. Retail's add is nw4r::math::VEC3Add, its three
// work registers show; nw4r/math.h does not compile under the game's include path (<cstdarg>),
// so the paired-single PSVECAdd the other Wii files use stands in and the two Updates differ in
// those registers
static inline void Move(CVec& pos, Vec offset)
{
	JMathInlineVEC::PSVECAdd(&pos, &offset, &pos);
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
