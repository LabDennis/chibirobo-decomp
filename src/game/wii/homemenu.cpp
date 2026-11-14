#include "game/wii/cgame.h"
#include "revolution/mem.h"

// One function of a larger file: its .sdata2 pool (0x805A4470-0x805A44CC) is shared with
// fn_800AAD1C through fn_800ABA98, so the unit cannot be linked until those are written too.

struct WiiHomeMenu {
	u32 state;
	u8 _004[0x4a0 - 4];
	u32 loadState;
	u8 _4a4[4];
	u8 resources[0x5e4 - 0x4a8];
	u32 layout, message, configuration, language;
	u32 flags, sound;
	u8 _5fc[4];
	u32 heapSize;
	f32 scaleX, scaleY, speed;
	MEMAllocator* allocator;
	u8 _614[4];
	struct {
		f32 x, y;
		u8 _08[8];
	} pointers[4];
	u32 active;
};
STATIC_ASSERT(sizeof(WiiHomeMenu) == 0x65c);

extern "C" {
u8 SCGetLanguage();
void fn_80019CBC(void*);
extern MEMAllocator gWiiPadAllocator;
}

extern "C" void InitWiiHomeMenu(WiiHomeMenu* menu)
{
	menu->loadState = 0;
	fn_80019CBC(menu->resources);
	menu->active = 0;
	for (int i = 0; i < 4; ++i) {
		menu->pointers[i].x = 0;
		menu->pointers[i].y = 0;
	}
	menu->state         = 0;
	menu->layout        = 0;
	menu->message       = 0;
	menu->configuration = 0;
	menu->language      = SCGetLanguage();
	menu->flags         = 0;
	menu->sound         = 0;
	menu->heapSize      = 0x80000;
	menu->scaleX        = 1;
	menu->scaleY        = 1;
	menu->speed         = 1;
	menu->allocator     = &gWiiPadAllocator;
}
