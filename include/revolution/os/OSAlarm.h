#ifndef _REVOLUTION_OS_OSALARM_H
#define _REVOLUTION_OS_OSALARM_H

// Wii variant of OSAlarm.h: the Revolution SDK alarm carries a userData field.
// It also defines the Dolphin and sdk guard macros so the GameCube variants are
// skipped when this header is included first in a Wii unit.
#ifndef _DOLPHIN_OS_OSALARM_H
#define _DOLPHIN_OS_OSALARM_H
#endif
#ifndef _DOLPHIN_OSALARM_H_
#define _DOLPHIN_OSALARM_H_
#endif

#include <dolphin/os.h>

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

/////////// ALARM TYPES //////////
typedef struct OSAlarm OSAlarm;

// Generic alarm handler function.
typedef void (*OSAlarmHandler)(OSAlarm* alarm, OSContext* context);

// Struct for storing alarm information (size 0x30).
struct OSAlarm {
	OSAlarmHandler handler; // _00
	u32 tag;                // _04
	OSTime fire;            // _08
	OSAlarm* prev;          // _10
	OSAlarm* next;          // _14
	OSTime period;          // _18, period of periodic alarm
	OSTime start;           // _20, start of periodic alarm
	void* userData;         // _28
};

// Alarm functions.
void OSInitAlarm();
void OSSetAlarm(OSAlarm* alarm, OSTime tick, OSAlarmHandler handler);
void OSCreateAlarm(OSAlarm* alarm);
void OSCancelAlarm(OSAlarm* alarm);

// Unused/inlined in P2.
BOOL OSCheckAlarmQueue();
void OSSetAbsAlarm(OSAlarm* alarm, OSTime time, OSAlarmHandler handler);
void OSSetPeriodicAlarm(OSAlarm* alarm, OSTime start, OSTime period, OSAlarmHandler handler);
void OSSetAlarmTag(OSAlarm* alarm, u32 tag);
void OSCancelAlarms(u32 tag);
void OSSetAlarmUserData(OSAlarm* alarm, void* userData);
void* OSGetAlarmUserData(const OSAlarm* alarm);

//////////////////////////////////

#ifdef __cplusplus
};
#endif // ifdef __cplusplus

#endif
