#include "game/xobjspiderboss.h"
#include "game/xchr.h"
#include "game/cgame.h"
#include "game/cparticle.h"

/**
 * @note Address: 0x801051A0
 * @note Size: 0x14
 */
s32 XObjSpiderBoss::OnPlayerContact(s32 unused, s32 value)
{
	mSpiderStatus.mFlags |= 1;
	return value;
}

/**
 * @note Address: 0x801051B4
 * @note Size: 0x1A8
 */
// hit from pos, only once chibi robo has the item that does it
void XObjSpiderBoss::OnHitFromPosition(const Vec& pos, f32 strength)
{
	u32 flags = mSpiderStatus.mFlags;
	if (flags & 2) {
		return;
	}
	if (flags & 0x10) {
		return;
	}
	if (flags & 0x8000) {
		return;
	}
	XObjChr* player = gGame.GetPlayer();
	if (GetAttachmentCountById(&player->mAttachments, 2) < 2) {
		return;
	}
	OnHitFromPosition__4XObjFRC3Vecf(this, &pos, strength);
	mSpiderStatus.mFlags |= 2;
	Vec hit             = { pos.x, pos.y, pos.z };
	mHitPos             = hit;
	mHitStrength        = strength;
	CParticle* particle = &gGame.mParticle;
	particle->Kill(0xB3);
	// NONMATCHING: retail builds rot straight in CreateSRT's by-value argument slot, this compiler
	// builds the local and copies it (same in states34, mamahide, scrc and evt); the literal pool is
	// also another unit's in retail
	Vec scale = { 1.0f, 1.0f, 1.0f };
	Vec rot   = { 0.0f, mTargetRotY, 0.0f };
	particle->CreateSRT(0xB3, 1, pos, scale, rot, TRUE);
	XSound* sound = &gGame.mSound;
	if (!IsGameSoundPlaying(sound, 0x127)) {
		PlayGameSound(sound, 0x127, 255); // npc_spider_bakuhatsu
	}
}
