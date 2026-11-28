#include "revolution/hw_regs.h"
#include "revolution/esp.h"
#include "revolution/dvd/dvd.h"
#include "revolution/os/OSAlarm.h"
#include "revolution/dvd/dvd_broadway.h"

#include "Dolphin/os.h"
#include "string.h"

typedef void (*stateFunc)(DVDCommandBlock* block);

// other files of the library
extern BOOL __OSInIPL;
void __DVDClearWaitingQueue(void);
BOOL __DVDPushWaitingQueue(s32 prio, DVDCommandBlock* block);
DVDCommandBlock* __DVDPopWaitingQueue(void);
BOOL __DVDCheckWaitingQueue(void);
DVDCommandBlock* __DVDGetNextWaitingQueue(void);
BOOL __DVDDequeueWaitingQueue(DVDCommandBlock* block);

// low memory: what the title was started from, and the partition it was started from
#define OS_APP_TYPE              (*(u8*)0x80003187)
#define OS_APP_TYPE_DVD          0x80
#define OS_BOOT_PARTITION_TYPE   (*(u32*)0x80003194)
#define OS_BOOT_PARTITION_OFFSET (*(u32*)0x80003198)

const char* __DVDVersion = "<< RVL_SDK - DVD \trelease build: Jul 30 2008 19:19:27 (0x4199_60831) >>";

static BOOL autoInvalidation = TRUE;

static void defaultOptionalCommandChecker(DVDCommandBlock* block, DVDCommandCheckerCallback callback);
static DVDCommandChecker checkOptionalCommand = defaultOptionalCommandChecker;

// IOS writes the ticket view (0xD8 bytes) and the TMD (at most 0x49E4 bytes) of the running title here
static u8 __DVDTicketViewBuffer[0xD8] ATTRIBUTE_ALIGN(32);
static u8 __DVDTmdBuffer[0x49E4] ATTRIBUTE_ALIGN(64);
static DVDBB2 BB2 ATTRIBUTE_ALIGN(32);
static DVDCommandBlock DummyCommandBlock;
static OSAlarm FatalAlarm;
static OSAlarm CoverAlarm;
static DVDDiskID CurrDiskID ATTRIBUTE_ALIGN(32);
static u32 GameTocBuffer[8] ATTRIBUTE_ALIGN(32);
static u32 PartInfoBuffer[8] ATTRIBUTE_ALIGN(32);
static OSAlarm ResetAlarm;
// the low-level driver hands this block to __DVDStopMotorAsync
DVDCommandBlock __DVDStopMotorCommandBlock;
// nothing in this game uses it (0x804D2340); pik2wii, Petari and xenoblade define it here as well
DVDCommandBlock __DVDRestartMotorCommandBlock;

void (*LastState)(DVDCommandBlock*);
static DVDCommandBlock* executing;
static DVDDiskID* IDShouldBe;
static OSBootInfo* bootInfo;
// The command/cover state changes inside interrupt handlers, so the SDK keeps
// it volatile; xenoblade's RVL_SDK dvd.c uses the same declarations.
// xenoblade: https://github.com/xbret/xenoblade/blob/main/libs/RVL_SDK/src/revolution/dvd/dvd.c
static volatile u32 CurrCommand;
static void (*CancelCallback)(s32, DVDCommandBlock*);
static volatile u32 CancelLastError;
static u32 LastError;
static BOOL ResetRequired;
static u32 MotorState;
static volatile OSTime LastResetEnd;
// 32-byte aligned, as in pik2wii, Petari and xenoblade: that is the unused gap in front of it in .sbss (0x805A2058)
static u32 __DVDNumTmdBytes ATTRIBUTE_ALIGN(32);
static DVDGameTOC* GameToc;
static DVDPartitionInfo* PartInfo;
static DVDPartitionInfo* BootGameInfo;
static volatile BOOL Prepared;
static volatile u32 CoverIntType;
static BOOL DVDInitialized;
volatile u32 __DVDLayoutFormat;
static volatile BOOL PreparingCover;
static BOOL ChangedDisc;
static BOOL MotorStopped;
static volatile BOOL WaitingForCoverClose;
static volatile BOOL WaitingForCoverOpen;
static BOOL Breaking;
static BOOL FirstTimeInBootrom;
static BOOL CancelAllSyncComplete;
static volatile s32 NumInternalRetry;
static volatile u32 ResumeFromHere;
static volatile u32 Canceling;
static BOOL FatalErrorFlag;
static volatile BOOL PausingFlag;
static volatile BOOL PauseFlag;
static volatile u32 CommandInfoCounter;

// prototypes
static void stateReadingFST(void);
static void cbForStateReadingFST(u32 intType);
static void cbForStateError(u32 intType);
static void stateTimeout(void);
static void stateSecurityError(void);
static void stateGettingError(void);
static u32 CategorizeError(u32 error);
static BOOL CheckCancel(u32 resume);
static void cbForStateGettingError(u32 intType);
static void cbForUnrecoveredError(u32 intType);
static void cbForUnrecoveredErrorRetry(u32 intType);
static void stateGoToRetry(void);
static void cbForStateGoToRetry(u32 intType);
static void stateCheckID(void);
static void stateReadingTOC(void);
static void cbForStateReadingTOC(u32 intType);
static void stateReadingPartitionInfo(void);
static void cbForStateReadingPartitionInfo(u32 intType);
static void stateOpenPartition(void);
static void cbForStateOpenPartition(u32 intType);
static void stateOpenPartition2(void);
static void cbForStateOpenPartition2(u32 intType);
static void stateCheckID2(void);
static void cbForStateCheckID1(u32 intType);
static void cbForStateCheckID2(u32 intType);
static void stateCoverClosed(void);
static void ResetAlarmHandler(OSAlarm* alarm, OSContext* context);
static void cbForStateReset(u32 intType);
static void stateDownRotation(DVDCommandBlock* command);
static void cbForStateDownRotation(u32 intType);
static void stateCoverClosed_CMD(DVDCommandBlock* command);
static void cbForStateCoverClosed(u32 intType);
static void cbForPrepareCoverRegister(u32 intType);
static void CoverAlarmHandler(OSAlarm* alarm, OSContext* context);
static void stateMotorStopped(void);
static void stateReady(void);
static void stateBusy(DVDCommandBlock* block);
static void cbForStateBusy(u32 intType);
static BOOL issueCommand(s32 prio, DVDCommandBlock* block);
static void cbForCancelSync(s32 result, DVDCommandBlock* block);
static void cbForCancelAllSync(s32 result, DVDCommandBlock* block);

// Logs a command for the error record. Called from so many states that retail keeps a copy of its own.
void StampCommand(u32 command, u32 offset, u32 length)
{
	BOOL enabled = OSDisableInterrupts();

	if (CommandInfoCounter >= 5) {
		CommandInfoCounter = 0;
	}

	__ErrorInfo.lastCommand[CommandInfoCounter].command = command;
	__ErrorInfo.lastCommand[CommandInfoCounter].offset  = offset;
	__ErrorInfo.lastCommand[CommandInfoCounter].length  = length;
	__ErrorInfo.lastCommand[CommandInfoCounter].tick    = OSGetTick();
	CommandInfoCounter++;

	OSRestoreInterrupts(enabled);
}

static void StampIntType(u32 intType)
{
	BOOL enabled = OSDisableInterrupts();

	if (CommandInfoCounter == 0) {
		__ErrorInfo.lastCommand[4].intType = intType;
	} else {
		__ErrorInfo.lastCommand[CommandInfoCounter - 1].intType = intType;
	}

	OSRestoreInterrupts(enabled);
}

static void defaultOptionalCommandChecker(DVDCommandBlock* block, DVDCommandCheckerCallback callback)
{
}

void DVDInit(void)
{
	s32 rv;
	DVDDiskID* id;

	if (DVDInitialized) {
		return;
	}

	OSRegisterVersion(__DVDVersion);
	DVDInitialized = TRUE;
	DVDLowInit();

	if (!__OSInIPL && OS_APP_TYPE == OS_APP_TYPE_DVD) {
		rv = ESP_InitLib();
		if (rv == 0) {
			rv = ESP_DiGetTicketView(NULL, (ESTicketView*)__DVDTicketViewBuffer);
		}
		if (rv == 0) {
			rv = ESP_DiGetTmd(NULL, &__DVDNumTmdBytes);
		}
		if (rv == 0) {
			rv = ESP_DiGetTmd((ESTitleMeta*)__DVDTmdBuffer, &__DVDNumTmdBytes);
		}
		ESP_CloseLib();
	}

	__DVDFSInit();
	__DVDClearWaitingQueue();

	MotorState = 0;
	bootInfo   = (OSBootInfo*)OSPhysicalToCached(0);
	IDShouldBe = &bootInfo->DVDDiskID;

	OSInitThreadQueue(&__DVDThreadQueue);
	DVDLowUnmaskStatusInterrupts();
	DVDLowMaskCoverInterrupt();

	if (bootInfo->magic != OS_BOOTINFO_MAGIC_JTAG && bootInfo->magic != OS_BOOTINFO_MAGIC) {
		FirstTimeInBootrom = TRUE;
	}

	memset(&__ErrorInfo, 0, sizeof(DVDErrorInfo));
	id = DVDGetCurrentDiskID();
	memcpy(__ErrorInfo.gameName, id->gameName, 4);
	__ErrorInfo.diskNumber  = id->diskNumber;
	__ErrorInfo.gameVersion = id->gameVersion;

	__DVDLayoutFormat = 0;
	DVDSetAutoFatalMessaging(TRUE);
}

static void stateReadingFST(void)
{
	LastState = (stateFunc)stateReadingFST;

	if (bootInfo->FSTMaxLength < BB2.FSTLength) {
		OSPanic(__FILE__, 0x43F, "DVDChangeDisk(): FST in the new disc is too big.   ");
	}

	DVDLowClearCoverInterrupt(NULL);
	StampCommand(1, BB2.FSTPosition >> __DVDLayoutFormat, OSRoundUp32B(BB2.FSTLength << (~__DVDLayoutFormat & 2)));
	DVDLowRead(bootInfo->FSTLocation, OSRoundUp32B(BB2.FSTLength << (~__DVDLayoutFormat & 2)), BB2.FSTPosition >> __DVDLayoutFormat,
	           cbForStateReadingFST);
}

static u32 DmaCommand[1] = { 0xFFFFFFFF };

static void cbForStateReadingFST(u32 intType)
{
	DVDCommandBlock* finished;

	StampIntType(intType);

	if (intType == 16) {
		stateTimeout();
		return;
	}

	if (intType == 32) {
		stateSecurityError();
		return;
	}

	if (intType & DVD_INTTYPE_TC) {
		NumInternalRetry = 0;

		__DVDFSInit();
		finished        = executing;
		executing       = &DummyCommandBlock;
		finished->state = DVD_STATE_END;

		if (finished->callback) {
			finished->callback(0, finished);
		}

		stateReady();
		return;
	}

	stateGettingError();
}

static void FatalAlarmHandler(OSAlarm* alarm, OSContext* context)
{
	__DVDPrintFatalMessage();
}

static void cbForStateError(u32 intType)
{
	DVDCommandBlock* finished;

	if (__DVDGetAutoFatalMessaging()) {
		OSCreateAlarm(&FatalAlarm);
		OSSetAlarm(&FatalAlarm, 1, FatalAlarmHandler);
		return;
	}

	executing->state = DVD_STATE_FATAL_ERROR;

	if (intType == 16) {
		stateTimeout();
		return;
	}

	if (intType == 32) {
		stateSecurityError();
		return;
	}

	FatalErrorFlag = TRUE;
	finished       = executing;
	executing      = &DummyCommandBlock;

	if (finished->callback) {
		(finished->callback)(-1, finished);
	}

	if (Canceling) {
		Canceling = FALSE;
		if (CancelCallback)
			(CancelCallback)(0, finished);
	}

	stateReady();
}

static void cbForStoreErrorCode1(s32 result, DVDCommandBlock* block)
{
	DVDLowStopMotor(FALSE, FALSE, cbForStateError);
}

static void stateError(u32 error)
{
	__DVDStoreErrorCode(error, cbForStoreErrorCode1);
}

static void cbForStoreErrorCode2(s32 result, DVDCommandBlock* block)
{
	DVDLowSetSpinupFlag(FALSE);
	DVDLowReset(cbForStateError);
	ResetRequired  = FALSE;
	ResumeFromHere = 0;
}

static void stateTimeout(void)
{
	__DVDStoreErrorCode(0x01234568, cbForStoreErrorCode2);
}

static void stateSecurityError(void)
{
	__DVDStoreErrorCode(0x01234569, cbForStoreErrorCode2);
}

static void stateGettingError(void)
{
	StampCommand(39, 0, 0);
	DVDLowRequestError(cbForStateGettingError);
}

static u32 CategorizeError(u32 error)
{
	if (error == 0x20400) {
		LastError = error;
		return 1;
	}

	error &= 0x00FFFFFF;
	if (error == 0x62800 || error == 0x23A00 || error == 0x53000 || error == 0xB5A01) {
		return 0;
	}

	if (error == 0x52000 && (executing->command == 37 || LastState == stateDownRotation)) {
		return 0;
	}

	NumInternalRetry++;
	if (NumInternalRetry == 2) {
		if (error == LastError) {
			LastError = error;
			return 1;
		}
		LastError = error;
		return 2;
	}

	LastError = error;

	if (error == 0x31100 || executing->command == 5) {
		return 2;
	}

	return 3;
}

static BOOL CheckCancel(u32 resume)
{
	DVDCommandBlock* finished;

	if (Canceling) {
		ResumeFromHere = resume;
		Canceling      = FALSE;

		finished  = executing;
		executing = &DummyCommandBlock;

		finished->state = DVD_STATE_CANCELED;

		if (finished->callback)
			(*finished->callback)(-3, finished);

		if (CancelCallback)
			(CancelCallback)(0, finished);

		stateReady();
		return TRUE;
	}

	return FALSE;
}

static void cbForStoreErrorCode3(s32 result, DVDCommandBlock* block)
{
	stateGoToRetry();
}

static void cbForStateGettingError(u32 intType)
{
	u32 error;
	u32 status;
	u32 errorCategory;
	u32 resume;

	StampIntType(intType);

	if (intType == 16) {
		stateTimeout();
		return;
	}

	if (intType == 32) {
		stateSecurityError();
		return;
	}

	if (intType & DVD_INTTYPE_DE) {
		stateError(0x1234567);
		return;
	}

	error  = DVDLowGetImmBufferReg();
	status = error & 0xFF000000;

	errorCategory = CategorizeError(error);

	if (errorCategory == 1) {
		stateError(error);
		return;
	}

	if (errorCategory == 2 || errorCategory == 3) {
		resume = 0;
	} else {
		if (status == 0x01000000)
			resume = 4;
		else if (status == 0x02000000)
			resume = 6;
		else if (status == 0x03000000)
			resume = 3;
		else if (status == 0x00000000) {
			if (error == 0x53000)
				resume = 1;
			else
				resume = 5;
		} else
			resume = 5;
	}

	if (CheckCancel(resume))
		return;

	if (errorCategory == 2) {
		__DVDStoreErrorCode(error, cbForStoreErrorCode3);
		return;
	}

	if (errorCategory == 3) {
		if ((error & 0x00FFFFFF) == 0x00031100) {
			StampCommand(2, executing->offset, 0);
			DVDLowSeek(executing->offset, cbForUnrecoveredError);
		} else {
			LastState(executing);
		}
		return;
	}

	if (status == 0x01000000) {
		executing->state = DVD_STATE_COVER_OPEN;
		stateMotorStopped();
		return;
	} else if (status == 0x02000000) {
		executing->state = DVD_STATE_COVER_CLOSED;
		stateCoverClosed();
		return;
	} else if (status == 0x03000000) {
		executing->state = DVD_STATE_NO_DISK;
		stateMotorStopped();
		return;
	} else if (status == 0x00000000) {
		if (error == 0x53000) {
			StampCommand(16, 0, 0);
			DVDLowStopMotor(FALSE, FALSE, cbForStateCheckID1);
		} else {
			stateError(0x1234567);
		}
		return;
	} else {
		stateError(0x1234567);
		return;
	}
}

static void cbForUnrecoveredError(u32 intType)
{
	StampIntType(intType);

	if (intType == 16) {
		stateTimeout();
		return;
	}

	if (intType == 32) {
		stateSecurityError();
		return;
	}

	if (intType & DVD_INTTYPE_TC) {
		stateGoToRetry();
		return;
	}

	StampCommand(39, 0, 0);
	DVDLowRequestError(cbForUnrecoveredErrorRetry);
}

static void cbForUnrecoveredErrorRetry(u32 intType)
{
	StampIntType(intType);

	if (intType == 16) {
		stateTimeout();
		return;
	}

	if (intType == 32) {
		stateSecurityError();
		return;
	}

	if (intType & DVD_INTTYPE_DE) {
		stateError(0x01234567);
		return;
	}

	stateError(DVDLowGetImmBufferReg());
}

static void stateGoToRetry(void)
{
	StampCommand(16, 0, 0);
	DVDLowStopMotor(FALSE, FALSE, cbForStateGoToRetry);
}

static void cbForStateGoToRetry(u32 intType)
{
	StampIntType(intType);

	if (intType == 16) {
		stateTimeout();
		return;
	}

	if (intType == 32) {
		stateSecurityError();
		return;
	}

	if (intType & DVD_INTTYPE_DE) {
		stateError(0x1234567);
		return;
	}

	NumInternalRetry = 0;

	if (CurrCommand == 4 || CurrCommand == 5 || CurrCommand == 13 || CurrCommand == 33 || CurrCommand == 34 || CurrCommand == 41
	    || CurrCommand == 42 || CurrCommand == 15 || CurrCommand == 37) {
		ResetRequired = TRUE;
	}

	if (!CheckCancel(2)) {
		executing->state = DVD_STATE_RETRY;
		stateMotorStopped();
	}
}

static void stateCheckID(void)
{
	switch (CurrCommand) {
	case 3:
		ChangedDisc = FALSE;
		if (DVDCompareDiskID(&CurrDiskID, executing->id)) {
			memcpy(IDShouldBe, &CurrDiskID, sizeof(DVDDiskID));
			executing->state = DVD_STATE_BUSY;
			DCInvalidateRange(&BB2, sizeof(DVDBB2));
			NumInternalRetry = 0;
			stateReadingTOC();
		} else {
			StampCommand(16, 0, 0);
			DVDLowStopMotor(FALSE, FALSE, cbForStateCheckID1);
		}
		break;
	default:
		if (memcmp(&CurrDiskID, IDShouldBe, sizeof(DVDDiskID)) != 0) {
			StampCommand(16, 0, 0);
			DVDLowStopMotor(FALSE, FALSE, cbForStateCheckID1);
		} else {
			NumInternalRetry = 0;
			stateReadingTOC();
		}
		break;
	}
}

static void stateReadingTOC(void)
{
	DVDLowClearCoverInterrupt(NULL);
	StampCommand(33, 0x10000, sizeof(GameTocBuffer));
	DVDLowUnencryptedRead(GameTocBuffer, sizeof(GameTocBuffer), 0x10000, cbForStateReadingTOC);
}

static void cbForStateReadingTOC(u32 intType)
{
	StampIntType(intType);

	if (intType == 16) {
		stateTimeout();
		return;
	}

	if (intType == 32) {
		stateSecurityError();
		return;
	}

	if (intType & DVD_INTTYPE_TC) {
		NumInternalRetry = 0;
		GameToc          = (DVDGameTOC*)GameTocBuffer;
		stateReadingPartitionInfo();
		return;
	}

	stateGettingError();
}

static void stateReadingPartitionInfo(void)
{
	DVDLowClearCoverInterrupt(NULL);
	StampCommand(33, 0x10008, sizeof(PartInfoBuffer));
	DVDLowUnencryptedRead(PartInfoBuffer, sizeof(PartInfoBuffer), 0x10008, cbForStateReadingPartitionInfo);
}

static void cbForStateReadingPartitionInfo(u32 intType)
{
	s16 i;

	StampIntType(intType);

	if (intType == 16) {
		stateTimeout();
		return;
	}

	if (intType == 32) {
		stateSecurityError();
		return;
	}

	if (intType & DVD_INTTYPE_TC) {
		NumInternalRetry = 0;
		PartInfo         = (DVDPartitionInfo*)PartInfoBuffer;
		BootGameInfo     = NULL;

		if (OS_BOOT_PARTITION_OFFSET != 0) {
			BootGameInfo         = PartInfo;
			PartInfo->type       = OS_BOOT_PARTITION_TYPE;
			BootGameInfo->offset = OS_BOOT_PARTITION_OFFSET;
		} else {
			for (i = 0; i < GameToc->numGamePartitions; i++) {
				if (PartInfo->type == OS_BOOT_PARTITION_TYPE) {
					BootGameInfo = PartInfo;
				}
				PartInfo++;
			}
		}

		if (BootGameInfo) {
			switch (CurrCommand) {
			case 3:
				NumInternalRetry = 0;
				stateOpenPartition();
				break;
			default:
				NumInternalRetry = 0;
				stateOpenPartition2();
				break;
			}
		} else if (!CheckCancel(1)) {
			executing->state = DVD_STATE_WRONG_DISK;
			stateMotorStopped();
		}
		return;
	}

	stateGettingError();
}

static void stateOpenPartition(void)
{
	DVDLowClearCoverInterrupt(NULL);
	StampCommand(34, BootGameInfo->offset, 0);

	if (OS_APP_TYPE == OS_APP_TYPE_DVD) {
		DVDLowOpenPartitionWithTmdAndTicketView(BootGameInfo->offset, (ESTicketView*)__DVDTicketViewBuffer, __DVDNumTmdBytes,
                                                 (ESTitleMeta*)__DVDTmdBuffer, 0, NULL,
		                                        cbForStateOpenPartition);
	} else {
		DVDLowOpenPartition(BootGameInfo->offset, NULL, 0, NULL, (ESTitleMeta*)__DVDTmdBuffer, cbForStateOpenPartition);
	}
}

static void cbForStateOpenPartition(u32 intType)
{
	StampIntType(intType);

	if (intType == 16) {
		stateTimeout();
		return;
	}

	if (intType == 32) {
		stateSecurityError();
		return;
	}

	if (intType & DVD_INTTYPE_TC) {
		NumInternalRetry = 0;
		stateCheckID2();
		return;
	}

	stateGettingError();
}

static void stateOpenPartition2(void)
{
	DVDLowClearCoverInterrupt(NULL);
	StampCommand(34, BootGameInfo->offset, 0);

	if (OS_APP_TYPE == OS_APP_TYPE_DVD) {
		DVDLowOpenPartitionWithTmdAndTicketView(BootGameInfo->offset, (ESTicketView*)__DVDTicketViewBuffer, __DVDNumTmdBytes,
                                                 (ESTitleMeta*)__DVDTmdBuffer, 0, NULL,
		                                        cbForStateOpenPartition2);
	} else {
		DVDLowOpenPartition(BootGameInfo->offset, NULL, 0, NULL, (ESTitleMeta*)__DVDTmdBuffer, cbForStateOpenPartition2);
	}
}

static void cbForStateOpenPartition2(u32 intType)
{
	StampIntType(intType);

	if (intType == 16) {
		stateTimeout();
		return;
	}

	if (intType == 32) {
		stateSecurityError();
		return;
	}

	if (intType & DVD_INTTYPE_TC) {
		NumInternalRetry = 0;
		if (!CheckCancel(0)) {
			executing->state = DVD_STATE_BUSY;
			stateBusy(executing);
		}
		return;
	}

	stateGettingError();
}

static void stateCheckID2(void)
{
	DVDLowClearCoverInterrupt(NULL);
	StampCommand(1, 0x108, sizeof(DVDBB2));
	DVDLowRead(&BB2, sizeof(DVDBB2), 0x108, cbForStateCheckID2);
}

static void cbForStateCheckID1(u32 intType)
{
	StampIntType(intType);

	if (intType == 16) {
		stateTimeout();
		return;
	}

	if (intType == 32) {
		stateSecurityError();
		return;
	}

	if (intType & DVD_INTTYPE_DE) {
		stateError(0x01234567);
		return;
	}

	NumInternalRetry = 0;

	if (!CheckCancel(1)) {
		executing->state = DVD_STATE_WRONG_DISK;
		stateMotorStopped();
	}
}

static void cbForStateCheckID2(u32 intType)
{
	StampIntType(intType);

	if (intType == 16) {
		stateTimeout();
		return;
	}

	if (intType == 32) {
		stateSecurityError();
		return;
	}

	if (intType & DVD_INTTYPE_TC) {
		NumInternalRetry = 0;
		stateReadingFST();
		return;
	}

	stateGettingError();
}

static void stateCoverClosed(void)
{
	DVDCommandBlock* finished;

	MotorState = 1;

	switch (CurrCommand) {
	case 4:
	case 5:
	case 13:
	case 15:
	case 33:
	case 34:
	case 37:
	case 41:
	case 42:
		__DVDClearWaitingQueue();
		finished  = executing;
		executing = &DummyCommandBlock;
		if (finished->callback) {
			finished->callback(-4, finished);
		}
		stateReady();
		break;
	case 32:
		MotorState = 0;
	case 35:
	case 36:
	case 38:
		executing->state = DVD_STATE_BUSY;
		stateBusy(executing);
		break;
	case 1:
	case 2:
		if (__OSInIPL) {
			break;
		}
	default:
		MotorState = 0;
		DVDLowSetSpinupFlag(TRUE);
		DVDLowReset(cbForStateReset);
		break;
	}
}

static void ResetAlarmHandler(OSAlarm* alarm, OSContext* context)
{
	// this drive model first gets its rotation speed limited
	if (__OSDeviceCode == 0x8003) {
		LastState = stateDownRotation;
		stateDownRotation(executing);
	} else {
		DCInvalidateRange(&CurrDiskID, sizeof(DVDDiskID));
		LastState = stateCoverClosed_CMD;
		stateCoverClosed_CMD(executing);
	}
}

static void cbForStateReset(u32 intType)
{
	if (intType == 16) {
		stateTimeout();
		return;
	}

	if (intType == 32) {
		stateSecurityError();
		return;
	}

	if (intType & DVD_INTTYPE_TC) {
		LastResetEnd   = __OSGetSystemTime();
		ResetRequired  = FALSE;
		ResumeFromHere = 0;
		OSCreateAlarm(&ResetAlarm);
		OSSetAlarm(&ResetAlarm, OSMillisecondsToTicks(100), ResetAlarmHandler);
		return;
	}

	stateGettingError();
}

static void stateDownRotation(DVDCommandBlock* command)
{
	DVDLowClearCoverInterrupt(NULL);
	StampCommand(37, 0, 0);
	DVDLowSetMaxRotation(0x20000, cbForStateDownRotation);
}

static void cbForStateDownRotation(u32 intType)
{
	StampIntType(intType);

	if (intType == 16) {
		stateTimeout();
		return;
	}

	if (intType == 32) {
		stateSecurityError();
		return;
	}

	if (intType & DVD_INTTYPE_TC) {
		DCInvalidateRange(&CurrDiskID, sizeof(DVDDiskID));
		LastState = stateCoverClosed_CMD;
		stateCoverClosed_CMD(executing);
		return;
	}

	stateGettingError();
}

static void stateCoverClosed_CMD(DVDCommandBlock* command)
{
	if (CurrCommand == 40) {
		NumInternalRetry = 0;
		if (!CheckCancel(0)) {
			executing->state = DVD_STATE_BUSY;
			stateBusy(executing);
		}
	} else {
		DVDLowClearCoverInterrupt(NULL);
		StampCommand(5, 0, sizeof(DVDDiskID));
		DVDLowReadDiskID(&CurrDiskID, cbForStateCoverClosed);
	}
}

static void cbForStateCoverClosed(u32 intType)
{
	StampIntType(intType);

	if (intType == 16) {
		stateTimeout();
		return;
	}

	if (intType == 32) {
		stateSecurityError();
		return;
	}

	if (intType & DVD_INTTYPE_TC) {
		NumInternalRetry = 0;
		stateCheckID();
		return;
	}

	stateGettingError();
}

// both paths of cbForPrepareCoverRegister that see the cover closed run this; the first one clears
// WaitingForCoverClose a second time in front of it
static void CoverClosed(void)
{
	WaitingForCoverClose = FALSE;

	if (CurrCommand == 3) {
		ChangedDisc = TRUE;
	}

	if (MotorState == 2) {
		if (executing) {
			executing->state = 12;
		}
	} else {
		DVDLowMaskCoverInterrupt();
		if (executing) {
			executing->state = DVD_STATE_COVER_CLOSED;
			stateCoverClosed();
		} else {
			ResumeFromHere = 7;
		}
	}
}

static void cbForPrepareCoverRegister(u32 intType)
{
	PreparingCover = FALSE;

	if (WaitingForCoverClose) {
		if (!(DVDLowGetCoverRegister() & 1)) {
			OSCancelAlarm(&CoverAlarm);
			WaitingForCoverClose = FALSE;
			CoverClosed();
		}
	} else if (DVDLowGetCoverRegister() & 1) {
		WaitingForCoverOpen  = FALSE;
		WaitingForCoverClose = TRUE;
		if (MotorState == 2) {
			executing->state = 12;
		} else {
			executing->state = DVD_STATE_COVER_OPEN;
		}
	} else if (DVDLowGetCoverRegister() & 4) {
		OSCancelAlarm(&CoverAlarm);
		WaitingForCoverOpen = FALSE;
		DVDLowClearCoverInterrupt(NULL);
		CoverClosed();
	}
}

static void CoverAlarmHandler(OSAlarm* alarm, OSContext* context)
{
	if (!PreparingCover) {
		PreparingCover = TRUE;
		DVDLowPrepareCoverRegister(cbForPrepareCoverRegister);
	}
}

static void stateMotorStopped(void)
{
	MotorState = 1;

	if (!WaitingForCoverOpen && !WaitingForCoverClose) {
		WaitingForCoverOpen = TRUE;
		OSCreateAlarm(&CoverAlarm);
		OSSetPeriodicAlarm(&CoverAlarm, OSGetTick(), OSMillisecondsToTicks(100), CoverAlarmHandler);
	}
}

static void stateReady(void)
{
	DVDCommandBlock* finished;

	if (PauseFlag) {
		PausingFlag = TRUE;
		executing   = NULL;
		return;
	}

	if (!__DVDCheckWaitingQueue()) {
		executing = NULL;
		return;
	}

	executing = __DVDPopWaitingQueue();

	if (FatalErrorFlag) {
		executing->state = DVD_STATE_FATAL_ERROR;
		finished         = executing;
		executing        = &DummyCommandBlock;
		if (finished->callback) {
			(*finished->callback)(-1, finished);
		}
		stateReady();
		return;
	}

	CurrCommand = executing->command;

	if (CurrCommand == 32 || CurrCommand == 14 || CurrCommand == 35) {
		ResumeFromHere = 0;
	}

	if (ResumeFromHere != 0) {
		switch (ResumeFromHere) {
		case 2:
			executing->state = DVD_STATE_RETRY;
			stateMotorStopped();
			break;
		case 3:
			executing->state = DVD_STATE_NO_DISK;
			stateMotorStopped();
			break;
		case 4:
			executing->state = DVD_STATE_COVER_OPEN;
			stateMotorStopped();
			break;
		case 1:
		case 6:
		case 7:
			executing->state = DVD_STATE_COVER_CLOSED;
			stateCoverClosed();
			break;
		case 5:
			stateError(CancelLastError);
			break;
		}
		ResumeFromHere = 0;
		return;
	}

	switch (MotorState) {
	case 2:
		if (MotorStopped) {
			executing->state = 12;
		} else {
			executing->state = DVD_STATE_COVER_CLOSED;
			stateCoverClosed();
		}
		break;
	case 0:
		executing->state = DVD_STATE_BUSY;
		stateBusy(executing);
		break;
	case 1:
	default:
		stateCoverClosed();
		break;
	}
}

static void stateBusy(DVDCommandBlock* block)
{
	DVDCommandBlock* finished;
	DVDPartitionParams* params;

	LastState = stateBusy;

	switch (block->command) {
	case 2:
	case 3:
	case 5:
	case 13:
	case 14:
	case 15:
	case 16:
	case 34:
	case 37:
	case 41:
	case 42:
		StampCommand(block->command, block->offset, block->length);
		break;
	}

	switch (block->command) {
	case 5:
		DVDLowClearCoverInterrupt(NULL);
		block->currTransferSize = sizeof(DVDDiskID);
		DVDLowReadDiskID(block->addr, cbForStateBusy);
		break;
	case 1:
	case 4:
		if (block->length == 0) {
			finished        = executing;
			executing       = &DummyCommandBlock;
			finished->state = DVD_STATE_END;
			if (finished->callback) {
				(*finished->callback)(0, finished);
			}
			stateReady();
		} else {
			DVDLowClearCoverInterrupt(NULL);
			block->currTransferSize
			    = (block->length - block->transferredSize > 0x80000) ? 0x80000 : (block->length - block->transferredSize);
			StampCommand(block->command, block->offset + (block->transferredSize >> 2), block->currTransferSize);
			DVDLowRead((char*)block->addr + block->transferredSize, block->currTransferSize, block->offset + (block->transferredSize >> 2),
			           cbForStateBusy);
		}
		break;
	case 2:
		DVDLowClearCoverInterrupt(NULL);
		DVDLowSeek(block->offset, cbForStateBusy);
		break;
	case 3:
		DVDLowStopMotor(FALSE, FALSE, cbForStateBusy);
		break;
	case 15:
		DVDLowStopMotor(FALSE, FALSE, cbForStateBusy);
		break;
	case 13:
		DVDLowClearCoverInterrupt(NULL);
		DVDLowAudioBufferConfig(block->offset, block->length, cbForStateBusy);
		break;
	case 14:
		DVDLowClearCoverInterrupt(NULL);
		block->currTransferSize = sizeof(DVDDriveInfo);
		DVDLowInquiry(block->addr, cbForStateBusy);
		break;
	case 16:
		DVDLowClearCoverInterrupt(NULL);
		DVDLowStopMotor(FALSE, FALSE, cbForStateBusy);
		break;
	case 32:
		DVDLowSetSpinupFlag(TRUE);
		DVDLowReset(cbForStateBusy);
		break;
	case 33:
		if (block->length == 0) {
			finished        = executing;
			executing       = &DummyCommandBlock;
			finished->state = DVD_STATE_END;
			if (finished->callback) {
				(*finished->callback)(0, finished);
			}
			stateReady();
		} else {
			DVDLowClearCoverInterrupt(NULL);
			block->currTransferSize
			    = (block->length - block->transferredSize > 0x80000) ? 0x80000 : (block->length - block->transferredSize);
			StampCommand(block->command, block->offset + (block->transferredSize >> 2), block->currTransferSize);
			DVDLowUnencryptedRead((char*)block->addr + block->transferredSize, block->currTransferSize,
			                      block->offset + (block->transferredSize >> 2), cbForStateBusy);
		}
		break;
	case 34:
		DVDLowClearCoverInterrupt(NULL);
		DVDLowOpenPartition(block->offset, NULL, 0, NULL, block->addr, cbForStateBusy);
		break;
	case 35:
		DVDLowClearCoverInterrupt(NULL);
		DVDLowClosePartition(cbForStateBusy);
		break;
	case 38:
		DVDLowPrepareCoverRegister(cbForStateBusy);
		break;
	case 36:
		DVDLowPrepareCoverRegister(cbForStateBusy);
		break;
	case 37:
		DVDLowClearCoverInterrupt(NULL);
		DVDLowSetMaxRotation(0x20000, cbForStateBusy);
		break;
	case 40:
		DVDLowClearCoverInterrupt(NULL);
		block->addr             = &CurrDiskID;
		block->currTransferSize = sizeof(DVDDiskID);
		DVDLowReadDiskID(&CurrDiskID, cbForStateBusy);
		break;
	case 41:
		DVDLowClearCoverInterrupt(NULL);
		params = block->addr;
		if (params->numTmdBytes == 0 && params->numCertBytes == 0) {
			DVDLowGetNoDiscBufferSizes(block->offset, &params->numTmdBytes, &params->numCertBytes, cbForStateBusy);
		} else {
			DVDLowGetNoDiscOpenPartitionParams(block->offset, (ESTicket*)params->ticket, &params->numTmdBytes, (ESTitleMeta*)params->tmd,
                                           &params->numCertBytes,
			                                   params->certs, &params->dataWordOffset, params->h3Hash, cbForStateBusy);
		}
		break;
	case 42:
		DVDLowClearCoverInterrupt(NULL);
		DVDLowOpenPartitionWithTmdAndTicketView(block->offset, (ESTicketView*)((DVDPartitionParams*)block->addr)->ticketView,
		                                        ((DVDPartitionParams*)block->addr)->numTmdBytes,
		                                        (ESTitleMeta*)((DVDPartitionParams*)block->addr)->tmd,
		                                        ((DVDPartitionParams*)block->addr)->numCertBytes, ((DVDPartitionParams*)block->addr)->certs,
		                                        cbForStateBusy);
		break;
	default:
		checkOptionalCommand(block, cbForStateBusy);
		break;
	}
}

static u32 ImmCommand[3] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };

static BOOL IsImmCommandWithResult(u32 command)
{
	u32 i;

	if (command == 9 || command == 10 || command == 11 || command == 12) {
		return TRUE;
	}

	for (i = 0; i < 3; i++) {
		if (command == ImmCommand[i]) {
			return TRUE;
		}
	}

	return FALSE;
}

static BOOL IsDmaCommand(u32 command)
{
	u32 i;

	if (command == 1 || command == 4 || command == 5 || command == 33 || command == 14) {
		return TRUE;
	}

	for (i = 0; i < 1; i++) {
		if (command == DmaCommand[i]) {
			return TRUE;
		}
	}

	return FALSE;
}

static void cbForStateBusy(u32 intType)
{
	DVDCommandBlock* finished;
	s32 result;
	u32 coverReg;

	StampIntType(intType);

	if (intType == 16) {
		stateTimeout();
		return;
	}

	if (intType == 32) {
		stateSecurityError();
		return;
	}

	if (CurrCommand == 3 || CurrCommand == 15) {
		if (intType & DVD_INTTYPE_DE) {
			stateError(0x01234567);
			return;
		}

		NumInternalRetry = 0;

		if (CurrCommand == 15) {
			ResetRequired = TRUE;
		}

		if (!CheckCancel(7)) {
			if (MotorState != 2) {
				executing->state = DVD_STATE_MOTOR_STOPPED;
				stateMotorStopped();
			}
		}
		return;
	}

	if (IsDmaCommand(CurrCommand)) {
		executing->transferredSize += (intType & (DVD_INTTYPE_TC | DVD_INTTYPE_BR)) ? executing->currTransferSize : 0;
	}

	if (Breaking) {
		Breaking        = FALSE;
		Canceling       = FALSE;
		finished        = executing;
		executing       = &DummyCommandBlock;
		finished->state = DVD_STATE_CANCELED;
		if (finished->callback) {
			finished->callback(-3, finished);
		}
		if (CancelCallback) {
			CancelCallback(0, finished);
		}
		stateReady();
		return;
	}

	if (intType & DVD_INTTYPE_TC) {
		NumInternalRetry = 0;

		if (CurrCommand == 16) {
			if (executing->offset) {
				MotorState = 2;
			} else {
				MotorState = 1;
			}
			finished        = executing;
			executing       = &DummyCommandBlock;
			finished->state = DVD_STATE_END;
			if (finished->callback) {
				(*finished->callback)(0, finished);
			}
			stateReady();
			return;
		}

		if (CurrCommand == 32) {
			LastResetEnd    = __OSGetSystemTime();
			ResetRequired   = FALSE;
			ResumeFromHere  = 0;
			finished        = executing;
			executing       = &DummyCommandBlock;
			finished->state = DVD_STATE_END;
			if (finished->callback) {
				(*finished->callback)(0, finished);
			}
			stateReady();
			return;
		}

		if (CheckCancel(0)) {
			return;
		}

		if (CurrCommand == 38) {
			coverReg = DVDLowGetCoverRegister();
			if (__OSGetSystemTime() - LastResetEnd < OSMillisecondsToTicks(100)) {
				result = 0;
			} else if (coverReg & 1) {
				result = 1;
			} else {
				result = 2;
			}
			finished         = executing;
			executing        = &DummyCommandBlock;
			finished->state  = DVD_STATE_END;
			finished->offset = result;
			if (finished->callback) {
				finished->callback(result, finished);
			}
			stateReady();
			return;
		}

		if (CurrCommand == 36) {
			coverReg = DVDLowGetCoverRegister();
			if (((coverReg >> 2) & 1) || (coverReg & 1)) {
				result = 0;
			} else if (ResumeFromHere != 0) {
				result = 0;
			} else {
				result = 1;
			}
			finished         = executing;
			executing        = &DummyCommandBlock;
			finished->state  = DVD_STATE_END;
			finished->offset = result;
			if (finished->callback) {
				finished->callback(result, finished);
			}
			stateReady();
			return;
		}

		if (CurrCommand == 40) {
			if (DVDCompareDiskID(&CurrDiskID, executing->id)) {
				memcpy(IDShouldBe, &CurrDiskID, sizeof(DVDDiskID));
				finished         = executing;
				executing        = &DummyCommandBlock;
				finished->state  = DVD_STATE_END;
				finished->offset = TRUE;
				if (finished->callback) {
					finished->callback(TRUE, finished);
				}
				NumInternalRetry = 0;
				stateReady();
			} else {
				StampCommand(16, 0, 0);
				DVDLowStopMotor(FALSE, FALSE, cbForStateCheckID1);
			}
			return;
		}

		if (CurrCommand == 41) {
			if (((DVDPartitionParams*)executing->addr)->dataWordOffset == 0) {
				stateBusy(executing);
			} else {
				finished        = executing;
				executing       = &DummyCommandBlock;
				finished->state = DVD_STATE_END;
				if (finished->callback) {
					finished->callback(0, finished);
				}
				stateReady();
			}
			return;
		}

		if (IsDmaCommand(CurrCommand)) {
			if (executing->transferredSize != executing->length) {
				stateBusy(executing);
				return;
			}
			finished        = executing;
			executing       = &DummyCommandBlock;
			finished->state = DVD_STATE_END;
			if (finished->callback) {
				finished->callback(finished->transferredSize, finished);
			}
			stateReady();
			return;
		} else if (IsImmCommandWithResult(CurrCommand)) {
			if (CurrCommand == 11 || CurrCommand == 10) {
				result = DVDLowGetImmBufferReg() << 2;
			} else {
				result = DVDLowGetImmBufferReg();
			}
			finished        = executing;
			executing       = &DummyCommandBlock;
			finished->state = DVD_STATE_END;
			if (finished->callback) {
				finished->callback(result, finished);
			}
			stateReady();
			return;
		} else {
			finished        = executing;
			executing       = &DummyCommandBlock;
			finished->state = DVD_STATE_END;
			if (finished->callback) {
				finished->callback(0, finished);
			}
			stateReady();
			return;
		}
	} else {
		if (CurrCommand == 14) {
			stateError(0x01234567);
			return;
		}

		if ((CurrCommand == 1 || CurrCommand == 4 || CurrCommand == 5 || CurrCommand == 33 || CurrCommand == 14)
		    && (executing->transferredSize == executing->length)) {
			if (CheckCancel(0)) {
				return;
			}
			finished        = executing;
			executing       = &DummyCommandBlock;
			finished->state = DVD_STATE_END;
			if (finished->callback) {
				(finished->callback)((s32)finished->transferredSize, finished);
			}
			stateReady();
			return;
		}

		stateGettingError();
	}
}

static BOOL issueCommand(s32 prio, DVDCommandBlock* block)
{
	BOOL level;
	BOOL result;

	if (autoInvalidation
	    && (block->command == 1 || block->command == 4 || block->command == 5 || block->command == 33 || block->command == 14)) {
		DCInvalidateRange(block->addr, block->length);
	}

	level        = OSDisableInterrupts();
	block->state = DVD_STATE_WAITING;
	result       = __DVDPushWaitingQueue(prio, block);

	if (executing == NULL && PauseFlag == FALSE) {
		stateReady();
	}

	OSRestoreInterrupts(level);
	return result;
}

BOOL DVDReadAbsAsyncPrio(DVDCommandBlock* block, void* addr, s32 length, s32 offset, DVDCBCallback callback, s32 prio)
{
	BOOL idle;

	block->command         = 1;
	block->addr            = addr;
	block->length          = length;
	block->offset          = offset;
	block->transferredSize = 0;
	block->callback        = callback;

	idle = issueCommand(prio, block);
	return idle;
}

BOOL DVDInquiryAsync(DVDCommandBlock* block, DVDDriveInfo* info, DVDCBCallback callback)
{
	BOOL idle;

	block->command         = 14;
	block->addr            = info;
	block->length          = sizeof(DVDDriveInfo);
	block->transferredSize = 0;
	block->callback        = callback;

	idle = issueCommand(2, block);
	return idle;
}

s32 DVDGetCommandBlockStatus(const DVDCommandBlock* block)
{
	BOOL enabled;
	s32 retVal;
	DVDCommandBlock* next;

	enabled = OSDisableInterrupts();

	if (block->state == DVD_STATE_COVER_CLOSED) {
		retVal = DVD_STATE_BUSY;
	} else if (block->state == DVD_STATE_COVER_OPEN) {
		retVal = DVD_STATE_NO_DISK;
	} else if (executing == &__DVDStopMotorCommandBlock) {
		next = __DVDGetNextWaitingQueue();
		if (next) {
			if (block == next) {
				retVal = DVD_STATE_BUSY;
			} else {
				retVal = block->state;
			}
		} else {
			if (block == &__DVDStopMotorCommandBlock) {
				retVal = DVD_STATE_END;
			} else {
				retVal = block->state;
			}
		}
	} else {
		retVal = block->state;
	}

	OSRestoreInterrupts(enabled);
	return retVal;
}

s32 DVDGetDriveStatus(void)
{
	BOOL enabled;
	s32 retVal;
	DVDCommandBlock* block;

	enabled = OSDisableInterrupts();

	if (FatalErrorFlag) {
		retVal = DVD_STATE_FATAL_ERROR;
	} else if (PausingFlag) {
		retVal = DVD_STATE_PAUSING;
	} else {
		block = executing;
		if (block == NULL) {
			retVal = DVD_STATE_END;
		} else if (block == &DummyCommandBlock) {
			retVal = DVD_STATE_END;
		} else {
			retVal = DVDGetCommandBlockStatus(block);
		}
	}

	OSRestoreInterrupts(enabled);
	return retVal;
}

BOOL DVDSetAutoInvalidation(BOOL autoInval)
{
	BOOL prev;

	prev             = autoInvalidation;
	autoInvalidation = autoInval;
	return prev;
}

void DVDPause(void)
{
	BOOL level;

	level     = OSDisableInterrupts();
	PauseFlag = TRUE;
	if (executing == NULL) {
		PausingFlag = TRUE;
	}
	OSRestoreInterrupts(level);
}

void DVDResume(void)
{
	BOOL level;

	level     = OSDisableInterrupts();
	PauseFlag = FALSE;
	if (PausingFlag) {
		PausingFlag = FALSE;
		stateReady();
	}
	OSRestoreInterrupts(level);
}

BOOL DVDCancelAsync(DVDCommandBlock* block, DVDCBCallback callback)
{
	BOOL enabled;

	enabled = OSDisableInterrupts();

	switch (block->state) {
	case DVD_STATE_FATAL_ERROR:
	case DVD_STATE_END:
	case DVD_STATE_CANCELED:
		if (callback)
			(*callback)(0, block);
		break;

	case DVD_STATE_BUSY:
		if (Canceling) {
			OSRestoreInterrupts(enabled);
			return FALSE;
		}

		Canceling      = TRUE;
		CancelCallback = callback;

		if (block->command == 4 || block->command == 33 || block->command == 34 || block->command == 41 || block->command == 42
		    || block->command == 1) {
			Breaking = TRUE;
		}
		break;

	case DVD_STATE_WAITING:
		__DVDDequeueWaitingQueue(block);
		block->state = DVD_STATE_CANCELED;
		if (block->callback)
			(block->callback)(-3, block);
		if (callback)
			(*callback)(0, block);
		break;

	case DVD_STATE_COVER_CLOSED:
		switch (block->command) {
		case 4:
		case 5:
		case 13:
		case 15:
		case 33:
		case 34:
		case 37:
		case 41:
		case 42:
			if (callback)
				(*callback)(0, block);
			break;

		case 1:
		case 2:
			if (__OSInIPL) {
				executing    = &DummyCommandBlock;
				block->state = DVD_STATE_CANCELED;
				if (block->callback)
					(block->callback)(-3, block);
				if (callback)
					(*callback)(0, block);
				stateReady();
				break;
			}

		default:
			if (Canceling) {
				OSRestoreInterrupts(enabled);
				return FALSE;
			}
			Canceling      = TRUE;
			CancelCallback = callback;
			break;
		}
		break;

	case DVD_STATE_NO_DISK:
	case DVD_STATE_COVER_OPEN:
	case DVD_STATE_WRONG_DISK:
	case DVD_STATE_MOTOR_STOPPED:
	case DVD_STATE_RETRY:
		if (!WaitingForCoverClose && !WaitingForCoverOpen) {
			OSRestoreInterrupts(enabled);
			return FALSE;
		}

		if (WaitingForCoverOpen) {
			OSCancelAlarm(&CoverAlarm);
			WaitingForCoverOpen = FALSE;
		}

		if (block->state == DVD_STATE_NO_DISK)
			ResumeFromHere = 3;
		if (block->state == DVD_STATE_COVER_OPEN)
			ResumeFromHere = 4;
		if (block->state == DVD_STATE_WRONG_DISK)
			ResumeFromHere = 1;
		if (block->state == DVD_STATE_RETRY)
			ResumeFromHere = 2;
		if (block->state == DVD_STATE_MOTOR_STOPPED)
			ResumeFromHere = 7;

		executing    = &DummyCommandBlock;
		block->state = DVD_STATE_CANCELED;
		if (block->callback) {
			(block->callback)(-3, block);
		}
		if (callback) {
			(callback)(0, block);
		}
		stateReady();
		break;

	case 12:
		executing    = &DummyCommandBlock;
		block->state = DVD_STATE_CANCELED;
		if (block->callback) {
			(block->callback)(-3, block);
		}
		if (callback) {
			(callback)(0, block);
		}
		stateReady();
		break;
	}

	OSRestoreInterrupts(enabled);
	return TRUE;
}

s32 DVDCancel(DVDCommandBlock* block)
{
	BOOL result;
	s32 state;
	u32 command;
	BOOL enabled;

	result = DVDCancelAsync(block, cbForCancelSync);

	if (result == FALSE) {
		return -1;
	}

	enabled = OSDisableInterrupts();

	for (;;) {
		state = block->state;

		if (state == DVD_STATE_END || state == DVD_STATE_FATAL_ERROR || state == DVD_STATE_CANCELED) {
			break;
		}

		if (state == DVD_STATE_COVER_CLOSED) {
			command = block->command;

			if (command == 4 || command == 5 || command == 13 || command == 33 || command == 34 || command == 41 || command == 42
			    || command == 15 || command == 37) {
				break;
			}
		}

		OSSleepThread(&__DVDThreadQueue);
	}

	OSRestoreInterrupts(enabled);
	return 0;
}

static void cbForCancelSync(s32 result, DVDCommandBlock* block)
{
	OSWakeupThread(&__DVDThreadQueue);
}

BOOL DVDCancelAllAsync(DVDCBCallback callback)
{
	BOOL enabled;
	DVDCommandBlock* p;
	BOOL retVal;

	enabled = OSDisableInterrupts();
	DVDPause();

	while ((p = __DVDPopWaitingQueue())) {
		DVDCancelAsync(p, NULL);
	}

	if (executing) {
		retVal = DVDCancelAsync(executing, callback);
	} else {
		retVal = TRUE;
		if (callback) {
			(*callback)(0, NULL);
		}
	}

	DVDResume();
	OSRestoreInterrupts(enabled);
	return retVal;
}

s32 DVDCancelAll(void)
{
	BOOL result;
	BOOL enabled;

	enabled               = OSDisableInterrupts();
	CancelAllSyncComplete = FALSE;

	result = DVDCancelAllAsync(cbForCancelAllSync);

	if (result == FALSE) {
		OSRestoreInterrupts(enabled);
		return -1;
	}

	for (;;) {
		if (CancelAllSyncComplete) {
			break;
		}
		OSSleepThread(&__DVDThreadQueue);
	}

	OSRestoreInterrupts(enabled);
	return 0;
}

static void cbForCancelAllSync(s32 result, DVDCommandBlock* block)
{
	CancelAllSyncComplete = TRUE;
	OSWakeupThread(&__DVDThreadQueue);
}

DVDDiskID* DVDGetCurrentDiskID(void)
{
	return (DVDDiskID*)OSPhysicalToCached(0);
}

static void cbForCoverStatus(u32 intType)
{
	CoverIntType = intType;
}

u32 __DVDGetCoverStatus(void)
{
	u32 coverReg;

	CoverIntType = 0;
	DVDLowPrepareCoverRegister(cbForCoverStatus);

	while (CoverIntType == 0) { }

	if (!(CoverIntType & DVD_INTTYPE_TC)) {
		return 0;
	}

	coverReg = DVDLowGetCoverRegister();

	if (__OSGetSystemTime() - LastResetEnd < OSMillisecondsToTicks(100)) {
		return 0;
	}

	if (coverReg & 1) {
		return 1;
	}

	return 2;
}

void __DVDResetWithNoSpinup(void)
{
	CoverIntType = 0;
	DVDLowSetSpinupFlag(FALSE);
	DVDLowReset(cbForCoverStatus);

	while (CoverIntType == 0) { }
}

BOOL DVDCheckDiskAsync(DVDCommandBlock* block, DVDCBCallback callback)
{
	BOOL enabled;
	BOOL retVal;
	s32 state;

	enabled = OSDisableInterrupts();

	if (FatalErrorFlag) {
		state = DVD_STATE_FATAL_ERROR;
	} else if (PausingFlag) {
		state = DVD_STATE_PAUSING;
	} else if (WaitingForCoverOpen, WaitingForCoverClose) { // retail reads the first flag and drops it
		state = DVD_STATE_COVER_OPEN;
	} else if (executing == NULL) {
		switch (ResumeFromHere) {
		case 3:
			state = DVD_STATE_NO_DISK;
			break;
		case 4:
			state = DVD_STATE_COVER_OPEN;
			break;
		case 1:
			state = DVD_STATE_WRONG_DISK;
			break;
		case 2:
			state = DVD_STATE_RETRY;
			break;
		case 7:
			state = DVD_STATE_MOTOR_STOPPED;
			break;
		default:
			state = DVD_STATE_END;
			break;
		}
	} else if (executing == &DummyCommandBlock) {
		state = DVD_STATE_END;
	} else {
		state = executing->state;
	}

	retVal = TRUE;

	switch (state) {
	case DVD_STATE_BUSY:
	case DVD_STATE_WAITING:
	case DVD_STATE_IGNORED:
	case DVD_STATE_CANCELED:
		block->state = DVD_STATE_END;
		if (callback) {
			(*callback)(TRUE, block);
		}
		OSRestoreInterrupts(enabled);
		break;

	case DVD_STATE_FATAL_ERROR:
	case DVD_STATE_COVER_CLOSED:
	case DVD_STATE_NO_DISK:
	case DVD_STATE_COVER_OPEN:
	case DVD_STATE_WRONG_DISK:
	case DVD_STATE_MOTOR_STOPPED:
	case DVD_STATE_RETRY:
	case 12:
		block->state = DVD_STATE_END;
		if (callback) {
			(*callback)(FALSE, block);
		}
		OSRestoreInterrupts(enabled);
		break;

	case DVD_STATE_END:
	case DVD_STATE_PAUSING:
		OSRestoreInterrupts(enabled);
		block->command  = 36;
		block->callback = callback;
		retVal          = issueCommand(2, block);
		break;
	}

	return retVal;
}

void __DVDPrepareResetAsync(DVDCBCallback callback)
{
	BOOL enabled;

	enabled = OSDisableInterrupts();

	__DVDClearWaitingQueue();

	if (Canceling) {
		CancelCallback = callback;
	} else {
		if (executing) {
			executing->callback = NULL;
		}
		DVDCancelAllAsync(callback);
	}

	OSRestoreInterrupts(enabled);
}

static void Callback(s32 result, DVDCommandBlock* block)
{
	Prepared = TRUE;
}

void __DVDPrepareReset(void)
{
	OSDisableInterrupts();
	Prepared = FALSE;
	__DVDPrepareResetAsync(Callback);
	OSEnableInterrupts();

	while (Prepared != TRUE) { }
}

BOOL __DVDTestAlarm(const OSAlarm* alarm)
{
	if (alarm == &ResetAlarm) {
		return TRUE;
	}

	return __DVDLowTestAlarm(alarm);
}

BOOL __DVDStopMotorAsync(DVDCommandBlock* block, DVDCBCallback callback)
{
	return TRUE;
}

void __DVDRestartMotor(void)
{
}
