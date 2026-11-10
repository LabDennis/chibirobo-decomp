#include "game/cgame.h"
#include "game/infoscrn.h"
#include "game/xchr.h"

extern "C" {
void SpawnPlayerPointParticles(XObj* player, s32 type, s32 value);
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
	if (stats->mScrap > 99999)
		stats->mScrap = 99999;
	if (delta != 0)
		SpawnPlayerPointParticles(gGame.GetPlayer(), 3, delta);
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
	stats->mRank       = 1000000;
	stats->mMaxBattery = 80.0f;
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
