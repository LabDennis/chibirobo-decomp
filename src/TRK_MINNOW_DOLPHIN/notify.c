#include "types.h"
#include "PowerPC_EABI_Support/MetroTRK/trk.h"

/**
 * @note Address: 0x8014D48C
 * @note Size: 0x98
 */
DSError TRKDoNotifyStopped(MessageCommandID cmd)
{
	int reqIdx;
	int bufIdx;
	MessageBuffer* msg;
	DSError err;
	DSError bufError;

	bufError = TRKGetFreeBuffer(&bufIdx, &msg);
	if ((err = bufError) == FALSE) {
		if (err == DS_NoError) {
			if (cmd == DSMSG_NotifyStopped) {
				TRKTargetAddStopInfo(msg);
			} else {
				TRKTargetAddExceptionInfo(msg);
			}
		}
		bufError = TRKRequestSend(msg, &reqIdx, 2, 3, 1);
		err      = bufError;
		if (err == DS_NoError) {
			TRKReleaseBuffer(reqIdx);
		}
		TRKReleaseBuffer(bufIdx);
	}
	return err;
}
