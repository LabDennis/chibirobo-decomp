#include "Dolphin/os.h"
#include "PowerPC_EABI_Support/MetroTRK/trk.h"

/**
 * @note Address: 0x8014BAF0
 * @note Size: 0x88
 */
BOOL usr_puts_serial(const char* msg)
{
	for (; *msg != '\0'; msg++) {
		BOOL connected = GetTRKConnected();

		// Route this character through the local console.
		SetTRKConnected(FALSE);
		OSReport("%c", (unsigned char)*msg);
		SetTRKConnected(connected);
	}
	return FALSE;
}

/**
 * @note Address: 0x8014BAEC
 * @note Size: 0x4
 */
void usr_put_initialize(void)
{
}
