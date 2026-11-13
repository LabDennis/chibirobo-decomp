#ifndef _GAME_RINGMENU_H
#define _GAME_RINGMENU_H

#include "game/menuhelpchar.h"

class XObjChr;

// one item on the ring
struct OctaMenuItem {
	s32 mType;       // _00, attachment or costume id
	s32 mAvailable;  // _04
	f32 mPulsePhase; // _08, spin, the pulse phase while it's selected
	f32 _0C;         // _0C, never accessed; usa/europe only
	f32 mScale; // _10, icon size: 0.5, grows to 0.7 under the cursor (0.3/0.4 while focused); also widens the ring radius (60 + 20 * scale)
	f32 mHighlight;        // _14
	f32 mSlotOffset;       // _18, horizontal position in the scrolling row
	f32 mTargetSlotOffset; // _1C
#if defined(VERSION_R24J01)
	f32 mRowOffset;
	f32 mTargetRowOffset;
#else
	u8 _20[8];
#endif
};
STATIC_ASSERT(sizeof(OctaMenuItem) == 0x28);

class OctaMenu {
public:
	s32 mOpen;              // _00, above 0 while open, below 0 while it closes
	s32 mCursorIndex;       // _04
	f32 mFade;              // _08, 1 once it has all faded in
	f32 mGlow;              // _0C, glow, 1 when opened from the pad
	f32 _10;                // _10, set at init to -1 on the attachment ring and 1 on the costume ring, never read
	OctaMenuItem mItems[8]; // _14
	f32 mPhase;             // _154
	f32 mOpenScreenX;       // _158, ring center on screen
	f32 mOpenScreenY;       // _15C
	f32 mCursorX;           // _160
	f32 mCursorY;           // _164
	XObjChr* mChr;          // _168
	f32 mOpeningHeading;    // _16C, mChr's heading when it opened
	s32 mLabelLength;       // _170
#if defined(VERSION_JP) || defined(VERSION_R24J01)
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

	s32 mMenuType;                // _280, _268 in japan
	f32 mRowOffset;               // _284, -1.5 on the attachment ring, 0.9 on the costume ring
	s32 mFirstVisibleItem;        // _288, item to start on
	s32 mPendingFirstVisibleItem; // _28C, queued start item, -1 for none
	s32 mFocus;                   // _290, set on the menu that has the cursor
#if defined(VERSION_R24J01)
	s32 mLastPointedItem;
	s32 mKeepPointedItemOnOpen;
#endif
	f32 mHeadTurn; // _294, nan to turn the head with the cursor
#if defined(VERSION_R24J01)
	BOOL UpdatePointedItem(BOOL enabled);
	s32 mPointedItem;
	s32 mPreviousPointedItem;
#endif
};
#if defined(VERSION_R24J01)
STATIC_ASSERT(sizeof(OctaMenu) == 0x290);
#elif defined(VERSION_JP)
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

#endif
