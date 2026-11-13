#include "game/cgame.h"
#include "types.h"
#include "Version.h"

#include "game/globals.h"
#include "game/stage.h"
#include "game/ut.h"
#include "Dolphin/GX/GXTypes.h"
#include <string.h>

extern "C" {
extern UnkStageObj gStageObjectPlacements[];
extern UnkObjData objTable[];
extern XObjSource gObjectSources[];
// HSD_VIInfo, its header needs the hsd include path
extern u8 HSD_VIData[];
BOOL LoadGameSoundBank(XSound* sound, s32 stage);
}

// Stage offsets are measured from the start of the file.
struct StageFileHeader {
	u8 environment[4];
	u8 placements[4];
	u8 scriptOffsets[4];
	u8 setupScript[4];
	u8 entryScript[4];
	u8 batteryDepletedScript[4];
	u8 zButtonScript[4];
	u8 timerExpiredScript[4];
	u8 unknown20[4];
	u8 eventObjects[4];
	u8 cameraAreas[4];
	u8 cameraInfo[4];
	u8 cameraAreasEx[4];
};
STATIC_ASSERT(sizeof(StageFileHeader) == 0x34);

Stage gStageResources;
Globals gGlobalScriptResources;

/**
 * @note Address: 0x8006EDB4
 * @note Size: 0x14
 */
extern "C" UnkStageObj* GetStageObjectPlacementByIndex(s32 index)
{
	return &gStageObjectPlacements[index];
}

extern "C" {
XObjSource gTestShortplayTitleSource   = { "test/shortplay_title" };
XObjSource gChargeFx01Source           = { "charge_fx01" };
XObjSource gCbPropellerLv2Source       = { "cb_propeller_lv2" };
XObjSource gCbCannonLv2Source          = { "cb_cannon_lv2" };
XObjSource gCbRadarSource              = { "cb_radar" };
XObjSource gMaruBatuSource             = { "maru_batu" };
XObjSource gExclamationSource          = { "exclamation" };
XObjSource gKisekaePartsKeathSource    = { "kisekae/parts_keath" };
XObjSource gKisekaePartsKaeruSource    = { "kisekae/parts_kaeru" };
XObjSource gKisekaePartsTaoSource10021 = { "kisekae/parts_tao" };
XObjSource gKisekaePartsKegaSource     = { "kisekae/parts_kega" };
XObjSource gKisekaePartsTaoSource10023 = { "kisekae/parts_tao" };
XObjSource gKisekaePartsObakeSource    = { "kisekae/parts_obake" };
XObjSource gKisekaePartsPajamaSource   = { "kisekae/parts_pajama" };
XObjSource gKisekaePartsSuperSource    = { "kisekae/parts_super" };
XObjSource gKisekaePartsPajama01Source = { "kisekae/parts_pajama_01" };
XObjSource gKisekaePartsPajama02Source = { "kisekae/parts_pajama_02" };
XObjSource gTitleIconIconBbSource      = { "title_icon/icon_bb" };
XObjSource gTitleIconIconArmySource    = { "title_icon/icon_army" };
XObjSource gTitleIconIconFafaSource    = { "title_icon/icon_fafa" };
XObjSource gTitleIconIconFunkySource   = { "title_icon/icon_funky" };
XObjSource gTitleIconIconHockSource    = { "title_icon/icon_hock" };
XObjSource gTitleIconIconKeathSource   = { "title_icon/icon_keath" };
XObjSource gTitleIconIconPeetsSource   = { "title_icon/icon_peets" };
XObjSource gTitleIconIconSunmidaSource = { "title_icon/icon_sunmida" };
XObjSource gTitleIconIconTrexSource    = { "title_icon/icon_trex" };
XObjSource gTitleIconIconZobinSource   = { "title_icon/icon_zobin" };
XObjSource gTitleIconIconArmy2Source   = { "title_icon/icon_army2" };
#if !defined(VERSION_JP)
XObjSource gUsMaruBatuSource  = { "us_maru_batu" };
XObjSource gUsMaruModelSource = { "us_maru_model" };
XObjSource gUsBatuModelSource = { "us_batu_model" };
#endif
XObjSource gTestWardrobeSource10029 = { "test/wardrobe" };
XObjSource gTestWardrobeSource10030 = { "test/wardrobe" };

// Extended object IDs start at 10000 and index this table in GetObjData.
UnkObjData objTableEx[] = {
	{ &gTitleIconIconBbSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconArmySource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconFafaSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconFunkySource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconHockSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconKeathSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconPeetsSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconSunmidaSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconTrexSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconZobinSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTitleIconIconArmy2Source, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTestShortplayTitleSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[15], 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gChargeFx01Source, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gCbPropellerLv2Source, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gCbCannonLv2Source, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gCbRadarSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gMaruBatuSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gExclamationSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsKeathSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsKaeruSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsTaoSource10021, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsKegaSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsTaoSource10023, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsObakeSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsPajamaSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsSuperSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsPajama01Source, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gKisekaePartsPajama02Source, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gTestWardrobeSource10029, 0, 0x401, 0, 0, 0, 22, 0 },
	{ &gTestWardrobeSource10030, 0, 0x401, 0, 0, 0, 22, 0 },
	{ &gObjectSources[312], 0, 0x1, 0, 0, 0, 20, 1 },
	{ &gObjectSources[313], 0, 0x1, 0, 0, 0, 20, 50 },
#if !defined(VERSION_JP)
	{ &gUsMaruBatuSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gUsMaruModelSource, 0, 0x1, 0, 0, 0, 22, 0 },
	{ &gUsBatuModelSource, 0, 0x1, 0, 0, 0, 22, 0 },
#endif
};

// Object IDs from 10000 reuse these global entries. A negative entry has no resource.
s32 gGlobalTextIdRemap[] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 19, 19, 312, 313,
#if !defined(VERSION_JP)
	-1, -1, -1,
#endif
};

UnkStageObj spawnTable[] = {
	// Title icons and demo.
	{ 10000, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10001, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10002, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10003, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10004, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10005, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10006, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10007, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10008, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10009, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
	{ 10010, { 0, 0, 0 }, { 0, 0, 0 }, { 150, 150, 150 }, 0, 0, 0, 0x3 },
#if defined(VERSION_PAL)
	{ 10011, { 0, 0, 0 }, { 0, 0, 0 }, { 104, 104, 104 }, 0, 0, 0, 0x3 },
#else
	{ 10011, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
#endif

	// Menu and message cursors.
	{ 10012, { 0, -1500, 0 }, { 0, 0, 0 }, { 600, 600, 600 }, 0, 0, 1, 0x3 },
	{ 10012, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 1, 0x3 },

	// Player and equipment.
	{ 0, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x2301 },
	{ 17, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x10003 },
	{ 10013, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10014, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10015, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10016, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10017, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10018, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },

	{ 105, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1000011 },
	{ 105, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x2000011 },
	{ 105, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1000001 },
	{ 105, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x2000001 },

	// Costume parts.
	{ 10019, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10020, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10021, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10022, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10023, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10024, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10025, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10026, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10027, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10028, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },

	{ 399, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 1, 0x3 },
	{ 444, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1000003 },
	{ 260, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x2000007 },
	{ 261, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x2000007 },
	{ 321, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x2000007 },
	{ 322, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x2000007 },
	{ 428, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1000003 },
	{ 435, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1000003 },
#if !defined(VERSION_JP)
	{ 10033, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10034, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1 },
	{ 10035, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1 },
#endif
};
}

/**
 * @note Address: 0x8006EDC8
 * @note Size: 0x14
 */
extern "C" UnkStageObj* GetSpawnData(s32 index)
{
	return &spawnTable[index];
}

/**
 * @note Address: 0x8006EDDC
 * @note Size: 0x3C
 */
extern "C" UnkObjData* GetObjData(const CoordInfo* info)
{
	s32 index = info->_00->_00;
	if (index >= 10000)
		return &objTableEx[index - 10000];
	return &objTable[index];
}

/**
 * @note Address: 0x8006EE18
 * @note Size: 0xC
 */
extern "C" XObjSource* GetObjectSources()
{
	return gObjectSources;
}

#if defined(VERSION_PAL)
#define STAGE(file, message, flags) { file, file, message, flags }
#else
#define STAGE(file, message, flags) { file, message, flags }
#endif

StageDesc stageTable[] = {
	STAGE("stage00", 0, 5),
	STAGE("stage01", 1, 5),
	STAGE("stage02", 2, 5),
	STAGE("stage03", 3, 5),
	STAGE("stage04", 4, 5),
	STAGE("stage05", 5, 4),
	STAGE("stage06", 6, 5),
	STAGE("stage07", 7, 5),
	STAGE("stage08", 8, 5),
	STAGE("stage09", 9, 5),
	STAGE("stage10", 10, 1),
	STAGE("stage11", 11, 5),
	STAGE("stage12", 12, 5),
	STAGE("stage13", 13, 5),
	STAGE("stage14", 14, 5),
	STAGE("stage15", 15, 5),
	STAGE("stage16", 16, 5),
	STAGE("stage17", 17, 5),
	STAGE("stage18", 18, 5),
	STAGE("stage19", 19, 5),
	STAGE("stage20", 20, 5),
	STAGE("stage21", 21, 5),
	STAGE("stage22", 22, 5),
	STAGE("stage23", 23, 5),
	STAGE("stage24", 24, 5),
	STAGE("stage25", 25, 5),
	STAGE("stage26", 26, 5),
	STAGE("stage27", 27, 5),
#if defined(VERSION_PAL)
	{ "stage28", "stage07", 28, 5 },
	{ "stage29", "stage07", 29, 5 },
#else
	STAGE("stage28", 28, 5),
	STAGE("stage29", 29, 5),
#endif
	STAGE(NULL, -1, 0),

	// FindStageIndexByName assigns these stages IDs starting at 100.
	STAGE("shun", -1, 5),
	STAGE("hori", -1, 5),
	STAGE("ahk", -1, 5),
	STAGE("junko", -1, 5),
	STAGE("sayoko", -1, 5),
	STAGE("mory", -1, 5),
	STAGE("ryosuke", -1, 5),
	STAGE("takanabe", -1, 5),
	STAGE("mariko", -1, 5),
	STAGE(NULL, 0, 0),
};

#undef STAGE

/**
 * @note Address: 0x8006EE24
 * @note Size: 0x514
 */
extern "C" BOOL AdvanceStageLoad(Stage* stage, s32 index)
{
	switch (stage->mState) {
	case 0: {
#if defined(VERSION_JP)
		strcpy(stage->mDataPath, FormatToStaticBuffer("bin/%s.bin", stageTable[index].fileName));
#else
		strcpy(stage->mDataPath,
		       FormatToStaticBuffer("bin%s/%s.bin", gResourceDirectorySuffixByLanguage[gGame.mLanguage], stageTable[index].fileName));
#endif
#if defined(VERSION_PAL)
		// europe's table has the module's name apart from the data file's
		StageDesc* desc = &stageTable[index];
		strcpy(stage->mModulePath, FormatToStaticBuffer("rel/%s%s.rel", desc->moduleName, ""));
#else
		strcpy(stage->mModulePath, FormatToStaticBuffer("rel/%s%s.rel", stageTable[index].fileName, ""));
#endif
		if (!(gGame.mStatus.mFlags & 0x100000000ULL))
			stage->mModulePath[0] = 0;
		++stage->mState;
	}
	// fall through, every step that's done moves on in the same frame
	case 1: {
		if (!stage->mDataFile.ReadEx(stage->mDataPath, 0, 0))
			break;
		stage->mDataFile.CloseHandle();
		u8* data                      = (u8*)stage->mDataFile.mBuffer;
		const StageFileHeader* header = (const StageFileHeader*)data;
		stage->mEnvironment.mData     = data + ReadResourceOffset(header->environment);
		stage->mPlacements._00        = (UnkStageObj*)(data + ReadResourceOffset(header->placements));
		stage->mScriptOffsets         = data + ReadResourceOffset(header->scriptOffsets);
		stage->mSetupScript           = data + ReadResourceOffset(header->setupScript);
		if (stage->mSetupScript == data)
			stage->mSetupScript = NULL;
		stage->mEntryScript = data + ReadResourceOffset(header->entryScript);
		if (stage->mEntryScript == data)
			stage->mEntryScript = NULL;
		stage->mBatteryDepletedScript = data + ReadResourceOffset(header->batteryDepletedScript);
		if (stage->mBatteryDepletedScript == data)
			stage->mBatteryDepletedScript = NULL;
		stage->mZButtonScript = data + ReadResourceOffset(header->zButtonScript);
		if (stage->mZButtonScript == data)
			stage->mZButtonScript = NULL;
		stage->mTimerExpiredScript = data + ReadResourceOffset(header->timerExpiredScript);
		if (stage->mTimerExpiredScript == data)
			stage->mTimerExpiredScript = NULL;
		stage->_198 = data + ReadResourceOffset(header->unknown20);
		if (stage->_198 == data)
			stage->_198 = NULL;
		stage->mEventObjectPairs  = (const StageEventObject*)(data + ReadResourceOffset(header->eventObjects));
		stage->mCameraAreaRecords = data + ReadResourceOffset(header->cameraAreas);
		if (stage->mCameraAreaRecords == data)
			stage->mCameraAreaRecords = NULL;
		stage->mCameraInfoRecords = data + ReadResourceOffset(header->cameraInfo);
		if (stage->mCameraInfoRecords == data)
			stage->mCameraInfoRecords = NULL;
		stage->mCameraAreaExRecords = data + ReadResourceOffset(header->cameraAreasEx);
		if (stage->mCameraAreaExRecords == data)
			stage->mCameraAreaExRecords = NULL;
		++stage->mState;
	}
	case 2:
		if ((s8)stage->mModulePath[0] != 0) {
			if (!stage->mModule.Load(stage->mModulePath))
				break;
			++stage->mState;
		} else {
			++stage->mState;
		}
	case 3:
		++stage->mState;
	case 4:
		if (LoadGameSoundBank(&gGame.mSound, index))
			++stage->mState;
		break;
	}
	return stage->mState > 4;
}

/**
 * @note Address: 0x8006F338
 * @note Size: 0x58
 */
extern "C" s32 CallStageModule(s32 function, s32 count, s32* args)
{
	if (IsModuleLoaded(&gStageResources.mModule))
		return gStageResources.CallModule(function, count, args);
	return -1;
}

/**
 * @note Address: 0x8006F390
 * @note Size: 0x1C
 */
extern "C" s32 GetStageCollectionFlagBase(s32 index)
{
	return ((s16*)(gStageResources.mEnvironment.mData + 0xA))[index];
}

/**
 * @note Address: 0x8006F3AC
 * @note Size: 0x10
 */
extern "C" CamAreaRecord* GetStageCameraAreaExRecords()
{
	return (CamAreaRecord*)gStageResources.mCameraAreaExRecords;
}

/**
 * @note Address: 0x8006F3BC
 * @note Size: 0x10
 */
extern "C" CamInfoData* GetStageCameraInfoRecords()
{
	return (CamInfoData*)gStageResources.mCameraInfoRecords;
}

/**
 * @note Address: 0x8006F3CC
 * @note Size: 0x10
 */
extern "C" CamAreaRecord* GetStageCameraAreaRecords()
{
	return (CamAreaRecord*)gStageResources.mCameraAreaRecords;
}

/**
 * @note Address: 0x8006F3DC
 * @note Size: 0x10
 */
extern "C" const StageEventObject* GetStageEventObjectPairs()
{
	return gStageResources.mEventObjectPairs;
}

/**
 * @note Address: 0x8006F3EC
 * @note Size: 0x10
 */
extern "C" u8* GetStageTimerExpiredScript()
{
	return gStageResources.mTimerExpiredScript;
}

/**
 * @note Address: 0x8006F3FC
 * @note Size: 0x10
 */
extern "C" u8* GetStageZButtonScript()
{
	return gStageResources.mZButtonScript;
}

/**
 * @note Address: 0x8006F40C
 * @note Size: 0x10
 */
extern "C" u8* GetStageBatteryDepletedScript()
{
	return gStageResources.mBatteryDepletedScript;
}

/**
 * @note Address: 0x8006F41C
 * @note Size: 0x10
 */
extern "C" u8* GetStageEntryScript()
{
	return gStageResources.mEntryScript;
}

/**
 * @note Address: 0x8006F42C
 * @note Size: 0x10
 */
extern "C" u8* GetStageSetupScript()
{
	return gStageResources.mSetupScript;
}

/**
 * @note Address: 0x8006F43C
 * @note Size: 0x54
 */
extern "C" u8* GetStageScriptByIndex(s32 index)
{
	u32 offset = ReadResourceOffset(gStageResources.mScriptOffsets + index * 4);
	return offset == 0 ? NULL : (u8*)gStageResources.mDataFile.mBuffer + offset;
}

/**
 * @note Address: 0x8006F490
 * @note Size: 0x10
 */
extern "C" u8* GetStageDataBuffer()
{
	return (u8*)gStageResources.mDataFile.mBuffer;
}

/**
 * @note Address: 0x8006F4A0
 * @note Size: 0x18
 */
extern "C" UnkStageObj* GetStagePlacementByIndex(s32 index)
{
	return gStageResources.mPlacements._00 + index;
}

/**
 * @note Address: 0x8006F4B8
 * @note Size: 0x10
 */
extern "C" EnvInfo* GetStageEnvironment()
{
	return &gStageResources.mEnvironment;
}

/**
 * @note Address: 0x8006F4C8
 * @note Size: 0x1C
 */
extern "C" BOOL IsStageDataLoaded()
{
	return gStageResources.mDataFile.mBuffer != NULL;
}

/**
 * @note Address: 0x8006F4E4
 * @note Size: 0x60
 */
extern "C" void ApplyStageClearColor()
{
	GXColor color                  = *(GXColor*)gStageResources.mEnvironment.mData;
	*(GXColor*)(HSD_VIData + 0x48) = color; // current.vi.bg_color
	gStageResources.mModule.Print();
}

/**
 * @note Address: 0x8006F544
 * @note Size: 0x90
 */
extern "C" void UnloadStageDataAndModule()
{
	if (IsModuleLoaded(&gStageResources.mModule))
		gStageResources.CallModule(6, 0, NULL);
	gStageResources.mModule.Unload();
	CFile* file = &gStageResources.mDataFile;
	file->Close();
	gStageResources.mState = 0;
}

/**
 * @note Address: 0x8006F5D4
 * @note Size: 0x2C
 */
extern "C" BOOL LoadStageByIndex(s32 index)
{
	return AdvanceStageLoad(&gStageResources, index);
}

static inline s32 StageCount()
{
	s32 count = 0;
	while (stageTable[count].fileName != NULL)
		++count;
	return count;
}

// 100 and up are the stages after the table's null entry
static inline s32 StageIndex(s32 index)
{
	if (index < 100)
		return index;
	s32 count = StageCount();
	count += index;
	return count - 99;
}

/**
 * @note Address: 0x8006F600
 * @note Size: 0x54
 */
extern "C" u32 CheckStageFlag(s32 index, u32 mask)
{
	StageDesc* desc = &stageTable[StageIndex(index)];
	return mask & desc->flags;
}

/**
 * @note Address: 0x8006F654
 * @note Size: 0xE8
 */
extern "C" s32 FindStageIndexByName(const char* name)
{
	s32 index = 0;
	while (stageTable[index].fileName != NULL) {
		if (strcmp(name, stageTable[index].fileName) == 0)
			return index;
		++index;
	}
	++index;
	while (stageTable[index].fileName != NULL) {
		if (strcmp(name, stageTable[index].fileName) == 0)
			return index - StageCount() + 99;
		++index;
	}
	return -1;
}

static inline const char* StageMessage(s32 index, s32 member)
{
	StageDesc* desc = &stageTable[StageIndex(index)];
	s32 message     = desc->message;
	if (message >= 0) {
		u8* data   = gGlobalScriptResources.mData;
		u8* record = data + ReadResourceOffset(data + 0x38) + message * 8;
		return (const char*)(data + ReadResourceOffset(record + member));
	}
	return NULL;
}

/**
 * @note Address: 0x8006F73C
 * @note Size: 0xD0
 */
extern "C" const char* GetStageDesc(s32 index)
{
	return StageMessage(index, 4);
}

/**
 * @note Address: 0x8006F80C
 * @note Size: 0xCC
 */
extern "C" const char* GetStageName(s32 index)
{
	return StageMessage(index, 0);
}

/**
 * @note Address: 0x8006F8D8
 * @note Size: 0x4C
 */
extern "C" const char* GetStageFileName(s32 index)
{
	index = StageIndex(index);
	return stageTable[index].fileName;
}

/**
 * @note Address: 0x8006F924
 * @note Size: 0x28
 */
extern "C" s32 GetStageNum()
{
	return StageCount();
}
