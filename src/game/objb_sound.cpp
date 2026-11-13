#include "game/xobjsound.h"
#include "game/cgame.h"

// XObjSound, an object that plays sounds along with its anim

extern "C" AnimSfxEvent gSoundObjectAnimEvents[3];

extern "C" {

// the weak destructor
/**
 * @note Address: 0x800DD32C
 * @note Size: 0x60
 */
XObjSound* __dt__9XObjSoundFv(XObjSound* object, s32 flags)
{
	if (object != NULL) {
		object->XObjSound::~XObjSound();
		if ((s16)flags > 0) {
			::operator delete(object);
		}
	}
	return object;
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
 * @note Address: 0x800DD43C
 * @note Size: 0xC8
 */
void XObjSound::UpdateObject(u32 frames)
{
	UpdateObject__4XObjFUl(this, frames);
	HSD_AObj* aobj = GetAObj(mJObj);
	UpdateSfx(aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f);
}

/**
 * @note Address: 0x800DD504
 * @note Size: 0x68
 */
void XObjSound::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
	_990 = 0;
	_994 = 0;
	switch (GetKind()) {
	case 0x1D2:
		mSfx.mEvents = gSoundObjectAnimEvents;
		break;
	}
}

/**
 * @note Address: 0x800DD56C
 * @note Size: 0x34
 */
void XObjSound::InitializeObject(u32 id)
{
	InitializeObject__4XObjFUl(this, id);
	mSfx.mEvents = NULL;
}

} // extern "C"
