#ifdef VERSION_R24J01
#ifndef _GAME_CVEC_H
#define _GAME_CVEC_H

#include "game/ut.h"
#if defined(VERSION_R24J01)
#include "nw4r/math/math_types.h"
#endif

// CVec's methods that call the sdk. this header doesn't pick a vec.h, include
// dolphin/mtx/vec.h or dolphin/vec.h first. the casts are for the hsd one, it
// takes non const pointers

inline CVec& CVec::operator+=(const Vec& v)
{
#if defined(VERSION_R24J01)
	// the Wii body is nw4r's VEC3Add (its three work registers show in every retail add),
	// the GameCube calls PSVECAdd
	nw4r::math::VEC3Add((nw4r::math::VEC3*)this, (nw4r::math::VEC3*)this, (const nw4r::math::VEC3*)&v);
#else
	PSVECAdd(this, (Vec*)&v, this);
#endif
	return *this;
}

inline CVec& CVec::operator*=(f32 s)
{
#if defined(VERSION_R24J01)
	nw4r::math::VEC3Scale((nw4r::math::VEC3*)this, (nw4r::math::VEC3*)this, s);
#else
	PSVECScale(this, this, s);
#endif
	return *this;
}

inline CVec& CVec::operator-=(const Vec& v)
{
#if defined(VERSION_R24J01)
	nw4r::math::VEC3Sub((nw4r::math::VEC3*)this, (nw4r::math::VEC3*)this, (const nw4r::math::VEC3*)&v);
#else
	PSVECSubtract(this, (Vec*)&v, this);
#endif
	return *this;
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
#else
#ifndef _GAME_CVEC_H
#define _GAME_CVEC_H

#include "game/ut.h"

// CVec's methods that call the sdk. this header doesn't pick a vec.h, include
// dolphin/mtx/vec.h or dolphin/vec.h first. the casts are for the hsd one, it
// takes non const pointers

inline void CVec::operator+=(const Vec& v)
{
	PSVECAdd(this, (Vec*)&v, this);
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
#endif
