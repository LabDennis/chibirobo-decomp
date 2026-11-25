#include "Dolphin/__start.h"
#include "PowerPC_EABI_Support/MetroTRK/trkenum.h"
#include "PowerPC_EABI_Support/Runtime/__mem.h"

#pragma section code_type ".init"

extern void __OSInitBootDebugger(HardwareType hardware);
extern void __flush_cache(void* address, unsigned int size);

/**
 * @note Address: 0x800051EC
 * @note Size: 0x40
 */
void __check_pad3(void)
{
	if ((Pad3Button & 0x0eef) == 0x0eef) {
		OSResetSystem(OS_RESET_RESTART, 0, FALSE);
	}
	return;
}

/**
 * @note Address: 0x8000522C
 * @note Size: 0xC
 */
void __set_debug_bba(void)
{
	Debug_BBA = 1;
}

/**
 * @note Address: 0x80005238
 * @note Size: 0x8
 */
u8 __get_debug_bba(void)
{
	return Debug_BBA;
}

enum {
	BOOT_DEBUG_AMC_DDH = 2,
	BOOT_DEBUG_GDEV = 3,
	BOOT_DEBUG_BBA = 4
};

static void ConfigureBootDebugger(void)
{
	BI2Debug* bootInfo = *(BI2Debug* volatile*)BOOTINFO2_ADDR;
	u32 debugFlag;

	if (bootInfo != NULL) {
		debugFlag = bootInfo->debugFlag;
	} else if (*(void* volatile*)ARENAHI_ADDR != NULL) {
		debugFlag = *(vu32*)DEBUGFLAG_ADDR;
	} else {
		return;
	}

	switch (debugFlag) {
	case BOOT_DEBUG_AMC_DDH:
		__OSInitBootDebugger(HARDWARE_AMC_DDH);
		break;
	case BOOT_DEBUG_GDEV:
		__OSInitBootDebugger(HARDWARE_GDEV);
		break;
	case BOOT_DEBUG_BBA:
		__set_debug_bba();
		break;
	}
}

static u32 ReadBootArguments(char*** argv)
{
	BI2Debug* bootInfo = *(BI2Debug* volatile*)BOOTINFO2_ADDR;
	u32 argOffset;
	u32* arguments;
	u32 argc;
	u32 i;

	*argv = NULL;
	if (bootInfo == NULL) {
		return 0;
	}

	argOffset = bootInfo->argOffset;
	if (argOffset == 0) {
		return 0;
	}

	arguments = (u32*)((u8*)bootInfo + argOffset);
	argc = *arguments++;
	if (argc == 0) {
		return 0;
	}

	// The boot loader stores each argument as an offset from the BI2 header.
	for (i = 0; i < argc; i++) {
		arguments[i] += (u32)bootInfo;
	}

	*argv = (char**)arguments;
	*(void* volatile*)ARENAHI_ADDR = (void*)OSRoundDown32B(arguments);
	return argc;
}

void __OSStartMain(void)
{
	char** argv;
	u32 argc;
	u16 deviceCode;

	*(vu32*)EXCEPTIONMASK_ADDR = 0;
	ConfigureBootDebugger();
	argc = ReadBootArguments(&argv);

	DBInit();
	OSInit();

	deviceCode = *(vu16*)DVD_DEVICECODE_ADDR;
	if ((deviceCode & 0x8000) == 0 || (deviceCode & 0x7fff) == 1) {
		__check_pad3();
	}
	if (__get_debug_bba() == 1) {
		InitMetroTRK_BBA();
	}

	__init_user();
	exit(main((int)argc, argv));
}

DECL_SECT(".init") extern __rom_copy_info _rom_copy_info[];
DECL_SECT(".init") extern __bss_init_info _bss_init_info[];

inline static void __copy_rom_section(void* dst, const void* src, u32 size)
{
	if (size && (dst != src)) {
		memcpy(dst, src, size);
		__flush_cache(dst, size);
	}
}

inline static void __init_bss_section(void* dst, u32 size)
{
	if (size) {
		memset(dst, 0, size);
	}
}

/**
 * @note Address: 0x8000542C
 * @note Size: 0xC0
 */
void __init_data(void)
{
	__rom_copy_info* dci;
	__bss_init_info* bii;

	dci = _rom_copy_info;
	while (TRUE) {
		if (dci->size == 0)
			break;
		__copy_rom_section(dci->addr, dci->rom, dci->size);
		dci++;
	}

	bii = _bss_init_info;
	while (TRUE) {
		if (bii->size == 0)
			break;
		__init_bss_section(bii->addr, bii->size);
		bii++;
	}
}
