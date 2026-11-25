#ifndef _GAME_CPTCL_H
#define _GAME_CPTCL_H

#include "game/ut.h"

// 2d sprite particles drawn through CGs, the dust puffs. class names from RTTI, the
// methods have no symbols so they stay fn_ below

class CGs;
class CTexObj;

// one particle
class CPtclOne {
public:
	BOOL mActive;    // _00
	CVec mPos;       // _04
	CVec mSize;      // _14
	CVec mVel;       // _24
	f32 mAngle;      // _34
	f32 mAngleSpeed; // _38
	u32 mColor;      // _3C, rgba, the alpha is scaled by mAlpha
	f32 mAlpha;      // _40
	f32 mAlphaStep;  // _44, negative while fading in
	CTexObj* mTex;   // _48, untextured particles draw as a fan
	// _4C vtable

	virtual ~CPtclOne() { }          // _08 (weak)
	virtual void Init();             // _0C, fn_800245F0
	virtual void Update(u32 frames); // _10, fn_80024E08
	virtual void Draw(CGs* gs);      // _14, fn_80024D80
};
typedef char CPtclOne_size_check[sizeof(CPtclOne) == 0x50 ? 1 : -1];

// a puff that floats off
class CPtclDust3 : public CPtclOne {
public:
	virtual ~CPtclDust3() { }        // _08 (weak)
	virtual void Update(u32 frames); // _10, fn_800247D8
	virtual void Draw(CGs* gs);      // _14, fn_80024600
};

// a puff that falls
class CPtclDust2 : public CPtclOne {
public:
	virtual ~CPtclDust2() { }        // _08 (weak)
	virtual void Update(u32 frames); // _10, fn_80024B1C
	virtual void Draw(CGs* gs);      // _14, fn_80024934
};

// a pool of particles of one kind, the array lives in the owner
class CPtcl {
public:
	u8* mPtcls;   // _00
	s32 mStride;  // _04, size of one particle
	s32 mCount;   // _08
	s32 mCounter; // _0C
	// _10 vtable

	virtual ~CPtcl() { } // _08

	CPtclOne* Get(s32 i) { return (CPtclOne*)(mPtcls + (u32)i * (u32)mStride); }
};
typedef char CPtcl_size_check[sizeof(CPtcl) == 0x14 ? 1 : -1];

struct DustParticlePool {
	CPtcl mPtcl;           // _00
	CPtclDust3 mPtcls[50]; // _14
};
typedef char DustParticlePool_size_check[sizeof(DustParticlePool) == 0xFB4 ? 1 : -1];

extern "C" {
void DrawParticlePool(CPtcl* ptcl);
void UpdateParticlePool(CPtcl* ptcl, u32 frames);
void ClearParticlePool(CPtcl* ptcl);
CPtclOne* SpawnPoolParticle(CPtcl* ptcl, const Vec* pos, const Vec* size, const Vec* vel, u32 color);
void InitParticlePool(CPtcl* ptcl, void* ptcls, s32 stride, s32 count);
}

#endif
