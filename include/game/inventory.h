#ifndef _GAME_INVENTORY_H
#define _GAME_INVENTORY_H

#include "types.h"

// one kind of item chibi robo carries
struct InventoryItem {
	s16 mId;    // _00
	s16 mCount; // _02, 0 is a free slot

	s16 GetId() const { return mId; }
	s16 GetCount() const { return mCount; }
};

// the player's items, the tail of Params. ten of a kind at most
struct Inventory {
	InventoryItem mItems[256]; // _00
	BOOL mCompact;             // _400, close gaps when a slot runs out
};
STATIC_ASSERT(sizeof(Inventory) == 0x404);

extern "C" {
void CompactInventory(Inventory* inventory);
// takes amount of item id away
s32 RemoveInventoryItemCount(Inventory* inventory, s16 id, s32 amount);
// Sets or adds the amount. Returns overflow above ten, zero on success, or -1 if full.
s32 SetOrAddInventoryItemCount(Inventory* inventory, s16 id, s16 amount, BOOL add);
s32 FindOwnedInventoryItemIndex(InventoryItem* items, s16 id);
}

#endif
