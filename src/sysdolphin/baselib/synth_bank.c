#include <dolphin/os.h>
#include <sdk/dolphin/dvd.h>
#include <string.h>
#include <sysdolphin/devcom.h>
#include <sysdolphin/synth.h>

#include <sysdolphin/synth_bank.h>

static inline void rebaseAXAddress(AXAddressPair* pair, u32 base)
{
	u32 address = (((u32)pair->hi << 16) | pair->lo) + base;

	pair->hi = (u16)(address >> 16);
	pair->lo = (u16)address;
}

#define ASSERT_SYNTH(line, cond)             ((cond) ? ((void)0) : __assert(__FILE__, line, #cond))
#define ASSERT_SYNTH_REPORT(line, cond, ...) ((cond) ? (void)0 : (OSReport(__VA_ARGS__), __assert(__FILE__, line, #cond)))

void HSD_SynthSFXLoadNewProc(void);

/**
 * @note Address: 0x801EC2AC
 * @note Size: 0xC
 */
void HSD_AudioSetMallocFreeProc(void* (*mallocProc)(s32), void (*freeProc)(void*))
{
	sSynthMallocProc = mallocProc;
	sSynthFreeProc = freeProc;
}

/**
 * @note Address: 0x801EC2B8
 * @note Size: 0x6C
 */
void* HSD_AudioMalloc(u32 size)
{
	void* p = sSynthMallocProc(size);
	ASSERT_SYNTH_REPORT(0xAE, p, "audio heap overflow. size = %u\n", size);
	return p;
}

/**
 * @note Address: N/A
 * @note Size: 0x28
 */
void HSD_AudioFree(void* ptr)
{
	sSynthFreeProc(ptr);
}

static inline u32 sfxLoadAllocationSize(u32 entryCount, u32 metadataSize)
{
	return OSRoundUp32B(sizeof(HSD_SynthSFXGroup) + entryCount * SFX_ENTRY_METADATA_OFFSET + metadataSize);
}

/**
 * @note Address: 0x801EC324
 * @note Size: 0x430
 */
void HSD_SynthSFXSampleLoadCallback(int requestID, void* context, void* buffer, BOOL cancelled)
{
	HSD_SynthSFXGroup* group = sSynthSfxLoadDestination;
	BOOL enabled;
	s32 i;

	sSynthSfxLoadDestination = NULL;
	if (!sSynthSfxLoadCancelled) {
		const HSD_SynthSFXFileHeader* header = &hsd_SynthSFXLoadBuf.header;
		s32 bankID = sSynthSfxLoadRequests[0].bankID;
		u32 totalSize = sfxLoadAllocationSize(header->entryCount, header->metadataSize);
		u32 aramBase = hsd_SynthSFXBank[bankID];
		u8* metadata = (u8*)group + totalSize - header->metadataSize;
		u8* destination = (u8*)(group + 1);
		HSD_SynthSFXGroup** groupLink = &sSynthSampleGroupsByBank[bankID];

		// Move the disc metadata behind the expanded in-memory entries.
		memmove(metadata + SFX_METADATA_PREFIX_SIZE, group, header->metadataSize - SFX_METADATA_PREFIX_SIZE);
		memcpy(metadata, hsd_SynthSFXLoadBuf.metadataPrefix, SFX_METADATA_PREFIX_SIZE);

		while (*groupLink != NULL) {
			groupLink = &(*groupLink)->next;
		}
		*groupLink = group;
		group->next = NULL;
		group->id = sSynthSfxLoadRequests[0].entrynum;
		group->araddr = aramBase;
		group->arsize = header->aramSize;
		group->base = header->firstId;
		group->count = header->entryCount;

		// AX uses nibble addresses for ADPCM samples.
		aramBase *= 2;
		for (i = 0; i < group->count; i++) {
			HSD_SynthSFXEntry* entry = (void*)destination;
			u32 sampleCount = *(const u32*)metadata;
			u32 sampleSize = sampleCount * sizeof(HSD_SynthSample);
			s32 id = group->base + i;
			u32 sampleIndex;

			memmove(destination + SFX_ENTRY_METADATA_OFFSET, metadata, sampleSize + SFX_ENTRY_METADATA_HEADER_SIZE);
			for (sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
				AXPBADDR* addr = &entry->sample[sampleIndex].addr;

				rebaseAXAddress(&addr->loopAddress, aramBase);
				rebaseAXAddress(&addr->endAddress, aramBase);
				rebaseAXAddress(&addr->currentAddress, aramBase);
			}
			entry->id = id;
			entry->next = sSynthSfxIdBuckets[id & (ARRAY_SIZE(sSynthSfxIdBuckets) - 1)];
			sSynthSfxIdBuckets[id & (ARRAY_SIZE(sSynthSfxIdBuckets) - 1)] = entry;
			metadata += SFX_ENTRY_METADATA_HEADER_SIZE + sampleSize;
			destination += SFX_ENTRY_SAMPLE_OFFSET + sampleSize;
		}
		if (sSynthSfxLoadRequests[0].callback != NULL) {
			sSynthSfxLoadRequests[0].callback(sSynthSfxLoadRequests[0].entrynum, sSynthSfxLoadRequests[0].arg);
		}
		hsd_SynthSFXBank[bankID] += header->aramSize;
	} else {
		if (group != NULL) {
			HSD_AudioFree(group);
		}
		sSynthSfxLoadCancelled = FALSE;
	}
	enabled = OSDisableInterrupts();
	sSynthSfxLoadQueueLength -= 1;
	for (i = 0; i < sSynthSfxLoadQueueLength; i++) {
		sSynthSfxLoadRequests[i] = sSynthSfxLoadRequests[i + 1];
	}
	HSD_SynthSFXLoadNewProc();
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EC754
 * @note Size: 0x17C
 */
void HSD_SynthSFXHeaderLoadCallback(int requestID, void* context, void* buffer, BOOL cancelled)
{
	s32 metadataSize;

	if (!sSynthSfxLoadCancelled) {
		s32 bankID = sSynthSfxLoadRequests[0].bankID;

		if (hsd_SynthSFXBankHead[bankID + 1] - hsd_SynthSFXBank[bankID] < hsd_SynthSFXLoadBuf.header.aramSize) {
			OSReport("Can't load SFX file; bank(id=%d) buffer overflow.\n", sSynthSfxLoadRequests[0].bankID);
			// Preserve the original diagnostic text.
			__assert(__FILE__, 0x14F, "hsd_SynthSFXBankHead[bankID + 1] - hsd_SynthSFXBank[bankID] >= hsd_SynthSFXLoadBuf[1]");
		}

		metadataSize = hsd_SynthSFXLoadBuf.header.metadataSize;
		sSynthSfxLoadDestination = HSD_AudioMalloc(sfxLoadAllocationSize(hsd_SynthSFXLoadBuf.header.entryCount, metadataSize));
		sSynthLoadRequestIds[1]
		    = HSD_DevComRequest(sSynthSfxLoadRequests[0].entrynum, sizeof(hsd_SynthSFXLoadBuf), (u32)sSynthSfxLoadDestination,
		                        OSRoundUp32B(metadataSize - SFX_METADATA_PREFIX_SIZE), HSD_DEVCOM_DVD_TO_MRAM, 1, NULL, NULL);
		sSynthLoadRequestIds[0]
		    = HSD_DevComRequest(sSynthSfxLoadRequests[0].entrynum, OSRoundUp32B(metadataSize + SFX_FILE_HEADER_SIZE),
		                        hsd_SynthSFXBank[sSynthSfxLoadRequests[0].bankID], hsd_SynthSFXLoadBuf.header.aramSize, HSD_DEVCOM_DVD_TO_ARAM, 1,
		                        HSD_SynthSFXSampleLoadCallback, NULL);
		return;
	}
	sSynthSfxLoadDestination = NULL;
	HSD_SynthSFXSampleLoadCallback(0, NULL, NULL, FALSE);
}

/**
 * @note Address: 0x801EC8D0
 * @note Size: 0x7C
 */
void HSD_SynthSFXLoadNewProc(void)
{
	if (sSynthSfxLoadQueueLength != 0) {
		BOOL enabled = OSDisableInterrupts();
		sSynthLoadRequestIds[0]
		    = HSD_DevComRequest(sSynthSfxLoadRequests[0].entrynum, 0, (u32)&hsd_SynthSFXLoadBuf, sizeof(hsd_SynthSFXLoadBuf),
		                        HSD_DEVCOM_DVD_TO_MRAM, 1, HSD_SynthSFXHeaderLoadCallback, NULL);
		OSRestoreInterrupts(enabled);
	}
}

/**
 * @note Address: 0x801EC94C
 * @note Size: 0x180
 */
s32 HSD_SynthSFXLoad(const char* filename, s32 bankID, void (*callback)(s32, s32), s32 arg)
{
	s32 entrynum;
	BOOL enabled;

	ASSERT_SYNTH_REPORT(0x18F, (bankID >= 0 && bankID < hsd_SynthSFXBankNum), "invalid bankID = %d; filename = %s\n", bankID, filename);

	entrynum = DVDConvertPathToEntrynum(filename);
	if (entrynum != -1) {
		while (sSynthSfxLoadQueueLength >= SYNTH_SFX_LOAD_QUEUE_SIZE) {
			ASSERT_SYNTH(0x193, 0);
		}

		enabled                             = OSDisableInterrupts();
		sSynthSfxLoadRequests[sSynthSfxLoadQueueLength].entrynum = entrynum;
		sSynthSfxLoadRequests[sSynthSfxLoadQueueLength].bankID   = bankID;
		sSynthSfxLoadRequests[sSynthSfxLoadQueueLength].callback = callback;
		sSynthSfxLoadRequests[sSynthSfxLoadQueueLength].arg      = arg;
		sSynthSfxLoadQueueLength += 1;

		if (sSynthSfxLoadQueueLength == 1) {
			HSD_SynthSFXLoadNewProc();
		}

		OSRestoreInterrupts(enabled);
	}
	return entrynum;
}

/**
 * @note Address: N/A
 * @note Size: 0x44
 */
void HSD_SynthSFXWaitForLoadCompletion(void (*callback)(void))
{
	while (sSynthSfxLoadQueueLength != 0) {
		callback();
	}
}

/**
 * @note Address: 0x801ECACC
 * @note Size: 0x10
 */
s32 HSD_SynthSFXGetPendingLoadCount(void)
{
	return sSynthSfxLoadQueueLength - sSynthSfxLoadCancelled;
}

static void unlinkSFXGroup(const HSD_SynthSFXGroup* group)
{
	s32 i;

	for (i = 0; i < group->count; i++) {
		s32 id = group->base + i;
		HSD_SynthSFXEntry** link = &sSynthSfxIdBuckets[id & (ARRAY_SIZE(sSynthSfxIdBuckets) - 1)];

		while (*link != NULL && (*link)->id != id) {
			link = &(*link)->next;
		}
		if (*link != NULL) {
			*link = (*link)->next;
		}
	}
}

/**
 * @note Address: 0x801ECADC
 * @note Size: 0x58
 */
s32 HSD_SynthSFXNewBank(s32 size)
{
	if (hsd_SynthSFXBankNum >= SYNTH_SFX_BANK_COUNT || size + hsd_SynthSFXBankHead[hsd_SynthSFXBankNum] > sSynthSfxBankHeapEnd) {
		return -1;
	}
	hsd_SynthSFXBank[hsd_SynthSFXBankNum]         = hsd_SynthSFXBankHead[hsd_SynthSFXBankNum];
	hsd_SynthSFXBankHead[hsd_SynthSFXBankNum + 1] = size + hsd_SynthSFXBankHead[hsd_SynthSFXBankNum];
	return hsd_SynthSFXBankNum++;
}

/**
 * @note Address: 0x801ECB34
 * @note Size: 0x120
 */
s32 HSD_SynthSFXBankClear(s32 bankID)
{
	HSD_SynthSFXGroup** head;
	HSD_SynthSFXGroup* group;
	BOOL enabled = OSDisableInterrupts();

	if (bankID < 0 || bankID >= hsd_SynthSFXBankNum) {
		OSRestoreInterrupts(enabled);
		return -1;
	}
	HSD_SynthSFXStopBank(bankID);
	head = &sSynthSampleGroupsByBank[bankID];
	while (*head != NULL) {
		unlinkSFXGroup(*head);
		group = *head;
		*head = (*head)->next;
		HSD_AudioFree(group);
	}
	hsd_SynthSFXBank[bankID] = hsd_SynthSFXBankHead[bankID];
	OSRestoreInterrupts(enabled);
	return 0;
}
