#ifndef _GAME_CTPL_H
#define _GAME_CTPL_H

#ifdef _MTXTYPES_h_
// built with the HSD headers, which have no texture objects, so the sdk types the class needs are here
#include <dolphin/gx/gxtypes.h>
typedef u8 GXBool;
typedef struct _GXTexObj {
	u8 pad[0x20]; // _00
} GXTexObj;
typedef struct _GXTlutObj {
	u8 padding[0xC]; // _00
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

	CTexObj();
	virtual ~CTexObj(); // _08 (weak)

	CTexObj& Init(void* image, u16 width, u16 height, GXTexFmt format, GXTexWrapMode wrapS, GXTexWrapMode wrapT, GXBool mipmap);
	CTexObj& Init(TPLDescriptor* desc, u32 tlutName);
	u32 Set(const CTexObj& other);
	u32 SetChar(const char* str);
	void Draw(GXTexMapID map, u32 mtxId, GXTexMtxType mtxType, const Vec* pos, u32 color0, u32 color1, u32 color2, u32 color3);
	void DrawLit(GXTexMapID map, u32 mtxId, GXTexMtxType mtxType, const Vec* pos, u32 color0, u32 color1, u32 color2, u32 color3);

	// inlined:
	void Load(GXTexMapID map, u32 mtxId, GXTexMtxType mtxType);
};
#ifndef DEFER_CTEXOBJ_LIFETIME
#include "game/ctexobj_lifetime.inl"
#endif
typedef char CTexObj_size_check[sizeof(CTexObj) == 0x100 ? 1 : -1];

// HUD and status textures start two entries earlier in the Japanese palette.
#if defined(VERSION_JP)
const int StatusTextureOffset = 2;
#else
const int StatusTextureOffset = 0;
#endif

// a .tpl file, bound once it's read in
class CTpl : public CFile {
public:
#ifdef STAGE_EXTERNAL_DTOR
	STAGE_EXTERNAL_DTOR
#endif
	virtual ~CTpl() { mBound = FALSE; } // _08 (weak)
	virtual s32 GetResult();            // _18
	virtual void Close();               // _20

	CTexObj mTex; // _EC
	BOOL mBound;  // _1EC
};
typedef char CTpl_size_check[sizeof(CTpl) == 0x1F0 ? 1 : -1];

#endif
