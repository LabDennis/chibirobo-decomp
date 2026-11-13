#include "game/cgame.h"
#include "game/globals.h"
#include "game/stage.h"
#include "Version.h"
#include <stdio.h>

extern "C" {
#if defined(VERSION_JP)
// japan has one language, so one table each
extern const char* gJapanesePCDialogue[];
extern const char* gJapaneseInterfaceText[];
const char* gGlobalResourceFileName = "bin/globals.bin";
#else
// a string table per language
extern const char** gPCDialogueByLanguage[];
extern const char** gInterfaceTextByLanguage[];
const char* gGlobalResourceFileName = "globals.bin";
#endif
extern s32 gGlobalTextIdRemap[];
}

// globals.bin offsets are measured from the end of its header.
struct GlobalResourceSection {
	u8 offset[4];
	u8 length[4];
};

struct GlobalResourceHeader {
	GlobalResourceSection data;
	GlobalResourceSection text;
	GlobalResourceSection scripts;
};
STATIC_ASSERT(sizeof(GlobalResourceHeader) == 0x18);

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
	    && script < gGlobalScriptResources.mScripts + ReadResourceOffset(gGlobalScriptResources.mHeader->scripts.length)) {
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
extern "C" u8* GetPCRankingName(s32 index)
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
extern "C" s16 GetItemHappyPointValue(s32 index)
{
	return *(s16*)(ResourceField(0x28, index, 20, 16));
}

/**
 * @note Address: 0x8006E2D4
 * @note Size: 0x48
 */
extern "C" s16 GetItemScrapValue(s32 index)
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
extern "C" s16 GetItemPickupHoldTime(s32 index)
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
extern "C" u32 GetInteractionParameter(s32 index)
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
		gGlobalScriptResources.mHeader  = (GlobalResourceHeader*)gGlobalScriptResources.mFile.mBuffer;
		GlobalResourceHeader* header    = gGlobalScriptResources.mHeader;
		u8* buffer                      = (u8*)header;
		gGlobalScriptResources.mData    = buffer + ReadResourceOffset(header->data.offset) + sizeof(*header);
		gGlobalScriptResources.mText    = buffer + ReadResourceOffset(header->text.offset) + sizeof(*header);
		gGlobalScriptResources.mScripts = buffer + ReadResourceOffset(header->scripts.offset) + sizeof(*header);
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
