#ifndef REVOLUTION_DVD_BROADWAY_H
#define REVOLUTION_DVD_BROADWAY_H
#include "types.h"
#include "Dolphin/dvd.h"
#include "revolution/esp.h"
#include "Dolphin/OS/OSAlarm.h"

BEGIN_SCOPE_EXTERN_C

// result bits a DVDLowCallback receives
#define DVD_INTTYPE_TC  1
#define DVD_INTTYPE_DE  2
#define DVD_INTTYPE_CVR 4
#define DVD_INTTYPE_BR  8

BOOL DVDLowInit(void);
BOOL DVDLowFinish(void);
BOOL DVDLowReadDiskID(DVDDiskID* diskID, DVDLowCallback callback);
BOOL DVDLowOpenPartition(u32 offset, const ESTicket* eTicket, u32 numCerts, const u8* certificates, ESTitleMeta* tmd, DVDLowCallback callback);
BOOL DVDLowOpenPartitionWithTmdAndTicketView(u32 offset, const ESTicketView* eTicketView, u32 numTmdBytes, const ESTitleMeta* tmd, u32 numCerts,
                                             const u8* certificates, DVDLowCallback callback);
BOOL DVDLowGetNoDiscBufferSizes(u32 offset, u32* numTmdBytes, u32* certificatesSize, DVDLowCallback callback);
BOOL DVDLowGetNoDiscOpenPartitionParams(u32 offset, ESTicket* eTicket, u32* numTmdBytes, ESTitleMeta* tmd, u32* certificatesSize,
                                        u8* certificates, u32* dataWordOffset, u8* h3Hash, DVDLowCallback callback);
BOOL DVDLowClosePartition(DVDLowCallback callback);
BOOL DVDLowUnencryptedRead(void* destAddr, u32 length, u32 offset, DVDLowCallback callback);
BOOL DVDLowStopMotor(BOOL eject, BOOL saving, DVDLowCallback callback);
BOOL DVDLowInquiry(void* info, DVDLowCallback callback);
BOOL DVDLowRequestError(DVDLowCallback callback);
BOOL DVDLowSetSpinupFlag(u32 spinUp);
BOOL DVDLowReset(DVDLowCallback callback);
BOOL DVDLowAudioBufferConfig(BOOL enable, u32 size, DVDLowCallback callback);
BOOL DVDLowReportKey(void* keyData, u32 format, u32 lsn, DVDLowCallback callback);
BOOL DVDLowSetMaxRotation(u32 subcmd, DVDLowCallback callback);
BOOL DVDLowRead(void* destAddr, u32 length, u32 offset, DVDLowCallback callback);
BOOL DVDLowSeek(u32 offset, DVDLowCallback callback);
u32 DVDLowGetCoverRegister(void);
u32 DVDLowGetStatusRegister(void);
u32 DVDLowGetControlRegister(void);
BOOL DVDLowPrepareCoverRegister(DVDLowCallback callback);
BOOL DVDLowPrepareStatusRegister(DVDLowCallback callback);
BOOL DVDLowPrepareControlRegister(DVDLowCallback callback);
u32 DVDLowGetImmBufferReg(void);
BOOL DVDLowUnmaskStatusInterrupts(void);
BOOL DVDLowMaskCoverInterrupt(void);
BOOL DVDLowClearCoverInterrupt(DVDLowCallback callback);
BOOL __DVDLowTestAlarm(const OSAlarm* alarm);

END_SCOPE_EXTERN_C

#endif
