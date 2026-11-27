#include "Dolphin/ai.h"
#include "Dolphin/hw_regs.h"
#include "Dolphin/os.h"

void __AICallbackStackSwitch(AIDCallback cb);

char* __AIVersion = "<< Dolphin SDK - AI\trelease build: Apr  5 2004 04:15:02 (0x2301) >>";

static AISCallback __AIS_Callback = NULL;
static AIDCallback __AID_Callback = NULL;
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

	__DSPRegs[DSP_DMA_START_HI] = (__DSPRegs[DSP_DMA_START_HI] & ~DSP_DMA_HI_FIELD_MASK) | (u16)(address >> 16);
	__DSPRegs[DSP_DMA_START_LO] = (__DSPRegs[DSP_DMA_START_LO] & ~DSP_DMA_LO_FIELD_MASK) | (u16)address;
	__DSPRegs[DSP_DMA_CONTROL_LEN] = (__DSPRegs[DSP_DMA_CONTROL_LEN] & ~DSP_DMA_LENGTH_MASK)
	    | (u16)(length >> DSP_DMA_BLOCK_SHIFT);

	OSRestoreInterrupts(previousInterruptState);
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
 * @note Address: 0x801747F0
 * @note Size: 0x1C
 */
u32 AIGetDMAStartAddr(void)
{
	const u32 startAddressHigh = (__DSPRegs[DSP_DMA_START_HI] & DSP_DMA_HI_FIELD_MASK) << 16;
	const u32 startAddressLow  = __DSPRegs[DSP_DMA_START_LO] & DSP_DMA_LO_FIELD_MASK;

	return startAddressHigh | startAddressLow;
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
 * @note Size: 0x18
 */
void AIResetStreamSampleCount(void)
{
	__AIRegs[AI_CONTROL] = (__AIRegs[AI_CONTROL] & ~AI_CONTROL_STREAM_COUNT_RESET) | AI_CONTROL_STREAM_COUNT_RESET;
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

		// Reset the stream counter and start playback
		AIResetStreamSampleCount();
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

		// Reset the stream counter and restore the sample rate and play state
		AIResetStreamSampleCount();
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

	// Reset the stream counter and set the new sample rate
	AIResetStreamSampleCount();
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

	// Initialize AI stream settings
	AISetStreamVolRight(0);
	AISetStreamVolLeft(0);
	AISetStreamTrigger(0);
	AIResetStreamSampleCount();
	__AI_set_stream_sample_rate(1);
	AISetDSPSampleRate(0);

	// Clear callbacks and set callback stack
	__AIS_Callback  = 0;
	__AID_Callback  = 0;
	__CallbackStack = stack;

	// Set interrupt handlers and unmask interrupts
	__OSSetInterruptHandler(__OS_INTERRUPT_DSP_AI, __AIDHandler);
	__OSUnmaskInterrupts(OS_INTERRUPTMASK_DSP_AI);
	__OSSetInterruptHandler(__OS_INTERRUPT_AI_AI, __AISHandler);
	__OSUnmaskInterrupts(OS_INTERRUPTMASK_AI);

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
	__AIRegs[AI_CONTROL] |= AI_STATUS_STREAM_INTERRUPT;
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
	u32 status = __DSPRegs[DSP_CONTROL_STATUS];
	__DSPRegs[DSP_CONTROL_STATUS] = (status & ~(DSP_STATUS_DSP_INTERRUPT | DSP_STATUS_ARAM_INTERRUPT)) | DSP_STATUS_AI_INTERRUPT;
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
	OSTime rising48kHz = 0;
	u32 calibrated = 0;
	OSTime wait = 0;
	OSTime deadline;

	while (!calibrated) {
		OSTime rising32kHz;
		OSTime elapsed;
		u32 sampleCount32kHz;
		u32 sampleCount48kHz;
		OSTime lower32kHzBound;

		AIResetStreamSampleCount();
		__AIRegs[AI_CONTROL] &= ~AI_CONTROL_STREAM_SAMPLE_RATE;
		__AIRegs[AI_CONTROL] = (__AIRegs[AI_CONTROL] & ~AI_CONTROL_PLAY_STATE) | AI_CONTROL_PLAY_STATE;

		sampleCount32kHz = __AIRegs[AI_SAMPLE_COUNTER];

		while (sampleCount32kHz == __AIRegs[AI_SAMPLE_COUNTER])
			;
		rising32kHz = OSGetTime();

		__AIRegs[AI_CONTROL] = (__AIRegs[AI_CONTROL] & ~AI_CONTROL_STREAM_SAMPLE_RATE) | AI_CONTROL_STREAM_SAMPLE_RATE;
		__AIRegs[AI_CONTROL] = (__AIRegs[AI_CONTROL] & ~AI_CONTROL_PLAY_STATE) | AI_CONTROL_PLAY_STATE;

		sampleCount48kHz = __AIRegs[AI_SAMPLE_COUNTER];
		while (sampleCount48kHz == __AIRegs[AI_SAMPLE_COUNTER])
			;

		rising48kHz = OSGetTime();

		elapsed = rising48kHz - rising32kHz;
		__AIRegs[AI_CONTROL] &= ~AI_CONTROL_STREAM_SAMPLE_RATE;
		__AIRegs[AI_CONTROL] &= ~AI_CONTROL_PLAY_STATE;

		lower32kHzBound = bound_32KHz - buffer;
		if (elapsed < lower32kHzBound) {
			wait = min_wait;
			calibrated = 1;
		} else {
			OSTime upper32kHzBound = bound_32KHz + buffer;
			if (elapsed >= upper32kHzBound && elapsed < (bound_48KHz - buffer)) {
				wait = max_wait;
				calibrated = 1;
			} else {
				calibrated = 0;
			}
		}
	}

	deadline = rising48kHz + wait;
	while (deadline > OSGetTime())
		;
}
