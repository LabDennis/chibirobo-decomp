#include "revolution/hw_regs.h"
#include "revolution/ipc.h"
#include "revolution/dvd/dvd_broadway.h"
#include "Dolphin/os.h"
#include "string.h"

#define DVD_LOW_CONTEXT_MAGIC 0xFEEBDAED
#define DVD_LOW_NUM_CONTEXTS  4
#define DVD_LOW_NUM_COMMANDS  4

// size of the H3 hash block IOS writes for the partition commands
#define DVD_LOW_H3_HASH_SIZE 0x18000

// commands of /dev/di
#define DVD_LOW_CMD_INQUIRY                    0x12
#define DVD_LOW_CMD_READ_DISK_ID               0x70
#define DVD_LOW_CMD_READ                       0x71
#define DVD_LOW_CMD_WAIT_FOR_COVER_CLOSE       0x79
#define DVD_LOW_CMD_GET_COVER_REGISTER         0x7A
#define DVD_LOW_CMD_NOTIFY_RESET               0x7E
#define DVD_LOW_CMD_READ_DVD_PHYSICAL          0x80
#define DVD_LOW_CMD_READ_DVD_COPYRIGHT         0x81
#define DVD_LOW_CMD_READ_DVD_DISC_KEY          0x82
#define DVD_LOW_CMD_CLEAR_COVER_INTERRUPT      0x86
#define DVD_LOW_CMD_GET_COVER_STATUS           0x88
#define DVD_LOW_CMD_RESET                      0x8A
#define DVD_LOW_CMD_OPEN_PARTITION             0x8B
#define DVD_LOW_CMD_CLOSE_PARTITION            0x8C
#define DVD_LOW_CMD_UNENCRYPTED_READ           0x8D
#define DVD_LOW_CMD_ENABLE_DVD_VIDEO           0x8E
#define DVD_LOW_CMD_NO_DISC_OPEN_PARTITION     0x8F
#define DVD_LOW_CMD_GET_NO_DISC_PARAMS         0x90
#define DVD_LOW_CMD_GET_NO_DISC_BUFFER_SIZES   0x92
#define DVD_LOW_CMD_OPEN_PARTITION_TICKET      0x93
#define DVD_LOW_CMD_OPEN_PARTITION_TICKET_VIEW 0x94
#define DVD_LOW_CMD_GET_STATUS_REGISTER        0x95
#define DVD_LOW_CMD_GET_CONTROL_REGISTER       0x96
#define DVD_LOW_CMD_REPORT_KEY                 0xA4
#define DVD_LOW_CMD_SEEK                       0xAB
#define DVD_LOW_CMD_READ_DVD                   0xD0
#define DVD_LOW_CMD_READ_DVD_CONFIG            0xD1
#define DVD_LOW_CMD_STOP_LASER                 0xD2
#define DVD_LOW_CMD_OFFSET                     0xD9
#define DVD_LOW_CMD_READ_DISK_BCA              0xDA
#define DVD_LOW_CMD_REQUEST_DISC_STATUS        0xDB
#define DVD_LOW_CMD_REQUEST_RETRY_NUMBER       0xDC
#define DVD_LOW_CMD_SET_MAX_ROTATION           0xDD
#define DVD_LOW_CMD_SER_MEAS_CONTROL           0xDF
#define DVD_LOW_CMD_REQUEST_ERROR              0xE0
#define DVD_LOW_CMD_STOP_MOTOR                 0xE3
#define DVD_LOW_CMD_AUDIO_BUFFER_CONFIG        0xE4

typedef struct DVDLowContext {
	DVDLowCallback callback; // _00
	u32 callbackType;        // _04
	u8 inUse;                // _08
	u32 magic;               // _0C
	u32 id;                  // _10
	u32 unused[3];           // _14
} DVDLowContext;

typedef struct DVDLowCommand {
	u8 theCommand; // _00
	u32 arg[7];    // _04
} DVDLowCommand;

static DVDLowContext dvdContexts[DVD_LOW_NUM_CONTEXTS] ATTRIBUTE_ALIGN(32);
// _00: the drive's immediate buffer as the last command returned it, _04: the cover register
static u32 diRegValCache[8] ATTRIBUTE_ALIGN(32);
static u32 coverRegister[8] ATTRIBUTE_ALIGN(32);
static IOSIoVector ioVec[10] ATTRIBUTE_ALIGN(32);
static u8 lastTicketError[32] ATTRIBUTE_ALIGN(32);
static u32 coverStatus[8] ATTRIBUTE_ALIGN(32);
static u32 coverRegisterSync[8] ATTRIBUTE_ALIGN(32);
static u32 statusRegister[8] ATTRIBUTE_ALIGN(32);
static u32 controlRegister[8] ATTRIBUTE_ALIGN(32);

static u8 breakRequested;
static DVDLowCommand* diCommand;
static char* pathBuf;
static u32 readLength;
static u32 spinUpValue;
static u8 DVDLowInitCalled;
static u8 dvdContextsInited;
static int freeDvdContext;
static int freeCommandBuf;
static u8 callbackInProgress;
static u8 requestInProgress;
static s32 DiFD = -1;

// retail emits this one on its own as well (the linker drops it): the contexts are the first thing in .bss,
// ahead of what the callbacks below use
void initDvdContexts(void)
{
	int i;

	for (i = 0; i < DVD_LOW_NUM_CONTEXTS; i++) {
		dvdContexts[i].callback     = NULL;
		dvdContexts[i].callbackType = 0;
		dvdContexts[i].inUse        = FALSE;
		dvdContexts[i].magic        = DVD_LOW_CONTEXT_MAGIC;
		dvdContexts[i].id           = i;
	}
	freeDvdContext = 0;
}

static inline DVDLowContext* newContext(DVDLowCallback callback, u32 callbackType)
{
	int id;

	requestInProgress = TRUE;

	if ((u8)(dvdContexts[freeDvdContext].inUse != FALSE) == TRUE) {
		OSReport("(newContext) ERROR: freeDvdContext.inUse (#%d) is true\n", freeDvdContext);
		OSReport("(newContext) Now spinning in infinite loop\n");
		while (TRUE) { }
	}

	if (dvdContexts[freeDvdContext].magic != DVD_LOW_CONTEXT_MAGIC) {
		OSReport("(newContext) Something overwrote the context magic - spinning \n");
		while (TRUE) { }
	}

	id                           = freeDvdContext;
	dvdContexts[id].callback     = callback;
	dvdContexts[id].callbackType = callbackType;
	dvdContexts[id].inUse        = TRUE;

	freeDvdContext++;
	if (freeDvdContext >= DVD_LOW_NUM_CONTEXTS) {
		freeDvdContext = 0;
	}

	return dvdContexts + id;
}

static void nextCommandBuf(int* bufNum)
{
	(*bufNum)++;
	if (*bufNum >= DVD_LOW_NUM_COMMANDS) {
		*bufNum = 0;
	}
}

static s32 doTransactionCallback(s32 result, void* usrData)
{
	DVDLowContext* ctx = (DVDLowContext*)usrData;
	u32 intType        = result;

	if (ctx->magic != DVD_LOW_CONTEXT_MAGIC) {
		OSReport("(doTransactionCallback) Error - context mangled!\n");
		ctx->magic = DVD_LOW_CONTEXT_MAGIC;
		goto out;
	}

	requestInProgress = FALSE;
	if (ctx->callback) {
		callbackInProgress = TRUE;
		if (breakRequested == TRUE) {
			breakRequested = FALSE;
			intType |= DVD_INTTYPE_BR;
		}
		if (intType & DVD_INTTYPE_TC) {
			readLength = 0;
		}
		ctx->callback(intType);
		callbackInProgress = FALSE;
	}

out:
	ctx->inUse = FALSE;
	return 0;
}

// not in the dol: only its message survives (0x8031E1EC). Body as in pik1wii's dvd_broadway.c, where it is
// unused as well and leaves the context pointer unset; it is set here the way doTransactionCallback does.
static s32 doCoverCallback(s32 result, void* usrData)
{
	DVDLowContext* ctx = (DVDLowContext*)usrData;

	requestInProgress = FALSE;

	if (ctx->magic != DVD_LOW_CONTEXT_MAGIC) {
		OSReport("(doCoverCallback) Error - context mangled!\n");
		ctx->magic = DVD_LOW_CONTEXT_MAGIC;
	} else {
		if (ctx->callback) {
			callbackInProgress = TRUE;

			if (breakRequested == TRUE) {
				breakRequested = FALSE;
				result |= DVD_INTTYPE_BR;
			}

			ctx->callback(result);
			callbackInProgress = FALSE;
		}
	}

	ctx->inUse = FALSE;
	return 0;
}

// 0x80196884, the callback of DVDLowPrepareCoverRegister. Retail reports a mangled context with
// doTransactionCallback's text (0x801968C4 loads the string at 0x8031E1B8).
static s32 doPrepareCoverRegisterCallback(s32 result, void* usrData)
{
	DVDLowContext* ctx = (DVDLowContext*)usrData;
	u32 intType        = result;

	requestInProgress = FALSE;
	diRegValCache[1]  = coverRegister[0];

	if (ctx->magic != DVD_LOW_CONTEXT_MAGIC) {
		OSReport("(doTransactionCallback) Error - context mangled!\n");
		ctx->magic = DVD_LOW_CONTEXT_MAGIC;
		goto out;
	}

	if (ctx->callback) {
		callbackInProgress = TRUE;
		if (breakRequested == TRUE) {
			breakRequested = FALSE;
			intType |= DVD_INTTYPE_BR;
		}
		ctx->callback(intType);
		callbackInProgress = FALSE;
	}

out:
	ctx->inUse = FALSE;
	return 0;
}

BOOL DVDLowFinish(void)
{
	if (IOS_Close(DiFD) != 0) {
		OSReport("(DVDLowFinish) Error: IOS_Close failed\n");
		return FALSE;
	}

	DVDLowInitCalled = FALSE;
	return TRUE;
}

static void* ddrAllocAligned32(const int size)
{
	u32 low  = (u32)IPCGetBufferLo();
	u32 high = (u32)IPCGetBufferHi();

	if (low & 31) {
		// as retail has it: this keeps the low five bits instead of clearing them
		low = (low + 31) & 31;
	}

	if (low + size > high) {
		OSReport("(ddrAllocAligned32) Not enough space to allocate %d bytes\n", size);
	}

	IPCSetBufferLo((void*)(low + size));
	return (void*)low;
}

static BOOL allocateStructures(void)
{
	diCommand = ddrAllocAligned32(DVD_LOW_NUM_COMMANDS * sizeof(DVDLowCommand));
	if (!diCommand) {
		OSReport("Allocation of diCommand blocks failed\n");
		return FALSE;
	}

	pathBuf = ddrAllocAligned32(32);
	if (!pathBuf) {
		OSReport("Allocation of pathBuf failed\n");
		return FALSE;
	}

	return TRUE;
}

BOOL DVDLowInit(void)
{
	s32 rv;

	if (!DVDLowInitCalled) {
		DVDLowInitCalled = TRUE;

		rv = IPCCltInit();
		if (rv != 0) {
			OSReport("IPCCltInit returned error: %d\n", rv);
			return FALSE;
		}

		if (!allocateStructures()) {
			return FALSE;
		}

		if (!dvdContextsInited) {
			initDvdContexts();
			dvdContextsInited = TRUE;
		}
	}

	strncpy(pathBuf, "/dev/di", 32);
	DiFD = IOS_Open(pathBuf, 0);
	if (DiFD >= 0) {
		return TRUE;
	}

	switch (DiFD) {
	case IPC_RESULT_NOENT:
		OSReport("(DVDLowInit) Error: IOS_Open failed - pathname '/dev/di' does not exist\n");
		return FALSE;
	case IPC_RESULT_ACCESS:
		OSReport("(DVDLowInit) Error: IOS_Open failed - calling thread lacks permission\n");
		return FALSE;
	case IPC_RESULT_CONN_MAX:
		OSReport("(DVDLowInit) Error: IOS_Open failed - connection limit has been reached\n");
		return FALSE;
	default:
		OSReport("(DVDLowInit) IOS_Open failed, errorcode = %d\n", DiFD);
		return FALSE;
	}
}

BOOL DVDLowReadDiskID(DVDDiskID* diskID, DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	if (!diskID) {
		OSReport("@@@@@@ WARNING - Calling DVDLowReadDiskId with NULL ptr\n");
	}

	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_READ_DISK_ID;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_READ_DISK_ID, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), diskID, sizeof(DVDDiskID),
	                    doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowReadDiskID) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

BOOL DVDLowOpenPartition(u32 offset, const ESTicket* eTicket, u32 numCerts, const u8* certificates, ESTitleMeta* tmd, DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	if (eTicket && ((u32)eTicket & 31)) {
		OSReport("(DVDLowOpenPartition) eTicket memory is unaligned\n");
		return FALSE;
	}

	if (certificates && ((u32)certificates & 31)) {
		OSReport("(DVDLowOpenPartition) certificates memory is unaligned\n");
		return FALSE;
	}

	if (tmd && ((u32)tmd & 31)) {
		OSReport("(DVDLowOpenPartition) certificates memory is unaligned\n");
		return FALSE;
	}

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_OPEN_PARTITION;
	diCommand[freeCommandBuf].arg[0]     = offset;

	ioVec[0].base   = &diCommand[freeCommandBuf];
	ioVec[0].length = sizeof(DVDLowCommand);

	ioVec[1].base = (u8*)eTicket;
	if (!eTicket) {
		ioVec[1].length = 0;
	} else {
		ioVec[1].length = sizeof(ESTicket);
	}

	ioVec[2].base = (u8*)certificates;
	if (!certificates) {
		ioVec[2].length = 0;
	} else {
		ioVec[2].length = numCerts;
	}

	ioVec[3].base   = (u8*)tmd;
	ioVec[3].length = sizeof(ESTitleMeta);

	ioVec[4].base   = lastTicketError;
	ioVec[4].length = sizeof(lastTicketError);

	rv = IOS_IoctlvAsync(DiFD, DVD_LOW_CMD_OPEN_PARTITION, 3, 2, ioVec, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowOpenPartition) IOS_IoctlvAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

// not in the dol: its name and three messages survive (0x8031E5C8-0x8031E67C), in this order of first use.
// Body as in pik1wii's dvd_broadway.c (unused there too), except for the command word and the ticket length,
// which pik1wii copies from the ticket view variant; those two are IOS's values, not read from the dol.
BOOL DVDLowOpenPartitionWithTmdAndTicket(u32 offset, const ESTicket* eTicket, u32 numTmdBytes, const ESTitleMeta* tmd, u32 numCerts,
                                         const u8* certificates, DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	if (eTicket && ((u32)eTicket & 31)) {
		OSReport("(%s) eTicket memory is unaligned\n", __FUNCTION__);
		return FALSE;
	}

	if (certificates && ((u32)certificates & 31)) {
		return FALSE;
	}

	if (!tmd) {
		OSReport("(%s) tmd parameter cannot be NULL\n", __FUNCTION__);
		return FALSE;
	} else if ((u32)tmd & 31) {
		OSReport("(%s) tmd memory is unaligned\n", __FUNCTION__);
		return FALSE;
	}

	if (!eTicket) {
		OSReport("(%s) eTicket parameter cannot be NULL\n", __FUNCTION__);
		return FALSE;
	} else if ((u32)eTicket & 31) {
		OSReport("(%s) eTicket memory is unaligned\n", __FUNCTION__);
		return FALSE;
	}

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_OPEN_PARTITION_TICKET;
	diCommand[freeCommandBuf].arg[0]     = offset;

	ioVec[0].base   = &diCommand[freeCommandBuf];
	ioVec[0].length = sizeof(DVDLowCommand);

	ioVec[1].base   = (u8*)eTicket;
	ioVec[1].length = sizeof(ESTicket);

	ioVec[2].base   = (u8*)tmd;
	ioVec[2].length = numTmdBytes;

	ioVec[3].base = (u8*)certificates;
	if (!certificates) {
		ioVec[3].length = 0;
	} else {
		ioVec[3].length = numCerts;
	}

	ioVec[4].base   = lastTicketError;
	ioVec[4].length = sizeof(lastTicketError);

	rv = IOS_IoctlvAsync(DiFD, DVD_LOW_CMD_OPEN_PARTITION_TICKET, 4, 1, ioVec, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowOpenPartition) IOS_IoctlvAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

BOOL DVDLowOpenPartitionWithTmdAndTicketView(u32 offset, const ESTicketView* eTicketView, u32 numTmdBytes, const ESTitleMeta* tmd, u32 numCerts,
                                             const u8* certificates, DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	if (certificates && ((u32)certificates & 31)) {
		return FALSE;
	}

	if (!tmd) {
		OSReport("(%s) tmd parameter cannot be NULL\n", __FUNCTION__);
		return FALSE;
	}

	if ((u32)tmd & 31) {
		OSReport("(%s) tmd memory is unaligned\n", __FUNCTION__);
		return FALSE;
	}

	if (!eTicketView) {
		OSReport("(%s) eTicketView parameter cannot be NULL\n", __FUNCTION__);
		return FALSE;
	}

	if ((u32)eTicketView & 31) {
		OSReport("(%s) eTicketView memory is unaligned\n", __FUNCTION__);
		return FALSE;
	}

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_OPEN_PARTITION_TICKET_VIEW;
	diCommand[freeCommandBuf].arg[0]     = offset;

	ioVec[0].base   = &diCommand[freeCommandBuf];
	ioVec[0].length = sizeof(DVDLowCommand);

	ioVec[1].base   = (u8*)eTicketView;
	ioVec[1].length = sizeof(ESTicketView);

	ioVec[2].base   = (u8*)tmd;
	ioVec[2].length = numTmdBytes;

	ioVec[3].base = (u8*)certificates;
	if (!certificates) {
		ioVec[3].length = 0;
	} else {
		ioVec[3].length = numCerts;
	}

	ioVec[4].base   = lastTicketError;
	ioVec[4].length = sizeof(lastTicketError);

	rv = IOS_IoctlvAsync(DiFD, DVD_LOW_CMD_OPEN_PARTITION_TICKET_VIEW, 4, 1, ioVec, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowOpenPartition) IOS_IoctlvAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

BOOL DVDLowGetNoDiscBufferSizes(u32 offset, u32* numTmdBytes, u32* certificatesSize, DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	if (!numTmdBytes || !certificatesSize) {
		OSReport("(%s) Error: NULL pointer argument\n", __FUNCTION__);
		return FALSE;
	}

	if ((u32)numTmdBytes & 31) {
		OSReport("(%s) numTmdBytes memory is unaligned\n", __FUNCTION__);
		return FALSE;
	}

	if ((u32)certificatesSize & 31) {
		OSReport("(%s) certificates memory is unaligned\n", __FUNCTION__);
		return FALSE;
	}

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_GET_NO_DISC_BUFFER_SIZES;
	diCommand[freeCommandBuf].arg[0]     = offset;

	ioVec[0].base   = &diCommand[freeCommandBuf];
	ioVec[0].length = sizeof(DVDLowCommand);

	ioVec[1].base   = numTmdBytes;
	ioVec[1].length = sizeof(u32);

	ioVec[2].base   = certificatesSize;
	ioVec[2].length = sizeof(u32);

	rv = IOS_IoctlvAsync(DiFD, DVD_LOW_CMD_GET_NO_DISC_BUFFER_SIZES, 1, 2, ioVec, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (%s) IOS_IoctlvAsync returned error: %d\n", __FUNCTION__, rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

BOOL DVDLowGetNoDiscOpenPartitionParams(u32 offset, ESTicket* eTicket, u32* numTmdBytes, ESTitleMeta* tmd, u32* certificatesSize,
                                        u8* certificates, u32* dataWordOffset, u8* h3Hash, DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	if (!eTicket || !numTmdBytes || !tmd || !certificatesSize || !certificates || !dataWordOffset || !h3Hash) {
		OSReport("(%s) Error: NULL pointer argument\n", __FUNCTION__);
		return FALSE;
	}

	if (((u32)eTicket & 31) || ((u32)numTmdBytes & 31) || ((u32)tmd & 31) || ((u32)certificatesSize & 31) || ((u32)certificates & 31)
	    || ((u32)dataWordOffset & 31) || ((u32)h3Hash & 31)) {
		OSReport("(%s) pointer argument is unaligned\n", __FUNCTION__);
		return FALSE;
	}

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_GET_NO_DISC_PARAMS;
	diCommand[freeCommandBuf].arg[0]     = offset;

	ioVec[0].base   = &diCommand[freeCommandBuf];
	ioVec[0].length = sizeof(DVDLowCommand);

	ioVec[1].base   = numTmdBytes;
	ioVec[1].length = sizeof(u32);

	ioVec[2].base   = certificatesSize;
	ioVec[2].length = sizeof(u32);

	ioVec[3].base   = (u8*)eTicket;
	ioVec[3].length = sizeof(ESTicket);

	ioVec[4].base   = numTmdBytes;
	ioVec[4].length = sizeof(u32);

	ioVec[5].base   = tmd;
	ioVec[5].length = *numTmdBytes;

	ioVec[6].base   = certificatesSize;
	ioVec[6].length = sizeof(u32);

	ioVec[7].base   = certificates;
	ioVec[7].length = *certificatesSize;

	ioVec[8].base   = dataWordOffset;
	ioVec[8].length = sizeof(u32);

	ioVec[9].base   = h3Hash;
	ioVec[9].length = DVD_LOW_H3_HASH_SIZE;

	rv = IOS_IoctlvAsync(DiFD, DVD_LOW_CMD_GET_NO_DISC_PARAMS, 3, 7, ioVec, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (%s) IOS_IoctlvAsync returned error: %d\n", __FUNCTION__, rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

// not in the dol: only its name survives (0x8031E804), as the __FUNCTION__ of a message it shares with the two
// functions above. Which of their messages it printed, its parameters and what it sent to IOS are not known,
// so only one such report stands here.
void DVDLowNoDiscOpenPartition(void)
{
	OSReport("(%s) Error: NULL pointer argument\n", __FUNCTION__);
}

BOOL DVDLowClosePartition(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_CLOSE_PARTITION;
	ctx                                  = newContext(callback, 1);

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_CLOSE_PARTITION, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0,
	                    doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowClosePartition) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

BOOL DVDLowUnencryptedRead(void* destAddr, u32 length, u32 offset, DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx        = newContext(callback, 1);
	readLength = length;
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_UNENCRYPTED_READ;
	diCommand[freeCommandBuf].arg[0]     = length;
	diCommand[freeCommandBuf].arg[1]     = offset;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_UNENCRYPTED_READ, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), destAddr, length,
	                    doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowUnencryptedRead) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

BOOL DVDLowStopMotor(BOOL eject, BOOL saving, DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_STOP_MOTOR;
	diCommand[freeCommandBuf].arg[0]     = eject;
	diCommand[freeCommandBuf].arg[1]     = saving;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_STOP_MOTOR, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), diRegValCache,
	                    sizeof(diRegValCache), doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowStopMotor) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

// not in the dol: only its message survives (0x8031E8DC). Body as in pik1wii's dvd_broadway.c (unused there
// too); the command word is IOS's, not read from the dol.
BOOL DVDLowWaitForCoverClose(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 2);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_WAIT_FOR_COVER_CLOSE;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_WAIT_FOR_COVER_CLOSE, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0, doCoverCallback,
	                    ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowWaitForCoverClose) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

BOOL DVDLowInquiry(void* info, DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_INQUIRY;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_INQUIRY, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), info, 0x20, doTransactionCallback,
	                    ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowInquiry) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

BOOL DVDLowRequestError(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_REQUEST_ERROR;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_REQUEST_ERROR, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), diRegValCache,
	                    sizeof(diRegValCache), doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowRequestError) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

// not in the dol: only its message survives (0x8031E994). The message names the function below, which retail
// keeps without this check, so it stood in a removed function whose own name and whose work are unknown.
// Only the check that the message belongs to is written (as in DVDLowGetCoverStatus).
BOOL DVDLowSetSpinupFlagChecked(void)
{
	if (callbackInProgress == TRUE) {
		OSReport("(DVDLowSetSpinupFlag): Synch functions can't be called in callbacks\n");
		return FALSE;
	}

	return TRUE;
}

BOOL DVDLowSetSpinupFlag(u32 spinUp)
{
	spinUpValue = spinUp;
	return TRUE;
}

// not in the dol: only its message survives (0x8031E9DC). Written as the plain request its neighbours send; its
// parameters and what it handed to IOS besides the command are not known and not written. The command word is IOS's.
BOOL DVDLowNotifyReset(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_NOTIFY_RESET;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_NOTIFY_RESET, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowNotifyReset) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

BOOL DVDLowReset(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_RESET;
	diCommand[freeCommandBuf].arg[0]     = spinUpValue;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_RESET, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowReset) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

BOOL DVDLowAudioBufferConfig(BOOL enable, u32 size, DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_AUDIO_BUFFER_CONFIG;
	diCommand[freeCommandBuf].arg[0]     = enable;
	diCommand[freeCommandBuf].arg[1]     = size;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_AUDIO_BUFFER_CONFIG, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), diRegValCache,
	                    sizeof(diRegValCache), doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowAudioBufferConfig) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

// not in the dol: its two messages (0x8031EA94, 0x8031EADC) and its 32-byte answer buffer in .bss survive.
// Body as in pik1wii's dvd_broadway.c (unused there too); the command word is IOS's.
u32 DVDLowGetCoverStatus(void)
{
	u32 status;
	s32 rv;

	if (callbackInProgress == TRUE) {
		OSReport("(DVDLowGetCoverStatus): Synch functions can't be called in callbacks\n");
		return 0;
	}

	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_GET_COVER_STATUS;

	rv = IOS_Ioctl(DiFD, DVD_LOW_CMD_GET_COVER_STATUS, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), coverStatus, sizeof(coverStatus));
	status = coverStatus[0];

	if (rv != 0) {
		OSReport("@@@ (DVDLowGetCoverStatus) IOS_Ioctl returned error: %d\n", rv);
		status = 0xDEADDEAD;
	}

	return status;
}

// not in the dol: only its message survives (0x8031EB18). Written as the plain request its neighbours send; its
// parameters and what it handed to IOS besides the command are not known and not written. The command word is IOS's.
BOOL DVDLowReadDVD(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_READ_DVD;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_READ_DVD, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowReadDVD) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

// not in the dol: only its message survives (0x8031EB50). Written as the plain request its neighbours send; its
// parameters and what it handed to IOS besides the command are not known and not written. The command word is IOS's.
BOOL DVDLowReadDVDConfig(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_READ_DVD_CONFIG;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_READ_DVD_CONFIG, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowReadDVDConfig) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

// not in the dol: only its message survives (0x8031EB90). Written as the plain request its neighbours send; its
// parameters and what it handed to IOS besides the command are not known and not written. The command word is IOS's.
BOOL DVDLowReadDvdCopyright(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_READ_DVD_COPYRIGHT;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_READ_DVD_COPYRIGHT, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowReadDvdCopyright) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

// not in the dol: only its message survives (0x8031EBD0). Written as the plain request its neighbours send; its
// parameters and what it handed to IOS besides the command are not known and not written. The command word is IOS's.
BOOL DVDLowReadDvdPhysical(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_READ_DVD_PHYSICAL;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_READ_DVD_PHYSICAL, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowReadDvdPhysical) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

// not in the dol: only its message survives (0x8031EC10). Written as the plain request its neighbours send; its
// parameters and what it handed to IOS besides the command are not known and not written. The command word is IOS's.
BOOL DVDLowReadDvdDiscKey(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_READ_DVD_DISC_KEY;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_READ_DVD_DISC_KEY, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowReadDvdDiscKey) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

BOOL DVDLowReportKey(void* keyData, u32 format, u32 lsn, DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_REPORT_KEY;
	diCommand[freeCommandBuf].arg[0]     = format >> 16;
	diCommand[freeCommandBuf].arg[1]     = lsn;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_REPORT_KEY, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), keyData, 0x20,
	                    doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowReportKey) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

// not in the dol: only its message survives (0x8031EC8C). Written as the plain request its neighbours send; its
// parameters and what it handed to IOS besides the command are not known and not written. The command word is IOS's.
BOOL DVDLowOffset(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_OFFSET;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_OFFSET, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowOffset) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

// not in the dol: only its message survives (0x8031ECC4). Written as the plain request its neighbours send; its
// parameters and what it handed to IOS besides the command are not known and not written. The command word is IOS's.
BOOL DVDLowStopLaser(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_STOP_LASER;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_STOP_LASER, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowStopLaser) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

// not in the dol: only its message survives (0x8031ED00). Written as the plain request its neighbours send; its
// parameters and what it handed to IOS besides the command are not known and not written. The command word is IOS's.
BOOL DVDLowReadDiskBca(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_READ_DISK_BCA;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_READ_DISK_BCA, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowReadDiskBca) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

// not in the dol: only its message survives (0x8031ED3C). Written as the plain request its neighbours send; its
// parameters and what it handed to IOS besides the command are not known and not written. The command word is IOS's.
BOOL DVDLowSerMeasControl(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_SER_MEAS_CONTROL;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_SER_MEAS_CONTROL, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowSerMeasControl) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

// not in the dol: only its message survives (0x8031ED7C). Written as the plain request its neighbours send; its
// parameters and what it handed to IOS besides the command are not known and not written. The command word is IOS's.
BOOL DVDLowRequestDiscStatus(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_REQUEST_DISC_STATUS;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_REQUEST_DISC_STATUS, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowRequestDiscStatus) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

// not in the dol: only its message survives (0x8031EDC0). Written as the plain request its neighbours send; its
// parameters and what it handed to IOS besides the command are not known and not written. The command word is IOS's.
BOOL DVDLowRequestRetryNumber(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_REQUEST_RETRY_NUMBER;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_REQUEST_RETRY_NUMBER, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowRequestRetryNumber) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

BOOL DVDLowSetMaxRotation(u32 subcmd, DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_SET_MAX_ROTATION;
	diCommand[freeCommandBuf].arg[0]     = (subcmd >> 16) & 3;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_SET_MAX_ROTATION, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0,
	                    doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowSetMaxRotation) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

BOOL DVDLowRead(void* destAddr, u32 length, u32 offset, DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	if ((u32)destAddr & 31) {
		OSReport("(DVDLowRead): ERROR - destAddr buffer is not 32 byte aligned\n");
		return FALSE;
	}

	ctx        = newContext(callback, 1);
	readLength = length;
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_READ;
	diCommand[freeCommandBuf].arg[0]     = length;
	diCommand[freeCommandBuf].arg[1]     = offset;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_READ, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), destAddr, length, doTransactionCallback,
	                    ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowRead) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

BOOL DVDLowSeek(u32 offset, DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_SEEK;
	diCommand[freeCommandBuf].arg[0]     = offset;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_SEEK, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowSeek) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

u32 DVDLowGetCoverRegister(void)
{
	return diRegValCache[1];
}

// not in the dol: its two messages (0x8031EEEC, 0x8031EF30), which give its name, and its 32-byte answer buffer
// in .bss survive. Body as in pik1wii's dvd_broadway.c (unused there too); the command word is IOS's.
u32 DVDLowGetCoverReg(void)
{
	u32 val;
	s32 rv;

	if (callbackInProgress == TRUE) {
		OSReport("(DVDLowGetCoverReg): Synch functions can't be called in callbacks\n");
		return 0;
	}

	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_GET_COVER_REGISTER;

	rv = IOS_Ioctl(DiFD, DVD_LOW_CMD_GET_COVER_REGISTER, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), coverRegisterSync,
	               sizeof(coverRegisterSync));
	val = coverRegisterSync[0];

	if (rv != 0) {
		OSReport("@@@ (DVDLowGetCoverReg) IOS_Ioctl returned error: %d\n", rv);
		val = 0;
	}

	return val;
}

u32 DVDLowGetStatusRegister(void)
{
	return statusRegister[0];
}

u32 DVDLowGetControlRegister(void)
{
	return controlRegister[0];
}

BOOL DVDLowPrepareCoverRegister(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_GET_COVER_REGISTER;
	ctx                                  = newContext(callback, 1);

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_GET_COVER_REGISTER, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), coverRegister,
	                    sizeof(coverRegister), doPrepareCoverRegisterCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowPrepareCoverRegsiter) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

BOOL DVDLowPrepareStatusRegister(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_GET_STATUS_REGISTER;
	ctx                                  = newContext(callback, 1);

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_GET_STATUS_REGISTER, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), statusRegister,
	                    sizeof(statusRegister), doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowPrepareStatusRegsiter) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

BOOL DVDLowPrepareControlRegister(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_GET_CONTROL_REGISTER;
	ctx                                  = newContext(callback, 1);

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_GET_CONTROL_REGISTER, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), controlRegister,
	                    sizeof(controlRegister), doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowPrepareControlRegister) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

u32 DVDLowGetImmBufferReg(void)
{
	return diRegValCache[0];
}

BOOL DVDLowUnmaskStatusInterrupts(void)
{
	return TRUE;
}

BOOL DVDLowMaskCoverInterrupt(void)
{
	return TRUE;
}

BOOL DVDLowClearCoverInterrupt(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_CLEAR_COVER_INTERRUPT;
	ctx                                  = newContext(callback, 1);

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_CLEAR_COVER_INTERRUPT, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0,
	                    doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowClearCoverInterrupt) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

// not in the dol: only its message survives (0x8031F080). Written as the plain request its neighbours send; its
// parameters and what it handed to IOS besides the command are not known and not written. The command word is IOS's.
BOOL DVDLowEnableDvdVideo(DVDLowCallback callback)
{
	DVDLowContext* ctx;
	s32 rv;

	ctx = newContext(callback, 1);
	nextCommandBuf(&freeCommandBuf);
	diCommand[freeCommandBuf].theCommand = DVD_LOW_CMD_ENABLE_DVD_VIDEO;

	rv = IOS_IoctlAsync(DiFD, DVD_LOW_CMD_ENABLE_DVD_VIDEO, &diCommand[freeCommandBuf], sizeof(DVDLowCommand), NULL, 0, doTransactionCallback, ctx);
	if (rv != 0) {
		OSReport("@@@ (DVDLowEnableDvdVideo) IOS_IoctlAsync returned error: %d\n", rv);
		ctx->inUse = FALSE;
		return FALSE;
	}

	return TRUE;
}

BOOL __DVDLowTestAlarm(const OSAlarm* alarm)
{
	return FALSE;
}
