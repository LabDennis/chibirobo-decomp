#ifndef _GAME_CVEC_H
#define _GAME_CVEC_H

#include "game/ut.h"
#if defined(VERSION_R24J01)
#include "nw4r/math/math_types.h"
#endif

// CVec's methods that call the sdk. this header doesn't pick a vec.h, include
// dolphin/mtx/vec.h or dolphin/vec.h first. the casts are for the hsd one, it
// takes non const pointers

inline void CVec::operator+=(const Vec& v)
{
#if defined(VERSION_R24J01)
	// the Wii body is nw4r's VEC3Add (its three work registers show in every retail add),
	// the GameCube calls PSVECAdd
	nw4r::math::VEC3Add((nw4r::math::VEC3*)this, (nw4r::math::VEC3*)this, (const nw4r::math::VEC3*)&v);
#else
	PSVECAdd(this, (Vec*)&v, this);
#endif
}

inline void CVec::operator-=(const Vec& v)
{
#if defined(VERSION_R24J01)
	// paired singles like operator+=
	register f32 axy, az, bxy, bz;
	register CVec* self     = this;
	register const Vec* rhs = &v;
	asm {
		psq_l  axy, 0(self), 0, 0
		psq_l  bxy, 0(rhs), 0, 0
		ps_sub axy, axy, bxy
		psq_st axy, 0(self), 0, 0
		psq_l  az, 8(self), 1, 0
		psq_l  bz, 8(rhs), 1, 0
		ps_sub az, az, bz
		psq_st az, 8(self), 1, 0
	}
#else
	PSVECSubtract(this, (Vec*)&v, this);
#endif
}

inline f32 CVec::Length() const
{
	return PSVECMag((Vec*)this);
}

inline CVec& CVec::Normalize()
{
	PSVECNormalize(this, this);
	return *this;
}

#endif
