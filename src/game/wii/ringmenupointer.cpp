#include "game/ringmenu.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include <math.h>

extern "C" {
extern s32 gRingScreenWidth;
extern s32 gRingScreenHeight;
}

// The item position follows the GameCube ring drawing code (originX + 68 * (scale * slotX)).
// The code matches; only the float literals differ, since retail pools them in the sdata2 of the
// translation unit this function was part of.
BOOL OctaMenu::UpdatePointedItem(BOOL enabled)
{
	if (mMenuType == 3) {
		mPreviousPointedItem = mPointedItem;
		if (enabled) {
			CPadAct* pad = &gWiiPointer;
			if (pad->mPointerValid) {
				for (s32 i = 0; i < 8; ++i) {
					f32 originX = gRingScreenWidth / 2;
					f32 originY = gRingScreenHeight / 2 - 12;
					f32 slotX   = mItems[i].mSlotOffset;
					f32 slotY   = mRowOffset + mItems[i].mRowOffset;
					f32 scale   = 1.1f;
					f32 x, y;
					y            = originY + 68.0f * (slotY * scale);
					x            = originX + 68.0f * (slotX * scale);
					s32 relative = i - mFirstVisibleItem;
					if (relative < 0)
						relative += 7;
					f32 pulse = mItems[i].mScale * (f32)sin(1.5707964f * mFade);
					f32 size  = 150.0f * (1.2f * pulse);
					if (pad->mPointer.x >= x - size / 2 && pad->mPointer.x <= x + size / 2 && pad->mPointer.y >= y - size / 2
					    && pad->mPointer.y <= y + size / 2) {
						if (relative != mLastPointedItem) {
							mLastPointedItem = relative;
							PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
						}
						mPointedItem = relative;
						return TRUE;
					}
				}
			} else if ((gGame.mMenuControls & 2) && (gGame.mMenuControls & 1) && pad->mPointerChanged) {
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
			}
		}
		mPointedItem = -1;
	}
	return FALSE;
}
