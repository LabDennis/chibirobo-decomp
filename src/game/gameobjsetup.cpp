#include "game/newgamestate.h"
#include "game/arcache.h"
#include "game/globals.h"

#include "game/cgame.h"
#include "game/stagedata.h"
#include "game/xobjmap.h"
#include "game/mm.h"
#include "game/hsd_dobj.h"
#include "game/cgameutil.h"

extern "C" {
u32 GXGetTexBufferSize(u16 width, u16 height, u32 format, GXBool mipmap, u8 maxLod);
void BuildObjectDrawList(CGame* game);
void SelectSceneLights(CGame* game);
BOOL UpdateStageObjectLoading(XOBJS* objs);
void Reset2__16CCameraActionMapFv(CInterfaceCameraAction* action);
}

static inline HSD_JObj* GetChild(HSD_JObj* jobj)
{
	return jobj == NULL ? NULL : jobj->child;
}

// the last joint down the first children
static inline HSD_JObj* GetLeaf(HSD_JObj* jobj)
{
	while (GetChild(jobj) != NULL) {
		jobj = GetChild(jobj);
	}
	return jobj;
}

/**
 * @note Address: 0x80011638
 * @note Size: 0x5A0
 */
// NONMATCHING: the stage's camera mode and the second loop's registers rotate
// the step after the objects are read: points the camera and its actions at them, lists them
// and sets up a mirror for each object that has one, at most five
extern "C" BOOL FinalizeStageObjectSetup(CGame* game)
{
	FbMirrorEx* mirror;
	s32 mirrors;
	XObj* obj;
	s32 mode;
	s32 i;
	HSD_JObj* jobj;
	if (UpdateStageObjectLoading(&game->mXObjs)) {
		mode                                                           = GetStageInfo()->mCameraMode;
		((CCameraActionScroll*)gGame.mCameraActions.mTable[11])->mZoom = 1;
		if (GetStageCameraAreaRecords() && GetStageCameraInfoRecords()) {
			game->mCameraActions.mTable[1]->Reset();
		}
		if (GetStageCameraAreaRecords() && GetStageCameraInfoRecords()) {
			((CCameraActionAreaExM*)game->mCameraActions.mTable[1])->LoadAreaState();
		}
		((CCameraActionAnim*)game->mCameraActions.mTable[6])->SetObj((XObjCamera*)game->mXObjs.mPools[0].First());
		game->mMotionControlCamera.Init();
		if (mode == 1 || mode == 7) {
			game->mMotionControlCamera.ChangeAction(7);
			Reset2__16CCameraActionMapFv(game->mCameraActions.mTable[7]);
			game->mMotionControlCamera.ApplyAction();
		} else {
			game->mMotionControlCamera.ChangeAction(mode);
		}
		for (i = 0; i < game->mXObjs.mObjectCount; i++) {
			game->mMotionControlCamera.UpdateTarget(game->mXObjs.mObjects[i]);
		}

		if (game->mStatus.mFlags & 0x100) {
			game->mStatus.mFlags &= ~0x300;
			StartScreenWipe(&game->mRoboeyes, 0, 0, 0, 0, 1, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		}
		SelectSceneLights(game);
		BuildObjectDrawList(game);

		game->mStatus.mFlags &= ~0x2000000000;
		mirror = gFramebufferMirrors;
		game->mStatus.mFlags &= ~0x8000000000;
		game->mStatus.mFlags &= ~0x100000000000;
		mirrors = 0;
		for (i = 0; i < game->mXObjs.mObjectCount; i++) {
			obj = game->mXObjs.mObjects[i];
			if (obj->mStatus.mFlags & 1) {
				if (!(game->mStatus.mFlags & 0x2000000000) && (obj->mStatus.mFlags & 0x20000000)
				    && !(obj->mStatus.mFlags & 0x400000000000000)) {
					game->mStatus.mFlags |= 0x2000000000;
				}
				if ((obj->mStatus.mFlags & 0x400) && mirrors < 5) {
					game->mStatus.mFlags |= 0x100000000000;
					jobj = obj->mJObj;
					if (jobj != NULL) {
						mirror->mOwner           = obj;
						jobj                     = GetLeaf(jobj);
						mirror->mJoint           = jobj;
						mirror->mVisible         = FALSE;
						mirror->mTexture.mWidth  = 256;
						mirror->mTexture.mHeight = 256;
						mirror->mTexture.mFormat = GX_TF_RGB565;
						mirror->mTexture._0C     = 0;
						mirror->mTexture.mX      = 0.0f;
						mirror->mTexture.mY      = 0.0f;
						mirror->mTexture.mMipmap = 0;
						mirror->mTexture.mImage  = mm::Alloc(GXGetTexBufferSize(256, 256, GX_TF_RGB565, GX_FALSE, 0), "(mirror)");
						mirror->mActive          = TRUE;
						mirror->mTexture.mTex.Init(mirror->mTexture.mImage, mirror->mTexture.mWidth, mirror->mTexture.mHeight,
						                           mirror->mTexture.mFormat, GX_CLAMP, GX_CLAMP, GX_FALSE);
						for (HSD_DObj* dobj = HSD_JObjGetDObj(jobj); dobj != NULL; dobj = HSD_DObjGetNext(dobj)) {
							HSD_MObj* mobj = HSD_DObjGetMObj(dobj);
							if (mobj != NULL) {
								SetMaterialAmbientColor(mobj, 255, 255, 255);
								HSD_TObj* tobj = HSD_MObjGetTObj(mobj);
								if (tobj != NULL) {
									tobj->imagedesc = (HSD_ImageDesc*)mirror;
								}
							}
						}
						mirror++;
						mirrors++;
					}
				}
				if (!(game->mStatus.mFlags & 0x8000000000) && (obj->mStatus.mFlags & 0x80000000)) {
					game->mStatus.mFlags |= 0x8000000000;
				}
			}
		}

		if (game->mStatus.mFlags & 0x2000000000) {
			f32 fade  = GetStageInfo()->mMirrorFade / 100.0f;
			XObj* obj = game->mXObjs.mPools[3].First();
			if ((obj->mStatus.mFlags & 1) && fade > 0.0f && fade < 1.0f) {
				obj->SetTranslucent(fade);
			}
		}
		return TRUE;
	}
	return FALSE;
}

extern "C" {
BOOL UpdateProgressiveScanPrompt(u32);
BOOL LoadGameSoundBank(XSound*, s32);
BOOL LoadMenuGroupResources(MenuEx*);
BOOL LoadMemoryCardResources(Memcard*);
BOOL LoadEventResources(CEvt*);
BOOL LoadMessageCursorObject(CMsg*);
BOOL HideHudPointCounters(InfoScrn*);
BOOL LoadGlobalResourceData();
void SetAttachmentCountById(AttachmentValues*, s32, s32);
void RestoreGameSaveData(Memcard*);
void RestoreSavedPlayerAndCamera(Memcard*);
u32 CheckStageFlag(s32, u32);
BOOL LoadStageByIndex(s32);
void ApplyStageClearColor();
const s32* GetStageEventObjectPairs();
void RegisterEventObjectId(CEvt*, s16, s16);
extern ARPreCache gAramPrecache;
u32 TakeNextCounterValue(void*, u32*);
s32 HSD_DevComRequest(s32, u32, void*, u32, s32, s32, u32 (*)(void*, u32*), void*);
u32 ARGetSize();
}

static inline BOOL PreloadStageArchives()
{
	ARPreCache& cache = gAramPrecache;
	switch (cache._00) {
	case 0:
		if (cache.mFile0.ConvertPathToEntrynum("/qp/pc.idx") < 0 || cache.mFile1.ConvertPathToEntrynum("/qp/pc.bin") < 0)
			return TRUE;
		++cache._00;
	case 1: {
		if (!cache.mFile0.ReadEx("/qp/pc.idx", 0, 0))
			break;
		++cache._00;
		s32 entry   = DVDConvertPathToEntrynum("/qp/pc.bin");
		u32 address = gARAMCacheInfo.mBaseAddress;
		HSD_DevComRequest(entry, 0, (void*)address, ARGetSize() - gARAMCacheInfo.mBaseAddress, 0x23, 1, TakeNextCounterValue, &cache);
		break;
	}
	case 3: {
		XObjSource* src = GetObjectSources();
		const u8* pair  = (const u8*)cache.mFile0.mBuffer;
		if (pair) {
			for (s32 id = 0;; ++id, ++src) {
				if (*(const u32*)pair == 0xffffffff || src->mCache.address == 0xffffffff)
					break;
				if (src->mCache.size != 1)
					continue;
				u32 address = ReadResourceOffset(pair), size = ReadResourceOffset(pair + 4);
				if (address + size >= ARGetSize())
					break;
				src->mCache.address = address;
				src->mCache.size    = size;
				u32 padded          = (size + 31) & ~31u;

				gARAMCacheInfo.mEntries[gARAMCacheInfo.mCount].address  = gARAMCacheInfo.mBaseAddress;
				gARAMCacheInfo.mEntries[gARAMCacheInfo.mCount].size     = padded;
				gARAMCacheInfo.mEntries[gARAMCacheInfo.mCount].objectId = id;
				gARAMCacheInfo.mEntries[gARAMCacheInfo.mCount].name     = src->mName;
				++gARAMCacheInfo.mCount;
				gARAMCacheInfo.mBaseAddress += padded;
				pair += 8;
			}
		}
		cache.mFile0.Close();
		++cache._00;
	}
	case 4:
		return TRUE;
	}
	return FALSE;
}

static inline BOOL LoadStartingAttachments(CGame* game)
{
	if (!LoadGlobalResourceData())
		return FALSE;
	if (!gCmdLineArgs.mCopterLevelOverride) {
		u32 level = GetStartingAttachmentLevel(0);
		SetAttachmentCountById(&game->GetPlayer()->mAttachments, 1, level);
	}
	if (!gCmdLineArgs.mPopperLevelOverride) {
		u32 level = GetStartingAttachmentLevel(1);
		SetAttachmentCountById(&game->GetPlayer()->mAttachments, 2, level);
	}
	if (!gCmdLineArgs.mRadarLevelOverride) {
		u32 level = GetStartingAttachmentLevel(2);
		SetAttachmentCountById(&game->GetPlayer()->mAttachments, 3, level);
	}
	return TRUE;
}

BOOL CGame::BeforeUpdate()
{
	// Ready stages fall through. Pending reads resume at the same stage next frame.
	switch (_3338) {
	case 0:
		return TRUE;
	case 1:
		if (!UpdateProgressiveScanPrompt(GetRetraceStep()))
			break;
#if defined(VERSION_PAL)
		mOutline.SetSize(gGame.mGs.mScreen.fbWidth, gGame.mGs.mScreen.efbHeight);
		mBlur.SetSize(gGame.mGs.mScreen.fbWidth / 2, gGame.mGs.mScreen.efbHeight / 2);
#endif
		++_3338;
	case 2:
		++_3338;
	case 3:
		if (!LoadGameSoundBank(&mSound, -1))
			break;
		++_3338;
	case 4:
		if ((mStatus.mFlags & 0x200000000ULL) && !mArchive.ReadEx("/qp.bin", 0, 0))
			break;
		++_3338;
	case 5:
		if (!mTpl.ReadEx("tpl/misc.tpl", 0, 0))
			break;
		++_3338;
		mScrnTex.mTex        = mTpl.mTex.Init(&((TPLPalette*)mTpl.mBuffer)->descriptorArray[121 - StatusTextureOffset], 0);
		mOutline.mPatternTex = &mScrnTex.mTex;
		mOutline.mFrame      = 0;
		mOutline.mThick2     = 0;
	case 6:
		if ((mStatus.mFlags & 0x400000000ULL) && mLogo.mState != 15) {
			if (!mLogo.IsActive())
				mLogo.mState = 1;
			if (mLogo.mState <= 2)
				break;
		}
		++_3338;
	case 7:
		if ((mStatus.mFlags & 0x40000000000ULL) && !PreloadStageArchives())
			break;
		++_3338;
	case 8:
		if ((mStatus.mFlags & 0x400000000ULL) && mLogo.mState != 15)
			break;
		++_3338;
	case 9:
		if (!mParticle.Read("particle/chibi_robo"))
			break;
		++_3338;
	case 10:
		if (!LoadMenuGroupResources(&mMenu))
			break;
		++_3338;
	case 11:
		if (!LoadMemoryCardResources(&mMemcard.mCard))
			break;
		++_3338;
	case 12:
		++_3338;
	case 13:
		if (!LoadEventResources(&mEvt))
			break;
		++_3338;
	case 14:
		if (!LoadMessageCursorObject(mCMsgs))
			break;
		++_3338;
	case 15:
		if (!HideHudPointCounters(&mInfoScrn))
			break;
		++_3338;
	case 16:
		++_3338;
	case 17:
		if (!LoadStartingAttachments(this))
			break;
		++_3338;
	case 18:
		if (!GetPlayer()->LoadFromPlacement(CoordInfo(GetSpawnData(14))))
			break;
		++_3338;
	case 19:
		if ((mStatus.mFlags & 0x20000000) && mTitle.mState != 23) {
			if (!(mTitle.mState >= 1 && mTitle.mState < 23)) {
				ResetNewGameState();
				mTitle.mState = 1;
			}
			break;
		}
		++_3338;
	case 20:
		if (mMemcardLoaded) {
			RestoreGameSaveData(&mMemcard.mCard);
			mMemcardLoaded = FALSE;
			mStatus.mFlags |= 0x2000;
			InitBatteryGauge(&mInfoScrn, GetPlayer()->mParams.mBattery);
		}
		++_3338;
	case 21:
		++_3338;
	case 22:
		CheckStageFlag(gGame.mStage, 2);
		++_3338;
		break;
	case 23: {
		if (!LoadStageByIndex(mStage))
			break;
		ApplyStageClearColor();
		const s32* pair = GetStageEventObjectPairs();
		while (pair[0] >= 0) {
			RegisterEventObjectId(&mEvt, pair[1], pair[0]);
			pair += 2;
		}
		++_3338;
	}
	case 24: {
		u8* script = GetStageSetupScript();
		if (script)
			StartOrQueueEventScript(&mEvt, script, 0, -1, -1);
		++_3338;
	}
	case 25:
		if (mEvt.mData.mScript)
			break;
		if (CheckStageFlag(gGame.mStage, 4))
			mStatus.mFlags |= 0x40;
		++_3338;
	case 26:
		if (!FinalizeStageObjectSetup(this))
			break;
		mStatus.mFlags &= ~0x40ULL;
		if (mStatus.mFlags & 0x2000)
			RestoreSavedPlayerAndCamera(&mMemcard.mCard);
		++_3338;
	case 27:
		CallStageModule(0, 0, NULL);
		++_3338;
	case 28: {
		if (mFade.mSpeed.r != 0 || mFade.mSpeed.g != 0 || mFade.mSpeed.b != 0 || mFade.mSpeed.a != 0)
			break;
		StartScreenFade(&mFade, 0, 0, 0, 255, 0, 0, 0, 0, 0.5f);
		StartOrQueueEventScript(&mEvt, GetStageEntryScript(), 0, -1, -1);
		u32 minutes = GetStageEntryTimeComponent(1), hours = GetStageEntryTimeComponent(0);
		_FF204 = (s32)(hours * 216000 + (minutes * 3600 + GetStageEntryTimeComponent(2) * 60));
		mStatus.mFlags |= 1;
		++_3338;
	}
	}
	return !(_3338 > 0 && _3338 < 29);
}
