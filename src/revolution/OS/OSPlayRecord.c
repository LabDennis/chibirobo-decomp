#include "revolution/hw_regs.h"
#include "revolution/os/OSAlarm.h"
#include "revolution/nand.h"

#include "Dolphin/os.h"

// one record of play_rec.dat
typedef struct OSPlayRecord {
	u32 checkSum;
	u8 unknown04[0x5C];
	OSTime lastTime;
	u8 unknown68[0x18];
} OSPlayRecord;

typedef enum PlayRecordStateKind {
	PLAY_RECORD_STATE_START,
	PLAY_RECORD_STATE_OPEN,
	PLAY_RECORD_STATE_READ,
	PLAY_RECORD_STATE_SEEK,
	PLAY_RECORD_STATE_WAIT,
	PLAY_RECORD_STATE_WRITE,
	PLAY_RECORD_STATE_CLOSE,
	PLAY_RECORD_STATE_END,
	PLAY_RECORD_STATE_TIMEOUT,
	PLAY_RECORD_STATE_STOPPED
} PlayRecordStateKind;

static void PlayRecordCallback(s32 result, NANDCommandBlock* block);

static OSPlayRecord PlayRecord ATTRIBUTE_ALIGN(32);
static OSAlarm PlayRecordAlarm;
static NANDFileInfo FileInfo;
static NANDCommandBlock Block;

static OSTime PlayRecordLastCloseTime;
static s32 PlayRecordLastError;
static BOOL PlayRecordRetry;
static BOOL PlayRecordTerminated;
static BOOL PlayRecordTerminate;
static BOOL PlayRecordError;
static BOOL PlayRecordGet;
static s32 PlayRecordState = PLAY_RECORD_STATE_STOPPED;

// Stamps the record with the current time and renews its checksum. Both users expand it in place and the
// linker dropped the function itself: only its being the first to use PlayRecord shows in the dol
// (PlayRecord lies first in .bss, ahead of the alarm the code reaches first).
void UpdatePlayRecord(void)
{
	u32* word;
	u32 checkSum;
	u32 i;

	PlayRecord.lastTime = OSGetTime();
	checkSum            = 0;
	word                = (u32*)&PlayRecord + 1;
	for (i = 0; i < sizeof(OSPlayRecord) / sizeof(u32) - 1; i++, word++) {
		checkSum += *word;
	}
	PlayRecord.checkSum = checkSum;
}

static void PlayRecordAlarmCallback(OSAlarm* alarm, OSContext* context)
{
	PlayRecordCallback(NAND_RESULT_OK, NULL);
}

static void PlayRecordCallback(s32 result, NANDCommandBlock* block)
{
	s32 error = NAND_RESULT_OK;

	PlayRecordLastError = result;

	if (PlayRecordTerminate) {
		PlayRecordTerminated = TRUE;
		return;
	}

	if (!PlayRecordRetry) {
		switch (PlayRecordState) {
		case PLAY_RECORD_STATE_START:
			PlayRecordState = PLAY_RECORD_STATE_OPEN;
			break;
		case PLAY_RECORD_STATE_OPEN:
			if (result == NAND_RESULT_MAXFD) {
				PlayRecordRetry = TRUE;
				OSCreateAlarm(&PlayRecordAlarm);
				OSSetAlarm(&PlayRecordAlarm, OSSecondsToTicks((OSTime)1), PlayRecordAlarmCallback);
				return;
			}
			if (result == NAND_RESULT_OK) {
				if (!PlayRecordGet) {
					PlayRecordState = PLAY_RECORD_STATE_READ;
				} else {
					PlayRecordState = PLAY_RECORD_STATE_WAIT;
				}
			} else {
				PlayRecordError = TRUE;
				PlayRecordState = PLAY_RECORD_STATE_END;
				return;
			}
			break;
		case PLAY_RECORD_STATE_READ:
			if (result == sizeof(OSPlayRecord)) {
				PlayRecordGet           = TRUE;
				PlayRecordLastCloseTime = PlayRecord.lastTime;
				PlayRecordState         = PLAY_RECORD_STATE_SEEK;
			} else {
				PlayRecordError = TRUE;
				PlayRecordState = PLAY_RECORD_STATE_CLOSE;
			}
			break;
		case PLAY_RECORD_STATE_SEEK:
			if (result == NAND_RESULT_OK) {
				PlayRecordState = PLAY_RECORD_STATE_WAIT;
			} else {
				PlayRecordError = TRUE;
				PlayRecordState = PLAY_RECORD_STATE_CLOSE;
			}
			break;
		case PLAY_RECORD_STATE_WAIT:
			PlayRecordState = PLAY_RECORD_STATE_WRITE;
			break;
		case PLAY_RECORD_STATE_WRITE:
			if (result == sizeof(OSPlayRecord)) {
				if (OSGetTime() - PlayRecordLastCloseTime > OSSecondsToTicks((OSTime)300)) {
					PlayRecordState = PLAY_RECORD_STATE_CLOSE;
				} else {
					PlayRecordState = PLAY_RECORD_STATE_SEEK;
				}
			} else {
				PlayRecordError = TRUE;
				PlayRecordState = PLAY_RECORD_STATE_CLOSE;
			}
			break;
		case PLAY_RECORD_STATE_CLOSE:
			if (PlayRecordError) {
				PlayRecordState = PLAY_RECORD_STATE_END;
				return;
			}
			if (result == NAND_RESULT_OK) {
				PlayRecordLastCloseTime = PlayRecord.lastTime;
				PlayRecordState         = PLAY_RECORD_STATE_OPEN;
			} else {
				PlayRecordState = PLAY_RECORD_STATE_END;
				PlayRecordError = TRUE;
				return;
			}
			break;
		default:
			PlayRecordState = PLAY_RECORD_STATE_END;
			PlayRecordError = TRUE;
			return;
		}
	}

	PlayRecordRetry = FALSE;

	switch (PlayRecordState) {
	case PLAY_RECORD_STATE_OPEN:
		error = NANDOpenAsync("/title/00000001/00000002/data/play_rec.dat", &FileInfo, NAND_ACCESS_RW, PlayRecordCallback, &Block);
		break;
	case PLAY_RECORD_STATE_READ:
		error = NANDReadAsync(&FileInfo, &PlayRecord, sizeof(OSPlayRecord), PlayRecordCallback, &Block);
		break;
	case PLAY_RECORD_STATE_SEEK:
		error = NANDSeekAsync(&FileInfo, 0, 0, PlayRecordCallback, &Block);
		break;
	case PLAY_RECORD_STATE_WAIT:
		OSCreateAlarm(&PlayRecordAlarm);
		OSSetAlarm(&PlayRecordAlarm, OSSecondsToTicks((OSTime)60), PlayRecordAlarmCallback);
		break;
	case PLAY_RECORD_STATE_WRITE:
		UpdatePlayRecord();
		error = NANDWriteAsync(&FileInfo, &PlayRecord, sizeof(OSPlayRecord), PlayRecordCallback, &Block);
		break;
	case PLAY_RECORD_STATE_CLOSE:
		error = NANDCloseAsync(&FileInfo, PlayRecordCallback, &Block);
		break;
	}

	if (error != NAND_RESULT_OK) {
		if (error == NAND_RESULT_BUSY) {
			OSCreateAlarm(&PlayRecordAlarm);
			OSSetAlarm(&PlayRecordAlarm, OSSecondsToTicks((OSTime)1), PlayRecordAlarmCallback);
			PlayRecordRetry = TRUE;
		} else {
			PlayRecordError = TRUE;
			switch (PlayRecordState) {
			case PLAY_RECORD_STATE_READ:
			case PLAY_RECORD_STATE_SEEK:
			case PLAY_RECORD_STATE_WRITE:
				PlayRecordState = PLAY_RECORD_STATE_CLOSE;
				error           = NANDCloseAsync(&FileInfo, PlayRecordCallback, &Block);
				if (error == NAND_RESULT_BUSY) {
					PlayRecordRetry = TRUE;
					OSCreateAlarm(&PlayRecordAlarm);
					OSSetAlarm(&PlayRecordAlarm, OSSecondsToTicks((OSTime)1), PlayRecordAlarmCallback);
				}
				break;
			default:
				PlayRecordState = PLAY_RECORD_STATE_END;
				break;
			}
		}
	}

	PlayRecordLastError = error;
}

void __OSStartPlayRecord(void)
{
	if (NANDInit() == NAND_RESULT_OK) {
		PlayRecordTerminate  = FALSE;
		PlayRecordGet        = FALSE;
		PlayRecordState      = PLAY_RECORD_STATE_START;
		PlayRecordError      = FALSE;
		PlayRecordRetry      = FALSE;
		PlayRecordTerminated = FALSE;
		PlayRecordLastError  = NAND_RESULT_OK;
		PlayRecordCallback(NAND_RESULT_OK, NULL);
	}
}

void __OSStopPlayRecord(void)
{
	BOOL enabled;
	OSTime start;

	enabled             = OSDisableInterrupts();
	PlayRecordTerminate = TRUE;

	if (PlayRecordState == PLAY_RECORD_STATE_END || PlayRecordState == PLAY_RECORD_STATE_START
	    || PlayRecordState == PLAY_RECORD_STATE_STOPPED || PlayRecordState == PLAY_RECORD_STATE_TIMEOUT) {
		OSRestoreInterrupts(enabled);
	} else if (PlayRecordState == PLAY_RECORD_STATE_WAIT) {
		OSCancelAlarm(&PlayRecordAlarm);
		OSRestoreInterrupts(enabled);

		UpdatePlayRecord();
		NANDWrite(&FileInfo, &PlayRecord, sizeof(OSPlayRecord));
		NANDClose(&FileInfo);
	} else {
		if (PlayRecordRetry) {
			OSCancelAlarm(&PlayRecordAlarm);
			OSRestoreInterrupts(enabled);
		} else {
			OSRestoreInterrupts(enabled);
			start = OSGetTime();
			while (TRUE) {
				if (PlayRecordTerminated) {
					break;
				}
				if (OSGetTime() - start > OSMillisecondsToTicks(500)) {
					PlayRecordState = PLAY_RECORD_STATE_TIMEOUT;
					return;
				}
			}
		}

		switch (PlayRecordState) {
		case PLAY_RECORD_STATE_READ:
		case PLAY_RECORD_STATE_SEEK:
		case PLAY_RECORD_STATE_WRITE:
			NANDClose(&FileInfo);
			break;
		case PLAY_RECORD_STATE_OPEN:
			if (PlayRecordLastError == NAND_RESULT_OK && !PlayRecordRetry) {
				NANDClose(&FileInfo);
			}
			break;
		case PLAY_RECORD_STATE_CLOSE:
			if (PlayRecordRetry) {
				NANDClose(&FileInfo);
			}
			break;
		}
	}

	PlayRecordState = PLAY_RECORD_STATE_STOPPED;
}
