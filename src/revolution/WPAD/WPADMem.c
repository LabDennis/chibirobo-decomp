#include "WPADInternal.h"
#include "Dolphin/OS/OSInterrupt.h"

void WPADiClearMemBlock(WPADChannel channel)
{
	WPADMEMControlBlock* mem = &_wpdcb[channel]->mem;
	mem->used                = FALSE;
	mem->p_buf               = NULL;
	mem->len                 = 0;
	mem->addr                = 0;
	mem->callback            = NULL;
}

s32 WPADWriteExtReg(WPADChannel channel, const void* data, u16 length, u16 address, WPADCallback callback)
{
	WPADControlBlock* control = _wpdcb[channel];
	BOOL queued;
	BOOL interrupts;
	BOOL ready;
	s32 result;

	interrupts = OSDisableInterrupts();
	result     = control->status;
	ready      = control->setup;
	OSRestoreInterrupts(interrupts);

	if (result != WPAD_ERR_NO_CONTROLLER) {
		if (!ready) {
			result = WPAD_ERR_BUSY;
		} else {
			queued = WPADiSendWriteData(&control->cmdq, data, length, 0x04A40000 | address, callback);
			result = queued ? WPAD_ERR_NONE : WPAD_ERR_BUSY;
		}
	}
	if (result != WPAD_ERR_NONE) {
		if (callback != NULL) {
			callback(channel, result);
		}
	}
	return result;
}
