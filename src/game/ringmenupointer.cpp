#include "game/ringmenu.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include <math.h>

extern "C" {
extern s32 gRingScreenWidth;
extern s32 gRingScreenHeight;
}

BOOL OctaMenu::UpdatePointedItem(BOOL enabled)
{
	if (mMenuType != 3)
		return FALSE;
	mPreviousPointedItem = mPointedItem;
	if (enabled) {
		if (gWiiPointer.mPointerValid) {
			for (s32 i = 0; i < 8; ++i) {
				const OctaMenuItem& item = mItems[i];
				const f32 x              = 68.0f * (item.mSlotOffset * 1.1f) + gRingScreenWidth / 2;
				const f32 y              = 68.0f * ((mRowOffset + item.mRowOffset) * 1.1f) + (gRingScreenHeight / 2 - 12);
				const f32 scale          = item.mScale * (f32)sin(1.5707964f * mFade);
				const f32 halfSize       = 150.0f * (1.2f * scale) * 0.5f;
				if (gWiiPointer.mPointer.x >= x - halfSize && gWiiPointer.mPointer.x <= x + halfSize
				    && gWiiPointer.mPointer.y >= y - halfSize && gWiiPointer.mPointer.y <= y + halfSize) {
					s32 relative = i - mFirstVisibleItem;
					if (relative < 0)
						relative += 7;
					if (mLastPointedItem != relative) {
						mLastPointedItem = relative;
						PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
					}
					mPointedItem = relative;
					return TRUE;
				}
			}
		} else if ((gGame.mMenuControls & 3) == 3 && gWiiPointer.mPointerChanged) {
			PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
		}
	}
	mPointedItem = -1;
	return FALSE;
}
