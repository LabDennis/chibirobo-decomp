#ifndef _REVOLUTION_MTX_H
#define _REVOLUTION_MTX_H

#include "Dolphin/mtx.h"

// the 4x4 routines the Wii SDK adds to the GameCube ones
void PSMTX44Identity(Mtx44 m);
void PSMTX44Concat(Mtx44 a, Mtx44 b, Mtx44 out);
u32 C_MTX44Inverse(Mtx44 src, Mtx44 inverse);

#endif
