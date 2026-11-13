#ifndef _GAME_CPRIMITIVE_H
#define _GAME_CPRIMITIVE_H

#include "game/ctpl.h"
#ifdef _MTXTYPES_h_
// built with the HSD headers, which don't have the primitive types or these two
typedef enum _GXPrimitive {
	GX_POINTS        = 0xB8,
	GX_LINES         = 0xA8,
	GX_LINESTRIP     = 0xB0,
	GX_TRIANGLES     = 0x90,
	GX_TRIANGLESTRIP = 0x98,
	GX_TRIANGLEFAN   = 0xa0,
	GX_QUADS         = 0x80,
} GXPrimitive;
typedef MtxPtr MtxP;
extern "C" {
void PSMTXRotRad(Mtx mtx, char axis, f32 angle);
void PSMTXTrans(Mtx mtx, f32 xT, f32 yT, f32 zT);
}
#else
#include "Dolphin/gx.h"
#include "Dolphin/mtx.h"
#endif

// simple shapes CGs draws for debug and effects. class names are RTTI, the rest are ours

// one vertex of a line or triangle list
struct PrimVertex {
	Vec pos;   // _00
	u32 color; // _0C
};

// the base: a transform built from translate, scale and three rotations, rebuilt
// on the next draw after any of them changes
class CPrimitive {
public:
	BOOL mDirty;     // _00
	u8 mBillboard;   // _04, 1 and 2 turn the model view to face the camera
	f32 mBillboardZ; // _08
	Mtx mTrans;      // _0C
	Mtx mScale;      // _3C
	Mtx mRotX;       // _6C
	Mtx mRotY;       // _9C
	Mtx mRotZ;       // _CC
	Mtx mModel;      // _FC
	Mtx mModelView;  // _12C
	s32 mRotOrder;   // _15C, which way round the three rotations go, 4 is z x y
	MtxP mFixedMtx;  // _160, used as is instead of the transform when set
	// _164 vtable

	virtual ~CPrimitive() { }                                                                  // _08
	virtual void Init(f32 tx, f32 ty, f32 tz, f32 sx, f32 sy, f32 sz, f32 rx, f32 ry, f32 rz); // _0C

	void Set(f32 tx, f32 ty, f32 tz, f32 sx, f32 sy, f32 sz, f32 rx, f32 ry, f32 rz);
	MtxP LoadMtx(Mtx view);

	void SetTrans(f32 x, f32 y, f32 z)
	{
		mDirty = TRUE;
		PSMTXTrans(mTrans, x, y, z);
	}
	void SetScale(f32 x, f32 y, f32 z)
	{
		mDirty = TRUE;
		PSMTXScale(mScale, x, y, z);
	}
	void SetRot(f32 x, f32 y, f32 z)
	{
		mDirty = TRUE;
		PSMTXRotRad(mRotX, 'x', x);
		PSMTXRotRad(mRotY, 'y', y);
		PSMTXRotRad(mRotZ, 'z', z);
	}
	void SetBillboard(u8 billboard)
	{
		mBillboard = billboard;
		mDirty     = TRUE;
	}
};
typedef char CPrimitive_size_check[sizeof(CPrimitive) == 0x168 ? 1 : -1];

class CLine : public CPrimitive {
public:
	u32 _168; // _168

	virtual ~CLine() { } // _08

	void Draw(Mtx view, PrimVertex* vtx, u16 count);
	void Draw(Mtx view, Vec* pos, u32* color, u16 count);
	void Draw(Mtx view, Vec* pos, u32 color, u16 count);
};
typedef char CLine_size_check[sizeof(CLine) == 0x16C ? 1 : -1];

// lit draws give the whole list the normal of its first three points
class CTriangle : public CPrimitive {
public:
	u32 _168; // _168

	virtual ~CTriangle() { } // _08

	void DrawLit(Mtx view, PrimVertex* vtx, u16 count, GXPrimitive type);
	void Draw(Mtx view, PrimVertex* vtx, u16 count, GXPrimitive type);
	void DrawLit(Mtx view, Vec* pos, u32* color, u16 count, GXPrimitive type);
	void Draw(Mtx view, Vec* pos, u32* color, u16 count, GXPrimitive type);
	void DrawLit(Mtx view, Vec* pos, u32 color, u16 count, GXPrimitive type);
	void Draw(Mtx view, Vec* pos, u32 color, u16 count, GXPrimitive type);
};
typedef char CTriangle_size_check[sizeof(CTriangle) == 0x16C ? 1 : -1];

// a unit circle fan, the size comes from the scale
class CCircle : public CPrimitive {
public:
	u32 _168;    // _168
	u32 _16C;    // _16C
	Vec mCenter; // _170
	// _17C

	virtual ~CCircle() { }                                                                     // _08
	virtual void Init(f32 tx, f32 ty, f32 tz, f32 sx, f32 sy, f32 sz, f32 rx, f32 ry, f32 rz); // _0C

	void DrawLit(Mtx view, u32 segments, u32 centerColor, u32 edgeColor);
	void Draw(Mtx view, u32 segments, u32 centerColor, u32 edgeColor);
};
typedef char CCircle_size_check[sizeof(CCircle) == 0x17C ? 1 : -1];

// a two by two quad with a color per corner
class CTile : public CPrimitive {
public:
	u32 mColor0; // _168
	u32 mColor1; // _16C
	u32 mColor2; // _170
	u32 mColor3; // _174
	Vec mCenter; // _178

	virtual ~CTile() { }                                                                       // _08
	virtual void Init(f32 tx, f32 ty, f32 tz, f32 sx, f32 sy, f32 sz, f32 rx, f32 ry, f32 rz); // _0C

	void DrawLit(Mtx view, u32 color0, u32 color1, u32 color2, u32 color3);
	void Draw(Mtx view, u32 color0, u32 color1, u32 color2, u32 color3);
};
typedef char CTile_size_check[sizeof(CTile) == 0x184 ? 1 : -1];

// a textured quad, the drawing is CTexObj's
class CSprite : public CPrimitive {
public:
	CTexObj* mTex; // _168
	u32 _16C;      // _16C
	u32 _170;      // _170
	u32 _174;      // _174
	u32 _178;      // _178
	Vec mCenter;   // _17C

	virtual ~CSprite() { }                                                                     // _08
	virtual void Init(f32 tx, f32 ty, f32 tz, f32 sx, f32 sy, f32 sz, f32 rx, f32 ry, f32 rz); // _0C

	void DrawLit(Mtx view, GXTexMapID map, u32 mtxId, GXTexMtxType mtxType, u32 color0, u32 color1, u32 color2, u32 color3);
	void Draw(Mtx view, GXTexMapID map, u32 mtxId, GXTexMtxType mtxType, u32 color0, u32 color1, u32 color2, u32 color3);
};
typedef char CSprite_size_check[sizeof(CSprite) == 0x188 ? 1 : -1];

class CCube : public CPrimitive {
public:
	virtual ~CCube() { } // _08

	void Draw(Mtx view);
};

class COctahedron : public CPrimitive {
public:
	virtual ~COctahedron() { } // _08
};

class CDodeca : public CPrimitive {
public:
	virtual ~CDodeca() { } // _08
};

class CIcosahedron : public CPrimitive {
public:
	virtual ~CIcosahedron() { } // _08
};

class CSphere : public CPrimitive {
public:
	virtual ~CSphere() { } // _08

	void Draw(Mtx view, u8 major, u8 minor);
};

class CCylinder : public CPrimitive {
public:
	virtual ~CCylinder() { } // _08

	void Draw(Mtx view, u8 sides);
};

class CTorus : public CPrimitive {
public:
	virtual ~CTorus() { } // _08
};

#endif
