// chibi robo's attachment objects on the Wii: the shared XObjAttachment base and the
// copter, popper, radar and OX mark objects that ride on the player, retail
// 0x8010E548-0x8010F944. This was its own translation unit in retail: the player's
// action code calls the animation helpers here with plain bl (UpdatePlayerPopperAction
// reaches ShowIdlePlayerPopper 4 times and the 10-instruction popper anims without
// inlining), so they must sit behind a TU boundary or -ipa file folds them into the
// callers.
#include "game/xobjattachment.h"
#include "game/wii/cgame.h"
#include "game/wii/csound.h"
#include "game/globals.h"
#include "game/gmath.h"

extern "C" void HSD_MtxGetScale(Mtx m, Vec* dst);

extern "C" void fn_8010E548(XObjAttachment*)
{
}

/**
 * @note Address: 0x8010E54C
 * @note Size: 0x40
 */
void XObjAttachment::InitializeObject()
{
	XObj::InitializeObject();
	ResetForNewGame();
}

/**
 * @note Address: 0x8010E58C
 * @note Size: 0x18
 */
void XObjAttachment::ResetForNewGame()
{
	mParent = NULL;
	Hide();
}

/**
 * @note Address: 0x8010E5A4
 * @note Size: 0x8
 */
void XObjAttachment::SetParent(MtxP parent, BOOL)
{
	mParent = parent;
}

/**
 * @note Address: 0x8010E5AC
 * @note Size: 0x4
 */
void XObjAttachment::ResetObjectState()
{
	XObj::ResetObjectState();
}

/**
 * @note Address: 0x8010E5B0
 * @note Size: 0x40
 */
void XObjAttachment::Release()
{
	Hide();
	XObj::Release();
}

// the GameCube's ExecCallbackB: while it is attached the parent matrix is the world matrix and the
// move translate and scale are read back out of it
/**
 * @note Address: 0x8010E5F0
 * @note Size: 0xB0
 */
void XObjAttachment::ExecCallbackB(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
	XObj::ExecCallbackB(pResult, mdl, pFuncObj);
	if (pFuncObj->GetCallbackNodeID() == 0 && mVisible && mParent != NULL) {
		if (mParent != NULL) {
			PSMTXCopy(mParent, pResult->mpMtx);
		} else {
			PSMTXIdentity(pResult->mpMtx);
		}
		mMoveTranslate.x = mParent[0][3];
		mMoveTranslate.y = mParent[1][3];
		mMoveTranslate.z = mParent[2][3];
		HSD_MtxGetScale(mParent, &mMoveScale);
	}
}

// the GameCube's ShowAndDeployCopter
/**
 * @note Address: 0x8010E6A0
 * @note Size: 0x64
 */
extern "C" void ShowAndDeployCopter(XObjCopter* copter)
{
	copter->Show();
	copter->SetAnim(5, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
}

/**
 * @note Address: 0x8010E704
 * @note Size: 0xC
 */
void XObjAttachment::Show()
{
	mVisible = TRUE;
}

// the GameCube's PlayCopterStowAnimation
/**
 * @note Address: 0x8010E710
 * @note Size: 0x78
 */
extern "C" void PlayCopterStowAnimation(XObjCopter* copter)
{
	copter->SetAnim(8, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	if (IsWiiSfxPlaying(&gGame.mSound, 0x75)) {
		StopWiiSfx(&gGame.mSound, 0x75, 0.0f);
	}
}

// the GameCube's PlayCopterFlightAnimation
/**
 * @note Address: 0x8010E788
 * @note Size: 0x28
 */
extern "C" void PlayCopterFlightAnimation(XObjCopter* copter)
{
	copter->SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
}

// the GameCube's PlayCopterDescentAnimation
/**
 * @note Address: 0x8010E7B0
 * @note Size: 0x28
 */
extern "C" void PlayCopterDescentAnimation(XObjCopter* copter)
{
	copter->SetAnim(3, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
}

// the GameCube's PlayCopterLandingAnimation
/**
 * @note Address: 0x8010E7D8
 * @note Size: 0x28
 */
extern "C" void PlayCopterLandingAnimation(XObjCopter* copter)
{
	copter->SetAnim(7, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
}

// the GameCube's ShowIdlePlayerCopter
/**
 * @note Address: 0x8010E800
 * @note Size: 0xB4
 */
extern "C" void ShowIdlePlayerCopter(XObjCopter* copter)
{
	copter->SetAnim(8, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
	copter->UpdateAnim(0.0f);
	copter->SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	if (!copter->mVisible)
		copter->Show();
}

// the copter's own state machine on its animations: 5 deploys, 8 folds the blades
// away, 6 and 7 are the landing helpers, the rest just keep the base update
/**
 * @note Address: 0x8010E8B4
 * @note Size: 0x1C4
 */
void XObjCopter::UpdateObject(u32 frames)
{
	if (!mVisible)
		return;
	switch (GetAnimId()) {
	case 5:
		if (IsAnimDone()) {
			SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		break;
	case 8:
		if (IsAnimDone()) {
			SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			Hide();
		}
		break;
	case 6:
		if (IsAnimDone()) {
			SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		break;
	case 7:
		if (IsAnimDone()) {
			SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		break;
	}
	XObj::UpdateObject(frames);
}

// the GameCube's ShowAndDeployPopper
/**
 * @note Address: 0x8010EA78
 * @note Size: 0x64
 */
extern "C" void ShowAndDeployPopper(XObjPopper* popper)
{
	popper->Show();
	popper->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
}

// the GameCube's PlayPopperStowAnimation
/**
 * @note Address: 0x8010EADC
 * @note Size: 0xA4
 */
extern "C" void PlayPopperStowAnimation(XObjPopper* popper)
{
	popper->SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	if (IsWiiSfxPlaying(&gGame.mSound, 0x1F3)) {
		StopWiiSfx(&gGame.mSound, 0x1F3, 0.0f);
	}
	if (IsWiiSfxPlaying(&gGame.mSound, 0x1F8)) {
		StopWiiSfx(&gGame.mSound, 0x1F8, 0.0f);
	}
}

// the GameCube's PlayPopperRaiseAnimation
/**
 * @note Address: 0x8010EB80
 * @note Size: 0x28
 */
extern "C" void PlayPopperRaiseAnimation(XObjPopper* popper)
{
	popper->SetAnim(3, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
}

// the GameCube's PlayPopperFireAnimation
/**
 * @note Address: 0x8010EBA8
 * @note Size: 0x28
 */
extern "C" void PlayPopperFireAnimation(XObjPopper* popper)
{
	popper->SetAnim(5, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
}

// the GameCube's IsPopperFireAnimationSelected
/**
 * @note Address: 0x8010EBD0
 * @note Size: 0x38
 */
extern "C" BOOL IsPopperFireAnimationSelected(XObjPopper* popper)
{
	return popper->GetAnimId() == 5;
}

// the GameCube's PlayPopperLowerAnimation
/**
 * @note Address: 0x8010EC08
 * @note Size: 0x28
 */
extern "C" void PlayPopperLowerAnimation(XObjPopper* popper)
{
	popper->SetAnim(6, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
}

// the GameCube's ShowIdlePlayerPopper
/**
 * @note Address: 0x8010EC30
 * @note Size: 0xB4
 */
extern "C" void ShowIdlePlayerPopper(XObjPopper* popper)
{
	popper->SetAnim(2, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
	popper->UpdateAnim(0.0f);
	popper->SetAnim(4, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	if (!popper->mVisible)
		popper->Show();
}

// the popper's own state machine: 1 deploys, 2 folds away, 3 and 5 raise and fire
/**
 * @note Address: 0x8010ECE4
 * @note Size: 0x1D4
 */
// NONMATCHING: the anim id reads and the hide ordering in the retail switch
void XObjPopper::UpdateObject(u32 frames)
{
	if (!mVisible)
		return;
	switch (GetAnimId()) {
	case 1:
		if (IsAnimDone()) {
			SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		break;
	case 2:
		if (IsAnimDone()) {
			SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			Hide();
		}
		break;
	case 3:
		if (IsAnimDone()) {
			SetAnim(4, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		break;
	case 4:
		break;
	case 5:
		if (IsAnimDone()) {
			SetAnim(4, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		break;
	case 6:
		if (IsAnimDone()) {
			SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		break;
	}
	XObj::UpdateObject(frames);
}

// the GameCube's ExecCallbackB for the popper: while it is attached the parent matrix is the
// world matrix; on the Wii the muzzle pitch and yaw ride on the eye aim
/**
 * @note Address: 0x8010EF00
 * @note Size: 0x244
 */
void XObjPopper::ExecCallbackB(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
	u16 node = pFuncObj->GetCallbackNodeID();
	XObj::ExecCallbackB(pResult, mdl, pFuncObj);
	if (node == 0 && mVisible && mParent != NULL) {
		MtxP parent = mParent;
		if (parent != NULL) {
			PSMTXCopy(parent, pResult->mpMtx);
		} else {
			PSMTXIdentity(pResult->mpMtx);
		}
		mMoveTranslate.x = parent[0][3];
		mMoveTranslate.y = parent[1][3];
		mMoveTranslate.z = parent[2][3];
		HSD_MtxGetScale(parent, &mMoveScale);
	}
	if (!mVisible)
		return;
	if (node == 0) {
		if (gGame.GetStatusFlags(0x100)) {
			WiiPlayerState* player = gGame.GetPlayer();
			if (player->GetAttachment() == 2) {
				pFuncObj->mNodeID = 1;
			}
		}
	} else if (node == 1) {
		// while the eye camera steers (status 0x100) the muzzle joint rides the eye
		// aim: a yaw on the aim's pitch over the body yaw, a roll on its yaw wrapped
		// past pi, all of it pivoted 1.5 out of the joint
		WiiPlayerState* player = gGame.GetPlayer();
		Mtx work, rotY, rotZ, trans;
		PSMTXCopy(pResult->mpMtx, work);
		Vec eye = player->GetEyeAim();
		PSMTXRotRad(rotY, 'y', eye.y - 12.0f * DEG2RAD - player->mMoveRotate.y);
		Vec pitch = player->GetEyeAim();
		f32 roll = pitch.x;
		if (roll >= PI)
			roll -= TWO_PI;
		roll *= 0.75f;
		PSMTXRotRad(rotZ, 'z', -roll);
		PSMTXTrans(trans, 1.5f, 0.0f, 0.0f);
		PSMTXConcat(work, trans, work);
		PSMTXConcat(work, rotY, work);
		PSMTXConcat(work, rotZ, work);
		PSMTXTrans(trans, -1.5f, 0.0f, 0.0f);
		PSMTXConcat(work, trans, work);
		PSMTXCopy(work, pResult->mpMtx);
	}
}

/**
 * @note Address: 0x8010F144
 * @note Size: 0x4C
 */
void XObjRadar::InitializeObject()
{
	XObj::InitializeObject();
	ResetForNewGame();
	mYaw   = 0.0f;
	mPitch = 0.0f;
}

/**
 * @note Address: 0x8010F190
 * @note Size: 0x40
 */
BOOL XObjRadar::LoadFromPlacement(const CoordInfo& info)
{
	BOOL ok = XObj::LoadFromPlacement(info);
	mYaw    = 0.0f;
	mPitch  = 0.0f;
	return ok;
}

// the GameCube's ActivatePlayerRadar
/**
 * @note Address: 0x8010F1D0
 * @note Size: 0x78
 */
extern "C" void ActivatePlayerRadar(XObjRadar* radar)
{
	radar->Show();
	radar->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	radar->mState = 1;
	radar->mYaw   = 0.0f;
	radar->mPitch = 0.0f;
}

// the GameCube's DeactivatePlayerRadar
/**
 * @note Address: 0x8010F248
 * @note Size: 0xE4
 */
extern "C" void DeactivatePlayerRadar(XObjRadar* radar)
{
	radar->SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	radar->mState = 2;
	if (IsWiiSfxPlaying(&gGame.mSound, 0x1F9)) {
		StopWiiSfx(&gGame.mSound, 0x1F9, 0.0f);
	}
	if (IsWiiSfxPlaying(&gGame.mSound, 0x1FA)) {
		StopWiiSfx(&gGame.mSound, 0x1FA, 0.0f);
	}
	if (IsWiiSfxPlaying(&gGame.mSound, 0x1FB)) {
		StopWiiSfx(&gGame.mSound, 0x1FB, 0.0f);
	}
}

// the GameCube's StartPlayerRadarScan: the scan itself is the animation
/**
 * @note Address: 0x8010F32C
 * @note Size: 0x4
 */
extern "C" void StartPlayerRadarScan(XObjRadar* radar)
{
}

// the GameCube's ShowIdlePlayerRadar
/**
 * @note Address: 0x8010F330
 * @note Size: 0xBC
 */
extern "C" void ShowIdlePlayerRadar(XObjRadar* radar)
{
	radar->mState = 10;
	radar->SetAnim(2, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
	radar->UpdateAnim(0.0f);
	radar->SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	if (!radar->mVisible)
		radar->Show();
}

// the radar's own state machine on its animations: 1 deploys, 2 folds away, 0 is idle
/**
 * @note Address: 0x8010F3EC
 * @note Size: 0x124
 */
// NONMATCHING: the anim id reads and the state transitions sit in different registers
void XObjRadar::UpdateObject(u32 frames)
{
	if (!mVisible)
		return;
	switch (mState) {
	case 1: // deployed: settle to idle
		if (IsAnimDone()) {
			SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mState = 10;
		}
		break;
	case 2: // stowed: hide
		if (IsAnimDone()) {
			SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			Hide();
			mState = 0;
		}
		break;
	case 10: // idle
		break;
	}
	XObj::UpdateObject(frames);
}

// the GameCube's ExecCallbackB for the radar: while it is attached the parent matrix is the
// world matrix, and the dish joints get the yaw and pitch on top of it
/**
 * @note Address: 0x8010F510
 * @note Size: 0x274
 */
// NONMATCHING: the node lookups and the matrix premultiplies order differently
void XObjRadar::ExecCallbackB(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
	u16 node = pFuncObj->GetCallbackNodeID();
	XObj::ExecCallbackB(pResult, mdl, pFuncObj);
	if (node == 0 && mVisible && mParent != NULL) {
		MtxP parent = mParent;
		if (parent != NULL) {
			PSMTXCopy(parent, pResult->mpMtx);
		} else {
			PSMTXIdentity(pResult->mpMtx);
		}
		mMoveTranslate.x = parent[0][3];
		mMoveTranslate.y = parent[1][3];
		mMoveTranslate.z = parent[2][3];
		HSD_MtxGetScale(parent, &mMoveScale);
	}
	if (!mVisible)
		return;
	nw4r::g3d::ResNode yawNode   = GetResNode("second");
	nw4r::g3d::ResNode pitchNode = GetResNode("obj1");
	if (node == 0) {
		if (yawNode.IsValid()) {
			pFuncObj->mNodeID = yawNode.GetID();
		}
	} else if (node == yawNode.GetID()) {
		Mtx rot, tmp;
		PSMTXRotRad(rot, 'y', mYaw);
		PSMTXCopy(pResult->mpMtx, tmp);
		PSMTXConcat(tmp, rot, tmp);
		PSMTXCopy(tmp, pResult->mpMtx);
		pFuncObj->mNodeID = yawNode.GetID();
	} else if (node == pitchNode.GetID()) {
		Mtx rot, tmp;
		PSMTXRotRad(rot, 'x', mPitch);
		PSMTXCopy(pResult->mpMtx, tmp);
		PSMTXConcat(tmp, rot, tmp);
		PSMTXCopy(tmp, pResult->mpMtx);
		pFuncObj->mNodeID = pitchNode.GetID();
	}
}

// the GameCube's ShowOXCircle
/**
 * @note Address: 0x8010F784
 * @note Size: 0x64
 */
extern "C" void ShowOXCircle(XObjOX* obj)
{
	obj->Show();
	obj->SetAnim(0, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
}

// the GameCube's ShowOXCross
/**
 * @note Address: 0x8010F7E8
 * @note Size: 0x64
 */
extern "C" void ShowOXCross(XObjOX* obj)
{
	obj->Show();
	obj->SetAnim(1, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
}

// the OX mark's own state machine on its animations
/**
 * @note Address: 0x8010F84C
 * @note Size: 0xD8
 */
// NONMATCHING: the anim id reads sit in different registers
void XObjOX::UpdateObject(u32 frames)
{
	if (!mVisible)
		return;
	switch (GetAnimId()) {
	case 0: // the mark plays once, then hides
		if (IsAnimDone()) {
			Hide();
		}
		break;
	case 1:
		if (IsAnimDone()) {
			Hide();
		}
		break;
	}
	XObj::UpdateObject(frames);
}
