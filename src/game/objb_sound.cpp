#include "game/xobjsound.h"
#include "game/cgame.h"
#include "game/animframe.h"

// XObjSound, an object that plays sounds along with its anim

extern "C" {
AnimSfxEvent gSoundObjectAnimEvents[] = {
	{ 2, 18.0f, 35.0f, 1, 0x11000e, 255, 4, 0, 0 },
	{ 3, 14.0f, 27.0f, 1, 0x11000e, 255, 4, 0, 0 },
	{ -1 },
};
} // extern "C"

/**
 * @note Address: 0x800DD56C
 * @note Size: 0x34
 */
void XObjSound::InitializeObject(u32 id)
{
	XObj::InitializeObject(id);
	mSfx.mEvents = NULL;
}

/**
 * @note Address: 0x800DD504
 * @note Size: 0x68
 */
void XObjSound::ResetObjectState()
{
	XObj::ResetObjectState();
	_990 = 0;
	_994 = 0;
	switch (GetKind()) {
	case 0x1D2:
		mSfx.mEvents = gSoundObjectAnimEvents;
		break;
	}
}

/**
 * @note Address: 0x800DD43C
 * @note Size: 0xC8
 */
void XObjSound::UpdateObject(u32 frames)
{
	XObj::UpdateObject(frames);
	UpdateSfx(GetAnimFrame(mJObj));
}

/**
 * @note Address: 0x800DD38C
 * @note Size: 0xB0
 */
void XObjSound::UpdateSfx(f32 frame)
{
	if ((mStatus.mFlags & 1) && (mStatus.mFlags & 0x10000)) {
		mSfx.Update(mAnimId, frame, GetSoundVolumeAtPosition(&gGame.mSound, &mMoveTranslate, 0));
	}
}

/**
 * @note Address: 0x800DD32C
 * @note Size: 0x60
 */
XObjSound::~XObjSound()
{
}
