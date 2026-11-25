#ifndef HSD_SYNTH_BANK_H
#define HSD_SYNTH_BANK_H

#include <global.h>
#include <sdk/msl/c/cstddef>
#include <sdk/dolphin/ax.h>

enum {
	SYNTH_SFX_BANK_COUNT = 32,
	SYNTH_SFX_LOAD_QUEUE_SIZE = 32,
	SYNTH_SFX_ID_BUCKET_COUNT = 32,
};

// A loaded SFX file with its sample data in ARAM.
typedef struct HSD_SynthSFXGroup {
	struct HSD_SynthSFXGroup* next; // _00
	s32 id;                         // _04
	s32 base;                       // _08
	s32 count;                      // _0C
	u32 araddr;                     // _10
	u32 arsize;                     // _14
} HSD_SynthSFXGroup;

typedef struct HSD_SynthSample {
	AXPBADDR addr;
	AXPBADPCM adpcm;
	AXPBADPCMLOOP adpcmLoop;
	u16 padding;
} HSD_SynthSample;

STATIC_ASSERT(sizeof(HSD_SynthSample) == 0x40);

// The sample records trail the entry header in the load allocation.
typedef struct HSD_SynthSFXEntry {
	struct HSD_SynthSFXEntry* next;
	s32 id;
	int count;
	s32 rate;
	HSD_SynthSample sample[];
} HSD_SynthSFXEntry;

enum {
	SFX_ENTRY_METADATA_OFFSET = offsetof(HSD_SynthSFXEntry, count),
	SFX_ENTRY_SAMPLE_OFFSET = offsetof(HSD_SynthSFXEntry, sample),
	SFX_ENTRY_METADATA_HEADER_SIZE = SFX_ENTRY_SAMPLE_OFFSET - SFX_ENTRY_METADATA_OFFSET,
};

STATIC_ASSERT(SFX_ENTRY_METADATA_OFFSET == 0x8);
STATIC_ASSERT(SFX_ENTRY_SAMPLE_OFFSET == 0x10);

typedef struct HSD_SynthSFXFileHeader {
	u32 metadataSize;
	u32 aramSize;
	u32 entryCount;
	u32 firstId;
} HSD_SynthSFXFileHeader;

enum {
	SFX_FILE_HEADER_SIZE = sizeof(HSD_SynthSFXFileHeader),
	SFX_METADATA_PREFIX_SIZE = 0x10,
};

// The first disc read includes the header and the start of the entry metadata.
typedef struct HSD_SynthSFXFileStart {
	HSD_SynthSFXFileHeader header;
	u32 metadataPrefix[SFX_METADATA_PREFIX_SIZE / sizeof(u32)];
} HSD_SynthSFXFileStart;

STATIC_ASSERT(sizeof(HSD_SynthSFXFileStart) == 0x20);

typedef struct HSD_SynthSFXLoadRequest {
	s32 entrynum;               // _00
	s32 bankID;                 // _04
	void (*callback)(s32, s32); // _08
	s32 arg;                    // _0C
} HSD_SynthSFXLoadRequest;

extern void* (*sSynthMallocProc)(s32);
extern void (*sSynthFreeProc)(void*);
extern s32 hsd_SynthSFXBankNum;
extern volatile s32 sSynthSfxLoadQueueLength;
extern BOOL sSynthSfxLoadCancelled;
extern s32 sSynthLoadRequestIds[2];
extern HSD_SynthSFXGroup* sSynthSfxLoadDestination;
extern u32 sSynthSfxBankHeapEnd;
extern HSD_SynthSFXFileStart hsd_SynthSFXLoadBuf;
extern HSD_SynthSFXGroup* sSynthSampleGroupsByBank[SYNTH_SFX_BANK_COUNT];
extern HSD_SynthSFXLoadRequest sSynthSfxLoadRequests[SYNTH_SFX_LOAD_QUEUE_SIZE];
extern s32 hsd_SynthSFXBank[SYNTH_SFX_BANK_COUNT];
extern HSD_SynthSFXEntry* sSynthSfxIdBuckets[SYNTH_SFX_ID_BUCKET_COUNT];
extern s32 hsd_SynthSFXBankHead[SYNTH_SFX_BANK_COUNT + 1];

void HSD_AudioFree(void* ptr);
void HSD_SynthSFXStopBank(s32 bankID);

#endif
