#include "game/xobjchrstate.h"
#include "game/hsd_dobj.h"
#include "game/xobjmap.h"
#include "game/xpad.h"
#include "game/cgame.h"
#include "game/cjobjset.h"
#include "game/globals.h"
#include "game/pointptcl.h"
#include <stdlib.h>

STATIC_ASSERT(sizeof(SuitParticle) == 0x18);
STATIC_ASSERT(sizeof(SuitChangeEffect) == 0x1E4);

/**
 * @note Address: 0x80093C14
 * @note Size: 0x490
 */
extern "C" void DrawSpiralTrail(SuitChangeEffect* trail, const CVec& origin)
{
	if (!trail->active) {
		return;
	}

	gGame.mGs.mBillboardEnabled = 1;
	SetGsDepthMode(gGame.mGs.mCurRenderMode, TRUE, 3, FALSE);
	if (gGame.mGs.mCurRenderMode) {
		ApplyGsRenderMode(gGame.mGs.mCurRenderMode, FALSE);
	}
	if (gGame.mGs.mCurTevStage) {
		ApplyGsTevStage(gGame.mGs.mCurTevStage, FALSE);
	}
	if (gGame.mGs.mCurChannel) {
		ApplyGsChannel(gGame.mGs.mCurChannel, FALSE);
	}

	Vec top  = origin;
	f32 peak = 0.0f;
	for (s32 i = 0; i < 2; ++i) {
		for (s32 j = 0; j < 10; ++j) {
			SuitParticle& point = trail->rings[i][j];
			if (point.scale > peak) {
				peak = point.scale;
			}
			if (point.position.y > top.y) {
				top.y = point.position.y;
			}
		}
	}

	f32 size = 15.0f * peak;

	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	Vec outerRotation = { 0.0f, 0.0f, 0.0f };
	DrawGsCircle(&gGame.mGs, 32, 0xFFFFFF00 | (u32)(64.0f * peak), 0xFFFFFF00, top.x, top.y, top.z, size, size, outerRotation.x,
	             outerRotation.y, outerRotation.z);
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 2, 4, 15);
	Vec innerRotation = { 0.0f, 0.0f, 0.0f };
	DrawGsCircle(&gGame.mGs, 32, 0xFFFFFF00 | (u32)(255.0f * peak), 0xFFFFFF00, top.x, top.y, top.z, size, size, innerRotation.x,
	             innerRotation.y, innerRotation.z);

	for (s32 i = 0; i < 2; ++i) {
		for (s32 j = 0; j < 10; ++j) {
			SuitParticle& point = trail->rings[i][j];
			Vec pos             = origin + point.position;
			f32 radius          = 4.0f * point.scale;
			f32 angle           = 4.0f * point.angle;
			const u32 edge      = point.color;
			u32 center          = edge | (u8)(s32)(255.0f * point.scale);
			SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 2, 4, 15);
			Vec glowRotation = { 0.0f, 0.0f, 0.0f };
			glowRotation.z   = angle;
			DrawGsCircle(&gGame.mGs, 6, 0xFFFFFF20, 0xFFFFFF00, pos.x, pos.y, pos.z, 2.5f * radius, 2.5f * radius, glowRotation.x,
			             glowRotation.y, glowRotation.z);
			SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
			Vec coreRotation = { 0.0f, 0.0f, 0.0f };
			coreRotation.z   = angle;
			DrawGsCircle(&gGame.mGs, 3, center, edge, pos.x, pos.y, pos.z, radius, radius, coreRotation.x, coreRotation.y, coreRotation.z);
		}
	}
	gGame.mGs.mBillboardEnabled = 0;
}

/**
 * @note Address: 0x800940A4
 * @note Size: 0x298
 */
extern "C" void UpdateSpiralTrail(SuitChangeEffect* trail, u32 frames)
{
	if (trail->active != 0) {
		f32 total = 0.0f;
		for (int i = 0; i < 2; i++) {
			SuitParticle* point = trail->rings[i];
			for (int j = 9; j > 0; j--) {
				point[j].position.x = point[j - 1].position.x;
				point[j].position.y = point[j - 1].position.y;
				point[j].position.z = point[j - 1].position.z;
				point[j].scale      = point[j - 1].scale;
				point[j].angle      = point[j - 1].angle;
			}
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

/**
 * @note Address: 0x80095AD8
 * @note Size: 0x1F8
 */
extern "C" void StartPointParticles(PointPtcl* ptcl, s32 type, s32 amount)
{
	ptcl->mType           = type;
	ptcl->display.mState  = 1;
	ptcl->display.mAmount = amount;
	switch (ptcl->mType) {
	case 0:
		ptcl->display.colors.mColor  = amount >= 0 ? 0xCECEFF00 : 0xFF000000;
		ptcl->display.colors.mColor2 = amount >= 0 ? 0xE037EE00 : 0xFF000000;
		ptcl->burst.mNum             = __abs(amount);
		if (ptcl->burst.mNum > 40)
			ptcl->burst.mNum = 40;
		for (s32 i = 0; i < ptcl->burst.mNum; ++i)
			ptcl->burst.mPtcls[i].mState = -1;
		break;
	case 2:
		ptcl->display.colors.mColor  = amount >= 0 ? 0xEFEBC500 : 0xFF000000;
		ptcl->display.colors.mColor2 = amount >= 0 ? 0xEDA83600 : 0xFF000000;
		ptcl->burst.mNum             = __abs(amount);
		if (ptcl->burst.mNum > 40)
			ptcl->burst.mNum = 40;
		for (s32 i = 0; i < ptcl->burst.mNum; ++i)
			ptcl->burst.mPtcls[i].mState = -2;
		break;
	case 3:
		ptcl->display.colors.mColor  = amount >= 0 ? 0xCECEFF00 : 0xFF000000;
		ptcl->display.colors.mColor2 = amount >= 0 ? 0x5959FF00 : 0xFF000000;
		ptcl->burst.mNum             = __abs(amount);
		if (ptcl->burst.mNum > 40)
			ptcl->burst.mNum = 40;
		for (s32 i = 0; i < ptcl->burst.mNum; ++i)
			ptcl->burst.mPtcls[i].mState = -3;
		break;
	default:
		ptcl->display.colors.mColor  = 0xFFFFFF00;
		ptcl->display.colors.mColor2 = 0xFFFFFF00;
		ptcl->burst.mNum             = 40;
		for (s32 i = 0; i < ptcl->burst.mNum; ++i)
			ptcl->burst.mPtcls[i].mState = 5;
		break;
	}
}

extern "C" void UpdatePointParticleDisplay(PointPtcl* ptcl, u32 frames)
{
	PointDisplay& display = ptcl->display;
	PointBurst& burst     = ptcl->burst;
	switch (display.mState) {
	case 1: {
		s32 value      = abs(display.mAmount);
		display.mCount = 0;
		while (display.mCount < 10) {
			display.digits[display.mCount].glyph = value % 10;
			value /= 10;
			++display.mCount;
			if (!value)
				break;
		}
		if (ptcl->mType == 1) {
			display.digits[display.mCount].glyph = 76;
			++display.mCount;
		}
		for (s32 i = display.mCount - 1; i >= 0; --i)
			display.digits[display.mCount - 1 - i].fade = 0.15f * -i;
		display.mState = 2;
		burst.mState   = 1;
		for (s32 i = 0; i < burst.mNum; ++i) {
			PointPtclOne& particle = burst.mPtcls[i];
			u32 green              = 100 + rand() % 156;
			u32 red                = 100 + rand() % 156;
			u32 blue               = 100 + rand() % 156;
			particle.mColor        = (red << 24) | (green << 16) | (blue << 8);
			particle.mFade         = 1.0f + 0.02f * i;
			// Retail consumes a random angle before assigning the evenly spaced directions.
			particle.mAngle = DEG2RAD * (rand() % 360);
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
		break;
	}
	case 2: {
		s32 moving = 0;
		for (s32 i = display.mCount - 1; i >= 0; --i) {
			f32& fade = display.digits[i].fade;
			if (fade < 1.f) {
				fade += 0.08f * frames;
				if (fade > 1.f)
					fade = 1.f;
				++moving;
			}
		}
		if (!moving) {
			display.mState = 3;
			for (s32 i = display.mCount - 1; i >= 0; --i)
				display.digits[display.mCount - 1 - i].fade = 1.f + 0.15f * i;
#if defined(VERSION_PAL)
			display.holdFrames = 0.2f * gVideoFramesPerSecond;
#else
			display.holdFrames = 12.f;
#endif
		}
		break;
	}
	case 3:
		if (display.holdFrames > 0.f) {
			display.holdFrames -= frames;
			if (display.holdFrames <= 0.f)
				display.holdFrames = 0.f;
		} else {
			s32 moving = 0;
			for (s32 i = display.mCount - 1; i >= 0; --i) {
				f32& fade = display.digits[i].fade;
				if (fade > 0.f) {
					fade -= 0.08f * frames;
					if (fade < 0.f)
						fade = 0.f;
					++moving;
				}
			}
			if (!moving)
				display.mState = 0;
		}
		break;
	}
	if (burst.mState == 1) {
		s32 moving = 0;
		for (s32 i = 0; i < burst.mNum; ++i) {
			PointPtclOne& particle = burst.mPtcls[i];
			if (particle.mFade > 0.f) {
				particle.mFade -= 0.015f * frames;
				if (particle.mFade < 0.f)
					particle.mFade = 0.f;
				++moving;
			}
			particle.mAngle += particle.mAngularSpeed * frames;
		}
		if (!moving)
			burst.mState = 0;
	}
}

#if defined(VERSION_JP)
extern "C" void PSMTXMultVecArray(const Mtx m, const Vec* src, Vec* dst, u32 count);

static inline void DrawRingStrip(Mtx transform, u32 color, Vec* vertices, bool separate)
{
	PSMTXMultVecArray(transform, vertices, vertices, 50);
	CGs& gs = gGame.mGs;
	if (separate) {
		for (s32 i = 0; i < 48; i += 2)
			DrawGsTrianglePrimitive(&gs, GX_TRIANGLESTRIP, gs.mCamera->mViewMtx, &vertices[i], color, 4);
	} else {
		DrawGsTrianglePrimitive(&gs, GX_TRIANGLESTRIP, gs.mCamera->mViewMtx, vertices, color, 50);
	}
}

static inline void BuildRingStrip(Vec* vertices, f32 firstRadius, f32 firstZ, f32 secondRadius, f32 secondZ)
{
	for (s32 i = 0; i <= 24; i++) {
		f32 angle = (TWO_PI / 24.0f) * i;
		Vec& a    = vertices[2 * i];
		Vec& b    = vertices[2 * i + 1];
		b.x       = -(secondRadius * (f32)sin(angle));
		b.y       = secondRadius * (f32)cos(angle);
		b.z       = secondZ;
		a.x       = -(firstRadius * (f32)sin(angle));
		a.y       = firstRadius * (f32)cos(angle);
		a.z       = firstZ;
	}
}

extern "C" void DrawMenuRing(Mtx transform, f32 firstRadius, f32 secondRadius, u32 color, BOOL separate)
{
	Vec vertices[50];
	BuildRingStrip(vertices, firstRadius, 10.0f, secondRadius, 10.0f);
	DrawRingStrip(transform, color, vertices, false);
	BuildRingStrip(vertices, firstRadius, 0.0f, firstRadius, 10.0f);
	DrawRingStrip(transform, color, vertices, separate != FALSE);
	BuildRingStrip(vertices, secondRadius, 10.0f, secondRadius, 0.0f);
	DrawRingStrip(transform, color, vertices, separate != FALSE);
}

static inline void DrawOXCross(CGs* gs, u32 color, f32 x, f32 y, f32 rx, f32 ry, f32 rz, f32 width, f32 length, f32 depth)
{
	DrawGsCube(gs, color, x, y, 50.0f, rx, ry, rz, width, length, depth);
	DrawGsCube(gs, color, x, y, 50.0f, rx, ry, rz, length, width, depth);
}

static inline s32 PulseMarkColor(s32 channel, f32 spin)
{
	return channel * (0.8f + 0.1f * (f32)sin(1.2f * spin));
}

extern "C" const Vec sOXMenuAnchorOffset;
extern "C" const OXJointPath sOXMenuAnchorJointPath;
extern "C" void DrawOXMenu(OXMenu* menu)
{
	if (!(menu->mFade > 0.0f))
		return;

	f32 screenX, screenY;
	if (gGame.mStatus.mFlags & 0x100ULL) {
		screenX = 320.0f;
		screenY = 418.0f;
	} else {
		XObj* chr        = menu->mChr;
		Vec offset       = sOXMenuAnchorOffset;
		OXJointPath path = sOXMenuAnchorJointPath;
		offset.y         = GetEventJointWorldPosition(chr, 7, path.ids).y - chr->mMoveTranslate.y;
		Vec anchor       = offset;
		menu->mChr->ProjectOffsetToScreen(&anchor, &screenX, &screenY, NULL, NULL);
	}
	f32 fade  = menu->mFade;
	f32 yBase = (gGame.mStatus.mFlags & 0x100ULL) ? screenY - 56.0f : screenY - 70.0f * (f32)sin(2.268928f * fade) - 21.0f;
	SetGsChannelEnabled(&gGame.mGs, 0, 1);
	f32 openAngle    = 2.0943952f * fade;
	u32 edgeAlpha    = (u32)(150.0f * fade);
	u32 selectedEdge = 0xffffff00 | edgeAlpha, otherEdge = 0x64646400 | edgeAlpha;
	u32 alpha = (u32)(255.0f * fade);
	for (s32 i = 0; i < 2; ++i) {
		OXMenuMark& mark = menu->mMark[i];
		f32 bob          = 4.0f * (f32)sin(mark.mSpin);
		f32 pitch        = 10.0f * (f32)sin(mark.mTilt);
		f32 yaw          = 10.0f * (f32)sin(0.5f * mark.mTilt);
		f32 roll         = 10.0f * (f32)sin(0.7f * mark.mTilt);
		f32 size         = mark.mScale * (f32)sin(openAngle);

		f32 x = (gGame.mStatus.mFlags & 0x100ULL) ? screenX + (90.0f * i - 45.0f) : screenX + fade * (90.0f * i - 45.0f);
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
		SetGsCullMode(gGame.mGs.mCurRenderMode, 2);
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 1, 0);
		u32 edgeColor = i == menu->mSelectedIndex ? selectedEdge : otherEdge;
		s32 mask      = 1 << i;
		if (!(menu->mCrossMask & mask)) {
			SetGsCullMode(gGame.mGs.mCurRenderMode, 1);
			f32 radius = 74.0f * size;
			DrawMenuRing(transform, 1.05f * radius, 0.55f * radius, edgeColor, FALSE);
		} else {
			f32 radius = 74.0f * (size + 0.02f);
			DrawOXCross(&gGame.mGs, edgeColor, x, y, DEG2RAD * pitch, DEG2RAD * yaw, DEG2RAD * (45.0f + roll), 0.36f * radius,
			            2.0f * radius, 0.3f * (74.0f * size));
		}
		SetGsChannelEnabled(&gGame.mGs, 0, 1);
		SetGsCullMode(gGame.mGs.mCurRenderMode, 1);
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 1, 1);
		s32 r, g, b;
		if (!(menu->mCrossMask & mask)) {
			r = 255;
			g = b = 0;
		} else {
			r = 0;
			g = 99;
			b = 209;
		}
		if (i == menu->mSelectedIndex) {
			r = PulseMarkColor(r, mark.mSpin);
			g = PulseMarkColor(g, mark.mSpin);
			b = PulseMarkColor(b, mark.mSpin);
		} else {
			r = (s32)(r * 0.5f);
			g = (s32)(g * 0.5f);
			b = (s32)(b * 0.5f);
		}
		u32 color = ((u32)r << 24) | ((u32)g << 16) | ((u32)b << 8) | alpha;
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
	SetGsCullMode(gGame.mGs.mCurRenderMode, 2);
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
}
#else
#include "game/cgameutil.h"

extern "C" {
void SetJointMaterialColorsAndAlpha(HSD_JObj*, f32**, s32);
CVec GetEventJObjWorldPosition(CJObj*, HSD_JObj*);
extern u32 gOXMenuMarkDiffuseColors[2];
extern f32 gOXMenuLabelOutlineX[4], gOXMenuLabelOutlineY[4];
}

static inline void MakeOXTransform(Mtx out, f32 x, f32 y, f32 rx, f32 ry, f32 rz)
{
	Mtx move, pitch, yaw, roll;
	PSMTXTrans(move, x, y, 50.0f);
	PSMTXRotRad(pitch, 'x', rx);
	PSMTXRotRad(yaw, 'y', ry);
	PSMTXRotRad(roll, 'z', rz);
	Mtx tmp;
	PSMTXConcat(move, roll, tmp);
	PSMTXConcat(tmp, pitch, tmp);
	PSMTXConcat(tmp, yaw, out);
}

static inline void DrawOXModel(XObj& model, u32 diffuse, u32 ambient, f32& alpha)
{
	f32* args[] = { (f32*)&diffuse, (f32*)&ambient, &alpha };
	if (model.mJObj)
		HSD_JObjWalkTree(model.mJObj, SetJointMaterialColorsAndAlpha, args);
	model.vfAC();
	EndGsCamera(&gGame.mGs);
	CBase* game       = &gGame;
	gGame.mGs.mCamera = game->vf34();
	BeginGsCamera(&gGame);
}

struct OXDrawData {
	u8 prefix[0x88];
	OXJointPath path;
	Vec offset;
};
extern "C" const OXDrawData gOXMenuDrawData;

extern "C" void DrawOXMenu(OXMenu* menu)
{
	if (!(menu->mFade > 0.0f))
		return;
	f32 screenX, screenY;
	if (gGame.mStatus.mFlags & 0x100ULL) {
		screenX = 320.0f;
		screenY = 418.0f;
	} else {
		XObj* chr        = menu->mChr;
		Vec offset       = gOXMenuDrawData.offset;
		OXJointPath path = gOXMenuDrawData.path;
		offset.y         = GetEventJointWorldPosition(chr, 7, path.ids).y - chr->mMoveTranslate.y;
		Vec anchor       = offset;
		menu->mChr->ProjectOffsetToScreen(&anchor, &screenX, &screenY, NULL, NULL);
	}
	f32 fade  = menu->mFade;
	f32 baseY = (gGame.mStatus.mFlags & 0x100ULL) ? screenY - 56.0f : screenY - 70.0f * (f32)sin(2.268928f * fade) - 42.0f;
	SetGsChannelEnabled(&gGame.mGs, 0, 1);
	f32 openAngle = 2.0943952f * fade;
	u32 alpha = (u32)(255.0f * fade), textColor = 0xf0f0f000 | alpha;
	for (s32 i = 0; i < 2; ++i) {
		OXMenuMark& mark = menu->mMark[i];
		f32 bob          = 4.0f * (f32)sin(mark.mSpin);
		f32 rx           = DEG2RAD * (10.0f * (f32)sin(mark.mTilt));
		f32 ry           = DEG2RAD * (10.0f * (f32)sin(0.5f * mark.mTilt));
		f32 rz           = DEG2RAD * (10.0f * (f32)sin(0.7f * mark.mTilt));
		f32 size         = mark.mScale * (f32)sin(openAngle);
		f32 x = (gGame.mStatus.mFlags & 0x100ULL) ? screenX + (90.0f * i - 45.0f) : screenX + 1.2f * (fade * (90.0f * i - 45.0f));
		f32 y = baseY + bob;
		Mtx shape, label;
		MakeOXTransform(shape, x, y, rx, ry, rz);
		MakeOXTransform(label, x, y, 0, 0, 0);
		s32 kind = (menu->mCrossMask & (1 << i)) != 0;

		if (i == menu->mSelectedIndex) {
			menu->mMark[kind].mObj.vf12C(0, 255, 255, 255, 0);
			u8 pulse = (s32)(64.0f * (0.5f * (f32)sin(1.2f * mark.mSpin) + 0.5f));
			menu->mMark[kind].mObj.vf12C(1, pulse, pulse, pulse, 0);
		} else {
			menu->mMark[kind].mObj.vf12C(0, 175, 175, 175, 0);
			menu->mMark[kind].mObj.vf12C(1, 0, 0, 0, 0);
		}
		XObj& model = menu->mMark[kind].mObj;
		model.UpdateObject(1);
		InvalidateAll();
		f32 scaleXZ = 74.0f * size, scaleY = -74.0f * size;
		for (s32 j = 0; j < 8; ++j) {
			f32 direction = (TWO_PI / 8.0f) * j;
			f32 dy        = 2.0f * (f32)cos(direction);
			f32 dx        = 3.0f * (f32)sin(direction);
			Mtx outline, scale;
			MakeOXTransform(outline, x + dx, y + dy, rx, ry, rz);
			PSMTXScale(scale, scaleXZ, scaleY, scaleXZ);
			PSMTXConcat(outline, scale, outline);
			HSD_JObjCopyMtx(model.mJObj, outline);
			f32 opacity = 0.3f * menu->mFade;
			DrawOXModel(model, 0xffffff00, 0xffffff00, opacity);
		}
		Mtx scale;
		PSMTXScale(scale, scaleXZ, scaleY, scaleXZ);
		PSMTXConcat(shape, scale, shape);
		HSD_JObjCopyMtx(model.mJObj, shape);
		DrawOXModel(model, gOXMenuMarkDiffuseColors[kind], 0x40404000, menu->mFade);
		PSMTXConcat(label, scale, label);
		InvalidateAll();
		HSD_JObjCopyMtx(model.mJObj, label);
		HSD_JObjSetupMatrix(model.mJObj);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		SetGsCullMode(gGame.mGs.mCurRenderMode, 2);
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 1, 1);
		Vec textRot        = { rx, ry, rz };
		gGame.mGs.mTextRot = textRot;
		Vec pos            = GetEventJObjWorldPosition(&model, model.mJObj->child);
		f32 fontSize       = 2.0f * (30.0f * size * size);
		f32 left           = pos.x - 0.5f * MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x104 + kind), TRUE, fontSize);
		f32 top            = pos.y - 0.5f * fontSize;
		for (s32 j = 0; j < 4; ++j)
			DrawGsProportionalTextFormat(&gGame.mGs, alpha, GetLocalizedInterfaceText(0x104 + kind),
			                             2.0f + (left + gOXMenuLabelOutlineX[j]), 2.0f + (top + gOXMenuLabelOutlineY[j]), pos.z, fontSize,
			                             fontSize);
		DrawGsProportionalTextFormat(&gGame.mGs, textColor, GetLocalizedInterfaceText(0x104 + kind), left, top, pos.z, fontSize, fontSize);
	}
	SetGsChannelEnabled(&gGame.mGs, 0, 0);
	SetGsCullMode(gGame.mGs.mCurRenderMode, 2);
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
}
#endif

extern "C" void GXProject(f32, f32, f32, const Mtx, const f32*, const f32*, f32*, f32*, f32*);

static inline void ProjectPoint(const CCamera& camera, const Vec& pos, f32* x, f32* y)
{
	f32 sx, sy, sz;
	GXProject(pos.x, pos.y, pos.z, camera.mViewMtx, camera.mProjection, camera.mViewport, &sx, &sy, &sz);
	if (x)
		*x = sx;
#if defined(VERSION_PAL)
	if (y)
		*y = 448.f * sy / gGame.mGs.mScreen.xfbHeight;
#else
	if (y)
		*y = sy;
#endif
}

static inline CTexObj& PointTexture(CTexObj& texture, int id)
{
	CTpl& tpl = gGame.mTpl;
	return texture.Init(&((TPLPalette*)tpl.mBuffer)->descriptorArray[id - StatusTextureOffset], 0);
}

static inline void DrawPointQuad(CTexObj& tex, u32 top, u32 bottom, f32 x, f32 y, f32 w, f32 h, f32 rotation)
{
	DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, x, y, 0, w, h, 0, 0, rotation);
}

extern "C" void DrawPointParticleDisplay(PointPtcl* ptcl, f32 row)
{
	CMotionControlCamera& camera = gGame.mMotionControlCamera;
	if (camera.IsAction(4))
		return;
	PointDisplay& display = ptcl->display;
	PointBurst& burst     = ptcl->burst;
	XObjChr* player       = gGame.GetPlayer();
	CVec origin           = player->GetPos();
	Vec jointOffset       = { 0, 0, 0 };
	s32 path[]            = { 0, 1, 0, 1, 2, 0, 1 };
	jointOffset.y         = GetEventJointWorldPosition(player, 7, path).y - origin.y;
	Vec offset            = jointOffset;
	offset.y += 4.5f;
	Vec half = offset;
	half.y *= .5f;
	f32 x, y, centerX, centerY;
	ProjectPoint(camera.mCamera, origin + half, &centerX, &centerY);
	if (display.mAmount < 0) {
		ProjectPoint(camera.mCamera, origin + offset, &x, &y);
		y -= 19.8f * row;
	}
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
	if (burst.mState) {
		CTexObj& texture = gGame.mTpl.mTex;
		f32 radius       = 64;
		for (int i = 0; i < burst.mNum; ++i) {
			PointPtclOne& p = burst.mPtcls[i];
			if (!(p.mFade > 0 && p.mFade < 1))
				continue;
			if (display.mAmount >= 0) {
				x = centerX + 300 * (f32)sin(p.mAngle);
				y = centerY + 300 * (f32)cos(p.mAngle);
			}
			f32 phase = p.mFade;
			if (phase > 1)
				phase = 1;
			f32 alpha = sin(PI * p.mFade);
			if (alpha > 1)
				alpha = 1;
			f32 angle = PI * phase, arc = 1.5f * (f32)sin(angle);
			f32 px = x + (centerX - x) * (1 - (f32)sin(angle * .5f)), py = y + (centerY - y) * (1 - (f32)sin(angle * .5f));
			if (p.mState > 0 || display.mAmount < 0) {
				f32 a  = (2 * PI / burst.mNum) * i;
				f32 tx = x + 200 * (f32)sin(a), ty = y + 200 * (f32)cos(a);
				px = x + (tx - x) * (1 - (f32)sin(angle * .5f));
				py = y + (ty - y) * (1 - (f32)sin(angle * .5f));
			}
			f32 dx = p.mSpeed * (arc * (f32)sin(p.mAngle)), dy = p.mSpeed * (arc * (f32)cos(p.mAngle));
			f32 shrink = 1 - (f32)sin(angle * .5f), rotation = PI * p.mAngle;
			SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
			if (p.mState > 0) {
				u32 color = p.mColor | (u8)(s32)(128 * alpha);
				DrawGsCircle(&gGame.mGs, p.mState, color, color, px + dx, py + dy, 0, (2 + radius) - shrink * radius,
				             (2 + radius) - shrink * radius, 0, 0, rotation);
			} else if (p.mState < 0) {
				CTexObj tex;
				if (p.mState == -1) {
					tex = PointTexture(texture, 154);
					if (p.mLastScreenX == px + dx && p.mLastScreenY == py + dy)
						rotation = p.mLastAngle;
					else
						rotation = (f32)atan2(p.mLastScreenX - (px + dx), p.mLastScreenY - (py + dy)) - .52359879f;
				} else if (p.mState == -2) {
					tex      = PointTexture(texture, 152);
					rotation = 0;
				} else if (p.mState == -3) {
					tex      = PointTexture(texture, 153);
					rotation = 0;
				} else {
					continue;
				}
				radius    = 44;
				u32 color = display.mAmount >= 0 ? (0xffffff00 | (u32)(200 * alpha)) : (0xff000000 | (u32)(128 * alpha));
				SetGsTevMode(gGame.mGs.mCurTevStage, 0);
				f32 size = 46 - (shrink * .5f) * radius;
				DrawPointQuad(tex, color, color, px + dx, py + dy, size, size, rotation);
				SetGsTevMode(gGame.mGs.mCurTevStage, 4);
				p.mLastScreenX = px + dx;
				p.mLastScreenY = py + dy;
				p.mLastAngle   = rotation;
			}
		}
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	}
	if (display.mState && ptcl->mType != 1) {
		ProjectPoint(camera.mCamera, origin + offset, &x, &y);
		y -= 19.8f * row;
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		CTexObj& texture = gGame.mTpl.mTex;
		CTexObj icon = PointTexture(texture, 127), digits = PointTexture(texture, 125);
		PSMTXScale(digits.mMtx[1], 1, .1f, 1);
		digits.mMtxDirty = TRUE;
		for (int i = display.mCount - 1; i >= 0; --i) {
			const PointDigit& digit = display.digits[i];
			f32 fade                = digit.fade;
			if (!(fade > 0))
				continue;
			f32 px = 17.f * (display.mCount - 1 - i) + ((x + 8.5f) - display.mCount * 17.f * .5f);
			if (fade > 1)
				fade = 1;
			f32 angle = PI * fade;
			f32 rise = 8 * (f32)sin(angle), expand = 16 * (f32)cos(angle * .5f);
			if (display.mState == 3)
				expand = 0;
			u8 alpha = (u8)(s32)(255 * fade);
			u32 top = display.colors.mColor | alpha, bottom = display.colors.mColor2 | alpha;
			if (digit.glyph == 76) {
				DrawPointQuad(icon, alpha, alpha, px, y - rise, 1.2f * (19 + expand), 24 + expand, 0);
				DrawPointQuad(icon, top, bottom, px, y - rise, 19 + expand, .8f * (24 + expand), 0);
			} else {
				PSMTXTrans(digits.mMtx[0], 0, .1f * digit.glyph, 0);
				digits.mMtxDirty = TRUE;
				DrawPointQuad(digits, top, bottom, px, y - rise, 19 + expand, 24 + expand, 0);
			}
		}
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	}
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
}

// fades in and out over 0.3 seconds
#if defined(VERSION_PAL)
#define OX_FADE_STEP (1.0f / (0.3f * gVideoFramesPerSecond))
#else
#define OX_FADE_STEP (1.0f / 18.0f)
#endif

extern "C" {
void ShowOXCircle(XObjOX* object);
void ShowOXCross(XObjOX* object);
}

/**
 * @note Address: 0x80096B90
 * @note Size: 0x2A8
 */
extern "C" void UpdateOXMenuTransition(OXMenu* menu, u32 frames)
{
	if (menu->mStarted > 0) {
		if (menu->mFade < 1.0f) {
			f32 fade    = menu->mFade + OX_FADE_STEP * frames;
			menu->mFade = fade;
			if (fade >= 1.0f)
				menu->mFade = 1.0f;
		}
	} else if (menu->mStarted < 0) {
		if (menu->mFade > 0.0f) {
			f32 fade    = menu->mFade - OX_FADE_STEP * frames;
			menu->mFade = fade;
			if (fade <= 0.0f)
				menu->mFade = 0.0f;
		}
		if (0.0f == menu->mFade) {
			s32 anim = menu->mChr->mAnimId;
			if (anim != 0x78 && anim != 0xEF && anim != 0x13B) {
				menu->mStarted = 0;
				gGame.mStatus.mFlags &= ~0x800ULL;
				menu->mChr->SetRotateY(menu->mStartRot, DEG2RAD * 12.0f);
				XObjChr* chr = gGame.GetPlayer();
				chr->mPad.Update(0);
			}
		}
	}
	if (menu->mFade > 0.0f) {
		f32 spin = DEG2RAD * 8.0f * frames;
		f32 tilt = DEG2RAD * 3.0f * frames;
		f32 grow = 0.05f * frames;
		for (s32 i = 0; i < 2; ++i) {
			OXMenuMark* mark = &menu->mMark[i];
			mark->mSpin += spin;
			mark->mTilt += tilt;
			if (i == menu->mSelectedIndex) {
				f32 scale    = mark->mScale + grow;
				mark->mScale = scale;
				if (scale > 0.8f)
					mark->mScale = 0.8f;
			} else {
				f32 scale    = mark->mScale - grow;
				mark->mScale = scale;
				if (scale < 0.5f)
					mark->mScale = 0.5f;
			}
#if !defined(VERSION_JP)
			mark->mObj.UpdateObject(frames);
#endif
		}
	}
}

// OXMenu's input, left and right pick, a confirms

/**
 * @note Address: 0x80096E38
 * @note Size: 0x2AC
 */
extern "C" void HandleOXMenuInput(OXMenu* menu, CPad* pad)
{
	if (menu->mStarted > 0) {
		if (!(gGame.mStatus.mFlags & 0x100ULL)) {
			menu->mChr->SetRotateY(PI + gGame.mMotionControlCamera.mCamera.mAngle.y, DEG2RAD * 8.0f);
		}
		u32 next     = (gGame.mStatus.mFlags & 0x80000000ULL) ? 0x80000 : 0x80002;
		u32 previous = (gGame.mStatus.mFlags & 0x80000000ULL) ? 0x40000 : 0x40001;
		if (pad->mPad->mRepeat & next) {
			if (menu->mSelectedIndex + 1 < 2) {
				menu->mSelectedIndex = menu->mSelectedIndex + 1;
				PlayGameSound(&gGame.mSound, 0x87, 255);
			}
		} else if (pad->mPad->mRepeat & previous) {
			if (menu->mSelectedIndex - 1 >= 0) {
				menu->mSelectedIndex = menu->mSelectedIndex - 1;
				PlayGameSound(&gGame.mSound, 0x87, 255);
			}
		}
		f32 angle             = menu->mSelectedIndex == 0 ? -(DEG2RAD * 20.0f) : DEG2RAD * 20.0f;
		XObjChr* chr          = (XObjChr*)menu->mChr;
		chr->mNeckRot.mMode   = 1;
		chr->mNeckRot.mTarget = angle;
		if (!(gGame.mStatus.mFlags & 0x20ULL) && 1.0f == menu->mFade && (pad->mPad->mTrigger & 0x100)) {
			menu->mStarted = -1;
			PlayGameSound(&gGame.mSound, 0x88, 255);
			menu->mConfirmedIndex = menu->mSelectedIndex;
			menu->mChr->SetAnim(0x78, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			if (!(menu->mCrossMask & (1U << menu->mConfirmedIndex))) {
				ShowOXCircle(&((XObjChr*)menu->mChr)->mOXObj);
			} else {
				ShowOXCross(&((XObjChr*)menu->mChr)->mOXObj);
			}
			chr                   = (XObjChr*)menu->mChr;
			chr->mNeckRot.mMode   = 2;
			chr->mNeckRot.mTarget = 0.0f;
			chr->mNeckRot.mLookAt = NULL;
		}
	}
}
