#include "Dolphin/vec.h"
#include "Dolphin/mtx.h"
#include "game/gmath.h"
#include "game/pointptcl.h"
#include "game/wii/cgs.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include "game/wii/xobj.h"
#include <math.h>
#include <stdlib.h>

extern "C" void PSMTXMultVecArray(const Mtx m, const Vec* src, Vec* dst, u32 count);
extern "C" void fn_8002BBD8(CGs* gs, f32 x, f32 y);
extern "C" void fn_8002BBDC(CGs* gs);
extern s32 gPointerScreenWidth;
extern s32 gScreenSafeLeft, gScreenSafeRight, gScreenSafeTop, gScreenSafeBottom;
extern s32 gRefreshRate;

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

// NONMATCHING: retail unrolls the two stagger-fade loops 8x and schedules the -i neg/xoris/lfd
// sequence differently inside the unroll (plus sdata2 pool layout); the state machine, digit
// decomposition and burst initialization match the GameCube source shape. Score ~83.4 %.
/**
 * @note Address: 0x800F1360
 * @note Size: 0xAF8
 */
extern "C" void UpdatePointParticleDisplay(PointPtcl* ptcl, u32 frames)
{
	PointDisplay& display = ptcl->display;
	PointBurst& burst     = ptcl->burst;
	if (display.mState == 1) { // build the digit strip
		s32 value      = __fabsf(display.mAmount);
		display.mCount = 0;
		while (display.mCount < 10) {
			display.digits[display.mCount].glyph = value % 10;
			value /= 10;
			++display.mCount;
			if (!value)
				break;
		}
		if (ptcl->mType == 1) {
			display.digits[display.mCount].glyph = 76; // glyph 76 is the icon
			++display.mCount;
		}
		for (s32 i = display.mCount - 1; i >= 0; --i)
			display.digits[display.mCount - 1 - i].fade = 0.15f * -i; // the digits come in staggered
		display.mState = 2;
		burst.mState   = 1;
		for (s32 i = 0; i < burst.mNum; ++i) {
			PointPtclOne& particle = burst.mPtcls[i];
			particle.mColor        = (100 + rand() % 156) << 24 | (100 + rand() % 156) << 16 | (100 + rand() % 156) << 8;
			particle.mFade         = 1.0f + 0.02f * i;
			// Retail consumes a random angle before assigning the evenly spaced directions, and stores the fade a second time.
			particle.mAngle = DEG2RAD * (rand() % 360);
			particle.mFade  = 1.0f + 0.02f * i;
			if (display.mAmount >= 0) {
				particle.mAngularSpeed = DEG2RAD * ((3000 - rand() % 6000) / 500.f);
				particle.mSpeed        = -(10000 + rand() % 10000) / 200.f;
			} else {
				particle.mAngularSpeed = DEG2RAD * ((3000 - rand() % 6000) / 1000.f);
				particle.mSpeed        = (10000 + rand() % 10000) / 200.f;
			}
			particle.mAngle     = DEG2RAD * ((360 / burst.mNum) * i);
			particle.mLastAngle = 0;
		}
	} else if (display.mState == 2) { // fading in
		s32 moving = 0;
		for (s32 i = display.mCount - 1; i >= 0; --i) {
			f32& fade = display.digits[i].fade;
			if (fade < 1.f) {
				if ((fade += 0.08f * frames) > 1.f)
					fade = 1.f;
				++moving;
			}
		}
		if (!moving) {
			display.mState = 3;
			for (s32 i = display.mCount - 1; i >= 0; --i)
				display.digits[display.mCount - 1 - i].fade = 1.f + 0.15f * i;
			display.holdFrames = 0.2f * gRefreshRate;
		}
	} else if (display.mState == 3) { // hold, then fade out
		if (display.holdFrames > 0.f) {
			if ((display.holdFrames -= frames) <= 0.f)
				display.holdFrames = 0.f;
		} else {
			s32 moving = 0;
			for (s32 i = display.mCount - 1; i >= 0; --i) {
				f32& fade = display.digits[i].fade;
				if (fade > 0.f) {
					if ((fade -= 0.08f * frames) < 0.f)
						fade = 0.f;
					++moving;
				}
			}
			if (!moving)
				display.mState = 0;
		}
	}
	if (burst.mState == 1) {
		s32 moving = 0;
		for (s32 i = 0; i < burst.mNum; ++i) {
			PointPtclOne& particle = burst.mPtcls[i];
			if (particle.mFade > 0.f) {
				if ((particle.mFade -= 0.015f * frames) < 0.f)
					particle.mFade = 0.f;
				++moving;
			}
			particle.mAngle += particle.mAngularSpeed * frames;
		}
		if (!moving)
			burst.mState = 0;
	}
}

static inline void ProjectPoint(const CCamera& camera, const Vec& pos, f32* x, f32* y)
{
	f32 sx, sy, sz;
	GXProject(pos.x, pos.y, pos.z, camera.mViewMtx, &camera.mIsOrtho, camera.mViewport, &sx, &sy, &sz);
	if (x)
		*x = sx * camera.mScale;
	if (y)
		*y = sy;
}

static inline CTexObj& PointTexture(CTexObj& texture, int id)
{
	texture.Init(&gGame.mTpl.mPalette->descriptorArray[id], 0);
	return texture;
}

static inline void DrawPointQuad(CTexObj& tex, u32 top, u32 bottom, f32 x, f32 y, f32 w, f32 h, f32 rotation)
{
	DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, x, y, 0, w, h, 0, 0, rotation);
}

// NONMATCHING: identical call set and instruction count (990 insns, same 11 sin/4 cos calls);
// the wall is register allocation: retail pins the gGame/camera/display/burst bases in callee-saved
// GPRs across the whole function, so the allocator spills the f64 conversion magic and float
// constants to eight stack slots (frame 0x5d0 vs 0x5b0), rematerializes 8.5f as 17*0.5 in an fmadds,
// and re-truncates the u8 alpha at every color argument; this build keeps the constants in FPRs and
// CSEs the truncation instead.
/**
 * @note Address: 0x800F1E58
 * @note Size: 0xE9C
 */
extern "C" void DrawPointParticleDisplay(PointPtcl* ptcl, f32 row)
{
	CMotionControlCamera& camera = gGame.mMotionControlCamera;
	if (camera.IsAction(4)) // that camera action hides the points
		return;
	PointDisplay& display = ptcl->display;
	PointBurst& burst     = ptcl->burst;
	XObj* player          = gGame.GetPlayer();
	CVec origin           = player->GetWorldPos();
	Vec jointOffset       = { 0, 0, 0 };
	jointOffset.y         = player->GetJointWorldPos("neck").y - origin.y;
	Vec offset            = jointOffset;
	offset.y += 4.5f;
	Vec half = offset;
	half.y *= .5f;
	f32 x, y, centerX, centerY;
	ProjectPoint(camera, origin + half, &centerX, &centerY);
	if (display.mAmount < 0) {
		ProjectPoint(camera, origin + offset, &x, &y);
		y -= 19.8f * row;
	}
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
	if (burst.mState) {
		f32 radius = 64;
		for (int i = 0; i < burst.mNum; ++i) {
			PointPtclOne& p = burst.mPtcls[i];
			if (!(p.mFade > 0 && p.mFade < 1))
				continue;
			if (display.mAmount >= 0) {
				x = centerX + 300 * std::sinf(p.mAngle);
				y = centerY + 300 * std::cosf(p.mAngle);
			}
			f32 phase = p.mFade;
			if (phase > 1)
				phase = 1;
			f32 alpha = std::sinf(PI * p.mFade); // strongest halfway through the fade
			if (alpha > 1)
				alpha = 1;
			f32 angle = PI * phase, arc = 1.5f * std::sinf(angle);
			f32 px = x + (centerX - x) * (1 - std::sinf(angle / 2)), py = y + (centerY - y) * (1 - std::sinf(angle / 2));
			if (p.mState > 0 || display.mAmount < 0) {
				f32 a  = (2 * PI / burst.mNum) * i;
				f32 tx = x + 200 * std::sinf(a), ty = y + 200 * std::cosf(a);
				px = x + (tx - x) * (1 - std::sinf(angle / 2));
				py = y + (ty - y) * (1 - std::sinf(angle / 2));
			}
			f32 dx = p.mSpeed * (arc * std::sinf(p.mAngle)), dy = p.mSpeed * (arc * std::cosf(p.mAngle));
			f32 shrink = 1 - std::sinf(angle / 2), rotation = PI * p.mAngle;
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			if (p.mState > 0) { // a polygon with mState sides
				u32 color = p.mColor | (u8)(s32)(128 * alpha);
				DrawGsCircle(&gGame.mGs, p.mState, color, color, px + dx, py + dy, 0, (2 + radius) - shrink * radius,
				             (2 + radius) - shrink * radius, 0, 0, rotation);
			} else if (p.mState < 0) {
				CTexObj tex;
				if (p.mState == -1) { // texture 155, turned toward where it was last frame
					tex = PointTexture(gGame.mTpl.mTex, 155);
					if (p.mLastScreenX == px + dx && p.mLastScreenY == py + dy)
						rotation = p.mLastAngle;
					else
						rotation = (f32)atan2(p.mLastScreenX - (px + dx), p.mLastScreenY - (py + dy)) - .52359879f;
				} else if (p.mState == -2) {
					tex      = PointTexture(gGame.mTpl.mTex, 153);
					rotation = 0;
				} else if (p.mState == -3) {
					tex      = PointTexture(gGame.mTpl.mTex, 154);
					rotation = 0;
				}
				shrink /= 2;
				radius    = 44;
				u32 color = display.mAmount >= 0 ? (0xffffff00 | (u32)(200 * alpha)) : (0xff000000 | (u32)(128 * alpha));
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				f32 size = (2 + radius) - shrink * radius;
				DrawPointQuad(tex, color, color, px + dx, py + dy, size, size, rotation);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
				p.mLastScreenX = px + dx;
				p.mLastScreenY = py + dy;
				p.mLastAngle   = rotation;
			}
		}
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	}
	if (display.mState && ptcl->mType != 1) {
		ProjectPoint(camera, origin + offset, &x, &y);
		y -= 19.8f * row;
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		CTexObj& texture = gGame.mTpl.mTex;
		CTexObj icon = PointTexture(texture, 128), digits = PointTexture(texture, 126);
		// the digit atlas is the digits 0-9 in a column
		PSMTXScale(digits.mTexMtx.mMtx[2], 1, .1f, 1);
		digits.mTexMtx.mDirty = TRUE;
		for (int i = display.mCount - 1; i >= 0; --i) {
			const PointDigit& digit = display.digits[i];
			if (!(digit.fade > 0))
				continue;
			f32 px = 17.f * (display.mCount - 1 - i) + ((x + 8.5f) - display.mCount * 17.f / 2), py = y;
			f32 fade = digit.fade;
			if (fade > 1)
				fade = 1;
			f32 angle = PI * fade;
			f32 rise = 8 * std::sinf(angle), expand = 16 * std::cosf(angle / 2);
			if (display.mState == 3)
				expand = 0;
			u8 alpha = (u8)(s32)(255 * fade);
			u32 top = display.colors.mColor | alpha, bottom = display.colors.mColor2 | alpha;
			if (digit.glyph == 76) { // the icon
				DrawPointQuad(icon, alpha, alpha, px, py - rise, 1.2f * (19 + expand), 24 + expand, 0);
				DrawPointQuad(icon, top, bottom, px, py - rise, 19 + expand, .8f * (24 + expand), 0);
			} else {
				PSMTXTrans(digits.mTexMtx.mMtx[0], 0, .1f * digit.glyph, 0);
				digits.mTexMtx.mDirty = TRUE;
				DrawPointQuad(digits, top, bottom, px, py - rise, 19 + expand, 24 + expand, 0);
			}
		}
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	}
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
}

// the suit-change spiral's pool, the GameCube's gPlayerSuitChangeEffect: xobj_chr.cpp
// models the same 0x258-byte object as SuitChangeTrail (scale-first records at +0x10).
// This file's functions index it the way the GameCube's spiral code does -- an active
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

// the suit-change spiral's reset, the GameCube's UpdatePlayerSuitChange case-1 block
// factored into its own function; xobj_chr.cpp calls it as fn_800F2CF4.
/**
 * @note Address: 0x800F2CF4
 * @note Size: 0x1B0
 */
extern "C" void fn_800F2CF4(SpiralTrail* trail)
{
	trail->active = TRUE;
	for (s32 i = 0; i < 2; ++i) {
		for (s32 j = 0; j < 10; ++j) {
			Vec position                = { 0.0f, 0.5f, 0.0f };
			trail->rings[i][j].position = position;
			trail->rings[i][j].scale    = 0.0f;
			trail->rings[i][j].color
			    = ((u32)(rand() % 156 + 100) << 24) | ((u32)(rand() % 156 + 100) << 16) | ((u32)(rand() % 156 + 100) << 8);
			trail->rings[i][j].angle = 0.0f;
		}
	}
	trail->rings[0][0].scale = 1.0f;
	trail->rings[0][0].angle = DEG2RAD * (rand() % 360);
	trail->rings[1][0].scale = 1.0f;
	trail->rings[1][0].angle = PI + trail->rings[0][0].angle;
}

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

// NONMATCHING (99.6 %): the GameCube source ported over reaches 97.4 % verbatim; writing
// trail->rings[i][j] member accesses instead of a `point` local gives retail's ring-base
// induction pointer. Rest is scheduling: f27/f28 (radius/angle) and r21-r23 renumber
// differently, the `center | alpha` or has its operands swapped, and the unnamed CVec +
// by-value Vec temporaries sit at other stack slots (the same class of wall as
// UpdateSpiralTrail and the OX-menu draws in this file).
// the GameCube's DrawSpiralTrail; on the Wii the render state is CGsRenderState and the
// SetupAll work is the three conditional Apply calls like the other draw units write them.
/**
 * @note Address: 0x800F3140
 * @note Size: 0x4A8
 */
extern "C" void DrawSpiralTrail(SpiralTrail* trail, const CVec& origin)
{
	if (!trail->active) {
		return;
	}

	gGame.mGs.mBillboardEnabled = 1;
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
	if (gGame.mGs.mCurRenderState != NULL) {
		gGame.mGs.mCurRenderState->Apply();
	}
	if (gGame.mGs.mCurTevStage != NULL) {
		gGame.mGs.mCurTevStage->Apply();
	}
	if (gGame.mGs.mCurChannel != NULL) {
		gGame.mGs.mCurChannel->Apply();
	}

	Vec top  = origin;
	f32 peak = 0.0f;
	for (s32 i = 0; i < 2; ++i) {
		for (s32 j = 0; j < 10; ++j) {
			if (trail->rings[i][j].scale > peak) {
				peak = trail->rings[i][j].scale;
			}
			if (trail->rings[i][j].position.y > top.y) {
				top.y = trail->rings[i][j].position.y;
			}
		}
	}

	f32 size = 15.0f * peak; // the halo at the trail's top

	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	Vec outerRotation = { 0.0f, 0.0f, 0.0f };
	DrawGsCircle(&gGame.mGs, 32, 0xFFFFFF00 | (u32)(64.0f * peak), 0xFFFFFF00, top.x, top.y, top.z, size, size, outerRotation.x,
	             outerRotation.y, outerRotation.z);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCCOL, GX_BL_SRCALPHA, GX_LO_SET);
	Vec innerRotation = { 0.0f, 0.0f, 0.0f };
	DrawGsCircle(&gGame.mGs, 32, 0xFFFFFF00 | (u32)(255.0f * peak), 0xFFFFFF00, top.x, top.y, top.z, size, size, innerRotation.x,
	             innerRotation.y, innerRotation.z);

	for (s32 i = 0; i < 2; ++i) {
		for (s32 j = 0; j < 10; ++j) {
			// each point: a wide glow disc under a solid core
			Vec pos        = origin + trail->rings[i][j].position;
			f32 angle      = trail->rings[i][j].angle;
			f32 radius     = 4.0f * trail->rings[i][j].scale;
			const u32 edge = trail->rings[i][j].color;
			u32 center     = edge;
			center |= (u8)(s32)(255.0f * trail->rings[i][j].scale);
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCCOL, GX_BL_SRCALPHA, GX_LO_SET);
			Vec glowRotation = { 0.0f, 0.0f, 0.0f };
			glowRotation.z   = 4.0f * angle;
			DrawGsCircle(&gGame.mGs, 6, 0xFFFFFF20, 0xFFFFFF00, pos.x, pos.y, pos.z, 2.5f * radius, 2.5f * radius, glowRotation.x,
			             glowRotation.y, glowRotation.z);
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			Vec coreRotation = { 0.0f, 0.0f, 0.0f };
			coreRotation.z   = 4.0f * angle;
			DrawGsCircle(&gGame.mGs, 3, center, edge, pos.x, pos.y, pos.z, radius, radius, coreRotation.x, coreRotation.y, coreRotation.z);
		}
	}
	gGame.mGs.mBillboardEnabled = 0;
}
