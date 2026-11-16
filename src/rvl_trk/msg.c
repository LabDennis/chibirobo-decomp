#include "PowerPC_EABI_Support/MetroTRK/trk.h"

DSError TRKMessageSend(MessageBuffer* msg)
{
	DSError writeErr = TRKWriteUARTN(&msg->data, msg->length);
	return DS_NoError;
}
