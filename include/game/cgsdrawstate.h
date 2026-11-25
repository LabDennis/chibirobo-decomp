#ifndef GAME_CGSDRAWSTATE_H
#define GAME_CGSDRAWSTATE_H

#include "game/cgs.h"

// sends whatever render, tev and channel state changed since the last draw
static inline void SetupDrawState(CGs* gs)
{
	if (gs->mCurRenderMode != NULL) {
		gs->mCurRenderMode->Setup(FALSE);
	}
	if (gs->mCurTevStage != NULL) {
		gs->mCurTevStage->Setup(FALSE);
	}
	if (gs->mCurChannel != NULL) {
		gs->mCurChannel->Setup(FALSE);
	}
}

#endif
