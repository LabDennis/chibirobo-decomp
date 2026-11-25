#include "Dolphin/os.h"
#include "PowerPC_EABI_Support/MetroTRK/trk.h"

DSIOResult __read_file(u32 handle, u8* buffer, size_t* count, void* ref_con);
DSIOResult __write_file(u32 handle, u8* buffer, size_t* count, void* ref_con);
DSIOResult __close_file(u32 handle, u8* buffer, size_t* count, void* ref_con);
DSIOResult __access_file(u32 handle, u8* buffer, size_t* count, void* ref_con, MessageCommandID cmd);

/**
 * @note Address: 0x801502AC
 * @note Size: 0xBC
 */
DSIOResult __read_console(u32 handle, u8* buffer, size_t* count, void* ref_con)
{
	if (GetUseSerialIO() == 0) {
		return DS_IOError;
	}
	return __read_file(DS_Stdin, buffer, count, ref_con);
}

/**
 * @note Address: 0x801501F0
 * @note Size: 0xBC
 */
DSIOResult __TRK_write_console(u32 handle, u8* buffer, size_t* count, void* ref_con)
{
	if (GetUseSerialIO() == 0) {
		return DS_IOError;
	}
	return __write_file(DS_Stdout, buffer, count, ref_con);
}

/**
 * @note Address: 0x8015016C
 * @note Size: 0x84
 */
DSIOResult __close_console(u32 handle, u8* buffer, size_t* count, void* ref_con)
{
	return __close_file(handle, buffer, count, ref_con);
}

/**
 * @note Address: N/A
 * @note Size: 0xB4
 */
DSIOResult __read_file(u32 handle, u8* buffer, size_t* count, void* ref_con)
{
	return __access_file(handle, buffer, count, ref_con, DSMSG_ReadFile);
}

/**
 * @note Address: N/A
 * @note Size: 0xB4
 */
DSIOResult __write_file(u32 handle, u8* buffer, size_t* count, void* ref_con)
{
	return __access_file(handle, buffer, count, ref_con, DSMSG_WriteFile);
}

/**
 * @note Address: N/A
 * @note Size: 0x84
 */
DSIOResult __close_file(u32 handle, u8* buffer, size_t* count, void* ref_con)
{
	u32 result;

	if (!GetTRKConnected()) {
		return DS_IOError;
	}

	result = TRKCloseFile(DSMSG_CloseFile, handle);

	switch ((u8)result) {
	case DS_IONoError:
		return DS_IONoError;
	case DS_IOEOF:
		return DS_IOEOF;
	}

	return DS_IOError;
}

/**
 * @note Address: N/A
 * @note Size: 0xC0
 */
DSIOResult __access_file(u32 handle, u8* buffer, size_t* count, void* ref_con, MessageCommandID cmd)
{
	size_t countTemp;
	u32 result;

	if (!GetTRKConnected()) {
		return DS_IOError;
	}

	countTemp = *count;
	result    = TRKAccessFile(cmd, handle, &countTemp, buffer);
	*count    = countTemp;

	switch ((u8)result) {
	case DS_IONoError:
		return DS_IONoError;
	case DS_IOEOF:
		return DS_IOEOF;
	}

	return DS_IOError;
}
