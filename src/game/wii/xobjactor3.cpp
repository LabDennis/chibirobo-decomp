#include "game/wii/cgame.h"
#include "game/wii/infoscrn.h"
#include "game/wii/xobj.h"
#include "arithmetic.h"
#include <stdlib.h>

extern "C" BOOL IsPlayerFlashingFromHit(WiiPlayerState* player);

static inline bool IsPlayerAway(WiiPlayerState* player)
{
	Vec pos = *player->GetPos();
	if (pos.z < -4.28f) {
		return true;
	}
	switch (gGame.GetPlayer()->GetAnimId()) {
	case 0x66:
	case 0x67:
	case 0x68:
	case 0x69:
	case 0x6A:
	case 0x6B:
	case 0x6C:
	case 0x6D:
	case 0x6E:
	case 0x6F:
	case 0x70:
	case 0x71:
	case 0x72:
	case 0x73:
		return true;
	}
	return false;
}

static inline bool IsLongReachAnim(s32 anim)
{
	switch (anim) {
	case 0x4:
	case 0xD:
	case 0x35:
	case 0x79:
	case 0x89:
	case 0x10D:
	case 0x819:
	case 0x898:
		return true;
	}
	return false;
}

// NONMATCHING: the distance expression keeps a second Vec temporary in retail;
// calls, constants and state transitions are otherwise reconstructed.
void XObjActor3::Kind10(u32 frames)
{
	WiiPlayerState* player = gGame.GetPlayer();
	Vec pos                = *player->GetPos();
	switch (mKindStep) {
	case 0:
		if (mActorFlags.mFlags & 4) {
			break;
		}
		mMoveTranslate.x = pos.x - (rand() % 3600) / 100.0f + 18.0f;
		mMoveTranslate.y = 64.0f;
		mMoveTranslate.z = pos.z - (rand() % 3600) / 100.0f + 18.0f;
		mStatus.mFlags |= 0x10000;
		mStatus.mFlags |= 4;
		_BA0        = 4.0f;
		mFrameCount = rand() % 30 + 30;
		mKindStep++;
		break;
	case 1:
		if (--mFrameCount < 0) {
			mKindStep++;
		}
		break;
	case 2:
		f32 dx   = pos.x - mMoveTranslate.x;
		f32 dy   = pos.y - mMoveTranslate.y;
		f32 dz   = pos.z - mMoveTranslate.z;
		f32 dist = FSqrt(dx * dx + dy * dy + dz * dz);
		if (dist < 8.0f && !IsPlayerFlashingFromHit(player) && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
			player->OnHitFromPosition(pos, 10.0f);
			mFrameCount = 30;
			mKindStep   = 3;
			break;
		}
		if (_BA0 >= -2.3f) {
			_BA0 -= 0.36f * frames;
		} else {
			_BA0 = -2.3f;
		}
		mMoveTranslate.y += _BA0;
		if (mMoveTranslate.y < 0.0f) {
			mMoveTranslate.y = 0.0f;
			mFrameCount      = 30;
			mKindStep        = 3;
		}
		break;
	case 3:
		if ((mFrameCount & 3) < 2) {
			mStatus.mFlags |= 0x10000;
		} else {
			mStatus.mFlags &= ~0x10000ULL;
		}
		if (--mFrameCount < 0) {
			mStatus.mFlags &= ~0x10000ULL;
			mStatus.mFlags &= ~4ULL;
			mActorFlags.mFlags |= 4;
			mKindStep = 0;
		}
		break;
	}
}

extern "C" bool IsPlayerWithinActorReach(XObjActor3* actor)
{
	WiiPlayerState* player = gGame.GetPlayer();
	Vec pos                = *player->GetPos();
	if (!(actor->mActorFlags.mFlags & 4)) {
		if (IsPlayerAway(gGame.GetPlayer())) {
			return false;
		}
		if (pos.y >= actor->mMoveTranslate.y - 16.0f && pos.y <= actor->mMoveTranslate.y + 16.0f) {
			if (actor->mHeading == HALF_PI) {
				if (actor->mMoveTranslate.x < pos.x && pos.x < actor->mWindowRight) {
					return true;
				}
				if (actor->mMoveTranslate.x > pos.x && pos.x > actor->mWindowLeft) {
					if (IsLongReachAnim(gGame.GetPlayer()->GetAnimId())) {
						if (actor->mMoveTranslate.x > pos.x && pos.x > actor->mMoveTranslate.x - 32.0f) {
							return true;
						}
					} else {
						if (actor->mMoveTranslate.x > pos.x && pos.x > actor->mMoveTranslate.x - 12.0f) {
							return true;
						}
					}
				}
			} else {
				if (actor->mMoveTranslate.x > pos.x && pos.x > actor->mWindowLeft) {
					return true;
				}
				if (actor->mMoveTranslate.x < pos.x && pos.x < actor->mWindowRight) {
					if (IsLongReachAnim(gGame.GetPlayer()->GetAnimId())) {
						if (actor->mMoveTranslate.x < pos.x && pos.x < actor->mMoveTranslate.x + 32.0f) {
							return true;
						}
					} else {
						if (actor->mMoveTranslate.x < pos.x && pos.x < actor->mMoveTranslate.x + 12.0f) {
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}
