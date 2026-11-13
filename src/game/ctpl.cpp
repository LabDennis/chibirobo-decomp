#include "game/ctpl.h"
#include "game/cfont.h"
#include "Dolphin/ai.h"
#include "Dolphin/os.h"

extern "C" {
void HSD_AudioSetMallocFreeProc(void* (*mallocProc)(s32), void (*freeProc)(void*));
BOOL HSD_AudioInit(u32 heapSize, u32 voices, u32 groups);
}

static void InitTextureLOD(CTexObj* texture);

/**
 * @note Address: 0x801A074C
 * @note Size: 0x28
 */
extern "C" void* AudioAlloc(s32 size)
{
	return OSAlloc(size);
}

/**
 * @note Address: 0x801A0724
 * @note Size: 0x28
 */
extern "C" void AudioFree(void* ptr)
{
	OSFree(ptr);
}

/**
 * @note Address: 0x801A06B8
 * @note Size: 0x6C
 */
// the caller passes its this along, nothing here reads it
extern "C" void InitAudio(void* owner, u32 heapSize, u32 voices, u32 groups)
{
	AIInit(NULL);
	HSD_AudioSetMallocFreeProc(AudioAlloc, AudioFree);
	HSD_AudioInit(heapSize, voices, groups);
}

/**
 * @note Address: 0x801A0554
 * @note Size: 0x164
 */
CTexObj& CTexObj::Init(TPLDescriptor* desc, u32 tlutName)
{
	mImage        = desc->textureHeader;
	mClut         = desc->CLUTHeader;
	GXBool mipmap = mImage->minLOD == mImage->maxLOD ? GX_FALSE : GX_TRUE; // one lod level means no mipmaps
	if (mClut == NULL) {
		GXInitTexObj(this, mImage->data, mImage->width, mImage->height, (GXTexFmt)mImage->format, mImage->wrapS, mImage->wrapT, mipmap);
	} else {
		GXInitTlutObj(&mTlutObj, mClut->data, mClut->format, mClut->numEntries);
		GXInitTexObjCI(this, mImage->data, mImage->width, mImage->height, (GXCITexFmt)mImage->format, mImage->wrapS, mImage->wrapT, mipmap,
		               tlutName);
		mTlutName = tlutName;
	}
	InitTextureLOD(this);
	mMtxDirty = FALSE;
	PSMTXIdentity(mMtx[0]);
	PSMTXIdentity(mMtx[1]);
	PSMTXIdentity(mMtx[2]);
	PSMTXIdentity(mMtx[3]);
	return *this;
}

/**
 * @note Address: 0x801A04D4
 * @note Size: 0x80
 */
CTexObj& CTexObj::Init(void* image, u16 width, u16 height, GXTexFmt format, GXTexWrapMode wrapS, GXTexWrapMode wrapT, GXBool mipmap)
{
	GXInitTexObj(this, image, width, height, format, wrapS, wrapT, mipmap);
	GXInitTexObjLOD(this, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
	mMtxDirty = FALSE;
	PSMTXIdentity(mMtx[0]);
	PSMTXIdentity(mMtx[1]);
	PSMTXIdentity(mMtx[2]);
	PSMTXIdentity(mMtx[3]);
	return *this;
}

/**
 * @note Address: 0x801A0410
 * @note Size: 0xC4
 */
u32 CTexObj::Set(const CTexObj& other)
{
	*this = other;
	return GXGetTexObjWidth(&other);
}

/**
 * @note Address: 0x801A0304
 * @note Size: 0x10C
 */
u32 CTexObj::SetChar(const char* str)
{
	CFont font;
	s32 width;
	*this = *GetFontGlyphTexture(&font, str, &width);
	return width;
}

// todo: cleanup
inline void CTexObj::Load(GXTexMapID map, u32 mtxId, GXTexMtxType mtxType)
{
	GXLoadTexObj(this, map);
	if (mClut != NULL) {
		GXLoadTlut(&mTlutObj, mTlutName);
	}
	if (mMtxDirty) {
		PSMTXConcat(mMtx[0], mMtx[2], mMtx[3]);
		PSMTXConcat(mMtx[3], mMtx[1], mMtx[3]);
		mMtxDirty = FALSE;
	}
	GXLoadTexMtxImm(mMtx[3], mtxId, mtxType);
}

/**
 * @note Address: 0x801A00E0
 * @note Size: 0x224
 */
void CTexObj::Draw(GXTexMapID map, u32 mtxId, GXTexMtxType mtxType, const Vec* pos, u32 color0, u32 color1, u32 color2, u32 color3)
{
	Load(map, mtxId, mtxType);

	GXVtxAttrFmtList fmt[] = {
		{ GX_VA_POS, GX_POS_XYZ, GX_F32, 0 },
		{ GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 },
		{ GX_VA_TEX0, GX_TEX_ST, GX_U8, 0 },
		{ GX_VA_NULL, GX_POS_XY, GX_U8, 0 },
	};
	GXVtxDescList desc[] = {
		{ GX_VA_POS, GX_DIRECT },
		{ GX_VA_CLR0, GX_DIRECT },
		{ GX_VA_TEX0, GX_DIRECT },
		{ GX_VA_NULL, GX_NONE },
	};
	GXClearVtxDesc();
	GXSetVtxAttrFmtv(GX_VTXFMT0, fmt);
	GXSetVtxDescv(desc);

	GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4); // a two-unit quad around pos, a color per corner
	GXPosition3f32(pos->x - 1.0f, pos->y + 1.0f, pos->z);
	GXColor1u32(color0);
	GXTexCoord2u8(0, 0);
	GXPosition3f32(pos->x + 1.0f, pos->y + 1.0f, pos->z);
	GXColor1u32(color1);
	GXTexCoord2u8(1, 0);
	GXPosition3f32(pos->x - 1.0f, pos->y - 1.0f, pos->z);
	GXColor1u32(color2);
	GXTexCoord2u8(0, 1);
	GXPosition3f32(pos->x + 1.0f, pos->y - 1.0f, pos->z);
	GXColor1u32(color3);
	GXTexCoord2u8(1, 1);
	GXEnd();
}

/**
 * @note Address: 0x8019FF28
 * @note Size: 0x1B8
 */
void CTexObj::DrawLit(GXTexMapID map, u32 mtxId, GXTexMtxType mtxType, const Vec* pos, u32 color0, u32 color1, u32 color2, u32 color3)
{
	Load(map, mtxId, mtxType);

	GXVtxAttrFmtList fmt[] = {
		{ GX_VA_POS, GX_POS_XYZ, GX_F32, 0 }, { GX_VA_NRM, GX_NRM_XYZ, GX_S8, 0 }, { GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 },
		{ GX_VA_TEX0, GX_TEX_ST, GX_U8, 0 },  { GX_VA_NULL, GX_POS_XY, GX_U8, 0 },
	};
	static const GXVtxDescList desc[] = {
		{ GX_VA_POS, GX_DIRECT }, { GX_VA_NRM, GX_DIRECT }, { GX_VA_CLR0, GX_DIRECT }, { GX_VA_TEX0, GX_DIRECT }, { GX_VA_NULL, GX_NONE },
	};
	GXClearVtxDesc();
	GXSetVtxAttrFmtv(GX_VTXFMT0, fmt);
	GXSetVtxDescv((GXVtxDescList*)desc);

	GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4); // same quad, every normal faces +z
	GXPosition3f32(pos->x - 1.0f, pos->y + 1.0f, pos->z);
	GXNormal3s8(0, 0, 1);
	GXColor1u32(color0);
	GXTexCoord2u8(0, 0);
	GXPosition3f32(pos->x + 1.0f, pos->y + 1.0f, pos->z);
	GXNormal3s8(0, 0, 1);
	GXColor1u32(color1);
	GXTexCoord2u8(1, 0);
	GXPosition3f32(pos->x - 1.0f, pos->y - 1.0f, pos->z);
	GXNormal3s8(0, 0, 1);
	GXColor1u32(color2);
	GXTexCoord2u8(0, 1);
	GXPosition3f32(pos->x + 1.0f, pos->y - 1.0f, pos->z);
	GXNormal3s8(0, 0, 1);
	GXColor1u32(color3);
	GXTexCoord2u8(1, 1);
	GXEnd();
}

// todo: cleanup
// the sdk's TPLBind, offsets in the file become pointers
static inline void TPLBind(TPLPalette* pal)
{
	pal->descriptorArray = (TPLDescriptor*)((u32)pal->descriptorArray + (u32)pal);
	for (u32 i = 0; i < pal->numDescriptors; i++) {
		TPLDescriptor* desc = &pal->descriptorArray[i];
		if (desc->textureHeader != NULL) {
			desc->textureHeader = (TPLHeader*)((u32)desc->textureHeader + (u32)pal);
			if (!desc->textureHeader->unpacked) {
				desc->textureHeader->data     = (char*)((u32)desc->textureHeader->data + (u32)pal);
				desc->textureHeader->unpacked = TRUE;
			}
		}
		if (desc->CLUTHeader != NULL) {
			desc->CLUTHeader = (TPLClutHeader*)((u32)desc->CLUTHeader + (u32)pal);
			if (!desc->CLUTHeader->unpacked) {
				desc->CLUTHeader->data     = (char*)((u32)desc->CLUTHeader->data + (u32)pal);
				desc->CLUTHeader->unpacked = TRUE;
			}
		}
	}
}

/**
 * @note Address: 0x8019FE3C
 * @note Size: 0xEC
 */
s32 CTpl::GetResult()
{
	s32 result = CFile::GetResult();
	if (result == 0) {
		TPLBind((TPLPalette*)mBuffer);
		mBound = TRUE;
	}
	return result;
}

/**
 * @note Address: 0x8019FE08
 * @note Size: 0x34
 */
void CTpl::Close()
{
	CFile::Close();
	mBound = FALSE;
}

/**
 * @note Address: 0x8019FD9C
 * @note Size: 0x6C
 */
// closes the disc or archive handle but keeps the buffer
s32 CFile::CloseHandle()
{
	if (mEntrynum >= 0) { // only when a file is open
		mEntrynum = -1;
		if (mArchive != NULL) {
			if (!ARCClose(&mArcInfo)) {
				return -5;
			}
		} else if (!DVDClose(this)) {
			return -5;
		}
	}
	return 0;
}

// todo: cleanup
/**
 * @note Address: N/A
 */
static void InitTextureLOD(CTexObj* texture)
{
	GXInitTexObjLOD(texture, texture->mImage->minFilter, texture->mImage->magFilter, texture->mImage->minLOD, texture->mImage->maxLOD,
	                texture->mImage->LODBias, GX_FALSE, texture->mImage->edgeLODEnable, GX_ANISO_1);
}
