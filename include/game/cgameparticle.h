#ifndef _GAME_CGAMEPARTICLE_H
#define _GAME_CGAMEPARTICLE_H

#include "game/cgame.h"
#include "game/cparticle.h"

// Inline helpers that make a particle with the game's CParticle (gGame + 0x90CC).
//
// MWCC builds the constant initializer of every Vec literal below while it parses the header,
// so each including unit gets these six Vecs at the start of its .rodata whether it calls the
// helpers or not: { 1, 1, 1 }, { 0, 0, 0 }, { 0, 0, 0 } twice. Retail has exactly that 0x48-byte
// block in front of 27 GameCube units (objb_attach, xobjactormama, evt, msg, xobjs, obja_chr,
// ccameraactionmap2, ccameraactionscroll2, ...) and not in units that never make particles
// (ccameraactioneye, objb_plug, objb_sound, ...). The Wii build drops the unreferenced copies.
//
// The bank/id helpers are the second triple: objb_act keeps out-of-line copies of both
// (0x800D57F4 copies its { 1, 1, 1 } and { 0, 0, 0 } from block + 0x24/0x30, 0x800D593C its
// rotation from + 0x3C), and xactradar inlines the scaled one (rotation from + 0x3C). The code
// helpers in front are the first triple; no retail code is left that reads it.

inline HSD_Generator* CreateParticle(CGame* game, u32 code, s32 status, const Vec& pos, BOOL keep)
{
	return game->mParticle.CreateSRT(code, status, pos, (Vec) { 1, 1, 1 }, (Vec) { 0, 0, 0 }, keep);
}

inline HSD_Generator* CreateParticle(CGame* game, u32 code, s32 status, const Vec& pos, const Vec& scale, BOOL keep)
{
	return game->mParticle.CreateSRT(code, status, pos, scale, (Vec) { 0, 0, 0 }, keep);
}

inline HSD_Generator* CreateParticle(CGame* game, u32 bank, u32 id, s32 status, const Vec& pos, BOOL keep)
{
	return game->mParticle.CreateSRT(id | (bank << 16), status, pos, (Vec) { 1, 1, 1 }, (Vec) { 0, 0, 0 }, keep);
}

inline HSD_Generator* CreateParticle(CGame* game, u32 bank, u32 id, s32 status, const Vec& pos, const Vec& scale, BOOL keep)
{
	return game->mParticle.CreateSRT(id | (bank << 16), status, pos, scale, (Vec) { 0, 0, 0 }, keep);
}

#endif
