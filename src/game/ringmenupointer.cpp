#include "game/ringmenu.h"
#include <math.h>

class CGame;
class XSound;
struct WiiPointer;
extern "C" {
extern CGame gGame;
extern WiiPointer gWiiPointer;
extern s32 gRingScreenWidth;
extern s32 gRingScreenHeight;
s32 PlayWiiSoundEffect(XSound* sound, s32 id, f32 volume, f32 pan);
}

BOOL OctaMenu::UpdatePointedItem(BOOL enabled)
{
	if (mMenuType != 3)
		return FALSE;
	mPreviousPointedItem = mPointedItem;
	const u8* pointer    = reinterpret_cast<const u8*>(&gWiiPointer);
	const u8* game       = reinterpret_cast<const u8*>(&gGame);
	XSound* sound        = reinterpret_cast<XSound*>(reinterpret_cast<u8*>(&gGame) + 0x22A4);
	if (enabled) {
		if (*reinterpret_cast<const BOOL*>(pointer + 0xE4)) {
			const f32* position = reinterpret_cast<const f32*>(pointer + 0xD0);
			for (s32 i = 0; i < 8; ++i) {
				const OctaMenuItem& item = mItems[i];
				const f32 x              = 68.0f * (item.mSlotOffset * 1.1f) + gRingScreenWidth / 2;
				const f32 y              = 68.0f * ((mRowOffset + item.mRowOffset) * 1.1f) + (gRingScreenHeight / 2 - 12);
				const f32 scale          = item.mScale * (f32)sin(1.5707964f * mFade);
				const f32 halfSize       = 150.0f * (1.2f * scale) * 0.5f;
				if (position[0] >= x - halfSize && position[0] <= x + halfSize && position[1] >= y - halfSize
				    && position[1] <= y + halfSize) {
					s32 relative = i - mFirstVisibleItem;
					if (relative < 0)
						relative += 7;
					if (mLastPointedItem != relative) {
						mLastPointedItem = relative;
						PlayWiiSoundEffect(sound, 244, 1.0f, 0.0f);
					}
					mPointedItem = relative;
					return TRUE;
				}
			}
		} else if ((*reinterpret_cast<const u32*>(game + 0xEE1DC) & 3) == 3 && *reinterpret_cast<const BOOL*>(pointer + 0xEC)) {
			PlayWiiSoundEffect(sound, 244, 1.0f, 0.0f);
		}
	}
	mPointedItem = -1;
	return FALSE;
}
