#ifndef _GAME_SPIDERPLACEMENT_H
#define _GAME_SPIDERPLACEMENT_H

#include "game/xobj.h"
#include "game/gmath.h"

static inline Vec PlacementPosition(const CoordInfo& coord, const Vec& initial)
{
	const UnkStageObj* entry = coord._00;
	Vec pos                  = initial;
	pos.x                    = entry->mPos[0] / 100.0f;
	pos.y                    = entry->mPos[1] / 100.0f;
	pos.z                    = entry->mPos[2] / 100.0f;
	return pos;
}

static inline Vec PlacementRotation(const CoordInfo& coord, const Vec& initial)
{
	const UnkStageObj* entry = coord._00;
	Vec rot                  = initial;
	rot.x                    = DEG2RAD * entry->_10[0];
	rot.y                    = DEG2RAD * entry->_10[1];
	rot.z                    = DEG2RAD * entry->_10[2];
	return rot;
}

static inline Vec PlacementScale(const CoordInfo& coord, const Vec& initial)
{
	const UnkStageObj* entry = coord._00;
	Vec scale                = initial;
	scale.x                  = entry->mScale[0] / 100.0f;
	scale.y                  = entry->mScale[1] / 100.0f;
	scale.z                  = entry->mScale[2] / 100.0f;
	return scale;
}

#endif
