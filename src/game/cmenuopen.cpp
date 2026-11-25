#include "game/stg.h"

/**
 * @note Address: 0x80010CD0
 * @note Size: 0x48
 */
void CMenu::Open()
{
	mState = 1;
	Resume();
	mWindow.mOpen = TRUE;
}

/**
 * @note Address: 0x80010D18
 * @note Size: 0x10
 */
void CMenu::Resume()
{
	mSuspended   = FALSE;
	mSuspendFade = FALSE;
}
