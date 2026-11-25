#include <dolphin/dolphin.h>
#include <dolphin/ax.h>

#include "__ax.h"

#define AX_COMMAND_LIST_COUNT         2
#define AX_AUX_MIX_CYCLES             0xDED
#define AX_COMPRESSOR_THRESHOLD       0x8000
#define AX_COMPRESSOR_RELEASE_STAGES  10

enum {
    AX_CMD_SETUP_STUDIO      = 0x00,
    AX_CMD_SET_VOICE_LIST    = 0x02,
    AX_CMD_PROCESS_VOICES    = 0x03,
    AX_CMD_MIX_AUX_A         = 0x04,
    AX_CMD_MIX_AUX_B         = 0x05,
    AX_CMD_LOAD_MAIN_LR      = 0x07,
    AX_CMD_OUTPUT            = 0x0E,
    AX_CMD_END               = 0x0F,
    AX_CMD_MIX_AUX_B_LR      = 0x10,
    AX_CMD_LOAD_SWAPPED_LR   = 0x11,
    AX_CMD_COMPRESS          = 0x12,
    AX_CMD_SEND_AUX_AND_MIX  = 0x13,
};

static u16 __AXCommandList[AX_COMMAND_LIST_COUNT][AX_COMMAND_LIST_WORDS];

static u32 __AXCommandListPosition;
static u16* __AXClWrite;
static u32 __AXCommandListCycles;
static u32 __AXCompressor;
u32 __AXClMode;

/**
 * @note Address: 0x80177A04
 * @note Size: 0x8
 */
u32 __AXGetCommandListCycles(void) {
    return __AXCommandListCycles;
}

/**
 * @note Address: 0x80177A0C
 * @note Size: 0x3C
 */
u32 __AXGetCommandListAddress(void) {
    u32 address;

    address = (u32)&__AXCommandList[__AXCommandListPosition][0];
    __AXCommandListPosition += 1;
    __AXCommandListPosition %= AX_COMMAND_LIST_COUNT;
    __AXClWrite = __AXCommandList[__AXCommandListPosition];
    return address;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void __AXWriteToCommandList(u16 data) {
    *__AXClWrite = data;
    __AXClWrite++;
}

/*
 * DSP addresses are written as two halfwords, high half first.
 * Pass a value without side effects because the address is read twice.
 */
#define WRITE_COMMAND_ADDRESS(address)                       \
    do {                                                     \
        __AXWriteToCommandList((u16)((u32)(address) >> 16)); \
        __AXWriteToCommandList((u16)(u32)(address));         \
    } while (0)

/**
 * @note Address: 0x80177A48
 * @note Size: 0x6B0
 */
void __AXNextFrame(s32* surroundBuffer, s16* outputBuffer) {
    u32 address;
    u16* commandListStart;

    __AXCommandListCycles = 0x1A9;
    commandListStart = __AXClWrite;
    address = __AXGetStudio();
    __AXWriteToCommandList(AX_CMD_SETUP_STUDIO);
    WRITE_COMMAND_ADDRESS(address);
    __AXCommandListCycles += 0x2E44;

    switch (__AXClMode) {
    case 0:
        __AXWriteToCommandList(AX_CMD_LOAD_MAIN_LR);
        WRITE_COMMAND_ADDRESS(surroundBuffer);
        __AXCommandListCycles += 0x546;
        break;
    case 1:
        __AXWriteToCommandList(AX_CMD_LOAD_SWAPPED_LR);
        WRITE_COMMAND_ADDRESS(surroundBuffer);
        __AXCommandListCycles += 0x5E6;
        break;
    case AX_CL_MODE_DPL2:
        break;
    default:
        ASSERTMSGLINE(193, 0, "Unknown AX mode!");
    }

    address = (u32)__AXGetPBs();
    __AXWriteToCommandList(AX_CMD_SET_VOICE_LIST);
    WRITE_COMMAND_ADDRESS(address);
    __AXWriteToCommandList(AX_CMD_PROCESS_VOICES);

    if (__AXClMode == AX_CL_MODE_DPL2) {
        __AXGetAuxAInput(&address);
        if (address != 0) {
            __AXWriteToCommandList(AX_CMD_SEND_AUX_AND_MIX);
            WRITE_COMMAND_ADDRESS(address);
            __AXGetAuxAInputDpl2(&address);
            WRITE_COMMAND_ADDRESS(address);
            __AXGetAuxAOutput(&address);
            WRITE_COMMAND_ADDRESS(address);
            __AXGetAuxAOutputDpl2R(&address);
            WRITE_COMMAND_ADDRESS(address);
            __AXGetAuxAOutputDpl2Ls(&address);
            WRITE_COMMAND_ADDRESS(address);
            __AXGetAuxAOutputDpl2Rs(&address);
            WRITE_COMMAND_ADDRESS(address);
            __AXCommandListCycles += AX_AUX_MIX_CYCLES;
        }
        __AXWriteToCommandList(AX_CMD_MIX_AUX_B_LR);
        __AXGetAuxBForDPL2(&address);
        WRITE_COMMAND_ADDRESS(address);
        __AXGetAuxBOutputDPL2(&address);
        WRITE_COMMAND_ADDRESS(address);
        __AXCommandListCycles += AX_AUX_MIX_CYCLES;
    } else {
        __AXGetAuxAInput(&address);

        if (address != 0) {
            __AXWriteToCommandList(AX_CMD_MIX_AUX_A);
            WRITE_COMMAND_ADDRESS(address);
            __AXGetAuxAOutput(&address);
            WRITE_COMMAND_ADDRESS(address);
            __AXCommandListCycles += AX_AUX_MIX_CYCLES;
        }

        __AXGetAuxBInput(&address);
        if (address != 0) {
            __AXWriteToCommandList(AX_CMD_MIX_AUX_B);
            __AXCommandListCycles += AX_AUX_MIX_CYCLES;
            WRITE_COMMAND_ADDRESS(address);
            __AXGetAuxBOutput(&address);
            WRITE_COMMAND_ADDRESS(address);
        }
    }

    if (__AXCompressor) {
        __AXWriteToCommandList(AX_CMD_COMPRESS);
        __AXWriteToCommandList(AX_COMPRESSOR_THRESHOLD);
        __AXWriteToCommandList(AX_COMPRESSOR_RELEASE_STAGES);
        WRITE_COMMAND_ADDRESS(__AXCompressorTable);
        __AXCommandListCycles += 0xBB8;
    }

    __AXWriteToCommandList(AX_CMD_OUTPUT);
    WRITE_COMMAND_ADDRESS(surroundBuffer);
    WRITE_COMMAND_ADDRESS(outputBuffer);
    __AXCommandListCycles += 0x2710;
    __AXWriteToCommandList(AX_CMD_END);
    __AXCommandListCycles += 2;
    DCFlushRange(commandListStart, sizeof(__AXCommandList[0]));
}

#undef WRITE_COMMAND_ADDRESS

/**
 * @note Address: 0x801780F8
 * @note Size: 0x24
 */
void __AXClInit(void) {
#if DEBUG
    OSReport("Initializing AXCL code module\n");
#endif
    ASSERTLINE(338, ((u32)&__AXCommandList[0][0] & 0x1F) == 0);
    ASSERTLINE(339, ((u32)&__AXCommandList[1][0] & 0x1F) == 0);
    __AXClMode = 0;
    __AXCommandListPosition = 0;
    __AXClWrite = __AXCommandList[0];
    __AXCompressor = 1;
}

/**
 * @note Address: 0x8017811C
 * @note Size: 0x4
 */
void __AXClQuit(void) {
#if DEBUG
    OSReport("Shutting down AXCL code module\n");
#endif
}

/**
 * @note Address: 0x80178120
 * @note Size: 0x14
 */
void AXSetMode(u32 mode) {
    if (__AXClMode != mode) {
        __AXClMode = mode;
    }
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
u32 AXGetMode(void) {
    return __AXClMode;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void AXSetCompressor(u32 i) {
    __AXCompressor = i;
}
