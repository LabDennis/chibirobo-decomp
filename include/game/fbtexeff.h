#ifndef _GAME_FBTEXEFF_H
#define _GAME_FBTEXEFF_H

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
