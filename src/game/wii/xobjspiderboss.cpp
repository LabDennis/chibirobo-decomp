#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/wii/infoscrn.h"

// the Wii file of the GameCube's xobjspiderboss.cpp (retail 0x80132330-0x801370BC), its functions in
// the Wii's order

extern "C" {
void ResetSpiderBossState(XObjSpiderBoss* spider);
void ApplySpiderBossPlacement(XObjSpiderBoss* spider);

extern AnimSfxEvent gSpiderBossAnimSfxEvents[];
}

// back to the start: state 0, waiting to be placed
static inline void ResetState(XObjSpiderBoss* spider)
{
	ResetSpiderBossState(spider);
	spider->SetState(0);
	spider->mSpiderStatus.mFlags |= 0x4000000;
}

/**
 * @note Address: 0x80132330
 * @note Size: 0x58
 */
void XObjSpiderBoss::InitializeObject()
{
	XObj::InitializeObject();
	ResetState(this);
}

/**
 * @note Address: 0x80132388
 * @note Size: 0x50
 */
void XObjSpiderBoss::ResetForNewGame()
{
	ResetState(this);
}

/**
 * @note Address: 0x80132AC4
 * @note Size: 0x50
 */
BOOL XObjSpiderBoss::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info)) {
		ResetSpiderBossState(this);
		ApplySpiderBossPlacement(this);
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80132B14
 * @note Size: 0x40
 */
void XObjSpiderBoss::ResetObjectState()
{
	XObj::ResetObjectState();
	mAnimSfx.Init((WiiPlayerState*)this, gSpiderBossAnimSfxEvents);
}

/**
 * @note Address: 0x80132DA8
 * @note Size: 0x14
 */
void XObjSpiderBoss::DrawObject()
{
	DrawWithView(NULL);
}

/**
 * @note Address: 0x80132DBC
 * @note Size: 0x4
 */
void XObjSpiderBoss::DrawGroundShadow(f32 opacity)
{
	XObj::DrawGroundShadow(opacity);
}

/**
 * @note Address: 0x80132DC0
 * @note Size: 0x4
 */
void XObjSpiderBoss::DrawGroundShadowAt(const Vec* origin, f32 opacity)
{
	XObj::DrawGroundShadowAt(origin, opacity);
}

/**
 * @note Address: 0x80132DC4
 * @note Size: 0x4
 */
void XObjSpiderBoss::DrawScreenOverlay()
{
}

/**
 * @note Address: 0x80132DC8
 * @note Size: 0x4
 */
void XObjSpiderBoss::DrawDebugStatus()
{
}

/**
 * @note Address: 0x80132DCC
 * @note Size: 0x4
 */
void XObjSpiderBoss::DrawObjectEffects()
{
}

/**
 * @note Address: 0x80133754
 * @note Size: 0x18
 */
s32 XObjSpiderBoss::OnPlayerContact(s32 unused, s32 value)
{
	mSpiderStatus.mFlags |= 1;
	return value;
}

/**
 * @note Address: 0x80134710
 * @note Size: 0x8C
 */
void XObjSpiderBoss::State0(u32 frames)
{
	mSpiderStatus.mFlags &= ~0x80000U;
	u32 flags = mSpiderStatus.mFlags;
	if (flags & 0x02000000) {
		return;
	}
	if (flags & 0x40000000) {
		mStep      = 0;
		mTimer     = 0;
		mPrevState = mState;
		mState     = 4;
		mAreaPos.x = -200.0f;
		mAreaPos.y = 0.0f;
		mAreaPos.z = -200.0f;
		mAreaWidth = 400.0f;
		mAreaDepth = 400.0f;
	} else {
		mStep      = 0;
		mTimer     = 0;
		mPrevState = mState;
		mState     = 3;
		mStep      = 100;
	}
	mSpiderStatus.mFlags &= ~0x8000U;
}

/**
 * @note Address: 0x801349D4
 * @note Size: 0x58
 */
void XObjSpiderBoss::State2(u32 frames)
{
	Vec zero = { 0.0f, 0.0f, 0.0f }; // retail builds it and never reads it
	switch (mStep) {
	case 0:
		mSpiderStatus.mFlags |= 0x80000;
		mSpiderStatus.mFlags |= 0x10;
		mSpiderStatus.mFlags &= ~0x200U;
		mSpiderStatus.mFlags &= ~0x100000U;
		++mStep;
		break;
	case 1:
		break;
	}
}

/**
 * @note Address: 0x80136FC4
 * @note Size: 0xC
 */
extern "C" bool IsSpiderBossProjectileHitSuppressed(XObjSpiderBoss* spider)
{
	return (spider->mSpiderStatus.mFlags & 0x200) != 0;
}

/**
 * @note Address: 0x80136FD0
 * @note Size: 0xBC
 */
void XObjSpiderBoss::UpdateAnimationSounds()
{
	if (mStatus.mFlags & 1ULL) {
		if (mStatus.mFlags & 0x10000ULL) {
			if (!InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
				s32 id = (mSpiderStatus.mFlags & 0x40000000) ? mAltAnimId : GetAnimId();
				mAnimSfx.Update(id, GetAnimFrame(), 255.0f);
			}
		}
	}
}
