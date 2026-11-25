#include <dolphin/dolphin.h>
#include <dolphin/ax.h>

#include "__ax.h"

#define AX_AUX_BUFFER_COUNT      3
#define AX_AUX_DPL2_BUFFER_COUNT 2

#define AX_AUX_CHANNEL_COUNT    3
#define AX_AUX_LEFT_OFFSET      0
#define AX_AUX_RIGHT_OFFSET     AX_SAMPLES_PER_FRAME
#define AX_AUX_SURROUND_OFFSET  (2 * AX_SAMPLES_PER_FRAME)

/* Each frame holds consecutive left, right, and surround sample planes. */
typedef s32 AXAuxFrame[AX_AUX_CHANNEL_COUNT * AX_SAMPLES_PER_FRAME];

static AXAuxFrame __AXBufferAuxA[AX_AUX_BUFFER_COUNT] ATTRIBUTE_ALIGN(32);
static AXAuxFrame __AXBufferAuxB[AX_AUX_BUFFER_COUNT] ATTRIBUTE_ALIGN(32);

static AXAuxCallback __AXCallbackAuxA;
static AXAuxCallback __AXCallbackAuxB;
static void* __AXContextAuxA;
static void* __AXContextAuxB;
static s32* __AXAuxADspWrite;
static s32* __AXAuxADspRead;
static s32* __AXAuxBDspWrite;
static s32* __AXAuxBDspRead;
static u32 __AXAuxDspWritePosition;
static u32 __AXAuxDspReadPosition;
static u32 __AXAuxDspWritePositionDpl2;
static u32 __AXAuxDspReadPositionDpl2;
static u32 __AXAuxCpuReadWritePosition;

/**
 * @note Address: 0x80177564
 * @note Size: 0xE4
 */
void __AXAuxInit(void) {
    int i;
    s32* pA;
    s32* pB;

#if DEBUG
    OSReport("Initializing AXAux code module\n");
#endif
    __AXCallbackAuxA = NULL;
    __AXCallbackAuxB = NULL;
    __AXContextAuxA = NULL;
    __AXContextAuxB = NULL;
    __AXAuxDspWritePosition = 0;
    __AXAuxDspReadPosition = 1;
    __AXAuxDspWritePositionDpl2 = 0;
    __AXAuxDspReadPositionDpl2 = 1;
    __AXAuxCpuReadWritePosition = 2;

    /* Only the first frame is cleared when the cursors are initialized. */
    pA = __AXBufferAuxA[0];
    pB = __AXBufferAuxB[0];

    for (i = 0; i < (int)(sizeof(AXAuxFrame) / sizeof(s32)); i++) {
        *pA++ = 0;
        *pB++ = 0;
    }
}

/**
 * @note Address: 0x80177648
 * @note Size: 0x10
 */
void __AXAuxQuit(void) {
#if DEBUG
    OSReport("Shutting down AXAux code module\n");
#endif
    __AXCallbackAuxA = NULL;
    __AXCallbackAuxB = NULL;
}

/**
 * @note Address: 0x80177658
 * @note Size: 0x34
 */
void __AXGetAuxAInput(u32* p) {
    if (__AXCallbackAuxA) {
        *p = (u32)&__AXBufferAuxA[__AXAuxDspWritePosition][AX_AUX_LEFT_OFFSET];
    } else {
        *p = 0;
    }
}

/**
 * @note Address: 0x8017768C
 * @note Size: 0x20
 */
void __AXGetAuxAInputDpl2(u32* p) {
    *p = (u32)&__AXBufferAuxB[__AXAuxDspWritePosition][AX_AUX_SURROUND_OFFSET];
}

/**
 * @note Address: 0x801776AC
 * @note Size: 0x1C
 */
void __AXGetAuxAOutput(u32* p) {
    *p = (u32)&__AXBufferAuxA[__AXAuxDspReadPosition][AX_AUX_LEFT_OFFSET];
}

/**
 * @note Address: 0x801776C8
 * @note Size: 0x20
 */
void __AXGetAuxAOutputDpl2R(u32* p) {
    *p = (u32)&__AXBufferAuxA[__AXAuxDspReadPosition][AX_AUX_RIGHT_OFFSET];
}

/**
 * @note Address: 0x801776E8
 * @note Size: 0x20
 */
void __AXGetAuxAOutputDpl2Ls(u32* p) {
    *p = (u32)&__AXBufferAuxA[__AXAuxDspReadPosition][AX_AUX_SURROUND_OFFSET];
}

/**
 * @note Address: 0x80177708
 * @note Size: 0x20
 */
void __AXGetAuxAOutputDpl2Rs(u32* p) {
    *p = (u32)&__AXBufferAuxB[__AXAuxDspReadPosition][AX_AUX_SURROUND_OFFSET];
}

/**
 * @note Address: 0x80177728
 * @note Size: 0x34
 */
void __AXGetAuxBInput(u32* p) {
    if (__AXCallbackAuxB) {
        *p = (u32)&__AXBufferAuxB[__AXAuxDspWritePosition][AX_AUX_LEFT_OFFSET];
    } else {
        *p = 0;
    }
}

/**
 * @note Address: 0x8017775C
 * @note Size: 0x1C
 */
void __AXGetAuxBOutput(u32* p) {
    *p = (u32)&__AXBufferAuxB[__AXAuxDspReadPosition][AX_AUX_LEFT_OFFSET];
}

/**
 * @note Address: 0x80177778
 * @note Size: 0x1C
 */
void __AXGetAuxBForDPL2(u32* p) {
    *p = (u32)&__AXBufferAuxB[__AXAuxDspWritePositionDpl2][AX_AUX_LEFT_OFFSET];
}

/**
 * @note Address: 0x80177794
 * @note Size: 0x1C
 */
void __AXGetAuxBOutputDPL2(u32* p) {
    *p = (u32)&__AXBufferAuxB[__AXAuxDspReadPositionDpl2][AX_AUX_LEFT_OFFSET];
}

/**
 * @note Address: 0x801777B0
 * @note Size: 0x23C
 */
void __AXProcessAux(void) {
    __AXAuxADspWrite = &__AXBufferAuxA[__AXAuxDspWritePosition][AX_AUX_LEFT_OFFSET];
    __AXAuxADspRead = &__AXBufferAuxA[__AXAuxDspReadPosition][AX_AUX_LEFT_OFFSET];
    __AXAuxBDspWrite = &__AXBufferAuxB[__AXAuxDspWritePosition][AX_AUX_LEFT_OFFSET];
    __AXAuxBDspRead = &__AXBufferAuxB[__AXAuxDspReadPosition][AX_AUX_LEFT_OFFSET];

    if (__AXCallbackAuxA) {
        if (__AXClMode == AX_CL_MODE_DPL2) {
            AX_AUX_DATA_DPL2 auxData;
            auxData.channels.left = &__AXBufferAuxA[__AXAuxCpuReadWritePosition][AX_AUX_LEFT_OFFSET];
            auxData.channels.right = &__AXBufferAuxA[__AXAuxCpuReadWritePosition][AX_AUX_RIGHT_OFFSET];
            auxData.channels.surround = &__AXBufferAuxA[__AXAuxCpuReadWritePosition][AX_AUX_SURROUND_OFFSET];
            auxData.rightSurround = &__AXBufferAuxB[__AXAuxCpuReadWritePosition][AX_AUX_SURROUND_OFFSET];
            DCInvalidateRange(auxData.channels.left, sizeof(AXAuxFrame));
            DCInvalidateRange(auxData.rightSurround, AX_SAMPLES_PER_FRAME * sizeof(*auxData.rightSurround));
            __AXCallbackAuxA(&auxData, __AXContextAuxA);
            DCFlushRangeNoSync(auxData.channels.left, sizeof(AXAuxFrame));
            DCFlushRangeNoSync(auxData.rightSurround, AX_SAMPLES_PER_FRAME * sizeof(*auxData.rightSurround));
        } else {
            AX_AUX_DATA auxData;
            auxData.left = &__AXBufferAuxA[__AXAuxCpuReadWritePosition][AX_AUX_LEFT_OFFSET];
            auxData.right = &__AXBufferAuxA[__AXAuxCpuReadWritePosition][AX_AUX_RIGHT_OFFSET];
            auxData.surround = &__AXBufferAuxA[__AXAuxCpuReadWritePosition][AX_AUX_SURROUND_OFFSET];
            DCInvalidateRange(auxData.left, sizeof(AXAuxFrame));
            __AXCallbackAuxA(&auxData, __AXContextAuxA);
            DCFlushRangeNoSync(auxData.left, sizeof(AXAuxFrame));
        }
    }

    if (__AXCallbackAuxB && __AXClMode != AX_CL_MODE_DPL2) {
        AX_AUX_DATA auxData;
        auxData.left = &__AXBufferAuxB[__AXAuxCpuReadWritePosition][AX_AUX_LEFT_OFFSET];
        auxData.right = &__AXBufferAuxB[__AXAuxCpuReadWritePosition][AX_AUX_RIGHT_OFFSET];
        auxData.surround = &__AXBufferAuxB[__AXAuxCpuReadWritePosition][AX_AUX_SURROUND_OFFSET];
        DCInvalidateRange(auxData.left, sizeof(AXAuxFrame));
        __AXCallbackAuxB(&auxData, __AXContextAuxB);
        DCFlushRangeNoSync(auxData.left, sizeof(AXAuxFrame));
    }

    __AXAuxDspWritePosition += 1;
    __AXAuxDspWritePosition %= AX_AUX_BUFFER_COUNT;
    __AXAuxDspReadPosition += 1;
    __AXAuxDspReadPosition %= AX_AUX_BUFFER_COUNT;

    __AXAuxDspWritePositionDpl2 += 1;
    __AXAuxDspWritePositionDpl2 %= AX_AUX_DPL2_BUFFER_COUNT;
    __AXAuxDspReadPositionDpl2 += 1;
    __AXAuxDspReadPositionDpl2 %= AX_AUX_DPL2_BUFFER_COUNT;

    __AXAuxCpuReadWritePosition += 1;
    __AXAuxCpuReadWritePosition %= AX_AUX_BUFFER_COUNT;
}

/**
 * @note Address: 0x801779EC
 * @note Size: 0xC
 */
void AXRegisterAuxACallback(AXAuxCallback callback, void* context) {
    __AXCallbackAuxA = callback;
    __AXContextAuxA = context;
}

/**
 * @note Address: 0x801779F8
 * @note Size: 0xC
 */
void AXRegisterAuxBCallback(AXAuxCallback callback, void* context) {
    __AXCallbackAuxB = callback;
    __AXContextAuxB = context;
}
