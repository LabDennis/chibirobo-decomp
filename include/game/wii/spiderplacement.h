#ifndef GAME_WII_SPIDERPLACEMENT_H
#define GAME_WII_SPIDERPLACEMENT_H

#include "game/coordinfo.h"
#include "game/gmath.h"

// the placement entry's position, rotation and scale (the GameCube's game/spiderplacement.h); the Wii
// builds all three axes each time instead of starting from a work Vec
static inline Vec PlacementPosition(const CoordInfo& coord)
{
	Vec pos;
	const UnkStageObj* entry = coord.mPlacement;
	pos.x                    = entry->mPos[0] / 100.0f;
	pos.y                    = entry->mPos[1] / 100.0f;
	pos.z                    = entry->mPos[2] / 100.0f;
	return pos;
}

static inline Vec PlacementRotation(const CoordInfo& coord)
{
	Vec rot;
	const UnkStageObj* entry = coord.mPlacement;
	rot.x                    = DEG2RAD * entry->mRot[0];
	rot.y                    = DEG2RAD * entry->mRot[1];
	rot.z                    = DEG2RAD * entry->mRot[2];
	return rot;
}

static inline Vec PlacementScale(const CoordInfo& coord)
{
	Vec scale;
	const UnkStageObj* entry = coord.mPlacement;
	scale.x                  = entry->mScale[0] / 100.0f;
	scale.y                  = entry->mScale[1] / 100.0f;
	scale.z                  = entry->mScale[2] / 100.0f;
	return scale;
}

#endif
