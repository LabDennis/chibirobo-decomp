#include "game/cscrn.h"

void CScrnRoboeyes::ResetLensState(f32 width, f32 height)
{
	mSeparation.value = mSeparation.target = 150.0f;
	mLeftWidth.value = mLeftWidth.target = width;
	mLeftHeight.value = mLeftHeight.target = height;
	mRightWidth.value = mRightWidth.target = width;
	mRightHeight.value = mRightHeight.target = height;
	mFocus.value = mFocus.target = 0.0f;
	mLensState                   = 0;
}
