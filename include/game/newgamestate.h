#ifndef _GAME_NEWGAMESTATE_H
#define _GAME_NEWGAMESTATE_H

#include "game/cgame.h"
#include "game/xchr.h"
#include "game/gmath.h"
#include <stdlib.h>

extern "C" void ResetPooledObjects(XObjPool*);
extern "C" void InitBatteryGauge(InfoScrn*, f32);

inline void CGame::ResetNewGameState()
{
	for (int i = 0; i < 128; ++i)
		gEventFlags.mFlags[i] = 0;
	for (int i = 0; i < 2048; ++i)
		gEventVars.mVars[i] = 0;

	mMemcardLoaded       = FALSE;
	mTimeState.mMode     = 0;
	mTimeState.mTime     = 0;
	mTimeState.mDuration = 1;
	mTimeState.mRunning  = FALSE;
	mPlayTime            = 0;
	_FF204               = 0;
	ResetPooledObjects(mXObjs.mPools);
	InitBatteryGauge(&mInfoScrn, GetPlayer()->mParams.mBattery);

	mScrnTex.mAngle = 0;
	StartScreenFade(&mScrnTex.mColor, 0, 0, 0, 0, 100, 100, 200, 0, 0.0f);
	StartScreenFade(&mColor0.mColor, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f);
	StartScreenFade(&mColor1.mColor, 0, 0, 0, 0, 255, 255, 255, 0, 0.0f);
	_4724              = 0;
	_4728              = 0.5f;
	mStageName.mActive = FALSE;
	for (int i = 0; i < 20; ++i) {
		mStageName.mChars[i].mScale = 0;
		mStageName.mChars[i].mWait  = 0;
		mStageName.mChars[i].mBob   = DEG2RAD * (rand() % 360);
		mStageName.mChars[i].mTilt  = DEG2RAD * (rand() % 360);
	}
}

#endif
