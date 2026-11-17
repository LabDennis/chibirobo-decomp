// The Wii's CGame frame code, 0x8001C2D8-0x80024AE8 (wii_units.md #11): the new game reset, BeforeUpdate
// with the floor reflection it renders, Update, and the small virtuals of CBase the vtables at
// 0x802E5F60 and 0x802E6028 point here. Retail keeps all of it in one file with main and ~CGame.

// before cgame.h: xobj.h's nw4r::math stand-in gives way to the real one
#include "nw4r/math/math_types.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include "game/wii/infoscrn.h"
#include "game/wii/animsfx.h"
#include "game/wii/xobj.h"
#include "game/wii/msg.h"
#include "game/wii/homemenu.h"
#include "game/wii/saveslots.h"
#include "game/wii/xchrmenu.h"
#include "game/globals.h"
#include "arithmetic.h"
#include "game/attachmentvalues.h"
#include "game/cmdline.h"
#include "game/cscrn.h"
#include "game/fbtexeff.h"
#include "game/stagedata.h"
#include "game/evt.h"
#include "game/gmath.h"
#include <math.h>
#include <stdlib.h>

#include "revolution/mem.h"
#include "revolution/sc.h"
#include "nw4r/snd/snd_AxManager.h"
#include "nw4r/math/math_types.h"

// the expanded heap mm::InitMem1 creates over the MEM1 arena; every operator delete site reads it
extern MEMiHeapHead* gWiiMem1Heap;

// this file's deletes go straight to the MEM1 heap: the Wii gives operator delete an inline body, so
// the destructors emitted here free through MEMFreeToExpHeap themselves
inline void operator delete(void* ptr) throw()
{
	MEMFreeToExpHeap(gWiiMem1Heap, ptr);
}

struct CSaveFile;
class CMsg;

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
void PSMTX44MultVec(const Mtx44 m, const Vec* src, Vec* dst);
BOOL FinalizeStageObjectSetup(CGame* game);
void BuildObjectDrawList(CGame* game);
void Reset2__16CCameraActionMapFv(CInterfaceCameraAction* action);
void LoadStageCameraAreaState(CCameraActionAreaEx* state);
// only the addresses are retail's, the signatures are read off the calls in this file
BOOL fn_800C54A4(XObjPool* pools);        // the objects' loading step, true once they are all read
void fn_800C7F70(void* map);              // takes the map object, the first of pool 3
BOOL fn_800CDC08(WiiPlayerState* player); // three steps of the player's own setup, each true when done
BOOL fn_800CE4B0(WiiPlayerState* player);
BOOL LoadPlayerAttachmentAnimation(WiiPlayerState* player);
void fn_802BE414(void* obj, CLight* light);
void fn_802BE4D4(void* obj, GXFogType type, const GXColor& color, f32 startZ, f32 endZ, f32 nearZ, f32 farZ);
void fn_8027AE04(WiiGsSceneObj* scene, s32 index);
void* fn_8027AE0C(WiiGsSceneObj* scene, s32 index); // the scene's fog
void fn_8028175C(void** fog, GXFogType* type, f32* startZ, f32* endZ, f32* nearZ, f32* farZ, GXColor* color);
BOOL HideHudPointCounters(InfoScrn* scrn);
BOOL LoadWiiHomeMenu(WiiHomeMenu* menu);
s32 LoadMessageCursorObject(CMsg* msg);
void ApplyWiiSaveSlot(CSaveFile* manager);
void RestoreSavedPlayerAndCamera(CSaveFile* manager);
void ApplyStageSoundEffects(XSound* sound);
BOOL LoadGlobalResourceData();
u32 GetStartingAttachmentLevel(s32 index);
u32 GetStageEntryTimeComponent(s32 index);
u32 CheckStageFlag(s32 stage, u32 flag);
BOOL LoadStageByIndex(s32 stage);
void ApplyStageClearColor();
BOOL fn_80084848(void* menu);                        // the menus' resources, the GameCube's LoadMenuGroupResources
void fn_8008492C(void* menu, s32 frames);            // steps the open menus
BOOL fn_80094B04(CEvt* evt);                         // the GameCube's LoadEventResources
void fn_800A10E4(CEvt* evt, s16 eventId, s16 index); // the GameCube's RegisterEventObjectId
void fn_800C5814(XObjPool* pools);                   // reads the stage's objects in again
}

extern s32 gRefreshRate;
extern u8 gPowerButtonPressed;

class Logo;
class Title;

extern "C" {
BOOL OSGetResetButtonState();
void OSPanic(const char* file, int line, const char* msg, ...);
char* FormatToStaticBuffer(const char* fmt, ...);
void PauseGameSounds(CGame* game, f32 fadeSeconds);
void ResumeGameSounds(CGame* game, f32 fadeSeconds);
void fn_80024CC0(CGame* game);                      // the frame's work at the start of gamedraw.cpp's range
void fn_802AEDA0(WiiGsSceneObj** head, u32 frames); // steps the queued scene draws
void fn_80084B90(void* menu, s32 index);            // opens one of the menus
void fn_80094C20(CEvt* evt, u32 frames);            // the GameCube's CEvt_Update
void CMsg_Update(CMsg* msg, u32 step);
void UpdateBootLogos(Logo* logo, u32 frames);
void UpdateTitleScreen(Title* title, u32 frames);
void UpdateHudTransitionsAndIndicators(InfoScrn* scrn, u32 frames);
void UpdateStageNameBubbles(CGame* game, u32 frames);
u8* GetStageTimerExpiredScript(void);
void fn_800C7764(XObj* camera, CCamera* view);        // the stage camera object follows the game camera
void fn_800B4988(XObj* obj);                          // one entry of the object reference list
void fn_800D1A8C(WiiPlayerState* player, u32 frames); // four steps of the player after the objects ran
void fn_800D1AF8(WiiPlayerState* player, u32 frames);
void fn_800D1BF0(WiiPlayerState* player, u32 frames);
void fn_800D1B64(WiiPlayerState* player, u32 frames);
void fn_800A13D0(void* effects, CCamera* camera);

extern u8 lbl_8044EE4C[];
// the pointer's position in the last frame, and each stage's scope sounds (0x802C49C8)
extern Vec lbl_80444E98;
Vec fn_8010BEE0(XAction* popper);                             // the popper's muzzle, the GameCube's joint path 0 1 0 0
void fn_800E8140(WiiPlayerState* player, f32 yaw, f32 pitch); // where the eye camera's popper aims
void fn_80114564(XObj* obj);                                  // the scope's new item target
}

BOOL fn_80169658(CMotionControlCamera* camera);
CInterfaceCameraAction* fn_80169674();

// steps the scene list and pops the draws it still holds; Update does it after each group of objects.
// Retail expands it every time, the name is ours
static inline void StepSceneDraws(u32 frames)
{
	fn_802AEDA0(&gGame.mGs.mSceneHead, frames);
	WiiGsSceneObj* scene = gGame.mGs.mSceneHead;
	while (scene->mPending != 0) {
		if (scene->mPending != 0) {
			scene->vf038(scene->mPending - 1);
		}
	}
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
 * @note Address: 0x8001C30C
 * @note Size: 0x20
 */
// the screen's size out of the render mode
void FbEffect::Init(const GXRenderModeObj* rmode, void* buf, f32 aspect)
{
	Init(rmode->fbWidth, rmode->efbHeight, buf, aspect);
}

/**
 * @note Address: 0x8001C32C
 * @note Size: 0x20
 */
void FbBlur::Init(const GXRenderModeObj* rmode, void* buf, f32 aspect)
{
	Init(rmode->fbWidth, rmode->efbHeight, buf, aspect);
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
	mPointer.mAlpha      = 1.0f;
	mPointer.mScopeTimer = 0.0f;
	mPointer._10C        = NAN;
	mPointer._120        = -1.0f;
	ResetPooledObjects(mXObjs.mPools);
	InitBatteryGauge(&mInfoScrn, GetPlayer()->mParams.mBattery);

	mScrnTex.mAngle = 0.0f;
	StartScreenFade(&mScrnTex.mColor, 0, 0, 0, 0, 100, 100, 200, 0, 0.0f);
	StartScreenFade(&mColor0.mColor, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f);
	StartScreenFade(&mColor1.mColor, 0, 0, 0, 0, 255, 255, 255, 0, 0.0f);
	mOutlineWidthControl = 0;
	mOutlineWidth        = 0.5f;
	Vec zero             = { 0.0f, 0.0f, 0.0f };
	mListener.mPos.Set(zero);
	Vec nan = { NAN, NAN, NAN };
	mListener.mPoint.Set(nan);
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

// a menu still open or the objects not read in again yet: steps 20 to 22 of BeforeUpdate wait for both.
// Retail expands the same code three times; the name is ours
static inline BOOL IsStageBusy(CGame* game)
{
	if (game->mMenu.mActive) {
		fn_8008492C(&game->mMenu.mActive, 10);
		if (game->mMenu.mActive) {
			return TRUE;
		}
	}
	if (game->_41F0 > 0) {
		if (--game->_41F0 <= 0) {
			game->mLightList = game->mLights;
			fn_800C5814(game->mXObjs.mPools);
		} else {
			return TRUE;
		}
	}
	return FALSE;
}

static inline BOOL LoadStartingAttachments(CGame* game)
{
	if (LoadGlobalResourceData()) {
		if (!gCmdLineArgs.mCopterLevelOverride) { // attachment slot 1 takes its starting level from the global resource table
			u32 level = GetStartingAttachmentLevel(0);
			SetAttachmentCountById((AttachmentValues*)game->GetPlayer()->mAttachments, 1, level);
		}
		if (!gCmdLineArgs.mPopperLevelOverride) { // slot 2
			u32 level = GetStartingAttachmentLevel(1);
			SetAttachmentCountById((AttachmentValues*)game->GetPlayer()->mAttachments, 2, level);
		}
		if (!gCmdLineArgs.mRadarLevelOverride) { // slot 3
			u32 level = GetStartingAttachmentLevel(2);
			SetAttachmentCountById((AttachmentValues*)game->GetPlayer()->mAttachments, 3, level);
		}
		return TRUE;
	}
	return FALSE;
}

static inline BOOL LoadStageAndEventObjects(CGame* game)
{
	if (LoadStageByIndex(game->mStage)) {
		ApplyStageClearColor();
		for (const StageEventObject* entry = GetStageEventObjectPairs(); entry->objectIndex >= 0; ++entry) { // the pairs end at a -1
			fn_800A10E4(&game->mEvt, entry->eventId, entry->objectIndex);
		}
		Vec origin = { 0.0f, 0.0f, 0.0f };
		((CSound3D*)&gGame.mSound.mSound3DManager)->SetListener(0.0f, &origin);
		((CSound3D*)&gGame.mSound.mSound3DManager)->SetVolumePanPitch(10.0f, 10.0f, 50.0f);
		return TRUE;
	}
	return FALSE;
}

static inline bool FadeStopped(const CScrnColor& fade)
{
	return 0.0f == fade.mSpeed.r && 0.0f == fade.mSpeed.g && 0.0f == fade.mSpeed.b && 0.0f == fade.mSpeed.a;
}

/**
 * @note Address: 0x8001C7B0
 * @note Size: 0xD50
 */
// NONMATCHING: CSound3D::Init takes no argument in its declaration, retail passes one (it seems, whatever is
// left in r4); retail reads the
// attachment overrides out of the whole word of gCmdLineArgs (its bitfields are u32 on the Wii, u8 in
// cmdline.h, which gameinit.cpp initializes for both); and the logo and title states are loaded once for
// their two tests, as if they sat in objects of their own (CGame destroys one at _5978 and one at _5C70)
// the GameCube function with the Wii's resources: one step a frame until the stage runs, true when done
BOOL CGame::BeforeUpdate()
{
	// Ready steps fall through. Pending reads resume at the same step next frame.
	switch (mLoadStep) {
	case 0:
		return TRUE;
	case 1:
		++mLoadStep;
	case 2:
		if (GetStatusFlags(0x400000000ULL) && mLogo.mState != 12) {
			if (!IsLogoActive())
				mLogo.mState = 1;
			if (mLogo.mState <= 3)
				break;
			++mLoadStep;
		} else {
			++mLoadStep;
		}
	case 3:
		if (!mGs.mFont.LoadResource())
			break;
		++mLoadStep;
	case 4:
		if (!gGame.mSound.OpenArchiveAsync("snd/cb_robo", 0))
			break;
		++mLoadStep;
	case 5:
		if (gGame.mSound.mLoadPending)
			break;
		switch (SCGetSoundMode()) {
		case SC_SOUND_MODE_MONO:
			nw4r::snd::detail::AxManager::GetInstance().SetOutputMode(nw4r::snd::OUTPUT_MODE_MONO);
			break;
		case SC_SOUND_MODE_STEREO:
			nw4r::snd::detail::AxManager::GetInstance().SetOutputMode(nw4r::snd::OUTPUT_MODE_STEREO);
			break;
		case SC_SOUND_MODE_SURROUND:
			nw4r::snd::detail::AxManager::GetInstance().SetOutputMode(nw4r::snd::OUTPUT_MODE_DPL2);
			break;
		}
		((CSound3D*)&gGame.mSound.mSound3DManager)->Init(NULL);
		++mLoadStep;
	case 6:
		if (GetStatusFlags(0x200000000ULL)) {
			if (!mArchive.ReadEx("/cb.bin", 0, 0))
				break;
			++mLoadStep;
		} else {
			++mLoadStep;
		}
	case 7: {
		CoordInfo spawn(GetSpawnData(12)); // spawn placement 12
		if (!((XObj*)mPointer.mCursor)->LoadFromPlacement(spawn))
			break;
		((XObj*)mPointer.mCursor)->SetScale(6.0f);
		++mLoadStep;
	}
	case 8:
		if (!mTpl.ReadEx("tpl/misc.tpl", 0, 0))
			break;
		++mLoadStep;
		mTpl.mTex.Init(&mTpl.mPalette->descriptorArray[261], 0);
		mPointer.mTex = mTpl.mTex;
		_41E0         = &mPointer.mTex;
		mTpl.mTex.Init(&mTpl.mPalette->descriptorArray[122], 0); // the outline texture
		mScrnTex.mTex        = mTpl.mTex;
		mOutline.mPatternTex = &mScrnTex.mTex;
		mOutline.mFrame      = 0;
		mOutline.mThick2     = 0.0f;
	case 9:
		if (!mEffects.Load("eff/cb_robo", 0))
			break;
		++mLoadStep;
	case 10:
		if (!mEffects.UpdateLoad(0))
			break;
		++mLoadStep;
	case 11:
		if (!fn_80084848(&mMenu.mActive))
			break;
		++mLoadStep;
	case 12:
		if (!LoadWiiHomeMenu(&mHomeMenu))
			break;
		++mLoadStep;
	case 13:
		++mLoadStep;
	case 14:
		if (!LoadStartingAttachments(this))
			break;
		++mLoadStep;
	case 15:
		if (!HideHudPointCounters(&mInfoScrn))
			break;
		++mLoadStep;
	case 16:
		if (!fn_80094B04(&mEvt))
			break;
		++mLoadStep;
	case 17:
		if (!LoadMessageCursorObject(&mMsgs[0]))
			break;
		LoadMessageCursorObject(&mMsgs[1]);
		++mLoadStep;
	case 18: {
		CoordInfo spawn(GetSpawnData(14)); // spawn placement 14
		if (!((XObj*)GetPlayer())->LoadFromPlacement(spawn))
			break;
		++mLoadStep;
	}
	case 19:
		if (GetStatusFlags(0x400000000ULL)) {
			if (mLogo.mState != 12)
				break;
			++mLoadStep;
		} else {
			++mLoadStep;
		}
		break;
	case 20:
		if (IsStageBusy(this))
			break;
		if (GetStatusFlags(0x20000000) && mTitle.mState != 23) {
			if (!IsTitleActive()) {
				ResetNewGameState();
				mTitle.mState = 1;
			}
			break;
		}
		++mLoadStep;
	case 21:
		if (IsStageBusy(this))
			break;
		if (mMemcardLoaded) { // a save restores the battery too
			ApplyWiiSaveSlot(&mSaveManager);
			mMemcardLoaded = FALSE;
			SetStatusFlags(0x8000);
			InitBatteryGauge(&mInfoScrn, GetPlayer()->mParams.mBattery);
		}
		++mLoadStep;
	case 22:
		if (IsStageBusy(this))
			break;
		mPointer._120 = -1.0f;
		++mLoadStep;
	case 23:
		if (!LoadStageAndEventObjects(this))
			break;
		++mLoadStep;
	case 24: {
		u8* script = GetStageSetupScript();
		if (script)
			StartOrQueueEventScript(&mEvt, script, 0, -1, -1);
		++mLoadStep;
	}
	case 25:
		if (mEvt.mData.mScript) // waits for the setup script
			break;
		if (CheckStageFlag(mStage, 4)) // stage flag 4 sets flag 0x40
			SetStatusFlags(0x40);
		mSetupStep = 0;
		++mLoadStep;
	case 26:
		if (!FinalizeStageObjectSetup(this))
			break;
		++mLoadStep;
		if (GetStatusFlags(0x40))
			break;
	case 27:
		ClearStatusFlags(0x40);
		mShadowFocus  = NULL;
		mShadowWidth  = 0.0f;
		mShadowHeight = 0.0f;
		mShadowAlpha  = 1.0f;
		if (GetStatusFlags(0x8000))
			RestoreSavedPlayerAndCamera(&mSaveManager);
		++mLoadStep;
	case 28:
		if (!CallStageModule(0, 0, NULL))
			break;
		ApplyStageSoundEffects(&gGame.mSound);
		++mLoadStep;
	case 29: {
		if (!FadeStopped(mFade))
			break;
		StartScreenFade(&mFade, 0, 0, 0, 255, 0, 0, 0, 0, 0.5f);
		StartOrQueueEventScript(&mEvt, GetStageEntryScript(), 0, -1, -1);
		u32 minutes = GetStageEntryTimeComponent(1), hours = GetStageEntryTimeComponent(0);
		_EC1A4 = (s32)(hours * 216000 + (minutes * 3600 + GetStageEntryTimeComponent(2) * 60)); // the clock, in frames
		SetStatusFlags(1);
		++mLoadStep;
	}
	}
	if (IsLoadStepRunning()) {
		// past the first steps, and the logos done or never shown: the loading screen may come up
		if (mLoadStep > 2 && (!GetStatusFlags(0x400000000ULL) || mLogo.mState == 12)) {
			if (!GetStatusFlags(0x100000000000000ULL))
				SetStatusFlags(0x100000000000000ULL);
			if (!(_41B0 & 1)) {
				_41B0 |= 7;
				_41C1 = 1;
				_41C0 = 0;
				_41C3 = 1;
			}
		}
		return FALSE;
	}
	return TRUE;
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
		if (!fn_800C54A4(game->mXObjs.mPools)) {
			break;
		}
		if (game->mResetRequested) {
			return TRUE;
		}
		game->mSetupStep++;
	case 1: {
		fn_800C7F70((void*)game->mXObjs.mPools[3].mObjects);
		game->ClearStatusFlags(0x2000000000ULL);
		game->ClearStatusFlags(0x8000000000ULL);
		game->ClearStatusFlags(0x100000000000ULL);
		game->mPointer.mFloorReflect = FALSE;
		mirror                       = game->mMirrors;
		mirrors                      = 0;
		for (i = 0; i < game->mXObjs.mObjectCount; i++) {
			obj = (CMirrorObj*)game->mXObjs.mObjects[i];
			if (obj->mStatus & 1) {
				if (!game->GetStatusFlags(0x2000000000ULL) && (obj->mStatus & 0x20000000)) {
					game->SetStatusFlags(0x2000000000ULL); // the stage wants the floor reflection
					game->mPointer.mFloorReflect = TRUE;

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
					PSMTXReflect(game->mPointer.mFloorReflectMtx, &point, &normal);
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
		if (!fn_800CDC08(game->GetPlayer())) {
			break;
		}
		game->mSetupStep++;
	case 3:
		if (!fn_800CE4B0(game->GetPlayer())) {
			break;
		}
		game->mSetupStep++;
	case 4:
		if (!LoadPlayerAttachmentAnimation(game->GetPlayer())) {
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
		fn_802BE414(&game->mEffects, gGame.mGs.mLight);
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
			fn_802BE4D4(&game->mEffects, type, fogColor, startZ, endZ, nearZ, farZ);
		} else {
			GXColor none = { 0, 0, 0, 0 };
			game->mFog.Init(GX_FOG_NONE, none, 0.0f, 0.0f, 0.0f, 0.0f);
			GXColor black = { 0, 0, 0, 0 };
			fn_802BE4D4(&game->mEffects, GX_FOG_NONE, black, 0.0f, 0.0f, 0.0f, 0.0f);
		}
		BuildObjectDrawList(game);
		if (game->GetStatusFlags(0x2000000000ULL)) {
			f32 fade        = GetStageInfo()->mMirrorFade / 100.0f;
			CMirrorObj* map = (CMirrorObj*)game->mXObjs.mPools[3].mObjects;
			if ((map->mStatus & 1) && fade > 0.0f) {
				// the GameCube made the map translucent here, nothing is left of that
			}
		}
		// three reads the code does nothing with
		game->GetStatusFlags(0x2000000000ULL);
		game->GetStatusFlags(0x100000000000ULL);
		game->GetStatusFlags(0x8000000000ULL);
		game->mPointer.mHudObject       = NULL;
		game->mPointer.mHudObjectX      = 0.0f;
		game->mPointer.mHudObjectY      = 0.0f;
		game->mPointer.mScopeTarget     = NULL;
		game->mPointer.mScopeMarker     = -1;
		game->mPointer.mScopeItem       = -1;
		game->mPointer.mScopeTicks      = 0;
		game->mPointer.mScopeRareItem   = 0;
		game->mPointer.mScopeLevel      = 0;
		game->mPointer.mScopeSoundIndex = 0;
		game->mPointer.mScopeSound      = -1;
		game->mPointer.mScopeZoom       = 0.0f;
		game->mPointer.mScopeAngle      = 0.0f;
		game->mPointer.mScopeFade       = 0.0f;
		game->mPointer.mScopeFade2      = 0.0f;
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
	((CCameraActionAnim*)mCameraActions.mTable[CAMACTION_ANIM])->SetObj((XObjCamera*)mXObjs.mPools[0].mObjects);
	mMotionControlCamera.Init();
	if (mode == 1 || mode == 7) { // both modes start on the map camera
		mMotionControlCamera.ChangeAction(7);
		Reset2__16CCameraActionMapFv(mCameraActions.mTable[CAMACTION_MAP]);
		mMotionControlCamera.ApplyAction();
	} else {
		mMotionControlCamera.ChangeAction(mode);
	}
	for (s32 i = 0; i < mXObjs.mObjectCount; i++) {
		mMotionControlCamera.UpdateTarget((XObj*)mXObjs.mObjects[i]);
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
	return &mCameras[0];
}

/**
 * @note Address: 0x8001DF48
 * @note Size: 0x280
 */
// NONMATCHING: the object and the loop counter swap registers in both loops (r6/r8)
// lists the objects in draw order: the ones with flag 0x4000 first, then the rest, then the player and
// last the first object of pool 0. Each entry keeps the object's slot in mXObjs.mObjects
extern "C" void BuildObjectDrawList(CGame* game)
{
	CMirrorObj* obj;
	s32 i;
	for (i = 0; i < 0x400; i++) {
		game->mDrawLists[0].mRefs[i].mObj = NULL;
	}
	game->mDrawLists[0].mCount = 0;
	for (i = 0; i < game->mXObjs.mObjectCount; i++) {
		obj = (CMirrorObj*)game->mXObjs.mObjects[i];
		if (obj != (CMirrorObj*)game->GetPlayer() && obj != (CMirrorObj*)game->mXObjs.mPools[0].mObjects && (obj->mStatus & 1)
		    && (obj->mStatus & 0x4000)) {
			game->mDrawLists[0].mRefs[game->mDrawLists[0].mCount].mObj   = obj;
			game->mDrawLists[0].mRefs[game->mDrawLists[0].mCount].mIndex = i;
			game->mDrawLists[0].mCount++;
		}
	}
	for (i = 0; i < game->mXObjs.mObjectCount; i++) {
		obj = (CMirrorObj*)game->mXObjs.mObjects[i];
		if (obj != (CMirrorObj*)game->GetPlayer() && obj != (CMirrorObj*)game->mXObjs.mPools[0].mObjects && (obj->mStatus & 1)
		    && !(obj->mStatus & 0x4000)) {
			game->mDrawLists[0].mRefs[game->mDrawLists[0].mCount].mObj   = obj;
			game->mDrawLists[0].mRefs[game->mDrawLists[0].mCount].mIndex = i;
			game->mDrawLists[0].mCount++;
		}
	}
	obj = (CMirrorObj*)game->GetPlayer();
	if (obj->mStatus & 1) {
		game->mDrawLists[0].mRefs[game->mDrawLists[0].mCount].mObj   = obj;
		game->mDrawLists[0].mRefs[game->mDrawLists[0].mCount].mIndex = 0;
		game->mDrawLists[0].mCount++;
	}
	obj = (CMirrorObj*)game->mXObjs.mPools[0].mObjects;
	if (obj->mStatus & 1) {
		game->mDrawLists[0].mRefs[game->mDrawLists[0].mCount].mObj   = obj;
		game->mDrawLists[0].mRefs[game->mDrawLists[0].mCount].mIndex = 0;
		game->mDrawLists[0].mCount++;
	}
}

/**
 * @note Address: 0x8001E1C8
 * @note Size: 0x668
 */
// sorts the objects into the ones to draw and the ones mirrors show, those of pool 3 first, then the plain
// ones, then the 0xA00000 ones, then the 0x40 ones. Objects out of view are only mirrored
extern "C" void BuildSceneDrawLists(CGame* game)
{
	XObj* obj;
	s32 i;
	if (game->GetStatusFlags(0x8000000000ULL)) { // only while flag 0x8000000000 stands
		CMotionControlCamera* camera = &game->mMotionControlCamera;
		for (i = 0; i < game->mXObjs.mObjectCount; i++) {
			obj = (XObj*)game->mXObjs.mObjects[i];
			if ((obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
				if (!game->GetStatusFlags(0x100) && (obj->mStatus.mFlags & 0x80000000) && obj->IsOutsideCameraDepth(camera)) {
					obj->mStatus.mFlags |= 0x1000000000000ULL; // out of depth: mirror-only
				} else {
					obj->mStatus.mFlags &= ~0x1000000000000ULL;
				}
			}
		}
	}

	for (i = 0; i < 0x400; i++) {
		game->mDrawLists[2].mRefs[i].mObj = NULL;
	}
	game->mDrawLists[2].mCount = 0;
	for (i = 0; i < 0x400; i++) {
		game->mDrawLists[3].mRefs[i].mObj = NULL;
	}
	game->mDrawLists[3].mCount = 0;

	for (i = 0; i < game->mXObjs.mPools[3].mUsed; i++) {
		obj = game->mXObjs.mPools[3].Get(i);
		if ((obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000) && !(obj->mStatus.mFlags & 0xA00040)) {
			if (!(obj->mStatus.mFlags & 0x1000000000000ULL)) {
				game->mDrawLists[2].mRefs[game->mDrawLists[2].mCount].mObj   = obj;
				game->mDrawLists[2].mRefs[game->mDrawLists[2].mCount].mIndex = 0;
				game->mDrawLists[2].mCount++;
			}
			game->mDrawLists[3].mRefs[game->mDrawLists[3].mCount].mObj   = obj;
			game->mDrawLists[3].mRefs[game->mDrawLists[3].mCount].mIndex = 0;
			game->mDrawLists[3].mCount++;
		}
	}
	for (i = 0; i < game->mXObjs.mObjectCount; i++) {
		obj = (XObj*)game->mXObjs.mObjects[i];
		if (obj->GetPool() != 3 && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000) && !(obj->mStatus.mFlags & 0xA00040)) {
			if (!(obj->mStatus.mFlags & 0x1000000000000ULL)) {
				game->mDrawLists[2].mRefs[game->mDrawLists[2].mCount].mObj   = obj;
				game->mDrawLists[2].mRefs[game->mDrawLists[2].mCount].mIndex = 0;
				game->mDrawLists[2].mCount++;
			}
			game->mDrawLists[3].mRefs[game->mDrawLists[3].mCount].mObj   = obj;
			game->mDrawLists[3].mRefs[game->mDrawLists[3].mCount].mIndex = 0;
			game->mDrawLists[3].mCount++;
		}
	}
	for (i = 0; i < game->mXObjs.mObjectCount; i++) {
		obj = (XObj*)game->mXObjs.mObjects[i];
		if (obj->GetPool() != 3 && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000) && (obj->mStatus.mFlags & 0xA00000)
		    && !(obj->mStatus.mFlags & 0x40)) {
			if (!(obj->mStatus.mFlags & 0x1000000000000ULL)) {
				game->mDrawLists[2].mRefs[game->mDrawLists[2].mCount].mObj   = obj;
				game->mDrawLists[2].mRefs[game->mDrawLists[2].mCount].mIndex = 0;
				game->mDrawLists[2].mCount++;
			}
			game->mDrawLists[3].mRefs[game->mDrawLists[3].mCount].mObj   = obj;
			game->mDrawLists[3].mRefs[game->mDrawLists[3].mCount].mIndex = 0;
			game->mDrawLists[3].mCount++;
		}
	}
	for (i = 0; i < game->mXObjs.mObjectCount; i++) {
		obj = (XObj*)game->mXObjs.mObjects[i];
		if (obj->GetPool() != 3 && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000) && (obj->mStatus.mFlags & 0x40)) {
			if (!(obj->mStatus.mFlags & 0x1000000000000ULL)) {
				game->mDrawLists[2].mRefs[game->mDrawLists[2].mCount].mObj   = obj;
				game->mDrawLists[2].mRefs[game->mDrawLists[2].mCount].mIndex = 0;
				game->mDrawLists[2].mCount++;
			}
			game->mDrawLists[3].mRefs[game->mDrawLists[3].mCount].mObj   = obj;
			game->mDrawLists[3].mRefs[game->mDrawLists[3].mCount].mIndex = 0;
			game->mDrawLists[3].mCount++;
		}
	}
}

// a select() is up in the first message window
static inline bool IsMessageSelecting(CMsg* msg)
{
	bool selecting = false;
	if (msg->mData.mCursor != NULL && msg->mFont.mSelectedLine >= 0) {
		selecting = true;
	}
	return selecting;
}

static inline BOOL IsMenuOpen(CMenu* menu)
{
	return menu != NULL ? menu->IsOpen() : FALSE;
}

// chibi robo stands still enough for the timer's script: no action running, or only a lift that
// is done or holds one of a few poses
static inline bool IsPlayerIdle(WiiPlayerState* player)
{
	s32 lift  = player->mLift.mState;
	bool free = true;
	if (!(lift == 0 || lift == 4 || lift == 0x1D || lift == 0x21 || lift == 0x23)) {
		bool posing = false;
		if (lift == 2 && ((XObj*)player)->GetAnimId() == 0x159) {
			posing = true;
		}
		if (!posing) {
			free = false;
		}
	}
	return free && player->mHang.mState == 0 && player->mJump.mState == 0 && player->mLadder.mState == 0 && player->mRope.mState == 0
	    && player->mPush.mState == 0 && player->mTumble.mState == 0;
}

// what the pointer is on and within the popper's reach, the HUD shows it
struct PointerPick {
	f32 mDist;
	XObj* mObj;
};

// what the scope picks: the object nearest to the eye camera among those the pointer is on
struct ScopePick {
	f32 mDist;
	XObj* mObj;
	f32 mSpeed; // how fast the pointer moved
	BOOL mOn;   // the pointer is on it, not only swept across it
	s32 mLevel; // 1 to 3 by the speed
};

// the pointer as Update hands it to the objects
struct PointerMotion {
	const Vec* mPos;
	Vec mMove;  // its move on the screen this frame, in screen widths and heights
	f32 mSpeed; // the move's length in screen widths
};

// the sounds a stage's items play under the scope, mCount for each of the three levels
struct ScopeStageSounds {
	const s32* mSounds;
	s32 mCount;
};
extern const ScopeStageSounds lbl_802C49C8[];

extern s32 gScreenLeft, gScreenRight, gScreenTop, gScreenBottom;
extern s32 gPointerScreenWidth, gPointerScreenHeight;
// gamesound.cpp's, the track playing an archive sound
CSoundHandle* FindSound(CSound* sound, u32 id);

// the scope takes obj if nothing keeps it from looking and it is nearer to the eye than its pick so far.
// Retail has these lines twice, the name is ours
static inline void PickScopeTarget(CGame* game, ScopePick* scope, XObj* obj, XObj* held, const Vec* eye, const PointerMotion& motion,
                                   BOOL on)
{
	if (!game->GetStatusFlags(0x100) && game->GetPlayer()->GetAttachment() != 2
	    && !(game->mMotionControlCamera.IsAction(CAMACTION_AREA) && ((CCameraActionArea*)game->mCameraActions.mTable[CAMACTION_AREA])->_2D0
	         && ((CCameraActionArea*)game->mCameraActions.mTable[CAMACTION_AREA])->mControlEnabled != 0)
	    && !(game->mMotionControlCamera.IsAction(CAMACTION_MAP) && ((CCameraActionMap*)game->mCameraActions.mTable[CAMACTION_MAP])->_11F8)
	    && (obj->_18C >= 0 || (obj->GetPool() == 8 && obj != held))) {
		f32 dist = PSVECDistance(obj->GetPos(), eye) - obj->mFloorXZExtent[0];
		if (scope->mObj == NULL || dist < scope->mDist) {
			scope->mDist  = dist;
			scope->mObj   = obj;
			scope->mSpeed = motion.mSpeed;
			scope->mOn    = on;
		}
	}
}

// the eye camera's popper aims along the ray through the pointer. Retail has these lines twice, the name
// is ours
static inline void AimAtPointer(CGame* game)
{
	CCamera* camera = game->GetCamera();
	Vec pointer     = gWiiPointer.mPointer;
	CVec screen(pointer);
	CVec world;
	Vec clip;
	clip.x = screen.x / camera->mScale;
	clip.y = screen.y;
	clip.z = screen.z;
	PSMTX44MultVec(camera->mScreenInv, &clip, &world);
	CVec from = world;
	CVec to   = world + (world - camera->mView.mEye);
	f32 yaw   = atan2(to.x - from.x, to.z - from.z);
	f32 dz    = to.z - from.z;
	f32 dx    = to.x - from.x;
	f32 dy    = to.y - from.y;
	f32 pitch = atan2(FSqrt(dx * dx + dz * dz), FSqrt(dy * dy));
	fn_800E8140(game->GetPlayer(), yaw, dy < 0.0f ? HALF_PI - pitch : pitch - HALF_PI);
}

// two sets of items the scope marks, each a bit mask over a run of item indices; the names are ours
static inline bool IsScopeRareItemA(s32 item)
{
	bool rare = false;
	if ((u32)(item - 0x12) <= 0x22 && (0x780000003ULL & (1ULL << (item - 0x12)))) {
		rare = true;
	}
	return rare;
}

static inline bool IsScopeRareItemB(s32 item)
{
	bool rare = false;
	if ((u32)(item - 0x5B) <= 0x2F && (0xF00400000001ULL & (1ULL << (item - 0x5B)))) {
		rare = true;
	}
	return rare;
}

static inline bool IsScopeRareItem(s32 item)
{
	return IsScopeRareItemA(item) || IsScopeRareItemB(item);
}

/**
 * @note Address: 0x8001E830
 * @note Size: 0x57C4
 */
// NONMATCHING: the source calls PauseGameSounds, ResumeGameSounds and IsSystemMessage3Shown where retail
// expands them (they are defined in the next file's range), and gamesound.cpp's FindSound, which retail
// expands in the scope's night check; it calls vf16C without the frame count retail passes; the three
// nw4r camera reads of the stage camera object (the GameCube had none) stand as their CCamera fallback
// only; isnan is fdlibm's function where retail calls the Wii MSL's __fpclassifyf. The rest is
// register and stack placement that follows from those
// the GameCube function with the Wii's pointer, home menu, movie and sound pauses
void CGame::Update(u32 frames)
{
	s32 i;
	gWiiPointer.Update(frames);
	if (!mMovieOnThread && !(GetStatusFlags(1) && IsSystemMessage3Shown())) {
		if (mMovie.mState != 0 && !mHomeMenu.IsOpen() && !mResetRequested && !GetStatusFlags(0x200000000000002ULL)) {
			s32 result = mMovie.Update();
			if (result < 0) {
				mMovie.Stop();
				OSPanic("game.cpp", 0x1697, FormatToStaticBuffer("error on play movie! - %d\n", result));
			}
		}
	}
	UpdateSystemMessage();
	UpdateDvdStatus();
	if (!mResetRequested) {
		BOOL pressed = OSGetResetButtonState();
		if (gPowerButtonPressed) {
			RequestReset(2);
		} else if (pressed) {
			RequestReset(3);
		}
	}

	if (gWiiPointer.mPointerValid) {
		mPointer.mScopeTimer = 0.0f;
	} else if (mPointer._120 >= 0.0f) {
		if (mPointer._120 < 1.0f) {
			if ((mPointer._120 += 0.1f * frames) > 1.0f) {
				mPointer._120 = 1.0f;
			}
		}
	} else if (mPointer.mScopeTimer < 5.0f) {
		if ((mPointer.mScopeTimer += 2.0f * (1.0f / gRefreshRate) * frames) > 5.0f) {
			mPointer.mScopeTimer = 5.0f;
		}
	}

	// _EC1A8 fades the pointer's cursor: out while the eye camera aims the radar, half way while nothing
	// else wants the pointer, in while something does
	if (mMotionControlCamera.IsAction(CAMACTION_EYE) && GetPlayer()->GetAttachment() == 2 && !mMenu.mActive && !GetStatusFlags(0x800)) {
		if (mPointer.mAlpha > 0.0f) {
			if ((mPointer.mAlpha -= 0.05f * frames) < 0.0f) {
				mPointer.mAlpha = 0.0f;
			}
		}
	} else if (mEvt.mData.mScript == NULL && !IsMessageSelecting(GetMsg(0)) && !GetStatusFlags(0x800) && !IsMenuOpen(mMenu.mMenus[1])
	           && !IsMenuOpen(mMenu.mMenus[2]) && !IsMenuOpen(mMenu.mMenus[4])
	           && !(mMotionControlCamera.IsAction(CAMACTION_AREA) && ((CCameraActionArea*)mCameraActions.mTable[CAMACTION_AREA])->_2D0
	                && ((CCameraActionArea*)mCameraActions.mTable[CAMACTION_AREA])->mControlEnabled != 0)
	           && !(mMotionControlCamera.IsAction(CAMACTION_MAP) && ((CCameraActionMap*)mCameraActions.mTable[CAMACTION_MAP])->_11F8)
	           && !(fn_80169658(&mMotionControlCamera) && ((u8*)fn_80169674())[0x74]) && !(mStage == 3 && CallStageModule(0x10, 0, NULL))) {
		if (mPointer.mAlpha > 0.5f) {
			if ((mPointer.mAlpha -= 0.1f * frames) < 0.5f) {
				mPointer.mAlpha = 0.5f;
			}
		}
	} else if (mPointer.mAlpha < 1.0f) {
		if ((mPointer.mAlpha += 0.1f * frames) > 1.0f) {
			mPointer.mAlpha = 1.0f;
		}
	}

	// _EC2B4 counts down how long the pointer may stay off the screen before its cursor hides (flag 8 of
	// _41B0); NAN while nothing on screen wants the pointer
	bool wanted = false;
	if (GetStatusFlags(1)) {
		if ((!(mMenuControls & 2) && GetStatusFlags(0x400) && mEvt.mData.mScript == NULL) || (!(mMenuControls & 2) && GetStatusFlags(0x800))
		    || (!(mMenuControls & 2) && (IsMenuOpen(mMenu.mMenus[1]) || IsMenuOpen(mMenu.mMenus[2]) || IsMenuOpen(mMenu.mMenus[4])))
		    || (mMotionControlCamera.IsAction(CAMACTION_EYE) && mEvt.mData.mScript == NULL && (!(mMenuControls & 2) || !mMenu.mActive))
		    || mMotionControlCamera.IsAction(CAMACTION_SCROLL2)
		    || (mMotionControlCamera.IsAction(CAMACTION_AREA) && ((CCameraActionArea*)mCameraActions.mTable[CAMACTION_AREA])->_2D0
		        && ((CCameraActionArea*)mCameraActions.mTable[CAMACTION_AREA])->mControlEnabled != 0)
		    || (mMotionControlCamera.IsAction(CAMACTION_MAP) && ((CCameraActionMap*)mCameraActions.mTable[CAMACTION_MAP])->_11F8)
		    || (fn_80169658(&mMotionControlCamera) && ((u8*)fn_80169674())[0x74]) || (mStage == 3 && CallStageModule(0x10, 0, NULL))) {
			wanted = true;
		}
	}
	if (wanted) {
		if (gWiiPads[0].mSamples[0].pointerValid >= 2) {
			if (_41B0 & 8) {
				_41B0 &= ~8;
			}
			if (isnan(mPointer._10C) || mPointer._10C < gRefreshRate * 5) {
				mPointer._10C = gRefreshRate * 5;
			}
		} else if (isnan(mPointer._10C)) {
			mPointer._10C = 0.0f;
		}
		if (!(_41B0 & 8)) {
			if (mPointer._10C > 0.0f) {
				if ((mPointer._10C -= frames) < 0.0f) {
					mPointer._10C = 0.0f;
				}
			}
			if (0.0f == mPointer._10C) {
				_41B0 |= 8;
			}
		}
	} else {
		if (_41B0 & 8) {
			_41B0 &= ~8;
		}
		if (!isnan(mPointer._10C)) {
			mPointer._10C = NAN;
		}
	}

	UpdateWiiSaveJob(&mSaveManager);
	fn_80024CC0(this);
	if (IsDiscError()) { // flag 8: the sounds pause while the disc can't be read
		if (!GetStatusFlags(8)) {
			SetStatusFlags(8);
			PauseGameSounds(this, 0.0f);
			UpdateXSound(&gGame.mSound, frames);
		}
		if (gGame.mGs._1BA8) {
			WiiGsSceneObj* scene = gGame.mGs.mSceneHead;
			scene->vf034(scene->mPending, mXObjs.mPools[3].Get(0)->mObject.m_pScnMdl);
			StepSceneDraws(frames);
		}
		UpdateXSound(&gGame.mSound, frames);
		return;
	}
	if (GetStatusFlags(8)) {
		ClearStatusFlags(8);
		ResumeGameSounds(this, 0.0f);
	}
	if (GetStatusFlags(1) || (mTitle.mState >= 11 && mTitle.mState < 23)) {
		if (IsSystemMessage3Shown()) { // the message that the remote is gone stops the game
			if (!GetStatusFlags(0x800000000000000ULL)) {
				SetStatusFlags(0x800000000000000ULL);
				PauseGameSounds(this, 0.0f);
				UpdateXSound(&gGame.mSound, frames);
			}
			if (gGame.mGs._1BA8) {
				WiiGsSceneObj* scene = gGame.mGs.mSceneHead;
				scene->vf034(scene->mPending, mXObjs.mPools[3].Get(0)->mObject.m_pScnMdl);
				StepSceneDraws(frames);
			}
			UpdateWiiHomeMenu(&mHomeMenu, frames);
			UpdateXSound(&gGame.mSound, frames);
			return;
		}
		if (GetStatusFlags(0x800000000000000ULL)) {
			ClearStatusFlags(0x800000000000000ULL);
			ResumeGameSounds(this, 0.0f);
		}
	}

	if (GetStatusFlags(0x1000)) { // flag 0x1000 queues a draw-list rebuild
		ClearStatusFlags(0x1000);
		BuildObjectDrawList(this);
	}
	if (GetStatusFlags(1) && !GetStatusFlags(0x200000000000002ULL)) {
		if (!GetStatusFlags(0x20) && !mMenu.mActive && mEvt.mData.mScript == NULL) {
			if (gWiiPointer.mFlags & 0x400) {
				fn_80084B90(&mMenu.mActive, 1);
			} else if (gWiiPointer.mFlags & 0x80) {
				fn_80084B90(&mMenu.mActive, 1);
			}
		}
		GetStatusFlags(0x20);
	}
	if (!mHomeMenu.IsOpen()) {
		for (i = 0; i < 4; i++) {
			if (gWiiPads[i].mTrigger & 0x800000400ULL) { // HOME on the remote or the classic controller
				OpenWiiHomeMenu(&mHomeMenu);
				break;
			}
		}
	}

	for (i = 0; i < 0x400; i++) {
		mDrawLists[1].mRefs[i].mObj = NULL;
	}
	mDrawLists[1].mCount = 0;
	if (!IsLoadStepRunning()) {
		if (GetStatusFlags(1) && !GetStatusFlags(0x200000000000002ULL) && !mMenu.mActive) {
			for (i = 0; i < mDrawLists[0].mCount; i++) {
				XObj* obj = (XObj*)mDrawLists[0].mRefs[i].mObj;
				if ((obj->mStatus.mFlags & 1) && obj->_188 && obj->mBaseMtxPtr == NULL) {
					obj->vf15C();
					obj->UpdateObject(frames);
					obj->vf160();
				}
			}
		} else if (GetStatusFlags(1) && GetStatusFlags(2)) { // the game paused: his looping sounds stop
			if (((XObj*)GetPlayer())->GetAnimId() == 0x12 || ((XObj*)GetPlayer())->GetAnimId() == 0xCFA
			    || ((XObj*)GetPlayer())->GetAnimId() == 0xCFD) {
				if (IsWiiSfxPlaying(&gGame.mSound, 0x79)) {
					StopWiiSfx(&gGame.mSound, 0x79, 0.0f);
				}
			}
			if (IsWiiSfxPlaying(&gGame.mSound, 0x11D)) {
				StopWiiSfx(&gGame.mSound, 0x11D, 0.0f);
			}
			if (IsWiiSfxPlaying(&gGame.mSound, 0x75)) {
				StopWiiSfx(&gGame.mSound, 0x75, 0.0f);
			}
			if (IsWiiSfxPlaying(&gGame.mSound, 0x1F3)) {
				StopWiiSfx(&gGame.mSound, 0x1F3, 0.0f);
			}
			if (IsWiiSfxPlaying(&gGame.mSound, 0x1F8)) {
				StopWiiSfx(&gGame.mSound, 0x1F8, 0.0f);
			}
			for (i = 0; i < 2; i++) {
				CSoundHandle* handle = ((AnimSfx*)GetPlayer()->mSfx)->mActors[i].GetHandle();
				if (handle != NULL) {
					handle->Stop(gRefreshRate * 0.0f);
				}
			}
		}
	}
	if (IsRunning() && !GetStatusFlags(0x200000000000002ULL)) {
		mPlayTime += frames * (60.0f / gRefreshRate);
		if (mPlayTime >= 21600000.0f) { // caps the play time at a hundred hours of frames
			mPlayTime = 21599999.0f;
		}
	}
	if (GetStatusFlags(0x2000000000000ULL)) { // flag 0x2000000000000 arms the stage timer
		if (IsRunning() && mEvt.mData.mScript == NULL && !mMenu.mActive && !GetStatusFlags(0xC00) && mTimeState.mDuration > 0.0f) {
			if (!mTimeState.mRunning) {
				mTimeState.mTime = frames * (mTimeState.mDuration / (600.0f * gRefreshRate)) + mTimeState.mTime;
				if (mTimeState.mTime >= 1.0f) { // a full timer runs down
					mTimeState.mRunning = TRUE;
				}
			}
			if (mTimeState.mRunning == TRUE && IsPlayerIdle(GetPlayer())) { // then waits for an idle player
				mTimeState.mRunning = FALSE;
				u8* script          = GetStageTimerExpiredScript();
				if (script != NULL) {
					StartOrQueueEventScript(&mEvt, script, 0, -1, -1);
				}
			}
		}
	}
	if (!GetStatusFlags(0x200000000000002ULL)) {
		fn_80094C20(&mEvt, frames);
		for (i = 0; i < 2; i++) {
			CMsg_Update(&mMsgs[i], frames);
		}
	}
	if (IsRunning()) {
		CallStageModule(1, frames, NULL);
	}
	UpdateWiiHomeMenu(&mHomeMenu, frames);
	if (!GetStatusFlags(0x200000000000002ULL)) {
		UpdateScreenFade(&mFade, frames);
		mWipeCircle.Update(frames);
		mWipeTex.Update(frames);
		mWipeCustom.Update(frames);
		mWipeTile.Update(frames);
		mWipeCine.Update(frames);
		if (GetStatusFlags(1)) {
			mRoboeyes.Update(frames);
			UpdateScreenShake(&mQuake, frames);
			mOutline.Update(frames);
			mScrnTex.mAngle = fmod(DEG2RAD * 6.0f * frames + mScrnTex.mAngle, TWO_PI); // six degrees a frame
			mOutline.mThick = 0.0f;
			u8 lineAlpha    = 100.0f * (f32)sin(mScrnTex.mAngle) + 150.0f; // pulses the outline alpha

			mOutline.SetLineColor(0xFF, 0xFF, 0xFF);
			mOutline.mColor0.a = lineAlpha;
			UpdateScreenFade(&mScrnTex.mColor, frames);
			mOutline.SetColor2(mScrnTex.mColor.mColor.r, mScrnTex.mColor.mColor.g, mScrnTex.mColor.mColor.b);
			UpdateScreenFade(&mColor0.mColor, frames);
			mOutline.SetColor3(mColor0.mColor.mColor.r, mColor0.mColor.mColor.g, mColor0.mColor.mColor.b, mColor0.mColor.mColor.a);
			mBlur.Update(frames);
		}
	} else if (mMotionControlCamera.IsAction(CAMACTION_SCROLL2)) {
		UpdateScreenFade(&mFade, frames);
		mWipeCircle.Update(frames);
		mWipeTex.Update(frames);
		mWipeCustom.Update(frames);
		mWipeTile.Update(frames);
		mWipeCine.Update(frames);
	}
	UpdateScreenFade(&mColor1.mColor, frames);
	if (mOutlineWidthControl > 0) { // the c-stick sets the outline width
		if (gWiiPads[0].mSubStick.y > 0.0f) {
			if ((mOutlineWidth -= 0.01f * frames) < 0.04f) {
				mOutlineWidth = 0.04f;
			}
		} else if (gWiiPads[0].mSubStick.y < 0.0f) {
			if ((mOutlineWidth += 0.01f * frames) > 0.5f) {
				mOutlineWidth = 0.5f;
			}
		}
	}
	if (!mHomeMenu.IsOpen()) {
		UpdateBootLogos(&mLogo, frames);
		UpdateTitleScreen(&mTitle, frames);
		if (!GetStatusFlags(0x200000000000000ULL)) {
			fn_8008492C(&mMenu.mActive, frames);
		}
	}
	if (GetStatusFlags(1) && !GetStatusFlags(0x200000000000002ULL)) {
		UpdateHudTransitionsAndIndicators(&mInfoScrn, frames);
		((CFbTexEff*)&mSubwin)->Update(frames);
		for (i = 0; i < 5; i++) {
			FbMirror* mirror = &mMirrors[i];
			bool off         = true;
			if (mirror->mJoint != NULL && !(mirror->mJoint->mStatus & 0x1000000000000ULL)) {
				off = false;
			}
			if (!off) {
				mirror->Update(frames);
			}
		}
	}
	if (!GetStatusFlags(0x200000000000002ULL)) {
		UpdateStageNameBubbles(this, frames);
	}

	// the 3D sound listens from a little in front of the camera, or of the point mListener.mPoint names
	if (GetStatusFlags(1) && !GetStatusFlags(0x200000000000002ULL) && !mMenu.mActive) {
		CCamera* camera = GetCamera();
		if (!isnan(mListener.mPoint.x)) {
			CVec dir = camera->mView.mEye - mListener.mPoint;
			PSVECNormalize(&dir, &dir);
			mListener.mPos = mListener.mPoint + dir * 50.0f;
		} else {
			CVec dir = camera->mView.mInterest - camera->mView.mEye;
			PSVECNormalize(&dir, &dir);
			mListener.mPos = camera->mView.mEye + dir * 50.0f;
		}
		((CSound3D*)&gGame.mSound.mSound3DManager)->SetListener(GetCamera()->mView.mAngle.y, &mListener.mPos);
	}
	UpdateXSound(&gGame.mSound, frames);
	for (i = 0; i < mDrawLists[1].mCount; i++) {
		XObj* obj = (XObj*)mDrawLists[1].mRefs[i].mObj;
		if (obj->mStatus.mFlags & 1) {
			gGame.mGs.mSceneHead->vf034(gGame.mGs.mSceneHead->mPending, obj->mObject.m_pScnMdl);
		}
	}
	if (IsLoadStepRunning() && mLoadStep > 20 && (((XObj*)mPointer.mCursor)->mStatus.mFlags & 1)) { // the loading screen's robot
		XObj* robot = (XObj*)mPointer.mCursor;
		if (robot->GetAnimId() != 1 && robot->IsAnimDone()) {
			robot->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		robot->SetPos(gWiiPointer.mPointer.x, gWiiPointer.mPointer.y, 50.0f);
		robot->UpdateObject(frames);
		gGame.mGs.mSceneHead->vf034(gGame.mGs.mSceneHead->mPending, robot->mObject.m_pScnMdl);
	}
	StepSceneDraws(frames);

	s32 count = mDrawLists[1].mCount;
	if (!IsLoadStepRunning() && GetStatusFlags(1) && !GetStatusFlags(0x200000000000002ULL) && !mMenu.mActive) {
		for (i = 0; i < mDrawLists[0].mCount; i++) {
			XObj* obj = (XObj*)mDrawLists[0].mRefs[i].mObj;
			if (obj->mStatus.mFlags & 1) {
				if (obj->_188 && obj->mBaseMtxPtr == NULL) {
					obj->vf16C();
				} else {
					obj->vf15C();
					obj->UpdateObject(frames);
					obj->vf160();
				}
			}
		}
		fn_800D1A8C(GetPlayer(), frames);
		fn_800D1AF8(GetPlayer(), frames);
	}
	for (i = count; i < mDrawLists[1].mCount; i++) { // the ones the objects listed while they ran
		XObj* obj = (XObj*)mDrawLists[1].mRefs[i].mObj;
		if ((obj->mStatus.mFlags & 1) && !(obj->_188 && obj->mBaseMtxPtr == NULL)) {
			gGame.mGs.mSceneHead->vf034(gGame.mGs.mSceneHead->mPending, obj->mObject.m_pScnMdl);
		}
	}
	StepSceneDraws(frames);
	if (GetStatusFlags(1) && !GetStatusFlags(0x200000000000002ULL) && !mMenu.mActive) {
		fn_800D1BF0(GetPlayer(), frames);
	}
	StepSceneDraws(frames);
	if (GetStatusFlags(1) && !GetStatusFlags(0x200000000000002ULL) && !mMenu.mActive) {
		fn_800D1B64(GetPlayer(), frames);
	}
	StepSceneDraws(frames);
	if (!GetStatusFlags(0x200000000000002ULL) && !mMenu.mActive) {
		fn_800A13D0(&lbl_8044EE4C, GetCamera());
		fn_800A13D0(mEvtPtcl.mEmitters, GetCamera());
	}

	// the effects follow the camera: the stage camera object's when its scene brings one, else the game's
	if (IsRunning() || (GetStatusFlags(0x20) && !GetStatusFlags(0x200000000000002ULL))) {
		CCamera camera = *GetCamera();
		Vec eye        = camera.mView.mEye;
		for (u32 j = 0; j < 6; j++) {
			if (j != 4) {
				mEffects.Update(&eye, camera.mViewMtx, camera.mProjection, frames, j, camera.mNear, camera.mFar);
			}
		}
		CCamera screen = *GetCamera();
		eye            = screen.mView.mEye;
		mEffects.Update(&eye, screen.mViewMtx, screen.mProjection, frames, 4, screen.mNear, screen.mFar);
	} else if (mResetRequested) {
		mEffects.UpdateLoad(0);
		mEffects.UpdateLoad(1);
	}

	if (GetStatusFlags(1) && !GetStatusFlags(0x200000000000000ULL) && !mMenu.mActive && !mHomeMenu.IsOpen()) {
		mMotionControlCamera.Update(frames);
	}
	if (!mHomeMenu.IsOpen()) {
		XObj* camera = mXObjs.mPools[0].Get(0);
		if (camera != NULL && (camera->mStatus.mFlags & 1) && ((XObjScnCam*)camera)->mScene != NULL
		    && ((XObjScnCam*)camera)->mScene->_2888 != NULL) {
			fn_800C7764(camera, GetCamera());
		}
	}
	if (BeginSceneCamera()) {
		vf54();
	}
	for (i = 0; i < mDrawLists[1].mCount; i++) {
		XObj* obj = (XObj*)mDrawLists[1].mRefs[i].mObj;
		if (obj->mStatus.mFlags & 1) {
			fn_800B4988(obj);
		}
	}

	// the pointer picks what it is on: the nearest such object within the popper's reach for the HUD,
	// and the nearest to the eye camera for the scope's sounds
	PointerPick pick = { NAN, NULL };
	ScopePick scope  = { NAN, NULL, 0.0f, FALSE, 0 };
	if (gWiiPointer.mPointerValid && GetStatusFlags(1) && (GetPlayer()->mPlayerFlags & 0x20000000) && !GetPlayer()->mMenu.IsOpen()
	    && mEvt.mData.mScript == NULL && !IsLoadStepRunning()) {
		// in tenths: popper parameter 0 is the range, 1 the range with the upgrade
		f32 range = (s32)GetPopperParameter(FindOwnedInventoryItemIndex(GetPlayer()->mParams.mInventory.mItems, 0x36) >= 0) / 10.0f;
		const Vec* pointer   = &gWiiPointer.mPointer;
		PointerMotion motion = { pointer };
		if (gWiiPads[0].IsRemoteActive()) {
			motion.mMove.x = gWiiPads[0].mSamples[0].pointer.velocity.x;
			motion.mMove.y = gWiiPads[0].mSamples[0].pointer.velocity.y;
			motion.mSpeed  = gWiiPads[0].mSamples[0].pointer.speed;
			if (motion.mPos->x >= gScreenLeft && motion.mPos->x <= gScreenRight && motion.mPos->y >= gScreenTop
			    && motion.mPos->y <= gScreenBottom) {
				Vec zero = { 0.0f, 0.0f, 0.0f };
				CVec center(zero);
				CVec from(lbl_80444E98);
				Vec toPos = { motion.mPos->x, motion.mPos->y, 0.0f };
				CVec to(toPos);
				CVec eye = GetCamera()->mView.mEye;
				CVec muzzle(fn_8010BEE0(&GetPlayer()->mPopper));
				XObj* held = GetPlayer()->mLift.mState == 0 ? NULL : GetPlayer()->mLift.mTarget;
				for (i = 0; i < mDrawLists[1].mCount; i++) {
					XObj* obj = (XObj*)mDrawLists[1].mRefs[i].mObj;
					if (obj == NULL) {
						continue;
					}
					if ((obj->mStatus.mFlags & 0x10001) != 0x10001 || !(obj->mStatus.mFlags & 0x100)
					    || (obj->mStatus.mFlags & 0x1000000000800ULL)) {
						continue;
					}
					bool on = false;
					f32 y   = motion.mPos->y;
					f32 x   = motion.mPos->x;
					if (obj->mScreenBoundsValid && !isnan(obj->mScreenLeft) && !isnan(obj->mScreenTop) && !isnan(obj->mScreenRight)
					    && !isnan(obj->mScreenBottom) && x >= obj->mScreenLeft && x <= obj->mScreenRight && y >= obj->mScreenTop
					    && y <= obj->mScreenBottom) {
						on = true;
					}
					if (on) {
						obj->vf208(motion.mPos, &motion.mMove, TRUE, motion.mSpeed);
						if (GetPlayer()->GetAttachment() == 2 && GetPlayer()->mHang.mState == 0 && GetPlayer()->mJump.mState == 0
						    && GetPlayer()->mLadder.mState == 0 && GetPlayer()->mRope.mState == 0 && GetPlayer()->mPush.mState == 0
						    && GetPlayer()->mTumble.mState == 0 && !(obj->mStatus.mFlags & 0x40000) && obj->_18C != 1 && obj->_18C != 3
						    && obj->_18C != 4 && obj->_18C != 10 && obj->_18C != 0x15 && obj->GetKind() != 0x324) {
							f32 dist = PSVECDistance(&muzzle, &obj->mToppleOffset) - obj->mFloorXZExtent[0];
							// a 0x46F before a 0x6B wins regardless of the order, a 0x6B never displaces a 0x46F
							if (obj->GetKind() == 0x46F && pick.mObj != NULL && pick.mObj->GetKind() == 0x6B) {
								if (dist < range) {
									pick.mDist = dist;
									pick.mObj  = obj;
								}
							} else if (!(obj->GetKind() == 0x6B && pick.mObj != NULL && pick.mObj->GetKind() == 0x46F)) {
								if ((pick.mObj == NULL || dist < pick.mDist) && dist < range) {
									pick.mDist = dist;
									pick.mObj  = obj;
								}
							}
						}
						PickScopeTarget(this, &scope, obj, held, &eye, motion, TRUE);
					} else {
						// off it: the pointer may still have crossed its rectangle since the last frame
						bool crossed = false;
						if (obj->mScreenBoundsValid) {
							f32 left   = obj->mScreenLeft;
							f32 right  = obj->mScreenRight;
							f32 top    = obj->mScreenTop;
							f32 bottom = obj->mScreenBottom;
							if (!isnan(left) && !isnan(top) && !isnan(right) && !isnan(bottom)) {
								f32 width  = right - left;
								f32 height = bottom - top;
								center.x   = width * 0.5f + left;
								center.y   = height * 0.5f + top;
								if ((center.x - from.x) * (center.x - to.x) < 0.0f) {
									Vec path, toCenter, normal, side, offset;
									PSVECSubtract(&from, &to, &path);
									PSVECSubtract(&from, &center, &toCenter);
									PSVECCrossProduct(&toCenter, &path, &normal);
									PSVECCrossProduct(&path, &normal, &side);
									PSVECNormalize(&side, &side);
									PSVECSubtract(&from, &center, &offset);
									if (PSVECDotProduct(&side, &offset) < ((height - width) * 0.5f + width) * 0.5f
									    && !(from.y > top && to.y > top) && !(from.y < bottom && to.y < bottom)) {
										crossed        = true;
										CVec step      = to - from;
										motion.mMove.x = step.x / gPointerScreenWidth;
										motion.mMove.y = step.y / gPointerScreenHeight;
										motion.mSpeed  = PSVECMag(&step) / gPointerScreenWidth;
									}
								}
							}
						}
						if (crossed) {
							obj->_6DC = TRUE;
							obj->vf208(motion.mPos, &motion.mMove, TRUE, motion.mSpeed);
							PickScopeTarget(this, &scope, obj, held, &eye, motion, FALSE);
						} else {
							obj->vf208(motion.mPos, &motion.mMove, FALSE, motion.mSpeed);
						}
					}
				}
			}
		}
		lbl_80444E98 = *motion.mPos;
		if (GetStatusFlags(0x100) && GetPlayer()->GetAttachment() == 2) {
			AimAtPointer(this);
		} else {
			fn_800E8140(GetPlayer(), GetPlayer()->mMoveRotate.y, 0.0f);
		}
	} else if (GetStatusFlags(0x100) && GetPlayer()->GetAttachment() == 2) {
		// the eye camera keeps the last aim while the pointer is briefly off the screen
		if (mPointer._10C > 0.0f) {
			AimAtPointer(this);
		} else {
			fn_800E8140(GetPlayer(), GetPlayer()->mMoveRotate.y, 0.0f);
		}
	}

	// the HUD's object fades in and its marker turns while the pointer stays on it
	if (pick.mObj != mPointer.mHudObject) {
		mPointer.mHudObject  = pick.mObj;
		mPointer.mHudObjectX = 0.0f;
	}
	if (mPointer.mHudObject != NULL) {
		if (mPointer.mHudObjectX < 1.0f) {
			if ((mPointer.mHudObjectX += 0.1f * frames) > 1.0f) {
				mPointer.mHudObjectX = 1.0f;
			}
		}
		mPointer.mHudObjectY += DEG2RAD * 3.0f * frames;
	} else if (mPointer.mHudObjectX > 0.0f) {
		if ((mPointer.mHudObjectX -= 0.1f * frames) < 0.0f) {
			mPointer.mHudObjectX = 0.0f;
		}
	}

	// a new scope target: its marker clicks, every tenth time a longer sound; an item plays one of its
	// stage's sounds for the speed the pointer came at, at night a random one of twenty
	if (scope.mObj != mPointer.mScopeTarget) {
		mPointer.mScopeTarget = scope.mObj;
		if (scope.mObj != NULL) {
			if (scope.mObj->_18C >= 0) {
				mPointer.mScopeMarker = scope.mObj->_18C;
				mPointer.mScopeItem   = -1;
			} else {
				s32 item                = scope.mObj->GetObjFlags();
				mPointer.mScopeItem     = item;
				mPointer.mScopeMarker   = -1;
				mPointer.mScopeRareItem = IsScopeRareItem(item);
				fn_80114564(mPointer.mScopeTarget);
			}
			if (scope.mSpeed < 0.007f) {
				scope.mLevel = 1;
			} else if (scope.mSpeed < 0.02f) {
				scope.mLevel = 2;
			} else {
				scope.mLevel = 3;
			}
			if (mPointer.mScopeZoom > scope.mLevel) {
				if (mPointer.mScopeZoom > 2.0f) {
					mPointer.mScopeLevel = 3;
				} else if (mPointer.mScopeZoom > 1.0f) {
					mPointer.mScopeLevel = 2;
				} else {
					mPointer.mScopeLevel = 1;
				}
			} else {
				mPointer.mScopeLevel = scope.mLevel;
				mPointer.mScopeSpeed = scope.mSpeed;
				mPointer.mScopeZoom  = scope.mLevel;
			}
			if (mPointer.mScopeMarker >= 0) {
				if (IsWiiSfxPlaying(&gGame.mSound, 0x262)) {
					StopWiiSfx(&gGame.mSound, 0x262, 0.1f);
				}
				if (++mPointer.mScopeTicks >= 10) {
					PlayWiiSoundEffect(&gGame.mSound, 0x263, 1.0f, 0.0f);
					mPointer.mScopeTicks = 0;
				} else {
					PlayWiiSoundEffect(&gGame.mSound, 0x262, 1.0f, 0.0f);
				}
			} else if (mPointer.mScopeItem >= 0) {
				if (mTimeState.mMode == 1 && mTimeState.mTime > 0.5f
				    && (gGame.mSound.mMusicFadeOutFrames <= 0 ? gGame.mSound.mCurrentMusicId : 0) == 0
				    && FindSound(&gGame.mSound, 0x10) == NULL) {
					if (mPointer.mScopeSound >= 0 && IsWiiSfxPlaying(&gGame.mSound, mPointer.mScopeSound)) {
						StopWiiSfx(&gGame.mSound, mPointer.mScopeSound, 0.1f);
					}
					s32 index = rand() % 20;
					if (index == mPointer.mScopeSound) {
						mPointer.mScopeSound = (index + 1) % 20;
					} else {
						mPointer.mScopeSound = index;
					}
					PlayWiiSoundEffect(&gGame.mSound, mPointer.mScopeSound + 0x264, 1.0f, 0.0f);
				} else if (lbl_802C49C8[mStage].mSounds != NULL) {
					if (mPointer.mScopeSound >= 0 && IsWiiSfxPlaying(&gGame.mSound, mPointer.mScopeSound)) {
						StopWiiSfx(&gGame.mSound, mPointer.mScopeSound, 0.2f);
					}
					const ScopeStageSounds& sounds = lbl_802C49C8[mStage];
					mPointer.mScopeSoundIndex      = (mPointer.mScopeSoundIndex + 1) % sounds.mCount;
					mPointer.mScopeSound           = sounds.mSounds[mPointer.mScopeSoundIndex + (mPointer.mScopeLevel - 1) * sounds.mCount];
					PlayWiiSoundEffect(&gGame.mSound, mPointer.mScopeSound, 1.0f, 0.0f);
				} else if (mStage == 11) {
					switch (mPointer.mScopeItem) {
					case 0:
						PlayWiiSoundEffect(&gGame.mSound, 0x431, 1.0f, 0.0f);
						break;
					case 0x7C:
						PlayWiiSoundEffect(&gGame.mSound, 0x432, 1.0f, 0.0f);
						break;
					}
				} else if (mStage == 18) {
					switch (mPointer.mScopeItem) {
					case 0xE:
						PlayWiiSoundEffect(&gGame.mSound, 0x455, 1.0f, 0.0f);
						break;
					case 0x6E:
						PlayWiiSoundEffect(&gGame.mSound, 0x456, 1.0f, 0.0f);
						break;
					}
				} else if (mStage == 22) {
					switch (mPointer.mScopeItem) {
					case 0:
						PlayWiiSoundEffect(&gGame.mSound, 0x47D, 1.0f, 0.0f);
						break;
					case 0x6D:
						PlayWiiSoundEffect(&gGame.mSound, 0x47E, 1.0f, 0.0f);
						break;
					}
				}
			}
		}
	}
	if (mPointer.mScopeTarget != NULL) {
		if (mPointer.mScopeFade < 1.0f) {
			mPointer.mScopeFade = 1.0f;
		}
		if (mPointer.mScopeFade2 < 1.0f) {
			mPointer.mScopeFade2 = 1.0f;
		}
	} else {
		if (mPointer.mScopeFade > 0.0f) {
			if ((mPointer.mScopeFade -= 0.05f * frames) <= 0.0f) {
				mPointer.mScopeFade = 0.0f;
				mPointer.mScopeZoom = 1.0f;
			}
		}
		if (mPointer.mScopeFade2 > 0.0f) {
			if ((mPointer.mScopeFade2 -= 0.02f * frames) <= 0.0f) {
				mPointer.mScopeFade2 = 0.0f;
			}
		}
	}
	if (mPointer.mScopeZoom > 1.0f) {
		if ((mPointer.mScopeZoom -= 0.01f * frames) < 1.0f) {
			mPointer.mScopeZoom = 1.0f;
		}
	}
	if (mPointer.mScopeZoom > 0.0f) {
		mPointer.mScopeAngle += frames * (DEG2RAD * 3.0f * (mPointer.mScopeZoom * mPointer.mScopeZoom));
	}
}

/**
 * @note Address: 0x80024A10
 * @note Size: 0x6C
 */
// NONMATCHING: the subtract is nw4r::math::VEC3Sub, the paired-single inline, as in retail; retail
// subtracts in a stack copy and copies that out, we build the result in the return slot
CVec CVec::operator-(Vec other) const
{
	CVec result = *this;
	nw4r::math::VEC3Sub((nw4r::math::VEC3*)&result, (nw4r::math::VEC3*)&result, (nw4r::math::VEC3*)&other);
	return result;
}
