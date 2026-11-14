#include "game/ringmenu.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include <math.h>

extern "C" {
extern s32 gRingScreenWidth;
extern s32 gRingScreenHeight;
}

// NONMATCHING: identical instruction multiset; residue is register allocation (this/item/i/relative
// land on a rotated callee-save set) and mid-block scheduling of the per-item math.
BOOL OctaMenu::UpdatePointedItem(BOOL enabled)
{
	if (mMenuType == 3) {
		mPreviousPointedItem = mPointedItem;
		if (enabled) {
			if (gWiiPointer.mPointerValid) {
				for (s32 i = 0; i < 8; ++i) {
					const f32 x  = 68.0f * (mItems[i].mSlotOffset * 1.1f) + gRingScreenWidth / 2;
					const f32 y  = 68.0f * ((mRowOffset + mItems[i].mRowOffset) * 1.1f) + (gRingScreenHeight / 2 - 12);
					s32 relative = i - mFirstVisibleItem;
					if (relative < 0)
						relative += 7;
					const f32 scale    = mItems[i].mScale * (f32)sin(1.5707964f * mFade);
					const f32 halfSize = 150.0f * (1.2f * scale);
					if (gWiiPointer.mPointer.x >= x - halfSize * 0.5f && gWiiPointer.mPointer.x <= x + halfSize * 0.5f
					    && gWiiPointer.mPointer.y >= y - halfSize * 0.5f && gWiiPointer.mPointer.y <= y + halfSize * 0.5f) {
						if (mLastPointedItem != relative) {
							mLastPointedItem = relative;
							PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
						}
						mPointedItem = relative;
						return TRUE;
					}
				}
			} else if ((gGame.mMenuControls & 2) && (gGame.mMenuControls & 1) && gWiiPointer.mPointerChanged) {
				PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
			}
		}
		mPointedItem = -1;
	}
	return FALSE;
}
