// chibi robo's tumble fade and the ring on the floor around him, the GameCube's
// xactradar.cpp tail: retail 0x801078C8-0x80107C50, right behind the tumble actions in
// wii/xacttumble.cpp (HandleCopterActionInput follows). The fade struct is the GameCube's,
// the draws go through the Wii CGs layer.
#include "game/wii/cgs.h"
#include "game/wii/cgame.h"
#include "game/gmath.h"
#include <math.h>

// the tumble ring sits just above the floor
static inline Vec RingPos(const Vec& pos, const Vec& offset)
{
	Vec v = { pos.x + offset.x, 0.01f + pos.y, pos.z + offset.z };
	return v;
}

extern "C" {

/**
 * @note Address: 0x801078C8
 * @note Size: 0xAC
 */
void UpdateTumbleFade(XActTumbleFade* fade, BOOL increase, u32 frames)
{
	if (fade->mActive) {
		if (increase) {
			if (fade->mAmount < 1.0f) {
				if ((fade->mAmount += 0.01f * frames) > 1.0f) {
					fade->mAmount = 1.0f;
				}
			}
		} else if (fade->mAmount > 0.0f) {
			if ((fade->mAmount -= 0.05f * frames) < 0.0f) {
				fade->mAmount = 0.0f;
				fade->mActive = FALSE;
			}
		}
	}
}

// the ring on the floor while he tumbles
/**
 * @note Address: 0x80107974
 * @note Size: 0x2DC
 */
// NONMATCHING: 97.31% - the CVec offset copy schedules as three batched loads then three
// stores in retail (z,y,x into f3,f2,f0), ours interleaves load/store pairs; some callee
// register colours differ in the loop constants. The ring loop itself matches, including
// the per-call rematerialization of the angle.
void DrawTumbleRing(XActTumbleFade* fade, const Vec& pos, f32 angle)
{
	if (fade->mAmount > 0.0f) {
		CVec offset(fade->mOffset);
		Mtx m;
		PSMTXRotRad(m, 'y', angle);
		PSMTXMultVec(m, &offset, &offset);
		Vec center = RingPos(pos, offset);
		f32 size   = 6.0f * fade->mAmount;
		u32 color  = (u32)(160.0f * fade->mAmount) | 0x641E00;
		Vec rot    = { -HALF_PI, 0.0f, 0.0f };
		DrawGsCircle(&gGame.mGs, 0x18, color, color, center.x, center.y, center.z, size, size, rot.x, rot.y, rot.z);
		PrimVertex verts[50];
		f32 outer = size * 0.5f + 1.0f;
		for (int i = 0; i < 25; i++) {
			verts[i * 2].pos.x     = center.x - size * std::sinf((TWO_PI / 24) * i) / 2.0f;
			verts[i * 2].pos.z     = center.z + size * std::cosf((TWO_PI / 24) * i) / 2.0f;
			verts[i * 2].pos.y     = center.y;
			verts[i * 2].color     = color;
			verts[i * 2 + 1].pos.x = center.x - outer * std::sinf((TWO_PI / 24) * i);
			verts[i * 2 + 1].pos.z = center.z + outer * std::cosf((TWO_PI / 24) * i);
			verts[i * 2 + 1].pos.y = center.y;
			verts[i * 2 + 1].color = 0;
		}
		DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, verts, 50);
	}
}

} // extern "C"
