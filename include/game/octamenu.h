#ifndef _GAME_OCTAMENU_H
#define _GAME_OCTAMENU_H

#include "game/xobj.h"
#include "game/ringmenu.h"
#include "game/cptcl.h"

// chibi robo's ring menus, class names from RTTI. AtcMenu picks the attachment, DrsMenu the
// costume, OXMenu is the yes/no pick with the two spinning marks. all of them live in XObjChr

class XObjChr;

// a joint of the menu model bobbing up and down, with its sparkles
struct OctaMenuBob {
	CJObj* mObj;          // _00
	u8 _04[0x30 - 0x4];   // _04
	f32 mPhase;           // _30
	f32 mHeight;          // _34
	u32 _38;              // _38, never accessed
	u8 _3C[0xC0 - 0x3C];  // _3C
	f32 _C0;              // _C0, never accessed
	u8 _C4[0xD8 - 0xC4];  // _C4
	f32 _D8;              // _D8, never accessed
	u8 _DC[0xE4 - 0xDC];  // _DC
	CPtcl mPtcl;          // _E4
	u8 _F8[0xD78 - 0xF8]; // _F8
	HSD_Generator* mGen;  // _D78
	HSD_Generator* mGen2; // _D7C
};
STATIC_ASSERT(sizeof(OctaMenuBob) == 0xD80);

struct OXJointPath {
	s32 ids[7];
};

// one of OXMenu's marks. usa and europe give each mark its own object
struct OXMenuMark {
	f32 mSpin;  // _00
	f32 mTilt;  // _04
	f32 mScale; // _08
#if !defined(VERSION_JP)
	u32 _0C; // _0C, random angle on Open, +3 degrees a frame while the ring is unfocused; only fed to sin() calls whose results are dropped
	XObj mObj; // _10

	OXMenuMark();
	~OXMenuMark();
#endif
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(OXMenuMark) == 0xC);
#else
STATIC_ASSERT(sizeof(OXMenuMark) == 0x990);
#endif

// offsets are usa, japan has no _00 or _14 and its marks are only 0xC each
class OXMenu {
public:
#if !defined(VERSION_JP)
	u32 mLoadStep; // _00, how far loading the two mark models (45, 46) has got, 0 on reset; usa/europe only
#endif
	BOOL mStarted;      // _04
	s32 mCrossMask;     // _08
	s32 mSelectedIndex; // _0C
	f32 mFade;          // _10
#if !defined(VERSION_JP)
	u32 _14; // _14, never accessed; usa/europe only
#endif
	OXMenuMark mMark[2]; // _18
	XObj* mChr;          // _1338, _28 in japan
	f32 mStartRot;       // _133C, the chr's y rotation when it opened
	u32 mConfirmedIndex; // _1340
	// _1344 vtable, _34 in japan

	OXMenu() { mChr = NULL; }

	virtual ~OXMenu() { } // _08 (weak)
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(OXMenu) == 0x38);
#else
STATIC_ASSERT(sizeof(OXMenu) == 0x1348);
#endif

#endif
