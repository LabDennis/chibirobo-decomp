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
	ring.mMenuType    = gCmdLineArgs.mAttachmentMenuType;
	ring.mFocus       = 0;
	ring.mHeadTurn    = NAN;
}

/**
 * @note Address: 0x800BBDAC
 * @note Size: 0x338
 */
void XObjChr::ResetForNewGame()
{
	ResetPlayerStats(&mParams);
	mAttachment           = AttachmentNone;
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
	mOXMenu.mLoadStep = 0;
#endif
	_15890           = 0;
	mSavedTarget     = -1;
	mPendingTarget   = NULL;
	mBatteryDrains   = TRUE;
	mSquirterContent = 0;
	mItemTintWord    = -1;
	for (s32 i = 0; i < 10; ++i) {
		PointPtcl& p = gPointParticlePool[i];
		GetHudPointParticleOrigin(&gGame.mInfoScrn, &p.mStartY, &p.mStartX);
		p.display.mState = p.burst.mState = 0;
	}
	gPlayerSuitChangeEffect.Reset();
	mPlayerStatus.mFlags = 0; // new-game reset clears these before handing control back
	RestoreControl();
}

static inline void SetupPlayerRing(OctaMenu& ring, XObjChr* owner, f32 direction, f32 offset)
{
	ring.mChr = owner;
	ResetRingMode(ring);
	ring._284                     = offset;
	ring.mFirstVisibleItem        = 4;
	ring.mPendingFirstVisibleItem = -1;
	ring._10                      = direction;
}

static void SetRingItems(OctaMenu& ring, const s32* order)
{
	for (s32 i = 0; i < ARRAY_SIZE(ring.mItems); ++i)
		ring.mItems[i].mType = order[i];
}

/**
 * @note Address: 0x800BC0E4
 * @note Size: 0x39C
 */
void XObjChr::InitializeObject(u32 id)
{
	XObj::InitializeObject(id);
	mLook.mActive = FALSE;
	mLook.mMode   = -1;
	mLook.mObject = NULL;
	Vec zero      = { 0.0f, 0.0f, 0.0f };
	mLook.mOffset = zero;
	mLook.mFade = mLook.mPhase = mLook.mPulse = 0.0f;
	mLoadStep                                 = 0;
	mPlug.InitializeObject(id);
	mCopterObj.InitializeObject(id);
	mPopperObj.InitializeObject(id);
	mRadarObj.InitializeObject(id);
	mOXObj.InitializeObject(id);
	InitializeAttachmentMenuEntries(&mAttachments);
	static const s32 attachmentOrder[] = { AttachmentCopter, AttachmentPopper, AttachmentRadar,    AttachmentToothbrush,
		                                   AttachmentMug,    AttachmentSpoon,  AttachmentSquirter, AttachmentUnequip };
	static const s32 costumeOrder[]    = { 3, 5, 4, 1, 6, 8, 7, 2 };
	static const s32 costumeRowOrder[] = { 1, 4, 6, 3, 2, 7, 8, 5 };
	SetupPlayerRing(mMenu.mAtc, this, -1.0f, -1.5f);
	SetRingItems(mMenu.mAtc, attachmentOrder);
	SetupPlayerRing(mMenu.mDrs, this, 1.0f, 0.9f);
	SetRingItems(mMenu.mDrs, mMenu.mDrs.mMenuType == 3 ? costumeRowOrder : costumeOrder);
	mMenu.mPrevSel         = 0;
	mMenu.mSel             = 0;
	mOXMenu.mChr           = this;
	mOXMenu.mSelectedIndex = 0;
#if !defined(VERSION_JP)
	mOXMenu.mLoadStep = 0;
#endif
	for (s32 i = 0; i < 9; ++i) { // every suit model
		mSuits.mObjects[i].InitializeObject(id);
		mSuits.mJoints[i] = NULL;
	}
	mPad.Init(0);
	ResetForNewGame();
	mAttachmentAnim.Clear();
	mSuitAnim.Clear();
}
