#ifndef _GAME_PLACEMENT_H
#define _GAME_PLACEMENT_H

#if defined(VERSION_R24J01)
#include "Dolphin/mtx.h"
#else
#include <dolphin/mtx/mtxtypes.h>
#endif
#include "game/coordinfo.h"
#include "game/gmath.h"

static inline Vec PlacementPosition(const CoordInfo& coord)
{
#if defined(VERSION_R24J01)
	Vec pos;
#else
	Vec pos = Vec();
#endif
	const StagePlacement* entry = coord.mPlacement;
	pos.x                       = entry->mPos[0] / 100.0f;
	pos.y                       = entry->mPos[1] / 100.0f;
	pos.z                       = entry->mPos[2] / 100.0f;
	return pos;
}

static inline Vec PlacementRotation(const CoordInfo& coord)
{
#if defined(VERSION_R24J01)
	Vec rot;
#else
	Vec rot = Vec();
#endif
	const StagePlacement* entry = coord.mPlacement;
	rot.x                       = DEG2RAD * entry->mRot[0];
	rot.y                       = DEG2RAD * entry->mRot[1];
	rot.z                       = DEG2RAD * entry->mRot[2];
	return rot;
}

static inline Vec PlacementScale(const CoordInfo& coord)
{
#if defined(VERSION_R24J01)
	Vec scale;
#else
	Vec scale = Vec();
#endif
	const StagePlacement* entry = coord.mPlacement;
	scale.x                     = entry->mScale[0] / 100.0f;
	scale.y                     = entry->mScale[1] / 100.0f;
	scale.z                     = entry->mScale[2] / 100.0f;
	return scale;
}

#endif
