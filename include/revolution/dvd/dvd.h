#ifndef _REVOLUTION_DVD_DVD_H
#define _REVOLUTION_DVD_DVD_H

// Wii variant of Dolphin/dvd.h: the Revolution SDK command block has a volatile state,
// and the library keeps a command log, partition data and a few more entry points.
// It defines the guard macro of the GameCube header, so include it first in a Wii unit.
#ifndef _DOLPHIN_DVD_H
#define _DOLPHIN_DVD_H
#endif

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

/////////// DVD TYPES ///////////
typedef struct DVDCommandBlock DVDCommandBlock;
typedef struct DVDFileInfo DVDFileInfo;

// Callback function types.
typedef void (*DVDCallback)(s32 result, DVDFileInfo* fileInfo);
typedef void (*DVDCBCallback)(s32 result, DVDCommandBlock* block);
typedef void (*DVDLowCallback)(u32 intType);
typedef void (*DVDDoneReadCallback)(s32, DVDFileInfo*);
typedef void (*DVDOptionalCommandChecker)(DVDCommandBlock* block, DVDLowCallback callback);

typedef struct DVDDriveInfo {
	u16 revisionLevel; // _00
	u16 deviceCode;    // _02
	u32 releaseDate;   // _04
	u8 padding[24];    // _08
} DVDDriveInfo;

// Struct for DVD information (size 0x20)
typedef struct DVDDiskID {
	char gameName[4]; // _00
	char company[2];  // _04
	u8 diskNumber;    // _06
	u8 gameVersion;   // _07
	u8 streaming;     // _08
	u8 streamBufSize; // _09, default = 0
	u8 padding[22];   // _0A, all 0s
} DVDDiskID;

// Struct for command information (size 0x30).
struct DVDCommandBlock {
	DVDCommandBlock* next;  // _00
	DVDCommandBlock* prev;  // _04
	u32 command;            // _08
	volatile s32 state;     // _0C
	u32 offset;             // _10
	u32 length;             // _14
	void* addr;             // _18
	u32 currTransferSize;   // _1C
	u32 transferredSize;    // _20
	DVDDiskID* id;          // _24
	DVDCBCallback callback; // _28
	void* userData;         // _2C
};

// Struct for file information (size 0x3C).
// NB: we had this as DVDPlayer previously.
struct DVDFileInfo {
	DVDCommandBlock cBlock; // _00
	u32 startAddr;          // _30
	u32 length;             // _34
	DVDCallback callback;   // _38
};

// Struct for directory information (size 0xC).
typedef struct DVDDir {
	u32 entryNum; // _00
	u32 location; // _04
	u32 next;     // _08
} DVDDir;

// Struct for directory entries (size 0xC).
typedef struct DVDDirEntry {
	u32 entryNum; // _00
	BOOL isDir;   // _04
	char* name;   // _08
} DVDDirEntry;

// Struct for handing queues.
typedef struct DVDQueue DVDQueue;

struct DVDQueue {
	DVDQueue* mHead; // _00
	DVDQueue* mTail; // _04
};

// DVD Boot information instructions.
// Struct 1.
typedef struct DVDBB1 {
	u32 appLoaderLength;  // _00
	void* appLoaderFunc1; // _04
	void* appLoaderFunc2; // _08
	void* appLoaderFunc3; // _0C
} DVDBB1;

// Struct 2.
typedef struct DVDBB2 {
	u32 bootFilePosition; // _00
	u32 FSTPosition;      // _04
	u32 FSTLength;        // _08
	u32 FSTMaxLength;     // _0C
	void* FSTAddress;     // _10
	u32 userPosition;     // _14
	u32 userLength;       // _18
	u32 reserved_1C;      // _1C
} DVDBB2;

//////////////////////////////////

///////// DVD FUNCTIONS //////////
// Basic DVD functions.
void DVDInit();
BOOL DVDOpen(char* filename, DVDFileInfo* fileInfo);
BOOL DVDFastOpen(s32 entryNum, DVDFileInfo* fileInfo);
s32 DVDReadPrio(DVDFileInfo* fileInfo, void* addr, s32 length, s32 offset, s32 prio);
BOOL DVDReadAsyncPrio(DVDFileInfo* fileInfo, void* addr, s32 length, s32 offset, DVDCallback callback, s32 prio);
BOOL DVDReadAbsAsyncPrio(DVDCommandBlock* block, void* addr, s32 length, s32 offset, DVDCBCallback callback, s32 prio);
BOOL DVDClose(DVDFileInfo* fileInfo);

void DVDResume();
void DVDReset();

BOOL DVDCancelAsync(DVDCommandBlock* block, DVDCBCallback callback);
s32 DVDCancel(DVDCommandBlock* block);

s32 DVDChangeDisk(DVDCommandBlock* block, DVDDiskID* id);
BOOL DVDChangeDiskAsync(DVDCommandBlock* block, DVDDiskID* id, DVDCBCallback callback);

// Status functions.
s32 DVDGetCommandBlockStatus(const DVDCommandBlock* block);
s32 DVDGetDriveStatus();
BOOL DVDSetAutoInvalidation(BOOL doAutoInval);
void* DVDGetFSTLocation();

// DVD Dir functions.
BOOL DVDOpenDir(char* dirName, DVDDir* dir);
BOOL DVDReadDir(DVDDir* dir, DVDDirEntry* dirEntry);
BOOL DVDCloseDir(DVDDir* dir);
BOOL DVDGetCurrentDir(char* path, u32 maxLength);
BOOL DVDChangeDir(char* dirName);
s32 DVDConvertPathToEntrynum(char* path);

// Other disk functions.
s32 DVDGetTransferredSize(DVDFileInfo* fileInfo);
DVDDiskID* DVDGetCurrentDiskID();
BOOL DVDCompareDiskID(DVDDiskID* id1, DVDDiskID* id2);
DVDLowCallback DVDLowClearCallback();

BOOL DVDCancelStreamAsync(DVDCommandBlock* block, DVDCBCallback callback);

BOOL DVDCheckDisk();

// Unused/inlined in P2.
void DVDPause();
s32 DVDSeekPrio(DVDFileInfo* fileInfo, s32 offset, s32 prio);
BOOL DVDSeekAsyncPrio(DVDFileInfo* fileInfo, s32 offset, DVDCallback callback, s32 prio);
s32 DVDGetFileInfoStatus(DVDFileInfo* fileInfo);
BOOL DVDFastOpenDir(s32 entryNum, DVDDir* dir);
BOOL DVDCancelAllAsync(DVDCBCallback callback);
s32 DVDCancelAll();
void DVDDumpWaitingQueue();

//////////////////////////////////

////// USEFUL DVD DEFINES ////////
// Macro for reading.
#define DVDReadAsync(fileInfo, addr, length, offset, callback) DVDReadAsyncPrio((fileInfo), (addr), (length), (offset), (callback), 2)

// Minimum transfer size.
#define DVD_MIN_TRANSFER_SIZE 32

// DVD states.
#define DVD_STATE_FATAL_ERROR   -1
#define DVD_STATE_END           0
#define DVD_STATE_BUSY          1
#define DVD_STATE_WAITING       2
#define DVD_STATE_COVER_CLOSED  3
#define DVD_STATE_NO_DISK       4
#define DVD_STATE_COVER_OPEN    5
#define DVD_STATE_WRONG_DISK    6
#define DVD_STATE_MOTOR_STOPPED 7
#define DVD_STATE_PAUSING       8
#define DVD_STATE_IGNORED       9
#define DVD_STATE_CANCELED      10
#define DVD_STATE_RETRY         11

// File info states.
#define DVD_FILEINFO_READY 0
#define DVD_FILEINFO_BUSY  1

// DVD results.
#define DVD_RESULT_GOOD        0
#define DVD_RESULT_FATAL_ERROR -1
#define DVD_RESULT_IGNORED     -2
#define DVD_RESULT_CANCELED    -3

#define DVD_AIS_SUCCESS 0

//////////////////////////////////

/////// WII ADDITIONS ///////////
// One of the last five commands sent to the drive (size 0x14).
typedef struct DVDCommandInfo {
	u32 command; // _00
	u32 offset;  // _04
	u32 length;  // _08
	u32 intType; // _0C, what the drive answered
	u32 tick;    // _10
} DVDCommandInfo;

// Record kept for the fatal error screen and the NAND error log (size 0x80).
typedef struct DVDErrorInfo {
	char gameName[4];              // _00
	u8 diskNumber;                 // _04
	u8 gameVersion;                // _05
	u8 reserved0[2];               // _06
	u32 error;                     // _08
	u32 dateTime;                  // _0C
	u32 status;                    // _10
	u32 control;                   // _14, drive control register
	u32 nextOffset;                // _18
	DVDCommandInfo lastCommand[5]; // _1C
} DVDErrorInfo;

// First words of the partition table at disc offset 0x40000.
typedef struct DVDGameTOC {
	u32 numGamePartitions; // _00
	u32 partInfoOffset;    // _04
} DVDGameTOC;

// Entry of the partition table.
typedef struct DVDPartitionInfo {
	u32 offset; // _00, in words
	u32 type;   // _04
} DVDPartitionInfo;

// Buffer the open-partition commands 41 and 42 work on (block->addr).
typedef struct DVDPartitionParams {
	u8 ticket[0x2C0];    // _0000
	u8 ticketView[0xE0]; // _02C0
	u32 numTmdBytes;     // _03A0
	u8 reserved0[0x1C];  // _03A4
	u8 tmd[0x4A00];      // _03C0
	u32 numCertBytes;    // _4DC0
	u8 reserved1[0x1C];  // _4DC4
	u8 certs[0x1000];    // _4DE0
	u32 dataWordOffset;  // _5DE0
	u8 reserved2[0x1C];  // _5DE4
	u8 h3Hash[0x18000];  // _5E00
} DVDPartitionParams;

typedef void (*DVDCommandCheckerCallback)(u32 intType);
typedef void (*DVDCommandChecker)(DVDCommandBlock* block, DVDCommandCheckerCallback callback);

extern DVDErrorInfo __ErrorInfo;

// dvderror.c
void __DVDStoreErrorCode(u32 error, DVDCBCallback callback);

// dvdFatal.c
BOOL DVDSetAutoFatalMessaging(BOOL enable);
BOOL __DVDGetAutoFatalMessaging(void);
void __DVDPrintFatalMessage(void);

// dvdfs.c
void __DVDFSInit(void);

// dvd.c
BOOL DVDInquiryAsync(DVDCommandBlock* block, DVDDriveInfo* info, DVDCBCallback callback);
BOOL DVDCheckDiskAsync(DVDCommandBlock* block, DVDCBCallback callback);
void __DVDPrepareResetAsync(DVDCBCallback callback);
void __DVDPrepareReset(void);
u32 __DVDGetCoverStatus(void);

#ifdef __cplusplus
};
#endif // ifdef __cplusplus

#endif
