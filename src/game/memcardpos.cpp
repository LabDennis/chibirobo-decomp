#include <dolphin/mtx/vec.h>
#include "game/savedata.h"
#include "game/cgame.h"
#include "game/xchr.h"
#include "game/cmotioncontrolcamera.h"
#include "game/xobjchr.h"

extern "C" void fn_8014127C(CInterfaceCameraAction* action);

/**
 * @note Address: 0x80087A4C
 * @note Size: 0x23C
 */
extern "C" void RestoreSavedPlayerAndCamera(Memcard* card)
{
	SaveData* save = &card->mFile.mData;
	XObj* player   = gGame.GetPlayer();
	player->SetPos(save->mPos, 0);
	gGame.GetPlayer()->SetRotationY(save->mRotY, 0);
	ResetPlayerHeadOffset(gGame.GetPlayer());
	ResetPlayerPlug(gGame.GetPlayer());
	if (save->mHeader.mVersion <= 0x10100 || (save->mHeader.mVersion & 2)) {
		CCamera* camera              = &gGame.mMotionControlCamera.mCamera;
		CMotionControlCamera* motion = &gGame.mMotionControlCamera;
		if (save->mCameraAction >= 0 && save->mCameraAction < CAMACTION_NUM) {
			motion->ChangeAction(save->mCameraAction);
			camera->SetEye(save->mCameraEye);
			camera->SetInterest(save->mCameraInterest);
			camera->SetAngle(save->mCameraAngle);
			camera->SetDistance(save->mCameraDistance);
			camera->SetFov(save->mCameraFov);
			CCameraMemento memento;
			memento.SetEye(save->mCameraEye);
			memento.SetInterest(save->mCameraInterest);
			memento.SetAngle(save->mCameraAngle);
			memento.mDistance = save->mCameraDistance;
			memento.mFov      = save->mCameraFov;
			memento.mAction   = gGame.mCameraActions.mTable[save->mCameraAction];
			fn_8014127C(gGame.mCameraActions.mTable[CAMACTION_MAP]);
			motion->SetMemento(&memento);
		}
	}
}
