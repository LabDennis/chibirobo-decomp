// Based on projectPiki/pik1wii. This release retains the compressor table and frame count in AX state.
#include "revolution/ax.h"
#include "revolution/ax_studio.h"
#include "Dolphin/OS.h"

#define AX_MAX_VOLUME  32768
#define LIST_MAX       2
#define THRESHOLD      32768
#define RELEASE_FRAMES 10

extern u16 __AXCompressorTable[];

/**
 * Command list opcodes (from Dolphin Emulator)
 */
typedef enum {
	COMMAND_SETUP,
	COMMAND_ADD_TO_LR,
	COMMAND_SUB_TO_LR,
	COMMAND_ADD_SUB_TO_LR,
	COMMAND_PROCESS,
	COMMAND_MIX_AUXA,
	COMMAND_MIX_AUXB,
	COMMAND_MIX_AUXC,
	COMMAND_UPL_AUXA_MIX_LRSC,
	COMMAND_UPL_AUXB_MIX_LRSC,
	COMMAND_COMPRESSOR,
	COMMAND_OUTPUT,
	COMMAND_OUTPUT_DPL2,
	COMMAND_WM_OUTPUT,
	COMMAND_END
} AXCommand;

static u32 __AXCommandListPosition;

static u16* __AXClWrite;
u32 __AXClMode;

static u32 __AXCommandListCycles;
static u8 __AXCommandList[AX_CL_SIZE * LIST_MAX];

static BOOL __AXCompressor;
static u16* compressorTable;
static u16 compressorReleaseFrames;

static volatile u16 __AXMasterVolume;
static u16 __AXAuxAVolume;
static u16 __AXAuxBVolume;
static u16 __AXAuxCVolume;

u32 __AXGetCommandListCycles(void)
{
	return __AXCommandListCycles;
}
void* __AXGetCommandListAddress(void)
{
	void* list = __AXCommandList + __AXCommandListPosition * AX_CL_SIZE;

	__AXCommandListPosition = (__AXCommandListPosition + 1) % LIST_MAX;
	__AXClWrite             = (u16*)(__AXCommandList + __AXCommandListPosition * AX_CL_SIZE);

	return list;
}

static void WriteCommand(u16 cmd)
{
	*__AXClWrite = cmd;
	__AXClWrite++;
}

#define WriteAddress(address)          \
	do {                               \
		WriteCommand((address) >> 16); \
		WriteCommand(address);         \
	} while (0)

void __AXNextFrame(void* surround, void* lr, void* rmt)
{
	void* imm;
	void* backup;

	__AXCommandListCycles = 7811;
	backup                = __AXClWrite;
	imm                   = __AXGetStudio();

	WriteCommand(COMMAND_SETUP);
	WriteAddress((u32)imm);
	__AXCommandListCycles += 4126;

	switch (__AXClMode) {
	case AX_OUTPUT_STEREO:
		WriteCommand(COMMAND_ADD_TO_LR);
		WriteAddress((u32)surround);
		__AXCommandListCycles += 733;
		break;
	case AX_OUTPUT_SURROUND:
		WriteCommand(COMMAND_SUB_TO_LR);
		WriteAddress((u32)surround);
		__AXCommandListCycles += 829;
		break;
	case AX_OUTPUT_DPL2:
		WriteCommand(COMMAND_ADD_SUB_TO_LR);
		WriteAddress((u32)surround);
		__AXCommandListCycles += 925;
		break;
	}

	imm = __AXGetPBs();
	WriteCommand(COMMAND_PROCESS);
	WriteAddress((u32)imm);

	if (__AXClMode == AX_OUTPUT_DPL2) {
		__AXGetAuxAInput(&imm);
		if (imm != NULL) {
			WriteCommand(COMMAND_UPL_AUXA_MIX_LRSC);
			WriteCommand(__AXAuxAVolume);
			WriteAddress((u32)imm);

			__AXGetAuxAInputDpl2(&imm);
			WriteAddress((u32)imm);

			__AXGetAuxAOutput(&imm);
			WriteAddress((u32)imm);

			__AXGetAuxAOutputDpl2R(&imm);
			WriteAddress((u32)imm);

			__AXGetAuxAOutputDpl2Ls(&imm);
			WriteAddress((u32)imm);

			__AXGetAuxAOutputDpl2Rs(&imm);
			WriteAddress((u32)imm);

			__AXCommandListCycles += 3036;
		}

		__AXGetAuxBInput(&imm);
		if (imm != NULL) {
			WriteCommand(COMMAND_UPL_AUXB_MIX_LRSC);
			WriteCommand(__AXAuxBVolume);
			WriteAddress((u32)imm);

			__AXGetAuxBInputDpl2(&imm);
			WriteAddress((u32)imm);

			__AXGetAuxBOutput(&imm);
			WriteAddress((u32)imm);

			__AXGetAuxBOutputDpl2R(&imm);
			WriteAddress((u32)imm);

			__AXGetAuxBOutputDpl2Ls(&imm);
			WriteAddress((u32)imm);

			__AXGetAuxBOutputDpl2Rs(&imm);
			WriteAddress((u32)imm);

			__AXCommandListCycles += 3036;
		}
	} else {
		__AXGetAuxAInput(&imm);
		if (imm != NULL) {
			WriteCommand(COMMAND_MIX_AUXA);
			WriteCommand(__AXAuxAVolume);
			WriteAddress((u32)imm);

			__AXGetAuxAOutput(&imm);
			WriteAddress((u32)imm);

			__AXCommandListCycles += 2235;
		}

		__AXGetAuxBInput(&imm);
		if (imm != NULL) {
			WriteCommand(COMMAND_MIX_AUXB);
			WriteCommand(__AXAuxBVolume);
			WriteAddress((u32)imm);

			__AXGetAuxBOutput(&imm);
			WriteAddress((u32)imm);

			__AXCommandListCycles += 2235;
		}

		__AXGetAuxCInput(&imm);
		if (imm != NULL) {
			WriteCommand(COMMAND_MIX_AUXC);
			WriteCommand(__AXAuxCVolume);
			WriteAddress((u32)imm);

			__AXGetAuxCOutput(&imm);
			WriteAddress((u32)imm);

			__AXCommandListCycles += 2235;
		}
	}

	if (__AXCompressor) {
		WriteCommand(COMMAND_COMPRESSOR);
		WriteCommand(THRESHOLD);
		WriteCommand(compressorReleaseFrames);
		WriteAddress((u32)compressorTable);
		__AXCommandListCycles += 1850;
	}

	WriteCommand(COMMAND_WM_OUTPUT);
	WriteAddress(((u32*)rmt)[0]);
	WriteAddress(((u32*)rmt)[1]);
	WriteAddress(((u32*)rmt)[2]);
	WriteAddress(((u32*)rmt)[3]);
	__AXCommandListCycles += 409;

	if (__AXClMode == AX_OUTPUT_DPL2) {
		WriteCommand(COMMAND_OUTPUT_DPL2);
		WriteCommand(__AXMasterVolume);
		WriteAddress((u32)surround);
		WriteAddress((u32)lr);
		__AXCommandListCycles += 1195;
	} else {
		WriteCommand(COMMAND_OUTPUT);
		WriteCommand(__AXMasterVolume);
		WriteAddress((u32)surround);
		WriteAddress((u32)lr);
		__AXCommandListCycles += 1172;
	}

	WriteCommand(COMMAND_END);
	__AXCommandListCycles += 30;

	DCFlushRange(backup, AX_CL_SIZE);
}

void __AXClInit(void)
{
	__AXClMode              = AX_OUTPUT_STEREO;
	__AXCommandListPosition = 0;
	__AXClWrite             = (u16*)__AXCommandList;
	__AXCompressor          = TRUE;
	compressorTable         = __AXCompressorTable;
	compressorReleaseFrames = RELEASE_FRAMES;

	__AXAuxCVolume = __AXAuxBVolume = __AXAuxAVolume = __AXMasterVolume = AX_MAX_VOLUME;
}

void __AXClQuit(void)
{
}

void AXSetMode(u32 mode)
{
	__AXClMode = mode;
}

u32 AXGetMode(void)
{
	return __AXClMode;
}

u16 AXGetAuxAReturnVolume(void)
{
	return __AXAuxAVolume;
}

u16 AXGetAuxBReturnVolume(void)
{
	return __AXAuxBVolume;
}

u16 AXGetAuxCReturnVolume(void)
{
	return __AXAuxCVolume;
}

void AXSetMasterVolume(u16 volume)
{
	if (volume > AX_MAX_VOLUME) {
		volume = AX_MAX_VOLUME;
	}

	__AXMasterVolume = volume;
}

void AXSetAuxAReturnVolume(u16 volume)
{
	__AXAuxAVolume = volume;
}

void AXSetAuxBReturnVolume(u16 volume)
{
	__AXAuxBVolume = volume;
}

void AXSetAuxCReturnVolume(u16 volume)
{
	__AXAuxCVolume = volume;
}
