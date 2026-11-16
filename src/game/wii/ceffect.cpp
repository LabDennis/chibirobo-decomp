// the Wii effect unit: the nw4r::ef effect system with its memory managers, and behind it the
// 2d particle pools the GameCube keeps in gxb.cpp. Only the pools are here, the rest needs the
// nw4r::ef headers.
#include "nw4r/ef/ef_effect.h"
#include "nw4r/ef/ef_effectsystem.h"
#include "nw4r/ef/ef_handle.h"
#ifndef ROUND_UP
#define ROUND_UP(x, align) (((x) + (align) - 1) & (-(align)))
#endif
#include "nw4r/ef/ef_memorymanagerimpl.h"
#include "revolution/mem.h"
#include "Dolphin/gx.h"
#include "Dolphin/mtx.h"
#include "game/wii/cgame.h"
#include "game/wii/cgs.h"
#include "game/cptcl.h"
#include "game/gmath.h"
#include <math.h>
#include <stdlib.h>

extern MEMiHeapHead* gWiiGameHeap;

// NONMATCHING: NW4R_ASSERT / nw4r::db::Panic in MemoryManager ctor and CEffectSystem handle array setup
void CEffectSystem::Init()
{
	u32 size       = nw4r::ef::MemoryManager::CalcMemorySize(100, 1000, 100, 3000, 16);
	mMemoryPool    = MEMAllocFromExpHeapEx(gWiiGameHeap, size, 32);
	mMemoryManager = new nw4r::ef::MemoryManager(mMemoryPool, size, 100, 1000, 100, 3000);
	nw4r::ef::EffectSystem::GetInstance()->SetMemoryManager(static_cast<nw4r::ef::MemoryManagerBase*>(mMemoryManager), 16);
	mLightMask  = 0;
	GXColor c0  = { 0xFF, 0xFF, 0xFF, 0xFF };
	mLightColor = c0;
	GXColor c1  = { 0xFF, 0xFF, 0xFF, 0xFF };
	_4E0        = c1;
	_4E4        = 0;
}

void CEffectSystem::RetireAll(u32 groupID, BOOL retireParticles)
{
	nw4r::ef::EffectSystem* sys = nw4r::ef::EffectSystem::GetInstance();
	sys->RetireEffectAll(groupID);
	sys->RetireEmitterAll(groupID);
	if (retireParticles) {
		sys->RetireParticleAll(groupID);
	}
}

void CEffectSystem::UpdateLights(CLight* light)
{
	mLightMask = 0;
	s32 idx    = 0;
	for (; light != NULL; light = light->mNext) {
		if (light->mFlags & 0x8000) {
			if ((u8)light->mFlags == 1) {
				mLightColor = light->GetColor();
			} else {
				mLightMask |= (1 << idx);
				idx++;
			}
		}
	}
}

void CEffectSystem::SetFog(u32 param, const GXColor* color, f32 f1, f32 f2, f32 f3, f32 f4)
{
	_4E4 = param;
	_4E8 = f1;
	_4EC = f2;
	_4F0 = f3;
	_4F4 = f4;
	_4F8 = *color;
}

void RetireEffect(nw4r::ef::HandleBase* handle, BOOL retireParticles)
{
	if (handle->IsValid()) {
		nw4r::ef::Effect* effect = static_cast<nw4r::ef::Effect*>(handle->GetPtr());
		effect->RetireEmitterAll();
		if (retireParticles) {
			effect->RetireParticleAll();
		}
	}
}

struct EffectInstance {
	nw4r::ef::HandleBase handle; // _00
	u32 _08;                     // _08
	Vec pos;                     // _0C
	Vec rot;                     // _18
	Vec scale;                   // _24
	u8 _30[0x60 - 0x30];         // _30
	MtxP mtx;                    // _60
	u8 _64[0x74 - 0x64];         // _64
};

// NONMATCHING: stack Vec copy and Mtx local scope layout (87.19%)
void UpdateEffectMtx(EffectInstance* inst)
{
	if (!inst->handle.IsValid()) {
		return;
	}

	if (inst->mtx != NULL) {
		nw4r::ef::Effect* effect = static_cast<nw4r::ef::Effect*>(inst->handle.GetPtr());
		Mtx mtx;
		PSMTXCopy(inst->mtx, mtx);
		effect->SetRootMtx(*reinterpret_cast<nw4r::math::MTX34*>(mtx));
		Vec pos   = { mtx[0][3], mtx[1][3], mtx[2][3] };
		inst->pos = pos;
	} else {
		Mtx trans;
		Mtx rot;
		Mtx scale;
		Mtx tmp;
		PSMTXTrans(trans, inst->pos.x, inst->pos.y, inst->pos.z);
		PSMTXScale(scale, inst->scale.x, inst->scale.y, inst->scale.z);
		PSMTXRotRad(rot, 'y', inst->rot.y);
		PSMTXConcat(trans, rot, tmp);
		PSMTXConcat(tmp, scale, tmp);
		nw4r::ef::Effect* effect = static_cast<nw4r::ef::Effect*>(inst->handle.GetPtr());
		effect->SetRootMtx(*reinterpret_cast<nw4r::math::MTX34*>(tmp));
	}
}

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
