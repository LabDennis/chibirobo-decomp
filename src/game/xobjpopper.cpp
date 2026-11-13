#include "game/xobjattachment.h"
#include "game/cgame.h"

// XObjPopper and XObjCopter, and the XObjAttachment base they share

extern "C" {

/**
 * @note Address: 0x800D6FE8
 * @note Size: 0x398
 */
void XObjPopper::UpdateObject(u32 frames)
{
	if (mVisible) {
		CopyParentMtx(this);
		HSD_AObj* aobj = GetAObj(mJObj);
		s32 anim       = mAnimId;
		f32 frame      = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
		f32 end        = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
		switch (anim) {
		case 1:
			if (aobj->flags & 0x44000000) {
				SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
			break;
		case 2:
			if (aobj->flags & 0x44000000) {
				SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				Hide();
			}
			break;
		case 3:
			if (aobj->flags & 0x44000000) {
				SetAnim(4, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
			break;
		case 4:
			break;
		case 5:
			if (aobj->flags & 0x44000000) {
				SetAnim(4, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
			break;
		case 6:
			if (aobj->flags & 0x44000000) {
				SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
			break;
		}
		UpdateObject__4XObjFUl(this, frames);
	}
}

/**
 * @note Address: 0x800D7380
 * @note Size: 0xCC
 */
void ShowIdlePlayerPopper(XObjPopper* object)
{
	object->SetAnim(2, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	object->UpdateAnim(0);
	object->SetAnim(4, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	if (!object->mVisible) {
		object->Show();
	}
}

/**
 * @note Address: 0x800D744C
 * @note Size: 0x50
 */
void PlayPopperLowerAnimation(XObjPopper* object)
{
	object->SetAnim(6, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800D749C
 * @note Size: 0x14
 */
BOOL IsPopperFireAnimationSelected(XObjPopper* object)
{
	return object->mAnimId == 5;
}

/**
 * @note Address: 0x800D74B0
 * @note Size: 0x50
 */
void PlayPopperFireAnimation(XObjPopper* object)
{
	object->SetAnim(5, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800D7500
 * @note Size: 0x50
 */
void PlayPopperRaiseAnimation(XObjPopper* object)
{
	object->SetAnim(3, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800D7550
 * @note Size: 0xA8
 */
void PlayPopperStowAnimation(XObjPopper* object)
{
	object->SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	gGame.mSound.StopIfPlaying(0x186);
	gGame.mSound.StopIfPlaying(0x18B);
}

/**
 * @note Address: 0x800D75F8
 * @note Size: 0x70
 */
void ShowAndDeployPopper(XObjPopper* object)
{
	object->Show();
	object->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800D7668
 * @note Size: 0x344
 */
void XObjCopter::UpdateObject(u32 frames)
{
	if (mVisible) {
		CopyParentMtx(this);
		HSD_AObj* aobj = GetAObj(mJObj);
		s32 anim       = mAnimId;
		f32 frame      = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
		f32 end        = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
		switch (anim) {
		case 5:
			if (aobj->flags & 0x44000000) {
				SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
			break;
		case 8:
			if (aobj->flags & 0x44000000) {
				SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				Hide();
			}
			break;
		case 6:
			if (aobj->flags & 0x44000000) {
				SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
			break;
		case 7:
			if (aobj->flags & 0x44000000) {
				SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			}
			break;
		}
		UpdateObject__4XObjFUl(this, frames);
	}
}

/**
 * @note Address: 0x800D79AC
 * @note Size: 0xCC
 */
void ShowIdlePlayerCopter(XObjCopter* object)
{
	object->SetAnim(8, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	object->UpdateAnim(0);
	object->SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	if (!object->mVisible) {
		object->Show();
	}
}

/**
 * @note Address: 0x800D7A78
 * @note Size: 0x50
 */
void PlayCopterLandingAnimation(XObjCopter* object)
{
	object->SetAnim(7, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800D7AC8
 * @note Size: 0x50
 */
void PlayCopterDescentAnimation(XObjCopter* object)
{
	object->SetAnim(3, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800D7B18
 * @note Size: 0x50
 */
void PlayCopterFlightAnimation(XObjCopter* object)
{
	object->SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800D7B68
 * @note Size: 0x88
 */
void PlayCopterStowAnimation(XObjCopter* object)
{
	object->SetAnim(8, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	gGame.mSound.StopIfPlaying(8);
}

/**
 * @note Address: 0x800D7BF0
 * @note Size: 0x70
 */
void ShowAndDeployCopter(XObjCopter* object)
{
	object->Show();
	object->SetAnim(5, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800D7C60
 * @note Size: 0x40
 */
void XObjAttachment::Release()
{
	Hide();
	XObj::Release();
}

/**
 * @note Address: 0x800D7CA0
 * @note Size: 0x20
 */
void XObjAttachment::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
}

/**
 * @note Address: 0x800D7CC0
 * @note Size: 0x8
 */
void SetParent__14XObjAttachmentFP9_HSD_JObji(XObjAttachment* object, HSD_JObj* parent)
{
	object->mParent = parent;
}

/**
 * @note Address: 0x800D7CC8
 * @note Size: 0x34
 */
void XObjAttachment::vf94()
{
	mParent = NULL;
	Hide();
}

/**
 * @note Address: 0x800D7CFC
 * @note Size: 0x40
 */
void XObjAttachment::InitializeObject(u32 id)
{
	InitializeObject__4XObjFUl(this, id);
	vf94();
}

} // extern "C"
