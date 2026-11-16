#ifndef NW4R_EF_GX_H
#define NW4R_EF_GX_H

// GX as nw4r::ef uses it: the public interface plus what the shared GX headers leave out
// (indexed 8-bit vertex writes, the vertex format getter).
#include "revolution/gx.h"

#include "Dolphin/GX/GXFifo.h"

#ifdef __cplusplus
extern "C" {
#endif

void GXGetVtxAttrFmt(GXVtxFmt fmt, GXAttr attr, GXCompCnt* cnt, GXCompType* type, u8* frac);

static inline void GXNormal1x16(u16 index) { GXWGFifo.u16 = index; }
static inline void GXNormal1x8(u8 index) { GXWGFifo.u8 = index; }
static inline void GXNormal3u16(u16 x, u16 y, u16 z) { GXWGFifo.u16 = x; GXWGFifo.u16 = y; GXWGFifo.u16 = z; }
static inline void GXNormal3u8(u8 x, u8 y, u8 z) { GXWGFifo.u8 = x; GXWGFifo.u8 = y; GXWGFifo.u8 = z; }
static inline void GXPosition1x16(u16 index) { GXWGFifo.u16 = index; }
static inline void GXPosition1x8(u8 index) { GXWGFifo.u8 = index; }
static inline void GXPosition2s16(s16 x, s16 y) { GXWGFifo.s16 = x; GXWGFifo.s16 = y; }
static inline void GXPosition2s8(s8 x, s8 y) { GXWGFifo.s8 = x; GXWGFifo.s8 = y; }
static inline void GXPosition2u8(u8 x, u8 y) { GXWGFifo.u8 = x; GXWGFifo.u8 = y; }
static inline void GXPosition3s8(s8 x, s8 y, s8 z) { GXWGFifo.s8 = x; GXWGFifo.s8 = y; GXWGFifo.s8 = z; }
static inline void GXPosition3u8(u8 x, u8 y, u8 z) { GXWGFifo.u8 = x; GXWGFifo.u8 = y; GXWGFifo.u8 = z; }
static inline void GXTexCoord1f32(f32 s) { GXWGFifo.f32 = s; }
static inline void GXTexCoord1s16(s16 s) { GXWGFifo.s16 = s; }
static inline void GXTexCoord1s8(s8 s) { GXWGFifo.s8 = s; }
static inline void GXTexCoord1u16(u16 s) { GXWGFifo.u16 = s; }
static inline void GXTexCoord1u8(u8 s) { GXWGFifo.u8 = s; }
static inline void GXTexCoord1x16(u16 index) { GXWGFifo.u16 = index; }
static inline void GXTexCoord1x8(u8 index) { GXWGFifo.u8 = index; }

static inline void GXSetTexCoordGen(GXTexCoordID dst, GXTexGenType func, GXTexGenSrc src, u32 mtx)
{
    GXSetTexCoordGen2(dst, func, src, mtx, GX_FALSE, GX_PTIDENTITY);
}

#ifdef __cplusplus
}
#endif

#endif
