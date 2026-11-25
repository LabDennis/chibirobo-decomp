#include "Dolphin/os.h"
#include "Dolphin/hw_regs.h"
#include <mem.h>

/* The bootstrap is stored as 16-bit DSP instruction words. */
static u16 DSPInitCode[] = {
	0x029F, 0x0010, 0x029F, 0x0033, 0x029F, 0x0034, 0x029F, 0x0035,
	0x029F, 0x0036, 0x029F, 0x0037, 0x029F, 0x0038, 0x029F, 0x0039,
	0x1206, 0x1203, 0x1204, 0x1205, 0x0080, 0x8000, 0x0088, 0xFFFF,
	0x0084, 0x1000, 0x0064, 0x001D, 0x0218, 0x0000, 0x8100, 0x1C1E,
	0x0044, 0x1B1E, 0x0084, 0x0800, 0x0064, 0x0027, 0x191E, 0x0000,
	0x00DE, 0xFFFC, 0x02A0, 0x8000, 0x029C, 0x0028, 0x16FC, 0x0054,
	0x16FD, 0x4348, 0x0021, 0x02FF, 0x02FF, 0x02FF, 0x02FF, 0x02FF,
	0x02FF, 0x02FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

#define DSP_BOOTSTRAP_ADDRESS 0x01000000
#define DSP_BOOTSTRAP_BUFFER OSPhysicalToCached(DSP_BOOTSTRAP_ADDRESS)
#define DSP_BOOTSTRAP_ARAM_SIZE 0x43
#define DSP_BOOTSTRAP_DELAY_TICKS 0x892
#define DSP_STOP_DELAY_TICKS 0x2C

#define DSP_CONTROL_BOOT_HALTED (DSP_CONTROL_INIT | DSP_CONTROL_HALT | DSP_STATUS_INTERRUPTS)
#define DSP_MAILBOX_FULL ((u32)DSP_MAILBOX_FULL_HI << 16)

static inline void WaitForDSPDelay(s32 ticks)
{
	OSTick start = OSGetTick();

	while ((s32)(OSGetTick() - start) < ticks) {
	}
}

static inline void WaitForDSPControlClear(u16 mask)
{
	u16 status = __DSPRegs[DSP_CONTROL_STATUS];
	while (status & mask) {
		status = __DSPRegs[DSP_CONTROL_STATUS];
	}
}

static inline void ResetHaltedDSP(void)
{
	__DSPRegs[DSP_CONTROL_STATUS] = DSP_CONTROL_BOOT_HALTED;
	__DSPRegs[DSP_CONTROL_STATUS] |= DSP_CONTROL_RESET;
	while ((__DSPRegs[DSP_CONTROL_STATUS] & DSP_CONTROL_RESET) != 0) {
	}
}

static inline void TransferDSPBootstrap(void)
{
	u16 status;

	// Each DMA parameter uses a single 32-bit write to its register pair.
	__DSPRegs32[DSP_ARAM_DMA_MM_WORD] = DSP_BOOTSTRAP_ADDRESS;
	__DSPRegs32[DSP_ARAM_DMA_ARAM_WORD] = 0;
	__DSPRegs32[DSP_ARAM_DMA_SIZE_WORD] = DSP_DMA_BLOCK_SIZE;

	status = __DSPRegs[DSP_CONTROL_STATUS];
	while (!(status & DSP_STATUS_ARAM_INTERRUPT)) {
		status = __DSPRegs[DSP_CONTROL_STATUS];
	}
	__DSPRegs[DSP_CONTROL_STATUS] = status;
}

/**
 * @note Address: 0x80162CD4
 * @note Size: 0x1BC
 */
void __OSInitAudioSystem(void)
{
	u32 mail;
	u16 status;
	u16 mailHigh;

	// Save the main memory overwritten by the bootstrap.
	memcpy((u8*)OSGetArenaHi() - sizeof(DSPInitCode), DSP_BOOTSTRAP_BUFFER, sizeof(DSPInitCode));
	memcpy(DSP_BOOTSTRAP_BUFFER, DSPInitCode, sizeof(DSPInitCode));

	DCFlushRange(DSP_BOOTSTRAP_BUFFER, sizeof(DSPInitCode));

	__DSPRegs[DSP_ARAM_SIZE] = DSP_BOOTSTRAP_ARAM_SIZE;
	ResetHaltedDSP();
	__DSPRegs[DSP_MAILBOX_IN_HI] = 0;
	// Reading the low half clears the full flag sampled in the high half.
	do {
		mail = (u32)__DSPRegs[DSP_MAILBOX_OUT_HI] << 16;
		mail |= __DSPRegs[DSP_MAILBOX_OUT_LO];
	} while (mail & DSP_MAILBOX_FULL);
	TransferDSPBootstrap();

	WaitForDSPDelay(DSP_BOOTSTRAP_DELAY_TICKS);

	TransferDSPBootstrap();

	__DSPRegs[DSP_CONTROL_STATUS] &= ~DSP_CONTROL_INIT;
	while (__DSPRegs[DSP_CONTROL_STATUS] & DSP_STATUS_INIT_CODE)
		;
	__DSPRegs[DSP_CONTROL_STATUS] &= ~DSP_CONTROL_HALT;

	mailHigh = __DSPRegs[DSP_MAILBOX_OUT_HI];
	while (!(mailHigh & DSP_MAILBOX_FULL_HI)) {
		mailHigh = __DSPRegs[DSP_MAILBOX_OUT_HI];
	}

	// Sample the control status before consuming the reply.
	status = __DSPRegs[DSP_CONTROL_STATUS];
	(void)__DSPRegs[DSP_MAILBOX_OUT_LO];
	__DSPRegs[DSP_CONTROL_STATUS] = status | DSP_CONTROL_HALT;
	ResetHaltedDSP();
	memcpy(DSP_BOOTSTRAP_BUFFER, (u8*)OSGetArenaHi() - sizeof(DSPInitCode), sizeof(DSPInitCode));
}

/**
 * @note Address: 0x80162E90
 * @note Size: 0xD8
 */
void __OSStopAudioSystem(void)
{
	u32 mail;
	u16 dmaControl;

	__DSPRegs[DSP_CONTROL_STATUS] = DSP_CONTROL_INIT | DSP_CONTROL_HALT;
	dmaControl = __DSPRegs[DSP_DMA_CONTROL_LEN];
	__DSPRegs[DSP_DMA_CONTROL_LEN] = dmaControl & ~DSP_DMA_START_FLAG;
	WaitForDSPControlClear(DSP_STATUS_INIT_CODE);
	WaitForDSPControlClear(DSP_STATUS_DMA_BUSY);
	__DSPRegs[DSP_CONTROL_STATUS] = DSP_CONTROL_BOOT_HALTED;
	__DSPRegs[DSP_MAILBOX_IN_HI] = 0;

	do {
		mail = (u32)__DSPRegs[DSP_MAILBOX_OUT_HI] << 16;
		mail |= __DSPRegs[DSP_MAILBOX_OUT_LO];
	} while (mail & DSP_MAILBOX_FULL);
	WaitForDSPDelay(DSP_STOP_DELAY_TICKS);
	__DSPRegs[DSP_CONTROL_STATUS] |= DSP_CONTROL_RESET;
	WaitForDSPControlClear(DSP_CONTROL_RESET);
}
