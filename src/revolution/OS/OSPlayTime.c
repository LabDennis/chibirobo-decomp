#include "revolution/hw_regs.h"
#include "revolution/os/OSAlarm.h"
#include "revolution/os/OSLaunch.h"

#include "Dolphin/ai.h"
#include "Dolphin/os.h"
#include "Dolphin/vi.h"

// What the thread that ends an expired title needs to fade the sound out.
typedef struct OSExpireWork {
	s16 buffer[2][0x120];    // _000
	u32 index;               // _480
	s16* last;               // _484
	u32 count;               // _488
	s16 left;                // _48C
	s16 right;               // _48E
	AIDCallback oldCallback; // _490
} OSExpireWork;

// other files of the library
void __OSSetVIForceDimming(BOOL enable, u32 y, u32 uv);
void __OSHotResetForError(void);
void OSReturnToMenu(void);

OSAlarm __OSExpireAlarm;
OSTime __OSExpireTime;
void (*__OSExpireCallback)(void);
BOOL __OSExpireSetExpiredFlag;
static OSExpireWork* Work;

BOOL OSPlayTimeIsLimited(void)
{
	return __OSExpireTime != 0;
}

static void FadeOutAudioCallback(void)
{
	s16* buffer;
	s16* p;
	u32 length;
	u32 n;
	f32 rate;

	if (Work->oldCallback) {
		Work->oldCallback();
	}

	if (Work->count == 0) {
		Work->last = (s16*)OSPhysicalToCached(AIGetDMAStartAddr());
	}

	if (Work->count == 1) {
		DCInvalidateRange(Work->last, 4);
		Work->left  = *Work->last++;
		Work->right = *Work->last;
	}

	if (Work->count >= 1) {
		buffer = Work->buffer[Work->index];
		p      = buffer;
		length = AIGetDMALength();
		rate   = 0.995f;
		for (n = length; n != 0; n -= 4) {
			*p++ = Work->left;
			*p++ = Work->right;
			Work->left *= rate;
			Work->right *= rate;
		}

		DCFlushRange(buffer, length);
		AIInitDMA((u32)buffer, length);
		Work->index++;
		Work->index %= 2;
	}

	Work->count++;
}

BOOL __OSWriteExpiredFlag(void)
{
	NANDFileInfo file;
	u64 titleId[4] ATTRIBUTE_ALIGN(32);
	s32 err     = 0;
	BOOL opened = FALSE;

	err = NANDPrivateCreate("/shared2/expired", 0x3F, 0);
	if (err != NAND_RESULT_OK && err != NAND_RESULT_EXISTS) {
		goto out;
	}

	err = NANDPrivateOpen("/shared2/expired", &file, NAND_ACCESS_WRITE);
	if (err != NAND_RESULT_OK) {
		goto out;
	}
	opened = TRUE;

	err = ESP_InitLib();
	if (err != 0) {
		goto out;
	}

	memset(titleId, 0, sizeof(titleId));
	err = ESP_GetTitleId(titleId);
	if (err != 0) {
		goto out;
	}

	err = NANDWrite(&file, titleId, sizeof(titleId));
	if (err < 0) {
		goto out;
	}

	if (err != 32) {
		err = NAND_RESULT_INVALID;
	} else {
		err = NAND_RESULT_OK;
	}

out:
	if (opened) {
		NANDClose(&file);
	}

	if (err == NAND_RESULT_OK) {
		return TRUE;
	}

	return FALSE;
}

BOOL __OSWriteExpiredFlagIfSet(void)
{
	if (__OSExpireSetExpiredFlag) {
		return __OSWriteExpiredFlag();
	}

	return FALSE;
}

static void* ExpireThread(void* arg)
{
	OSExpireWork work ATTRIBUTE_ALIGN(32);
	BOOL enabled;
	u32 i;
	u32 dimming;

	Work = &work;
	memset(Work, 0, sizeof(OSExpireWork));
	Work->oldCallback = AIRegisterDMACallback(FadeOutAudioCallback);

	for (i = 0; i < 20; i++) {
		dimming = i / 5 + 1;
		if (dimming > 7) {
			dimming = 7;
		}

		VIWaitForRetrace();
		__OSSetVIForceDimming(TRUE, dimming, dimming);
	}

	AIRegisterDMACallback(NULL);
	VISetBlack(TRUE);
	VIFlush();

	enabled = OSDisableInterrupts();
	if (__OSExpireSetExpiredFlag) {
		__OSWriteExpiredFlag();
	}
	OSRestoreInterrupts(enabled);

	OSReturnToMenu();
	return NULL;
}

static void __OSPlayTimeAlarmExpired(OSAlarm* alarm, OSContext* context)
{
	OSThread* thread;
	u8* base;

	if (__OSExpireCallback) {
		__OSExpireCallback();
		return;
	}

	for (thread = __OSActiveThreadQueue.head; thread; thread = thread->linkActive.next) {
		OSSuspendThread(thread);
	}

	// the thread and its stack sit at the top of MEM2
	base   = (u8*)(*(u32*)OSPhysicalToCached(0x3128)) - (sizeof(OSThread) + 0x1008);
	thread = (OSThread*)base;
	if (!OSCreateThread(thread, ExpireThread, NULL, base + (sizeof(OSThread) + 0x1008), 0x1000, 0, 0)) {
		__OSHotResetForError();
	}

	OSResumeThread(thread);
}

s32 __OSGetPlayTime(ESTicketView* ticket, s32* type, u32* remaining)
{
	ESTicketView aligned ATTRIBUTE_ALIGN(32);
	ESLimit consumption[8] ATTRIBUTE_ALIGN(32);
	u32 count ATTRIBUTE_ALIGN(32);
	s32 err;
	u32 i;
	s32 last;

	count = 0;
	last  = 0;

	if ((u32)ticket & 31) {
		memcpy(&aligned, ticket, sizeof(ESTicketView));
		ticket = &aligned;
	}

	err = ESP_GetConsumption(ticket->ticketId, NULL, &count);
	if (err > 0) {
		goto out;
	} else {
		switch (err) {
		case 0:
			break;
		default:
			goto out;
		}
	}

	if (count != 0) {
		err = ESP_GetConsumption(ticket->ticketId, consumption, &count);
		switch (err) {
		case 0:
			break;
		default:
			goto out;
		}
	}

out:
	if (err == 0) {
		for (i = 0; i < 8; i++) {
			if (ticket->limits[i].code == 1) {
				*type = 1;
				if (count == 0) {
					*remaining = ticket->limits[i].limit;
				} else if (consumption[i].limit >= ticket->limits[i].limit) {
					*remaining = 0;
				} else {
					*remaining = ticket->limits[i].limit - consumption[i].limit;
				}
				return err;
			} else if (ticket->limits[i].code != 0) {
				last = i + 1;
			}
		}

		if (last == 0) {
			*type      = 0;
			*remaining = -1;
		} else {
			last--;
			if (ticket->limits[last].code == 4) {
				*type      = 4;
				*remaining = ticket->limits[last].limit;
				if (count != 0) {
					*remaining -= consumption[last].limit;
				}
			} else {
				*type = 9;
			}
		}
	}

	return err;
}

void __OSInitPlayTime(void)
{
	ESTicketView ticket ATTRIBUTE_ALIGN(32);
	u32 remaining;
	s32 type;
	s32 err;

	__OSExpireTime           = 0;
	__OSExpireCallback       = NULL;
	__OSExpireSetExpiredFlag = TRUE;

	err = ESP_InitLib();
	switch (err) {
	case 0:
		break;
	default:
		goto out;
	}

	err = ESP_DiGetTicketView(NULL, &ticket);
	if (err != ES_ERR_INVALID) {
		switch (err) {
		case 0:
			err = __OSGetPlayTime(&ticket, &type, &remaining);
			break;
		default:
			break;
		}
	}

	if (err != 0) {
		switch (err) {
		case ES_ERR_INVALID:
			goto out;
		default:
			goto out;
		}
	}

	if (type == 0) {
	} else if (type == 1) {
		if (remaining == 0) {
			OSPanic(__FILE__, 0x2E1, "Expired");
		}

		OSCreateAlarm(&__OSExpireAlarm);
		OSSetAlarm(&__OSExpireAlarm, OSSecondsToTicks((OSTime)remaining + 20), __OSPlayTimeAlarmExpired);
		__OSExpireTime = __OSExpireAlarm.fire;
		OSReport("PlayTime: %d seconds left\n", remaining);
	}

out:
	ESP_CloseLib();
}
