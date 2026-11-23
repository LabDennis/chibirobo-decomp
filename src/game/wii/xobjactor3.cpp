#include "game/wii/cgame.h"
#include "game/wii/infoscrn.h"
#include "game/wii/xobj.h"
#include "arithmetic.h"
#include <stdlib.h>

extern "C" BOOL IsPlayerFlashingFromHit(WiiPlayerState* player);

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
