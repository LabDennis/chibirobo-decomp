#ifndef _GXFIFO_H_
#define _GXFIFO_H_

#include <dolphin/types.h>

#define GX_FIFO_ADDR 0xCC008000
#define GX_WRITE_U8(value) (*(volatile u8*)GX_FIFO_ADDR = (value))
#define GX_WRITE_U16(value) (*(volatile u16*)GX_FIFO_ADDR = (value))
#define GX_WRITE_F32(value) (*(volatile f32*)GX_FIFO_ADDR = (value))

static inline void GXPosition2u8(u8 x, u8 y)
{
	GX_WRITE_U8(x);
	GX_WRITE_U8(y);
}

static inline void GXPosition3f32(const f32 x, const f32 y, const f32 z)
{
	GX_WRITE_F32(x);
	GX_WRITE_F32(y);
	GX_WRITE_F32(z);
}

static inline void GXNormal3f32(f32 x, f32 y, f32 z)
{
	GX_WRITE_F32(x);
	GX_WRITE_F32(y);
	GX_WRITE_F32(z);
}

static inline void GXMatrixIndex1u8(u8 index)
{
	GX_WRITE_U8(index);
}

static inline void GXTexCoord1x8(const u8 index)
{
	GX_WRITE_U8(index);
}

static inline void GXTexCoord1x16(u16 index)
{
	GX_WRITE_U16(index);
}

static inline void GXTexCoord2f32(const f32 s, const f32 t)
{
	GX_WRITE_F32(s);
	GX_WRITE_F32(t);
}

static inline void GXColor1x8(u8 index)
{
	GX_WRITE_U8(index);
}

static inline void GXColor1x16(u16 index)
{
	GX_WRITE_U16(index);
}

static inline void GXColor1u16(u16 color)
{
	GX_WRITE_U16(color);
}

static inline void GXColor3u8(u8 r, u8 g, u8 b)
{
	GX_WRITE_U8(r);
	GX_WRITE_U8(g);
	GX_WRITE_U8(b);
}

static inline void GXColor4u8(const u8 r, const u8 g, const u8 b, const u8 a)
{
	GX_WRITE_U8(r);
	GX_WRITE_U8(g);
	GX_WRITE_U8(b);
	GX_WRITE_U8(a);
}

static inline void GXEnd(void)
{
}

#endif
