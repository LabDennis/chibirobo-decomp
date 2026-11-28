#include "game/ringmenu.h"
#include "game/wii/cgame.h"

// the GameCube's AtcMenu::ConfirmSelection (obja_chr.cpp) plus the eye lens reset when the
// eye attachment is picked.
// The function matches, but the object can't be linked yet: the included headers emit the OS
// fast-cast statics, __FUNCTION__ strings and two .sdata2 constants of their own into it.
void AtcMenu::ConfirmSelection()
{
	WiiPlayerState* player = reinterpret_cast<WiiPlayerState*>(mChr);
	s32 current            = player->GetAttachment();
	s32 type               = mItems[mCursorIndex].mType;
	if (type == current) {
		if (player->mAttachment != 0) {
			player->mAttachmentRequest = 0;
			player->mAttachmentStep    = 0;
		}
	} else {
		if (type != player->mAttachment) {
			player->mAttachmentRequest = type;
			player->mAttachmentStep    = 0;
		}
		if (mItems[mCursorIndex].mType == 2) {
			gGame.mRoboeyes.ResetLensState(430.0f, 400.0f);
		}
	}
}
