#include "revolution/hw_regs.h"

#include "Dolphin/dvd.h"
#include "Dolphin/os.h"
#include "revolution/nand.h"

// low-level driver
BOOL DVDLowPrepareStatusRegister(DVDLowCallback callback);
BOOL DVDLowPrepareControlRegister(DVDLowCallback callback);
u32 DVDLowGetStatusRegister(void);
u32 DVDLowGetControlRegister(void);

#define DVD_ERROR_RECORD_SIZE sizeof(DVDErrorInfo)
#define DVD_ERROR_RECORDS     7

// shortcut: "revolution/dvd/dvd.h" has these two as well, but without the control register at 0x14;
// use that header instead of this copy once its DVDErrorInfo names the field.
typedef struct DVDCommandInfo {
	u32 command; // _00
	u32 offset;  // _04
	u32 length;  // _08
	u32 intType; // _0C
	u32 tick;    // _10
} DVDCommandInfo;

// one record of /shared2/test2/dvderror.dat
typedef struct DVDErrorInfo {
	char gameName[4];              // _00
	u8 diskNumber;                 // _04
	u8 gameVersion;                // _05
	u8 reserved0[2];               // _06
	u32 error;                     // _08
	u32 dateTime;                  // _0C, seconds
	u32 status;                    // _10, drive status register
	u32 control;                   // _14, drive control register
	u32 nextOffset;                // _18, record to write next
	DVDCommandInfo lastCommand[5]; // _1C, filled by dvd.c
} DVDErrorInfo;

static NANDFileInfo NandInfo;
static NANDCommandBlock NandCb;
DVDErrorInfo __ErrorInfo ATTRIBUTE_ALIGN(32);
static DVDErrorInfo __FirstErrorInfo ATTRIBUTE_ALIGN(32);

static DVDCBCallback Callback;
static u32 NextOffset;
static BOOL FileExists;

static void cbForNandClose(s32 result, NANDCommandBlock* block)
{
	if (Callback) {
		(*Callback)((result == NAND_RESULT_OK) ? 1 : 2, NULL);
	}
}

static void cbForNandWrite(s32 result, NANDCommandBlock* block)
{
	if (NANDCloseAsync(&NandInfo, cbForNandClose, &NandCb) != NAND_RESULT_OK) {
		cbForNandClose(-1, block);
	}
}

static void cbForNandSeek2(s32 result, NANDCommandBlock* block)
{
	if (result == (NextOffset + 1) * DVD_ERROR_RECORD_SIZE) {
		if (NextOffset == 0) {
			__ErrorInfo.nextOffset = (NextOffset + 1) % DVD_ERROR_RECORDS;
		}
		DCFlushRange(&__ErrorInfo, DVD_ERROR_RECORD_SIZE);
		if (NANDWriteAsync(&NandInfo, &__ErrorInfo, DVD_ERROR_RECORD_SIZE, cbForNandWrite, &NandCb) != NAND_RESULT_OK) {
			cbForNandWrite(-1, block);
		}
	} else if (Callback) {
		(*Callback)(2, NULL);
	}
}

static void cbForNandWrite0(s32 result, NANDCommandBlock* block)
{
	if (result == DVD_ERROR_RECORD_SIZE) {
		if (NANDSeekAsync(&NandInfo, (NextOffset + 1) * DVD_ERROR_RECORD_SIZE, 0, cbForNandSeek2, &NandCb) != NAND_RESULT_OK) {
			cbForNandSeek2(-1, block);
		}
	} else if (Callback) {
		(*Callback)(2, NULL);
	}
}

static void cbForNandSeek1(s32 result, NANDCommandBlock* block)
{
	if (result == DVD_ERROR_RECORD_SIZE) {
		__FirstErrorInfo.nextOffset = (__FirstErrorInfo.nextOffset + 1) % DVD_ERROR_RECORDS;
		if (NANDWriteAsync(&NandInfo, &__FirstErrorInfo, DVD_ERROR_RECORD_SIZE, cbForNandWrite0, &NandCb) != NAND_RESULT_OK) {
			cbForNandWrite0(-1, block);
		}
	} else if (Callback) {
		(*Callback)(2, NULL);
	}
}

static void cbForNandRead(s32 result, NANDCommandBlock* block)
{
	if (result == DVD_ERROR_RECORD_SIZE) {
		NextOffset = __FirstErrorInfo.nextOffset;
		if (NANDSeekAsync(&NandInfo, DVD_ERROR_RECORD_SIZE, 0, cbForNandSeek1, &NandCb) != NAND_RESULT_OK) {
			cbForNandSeek1(-1, block);
		}
	} else {
		__ErrorInfo.nextOffset = 1;
		if (NANDWriteAsync(&NandInfo, &__ErrorInfo, DVD_ERROR_RECORD_SIZE, cbForNandWrite, &NandCb) != NAND_RESULT_OK) {
			cbForNandWrite(-1, block);
		}
	}
}

static void cbForNandSeek0(s32 result, NANDCommandBlock* block)
{
	if (result == 0) {
		NextOffset             = 0;
		__ErrorInfo.nextOffset = 1;
		if (NANDWriteAsync(&NandInfo, &__FirstErrorInfo, DVD_ERROR_RECORD_SIZE, cbForNandWrite0, &NandCb) != NAND_RESULT_OK) {
			cbForNandWrite0(-1, block);
		}
	} else if (Callback) {
		(*Callback)(2, NULL);
	}
}

static void cbForNandSeek(s32 result, NANDCommandBlock* block)
{
	if (result == DVD_ERROR_RECORD_SIZE) {
		if (NANDReadAsync(&NandInfo, &__FirstErrorInfo, DVD_ERROR_RECORD_SIZE, cbForNandRead, &NandCb) != NAND_RESULT_OK) {
			cbForNandRead(-1, block);
		}
	} else {
		if (NANDSeekAsync(&NandInfo, 0, 0, cbForNandSeek0, &NandCb) != NAND_RESULT_OK) {
			cbForNandSeek0(-1, block);
		}
	}
}

static void cbForNandOpen(s32 result, NANDCommandBlock* block)
{
	if (result == NAND_RESULT_OK) {
		if (FileExists) {
			if (NANDSeekAsync(&NandInfo, DVD_ERROR_RECORD_SIZE, 0, cbForNandSeek, &NandCb) != NAND_RESULT_OK) {
				cbForNandSeek(-1, block);
			}
		} else {
			NextOffset             = 0;
			__ErrorInfo.nextOffset = 1;
			if (NANDWriteAsync(&NandInfo, &__FirstErrorInfo, DVD_ERROR_RECORD_SIZE, cbForNandWrite0, &NandCb) != NAND_RESULT_OK) {
				cbForNandWrite0(-1, block);
			}
		}
	} else if (Callback) {
		(*Callback)(2, NULL);
	}
}

static void cbForNandCreate(s32 result, NANDCommandBlock* block)
{
	if (result == NAND_RESULT_OK || result == NAND_RESULT_EXISTS) {
		if (result == NAND_RESULT_EXISTS) {
			FileExists = TRUE;
		}
		if (NANDPrivateOpenAsync("/shared2/test2/dvderror.dat", &NandInfo, NAND_ACCESS_RW, cbForNandOpen, &NandCb) != NAND_RESULT_OK) {
			cbForNandOpen(-1, block);
		}
	} else if (Callback) {
		(*Callback)(2, NULL);
	}
}

static void cbForNandCreateDir(s32 result, NANDCommandBlock* block)
{
	if (result == NAND_RESULT_OK || result == NAND_RESULT_EXISTS) {
		if (NANDPrivateCreateAsync("/shared2/test2/dvderror.dat", 0x3F, 0, cbForNandCreate, &NandCb) != NAND_RESULT_OK) {
			cbForNandCreate(-1, block);
		}
	} else if (Callback) {
		(*Callback)(2, NULL);
	}
}

static void cbForPrepareControlRegister(u32 intType)
{
	if (intType == 1) {
		__ErrorInfo.control = DVDLowGetControlRegister();
	} else {
		__ErrorInfo.control = 0xFFFFFFFF;
	}

	if (NANDPrivateCreateDirAsync("/shared2/test2", 0x3F, 0, cbForNandCreateDir, &NandCb) != NAND_RESULT_OK) {
		cbForNandCreateDir(-1, &NandCb);
	}
}

static void cbForPrepareStatusRegister(u32 intType)
{
	if (intType == 1) {
		__ErrorInfo.status = DVDLowGetStatusRegister();
	} else {
		__ErrorInfo.status = 0xFFFFFFFF;
	}

	if (!DVDLowPrepareControlRegister(cbForPrepareControlRegister) && Callback) {
		(*Callback)(2, NULL);
	}
}

void __DVDStoreErrorCode(u32 error, DVDCBCallback callback)
{
	__ErrorInfo.error    = error;
	__ErrorInfo.dateTime = OSTicksToSeconds(OSGetTime());
	Callback             = callback;
	DVDLowPrepareStatusRegister(cbForPrepareStatusRegister);
}
