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

static inline void ReadSpiderPlacement(XObj* spider, Vec& home, f32& heading, f32& radius, f32 baseRadius, const Vec& position,
                                       const Vec& rotation, const Vec& scale)
{
	spider->mMoveTranslate.x = PlacementPosition(spider->mCoord, position).x;
	spider->mMoveTranslate.y = PlacementPosition(spider->mCoord, position).y;
	spider->mMoveTranslate.z = PlacementPosition(spider->mCoord, position).z;
	home.x                   = PlacementPosition(spider->mCoord, position).x;
	home.y                   = PlacementPosition(spider->mCoord, position).y;
	home.z                   = PlacementPosition(spider->mCoord, position).z;
	heading                  = PlacementRotation(spider->mCoord, rotation).y;
	radius                   = baseRadius * PlacementScale(spider->mCoord, scale).x;
}

#endif
