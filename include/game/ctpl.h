#ifndef _GAME_CTPL_H
#define _GAME_CTPL_H

#ifdef _MTXTYPES_h_
// built with the HSD headers, which have no texture objects, so the sdk types the class needs are here
#include <dolphin/gx/gxtypes.h>
typedef u8 GXBool;
typedef struct _GXTexObj {
	u32 pad[8]; // _00
} GXTexObj;
typedef struct _GXTlutObj {
	u32 padding[3]; // _00
} GXTlutObj;
typedef enum _GXTexMtxType { GX_MTX3x4 = 0, GX_MTX2x4 = 1 } GXTexMtxType;
extern "C" {
u16 GXGetTexObjWidth(const GXTexObj* obj);
u16 GXGetTexObjHeight(const GXTexObj* obj);
}
#else
#include "Dolphin/gx.h"
#endif
// files built with the HSD headers already have Vec and Mtx from them
#ifndef _MTXTYPES_h_
#include "Dolphin/mtx.h"
#endif
#include "Dolphin/tpl.h"
#include "game/cfile.h"

#if defined(VERSION_R24J01)
// The Wii texture stores the pixel and palette addresses from the TPL headers.
class CTlutObj : public GXTlutObj {
public:
	u32 mTlutName; // _0C

	virtual ~CTlutObj() { } // _10 vtable
};
STATIC_ASSERT(sizeof(CTlutObj) == 0x14);

// the texture matrices and the flag that has the last one rebuilt. Copying a CTexObj moves them as one
// block (0x800ABE94), so they are one member; the struct's name is ours
struct CTexObjMtx {
	bool mDirty; // _00
	Mtx mMtx[4]; // _04
};

class CTexObj : public _GXTexObj {
public:
	void* mImageData;   // _20
	void* mClutData;    // _24
	CTlutObj mTlut;     // _28
	CTexObjMtx mTexMtx; // _3C
	// _100 vtable

	virtual ~CTexObj() { }
	void Init(TPLDescriptor* desc, u32 tlutName);
	void Init(void* image, u16 width, u16 height, GXTexFmt format, GXTexWrapMode wrapS, GXTexWrapMode wrapT, GXBool mipmap);
	void Init(void* image, u16 width, u16 height, GXTexFmt format, GXTexWrapMode wrapS, GXTexWrapMode wrapT, GXBool mipmap, u32 tlutName);
	void InitMatrices();
	void LoadTexture(GXTexMapID map);
	void Load(GXTexMapID map, u32 mtxId, GXTexMtxType mtxType);
	u32 Set(const CTexObj& other);
	u32 SetChar(const char* str, u32 font); // the font index goes to GetFontGlyphTexture's arg3
	void Draw(GXTexMapID map, u32 mtxId, GXTexMtxType mtxType, const Vec* pos, u32 color0, u32 color1, u32 color2, u32 color3);
	void DrawLit(GXTexMapID map, u32 mtxId, GXTexMtxType mtxType, const Vec* pos, u32 color0, u32 color1, u32 color2, u32 color3);
};
STATIC_ASSERT(sizeof(CTexObj) == 0x104);
#else
// a GX texture with its palette and texture matrix. class names are RTTI strings,
class CTexObj : public _GXTexObj {
public:
	// _00 _GXTexObj
	TPLHeader* mImage;    // _20
	TPLClutHeader* mClut; // _24
	GXTlutObj mTlutObj;   // _28
	u32 mTlutName;        // _34
	BOOL mMtxDirty;       // _38, the next draw rebuilds mMtx[3]
	Mtx mMtx[4];          // _3C, the last is the first times the third times the second, loaded as the texture matrix
	// _FC vtable

	CTexObj() { }
	virtual ~CTexObj() { } // _08 (weak)

	CTexObj& Init(void* image, u16 width, u16 height, GXTexFmt format, GXTexWrapMode wrapS, GXTexWrapMode wrapT, GXBool mipmap);
	CTexObj& Init(TPLDescriptor* desc, u32 tlutName);
	u32 Set(const CTexObj& other);
	u32 SetChar(const char* str);
	u16 GetWidth() const { return GXGetTexObjWidth(this); }
	u16 GetHeight() const { return GXGetTexObjHeight(this); }
	void Draw(GXTexMapID map, u32 mtxId, GXTexMtxType mtxType, const Vec* pos, u32 color0, u32 color1, u32 color2, u32 color3);
	void DrawLit(GXTexMapID map, u32 mtxId, GXTexMtxType mtxType, const Vec* pos, u32 color0, u32 color1, u32 color2, u32 color3);

	// inlined:
	void Load(GXTexMapID map, u32 mtxId, GXTexMtxType mtxType);
};
typedef char CTexObj_size_check[sizeof(CTexObj) == 0x100 ? 1 : -1];
#endif

// HUD and status textures start two entries earlier in the Japanese palette.
#if defined(VERSION_JP)
const int StatusTextureOffset = 2;
#else
const int StatusTextureOffset = 0;
#endif

// a .tpl file, bound once it's read in
class CTpl : public CFile {
public:
#if defined(VERSION_R24J01)
	virtual ~CTpl() // _08 (weak), 0x80018224
	{
		mBound   = false;
		mPalette = NULL;
	}
	virtual s32 ReadAsync(const char* path, u32 offset, u32 size, BOOL fromAram); // _14
#else
	virtual ~CTpl() { mBound = FALSE; } // _08 (weak)
#endif
	virtual s32 GetResult(); // _18
#if defined(VERSION_R24J01)
	virtual BOOL Close();  // _20
	void Bind(void* data); // converts the offsets in a just-loaded palette
	void BindPalette(TPLPalette* pal);
#else
	virtual void Close(); // _20
#endif

#if defined(VERSION_R24J01)
	bool mBound;          // _EC
	TPLPalette* mPalette; // _F0
	CTexObj mTex;         // _F4
#else
	CTexObj mTex; // _EC
	BOOL mBound;  // _1EC
#endif
};
#if defined(VERSION_R24J01)
STATIC_ASSERT(sizeof(CTpl) == 0x1f8);
#else
typedef char CTpl_size_check[sizeof(CTpl) == 0x1F0 ? 1 : -1];
#endif

#endif
