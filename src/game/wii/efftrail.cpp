#include "Dolphin/vec.h"
#include "Dolphin/mtx.h"
#include "game/gmath.h"
#include "game/pointptcl.h"
#include "game/wii/cgs.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include "game/wii/xobj.h"
#include <math.h>

extern "C" void PSMTXMultVecArray(const Mtx m, const Vec* src, Vec* dst, u32 count);
extern "C" void fn_8002BBD8(CGs* gs, f32 x, f32 y);
extern "C" void fn_8002BBDC(CGs* gs);
extern s32 gPointerScreenWidth;
extern s32 gScreenSafeLeft, gScreenSafeRight, gScreenSafeTop, gScreenSafeBottom;

// NONMATCHING: retail keeps `angle` in a saved FPR across the sin/cos calls and walks the vertex
// array with a +0x18 pointer increment (uses f25-f31); this spelling rematerializes angle per call
// and indexes the array, same shape as the GameCube source.
/**
 * @note Address: 0x800F0100
 * @note Size: 0x45C
 */
extern "C" void DrawMenuRing(Mtx transform, f32 firstRadius, f32 secondRadius, u32 color, BOOL separate)
{
	Vec vertices[50];
	for (s32 i = 0; i < 25; i++) {
		f32 angle             = (TWO_PI / 24.0f) * i;
		vertices[2 * i + 1].x = -(secondRadius * std::sinf(angle));
		vertices[2 * i + 1].y = secondRadius * std::cosf(angle);
		vertices[2 * i + 1].z = 10.0f;
		vertices[2 * i].x     = -(firstRadius * std::sinf(angle));
		vertices[2 * i].y     = firstRadius * std::cosf(angle);
		vertices[2 * i].z     = 10.0f;
	}
	PSMTXMultVecArray(transform, vertices, vertices, 50);
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, vertices, color, 50);

	for (s32 i = 0; i < 25; i++) {
		f32 angle             = (TWO_PI / 24.0f) * i;
		vertices[2 * i + 1].x = -(firstRadius * std::sinf(angle));
		vertices[2 * i + 1].y = firstRadius * std::cosf(angle);
		vertices[2 * i + 1].z = 10.0f;
		vertices[2 * i].x     = -(firstRadius * std::sinf(angle));
		vertices[2 * i].y     = firstRadius * std::cosf(angle);
		vertices[2 * i].z     = 0.0f;
	}
	PSMTXMultVecArray(transform, vertices, vertices, 50);
	if (separate) {
		for (s32 i = 0; i < 48; i += 2)
			DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, &vertices[i], color, 4);
	} else {
		DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, vertices, color, 50);
	}

	for (s32 i = 0; i < 25; i++) {
		f32 angle             = (TWO_PI / 24.0f) * i;
		vertices[2 * i + 1].x = -(secondRadius * std::sinf(angle));
		vertices[2 * i + 1].y = secondRadius * std::cosf(angle);
		vertices[2 * i + 1].z = 0.0f;
		vertices[2 * i].x     = -(secondRadius * std::sinf(angle));
		vertices[2 * i].y     = secondRadius * std::cosf(angle);
		vertices[2 * i].z     = 10.0f;
	}
	PSMTXMultVecArray(transform, vertices, vertices, 50);
	if (separate) {
		for (s32 i = 0; i < 48; i += 2)
			DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, &vertices[i], color, 4);
	} else {
		DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, vertices, color, 50);
	}
}

static inline void DrawOXCross(CGs* gs, u32 color, f32 x, f32 y, f32 rx, f32 ry, f32 rz, f32 width, f32 length, f32 depth)
{
	DrawGsCube(gs, color, x, y, 50.0f, rx, ry, rz, width, length, depth);
	DrawGsCube(gs, color, x, y, 50.0f, rx, ry, rz, length, width, depth);
}

// NONMATCHING: reconstructed from the GameCube JP geometry path plus the Wii safe-area/pointer
// additions (gScreenSafe* bounds recenter, mMenuControls pointer-hint gate). Residual: FPR/GPR
// scheduling and temporary layout differ from retail throughout the two-draw loop.
/**
 * @note Address: 0x800F055C
 * @note Size: 0xBB4
 */
extern "C" void DrawOXMenu(WiiOXMenu* menu)
{
	if (!(menu->mFade > 0.0f))
		return;

	f32 screenX, screenY;
	if (gGame.GetStatusFlags(0x100)) {
		screenX = gPointerScreenWidth / 2;
		screenY = gScreenSafeBottom;
	} else {
		XObj* chr  = menu->mChr;
		Vec offset = { 0.0f, 0.0f, 0.0f };
		offset.y   = chr->GetJointWorldPos("neck").y - chr->GetPos()->y;
		Vec anchor = offset;
		menu->mChr->ProjectOffsetToScreen(&anchor, &screenX, &screenY, NULL, NULL);
	}
	// both marks must stay inside the safe screen area, otherwise the menu recenters
	for (s32 i = 0; i < 2; i++) {
		f32 testX = screenX + 1.0f * (90.0f * i - 45.0f);
		f32 testY = screenY - 70.0f * (f32)sin(2.268928) - 21.0f;
		if (testX < gScreenSafeLeft || testX > gScreenSafeRight || testY < gScreenSafeTop || testY > gScreenSafeBottom) {
			screenX = gPointerScreenWidth / 2;
			screenY = gScreenSafeBottom;
			break;
		}
	}
	f32 fade  = menu->mFade;
	f32 yBase = gGame.GetStatusFlags(0x100) ? screenY - 56.0f : screenY - 70.0f * std::sinf(2.268928f * fade) - 21.0f;
	SetGsChannelEnabled(&gGame.mGs, 0, 1);
	f32 openAngle = 2.0943952f * fade;
	for (s32 i = 0; i < 2; ++i) {
		WiiOXMenuMark& mark = menu->mMark[i];
		f32 bob             = 4.0f * std::sinf(mark.mSpin);
		f32 pitch           = 10.0f * std::sinf(mark.mTilt);
		f32 yaw             = 10.0f * std::sinf(0.5f * mark.mTilt);
		f32 roll            = 10.0f * std::sinf(0.7f * mark.mTilt);
		f32 size            = mark.mScale * std::sinf(openAngle);

		f32 x = gGame.GetStatusFlags(0x100) ? screenX + (90.0f * i - 45.0f) : screenX + 1.0f * (fade * (90.0f * i - 45.0f));
		if (gGame.mMenuControls & 2) {
			if (i == menu->mSelectedIndex && menu->mStarted > 0 && menu->mFade == 1.0f
			    && (!(gGame.mMenuControls & 1) || !gWiiPointer.mPointerValid)) {
				fn_8002BBD8(&gGame.mGs, x, yBase + 0.2f * 70.0f);
			} else {
				fn_8002BBDC(&gGame.mGs);
			}
		}
		f32 y = yBase + bob;
		Mtx translation, rotX, rotY, rotZ, transform;
		PSMTXTrans(translation, x, y, 50.0f);
		PSMTXRotRad(rotX, 'x', DEG2RAD * pitch);
		PSMTXRotRad(rotY, 'y', DEG2RAD * yaw);
		PSMTXRotRad(rotZ, 'z', DEG2RAD * roll);
		PSMTXConcat(translation, rotZ, transform);
		PSMTXConcat(transform, rotX, transform);
		PSMTXConcat(transform, rotY, transform);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_BACK);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LESS, GX_FALSE);
		f32 crossSize = size + 0.02f;
		BOOL sharp    = i == menu->mSelectedIndex
		          && (menu->_10 >= 0 || ((gGame.mMenuControls & 2) && (!(gGame.mMenuControls & 1) || !gWiiPointer.mPointerValid)));
		u32 edgeColor = sharp ? 0xffffff00 | (u32)(150.0f * fade) : 0x64646400 | (u32)(150.0f * fade);
		s32 mask      = 1 << i;
		if (!(menu->mCrossMask & mask)) {
			gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_FRONT);
			f32 radius = 74.0f * size;
			DrawMenuRing(transform, 1.05f * radius, 0.55f * radius, edgeColor, FALSE);
		} else {
			f32 radius = 74.0f * crossSize;
			DrawOXCross(&gGame.mGs, edgeColor, x, y, DEG2RAD * pitch, DEG2RAD * yaw, DEG2RAD * (45.0f + roll), 0.36f * radius,
			            2.0f * radius, 0.3f * (74.0f * size));
		}
		SetGsChannelEnabled(&gGame.mGs, 0, 1);
		gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_FRONT);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LESS, GX_TRUE);
		s32 r, g, b;
		if (!(menu->mCrossMask & mask)) {
			r = 255;
			g = 0;
			b = 0;
		} else {
			r = 0;
			g = 99;
			b = 209;
		}
		if (sharp) {
			r *= 0.8f + 0.1f * std::sinf(1.2f * mark.mSpin);
			g *= 0.8f + 0.1f * std::sinf(1.2f * mark.mSpin);
			b *= 0.8f + 0.1f * std::sinf(1.2f * mark.mSpin);
		} else {
			r = (s32)(r * 0.5f);
			g = (s32)(g * 0.5f);
			b = (s32)(b * 0.5f);
		}
		u32 color = ((u32)r << 24) | ((u32)g << 16) | ((u32)b << 8) | (u32)(255.0f * fade);
		if (!(menu->mCrossMask & mask)) {
			f32 radius = 74.0f * size;
			DrawMenuRing(transform, radius, 0.6f * radius, color, TRUE);
		} else {
			f32 radius = 74.0f * size;
			DrawOXCross(&gGame.mGs, color, x, y, DEG2RAD * pitch, DEG2RAD * yaw, DEG2RAD * (45.0f + roll), 0.3f * radius, 2.0f * radius,
			            0.3f * radius);
		}
	}
	SetGsChannelEnabled(&gGame.mGs, 0, 0);
	gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_BACK);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
}

/**
 * @note Address: 0x800F1110
 * @note Size: 0x250
 */
extern "C" void StartPointParticles(PointPtcl* ptcl, s32 type, s32 amount)
{
	ptcl->mType           = type;
	ptcl->display.mState  = 1;
	ptcl->display.mAmount = amount;
	switch (type) {
	case 0: // a negative amount draws in red
		ptcl->display.colors.mColor  = amount >= 0 ? 0xCECEFF00 : 0xFF000000;
		ptcl->display.colors.mColor2 = amount >= 0 ? 0xE037EE00 : 0xFF000000;
		ptcl->burst.mNum             = __fabsf(amount);
		if (ptcl->burst.mNum > 40)
			ptcl->burst.mNum = 40;
		for (s32 i = 0; i < ptcl->burst.mNum; ++i)
			ptcl->burst.mPtcls[i].mState = -1; // -1 draws texture 154
		break;
	case 2:
		ptcl->display.colors.mColor  = amount >= 0 ? 0xEFEBC500 : 0xFF000000;
		ptcl->display.colors.mColor2 = amount >= 0 ? 0xEDA83600 : 0xFF000000;
		ptcl->burst.mNum             = __fabsf(amount);
		if (ptcl->burst.mNum > 40)
			ptcl->burst.mNum = 40;
		for (s32 i = 0; i < ptcl->burst.mNum; ++i)
			ptcl->burst.mPtcls[i].mState = -2; // -2 draws texture 152
		break;
	case 3:
		ptcl->display.colors.mColor  = amount >= 0 ? 0xCECEFF00 : 0xFF000000;
		ptcl->display.colors.mColor2 = amount >= 0 ? 0x5959FF00 : 0xFF000000;
		ptcl->burst.mNum             = __fabsf(amount);
		if (ptcl->burst.mNum > 40)
			ptcl->burst.mNum = 40;
		for (s32 i = 0; i < ptcl->burst.mNum; ++i)
			ptcl->burst.mPtcls[i].mState = -3; // -3 draws texture 153
		break;
	default:
		ptcl->display.colors.mColor  = 0xFFFFFF00;
		ptcl->display.colors.mColor2 = 0xFFFFFF00;
		ptcl->burst.mNum             = 40;
		for (s32 i = 0; i < ptcl->burst.mNum; ++i)
			ptcl->burst.mPtcls[i].mState = 5; // a positive state is a polygon's sides
		break;
	}
}

// the suit-change spiral's pool, the GameCube's gPlayerSuitChangeEffect: xobj_chr.cpp
// models the same 0x258-byte object as SuitChangeTrail (scale-first records at +0x10).
// This file's functions index it the way the GameCube's spiral code does — an active
// flag, then two rings of ten records holding a position, a scale, a color and an angle.
struct SpiralTrailPoint {
	Vec position; // _00
	f32 scale;    // _0C
	u32 color;    // _10
	f32 angle;    // _14
};

struct SpiralTrail {
	s32 active;                    // _00
	SpiralTrailPoint rings[2][10]; // _04
	u8 _1E4[0x258 - 0x1E4];
};

// NONMATCHING: the GameCube source ported over verbatim reaches 95.9 %; retail keeps a
// second ring pointer for the scale sum (its offsets fold the +4 ring base in) and
// schedules the head's loads around the sin/cos calls a little differently.
/**
 * @note Address: 0x800F2EA4
 * @note Size: 0x29C
 */
extern "C" void UpdateSpiralTrail(SpiralTrail* trail, u32 frames)
{
	if (trail->active != 0) {
		f32 total = 0.0f;
		for (int i = 0; i < 2; i++) {
			SpiralTrailPoint* point = trail->rings[i];
			for (int j = 9; j > 0; j--) { // the ring shifts down one slot
				point[j].position.x = point[j - 1].position.x;
				point[j].position.y = point[j - 1].position.y;
				point[j].position.z = point[j - 1].position.z;
				point[j].scale      = point[j - 1].scale;
				point[j].angle      = point[j - 1].angle;
			}
			// the head shrinks and rises until nothing is left
			if ((point[0].scale -= 0.02f * frames) <= 0.0f)
				point[0].scale = 0.0f;
			point[0].angle += DEG2RAD * 9.0f * frames;
			f32 sine            = sin(point[0].angle);
			point[0].position.x = point[0].scale * (15.0f * ((1.0f - point[0].scale) * sine));
			f32 cosine          = cos(point[0].angle);
			point[0].position.z = point[0].scale * (15.0f * ((1.0f - point[0].scale) * cosine));
			point[0].position.y += 0.3f * frames;
			for (int j = 0; j < 10; j++)
				total += point[j].scale;
		}
		if (total == 0.0f)
			trail->active = 0;
	}
}
