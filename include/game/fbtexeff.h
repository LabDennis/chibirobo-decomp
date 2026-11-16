#ifndef _GAME_FBTEXEFF_H
#define _GAME_FBTEXEFF_H

#include "Version.h"
#if defined(VERSION_R24J01)
#include "game/wii/cgame.h"

// Wii FbSubwin - partial: only the members below are verified, by the retail functions'
// offsets (CreateFbTexture writes _04/_06, Init stores _58/_5C/_BC, the function-table
// pointer is at _124, four bytes later than the GameCube class because WiiFbTexture is
// one word bigger, _128/_134 are the fade state, the camera is embedded at _148). The
// gaps stay unverified.
class FbSubwin {
public:
	u8 _000[0x4];
	u16 mWidth;    // _04, written by CreateFbTexture
	u16 mHeight;   // _06, written by CreateFbTexture
	u8 _008[0x50]; // unverified (rest of the WiiFbTexture head)
	u8 mTexDirty;  // _58, texture-matrix flag, set TRUE around the matrix writes
	u8 _059[0x3];
	Mtx mTexMtx[4]; // _5C, the texture's matrices; Init writes [0] and [2]
	u8 _11C[0x8];
	void (**mVtable)(FbSubwin*); // _124
	BOOL mEnabled;               // _128, fades in while set, out while not
	u32 mFlags;                  // _12C, Draw tests bit 2
	u8 _130[0x4];
	f32 mAmount;     // _134, 0 to 1
	f32 mRectX;      // _138, centre x of the window's rect on screen
	f32 mRectY;      // _13C, centre y of the window's rect on screen
	f32 mRectWidth;  // _140, window width, NaN (with mRectHeight) picks the round frame
	f32 mRectHeight; // _144, window height, NaN picks the round frame
	CCamera mCamera; // _148

	void Init();
	void Reset();
	void Update(u32 frames);
	void Copy();
	void Draw();
};

extern "C" void ConfigureSubwindowCamera(FbSubwin* subwin, const Vec& interest, f32 angleY, f32 angleX, f32 distance, f32 fov);

// frame buffer effects that draw through a copied texture; the Wii base keeps the bigger
// WiiFbTexture and drops mActive (WiiFbTexture::mOwnsImage took its slot), the vtable sits
// at _124 right behind the texture.
class CFbTexEff {
public:
	WiiFbTexture mTexture; // _00
	// _124 vtable

	virtual void Init();             // _08
	virtual void Reset();            // _0C
	virtual void Copy();             // _10
	virtual void Update(u32 frames); // _14
	virtual void Draw();             // _18
};

// the model object a mirror binds to: a stage object's scene carrier. Only what the retail
// functions prove is filled in - the vtable's _50 finds a node by name ("mirror" is the
// mirror surface), _A0 returns a node's matrix, _EC is the model's nw4r resource handle and
// _F0 a related data object. The name is ours.
class CMirrorObj {
public:
	virtual void vf08();
	virtual void vf0C();
	virtual void vf10();
	virtual void vf14();
	virtual void vf18();
	virtual void vf1C();
	virtual void vf20();
	virtual void vf24();
	virtual void vf28();
	virtual void vf2C();
	virtual void vf30();
	virtual void vf34();
	virtual void vf38();
	virtual void vf3C();
	virtual void vf40();
	virtual void vf44();
	virtual void vf48();
	virtual void vf4C();
	virtual void* vf50(const char* name); // _50
	virtual void vf54();
	virtual void vf58();
	virtual void vf5C();
	virtual void vf60();
	virtual void vf64();
	virtual void vf68();
	virtual void vf6C();
	virtual void vf70();
	virtual void vf74();
	virtual void vf78();
	virtual void vf7C();
	virtual void vf80();
	virtual void vf84();
	virtual void vf88();
	virtual void vf8C();
	virtual void vf90();
	virtual void vf94();
	virtual void vf98();
	virtual void vf9C();
	virtual MtxP vfA0(void** node); // _A0

	u8 _04[0xE8];
	u32 mMdl;  // _EC, the model's nw4r ResMdl handle
	void* mF0; // _F0, the object the node matrices live in
	u8 _F4[0x150 - 0xF4];
	u64 mStatus; // _150, the object's status flags
};

// a mirror, renders the scene again through a camera reflected in a named node's plane;
// the Wii version keeps its own 256-square texture and binds the model's "mirror_mat"
// material animation so the mirror shows the copy
class FbMirror : public CFbTexEff {
public:
	CMirrorObj* mJoint;     // _128
	void* mJointAnm;        // _12C, the ResAnmTexPat bound to "mirror_mat"
	u8 mJointAnmCopy[0x20]; // _130, a copy of the anm with the mirror's texture patched in
	CCamera mCamera;        // _150
	BOOL mVisible;          // _348, the eye is in front of the mirror
	Mtx mView;              // _34C

	virtual void Init();             // _08, Init__8FbMirrorFv
	virtual void Reset();            // _0C, Reset__8FbMirrorFv
	virtual void Copy();             // _10, Copy__8FbMirrorFv
	virtual void Update(u32 frames); // _14, Update__8FbMirrorFUl
	virtual void Draw();             // _18, Draw__8FbMirrorFv

	void SetMirror(CMirrorObj* obj); // fn_80019F24
	bool IsUsed() const { return mJoint ? true : false; }
};
STATIC_ASSERT(sizeof(FbMirror) == 0x37C);

// the game's mirrors
extern "C" FbMirror gFramebufferMirrors[5];

#else
#include "game/ccamera.h"
#include "game/ctpl.h"

class XObj;

// a frame buffer copy and the texture that shows it
struct FbTexture {
	void* mImage;     // _00
	u16 mWidth;       // _04
	u16 mHeight;      // _06
	GXTexFmt mFormat; // _08
	u32 _0C;          // _0C, written 0 when the subwindow and mirror textures are set up, never read
	f32 mX;           // _10
	f32 mY;           // _14
	u8 mMipmap;       // _18, copies the efb at twice the size
	CTexObj mTex;     // _1C
};
STATIC_ASSERT(sizeof(FbTexture) == 0x11C);

// frame buffer effects that draw through a copied texture, class names and bases from RTTI.
// the virtuals' names are ours, the base ones are all empty
class CFbTexEff {
public:
	FbTexture mTexture; // _00
	BOOL mActive;       // _11C
	// _120 vtable

	virtual void Init();             // _08
	virtual void Reset();            // _0C
	virtual void Copy();             // _10
	virtual void Update(u32 frames); // _14
	virtual void Draw();             // _18
};
STATIC_ASSERT(sizeof(CFbTexEff) == 0x124);

// the small window the camera's second view goes in, CGame keeps it at _42AC
class FbSubwin : public CFbTexEff {
public:
	BOOL mEnabled;   // _124, fades in while set, out while not
	CStatus mFlags;  // _128
	f32 mAmount;     // _130, 0 to 1
	f32 mRectX;      // _134, centre x of the window's rect on screen
	f32 mRectY;      // _138, centre y of the window's rect on screen
	f32 mRectWidth;  // _13C, window width, NaN (with mRectHeight) picks the round frame
	f32 mRectHeight; // _140, window height, NaN picks the round frame
	CCamera mCamera; // _144

	virtual void Init();             // _08, Init__8FbSubwinFv
	virtual void Reset();            // _0C, Reset__8FbSubwinFv
	virtual void Copy();             // _10, Copy__8FbSubwinFv
	virtual void Update(u32 frames); // _14, Update__8FbSubwinFUl
	virtual void Draw();             // _18, Draw__8FbSubwinFv
};
STATIC_ASSERT(sizeof(FbSubwin) == 0x2D8);

extern "C" void ConfigureSubwindowCamera(FbSubwin* subwin, const Vec& interest, f32 angleY, f32 angleX, f32 distance, f32 fov);

// a mirror, renders the scene again through a camera reflected in a joint's plane
class FbMirror : public CFbTexEff {
public:
	HSD_JObj* mJoint; // _124, the mirror's surface
	CCamera mCamera;  // _128
	BOOL mVisible;    // _2BC, the eye is in front of the mirror
	Mtx mView;        // _2C0

	virtual void Init();             // _08, Init__8FbMirrorFv
	virtual void Reset();            // _0C, Reset__8FbMirrorFv
	virtual void Copy();             // _10, Copy__8FbMirrorFv
	virtual void Update(u32 frames); // _14, Update__8FbMirrorFUl
	virtual void Draw();             // _18, Draw__8FbMirrorFv

	bool IsUsed() const { return mJoint ? true : false; }
};
STATIC_ASSERT(sizeof(FbMirror) == 0x2F0);

// a mirror that belongs to an object and hides with it
class FbMirrorEx : public FbMirror {
public:
	XObj* mOwner; // _2F0

	FbMirrorEx();
	~FbMirrorEx();

	virtual void Init(); // _08
};
STATIC_ASSERT(sizeof(FbMirrorEx) == 0x2F4);

// the game's mirrors
extern "C" FbMirrorEx gFramebufferMirrors[5];
#endif

#endif
