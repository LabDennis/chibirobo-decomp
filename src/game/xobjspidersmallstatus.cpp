#include "game/xobjspidersmall.h"
#include "PowerPC_EABI_Support/Runtime/MWCPlusLib.h"

// what the compiler made for gSmallSpiderStatusByStage, CStatus[39][16]

extern "C" {
extern void* __vt__7CStatus[];
}

/**
 * @note Address: 0x8010011C
 * @note Size: 0x38
 */
// todo: weak copy
extern "C" void DestroySmallSpiderStatusTable()
{
	__destroy_arr(gSmallSpiderStatusByStage, (ConstructorDestructor*)__dt__22SmallSpiderStageStatusFv, sizeof(gSmallSpiderStatusByStage[0]),
	              39);
}

/**
 * @note Address: 0x80100154
 * @note Size: 0x60
 */
// todo: weak copy
extern "C" CStatus* __dt__22SmallSpiderStageStatusFv(CStatus* row, s32 flags)
{
	if (row != NULL) {
		__destroy_arr(row, (ConstructorDestructor*)DestroySmallSpiderStatusEntry, sizeof(CStatus), 16);
		if ((s16)flags > 0) {
			::operator delete(row);
		}
	}
	return row;
}

/**
 * @note Address: 0x801001B4
 * @note Size: 0x50
 */
// todo: weak copy
extern "C" CStatus* DestroySmallSpiderStatusEntry(CStatus* status, s32 flags)
{
	if (status != NULL) {
		status->CStatus::~CStatus();
		if ((s16)flags > 0) {
			::operator delete(status);
		}
	}
	return status;
}

/**
 * @note Address: 0x80100204
 * @note Size: 0x48
 */
// todo: weak copy
extern "C" CStatus* __ct__22SmallSpiderStageStatusFv(CStatus* row)
{
	__construct_array(row, (ConstructorDestructor)__ct__7CStatusFv, (ConstructorDestructor)DestroySmallSpiderStatusEntry, sizeof(CStatus),
	                  16);
	return row;
}

/**
 * @note Address: 0x8010024C
 * @note Size: 0x18
 */
// todo: weak copy
extern "C" CStatus* __ct__7CStatusFv(CStatus* status)
{
	*(void***)((u8*)status + 0x4) = __vt__7CStatus;
	status->mFlags                = 0;
	return status;
}
