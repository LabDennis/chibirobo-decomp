#include "game/ringmenu.h"
#include "game/cscrn.h"

class CGame;
extern "C" CGame gGame;

struct AttachmentChange {
	s32 equipped;
	s32 requested;
	s32 step;
};

void AtcMenu::ConfirmSelection()
{
	// The Wii player stores its attachment change at 0x9920.
	AttachmentChange& attachment = *reinterpret_cast<AttachmentChange*>(reinterpret_cast<u8*>(mChr) + 0x9920);
	s32 current                  = attachment.requested >= 0 ? attachment.requested : attachment.equipped;
	s32 type                     = mItems[mCursorIndex].mType;
	if (type == current) {
		if (attachment.equipped != 0) {
			attachment.requested = 0;
			attachment.step      = 0;
		}
		return;
	}
	if (type != attachment.equipped) {
		attachment.requested = type;
		attachment.step      = 0;
	}
	if (type == 2) {
		// CGame::mRoboeyes in the Wii layout.
		CScrnRoboeyes& eyes = *reinterpret_cast<CScrnRoboeyes*>(reinterpret_cast<u8*>(&gGame) + 0x5078);
		eyes.ResetLensState(430.0f, 400.0f);
	}
}
