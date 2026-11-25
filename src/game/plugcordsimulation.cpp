#include "game/xchr.h"
#include "game/cgame.h"
#include "game/gmath.h"
#include "game/cvec.h"

extern "C" {
void ClearWireEffect(XObjWireEff* wire);
void ConfigurePlugWireEffect(XObjWireEff* wire, s32 anim, s32 effect, f32 rate, u32 color);
}

static inline void PinTail(XObjPlugCord* cord, XObjPlugCordPoint* points, s32 end)
{
	XObjPlugCordPoint& point = points[cord->mCount - 3 + end];
	if (cord->mEndSource[end])
		point.mPos.Set(*cord->mEndSource[end]);
	else if (cord->mEndHeight[end])
		point.mPos.y = *cord->mEndHeight[end];
}
static inline void PinCordEnds(XObjPlugCord* cord, XObjPlugCordPoint* points)
{
	if (cord->mEndSource[0]) {
		points[0].mPos.Set(*cord->mEndSource[0]);
		points[0].mTarget = points[0].mPos;
	} else if (cord->mEndHeight[0]) {
		points[0].mPos.y = points[0].mTarget.y = *cord->mEndHeight[0];
	}
	PinTail(cord, points, 1);
	PinTail(cord, points, 2);
}

static inline void ResetSparks(XObjPlugCordNodes& nodes, u32 color, s32 mode, s32 count, s32 enabled)
{
	nodes.mColor = color;
	nodes.mMode  = mode;
	nodes.mCount = count;
	for (u32 i = 0; i < ARRAY_SIZE(nodes.mNode); ++i)
		nodes.mNode[i].mState = 0;
	nodes.mEnabled = enabled;
}

extern "C" void UpdatePlugCordSimulation(XObjPlugCord* cord, u32 frames)
{
	XObjPlugCordPoint* points = cord->mPoints;
	Vec probe                 = GetPlugCordPointPosition(cord, 18);
	probe.y                   = gGame.GetPlayer()->mMoveTranslate.y;
	f32 floorHeight           = FindPlugCordFloor(cord, &probe, -1, 0);
	Vec anchor;
	GetPlayerPlugCordAnchorTransform(&gGame.GetPlayer()->mPlug, &anchor, NULL, NULL);
	if (floorHeight > anchor.y)
		floorHeight = gGame.GetPlayer()->mFloorHit.mHeight;
	const f32 bounceHeight = 0.2f + floorHeight;
	for (u32 frame = 0; frame < frames; ++frame) {
		const f32 gravity = 0.25f;
		for (s32 i = 0; i < cord->mCount; ++i) {
			if (i < 4)
				points[i].mPos.y += 0.0049f; // the near ends drift up
			else if (i < 10)
				points[i].mPos.y += 0.98f * -gravity * 0.5f; // half gravity on the middle
			else
				points[i].mPos.y += 0.98f * -gravity;
		}
		for (s32 pass = 0; pass < 10; ++pass) { // ten relaxation passes
			PinCordEnds(cord, points);
			for (s32 i = 0; i < cord->mCount - 1; ++i) {
				CVec& a        = points[i].mPos;
				CVec& b        = points[i + 1].mPos;
				CVec delta     = b - a;
				f32 distance   = PSVECDistance(&b, &a);
				bool pinned    = i == 0 && cord->mEndSource[0];
				f32 restLength = !pinned && i == cord->mCount - 2 ? 5.0f : cord->mSpeed;
				CVec step      = delta / distance * (0.7f * (restLength - distance));
				if (pinned) {
					a.Set(*cord->mEndSource[0]);
					cord->mPull = step * -1.0f;
				} else {
					PSVECSubtract(&a, &step, &a);
				}
				b += step;
			}
			PinCordEnds(cord, points);
		}
		for (s32 i = 0; i < cord->mCount; ++i) {
			if (points[i].mPos.y < bounceHeight)
				points[i].mPos.y = bounceHeight - 0.8f * points[i].mSpeed.y;
		}
		if (cord->mFlags & XObjPlugCord::CollideWithObjects) {
			for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
				XObj* object = gGame.GetObject(i);
				if (!(object->mStatus.mFlags & 1) || !(object->mStatus.mFlags & 0x10000) || object->mStatus.mFlags & 0x100000
				    || object == gGame.GetPlayer())
					continue;
				Vec pos = gGame.GetPlayer()->mMoveTranslate;
				f32 dx = object->mMoveTranslate.x - pos.x, dz = object->mMoveTranslate.z - pos.z;
				if (sqrtf(dx * dx + dz * dz) > 150.0f) // only nearby objects
					continue;
				for (s32 j = 0; j < cord->mCount; ++j)
					PushPlugCordPointFromObject(&cord->mPoints[j].mPos, object, 0);
			}
		}
		for (s32 i = 0; i < cord->mCount; ++i)
			points[i].mTarget = points[i].mPos;
	}
	u32 old   = cord->mMode;
	s32 state = gGame.GetPlayer()->mPlug.mConnection;
	if (state < 0)
		cord->mMode = 3;
	else {
		s32 kind = (state >> 8) & 255;
		if (kind == 0)
			cord->mMode = 0;
		else if (kind == 1)
			cord->mMode = 1;
		else if (kind == 2)
			cord->mMode = 2;
		else if (kind == 3)
			cord->mMode = 4;
	}
	if (cord->mMode != old) {
		switch (cord->mMode) {
		case 1:
			ResetSparks(cord->mNodes, 0xff00ff, 1, 8, 1);
			ConfigurePlugWireEffect(&gGame.GetPlayer()->mPlug.mWire, 0, -1, 0.4f, 0xff00ff);
			break;
		case 2:
			ResetSparks(cord->mNodes, 0x6400ffff, 1, 8, 1);
			ConfigurePlugWireEffect(&gGame.GetPlayer()->mPlug.mWire, 0, -1, 1, 0x6400ffff);
			break;
		case 3:
			ResetSparks(cord->mNodes, 0xff0064ff, -1, 8, 1);
			ConfigurePlugWireEffect(&gGame.GetPlayer()->mPlug.mWire, 0, -1, 1, 0xff0064ff);
			break;
		case 4:
			ResetSparks(cord->mNodes, 0xff, -1, 1, 2);
			ConfigurePlugWireEffect(&gGame.GetPlayer()->mPlug.mWire, -1, 0x52, 1, 0xff);
			break;
		default:
			ResetSparks(cord->mNodes, 0, 0, 0, 0);
			ClearWireEffect(&gGame.GetPlayer()->mPlug.mWire);
			break;
		}
	}
	UpdatePlugCordNodes(&cord->mNodes, cord->mPoints, cord->mCount, frames);
}
