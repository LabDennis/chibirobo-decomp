// The Wii's map floor collision, .text 0x802BA4F4-0x802BD294 (its static initializer is
// __sinit_\ctrigon_cpp). Unlike the GameCube file, whose functions sit in reverse, the Wii
// keeps them in source order, and the two selector switches stay functions of their own.
// The readers are global functions as in the GameCube map (no local scope there either).
// NONMATCHING: only the vertex readers and their selectors are reconstructed; the triangle
// parser on the NW4R shapes, the vertex size scan, Create/Search/Check and the two
// destructors with the counter global are not written yet. TrigonGetVtxS8 differs from
// retail in register choice and load order only.
#include "game/ctrigon.h"
#include "Dolphin/gx.h"

extern "C" void PSMTXMultVecArray(const Mtx m, const Vec* srcBase, Vec* dstBase, u32 count);

// returns the position array entry the index at dl points at; the 16 and 32 bit readers
// step by the whole vertex (6 or 12 bytes), the byte readers by three
typedef void* (*TrigonGetIdxFunc)(void* base, u8* dl);

extern "C" {
void TrigonGetVtxF32(const TRIGON_INFO* tri, Vec* out);
void TrigonGetVtxS16(const TRIGON_INFO* tri, Vec* out);
void TrigonGetVtxU16(const TRIGON_INFO* tri, Vec* out);
void TrigonGetVtxS8(const TRIGON_INFO* tri, Vec* out);
void TrigonGetVtxU8(const TRIGON_INFO* tri, Vec* out);
void* TrigonGetIdxF32U8(void* base, u8* dl);
void* TrigonGetIdxF32U16(void* base, u8* dl);
void* TrigonGetIdxS16U8(void* base, u8* dl);
void* TrigonGetIdxS16U16(void* base, u8* dl);
void* TrigonGetIdxU16U8(void* base, u8* dl);
void* TrigonGetIdxU16U16(void* base, u8* dl);
void* TrigonGetIdxS8U8(void* base, u8* dl);
void* TrigonGetIdxS8U16(void* base, u8* dl);
void* TrigonGetIdxU8U8(void* base, u8* dl);
void* TrigonGetIdxU8U16(void* base, u8* dl);
}

/**
 * @note Address: 0x802BC3E4
 * @note Size: 0xC0
 */
TrigonGetIdxFunc GetIdxFunc(int compType, int idxSize)
{
	switch (compType) {
	case GX_U8:
		return idxSize == 2 ? TrigonGetIdxU8U16 : TrigonGetIdxU8U8;
	case GX_S8:
		return idxSize == 2 ? TrigonGetIdxS8U16 : TrigonGetIdxS8U8;
	case GX_U16:
		return idxSize == 2 ? TrigonGetIdxU16U16 : TrigonGetIdxU16U8;
	case GX_S16:
		return idxSize == 2 ? TrigonGetIdxS16U16 : TrigonGetIdxS16U8;
	case GX_F32:
		return idxSize == 2 ? TrigonGetIdxF32U16 : TrigonGetIdxF32U8;
	}
	return NULL;
}

/**
 * @note Address: 0x802BC4A4
 * @note Size: 0x14
 */
void* TrigonGetIdxU8U16(void* base, u8* dl)
{
	return (u8*)base + *(u16*)dl * 3;
}

/**
 * @note Address: 0x802BC4B8
 * @note Size: 0x14
 */
void* TrigonGetIdxU8U8(void* base, u8* dl)
{
	return (u8*)base + *(u8*)dl * 3;
}

/**
 * @note Address: 0x802BC4CC
 * @note Size: 0x14
 */
void* TrigonGetIdxS8U16(void* base, u8* dl)
{
	return (s8*)base + *(u16*)dl * 3;
}

/**
 * @note Address: 0x802BC4E0
 * @note Size: 0x14
 */
void* TrigonGetIdxS8U8(void* base, u8* dl)
{
	return (s8*)base + *(u8*)dl * 3;
}

/**
 * @note Address: 0x802BC4F4
 * @note Size: 0x10
 */
void* TrigonGetIdxU16U16(void* base, u8* dl)
{
	return (u8*)base + *(u16*)dl * 6;
}

/**
 * @note Address: 0x802BC504
 * @note Size: 0x10
 */
void* TrigonGetIdxU16U8(void* base, u8* dl)
{
	return (u8*)base + *(u8*)dl * 6;
}

/**
 * @note Address: 0x802BC514
 * @note Size: 0x10
 */
void* TrigonGetIdxS16U16(void* base, u8* dl)
{
	return (u8*)base + *(u16*)dl * 6;
}

/**
 * @note Address: 0x802BC524
 * @note Size: 0x10
 */
void* TrigonGetIdxS16U8(void* base, u8* dl)
{
	return (u8*)base + *(u8*)dl * 6;
}

/**
 * @note Address: 0x802BC534
 * @note Size: 0x10
 */
void* TrigonGetIdxF32U16(void* base, u8* dl)
{
	return (u8*)base + *(u16*)dl * 12;
}

/**
 * @note Address: 0x802BC544
 * @note Size: 0x10
 */
void* TrigonGetIdxF32U8(void* base, u8* dl)
{
	return (u8*)base + *(u8*)dl * 12;
}

/**
 * @note Address: 0x802BC554
 * @note Size: 0x70
 */
TrigonGetVtxFunc GetVtxFunc(int compType)
{
	switch (compType) {
	case GX_U8:
		return TrigonGetVtxU8;
	case GX_S8:
		return TrigonGetVtxS8;
	case GX_U16:
		return TrigonGetVtxU16;
	case GX_S16:
		return TrigonGetVtxS16;
	case GX_F32:
		return TrigonGetVtxF32;
	}
	return NULL;
}

/**
 * @note Address: 0x802BC5C4
 * @note Size: 0x1AC
 */
void TrigonGetVtxU8(const TRIGON_INFO* tri, Vec* out)
{
	int i;
	int scale = 1 << tri->mFrac;

	// frac is the fixed point's fraction bits
	for (i = 0; i < 3; i++) {
		out[i].x = (f32)((u8*)tri->mVtx[i])[0] / (f32)scale;
		out[i].y = (f32)((u8*)tri->mVtx[i])[1] / (f32)scale;
		out[i].z = (f32)((u8*)tri->mVtx[i])[2] / (f32)scale;
	}
	if (tri->mMtx) {
		PSMTXMultVecArray(tri->mMtx, out, out, 3);
	}
}

/**
 * @note Address: 0x802BC770
 * @note Size: 0x1EC
 */
void TrigonGetVtxS8(const TRIGON_INFO* tri, Vec* out)
{
	int i;
	int scale = 1 << tri->mFrac;

	for (i = 0; i < 3; i++) {
		out[i].x = (f32)((s8*)tri->mVtx[i])[0] / (f32)scale;
		out[i].y = (f32)((s8*)tri->mVtx[i])[1] / (f32)scale;
		out[i].z = (f32)((s8*)tri->mVtx[i])[2] / (f32)scale;
	}
	if (tri->mMtx) {
		PSMTXMultVecArray(tri->mMtx, out, out, 3);
	}
}

/**
 * @note Address: 0x802BC95C
 * @note Size: 0x1AC
 */
void TrigonGetVtxU16(const TRIGON_INFO* tri, Vec* out)
{
	int i;
	int scale = 1 << tri->mFrac;

	for (i = 0; i < 3; i++) {
		out[i].x = (f32)((u16*)tri->mVtx[i])[0] / (f32)scale;
		out[i].y = (f32)((u16*)tri->mVtx[i])[1] / (f32)scale;
		out[i].z = (f32)((u16*)tri->mVtx[i])[2] / (f32)scale;
	}
	if (tri->mMtx) {
		PSMTXMultVecArray(tri->mMtx, out, out, 3);
	}
}

/**
 * @note Address: 0x802BCB08
 * @note Size: 0x1C8
 */
void TrigonGetVtxS16(const TRIGON_INFO* tri, Vec* out)
{
	int i;
	int scale = 1 << tri->mFrac;

	for (i = 0; i < 3; i++) {
		out[i].x = (f32)((s16*)tri->mVtx[i])[0] / (f32)scale;
		out[i].y = (f32)((s16*)tri->mVtx[i])[1] / (f32)scale;
		out[i].z = (f32)((s16*)tri->mVtx[i])[2] / (f32)scale;
	}
	if (tri->mMtx) {
		PSMTXMultVecArray(tri->mMtx, out, out, 3);
	}
}

/**
 * @note Address: 0x802BCCD0
 * @note Size: 0x74
 */
void TrigonGetVtxF32(const TRIGON_INFO* tri, Vec* out)
{
	int i;

	for (i = 0; i < 3; i++) {
		out[i] = *(Vec*)tri->mVtx[i];
	}
	if (tri->mMtx) {
		PSMTXMultVecArray(tri->mMtx, out, out, 3);
	}
}
