#ifndef _GAME_OCTAMENU_H
#define _GAME_OCTAMENU_H

#include "game/xobj.h"
#include "game/menuhelpchar.h"
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
	u32 _38;              // _38
	u8 _3C[0xC0 - 0x3C];  // _3C
	f32 _C0;              // _C0
	u8 _C4[0xD8 - 0xC4];  // _C4
	f32 _D8;              // _D8
	u8 _DC[0xE4 - 0xDC];  // _DC
	CPtcl mPtcl;          // _E4
	u8 _F8[0xD78 - 0xF8]; // _F8
	HSD_Generator* mGen;  // _D78
	HSD_Generator* mGen2; // _D7C
};
STATIC_ASSERT(sizeof(OctaMenuBob) == 0xD80);

// one item on the ring
struct OctaMenuItem {
	s32 mType;             // _00, attachment or costume id
	s32 mAvailable;        // _04
	f32 _08;               // _08, spin, the pulse phase while it's selected
	f32 _0C;               // _0C
	f32 _10;               // _10
	f32 mHighlight;        // _14
	f32 mSlotOffset;       // _18, horizontal position in the scrolling row
	f32 mTargetSlotOffset; // _1C
	u8 _20[8];             // _20
};
STATIC_ASSERT(sizeof(OctaMenuItem) == 0x28);

class OctaMenu {
public:
	s32 mOpen;              // _00, above 0 while open, below 0 while it closes
	s32 mCursorIndex;       // _04
	f32 mFade;              // _08, 1 once it has all faded in
	f32 _0C;                // _0C, glow, 1 when opened from the pad
	f32 _10;                // _10
	OctaMenuItem mItems[8]; // _14
	f32 mPhase;             // _154
	f32 mOpenScreenX;       // _158, ring center on screen
	f32 mOpenScreenY;       // _15C
	f32 mCursorX;           // _160
	f32 mCursorY;           // _164
	XObjChr* mChr;          // _168
	f32 mOpeningHeading;    // _16C, mChr's heading when it opened
	s32 mLabelLength;       // _170
#if defined(VERSION_JP)
	MenuHelpChar mLabelChars[20]; // _174
#else
	MenuHelpChar mLabelChars[22]; // _174
#endif
	// _27C vtable, _264 in japan

	OctaMenu() { mChr = NULL; }

	virtual ~OctaMenu() { }                         // _08 (weak)
	virtual void Open(s32 item);                    // _0C
	virtual void Select(s32 dir);                   // _10
	virtual void OpenActive();                      // _14, Open(0): the ring that has the cursor
	virtual void FinishClose();                     // _18, marks the ring closed once its fade reached 0
	virtual void ConfirmSelection();                // _1C
	virtual s32 GetItemIconTextureIndex(s32 index); // _20
	virtual const char* GetItemLabel(s32 index);    // _24
	virtual u32 GetItemColor(s32 index);            // _28
	virtual BOOL IsItemEquipped(s32 index);         // _2C, the attachment held or the costume worn

	s32 _280;                     // _280, _268 in japan
	f32 _284;                     // _284
	s32 mFirstVisibleItem;        // _288, item to start on
	s32 mPendingFirstVisibleItem; // _28C, queued start item, -1 for none
	s32 mFocus;                   // _290, set on the menu that has the cursor
	f32 mHeadTurn;                // _294, nan to turn the head with the cursor
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(OctaMenu) == 0x280);
#else
STATIC_ASSERT(sizeof(OctaMenu) == 0x298);
#endif

class AtcMenu : public OctaMenu {
public:
	virtual ~AtcMenu() { }                          // _08 (weak)
	virtual void Open(s32 item);                    // _0C
	virtual void ConfirmSelection();                // _1C
	virtual s32 GetItemIconTextureIndex(s32 index); // _20
	virtual const char* GetItemLabel(s32 index);    // _24
	virtual u32 GetItemColor(s32 index);            // _28
	virtual BOOL IsItemEquipped(s32 index);         // _2C
};

class DrsMenu : public OctaMenu {
public:
	virtual ~DrsMenu() { }                          // _08
	virtual void Open(s32 item);                    // _0C
	virtual void ConfirmSelection();                // _1C
	virtual s32 GetItemIconTextureIndex(s32 index); // _20
	virtual const char* GetItemLabel(s32 index);    // _24
	virtual u32 GetItemColor(s32 index);            // _28
	virtual BOOL IsItemEquipped(s32 index);         // _2C
};

struct OXJointPath {
	s32 ids[7];
};

// one of OXMenu's marks. usa and europe give each mark its own object
struct OXMenuMark {
	f32 mSpin;  // _00
	f32 mTilt;  // _04
	f32 mScale; // _08
#if !defined(VERSION_JP)
	u32 _0C;   // _0C
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
	u32 _00; // _00
#endif
	BOOL mStarted;      // _04
	s32 mCrossMask;     // _08
	s32 mSelectedIndex; // _0C
	f32 mFade;          // _10
#if !defined(VERSION_JP)
	u32 _14; // _14
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
