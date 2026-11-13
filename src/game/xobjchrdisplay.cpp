#include "game/xobjchrstate.h"
#include "game/cmdline.h"
#include <math.h>

extern "C" {
void SetPlayerPopperMountHidden(XObj*, BOOL);
void InitializePlayerLiftAction(XActLift*, XObjChr*);
void RefreshAttachmentMenuEntries(AttachmentValues*);
void InitializeAttachmentMenuEntries(AttachmentValues*);
}

static inline void ResetRingMode(OctaMenu& ring)
{
	ring.mOpen        = 0;
	ring.mCursorIndex = 0;
	u32 mode          = gCmdLineArgs.mAttachmentMenuType;
	if (mode <= 3)
		ring._280 = mode;
	else
		ring._280 = 0;
	ring.mFocus    = 0;
	ring.mHeadTurn = NAN;
}

/**
 * @note Address: 0x800BBDAC
 * @note Size: 0x338
 */
// NONMATCHING: retail writes gPlayerSuitChangeEffect without the extra compare/branch this source produces
void XObjChr::ResetForNewGame()
{
	ResetPlayerStats(&mParams);
	mAttachment           = 0;
	mLiftPlugFlags        = gCmdLineArgs.mLiftPlugFlags;
	mAttachmentActionType = gCmdLineArgs.mAttachmentActionType;
	mToolActionType       = gCmdLineArgs.mToolActionType;
	SetPlayerPopperMountHidden(this, FALSE);
	InitializePlayerLiftAction(&mLift, this);
	mPlug.ResetForNewGame();
	mCopterObj.ResetForNewGame();
	mPopperObj.ResetForNewGame();
	mRadarObj.ResetForNewGame();
	mOXObj.ResetForNewGame();
	mSuits.mLoadStep = mSuits.mChangeStep = mSuits.mRequested = mSuits.mCurrent = mSuits.mSpecial = 0;
	for (s32 i = 0; i < 9; ++i)
		mSuits.mObjects[i].ResetForNewGame();
	ApplyPlayerSuit(this);
	mPad.mReset = TRUE;
	RefreshAttachmentMenuEntries(&mAttachments);
	ResetRingMode(mMenu.mAtc);
	ResetRingMode(mMenu.mDrs);
	mMenu.mPrevSel = mMenu.mSel = 0;
	mOXMenu.mSelectedIndex      = 0;
#if !defined(VERSION_JP)
	mOXMenu._00 = 0;
#endif
	_15890         = 0;
	mSavedTarget   = -1;
	mPendingTarget = NULL;
	_158A4         = TRUE;
	_158A8         = 0;
	mItemTintWord  = -1;
	for (s32 i = 0; i < 10; ++i) {
		PointPtcl& p = gPointParticlePool[i];
		GetHudPointParticleOrigin(&gGame.mInfoScrn, &p.mStartY, &p.mStartX);
		p.display.mState = p.burst.mState = 0;
	}
	gPlayerSuitChangeEffect.active = FALSE;
	// NONMATCHING: retail emits the clear as two branch-free straight-line runs
	// (one base for rings[0], a second base at +0xf0 for rings[1]). Nested loops
	// and two separate loops both keep a bound check under MWCC, so the flat
	// loop stays as the honest form.
	for (s32 i = 0; i < 20; ++i) {
		gPlayerSuitChangeEffect.rings[0][i].scale = 0.0f;
		gPlayerSuitChangeEffect.rings[0][i].angle = 0.0f;
	}
	mPlayerStatus.mFlags = 0;
	mPlayerStatus.mFlags = 0;
	mPlayerStatus.mFlags |= 0x20000000;
	mPlayerStatus.mFlags |= 7;
	mPad.mReset = TRUE;
}

static inline void SetupPlayerRing(OctaMenu& ring, XObjChr* owner, f32 direction)
{
	ring.mChr = owner;
	ResetRingMode(ring);
	ring._284                     = 0.0f;
	ring.mFirstVisibleItem        = 0;
	ring.mPendingFirstVisibleItem = -1;
	ring._10                      = direction;
}

static inline void SetRingItems(OctaMenu& ring, s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g, s32 h)
{
	ring.mItems[0].mType = a;
	ring.mItems[1].mType = b;
	ring.mItems[2].mType = c;
	ring.mItems[3].mType = d;
	ring.mItems[4].mType = e;
	ring.mItems[5].mType = f;
	ring.mItems[6].mType = g;
	ring.mItems[7].mType = h;
}

/**
 * @note Address: 0x800BC0E4
 * @note Size: 0x39C
 */
// NONMATCHING: same code size; only regalloc and store ordering differ
void XObjChr::InitializeObject(u32 id)
{
	InitializeObject__4XObjFUl(this, id);
	mLook.mActive = FALSE;
	mLook.mMode   = -1;
	mLook.mObject = NULL;
	Vec zero      = { 0.0f, 0.0f, 0.0f };
	mLook.mOffset = zero;
	mLook.mFade = mLook.mPhase = mLook.mPulse = 0.0f;
	mLook.mObject                             = NULL;
	mLoadStep                                 = 0;
	mPlug.InitializeObject(id);
	mCopterObj.InitializeObject(id);
	mPopperObj.InitializeObject(id);
	mRadarObj.InitializeObject(id);
	mOXObj.InitializeObject(id);
	InitializeAttachmentMenuEntries(&mAttachments);
	{
		OctaMenu& ring = mMenu.mAtc;
		SetupPlayerRing(ring, this, -1.0f);
		SetRingItems(ring, 1, 2, 3, 4, 6, 5, 7, 8);
		ring._284              = -1.5f;
		ring.mFirstVisibleItem = 4;
	}
	{
		OctaMenu& ring = mMenu.mDrs;
		SetupPlayerRing(ring, this, 1.0f);
		if (ring._280 == 3) {
			SetRingItems(ring, 1, 4, 6, 3, 2, 7, 8, 5);
		} else {
			SetRingItems(ring, 3, 5, 4, 1, 6, 8, 7, 2);
		}
		ring._284              = 0.9f;
		ring.mFirstVisibleItem = 4;
	}
	mMenu.mPrevSel         = 0;
	mMenu.mSel             = 0;
	mOXMenu.mChr           = this;
	mOXMenu.mSelectedIndex = 0;
#if !defined(VERSION_JP)
	mOXMenu._00 = 0;
#endif
	for (s32 i = 0; i < 9; ++i) {
		mSuits.mObjects[i].InitializeObject(id);
		mSuits.mJoints[i] = NULL;
	}
	mPad.Init(0);
	ResetForNewGame();
	mAttachmentAnim.Clear();
	mSuitAnim.Clear();
}
