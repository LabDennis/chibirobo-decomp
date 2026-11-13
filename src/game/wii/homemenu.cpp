#include "game/wii/cgame.h"
#include "revolution/mem.h"

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
void fn_80019CBC(void*);
extern MEMAllocator gWiiPadAllocator;
}

extern "C" void InitWiiHomeMenu(WiiHomeMenu* menu)
{
	menu->loadState = 0;
	fn_80019CBC(menu->resources);
	menu->active = 0;
	for (int i = 0; i < 4; ++i)
		menu->pointers[i].x = menu->pointers[i].y = 0;
	menu->state = menu->layout = menu->message = menu->configuration = 0;
	// The archive loader indexes its retail homeBtn table with the same field.
	menu->language = gGame.mLanguage;
	menu->flags = menu->sound = 0;
	menu->heapSize            = 0x80000;
	menu->scaleX = menu->scaleY = menu->speed = 1;
	menu->allocator                           = &gWiiPadAllocator;
}
