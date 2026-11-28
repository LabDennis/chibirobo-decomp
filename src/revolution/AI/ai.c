#include "Dolphin/ai.h"
#include "revolution/hw_regs.h"
#include "Dolphin/os.h"

char* __AIVersion = "<< RVL_SDK - AI \trelease build: Jul 30 2008 19:18:38 (0x4199_60831) >>";

static AISCallback __AIS_Callback;
static AIDCallback __AID_Callback;
u8* __CallbackStack;
u8* __OldStack;
static vs32 __AI_init_flag = FALSE;
static vs32 __AID_Active   = FALSE;

static OSTime bound_32KHz;
static OSTime bound_48KHz;
static OSTime min_wait;
static OSTime max_wait;
static OSTime buffer;

static void __AI_set_stream_sample_rate(u32 rate);
static void __AISHandler(s16 interrupt, OSContext* context);
static void __AIDHandler(s16 interrupt, OSContext* context);
void __AICallbackStackSwitch(AIDCallback cb);
static void __AI_SRC_INIT(void);

/**
 * @note Address: 0x801746F4
 * @note Size: 0x44
 */
AIDCallback AIRegisterDMACallback(AIDCallback callback)
{
	s32 oldInts;
	AIDCallback ret;

	ret            = __AID_Callback;
	oldInts        = OSDisableInterrupts();
	__AID_Callback = callback;
	OSRestoreInterrupts(oldInts);
	return ret;
}

/**
 * @note Address: 0x80174738
 * @note Size: 0x88
 */
void AIInitDMA(u32 address, u32 length)
{
	s32 previousInterruptState;

	previousInterruptState = OSDisableInterrupts();

	__DSPRegs[DSP_DMA_START_HI]    = (u16)((__DSPRegs[DSP_DMA_START_HI] & ~0x1FFF) | (address >> 16));
	__DSPRegs[DSP_DMA_START_LO]    = (u16)((__DSPRegs[DSP_DMA_START_LO] & ~0xFFE0) | (address & 0xFFFF));
	__DSPRegs[DSP_DMA_CONTROL_LEN] = (u16)((__DSPRegs[DSP_DMA_CONTROL_LEN] & ~0x7FFF) | ((length >> 5) & 0xFFFF));

	OSRestoreInterrupts(previousInterruptState);
}

/**
 * @note Address: N/A
 * @note Size: 0x10
 */
BOOL AIGetDMAEnableFlag(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x801747C0
 * @note Size: 0x18
 */
void AIStartDMA(void)
{
	SET_FLAG(__DSPRegs[DSP_DMA_CONTROL_LEN], DSP_DMA_START_FLAG);
}

/**
 * @note Address: 0x801747D8
 * @note Size: 0x18
 */
void AIStopDMA(void)
{
	RESET_FLAG(__DSPRegs[DSP_DMA_CONTROL_LEN], DSP_DMA_START_FLAG);
}

/**
 * @note Address: N/A
 * @note Size: 0x10
 */
u32 AIGetDMABytesLeft(void)
{
	return (__DSPRegs[DSP_DMA_BYTES_LEFT] & 0x7FFF) << 5;
}

/**
 * @note Address: 0x801747F0
 * @note Size: 0x1C
 */
u32 AIGetDMAStartAddr(void)
{
	const u32 startAddressHigh = (__DSPRegs[DSP_DMA_START_HI] & 0x1FFF) << 16;
	const u32 startAddressLow  = __DSPRegs[DSP_DMA_START_LO] & 0xFFE0;

	return startAddressHigh | startAddressLow;
}

/**
 * @note Address: N/A
 * @note Size: 0x10
 */
u32 AIGetDMALength(void)
{
	return (__DSPRegs[DSP_DMA_CONTROL_LEN] & 0x7FFF) << 5;
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
BOOL AICheckInit(void)
{
	return __AI_init_flag;
}

/**
 * @note Address: N/A
 * @note Size: 0x44
 */
AISCallback AIRegisterStreamCallback(AISCallback newCallback)
{
	s32 previousInterruptState;
	AISCallback previousCallback;

	previousCallback = __AIS_Callback;

	previousInterruptState = OSDisableInterrupts();
	__AIS_Callback         = newCallback;
	OSRestoreInterrupts(previousInterruptState);

	return previousCallback;
}

/**
 * @note Address: N/A
 * @note Size: 0x10
 */
u32 AIGetStreamSampleCount(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0x18
 */
void AIResetStreamSampleCount(void)
{
	__AIRegs[AI_CONTROL] = (__AIRegs[AI_CONTROL] & ~AI_CONTROL_STREAM_SAMPLE_COUNT) | AI_CONTROL_STREAM_SAMPLE_COUNT;
}

/**
 * @note Address: N/A
 * @note Size: 0xC
 */

void AISetStreamTrigger(u32 trigger)
{
	__AIRegs[AI_INTRPT_TIMING] = trigger;
}

/**
 * @note Address: N/A
 * @note Size: 0x10
 */
u32 AIGetStreamTrigger(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x8017480C
 * @note Size: 0xD8
 */
void AISetStreamPlayState(u32 playState)
{
	s32 previousInterruptState;
	u8 rightVolume;
	u8 leftVolume;

	// If the requested state is the same as the current state, do nothing
	if (playState == AIGetStreamPlayState()) {
		return;
	}

	// If the sample rate is 0 and the requested state is play, initialize the sample rate converter
	if (AIGetStreamSampleRate() == 0 && playState == TRUE) {
		rightVolume = AIGetStreamVolRight();
		leftVolume  = AIGetStreamVolLeft();

		// Temporarily mute the audio
		AISetStreamVolRight(0);
		AISetStreamVolLeft(0);

		// Disable interrupts and initialize the sample rate converter
		previousInterruptState = OSDisableInterrupts();
		__AI_SRC_INIT();

		// Set the stream and state bits in the control register
		__AIRegs[AI_CONTROL] = (__AIRegs[AI_CONTROL] & ~AI_CONTROL_STREAM_SAMPLE_COUNT) | AI_CONTROL_STREAM_SAMPLE_COUNT;
		__AIRegs[AI_CONTROL] = (__AIRegs[AI_CONTROL] & ~AI_CONTROL_PLAY_STATE) | AI_CONTROL_PLAY_STATE;

		// Restore the previous interrupt state
		OSRestoreInterrupts(previousInterruptState);

		// Restore the audio volume
		AISetStreamVolLeft(rightVolume);
		AISetStreamVolRight(leftVolume);
	} else {
		// Set the state bit in the control register to the requested state
		__AIRegs[AI_CONTROL] = (__AIRegs[AI_CONTROL] & ~AI_CONTROL_PLAY_STATE) | playState;
	}
}

/**
 * @note Address: 0x801748E4
 * @note Size: 0x10
 */
u32 AIGetStreamPlayState(void)
{
	return __AIRegs[AI_CONTROL] & AI_CONTROL_PLAY_STATE;
}

/**
 * @note Address: 0x801748F4
 * @note Size: 0xE0
 */
void AISetDSPSampleRate(u32 rate)
{
	u32 playState;
	s32 previousInterruptState;
	u8 leftVolume;
	u8 rightVolume;
	u32 streamSampleRate;

	// If the requested rate is the same as the current rate, do nothing
	if (rate == AIGetDSPSampleRate()) {
		return;
	}

	// Clear the DSP sample rate bit in the control register
	__AIRegs[AI_CONTROL] &= ~AI_CONTROL_DSP_SAMPLE_RATE;

	if (rate == 0) {
		leftVolume       = AIGetStreamVolLeft();
		rightVolume      = AIGetStreamVolRight();
		playState        = AIGetStreamPlayState();
		streamSampleRate = AIGetStreamSampleRate();

		// Temporarily mute the audio
		AISetStreamVolLeft(0);
		AISetStreamVolRight(0);

		// Disable interrupts and initialize the sample rate converter
		previousInterruptState = OSDisableInterrupts();
		__AI_SRC_INIT();

		// Set the stream sample count, stream sample rate, and play state bits in the control register
		__AIRegs[AI_CONTROL] = (__AIRegs[AI_CONTROL] & ~AI_CONTROL_STREAM_SAMPLE_COUNT) | AI_CONTROL_STREAM_SAMPLE_COUNT;
		__AIRegs[AI_CONTROL] = (__AIRegs[AI_CONTROL] & ~AI_CONTROL_STREAM_SAMPLE_RATE) | (streamSampleRate * 2);
		__AIRegs[AI_CONTROL] = (__AIRegs[AI_CONTROL] & ~AI_CONTROL_PLAY_STATE) | playState;

		// Set the DSP sample rate bit in the control register
		__AIRegs[AI_CONTROL] |= AI_CONTROL_DSP_SAMPLE_RATE;

		// Restore the previous interrupt state
		OSRestoreInterrupts(previousInterruptState);

		// Restore the audio volume
		AISetStreamVolLeft(leftVolume);
		AISetStreamVolRight(rightVolume);
	}
}

/**
 * @note Address: 0x801749D4
 * @note Size: 0x14
 */
u32 AIGetDSPSampleRate(void)
{
	return ((__AIRegs[AI_CONTROL] >> 6) & 1) ^ 1;
}

/**
 * @note Address: N/A
 * @note Size: 0x28
 */
void AISetStreamSampleRate(u32 rate)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0x20
 */
void __AI_DEBUG_set_stream_sample_rate(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x801749E8
 * @note Size: 0xD4
 */
static void __AI_set_stream_sample_rate(u32 rate)
{
	s32 previousInterruptState;
	s32 playState;
	u8 leftVolume;
	u8 rightVolume;
	s32 dspSampleRateState;

	// If the requested rate is the same as the current rate, do nothing
	if (rate == AIGetStreamSampleRate()) {
		return;
	}

	playState   = AIGetStreamPlayState();
	leftVolume  = AIGetStreamVolLeft();
	rightVolume = AIGetStreamVolRight();

	// Temporarily mute the audio
	AISetStreamVolRight(0);
	AISetStreamVolLeft(0);

	// Save the state of the DSP sample rate bit and clear it in the control register
	dspSampleRateState = __AIRegs[AI_CONTROL] & AI_CONTROL_DSP_SAMPLE_RATE;
	__AIRegs[AI_CONTROL] &= ~AI_CONTROL_DSP_SAMPLE_RATE;

	// Disable interrupts and initialize the sample rate converter
	previousInterruptState = OSDisableInterrupts();
	__AI_SRC_INIT();

	// Restore the DSP sample rate bit in the control register
	__AIRegs[AI_CONTROL] |= dspSampleRateState;

	// Set the stream sample count and stream sample rate bits in the control register
	__AIRegs[AI_CONTROL] = (__AIRegs[AI_CONTROL] & ~AI_CONTROL_STREAM_SAMPLE_COUNT) | AI_CONTROL_STREAM_SAMPLE_COUNT;
	__AIRegs[AI_CONTROL] = (__AIRegs[AI_CONTROL] & ~AI_CONTROL_STREAM_SAMPLE_RATE) | (rate * 2);

	// Restore the previous interrupt state
	OSRestoreInterrupts(previousInterruptState);

	// Restore the audio play state and volume
	AISetStreamPlayState(playState);
	AISetStreamVolLeft(leftVolume);
	AISetStreamVolRight(rightVolume);
}

/**
 * @note Address: 0x80174ABC
 * @note Size: 0x10
 */
u32 AIGetStreamSampleRate(void)
{
	return (__AIRegs[AI_CONTROL] >> 1) & 1;
}

/**
 * @note Address: 0x80174ACC
 * @note Size: 0x1C
 */
void AISetStreamVolLeft(u8 volume)
{
	__AIRegs[AI_VOLUME] = (__AIRegs[AI_VOLUME] & ~0xFF) | (volume & 0xFF);
}

/**
 * @note Address: 0x80174AE8
 * @note Size: 0x10
 */
u8 AIGetStreamVolLeft(void)
{
	return __AIRegs[AI_VOLUME];
}

/**
 * @note Address: 0x80174AF8
 * @note Size: 0x1C
 */
void AISetStreamVolRight(u8 volume)
{
	__AIRegs[AI_VOLUME] = (__AIRegs[AI_VOLUME] & ~0xFF00) | ((volume & 0xFF) << 8);
}

/**
 * @note Address: 0x80174B14
 * @note Size: 0x10
 */
u8 AIGetStreamVolRight(void)
{
	return __AIRegs[AI_VOLUME] >> 8;
}

/**
 * @note Address: 0x80174B24
 * @note Size: 0x16C
 */
void AIInit(u8* stack)
{
	// If AI is already initialized, do nothing
	if (__AI_init_flag == TRUE) {
		return;
	}

	// Register AI version
	OSRegisterVersion(__AIVersion);

	// Set bounds and buffer sizes in ticks
	bound_32KHz = OSNanosecondsToTicks(31524);
	bound_48KHz = OSNanosecondsToTicks(42024);
	min_wait    = OSNanosecondsToTicks(42000);
	max_wait    = OSNanosecondsToTicks(63000);
	buffer      = OSNanosecondsToTicks(3000);

	__AIRegs[AI_CONTROL] &= ~0x15;
	__AIRegs[AI_VOLUME]        = 0;
	__AIRegs[AI_INTRPT_TIMING] = 0;
	AIResetStreamSampleCount();
	if (AIGetDSPSampleRate() != 0) {
		s32 oldInts;
		__AIRegs[AI_CONTROL] &= ~AI_CONTROL_DSP_SAMPLE_RATE;
		oldInts = OSDisableInterrupts();
		__AI_SRC_INIT();
		__AIRegs[AI_CONTROL] |= AI_CONTROL_DSP_SAMPLE_RATE;
		OSRestoreInterrupts(oldInts);
	}

	// Clear callback and set callback stack
	__AID_Callback  = 0;
	__CallbackStack = stack;

	// Set interrupt handler and unmask interrupt
	__OSSetInterruptHandler(5, __AIDHandler);
	__OSUnmaskInterrupts(OS_INTERRUPTMASK_DSP_AI);

	// Set AI initialisation flag to TRUE
	__AI_init_flag = TRUE;
}

/**
 * @note Address: 0x80174C90
 * @note Size: 0xC
 */
void AIReset(void)
{
	__AI_init_flag = FALSE;
}

/**
 * @note Address: 0x80174C9C
 * @note Size: 0x7C
 */
static void __AISHandler(s16 interrupt, OSContext* context)
{
	OSContext tmpContext;
	__AIRegs[AI_CONTROL] |= 8;
	OSClearContext(&tmpContext);
	OSSetCurrentContext(&tmpContext);
	if (__AIS_Callback != NULL) {
		__AIS_Callback(__AIRegs[AI_SAMPLE_COUNTER]);
	}
	OSClearContext(&tmpContext);
	OSSetCurrentContext(context);
}

/**
 * @note Address: 0x80174D18
 * @note Size: 0xAC
 */
static void __AIDHandler(s16 interrupt, OSContext* context)
{
	OSContext tempContext;
	__DSPRegs[5] = (__DSPRegs[5] & ~0xA0) | 8;
	OSClearContext(&tempContext);
	OSSetCurrentContext(&tempContext);
	if (__AID_Callback && !__AID_Active) {
		__AID_Active = TRUE;
		if (__CallbackStack) {
			__AICallbackStackSwitch(__AID_Callback);
		} else {
			__AID_Callback();
		}

		__AID_Active = FALSE;
	}

	OSClearContext(&tempContext);
	OSSetCurrentContext(context);
}

/**
 * @note Address: 0x80174E1C
 * @note Size: 0x1E4
 */
static void __AI_SRC_INIT(void)
{
	OSTime rising_32khz = 0;
	OSTime rising_48khz = 0;
	OSTime diff         = 0;
	OSTime t1           = 0;
	OSTime temp         = 0;
	u32 temp0           = 0;
	u32 temp1           = 0;
	u32 done            = 0;
	u32 volume          = 0;
	u32 Init_Cnt        = 0;
	u32 walking         = 0;

	walking  = 0;
	Init_Cnt = 0;
	temp     = 0;

	while (!done) {
		__AIRegs[AI_CONTROL] = (__AIRegs[AI_CONTROL] & ~0x20) | 0x20;
		__AIRegs[AI_CONTROL] &= ~2;
		__AIRegs[AI_CONTROL] = (__AIRegs[AI_CONTROL] & ~1) | 1;

		temp0 = __AIRegs[AI_SAMPLE_COUNTER] & 0x7FFFFFFF;

		while (temp0 == (__AIRegs[AI_SAMPLE_COUNTER] & 0x7FFFFFFF))
			;
		rising_32khz = OSGetTime();

		__AIRegs[AI_CONTROL] = (__AIRegs[AI_CONTROL] & ~2) | 2;
		__AIRegs[AI_CONTROL] = (__AIRegs[AI_CONTROL] & ~1) | 1;

		temp1 = __AIRegs[AI_SAMPLE_COUNTER] & 0x7FFFFFFF;
		while (temp1 == (__AIRegs[AI_SAMPLE_COUNTER] & 0x7FFFFFFF))
			;

		rising_48khz = OSGetTime();

		diff = rising_48khz - rising_32khz;
		__AIRegs[AI_CONTROL] &= ~2;
		__AIRegs[AI_CONTROL] &= ~1;

		if (diff < (bound_32KHz - buffer)) {
			temp = min_wait;
			done = 1;
			++Init_Cnt;
		} else if (diff >= (bound_32KHz + buffer) && diff < (bound_48KHz - buffer)) {
			temp = max_wait;
			done = 1;
			++Init_Cnt;
		} else {
			done    = 0;
			walking = 1;
			++Init_Cnt;
		}
	}

	while ((rising_48khz + temp) > OSGetTime())
		;
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
void __ai_src_get_time(void)
{
	// UNUSED FUNCTION
}
