#include "PowerPC_EABI_Support/MetroTRK/trk.h"
#include "Dolphin/os.h"

// TRKLoadContext uses the same instruction stream as the GameCube TRK unit;
// it is assembled in TRK_MINNOW_DOLPHIN/trk_context_load.s.
// pik2wii: https://github.com/projectPiki/pik2wii/blob/main/src/RevoSDK/TRK_Hollywood_Revolution/dolphin_trk_glue.c
BOOL _MetroTRK_Has_Framing; // UNUSED
// GPVE01     _MetroTRK_Has_Framing = .bss:0x804F4814; // type:object size:0x4 scope:global align:4
// GPVE01_D17 _MetroTRK_Has_Framing = .bss:0x804F4754; // type:object size:0x4 scope:global align:4
u8 TRK_Use_BBA;

DBCommTable gDBCommTable = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

void TRKEXICallBack(s16 r3, OSContext* ctx)
{
	OSEnableScheduler();
	TRKLoadContext(ctx, 0x500);
}

int InitMetroTRKCommTable(int hwId)
{
	int result = 1;
	OSReport("Devkit set to : %ld\n", hwId);
	OSReport("MetroTRK : Sizeof Reply - %ld bytes\n", sizeof(CommandReply));
	TRK_Use_BBA = 0;

	if (hwId == HARDWARE_BBA) { // BBA hardware
		OSReport("MetroTRK : Set to BBA\n");
		// Initialize gDBCommTable
		TRK_Use_BBA                       = 1;
		gDBCommTable.initialize_func      = udp_cc_initialize;
		gDBCommTable.open_func            = udp_cc_open;
		gDBCommTable.close_func           = udp_cc_close;
		gDBCommTable.read_func            = udp_cc_read;
		gDBCommTable.write_func           = udp_cc_write;
		gDBCommTable.shutdown_func        = udp_cc_shutdown;
		gDBCommTable.peek_func            = udp_cc_peek;
		gDBCommTable.pre_continue_func    = udp_cc_pre_continue;
		gDBCommTable.post_stop_func       = udp_cc_post_stop;
		gDBCommTable.init_interrupts_func = nullptr;
		return 0;
	} else if (hwId == HARDWARE_GDEV) { // NDEV hardware
		OSReport("MetroTRK : Set to NDEV hardware\n");
		// Initialize gDBCommTable
		gDBCommTable.initialize_func      = gdev_cc_initialize;
		gDBCommTable.open_func            = gdev_cc_open;
		gDBCommTable.close_func           = gdev_cc_close;
		gDBCommTable.read_func            = gdev_cc_read;
		gDBCommTable.write_func           = gdev_cc_write;
		gDBCommTable.shutdown_func        = gdev_cc_shutdown;
		gDBCommTable.peek_func            = gdev_cc_peek;
		gDBCommTable.pre_continue_func    = gdev_cc_pre_continue;
		gDBCommTable.post_stop_func       = gdev_cc_post_stop;
		gDBCommTable.init_interrupts_func = gdev_cc_initinterrupts;
		return 0;
	} else { // unknown hardware
		OSReport("MetroTRK : Set to UNKNOWN hardware. (%ld)\n", hwId);
		OSReport("MetroTRK : Invalid hardware ID passed from OS\n");
		OSReport("MetroTRK : Defaulting to GDEV Hardware\n");
		return 1;
	}
}

DSError TRKInitializeIntDrivenUART(u32, u32, u32, void* r5)
{
	gDBCommTable.initialize_func(r5, TRKEXICallBack);
	gDBCommTable.open_func();
	return DS_NoError;
}

void EnableEXI2Interrupts(void)
{
	if (!TRK_Use_BBA) {
		if (gDBCommTable.init_interrupts_func != nullptr) {
			gDBCommTable.init_interrupts_func();
		}
	}
}

int TRKPollUART(void)
{
	return gDBCommTable.peek_func();
}

UARTError TRKReadUARTN(void* bytes, u32 length)
{
	int readErr = gDBCommTable.read_func(bytes, length);
	return ((-readErr | readErr) >> 31);
}

UARTError TRKWriteUARTN(const void* bytes, u32 length)
{
	int writeErr = gDBCommTable.write_func(bytes, length);
	return ((-writeErr | writeErr) >> 31);
}

void WriteUARTFlush(void)
{
	// UNUSED FUNCTION
}

void WriteUART1(void)
{
	// UNUSED FUNCTION
}

void TRKReadUARTPoll(void)
{
	// UNUSED FUNCTION
}

void ReserveEXI2Port(void)
{
	gDBCommTable.post_stop_func();
}

void UnreserveEXI2Port(void)
{
	gDBCommTable.pre_continue_func();
}

// NONMATCHING: same instructions and rodata bytes. Retail exposes the adjacent format strings
// as one 260-byte object, while MWCC emits "%s\n" as an object of its own; the strict
// data-value relocation comparison deducts the two address instructions. Pooling the strings is
// not the retail form: it packs out the alignment zeroes between them.
void TRK_board_display(char* str)
{
	OSReport("%s\n", str);
}

DSError InitializeProgramEndTrap(void)
{
	static const u32 EndofProgramInstruction = 'END';

	u8* endOfProgramInstructionBytes = (u8*)&EndofProgramInstruction;
	u8* ppcHaltPtr                   = (u8*)PPCHalt;
	TRK_memcpy(ppcHaltPtr + 4, endOfProgramInstructionBytes, 4);
	ICInvalidateRange(ppcHaltPtr + 4, 4);
	DCFlushRange(ppcHaltPtr + 4, 4);
}

void TRKUARTInterruptHandler(void)
{
}
