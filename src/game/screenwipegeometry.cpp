#include "game/screenwipegeometry.h"

#define SCREEN_CORNERS(z)                                \
	{                                                    \
		{                                                \
			{ 0, 0, z }, { 640, 0, z }, { 640, 448, z }, \
			{                                            \
				0, 448, z                                \
			}                                            \
		}                                                \
	}

const WipeGeometry gScreenWipeGeometry = {
	{ { 1, 1, 1 }, {}, {}, { 1, 1, 1 } },
	SCREEN_CORNERS(0),
	{},
	{ SCREEN_CORNERS(-1) },
	{ SCREEN_CORNERS(0) },
	{ SCREEN_CORNERS(0) },
	{ SCREEN_CORNERS(0) },
	{},
	{ 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0 },
	{ SCREEN_CORNERS(0) },
};
