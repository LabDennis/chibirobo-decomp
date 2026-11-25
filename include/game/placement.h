#ifndef _GAME_PLACEMENT_H
#define _GAME_PLACEMENT_H

#include <dolphin/mtx/mtxtypes.h>
#include "game/coordinfo.h"
#include "game/gmath.h"

static inline Vec PlacementPosition(const CoordInfo& coord)
{
	Vec pos;
	const StagePlacement* entry = coord.mPlacement;
	pos.x                       = entry->mPos[0] / 100.0f;
	pos.y                       = entry->mPos[1] / 100.0f;
	pos.z                       = entry->mPos[2] / 100.0f;
	return pos;
}

static inline Vec PlacementRotation(const CoordInfo& coord)
{
	Vec rot;
	const StagePlacement* entry = coord.mPlacement;
	rot.x                       = DEG2RAD * entry->mRot[0];
	rot.y                       = DEG2RAD * entry->mRot[1];
	rot.z                       = DEG2RAD * entry->mRot[2];
	return rot;
}

static inline Vec PlacementScale(const CoordInfo& coord)
{
	Vec scale;
	const StagePlacement* entry = coord.mPlacement;
	scale.x                     = entry->mScale[0] / 100.0f;
	scale.y                     = entry->mScale[1] / 100.0f;
	scale.z                     = entry->mScale[2] / 100.0f;
	return scale;
}

#endif
