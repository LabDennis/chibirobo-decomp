// the Wii effect unit: the nw4r::ef effect system with its memory managers, and behind it the
// 2d particle pools the GameCube keeps in gxb.cpp. Only the pools are here, the rest needs the
// nw4r::ef headers.
#include "Dolphin/gx.h"
#include "Dolphin/mtx.h"
#include "game/wii/cgs.h"
#include "game/cptcl.h"
#include "game/gmath.h"
#include <math.h>
#include <stdlib.h>

/**
 * @note Address: 0x802BEBDC
 * @note Size: 0x84
 */
extern "C" void InitParticlePool(CPtcl* ptcl, void* ptcls, s32 stride, s32 count)
{
	ptcl->mPtcls   = (u8*)ptcls;
	ptcl->mStride  = stride;
	ptcl->mCount   = count;
	ptcl->mCounter = 0;
	for (s32 i = 0; i < count; ++i) {
		ptcl->Get(i)->Init();
	}
}

/**
 * @note Address: 0x802BEC60
 * @note Size: 0x160
 */
extern "C" CPtclOne* SpawnPoolParticle(CPtcl* ptcl, const Vec* pos, const Vec* size, const Vec* vel, u32 color)
{
	for (s32 i = 0; i < ptcl->mCount; ++i) {
		CPtclOne* one = ptcl->Get(i);
		if (one->mActive == FALSE) {
			one->mActive = TRUE;
			// the Wii copies each vector to the stack before it stores it
			Vec p = *pos;
			one->mPos.Set(p);
			Vec v = *vel;
			one->mVel.Set(v);
			Vec s = *size;
			one->mSize.Set(s);
			one->mAngle      = (f32)fmod((f32)rand(), PI);
			one->mAngleSpeed = 0.0f;
			one->mColor      = color;
			one->mAlpha      = 1.0f;
			one->mAlphaStep  = 0.02f;
			one->mTex        = NULL;
			return one;
		}
	}
	return NULL;
}

/**
 * @note Address: 0x802BEDC0
 * @note Size: 0x40
 */
extern "C" void ClearParticlePool(CPtcl* ptcl)
{
	for (s32 i = 0; i < ptcl->mCount; ++i) {
		CPtclOne* one = ptcl->Get(i);
		if (one->mActive != FALSE) {
			one->mActive = FALSE;
			one->mTex    = NULL;
		}
	}
}

/**
 * @note Address: 0x802BEE00
 * @note Size: 0x80
 */
extern "C" void UpdateParticlePool(CPtcl* ptcl, u32 frames)
{
	for (s32 i = 0; i < ptcl->mCount; ++i) {
		CPtclOne* one = ptcl->Get(i);
		if (one->mActive != FALSE) {
			one->Update(frames);
		}
	}
}

/**
 * @note Address: 0x802BEE80
 * @note Size: 0xA0
 */
void CPtcl::SetRenderState(CGs* gs)
{
	SetGsChannelEnabled(gs, 0, 0);
	gs->mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gs->mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
}

/**
 * @note Address: 0x802BEF20
 * @note Size: 0x90
 */
extern "C" void DrawParticlePool(CPtcl* ptcl, CGs* gs)
{
	gs->mBillboardEnabled = 1;
	for (s32 i = 0; i < ptcl->mCount; ++i) {
		CPtclOne* one = ptcl->Get(i);
		if (one->mActive != FALSE) {
			one->Draw(gs);
		}
	}
	gs->mBillboardEnabled = 0;
}
