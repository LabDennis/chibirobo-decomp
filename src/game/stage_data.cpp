#include "game/cgame.h"
#include "types.h"
#include "Version.h"

#include "game/globals.h"
#include "game/stage.h"
#include "game/ut.h"
#include "Dolphin/GX/GXTypes.h"
#include <stdio.h>
#include <string.h>

extern "C" {
#if defined(VERSION_JP)
// japan has one language, so one table each
extern const char* gJapanesePCDialogue[];
extern const char* gJapaneseInterfaceText[];
#else
// a string table per language
extern const char** gPCDialogueByLanguage[];
extern const char** gInterfaceTextByLanguage[];
#endif
extern s32 gGlobalTextIdRemap[];
// the globals file's name
extern const char* gGlobalResourceFileName;
extern UnkStageObj gStageObjectPlacements[];
extern UnkStageObj spawnTable[];
extern UnkObjData objTable[];
extern UnkObjData objTableEx[];
extern XObjSource gObjectSources[];
// HSD_VIInfo, its header needs the hsd include path
extern u8 HSD_VIData[];
BOOL LoadGameSoundBank(XSound* sound, s32 stage);
}

Stage gStageResources;
Globals gGlobalScriptResources;

// an entry of one of the data part's tables, the table offsets sit at the start of it

static inline u8* ResourceField(s32 table, s32 index, s32 stride, s32 member)
{
	u8* data = gGlobalScriptResources.mData;
	return data + ReadResourceOffset(data + table) + member + index * stride;
}

// the same, through an offset the entry holds
static inline u8* ResourcePointer(s32 table, s32 index, s32 stride, s32 member)
{
	u8* data   = gGlobalScriptResources.mData;
	u8* record = data + ReadResourceOffset(data + table) + index * stride;
	return data + ReadResourceOffset(record + member);
}

// a 4 byte offset from base, 0 is none
static inline u8* ReadScriptPointer(u8* base, const u8* operand)
{
	u32 offset = ReadResourceOffset(operand);
	if (offset == 0)
		return NULL;
	return base + offset;
}

static inline BOOL IsGlobalScript(const u8* script)
{
	bool inside = false;
	if (script >= gGlobalScriptResources.mScripts
	    && script < gGlobalScriptResources.mScripts + ReadResourceOffset(gGlobalScriptResources.mHeader + 0x14)) {
		inside = true;
	}
	return inside != false;
}

/**
 * @note Address: 0x8006DBC8
 * @note Size: 0x28
 */
extern "C" const char* GetLocalizedPCDialogue(s32 index)
{
#if defined(VERSION_JP)
	return gJapanesePCDialogue[index];
#else
	return gPCDialogueByLanguage[gGame.mLanguage][index];
#endif
}

/**
 * @note Address: 0x8006DBF0
 * @note Size: 0x28
 */
extern "C" const char* GetLocalizedInterfaceText(s32 index)
{
#if defined(VERSION_JP)
	return gJapaneseInterfaceText[index];
#else
	return gInterfaceTextByLanguage[gGame.mLanguage][index];
#endif
}

/**
 * @note Address: 0x8006DC18
 * @note Size: 0x74
 */
extern "C" u8* GetStatusOverviewDescription(s32 index)
{
	return ResourcePointer(0x50, index, 8, 4);
}

/**
 * @note Address: 0x8006DC8C
 * @note Size: 0x70
 */
extern "C" u8* GetStatusOverviewLabel(s32 index)
{
	return ResourcePointer(0x50, index, 8, 0);
}

/**
 * @note Address: 0x8006DCFC
 * @note Size: 0x48
 */
extern "C" u32 GetStatusIconEventFlagId(s32 index)
{
	return *(u32*)(ResourceField(0x4C, index, 12, 8));
}

/**
 * @note Address: 0x8006DD44
 * @note Size: 0x74
 */
extern "C" u8* GetStatusIconDescription(s32 index)
{
	return ResourcePointer(0x4C, index, 12, 4);
}

/**
 * @note Address: 0x8006DDB8
 * @note Size: 0x70
 */
extern "C" u8* GetStatusIconName(s32 index)
{
	return ResourcePointer(0x4C, index, 12, 0);
}

/**
 * @note Address: 0x8006DE28
 * @note Size: 0x48
 */
extern "C" s16 GetUtilibotPrice(s32 index)
{
	return *(s16*)(ResourceField(0x48, index, 12, 8));
}

/**
 * @note Address: 0x8006DE70
 * @note Size: 0x74
 */
extern "C" u8* GetUtilibotDescription(s32 index)
{
	return ResourcePointer(0x48, index, 12, 4);
}

/**
 * @note Address: 0x8006DEE4
 * @note Size: 0x70
 */
extern "C" u8* GetUtilibotName(s32 index)
{
	return ResourcePointer(0x48, index, 12, 0);
}

/**
 * @note Address: 0x8006DF54
 * @note Size: 0x70
 */
extern "C" u8* fn_8006DF54(s32 index)
{
	return ResourcePointer(0x44, index, 4, 0);
}

/**
 * @note Address: 0x8006DFC4
 * @note Size: 0x70
 */
extern "C" u8* GetStageDisplayName(s32 index)
{
	return ResourcePointer(0x38, index, 8, 0);
}

/**
 * @note Address: 0x8006E034
 * @note Size: 0x70
 */
extern "C" u8* GetCostumeName(s32 index)
{
	return ResourcePointer(0x34, index, 4, 0);
}

/**
 * @note Address: 0x8006E0A4
 * @note Size: 0x48
 */
extern "C" s16 GetAttachmentShopPrice(s32 index)
{
	return *(s16*)(ResourceField(0x30, index, 12, 8));
}

/**
 * @note Address: 0x8006E0EC
 * @note Size: 0x74
 */
extern "C" u8* GetAttachmentDescription(s32 index)
{
	return ResourcePointer(0x30, index, 12, 4);
}

/**
 * @note Address: 0x8006E160
 * @note Size: 0x70
 */
extern "C" u8* GetAttachmentName(s32 index)
{
	return ResourcePointer(0x30, index, 12, 0);
}

/**
 * @note Address: 0x8006E1D0
 * @note Size: 0x70
 */
extern "C" u8* GetActorName(s32 index)
{
	return ResourcePointer(0x2C, index, 4, 0);
}

/**
 * @note Address: 0x8006E240
 * @note Size: 0x4C
 */
extern "C" u8 GetItemSoundIndex(s32 index, s32 member)
{
	return ResourceField(0x28, index, 20, 18)[member];
}

/**
 * @note Address: 0x8006E28C
 * @note Size: 0x48
 */
extern "C" s16 fn_8006E28C(s32 index)
{
	return *(s16*)(ResourceField(0x28, index, 20, 16));
}

/**
 * @note Address: 0x8006E2D4
 * @note Size: 0x48
 */
extern "C" s16 fn_8006E2D4(s32 index)
{
	return *(s16*)(ResourceField(0x28, index, 20, 14));
}

/**
 * @note Address: 0x8006E31C
 * @note Size: 0x48
 */
extern "C" s16 GetItemShopPrice(s32 index)
{
	return *(s16*)(ResourceField(0x28, index, 20, 12));
}

/**
 * @note Address: 0x8006E364
 * @note Size: 0x48
 */
extern "C" s16 fn_8006E364(s32 index)
{
	return *(s16*)(ResourceField(0x28, index, 20, 10));
}

/**
 * @note Address: 0x8006E3AC
 * @note Size: 0x50
 */
extern "C" void ClearItemFlags(s32 index, u16 mask)
{
	u16* flags = (u16*)(ResourceField(0x28, index, 20, 8));
	*flags &= ~mask;
}

/**
 * @note Address: 0x8006E3FC
 * @note Size: 0x50
 */
extern "C" void SetItemFlags(s32 index, u16 mask)
{
	u16* flags = (u16*)(ResourceField(0x28, index, 20, 8));
	*flags |= mask;
}

/**
 * @note Address: 0x8006E44C
 * @note Size: 0x50
 */
extern "C" u16 GetItemFlagsMasked(s32 index, u16 mask)
{
	return *(u16*)(ResourceField(0x28, index, 20, 8)) & mask;
}

/**
 * @note Address: 0x8006E49C
 * @note Size: 0x74
 */
extern "C" u8* GetItemDescription(s32 index)
{
	return ResourcePointer(0x28, index, 20, 4);
}

/**
 * @note Address: 0x8006E510
 * @note Size: 0x70
 */
extern "C" u8* GetItemName(s32 index)
{
	return ResourcePointer(0x28, index, 20, 0);
}

// script offsets count from the buffer the operand is in, the globals file or the stage's, 0 is none
/**
 * @note Address: 0x8006E580
 * @note Size: 0xEC
 */
extern "C" u8* ResolveEventScriptOffset(u8* operand)
{
	if (IsGlobalScript(operand)) {
		u32 offset = ReadResourceOffset(operand);
		return offset == 0 ? NULL : gGlobalScriptResources.mScripts + offset;
	}
	u32 offset = ReadResourceOffset(operand);
	return offset == 0 ? NULL : (u8*)gStageResources.mDataFile.mBuffer + offset;
}

/**
 * @note Address: 0x8006E66C
 * @note Size: 0x68
 */
extern "C" BOOL IsGlobalEventScript(u8* script)
{
	return IsGlobalScript(script);
}

/**
 * @note Address: 0x8006E6D4
 * @note Size: 0x50
 */
extern "C" u8* GetGlobalScriptByIndex(s32 index)
{
	return ReadScriptPointer(gGlobalScriptResources.mScripts, gGlobalScriptResources.mScripts + index * 4);
}

/**
 * @note Address: 0x8006E724
 * @note Size: 0x10
 */
extern "C" u8* GetGlobalScriptTable(s32)
{
	return gGlobalScriptResources.mScripts;
}

// a text entry, ids from 10000 go through a remap table first
/**
 * @note Address: 0x8006E734
 * @note Size: 0x88
 */
extern "C" u8* GetGlobalTextEntry(s32 index)
{
	if (index >= 10000) {
		index = gGlobalTextIdRemap[index - 10000];
		if (index < 0)
			return NULL;
	}
	u8* text  = gGlobalScriptResources.mText;
	u8* entry = text + index * 4;
	if (*(u32*)entry != 0)
		return text + ReadResourceOffset(entry);
	return NULL;
}

/**
 * @note Address: 0x8006E7BC
 * @note Size: 0x40
 */
extern "C" s32 GetDefaultStageIndex()
{
	u64 flags = gGame.mStatus.mFlags;
	if (flags & 0x8000000000000ULL)
		return 0;
	return 14;
}

/**
 * @note Address: 0x8006E7FC
 * @note Size: 0x6C
 */
extern "C" u32 GetItemReleaseSoundId(s32 index)
{
	return ReadResourceOffset(ResourceField(0x24, index, 4, 0));
}

/**
 * @note Address: 0x8006E868
 * @note Size: 0x6C
 */
extern "C" u32 GetMaterialSoundId(s32 index)
{
	return ReadResourceOffset(ResourceField(0x20, index, 4, 0));
}

/**
 * @note Address: 0x8006E8D4
 * @note Size: 0x6C
 */
extern "C" u32 GetPickupRewardValue(s32 index)
{
	return ReadResourceOffset(ResourceField(0x1C, index, 4, 0));
}

/**
 * @note Address: 0x8006E940
 * @note Size: 0x6C
 */
extern "C" u32 GetStartingAttachmentLevel(s32 index)
{
	return ReadResourceOffset(ResourceField(0x18, index, 4, 0));
}

/**
 * @note Address: 0x8006E9AC
 * @note Size: 0x6C
 */
extern "C" u32 fn_8006E9AC(s32 index)
{
	return ReadResourceOffset(ResourceField(0x14, index, 4, 0));
}

/**
 * @note Address: 0x8006EA18
 * @note Size: 0x6C
 */
extern "C" u32 GetStageEntryTimeComponent(s32 index)
{
	return ReadResourceOffset(ResourceField(0x10, index, 4, 0));
}

/**
 * @note Address: 0x8006EA84
 * @note Size: 0x6C
 */
extern "C" u32 GetRadarParameter(s32 index)
{
	return ReadResourceOffset(ResourceField(0xC, index, 4, 0));
}

/**
 * @note Address: 0x8006EAF0
 * @note Size: 0x6C
 */
extern "C" u32 GetCopterParameter(s32 index)
{
	return ReadResourceOffset(ResourceField(0x8, index, 4, 0));
}

/**
 * @note Address: 0x8006EB5C
 * @note Size: 0x6C
 */
extern "C" u32 GetPopperParameter(s32 index)
{
	return ReadResourceOffset(ResourceField(0x4, index, 4, 0));
}

/**
 * @note Address: 0x8006EBC8
 * @note Size: 0x6C
 */
extern "C" u32 GetPlayerActionParameter(s32 index)
{
	return ReadResourceOffset(ResourceField(0x0, index, 4, 0));
}

/**
 * @note Address: 0x8006EC34
 * @note Size: 0x1C
 */
extern "C" BOOL IsGlobalDataLoaded()
{
	return gGlobalScriptResources.mFile.mBuffer != NULL;
}

// reads the globals file and finds its three parts
/**
 * @note Address: 0x8006EC50
 * @note Size: 0x140
 */
extern "C" BOOL LoadGlobalResourceData()
{
#if defined(VERSION_JP)
	const char* path = gGlobalResourceFileName;
#else
	char path[0x40];
	sprintf(path, "bin%s/%s", gResourceDirectorySuffixByLanguage[gGame.mLanguage], gGlobalResourceFileName);
#endif
	CFile* file = &gGlobalScriptResources.mFile;
	if (file->ReadEx(path, 0, 0)) {
		gGlobalScriptResources.mFile.CloseHandle();
		gGlobalScriptResources.mHeader  = (u8*)gGlobalScriptResources.mFile.mBuffer;
		u8* header                      = gGlobalScriptResources.mHeader;
		gGlobalScriptResources.mData    = header + ReadResourceOffset(header) + 0x18;
		gGlobalScriptResources.mText    = header + ReadResourceOffset(header + 8) + 0x18;
		gGlobalScriptResources.mScripts = header + ReadResourceOffset(header + 16) + 0x18;
		return TRUE;
	}
	return FALSE;
}

// the status screen's icon for each item
#if defined(VERSION_JP)
enum { DefaultItemIcon = 124, Item32Icon = 242 };
#else
enum { DefaultItemIcon = 126, Item32Icon = 244 };
#endif

// clang-format off
s32 gItemIconTextureIndices[] = {
	/* 0x00 */ 0, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon,
	/* 0x08 */ 1, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, 2, DefaultItemIcon,
	/* 0x10 */ 3, DefaultItemIcon, 4, 5, DefaultItemIcon, 6, 7, 8,
	/* 0x18 */ DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon,
	/* 0x20 */ Item32Icon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon,
	/* 0x28 */ DefaultItemIcon, 9, 10, 11, 12, 13, DefaultItemIcon, DefaultItemIcon,
	/* 0x30 */ 14, 15, 16, 18, 17, 19, 19, 20,
	/* 0x38 */ 21, 22, 23, 24, 25, 26, 19, DefaultItemIcon,
	/* 0x40 */ DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon,
	/* 0x48 */ DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon,
	/* 0x50 */ DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, 27,
	/* 0x58 */ 28, 27, 27, 29, 30, 31, 32, 33,
	/* 0x60 */ 34, 35, 36, 37, 38, 39, 40, 41,
	/* 0x68 */ DefaultItemIcon, 42, DefaultItemIcon, 43, 44, 45, 46, 47,
	/* 0x70 */ 48, 49, 50, 51, 52, 53, 54, 55,
	/* 0x78 */ 56, 57, 58, 59, 60, 61, 62, 63,
	/* 0x80 */ DefaultItemIcon, 64, DefaultItemIcon, 65, 66, 67, 68, 69,
	/* 0x88 */ 70, 71, 72, 73, 74, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon,
	/* 0x90 */ DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, 75, 76, 77, 78, 79,
	/* 0x98 */ 80, 81, 82, 83, 84, 85, 86,
};
// clang-format on

/**
 * @note Address: 0x8006ED90
 * @note Size: 0x24
 */
extern "C" s32 GetItemIconTextureIndex(s32 index)
{
	if (index < 0)
		return -1;
	return gItemIconTextureIndices[index];
}

/**
 * @note Address: 0x8006EDB4
 * @note Size: 0x14
 */
extern "C" UnkStageObj* GetStageObjectPlacementByIndex(s32 index)
{
	return &gStageObjectPlacements[index];
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
#elif defined(VERSION_PAL)
		StageDesc* desc = &stageTable[index];
		// NONMATCHING: retail schedules the language index and the table pointer differently here
		strcpy(stage->mDataPath, FormatToStaticBuffer("bin%s/%s.bin", gResourceDirectorySuffixByLanguage[gGame.mLanguage], desc->fileName));
#else
		strcpy(stage->mDataPath,
		       FormatToStaticBuffer("bin%s/%s.bin", gResourceDirectorySuffixByLanguage[gGame.mLanguage], stageTable[index].fileName));
#endif
#if defined(VERSION_PAL)
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
		u8* data                  = (u8*)stage->mDataFile.mBuffer;
		stage->mEnvironment.mData = data + ReadResourceOffset(data);
		stage->mPlacements._00    = (UnkStageObj*)(data + ReadResourceOffset(data + 4));
		stage->mScriptOffsets     = data + ReadResourceOffset(data + 8);
		stage->mSetupScript       = data + ReadResourceOffset(data + 0xC);
		if (stage->mSetupScript == data)
			stage->mSetupScript = NULL;
		stage->mEntryScript = data + ReadResourceOffset(data + 0x10);
		if (stage->mEntryScript == data)
			stage->mEntryScript = NULL;
		stage->mBatteryDepletedScript = data + ReadResourceOffset(data + 0x14);
		if (stage->mBatteryDepletedScript == data)
			stage->mBatteryDepletedScript = NULL;
		stage->mZButtonScript = data + ReadResourceOffset(data + 0x18);
		if (stage->mZButtonScript == data)
			stage->mZButtonScript = NULL;
		stage->mTimerExpiredScript = data + ReadResourceOffset(data + 0x1C);
		if (stage->mTimerExpiredScript == data)
			stage->mTimerExpiredScript = NULL;
		stage->_198 = data + ReadResourceOffset(data + 0x20);
		if (stage->_198 == data)
			stage->_198 = NULL;
		stage->mEventObjectPairs  = (const s32*)(data + ReadResourceOffset(data + 0x24));
		stage->mCameraAreaRecords = data + ReadResourceOffset(data + 0x28);
		if (stage->mCameraAreaRecords == data)
			stage->mCameraAreaRecords = NULL;
		stage->mCameraInfoRecords = data + ReadResourceOffset(data + 0x2C);
		if (stage->mCameraInfoRecords == data)
			stage->mCameraInfoRecords = NULL;
		stage->mCameraAreaExRecords = data + ReadResourceOffset(data + 0x30);
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
extern "C" const s32* GetStageEventObjectPairs()
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

/**
 * @note Address: 0x8006F94C
 * @note Size: 0x128
 */
// __sinit_stage_cpp, builds gStageResources and gGlobalScriptResources
