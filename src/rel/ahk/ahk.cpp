#include "types.h"
#include "game/gmath.h"
#include <stdlib.h>
#include <math.h>

// name from RTTI
class Laser {
public:
	f32 _00[20][2]; // _00
	f32 _A0; // _A0, 6 * sin of the random angle the ctor picks, the x part of a length-6 step; only the ctor writes it, ahk's update and
	         // draw slots are empty
	f32 _A4; // _A4, 6 * cos of the same random angle, the other part of the length-6 step; written by the ctor only, never read
	u32 _A8; // _A8, colour packed as 0xRRGGBB00, three random 0x40-0xFF channels with a zero low byte; written by the ctor only, never read

	Laser();
	virtual ~Laser(); // _08
};
STATIC_ASSERT(sizeof(Laser) == 0xB0);

extern "C" s32 CmdNop(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdPrintArgs(s32 cmd, s32 count, s32* args);
extern "C" void AhkNoOpSlot5(void* arg);
extern "C" void DrawStageHud(void* arg);
extern "C" void UpdateStage(void* arg);

typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 count, s32* args);

extern "C" {
RelSlotFunc _prolog_slots[7] = { 0, UpdateStage, 0, 0, DrawStageHud, AhkNoOpSlot5 };
RelCmdFunc _prolog_cmds[]    = { (RelCmdFunc)CmdPrintArgs, CmdNop };
}

/**
 * @note Address: ahk .text 0x0
 * @note Size: 0x264
 */
Laser::Laser()
{
	int i;
	f32 angle;
	u8 r;
	u8 g;
	u8 b;

	_00[0][0] = rand() % 608 + 16;
	_00[0][1] = rand() % 388 + 30;
	angle     = DEG2RAD * (rand() % 360);
	_A0       = 6.0f * (f32)sin(angle);
	_A4       = 6.0f * (f32)cos(angle);
	r         = (rand() & 0xBF) + 0x40;
	g         = (rand() & 0xBF) + 0x40;
	b         = (rand() & 0xBF) + 0x40;
	_A8       = (r << 24) | (g << 16) | (b << 8);
	for (i = 1; i < 20; i++) {
		_00[i][0] = _00[0][0];
		_00[i][1] = _00[0][1];
	}
}

/**
 * @note Address: ahk .text 0x264
 * @note Size: 0x8
 */
extern "C" s32 CmdNop(s32 cmd, s32 count, s32* args)
{
	return 0;
}

/**
 * @note Address: ahk .text 0x26C
 * @note Size: 0x50
 */
extern "C" s32 CmdPrintArgs(s32 cmd, s32 count, s32* args)
{
	int i;

	// body compiled out, the empty loop is still there
	for (i = 0; i < count; i++) { }
	return 0;
}

/**
 * @note Address: ahk .text 0x2BC
 * @note Size: 0x4
 */
extern "C" void AhkNoOpSlot5(void* arg)
{
}

/**
 * @note Address: ahk .text 0x2C0
 * @note Size: 0x4
 */
extern "C" void DrawStageHud(void* arg)
{
}

/**
 * @note Address: ahk .text 0x2C4
 * @note Size: 0x4
 */
extern "C" void UpdateStage(void* arg)
{
}

/**
 * @note Address: ahk .text 0x2C8
 * @note Size: 0x48
 */
Laser::~Laser()
{
}

/**
 * @note Address: ahk .text 0x310
 * @note Size: 0x58
 */
static Laser lasers[20];

/**
 * @note Address: ahk .text 0x368
 * @note Size: 0x38
 */
// compiler generated array destructor for lasers
