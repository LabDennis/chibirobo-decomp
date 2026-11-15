#if defined(VERSION_R24J01)
#include "game/wii/cgame.h"

// the Wii port kept the GameCube Params layout (verified member offsets in WiiPlayerParams)
typedef WiiPlayerParams Params;
#else
#include "game/cgame.h"
#include "game/infoscrn.h"
#include "game/xchr.h"
#endif

extern "C" {
#if defined(VERSION_R24J01)
void SpawnPlayerPointParticles(WiiPlayerState* player, s32 type, s32 value);
// the Wii HUD callbacks are empty stubs; the object at CGame+0xAF58 is unreconstructed
void InfoScrn_OnPlayerLevelChanged(void* scrn, s32 level);
void InfoScrn_OnPlayerExperienceChanged(void* scrn, s32 delta);
#else
void SpawnPlayerPointParticles(XObj* player, s32 type, s32 value);
#endif
}

/**
 * @note Address: 0x800DCD30
 * @note Size: 0x140
 */
extern "C" s32 FindOwnedInventoryItemIndex(InventoryItem* entries, s16 id)
{
	for (u32 i = 0; i < 256; ++i) {
		if (id == entries[i].mId && entries[i].mCount > 0)
			return i;
	}
	return -1;
}

/**
 * @note Address: 0x800DCE70
 * @note Size: 0x15C
 */
extern "C" s32 GetOwnedInventoryItemCount(Params* stats, s16 id)
{
	s32 index = FindOwnedInventoryItemIndex(stats->mInventory.mItems, id);
	return index >= 0 ? stats->mInventory.mItems[index].mCount : 0;
}

/**
 * @note Address: 0x800DCFCC
 * @note Size: 0x78
 */
extern "C" void AddPlayerScrap(Params* stats, s32 delta)
{
	stats->mScrap += delta;
	if (stats->mScrap < 0)
		stats->mScrap = 0;
	if (stats->mScrap > 99999) // the 5-digit cap
		stats->mScrap = 99999;
	if (delta != 0)
		SpawnPlayerPointParticles(gGame.GetPlayer(), 3, delta); // type 3 is the scrap particle
}

/**
 * @note Address: 0x800DD044
 * @note Size: 0x78
 */
extern "C" void AdjustPlayerMoney(Params* stats, s32 delta)
{
	stats->mMoney += delta;
	if (stats->mMoney < 0)
		stats->mMoney = 0;
	if (stats->mMoney > 99999)
		stats->mMoney = 99999;
	if (delta != 0)
		SpawnPlayerPointParticles(gGame.GetPlayer(), 2, delta);
}

/**
 * @note Address: 0x800DD0BC
 * @note Size: 0x3C
 */
extern "C" void AddPlayerRank(Params* stats, s32 delta)
{
	stats->mRank += delta;
	if (stats->mRank < 1)
		stats->mRank = 1;
	if (stats->mRank > 1000000)
		stats->mRank = 1000000;
}

/**
 * @note Address: 0x800DD0F8
 * @note Size: 0xA0
 */
extern "C" void AdjustPlayerLevel(Params* stats, s32 delta)
{
	stats->mLevel += delta;
	if (stats->mLevel < 0)
		stats->mLevel = 0;
	if (stats->mLevel > 30)
		stats->mLevel = 30;
	if (delta != 0) {
		InfoScrn_OnPlayerLevelChanged(&gGame.mInfoScrn, stats->mLevel + 1);
		SpawnPlayerPointParticles(gGame.GetPlayer(), 1, stats->mLevel + 1);
	}
}

/**
 * @note Address: 0x800DD198
 * @note Size: 0x9C
 */
extern "C" void AdjustPlayerExperience(Params* stats, s32 delta)
{
	stats->mExperience += delta;
	if (stats->mExperience < 0)
		stats->mExperience = 0;
	if (stats->mExperience > 99999)
		stats->mExperience = 99999;
	if (delta != 0) {
		InfoScrn_OnPlayerExperienceChanged(&gGame.mInfoScrn, delta);
		SpawnPlayerPointParticles(gGame.GetPlayer(), 0, delta);
	}
}

/**
 * @note Address: 0x800DD234
 * @note Size: 0xF8
 */
extern "C" void ResetPlayerStats(Params* stats)
{
	stats->mExperience = 0;
	stats->mLevel      = 0;
	stats->mRank       = 1000000; // rank counts down from the max
	stats->mMaxBattery = 80.0f;   // a fresh battery
	stats->mBattery    = stats->mMaxBattery;
	stats->mMoney      = 0;
	stats->mScrap      = 0;
	stats->_1C[0]      = 0;
	stats->_1C[1]      = 0;
	stats->_1C[2]      = 0;
	stats->_1C[3]      = 0;
	stats->_1C[4]      = 0;
	stats->_1C[5]      = 0;
	stats->_1C[6]      = 0;
	stats->_1C[7]      = 0;
	for (u32 i = 0; i < 256; ++i) {
		stats->mInventory.mItems[i].mId    = -1;
		stats->mInventory.mItems[i].mCount = 0;
	}
	stats->mInventory.mCompact = 1;
}
