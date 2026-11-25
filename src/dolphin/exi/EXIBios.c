#include "Dolphin/exi.h"

#pragma scheduling off

#define EXI_INTERRUPT_STRIDE (__OS_INTERRUPT_EXI_1_EXI - __OS_INTERRUPT_EXI_0_EXI)
#define EXI_PROBE_INTERVAL_MS 100
#define EXI_PROBE_SETTLE_MS   300

char* __EXIVersion = "<< Dolphin SDK - EXI\trelease build: Apr  5 2004 04:14:14 (0x2301) >>";

static EXIControl Ecb[EXI_MAX_CHAN];
static u32 IDSerialPort1;

static BOOL __EXIProbe(s32 chan);

/**
 * @note Address: 0x8015D9C4
 * @note Size: 0xF4
 */
static void SetExiInterruptMask(s32 chan, EXIControl* exi)
{
	EXIControl* exi2;

	exi2 = &Ecb[2];
	switch (chan) {
	case 0:
		if ((exi->exiCallback == 0 && exi2->exiCallback == 0) || (exi->state & EXI_STATE_LOCKED)) {
			__OSMaskInterrupts(OS_INTERRUPTMASK_EXI_0_EXI | OS_INTERRUPTMASK_EXI_2_EXI);
		} else {
			__OSUnmaskInterrupts(OS_INTERRUPTMASK_EXI_0_EXI | OS_INTERRUPTMASK_EXI_2_EXI);
		}
		break;
	case 1:
		if (exi->exiCallback == 0 || (exi->state & EXI_STATE_LOCKED)) {
			__OSMaskInterrupts(OS_INTERRUPTMASK_EXI_1_EXI);
		} else {
			__OSUnmaskInterrupts(OS_INTERRUPTMASK_EXI_1_EXI);
		}
		break;
	case 2:
		if (__OSGetInterruptHandler(__OS_INTERRUPT_PI_DEBUG) == 0 || (exi->state & EXI_STATE_LOCKED)) {
			__OSMaskInterrupts(OS_INTERRUPTMASK_PI_DEBUG);
		} else {
			__OSUnmaskInterrupts(OS_INTERRUPTMASK_PI_DEBUG);
		}
		break;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x154
 */
static void CompleteTransfer(s32 chan)
{
	EXIControl* exi = &Ecb[chan];
	u8* buf;
	u32 data;
	int i;
	int len;

	if (exi->state & EXI_STATE_BUSY) {
		if ((exi->state & EXI_STATE_IMM) && (len = exi->immLen)) {
			buf  = exi->immBuf;
			data = EXI_REG(chan, EXI_REG_DATA);
			for (i = 0; i < len; i++) {
				*buf++ = (u8)(data >> ((3 - i) * 8));
			}
		}
		exi->state &= ~EXI_STATE_BUSY;
	}
}

/**
 * @note Address: 0x8015DAB8
 * @note Size: 0x25C
 */
BOOL EXIImm(s32 chan, void* buf, s32 len, u32 type, EXICallback callback)
{
	EXIControl* exi = &Ecb[chan];
	BOOL enabled;

	enabled = OSDisableInterrupts();
	if ((exi->state & EXI_STATE_BUSY) || !(exi->state & EXI_STATE_SELECTED)) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}

	exi->tcCallback = callback;
	if (exi->tcCallback) {
		EXIClearInterrupts(chan, FALSE, TRUE, FALSE);
		__OSUnmaskInterrupts(OS_INTERRUPTMASK_EXI_0_TC >> (EXI_INTERRUPT_STRIDE * chan));
	}

	exi->state |= EXI_STATE_IMM;

	if (type != EXI_READ) {
		u32 data;
		int i;

		data = 0;
		for (i = 0; i < len; i++) {
			data |= (u32)((u8*)buf)[i] << ((3 - i) * 8);
		}
		EXI_REG(chan, EXI_REG_DATA) = data;
	}

	exi->immBuf = buf;
	exi->immLen = (type != EXI_WRITE) ? len : 0;

	EXI_REG(chan, EXI_REG_CONTROL) = EXI_CR_IMM(type, len);

	OSRestoreInterrupts(enabled);

	return TRUE;
}

/**
 * @note Address: 0x8015DD14
 * @note Size: 0xA0
 */
BOOL EXIImmEx(s32 chan, void* buf, s32 len, u32 mode)
{
	s32 xLen;

	while (len) {
		xLen = (len < 4) ? len : 4;
		if (!EXIImm(chan, buf, xLen, mode, NULL)) {
			return FALSE;
		}

		if (!EXISync(chan)) {
			return FALSE;
		}

		buf = (u8*)buf + xLen;
		len -= xLen;
	}
	return TRUE;
}

/**
 * @note Address: 0x8015DDB4
 * @note Size: 0xEC
 */
BOOL EXIDma(s32 chan, void* buf, s32 len, u32 type, EXICallback callback)
{
	EXIControl* exi = &Ecb[chan];
	BOOL enabled;

	enabled = OSDisableInterrupts();
	if ((exi->state & EXI_STATE_BUSY) || !(exi->state & EXI_STATE_SELECTED)) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}

	exi->tcCallback = callback;
	if (exi->tcCallback) {
		EXIClearInterrupts(chan, FALSE, TRUE, FALSE);
		__OSUnmaskInterrupts(OS_INTERRUPTMASK_EXI_0_TC >> (EXI_INTERRUPT_STRIDE * chan));
	}

	exi->state |= EXI_STATE_DMA;

	EXI_REG(chan, EXI_REG_DMA_ADDR) = (u32)buf & EXI_DMA_ADDR_MASK;
	EXI_REG(chan, EXI_REG_DMA_LEN)  = (u32)len;
	EXI_REG(chan, EXI_REG_CONTROL) = EXI_CR_START | EXI_CR_DMA | EXI_CR_RW(type);

	OSRestoreInterrupts(enabled);

	return TRUE;
}

/**
 * @note Address: 0x8015DEA0
 * @note Size: 0x24C
 */
BOOL EXISync(s32 chan)
{
	EXIControl* exi = &Ecb[chan];
	BOOL rc         = FALSE;
	BOOL enabled;

	while (exi->state & EXI_STATE_SELECTED) {
		if (!(EXI_REG(chan, EXI_REG_CONTROL) & EXI_CR_START)) {
			enabled = OSDisableInterrupts();
			if (exi->state & EXI_STATE_SELECTED) {
				CompleteTransfer(chan);
				if (__OSGetDIConfig() != 0xff || ((OSGetConsoleType() & 0xF0000000) == OS_CONSOLE_TDEVKIT) || exi->immLen != 4
				    || (EXI_REG(chan, EXI_REG_CPR) & EXI_CPR_CLK_MASK) != EXI_CPR_CLK(EXI_FREQ_1M)
				    || (EXI_REG(chan, EXI_REG_DATA) != EXI_USB_ADAPTER
				        && EXI_REG(chan, EXI_REG_DATA) != EXI_IS_VIEWER
				        && EXI_REG(chan, EXI_REG_DATA) != EXI_ETHER_VIEWER)
				    || __OSDeviceCode == 0x8200) {
					rc = TRUE;
				}
			}
			OSRestoreInterrupts(enabled);
			break;
		}
	}
	return rc;
}

/**
 * Returns the channel parameters and pending flags as read before the write.
 *
 * @note Address: 0x8015E0EC
 * @note Size: 0x48
 */
s32 EXIClearInterrupts(s32 chan, BOOL exi, BOOL tc, BOOL ext)
{
	u32 cpr;
	s32 prev;

	cpr = EXI_REG(chan, EXI_REG_CPR);
	prev = (s32)cpr;
	cpr &= EXI_CPR_INT_ENABLES | EXI_CPR_CLK_MASK | EXI_CPR_CS_MASK;
	if (exi)
		cpr |= EXI_CPR_EXI_INT;
	if (tc)
		cpr |= EXI_CPR_TC_INT;
	if (ext)
		cpr |= EXI_CPR_EXT_INT;
	EXI_REG(chan, EXI_REG_CPR) = cpr;
	return prev;
}

/**
 * @note Address: 0x8015E134
 * @note Size: 0x7C
 */
EXICallback EXISetExiCallback(s32 chan, EXICallback exiCallback)
{
	EXIControl* exi = &Ecb[chan];
	EXICallback prev;
	BOOL enabled;

	enabled          = OSDisableInterrupts();
	prev             = exi->exiCallback;
	exi->exiCallback = exiCallback;

	if (chan != 2) {
		SetExiInterruptMask(chan, exi);
	} else {
		SetExiInterruptMask(0, &Ecb[0]);
	}

	OSRestoreInterrupts(enabled);
	return prev;
}

/**
 * @note Address: N/A
 * @note Size: 0x4C
 */
void EXIProbeReset(void)
{
	__EXIProbeStartTime[0] = __EXIProbeStartTime[1] = 0;
	Ecb[0].idTime = Ecb[1].idTime = 0;
	__EXIProbe(0);
	__EXIProbe(1);
}

/**
 * @note Address: 0x8015E1B0
 * @note Size: 0x174
 */
static BOOL __EXIProbe(s32 chan)
{
	EXIControl* exi = &Ecb[chan];
	BOOL enabled;
	BOOL rc;
	u32 cpr;
	s32 t;

	if (chan == 2) {
		return TRUE;
	}

	rc      = TRUE;
	enabled = OSDisableInterrupts();
	cpr     = EXI_REG(chan, EXI_REG_CPR);
	if (!(exi->state & EXI_STATE_ATTACHED)) {
		if (cpr & EXI_CPR_EXT_INT) {
			EXIClearInterrupts(chan, FALSE, FALSE, TRUE);
			__EXIProbeStartTime[chan] = exi->idTime = 0;
		}

		if (cpr & EXI_CPR_EXT_PRESENT) {
			t = (s32)(OSTicksToMilliseconds(OSGetTime()) / EXI_PROBE_INTERVAL_MS) + 1;
			if (__EXIProbeStartTime[chan] == 0) {
				__EXIProbeStartTime[chan] = t;
			}
			if (t - __EXIProbeStartTime[chan] < EXI_PROBE_SETTLE_MS / EXI_PROBE_INTERVAL_MS) {
				rc = FALSE;
			}
		} else {
			__EXIProbeStartTime[chan] = exi->idTime = 0;
			rc                                      = FALSE;
		}
	} else if (!(cpr & EXI_CPR_EXT_PRESENT) || (cpr & EXI_CPR_EXT_INT)) {
		__EXIProbeStartTime[chan] = exi->idTime = 0;
		rc                                      = FALSE;
	}
	OSRestoreInterrupts(enabled);

	return rc;
}

/**
 * @note Address: 0x8015E324
 * @note Size: 0x80
 */
BOOL EXIProbe(s32 chan)
{
	EXIControl* exi = &Ecb[chan];
	BOOL rc;
	u32 id;

	rc = __EXIProbe(chan);
	if (rc && exi->idTime == 0) {
		rc = EXIGetID(chan, 0, &id) ? TRUE : FALSE;
	}
	return rc;
}

/**
 * @note Address: 0x8015E3A4
 * @note Size: 0xB4
 */
s32 EXIProbeEx(s32 chan)
{
	if (EXIProbe(chan)) {
		return 1;
	} else if (__EXIProbeStartTime[chan] != 0) {
		return 0;
	} else {
		return -1;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0xD4
 */
static BOOL __EXIAttach(s32 chan, EXICallback extCallback)
{
	EXIControl* exi = &Ecb[chan];
	BOOL enabled;

	enabled = OSDisableInterrupts();
	if ((exi->state & EXI_STATE_ATTACHED) || __EXIProbe(chan) == FALSE) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}

	EXIClearInterrupts(chan, TRUE, FALSE, FALSE);

	exi->extCallback = extCallback;
	__OSUnmaskInterrupts(OS_INTERRUPTMASK_EXI_0_EXT >> (EXI_INTERRUPT_STRIDE * chan));
	exi->state |= EXI_STATE_ATTACHED;
	OSRestoreInterrupts(enabled);

	return TRUE;
}

/**
 * @note Address: 0x8015E458
 * @note Size: 0x10C
 */
BOOL EXIAttach(s32 chan, EXICallback extCallback)
{
	EXIControl* exi = &Ecb[chan];
	BOOL enabled;
	BOOL rc;

	EXIProbe(chan);

	enabled = OSDisableInterrupts();
	if (exi->idTime == 0) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}
	rc = __EXIAttach(chan, extCallback);
	OSRestoreInterrupts(enabled);
	return rc;
}

/**
 * @note Address: 0x8015E564
 * @note Size: 0xBC
 */
BOOL EXIDetach(s32 chan)
{
	EXIControl* exi = &Ecb[chan];
	BOOL enabled;

	enabled = OSDisableInterrupts();
	if (!(exi->state & EXI_STATE_ATTACHED)) {
		OSRestoreInterrupts(enabled);
		return TRUE;
	}
	if ((exi->state & EXI_STATE_LOCKED) && exi->dev == 0) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}

	exi->state &= ~EXI_STATE_ATTACHED;
	__OSMaskInterrupts((OS_INTERRUPTMASK_EXI_0_EXT | OS_INTERRUPTMASK_EXI_0_EXI) >> (EXI_INTERRUPT_STRIDE * chan));
	OSRestoreInterrupts(enabled);
	return TRUE;
}

/**
 * @note Address: 0x8015E620
 * @note Size: 0x12C
 */
BOOL EXISelect(s32 chan, u32 dev, u32 freq)
{
	EXIControl* exi = &Ecb[chan];
	u32 cpr;
	BOOL enabled;

	enabled = OSDisableInterrupts();
	if ((exi->state & EXI_STATE_SELECTED)
	    || chan != 2
	           && (dev == 0 && !(exi->state & EXI_STATE_ATTACHED) && !__EXIProbe(chan) || !(exi->state & EXI_STATE_LOCKED)
	               || (exi->dev != dev))) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}

	exi->state |= EXI_STATE_SELECTED;
	cpr = EXI_REG(chan, EXI_REG_CPR);
	cpr &= EXI_CPR_INT_ENABLES;
	cpr |= EXI_CPR_CS(dev) | EXI_CPR_CLK(freq);
	EXI_REG(chan, EXI_REG_CPR) = cpr;

	if (exi->state & EXI_STATE_ATTACHED) {
		switch (chan) {
		case 0:
			__OSMaskInterrupts(OS_INTERRUPTMASK_EXI_0_EXT);
			break;
		case 1:
			__OSMaskInterrupts(OS_INTERRUPTMASK_EXI_1_EXT);
			break;
		}
	}

	OSRestoreInterrupts(enabled);
	return TRUE;
}

/**
 * @note Address: 0x8015E74C
 * @note Size: 0x110
 */
BOOL EXIDeselect(s32 chan)
{
	EXIControl* exi = &Ecb[chan];
	u32 cpr;
	BOOL enabled;

	enabled = OSDisableInterrupts();
	if (!(exi->state & EXI_STATE_SELECTED)) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}
	exi->state &= ~EXI_STATE_SELECTED;
	cpr = EXI_REG(chan, EXI_REG_CPR);
	EXI_REG(chan, EXI_REG_CPR) = cpr & EXI_CPR_INT_ENABLES;

	if (exi->state & EXI_STATE_ATTACHED) {
		switch (chan) {
		case 0:
			__OSUnmaskInterrupts(OS_INTERRUPTMASK_EXI_0_EXT);
			break;
		case 1:
			__OSUnmaskInterrupts(OS_INTERRUPTMASK_EXI_1_EXT);
			break;
		}
	}

	OSRestoreInterrupts(enabled);

	if (chan != 2 && (cpr & EXI_CPR_CS(0))) {
		return __EXIProbe(chan) ? TRUE : FALSE;
	}

	return TRUE;
}

static inline void BeginCallbackContext(OSContext* context)
{
	OSClearContext(context);
	OSSetCurrentContext(context);
}

static inline void EndCallbackContext(OSContext* context, OSContext* interrupted)
{
	OSClearContext(context);
	OSSetCurrentContext(interrupted);
}

/**
 * @note Address: 0x8015E85C
 * @note Size: 0xC8
 */
static void EXIIntrruptHandler(__OSInterrupt interrupt, OSContext* context)
{
	s32 chan;
	EXIControl* exi;
	EXICallback callback;
	OSContext callbackContext;

	chan = (interrupt - __OS_INTERRUPT_EXI_0_EXI) / EXI_INTERRUPT_STRIDE;
	exi  = &Ecb[chan];
	EXIClearInterrupts(chan, TRUE, FALSE, FALSE);
	callback = exi->exiCallback;
	if (callback) {
		BeginCallbackContext(&callbackContext);
		callback(chan, context);
		EndCallbackContext(&callbackContext, context);
	}
}

/**
 * @note Address: 0x8015E924
 * @note Size: 0x218
 */
static void TCIntrruptHandler(__OSInterrupt interrupt, OSContext* context)
{
	s32 chan;
	EXIControl* exi;
	EXICallback callback;
	OSContext callbackContext;

	chan = (interrupt - __OS_INTERRUPT_EXI_0_TC) / EXI_INTERRUPT_STRIDE;
	exi  = &Ecb[chan];
	__OSMaskInterrupts(OS_INTERRUPTMASK(interrupt));
	EXIClearInterrupts(chan, FALSE, TRUE, FALSE);
	callback = exi->tcCallback;
	if (callback) {
		exi->tcCallback = 0;
		CompleteTransfer(chan);
		BeginCallbackContext(&callbackContext);
		callback(chan, context);
		EndCallbackContext(&callbackContext, context);
	}
}

/**
 * @note Address: 0x8015EB3C
 * @note Size: 0xD0
 */
static void EXTIntrruptHandler(__OSInterrupt interrupt, OSContext* context)
{
	s32 chan;
	EXIControl* exi;
	EXICallback callback;

	chan = (interrupt - __OS_INTERRUPT_EXI_0_EXT) / EXI_INTERRUPT_STRIDE;
	__OSMaskInterrupts((OS_INTERRUPTMASK_EXI_0_EXT | OS_INTERRUPTMASK_EXI_0_EXI) >> (EXI_INTERRUPT_STRIDE * chan));
	exi      = &Ecb[chan];
	callback = exi->extCallback;
	exi->state &= ~EXI_STATE_ATTACHED;
	if (callback) {
		OSContext exceptionContext;

		BeginCallbackContext(&exceptionContext);

		exi->extCallback = 0;
		callback(chan, context);

		EndCallbackContext(&exceptionContext, context);
	}
}

/**
 * @note Address: 0x8015EC0C
 * @note Size: 0x1D4
 */
void EXIInit(void)
{
	u32 id;
	while (((EXI_REG(0, EXI_REG_CONTROL) & EXI_CR_START) == EXI_CR_START)
	       || ((EXI_REG(1, EXI_REG_CONTROL) & EXI_CR_START) == EXI_CR_START)
	       || ((EXI_REG(2, EXI_REG_CONTROL) & EXI_CR_START) == EXI_CR_START)) {
		continue;
	}

	__OSMaskInterrupts(OS_INTERRUPTMASK_EXI);

	EXI_REG(0, EXI_REG_CPR) = 0;
	EXI_REG(1, EXI_REG_CPR) = 0;
	EXI_REG(2, EXI_REG_CPR) = 0;

	EXI_REG(0, EXI_REG_CPR) = EXI_CPR_ROM_DISABLE;

	__OSSetInterruptHandler(__OS_INTERRUPT_EXI_0_EXI, EXIIntrruptHandler);
	__OSSetInterruptHandler(__OS_INTERRUPT_EXI_0_TC, TCIntrruptHandler);
	__OSSetInterruptHandler(__OS_INTERRUPT_EXI_0_EXT, EXTIntrruptHandler);
	__OSSetInterruptHandler(__OS_INTERRUPT_EXI_1_EXI, EXIIntrruptHandler);
	__OSSetInterruptHandler(__OS_INTERRUPT_EXI_1_TC, TCIntrruptHandler);
	__OSSetInterruptHandler(__OS_INTERRUPT_EXI_1_EXT, EXTIntrruptHandler);
	__OSSetInterruptHandler(__OS_INTERRUPT_EXI_2_EXI, EXIIntrruptHandler);
	__OSSetInterruptHandler(__OS_INTERRUPT_EXI_2_TC, TCIntrruptHandler);

	EXIGetID(0, 2, &IDSerialPort1);

	if (__OSInIPL) {
		EXIProbeReset();
	} else if (EXIGetID(0, 0, &id) && id == 0x07010000) {
		__OSEnableBarnacle(1, 0);
	} else if (EXIGetID(1, 0, &id) && id == 0x07010000) {
		__OSEnableBarnacle(0, 2);
	}

	OSRegisterVersion(__EXIVersion);
}

/**
 * @note Address: 0x8015EDE0
 * @note Size: 0xF4
 */
BOOL EXILock(s32 chan, u32 dev, EXICallback unlockedCallback)
{
	EXIControl* exi = &Ecb[chan];
	BOOL enabled;
	int i;

	enabled = OSDisableInterrupts();
	if (exi->state & EXI_STATE_LOCKED) {
		if (unlockedCallback) {
			for (i = 0; i < exi->items; i++) {
				if (exi->queue[i].dev == dev) {
					OSRestoreInterrupts(enabled);
					return FALSE;
				}
			}
			exi->queue[exi->items].callback = unlockedCallback;
			exi->queue[exi->items].dev      = dev;
			exi->items++;
		}
		OSRestoreInterrupts(enabled);
		return FALSE;
	}

	exi->state |= EXI_STATE_LOCKED;
	exi->dev = dev;
	SetExiInterruptMask(chan, exi);

	OSRestoreInterrupts(enabled);
	return TRUE;
}

/**
 * @note Address: 0x8015EED4
 * @note Size: 0xDC
 */
BOOL EXIUnlock(s32 chan)
{
	EXIControl* exi = &Ecb[chan];
	BOOL enabled;
	EXICallback unlockedCallback;

	enabled = OSDisableInterrupts();
	if (!(exi->state & EXI_STATE_LOCKED)) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}
	exi->state &= ~EXI_STATE_LOCKED;
	SetExiInterruptMask(chan, exi);

	if (0 < exi->items) {
		unlockedCallback = exi->queue[0].callback;
		if (0 < --exi->items) {
			memmove(&exi->queue[0], &exi->queue[1], sizeof(exi->queue[0]) * exi->items);
		}
		unlockedCallback(chan, 0);
	}

	OSRestoreInterrupts(enabled);
	return TRUE;
}

/**
 * @note Address: 0x8015EFB0
 * @note Size: 0x18
 */
u32 EXIGetState(s32 chan)
{
	EXIControl* exi = &Ecb[chan];

	return exi->state;
}

/**
 * @note Address: 0x8015EFC8
 * @note Size: 0x28
 */
static void UnlockedHandler(s32 chan, OSContext* context)
{
	u32 id;

	EXIGetID(chan, 0, &id);
}

/**
 * @note Address: 0x8015EFF0
 * @note Size: 0x3B0
 */
s32 EXIGetID(s32 chan, u32 dev, u32* id)
{
	EXIControl* exi = &Ecb[chan];
	BOOL err;
	u32 cmd;
	s32 startTime;
	BOOL enabled;
	BOOL interrupt;

	if (chan == 0 && dev == 2 && IDSerialPort1) {
		*id = IDSerialPort1;
		return 1;
	}

	if (chan < 2 && dev == 0) {
		if (!__EXIProbe(chan)) {
			return 0;
		}

		if (exi->idTime == __EXIProbeStartTime[chan]) {
			*id = exi->id;
			return exi->idTime;
		}

		if (!__EXIAttach(chan, NULL)) {
			return 0;
		}
		startTime = __EXIProbeStartTime[chan];
	}

	interrupt = OSDisableInterrupts();
	err       = !EXILock(chan, dev, (chan < 2 && dev == 0) ? UnlockedHandler : NULL);
	if (!err) {
		err = !EXISelect(chan, dev, EXI_FREQ_1M);
		if (!err) {
			cmd = 0;
			err |= !EXIImm(chan, &cmd, 2, EXI_WRITE, NULL);
			err |= !EXISync(chan);
			err |= !EXIImm(chan, id, 4, EXI_READ, NULL);
			err |= !EXISync(chan);
			err |= !EXIDeselect(chan);
		}
		EXIUnlock(chan);
	}
	OSRestoreInterrupts(interrupt);

	if (chan < 2 && dev == 0) {
		EXIDetach(chan);
		enabled = OSDisableInterrupts();
		err |= (startTime != __EXIProbeStartTime[chan]);
		if (!err) {
			exi->id     = *id;
			exi->idTime = startTime;
		}
		OSRestoreInterrupts(enabled);

		return err ? 0 : exi->idTime;
	}

	return err ? 0 : !0;
}

/**
 * @note Address: N/A
 * @note Size: 0x234
 */
char* EXIGetTypeString(u32 type)
{
	switch (type) {
	case EXI_MEMORY_CARD_59:
		return "Memory Card 59";
	case EXI_MEMORY_CARD_123:
		return "Memory Card 123";
	case EXI_MEMORY_CARD_251:
		return "Memory Card 251";
	case EXI_MEMORY_CARD_507:
		return "Memory Card 507";
	case EXI_MEMORY_CARD_1019:
		return "Memory Card 1019";
	case EXI_MEMORY_CARD_2043:
		return "Memory Card 2043";
	case EXI_USB_ADAPTER:
		return "USB Adapter";
	case 0x80000000 | EXI_MEMORY_CARD_59:
	case 0x80000000 | EXI_MEMORY_CARD_123:
	case 0x80000000 | EXI_MEMORY_CARD_251:
	case 0x80000000 | EXI_MEMORY_CARD_507:
		return "Net Card";
	case EXI_ETHER_VIEWER:
		return "Artist Ether";
	case EXI_MODEM:
		return "Broadband Adapter";
	case EXI_STREAM_HANGER:
		return "Stream Hanger";
	case EXI_IS_VIEWER:
		return "IS-DOL-VIEWER";
	default:
		return NULL;
	}
}

#pragma scheduling reset
