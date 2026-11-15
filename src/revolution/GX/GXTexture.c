#include <cstring>
#include "revolution/gx.h"
#include <dolphin/os.h>

#include "revolution/gx/__gx.h"



typedef struct __GXTexObjInt_struct {
    u32 mode0;
    u32 mode1;
    u32 image0;
    u32 image3;
    void* userData;
    GXTexFmt fmt;
    u32 tlutName;
    u16 loadCnt;
    u8 loadFmt;
    u8 flags;
} __GXTexObjInt;

typedef struct __GXTexRegionInt_struct {
    u32 image1;
    u32 image2;
    u16 sizeEven;
    u16 sizeOdd;
    u8 is32bMipmap;
    u8 isCached;
} __GXTexRegionInt;

typedef struct __GXTlutObjInt_struct {
    u32 tlut;
    u32 loadTlut0;
    u16 numEntries;
} __GXTlutObjInt;

typedef struct __GXTlutRegionInt_struct {
    u32 loadTlut1;
    __GXTlutObjInt tlutObj;
} __GXTlutRegionInt;

u8 GXTexMode0Ids[8]  = { 0x80, 0x81, 0x82, 0x83, 0xA0, 0xA1, 0xA2, 0xA3 };
u8 GXTexMode1Ids[8]  = { 0x84, 0x85, 0x86, 0x87, 0xA4, 0xA5, 0xA6, 0xA7 };
u8 GXTexImage0Ids[8] = { 0x88, 0x89, 0x8a, 0x8b, 0xA8, 0xA9, 0xAa, 0xAb };
u8 GXTexImage1Ids[8] = { 0x8c, 0x8d, 0x8e, 0x8f, 0xAc, 0xAd, 0xAe, 0xAf };
u8 GXTexImage2Ids[8] = { 0x90, 0x91, 0x92, 0x93, 0xB0, 0xB1, 0xB2, 0xB3 };
u8 GXTexImage3Ids[8] = { 0x94, 0x95, 0x96, 0x97, 0xB4, 0xB5, 0xB6, 0xB7 };
u8 GXTexTlutIds[8]   = { 0x98, 0x99, 0x9a, 0x9b, 0xB8, 0xB9, 0xBa, 0xBb };

u8 GX2HWFiltConv[6] = { 0x00, 0x04, 0x01, 0x05, 0x02, 0x06 };
u8 HW2GXFiltConv[8] = { 0x00, 0x02, 0x04, 0x00, 0x01, 0x03, 0x05, 0x00 };

#define GET_TILE_COUNT(a, b) (((a) + (1 << (b)) - 1) >> (b))

inline void __GXGetTexTileShift(GXTexFmt format, u32* widthTiles, u32* heightTiles)
{
    switch (format) {
    case GX_TF_I4:
    case GX_TF_C4:
    case GX_TF_CMPR:
    case GX_CTF_R4:
    case GX_CTF_Z4:
        *widthTiles  = 3;
        *heightTiles = 3;
        break;

    case GX_TF_I8:
    case GX_TF_IA4:
    case GX_TF_C8:
    case GX_TF_Z8:
    case GX_CTF_RA4:
    case GX_CTF_A8:
    case GX_CTF_R8:
    case GX_CTF_G8:
    case GX_CTF_B8:
    case GX_CTF_Z8M:
    case GX_CTF_Z8L:
        *widthTiles  = 3;
        *heightTiles = 2;
        break;

    case GX_TF_IA8:
    case GX_TF_RGB565:
    case GX_TF_RGB5A3:
    case GX_TF_RGBA8:
    case GX_TF_C14X2:
    case GX_TF_Z16:
    case GX_TF_Z24X8:
    case GX_CTF_RA8:
    case GX_CTF_RG8:
    case GX_CTF_GB8:
    case GX_CTF_Z16L:
        *widthTiles  = 2;
        *heightTiles = 2;
        break;

    default:
        *heightTiles = 0;
        *widthTiles  = 0;
        break;
    }
}

u32 GXGetTexBufferSize(u16 width, u16 height, u32 format, GXBool mipmap, u8 max_lod)
{
    u32 widthTiles, heightTiles, tileSize, bufferSize, numX, numY, i;

    __GXGetTexTileShift(format, &widthTiles, &heightTiles);

    if (format == GX_TF_RGBA8 || format == GX_TF_Z24X8) {
        tileSize = 0x40;
    } else {
        tileSize = 0x20;
    }

    if (mipmap == GX_TRUE) {

        bufferSize = 0;

        for (i = 0; i < max_lod; i++) {
            numX = GET_TILE_COUNT(width, widthTiles);
            numY = GET_TILE_COUNT(height, heightTiles);

            bufferSize += numX * numY * tileSize;
            if (width == 1 && height == 1) {
                break;
            }

            width  = (width > 1) ? (width >> 1) : 1;
            height = (height > 1) ? (height >> 1) : 1;
        }

    } else {
        numX       = GET_TILE_COUNT(width, widthTiles);
        numY       = GET_TILE_COUNT(height, heightTiles);
        bufferSize = numX * numY * tileSize;
    }

    return bufferSize;
}

void __GetImageTileCount(GXTexFmt format, u16 width, u16 height, u32* a, u32* b, u32* c)
{
    u32 widthTiles, heightTiles;

    __GXGetTexTileShift(format, &widthTiles, &heightTiles);

    if (width <= 0) {
        width = 1;
    }

    if (height <= 0) {
        height = 1;
    }

    *a = GET_TILE_COUNT(width, widthTiles);
    *b = GET_TILE_COUNT(height, heightTiles);
    *c = (format == GX_TF_RGBA8 || format == GX_TF_Z24X8) ? 2 : 1;
}

void GXInitTexObj(GXTexObj* obj, void* imagePtr, u16 width, u16 height, GXTexFmt format, GXTexWrapMode sWrap, GXTexWrapMode tWrap,
                  GXBool useMIPmap)
{
    u32 imageBase;
    u16 a, b;
    u32 c, d;

    __GXTexObjInt* internal = (__GXTexObjInt*)obj;
    memset(internal, 0, sizeof(*internal));

    SET_REG_FIELD(0, internal->mode0, 2, 0, sWrap);
    SET_REG_FIELD(0, internal->mode0, 2, 2, tWrap);
    SET_REG_FIELD(0, internal->mode0, 1, 4, GX_TRUE);

    if (useMIPmap) {
        u32 maxDimSize;
        internal->flags |= 1;
        if (format == 8 || format == 9 || format == 10) {
            SET_REG_FIELD(0, internal->mode0, 3, 5, 5);
        } else {
            SET_REG_FIELD(0, internal->mode0, 3, 5, 6);
        }

        maxDimSize = width > height ? 31 - __cntlzw(width) : 31 - __cntlzw(height);

        SET_REG_FIELD(0, internal->mode1, 8, 8, (maxDimSize) * 16.f);
    } else {
        SET_REG_FIELD(0, internal->mode0, 3, 5, 4);
    }

    internal->fmt = format;
    SET_REG_FIELD(0, internal->image0, 10, 0, width - 1);
    SET_REG_FIELD(0, internal->image0, 10, 10, height - 1);

    SET_REG_FIELD(0, internal->image0, 4, 20, format & 0xf);
    imageBase = (u32)imagePtr >> 5;
    SET_REG_FIELD(0, internal->image3, 24, 0, imageBase);

    switch (format & 0xf) {
    case 0:
    case 8:
        internal->loadFmt = 1;
        a                    = 3;
        b                    = 3;
        break;
    case 1:
    case 2:
    case 9:
        internal->loadFmt = 2;
        a                    = 3;
        b                    = 2;
        break;
    case 3:
    case 4:
    case 5:
    case 10:
        internal->loadFmt = 2;
        a                    = 2;
        b                    = 2;
        break;
    case 6:
        internal->loadFmt = 3;
        a                    = 2;
        b                    = 2;
        break;
    case 0xe:
        internal->loadFmt = 0;
        a                    = 3;
        b                    = 3;
        break;
    default:
        internal->loadFmt = 2;
        a                    = 2;
        b                    = 2;
        break;
    }

    internal->loadCnt = (GET_TILE_COUNT(width, a) * GET_TILE_COUNT(height, b)) & 0x7fff;

    internal->flags |= 2;
}

void GXInitTexObjCI(GXTexObj* obj, void* imagePtr, u16 width, u16 height, GXCITexFmt format, GXTexWrapMode sWrap, GXTexWrapMode tWrap,
                    GXBool useMIPmap, u32 tlutName)
{
    __GXTexObjInt* internal = (__GXTexObjInt*)obj;

    GXInitTexObj(obj, imagePtr, width, height, format, sWrap, tWrap, useMIPmap);

    internal->flags &= ~2;
    internal->tlutName = tlutName;
}

void GXInitTexObjLOD(GXTexObj* obj, GXTexFilter minFilter, GXTexFilter maxFilter, f32 minLOD, f32 maxLOD, f32 lodBias, GXBool doBiasClamp,
                     GXBool doEdgeLOD, GXAnisotropy maxAniso)
{
    __GXTexObjInt* internal = (__GXTexObjInt*)obj;
    u8 reg1, reg2;
    if (lodBias < -4.0f) {
        lodBias = -4.0f;
    } else if (lodBias >= 4.0f) {
        lodBias = 3.99f;
    }

    SET_REG_FIELD(0, internal->mode0, 8, 9, lodBias * 32.0f);
    SET_REG_FIELD(0, internal->mode0, 1, 4, maxFilter == 1 ? 1 : 0);
    SET_REG_FIELD(0, internal->mode0, 3, 5, GX2HWFiltConv[minFilter]);
    SET_REG_FIELD(0, internal->mode0, 1, 8, doEdgeLOD ? 0 : 1);
    SET_REG_FIELD(0, internal->mode0, 1, 17, 0);
    SET_REG_FIELD(0, internal->mode0, 1, 18, 0);
    SET_REG_FIELD(0, internal->mode0, 2, 19, maxAniso);
    SET_REG_FIELD(0, internal->mode0, 1, 21, doBiasClamp);

    if (minLOD < 0.0f) {
        minLOD = 0.0f;
    } else if (minLOD > 10.0f) {
        minLOD = 10.0f;
    }
    reg1 = minLOD * 16.0f;

    if (maxLOD < 0.0f) {
        maxLOD = 0.0f;
    } else if (maxLOD > 10.0f) {
        maxLOD = 10.0f;
    }
    reg2 = maxLOD * 16.0f;

    SET_REG_FIELD(0, internal->mode1, 8, 0, reg1);
    SET_REG_FIELD(0, internal->mode1, 8, 8, reg2);
}

void GXInitTexObjWrapMode(GXTexObj* obj, GXTexWrapMode sWrap, GXTexWrapMode tWrap) {
    __GXTexObjInt* internal = (__GXTexObjInt*)obj;

    SET_REG_FIELD(0, internal->mode0, 2, 0, sWrap);
    SET_REG_FIELD(0, internal->mode0, 2, 2, tWrap);
}

void GXInitTexObjTlut(GXTexObj* obj, u32 tlutName) {
    __GXTexObjInt* internal = (__GXTexObjInt*)obj;

    internal->tlutName = tlutName;
}

void GXInitTexObjUserData(GXTexObj* obj, void* userData) {
    __GXTexObjInt* internal = (__GXTexObjInt*)obj;

    internal->userData = userData;
}

void* GXGetTexObjUserData(const GXTexObj* obj) {
    const __GXTexObjInt* internal = (const __GXTexObjInt*)obj;

    return internal->userData;
}

u16 GXGetTexObjWidth(const GXTexObj* obj)
{
    __GXTexObjInt* pObj = (__GXTexObjInt*)obj;
    return (pObj->image0 & 0x3FF) + 1;
}

u16 GXGetTexObjHeight(const GXTexObj* obj)
{
    __GXTexObjInt* pObj = (__GXTexObjInt*)obj;
    return ((pObj->image0 >> 10) & 0x3FF) + 1;
}

GXTexFmt GXGetTexObjFmt(const GXTexObj* obj)
{
    __GXTexObjInt* pObj = (__GXTexObjInt*)obj;
    return pObj->fmt;
}

GXTexWrapMode GXGetTexObjWrapS(const GXTexObj* obj) {
    const __GXTexObjInt* internal = (const __GXTexObjInt*)obj;

    return GET_REG_FIELD(internal->mode0, 2, 0);
}

GXTexWrapMode GXGetTexObjWrapT(const GXTexObj* obj) {
    const __GXTexObjInt* internal = (const __GXTexObjInt*)obj;

    return GET_REG_FIELD(internal->mode0, 2, 2);
}

GXBool GXGetTexObjMipMap(const GXTexObj* obj)
{
    __GXTexObjInt* internal = (__GXTexObjInt*)obj;
    return (internal->flags & 1) == 1;
}

void GXGetTexObjLODAll(const GXTexObj* obj, GXTexFilter* minFilter, GXTexFilter* magFilter, f32* minLOD, f32* maxLOD, f32* lodBias,
                       GXBool* doBiasClamp, GXBool* doEdgeLOD, GXAnisotropy* maxAniso) {
    const __GXTexObjInt* internal = (const __GXTexObjInt*)obj;
    s16 bias;

    *minFilter = HW2GXFiltConv[GET_REG_FIELD(internal->mode0, 3, 5)];
    *magFilter = GET_REG_FIELD(internal->mode0, 1, 4);
    *minLOD = (internal->mode1 & 0xFF) / 16.0f;
    *maxLOD = ((internal->mode1 >> 8) & 0xFF) / 16.0f;
    bias = (internal->mode0 >> 9) & 0xFF;
    *lodBias = (s8)bias / 32.0f;
    *doBiasClamp = GET_REG_FIELD(internal->mode0, 1, 21);
    *doEdgeLOD = !GET_REG_FIELD(internal->mode0, 1, 8);
    *maxAniso = GET_REG_FIELD(internal->mode0, 2, 19);
}

// NONMATCHING: retail keeps the six register values in r12, r11, r9, r7, r6 and r0, writes them to the
// FIFO from there and stores them back to the objects only at the end of the block; this spelling
// stores each at once and reloads it for the FIFO. Locals with a late write-back come out shorter (340).
void GXLoadTexObjPreLoaded(GXTexObj* obj, GXTexRegion* region, GXTexMapID map)
{
    __GXTexObjInt* internalObj = (__GXTexObjInt*)obj;
    __GXTexRegionInt* internalRegion = (__GXTexRegionInt*)region;

    SET_REG_FIELD(0, internalObj->mode0, 8, 24, GXTexMode0Ids[map]);
    SET_REG_FIELD(0, internalObj->mode1, 8, 24, GXTexMode1Ids[map]);
    SET_REG_FIELD(0, internalObj->image0, 8, 24, GXTexImage0Ids[map]);
    SET_REG_FIELD(0, internalRegion->image1, 8, 24, GXTexImage1Ids[map]);
    SET_REG_FIELD(0, internalRegion->image2, 8, 24, GXTexImage2Ids[map]);
    SET_REG_FIELD(0, internalObj->image3, 8, 24, GXTexImage3Ids[map]);

    GX_WRITE_RAS_REG(internalObj->mode0);
    GX_WRITE_RAS_REG(internalObj->mode1);
    GX_WRITE_RAS_REG(internalObj->image0);
    GX_WRITE_RAS_REG(internalRegion->image1);
    GX_WRITE_RAS_REG(internalRegion->image2);
    GX_WRITE_RAS_REG(internalObj->image3);

    if ((internalObj->flags & 2) == 0) {
        __GXTlutObjInt* tlut = (__GXTlutObjInt*)__GXData->tlutRegionCallback(internalObj->tlutName);
        SET_REG_FIELD(0, tlut->loadTlut0, 8, 24, GXTexTlutIds[map]);

        GX_WRITE_RAS_REG(tlut->loadTlut0);
    }

    __GXData->tImage0[map] = internalObj->image0;
    __GXData->tMode0[map]  = internalObj->mode0;

    __GXData->dirtyState |= 1;
    __GXData->bpSentNot = GX_FALSE;
}

void GXLoadTexObj(GXTexObj* obj, GXTexMapID map)
{
    GXTexRegion* ret = (GXTexRegion*)__GXData->texRegionCallback(obj, map);

    GXLoadTexObjPreLoaded(obj, ret, map);
}

void GXInitTlutObj(GXTlutObj* obj, void* table, GXTlutFmt format, u16 numEntries)
{
    __GXTlutObjInt* internal = (__GXTlutObjInt*)obj;

    internal->tlut = 0;

    SET_REG_FIELD(0, internal->tlut, 2, 10, format);
    SET_REG_FIELD(0, internal->loadTlut0, 24, 0, (u32)table >> 5);
    SET_REG_FIELD(0, internal->loadTlut0, 8, 24, 100);

    internal->numEntries = numEntries;
}

// NONMATCHING: same instructions; retail stores the merged word back to the TLUT object last,
// after the copy into the region.
void GXLoadTlut(GXTlutObj* obj, u32 tlutName)
{
    __GXTlutObjInt* internal = (__GXTlutObjInt*)obj;
    __GXTlutRegionInt* ret   = (__GXTlutRegionInt*)__GXData->tlutRegionCallback(tlutName);
    u32 reg;

    __GXFlushTextureState();

    GX_WRITE_RAS_REG(internal->loadTlut0);
    GX_WRITE_RAS_REG(ret->loadTlut1);

    __GXFlushTextureState();

    reg = ret->loadTlut1;
    SET_REG_FIELD(0, internal->tlut, 10, 0, reg);

    ret->tlutObj = *internal;
}

void GXInitTexCacheRegion(GXTexRegion* region, GXBool is32bMIPmap, u32 memEven, GXTexCacheSize sizeEven, u32 memOdd, GXTexCacheSize sizeOdd)
{
    __GXTexRegionInt* internal = (__GXTexRegionInt*)region;

    u32 reg;
    switch (sizeEven) {
    case 0:
        reg = 3;
        break;
    case 1:
        reg = 4;
        break;
    case 2:
        reg = 5;
        break;
    }

    internal->image1 = 0;

    SET_REG_FIELD(0, internal->image1, 15, 0, memEven >> 5);
    SET_REG_FIELD(0, internal->image1, 3, 15, reg);
    SET_REG_FIELD(0, internal->image1, 3, 18, reg);
    SET_REG_FIELD(0, internal->image1, 1, 21, 0);

    switch (sizeOdd) {
    case 0:
        reg = 3;
        break;
    case 1:
        reg = 4;
        break;
    case 2:
        reg = 5;
        break;
    case 3:
        reg = 0;
        break;
    }

    internal->image2 = 0;
    SET_REG_FIELD(0, internal->image2, 15, 0, memOdd >> 5);
    SET_REG_FIELD(0, internal->image2, 3, 15, reg);
    SET_REG_FIELD(0, internal->image2, 3, 18, reg);

    internal->is32bMipmap = is32bMIPmap;
    internal->isCached = 1;
}

void GXInitTlutRegion(GXTlutRegion* region, u32 memAddr, GXTlutSize tlutSize)
{
    __GXTlutRegionInt* internal = (__GXTlutRegionInt*)region;

    internal->loadTlut1 = 0;
    SET_REG_FIELD(0, internal->loadTlut1, 10, 0, (memAddr - 0x80000) >> 9);
    SET_REG_FIELD(0, internal->loadTlut1, 11, 10, tlutSize);
    SET_REG_FIELD(0, internal->loadTlut1, 8, 24, 0x65);
}

void GXInvalidateTexAll(void)
{
    __GXFlushTextureState();
    GX_WRITE_RAS_REG(0x66001000);
    GX_WRITE_RAS_REG(0x66001100);
    __GXFlushTextureState();
}

GXTexRegionCallback GXSetTexRegionCallback(GXTexRegionCallback func)
{
    GXTexRegionCallback oldFunc = __GXData->texRegionCallback;

    __GXData->texRegionCallback = func;

    return oldFunc;
}

GXTlutRegionCallback GXSetTlutRegionCallback(GXTlutRegionCallback func)
{
    GXTlutRegionCallback oldFunc = __GXData->tlutRegionCallback;

    __GXData->tlutRegionCallback = func;

    return oldFunc;
}

void __SetSURegs(u32 texImgIndex, u32 setUpRegIndex)
{
    u16 a1, a2;
    GXBool b, c;

    a1 = GET_REG_FIELD(__GXData->tImage0[texImgIndex], 10, 0);
    // a2 = GET_REG_FIELD(__GXData->tImage0[texImgIndex], 10, 10);
    a2 = (__GXData->tImage0[texImgIndex] & (0x3ff << 10)) >> 10;

    SET_REG_FIELD(0, __GXData->suTs0[setUpRegIndex], 16, 0, a1);
    SET_REG_FIELD(0, __GXData->suTs1[setUpRegIndex], 16, 0, a2);

    b = GET_REG_FIELD(__GXData->tMode0[texImgIndex], 2, 0) == 1;
    c = GET_REG_FIELD(__GXData->tMode0[texImgIndex], 2, 2) == 1;

    SET_REG_FIELD(0, __GXData->suTs0[setUpRegIndex], 1, 16, b);
    SET_REG_FIELD(0, __GXData->suTs1[setUpRegIndex], 1, 16, c);

    GX_WRITE_RAS_REG(__GXData->suTs0[setUpRegIndex]);
    GX_WRITE_RAS_REG(__GXData->suTs1[setUpRegIndex]);

    __GXData->bpSentNot = GX_FALSE;
}

void __GXSetSUTexRegs(void)
{
    u32 a;
    u32 b;
    u32 i;
    u32 c;
    u32 d;

    if (__GXData->tcsManEnab != 0xff) {
        a = GET_REG_FIELD(__GXData->genMode, 4, 10) + 1;
        b = GET_REG_FIELD(__GXData->genMode, 3, 16);
        for (i = 0; i < b; i++) {
            switch (i) {
            case 0:
                c = GET_REG_FIELD(__GXData->iref, 3, 0);
                d = GET_REG_FIELD(__GXData->iref, 3, 3);
                break;
            case 1:
                c = GET_REG_FIELD(__GXData->iref, 3, 6);
                d = GET_REG_FIELD(__GXData->iref, 3, 9);
                break;
            case 2:
                c = GET_REG_FIELD(__GXData->iref, 3, 12);
                d = GET_REG_FIELD(__GXData->iref, 3, 15);
                break;
            case 3:
                c = GET_REG_FIELD(__GXData->iref, 3, 18);
                d = GET_REG_FIELD(__GXData->iref, 3, 21);
                break;
            }

            if (!(__GXData->tcsManEnab & (1 << d))) {
                __SetSURegs(c, d);
            }
        }

        for (i = 0; i < a; i++) {
            u32* g = &__GXData->tref[i / 2];

            c = __GXData->texmapId[i] & ~0x100;

            if (i & 1) {
                d = GET_REG_FIELD(*g, 3, 15);
            } else {
                d = GET_REG_FIELD(*g, 3, 3);
            }

            if (c != 0xff && !(__GXData->tcsManEnab & (1 << d)) && __GXData->tevTcEnab & (1 << i)) {
                __SetSURegs(c, d);
            }
        }
    }
}

void __GXSetTmemConfig(u32 config)
{
    switch (config) {
    case 2:
        GX_WRITE_RAS_REG(0x8c0d8000);
        GX_WRITE_RAS_REG(0x900dc000);

        GX_WRITE_RAS_REG(0x8d0d8800);
        GX_WRITE_RAS_REG(0x910dc800);

        GX_WRITE_RAS_REG(0x8e0d9000);
        GX_WRITE_RAS_REG(0x920dd000);

        GX_WRITE_RAS_REG(0x8f0d9800);
        GX_WRITE_RAS_REG(0x930dd800);

        GX_WRITE_RAS_REG(0xac0da000);
        GX_WRITE_RAS_REG(0xb00dc400);

        GX_WRITE_RAS_REG(0xad0da800);
        GX_WRITE_RAS_REG(0xb10dcc00);

        GX_WRITE_RAS_REG(0xae0db000);
        GX_WRITE_RAS_REG(0xb20dd400);

        GX_WRITE_RAS_REG(0xaf0db800);
        GX_WRITE_RAS_REG(0xb30ddc00);
        break;
    case 1:
        GX_WRITE_RAS_REG(0x8c0d8000);
        GX_WRITE_RAS_REG(0x900dc000);

        GX_WRITE_RAS_REG(0x8d0d8800);
        GX_WRITE_RAS_REG(0x910dc800);

        GX_WRITE_RAS_REG(0x8e0d9000);
        GX_WRITE_RAS_REG(0x920dd000);

        GX_WRITE_RAS_REG(0x8f0d9800);
        GX_WRITE_RAS_REG(0x930dd800);

        GX_WRITE_RAS_REG(0xac0da000);
        GX_WRITE_RAS_REG(0xb00de000);

        GX_WRITE_RAS_REG(0xad0da800);
        GX_WRITE_RAS_REG(0xb10de800);

        GX_WRITE_RAS_REG(0xae0db000);
        GX_WRITE_RAS_REG(0xb20df000);

        GX_WRITE_RAS_REG(0xaf0db800);
        GX_WRITE_RAS_REG(0xb30df800);

        break;
    case 0:
    default:
        GX_WRITE_RAS_REG(0x8c0d8000);
        GX_WRITE_RAS_REG(0x900dc000);

        GX_WRITE_RAS_REG(0x8d0d8400);
        GX_WRITE_RAS_REG(0x910dc400);

        GX_WRITE_RAS_REG(0x8e0d8800);
        GX_WRITE_RAS_REG(0x920dc800);

        GX_WRITE_RAS_REG(0x8f0d8c00);
        GX_WRITE_RAS_REG(0x930dcc00);

        GX_WRITE_RAS_REG(0xac0d9000);
        GX_WRITE_RAS_REG(0xb00dd000);

        GX_WRITE_RAS_REG(0xad0d9400);
        GX_WRITE_RAS_REG(0xb10dd400);

        GX_WRITE_RAS_REG(0xae0d9800);
        GX_WRITE_RAS_REG(0xb20dd800);

        GX_WRITE_RAS_REG(0xaf0d9c00);
        GX_WRITE_RAS_REG(0xb30ddc00);

        break;
    }
}
