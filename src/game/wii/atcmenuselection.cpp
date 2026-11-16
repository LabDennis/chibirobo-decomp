#include "game/ringmenu.h"
#include "game/wii/cgame.h"

// NONMATCHING (Wii): same instruction multiset; retail allocates `player` to r6
// and `mType` to r7 (we get r7/r6 plus the flipped cmpw operand order).
void AtcMenu::ConfirmSelection()
{
	WiiPlayerState* player = reinterpret_cast<WiiPlayerState*>(mChr);
	s32 current            = player->mAttachmentRequest >= 0 ? player->mAttachmentRequest : player->mAttachment;
	if (mItems[mCursorIndex].mType == current) {
		if (player->mAttachment != 0) {
			player->mAttachmentRequest = 0;
			player->mAttachmentStep    = 0;
		}
		return;
	}
	if (mItems[mCursorIndex].mType != player->mAttachment) {
		player->mAttachmentRequest = mItems[mCursorIndex].mType;
		player->mAttachmentStep    = 0;
	}
	if (mItems[mCursorIndex].mType == 2) {
		gGame.mRoboeyes.ResetLensState(430.0f, 400.0f);
	}
}
