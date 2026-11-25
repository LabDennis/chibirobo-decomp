#include "Dolphin/gx.h"

STATIC_ASSERT(sizeof(GXTexObjPriv) == sizeof(GXTexObj));
STATIC_ASSERT(sizeof(GXTexRegionPriv) == sizeof(GXTexRegion));
STATIC_ASSERT(sizeof(GXTlutObjPriv) == sizeof(GXTlutObj));
STATIC_ASSERT(sizeof(GXTlutRegionPriv) == sizeof(GXTlutRegion));

u8 GXTexMode0Ids[8]  = { 0x80, 0x81, 0x82, 0x83, 0xA0, 0xA1, 0xA2, 0xA3 };
u8 GXTexMode1Ids[8]  = { 0x84, 0x85, 0x86, 0x87, 0xA4, 0xA5, 0xA6, 0xA7 };
u8 GXTexImage0Ids[8] = { 0x88, 0x89, 0x8a, 0x8b, 0xA8, 0xA9, 0xAa, 0xAb };
u8 GXTexImage1Ids[8] = { 0x8c, 0x8d, 0x8e, 0x8f, 0xAc, 0xAd, 0xAe, 0xAf };
u8 GXTexImage2Ids[8] = { 0x90, 0x91, 0x92, 0x93, 0xB0, 0xB1, 0xB2, 0xB3 };
u8 GXTexImage3Ids[8] = { 0x94, 0x95, 0x96, 0x97, 0xB4, 0xB5, 0xB6, 0xB7 };
u8 GXTexTlutIds[8]   = { 0x98, 0x99, 0x9a, 0x9b, 0xB8, 0xB9, 0xBa, 0xBb };

u8 GX2HWFiltConv[6] = { 0x00, 0x04, 0x01, 0x05, 0x02, 0x06 };

#define GET_TILE_COUNT(a, b) (((a) + (1 << (b)) - 1) >> (b))

enum {
	GX_TEXOBJ_FLAG_MIPMAP = 1,
	GX_TEXOBJ_FLAG_DIRECT = 2,
};

/**
 * @note Address: N/A
 * @note Size: 0x64
 */
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

/**
 * @note Address: 0x801890A8
 * @note Size: 0x15C
 */
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

/**
 * @note Address: 0x80189204
 * @note Size: 0xC8
 */
void __GetImageTileCount(GXTexFmt format, u16 width, u16 height, u32* rowTiles, u32* colTiles, u32* componentTiles)
{
	u32 widthTiles, heightTiles;

	__GXGetTexTileShift(format, &widthTiles, &heightTiles);

	if (width <= 0) {
		width = 1;
	}

	if (height <= 0) {
		height = 1;
	}

	*rowTiles = GET_TILE_COUNT(width, widthTiles);
	*colTiles = GET_TILE_COUNT(height, heightTiles);
	*componentTiles = (format == GX_TF_RGBA8 || format == GX_TF_Z24X8) ? 2 : 1;
}

/**
 * @note Address: 0x801892CC
 * @note Size: 0x24C
 */
void GXInitTexObj(GXTexObj* obj, void* imagePtr, u16 width, u16 height, GXTexFmt format, GXTexWrapMode sWrap, GXTexWrapMode tWrap,
                  GXBool useMIPmap)
{
	u32 imageBase;
	u16 widthShift, heightShift;
	u32 rowTiles, colTiles;

	GXTexObjPriv* internal = (GXTexObjPriv*)obj;
	memset(internal, 0, sizeof(*internal));

	GX_SET_REG(internal->mode0, sWrap, 30, 31);
	GX_SET_REG(internal->mode0, tWrap, 28, 29);
	GX_SET_REG(internal->mode0, GX_TRUE, 27, 27);

	if (useMIPmap) {
		u32 maxDimSize;
		internal->flags |= GX_TEXOBJ_FLAG_MIPMAP;
		if (format == GX_TF_C4 || format == GX_TF_C8 || format == GX_TF_C14X2) {
			GX_SET_REG(internal->mode0, 5, 24, 26);
		} else {
			GX_SET_REG(internal->mode0, 6, 24, 26);
		}

		maxDimSize = width > height ? 31 - __cntlzw(width) : 31 - __cntlzw(height);

		GX_SET_REG(internal->mode1, (maxDimSize) * 16.f, 16, 23);
	} else {
		GX_SET_REG(internal->mode0, 4, 24, 26);
	}

	internal->format = format;
	GX_SET_REG(internal->image0, width - 1, 22, 31);
	GX_SET_REG(internal->image0, height - 1, 12, 21);

	GX_SET_REG(internal->image0, format & 0xf, 8, 11);
	imageBase = (u32)imagePtr >> 5;
	GX_SET_REG(internal->image3, imageBase, 11, 31);

	switch (format & 0xf) {
	case GX_TF_I4:
	case GX_TF_C4:
		internal->loadFormat = 1;
		widthShift           = 3;
		heightShift          = 3;
		break;
	case GX_TF_I8:
	case GX_TF_IA4:
	case GX_TF_C8:
		internal->loadFormat = 2;
		widthShift           = 3;
		heightShift          = 2;
		break;
	case GX_TF_IA8:
	case GX_TF_RGB565:
	case GX_TF_RGB5A3:
	case GX_TF_C14X2:
		internal->loadFormat = 2;
		widthShift           = 2;
		heightShift          = 2;
		break;
	case GX_TF_RGBA8:
		internal->loadFormat = 3;
		widthShift           = 2;
		heightShift          = 2;
		break;
	case GX_TF_CMPR:
		internal->loadFormat = 0;
		widthShift           = 3;
		heightShift          = 3;
		break;
	default:
		internal->loadFormat = 2;
		widthShift           = 2;
		heightShift          = 2;
		break;
	}

	rowTiles = GET_TILE_COUNT(width, widthShift);
	colTiles = GET_TILE_COUNT(height, heightShift);
	internal->loadCount = (rowTiles * colTiles) & 0x7fff;

	internal->flags |= GX_TEXOBJ_FLAG_DIRECT;
}

/**
 * @note Address: 0x80189518
 * @note Size: 0x48
 */
void GXInitTexObjCI(GXTexObj* obj, void* imagePtr, u16 width, u16 height, GXCITexFmt format, GXTexWrapMode sWrap, GXTexWrapMode tWrap,
                    GXBool useMIPmap, u32 tlutName)
{
	GXTexObjPriv* internal = (GXTexObjPriv*)obj;

	GXInitTexObj(obj, imagePtr, width, height, format, sWrap, tWrap, useMIPmap);

	internal->flags &= ~GX_TEXOBJ_FLAG_DIRECT;
	internal->tlutName = tlutName;
}

/**
 * @note Address: 0x80189560
 * @note Size: 0x164
 */
void GXInitTexObjLOD(GXTexObj* obj, GXTexFilter minFilter, GXTexFilter maxFilter, f32 minLOD, f32 maxLOD, f32 lodBias, GXBool doBiasClamp,
                     GXBool doEdgeLOD, GXAnisotropy maxAniso)
{
	GXTexObjPriv* internal = (GXTexObjPriv*)obj;
	u8 reg1, reg2;
	if (lodBias < -4.0f) {
		lodBias = -4.0f;
	} else if (lodBias >= 4.0f) {
		lodBias = 3.99f;
	}

	GX_SET_REG(internal->mode0, lodBias * 32.0f, 15, 22);
	GX_SET_REG(internal->mode0, maxFilter == 1 ? 1 : 0, 27, 27);
	GX_SET_REG(internal->mode0, GX2HWFiltConv[minFilter], 24, 26);
	GX_SET_REG(internal->mode0, doEdgeLOD ? 0 : 1, 23, 23);
	GX_SET_REG(internal->mode0, 0, 14, 14);
	GX_SET_REG(internal->mode0, 0, 13, 13);
	GX_SET_REG(internal->mode0, maxAniso, 11, 12);
	GX_SET_REG(internal->mode0, doBiasClamp, 10, 10);

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

	GX_SET_REG(internal->mode1, reg1, 24, 31);
	GX_SET_REG(internal->mode1, reg2, 16, 23);
}

/**
 * @note Address: 0x801896C4
 * @note Size: 0x10
 */
u16 GXGetTexObjWidth(const GXTexObj* obj)
{
	GXTexObjPriv* pObj = (GXTexObjPriv*)obj;
	return (pObj->image0 & 0x3FF) + 1;
}

/**
 * @note Address: 0x801896D4
 * @note Size: 0x10
 */
u16 GXGetTexObjHeight(const GXTexObj* obj)
{
	GXTexObjPriv* pObj = (GXTexObjPriv*)obj;
	return ((pObj->image0 >> 10) & 0x3FF) + 1;
}

/**
 * @note Address: 0x801896E4
 * @note Size: 0x8
 */
GXTexFmt GXGetTexObjFmt(const GXTexObj* obj)
{
	GXTexObjPriv* pObj = (GXTexObjPriv*)obj;
	return pObj->format;
}

/**
 * @note Address: 0x801896EC
 * @note Size: 0x18
 */
GXBool GXGetTexObjMipMap(const GXTexObj* obj)
{
	const GXTexObjPriv* internal = (const GXTexObjPriv*)obj;
	return (internal->flags & GX_TEXOBJ_FLAG_MIPMAP) == GX_TEXOBJ_FLAG_MIPMAP;
}

static inline void LoadTextureReg(const u32* reg)
{
	u32 word;

	GXWGFifo.s8 = GX_FIFO_CMD_LOAD_BP_REG;
	word = *reg;
	GXWGFifo.s32 = word;
}

/**
 * @note Address: 0x80189704
 * @note Size: 0x17C
 */
void GXLoadTexObjPreLoaded(GXTexObj* obj, GXTexRegion* region, GXTexMapID map)
{
	GXTexObjPriv* internalObj       = (GXTexObjPriv*)obj;
	GXTexRegionPriv* internalRegion = (GXTexRegionPriv*)region;

	GX_SET_REG(internalObj->mode0, GXTexMode0Ids[map], 0, 7);
	GX_SET_REG(internalObj->mode1, GXTexMode1Ids[map], 0, 7);
	GX_SET_REG(internalObj->image0, GXTexImage0Ids[map], 0, 7);

	GX_SET_REG(internalRegion->image1, GXTexImage1Ids[map], 0, 7);
	GX_SET_REG(internalRegion->image2, GXTexImage2Ids[map], 0, 7);

	GX_SET_REG(internalObj->image3, GXTexImage3Ids[map], 0, 7);

	LoadTextureReg(&internalObj->mode0);
	LoadTextureReg(&internalObj->mode1);
	LoadTextureReg(&internalObj->image0);
	LoadTextureReg(&internalRegion->image1);
	LoadTextureReg(&internalRegion->image2);
	LoadTextureReg(&internalObj->image3);

	if ((internalObj->flags & GX_TEXOBJ_FLAG_DIRECT) == 0) {
		GXTlutRegionPriv* tlutRegion = (GXTlutRegionPriv*)gx->tlutRegionCallback(internalObj->tlutName);
		GX_SET_REG(tlutRegion->tlutObj.texTlut, GXTexTlutIds[map], 0, 7);

		LoadTextureReg(&tlutRegion->tlutObj.texTlut);
	}

	gx->tImage0[map] = internalObj->image0;
	gx->tMode0[map]  = internalObj->mode0;

	gx->dirtyState |= GX_DIRTY_SU_TEX;
	gx->bpSentNot = GX_FALSE;
}

/**
 * @note Address: 0x80189880
 * @note Size: 0x54
 */
void GXLoadTexObj(GXTexObj* obj, GXTexMapID map)
{
	GXTexRegion* region = gx->texRegionCallback(obj, map);

	GXLoadTexObjPreLoaded(obj, region, map);
}

/**
 * @note Address: 0x801898D4
 * @note Size: 0x38
 */
void GXInitTlutObj(GXTlutObj* obj, void* table, GXTlutFmt format, u16 numEntries)
{
	GXTlutObjPriv* internal = (GXTlutObjPriv*)obj;

	internal->texTlut = 0;

	GX_SET_REG(internal->texTlut, format, 20, 21);
	GX_SET_REG(internal->loadTlutSource, (u32)table >> 5, 11, 31);
	GX_SET_REG(internal->loadTlutSource, GX_BP_REG_TMEMTLUTSRC, 0, 7);

	internal->numEntries = numEntries;
}

/**
 * @note Address: 0x8018990C
 * @note Size: 0x98
 */
void GXLoadTlut(GXTlutObj* obj, u32 tlutName)
{
	GXTlutObjPriv* tlut = (GXTlutObjPriv*)obj;
	GXTlutRegionPriv* region = (GXTlutRegionPriv*)gx->tlutRegionCallback(tlutName);
	u32 destination;

	__GXFlushTextureState();

	GX_BP_LOAD_REG(tlut->loadTlutSource);
	GX_BP_LOAD_REG(region->loadTlutDestination);

	__GXFlushTextureState();

	destination = region->loadTlutDestination;
	GX_SET_REG(tlut->texTlut, destination, 22, 31);

	region->tlutObj = *tlut;
}

/**
 * @note Address: 0x801899A4
 * @note Size: 0xF4
 */
void GXInitTexCacheRegion(GXTexRegion* region, GXBool is32bMIPmap, u32 memEven, GXTexCacheSize sizeEven, u32 memOdd, GXTexCacheSize sizeOdd)
{
	GXTexRegionPriv* internal = (GXTexRegionPriv*)region;

	u32 reg;
	switch (sizeEven) {
	case GX_TEXCACHE_32K:
		reg = 3;
		break;
	case GX_TEXCACHE_128K:
		reg = 4;
		break;
	case GX_TEXCACHE_512K:
		reg = 5;
		break;
	}

	internal->image1 = 0;

	GX_SET_REG(internal->image1, memEven >> 5, 17, 31);
	GX_SET_REG(internal->image1, reg, 14, 16);
	GX_SET_REG(internal->image1, reg, 11, 13);
	GX_SET_REG(internal->image1, 0, 10, 10);

	switch (sizeOdd) {
	case GX_TEXCACHE_32K:
		reg = 3;
		break;
	case GX_TEXCACHE_128K:
		reg = 4;
		break;
	case GX_TEXCACHE_512K:
		reg = 5;
		break;
	case GX_TEXCACHE_NONE:
		reg = 0;
		break;
	}

	internal->image2 = 0;
	GX_SET_REG(internal->image2, memOdd >> 5, 17, 31);
	GX_SET_REG(internal->image2, reg, 14, 16);
	GX_SET_REG(internal->image2, reg, 11, 13);

	internal->is32bMipMap = is32bMIPmap;
	internal->isCached = GX_TRUE;
}

/**
 * @note Address: 0x80189A98
 * @note Size: 0x38
 */
void GXInitTlutRegion(GXTlutRegion* region, u32 memAddr, GXTlutSize tlutSize)
{
	GXTlutRegionPriv* internal = (GXTlutRegionPriv*)region;

	internal->loadTlutDestination = 0;
	GX_SET_REG(internal->loadTlutDestination, (memAddr - 0x80000) >> 9, 22, 31);
	GX_SET_REG(internal->loadTlutDestination, tlutSize, 11, 21);
	GX_SET_REG(internal->loadTlutDestination, GX_BP_REG_TMEMTLUTDST, 0, 7);
}

/**
 * @note Address: 0x80189AD0
 * @note Size: 0x48
 */
void GXInvalidateTexAll(void)
{
	__GXFlushTextureState();
	GX_BP_LOAD_REG(0x66001000);
	GX_BP_LOAD_REG(0x66001100);
	__GXFlushTextureState();
}

/**
 * @note Address: 0x80189B18
 * @note Size: 0x14
 */
GXTexRegionCallback GXSetTexRegionCallback(GXTexRegionCallback func)
{
	GXTexRegionCallback oldFunc = gx->texRegionCallback;

	gx->texRegionCallback = func;

	return oldFunc;
}

/**
 * @note Address: 0x80189B2C
 * @note Size: 0x14
 */
GXTlutRegionCallback GXSetTlutRegionCallback(GXTlutRegionCallback func)
{
	GXTlutRegionCallback oldFunc = gx->tlutRegionCallback;

	gx->tlutRegionCallback = func;

	return oldFunc;
}

/**
 * @note Address: 0x80189B40
 * @note Size: 0xA0
 */
void __SetSURegs(u32 texMap, u32 texCoord)
{
	u16 widthMinusOne, heightMinusOne;
	GXBool repeatS, repeatT;

	widthMinusOne = GX_GET_REG(gx->tImage0[texMap], 22, 31);
	heightMinusOne = (gx->tImage0[texMap] & (0x3ff << 10)) >> 10;

	GX_SET_REG(gx->suTs0[texCoord], widthMinusOne, 16, 31);
	GX_SET_REG(gx->suTs1[texCoord], heightMinusOne, 16, 31);

	repeatS = GX_GET_REG(gx->tMode0[texMap], 30, 31) == GX_REPEAT;
	repeatT = GX_GET_REG(gx->tMode0[texMap], 28, 29) == GX_REPEAT;

	GX_SET_REG(gx->suTs0[texCoord], repeatS, 15, 15);
	GX_SET_REG(gx->suTs1[texCoord], repeatT, 15, 15);

	GX_BP_LOAD_REG(gx->suTs0[texCoord]);
	GX_BP_LOAD_REG(gx->suTs1[texCoord]);

	gx->bpSentNot = GX_FALSE;
}
