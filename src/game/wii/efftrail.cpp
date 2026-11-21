#include "Dolphin/vec.h"
#include "game/gmath.h"
#include <math.h>

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
