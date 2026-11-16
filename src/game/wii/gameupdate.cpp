// The Wii's CGame frame code, 0x8001C2D8-0x80024AE8 (wii_units.md #11): the new game reset, BeforeUpdate
// with the floor reflection it renders, Update, and the small virtuals of CBase the vtables at
// 0x802E5F60 and 0x802E6028 point here. Retail keeps all of it in one file with main and ~CGame.

#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include "game/wii/infoscrn.h"
#include "game/cscrn.h"
#include "game/fbtexeff.h"
#include "game/stagedata.h"
#include "game/evt.h"
#include "game/gmath.h"
#include <math.h>
#include <stdlib.h>

#include "revolution/mem.h"

// the expanded heap mm::InitMem1 creates over the MEM1 arena; every operator delete site reads it
extern MEMiHeapHead* gWiiMem1Heap;

// this file's deletes go straight to the MEM1 heap: the Wii gives operator delete an inline body, so
// the destructors emitted here free through MEMFreeToExpHeap themselves
inline void operator delete(void* ptr) throw()
{
	MEMFreeToExpHeap(gWiiMem1Heap, ptr);
}

struct XObjPool;

// the head of the stage's info record, as far as this file reads it: the GameCube's UnkStageInfo
// (game/cgameutil.h, which the Wii cannot include). The loaded stage's EnvInfo starts with a pointer to it
struct WiiStageInfo {
	u8 _00[0x6];
	s16 mCameraMode; // _06
	u8 mMirrorFade;  // _08
};

static inline WiiStageInfo* GetStageInfo()
{
	return *(WiiStageInfo**)GetStageEnvironment();
}

extern "C" {
void ResetPooledObjects(XObjPool* pools); // 0x800C5424
void InitBatteryGauge(InfoScrn* scrn, f32 battery);
void PSMTXReflect(Mtx m, const Vec* p, const Vec* n);
BOOL FinalizeStageObjectSetup(CGame* game);
void BuildObjectDrawList(CGame* game);
void Reset2__16CCameraActionMapFv(CInterfaceCameraAction* action);
// only the addresses are retail's, the signatures are read off the calls in this file
BOOL fn_800C54A4(XObjPool* pools);        // the objects' loading step, true once they are all read
void fn_800C7F70(void* map);              // takes the map object, the first of pool 3
BOOL fn_800CDC08(WiiPlayerState* player); // three steps of the player's own setup, each true when done
BOOL fn_800CE4B0(WiiPlayerState* player);
BOOL fn_800CD700(WiiPlayerState* player);
void fn_802BE414(void* obj, CLight* light);
void fn_802BE4D4(void* obj, GXFogType type, const GXColor& color, f32 startZ, f32 endZ, f32 nearZ, f32 farZ);
void fn_8027AE04(WiiGsSceneObj* scene, s32 index);
void* fn_8027AE0C(WiiGsSceneObj* scene, s32 index); // the scene's fog
void fn_8028175C(void** fog, GXFogType* type, f32* startZ, f32* endZ, f32* nearZ, f32* farZ, GXColor* color);
}

/**
 * @note Address: 0x8001C2D8
 * @note Size: 0x34
 */
void CScrnWipeCine::Reset()
{
	CScrnWipe::Reset();
	mScaleToWidescreen = FALSE;
}

/**
 * @note Address: 0x8001C34C
 * @note Size: 0x30
 */
void CScrnWipeTile::StartTransition(u8 r, u8 g, u8 b, u8 a, u8 targetR, u8 targetG, u8 targetB, f32 x, f32 y, f32 width, f32 height,
                                    f32 targetWidth, f32 targetHeight, f32 time, u8 targetA, BOOL hideWhenDone)
{
	CScrnWipe::StartTransition(r, g, b, a, targetR, targetG, targetB, x, y, width, height, targetWidth, targetHeight, time, targetA,
	                           hideWhenDone);
}

/**
 * @note Address: 0x8001C37C
 * @note Size: 0x410
 */
// NONMATCHING: retail hands both vectors to their setter by value (a second copy of each on the stack);
// CVec has no such setter in ut.h and its name is not known
// what a new game starts from, the GameCube's inline of the same name made a virtual
void CGame::ResetNewGameState()
{
	ClearStatusFlags(0x40);
	gWiiPointer.ResetRumble(TRUE);
	for (int i = 0; i < 128; ++i)
		gEventFlags.mFlags[i] = 0;
	for (int i = 0; i < 2048; ++i)
		gEventVars.mVars[i] = 0;

	mMemcardLoaded       = FALSE;
	mTimeState.mMode     = 0;
	mTimeState.mTime     = 0.0f;
	mTimeState.mDuration = 1.0f;
	mTimeState.mRunning  = FALSE;
	mPlayTime            = 0.0f;
	_EC1A4               = 0.0f;
	_EC1A8               = 1.0f;
	mScopeTimer          = 0.0f;
	_EC2B4               = NAN;
	_EC2C8               = -1.0f;
	ResetPooledObjects((XObjPool*)mXObjPools);
	InitBatteryGauge((InfoScrn*)mInfoScrn, GetPlayer()->mParams.mBattery);

	mScrnTex.mAngle = 0.0f;
	StartScreenFade(&mScrnTex.mColor, 0, 0, 0, 0, 100, 100, 200, 0, 0.0f);
	StartScreenFade(&mColor0.mColor, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f);
	StartScreenFade(&mColor1.mColor, 0, 0, 0, 0, 255, 255, 255, 0, 0.0f);
	mOutlineWidthControl = 0;
	mOutlineWidth        = 0.5f;
	Vec zero             = { 0.0f, 0.0f, 0.0f };
	_EE1E0.Set(zero);
	Vec nan = { NAN, NAN, NAN };
	_EE1F0.Set(nan);
	_D9B60             = 0;
	_D9B5C             = 0;
	mStageName.mActive = FALSE;
	for (int i = 0; i < 20; ++i) {
		mStageName.mChars[i].mScale = 0.0f;
		mStageName.mChars[i].mWait  = 0.0f;
		mStageName.mChars[i].mBob   = DEG2RAD * (rand() % 360);
		mStageName.mChars[i].mTilt  = DEG2RAD * (rand() % 360);
	}
	mMotionControlCamera.fn_80167728();
}

/**
 * @note Address: 0x8001C78C
 * @note Size: 0x24
 */
void CBase::ClearStatusFlags(u64 mask)
{
	mStatus.mFlags &= ~mask;
}

/**
 * @note Address: 0x8001D500
 * @note Size: 0x14
 */
u64 CBase::GetStatusFlags(u64 mask) const
{
	return mask & mStatus.mFlags;
}

/**
 * @note Address: 0x8001D5B4
 * @note Size: 0x1C
 */
void CBase::SetStatusFlags(u64 mask)
{
	mStatus.mFlags |= mask;
}

/**
 * @note Address: 0x8001D5D0
 * @note Size: 0x6B8
 */
// NONMATCHING: retail copies rot, trans, point and normal once more on the stack, so the matrix is
// built by inlines that take them by value and whose split is not known; the registers follow from that
// the step after the objects are read, the GameCube function of the same name cut into six steps
// (mSetupStep): lists the mirrors and the floor reflection, sets the player up, then points the camera,
// the lights and the fog at the stage
extern "C" BOOL FinalizeStageObjectSetup(CGame* game)
{
	FbMirror* mirror;
	s32 mirrors;
	CMirrorObj* obj;
	s32 i;
	switch (game->mSetupStep) {
	case 0:
		if (!fn_800C54A4((XObjPool*)game->mXObjPools)) {
			break;
		}
		if (game->mResetRequested) {
			return TRUE;
		}
		game->mSetupStep++;
	case 1: {
		fn_800C7F70(*(void**)&game->mXObjPools[3 * 0x14]);
		game->ClearStatusFlags(0x2000000000ULL);
		game->ClearStatusFlags(0x8000000000ULL);
		game->ClearStatusFlags(0x100000000000ULL);
		game->mFloorReflect = FALSE;
		mirror              = (FbMirror*)game->mMirrors;
		mirrors             = 0;
		for (i = 0; i < game->mObjectCount; i++) {
			obj = (CMirrorObj*)game->mObjects[i];
			if (obj->mStatus & 1) {
				if (!game->GetStatusFlags(0x2000000000ULL) && (obj->mStatus & 0x20000000)) {
					game->SetStatusFlags(0x2000000000ULL); // the stage wants the floor reflection
					game->mFloorReflect = TRUE;

					Vec rot   = { 0.0f, 0.0f, 0.0f };
					Vec trans = { 0.0f, 0.0f, 0.0f };
					trans.y   = 0.0f;
					Mtx mtx;
					Mtx rotX;
					Mtx rotY;
					Mtx rotMtx;
					Mtx transMtx;
					PSMTXRotRad(rotX, 'X', DEG2RAD * rot.x);
					PSMTXRotRad(rotY, 'Y', DEG2RAD * rot.y);
					PSMTXConcat(rotY, rotX, rotMtx);
					PSMTXTrans(transMtx, trans.x, trans.y, trans.z);
					PSMTXConcat(transMtx, rotMtx, mtx);

					Vec point  = { 0.0f, 0.0f, 0.0f };
					Vec normal = { 0.0f, 1.0f, 0.0f };
					PSMTXMultVec(mtx, &point, &point);
					PSMTXInverse(mtx, mtx);
					PSMTXTranspose(mtx, mtx);
					PSMTXMultVec(mtx, &normal, &normal);
					PSVECNormalize(&normal, &normal);
					PSMTXReflect(game->mFloorReflectMtx, &point, &normal);
				}
				if ((obj->mStatus & 0x400) && mirrors < 5) { // flag 0x400 makes an object a mirror
					game->SetStatusFlags(0x100000000000ULL);
					if (obj->mF0 != NULL) {
						mirror->SetMirror(obj);
						mirror++;
						mirrors++;
					}
				}
				if (!game->GetStatusFlags(0x8000000000ULL) && (obj->mStatus & 0x80000000)) {
					game->SetStatusFlags(0x8000000000ULL); // an object needs the depth check
				}
			}
		}
		game->mSetupStep++;
	}
	case 2:
		if (!fn_800CDC08(game->mPlayer)) {
			break;
		}
		game->mSetupStep++;
	case 3:
		if (!fn_800CE4B0(game->mPlayer)) {
			break;
		}
		game->mSetupStep++;
	case 4:
		if (!fn_800CD700(game->mPlayer)) {
			break;
		}
		game->mSetupStep++;
	case 5: {
		game->SetupStageCamera(GetStageInfo()->mCameraMode);
		game->mLightList = NULL;
		if (gGame.mGs.mSceneHead->_2888 == NULL) {
			game->mLightList = game->mLights;
		}
		for (i = 0; i < 9; i++) {
			gGame.mGs._1FE4[i]._18 = FALSE;
			gGame.mGs._1FE4[i]._08 = FALSE;
			gGame.mGs._1FE4[i]._00 = FALSE;
		}
		game->SetCameraLight();
		fn_802BE414(game->_B7A8, gGame.mGs.mLight);
		if (gGame.mGs.mSceneHead->_2888 != NULL) {
			void* fog = fn_8027AE0C(gGame.mGs.mSceneHead, 0);
			GXFogType type;
			f32 startZ;
			f32 endZ;
			f32 nearZ;
			f32 farZ;
			GXColor color;
			fn_8028175C(&fog, &type, &startZ, &endZ, &nearZ, &farZ, &color);
			GXColor fogColor = color;
			fn_802BE4D4(game->_B7A8, type, fogColor, startZ, endZ, nearZ, farZ);
		} else {
			GXColor none = { 0, 0, 0, 0 };
			game->mFog.Init(GX_FOG_NONE, none, 0.0f, 0.0f, 0.0f, 0.0f);
			GXColor black = { 0, 0, 0, 0 };
			fn_802BE4D4(game->_B7A8, GX_FOG_NONE, black, 0.0f, 0.0f, 0.0f, 0.0f);
		}
		BuildObjectDrawList(game);
		if (game->GetStatusFlags(0x2000000000ULL)) {
			f32 fade        = GetStageInfo()->mMirrorFade / 100.0f;
			CMirrorObj* map = *(CMirrorObj**)&game->mXObjPools[3 * 0x14];
			if ((map->mStatus & 1) && fade > 0.0f) {
				// the GameCube made the map translucent here, nothing is left of that
			}
		}
		// three reads the code does nothing with
		game->GetStatusFlags(0x2000000000ULL);
		game->GetStatusFlags(0x100000000000ULL);
		game->GetStatusFlags(0x8000000000ULL);
		game->mHudObject  = NULL;
		game->mHudObjectX = 0.0f;
		game->mHudObjectY = 0.0f;
		game->_ECCF4      = 0;
		game->_ECCFC      = -1;
		game->_ECD00      = -1;
		game->_ECD04      = 0;
		game->_ECD08      = 0;
		game->_ECD0C      = 0;
		game->_ECD10      = 0;
		game->_ECD14      = -1;
		game->_ECD18[0]   = 0.0f;
		game->_ECD18[1]   = 0.0f;
		game->_ECD18[2]   = 0.0f;
		game->_ECD18[3]   = 0.0f;
	}
	}
	return game->mSetupStep == 5;
}

/**
 * @note Address: 0x8001DC88
 * @note Size: 0x1F0
 */
// points the camera and its actions at the stage's objects; mode is the stage's camera action
void CGame::SetupStageCamera(s32 mode)
{
	((CCameraActionScroll*)mCameraActions.mTable[CAMACTION_SCROLL])->mZoom = 1;
	if (GetStageCameraAreaRecords() && GetStageCameraInfoRecords()) {
		mCameraActions.mTable[CAMACTION_AREA]->Reset();
	}
	if (GetStageCameraAreaRecords() && GetStageCameraInfoRecords()) {
		((CCameraActionAreaExM*)mCameraActions.mTable[CAMACTION_AREA])->LoadAreaState();
	}
	((CCameraActionAnim*)mCameraActions.mTable[CAMACTION_ANIM])->SetObj(*(XObjCamera**)&mXObjPools[0]);
	mMotionControlCamera.Init();
	if (mode == 1 || mode == 7) { // both modes start on the map camera
		mMotionControlCamera.ChangeAction(7);
		Reset2__16CCameraActionMapFv(mCameraActions.mTable[CAMACTION_MAP]);
		mMotionControlCamera.ApplyAction();
	} else {
		mMotionControlCamera.ChangeAction(mode);
	}
	for (s32 i = 0; i < mObjectCount; i++) {
		mMotionControlCamera.UpdateTarget((XObj*)mObjects[i]);
	}
	if (GetStatusFlags(0x100)) { // flag 0x100: a fresh roboeyes wipe
		ClearStatusFlags(0x300);
		mRoboeyes.Start(0, 0, 0, 0, TRUE, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	}
	fn_8027AE04(gGame.mGs.mSceneHead, 0);
}

/**
 * @note Address: 0x8001DE88
 * @note Size: 0x78
 */
void CBase::SetCameraLight()
{
	CCamera* camera = GetCamera();
	SetLight(GetLight(), camera->mViewMtx, TRUE);
}

/**
 * @note Address: 0x8001DF00
 * @note Size: 0x18
 */
void CBase::SetLight(CLight* light, MtxP view, BOOL flag)
{
	mGs.mLight = light;
	mGs.fn_802AB81C(view, flag);
}

/**
 * @note Address: 0x8001DF18
 * @note Size: 0x10
 */
CLight* CBase::GetLight()
{
	return GetDefaultLight();
}

/**
 * @note Address: 0x8001DF28
 * @note Size: 0x8
 */
// the member behind the two cameras; cgame.h does not type that block yet
CLight* CBase::GetDefaultLight()
{
	return mLights;
}

/**
 * @note Address: 0x8001DF30
 * @note Size: 0x10
 */
CCamera* CBase::GetCamera()
{
	return GetPerspectiveCamera();
}

/**
 * @note Address: 0x8001DF40
 * @note Size: 0x8
 */
CCamera* CBase::GetPerspectiveCamera()
{
	return (CCamera*)mCameras[0];
}

/**
 * @note Address: 0x8001DF48
 * @note Size: 0x280
 */
// NONMATCHING: the object and the loop counter swap registers in both loops (r6/r8)
// lists the objects in draw order: the ones with flag 0x4000 first, then the rest, then the player and
// last the first object of pool 0. Each entry keeps the object's slot in mObjects
extern "C" void BuildObjectDrawList(CGame* game)
{
	CMirrorObj* obj;
	s32 i;
	for (i = 0; i < 0x400; i++) {
		game->mDrawRefs[i].mObj = NULL;
	}
	game->mDrawRefCount = 0;
	for (i = 0; i < game->mObjectCount; i++) {
		obj = (CMirrorObj*)game->mObjects[i];
		if (obj != (CMirrorObj*)game->mPlayer && obj != *(CMirrorObj**)&game->mXObjPools[0] && (obj->mStatus & 1)
		    && (obj->mStatus & 0x4000)) {
			game->mDrawRefs[game->mDrawRefCount].mObj   = obj;
			game->mDrawRefs[game->mDrawRefCount].mFlags = i;
			game->mDrawRefCount++;
		}
	}
	for (i = 0; i < game->mObjectCount; i++) {
		obj = (CMirrorObj*)game->mObjects[i];
		if (obj != (CMirrorObj*)game->mPlayer && obj != *(CMirrorObj**)&game->mXObjPools[0] && (obj->mStatus & 1)
		    && !(obj->mStatus & 0x4000)) {
			game->mDrawRefs[game->mDrawRefCount].mObj   = obj;
			game->mDrawRefs[game->mDrawRefCount].mFlags = i;
			game->mDrawRefCount++;
		}
	}
	obj = (CMirrorObj*)game->mPlayer;
	if (obj->mStatus & 1) {
		game->mDrawRefs[game->mDrawRefCount].mObj   = obj;
		game->mDrawRefs[game->mDrawRefCount].mFlags = 0;
		game->mDrawRefCount++;
	}
	obj = *(CMirrorObj**)&game->mXObjPools[0];
	if (obj->mStatus & 1) {
		game->mDrawRefs[game->mDrawRefCount].mObj   = obj;
		game->mDrawRefs[game->mDrawRefCount].mFlags = 0;
		game->mDrawRefCount++;
	}
}

/**
 * @note Address: 0x80024A10
 * @note Size: 0x6C
 */
// NONMATCHING: retail has the paired-single subtract inline here, nw4r::math::VEC3Sub from math_types.h; the
// game's include path (include/stl) cannot take the nw4r headers, they want the Wii MSL's <cmath>
CVec CVec::operator-(Vec other) const
{
	CVec result = *this;
	PSVECSubtract(&result, &other, &result);
	return result;
}
