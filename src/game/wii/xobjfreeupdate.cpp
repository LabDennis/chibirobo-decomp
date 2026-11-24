#include "game/wii/xobjflags.h" // first: its AnimSfx needs the real nw4r::math, ahead of xobj.h's shim
#include "game/wii/xobj.h"
#include "game/wii/xobjfree.h"
#include "game/wii/cgame.h"
#include "game/wii/cgs.h"
#include "game/cptcl.h"
#include "game/evt.h"
#include "game/gmath.h"
#include "nw4r/snd/snd_BasicSound.h"
#include <math.h>
#include <stdlib.h>

// the plain object's burst update and hit overlay. The GameCube's xobjfreeupdate.cpp
// carries the same body; the Wii reads the particle position through GetPos() and its
// anim-end flag through the IsAnimDone virtual. The unit also hosts the coin's two
// sound fades (the vtable's _20C/_210 slots point here) and owns the three per-kind
// collection flags its __sinit registers

extern s32 gRefreshRate;

// the per-kind collection flags, one CEvtFlags each; the item, coin and dust ctors
// point their mFlags at these, and the unit's __sinit registers them in this order.
// gCoinHandleTable sits between the coin and dust flags in .bss (auto bss order)
CEvtFlags gItemEventFlags;
CEvtFlags gCoinEventFlags;
u32 gCoinHandleTable[5];
CEvtFlags gDustEventFlags;

// NONMATCHING: float regalloc in the burst loop, f25-f28 rotate against retail
/**
 * @note Address: 0x80118860
 * @note Size: 0x408
 */
void XObjFree::UpdateObject(u32 frames)
{
	XObj::UpdateObject(frames);
	if (mStatus.mFlags & 0x2000000000000000ULL) {
		mStatus.mFlags &= ~0x2000000000000000ULL;
		mStatus.mFlags |= 0x4000000000000000ULL;
		SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		SetColor(1, 200, 200, 200, 0.0f);
		SetColor(1, 0, 0, 0, 0.4f);
		for (s32 i = 0; i < 10; ++i) {
			f32 angle    = DEG2RAD * (rand() % 360);
			f32 sinAngle = (f32)sin(angle);
			f32 x        = sinAngle * (rand() % 100) / 150.0f;
			f32 cosAngle = (f32)cos(angle);
			f32 z        = cosAngle * (rand() % 100) / 150.0f;
			f32 y        = (rand() % 100) / 200.0f;
			Vec position = *GetPos();
			position.x += x * (rand() % 100) / 100.0f;
			position.z += z * (rand() % 100) / 100.0f;
			SpawnPoolParticle(&gGame.GetPlayer()->mDust.mPtcl, &position, &(Vec) { 20.0f, 20.0f, 1.0f }, &(Vec) { x, y, z }, 0xFFFFFFF0);
		}
		mFlash = 1.0f;
	}
	if (mStatus.mFlags & 0x4000000000000000ULL) {
		if (IsAnimDone())
			ClearAnim();
		if (mFlash > 0.0f) {
			if ((mFlash -= 0.04f) <= 0.0f)
				Release();
		}
	}
}

/**
 * @note Address: 0x80118C68
 * @note Size: 0x278
 */
void XObjFree::DrawScreenOverlay()
{
	// NONMATCHING: retail zeroes a second 12-byte local beside pos (stw 0x10/0x14/0x18):
	// an unnamed Vec the source had; a Vec[2] spelling gets the stores back but moves the
	// vcall's vptr loads, so the plain Vec scores higher.
	XObj::DrawScreenOverlay();
	if ((mStatus.mFlags & 0x4000000000000000ULL) && mFlash > 0.0f) {
		Vec pos = { 0.0f, 0.0f, 0.0f };
		f32 x, y;
		ProjectOffsetToScreen(&pos, &x, &y, NULL, NULL);
		f32 flash = mFlash;
		f32 inner = 400.0f * flash;
		f32 outer = 400.0f * (1.0f - flash);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCCOL, GX_BL_SRCALPHA, GX_LO_SET);
		s32 i;
		for (i = 0; i < 1; i++) {
			DrawGsCircle(&gGame.mGs, 16, (u32)(255.0f * flash) | 0xFFFFFF00, 0xFFFFFF00, x, y, 0.0f, inner + i, inner + i, 0.0f, 0.0f,
			             0.0f);
		}
		for (i = 0; i < 1; i++) {
			DrawGsCircle(&gGame.mGs, 16, (u32)(255.0f * flash) | 0xFFFFFF00, 0xFFFFFF00, x, y, 0.0f, outer + i, outer + i, 0.0f, 0.0f,
			             0.0f);
		}
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
	}
}

/**
 * @note Address: 0x80118EE0
 * @note Size: 0x4
 */
void XObjFree::SetAnim(s32 id, BOOL force, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	XObj::SetAnim(id, force, frame, rate, blend, blendRate);
}

/**
 * @note Address: 0x80118EE4
 * @note Size: 0x78
 */
void XObjFree::OnHitFromPosition(const Vec& position, f32 strength)
{
	XObj::OnHitFromPosition(position, strength);
	if (mStatus.mFlags & 0x04000000ULL) {
		f32 remaining = mLife - strength;
		mLife         = remaining;
		if (remaining <= 0.0f)
			mStatus.mFlags |= 0x2000000000000000ULL;
	}
}

// NONMATCHING: regalloc, retail keeps the actor in r27 and the handle in r3; ours swaps them
/**
 * @note Address: 0x80118F5C
 * @note Size: 0x10C
 */
void XObjCoin::ResumeSounds(f32 fade)
{
	for (s32 i = 0; i < 2; i++) {
		CSoundHandle* handle = mActors[i].GetHandle();
		if (handle != NULL && mActors[i].mActor != NULL) {
			handle->Resume((s32)(gRefreshRate * fade));
		}
	}
}

// NONMATCHING: as ResumeSounds
/**
 * @note Address: 0x80119068
 * @note Size: 0x10C
 */
void XObjCoin::PauseSounds(f32 fade)
{
	for (s32 i = 0; i < 2; i++) {
		CSoundHandle* handle = mActors[i].GetHandle();
		if (handle != NULL && mActors[i].mActor != NULL) {
			handle->Pause((s32)(gRefreshRate * fade));
		}
	}
}
