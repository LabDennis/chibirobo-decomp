#ifndef _GXTYPES_h_
#define _GXTYPES_h_

#include "dolphin/types.h"
#include <sdk/dolphin/gx/GXRenderTypes.h>
#include <sdk/dolphin/gx/GXTevTypes.h>
#include <sdk/dolphin/gx/GXTextureTypes.h>

#define GX_FIFO_OBJSIZE 128

typedef enum _GXTexMtxType {
	GX_MTX3x4,
	GX_MTX2x4,
} GXTexMtxType;

typedef enum _GXTexGenType {
	GX_TG_MTX3x4,
	GX_TG_MTX2x4,
	GX_TG_BUMP0,
	GX_TG_BUMP1,
	GX_TG_BUMP2,
	GX_TG_BUMP3,
	GX_TG_BUMP4,
	GX_TG_BUMP5,
	GX_TG_BUMP6,
	GX_TG_BUMP7,
	GX_TG_SRTG,
} GXTexGenType;

typedef enum _GXTexGenSrc {
	GX_TG_POS,
	GX_TG_NRM,
	GX_TG_BINRM,
	GX_TG_TANGENT,
	GX_TG_TEX0,
	GX_TG_TEX1,
	GX_TG_TEX2,
	GX_TG_TEX3,
	GX_TG_TEX4,
	GX_TG_TEX5,
	GX_TG_TEX6,
	GX_TG_TEX7,
	GX_TG_TEXCOORD0,
	GX_TG_TEXCOORD1,
	GX_TG_TEXCOORD2,
	GX_TG_TEXCOORD3,
	GX_TG_TEXCOORD4,
	GX_TG_TEXCOORD5,
	GX_TG_TEXCOORD6,
	GX_TG_COLOR0,
	GX_TG_COLOR1,
} GXTexGenSrc;

typedef enum _GXTexMtx {
	GX_TEXMTX0  = 30,
	GX_TEXMTX1  = 33,
	GX_TEXMTX2  = 36,
	GX_TEXMTX3  = 39,
	GX_TEXMTX4  = 42,
	GX_TEXMTX5  = 45,
	GX_TEXMTX6  = 48,
	GX_TEXMTX7  = 51,
	GX_TEXMTX8  = 54,
	GX_TEXMTX9  = 57,
	GX_IDENTITY = 60
} GXTexMtx;

typedef enum _GXPTTexMtx {
	GX_PTTEXMTX0  = 64,
	GX_PTTEXMTX1  = 67,
	GX_PTTEXMTX2  = 70,
	GX_PTTEXMTX3  = 73,
	GX_PTTEXMTX4  = 76,
	GX_PTTEXMTX5  = 79,
	GX_PTTEXMTX6  = 82,
	GX_PTTEXMTX7  = 85,
	GX_PTTEXMTX8  = 88,
	GX_PTTEXMTX9  = 91,
	GX_PTTEXMTX10 = 94,
	GX_PTTEXMTX11 = 97,
	GX_PTTEXMTX12 = 100,
	GX_PTTEXMTX13 = 103,
	GX_PTTEXMTX14 = 106,
	GX_PTTEXMTX15 = 109,
	GX_PTTEXMTX16 = 112,
	GX_PTTEXMTX17 = 115,
	GX_PTTEXMTX18 = 118,
	GX_PTTEXMTX19 = 121,
	GX_PTIDENTITY = 125
} GXPTTexMtx;

typedef struct _GXFifoObj {
	u8 pad[GX_FIFO_OBJSIZE];
} GXFifoObj;

typedef struct _GXRenderModeObj {
	u32 viTVMode;
	u16 fbWidth;
	u16 efbHeight;
	u16 xfbHeight;
	u16 viXOrigin;
	u16 viYOrigin;
	u16 viWidth;
	u16 viHeight;
	u32 xfbMode;
	u8 field_rendering;
	u8 aa;
	u8 sample_pattern[12][2];
	u8 vfilter[7];
} GXRenderModeObj;

typedef struct _GXLightObj {
	u32 dummy[16];
} GXLightObj;

// GXBool
#define GX_FALSE   0
#define GX_TRUE    1
#define GX_DISABLE 0
#define GX_ENABLE  1

typedef enum _GXAttr {
	GX_VA_PNMTXIDX   = 0,
	GX_VA_TEX0MTXIDX = 1,
	GX_VA_TEX1MTXIDX = 2,
	GX_VA_TEX2MTXIDX = 3,
	GX_VA_TEX3MTXIDX = 4,
	GX_VA_TEX4MTXIDX = 5,
	GX_VA_TEX5MTXIDX = 6,
	GX_VA_TEX6MTXIDX = 7,
	GX_VA_TEX7MTXIDX = 8,
	GX_VA_POS        = 9,
	GX_VA_NRM        = 10,
	GX_VA_CLR0       = 11,
	GX_VA_CLR1       = 12,
	GX_VA_TEX0       = 13,
	GX_VA_TEX1       = 14,
	GX_VA_TEX2       = 15,
	GX_VA_TEX3       = 16,
	GX_VA_TEX4       = 17,
	GX_VA_TEX5       = 18,
	GX_VA_TEX6       = 19,
	GX_VA_TEX7       = 20,
	GX_POS_MTX_ARRAY = 21,
	GX_NRM_MTX_ARRAY = 22,
	GX_TEX_MTX_ARRAY = 23,
	GX_LIGHT_ARRAY   = 24,
	GX_VA_NBT        = 25,
	GX_VA_MAXATTR    = 26,
	GX_VA_NULL       = 0xff,
} GXAttr;

// GXGamma
#define GX_GM_1_0 0
#define GX_GM_1_7 1
#define GX_GM_2_2 2

#define GX_MAX_Z24 0x00ffffff

#define MAX_GXLIGHT 10

#endif
