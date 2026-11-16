#ifndef _REVOSDK_OS_API_H
#define _REVOSDK_OS_API_H

// OS names NW4R uses that revolution/os/*.h and Dolphin/os.h do not declare.

#include "revolution/os/OSAlarm.h"
#include "Dolphin/os.h"

#ifdef __cplusplus
extern "C" {
#endif

void OSSetAlarmUserData(OSAlarm* alarm, void* userData);
void* OSGetAlarmUserData(const OSAlarm* alarm);

#ifdef __cplusplus
}
#endif

#endif
