#include "revolution/hw_regs.h"
#include "revolution/dvd/dvd_broadway.h"
#include "Dolphin/os.h"
#include "Dolphin/gx.h"

typedef struct OSIOSRev {
	u8 reserved;
	u8 major;
	u8 minor;
	u8 micro;
	u8 month;
	u8 date;
	u16 year;
} OSIOSRev;

// other files of the library
extern const char* const __DVDDeviceErrorMessage[7];
void __DVDShowFatalMessage(void);
void __OSGetIOSRev(OSIOSRev* rev);
u32 OSGetPhysicalMem2Size(void);
u8 SCGetLanguage(void);
u16 OSSetFontEncode(u16 encode);
void OSFatal(GXColor fg, GXColor bg, const char* msg);

// low memory: the kind of disc drive the console was built with
#define OS_DEVICE_CHECK_CODE (*(u8*)0x8000319C)

static u32 CheckBuffer[8] ATTRIBUTE_ALIGN(32);
static volatile u32 lowIntType;
static volatile BOOL lowDone = TRUE;
// the colour of the message text; retail keeps it in .sdata, so it is a variable and not a literal
static GXColor MessageColor = { 0xFF, 0xFF, 0xFF, 0x00 };

static void lowCallback(u32 intType)
{
	lowIntType = intType;
	lowDone    = TRUE;
}

static void ShowMessage(void)
{
	const char* message;
	const char* const* messages;
	GXColor fg = MessageColor;
	GXColor bg = { 0x00, 0x00, 0x00, 0x00 };

	if (SCGetLanguage() == 0) {
		OSSetFontEncode(1);
	} else {
		OSSetFontEncode(0);
	}

	messages = __DVDDeviceErrorMessage;
	if (SCGetLanguage() > 6) {
		message = messages[1];
	} else {
		message = messages[SCGetLanguage()];
	}

	OSFatal(fg, bg, message);
}

BOOL __DVDCheckDevice(void)
{
	OSIOSRev iosRev;
	u32 offset = 0x460A0000;
	u32 error;

	if (OSGetPhysicalMem2Size() == 0x8000000) {
		return TRUE;
	}

	__OSGetIOSRev(&iosRev);
	if (iosRev.major < 30 || iosRev.major >= 254) {
		return TRUE;
	}

	if (OS_DEVICE_CHECK_CODE == 0x81) {
		offset = 0x7ED40000;
	}

	lowDone = FALSE;
	DVDLowUnencryptedRead(CheckBuffer, sizeof(CheckBuffer), offset, lowCallback);
	while (!lowDone) { }

	switch (lowIntType) {
	case DVD_INTTYPE_TC:
		goto deviceError;
	case DVD_INTTYPE_DE:
		break;
	default:
		goto fatal;
	}

	lowDone = FALSE;
	DVDLowRequestError(lowCallback);
	while (!lowDone) { }
	error = DVDLowGetImmBufferReg();

	switch (lowIntType) {
	case DVD_INTTYPE_TC:
		break;
	default:
		goto fatal;
	}

	if (DVDLowGetImmBufferReg() & 0xFF000000) {
		goto driveError;
	}

	switch (DVDLowGetImmBufferReg() & 0x00FFFFFF) {
	case 0x052100:
		break;
	default:
		goto deviceError;
	}

	lowDone = FALSE;
	DVDLowReportKey(CheckBuffer, 0x40000, 0, lowCallback);
	while (!lowDone) { }

	switch (lowIntType) {
	case DVD_INTTYPE_TC:
		goto deviceError;
	case DVD_INTTYPE_DE:
		break;
	default:
		goto fatal;
	}

	lowDone = FALSE;
	DVDLowRequestError(lowCallback);
	while (!lowDone) { }
	error = DVDLowGetImmBufferReg();

	switch (lowIntType) {
	case DVD_INTTYPE_TC:
		break;
	default:
		goto fatal;
	}

	if (DVDLowGetImmBufferReg() & 0xFF000000) {
		goto driveError;
	}

	switch (DVDLowGetImmBufferReg() & 0x00FFFFFF) {
	case 0x053100:
	case 0x052000:
		break;
	default:
		goto deviceError;
	}

	return TRUE;

deviceError:
	ShowMessage();
	return FALSE;

driveError:
	return FALSE;

fatal:
	__DVDShowFatalMessage();
	return FALSE;
}
