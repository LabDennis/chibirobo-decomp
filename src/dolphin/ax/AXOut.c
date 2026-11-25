#include <dolphin/dolphin.h>
#include <dolphin/ax.h>

#include "__ax.h"
#include "__dsp.h"

#define AX_DSP_MAIL_COMMAND_LIST 0xBABE0000U

#define AX_OUTPUT_CHANNEL_COUNT       2
#define AX_OUTPUT_BUFFER_COUNT        3
#define AX_OUTPUT_DOUBLE_BUFFER_COUNT 2
#define AX_OUTPUT_MODE_TRIPLE_BUFFER  1

static s16 __AXOutBuffer[AX_OUTPUT_BUFFER_COUNT][AX_OUTPUT_CHANNEL_COUNT * AX_SAMPLES_PER_FRAME];
static s32 __AXOutSBuffer[AX_SAMPLES_PER_FRAME];
static u16 __AXDramImage[8192];
static DSPTaskInfo __AXDSPTask;
AXPROFILE __AXLocalProfile;

volatile static u32 __AXOutFrame;
volatile static u32 __AXAiDmaFrame;
volatile static u32 __AXOutDspReady;
volatile static OSTime __AXOsTime;
static AXCallback __AXUserFrameCallback;
volatile static int __AXDSPInitFlag;
static int __AXDSPDoneFlag;

static volatile u32 __AXDebugSteppingMode;
static OSThreadQueue __AXOutThreadQueue;
static u32 __AXOutputBufferMode;

// prototypes
static void __AXDSPInitCallback(void* task);
static void __AXDSPResumeCallback(void* task);
static void __AXDSPDoneCallback(void* task);

/**
 * @note Address: 0x80178134
 * @note Size: 0x1B8
 */
void __AXOutNewFrame(u32 lessDspCycles) {
    u32 commandListAddress;
    AXPROFILE* profile;
    const u8* src;
    u8* dest;
    u32 i;

    __AXLocalProfile.axFrameStart = OSGetTime();
    __AXSyncPBs(lessDspCycles);
    __AXPrintStudio();
    commandListAddress = __AXGetCommandListAddress();

    __DSPSendMailAndWait(AX_DSP_MAIL_COMMAND_LIST | AX_COMMAND_LIST_WORDS);
    __DSPSendMailAndWait(commandListAddress);

    __AXServiceCallbackStack();
    __AXLocalProfile.auxProcessingStart = OSGetTime();
    __AXProcessAux();
    __AXLocalProfile.auxProcessingEnd = OSGetTime();
    __AXLocalProfile.userCallbackStart = OSGetTime();

    if (__AXUserFrameCallback) {
        __AXUserFrameCallback();
    }

    __AXLocalProfile.userCallbackEnd = OSGetTime();
    __AXNextFrame(__AXOutSBuffer, &__AXOutBuffer[__AXOutFrame][0]);
    __AXOutFrame += 1;

    if (__AXOutputBufferMode == AX_OUTPUT_MODE_TRIPLE_BUFFER) {
        __AXOutFrame %= AX_OUTPUT_BUFFER_COUNT;
    } else {
        __AXOutFrame %= AX_OUTPUT_DOUBLE_BUFFER_COUNT;
        AIInitDMA((u32)&__AXOutBuffer[__AXOutFrame][0], sizeof(__AXOutBuffer[0]));
    }

    __AXLocalProfile.axFrameEnd = OSGetTime();
    __AXLocalProfile.axNumVoices = __AXGetNumVoices();
    profile = __AXGetCurrentProfile();

    if (profile) {
        i = sizeof(__AXLocalProfile);
        dest = (u8*)profile;
        src = (const u8*)&__AXLocalProfile;

        while (i != 0) {
            *dest++ = *src++;
            i--;
        }
    }
}

/**
 * @note Address: 0x801782EC
 * @note Size: 0xBC
 */
void __AXOutAiCallback(void) {
    if (__AXOutDspReady == 0) {
        __AXOsTime = OSGetTime();
    }

    if (__AXOutDspReady == 1) {
        __AXOutDspReady = 0;
        __AXOutNewFrame(0);
    } else {
        __AXOutDspReady = 2;
        DSPAssertTask(&__AXDSPTask);
    }

    if (__AXOutputBufferMode == AX_OUTPUT_MODE_TRIPLE_BUFFER) {
        AIInitDMA((u32)__AXOutBuffer[__AXAiDmaFrame], sizeof(__AXOutBuffer[0]));
        __AXAiDmaFrame++;
        __AXAiDmaFrame %= AX_OUTPUT_BUFFER_COUNT;
    }
}

/**
 * @note Address: 0x801783A8
 * @note Size: 0xC
 */
static void __AXDSPInitCallback(void* task) {
    __AXDSPInitFlag = 1;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void AXSetStepMode(u32 i) {
    __AXDebugSteppingMode = i;
}

/**
 * @note Address: 0x801783B4
 * @note Size: 0x58
 */
static void __AXDSPResumeCallback(void* task) {
#if DEBUG
    if (__AXDebugSteppingMode != 0) {
        __AXOutDspReady = 1;
        return;
    }
#endif

    if (__AXOutDspReady == 2) {
        __AXOutDspReady = 0;
        __AXOutNewFrame((u32)(OSGetTime() - __AXOsTime) / 4);
        return;
    }
    __AXOutDspReady = 1;
}

/**
 * @note Address: 0x8017840C
 * @note Size: 0x2C
 */
static void __AXDSPDoneCallback(void* task) {
    __AXDSPDoneFlag = 1;
    OSWakeupThread(&__AXOutThreadQueue);
}

#define CLEAR_OUTPUT_BUFFER(buffer)                                    \
    do {                                                               \
        u32* words = (void*)(buffer);                                  \
        int i;                                                         \
        for (i = 0; i < (int)(sizeof(buffer) / sizeof(*words)); i++) { \
            *words = 0;                                                \
            words++;                                                   \
        }                                                              \
    } while (0)

/**
 * @note Address: 0x80178438
 * @note Size: 0xCC
 */
void __AXOutInitDSP(void) {
    __AXDSPTask.iram_mmem_addr = axDspSlave;
    __AXDSPTask.iram_length = axDspSlaveLength;
    __AXDSPTask.iram_addr = 0;
    __AXDSPTask.dram_mmem_addr = __AXDramImage;
    __AXDSPTask.dram_length = 0x2000;
    __AXDSPTask.dram_addr = 0;
    __AXDSPTask.dsp_init_vector = 0x10;
    __AXDSPTask.dsp_resume_vector = 0x30;
    __AXDSPTask.init_cb = __AXDSPInitCallback;
    __AXDSPTask.res_cb = __AXDSPResumeCallback;
    __AXDSPTask.done_cb = __AXDSPDoneCallback;
    __AXDSPTask.req_cb = NULL;
    __AXDSPTask.priority = 0;
    __AXDSPInitFlag = 0;
    __AXDSPDoneFlag = 0;

    OSInitThreadQueue(&__AXOutThreadQueue);
    if (DSPCheckInit() == 0) {
        DSPInit();
    }

    DSPAddTask(&__AXDSPTask);
    do {} while (__AXDSPInitFlag == 0);
}

/**
 * @note Address: 0x80178504
 * @note Size: 0x3A0
 */
void __AXOutInit(u32 outputBufferMode) {
#if DEBUG
    OSReport("Initializing AXOut code module\n");
#endif
    ASSERTLINE(404, ((u32)&__AXOutBuffer[0][0] & 0x1F) == 0);
    ASSERTLINE(405, ((u32)&__AXOutBuffer[1][0] & 0x1F) == 0);
    ASSERTLINE(406, ((u32)&__AXOutBuffer[2][0] & 0x1F) == 0);
    ASSERTLINE(407, ((u32)&__AXOutSBuffer[0] & 0x1F) == 0);

    __AXOutputBufferMode = outputBufferMode;
    __AXOutFrame = 0;
    __AXAiDmaFrame = 0;
    __AXDebugSteppingMode = 0;

    CLEAR_OUTPUT_BUFFER(__AXOutBuffer);
    DCFlushRange(__AXOutBuffer, sizeof(__AXOutBuffer));

    CLEAR_OUTPUT_BUFFER(__AXOutSBuffer);
    DCFlushRange(__AXOutSBuffer, sizeof(__AXOutSBuffer));

    __AXOutInitDSP();
    AIRegisterDMACallback(__AXOutAiCallback);

    if (__AXOutputBufferMode == AX_OUTPUT_MODE_TRIPLE_BUFFER) {
        __AXNextFrame(__AXOutSBuffer, &__AXOutBuffer[AX_OUTPUT_BUFFER_COUNT - 1][0]);
    } else {
        __AXNextFrame(__AXOutSBuffer, &__AXOutBuffer[AX_OUTPUT_DOUBLE_BUFFER_COUNT - 1][0]);
    }

    __AXOutDspReady = 1;
    __AXUserFrameCallback = NULL;

    if (__AXOutputBufferMode == AX_OUTPUT_MODE_TRIPLE_BUFFER) {
        AIInitDMA((u32)&__AXOutBuffer[__AXAiDmaFrame][0], sizeof(__AXOutBuffer[0]));
        __AXAiDmaFrame++;
        __AXAiDmaFrame &= 1;
    } else {
        AIInitDMA((u32)&__AXOutBuffer[__AXOutFrame][0], sizeof(__AXOutBuffer[0]));
    }

    AIStartDMA();
}

#undef CLEAR_OUTPUT_BUFFER

/**
 * @note Address: 0x801788A4
 * @note Size: 0x58
 */
void __AXOutQuit(void) {
    BOOL old;
#if DEBUG
    OSReport("Shutting down AXOut code module\n");
#endif
    old = OSDisableInterrupts();
    __AXUserFrameCallback = NULL;
    DSPCancelTask(&__AXDSPTask);
    OSSleepThread(&__AXOutThreadQueue);
    AIStopDMA();
    OSRestoreInterrupts(old);
}

/**
 * @note Address: 0x801788FC
 * @note Size: 0x44
 */
AXCallback AXRegisterCallback(AXCallback callback) {
    BOOL enabled;
    AXCallback oldCB;

    oldCB = __AXUserFrameCallback;
    enabled = OSDisableInterrupts();
    __AXUserFrameCallback = callback;

    OSRestoreInterrupts(enabled);
    return oldCB;
}
