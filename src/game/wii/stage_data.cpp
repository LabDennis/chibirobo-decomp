// Stage and global resource tables and accessors, one retail file (.text 0x80092308-0x8009453C).
#include "Dolphin/GX/GXFrameBuffer.h"
#include "game/wii/cgame.h"
#include "game/cfile.h"
#include "game/globals.h"
#include "game/stage.h"
#include <stdio.h>
#include <string.h>

extern "C" {
char* FormatToStaticBuffer(const char* format, ...);
s32 _prolog(s32 function, s32 count, s32* args);
}
// the game's own aligned array new: MEMAllocFromExpHeapEx on the MEM2 game heap
void* operator new[](size_t size, int align);

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

// globals.bin offsets are measured from the end of its header.
struct GlobalResourceSection {
	u8 offset[4];
	u8 length[4];
};

struct GlobalResourceHeader {
	GlobalResourceSection data;
	GlobalResourceSection collision;
	GlobalResourceSection scripts;
};
STATIC_ASSERT(sizeof(GlobalResourceHeader) == 0x18);

extern "C" {
StagePlacement gStageObjectPlacements[] = {
	{ 161, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 59, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 60, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 61, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 62, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 63, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 64, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 66, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 69, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 253, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 254, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 255, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 256, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 257, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 258, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 259, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 282, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x60012000 },
	{ 286, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 309, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 310, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 286, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 332, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x60012000 },
	{ 341, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 1, 0x60012000 },
	{ 346, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 365, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 366, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 367, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 368, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 369, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 370, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 371, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 372, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 373, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 374, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 375, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 387, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 388, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 389, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 390, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 391, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 392, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 393, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 394, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 395, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 412, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 413, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 418, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 422, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 274, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 437, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 144, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 458, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 459, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 462, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 463, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 496, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 504, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x60012000 },
	{ 558, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 578, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 579, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 580, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 581, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 582, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 532, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 533, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 534, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 535, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 536, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 537, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 538, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 539, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 540, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 541, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 542, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 543, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 544, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 545, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 546, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 547, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 548, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 549, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 550, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 551, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 552, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 553, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 554, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 555, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 636, { 0, 0, 0 }, { 0, 180, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 638, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 640, { 0, 0, 0 }, { 0, 180, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 641, { 0, 0, 0 }, { 0, 180, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 643, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 647, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 648, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 649, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 650, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 651, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 703, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 655, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 656, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 657, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 658, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 659, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 660, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 661, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 662, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 663, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 665, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 666, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 683, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 685, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 686, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 693, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 704, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 804, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 811, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 812, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 819, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 820, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 821, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 822, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 823, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 824, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 825, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 830, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 834, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 836, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 837, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 860, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 868, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 866, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 871, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 872, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 885, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 910, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 919, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 920, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 921, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 922, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 923, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 933, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 950, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 951, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 952, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 953, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 954, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 982, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 983, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 984, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 985, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 986, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 987, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 1045, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 1058, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 1063, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 1064, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 1098, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 1099, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
	{ 1129, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x20012000 },
};
}

// the status screen's icon for each item
enum { DefaultItemIcon = 127, Item32Icon = 245 };

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

// The game's displayed text comes from the disc: tools/gentext.py writes
// build/<V>/include/assets/text/<table>.inc from orig/<V>/sys/main.dol.
extern "C" {
const char* gJapaneseInterfaceText[] = {
#include "assets/text/gJapaneseInterfaceText.inc"
};

const char* gJapanesePCDialogue[] = {
#include "assets/text/gJapanesePCDialogue.inc"
};

const char* gEnglishInterfaceText[] = {
#include "assets/text/gEnglishInterfaceText.inc"
};

const char* gEnglishPCDialogue[] = {
#include "assets/text/gEnglishPCDialogue.inc"
};

const char* gGermanInterfaceText[] = {
#include "assets/text/gGermanInterfaceText.inc"
};

const char* gGermanPCDialogue[] = {
#include "assets/text/gGermanPCDialogue.inc"
};

const char* gFrenchInterfaceText[] = {
#include "assets/text/gFrenchInterfaceText.inc"
};

const char* gFrenchPCDialogue[] = {
#include "assets/text/gFrenchPCDialogue.inc"
};

const char* gSpanishInterfaceText[] = {
#include "assets/text/gSpanishInterfaceText.inc"
};

const char* gSpanishPCDialogue[] = {
#include "assets/text/gSpanishPCDialogue.inc"
};

const char* gItalianInterfaceText[] = {
#include "assets/text/gItalianInterfaceText.inc"
};

const char* gItalianPCDialogue[] = {
#include "assets/text/gItalianPCDialogue.inc"
};
}

static const char* sCbRoboAnims[]               = { "cb_def",
	                                                "cb_wait_lp",
	                                                "cb_wait_B_lp",
	                                                "cb_wlk_lp",
	                                                "cb_run_lp",
	                                                "cb_lift_up_obj_st",
	                                                "cb_lift_up_obj_lp",
	                                                "cb_lift_up_obj_en",
	                                                "cb_lift_obj_1f",
	                                                "cb_put_down_obj",
	                                                "cb_put_in_obj",
	                                                "cb_take_out_obj",
	                                                "cb_wlk_lift_obj_lp",
	                                                "cb_run_lift_obj_lp",
	                                                "cb_mabataki",
	                                                "cb_wait_st",
	                                                "cb_wait_en",
	                                                "cb_std",
	                                                "cb_wait_C_lp",
	                                                "cb_go_up_the_step12_st",
	                                                "cb_hang_lp",
	                                                "cb_hang_move_r_lp",
	                                                "cb_hang_move_l_lp",
	                                                "cb_go_up_the_step12_en",
	                                                "cb_getoff_hang",
	                                                "cb_getoff_the_step12_st",
	                                                "cb_getoff_the_step12_1f",
	                                                "cb_getoff_the_step12_en",
	                                                "cb_jump_down_st",
	                                                "cb_jump_down_1f",
	                                                "cb_jump_down_en",
	                                                "cb_getoff_the_step8",
	                                                "cb_getoff_the_step4",
	                                                "cb_cling_the_rope_st",
	                                                "cb_go_up_the_rope_st",
	                                                "cb_getoff_the_rope_1f",
	                                                "cb_go_up_the_rope_en",
	                                                "cb_getoff_the_rope_en01",
	                                                "cb_propeller_st",
	                                                "cb_propeller_lp",
	                                                "cb_propeller_lp2",
	                                                "cb_propeller_en",
	                                                "cb_lift_up_put_in_obj",
	                                                "cb_go_up_hold_on_ladder",
	                                                "cb_go_up_ladder_st",
	                                                "cb_go_up_ladder_lp",
	                                                "cb_go_up_ladder_en",
	                                                "cb_hold_on_ladder",
	                                                "cb_go_up_the_rope_r",
	                                                "cb_go_up_the_rope_l",
	                                                "cb_rope_r_cling",
	                                                "cb_rope_l_cling",
	                                                "cb_go_up_the_rope_lp",
	                                                "cb_run_slip",
	                                                "cb_aomuke_1f",
	                                                "cb_aomuke_getup",
	                                                "cb_getoff_the_rope_en02",
	                                                "cb_getoff_the_rope_en03",
	                                                "cb_getoff_hold_on_ladder",
	                                                "cb_getoff_ladder_st",
	                                                "cb_getoff_ladder_lp",
	                                                "cb_getoff_ladder_en",
	                                                "cb_push_pull_obj_st",
	                                                "cb_push_pull_obj_taiki",
	                                                "cb_push_obj_lp",
	                                                "cb_pull_obj_lp",
	                                                "cb_push_pull_obj_en",
	                                                "cb_energy_0",
	                                                "cb_propeller_B_out",
	                                                "cb_propeller_B_rot_st",
	                                                "cb_propeller_B_rot_en",
	                                                "cb_propeller_B_in",
	                                                "cb_wlk_propeller_lp",
	                                                "cb_run_propeller_lp",
	                                                "cb_std_propeller",
	                                                "cb_push_obj_st",
	                                                "cb_pull_obj_st",
	                                                "cb_getoff_ladder_en02",
	                                                "cb_getoff_ladder_en03",
	                                                "cb_cannon_out",
	                                                "cb_cannon_in",
	                                                "cb_cannon_ready",
	                                                "cb_cannon_aim",
	                                                "cb_cannon_shoot",
	                                                "cb_cannon_yameru",
	                                                "cb_sharpener_hold",
	                                                "cb_sharpener_hold_lp",
	                                                "cb_sharpener_rot",
	                                                "cb_sharpener_hanasu",
	                                                "cb_getoff_the_rope_en04",
	                                                "cb_go_up_the_step_low_st",
	                                                "cb_go_up_the_step_low_1f",
	                                                "cb_go_up_the_step_low_en",
	                                                "cb_think_st",
	                                                "cb_think_lp",
	                                                "cb_think_en",
	                                                "cb_go_up_the_step_mid_st",
	                                                "cb_go_up_the_step_mid_1f",
	                                                "cb_go_up_the_step_mid_en",
	                                                "cb_go_up_the_step_hi_st",
	                                                "cb_go_up_the_step_hi_1f",
	                                                "cb_go_up_the_step_hi_en",
	                                                "cb_go_up_hasigo_st",
	                                                "cb_wait_hasigo_L",
	                                                "cb_wait_hasigo_R",
	                                                "cb_go_up_hasigo_L",
	                                                "cb_go_up_hasigo_R",
	                                                "cb_getoff_hasigo_L",
	                                                "cb_getoff_hasigo_R",
	                                                "cb_getoff_hasigo_st",
	                                                "cb_go_up_hasigo_L_en",
	                                                "cb_go_up_hasigo_R_en",
	                                                "cb_getoff_hasigo_L_en",
	                                                "cb_getoff_hasigo_R_en",
	                                                "cb_cancel_hasigo_L_st",
	                                                "cb_cancel_hasigo_R_st",
	                                                "cb_cancel_hasigo_1f",
	                                                "cb_cancel_hasigo_en",
	                                                "cb_drop_hold",
	                                                "cb_jog_lp",
	                                                "cb_maru_batu",
	                                                "cb_run_U_turn",
	                                                "cb_ottotto",
	                                                "cb_leticer_rot",
	                                                "cb_damage_drop_down",
	                                                "cb_go_up_the_step12_en02",
	                                                "cb_kin_switch_out",
	                                                "cb_kin_osu_kamae",
	                                                "cb_kin_osu_kamae_02",
	                                                "cb_kin_osu",
	                                                "cb_kin_osu_1f",
	                                                "cb_kin_osu_std",
	                                                "cb_kin_switch_in",
	                                                "cb_drop_hold_rope",
	                                                "cb_drop_hold_ladder",
	                                                "cb_drop_hold_ladder_02",
	                                                "cb_jog_lift_obj_lp",
	                                                "cb_run_U_turn_lift_obj",
	                                                "cb_cling_the_rope_lp_01",
	                                                "cb_go_up_the_rope_01",
	                                                "cb_cling_the_rope_lp_02",
	                                                "cb_go_up_the_rope_02",
	                                                "cb_jump_down_en_01",
	                                                "cb_jump_down_en_02",
	                                                "cb_jump_down_en_03",
	                                                "cb_getoff_the_rope_en0401",
	                                                "cb_jump_down_st_obj",
	                                                "cb_jump_down_1f_obj",
	                                                "cb_jump_down_en_obj",
	                                                "cb_getoff_the_step8_obj",
	                                                "cb_getoff_the_step4_obj",
	                                                "cb_lift_up_plug_st",
	                                                "cb_getoff_hold_on_rope",
	                                                "cb_fall_tub",
	                                                "cb_jump_down_en_0301",
	                                                "cb_jump_down_en_01_obj",
	                                                "cb_jump_down_en_02_obj",
	                                                "cb_jump_down_en_03_obj",
	                                                "cb_jump_down_en_0301_obj",
	                                                "cb_plug_in_concent_A",
	                                                "cb_plug_in_leticer",
	                                                "cb_propeller_B_idling",
	                                                "cb_plug_in_floor",
	                                                "cb_propeller_B_idling_wlk",
	                                                "cb_cannon_aim_wlk",
	                                                "cb_plug_in_concent_B",
	                                                "cb_fall_tub_01",
	                                                "cb_fall_tub_02",
	                                                "cb_fall_tub_03",
	                                                "cb_radar_out",
	                                                "cb_radar_in",
	                                                "cb_plug_leticer_1f",
	                                                "cb_plug_out_leticer",
	                                                "cb_plug_out_concent_A",
	                                                "cb_plug_leticer_1f_02",
	                                                "cb_out_chibipot",
	                                                "cb_in_chibipot",
	                                                "cb_sleep_mode",
	                                                "cb_jog_propeller_lp",
	                                                "cb_plug_out_run",
	                                                "cb_cbhouse_lever_st",
	                                                "cb_cbhouse_lever_lp",
	                                                "cb_cbhouse_lever_choice",
	                                                "cb_cbhouse_lever_decision",
	                                                "cb_cbhouse_lever_en",
	                                                "cb_pencase_ottotto_A",
	                                                "cb_eye_rot",
	                                                "cb_check_knock",
	                                                "cb_plug_in_door",
	                                                "cb_plug_out_door",
	                                                "cb_plug_door_taiki",
	                                                "cb_plug_door_rot",
	                                                "cb_gacha_gacha",
	                                                "cb_gacha_gacha_in",
	                                                "cb_dokan_in",
	                                                "cb_dokan_in_1f",
	                                                "cb_brush_pose_std",
	                                                "cb_brush_floor",
	                                                "cb_brush_pose_wlk",
	                                                "cb_brush_pose_jog",
	                                                "cb_brush_pose_run",
	                                                "cb_brush_U_turn",
	                                                "cb_ottotto_obj",
	                                                "cb_pose_keath_st",
	                                                "cb_pose_keath_lp",
	                                                "cb_pose_keath_en",
	                                                "cb_brush_floor_10f",
	                                                "cb_spider_st",
	                                                "cb_spider_trans",
	                                                "cb_spider_todome",
	                                                "cb_pose_tao",
	                                                "cb_pose_tao_lp",
	                                                "cb_bb_dan_hit",
	                                                "cb_brush_kime",
	                                                "cb_sibarare_spider",
	                                                "cb_sibarare_std",
	                                                "cb_sibarare_wlk",
	                                                "cb_sibarare_jog",
	                                                "cb_sibarare_run",
	                                                "cb_sibarare_U_turn",
	                                                "cb_sibarare_hodoku",
	                                                "cb_give_ITEM",
	                                                "cb_spoon_horu",
	                                                "cb_spoon_horenai",
	                                                "cb_U_turn_obj",
	                                                "cb_mag_kakureru",
	                                                "cb_mag_kakureru_lp",
	                                                "cb_mag_kakure_yameru",
	                                                "cb_bb_dan_hit_lift_obj",
	                                                "cb_bb_dan_hit_hold_tools",
	                                                "cb_kurakura",
	                                                "cb_spoon_horu_st",
	                                                "cb_mag_kime",
	                                                "cb_mag_kime_lp",
	                                                "cb_out_chibipot_lift",
	                                                "cb_in_chibipot_lift",
	                                                "cb_get_ITEM",
	                                                "cb_energy_0_1f",
	                                                "cb_yokotaore_getup",
	                                                "cb_maru_batu2",
	                                                "cb_tyu_suu_ekitai",
	                                                "cb_tyu_suu_mitu",
	                                                "cb_tyu_haku1",
	                                                "cb_tyu_haku2",
	                                                "cb_tyu_haku3",
	                                                "cb_tyu_haku_kuuki",
	                                                "cb_tyu_kime",
	                                                "cb_tyu_kime_1f",
	                                                "cb_tyu_haku1_1f",
	                                                "cb_tyu_haku2_1f",
	                                                "cb_tyu_haku3_1f",
	                                                "cb_pose_kaeru",
	                                                "cb_pose_kaeru_lp",
	                                                "cb_kaiten",
	                                                "cb_kaiten2",
	                                                "cb_saisun_st",
	                                                "cb_saisun_lp",
	                                                "cb_saisun_en",
	                                                "cb_tyu_suu_ekitai_st",
	                                                "cb_tyu_suu_ekitai_en",
	                                                "cb_saisun_lp2",
	                                                "cb_wlk_tenkan",
	                                                "cb_in_ufo",
	                                                "cb_sit_chair_st",
	                                                "cb_sit_chair",
	                                                "cb_sit_chair_en",
	                                                "cb_wait_B_lp_futa",
	                                                "cb_jump_down_en_03_futa",
	                                                "cb_jump_down_en_0301_futa",
	                                                "cb_run_slip_futa",
	                                                "cb_energy_0_futa",
	                                                "cb_go_up_the_step_low_obj_st",
	                                                "cb_go_up_the_step_low_obj_1f",
	                                                "cb_go_up_the_step_low_obj_en",
	                                                "cb_go_up_the_step_mid_obj_st",
	                                                "cb_go_up_the_step_mid_obj_1f",
	                                                "cb_go_up_the_step_mid_obj_en",
	                                                "cb_go_up_the_step_low_tools_st",
	                                                "cb_go_up_the_step_low_tools_1f",
	                                                "cb_go_up_the_step_low_tools_en",
	                                                "cb_go_up_the_step_mid_tools_st",
	                                                "cb_go_up_the_step_mid_tools_1f",
	                                                "cb_go_up_the_step_mid_tools_en",
	                                                "cb_go_up_the_step_hi_obj_st",
	                                                "cb_go_up_the_step_hi_obj_1f",
	                                                "cb_go_up_the_step_hi_obj_en",
	                                                "cb_go_up_the_step_hi_tools_st",
	                                                "cb_go_up_the_step_hi_tools_1f",
	                                                "cb_go_up_the_step_hi_tools_en",
	                                                "cb_jump_down_st_tools",
	                                                "cb_jump_down_1f_tools",
	                                                "cb_jump_down_en_tools",
	                                                "cb_jump_down_en_01_tools",
	                                                "cb_jump_down_en_02_tools",
	                                                "cb_jump_down_en_03_tools",
	                                                "cb_jump_down_en_0301_tools",
	                                                "cb_getoff_the_step8_tools",
	                                                "cb_getoff_the_step4_tools",
	                                                "cb_backwlk_tools",
	                                                "cb_umeru_tane",
	                                                "cb_rom_act",
	                                                "cb_jaguchi_on",
	                                                "cb_jaguchi_off",
	                                                "cb_pose_obake",
	                                                "cb_pose_obake_1",
	                                                "cb_rom_act_1",
	                                                "cb_pose_pajama",
	                                                "cb_pose_pajama_1",
	                                                "cb_pose_pajama_2",
	                                                "cb_pose_pajama_3",
	                                                "cb_me_patipati",
	                                                "cb_tv_channel",
	                                                "cb_in_ufo1",
	                                                "cb_in_ufo2",
	                                                "cb_lever_taosu",
	                                                "cb_maru_batu3",
	                                                "cb_kiki_stop",
	                                                "cb_pose_super",
	                                                "cb_pose_super_lp",
	                                                "cb_wait_gittyo",
	                                                "cb_wait_tao",
	                                                "cb_wait_kaeru",
	                                                "cb_wait_obake",
	                                                "cb_wait_pajama",
	                                                "cb_wait_super",
	                                                "cb_wait_kega",
	                                                "cb_junk_ireru",
	                                                "cb_kata_dekarobo",
	                                                "cb_std_lookup",
	                                                "cb_denchi_lever",
	                                                "cb_denchi_coin",
	                                                "cb_mugen",
	                                                "cb_tukamare_decahand",
	                                                "cb_plug_in_door_re",
	                                                "cb_plug_out_door_re",
	                                                "cb_plug_door_taiki_re",
	                                                "cb_plug_door_rot_re",
	                                                "cb_plug_door_rot_end_re",
	                                                "re_cb_brush_kamae_st",
	                                                "re_cb_brush_kamae_wait",
	                                                "re_cb_brush_kamae_end",
	                                                "re_cb_spoon_sasu",
	                                                "re_cb_spoon_sasu_wait",
	                                                "re_cb_spoon_horiageru",
	                                                "re_cb_lift_up_obj_st",
	                                                "re_cb_lift_up_obj_lp",
	                                                "re_cb_concent_kamae",
	                                                "re_cb_concent_kamae_wait",
	                                                "re_cb_plug_in_concent",
	                                                "re_cb_leticer_kamae",
	                                                "re_cb_leticer_kamae_wait",
	                                                "re_cb_plug_in_leticer",
	                                                "cb_wii_remocon_shake",
	                                                "cb_wii_remocon_shake_popper",
	                                                "cb_wii_remocon_shake_plug",
	                                                "cb_wii_remocon_shake_tools",
	                                                "cb_kurakura_popper",
	                                                "cb_kurakura_plug",
	                                                "cb_kurakura_tools",
	                                                NULL };
static const char* sKisekaePartsObakeAddAnims[] = { "obake_bero_wait", "obake_bero", "obake_bero_1", NULL };
static const char* sItemBrushAnims[]
    = { "brush_def", "brush_floor", "brush_U_turn", "brush_hold_def", "brush_floor_10f", "brush_kime", "brush_kime_1f", NULL };
static const char* sCbPropellerLv2Anims[]
    = { "propeller_def_lv2",   "propeller_st",           "propeller_lp_lv2",       "propeller_lp2_lv2",          "propeller_en",
	    "propeller_B_out_lv2", "propeller_B_rot_st_lv2", "propeller_B_rot_en_lv2", "propeller_B_in_lv2",         "wlk_propeller_lp",
	    "run_propeller_lp",    "std_propeller",          "propeller_B_idling_lv2", "propeller_B_idling_wlk_lv2", NULL };
static const char* sCbCannonLv2Anims[] = { "cannon_def_lv2",   "cb_cannon_out",      "cannon_in_lv2",
	                                       "cannon_ready_lv2", "cannon_aim_lv2",     "cannon_shoot_lv2",
	                                       "cb_cannon_yameru", "cannon_aim_wlk_lv2", NULL };
static const char* sCbRadarAnims[]     = { "radar_def", "radar_out", "radar_in", "radar_rot", NULL };
static const char* sItemMagCupAnims[]  = { "mag_def",     "mag_std",    "mag_out", "mag_in",  "mag_kakureru", "mag_kime",
	                                       "mag_kime_lp", "mag_wareru", "mag_wlk", "mag_jog", "mag_run",      NULL };
static const char* sItemSpoonAnims[]
    = { "spoon_def",  "spoon_std",     "spoon_horu",    "spoon_horenai",      "spoon_hold_def",     "spoon_horu_st",
	    "spoon_kime", "spoon_kime_1f", "re_spoon_sasu", "re_spoon_sasu_wait", "re_spoon_horiageru", NULL };
static const char* sItemTyuusyakiAnims[] = { "tyu_def",      "tyu_std",       "tyu_std1",          "tyu_std2",          "tyu_std3",
	                                         "tyu_hold_def", "tyu_hold_def1", "tyu_hold_def2",     "tyu_hold_def3",     "tyu_suu_ekitai",
	                                         "tyu_suu_mitu", "tyu_haku1",     "tyu_haku2",         "tyu_haku3",         "tyu_haku_kuuki",
	                                         "tyu_kime",     "tyu_kime_1f",   "tyu_suu_ekitai_st", "tyu_suu_ekitai_en", NULL };
static const char* sNpcConsentmanAnims[] = { "cs_def",     "cs_wait",     "cs_walk", "cs_jump", "cs_speak",    "cs_good",  "cs_door_push",
	                                         "cs_cs_wait", "cs_cs_speak", "cs_cccc", "cs_run",  "cs_run_jump", "cs_catch", NULL };
static const char* sNpcTonpyAnims[]      = { "tp_def",
	                                         "tp_wait",
	                                         "tp_talk",
	                                         "tp_mov_st",
	                                         "tp_mov_lp",
	                                         "tp_mov_en",
	                                         "tp_kenson_rot",
	                                         "tp_ukare_rot",
	                                         "tp_himo",
	                                         "tp_jomaku_tukamu",
	                                         "tp_jomaku",
	                                         "tp_mov_awate",
	                                         "tp_talk_awate",
	                                         "tp_kyorokyoro",
	                                         "tp_bikkuri",
	                                         "tp_wait_utumuki",
	                                         "tp_talk_utumuki",
	                                         "tp_look_tyoi_ue",
	                                         "tp_look_sugoku_ue",
	                                         "tp_namida",
	                                         "tp_bibiri",
	                                         "tp_talk_bibiri",
	                                         "tp_narasu_cracker",
	                                         "tp_aisatu",
	                                         "tp_hatena",
	                                         "tp_ibaru",
	                                         "tp_akire",
	                                         "tp_song_a",
	                                         "tp_song_b",
	                                         "tp_song_c",
	                                         "tp_song_d",
	                                         "tp_bibiri_up",
	                                         "tp_bibiri_otikomi",
	                                         "tp_wait_utumuki_ao",
	                                         "tp_talk_utumuki_ao",
	                                         "tp_tarai1",
	                                         "tp_tarai2",
	                                         "tp_tarai3",
	                                         "tp_kiki_stop",
	                                         "tp_kiku_sinzou",
	                                         "tp_kurakura",
	                                         NULL };
static const char* sItemTonpyItemGAnims[]
    = { "tp_isyou_def", "tp_isyou_song_a", "tp_isyou_song_b", "tp_isyou_song_c", "tp_isyou_song_d", NULL };
static const char* sNpcTaoAnims[]  = { "tao_def",
	                                   "tao_slp",
	                                   "tao_wlk",
	                                   "tao_get_up",
	                                   "tao_go_to_slp",
	                                   "tao_std",
	                                   "tao_slp_react_01",
	                                   "tao_growl",
	                                   "tao_growl_st",
	                                   "tao_growl_en",
	                                   "tao_slp_lookback",
	                                   "tao_std2",
	                                   "tao_std2_hoeru",
	                                   "tao_std2_uresii",
	                                   "tao_asi_table_st",
	                                   "tao_asi_table_lp",
	                                   "tao_asi_table_uresii",
	                                   "tao_asi_table_hoeru",
	                                   "tao_asi_table_present",
	                                   "tao_kamu_bb",
	                                   "tao_furimawasu_bb",
	                                   "tao_unaru_bb",
	                                   "tao_std2_R_L_face",
	                                   "tao_std2_L",
	                                   "tao_std2_R",
	                                   "tao_kaidan_suberi",
	                                   "tao_tobasu_gunsou",
	                                   "tao_kamituki_guwam",
	                                   "tao_mizunomu_1",
	                                   "tao_mizunomu_2",
	                                   "tao_run",
	                                   "tao_tobikomu_door",
	                                   "tao_osuwari",
	                                   "tao_osuwari_wait",
	                                   "tao_osuwari_wait2",
	                                   "tao_osuwari_syun",
	                                   "tao_osuwari_std",
	                                   "tao_asi_table_lp2",
	                                   "tao_mizukure_wait",
	                                   "tao_mizukure_hoe",
	                                   "tao_osuwari_wait3",
	                                   "tao_osuwari_wait3_hoe",
	                                   "tao_osuwari_wait_hoe",
	                                   "tao_std_miage",
	                                   "tao_miage_taiki",
	                                   "tao_std_syuui",
	                                   "tao_std_yorokobu",
	                                   "tao_std_jareru",
	                                   "tao_keikai",
	                                   "tao_keikai_hoe",
	                                   "tao_tyuusya",
	                                   "tao_std_jareru2",
	                                   "tao_osuwari_wait4",
	                                   "tao_tyuusya_wait",
	                                   "tao_haraheri_wait",
	                                   "tao_osuwari_syun_wait",
	                                   "tao_osuwari_woo",
	                                   "tao_osuwari_woo_wait",
	                                   "tao_osuwari_talk",
	                                   "tao_osuwari_syun2",
	                                   "tao_bibiri",
	                                   "tao_osuwari_to_keikai",
	                                   "tao_keikai_to_osuwari",
	                                   "tao_sibari_oti",
	                                   "tao_mizukure_kubi",
	                                   "tao_asi_table_lp2_kubi1",
	                                   "tao_asi_table_lp2_kubi2",
	                                   "tao_asi_table_lp2_kubi3",
	                                   NULL };
static const char* sNpcTaoSAnims[] = { "tao2_def",
	                                   "tao2_slp",
	                                   "tao2_wlk",
	                                   "tao2_get_up",
	                                   "tao2_go_to_slp",
	                                   "tao2_std",
	                                   "tao2_slp_react_01",
	                                   "tao2_growl",
	                                   "tao2_growl_st",
	                                   "tao2_growl_en",
	                                   "tao2_slp_lookback",
	                                   "tao2_std2",
	                                   "tao2_std2_hoeru",
	                                   "tao2_std2_uresii",
	                                   "tao2_asi_table_st",
	                                   "tao2_asi_table_lp",
	                                   "tao2_asi_table_uresii",
	                                   "tao2_asi_table_hoeru",
	                                   "tao2_asi_table_present",
	                                   "tao2_kamu_bb",
	                                   "tao2_furimawasu_bb",
	                                   "tao2_unaru_bb",
	                                   "tao2_std2_r_l_face",
	                                   "tao2_std2_l",
	                                   "tao2_std2_r",
	                                   "tao2_kaidan_suberi",
	                                   "tao2_tobasu_gunsou",
	                                   "tao2_kamituki_guwam",
	                                   "tao2_mizunomu_1",
	                                   "tao2_mizunomu_2",
	                                   "tao2_run",
	                                   "tao2_tobikomu_door",
	                                   "tao2_osuwari",
	                                   "tao2_osuwari_wait",
	                                   "tao2_osuwari_wait2",
	                                   "tao2_osuwari_syun",
	                                   "tao2_osuwari_std",
	                                   "tao2_asi_table_lp2",
	                                   "tao2_mizukure_wait",
	                                   "tao2_mizukure_hoe",
	                                   "tao2_osuwari_wait3",
	                                   "tao2_osuwari_wait3_hoe",
	                                   "tao2_osuwari_wait_hoe",
	                                   "tao2_std_miage",
	                                   "tao2_miage_taiki",
	                                   "tao2_std_syuui",
	                                   "tao2_std_yorokobu",
	                                   "tao2_std_jareru",
	                                   "tao2_keikai",
	                                   "tao2_keikai_hoe",
	                                   "tao2_tyuusya",
	                                   "tao2_std_jareru2",
	                                   "tao2_osuwari_wait4",
	                                   "tao2_tyuusya_wait",
	                                   "tao2_haraheri_wait",
	                                   "tao2_osuwari_syun_wait",
	                                   "tao2_osuwari_woo",
	                                   "tao2_osuwari_woo_wait",
	                                   "tao2_osuwari_talk",
	                                   "tao2_osuwari_syun2",
	                                   "tao2_haraheri_atama",
	                                   "tao2_haraheri_okiru",
	                                   NULL };
static const char* sNpcHockAnims[]
    = { "hc_def",  "hc_wait",  "hc_walk",  "hc_speak", "hc_drink",  "hc_drink2",  "hc_look_up", "hc_speak2",  "hc_fufu",
	    "hc_umu",  "hc_pose1", "hc_pose2", "hc_pose3", "hc_glad",   "hc_coin1",   "hc_coin2",   "hc_coin3",   "hc_cry",
	    "hc_sit1", "hc_sit2",  "hc_sit3",  "hc_throw", "hc_speak3", "hc_camera1", "hc_camera2", "hc_camera3", NULL };
static const char* sNpcJennyAnims[] = { "jn_def",
	                                    "jn_wait",
	                                    "jn_walk",
	                                    "jn_sit1",
	                                    "jn_sit2",
	                                    "jn_sit3",
	                                    "jn_fasit_wait",
	                                    "jn_fasit_speak",
	                                    "jn_sit_hand",
	                                    "jn_kg_wait",
	                                    "jn_kg_speak",
	                                    "jn_sit_sleep",
	                                    "jn_eat",
	                                    "jn_hakusyu",
	                                    "jn_sit_trip",
	                                    "jn_fasit_set",
	                                    "jn_wait_sowa",
	                                    "jn_sit_glad",
	                                    "jn_sit_wait",
	                                    "jn_glad2",
	                                    "jn_sit_speak",
	                                    "jn_sit_table",
	                                    "jn_hand",
	                                    "jn_hand_speak",
	                                    "jn_sit_hand_spk",
	                                    "jn_sit_nose4",
	                                    "jn_aa",
	                                    "jn_wait_up",
	                                    "jn_oek1",
	                                    "jn_oek2",
	                                    "jn_sit_hand_syn",
	                                    "jn_sit_hand_ojg",
	                                    "jn_sit_hand_kiss",
	                                    "jn_oek_speak",
	                                    "jn_sit_nose1",
	                                    "jn_sit_nose2",
	                                    "jn_sit_nose3",
	                                    "jn_oek_wait",
	                                    "jn_tb_wait",
	                                    "jn_tb_speak",
	                                    "jn_tb_glad",
	                                    "jn_tb_ojigi",
	                                    "jn_tb_look",
	                                    "jn_tb_look2",
	                                    "jn_tb_look3",
	                                    "jn_tb_cake",
	                                    "jn_tb_haku",
	                                    "jn_tb_wait_r",
	                                    "jn_tb_wait_l",
	                                    "jn_tb_ojigi_r",
	                                    "jn_tb_ojigi_l",
	                                    "jn_tb_rot_r1",
	                                    "jn_tb_rot_r2",
	                                    "jn_tb_rot_l1",
	                                    "jn_tb_rot_l2",
	                                    "jn_tb_wait2",
	                                    "jn_tb_wait_r2",
	                                    "jn_tb_wait_l2",
	                                    "jn_kd_wait",
	                                    "jn_kd_come",
	                                    "jn_tb_no",
	                                    "jn_tb_pre",
	                                    "jn_tb_wait3",
	                                    "jn_tb_look_dw",
	                                    "jn_fuan_wait",
	                                    "jn_fuan_speak",
	                                    "jn_fuan_door1",
	                                    "jn_fuan_door2",
	                                    "jn_fuan_cb1",
	                                    "jn_fuan_cb2",
	                                    "jn_loop",
	                                    "jn_kd_cry",
	                                    "jn_sit_hand_glad",
	                                    "jn_sit_hand_look",
	                                    "jn_dr_run",
	                                    "jn_sf_push",
	                                    "jn_cus_wait",
	                                    "jn_kaeru_off",
	                                    "jn_kaeru_on",
	                                    "jn_sit_album",
	                                    "jn_rakka",
	                                    NULL };
static const char* sNpcMamaAnims[]  = { "ma_def",          "ma_wait",          "ma_walk",       "ma_sit1",        "ma_sit2",
	                                    "ma_sit3",         "ma_sit_blue",      "ma_misin",      "ma_saisun1",     "ma_saisun2",
	                                    "ma_saisun3",      "ma_tb_hand1",      "ma_tb_hand2",   "ma_shd_surp",    "ma_hand",
	                                    "ma_shd_wait2",    "ma_shd_speak2",    "ma_shd_bero",   "ma_dead",        "ma_tb_sleep",
	                                    "ma_tb_sleep_up",  "ma_hz_sleep",      "ma_hz_wait",    "ma_hz_speak",    "ma_hz_speak2",
	                                    "ma_hz_look",      "ma_cf_wait",       "ma_cf_drink",   "ma_cf_speak",    "ma_sink",
	                                    "ma_hand_off1",    "ma_hand_off2",     "ma_shd_wait",   "ma_shd_speak",   "ma_hz_look2",
	                                    "ma_hz_look3",     "ma_hz_haku",       "ma_hz_speak3",  "ma_sink_speak",  "ma_hz_wait2",
	                                    "ma_hzbs_wait",    "ma_hzbs_take",     "ma_hzbs_take2", "ma_hzbs_speak1", "ma_hzbs_speak2",
	                                    "ma_hzbs_wait2",   "ma_dr_run",        "ma_spider1",    "ma_spider2",     "ma_sf_push",
	                                    "ma_papa_hd",      "ma_papa_hd2",      "ma_papa_hd3",   "ma_cus_wait",    "ma_sit_manzara",
	                                    "ma_hz_fear",      "ma_look_around",   "ma_kiss_dw",    "ma_kiss_wait",   "ma_kiss_up",
	                                    "ma_kiss_up_wait", "ma_kiss_up_speak", "ma_kiss",       "ma_ouen",        "ma_hz_flypan1",
	                                    "ma_hz_flypan2",   "ma_shd_wink",      "ma_hakusyu",    "ma_speak_aseri", "ma_sit_speak",
	                                    "ma_hz_hello",     "ma_hz_care_r1",    "ma_hz_care_r2", "ma_hz_care_r3",  NULL };
static const char* sNpcArmy1Anims[] = { "am1_def",          "am1_wait",
	                                    "am1_walk",         "am1_run",
	                                    "am1_kaiten_f",     "am1_kaiten_l",
	                                    "am1_kaiten_r",     "am1_gun1",
	                                    "am1_gun2",         "am1_gun3",
	                                    "am1_dead",         "am1_dead2",
	                                    "am1_dead3",        "am1_jump1",
	                                    "am1_jump2",        "am1_jump3",
	                                    "am1_attack1",      "am1_attack2",
	                                    "am1_attack3",      "am1_dead_long",
	                                    "am1_speak",        "am1_keirei",
	                                    "am1_ooo",          "am1_go1",
	                                    "am1_go2",          "am1_speak2",
	                                    "am1_fuse1",        "am1_fuse2",
	                                    "am1_fuse3",        "am1_fuse_shout",
	                                    "am1_look_wait",    "am1_look_good",
	                                    "am1_surp",         "am1_surp2",
	                                    "am1_me",           "am1_st_sleep",
	                                    "am1_st_akubi",     "am1_st_wakeup",
	                                    "am1_otikomi_wait", "am1_otikomi_speak",
	                                    "am1_otikomi_walk", "am1_otikomi_up1",
	                                    "am1_otikomi_up2",  "am1_otikomi_up3",
	                                    "am1_wall_up",      "am1_surp3",
	                                    "am1_awate_run",    "am1_awate_wait",
	                                    "am1_awate_speak",  "am1_otikomi_kuyasi",
	                                    "am1_keirei_up",    "am1_namida1",
	                                    "am1_namida2",      "am1_namida3",
	                                    "am1_surp_glad",    "am1_cry_on",
	                                    "am1_cry_wait",     "am1_cry_speak",
	                                    "am1_cry_walk",     "am1_hug1",
	                                    "am1_hug2",         NULL };
static const char* sNpcArmy2Anims[]
    = { "am2_def",     "am2_wait",     "am2_walk",       "am2_run",          "am2_kaiten_f",    "am2_kaiten_l",   "am2_kaiten_r",
	    "am2_gun1",    "am2_gun2",     "am2_gun3",       "am2_dead",         "am2_dead2",       "am2_dead3",      "am2_jump1",
	    "am2_jump2",   "am2_jump3",    "am2_attack1",    "am2_attack2",      "am2_attack3",     "am2_dead_long",  "am2_speak",
	    "am2_keirei",  "am2_ooo",      "am2_go1",        "am2_go2",          "am2_speak2",      "am2_wall_l",     "am2_wall_r",
	    "am2_wall_up", "am2_st_sleep", "am2_st_akubi",   "am2_st_wakeup",    "am2_gun4",        "am2_cup_wait",   "am2_cup_walk",
	    "am2_cup_up1", "am2_cup_up2",  "am2_fuse1",      "am2_fuse2",        "am2_fuse3",       "am2_udetate",    "am2_fuckin",
	    "am2_earobi",  "am2_run_hand", "am2_tao",        "am2_keirei_no1",   "am2_keirei_no2",  "am2_keirei_no3", "am2_walk_silent",
	    "am2_surp",    "am2_surp2",    "am2_surp3",      "am2_bibiru1",      "am2_bibiru2",     "am2_bibiru3",    "am2_bibiru4",
	    "am2_haku",    "am2_glad",     "am2_otikomi_on", "am2_otikomi_wait", "am2_otikomi_off", "am2_hug1",       "am2_hug2",
	    NULL };
static const char* sNpcArmy3Anims[]
    = { "am3_def",    "am3_wait", "am3_walk", "am3_run", "am3_dead",   "am3_dead2",    "am3_dead3",     "am3_dead_long", "am3_speak",
	    "am3_keirei", "am3_ooo",  "am3_go1",  "am3_go2", "am3_speak2", "am3_fly_wait", "am3_fly_speak", "am3_fly_ooo",   NULL };
static const char* sNpcArmy4Anims[]
    = { "am4_def",       "am4_wait",      "am4_walk",     "am4_run",         "am4_surp", "am4_surp2",      "am4_haku",
	    "am4_dead_long", "am4_speak",     "am4_keirei",   "am4_fune_keirei", "am4_glad", "am4_fune_speak", "am4_fune_ooo",
	    "am4_fune",      "am4_fune_wait", "am4_st_sleep", "am4_stop",        NULL };
static const char* sNpcArmy5Anims[]
    = { "am5_def", "am5_wait", "am5_walk", "am5_run", "am5_speak", "am5_speak2", "am5_car", "am5_twist", NULL };
static const char* sNpcDecaRoboAnims[]  = { "dr_def",         "dr_wait",        "dr_taore",
	                                        "dr_gata",        "dr_kidou",       "dr_gata2",
	                                        "dr_stand",       "dr_wait2",       "dr_stand_no",
	                                        "dr_walk_r",      "dr_walk_l",      "dr_walk_move",
	                                        "dr_walk_move2",  "dr_wait_r",      "dr_sit1",
	                                        "dr_sit2",        "dr_sit3",        "dr_wait_nopanel",
	                                        "dr_kaidan_move", "dr_sit_energy1", "dr_sit_energy2",
	                                        "dr_sit_energy3", "dr_sit_energy4", "dr_sit_energy5",
	                                        "dr_sit_energy6", "dr_sit_energy7", "dr_sit_ojigi",
	                                        "dr_ojigi",       "dr_sit_put",     "dr_infinity1",
	                                        "dr_infinity2",   "dr_infinity3",   NULL };
static const char* sNpcDecaRobo2Anims[] = { "dr2_def", "dr2_wait", "dr2_ar_wait", "dr2_sofa_wait", "dr2_br_wait", "dr2_br_play", NULL };
static const char* sNpcKeathAnims[]
    = { "kaya",          "kt_wait",       "kt_walk",       "kt_speak",      "kt_ps1",        "kt_ps2",
	    "kt_ps3",        "kt_watch",      "kt_stop",       "kt_run",        "kt_jump",       "kt_pose_sp",
	    "kt_speak2",     "kt_jump_sp1",   "kt_jump_sp2",   "kt_jump_sp3",   "kt_look_l",     "kt_look_r",
	    "kt_look_d",     "kt_speak3",     "kt_wait2",      "kt_gift",       "kt_pose_sp2",   "kt_run_start",
	    "kt_watch_ok",   "kt_dw_watch",   "kt_barrier1",   "kt_barrier2",   "kt_barrier3",   "kt_fl_wait",
	    "kt_fl_walk",    "kt_fl_speak",   "kt_fl_gift1",   "kt_fl_gift2",   "kt_fl_gift3",   "kt_stop2",
	    "kt_bri1",       "kt_bri2",       "kt_bri3",       "kt_surp1",      "kt_surp2",      "kt_surp3",
	    "kt_br_damage1", "kt_br_damage2", "kt_br_damage3", "kt_br_damage4", "kt_br_damage5", "kt_br_damage6",
	    "kt_kayaku1",    "kt_kayaku2",    "kt_kayaku3",    "kt_nayamu1",    "kt_nayamu2",    NULL };
static const char* sNpcPapaAnims[] = { "pa_def",          "pa_wait",
	                                   "pa_walk",         "pa_sit1",
	                                   "pa_sit2",         "pa_sit3",
	                                   "pa_au_wait",      "pa_au_speak",
	                                   "pa_oh",           "pa_sorry",
	                                   "pa_oh2",          "pa_au_speak2",
	                                   "pa_au_surp",      "pa_walk_dogi",
	                                   "pa_goso1",        "pa_goso2",
	                                   "pa_shd_wait",     "pa_shd_speak",
	                                   "pa_shd_happy",    "pa_shd_cry",
	                                   "pa_slp_book1",    "pa_slp_book2",
	                                   "pa_slp_book3",    "pa_oh_start",
	                                   "pa_oh_end",       "pa_haku",
	                                   "pa_sit_st_oh",    "pa_slp_book4",
	                                   "pa_oh_up",        "pa_au_speak3",
	                                   "pa_au_surp1",     "pa_au_surp2",
	                                   "pa_au_surp3",     "pa_au_botan",
	                                   "pa_au_botan2",    "pa_au_botan3",
	                                   "pa_wait_dogi",    "pa_mizumaki",
	                                   "pa_loop",         "pa_sit_play",
	                                   "pa_sit_play2",    "pa_dead",
	                                   "pa_ha1",          "pa_ha2",
	                                   "pa_dr_kick1",     "pa_dr_kick2",
	                                   "pa_dr_kick3",     "pa_dr_run",
	                                   "pa_oh_up2",       "pa_dr_run2",
	                                   "pa_tb_push",      "pa_mama_hd",
	                                   "pa_mama_hd2",     "pa_mama_hd3",
	                                   "pa_kg_wait",      "pa_kg_walk",
	                                   "pa_kg_speak",     "pa_kg_up",
	                                   "pa_kg_up_wait",   "pa_kg_up_speak",
	                                   "pa_cus_wait",     "pa_cus_bati",
	                                   "pa_cus_outwait",  "pa_cus_on",
	                                   "pa_cus_off",      "pa_hena1",
	                                   "pa_hena2",        "pa_block_wait",
	                                   "pa_block_speak",  "pa_eat",
	                                   "pa_cook",         "pa_kiss_dw",
	                                   "pa_kiss_wait",    "pa_kiss_up",
	                                   "pa_kiss_up_wait", "pa_kiss_up_speak",
	                                   "pa_kiss",         "pa_ar_wait",
	                                   "pa_sit_play3",    "pa_cus_start",
	                                   "pa_kyoro",        NULL };
static const char* sNpcFafaAnims[] = { "fa_def",
	                                   "fa_wait",
	                                   "fa_walk",
	                                   "fa_speak",
	                                   "fa_stop",
	                                   "fa_stop2",
	                                   "fa_mitsu",
	                                   "fa_ch_ab",
	                                   "fa_ch_ba",
	                                   "fa_wait_ang",
	                                   "fa_walk_ang",
	                                   "fa_speak_ang",
	                                   "fa_ch_aa",
	                                   "fa_glad",
	                                   "fa_door1",
	                                   "fa_door3",
	                                   "fa_wait_look",
	                                   "fa_walk_look",
	                                   "fa_just_look",
	                                   "fa_think",
	                                   "fa_door_wait",
	                                   "fa_door_speak",
	                                   "fa_door2",
	                                   "fa_ang_jump1",
	                                   "fa_violence",
	                                   "fa_violence2",
	                                   "fa_hiku",
	                                   "fa_mitsu2",
	                                   "fa_clean",
	                                   "fa_happy_ang",
	                                   "fa_syobon",
	                                   "fa_ch_ac",
	                                   "fa_wait_hi",
	                                   "fa_speak_hi",
	                                   "fa_kamae_ang1",
	                                   "fa_kamae_ang2",
	                                   "fa_warau_hi1",
	                                   "fa_warau_hi2",
	                                   "fa_warau_hi3",
	                                   "fa_walk_hi",
	                                   "fa_clean_hi",
	                                   "fa_warau_hi1ac",
	                                   NULL };
static const char* sNpcSpider1Anims[]
    = { "sp1_def",      "sp1_wait",    "sp1_walk",      "sp1_run",     "sp1_up",        "sp1_walk_up",     "sp1_kaiten_l",
	    "sp1_kaiten_r", "sp1_back",    "sp1_attack1",   "sp1_attack2", "sp1_attack3",   "sp1_attack4",     "sp1_attack_l",
	    "sp1_attack_r", "sp1_look",    "sp1_hm_down",   "sp1_hm_wait", "sp1_hm_jump1",  "sp1_hm_jump2",    "sp1_hm_jump3",
	    "sp1_damage",   "sp1_damage2", "sp1_damage3",   "sp1_shot",    "sp1_end",       "sp1_attack3furi", "sp1_case1",
	    "sp1_case2",    "sp1_jump",    "sp1_hm_dcfoot", "sp1_face",    "sp1_face_jump", "sp1_hm_dcfoot2",  NULL };
static const char* sNpcSpider2Anims[] = { "sp2_def",       "sp2_wait",     "sp2_walk",        "sp2_at_start",  "sp2_at_jump1",
	                                      "sp2_at_stick",  "sp2_at_jump2", "sp2_at_end",      "sp2_ten_wait",  "sp2_ten_jump1",
	                                      "sp2_ten_jump2", "sp2_run",      "sp2_jump_sonoba", "sp2_jump_move", NULL };
static const char* sNpcSpider3Anims[] = { "sp3_def",
	                                      "sp3_wait",
	                                      "sp3_walk",
	                                      "sp3_saka_wait",
	                                      "sp3_saka_oti1",
	                                      "sp3_saka_oti2",
	                                      "sp3_ikaku",
	                                      "sp3_wait2",
	                                      "sp3_kaiten_l",
	                                      "sp3_kaiten_r",
	                                      "sp3_damage1",
	                                      "sp3_damage2",
	                                      "sp3_mg_on",
	                                      "sp3_mg_wait",
	                                      "sp3_mg_kaiten_l",
	                                      "sp3_mg_kaiten_r",
	                                      "sp3_mgat_on",
	                                      "sp3_mgat_wait",
	                                      "sp3_mgat_kaiten_l",
	                                      "sp3_mgat_kaiten_r",
	                                      "sp3_dead",
	                                      "sp3_dead2",
	                                      "sp3_ikaku2",
	                                      "sp3_wait3",
	                                      "sp3_run",
	                                      "sp3_kaiten_l2",
	                                      "sp3_kaiten_r2",
	                                      "sp3_gun",
	                                      NULL };
static const char* sNpcSpider4Anims[]
    = { "sp4_def",      "sp4_wait",    "sp4_walk",      "sp4_run",     "sp4_up",        "sp4_walk_up",     "sp4_kaiten_l",
	    "sp4_kaiten_r", "sp4_back",    "sp4_attack1",   "sp4_attack2", "sp4_attack3",   "sp4_attack4",     "sp4_attack_l",
	    "sp4_attack_r", "sp4_look",    "sp4_hm_down",   "sp4_hm_wait", "sp4_hm_jump1",  "sp4_hm_jump2",    "sp4_hm_jump3",
	    "sp4_damage",   "sp4_damage2", "sp4_damage3",   "sp4_shot",    "sp4_end",       "sp4_attack3furi", "sp4_case1",
	    "sp4_case2",    "sp4_jump",    "sp4_hm_dcfoot", "sp4_face",    "sp4_face_jump", "sp4_hm_dcfoot2",  NULL };
static const char* sNpcUfoAnims[] = { "ufo_def",   "ufo_wait",      "ufo_start",     "ufo_spot1",     "ufo_spot2", "ufo_spot3",
	                                  "ufo_spot4", "ufo_light_dw1", "ufo_light_dw2", "ufo_light_dw3", NULL };
static const char* sNpcUfo2Anims[]
    = { "ufo2_def", "ufo2_start", "ufo2_spot1", "ufo2_spot2", "ufo2_light_dw1", "ufo2_light_dw2", "ufo2_wait", NULL };
static const char* sNpcEt1Anims[]     = { "et1_def",          "et1_wait",         "et1_walk",
	                                      "et1_run",          "et1_speak",        "et1_speak2",
	                                      "et1_wait_up",      "et1_gaku1",        "et1_gaku2",
	                                      "et1_cold_wait",    "et1_cold_walk",    "et1_cold_speak",
	                                      "et1_cold_down",    "et1_cold2_wait",   "et1_cold2_speak",
	                                      "et1_cold2_up",     "et1_hot",          "et1_dameda",
	                                      "et1_gaku3",        "et1_gaku4",        "et1_cold2_water",
	                                      "et1_cold2_noisy1", "et1_cold2_noisy2", "et1_reaction1",
	                                      "et1_reaction2",    "et1_reaction3",    "et1_beam1",
	                                      "et1_beam2",        "et1_beam3",        "et1_beam4",
	                                      "et1_beam5",        "et1_beam4_long",   NULL };
static const char* sNpcSunMidaAnims[] = { "sm_def",       "sm_wait",       "sm_walk",  "sm_speak", "sm_stop",    "sm_box_wait",
	                                      "sm_box_speak", "sm_box_up",     "sm_shop1", "sm_shop2", "sm_shop3",   "sm_sr_wait",
	                                      "sm_sr_speak",  "sm_shop_speak", "sm_fire1", "sm_fire2", "sm_sakasa1", "sm_sakasa2",
	                                      "sm_inori",     "sm_surp",       "sm_surp2", "sm_haku",  NULL };
static const char* sNpcSunKaoAnims[]
    = { "smk_def",     "smk_wait_cl", "smk_wait_l", "smk_wait_r", "smk_open_l", "smk_open_r", "smk_speak_cl",
	    "smk_speak_l", "smk_speak_r", "smk_wait",   "smk_speak",  "smk_close",  "smk_drum",   NULL };
static const char* sNpcBbAnims[]     = { "bb_def",
	                                     "bb_omotya_def",
	                                     "bb_omotya_hasamaru_reaction",
	                                     "bb_omotya_reaction",
	                                     "bb_chara_hasamaru",
	                                     "bb_chara_hasamaru_reaction",
	                                     "bb_chara_hasamaru_nikki_st",
	                                     "bb_chara_hasamaru_nikki_lp",
	                                     "bb_nikki_wait",
	                                     "bb_chara_hasamaru_nikki_kiduku",
	                                     "x",
	                                     "bb_chara_hasamaru_nikki_simau",
	                                     "bb_chara_hasamaru_donaru",
	                                     "bb_chara_hasamaru_reisei",
	                                     "x",
	                                     "bb_chara_hasamaru_hatena",
	                                     "bb_chara_hasamaru_bikkuri",
	                                     "bb_chara_hasamaru_mojimoji",
	                                     "bb_chara_hasamaru_hazukasi",
	                                     "bb_chara_hasamaru_talk",
	                                     "x",
	                                     "bb_chara_hasamaru_hajimemasite",
	                                     "bb_chara_hasamaru_sekimen",
	                                     "bb_chara_hasamaru_hikkomu",
	                                     "bb_yoko_taiki_reaction",
	                                     "bb_stand_up",
	                                     "bb_std_taiki",
	                                     "bb_std_donaru",
	                                     "bb_std_reisei",
	                                     "x",
	                                     "bb_std_hatena",
	                                     "bb_std_hazukasi",
	                                     "bb_std_tenkan",
	                                     "bb_std_talk_hanikami",
	                                     "bb_std_talk_kusou",
	                                     "bb_std_talk_kusou_wait",
	                                     "bb_std_utagau",
	                                     "bb_sit_down",
	                                     "bb_wlk",
	                                     "bb_odoroku1",
	                                     "bb_odoroku2",
	                                     "bb_odoroku2_wait",
	                                     "bb_odoroku3",
	                                     "bb_tegami_watasu",
	                                     "bb_std_heart",
	                                     "bb_tyou_kizetu",
	                                     "bb_odoroku_kurukuru",
	                                     "bb_chara_hasamaru_utagau",
	                                     "bb_kaidan_nobori",
	                                     "bb_kaidan_nobori_taiki",
	                                     "bb_kaidan_oriru",
	                                     "bb_pen_denai",
	                                     "bb_odoroku3_kurukuru",
	                                     "bb_douyou",
	                                     "bb_miokuru_gittyo",
	                                     "bb_miokuru_gittyo_1",
	                                     "bb_miokuru_gittyo_2",
	                                     "bb_std_aisatu",
	                                     NULL };
static const char* sNpcTyouBbAnims[] = { "tyou_bb_def", "tyou_bb_syouten1", "tyou_bb_syouten2", "tyou_bb_fukki", "tyou_bb_fry", NULL };
static const char* sNpcPrimoAnims[]
    = { "pr_def",    "pr_wait", "pr_walk",     "pr_speak",   "pr_stop",     "pr_omc_speak", "pr_sit_wait", "pr_speak2",
	    "pr_listen", "pr_box",  "pr_box_stay", "pr_wait_dw", "pr_speak_dw", "pr_speak2_dw", NULL };
static const char* sNpcFunkyFAnims[]
    = { "ff_def",       "ff_wait",        "ff_walk",     "ff_speak",    "ff_stop",     "ff_dc_pan",    "ff_dc_wave",
	    "ff_dc_scat",   "ff_dc_link",     "ff_dc_furi",  "ff_dc_up",    "ff_dc_end",   "ff_mt_on",     "ff_mt_wait",
	    "ff_mt_speak",  "ff_mt_look",     "ff_gc_wait",  "ff_gc_wait2", "ff_gc_pose1", "ff_gc_pose2",  "ff_gc_pose3",
	    "ff_gc_pose4",  "ff_gc_beam1",    "ff_gc_beam2", "ff_gc_simei", "ff_gc_go",    "ff_gc_what",   "ff_gc_earth",
	    "ff_dc_end2",   "ff_tp_gomakasi", "ff_tp_kiki",  "ff_tp_tere",  "ff_tonpy",    "ff_brw_wait",  "ff_brw_speak",
	    "ff_brw_on",    "ff_brw_off",     "ff_brw_open", "ff_look_up",  "ff_wait_up",  "ff_look_down", "ff_brw_open2",
	    "ff_brw_open3", "ff_jump",        "ff_record",   "ff_syoukai1", "ff_syoukai2", "ff_syoukai3",  "ff_brw_look1",
	    "ff_brw_look2", "ff_dead",        "ff_dead_on",  "ff_dead_off", NULL };
static const char* sNpcFunkyF2Anims[] = { "ff2_def",      "ff2_wait",     "ff2_walk",
	                                      "ff2_speak",    "ff2_stop",     "ff2_dc_pan",
	                                      "ff2_dc_wave",  "ff2_dc_scat",  "ff2_dc_link",
	                                      "ff2_dc_furi",  "ff2_dc_up",    "ff2_dc_end",
	                                      "non",          "non",          "non",
	                                      "non",          "ff2_gc_wait",  "ff2_gc_wait2",
	                                      "ff2_gc_pose1", "ff2_gc_pose2", "ff2_gc_pose3",
	                                      "ff2_gc_pose4", "ff2_gc_beam1", "ff2_gc_beam2",
	                                      "ff2_gc_simei", "ff2_gc_go",    "ff2_gc_what",
	                                      "ff2_gc_earth", "non",          "non",
	                                      "non",          "non",          "ff2_tonpy",
	                                      "ff2_papy",     "ff2_sleep",    "ff2_oti_on",
	                                      "ff2_oti_wait", "ff2_oti_off",  NULL };
static const char* sNpcFunkyF3Anims[]
    = { "ff3_def",      "ff3_wait",     "ff3_walk",     "ff3_speak",     "ff3_stop",      "ff3_dc_pan",    "ff3_dc_wave",
	    "ff3_dc_scat",  "ff3_dc_link",  "ff3_dc_furi",  "ff3_dc_up",     "ff3_dc_end",    "non",           "non",
	    "non",          "non",          "ff3_gc_wait",  "ff3_gc_wait2",  "ff3_gc_pose1",  "ff3_gc_pose2",  "ff3_gc_pose3",
	    "ff3_gc_pose4", "ff3_gc_beam1", "ff3_gc_beam2", "ff3_gc_simei",  "ff3_gc_go",     "ff3_gc_what",   "ff3_gc_earth",
	    "ff3_kk_stop",  "non",          "non",          "non",           "ff3_tonpy",     "ff3_papy",      "ff3_sleep",
	    "ff3_kk_wait",  "ff3_kk_speak", "ff3_kk_walk",  "ff3_kk_kenka1", "ff3_kk_kenka2", "ff3_kk_kenka3", "ff3_bottan",
	    "ff3_run_move", "ff3_taore1",   "ff3_taore2",   "ff3_oti_on",    "ff3_oti_wait",  "ff3_oti_off",   NULL };
static const char* sNpcFunkyF2BAnims[]
    = { "ff2b_def",      "ff2b_wait",     "ff2b_walk",     "ff2b_speak",    "ff2b_stop",     "ff2b_dc_pan",
	    "ff2b_dc_wave",  "ff2b_dc_scat",  "ff2b_dc_link",  "ff2b_dc_furi",  "ff2b_dc_up",    "ff2b_dc_end",
	    "ff2b_syoukai1", "ff2b_syoukai2", "ff2b_syoukai3", "ff2b_oti_wait", "ff2b_gc_wait",  "ff2b_gc_wait2",
	    "ff2b_gc_pose1", "ff2b_gc_pose2", "ff2b_gc_pose3", "ff2b_gc_pose4", "ff2b_gc_beam1", "ff2b_gc_beam2",
	    "ff2b_gc_simei", "ff2b_gc_go",    "ff2b_gc_what",  "ff2b_gc_earth", "non",           "non",
	    "non",           "non",           "ff2b_tonpy",    "ff2b_papy",     "ff2b_sleep",    NULL };
static const char* sNpcFunkyF3BAnims[]
    = { "ff3b_def",      "ff3b_wait",     "ff3b_walk",     "ff3b_speak",    "ff3b_stop",     "ff3b_dc_pan",
	    "ff3b_dc_wave",  "ff3b_dc_scat",  "ff3b_dc_link",  "ff3b_dc_furi",  "ff3b_dc_up",    "ff3b_dc_end",
	    "ff3b_syoukai1", "ff3b_syoukai2", "ff3b_syoukai3", "ff3b_oti_wait", "ff3b_gc_wait",  "ff3b_gc_wait2",
	    "ff3b_gc_pose1", "ff3b_gc_pose2", "ff3b_gc_pose3", "ff3b_gc_pose4", "ff3b_gc_beam1", "ff3b_gc_beam2",
	    "ff3b_gc_simei", "ff3b_gc_go",    "ff3b_gc_what",  "ff3b_gc_earth", "non",           "non",
	    "non",           "non",           "ff3b_tonpy",    "ff3b_papy",     "ff3b_sleep",    NULL };
static const char* sNpcFunkyFAllAnims[]  = { "ffall_def",       "ffall_wait",        "ffall_walk",
	                                         "ffall_speak",     "ffall_stop",        "ffall_dc_pan",
	                                         "ffall_dc_wave",   "ffall_dc_scat",     "ffall_dc_link",
	                                         "ffall_dc_furi",   "ffall_dc_up",       "ffall_dc_end",
	                                         "ffall_mt_on",     "ffall_mt_wait",     "ffall_mt_speak",
	                                         "ffall_mt_look",   "ffall_gc_wait",     "ffall_gc_wait2",
	                                         "ffall_gc_pose1",  "ffall_gc_pose2",    "ffall_gc_pose3",
	                                         "ffall_gc_pose4",  "ffall_gc_beam1",    "ffall_gc_beam2",
	                                         "ffall_gc_simei",  "ffall_gc_go",       "ffall_gc_what",
	                                         "ffall_gc_earth",  "ffall_tp_sukiyaki", "ffall_tp_gomakasi",
	                                         "ffall_tp_kiki",   "ffall_tp_tere",     "ffall_tonpy",
	                                         "ffall_brw_wait",  "ffall_brw_speak",   "ffall_brw_on",
	                                         "ffall_brw_off",   "ffall_brw_open",    "ffall_look_up",
	                                         "ffall_wait_up",   "ffall_look_down",   "ffall_brw_open2",
	                                         "ffall_brw_open3", "ffall_jump",        "ffall_record",
	                                         "ffall_syoukai1",  "ffall_syoukai2",    "ffall_syoukai3",
	                                         "ffall_brw_look1", "ffall_brw_look2",   NULL };
static const char* sNpcFunkyF2AllAnims[] = { "ff2all_def",
	                                         "ff2all_wait",
	                                         "ff2all_walk",
	                                         "ff2all_speak",
	                                         "ff2all_stop",
	                                         "ff2all_dc_pan",
	                                         "ff2all_dc_wave",
	                                         "ff2all_dc_scat",
	                                         "ff2all_dc_link",
	                                         "ff2all_dc_furi",
	                                         "ff2all_dc_up",
	                                         "ff2all_dc_end",
	                                         "non",
	                                         "non",
	                                         "non",
	                                         "non",
	                                         "ff2all_gc_wait",
	                                         "ff2all_gc_wait2",
	                                         "ff2all_gc_pose1",
	                                         "ff2all_gc_pose2",
	                                         "ff2all_gc_pose3",
	                                         "ff2all_gc_pose4",
	                                         "ff2all_gc_beam1",
	                                         "ff2all_gc_beam2",
	                                         "ff2all_gc_simei",
	                                         "ff2all_gc_go",
	                                         "ff2all_gc_what",
	                                         "ff2all_gc_earth",
	                                         "non",
	                                         "non",
	                                         "non",
	                                         "non",
	                                         "ff2all_tonpy",
	                                         "ff2all_papy",
	                                         "ff2all_sleep",
	                                         NULL };
static const char* sNpcFunkyF3AllAnims[] = { "ff3all_def",
	                                         "ff3all_wait",
	                                         "ff3all_walk",
	                                         "ff3all_speak",
	                                         "ff3all_stop",
	                                         "ff3all_dc_pan",
	                                         "ff3all_dc_wave",
	                                         "ff3all_dc_scat",
	                                         "ff3all_dc_link",
	                                         "ff3all_dc_furi",
	                                         "ff3all_dc_up",
	                                         "ff3all_dc_end",
	                                         "non",
	                                         "non",
	                                         "non",
	                                         "non",
	                                         "ff3all_gc_wait",
	                                         "ff3all_gc_wait2",
	                                         "ff3all_gc_pose1",
	                                         "ff3all_gc_pose2",
	                                         "ff3all_gc_pose3",
	                                         "ff3all_gc_pose4",
	                                         "ff3all_gc_beam1",
	                                         "ff3all_gc_beam2",
	                                         "ff3all_gc_simei",
	                                         "ff3all_gc_go",
	                                         "ff3all_gc_what",
	                                         "ff3all_gc_earth",
	                                         "non",
	                                         "non",
	                                         "non",
	                                         "non",
	                                         "ff3all_tonpy",
	                                         "ff3all_papy",
	                                         "ff3all_sleep",
	                                         "ff3all_kk_wait",
	                                         "ff3all_kk_speak",
	                                         "ff3all_kk_walk",
	                                         "ff3all_kk_kenka1",
	                                         "ff3all_kk_kenka2",
	                                         "ff3all_kk_kenka3",
	                                         "ff3all_bottan",
	                                         NULL };
static const char* sNpcTRexAnims[]
    = { "tr_def",      "tr_wait",     "tr_walk",     "tr_speak",     "tr_stop",      "tr_sr_wait",   "tr_sr_gao",   "tr_sr_gabu",
	    "tr_sr_loft1", "tr_sr_loft2", "tr_sr_jp1",   "tr_sr_jp2",    "tr_sr_walk_r", "tr_sr_walk_l", "tr_ha1",      "tr_ha2",
	    "tr_ha3",      "tr_rhythm",   "tr_sit_wait", "tr_sit_speak", "tr_sit_bata",  "tr_pnc_start", "tr_pnc_wait", "tr_pnc_speak",
	    "tr_pnc_end",  "tr_zuko",     "tr_ha_stop",  "tr_sit_stop",  "tr_pnc_stop",  "tr_look_dw",   "tr_ha4",      NULL };
static const char* sNpcTTrackAnims[] = { "tt_def", "tt_wait", "tt_walk", "tt_speak", NULL };
static const char* sNpcFrogAnims[]
    = { "frog_def",      "frog_sit",       "frog_talk",         "frog_dry",           "frog_recover",     "frog_jump",
	    "frog_jump2",    "frog_wlk",       "frog_dry_reaction", "frog_tobasare_trex", "frog_sit2",        "frog_jump1",
	    "frog_yorokobu", "frog_jump2_end", "frog_okoru",        "frog_yorokobu_02",   "frog_sit_ue_miru", NULL };
static const char* sNpcZobinAnims[]  = { "zb_def",
	                                     "zb_wait",
	                                     "zb_walk",
	                                     "zb_speak",
	                                     "zb_stop",
	                                     "zb_start",
	                                     "zb_surp1",
	                                     "zb_surp2",
	                                     "zb_surp3",
	                                     "zb_end",
	                                     "zb_blue_on",
	                                     "zb_blue_wait",
	                                     "zb_blue_speak",
	                                     "zb_blue_off",
	                                     "zb_fl_take",
	                                     "zb_fl_wait",
	                                     "zb_fl_biku",
	                                     "zb_flblue_on",
	                                     "zb_flblue_wait",
	                                     "zb_flblue_speak",
	                                     "zb_flblue_kaiten",
	                                     "zb_flblue_surp",
	                                     "zb_flblue_surp2",
	                                     "zb_denguri",
	                                     "zb_flblue_surp3",
	                                     "zb_flblue_surp4",
	                                     "zb_denguri_start",
	                                     "zb_shot",
	                                     "zb_drug1",
	                                     "zb_drug2",
	                                     "zb_drug3",
	                                     "zb_drug_wait",
	                                     "zb_drug_run_furi",
	                                     "zb_drug_run",
	                                     "zb_drug_run_stop1",
	                                     "zb_drug_run_stop2",
	                                     "zb_drug2_heat",
	                                     "zb_surp_love",
	                                     "zb_dere1",
	                                     "zb_dere2",
	                                     NULL };
static const char* sNpcZobin2Anims[] = { "zb2_def",   "zb2_wait",    "zb2_walk",      "zb2_speak",    "zb2_surp1", "zb2_surp2",
	                                     "zb2_surp3", "zb2_blue_on", "zb2_blue_wait", "zb2_blue_off", NULL };
static const char* sNpcPeetsAnims[]  = { "pt_def",        "pt_wait",       "pt_walk",       "pt_speak",   "pt_stop",
	                                     "pt_hello",      "pt_happy",      "pt_tefuri",     "pt_kiss",    "pt_dw_on",
	                                     "pt_dw_wait",    "pt_dw_speak",   "pt_dw_off",     "pt_fl_give", "pt_fl_wait",
	                                     "pt_fl_speak",   "pt_fl_thank",   "pt_fl_walk",    "pt_surp1",   "pt_surp1_wait",
	                                     "pt_surp1_back", "pt_surp2_wait", "pt_surp2_back", "pt_surp3",   "pt_surp3_wait",
	                                     "pt_surp3_back", "pt_surp1_kutu", "pt_ho",         NULL };
static const char* sNpcNasubiAnims[]
    = { "nsb_def",    "nsb_wait",   "nsb_walk",   "nsb_speak",  "nsb_jump",    "nsb_mp1",     "nsb_mp2", "nsb_mp3",
	    "nsb_ibaru1", "nsb_ibaru2", "nsb_ibaru3", "nsb_saraba", "nsb_cb_wait", "nsb_cb_look", NULL };
static const char* sHaisuiNezumiAnims[]
    = { "nz_def",        "nz_std",      "nz_std2",     "nz_wlk",      "nz_run",      "nz_bikkuri",  "nz_kizetu", "nz_ex_run",
	    "nz_wlk_tenkan", "nz_attack_1", "nz_attack_2", "nz_attack_3", "nz_attack_4", "nz_attack_5", NULL };
static const char* sNpcBirdAnims[]              = { "bd_def",   "bd_std",   "bd_talk",       "bd_slp",       "bd_bikkuri",      "bd_flap1",
	                                                "bd_flap2", "bd_flap3", "bd_flap_talk1", "bd_std_talk1", "bd_slp_reaction", NULL };
static const char* sLivingCbBook2Anims[]        = { "book_hiraiteru",    "book_kakae01_papa", "book_ugoku01_papa",
	                                                "book_kakae02_papa", "book_ugoku02_papa", "book_kakae03_papa",
	                                                "book_ugoku03_papa", "book_oti_hiraku",   NULL };
static const char* sChibihouseCusCbPrLv1Anims[] = { "cb_def_custom_pr_lv1",
	                                                "cb_out_custom_pr_lv1",
	                                                "cb_std_custom_pr_lv1",
	                                                "cb_wlk_lp_custom_pr_lv1",
	                                                "cb_jog_lp_custom_pr_lv1",
	                                                "cb_run_lp_custom_pr_lv1",
	                                                "cb_idling_custom_pr_lv1",
	                                                "cb_lp_custom_pr_lv1",
	                                                "cb_lp2_custom_pr_lv1",
	                                                "cb_rot_en_custom_pr_lv1",
	                                                "cb_in_custom_pr_lv1",
	                                                "cb_in_custom_S",
	                                                NULL };
static const char* sUfoTimeMcAnims[]            = { "tmc_def",       "tmc_ball1", "tmc_ball2", "tmc_ball3",  "tmc_wait", "tmc_open",
	                                                "tmc_open_wait", "tmc_close", "tmc_go",    "tmc_arrive", "tmc_fly",  NULL };

extern "C" {
XObjSource gObjectSources[] = {
	{ "cb_robo", NULL, 0x00000000, 0x00000000, sCbRoboAnims, 0 },
	{ "kitchen/map", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "kitchen/reizouko", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/gas_dai", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/sink_under1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/sink_midle", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/tao_e", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/plant_box", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/counter1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/counter2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/table", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/shelf_l", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/door", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/wall_walk", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/consentman", NULL, 0x00000000, 0x00000000, sNpcConsentmanAnims, 0 },
	{ "npc/tonpy", NULL, 0x00000000, 0x00000000, sNpcTonpyAnims, 0 },
	{ "kitchen/window_s", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "cb_plug", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/box_4", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/box_8", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/box_12", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/box_lift", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/rope", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/ladder", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "rouka/map", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "rouka/door_j", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/door_k", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/door_pama", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/entrance", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ "rouka/kaidan1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/kaidan2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/kaidan3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/mat", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "rouka/ro_counter", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/ro_plant", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/ro_sofa", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/table_kaku", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/table_maru", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/chair1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/tesuri_2f", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/wall_1_2f", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/wall_1f", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "rouka/wall_2f", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "pushobj3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/map", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "coin_c", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "coin_g", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "coin_s", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/base", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "backyard/bg", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "backyard/door", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ "backyard/fence", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/wall", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/map", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "souko/wall", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "pushobj1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "pushobj2", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ "item/pen", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "socket", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "kitchen/wall", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/deka_denchi", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/pc_gamen", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "chibihouse/gacha", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/air", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/map", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "jenny/bed", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "jenny/chair1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/chair2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/window", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/closet", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "jenny/desk", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/door", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/dresser", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/light1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/mat", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/shelf", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/table", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/wall", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/wall_shelf", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/window1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/window2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "parent/map", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "parent/barwall_l", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/barwall_m", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/barwall_r", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "parent/bed", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/chair", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/closet", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "parent/cornershelf", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/dresser", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/sofa", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/table", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "map_jump_box", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/tao", NULL, 0x00000000, 0x00000000, sNpcTaoAnims, 0 },
	{ "parent/wall", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "parent/wallshelf", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/window", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/hock", NULL, 0x00000000, 0x00000000, sNpcHockAnims, 0 },
	{ "souko/kaidan", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/dc_dai", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/shelf_dan_box", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/shelf_dan_dai", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/door", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/door_l", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/door_l_dami", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/cus_cb_pr_lv1", NULL, 0x00000000, 0x00000000, sChibihouseCusCbPrLv1Anims, 0 },
	{ "chibihouse/gacha_pon1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/wall_shelf1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/tel", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/tel_counter", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/tel_j", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/wall_counter", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/shelf_w", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/wall_up", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/shelf_slope", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/flowerclock", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "souko/hasira", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/living_sky", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/butterfly", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/flower_bed", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "rouka/kaidan_2h", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/kaidan_3h", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/wine_bottle", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "rouka/hashira_t", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/hashira_e", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/wall_under", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/floor_2f", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/map", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "chibihouse/wall", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "chibihouse/chi_pc", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/chi_socket", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/chi_e", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/door_e", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/wall_2f_under", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibi_house", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/window", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "Item/frog_ring", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "chibi_jump", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "energyb", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/jenny", NULL, 0x00000000, 0x00000000, sNpcJennyAnims, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/map", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/danro", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/shelf_b", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/shelf_dan", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/sidetable", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/sofa1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/sofa2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/standlight", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/table", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/tv", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/wall", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/window", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "house_r/chibi_h_main", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "house_r/chibi_h_dai", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/mama", NULL, 0x00000000, 0x00000000, sNpcMamaAnims, 0 },
	{ "living/sofa_hashigo", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/books1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/books2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/sofa_cu1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/sofa_cu2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/flowerpot2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/shelf_dan_book", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/tako", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/oekaki", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "house_r/chibi_h_hasigo", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/army1", NULL, 0x00000000, 0x00000000, sNpcArmy1Anims, 0 },
	{ "npc/army2", NULL, 0x00000000, 0x00000000, sNpcArmy2Anims, 0 },
	{ "house_r/chibi_h_door1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "house_r/chibi_h_door2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "doorroom", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "doorroom_dokan", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/snack_bone", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/junk_unko", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/junk_a", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "test/test_ring", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/table_foot", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/mat", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/tub", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "plug_start", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/deca_robo", NULL, 0x00000000, 0x00000000, sNpcDecaRoboAnims, 0 },
	{ "npc/keath", NULL, 0x00000000, 0x00000000, sNpcKeathAnims, 0 },
	{ "chibihouse/denti1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/denti2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/papa", NULL, 0x00000000, 0x00000000, sNpcPapaAnims, 0 },
	{ "house_r/chibi_h_hasi", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "house_r/chibi_h_warp", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/fafa", NULL, 0x00000000, 0x00000000, sNpcFafaAnims, 0 },
	{ "npc/spider1", NULL, 0x00000000, 0x00000000, sNpcSpider1Anims, 0 },
	{ "npc/ufo", NULL, 0x00000000, 0x00000000, sNpcUfoAnims, 0 },
	{ "item/brush", NULL, 0x00000000, 0x00000001, sItemBrushAnims, 0 },
	{ "living/sakana", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/kaiga_j", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/capsule", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/ari", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/bg", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/ari_daidai", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "backyard/tree_b", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "backyard/kadan", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "plug_dokodakadoor", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/bb_dan", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/trashbox", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/tao_dish", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/spider_su_b", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/suisou_r2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/sakana_awa", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/kumo_himo", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/et1", NULL, 0x00000000, 0x00000000, sNpcEt1Anims, 0 },
	{ "item/kami_kuzu", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/cookie_kakera", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/tv_keath", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/dust0", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/dust1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/sakana_esa", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/tana_ana", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "test/sp_ito", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/wall_corner", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/co_under", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "test/jenny_hand", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/window_kazari", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/junk_b", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/junk_c", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/gabyou", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "backyard/sky", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "test/fly_root1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/army3", NULL, 0x00000000, 0x00000000, sNpcArmy3Anims, 0 },
	{ "npc/n_wing1", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/n_wing2", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "rouka/planter", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/spoon", NULL, 0x00000000, 0x00000001, sItemSpoonAnims, 0 },
	{ "test/n_mato", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/cube_box", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/cube_l", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/cube_o", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/cube_v", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/cube_e", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/cu_animal1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/cu_animal2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/mag_cup", NULL, 0x00000000, 0x00000001, sItemMagCupAnims, 0 },
	{ "npc/sun_mida", NULL, 0x00000000, 0x00000000, sNpcSunMidaAnims, 0 },
	{ "npc/sun_dai", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/cake", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/bb", NULL, 0x00000000, 0x00000000, sNpcBbAnims, 0 },
	{ "item/bara", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/orange_glass", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/orange_bin", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/cham_glass", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/cham_bin", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/vase", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/chicken", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/living_cam", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ "npc/spider2", NULL, 0x00000000, 0x00000000, sNpcSpider2Anims, 0 },
	{ "living/cake_dish", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/kazari_t", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/door_r", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/asiato", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/hat", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/kazari_w", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/birthday_c", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/bone_r", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/door_dami", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/cos_obake", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ "kitchen/tuti_huta", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/sink_hi", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/sink_hi_under", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/trashbox", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/hammock", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/shoes", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "kitchen/sink_2hi", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/s_rope", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/happy_box", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/consentman_atari", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/tonpy_item_a", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/tonpy_item_b", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/tonpy_item_c", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/hock_tizu", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "how_to_chibirobo/cb_book", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "fx_cb_house01", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/cb_cover", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/happy_heart", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/yogore_cook_o", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/yogore_water", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/yogore_oil", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/cam_living", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "house_r/chibi_h_maku", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/yakan", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/teacup", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/teapot", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/milk_bin", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/sugar_bin", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/cookie_dish", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/cb_teaset", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/block_sugar", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/flower_cookie", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/sink_s_shelf", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/spider_kage", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/sink_dish", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/hukin", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "kitchen/sink_kago", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/tv_switch", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/teacup_s", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/cracker", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_deca_spark01", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/tv_news", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/tv_kage", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/cb_book2", NULL, 0x00000000, 0x00000001, sLivingCbBook2Anims, 0 },
	{ "fx_tomp_cry01", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/sw_1planter", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/sw_2planter", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/bb_nikki", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/bb_pen", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/wall_under", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "how_to_chibirobo/map", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "fx_tomp_scare01", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/door_k", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/cracker", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/cracker_gomi", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/f_clock_hb", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/f_clock_hs", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/f_clock_f", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/present_box", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/bg_tree", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_spider01", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/chicken_bone", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/cake_ato", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/cup_army", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/wine_bottle_a", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/wine_bottle_b", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/sink_3hi", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/tana1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/chip", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/window_ware", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/window_hiraku", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/army4", NULL, 0x00000000, 0x00000000, sNpcArmy4Anims, 0 },
	{ "npc/primo", NULL, 0x00000000, 0x00000000, sNpcPrimoAnims, 0 },
	{ "parent/partition", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/partition2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/partition3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "parent/partition5", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/wallshelf2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/wallshelf3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/wallshelf4", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/wallshelf5", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/wallshelf6", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/switch", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "house_r/dream1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/sidetable", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "parent/ladder", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/closet2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "parent/door", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/table2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/chest", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/chest2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "parent/blind_r", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/blind_l", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/blindhandle_r", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/blindhandle_l", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/sewingmachine", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/flower_lamp", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/receipt", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/mama_pen", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/mama_calculator", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/bills", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/receipt", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/account_book", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/map", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "ufo/wall", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/tyuusyaki", NULL, 0x00000000, 0x00000001, sItemTyuusyakiAnims, 0 },
	{ "kitchen/hamburg", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/jenny_tv", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/funky_f", NULL, 0x00000000, 0x00000000, sNpcFunkyFAnims, 0 },
	{ "npc/t_rex", NULL, 0x00000000, 0x00000000, sNpcTRexAnims, 0 },
	{ "all/ufo_warp", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/cll_papa", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/cll_mama", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/gas_plate", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/doorbell", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/doorwindow", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/doorwindow2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/planter", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/planter2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/pole", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/pole2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/pole3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/pole4", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/pole5", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/pole6", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/pole7", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/pole8", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "parent/recplayer", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/recspeaker_l", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/recspeaker_r", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/stpole", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/stpole2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/tonpy_item_e", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/tonpy_item_f", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/tonpy_item_g", NULL, 0x00000000, 0x00000001, sItemTonpyItemGAnims, 0 },
	{ "item/tonpy_item_h", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/tonpy_item_i", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/tonpy_item_j", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/tonpy_item_k", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/tonpy_item_l", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/tonpy_item_m", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/tonpy_item_n", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/tonpy_item_o", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/tonpy_item_p", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/tonpy_item_q", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/tonpy_item_r", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/tonpy_item_s", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/tonpy_item_t", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/tonpy_item_u", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "backyard/window_1l", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/window_2l", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/letter", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/doorswitch", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/asiba1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/asiba2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/asiba3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/castle_door", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/heitai1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/kaidan", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/hikidasi_close", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/hikidasi_open", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/door_pama_h", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/rope_door", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/rope_nobu", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/asiba_s1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/castle_hasigo", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/hari_l", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/hari_s", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ "jenny/heitai2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/heitai3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/heitai4", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/denchi1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/denchi2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/denchi3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/peets_kutu", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/hocyouki", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ "jenny/asiba_kado1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/asiba4", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/mitu_flower", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/hari_l2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/papa_sleep", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/mama_sleep", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/jenny_sleep", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "all/sa_seal", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/skirting_board", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/skirting_board2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/skirting_boardp", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/recplayer_cable", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/bed2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/taki_obj", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/ceiling", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/tv_gamen", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/bed_stay", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "all/yogore_taiya", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/yogore_kureyon", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/yogore_toy", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/frog", NULL, 0x00000000, 0x00000000, sNpcFrogAnims, 0 },
	{ "parent/cornershelf_ladder", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/window_ware", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/jaguti", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/hasi", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/ham_pan_up", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/ham_pan_down", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/heitai5", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_leticwarp01", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/hose", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/nebukuro", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/cam_papamama", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/cam_jenny", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/enjoy_light", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/zobin", NULL, 0x00000000, 0x00000000, sNpcZobinAnims, 0 },
	{ "npc/peets", NULL, 0x00000000, 0x00000000, sNpcPeetsAnims, 0 },
	{ "jenny/heitai_taiko", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/heitai_rappa", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/jenny_himo", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/jenny_bag", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_zact_gicco", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_zact_tao", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_zact_frog", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_zact_injured", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_zact_pajamas", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/saisun_box", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "backyard/kumo_ufo", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/happy_heart2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/bed_top", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/1day_timer", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/door_stop", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/rex_tooth", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/zobin_box", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "rouka/kaidan_atama", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/leaf", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/door_kazari", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/heitai_guitar", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/zobin_bm1", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/tane_a", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/bb_pen", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/ink", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ "item/kare_flower", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/present_hana", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/sunmidasky", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/kayaku", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/goggle", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/love_letter", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/tug", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/ticket", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/houtai", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/block", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ "item/record_giccio", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/record_ff", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/wall_c_h", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/nwing_item", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/army_photo", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/counter_h", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "test/fly_root2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/car1", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/car2", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/funky_f2", NULL, 0x00000000, 0x00000000, sNpcFunkyF2Anims, 0 },
	{ "souko/shelf", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "souko/chest", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/renga", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/kamare_bb", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "souko/shelf_dami", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "kitchen/tuti_ana", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/hock_ship", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/left_foot", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/suit_case", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/kairozu", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/deka_denchi_full", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/haguruma", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/planter_zobin", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/planter_jenny1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/planter_jenny2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/ufo_ball", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/time_mc", NULL, 0x00000000, 0x00000000, sUfoTimeMcAnims, 0 },
	{ "item/pajama_kiji", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/present_hana_zobin", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/flower1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/flower2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/flower3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/junk_tree", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ "all/coin_tree", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ "all/happy_tree", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ "all/mame_tree", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ "jenny/sanpu_house", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/kare_flower_mitu", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/saki_flower1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/jaguchi_h", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/tuti_huta_d", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/t_track", NULL, 0x00000000, 0x00000000, sNpcTTrackAnims, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/army5", NULL, 0x00000000, 0x00000000, sNpcArmy5Anims, 0 },
	{ "ufo/wall2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/wall_tonneru", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/wall_t_ware", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/toy_denwa", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/toy_piano", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/exit", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/densetu_hana", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "haisui/map", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "haisui/pipe_a", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "haisui/hashigo_b", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "haisui/hashigo_c", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "haisui/hashigo_d", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "ufo/ground", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/ground2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/ground3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/ball_rot", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/game1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/game2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/game3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/game4", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/game_botan", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/game_p1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/game_p2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/game_p3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/game_p4", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/wall3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "test/car_root1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "test/car_root2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/game_botan2", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/n_mato2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/sink_hashigo", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/tuta_a", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/tuta_aRa", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/tuta_aRb", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/chibi_isu", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/tuta_b", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/tuta_bR", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/tuta_c", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/tuta_cR", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/tuta_d", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/tuta_dR", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/conro", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/kingyo", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/w_planter", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/taki_hashi_a", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/taki_hashi_b", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/kenban1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/kenban2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "parent/tp_mike1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/tp_mike2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/house_himo", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/bag_himo", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/taki_kakoi", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/army_ita1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/army_ita2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/army_heri1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/ufo_bed", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/jenny_dress", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_kitchenburner", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_warpzone", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/ufo_bed2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/ufo_bed3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "ufo/ufo_bed4", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/nasubi", NULL, 0x00000000, 0x00000000, sNpcNasubiAnims, 0 },
	{ "all/tonpy_item_kage", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ "rouka/race_ita", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "haisui/soto", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "haisui/nezumi", NULL, 0x00000000, 0x00000000, sHaisuiNezumiAnims, 0 },
	{ "haisui/hashigo_f", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "haisui/rope", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "haisui/pipe_b", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/jenny_dress2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/jenny_dress3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "haisui/kabe", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "souko/birdcage", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/board", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/nail", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/v_rope", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "haisui/water", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/pajama_kiji2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/pajama_kiji3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/t_kazan1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/t_kazan2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/t_kazan3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/t_kazan4", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/t_kazan5", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/t_kazan6", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/t_block1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/t_block2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/t_block3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/t_block4", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/t_block5", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/t_block6", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/car_item", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "backyard/t_hashigo", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/branko", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ "backyard/b_rope", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/papa_yubiwa", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/t_kage", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_zact_ghost", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/frog_hagata", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/kusa", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/bebe_binsen", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/sekkeizu", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/item_frog", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_pc_buy01", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/jenny_book", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/planter_hachi", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/kureyon", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_cbdoor01", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_tm_barrier", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/suberidai1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/suberidai2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/album_t", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/album_y", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/table_stand", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/kaizou_poper", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "parent/welding_maskp", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/welding_maskm", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/welding_maskj", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/welding_maskt", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/welding_torch", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/welding_toolbox", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/welding_tools", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/drafter", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/gas_hashigo", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/gas_rope", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "parent/case_janks", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "yane/torinosu", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/kusa_tuti", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/keath_omc", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "title_icon/icon_bb", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/tao_bag", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/kokuban", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "parent/dcfoot_spider", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/takarabako", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "cb_cannon_lv2", NULL, 0x00000000, 0x00000001, sCbCannonLv2Anims, 0 },
	{ "cb_radar", NULL, 0x00000000, 0x00000001, sCbRadarAnims, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/cb_kai_poper", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "haisui/room1_c", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "fx_deca_energy", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_deca_battery", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_3pu01", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_papa01", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/kankisen", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/tamagotti", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/deca_robo2", NULL, 0x00000000, 0x00000000, sNpcDecaRobo2Anims, 0 },
	{ "kitchen/toaster", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/senzai", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/suponji", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "kitchen/jam", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "kitchen/gus_up", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/yogore_h_s", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/yogore_h_k", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/spider3", NULL, 0x00000000, 0x00000000, sNpcSpider3Anims, 0 },
	{ "npc/sun_kao", NULL, 0x00000000, 0x00000000, sNpcSunKaoAnims, 0 },
	{ "kitchen/puropera", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/castle_hasigo2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/yogore_hito", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/yogore_hun", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/yogore_shimi", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_army_sgt", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/jenny_kagami", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/tamagotti_gamen", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "kitchen/kankisen_asiba", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/light_rope", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/audio", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/sibari_mama", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/sibari_papa", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/sibari_tao", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/sibari_jenny", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/akikan", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/candy_gomi", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/okasi_gomi1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/okasi_gomi2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/item_nasubi", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/wall_habaki", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/wall_top", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/wall_tile", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/papa_toybox", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/papa_toybox2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/records", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_gicco_twinb01", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_gicco_twinb02", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_gicco_twinb03", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/densetu_tane", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_backyard_rain", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/co_under", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/taki_s", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/taki_s_c", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/recplayer_needle", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_alien01", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/ari_su", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/co_under", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/bird", NULL, 0x00000000, 0x00000000, sNpcBirdAnims, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/tao_s", NULL, 0x00000000, 0x00000000, sNpcTaoSAnims, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "parent/t_blocks", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "kitchen/mitu_bin", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/sanpu_bin", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "test/spider_janks1", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "chibihouse/ranking_no", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/ranking_board", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "souko/sun_rag", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "souko/sun_rou", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/tv_news2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/kumo_ame", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/yogore_30k", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/yogore_50k", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "haisui/ana_a", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "kitchen/bg", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "kitchen/fruit", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/hae", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_mspider_eye", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "test/mt_jank1", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/mt_jank2", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/mt_jank3", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/mt_jank4", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/mt_jank5", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/mt_jank6", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/mt_jank7", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/mt_jank8", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/mt_jank9", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/kure1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/kure2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/kure3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/kure4", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/kure5", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/gbox_big", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "parent/gbox_small", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/fafa_omc", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "test/sp3_bom", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "backyard/window_k", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/tv_suna", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/jimen_ryuuki", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "backyard/fune_tuti_d", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/elect10", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/elect2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/elect3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/elect4", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/elect5", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/elect6", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/elect7", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/elect8", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/elect9", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/hodoke_ito", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/denti2con", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/yogore_kureyon_k", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/grass", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/mother_wall", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/mother_map", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/protector", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/apron", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/curtain", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/t_blocks2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/nwing_dai", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/shelf_l_ha", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/shelf_l_hb", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/clock_l", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/clock_s", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/shelf_l_r", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/hamburg_k", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/mini_box", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/mini_hikidasi", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/map_kure1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/map_kure2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/map_kure3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/map_kure4", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/map_kure5", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_tomp_cam2d", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_mummiform", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_pirateboat", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_mspider_01", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_mspider_02", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_tm_unite", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/j_clock_l", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/j_clock_s", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/s_clock_l", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/s_clock_s", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/r_clock_l", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/r_clock_s", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "jenny/cartain", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/tv_sampoo", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/bag_himo2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/bag_himo3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/bag_himo4", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/heitai_taore", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/shelf_books", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/move_book1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/move_book2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/sanpu_bin2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/birdcage_shortcut", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "souko/birdcage_shortcut2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/mini_funky1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/spider4", NULL, 0x00000000, 0x00000000, sNpcSpider4Anims, 0 },
	{ "npc/zobin_bm2", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/zobin_bm3", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "parent/funky_stage", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "item/mini_funky_buti", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/tane_funky", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/funky_f3", NULL, 0x00000000, 0x00000000, sNpcFunkyF3Anims, 0 },
	{ "kitchen/shelf_l_rb", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/spider_b", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/tyou_bb", NULL, 0x00000000, 0x00000000, sNpcTyouBbAnims, 0 },
	{ "rouka/w_hashigo", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/w_rope", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/te_rope_a", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ "rouka/te_rope_b", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/bg", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/light", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/co_under", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/s_under_obj", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/clock_l", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/clock_s", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/clock_f", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/clock_cuckoo", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/bg01", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_spider02", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/obj_a", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/obj_b", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "souko/obj_c", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/c_obj", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "rouka/2f_obj", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/1f_s_rope", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/2f_s_rope", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/igagu_book", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/ff_hana", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/tanaue_rope", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/funky_f2_b", NULL, 0x00000000, 0x00000000, sNpcFunkyF2BAnims, 0 },
	{ "npc/funky_f3_b", NULL, 0x00000000, 0x00000000, sNpcFunkyF3BAnims, 0 },
	{ "parent/funky_stage2", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "backyard/cam_backyard", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/saki_flower2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "item/saki_flower3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/flower_kazari1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/flower_kazari2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "living/flower_kazari3", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/tao_hunburg", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/ito_army", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "house_r/ito_h_main", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "kitchen/tao_collision_box01", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/tao_collision_box02", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/tao_collision_box03", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/bb_collision_box01", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/yogore_ff", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/et_tako", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/dry_frog", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/spider_b_l", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "kitchen/frypan", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/spider_kakoi", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/ff_hanataba", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/shelf_l_obj", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_scream2d", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "fx_prose2d", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ "fx_caution2d", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_tm_takeoff", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_mspider_eye2d", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/funky_f_all", NULL, 0x00000000, 0x00000000, sNpcFunkyFAllAnims, 0 },
	{ "npc/funky_f2_all", NULL, 0x00000000, 0x00000000, sNpcFunkyF2AllAnims, 0 },
	{ "npc/funky_f3_all", NULL, 0x00000000, 0x00000000, sNpcFunkyF3AllAnims, 0 },
	{ "jenny/j_obj1", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/j_obj2", NULL, 0x00000000, 0x00000002, NULL, 0 },
	{ "item/c_denchi", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "chibihouse/junk_ireru", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/full_moon", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/branko_end", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "all/rakugaki_deka", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/denkiconro", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/tao_bag2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/t_hanamizu", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/sp_ito2", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/tao_slp", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "living/tv_keath2", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/cus_decarobo", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/decarobo_hand", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "jenny/j_trashbox", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/pole_rope", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/ufo2", NULL, 0x00000000, 0x00000001, sNpcUfo2Anims, 0 },
	{ "kitchen/pot", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/pot_rope", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "parent/datu_tama", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "rouka/army_heri2", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "rouka/army_run1", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "rouka/army_run2", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "jenny/peets_foot", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/hock_ana", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "backyard/hock_ana_kaidan", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_alien_end", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "fx_deca_end", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "npc/zobin2", NULL, 0x00000000, 0x00000000, sNpcZobin2Anims, 0 },
	{ "parent/tp_stage", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "jenny/zobin_dai", NULL, 0x00000000, 0x00000001, NULL, 0 },
	{ "kitchen/tao_collision_box04", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "npc/sun_ufo", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0xFFFFFFFF, 0xFFFFFFFF, NULL, 0 },
};

UnkObjData objTable[] = {
	{ &gObjectSources[0], 0x0, 0x2C9, 3, 0, 0, 2, 0 },      { &gObjectSources[1], 0x0, 0x3, 0, 0, 0, 3, 0 },
	{ &gObjectSources[2], 0x0, 0x3, 0, 0, 0, 22, 0 },       { &gObjectSources[3], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[4], 0x0, 0x1, 0, 0, 0, 22, 0 },       { &gObjectSources[5], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[6], 0x0, 0x1, 0, 0, 0, 22, 0 },       { &gObjectSources[7], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[8], 0x0, 0x3, 0, 0, 0, 22, 0 },       { &gObjectSources[9], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[10], 0x0, 0x3, 0, 0, 0, 22, 0 },      { &gObjectSources[11], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[12], 0x0, 0x1, 0, 0, 0, 22, 0 },      { &gObjectSources[13], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[14], 0x0, 0x81, 0, 0, 0, 22, 0 },     { &gObjectSources[15], 0x0, 0x289, 3, 0, 0, 10, 10 },
	{ &gObjectSources[16], 0x0, 0x1, 0, 0, 0, 22, 0 },      { &gObjectSources[17], 0x0, 0x1, 0, 0, 0, 24, 0 },
	{ &gObjectSources[18], 0x0, 0x1, 0, 0, 0, 5, 11 },      { &gObjectSources[19], 0x0, 0x1, 0, 0, 0, 5, 13 },
	{ &gObjectSources[20], 0x0, 0x1, 0, 0, 0, 22, 0 },      { &gObjectSources[21], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[22], 0x0, 0x1, 0, 0, 0, 22, 0 },      { &gObjectSources[23], 0x0, 0x1, 0, 1, 15, 22, 0 },
	{ &gObjectSources[24], 0x0, 0x3, 0, 0, 0, 3, 0 },       { &gObjectSources[25], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[26], 0x0, 0x1, 0, 0, 0, 22, 0 },      { &gObjectSources[27], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[28], 0x0, 0x1, 0, 0, 0, 22, 0 },      { &gObjectSources[29], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[30], 0x0, 0x3, 0, 0, 0, 22, 0 },      { &gObjectSources[31], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[32], 0x0, 0x3, 0, 0, 0, 22, 0 },      { &gObjectSources[33], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[34], 0x0, 0x3, 0, 0, 0, 22, 0 },      { &gObjectSources[35], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[36], 0x0, 0x3, 0, 0, 0, 22, 0 },      { &gObjectSources[37], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[38], 0x0, 0x3, 0, 0, 0, 22, 0 },      { &gObjectSources[39], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[40], 0x0, 0x3, 0, 0, 0, 22, 0 },      { &gObjectSources[41], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[42], 0x0, 0x3, 0, 0, 0, 22, 0 },      { &gObjectSources[43], 0x0, 0x1, 0, 0, 0, 22, 30 },
	{ &gObjectSources[44], 0x0, 0x3, 0, 0, 0, 3, 0 },       { &gObjectSources[45], 0x0, 0x1, 0, 1, 15, 7, 2 },
	{ &gObjectSources[46], 0x0, 0x1, 0, 1, 15, 7, 0 },      { &gObjectSources[47], 0x0, 0x1, 0, 1, 15, 7, 1 },
	{ &gObjectSources[48], 0x0, 0x3, 0, 0, 0, 22, 0 },      { &gObjectSources[49], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[50], 0x0, 0x1, 0, 0, 0, 22, 0 },      { &gObjectSources[51], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[52], 0x0, 0x3, 0, 0, 0, 22, 0 },      { &gObjectSources[53], 0x0, 0x3, 0, 0, 0, 3, 0 },
	{ &gObjectSources[54], 0x0, 0x1, 0, 0, 0, 22, 0 },      { &gObjectSources[55], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[56], 0x0, 0x1, 0, 0, 0, 22, 0 },      { &gObjectSources[57], 0x0, 0x1, 0, 0, 0, 22, 30 },
	{ &gObjectSources[58], 0x0, 0x1, 0, 0, 0, 22, 30 },     { &gObjectSources[59], 0x0, 0x1, 0, 1, 15, 8, 1 },
	{ &gObjectSources[60], 0x0, 0x1, 0, 1, 15, 8, 2 },      { &gObjectSources[61], 0x0, 0x1, 0, 1, 15, 8, 3 },
	{ &gObjectSources[62], 0x0, 0x1, 0, 1, 15, 8, 4 },      { &gObjectSources[63], 0x0, 0x1, 0, 1, 15, 8, 5 },
	{ &gObjectSources[64], 0x0, 0x1, 0, 1, 15, 8, 6 },      { &gObjectSources[65], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[66], 0x0, 0x1, 0, 1, 15, 8, 7 },      { &gObjectSources[67], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[68], 0x0, 0x1, 0, 0, 0, 22, 0 },      { &gObjectSources[69], 0x0, 0x1, 0, 0, 0, 8, 8 },
	{ &gObjectSources[70], 0x0, 0x1, 0, 0, 0, 22, 0 },      { &gObjectSources[71], 0x0, 0x1, 0, 0, 0, 5, 7 },
	{ &gObjectSources[72], 0x0, 0x1, 0, 0, 0, 22, 0 },      { &gObjectSources[73], 0x0, 0x3, 0, 0, 0, 3, 0 },
	{ &gObjectSources[74], 0x0, 0x3, 0, 0, 0, 22, 0 },      { &gObjectSources[75], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[76], 0x0, 0x3, 0, 0, 0, 22, 0 },      { &gObjectSources[77], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[78], 0x0, 0x1, 0, 0, 0, 22, 0 },      { &gObjectSources[79], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[80], 0x0, 0x1, 0, 0, 0, 22, 0 },      { &gObjectSources[81], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[82], 0x0, 0x3, 0, 0, 0, 22, 0 },      { &gObjectSources[83], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[84], 0x0, 0x3, 0, 0, 0, 22, 0 },      { &gObjectSources[85], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[86], 0x0, 0x3, 0, 0, 0, 22, 0 },      { &gObjectSources[87], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[88], 0x0, 0x3, 0, 0, 0, 22, 0 },      { &gObjectSources[89], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[90], 0x0, 0x1, 0, 0, 0, 22, 0 },      { &gObjectSources[91], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[92], 0x0, 0x3, 0, 0, 0, 3, 0 },       { &gObjectSources[93], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[94], 0x0, 0x3, 0, 0, 0, 22, 0 },      { &gObjectSources[95], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[96], 0x0, 0x1, 0, 0, 0, 22, 0 },      { &gObjectSources[97], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[98], 0x0, 0x3, 0, 0, 0, 22, 0 },      { &gObjectSources[99], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[100], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[101], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[102], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[103], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[104], 0x0, 0x3, 0, 0, 0, 22, 0 },     { NULL, 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[106], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[107], 0x0, 0x81, 0, 0, 0, 14, 0 },
	{ &gObjectSources[108], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[109], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[110], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[111], 0x0, 0x81, 0, 0, 0, 10, 20 },
	{ &gObjectSources[112], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[113], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[114], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[115], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[116], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[117], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[118], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[119], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[120], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[121], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[122], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[123], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[124], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[125], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[126], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[127], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[128], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[129], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[130], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[131], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[132], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[133], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[134], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[135], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[136], 0x0, 0x1, 0, 0, 0, 22, 0 },     { NULL, 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[138], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[139], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[140], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[141], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[142], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[143], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[144], 0x0, 0x1, 0, 1, 15, 8, 50 },    { &gObjectSources[145], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[146], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[147], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[148], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[149], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[150], 0x0, 0x3, 0, 0, 0, 3, 0 },      { &gObjectSources[151], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[152], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[153], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[154], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[155], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[156], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[157], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[158], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[159], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[160], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[161], 0x0, 0x1, 0, 1, 15, 8, 0 },
	{ &gObjectSources[162], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[163], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[164], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[165], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[166], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[167], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[168], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[169], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[170], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[171], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[172], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[173], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[174], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[175], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[176], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[177], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[178], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[179], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[180], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[181], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[182], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[183], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[184], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[185], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[186], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[187], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[188], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[189], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[190], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[191], 0x0, 0x1, 0, 1, 15, 7, 200 },
	{ &gObjectSources[192], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[193], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[194], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[195], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[196], 0x0, 0x81, 0, 0, 0, 11, 0 },    { &gObjectSources[197], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[198], 0x0, 0x3, 0, 0, 0, 3, 0 },      { &gObjectSources[199], 0x0, 0x3, 0, 0, 0, 3, 0 },
	{ &gObjectSources[200], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[201], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[202], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[203], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[204], 0x0, 0x4003, 0, 0, 0, 22, 0 },  { &gObjectSources[205], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[206], 0x0, 0x2003, 0, 0, 0, 22, 0 },  { &gObjectSources[207], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[208], 0x0, 0x4003, 0, 0, 0, 22, 0 },  { &gObjectSources[209], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[210], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[211], 0x0, 0x4021, 0, 1, 20, 9, 0 },
	{ &gObjectSources[212], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[213], 0x0, 0x4003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[214], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[215], 0x0, 0x81, 0, 0, 0, 12, 0 },
	{ &gObjectSources[216], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[217], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[218], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[219], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[220], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[221], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[222], 0x0, 0x4003, 0, 0, 0, 22, 0 },  { &gObjectSources[223], 0x0, 0x4003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[224], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[225], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[226], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[227], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[228], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[229], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[230], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[231], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[232], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[233], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[234], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[235], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[236], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[237], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[238], 0x0, 0x4001, 0, 0, 0, 22, 0 },  { &gObjectSources[239], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[240], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[241], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[242], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[243], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[244], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[245], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[246], 0x0, 0x3, 0, 0, 0, 3, 0 },      { &gObjectSources[247], 0x0, 0x81, 0, 0, 0, 16, 0 },
	{ &gObjectSources[248], 0x0, 0x81, 4, 0, 0, 16, 0 },    { &gObjectSources[249], 0x0, 0x4003, 0, 0, 0, 5, 1 },
	{ &gObjectSources[250], 0x0, 0x4001, 0, 0, 0, 22, 0 },  { &gObjectSources[251], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[252], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[253], 0x0, 0x1, 0, 1, 15, 8, 9 },
	{ &gObjectSources[254], 0x0, 0x1, 0, 1, 15, 8, 10 },    { &gObjectSources[255], 0x0, 0x1, 0, 1, 15, 8, 11 },
	{ &gObjectSources[256], 0x0, 0x1, 0, 1, 15, 8, 12 },    { &gObjectSources[257], 0x0, 0x1, 0, 1, 15, 8, 13 },
	{ &gObjectSources[258], 0x0, 0x1, 0, 1, 15, 8, 14 },    { &gObjectSources[259], 0x0, 0x1, 0, 1, 15, 8, 15 },
	{ &gObjectSources[260], 0x0, 0x1, 0, 1, 15, 7, 103 },   { &gObjectSources[261], 0x0, 0x1, 0, 1, 15, 7, 100 },
	{ &gObjectSources[262], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[263], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[264], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[265], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[266], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[267], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[268], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[269], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[270], 0x0, 0x21, 0, 0, 0, 22, 0 },    { &gObjectSources[271], 0x0, 0x81, 0, 0, 0, 15, 0 },
	{ &gObjectSources[272], 0x0, 0x81, 0, 0, 0, 10, 30 },   { &gObjectSources[273], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[274], 0x0, 0x1, 0, 0, 0, 8, 48 },     { &gObjectSources[275], 0x0, 0x81, 0, 0, 0, 13, 0 },
	{ &gObjectSources[276], 0x0, 0x6003, 0, 0, 0, 22, 0 },  { &gObjectSources[277], 0x0, 0x23, 0, 1, 20, 9, 1 },
	{ &gObjectSources[278], 0x0, 0x1, 0, 1, 20, 9, 2 },     { &gObjectSources[279], 0x0, 0x81, 0, 0, 0, 10, 40 },
	{ &gObjectSources[280], 0x0, 0x181, 0, 0, 0, 17, 0 },   { &gObjectSources[281], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[282], 0x0, 0x81, 0, 1, 15, 8, 16 },   { &gObjectSources[283], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[284], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[285], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[286], 0x0, 0x1, 0, 1, 15, 8, 17 },    { &gObjectSources[287], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[288], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[289], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[290], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[291], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[292], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[293], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[294], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[295], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[296], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[297], 0x0, 0x21, 0, 0, 0, 22, 0 },
	{ &gObjectSources[298], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[299], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[300], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[301], 0x0, 0x1, 0, 0, 0, 5, 6 },
	{ &gObjectSources[302], 0x0, 0x4001, 0, 0, 0, 22, 0 },  { &gObjectSources[303], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[304], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[305], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[306], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[307], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[308], 0x0, 0x81, 0, 0, 0, 22, 0 },    { &gObjectSources[309], 0x0, 0x1, 0, 1, 15, 8, 18 },
	{ &gObjectSources[310], 0x0, 0x1, 0, 1, 15, 8, 19 },    { &gObjectSources[311], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[312], 0x0, 0x1, 0, 0, 0, 20, 1 },     { &gObjectSources[313], 0x0, 0x1, 0, 0, 0, 20, 18 },
	{ &gObjectSources[314], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[315], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[316], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[317], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[318], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[319], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[320], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[321], 0x0, 0x1, 0, 0, 0, 7, 101 },
	{ &gObjectSources[322], 0x0, 0x1, 0, 0, 0, 7, 102 },    { &gObjectSources[323], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[324], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[325], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[326], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[327], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[328], 0x0, 0x81, 0, 0, 0, 22, 0 },    { &gObjectSources[329], 0x0, 0x1, 0, 0, 0, 5, 0 },
	{ &gObjectSources[330], 0x0, 0x1, 0, 0, 0, 5, 0 },      { &gObjectSources[331], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[332], 0x0, 0x81, 0, 1, 15, 8, 21 },   { &gObjectSources[333], 0x0, 0x1, 0, 1, 15, 22, 0 },
	{ &gObjectSources[334], 0x0, 0x3, 0, 1, 15, 22, 0 },    { &gObjectSources[335], 0x0, 0x3, 0, 1, 15, 22, 0 },
	{ &gObjectSources[336], 0x0, 0x3, 0, 1, 15, 22, 0 },    { &gObjectSources[337], 0x0, 0x3, 0, 1, 15, 22, 0 },
	{ &gObjectSources[338], 0x0, 0x3, 0, 1, 15, 22, 0 },    { &gObjectSources[339], 0x0, 0x3, 0, 1, 15, 22, 0 },
	{ &gObjectSources[340], 0x0, 0x3, 0, 1, 15, 22, 0 },    { &gObjectSources[341], 0x0, 0x81, 0, 1, 15, 8, 22 },
	{ &gObjectSources[342], 0x0, 0x81, 0, 0, 0, 10, 50 },   { &gObjectSources[343], 0x0, 0x1, 0, 0, 0, 10, 51 },
	{ &gObjectSources[344], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[345], 0x0, 0x81, 0, 0, 0, 10, 60 },
	{ &gObjectSources[346], 0x0, 0x1, 0, 1, 15, 8, 23 },    { &gObjectSources[347], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[348], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[349], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[350], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[351], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[352], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[353], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[354], 0x0, 0x181, 0, 0, 0, 18, 0 },   { &gObjectSources[355], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[356], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[357], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[358], 0x0, 0x1, 0, 0, 0, 20, 18 },    { &gObjectSources[359], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[360], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[361], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[362], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[363], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[364], 0x0, 0x1, 0, 0, 0, 10, 70 },    { &gObjectSources[286], 0x0, 0x1, 0, 1, 15, 8, 24 },
	{ &gObjectSources[286], 0x0, 0x1, 0, 1, 15, 8, 25 },    { &gObjectSources[286], 0x0, 0x1, 0, 1, 15, 8, 26 },
	{ &gObjectSources[286], 0x0, 0x1, 0, 1, 15, 8, 27 },    { &gObjectSources[286], 0x0, 0x1, 0, 1, 15, 8, 28 },
	{ &gObjectSources[286], 0x0, 0x1, 0, 1, 15, 8, 29 },    { &gObjectSources[286], 0x0, 0x1, 0, 1, 15, 8, 30 },
	{ &gObjectSources[286], 0x0, 0x1, 0, 1, 15, 8, 31 },    { &gObjectSources[286], 0x0, 0x1, 0, 1, 15, 8, 32 },
	{ &gObjectSources[374], 0x0, 0x1, 0, 0, 0, 8, 33 },     { &gObjectSources[375], 0x0, 0x1, 0, 0, 0, 8, 34 },
	{ &gObjectSources[376], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[377], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[378], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[379], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[380], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[381], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[382], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[383], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[384], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[385], 0x0, 0x9, 3, 0, 0, 21, 0 },
	{ &gObjectSources[386], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[387], 0x0, 0x1, 0, 1, 15, 8, 35 },
	{ &gObjectSources[388], 0x0, 0x1, 0, 1, 15, 8, 36 },    { &gObjectSources[389], 0x0, 0x1, 0, 1, 15, 8, 37 },
	{ &gObjectSources[390], 0x0, 0x1, 0, 1, 15, 8, 38 },    { &gObjectSources[391], 0x0, 0x1, 0, 1, 15, 8, 39 },
	{ &gObjectSources[392], 0x0, 0x1, 0, 1, 15, 8, 40 },    { &gObjectSources[393], 0x0, 0x1, 0, 1, 15, 8, 41 },
	{ &gObjectSources[393], 0x0, 0x1, 0, 1, 15, 8, 42 },    { &gObjectSources[393], 0x0, 0x1, 0, 1, 15, 8, 43 },
	{ &gObjectSources[396], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[397], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[398], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[399], 0x0, 0x1, 0, 0, 0, 7, 300 },
	{ &gObjectSources[400], 0x0, 0x1, 0, 0, 0, 20, 50 },    { &gObjectSources[401], 0x0, 0x1, 0, 0, 0, 20, 50 },
	{ &gObjectSources[402], 0x0, 0x1, 0, 0, 0, 20, 50 },    { &gObjectSources[403], 0x0, 0x1, 0, 0, 0, 0, 0 },
	{ &gObjectSources[404], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[405], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[406], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[407], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[408], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[409], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[410], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[411], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[412], 0x0, 0x1, 0, 1, 15, 8, 44 },    { &gObjectSources[413], 0x0, 0x1, 0, 1, 15, 8, 45 },
	{ &gObjectSources[414], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[415], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[416], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[417], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[418], 0x0, 0x1, 0, 1, 15, 8, 46 },    { &gObjectSources[419], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[420], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[421], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[422], 0x0, 0x1, 0, 1, 15, 8, 47 },    { &gObjectSources[423], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[424], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[425], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[426], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[427], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[428], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[429], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[430], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[431], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[432], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[433], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[434], 0x0, 0x3, 0, 0, 0, 3, 0 },      { &gObjectSources[435], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[436], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[437], 0x0, 0x1, 0, 1, 15, 8, 49 },
	{ &gObjectSources[438], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[439], 0x0, 0x1, 0, 0, 0, 5, 2 },
	{ &gObjectSources[440], 0x0, 0x1, 0, 0, 0, 5, 3 },      { &gObjectSources[441], 0x0, 0x1, 0, 0, 0, 5, 4 },
	{ &gObjectSources[442], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[443], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[444], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[445], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[446], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[447], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[448], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[449], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[450], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[451], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[452], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[453], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[454], 0x0, 0x1, 0, 0, 0, 20, 18 },    { &gObjectSources[455], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[456], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[457], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[458], 0x0, 0x1, 0, 1, 15, 8, 51 },    { &gObjectSources[459], 0x0, 0x1, 0, 1, 15, 8, 52 },
	{ &gObjectSources[460], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[461], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[462], 0x0, 0x1, 0, 1, 15, 8, 53 },    { &gObjectSources[462], 0x0, 0x1, 0, 1, 15, 8, 54 },
	{ &gObjectSources[464], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[465], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[466], 0x0, 0x81, 0, 0, 0, 6, 0 },     { &gObjectSources[467], 0x0, 0x81, 0, 0, 0, 10, 0 },
	{ &gObjectSources[468], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[469], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[470], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[471], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[472], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[473], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[474], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[475], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[476], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[477], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[478], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[479], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[480], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[481], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[482], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[483], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[484], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[485], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[486], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[487], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[488], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[489], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[490], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[491], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[492], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[493], 0x0, 0x2001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[494], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[495], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[496], 0x0, 0x1, 0, 1, 15, 8, 55 },    { &gObjectSources[497], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[498], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[499], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[500], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[501], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[502], 0x0, 0x3, 0, 0, 0, 3, 0 },      { &gObjectSources[503], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[504], 0x0, 0x81, 0, 1, 15, 8, 56 },   { &gObjectSources[505], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[506], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[507], 0x0, 0x81, 0, 0, 0, 10, 100 },
	{ &gObjectSources[508], 0x0, 0x81, 0, 0, 0, 10, 80 },   { &gObjectSources[509], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[510], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[511], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[512], 0x0, 0x4001, 0, 0, 0, 22, 0 },  { &gObjectSources[513], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[514], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[515], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[516], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[517], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[518], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[519], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[520], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[521], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[522], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[523], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[524], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[525], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[526], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[527], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[528], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[529], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[530], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[531], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[532], 0x0, 0x1, 0, 1, 15, 8, 63 },    { &gObjectSources[533], 0x0, 0x1, 0, 1, 15, 8, 64 },
	{ &gObjectSources[534], 0x0, 0x1, 0, 1, 15, 8, 65 },    { &gObjectSources[535], 0x0, 0x1, 0, 1, 15, 8, 66 },
	{ &gObjectSources[536], 0x0, 0x1, 0, 1, 15, 8, 67 },    { &gObjectSources[537], 0x0, 0x1, 0, 1, 15, 8, 68 },
	{ &gObjectSources[538], 0x0, 0x1, 0, 1, 15, 8, 69 },    { &gObjectSources[539], 0x0, 0x1, 0, 1, 15, 8, 70 },
	{ &gObjectSources[540], 0x0, 0x1, 0, 1, 15, 8, 71 },    { &gObjectSources[541], 0x0, 0x1, 0, 1, 15, 8, 72 },
	{ &gObjectSources[542], 0x0, 0x9, 2, 1, 15, 8, 73 },    { &gObjectSources[543], 0x0, 0x1, 0, 1, 15, 8, 74 },
	{ &gObjectSources[544], 0x0, 0x1, 0, 1, 15, 8, 75 },    { &gObjectSources[545], 0x0, 0x1, 0, 1, 15, 8, 76 },
	{ &gObjectSources[546], 0x0, 0x1, 0, 1, 15, 8, 77 },    { &gObjectSources[547], 0x0, 0x1, 0, 1, 15, 8, 78 },
	{ &gObjectSources[548], 0x0, 0x1, 0, 1, 15, 8, 79 },    { &gObjectSources[549], 0x0, 0x1, 0, 1, 15, 8, 80 },
	{ &gObjectSources[550], 0x0, 0x1, 0, 1, 15, 8, 81 },    { &gObjectSources[551], 0x0, 0x1, 0, 1, 15, 8, 82 },
	{ &gObjectSources[552], 0x0, 0x1, 0, 1, 15, 8, 83 },    { &gObjectSources[553], 0x0, 0x1, 0, 1, 15, 8, 84 },
	{ &gObjectSources[554], 0x0, 0x1, 0, 1, 15, 8, 85 },    { &gObjectSources[555], 0x0, 0x1, 0, 1, 15, 8, 86 },
	{ &gObjectSources[556], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[557], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[558], 0x0, 0x1, 0, 1, 15, 8, 57 },    { &gObjectSources[559], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[560], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[561], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[562], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[563], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[564], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[565], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[566], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[567], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[568], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[569], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[570], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[571], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[572], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[573], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[574], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[575], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[576], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[577], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[578], 0x0, 0x1, 0, 1, 15, 8, 58 },    { &gObjectSources[579], 0x0, 0x1, 0, 1, 15, 8, 59 },
	{ &gObjectSources[580], 0x0, 0x1, 0, 1, 15, 8, 60 },    { &gObjectSources[581], 0x0, 0x1, 0, 1, 15, 8, 61 },
	{ &gObjectSources[582], 0x0, 0x1, 0, 1, 15, 8, 62 },    { &gObjectSources[583], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[584], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[585], 0x0, 0x1, 0, 0, 0, 20, 36 },
	{ &gObjectSources[586], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[587], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[588], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[589], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[590], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[591], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[592], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[593], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[594], 0x0, 0x2401, 0, 0, 0, 22, 0 },  { &gObjectSources[595], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[596], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[597], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[598], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[599], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[600], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[601], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[602], 0x0, 0x1, 0, 0, 0, 20, 18 },    { &gObjectSources[603], 0x0, 0x1, 0, 0, 0, 20, 18 },
	{ &gObjectSources[604], 0x0, 0x1, 0, 0, 0, 20, 18 },    { &gObjectSources[605], 0x0, 0x1, 0, 0, 0, 20, 50 },
	{ &gObjectSources[606], 0x0, 0x81, 0, 0, 0, 10, 120 },  { &gObjectSources[607], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[608], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[609], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[610], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[611], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[612], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[613], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[614], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[615], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[616], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[617], 0x0, 0x1, 0, 0, 0, 0, 0 },
	{ &gObjectSources[618], 0x0, 0x1, 0, 0, 0, 0, 0 },      { &gObjectSources[619], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[620], 0x0, 0x81, 0, 0, 0, 10, 90 },   { &gObjectSources[621], 0x0, 0x81, 0, 0, 0, 10, 110 },
	{ &gObjectSources[622], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[623], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[624], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[625], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[626], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[627], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[628], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[629], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[630], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[631], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[632], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[633], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[634], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[635], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[636], 0x0, 0x1, 0, 1, 15, 8, 87 },    { &gObjectSources[637], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[638], 0x0, 0x1, 0, 1, 15, 8, 88 },    { &gObjectSources[639], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[636], 0x0, 0x1, 0, 1, 15, 8, 89 },    { &gObjectSources[636], 0x0, 0x1, 0, 1, 15, 8, 90 },
	{ &gObjectSources[642], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[643], 0x0, 0x1, 0, 1, 15, 8, 91 },
	{ &gObjectSources[644], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[645], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[646], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[647], 0x0, 0x1, 0, 1, 15, 8, 92 },
	{ &gObjectSources[647], 0x0, 0x1, 0, 1, 15, 8, 93 },    { &gObjectSources[647], 0x0, 0x1, 0, 1, 15, 8, 94 },
	{ &gObjectSources[650], 0x0, 0x1, 0, 1, 15, 8, 95 },    { &gObjectSources[651], 0x0, 0x1, 0, 1, 15, 8, 96 },
	{ &gObjectSources[652], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[653], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[654], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[655], 0x0, 0x1, 0, 1, 15, 8, 98 },
	{ &gObjectSources[656], 0x0, 0x1, 0, 1, 15, 8, 99 },    { &gObjectSources[657], 0x0, 0x1, 0, 1, 15, 8, 100 },
	{ &gObjectSources[658], 0x0, 0x1, 0, 1, 15, 8, 101 },   { &gObjectSources[659], 0x0, 0x1, 0, 1, 15, 8, 102 },
	{ &gObjectSources[660], 0x0, 0x1, 0, 1, 15, 8, 103 },   { &gObjectSources[661], 0x0, 0x1, 0, 1, 15, 8, 104 },
	{ &gObjectSources[662], 0x0, 0x1, 0, 1, 15, 8, 105 },   { &gObjectSources[663], 0x0, 0x1, 0, 1, 15, 8, 106 },
	{ &gObjectSources[664], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[665], 0x0, 0x1, 0, 0, 0, 8, 107 },
	{ &gObjectSources[666], 0x0, 0x1, 0, 1, 15, 8, 108 },   { &gObjectSources[667], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[668], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[669], 0x0, 0x81, 0, 0, 0, 22, 8 },
	{ &gObjectSources[670], 0x0, 0x81, 0, 0, 0, 5, 8 },     { &gObjectSources[671], 0x0, 0x81, 0, 0, 0, 22, 0 },
	{ &gObjectSources[672], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[673], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[674], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[675], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[676], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[677], 0x0, 0x1, 0, 0, 0, 3, 0 },
	{ &gObjectSources[678], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[679], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[680], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[681], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[682], 0x0, 0x81, 0, 0, 0, 5, 8 },     { &gObjectSources[683], 0x0, 0x1, 0, 1, 15, 8, 109 },
	{ &gObjectSources[684], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[685], 0x0, 0x1, 0, 1, 15, 8, 110 },
	{ &gObjectSources[686], 0x0, 0x1, 0, 0, 0, 8, 111 },    { &gObjectSources[687], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[688], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[689], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[690], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[691], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[692], 0x0, 0x201, 0, 0, 0, 22, 0 },   { &gObjectSources[693], 0x0, 0x1, 0, 1, 15, 8, 112 },
	{ &gObjectSources[694], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[695], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[696], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[697], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[698], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[699], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[700], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[701], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[702], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[703], 0x0, 0x1, 0, 1, 15, 8, 97 },
	{ &gObjectSources[704], 0x0, 0x1, 0, 1, 15, 8, 113 },   { &gObjectSources[705], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[706], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[707], 0x0, 0x81, 0, 0, 0, 5, 19 },
	{ &gObjectSources[708], 0x0, 0x81, 0, 0, 0, 22, 0 },    { &gObjectSources[709], 0x0, 0x81, 0, 0, 0, 22, 0 },
	{ &gObjectSources[710], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[711], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[712], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[713], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[714], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[715], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[716], 0x0, 0x3, 0, 0, 0, 3, 0 },      { &gObjectSources[717], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[718], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[719], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[720], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[721], 0x0, 0x1001, 0, 0, 0, 20, 36 },
	{ &gObjectSources[722], 0x0, 0x3, 0, 0, 0, 3, 0 },      { &gObjectSources[723], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[724], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[725], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[726], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[727], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[728], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[729], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[730], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[731], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[732], 0x0, 0x1, 0, 0, 0, 10, 0 },     { &gObjectSources[733], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[734], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[735], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[736], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[737], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[738], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[739], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[740], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[741], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[742], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[743], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[744], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[745], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[746], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[747], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[748], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[749], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[750], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[751], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[752], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[753], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[754], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[755], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[756], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[757], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[758], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[759], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[760], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[761], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[762], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[763], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[764], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[763], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[764], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[763], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[763], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[764], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[763], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[764], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[763], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[764], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[763], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[763], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[776], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[777], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[778], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[779], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[780], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[781], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[782], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[783], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[784], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[785], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[786], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[787], 0x0, 0x201, 0, 0, 0, 22, 0 },
	{ &gObjectSources[788], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[789], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[790], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[791], 0x0, 0x81, 0, 0, 0, 10, 0 },
	{ &gObjectSources[792], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[793], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[794], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[795], 0x0, 0x1, 0, 0, 0, 5, 9 },
	{ &gObjectSources[796], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[797], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[798], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[799], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[800], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[801], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[802], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[803], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[682], 0x0, 0x1, 0, 1, 15, 8, 114 },   { &gObjectSources[805], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[806], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[807], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[808], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[809], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[810], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[811], 0x0, 0x1, 0, 1, 15, 8, 115 },
	{ &gObjectSources[812], 0x0, 0x1, 0, 1, 15, 8, 116 },   { &gObjectSources[813], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[814], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[815], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[816], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[817], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[818], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[819], 0x0, 0x1, 0, 1, 15, 8, 117 },
	{ &gObjectSources[820], 0x0, 0x1, 0, 1, 15, 8, 118 },   { &gObjectSources[821], 0x0, 0x1, 0, 1, 15, 8, 119 },
	{ &gObjectSources[822], 0x0, 0x1, 0, 1, 15, 8, 120 },   { &gObjectSources[823], 0x0, 0x1, 0, 1, 15, 8, 121 },
	{ &gObjectSources[824], 0x0, 0x1, 0, 1, 15, 8, 122 },   { &gObjectSources[825], 0x0, 0x1, 0, 1, 15, 8, 123 },
	{ &gObjectSources[826], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[827], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[828], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[829], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[830], 0x0, 0x1, 0, 1, 15, 8, 124 },   { &gObjectSources[831], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[832], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[833], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[834], 0x0, 0x1, 0, 1, 15, 8, 125 },   { &gObjectSources[835], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[836], 0x0, 0x1, 0, 1, 15, 8, 126 },   { &gObjectSources[837], 0x0, 0x1, 0, 1, 15, 8, 127 },
	{ &gObjectSources[838], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[839], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[840], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[841], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[842], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[843], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[844], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[845], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[846], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[847], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[848], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[849], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[850], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[851], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[852], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[853], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[854], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[855], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[856], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[857], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[858], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[859], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[860], 0x0, 0x1, 0, 1, 15, 8, 128 },   { &gObjectSources[861], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[862], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[863], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[864], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[865], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[866], 0x0, 0x1, 0, 1, 15, 8, 130 },   { &gObjectSources[867], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[868], 0x0, 0x1, 0, 1, 15, 8, 129 },   { &gObjectSources[869], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[870], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[871], 0x0, 0x1, 0, 1, 15, 8, 131 },
	{ &gObjectSources[872], 0x0, 0x1, 0, 1, 15, 8, 132 },   { &gObjectSources[873], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[874], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[875], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[876], 0x0, 0x81, 0, 0, 0, 22, 0 },    { &gObjectSources[877], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[878], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[879], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[880], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[881], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[882], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[883], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[884], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[885], 0x0, 0x1, 0, 1, 15, 8, 133 },
	{ &gObjectSources[886], 0x0, 0x81, 0, 0, 0, 15, 0 },    { &gObjectSources[887], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[888], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[889], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[890], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[891], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[892], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[893], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[894], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[895], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[896], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[897], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[898], 0x0, 0x1, 0, 0, 0, 20, 18 },    { &gObjectSources[899], 0x0, 0x1, 0, 0, 0, 20, 18 },
	{ &gObjectSources[900], 0x0, 0x181, 0, 0, 0, 19, 0 },   { &gObjectSources[901], 0x0, 0x81, 0, 0, 0, 22, 0 },
	{ &gObjectSources[902], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[903], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[904], 0x0, 0x1, 0, 0, 0, 20, 18 },    { &gObjectSources[905], 0x0, 0x1, 0, 0, 0, 20, 18 },
	{ &gObjectSources[906], 0x0, 0x1, 0, 0, 0, 20, 34 },    { &gObjectSources[907], 0x0, 0x801, 0, 0, 0, 22, 0 },
	{ &gObjectSources[908], 0x0, 0x401, 0, 0, 0, 22, 0 },   { &gObjectSources[909], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[467], 0x0, 0x81, 0, 1, 15, 8, 134 },  { &gObjectSources[911], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[912], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[913], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[914], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[915], 0x0, 0x81, 0, 0, 0, 22, 0 },
	{ &gObjectSources[916], 0x0, 0x81, 0, 0, 0, 22, 0 },    { &gObjectSources[917], 0x0, 0x81, 0, 0, 0, 22, 0 },
	{ &gObjectSources[918], 0x0, 0x81, 0, 0, 0, 22, 0 },    { &gObjectSources[919], 0x0, 0x1, 0, 1, 15, 8, 135 },
	{ &gObjectSources[920], 0x0, 0x1, 0, 1, 15, 8, 136 },   { &gObjectSources[921], 0x0, 0x1, 0, 1, 15, 8, 137 },
	{ &gObjectSources[922], 0x0, 0x1, 0, 1, 15, 8, 138 },   { &gObjectSources[923], 0x0, 0x1, 0, 1, 15, 8, 139 },
	{ &gObjectSources[924], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[925], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[926], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[927], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[928], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[929], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[930], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[931], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[932], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[933], 0x0, 0x1, 0, 1, 15, 8, 140 },
	{ &gObjectSources[934], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[935], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[936], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[937], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[938], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[939], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[940], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[941], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[942], 0x0, 0x81, 0, 0, 0, 22, 0 },    { &gObjectSources[943], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[944], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[945], 0x0, 0x81, 0, 0, 0, 22, 0 },
	{ &gObjectSources[946], 0x0, 0x3, 0, 0, 0, 3, 0 },      { &gObjectSources[947], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[948], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[949], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[950], 0x0, 0x1, 0, 1, 15, 8, 141 },   { &gObjectSources[950], 0x0, 0x1, 0, 1, 15, 8, 142 },
	{ &gObjectSources[950], 0x0, 0x1, 0, 1, 15, 8, 143 },   { &gObjectSources[950], 0x0, 0x1, 0, 1, 15, 8, 144 },
	{ &gObjectSources[950], 0x0, 0x1, 0, 1, 15, 8, 145 },   { &gObjectSources[955], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[956], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[957], 0x0, 0x1, 0, 0, 0, 5, 12 },
	{ &gObjectSources[958], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[959], 0x0, 0x1, 0, 0, 0, 5, 8 },
	{ &gObjectSources[343], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[961], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[962], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[963], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[964], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[965], 0x0, 0x1, 0, 0, 0, 20, 18 },
	{ &gObjectSources[966], 0x0, 0x1, 0, 0, 0, 20, 18 },    { &gObjectSources[967], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[968], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[969], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[970], 0x0, 0x3, 0, 0, 0, 22, 0 },     { &gObjectSources[971], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[972], 0x0, 0x801, 0, 0, 0, 22, 0 },   { &gObjectSources[973], 0x0, 0x1, 0, 0, 0, 5, 11 },
	{ &gObjectSources[974], 0x0, 0x1, 0, 0, 0, 5, 11 },     { &gObjectSources[975], 0x0, 0x1, 0, 0, 0, 5, 11 },
	{ &gObjectSources[976], 0x0, 0x1, 0, 0, 0, 5, 11 },     { &gObjectSources[977], 0x0, 0x1, 0, 0, 0, 5, 11 },
	{ &gObjectSources[978], 0x0, 0x1, 0, 0, 0, 5, 11 },     { &gObjectSources[979], 0x0, 0x1, 0, 0, 0, 5, 11 },
	{ &gObjectSources[980], 0x0, 0x1, 0, 0, 0, 5, 11 },     { &gObjectSources[981], 0x0, 0x1, 0, 0, 0, 5, 11 },
	{ &gObjectSources[982], 0x0, 0x1, 0, 1, 15, 8, 146 },   { &gObjectSources[983], 0x0, 0x1, 0, 1, 15, 8, 147 },
	{ &gObjectSources[984], 0x0, 0x1, 0, 1, 15, 8, 148 },   { &gObjectSources[985], 0x0, 0x1, 0, 1, 15, 8, 149 },
	{ &gObjectSources[986], 0x0, 0x1, 0, 1, 15, 8, 150 },   { &gObjectSources[987], 0x0, 0x1, 0, 1, 15, 8, 151 },
	{ &gObjectSources[988], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[989], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[990], 0x0, 0x4001, 0, 0, 0, 22, 0 },  { &gObjectSources[991], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[992], 0x0, 0x1, 0, 0, 0, 5, 13 },     { &gObjectSources[993], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[994], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[995], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[996], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[997], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[998], 0x0, 0x1, 0, 0, 0, 22, 0 },     { &gObjectSources[999], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1000], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1001], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1002], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1003], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1004], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1005], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1006], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1007], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1008], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1009], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1010], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1011], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1012], 0x0, 0x3, 0, 0, 0, 22, 0 },    { &gObjectSources[1013], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1014], 0x0, 0x3, 0, 0, 0, 3, 0 },     { &gObjectSources[1015], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1016], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1017], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1018], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1019], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1020], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1021], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1022], 0x0, 0x1, 0, 0, 0, 5, 2 },     { &gObjectSources[1023], 0x0, 0x1, 0, 0, 0, 5, 3 },
	{ &gObjectSources[1024], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1025], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1026], 0x0, 0x3, 0, 0, 0, 22, 0 },    { &gObjectSources[1027], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1028], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1029], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1030], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1031], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1032], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1033], 0x0, 0x801, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1034], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1035], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1036], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1037], 0x0, 0x1, 0, 0, 0, 5, 16 },
	{ &gObjectSources[1038], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1039], 0x0, 0x1, 0, 0, 0, 5, 2 },
	{ &gObjectSources[1040], 0x0, 0x1, 0, 0, 0, 5, 3 },     { &gObjectSources[1041], 0x0, 0x1, 0, 0, 0, 5, 2 },
	{ &gObjectSources[1042], 0x0, 0x1, 0, 0, 0, 5, 3 },     { &gObjectSources[1043], 0x0, 0x1, 0, 0, 0, 5, 2 },
	{ &gObjectSources[1044], 0x0, 0x1, 0, 0, 0, 5, 3 },     { &gObjectSources[647], 0x0, 0x1, 0, 1, 15, 8, 152 },
	{ &gObjectSources[1046], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1047], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1048], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1049], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1050], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1051], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1052], 0x0, 0x3, 0, 0, 0, 22, 0 },    { &gObjectSources[1053], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1054], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1055], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1056], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1057], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1058], 0x0, 0x1, 0, 1, 15, 8, 153 },  { &gObjectSources[1059], 0x0, 0x1, 0, 0, 0, 17, 0 },
	{ &gObjectSources[1060], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1061], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1062], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1063], 0x0, 0x1, 0, 1, 15, 8, 154 },
	{ &gObjectSources[1064], 0x0, 0x1, 0, 1, 15, 8, 155 },  { &gObjectSources[1065], 0x0, 0x81, 0, 0, 0, 5, 0 },
	{ &gObjectSources[1066], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1067], 0x0, 0x1, 0, 0, 0, 5, 14 },
	{ &gObjectSources[1068], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1069], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1070], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1071], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1072], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1073], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1074], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1075], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1076], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1077], 0x0, 0x1, 0, 0, 0, 5, 2 },
	{ &gObjectSources[1078], 0x0, 0x1, 0, 0, 0, 5, 3 },     { &gObjectSources[1079], 0x0, 0x1, 0, 0, 0, 5, 4 },
	{ &gObjectSources[1080], 0x0, 0x3, 0, 0, 0, 5, 18 },    { &gObjectSources[1081], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1082], 0x0, 0x1, 0, 0, 0, 5, 15 },    { &gObjectSources[1083], 0x0, 0x4003, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1084], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1085], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1086], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1087], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1088], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1089], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1090], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1091], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1092], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1093], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1094], 0x0, 0x81, 0, 0, 0, 22, 0 },   { &gObjectSources[1095], 0x0, 0x81, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1096], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1097], 0x0, 0x1, 0, 0, 0, 0, 0 },
	{ &gObjectSources[1098], 0x0, 0x1, 0, 1, 15, 8, 156 },  { &gObjectSources[1099], 0x0, 0x1, 0, 1, 15, 8, 157 },
	{ &gObjectSources[1100], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1101], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1102], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1103], 0x0, 0x81, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1104], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1105], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[274], 0x0, 0x1, 0, 0, 0, 5, 0 },      { &gObjectSources[1107], 0x0, 0x4001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1108], 0x0, 0x4001, 0, 0, 0, 22, 0 }, { &gObjectSources[1109], 0x0, 0x4001, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1110], 0x0, 0x4001, 0, 0, 0, 22, 0 }, { &gObjectSources[1111], 0x0, 0x1001, 0, 0, 0, 20, 50 },
	{ &gObjectSources[1112], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1113], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1114], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1115], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1116], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1117], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1118], 0x0, 0x3, 0, 0, 0, 22, 0 },    { &gObjectSources[1119], 0x0, 0x801, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1120], 0x0, 0x801, 0, 0, 0, 22, 0 },  { &gObjectSources[1121], 0x0, 0x801, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1122], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1123], 0x0, 0x801, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1124], 0x0, 0x81, 0, 0, 0, 22, 0 },   { &gObjectSources[1125], 0x0, 0x81, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1126], 0x0, 0x81, 0, 0, 0, 22, 0 },   { &gObjectSources[1127], 0x0, 0x3, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1128], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1129], 0x0, 0x1, 0, 0, 0, 8, 158 },
	{ &gObjectSources[1130], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1131], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1132], 0x0, 0x3, 0, 0, 0, 22, 0 },    { &gObjectSources[1133], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1134], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1135], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1136], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1137], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1138], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1139], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1140], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1141], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1142], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1143], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1144], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1145], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1146], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1147], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1148], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1149], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1150], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1151], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1152], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1153], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1154], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1155], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1156], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1157], 0x0, 0x81, 0, 0, 0, 10, 90 },
	{ &gObjectSources[1158], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSources[1159], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[1160], 0x0, 0x4001, 0, 0, 0, 22, 0 }, { &gObjectSources[1161], 0x0, 0x1, 0, 0, 0, 22, 0 },
};

XObjSource gObjectSourcesEx[] = {
	{ "sample", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/shortplay_title", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "charge_fx01", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "cb_propeller_lv2", NULL, 0x00000000, 0x00000000, sCbPropellerLv2Anims, 0 },
	{ "cb_cannon_lv2", NULL, 0x00000000, 0x00000000, sCbCannonLv2Anims, 0 },
	{ "cb_radar", NULL, 0x00000000, 0x00000000, sCbRadarAnims, 0 },
	{ "maru_batu", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "exclamation", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "kisekae/parts_keath", NULL, 0x00000000, 0x00000000, sCbRoboAnims, 0 },
	{ "kisekae/parts_kaeru", NULL, 0x00000000, 0x00000000, sCbRoboAnims, 0 },
	{ "kisekae/parts_tao", NULL, 0x00000000, 0x00000000, sCbRoboAnims, 0 },
	{ "kisekae/parts_kega", NULL, 0x00000000, 0x00000000, sCbRoboAnims, 0 },
	{ "kisekae/parts_tao", NULL, 0x00000000, 0x00000000, sCbRoboAnims, 0 },
	{ "kisekae/parts_obake", NULL, 0x00000000, 0x00000000, sCbRoboAnims, 0 },
	{ "kisekae/parts_pajama", NULL, 0x00000000, 0x00000000, sCbRoboAnims, 0 },
	{ "kisekae/parts_super", NULL, 0x00000000, 0x00000000, sCbRoboAnims, 0 },
	{ "kisekae/parts_pajama_01", NULL, 0x00000000, 0x00000000, sCbRoboAnims, 0 },
	{ "kisekae/parts_pajama_02", NULL, 0x00000000, 0x00000000, sCbRoboAnims, 0 },
	{ "kisekae/parts_tao_add", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "kisekae/parts_obake_add", NULL, 0x00000000, 0x00000000, sKisekaePartsObakeAddAnims, 0 },
	{ "title_icon/icon_bb", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "title_icon/icon_army", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "title_icon/icon_fafa", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "title_icon/icon_funky", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "title_icon/icon_hock", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "title_icon/icon_keath", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "title_icon/icon_peets", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "title_icon/icon_sunmida", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "title_icon/icon_trex", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "title_icon/icon_zobin", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "title_icon/icon_army2", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "us_maru_batu", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "us_maru_model", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "us_batu_model", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/wardrobe", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/wardrobe", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/shortplay_a", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/shortplay_b", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ "test/shortplay_c", NULL, 0x00000000, 0x00000000, NULL, 0 },
	{ NULL, NULL, 0xFFFFFFFF, 0xFFFFFFFF, NULL, 0 },
};

// Extended object IDs start at 10000 and index this table in GetObjData.
UnkObjData objTableEx[] = {
	{ &gObjectSourcesEx[20], 0x0, 0x1, 0, 0, 0, 22, 0 },   { &gObjectSourcesEx[21], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSourcesEx[22], 0x0, 0x1, 0, 0, 0, 22, 0 },   { &gObjectSourcesEx[23], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSourcesEx[24], 0x0, 0x1, 0, 0, 0, 22, 0 },   { &gObjectSourcesEx[25], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSourcesEx[26], 0x0, 0x1, 0, 0, 0, 22, 0 },   { &gObjectSourcesEx[27], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSourcesEx[28], 0x0, 0x1, 0, 0, 0, 22, 0 },   { &gObjectSourcesEx[29], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSourcesEx[30], 0x0, 0x1, 0, 0, 0, 22, 0 },   { &gObjectSourcesEx[1], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSources[15], 0x0, 0x81, 0, 0, 0, 22, 0 },    { &gObjectSourcesEx[2], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSourcesEx[3], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSourcesEx[4], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSourcesEx[5], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSourcesEx[6], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSourcesEx[7], 0x0, 0x1, 0, 0, 0, 22, 0 },    { &gObjectSourcesEx[8], 0x0, 0x281, 0, 0, 0, 22, 0 },
	{ &gObjectSourcesEx[9], 0x0, 0x281, 0, 0, 0, 22, 0 },  { &gObjectSourcesEx[10], 0x0, 0x281, 0, 0, 0, 22, 0 },
	{ &gObjectSourcesEx[11], 0x0, 0x281, 0, 0, 0, 22, 0 }, { &gObjectSourcesEx[12], 0x0, 0x281, 0, 0, 0, 22, 0 },
	{ &gObjectSourcesEx[13], 0x0, 0x281, 0, 0, 0, 22, 0 }, { &gObjectSourcesEx[14], 0x0, 0x281, 0, 0, 0, 22, 0 },
	{ &gObjectSourcesEx[15], 0x0, 0x281, 0, 0, 0, 22, 0 }, { &gObjectSourcesEx[16], 0x0, 0x281, 0, 0, 0, 22, 0 },
	{ &gObjectSourcesEx[17], 0x0, 0x281, 0, 0, 0, 22, 0 }, { &gObjectSourcesEx[18], 0x0, 0x201, 0, 0, 0, 22, 0 },
	{ &gObjectSourcesEx[19], 0x0, 0x201, 0, 0, 0, 22, 0 }, { &gObjectSourcesEx[34], 0x0, 0x401, 0, 0, 0, 22, 0 },
	{ &gObjectSourcesEx[35], 0x0, 0x401, 0, 0, 0, 22, 0 }, { &gObjectSources[312], 0x0, 0x1, 0, 0, 0, 20, 1 },
	{ &gObjectSources[313], 0x0, 0x1, 0, 0, 0, 20, 50 },   { &gObjectSourcesEx[31], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSourcesEx[32], 0x0, 0x1, 0, 0, 0, 22, 0 },   { &gObjectSourcesEx[33], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSourcesEx[36], 0x0, 0x1, 0, 0, 0, 22, 0 },   { &gObjectSourcesEx[37], 0x0, 0x1, 0, 0, 0, 22, 0 },
	{ &gObjectSourcesEx[38], 0x0, 0x1, 0, 0, 0, 22, 0 },
};

// Object IDs from 10000 reuse these collision lists. A negative entry has no list.
s32 gObjectCollisionIdRemap[] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  -1,  -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 19, 19, 312, 313, -1, -1, -1, -1, -1, -1, 0,
};

StagePlacement spawnTable[] = {
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
	{ 10011, { 0, 0, 0 }, { 0, 0, 0 }, { 104, 104, 104 }, 0, 0, 0, 0x3 },
	{ 10012, { 0, -1500, 0 }, { 0, 0, 0 }, { 600, 600, 600 }, 0, 0, 1, 0x3 },
	{ 10012, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 1, 0x3 },
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
	{ 10029, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10030, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 399, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 1, 0x3 },
	{ 444, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1000003 },
	{ 260, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x2000007 },
	{ 261, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x2000007 },
	{ 321, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x2000007 },
	{ 322, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x2000007 },
	{ 428, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1000003 },
	{ 435, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1000003 },
	{ 10035, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x3 },
	{ 10036, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1 },
	{ 10037, { 0, 0, 0 }, { 0, 0, 0 }, { 100, 100, 100 }, 0, 0, 0, 0x1 },
	{ 10038, { 0, 0, 0 }, { 0, 0, 0 }, { 104, 104, 104 }, 0, 0, 0, 0x3 },
	{ 10039, { 0, 0, 0 }, { 0, 0, 0 }, { 104, 104, 104 }, 0, 0, 0, 0x3 },
	{ 10040, { 0, 0, 0 }, { 0, 0, 0 }, { 104, 104, 104 }, 0, 0, 0, 0x3 },
};
}

extern "C" const char* gResourceDirectorySuffixByLanguage[] = {
	"", "/e", "/g", "/f", "/s", "/i", "/e", NULL,
};
extern "C" const char* gGlobalResourceFileName = "globals.bin";

// the Wii's table also names each stage's sound bank and the bank's first sound id
StageDesc stageTable[] = {
	{ "stage00", "stage00", NULL, -1, 0, 5 },
	{ "stage01", "stage01", "sfx_kitchen.ssm", 0x245, 1, 5 },
	{ "stage02", "stage02", "sfx_stage02.ssm", 0x22d, 2, 5 },
	{ "stage03", "stage03", "sfx_souko.ssm", 0x24d, 3, 5 },
	{ "stage04", "stage04", "sfx_jennyroom.ssm", 0x23e, 4, 5 },
	{ "stage05", "stage05", "sfx_stage05.ssm", 0x252, 5, 4 },
	{ "stage06", "stage06", "sfx_papamama.ssm", 0x237, 6, 5 },
	{ "stage07", "stage07", "sfx_stage07.ssm", 0x233, 7, 5 },
	{ "stage08", "stage08", NULL, -1, 8, 5 },
	{ "stage09", "stage09", "sfx_uraniwa.ssm", 0x249, 9, 5 },
	{ "stage10", "stage10", NULL, -1, 10, 1 },
	{ "stage11", "stage11", "sfx_pipe.ssm", 0x259, 11, 5 },
	{ "stage12", "stage12", NULL, -1, 12, 5 },
	{ "stage13", "stage13", "sfx_manual.ssm", -1, 13, 5 },
	{ "stage14", "stage14", "sfx_stage07.ssm", -1, 14, 5 },
	{ "stage15", "stage15", NULL, -1, 15, 5 },
	{ "stage16", "stage16", "sfx_ufo.ssm", 0x25f, 16, 5 },
	{ "stage17", "stage17", NULL, -1, 17, 5 },
	{ "stage18", "stage18", "sfx_ufo.ssm", 0x237, 18, 5 },
	{ "stage19", "stage19", "sfx_stage07.ssm", -1, 19, 5 },
	{ "stage20", "stage20", NULL, -1, 20, 5 },
	{ "stage21", "stage21", NULL, -1, 21, 5 },
	{ "stage22", "stage22", "sfx_martial.ssm", -1, 22, 5 },
	{ "stage23", "stage23", NULL, -1, 23, 5 },
	{ "stage24", "stage24", NULL, -1, 24, 5 },
	{ "stage25", "stage25", NULL, -1, 25, 5 },
	{ "stage26", "stage26", NULL, -1, 26, 5 },
	{ "stage27", "stage27", NULL, -1, 27, 5 },
	{ "stage28", "stage07", NULL, -1, 28, 5 },
	{ "stage29", "stage07", NULL, -1, 29, 5 },
	{ NULL, NULL, NULL, -1, -1, 0 },
	{ "shun", "shun", NULL, -1, -1, 5 },
	{ "hori", "hori", "sfx_manual.ssm", -1, -1, 5 },
	{ "ahk", "ahk", NULL, -1, -1, 5 },
	{ "junko", "junko", NULL, -1, -1, 5 },
	{ "sayoko", "sayoko", NULL, -1, -1, 5 },
	{ "mory", "mory", NULL, -1, -1, 5 },
	{ "ryosuke", "ryosuke", NULL, -1, -1, 5 },
	{ "takanabe", "takanabe", NULL, -1, -1, 5 },
	{ "mariko", "mariko", NULL, -1, -1, 5 },
	{ NULL, NULL, NULL, 0, 0, 0 },
};

extern "C" {
const char** gInterfaceTextByLanguage[] = {
	gJapaneseInterfaceText, gEnglishInterfaceText, gGermanInterfaceText,  gFrenchInterfaceText,
	gSpanishInterfaceText,  gItalianInterfaceText, gEnglishInterfaceText,
};

const char** gPCDialogueByLanguage[] = {
	gJapanesePCDialogue, gEnglishPCDialogue, gGermanPCDialogue,  gFrenchPCDialogue,
	gSpanishPCDialogue,  gItalianPCDialogue, gEnglishPCDialogue,
};
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

static inline const char* StageMessage(s32 index, s32 member)
{
	StageDesc* desc = &stageTable[StageIndex(index)];
	s32 message     = desc->message;
	if (message >= 0) {
		u8* data   = gGlobalScriptResources.mData;
		u8* record = data + ReadResourceOffset(data + GLOBAL_STAGE_NAMES) + message * 8;
		return (const char*)(data + ReadResourceOffset(record + member));
	}
	return NULL;
}

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
 * @note Address: 0x80092308
 * @note Size: 0x28
 */
extern "C" s32 GetStageNum()
{
	return StageCount();
}

/**
 * @note Address: 0x80092330
 * @note Size: 0x4C
 */
extern "C" const char* GetStageFileName(s32 index)
{
	index = StageIndex(index);
	return stageTable[index].fileName;
}

/**
 * @note Address: 0x8009237C
 * @note Size: 0xCC
 */
extern "C" const char* GetStageName(s32 index)
{
	return StageMessage(index, 0);
}

/**
 * @note Address: 0x80092448
 * @note Size: 0xCC
 */
extern "C" const char* GetStageDesc(s32 index)
{
	return StageMessage(index, 4);
}

/**
 * @note Address: 0x80092514
 * @note Size: 0x124
 */
extern "C" const char* GetStageNameOrFileName(s32 index)
{
	const char* name = GetStageName(index);
	if (name == NULL)
		name = GetStageFileName(index);
	return name;
}

/**
 * @note Address: 0x80092638
 * @note Size: 0xE4
 */
extern "C" s32 FindStageIndexByName(const char* name)
{
	s32 index = 0;
	while (stageTable[index].fileName != NULL) { // the direct ids before the null entry
		if (strcmp(name, stageTable[index].fileName) == 0)
			return index;
		++index;
	}
	++index; // past the null entry the ids are 100-based
	while (stageTable[index].fileName != NULL) {
		if (strcmp(name, stageTable[index].fileName) == 0)
			return index - StageCount() + 99;
		++index;
	}
	return -1;
}

/**
 * @note Address: 0x8009271C
 * @note Size: 0x50
 */
extern "C" s32 GetStageFirstSoundId(s32 index)
{
	return stageTable[StageIndex(index)].firstSoundId;
}

/**
 * @note Address: 0x8009276C
 * @note Size: 0x54
 */
extern "C" u32 CheckStageFlag(s32 index, u32 mask)
{
	StageDesc* desc = &stageTable[StageIndex(index)];
	return mask & desc->flags;
}

extern "C" BOOL AdvanceStageLoad(Stage* stage, s32 index);

/**
 * @note Address: 0x800927C0
 * @note Size: 0x10
 */
extern "C" BOOL LoadStageByIndex(s32 index)
{
	return AdvanceStageLoad(&gStageResources, index);
}

/**
 * @note Address: 0x800927D0
 * @note Size: 0x60
 */
extern "C" void UnloadStageDataAndModule()
{
	_prolog(6, 0, NULL);
	gStageResources.mModule.Unload();
	CFile* file = &gStageResources.mDataFile;
	file->Close();
	gStageResources.mState = 0;
}

/**
 * @note Address: 0x80092830
 * @note Size: 0x68
 */
extern "C" void ApplyStageClearColor()
{
	// the environment record's first word is the VI background colour; the by-value GXColor
	// argument materializes as byte copies from the unaligned record
	GXSetCopyClear(*(GXColor*)gStageResources.mEnvironment.mData, 0xFFFFFF);
	gStageResources.mModule.Print();
}

/**
 * @note Address: 0x80092898
 * @note Size: 0x1C
 */
extern "C" BOOL IsStageDataLoaded()
{
	return gStageResources.mDataFile.mBuffer != NULL;
}

/**
 * @note Address: 0x800928B4
 * @note Size: 0x10
 */
extern "C" EnvInfo* GetStageEnvironment()
{
	return &gStageResources.mEnvironment;
}

/**
 * @note Address: 0x800928C4
 * @note Size: 0x18
 */
extern "C" StagePlacement* GetStagePlacementByIndex(s32 index)
{
	return gStageResources.mPlacements.mPlacement + index;
}

/**
 * @note Address: 0x800928DC
 * @note Size: 0x10
 */
extern "C" u8* GetStageDataBuffer()
{
	return (u8*)gStageResources.mDataFile.mBuffer;
}

/**
 * @note Address: 0x800928EC
 * @note Size: 0x54
 */
extern "C" u8* GetStageScriptByIndex(s32 index)
{
	u32 offset = ReadResourceOffset(gStageResources.mScriptOffsets + index * 4);
	return offset == 0 ? NULL : (u8*)gStageResources.mDataFile.mBuffer + offset;
}

/**
 * @note Address: 0x80092940
 * @note Size: 0x10
 */
extern "C" u8* GetStageSetupScript()
{
	return gStageResources.mSetupScript;
}

/**
 * @note Address: 0x80092950
 * @note Size: 0x10
 */
extern "C" u8* GetStageEntryScript()
{
	return gStageResources.mEntryScript;
}

/**
 * @note Address: 0x80092960
 * @note Size: 0x10
 */
extern "C" u8* GetStageBatteryDepletedScript()
{
	return gStageResources.mBatteryDepletedScript;
}

/**
 * @note Address: 0x80092970
 * @note Size: 0x10
 */
extern "C" u8* GetStageZButtonScript()
{
	return gStageResources.mZButtonScript;
}

/**
 * @note Address: 0x80092980
 * @note Size: 0x10
 */
extern "C" u8* GetStageTimerExpiredScript()
{
	return gStageResources.mTimerExpiredScript;
}

/**
 * @note Address: 0x80092990
 * @note Size: 0x10
 */
extern "C" const StageEventObject* GetStageEventObjectPairs()
{
	return gStageResources.mEventObjectPairs;
}

/**
 * @note Address: 0x800929A0
 * @note Size: 0x10
 */
extern "C" CamAreaRecord* GetStageCameraAreaRecords()
{
	return (CamAreaRecord*)gStageResources.mCameraAreaRecords;
}

/**
 * @note Address: 0x800929B0
 * @note Size: 0x10
 */
extern "C" CamInfoData* GetStageCameraInfoRecords()
{
	return (CamInfoData*)gStageResources.mCameraInfoRecords;
}

/**
 * @note Address: 0x800929C0
 * @note Size: 0x10
 */
extern "C" CamAreaRecord* GetStageCameraAreaExRecords()
{
	return (CamAreaRecord*)gStageResources.mCameraAreaExRecords;
}

/**
 * @note Address: 0x800929D0
 * @note Size: 0x1C
 */
extern "C" s32 GetStageCollectionFlagBase(s32 index)
{
	return ((s16*)(gStageResources.mEnvironment.mData + 0xA))[index];
}

// Three Wii callers query this switch. The retail implementation always returns zero.
/**
 * @note Address: 0x800929EC
 * @note Size: 0x8
 */
extern "C" s32 fn_800929EC()
{
	return 0;
}

/**
 * @note Address: 0x800929F4
 * @note Size: 0x4
 */
extern "C" s32 CallStageModule(s32 function, s32 count, s32* args)
{
	return _prolog(function, count, args);
}

/**
 * @note Address: 0x800929F8
 * @note Size: 0x4D0
 */
extern "C" BOOL AdvanceStageLoad(Stage* stage, s32 index)
{
	switch (stage->mState) {
	case 0: {
		strcpy(stage->mDataPath,
		       FormatToStaticBuffer("bin%s/%s.bin", gResourceDirectorySuffixByLanguage[gGame.mLanguage], stageTable[index].fileName));
		stage->mModulePath[0] = 0;
		++stage->mState;
	}
	// fall through, every step that's done moves on in the same frame
	case 1: {
		// read the stage's data file and resolve its sections
		if (!stage->mDataFile.ReadEx(stage->mDataPath, 0, 0))
			break;
		stage->mDataFile.CloseHandle();
		u8* data                      = (u8*)stage->mDataFile.mBuffer;
		const StageFileHeader* header = (const StageFileHeader*)data;
		stage->mEnvironment.mData     = data + ReadResourceOffset(header->environment);
		stage->mPlacements.mPlacement = (StagePlacement*)(data + ReadResourceOffset(header->placements));
		stage->mScriptOffsets         = data + ReadResourceOffset(header->scriptOffsets);
		stage->mSetupScript           = data + ReadResourceOffset(header->setupScript);
		if (stage->mSetupScript == data) // a zero script offset means no script
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
	case 2: // load the stage's rel module when it has one
		if ((s8)stage->mModulePath[0] != 0) {
			if (!stage->mModule.Load(stage->mModulePath))
				break;
			++stage->mState;
		} else {
			++stage->mState;
		}
	case 3:
		++stage->mState;
	case 4: // the stage's sound bank
		if (stageTable[index].soundBank) {
			LoadWiiSoundBankByName(&gGame.mSound, stageTable[index].soundBank);
			++stage->mState;
		} else {
			stage->mState += 2;
		}
		break;
	case 5:
		// The Wii sound system clears this flag when the bank finishes loading.
		if (!gGame.mSound.mLoadPending)
			++stage->mState;
		break;
	}
	return stage->mState > 5;
}

/**
 * @note Address: 0x80092EC8
 * @note Size: 0xBC
 */
extern "C" void fn_80092EC8(void)
{
	XObjSource* entry = gObjectSources;
	for (;;) {
		if (entry->_08 == -1)
			break;
		entry->mAnimCount = 0;
		if (entry->mAnimNames != NULL)
			while (entry->mAnimNames[entry->mAnimCount] != NULL)
				entry->mAnimCount++;
		entry++;
	}
	entry = gObjectSourcesEx;
	for (;;) {
		if (entry->_08 == -1)
			break;
		entry->mAnimCount = 0;
		if (entry->mAnimNames != NULL)
			while (entry->mAnimNames[entry->mAnimCount] != NULL)
				entry->mAnimCount++;
		entry++;
	}
}

/**
 * @note Address: 0x80092F84
 * @note Size: 0x3C
 */
extern "C" UnkObjData* GetObjData(const CoordInfo* info)
{
	s32 index = info->mPlacement->mObjectId;
	if (index >= 10000)
		return &objTableEx[index - 10000];
	return &objTable[index];
}

/**
 * @note Address: 0x80092FC0
 * @note Size: 0x14
 */
extern "C" StagePlacement* GetSpawnData(s32 index)
{
	return &spawnTable[index];
}

/**
 * @note Address: 0x80092FD4
 * @note Size: 0x14
 */
extern "C" StagePlacement* GetStageObjectPlacementByIndex(s32 index)
{
	return &gStageObjectPlacements[index];
}

/**
 * @note Address: 0x80092FE8
 * @note Size: 0x24
 */
extern "C" s32 GetItemIconTextureIndex(s32 index)
{
	if (index < 0)
		return -1;
	return gItemIconTextureIndices[index];
}

// the object archives are ResourceFile slots from a shared pool, claimed as objects spawn
ResourceFile* gObjResourceFiles;
s32 gObjResourceFileCount;

/**
 * @note Address: 0x8009300C
 * @note Size: 0x12C
 */
void AttrInfo::Init(CoordInfo* info)
{
	UnkObjData* objData       = GetObjData(info);
	StagePlacement* placement = info->mPlacement;
	ResourceFile* files       = gObjResourceFiles;
	mObjData                  = objData;
	mPlacement                = placement;
	if (files == NULL)
		gObjResourceFiles = new (4) ResourceFile[0x530];
	if ((mObjData ? mObjData->mSource : NULL) == NULL)
		return;
	if ((mObjData ? mObjData->mSource : NULL) ? mObjData->mSource->mArchive : NULL)
		return;
	(mObjData ? mObjData->mSource : NULL)->mArchive = &gObjResourceFiles[gObjResourceFileCount++];
}

// reads the globals file and finds its three parts
/**
 * @note Address: 0x80093138
 * @note Size: 0x144
 */
extern "C" BOOL LoadGlobalResourceData()
{
	char path[0x40];
	sprintf(path, "bin%s/%s", gResourceDirectorySuffixByLanguage[gGame.mLanguage], gGlobalResourceFileName);
	CFile* file = &gGlobalScriptResources.mFile;
	if (file->ReadEx(path, 0, 0)) {
		gGlobalScriptResources.mFile.CloseHandle();
		gGlobalScriptResources.mHeader    = (GlobalResourceHeader*)gGlobalScriptResources.mFile.mBuffer;
		GlobalResourceHeader* header      = gGlobalScriptResources.mHeader;
		u8* buffer                        = (u8*)header;
		gGlobalScriptResources.mData      = buffer + sizeof(*header) + ReadResourceOffset(header->data.offset);
		gGlobalScriptResources.mCollision = buffer + sizeof(*header) + ReadResourceOffset(header->collision.offset);
		gGlobalScriptResources.mScripts   = buffer + sizeof(*header) + ReadResourceOffset(header->scripts.offset);
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x8009327C
 * @note Size: 0x1C
 */
extern "C" BOOL IsGlobalDataLoaded()
{
	return gGlobalScriptResources.mFile.mBuffer != NULL;
}

/**
 * @note Address: 0x80093298
 * @note Size: 0x6C
 */
extern "C" u32 GetPlayerActionParameter(s32 index)
{
	return ReadResourceOffset(ResourceField(GLOBAL_PLAYER_ACTION_PARAMS, index, 4, 0));
}

/**
 * @note Address: 0x80093304
 * @note Size: 0x6C
 */
extern "C" u32 GetPopperParameter(s32 index)
{
	return ReadResourceOffset(ResourceField(GLOBAL_POPPER_PARAMS, index, 4, 0));
}

/**
 * @note Address: 0x80093370
 * @note Size: 0x6C
 */
extern "C" u32 GetCopterParameter(s32 index)
{
	return ReadResourceOffset(ResourceField(GLOBAL_COPTER_PARAMS, index, 4, 0));
}

/**
 * @note Address: 0x800933DC
 * @note Size: 0x6C
 */
extern "C" u32 GetRadarParameter(s32 index)
{
	return ReadResourceOffset(ResourceField(GLOBAL_RADAR_PARAMS, index, 4, 0));
}

/**
 * @note Address: 0x80093448
 * @note Size: 0x6C
 */
extern "C" u32 GetStageEntryTimeComponent(s32 index)
{
	return ReadResourceOffset(ResourceField(GLOBAL_STAGE_ENTRY_TIMES, index, 4, 0));
}

/**
 * @note Address: 0x800934B4
 * @note Size: 0x6C
 */
extern "C" u32 GetInteractionParameter(s32 index)
{
	return ReadResourceOffset(ResourceField(GLOBAL_INTERACTION_PARAMS, index, 4, 0));
}

/**
 * @note Address: 0x80093520
 * @note Size: 0x6C
 */
extern "C" u32 GetStartingAttachmentLevel(s32 index)
{
	return ReadResourceOffset(ResourceField(GLOBAL_ATTACHMENT_LEVELS, index, 4, 0));
}

/**
 * @note Address: 0x8009358C
 * @note Size: 0x6C
 */
extern "C" u32 GetPickupRewardValue(s32 index)
{
	return ReadResourceOffset(ResourceField(GLOBAL_PICKUP_REWARDS, index, 4, 0));
}

/**
 * @note Address: 0x800935F8
 * @note Size: 0x6C
 */
extern "C" u32 GetMaterialSoundId(s32 index)
{
	return ReadResourceOffset(ResourceField(GLOBAL_MATERIAL_SOUNDS, index, 4, 0));
}

/**
 * @note Address: 0x80093664
 * @note Size: 0x6C
 */
extern "C" u32 GetItemReleaseSoundId(s32 index)
{
	return ReadResourceOffset(ResourceField(GLOBAL_ITEM_RELEASE_SOUNDS, index, 4, 0));
}

/**
 * @note Address: 0x800936D0
 * @note Size: 0x50
 */
extern "C" s32 GetDefaultStageIndex()
{
	if (gGame.GetStatusFlags(0x8000000000000ULL))
		return 0;
	return 14;
}

// Object IDs from 10000 share collision lists with base objects.
/**
 * @note Address: 0x80093720
 * @note Size: 0x84
 */
extern "C" XObjFloor* GetObjectCollisionList(s32 objectId)
{
	if (objectId >= 10000) {
		// the Wii build subtracts first and indexes the table; the GameCube ones fold the 10000 into the table's address
		s32 index = objectId - 10000;
		objectId  = gObjectCollisionIdRemap[index];
		if (objectId < 0)
			return NULL;
	}
	u8* collision = gGlobalScriptResources.mCollision;
	if (*(u32*)(collision + objectId * 4) != 0) {
		const u8* entry = collision + objectId * 4;
		return reinterpret_cast<XObjFloor*>(collision + ReadResourceOffset(entry));
	}
	return NULL;
}

/**
 * @note Address: 0x800937A4
 * @note Size: 0x10
 */
extern "C" u8* GetGlobalScriptTable(s32)
{
	return gGlobalScriptResources.mScripts;
}

/**
 * @note Address: 0x800937B4
 * @note Size: 0x50
 */
extern "C" u8* GetGlobalScriptByIndex(s32 index)
{
	return ReadScriptPointer(gGlobalScriptResources.mScripts, gGlobalScriptResources.mScripts + index * 4);
}

/**
 * @note Address: 0x80093804
 * @note Size: 0x64
 */
extern "C" BOOL IsGlobalEventScript(u8* script)
{
	return IsGlobalScript(script);
}

// script offsets count from the buffer the operand is in, the globals file or the stage's, 0 is none
/**
 * @note Address: 0x80093868
 * @note Size: 0xE0
 */
extern "C" u8* ResolveEventScriptOffset(u8* operand)
{
	// a globals operand resolves against the globals' script blob, a stage one against the stage data
	s32 isGlobal = IsGlobalScript(operand);
	if (isGlobal != 0) {
		u32 offset = ReadResourceOffset(operand);
		return offset == 0 ? NULL : gGlobalScriptResources.mScripts + offset;
	}
	u32 offset = ReadResourceOffset(operand);
	return offset == 0 ? NULL : (u8*)gStageResources.mDataFile.mBuffer + offset;
}

/**
 * @note Address: 0x80093948
 * @note Size: 0x70
 */
extern "C" u8* GetItemName(s32 index)
{
	return ResourcePointer(GLOBAL_ITEMS, index, 20, 0);
}

/**
 * @note Address: 0x800939B8
 * @note Size: 0x70
 */
extern "C" u8* GetItemDescription(s32 index)
{
	return ResourcePointer(GLOBAL_ITEMS, index, 20, 4);
}

/**
 * @note Address: 0x80093A28
 * @note Size: 0x4C
 */
extern "C" u16 GetItemFlagsMasked(s32 index, s32 mask)
{
	return *(u16*)(ResourceField(GLOBAL_ITEMS, index, 20, 8)) & mask;
}

/**
 * @note Address: 0x80093A74
 * @note Size: 0x50
 */
extern "C" void SetItemFlags(s32 index, s32 mask)
{
	u16* flags = (u16*)(ResourceField(GLOBAL_ITEMS, index, 20, 8));
	*flags     = *flags | mask;
}

/**
 * @note Address: 0x80093AC4
 * @note Size: 0x50
 */
extern "C" void ClearItemFlags(s32 index, s32 mask)
{
	u16* flags = (u16*)(ResourceField(GLOBAL_ITEMS, index, 20, 8));
	*flags     = *flags & ~mask;
}

/**
 * @note Address: 0x80093B14
 * @note Size: 0x48
 */
extern "C" s16 GetItemPickupHoldTime(s32 index)
{
	return *(s16*)(ResourceField(GLOBAL_ITEMS, index, 20, 10));
}

/**
 * @note Address: 0x80093B5C
 * @note Size: 0x48
 */
extern "C" s16 GetItemShopPrice(s32 index)
{
	return *(s16*)(ResourceField(GLOBAL_ITEMS, index, 20, 12));
}

/**
 * @note Address: 0x80093BA4
 * @note Size: 0x48
 */
extern "C" s16 GetItemScrapValue(s32 index)
{
	return *(s16*)(ResourceField(GLOBAL_ITEMS, index, 20, 14));
}

/**
 * @note Address: 0x80093BEC
 * @note Size: 0x48
 */
extern "C" s16 GetItemHappyPointValue(s32 index)
{
	return *(s16*)(ResourceField(GLOBAL_ITEMS, index, 20, 16));
}

/**
 * @note Address: 0x80093C34
 * @note Size: 0x4C
 */
extern "C" u8 GetItemSoundIndex(s32 index, s32 member)
{
	return ResourceField(GLOBAL_ITEMS, index, 20, 18)[member];
}

/**
 * @note Address: 0x80093C80
 * @note Size: 0x70
 */
extern "C" u8* GetActorName(s32 index)
{
	return ResourcePointer(GLOBAL_ACTOR_NAMES, index, 4, 0);
}

/**
 * @note Address: 0x80093CF0
 * @note Size: 0x70
 */
extern "C" u8* GetAttachmentName(s32 index)
{
	return ResourcePointer(GLOBAL_ATTACHMENTS, index, 12, 0);
}

/**
 * @note Address: 0x80093D60
 * @note Size: 0x70
 */
extern "C" u8* GetAttachmentDescription(s32 index)
{
	return ResourcePointer(GLOBAL_ATTACHMENTS, index, 12, 4);
}

/**
 * @note Address: 0x80093DD0
 * @note Size: 0x48
 */
extern "C" s16 GetAttachmentShopPrice(s32 index)
{
	return *(s16*)(ResourceField(GLOBAL_ATTACHMENTS, index, 12, 8));
}

/**
 * @note Address: 0x80093E18
 * @note Size: 0x70
 */
extern "C" u8* GetCostumeName(s32 index)
{
	return ResourcePointer(GLOBAL_COSTUME_NAMES, index, 4, 0);
}

/**
 * @note Address: 0x80093E88
 * @note Size: 0x70
 */
extern "C" u8* GetStageDisplayName(s32 index)
{
	return ResourcePointer(GLOBAL_STAGE_NAMES, index, 8, 0);
}

/**
 * @note Address: 0x80093EF8
 * @note Size: 0x70
 */
extern "C" u8* GetPCRankingName(s32 index)
{
	return ResourcePointer(GLOBAL_PC_RANKING_NAMES, index, 4, 0);
}

/**
 * @note Address: 0x80093F68
 * @note Size: 0x70
 */
extern "C" u8* GetUtilibotName(s32 index)
{
	return ResourcePointer(GLOBAL_UTILIBOTS, index, 12, 0);
}

/**
 * @note Address: 0x80093FD8
 * @note Size: 0x70
 */
extern "C" u8* GetUtilibotDescription(s32 index)
{
	return ResourcePointer(GLOBAL_UTILIBOTS, index, 12, 4);
}

/**
 * @note Address: 0x80094048
 * @note Size: 0x48
 */
extern "C" s16 GetUtilibotPrice(s32 index)
{
	return *(s16*)(ResourceField(GLOBAL_UTILIBOTS, index, 12, 8));
}

/**
 * @note Address: 0x80094090
 * @note Size: 0x70
 */
extern "C" u8* GetStatusIconName(s32 index)
{
	return ResourcePointer(GLOBAL_STATUS_ICONS, index, 12, 0);
}

/**
 * @note Address: 0x80094100
 * @note Size: 0x70
 */
extern "C" u8* GetStatusIconDescription(s32 index)
{
	return ResourcePointer(GLOBAL_STATUS_ICONS, index, 12, 4);
}

/**
 * @note Address: 0x80094170
 * @note Size: 0x48
 */
extern "C" u32 GetStatusIconEventFlagId(s32 index)
{
	return *(u32*)(ResourceField(GLOBAL_STATUS_ICONS, index, 12, 8));
}

/**
 * @note Address: 0x800941B8
 * @note Size: 0x70
 */
extern "C" u8* GetStatusOverviewLabel(s32 index)
{
	return ResourcePointer(GLOBAL_STATUS_OVERVIEW, index, 8, 0);
}

/**
 * @note Address: 0x80094228
 * @note Size: 0x70
 */
extern "C" u8* GetStatusOverviewDescription(s32 index)
{
	return ResourcePointer(GLOBAL_STATUS_OVERVIEW, index, 8, 4);
}

/**
 * @note Address: 0x80094298
 * @note Size: 0x28
 */
extern "C" const char* GetLocalizedInterfaceText(s32 index)
{
	return gInterfaceTextByLanguage[gGame.mLanguage][index];
}

/**
 * @note Address: 0x800942C0
 * @note Size: 0x28
 */
extern "C" const char* GetLocalizedPCDialogue(s32 index)
{
	return gPCDialogueByLanguage[gGame.mLanguage][index];
}

// NONMATCHING: retail emits Globals' weak members first (__RTTI__7Globals at .sdata +8, __vt__7Globals at
// .data 0x153C8, __dt__7GlobalsFv first in the dtor run) and takes CoordInfo's dtor and vtable from outside;
// ours emits Globals last and CoordInfo's here, so .sdata and extab differ. The data itself is retail's.
Stage gStageResources;
Globals gGlobalScriptResources;
