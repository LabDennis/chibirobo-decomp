// the Wii effect unit: the nw4r::ef effect system with its memory managers, and behind it the
// 2d particle pools the GameCube keeps in gxb.cpp. Only the pools are here, the rest needs the
// nw4r::ef headers.
#define NW4R_ASSERT
#include "nw4r/ef/ef_effect.h"
#include "nw4r/ef/ef_effectsystem.h"
#include "nw4r/ef/ef_handle.h"
#include "nw4r/ef/ef_memorymanagerimpl.h"
#include "nw4r/ef/ef_resource.h"
#include "nw4r/ef/ef_drawinfo.h"
#include "nw4r/ef/ef_res_emitter.h"
#include "revolution/mem.h"
#include "Dolphin/gx.h"
#include "Dolphin/mtx.h"
#include "Dolphin/os.h"
#include "game/wii/cgame.h"
#include "game/wii/ceffect.h"
#include "game/wii/cgs.h"
#include "game/cptcl.h"
#include "game/gmath.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern MEMiHeapHead* gWiiGameHeap;

static u32 sNumEmitter[2];
struct SortInfo {
	MtxP mtx;
	MtxP proj;
};
static SortInfo sSortInfo;

bool EffectSortFunc(const nw4r::ef::Effect* op1, const nw4r::ef::Effect* op2);

// NONMATCHING: CEffectSystem handle array setup
void CEffectSystem::Init()
{
	u32 size       = nw4r::ef::MemoryManager::CalcMemorySize(100, 1000, 100, 3000, 16);
	mMemoryPool    = MEMAllocFromExpHeapEx(gWiiGameHeap, size, 32);
	mMemoryManager = new nw4r::ef::MemoryManager(mMemoryPool, size, 100, 1000, 100, 3000);
	nw4r::ef::EffectSystem::GetInstance()->SetMemoryManager(static_cast<nw4r::ef::MemoryManagerBase*>(mMemoryManager), 16);
	mLightMask  = 0;
	GXColor c0  = { 0, 0, 0, 0xFF };
	mLightColor = c0;
	GXColor c1  = { 0xFF, 0xFF, 0xFF, 0xFF };
	_4E0        = c1;
	_4E4        = 0;
}

BOOL CEffectSystem::Load(const char* name, s32 idx)
{
	if (mResources[idx].mFiles[0].IsLoaded()) {
		if (idx == 0) {
			OSPanic("ceffect.cpp", 0x40, "Already read %s\n", name);
		} else {
			Unload(idx);
		}
	}
	mResources[idx].mState = 1;
	strcpy(mResources[idx].mName, name);
	mResources[idx].mEffectProj = NULL;
	mResources[idx].mTexProj    = NULL;
	return TRUE;
}

// NONMATCHING: case fallthrough and CFile::mBuffer offset scheduling (82.38%)
BOOL CEffectSystem::UpdateLoad(s32 idx)
{
	char path[0x80];
	Resource* res = &mResources[idx];
	switch (res->mState) {
	case 1: {
		sprintf(path, "%s.breff", res->mName);
		if (!res->mFiles[0].ReadEx(path, 0, 0)) {
			break;
		}
		nw4r::ef::Resource* efRes = nw4r::ef::Resource::GetInstance();
		if (*(u16*)((u8*)res->mFiles[0].mBuffer + 6) < 0xB) {
			res->mFiles[0].Close();
			res->mState = 0;
			return TRUE;
		}
		res->mEffectProj = efRes->Add(static_cast<u8*>(res->mFiles[0].mBuffer));
		res->mState++;
	}
	case 2: {
		sprintf(path, "%s.breft", res->mName);
		if (!res->mFiles[1].ReadEx(path, 0, 0)) {
			break;
		}
		nw4r::ef::Resource* efRes = nw4r::ef::Resource::GetInstance();
		res->mTexProj             = efRes->AddTexture(static_cast<u8*>(res->mFiles[1].mBuffer));
		DCFlushRange(res->mFiles[1].mBuffer, res->mFiles[1].mSize);
		res->mState++;
	}
	case 3: {
		nw4r::ef::Resource* efRes = nw4r::ef::Resource::GetInstance();
		efRes->RelocateCommand();
		GXInvalidateVtxCache();
		GXInvalidateTexAll();
		res->mNumEmitter = efRes->NumEmitter(NULL);
		sNumEmitter[idx] = res->mNumEmitter;
		res->mState++;
		break;
	}
	}
	if ((u32)res->mState > 3) {
		res->mState = 0;
		return TRUE;
	}
	return res->mState == 0;
}

void CEffectSystem::Unload(s32 idx)
{
	nw4r::ef::Resource* efRes = nw4r::ef::Resource::GetInstance();
	if (idx == 0) {
		for (u32 i = 0; i < 16; ++i) {
			RetireAll(i, FALSE);
		}
		for (s32 i = 1; i >= 0; --i) {
			if (mResources[i].mTexProj != NULL) {
				efRes->RemoveTextureProject(mResources[i].mTexProj);
			}
			if (mResources[i].mEffectProj != NULL) {
				efRes->RemoveEffectProject(mResources[i].mEffectProj);
			}
			efRes->RelocateCommand();
			GXInvalidateVtxCache();
			GXInvalidateTexAll();
			mResources[i].mFiles[0].Close();
			mResources[i].mFiles[1].Close();
			mResources[i].mEffectProj = NULL;
			mResources[i].mTexProj    = NULL;
			mResources[i].mNumEmitter = 0;
			sNumEmitter[i]            = 0;
		}
		if (mMemoryManager != NULL) {
			delete static_cast<nw4r::ef::MemoryManagerBase*>(mMemoryManager);
		}
		if (mMemoryPool != NULL) {
			MEMFreeToExpHeap(gWiiGameHeap, mMemoryPool);
			mMemoryPool = NULL;
		}
	} else {
		if (mResources[idx].mTexProj != NULL) {
			efRes->RemoveTextureProject(mResources[idx].mTexProj);
		}
		if (mResources[idx].mEffectProj != NULL) {
			efRes->RemoveEffectProject(mResources[idx].mEffectProj);
		}
		efRes->RelocateCommand();
		GXInvalidateVtxCache();
		GXInvalidateTexAll();
		mResources[idx].mFiles[0].Close();
		mResources[idx].mFiles[1].Close();
		mResources[idx].mEffectProj = NULL;
		mResources[idx].mTexProj    = NULL;
		mResources[idx].mNumEmitter = 0;
		sNumEmitter[idx]            = 0;
	}
}

// NONMATCHING: GC/3.0a5 prolog/epilog differs from retail psq_st on f30/f31 (99.78%)
void CEffectSystem::Update(const Vec* camPos, MtxP camMtx, MtxP projMtx, u32 steps, s32 groupID, f32 f1, f32 f2)
{
	UpdateLoad(0);
	UpdateLoad(1);
	nw4r::ef::EffectSystem* sys = nw4r::ef::EffectSystem::GetInstance();
	Mtx mtx;
	PSMTXCopy(camMtx, mtx);
	nw4r::math::VEC3 pos(camPos->x, camPos->y, camPos->z);
	sys->SetProcessCamera(pos, *reinterpret_cast<const nw4r::math::MTX34*>(mtx), f1, f2);
	sSortInfo.mtx  = camMtx;
	sSortInfo.proj = projMtx;
	if ((u32)groupID == 0xFFFFFFFF) {
		for (u32 g = 0; g < 16; ++g) {
			sys->SortEffect(EffectSortFunc, g);
			for (u32 s = 0; s < steps; ++s) {
				sys->Calc(g, false);
			}
		}
	} else {
		sys->SortEffect(EffectSortFunc, groupID);
		for (u32 s = 0; s < steps; ++s) {
			sys->Calc(groupID, false);
		}
	}
}

// NONMATCHING: DrawInfo struct copy and GXColor stack temporary order (86.18%)
void CEffectSystem::Draw(MtxP viewMtx, s32 groupID)
{
	nw4r::ef::EffectSystem* sys = nw4r::ef::EffectSystem::GetInstance();
	nw4r::ef::DrawInfo drawInfo;
	Mtx mtx;
	PSMTXCopy(viewMtx, mtx);
	drawInfo.SetViewMtx(*reinterpret_cast<const nw4r::math::MTX34*>(mtx));
	if (mLightMask != 0) {
		drawInfo.SetLightEnable(true);
		drawInfo.SetLightMask(static_cast<GXLightID>(mLightMask));
		drawInfo.SetChanAmbColor(mLightColor);
		drawInfo.SetChanMatColor(_4E0);
	}
	if (_4E4 != 0) {
		drawInfo.SetFogType(static_cast<GXFogType>(_4E4));
		drawInfo.SetFogRange(_4E8, _4EC, _4F0, _4F4);
		drawInfo.SetFogColor(_4F8);
	}
	if ((u32)groupID == 0xFFFFFFFF) {
		for (u32 g = 0; g < 16; ++g) {
			sys->Draw(drawInfo, g);
		}
	} else {
		sys->Draw(drawInfo, groupID);
	}
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

// NONMATCHING: trailing return branch inversion (99.99%)
bool EffectSortFunc(const nw4r::ef::Effect* op1, const nw4r::ef::Effect* op2)
{
	NW4R_ASSERT_PTR_FILELINE("ceffect.cpp", 401, op1);
	NW4R_ASSERT_PTR_FILELINE("ceffect.cpp", 402, op2);

	const nw4r::math::MTX34* m1 = op1->GetRootMtx();
	const nw4r::math::MTX34* m2 = op2->GetRootMtx();
	f32 z1 = sSortInfo.mtx[2][0] * m1->_03 + sSortInfo.mtx[2][1] * m1->_13 + sSortInfo.mtx[2][2] * m1->_23 + sSortInfo.mtx[2][3];
	f32 z2 = sSortInfo.mtx[2][0] * m2->_03 + sSortInfo.mtx[2][1] * m2->_13 + sSortInfo.mtx[2][2] * m2->_23 + sSortInfo.mtx[2][3];
	if (z1 == z2) {
		return false;
	}
	bool res;
	if (z1 < 0.0f) {
		if (z2 < 0.0f) {
			res = (z1 < z2);
		} else {
			res = true;
		}
	} else {
		if (z2 < 0.0f) {
			res = false;
		} else {
			res = (z1 > z2);
		}
	}
	if (sSortInfo.proj[0][2] == 0.0f) {
		if (sSortInfo.proj[2][3] < 0.0f) {
			res = !res;
		}
	} else {
		if (sSortInfo.proj[2][2] > 0.0f) {
			res = !res;
		}
	}
	return !res;
}

// NONMATCHING: HandleBase in-place construction vs temporary copy assignment (92.30%)
BOOL CreateEffectInstance(EffectInstance* inst, u32 emitterIdx, u32 groupID)
{
	nw4r::ef::EffectSystem* sys = nw4r::ef::EffectSystem::GetInstance();
	nw4r::ef::Resource* res     = nw4r::ef::Resource::GetInstance();
	if (emitterIdx >= 10000) {
		emitterIdx = emitterIdx + sNumEmitter[0] - 10000;
	}
	nw4r::ef::ResEmitter resEmitter = res->GetEmitterIndexOf(emitterIdx, NULL);
	nw4r::ef::Effect* effect        = sys->CreateEffect(resEmitter.ptr(), groupID, 0);
	if (effect != NULL) {
		inst->handle            = nw4r::ef::HandleBase(effect);
		inst->_08               = groupID;
		Vec zero                = { 0.0f, 0.0f, 0.0f };
		inst->pos               = zero;
		inst->rot               = zero;
		Vec one                 = { 1.0f, 1.0f, 1.0f };
		inst->scale             = one;
		inst->mtx               = NULL;
		nw4r::ef::ResEmitter re = res->GetEmitterIndexOf(emitterIdx, NULL);
		if (re.IsValid()) {
			nw4r::ef::ResEmitterDesc desc = re.GetEmitterDesc();
			if (desc.IsValid()) {
				inst->_64 = desc.GetEmitLife();
			}
		}
		return TRUE;
	}
	return FALSE;
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
