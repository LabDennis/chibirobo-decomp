#include <global.h>
#include <dolphin/types.h>
#include <sysdolphin/objalloc.h>

typedef struct _AXPBMIX {
	u16 vL;
	u16 vDeltaL;
	u16 vR;
	u16 vDeltaR;
	u16 vAuxAL;
	u16 vDeltaAuxAL;
	u16 vAuxAR;
	u16 vDeltaAuxAR;
	u16 vAuxBL;
	u16 vDeltaAuxBL;
	u16 vAuxBR;
	u16 vDeltaAuxBR;
	u16 vAuxBS;
	u16 vDeltaAuxBS;
	u16 vS;
	u16 vDeltaS;
	u16 vAuxAS;
	u16 vDeltaAuxAS;
} AXPBMIX;

typedef struct _AXPBVE {
	u16 currentVolume;
	s16 currentDelta;
} AXPBVE;

typedef struct _AXPBADDR {
	u16 loopFlag;
	u16 format;
	u32 loopAddressHi;
	u32 loopAddressLo;
	u32 endAddressHi;
	u32 endAddressLo;
	u32 currentAddressHi;
	u32 currentAddressLo;
} AXPBADDR;

typedef struct _AXPBADPCM {
	s16 coefs[16];
	u16 gain;
	u16 pred_scale;
	u16 yn1;
	u16 yn2;
} AXPBADPCM;

typedef struct _AXPBADPCMLOOP {
	u16 loop_pred_scale;
	u16 loop_yn1;
	u16 loop_yn2;
} AXPBADPCMLOOP;

typedef struct _AXPBSRC {
	u16 ratioHi;
	u16 ratioLo;
	u16 currentAddressFrac;
	u16 pad[4];
} AXPBSRC;

typedef struct _AXVPB {
	struct _AXVPB* next;
	struct _AXVPB* prev;
	void* next1;
	u32 priority;
	void* callback;
	u32 userContext;
	u32 index;
	u32 sync;
	/* remaining PB fields not accessed directly by this unit */
	u8 pad20[0x1D6 - 0x20];
} AXVPB;

#define AXVPB_PB_STATE(v)         (*(u16*)((u8*)(v) + 0x146))
#define AXVPB_CURRENT_ADDR(v)     (*(u32*)((u8*)(v) + 0x1B2))
#define AXVPB_ITD_FLAG(v)         (*(u16*)((u8*)(v) + 0x166))
#define AXVPB_ITD_SHIFTL(v)       (*(u16*)((u8*)(v) + 0x16C))
#define AXVPB_ITD_SHIFTR(v)       (*(u16*)((u8*)(v) + 0x16E))
#define AXVPB_ITD_TARGETSHIFTL(v) (*(u16*)((u8*)(v) + 0x170))
#define AXVPB_ITD_TARGETSHIFTR(v) (*(u16*)((u8*)(v) + 0x172))

struct HSD_SynthPStream;

typedef struct HSD_SynthMix {
	u16 vL;     // _00
	u16 vR;     // _02
	u16 vAuxAL; // _04
	u16 vAuxAR; // _06
	u16 vAuxBL; // _08
	u16 vAuxBR; // _0A
	u16 vAuxBS; // _0C
	u16 vS;     // _0E
	u16 vAuxAS; // _10
} HSD_SynthMix;

typedef struct HSD_SynthUserVol {
	f32 volume; // _00
	s32 mode;   // _04
	union {
		f32 f;
		s32 i;
	} target;      // _08
	s32 dbxRaw;    // _0C
	s32 fadeCount; // _10
	u8 _14[4];     // _14
} HSD_SynthUserVol;

// a rear node shares the pool with the front ones, but keeps its own
// state where a front node has its user volumes
typedef struct HSD_SynthSFXNode {
	s32 sfxID;                              // _00
	struct HSD_SynthSFXNode* next;          // _04
	u8 unk8;                                // _08
	u8 flags;                               // _09
	u8 voice_count;                         // _0A
	u8 channel;                             // _0B
	AXVPB* voice[2];                        // _0C
	s16 rearIndex;                          // _14
	s8 unk16;                               // _16
	s8 unk17;                               // _17
	f32 freqBase;                           // _18
	f32 pitch[2];                           // _1C
	struct HSD_SynthSFXNode* volUpdateNext; // _24
	struct HSD_SynthSFXNode* mixUpdateNext; // _28
	u16 curVol;                             // _2C
	u8 volUpdatePending;                    // _2E
	u8 mixUpdatePending;                    // _2F
	union {
		struct HSD_SynthPStream* pstream;  // front nodes
		struct HSD_SynthSFXNode* rearNext; // rear nodes, the pitch update list
	} link;                                // _30
	union {
		HSD_SynthUserVol user_vol[2]; // front nodes
		struct {
			s16 side;         // _34
			s16 newSide;      // _36
			HSD_SynthMix mix; // _38
			u16 lastVol;      // _4A
			s16 lastDelta;    // _4C
			u8 _4E[2];        // _4E
			f32 pitch;        // _50
		} rear;
	} u;                 // _34
	u8 userPan[2];       // _64
	u8 userSPan[2];      // _66
	f32 mixMain;         // _68
	f32 mixAuxA;         // _6C
	f32 mixAuxB;         // _70
	HSD_SynthMix mix;    // _74
	HSD_SynthMix mixDst; // _86
} HSD_SynthSFXNode;

typedef struct HSD_SynthBankRange {
	/* 0x00 */ s32 base;
	/* 0x04 */ s32 top;
} HSD_SynthBankRange;

typedef struct HSD_SynthPStreamEntry {
	/* 0x00 */ struct HSD_SynthPStreamEntry* next; /* free-list link */
	/* 0x04 */ struct HSD_SynthSFXNode* node;      /* owning node back-ref */
	/* 0x08 */ s32 entrynum;
	/* 0x0C */ s32 schedulerSlot;
	/* 0x10 */ s16 bufIdx;
	/* 0x12 */ u8 pad12[10];
	/* 0x1C */ s32 bufTableHeadIx;
	/* 0x20 */ s32 unk20;
	/* 0x24 */ s32 unk24;
} HSD_SynthPStreamEntry;

extern void* OSAllocFromHeap(u32 heap, u32 size);
extern void OSFreeToHeap(u32 heap, void* ptr);
extern u32 OSDisableInterrupts(void);
extern u32 OSRestoreInterrupts(u32 level);
extern void OSReport(const char*, ...);
extern void OSPanic(const char*, s32, const char*, ...);
extern u32 OSGetSoundMode(void);
extern void OSSetSoundMode(u32 mode);
#define OSRoundUp32B(x) (((u32)(x) + 32 - 1) & ~(32 - 1))
extern void* memcpy(void*, const void*, u32);
extern f64 sin(f64);
extern f64 sqrt(f64);

extern void AISetDSPSampleRate(u32 rate);
extern void AISetStreamPlayState(s32);
extern void AISetStreamVolLeft(u8 vol);
extern void AISetStreamVolRight(u8 vol);
extern void* ARAlloc(s32);
extern void AXFreeVoice(AXVPB*);
extern AXVPB* AXAcquireVoice(u32 priority, void* callback, u32 userContext);
extern u64 AXInitEx(void*, s8*);
extern void AXRegisterCallback(void (*)(void));

extern void fn_803ED0CC(s32);
extern void fn_803ED76C(void (*)(void), void (*)(void));
extern void fn_803EE7F4(AXVPB*, s32);
extern void fn_803EEB68(AXVPB*);
extern void fn_803EEBCC(u16, u16);
extern void fn_803EEC28(AXVPB*, s16*);
extern void fn_803EEC80(AXVPB*, s16);
extern void fn_803EECCC(AXVPB*, void*);
extern void fn_803EEDDC(AXVPB*, u32);
extern void fn_803EEE28(AXVPB*, void*, s32);
extern void fn_803EEE7C(AXVPB*, s32, s32, s32);
extern void fn_803EEED0(AXVPB*, s32, s32);
extern void fn_803EEF24(AXVPB*, void*);
extern void fn_803EEFC8(AXVPB*, u32*);
extern void fn_803EF0F8(AXVPB*, void*);
extern void hwSetPitch(f32);
extern void hwSetVolume(u16*, s32, s32, u16);

extern s32 HSD_DevComRequest(s32 entrynum, s32 offset, void* dst, u32 length, s32 priority, s32 sync,
                             void (*callback)(s32, void*, s32, s32), void* userdata);

extern void* HSD_GObjObjectLink(void* gobj, u8 kind, void* obj);
extern void* HSD_GObjObjectUnlink(void* gobj);

void SetDvdStreamingLimits(s32 max, s32 key);
s32 AcquireDvdStreamingSlot(void (*callback)(s32, void*), s32 param);
void ReleaseDvdStreamingSlot(s32 id);
void SetDvdStreamingSlotKey(s32 id, s32 key);

extern f64 exp(f64);
extern f64 log(f64);

#define ASSERT_SYNTH(line, cond)             ((cond) ? ((void)0) : __assert(__FILE__, line, #cond))
#define ASSERT_SYNTH_MSG(line, cond, msg)    ((cond) ? ((void)0) : __assert(__FILE__, line, msg))
#define ASSERT_SYNTH_REPORT(line, cond, ...) ((cond) ? (void)0 : (OSReport(__VA_ARGS__), __assert(__FILE__, line, #cond)))

#define USERVOL_NUM          2
#define USERPAN_NUM          2
#define HSD_SYNTHSFXNODE_MAX 0x40

// one beat marker from the stream data
typedef struct HSD_SynthBeatMarker {
	s32 pos;  // _00
	s32 data; // _04
} HSD_SynthBeatMarker;

// beat markers live in a chain of small blocks, three per block
typedef struct HSD_SynthBeatMarkerBlock {
	struct HSD_SynthBeatMarkerBlock* next; // _00
	s32 _04;                               // _04
	HSD_SynthBeatMarker marker[3];         // _08
} HSD_SynthBeatMarkerBlock;

typedef struct HSD_SynthSFXLoadRequest {
	s32 entrynum;               // _00
	s32 bankID;                 // _04
	void (*callback)(s32, s32); // _08
	s32 arg;                    // _0C
} HSD_SynthSFXLoadRequest;

// one 0x40 byte header per stream buffer
typedef struct HSD_SynthPStreamHako {
	s32 size;                          // _00
	s32 end;                           // _04
	s32 next;                          // _08
	u32 loop[2][2];                    // _0C
	u8 markerNum;                      // _1C
	HSD_SynthBeatMarkerBlock* markers; // _20
	s32 markerIx;                      // _24
	u8 _28[0x40 - 0x28];               // _28
} HSD_SynthPStreamHako;

// the header's pstream entry is an older layout, this build has this one
typedef struct HSD_SynthPStream {
	struct HSD_SynthPStream* next;   // _00
	HSD_SynthSFXNode* node;          // _04
	s32 entrynum;                    // _08
	s32 schedulerSlot;               // _0C
	s32 bufTableHeadIx;              // _10
	s32 _14;                         // _14
	s32 _18;                         // _18
	s32 _1C;                         // _1C
	void (*callback)(s32, s32, s32); // _20
	s32 _24;                         // _24
	s32 _28;                         // _28
} HSD_SynthPStream;

#define PSTREAM(node) ((HSD_SynthPStream*)(node)->link.pstream)

typedef struct {
	f32 volume;
	s32 mode;
	union {
		f32 f;
		s32 i;
	} target;
	s32 dbxRaw;
	s32 duration;
	s32 delay;
} HSD_SynthVolFader;

#define HSD_SYNTHPSTREAM_MAX 4

// a queued stream start, kicked off when the current stream goes away
typedef struct HSD_SynthPStreamRequest {
	struct HSD_SynthPStreamRequest* next; // _00
	s32 entrynum;                         // _04
	s16 vol;                              // _08
	s16 vol2;                             // _0A
	u8 volMode;                           // _0C
	u8 vol2Mode;                          // _0D
	u8 priority;                          // _0E
	u8 itd;                               // _0F
	u8 channel;                           // _10
	u8 unk11;                             // _11
	u8 pad12[2];
	f32 pitch1;  // _14
	f32 pitch2;  // _18
	f32 mixMain; // _1C
	f32 mixAuxA; // _20
	f32 mixAuxB; // _24
	s16 unk28;   // _28
	s16 unk2A;   // _2A
} HSD_SynthPStreamRequest;

static s32 sSynthPendingVoiceFreeCount                         = 0;
static void* (*sSynthMallocProc)(s32)               = NULL;
static void (*sSynthFreeProc)(void*)              = NULL;
static s32 hsd_SynthSFXBankNum                  = 0;
static volatile s32 sSynthSfxLoadQueueLength                = 0;
static s32 lbl_8065BB8C                         = 0;
static void (*sSynthDriverInactivatedCallback)(s32)                = NULL;
static void (*sSynthDriverMasterClockCallback)(u32)                = NULL;
static void (*sSynthDriverPauseCallback)(s32)                = NULL;
static void (*sSynthPreFrameCallback)(void)               = NULL; /* driver pre-frame hook */
static void (*sSynthPostFrameCallback)(void)               = NULL; /* driver post-frame hook */
static HSD_SynthSFXNode* sSynthPendingVolumeUpdates           = NULL; /* vol-update pending list head */
static HSD_SynthSFXNode* sSynthPendingMixUpdates           = NULL; /* mix-update pending list head */
static HSD_SynthSFXNode* sSynthPendingRearPitchUpdates           = NULL; /* rear-link update list head */
static s32 sSynthSfxIdBase                         = 0;
static HSD_SynthBeatMarkerBlock* sSynthFreeBeatMarkerBlocks   = NULL; /* bmb free list */
static HSD_SynthPStreamRequest* sSynthFreePStreamRequests    = NULL; /* request free list */
static HSD_SynthPStreamRequest* sSynthQueuedPStreamRequests    = NULL; /* request queue */
static HSD_SynthPStream* sSynthFreePStreams           = NULL; /* pstream entry free list */
static HSD_SynthPStream* sSynthActivePStreams           = NULL; /* pstream entry active list */
static s32 sSynthNextFreeStreamBuffer                         = 0;
static s32 sSynthDriverFrame                         = 0;
static s32 lbl_8065BBD0                         = 0;
static s32 sSynthStreamVolumeScale                         = 0;
static s32 sSynthStreamBufferAramBase                         = 0;
static s32 sSynthSoundMode                         = 0; /* sound mode */
static s32 sSynthStreamBufferCount                         = 0;
static void* lbl_8065BBE4                       = NULL; /* default silent sample bank base */
static u32* sSynthSfxLoadReadCursor                        = NULL; /* current SFX load source parse pointer */
static void* sSynthSfxLoadDestination                       = NULL; /* current SFX load dest pointer */
static void* sSynthSfxBankHeapEnd                       = NULL; /* max SFX bank heap boundary */

static u32 hsd_SynthSFXLoadBuf[8] __attribute__((aligned(32))); // read from disc, which wants 32 byte alignment
static void* lbl_80655F60[0x20]; // sample lists, one per bank
static HSD_SynthSFXLoadRequest sSynthSfxLoadRequests[0x20];
static s32 hsd_SynthSFXBank[0x20];
static void* lbl_80656260[0x20]; // sfx entries hashed by id
static s32 hsd_SynthSFXBankHead[0x21];
// next link of each stream buffer, -2 ends a list and -1 is free
static s32 lbl_80656364[0x17];
static HSD_SynthPStreamHako sSynthStreamHakoBuffers[0x11];
static AXVPB* sSynthVoicesPendingFree[0x40];
static HSD_SynthSFXNode sSynthSfxNodes[HSD_SYNTHSFXNODE_MAX];
static f32 sSynthSqrtLookup[0x100];
static HSD_SynthVolFader lbl_80659300;
static HSD_SynthVolFader lbl_80659318[0x40];
static HSD_SynthPStream lbl_80659918[HSD_SYNTHPSTREAM_MAX];

// defined further down, next to the code that first needs them in .sdata order
static u32 sSynthActiveChannelFaderMasks[2];
static f32 sSynthStreamVolume;


void HSD_SynthSFXLoadNewProc(void);
void HSD_SynthSFXFreeVoice(HSD_SynthSFXNode* node);
void HSD_SynthSFXStopBank(s32 bankID);
void HSD_SynthSFXStopRange(u32 lo, u32 hi);
void HSD_SynthSFXKeyOff(s32 id);
void HSD_SynthSFXUpdatePitch(HSD_SynthSFXNode* node);
void HSD_SynthSFXUpdateVolume(HSD_SynthSFXNode* node);
void HSD_SynthSFXInitMix(HSD_SynthSFXNode* node);
void HSD_SynthSFXUpdateMix(HSD_SynthSFXNode* node, s32 interpolate);
void ApplySynthMixTargetImmediately(HSD_SynthSFXNode* node);
void HSD_SynthSFXLinkUpdateRearPitch(HSD_SynthSFXNode* rear, u16 unused);
void UpdateSynthVolumeFadersAndVoices(void);
void AdvancePendingSynthMixUpdates(void);
void HSD_SynthCallback(void);
void QueueLoadedSynthStreamBuffer(s32 result, s32 pstreamID);
void HSD_SynthPStreamHakoMarkerCallback(s32 unused, s32 id, HSD_SynthBeatMarker* markers);
void HSD_SynthPStreamHakoHeaderCallback(s32 result, s32 pstreamID);
void HSD_SynthPStreamFirstHakoDataCallback(s32 result, s32 pstreamID);
void HSD_SynthPStreamFirstHakoMarkerCallback(s32 unused, s32 id, HSD_SynthBeatMarker* markers);
void HSD_SynthPStreamFirstHakoHeaderCallback(s32 result, s32 pstreamID);
void HSD_SynthPStreamHeaderCallback(s32 result, s32 pstreamID, u32* header);
void dropcallback(void* dropped);

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline HSD_SynthSFXNode* getNode(s32 id)
{
	HSD_SynthSFXNode* node = &sSynthSfxNodes[id & (HSD_SYNTHSFXNODE_MAX - 1)];
	if (id > 0 && node->sfxID == id) {
		return node;
	}
	return NULL;
}

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

// one loaded sfx file, a sample group in aram plus its sfx entries
typedef struct HSD_SynthSFXGroup {
	struct HSD_SynthSFXGroup* next; // _00
	s32 id;                         // _04
	s32 base;                       // _08
	s32 count;                      // _0C
	u32 araddr;                     // _10
	u32 arsize;                     // _14
} HSD_SynthSFXGroup;

// one sfx, threaded into lbl_80656260 by id
typedef struct HSD_SynthSFXData {
	struct HSD_SynthSFXData* next; // _00
	s32 id;                        // _04
} HSD_SynthSFXData;

static s32 sSynthLoadRequestIds[2] = { 0, 0 }; /* devcom request ids, sample data then header */

/**
 * @note Address: N/A
 * @note Size: 0x430
 */
void HSD_SynthSFXSampleLoadCallback(s32 result, s32 length, void* addr, s32 cancelflag)
{
	u32 enabled;
	s32 i;
	s32 n;
	s32 k;
	s32 id;
	unsigned int header_size;
	u32 alloc_size;
	u32 total;
	u32 dnw;
	s32 bankID;
	HSD_SynthSFXGroup** pp;
	s32 count;
	s32 base;

	if (lbl_8065BB8C == 0) {
		header_size = hsd_SynthSFXLoadBuf[0];
		alloc_size  = hsd_SynthSFXLoadBuf[2] * 8 + sizeof(HSD_SynthSFXGroup);
		total       = OSRoundUp32B(alloc_size + header_size);
		for (i = ((header_size - 0x10) >> 2) - 1; i >= 0; i--) {
			((u32*)sSynthSfxLoadDestination)[i + ((total - (header_size - 0x10)) >> 2)] = ((u32*)sSynthSfxLoadDestination)[i];
		}
		dnw = total - header_size;
		for (i = 0; i < 4; i++) {
			((u32*)sSynthSfxLoadDestination)[(dnw >> 2) + i] = hsd_SynthSFXLoadBuf[4 + i];
		}
		sSynthSfxLoadReadCursor = (u32*)((u8*)sSynthSfxLoadDestination + (dnw & ~3));

		bankID = sSynthSfxLoadRequests[0].bankID;
		pp     = (HSD_SynthSFXGroup**)&lbl_80655F60[bankID];
		while (*pp != NULL) {
			pp = &(*pp)->next;
		}
		*pp = sSynthSfxLoadDestination;

		((HSD_SynthSFXGroup*)sSynthSfxLoadDestination)->next   = NULL;
		((HSD_SynthSFXGroup*)sSynthSfxLoadDestination)->id     = sSynthSfxLoadRequests[0].entrynum;
		((HSD_SynthSFXGroup*)sSynthSfxLoadDestination)->araddr = hsd_SynthSFXBank[bankID];
		((HSD_SynthSFXGroup*)sSynthSfxLoadDestination)->arsize = hsd_SynthSFXLoadBuf[1];
		count                                      = hsd_SynthSFXLoadBuf[2];
		base                                       = hsd_SynthSFXLoadBuf[3];
		((HSD_SynthSFXGroup*)sSynthSfxLoadDestination)->base   = base;
		((HSD_SynthSFXGroup*)sSynthSfxLoadDestination)->count  = count;
		sSynthSfxLoadDestination                               = (HSD_SynthSFXGroup*)sSynthSfxLoadDestination + 1;
		for (i = 0; i < count; i++) {
			n = *sSynthSfxLoadReadCursor;
			memcpy((u8*)sSynthSfxLoadDestination + 8, sSynthSfxLoadReadCursor, n * 0x40 + 8);
			for (k = 0; k < n; k++) {
				u8* e = (u8*)sSynthSfxLoadDestination + k * 0x40;
				if (e + 0x10 != NULL) {
					*(u32*)(e + 0x14) += hsd_SynthSFXBank[bankID] * 2;
				} else {
					*(u32*)(e + 0x14) = (u32)lbl_8065BBE4;
				}
				*(u32*)((u8*)sSynthSfxLoadDestination + k * 0x40 + 0x18) += hsd_SynthSFXBank[bankID] * 2;
				*(u32*)((u8*)sSynthSfxLoadDestination + k * 0x40 + 0x1C) += hsd_SynthSFXBank[bankID] * 2;
			}
			id                                                  = base + i;
			((HSD_SynthSFXData*)sSynthSfxLoadDestination)->id   = id;
			((HSD_SynthSFXData*)sSynthSfxLoadDestination)->next = lbl_80656260[id & 0x1F];
			lbl_80656260[id & 0x1F]                             = sSynthSfxLoadDestination;
			sSynthSfxLoadReadCursor += (n * 0x40 + 8) / sizeof(u32);
			sSynthSfxLoadDestination = (u32*)sSynthSfxLoadDestination + (n * 0x40 + 0x10) / sizeof(u32);
		}
		if (sSynthSfxLoadRequests[0].callback != NULL) {
			sSynthSfxLoadRequests[0].callback(sSynthSfxLoadRequests[0].entrynum, sSynthSfxLoadRequests[0].arg);
		}
		hsd_SynthSFXBank[bankID] += hsd_SynthSFXLoadBuf[1];
	} else {
		if (sSynthSfxLoadDestination != NULL) {
			HSD_AudioFree(sSynthSfxLoadDestination);
		}
		lbl_8065BB8C = 0;
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
 * @note Address: 0x801EC324
 * @note Size: 0x430
 */
// HSD_SynthSFXSampleLoadCallback
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    stw      r0, 0x44(r1)
    addi     r11, r1, 0x40
    bl       _savegpr_21
    lwz      r0, lbl_8065BB8C@sda21(r0)
    lis      r3, hsd_SynthSFXLoadBuf@ha
    addi     r31, r3, hsd_SynthSFXLoadBuf@l
    cmpwi    r0, 0x0
    bne      .L_801EC6BC
    addi     r3, r31, 0x0
    lwz      r11, 0x0(r31)
    lwz      r4, 0x8(r3)
    subi     r0, r11, 0x10
    slwi     r4, r4, 3
    add      r5, r4, r11
    srwi     r4, r0, 2
    subic.   r10, r4, 0x1
    addi     r4, r5, 0x37
    clrrwi   r12, r4, 5
    subf     r0, r0, r12
    slwi     r7, r10, 2
    srwi     r8, r0, 2
    addi     r4, r10, 0x1
    blt      .L_801EC4A4
    srwi.    r0, r4, 3
    mtctr    r0
    beq      .L_801EC480
.L_801EC394:
    lwz      r9, sSynthSfxLoadDestination@sda21(r0)
    add      r5, r10, r8
    slwi     r0, r5, 2
    subi     r10, r10, 0x1
    lwzx     r6, r9, r7
    add      r5, r10, r8
    subi     r7, r7, 0x4
    subi     r10, r10, 0x1
    stwx     r6, r9, r0
    slwi     r0, r5, 2
    add      r5, r10, r8
    subi     r10, r10, 0x1
    lwz      r9, sSynthSfxLoadDestination@sda21(r0)
    lwzx     r6, r9, r7
    subi     r7, r7, 0x4
    stwx     r6, r9, r0
    slwi     r0, r5, 2
    add      r5, r10, r8
    subi     r10, r10, 0x1
    lwz      r9, sSynthSfxLoadDestination@sda21(r0)
    lwzx     r6, r9, r7
    subi     r7, r7, 0x4
    stwx     r6, r9, r0
    slwi     r0, r5, 2
    add      r5, r10, r8
    subi     r10, r10, 0x1
    lwz      r9, sSynthSfxLoadDestination@sda21(r0)
    lwzx     r6, r9, r7
    subi     r7, r7, 0x4
    stwx     r6, r9, r0
    slwi     r0, r5, 2
    add      r5, r10, r8
    subi     r10, r10, 0x1
    lwz      r9, sSynthSfxLoadDestination@sda21(r0)
    lwzx     r6, r9, r7
    subi     r7, r7, 0x4
    stwx     r6, r9, r0
    slwi     r0, r5, 2
    add      r5, r10, r8
    subi     r10, r10, 0x1
    lwz      r9, sSynthSfxLoadDestination@sda21(r0)
    lwzx     r6, r9, r7
    subi     r7, r7, 0x4
    stwx     r6, r9, r0
    slwi     r0, r5, 2
    add      r5, r10, r8
    subi     r10, r10, 0x1
    lwz      r9, sSynthSfxLoadDestination@sda21(r0)
    lwzx     r6, r9, r7
    subi     r7, r7, 0x4
    stwx     r6, r9, r0
    slwi     r0, r5, 2
    lwz      r9, sSynthSfxLoadDestination@sda21(r0)
    lwzx     r6, r9, r7
    subi     r7, r7, 0x4
    stwx     r6, r9, r0
    bdnz     .L_801EC394
    andi.    r4, r4, 0x7
    beq      .L_801EC4A4
.L_801EC480:
    mtctr    r4
.L_801EC484:
    lwz      r9, sSynthSfxLoadDestination@sda21(r0)
    add      r5, r10, r8
    slwi     r0, r5, 2
    subi     r10, r10, 0x1
    lwzx     r6, r9, r7
    subi     r7, r7, 0x4
    stwx     r6, r9, r0
    bdnz     .L_801EC484
.L_801EC4A4:
    addi     r10, r31, 0x0
    subf     r4, r11, r12
    lwz      r0, 0x10(r10)
    srwi     r7, r4, 2
    lwz      r6, sSynthSfxLoadDestination@sda21(r0)
    clrrwi   r9, r4, 2
    addi     r5, r7, 0x1
    addi     r4, r7, 0x2
    stwx     r0, r6, r9
    addi     r0, r7, 0x3
    slwi     r6, r5, 2
    slwi     r5, r4, 2
    lwz      r8, 0x14(r10)
    slwi     r0, r0, 2
    lwz      r7, sSynthSfxLoadDestination@sda21(r0)
    addi     r4, r31, 0xa0
    addi     r11, r31, 0x20
    stwx     r8, r7, r6
    lwz      r7, 0x18(r10)
    lwz      r6, sSynthSfxLoadDestination@sda21(r0)
    stwx     r7, r6, r5
    lwz      r6, 0x1c(r10)
    lwz      r5, sSynthSfxLoadDestination@sda21(r0)
    stwx     r6, r5, r0
    lwz      r5, sSynthSfxLoadDestination@sda21(r0)
    lwz      r0, 0x4(r4)
    add      r4, r5, r9
    slwi     r30, r0, 2
    stw      r4, sSynthSfxLoadReadCursor@sda21(r0)
    add      r11, r11, r30
    b        .L_801EC524
.L_801EC520:
    mr       r11, r0
.L_801EC524:
    lwz      r0, 0x0(r11)
    cmplwi   r0, 0x0
    bne      .L_801EC520
    stw      r5, 0x0(r11)
    li       r24, 0x0
    addi     r29, r31, 0x2a0
    addi     r25, r31, 0x0
    lwz      r4, sSynthSfxLoadDestination@sda21(r0)
    addi     r28, r31, 0x320
    stw      r24, 0x0(r4)
    lwz      r0, 0xa0(r31)
    lwz      r4, sSynthSfxLoadDestination@sda21(r0)
    stw      r0, 0x4(r4)
    lwzx     r0, r29, r30
    lwz      r4, sSynthSfxLoadDestination@sda21(r0)
    stw      r0, 0x10(r4)
    lwz      r0, 0x4(r25)
    lwz      r4, sSynthSfxLoadDestination@sda21(r0)
    stw      r0, 0x14(r4)
    lwz      r23, 0x8(r3)
    lwz      r22, 0xc(r25)
    lwz      r3, sSynthSfxLoadDestination@sda21(r0)
    stw      r22, 0x8(r3)
    lwz      r3, sSynthSfxLoadDestination@sda21(r0)
    stw      r23, 0xc(r3)
    lwz      r3, sSynthSfxLoadDestination@sda21(r0)
    addi     r0, r3, 0x18
    stw      r0, sSynthSfxLoadDestination@sda21(r0)
    b        .L_801EC680
.L_801EC598:
    lwz      r4, sSynthSfxLoadReadCursor@sda21(r0)
    lwz      r3, sSynthSfxLoadDestination@sda21(r0)
    lwz      r21, 0x0(r4)
    addi     r3, r3, 0x8
    slwi     r26, r21, 6
    addi     r27, r26, 0x8
    mr       r5, r27
    bl       memcpy
    li       r3, 0x0
    mtctr    r21
    cmpwi    r21, 0x0
    ble      .L_801EC638
.L_801EC5C8:
    lwz      r0, sSynthSfxLoadDestination@sda21(r0)
    add      r5, r0, r3
    addic.   r0, r5, 0x10
    beq      .L_801EC5F0
    lwzx     r0, r29, r30
    lwz      r4, 0x14(r5)
    slwi     r0, r0, 1
    add      r0, r4, r0
    stw      r0, 0x14(r5)
    b        .L_801EC5F8
.L_801EC5F0:
    lwz      r0, lbl_8065BBE4@sda21(r0)
    stw      r0, 0x14(r5)
.L_801EC5F8:
    lwz      r7, sSynthSfxLoadDestination@sda21(r0)
    addi     r6, r3, 0x18
    lwzx     r0, r29, r30
    addi     r5, r3, 0x1c
    lwzx     r4, r7, r6
    addi     r3, r3, 0x40
    slwi     r0, r0, 1
    add      r0, r4, r0
    stwx     r0, r7, r6
    lwz      r6, sSynthSfxLoadDestination@sda21(r0)
    lwzx     r0, r29, r30
    lwzx     r4, r6, r5
    slwi     r0, r0, 1
    add      r0, r4, r0
    stwx     r0, r6, r5
    bdnz     .L_801EC5C8
.L_801EC638:
    lwz      r4, sSynthSfxLoadDestination@sda21(r0)
    add      r5, r22, r24
    addi     r0, r26, 0x10
    clrrwi   r3, r27, 2
    stw      r5, 0x4(r4)
    clrlslwi r6, r5, 27, 2
    clrrwi   r0, r0, 2
    addi     r24, r24, 0x1
    lwzx     r5, r28, r6
    lwz      r4, sSynthSfxLoadDestination@sda21(r0)
    stw      r5, 0x0(r4)
    lwz      r5, sSynthSfxLoadDestination@sda21(r0)
    lwz      r4, sSynthSfxLoadReadCursor@sda21(r0)
    add      r0, r5, r0
    stwx     r5, r28, r6
    add      r3, r4, r3
    stw      r3, sSynthSfxLoadReadCursor@sda21(r0)
    stw      r0, sSynthSfxLoadDestination@sda21(r0)
.L_801EC680:
    cmpw     r24, r23
    blt      .L_801EC598
    addi     r4, r31, 0xa0
    lwz      r12, 0x8(r4)
    cmplwi   r12, 0x0
    beq      .L_801EC6A8
    lwz      r3, 0xa0(r31)
    lwz      r4, 0xc(r4)
    mtctr    r12
    bctrl
.L_801EC6A8:
    lwzx     r3, r29, r30
    lwz      r0, 0x4(r25)
    add      r0, r3, r0
    stwx     r0, r29, r30
    b        .L_801EC6DC
.L_801EC6BC:
    lwz      r3, sSynthSfxLoadDestination@sda21(r0)
    cmplwi   r3, 0x0
    beq      .L_801EC6D4
    lwz      r12, sSynthFreeProc@sda21(r0)
    mtctr    r12
    bctrl
.L_801EC6D4:
    li       r0, 0x0
    stw      r0, lbl_8065BB8C@sda21(r0)
.L_801EC6DC:
    bl       OSDisableInterrupts
    lwz      r4, sSynthSfxLoadQueueLength@sda21(r0)
    mr       r22, r3
    addi     r5, r31, 0xa0
    subi     r0, r4, 0x1
    li       r4, 0x0
    stw      r0, sSynthSfxLoadQueueLength@sda21(r0)
    b        .L_801EC724
.L_801EC6FC:
    lwz      r3, 0x10(r5)
    addi     r4, r4, 0x1
    lwz      r0, 0x14(r5)
    stw      r3, 0x0(r5)
    stw      r0, 0x4(r5)
    lwz      r3, 0x18(r5)
    lwz      r0, 0x1c(r5)
    stw      r3, 0x8(r5)
    stw      r0, 0xc(r5)
    addi     r5, r5, 0x10
.L_801EC724:
    lwz      r0, sSynthSfxLoadQueueLength@sda21(r0)
    cmpw     r4, r0
    blt      .L_801EC6FC
    bl       HSD_SynthSFXLoadNewProc
    mr       r3, r22
    bl       OSRestoreInterrupts
    addi     r11, r1, 0x40
    bl       _restgpr_21
    lwz      r0, 0x44(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

/**
 * @note Address: 0x801EC754
 * @note Size: 0x17C
 */
void HSD_SynthSFXHeaderLoadCallback(s32 result, s32 length, void* addr, s32 cancelflag)
{
	s32 header_size;
	u32 alloc_size;

	if (lbl_8065BB8C == 0) {
		s32 bankID = sSynthSfxLoadRequests[0].bankID;

		ASSERT_SYNTH_REPORT(0x14F, hsd_SynthSFXBankHead[bankID + 1] - hsd_SynthSFXBank[bankID] >= hsd_SynthSFXLoadBuf[1],
		                    "Can't load SFX file; bank(id=%d) buffer overflow.\n", sSynthSfxLoadRequests[0].bankID);

		alloc_size   = hsd_SynthSFXLoadBuf[2] * 8 + sizeof(HSD_SynthSFXGroup);
		header_size  = hsd_SynthSFXLoadBuf[0];
		sSynthSfxLoadDestination = HSD_AudioMalloc(OSRoundUp32B(alloc_size + header_size));
		sSynthLoadRequestIds[1]
		    = HSD_DevComRequest(sSynthSfxLoadRequests[0].entrynum, 0x20, sSynthSfxLoadDestination, OSRoundUp32B(header_size - 0x10), 0x21, 1, NULL, 0);
		sSynthLoadRequestIds[0]
		    = HSD_DevComRequest(sSynthSfxLoadRequests[0].entrynum, OSRoundUp32B(header_size + 0x10), (void*)hsd_SynthSFXBank[sSynthSfxLoadRequests[0].bankID],
		                        hsd_SynthSFXLoadBuf[1], 0x23, 1, (void (*)(s32, void*, s32, s32))HSD_SynthSFXSampleLoadCallback, NULL);
		return;
	}
	sSynthSfxLoadDestination = NULL;
	HSD_SynthSFXSampleLoadCallback(0, 0, NULL, 0);
}

/**
 * @note Address: 0x801EC8D0
 * @note Size: 0x7C
 */
void HSD_SynthSFXLoadNewProc(void)
{
	if (sSynthSfxLoadQueueLength != 0) {
		u32 enabled     = OSDisableInterrupts();
		sSynthLoadRequestIds[0] = HSD_DevComRequest(sSynthSfxLoadRequests[0].entrynum, 0, hsd_SynthSFXLoadBuf, 0x20, 0x21, 1,
		                                    (void (*)(s32, void*, s32, s32))HSD_SynthSFXHeaderLoadCallback, NULL);
		OSRestoreInterrupts(enabled);
	}
}

/**
 * @note Address: 0x801EC94C
 * @note Size: 0x180
 */
s32 HSD_SynthSFXLoad(const s8* filename, s32 bankID, void (*callback)(s32, s32), s32 arg)
{
	s32 entrynum;
	u32 enabled;

	ASSERT_SYNTH_REPORT(0x18F, bankID >= 0 && bankID < hsd_SynthSFXBankNum, "invalid bankID = %d; filename = %s\n", bankID, filename);

	entrynum = DVDConvertPathToEntrynum(filename);
	if (entrynum != -1) {
		// spins on the assert until a load slot frees up, lol
		while (sSynthSfxLoadQueueLength >= 0x20) {
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
	return sSynthSfxLoadQueueLength - lbl_8065BB8C;
}

static inline void unlinkSFXData(s32 id)
{
	HSD_SynthSFXData** p = (HSD_SynthSFXData**)&lbl_80656260[id & 0x1F];

	while (*p != NULL) {
		if ((*p)->id == id) {
			*p = (*p)->next;
			return;
		}
		p = &(*p)->next;
	}
}

static inline void unlinkSFXGroup(HSD_SynthSFXGroup* group)
{
	s32 i;

	for (i = 0; i < group->count; i++) {
		unlinkSFXData(group->base + i);
	}
}

/**
 * @note Address: 0x801ECADC
 * @note Size: 0x58
 */
s32 HSD_SynthSFXNewBank(s32 size)
{
	if (hsd_SynthSFXBankNum >= 0x20 || size + hsd_SynthSFXBankHead[hsd_SynthSFXBankNum] > (u32)sSynthSfxBankHeapEnd) {
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
	u32 enabled = OSDisableInterrupts();

	if (bankID < 0 || bankID >= hsd_SynthSFXBankNum) {
		OSRestoreInterrupts(enabled);
		return -1;
	}
	HSD_SynthSFXStopBank(bankID);
	head = (HSD_SynthSFXGroup**)&lbl_80655F60[bankID];
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

// surround pan curve, front gain by span
static f32 lbl_8025BA10[0x100] = {
	1.0f,          0.996429205f,   0.992854357f,   0.989275455f,  0.985692501f,  0.982105553f,  0.978514612f,  0.974919617f,  0.971320748f,
	0.967717826f,  0.96411103f,    0.96050024f,    0.956885576f,  0.953266978f,  0.949644566f,  0.946018219f,  0.942387998f,  0.938753963f,
	0.935116112f,  0.931474447f,   0.927828968f,   0.924179733f,  0.920526683f,  0.916869938f,  0.913209438f,  0.909545183f,  0.905877233f,
	0.902205646f,  0.898530304f,   0.894851327f,   0.891168714f,  0.887482464f,  0.883792579f,  0.880099118f,  0.87640202f,   0.872701406f,
	0.868997216f,  0.86528945f,    0.861578226f,   0.857863426f,  0.854145169f,  0.850423396f,  0.846698165f,  0.842969477f,  0.839237392f,
	0.83550185f,   0.83176291f,    0.828020573f,   0.824274838f,  0.820525765f,  0.816773355f,  0.813017607f,  0.80925858f,   0.805496216f,
	0.801730573f,  0.797961652f,   0.794189513f,   0.790414095f,  0.786635458f,  0.782853663f,  0.779068649f,  0.775280416f,  0.771489084f,
	0.767694592f,  0.763896942f,   0.760096192f,   0.756292343f,  0.752485394f,  0.748675406f,  0.744862378f,  0.74104625f,   0.737227201f,
	0.733405054f,  0.729579985f,   0.725751877f,   0.721920848f,  0.718086839f,  0.714249969f,  0.710410118f,  0.706567407f,  0.702721834f,
	0.698873401f,  0.695022047f,   0.691167951f,   0.687310994f,  0.683451235f,  0.679588675f,  0.675723314f,  0.671855271f,  0.667984426f,
	0.664110899f,  0.66023469f,    0.656355739f,   0.652474105f,  0.648589849f,  0.644702971f,  0.64081341f,   0.636921227f,  0.633026481f,
	0.629129171f,  0.625229239f,   0.621326804f,   0.617421865f,  0.613514364f,  0.609604359f,  0.60569185f,   0.601776898f,  0.597859502f,
	0.593939662f,  0.590017378f,   0.58609271f,    0.582165658f,  0.578236222f,  0.574304461f,  0.570370317f,  0.566433847f,  0.562495112f,
	0.558554053f,  0.554610729f,   0.550665081f,   0.546717286f,  0.542767167f,  0.538814902f,  0.534860432f,  0.530903757f,  0.526944935f,
	0.522983968f,  0.519020855f,   0.515055656f,   0.511088312f,  0.50711894f,   0.503147423f,  0.499173939f,  0.495198369f,  0.491220802f,
	0.487241209f,  0.483259648f,   0.479276121f,   0.475290626f,  0.471303195f,  0.467313826f,  0.46332258f,   0.459329456f,  0.455334425f,
	0.451337546f,  0.447338849f,   0.443338305f,   0.439335972f,  0.435331821f,  0.431325912f,  0.427318245f,  0.423308849f,  0.419297695f,
	0.415284872f,  0.41127032f,    0.4072541f,     0.40323624f,   0.399216712f,  0.395195574f,  0.391172826f,  0.38714847f,   0.383122534f,
	0.379095048f,  0.375066012f,   0.371035427f,   0.367003351f,  0.362969786f,  0.35893473f,   0.354898214f,  0.350860238f,  0.346820861f,
	0.342780054f,  0.338737845f,   0.334694237f,   0.330649287f,  0.326602995f,  0.322555363f,  0.31850642f,   0.314456165f,  0.310404629f,
	0.30635184f,   0.302297801f,   0.298242539f,   0.294186026f,  0.29012835f,   0.286069483f,  0.282009453f,  0.27794826f,   0.273885936f,
	0.269822478f,  0.265757948f,   0.261692345f,   0.257625639f,  0.25355792f,   0.249489129f,  0.245419338f,  0.24134855f,   0.237276778f,
	0.233204037f,  0.229130343f,   0.225055724f,   0.220980182f,  0.216903731f,  0.212826401f,  0.208748206f,  0.204669163f,  0.200589284f,
	0.196508586f,  0.192427084f,   0.188344792f,   0.184261739f,  0.180177942f,  0.1760934f,    0.172008142f,  0.167922184f,  0.16383554f,
	0.159748226f,  0.155660272f,   0.151571676f,   0.147482455f,  0.143392652f,  0.139302254f,  0.135211274f,  0.131119758f,  0.127027705f,
	0.122935139f,  0.118842065f,   0.114748508f,   0.110654481f,  0.106560007f,  0.102465093f,  0.0983697623f, 0.0942740366f, 0.0901779234f,
	0.0860814452f, 0.0819846094f,  0.0778874457f,  0.0737899616f, 0.0696921721f, 0.0655941069f, 0.0614957698f, 0.057397183f,  0.0532983616f,
	0.0491993241f, 0.0451000854f,  0.0410006605f,  0.0369010717f, 0.0328013338f, 0.0287014581f, 0.024601467f,  0.0205013752f, 0.0164012015f,
	0.0123009589f, 0.00820066687f, 0.00410034182f, 0.0f,
};

static AXPBSRC lbl_8050932C = { 1, 0, 0, { 0, 0, 0, 0 } };

s32 sSynthDecibelFloor = -0x03880000;
static s32 sSynthQueuedStreamPlaybackId = -1;

/**
 * @note Address: 0x801ECC54
 * @note Size: 0x70
 */
s32 HSD_SynthVolToDBx(f32 vol)
{
	s32 dbx;

	if (vol < 0.000030199517f) {
		return sSynthDecibelFloor;
	}
	if (vol >= 1.0f) {
		return 0;
	}
	dbx = 5692384.633202305 * log(vol);
	if (dbx < sSynthDecibelFloor) {
		dbx = sSynthDecibelFloor;
	}
	return dbx;
}

/**
 * @note Address: 0x801ECCC4
 * @note Size: 0x6C
 */
f32 HSD_SynthDBxToVol(s32 dbx)
{
	if (dbx <= sSynthDecibelFloor) {
		return 0.0f;
	}
	if (dbx >= 0) {
		return 1.0f;
	}
	return exp(1.75673301162265e-07 * dbx);
}

// these are plain statics, they get compiled here even though every call
// inlines them, which is why the stream buffer tables come before the
// nodes in .bss
static s32* getBufTableTail(s32* p)
{
	s32 ix;

	while ((ix = *p) != -2) {
		p = &lbl_80656364[ix];
	}
	return p;
}

static void unlinkBufTable(s32* p, s32 ix)
{
	while (*p != ix) {
		p = &lbl_80656364[*p];
	}
	*p               = lbl_80656364[ix];
	lbl_80656364[ix] = -1;
}

static void freeBeatMarkers(s32 buf)
{
	HSD_SynthBeatMarkerBlock* bmb = sSynthStreamHakoBuffers[buf].markers;
	HSD_SynthBeatMarkerBlock* next;

	while (bmb != NULL) {
		next         = bmb->next;
		bmb->next    = sSynthFreeBeatMarkerBlocks;
		sSynthFreeBeatMarkerBlocks = bmb;
		bmb          = next;
	}
	sSynthStreamHakoBuffers[buf].markers = NULL;
}

s32 HSD_SynthPStreamStart(s32 entrynum, s32 vol, s32 volMode, s32 vol2, s32 vol2Mode, u8 vol1Clamp, u8 vol2Clamp, u8 pan, f32 pitch1,
                          f32 pitch2, f32 mixMain, f32 mixAuxA, f32 mixAuxB, s32 unk67, s32 channel, s32 useRear);

static inline void releasePStream(HSD_SynthPStream* pst)
{
	HSD_SynthPStream** pit;
	HSD_SynthPStreamRequest** rit;
	HSD_SynthPStreamRequest* req;
	s32 buf;
	s32* head;
	u32 enabled;

	pit = &sSynthActivePStreams;
	if (pst->schedulerSlot >= 0) {
		ReleaseDvdStreamingSlot(pst->schedulerSlot);
	}
	for (; *pit != NULL && *pit != pst; pit = &(*pit)->next) { }
	if (*pit != NULL) {
		while ((buf = *(head = &(*pit)->bufTableHeadIx)) != -2) {
			freeBeatMarkers(buf);
			unlinkBufTable(head, buf);
		}
		*pit = (*pit)->next;
	}
	pst->next    = sSynthFreePStreams;
	sSynthFreePStreams = pst;

	if (pst->node->sfxID == sSynthQueuedStreamPlaybackId) {
		req = sSynthQueuedPStreamRequests;
		for (rit = &sSynthQueuedPStreamRequests; *rit != NULL; rit = &(*rit)->next) {
			if (*rit == req) {
				*rit = req->next;
				break;
			}
		}
		req->next    = sSynthFreePStreamRequests;
		sSynthFreePStreamRequests = req;

		enabled = OSDisableInterrupts();
		req     = sSynthQueuedPStreamRequests;
		if (req != NULL) {
			sSynthQueuedStreamPlaybackId = HSD_SynthPStreamStart(req->entrynum, req->vol, req->volMode, req->vol2, req->vol2Mode, req->priority, req->itd,
			                                     req->channel, req->pitch1, req->pitch2, req->mixMain, req->mixAuxA, req->mixAuxB,
			                                     req->unk11, req->unk28, req->unk2A);
		} else {
			sSynthQueuedStreamPlaybackId = -1;
		}
		OSRestoreInterrupts(enabled);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
u32 HSD_SynthGetSoundMode(void)
{
	return sSynthSoundMode;
}

void AXSetMode(u32 mode);

/**
 * @note Address: 0x801ECD30
 * @note Size: 0xC8
 */
void HSD_SynthSetSoundMode(s32 mode)
{
	s32 i;
	u32 enabled;

	sSynthSoundMode = mode;
	if (mode == 2 || mode == 4) {
		AXSetMode(1);
	} else if (mode == 3) {
		AXSetMode(2);
	} else {
		AXSetMode(0);
	}
	OSSetSoundMode(mode != 0);
	enabled = OSDisableInterrupts();
	for (i = 0; i < HSD_SYNTHSFXNODE_MAX; i++) {
		if (sSynthSfxNodes[i].sfxID > 0) {
			HSD_SynthSFXUpdateMix(&sSynthSfxNodes[i], 1);
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: N/A
 * @note Size: ?
 * Only reached inlined in HSD_SynthInit; the out-of-line copy is what puts
 * its constant early in .sdata2.
 */
void HSD_SynthInitPantable(void)
{
	s32 i;

	for (i = 0; i <= 0xFE; i++) {
		sSynthSqrtLookup[i] = sqrt(0.003937008f * i);
	}
	sSynthSqrtLookup[0xFF] = sSynthSqrtLookup[0xFE];
}

static inline s32 lastBuf(HSD_SynthPStream* pst)
{
	s32* link = &pst->bufTableHeadIx;

	while (lbl_80656364[*link] != -2) {
		link = &lbl_80656364[*link];
	}
	return *link;
}

// hands out the buffer picked last time and picks the next free one
static inline s32 getFreeBuf(void)
{
	s32 i;
	s32 buf = sSynthNextFreeStreamBuffer;

	for (i = 0; i < sSynthStreamBufferCount; i++) {
		if (lbl_80656364[i] == -1 && i != buf) {
			sSynthNextFreeStreamBuffer = i;
		}
	}
	if (buf == sSynthNextFreeStreamBuffer) {
		return -1;
	}
	return buf;
}

s32 HSD_SynthPStreamStart(s32 entrynum, s32 vol, s32 volMode, s32 vol2, s32 vol2Mode, u8 vol1Clamp, u8 vol2Clamp, u8 pan, f32 pitch1,
                          f32 pitch2, f32 mixMain, f32 mixAuxA, f32 mixAuxB, s32 unk67, s32 channel, s32 useRear);

// tells the driver the sound is gone and pulls the node off the update lists
static inline void unlinkNode(HSD_SynthSFXNode* node)
{
	HSD_SynthSFXNode** it;
	HSD_SynthSFXNode* rear;

	if (!(node->flags & 1) && node->link.pstream == NULL && sSynthDriverInactivatedCallback != NULL) {
		sSynthDriverInactivatedCallback(node->sfxID);
	}

	if (node->volUpdatePending) {
		for (it = &sSynthPendingVolumeUpdates; *it != NULL; it = &(*it)->volUpdateNext) {
			if (*it == node) {
				node->volUpdatePending = 0;
				*it                    = node->volUpdateNext;
				break;
			}
		}
	}
	if (node->mixUpdatePending) {
		for (it = &sSynthPendingMixUpdates; *it != NULL; it = &(*it)->mixUpdateNext) {
			if (*it == node) {
				node->mixUpdatePending = 0;
				*it                    = node->mixUpdateNext;
				break;
			}
		}
	}
	if (node->rearIndex != -1) {
		rear = &sSynthSfxNodes[node->rearIndex];
		for (it = &sSynthPendingRearPitchUpdates; *it != NULL; it = &(*it)->link.rearNext) {
			if (*it == rear) {
				*it = rear->link.rearNext;
				break;
			}
		}
	}
}

/**
 * @note Address: 0x801ECDF8
 * @note Size: 0x368
 */
void HSD_SynthSFXFreeVoice(HSD_SynthSFXNode* node)
{
	HSD_SynthSFXNode* base;
	s32 i;
	s32 j;
	u32 enabled = OSDisableInterrupts();

	unlinkNode(node);

	if (node->link.pstream != NULL) {
		releasePStream(PSTREAM(node));
	}

	for (i = 0; i < (node->rearIndex != -1) + 1; i++) {
		if (i == 0) {
			base = node;
		} else {
			base = &sSynthSfxNodes[node->rearIndex];
		}
		for (j = 0; j < node->voice_count; j++) {
			AXFreeVoice(base->voice[j]);
			sSynthSfxNodes[base->voice[j]->index].sfxID = 0;
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801ED160
 * @note Size: 0x2E4
 */
void dropcallback(void* dropped)
{
	AXVPB* voice = dropped;
	HSD_SynthSFXNode* node;
	HSD_SynthSFXNode* base;
	u32 enabled;
	s32 i;
	s32 j;

	enabled = OSDisableInterrupts();
	node    = &sSynthSfxNodes[voice->index];
	for (j = 0; j < sSynthPendingVoiceFreeCount; j++) {
		if (sSynthVoicesPendingFree[j] == voice) {
			sSynthVoicesPendingFree[j] = NULL;
			break;
		}
	}

	if (node->sfxID == 0) {
		OSRestoreInterrupts(enabled);
		return;
	}
	if (node->sfxID == -1) {
		node = &sSynthSfxNodes[node->voice[0]->index];
	}
	if (node->sfxID < -1) {
		node = &sSynthSfxNodes[sSynthSfxNodes[node->voice[0]->index].rearIndex];
	}
	if (node->link.pstream != NULL) {
		OSReport("Program streaming dropped.\n");
		ASSERT_SYNTH(0x636, 0);
	}

	unlinkNode(node);

	// the driver already took the dropped voice, queue the others for freeing
	for (i = 0; i < (node->rearIndex != -1) + 1; i++) {
		if (i == 0) {
			base = node;
		} else {
			base = &sSynthSfxNodes[node->rearIndex];
		}
		for (j = 0; j < node->voice_count; j++) {
			if (base->voice[j] != voice) {
				sSynthVoicesPendingFree[sSynthPendingVoiceFreeCount++] = base->voice[j];
			}
			sSynthSfxNodes[base->voice[j]->index].sfxID = 0;
		}
	}
	OSRestoreInterrupts(enabled);
}

// one loaded sfx, the bank loader links these into the hash heads
typedef struct HSD_SynthSFXEntry {
	struct HSD_SynthSFXEntry* next; // _00
	s32 id;                         // _04
	int count;                      // _08
	s32 rate;                       // _0C
	struct {
		u8 addr[0x10];      // _00
		u8 adpcm[0x28];     // _10
		u8 adpcmloop[0x08]; // _38
	} sample[1];            // _10
} HSD_SynthSFXEntry;

extern void AXSetVoicePriority(AXVPB* p, u32 priority);
extern void AXSetVoiceVe(AXVPB* p, AXPBVE* ve);
extern void AXSetVoiceSrcType(AXVPB* p, u32 type);
extern void AXSetVoiceSrc(AXVPB* p, AXPBSRC* src);
extern void AXSetVoiceAddr(AXVPB* p, void* addr);
extern void AXSetVoiceAdpcm(AXVPB* p, void* adpcm);
extern void AXSetVoiceAdpcmLoop(AXVPB* p, void* loop);
extern void AXSetVoiceItdOn(AXVPB* p);
extern void AXSetVoiceState(AXVPB* p, u16 state);

/**
 * @note Address: 0x801ED444
 * @note Size: 0x740
 */
s32 StartSynthSoundEffect(s32 sfxID, s32 vol, s32 volMode, s32 vol2, s32 vol2Mode, u8 clamp1, u8 clamp2, u8 pan, f32 mixMain, f32 mixAuxA,
                f32 mixAuxB, u8 span, s32 priority, s32 channel, s32 rear, s32 unk16)
{
	AXVPB* voices[4] = { NULL, NULL, NULL, NULL };
	AXPBVE ve;
	HSD_SynthSFXEntry* entry;
	s32 uid;
	HSD_SynthSFXNode* node;
	s32 i;
	u32 enabled;
	s32 dup;
	HSD_SynthSFXNode* rearNode;
	s32 j;
	s32 k;
	s32 raw;

	dup = 0;
	if (channel < 0 || channel >= 0x40) {
		return 0;
	}

	enabled = OSDisableInterrupts();
	entry   = (HSD_SynthSFXEntry*)lbl_80656260[sfxID & 0x1F];
	if (sSynthSoundMode != 4) {
		rear = 0;
	}
	for (; entry != NULL; entry = entry->next) {
		if (entry->id != sfxID) {
			continue;
		}

		for (i = 0; i < entry->count * (rear + 1); i++) {
			voices[i] = AXAcquireVoice(priority + 1, dropcallback, 0);
			if (voices[i] == NULL) {
				for (i--; i >= 0; i--) {
					if (voices[i] != NULL) {
						AXFreeVoice(voices[i]);
					}
				}
				OSRestoreInterrupts(enabled);
				return -1;
			}
		}

		// the driver can hand out the same voice twice, drop the copies
		for (i = 0; i < entry->count * (rear + 1); i++) {
			for (j = i + 1; j < entry->count * (rear + 1); j++) {
				if (voices[i] == voices[j]) {
					dup       = 1;
					voices[j] = NULL;
				}
			}
		}
		if (dup) {
			for (i = 0; i < entry->count * (rear + 1); i++) {
				if (voices[i] != NULL) {
					AXFreeVoice(voices[i]);
				}
			}
			OSRestoreInterrupts(enabled);
			return -1;
		}

		if (entry->count == 1 && rear != 0) {
			voices[2] = voices[1];
			voices[1] = NULL;
		}
		if (entry->count == 2) {
			sSynthSfxNodes[voices[1]->index].sfxID    = -1;
			sSynthSfxNodes[voices[1]->index].voice[0] = voices[0];
		}

		uid                = voices[0]->index;
		node               = &sSynthSfxNodes[uid];
		node->link.pstream = NULL;
		node->next         = (HSD_SynthSFXNode*)sfxID;
		node->flags        = 0;
		node->voice_count  = entry->count;
		node->channel      = channel;
		node->voice[0]     = voices[0];
		node->voice[1]     = voices[1];
		node->freqBase     = entry->rate / 32000.0f;
		node->pitch[0]     = 1.0f;
		node->pitch[1]     = 1.0f;

		if (volMode == 0) {
			node->u.user_vol[0].volume   = 0.003921569f * vol;
			node->u.user_vol[0].target.f = node->u.user_vol[0].volume;
		} else {
			raw                          = vol << 16;
			node->u.user_vol[0].volume   = HSD_SynthDBxToVol(raw);
			node->u.user_vol[0].target.i = raw;
			node->u.user_vol[0].dbxRaw   = raw;
		}
		node->u.user_vol[0].mode      = volMode;
		node->u.user_vol[0].fadeCount = 0;

		if (vol2Mode == 0) {
			node->u.user_vol[1].volume   = 0.003921569f * vol2;
			node->u.user_vol[1].target.f = node->u.user_vol[1].volume;
		} else {
			raw                          = vol2 << 16;
			node->u.user_vol[1].volume   = HSD_SynthDBxToVol(raw);
			node->u.user_vol[1].target.i = raw;
			node->u.user_vol[1].dbxRaw   = raw;
		}
		node->u.user_vol[1].mode      = vol2Mode;
		node->u.user_vol[1].fadeCount = 0;

		node->userPan[0]  = clamp1 < 0xFE ? clamp1 : 0xFE;
		node->userPan[1]  = clamp2 < 0xFE ? clamp2 : 0xFE;
		node->userSPan[0] = pan;
		node->userSPan[1] = span;
		node->mixMain     = mixMain;
		node->mixAuxA     = mixAuxA;
		node->mixAuxB     = mixAuxB;
		node->unk16       = unk16;

		if (rear != 0) {
			k                   = voices[2]->index;
			rearNode            = &sSynthSfxNodes[k];
			rearNode->sfxID     = -2;
			rearNode->rearIndex = uid;
			node->rearIndex     = k;
			if (entry->count == 2) {
				sSynthSfxNodes[voices[3]->index].sfxID    = -3;
				sSynthSfxNodes[voices[3]->index].voice[0] = voices[2];
			}
			rearNode->voice_count = entry->count;
			rearNode->voice[0]    = voices[2];
			rearNode->voice[1]    = voices[3];
		} else {
			node->rearIndex = -1;
		}

		ve.currentVolume = 32767.0f
		                 * (node->u.user_vol[1].volume
		                    * (node->u.user_vol[0].volume
		                       * (lbl_80659318[node->channel].volume
		                          * (node->channel < 0x3F ? sSynthStreamVolume * lbl_80659300.volume : sSynthStreamVolume))));
		ve.currentDelta = 0;
		node->curVol    = ve.currentVolume;
		if (rear != 0) {
			rearNode->curVol = ve.currentVolume;
		}

		// ratioHi and ratioLo together as one 16.16 value
		*(u32*)&lbl_8050932C = 65536.0f * node->freqBase;

		for (k = 0; k < rear + 1; k++) {
			if (k == 1) {
				rearNode->u.rear.lastVol   = ve.currentVolume;
				rearNode->u.rear.lastDelta = ve.currentDelta;
				rearNode->u.rear.pitch     = node->freqBase;
				HSD_SynthSFXLinkUpdateRearPitch(rearNode, 0);
			}
			for (j = 0; j < entry->count; j++) {
				AXSetVoicePriority(voices[k * 2 + j], priority);
				AXSetVoiceVe(voices[k * 2 + j], &ve);
				AXSetVoiceSrcType(voices[k * 2 + j], 2);
				AXSetVoiceSrc(voices[k * 2 + j], &lbl_8050932C);
				AXSetVoiceAddr(voices[k * 2 + j], entry->sample[j].addr);
				AXSetVoiceAdpcm(voices[k * 2 + j], entry->sample[j].adpcm);
				AXSetVoiceAdpcmLoop(voices[k * 2 + j], entry->sample[j].adpcmloop);
				if (sSynthSoundMode == 1) {
					AXSetVoiceItdOn(voices[k * 2 + j]);
				}
				if (k == 0) {
					AXSetVoiceState(voices[k * 2 + j], 1);
				}
			}
		}

		HSD_SynthSFXUpdateVolume(node);
		HSD_SynthSFXInitMix(node);
		HSD_SynthSFXUpdateMix(node, 0);
		ApplySynthMixTargetImmediately(node);

		if ((sSynthSfxIdBase += 0x40) < 0) {
			sSynthSfxIdBase = 0x40;
		}
		node->sfxID = sSynthSfxIdBase + uid;
		OSRestoreInterrupts(enabled);
		return node->sfxID;
	}
	OSRestoreInterrupts(enabled);
	return -1;
}

/**
 * @note Address: 0x801EDB84
 * @note Size: 0x284
 */
void HSD_SynthSFXKeyOff(s32 id)
{
	HSD_SynthSFXNode* node;
	s32 i;
	u32 enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL) {
		if (node->flags & 8) {
			HSD_SynthSFXFreeVoice(node);
		} else if (!(node->flags & 1)) {
			node->flags |= 1;
			HSD_SynthSFXUpdateVolume(node);
			for (i = 0; i < node->voice_count; i++) {
				AXSetVoicePriority(node->voice[i], 1);
			}
			if (node->link.pstream != NULL) {
				releasePStream(PSTREAM(node));
				node->link.pstream = NULL;
			}
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801ED160
 * @note Size: 0x2E4
 */
// dropcallback
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    stw      r31, 0x1c(r1)
    stw      r30, 0x18(r1)
    stw      r29, 0x14(r1)
    mr       r29, r3
    bl       OSDisableInterrupts
    lwz      r0, 0x18(r29)
    lis      r4, sSynthVoicesPendingFree@ha
    lis      r5, sSynthSfxNodes@ha
    mr       r30, r3
    mulli    r6, r0, 0x98
    addi     r4, r4, sSynthVoicesPendingFree@l
    addi     r0, r5, sSynthSfxNodes@l
    lwz      r5, sSynthPendingVoiceFreeCount@sda21(r0)
    add      r31, r0, r6
    li       r6, 0x0
    mtctr    r5
    cmpwi    r5, 0x0
    ble      .L_801ED1E4
.L_801ED1B4:
    lwz      r0, 0x0(r4)
    cmplw    r0, r29
    bne      .L_801ED1D8
    lis      r3, sSynthVoicesPendingFree@ha
    slwi     r0, r6, 2
    addi     r3, r3, sSynthVoicesPendingFree@l
    li       r4, 0x0
    stwx     r4, r3, r0
    b        .L_801ED1E4
.L_801ED1D8:
    addi     r4, r4, 0x4
    addi     r6, r6, 0x1
    bdnz     .L_801ED1B4
.L_801ED1E4:
    lwz      r0, 0x0(r31)
    cmpwi    r0, 0x0
    bne      .L_801ED1FC
    mr       r3, r30
    bl       OSRestoreInterrupts
    b        .L_801ED428
.L_801ED1FC:
    cmpwi    r0, -0x1
    bne      .L_801ED21C
    lwz      r4, 0xc(r31)
    lis      r3, sSynthSfxNodes@ha
    addi     r0, r3, sSynthSfxNodes@l
    lwz      r3, 0x18(r4)
    mulli    r3, r3, 0x98
    add      r31, r0, r3
.L_801ED21C:
    lwz      r0, 0x0(r31)
    cmpwi    r0, -0x1
    bge      .L_801ED24C
    lwz      r3, 0xc(r31)
    lis      r4, sSynthSfxNodes@ha
    addi     r4, r4, sSynthSfxNodes@l
    lwz      r0, 0x18(r3)
    mulli    r0, r0, 0x98
    add      r3, r4, r0
    lha      r0, 0x14(r3)
    mulli    r0, r0, 0x98
    add      r31, r4, r0
.L_801ED24C:
    lwz      r0, 0x30(r31)
    cmplwi   r0, 0x0
    beq      .L_801ED278
    lis      r3, lbl_8025BE44@ha
    addi     r3, r3, lbl_8025BE44@l
    crclr    cr1eq
    bl       OSReport
    li       r3, sPathSynthC@sda21
    li       r4, 0x636
    li       r5, sSynthFalseCondition@sda21
    bl       __assert
.L_801ED278:
    lbz      r0, 0x9(r31)
    clrlwi.  r0, r0, 31
    bne      .L_801ED2A8
    lwz      r0, 0x30(r31)
    cmplwi   r0, 0x0
    bne      .L_801ED2A8
    lwz      r12, sSynthDriverInactivatedCallback@sda21(r0)
    cmplwi   r12, 0x0
    beq      .L_801ED2A8
    lwz      r3, 0x0(r31)
    mtctr    r12
    bctrl
.L_801ED2A8:
    lbz      r0, 0x2e(r31)
    cmplwi   r0, 0x0
    beq      .L_801ED2E8
    li       r3, sSynthPendingVolumeUpdates@sda21
    b        .L_801ED2DC
.L_801ED2BC:
    cmplw    r4, r31
    bne      .L_801ED2D8
    li       r0, 0x0
    stb      r0, 0x2e(r31)
    lwz      r0, 0x24(r31)
    stw      r0, 0x0(r3)
    b        .L_801ED2E8
.L_801ED2D8:
    addi     r3, r4, 0x24
.L_801ED2DC:
    lwz      r4, 0x0(r3)
    cmplwi   r4, 0x0
    bne      .L_801ED2BC
.L_801ED2E8:
    lbz      r0, 0x2f(r31)
    cmplwi   r0, 0x0
    beq      .L_801ED328
    li       r4, sSynthPendingMixUpdates@sda21
    b        .L_801ED31C
.L_801ED2FC:
    cmplw    r3, r31
    bne      .L_801ED318
    li       r0, 0x0
    stb      r0, 0x2f(r31)
    lwz      r0, 0x28(r31)
    stw      r0, 0x0(r4)
    b        .L_801ED328
.L_801ED318:
    addi     r4, r3, 0x28
.L_801ED31C:
    lwz      r3, 0x0(r4)
    cmplwi   r3, 0x0
    bne      .L_801ED2FC
.L_801ED328:
    lha      r0, 0x14(r31)
    cmpwi    r0, -0x1
    beq      .L_801ED370
    mulli    r4, r0, 0x98
    lis      r3, sSynthSfxNodes@ha
    li       r5, sSynthPendingRearPitchUpdates@sda21
    addi     r0, r3, sSynthSfxNodes@l
    add      r3, r0, r4
    b        .L_801ED364
.L_801ED34C:
    cmplw    r4, r3
    bne      .L_801ED360
    lwz      r0, 0x30(r3)
    stw      r0, 0x0(r5)
    b        .L_801ED370
.L_801ED360:
    addi     r5, r4, 0x30
.L_801ED364:
    lwz      r4, 0x0(r5)
    cmplwi   r4, 0x0
    bne      .L_801ED34C
.L_801ED370:
    lis      r4, sSynthVoicesPendingFree@ha
    lis      r3, sSynthSfxNodes@ha
    addi     r6, r4, sSynthVoicesPendingFree@l
    li       r11, 0x0
    addi     r4, r3, sSynthSfxNodes@l
    b        .L_801ED400
.L_801ED388:
    cmpwi    r11, 0x0
    bne      .L_801ED398
    mr       r0, r31
    b        .L_801ED3A8
.L_801ED398:
    mulli    r5, r5, 0x98
    lis      r3, sSynthSfxNodes@ha
    addi     r0, r3, sSynthSfxNodes@l
    add      r0, r0, r5
.L_801ED3A8:
    li       r10, 0x0
    mr       r8, r0
    mr       r5, r10
    b        .L_801ED3F0
.L_801ED3B8:
    lwz      r9, 0xc(r8)
    cmplw    r9, r29
    beq      .L_801ED3D8
    lwz      r7, sSynthPendingVoiceFreeCount@sda21(r0)
    addi     r3, r7, 0x1
    slwi     r0, r7, 2
    stw      r3, sSynthPendingVoiceFreeCount@sda21(r0)
    stwx     r9, r6, r0
.L_801ED3D8:
    lwz      r3, 0xc(r8)
    addi     r8, r8, 0x4
    addi     r10, r10, 0x1
    lwz      r0, 0x18(r3)
    mulli    r0, r0, 0x98
    stwx     r5, r4, r0
.L_801ED3F0:
    lbz      r0, 0xa(r31)
    cmpw     r10, r0
    blt      .L_801ED3B8
    addi     r11, r11, 0x1
.L_801ED400:
    lha      r5, 0x14(r31)
    subfic   r3, r5, -0x1
    addi     r0, r5, 0x1
    or       r0, r3, r0
    srwi     r3, r0, 31
    addi     r0, r3, 0x1
    cmpw     r11, r0
    blt      .L_801ED388
    mr       r3, r30
    bl       OSRestoreInterrupts
.L_801ED428:
    lwz      r0, 0x24(r1)
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: 0x801ED444
 * @note Size: 0x740
 */
// StartSynthSoundEffect
/*
    stwu     r1, -0xa0(r1)
    mflr     r0
    stw      r0, 0xa4(r1)
    stfd     f31, 0x90(r1)
    psq_st   f31, 0x98(r1), 0, qr0
    stfd     f30, 0x80(r1)
    psq_st   f30, 0x88(r1), 0, qr0
    stfd     f29, 0x70(r1)
    psq_st   f29, 0x78(r1), 0, qr0
    addi     r11, r1, 0x70
    bl       _savegpr_14
    lis      r11, "@585"@ha
    lwzu     r0, "@585"@l(r11)
    lwz      r19, 0xb0(r1)
    lis      r22, hsd_SynthSFXLoadBuf@ha
    lwz      r14, 0x4(r11)
    addi     r29, r22, hsd_SynthSFXLoadBuf@l
    lwz      r12, 0x8(r11)
    fmr      f29, f1
    lwz      r11, 0xc(r11)
    fmr      f30, f2
    stw      r14, 0x14(r1)
    fmr      f31, f3
    cmpwi    r19, 0x0
    stw      r0, 0x10(r1)
    mr       r15, r3
    lwz      r20, 0xb4(r1)
    mr       r16, r4
    stw      r12, 0x18(r1)
    mr       r17, r5
    mr       r18, r6
    mr       r14, r7
    stw      r11, 0x1c(r1)
    li       r22, 0x0
    stb      r8, 0x8(r1)
    stb      r9, 0x9(r1)
    stb      r10, 0xa(r1)
    blt      .L_801ED4E4
    cmpwi    r19, 0x40
    blt      .L_801ED4EC
.L_801ED4E4:
    li       r3, 0x0
    b        .L_801EDB54
.L_801ED4EC:
    bl       OSDisableInterrupts
    lwz      r0, sSynthSoundMode@sda21(r0)
    clrlslwi r5, r15, 27, 2
    addi     r4, r29, 0x320
    mr       r23, r3
    cmpwi    r0, 0x4
    lwzx     r25, r4, r5
    beq      .L_801EDB40
    li       r20, 0x0
    b        .L_801EDB40
.L_801ED514:
    lwz      r0, 0x4(r25)
    cmpw     r0, r15
    bne      .L_801EDB3C
    lwz      r3, 0xac(r1)
    addi     r26, r1, 0x10
    lis      r4, dropcallback@ha
    addi     r31, r20, 0x1
    mr       r28, r26
    addi     r27, r3, 0x1
    addi     r30, r4, dropcallback@l
    li       r24, 0x0
    b        .L_801ED5B0
.L_801ED544:
    mr       r3, r27
    mr       r4, r30
    li       r5, 0x0
    bl       AXAcquireVoice
    stw      r3, 0x0(r28)
    lwz      r0, 0x0(r28)
    cmplwi   r0, 0x0
    bne      .L_801ED5A8
    subi     r24, r24, 0x1
    addi     r14, r1, 0x10
    slwi     r0, r24, 2
    add      r14, r14, r0
    b        .L_801ED590
.L_801ED578:
    lwz      r3, 0x0(r14)
    cmplwi   r3, 0x0
    beq      .L_801ED588
    bl       AXFreeVoice
.L_801ED588:
    subi     r14, r14, 0x4
    subi     r24, r24, 0x1
.L_801ED590:
    cmpwi    r24, 0x0
    bge      .L_801ED578
    mr       r3, r23
    bl       OSRestoreInterrupts
    li       r3, -0x1
    b        .L_801EDB54
.L_801ED5A8:
    addi     r28, r28, 0x4
    addi     r24, r24, 0x1
.L_801ED5B0:
    lwz      r0, 0x8(r25)
    mullw    r0, r0, r31
    cmpw     r24, r0
    blt      .L_801ED544
    li       r8, 0x0
    mr       r7, r26
    mr       r4, r8
    addi     r3, r20, 0x1
    b        .L_801ED620
.L_801ED5D4:
    addi     r9, r8, 0x1
    addi     r6, r1, 0x10
    slwi     r0, r9, 2
    add      r6, r6, r0
    b        .L_801ED608
.L_801ED5E8:
    lwz      r5, 0x0(r7)
    lwz      r0, 0x0(r6)
    cmplw    r5, r0
    bne      .L_801ED600
    li       r22, 0x1
    stw      r4, 0x0(r6)
.L_801ED600:
    addi     r6, r6, 0x4
    addi     r9, r9, 0x1
.L_801ED608:
    lwz      r0, 0x8(r25)
    mullw    r0, r0, r3
    cmpw     r9, r0
    blt      .L_801ED5E8
    addi     r7, r7, 0x4
    addi     r8, r8, 0x1
.L_801ED620:
    lwz      r5, 0x8(r25)
    mullw    r0, r5, r3
    cmpw     r8, r0
    blt      .L_801ED5D4
    cmpwi    r22, 0x0
    beq      .L_801ED67C
    addi     r14, r20, 0x1
    li       r15, 0x0
    b        .L_801ED65C
.L_801ED644:
    lwz      r3, 0x0(r26)
    cmplwi   r3, 0x0
    beq      .L_801ED654
    bl       AXFreeVoice
.L_801ED654:
    addi     r26, r26, 0x4
    addi     r15, r15, 0x1
.L_801ED65C:
    lwz      r0, 0x8(r25)
    mullw    r0, r0, r14
    cmpw     r15, r0
    blt      .L_801ED644
    mr       r3, r23
    bl       OSRestoreInterrupts
    li       r3, -0x1
    b        .L_801EDB54
.L_801ED67C:
    cmpwi    r5, 0x1
    bne      .L_801ED69C
    cmpwi    r20, 0x0
    beq      .L_801ED69C
    lwz      r3, 0x14(r1)
    li       r0, 0x0
    stw      r3, 0x18(r1)
    stw      r0, 0x14(r1)
.L_801ED69C:
    lwz      r0, 0x8(r25)
    cmpwi    r0, 0x2
    bne      .L_801ED6D4
    lwz      r6, 0x14(r1)
    addi     r3, r29, 0x8c0
    li       r5, -0x1
    lwz      r4, 0x10(r1)
    lwz      r0, 0x18(r6)
    mulli    r0, r0, 0x98
    stwx     r5, r3, r0
    lwz      r0, 0x18(r6)
    mulli    r0, r0, 0x98
    add      r3, r3, r0
    stw      r4, 0xc(r3)
.L_801ED6D4:
    lwz      r6, 0x10(r1)
    lis      r3, 0x4330
    addi     r24, r29, 0x8c0
    li       r4, 0x0
    lwz      r22, 0x18(r6)
    cmpwi    r17, 0x0
    lwz      r0, 0x14(r1)
    mulli    r5, r22, 0x98
    stw      r3, 0x20(r1)
    lfd      f2, "@237"@sda21(r0)
    lfs      f0, lbl_8065FEBC@sda21(r0)
    add      r24, r24, r5
    lfs      f3, "@223"@sda21(r0)
    stw      r4, 0x30(r24)
    stw      r15, 0x4(r24)
    stb      r4, 0x9(r24)
    lwz      r4, 0x8(r25)
    stb      r4, 0xa(r24)
    stb      r19, 0xb(r24)
    stw      r6, 0xc(r24)
    stw      r0, 0x10(r24)
    lwz      r0, 0xc(r25)
    xoris    r0, r0, 0x8000
    stw      r0, 0x24(r1)
    lfd      f1, 0x20(r1)
    fsubs    f1, f1, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x18(r24)
    stfs     f3, 0x1c(r24)
    stfs     f3, 0x20(r24)
    bne      .L_801ED77C
    xoris    r0, r16, 0x8000
    stw      r3, 0x20(r1)
    lfs      f1, lbl_8065FEC0@sda21(r0)
    stw      r0, 0x24(r1)
    lfd      f0, 0x20(r1)
    fsubs    f0, f0, f2
    fmuls    f0, f1, f0
    stfs     f0, 0x34(r24)
    lfs      f0, 0x34(r24)
    stfs     f0, 0x3c(r24)
    b        .L_801ED7D0
.L_801ED77C:
    lwz      r0, sSynthDecibelFloor@sda21(r0)
    slwi     r15, r16, 16
    cmpw     r15, r0
    bgt      .L_801ED794
    lfs      f3, "@234"@sda21(r0)
    b        .L_801ED7C4
.L_801ED794:
    cmpwi    r15, 0x0
    blt      .L_801ED7A0
    b        .L_801ED7C4
.L_801ED7A0:
    xoris    r0, r15, 0x8000
    stw      r3, 0x20(r1)
    lfd      f1, "@235"@sda21(r0)
    stw      r0, 0x24(r1)
    lfd      f0, 0x20(r1)
    fsub     f0, f0, f2
    fmul     f1, f1, f0
    bl       exp
    frsp     f3, f1
.L_801ED7C4:
    stfs     f3, 0x34(r24)
    stw      r15, 0x3c(r24)
    stw      r15, 0x40(r24)
.L_801ED7D0:
    stw      r17, 0x38(r24)
    li       r0, 0x0
    cmpwi    r14, 0x0
    stw      r0, 0x44(r24)
    bne      .L_801ED818
    xoris    r3, r18, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x24(r1)
    lfd      f1, "@237"@sda21(r0)
    stw      r0, 0x20(r1)
    lfs      f2, lbl_8065FEC0@sda21(r0)
    lfd      f0, 0x20(r1)
    fsubs    f0, f0, f1
    fmuls    f0, f2, f0
    stfs     f0, 0x4c(r24)
    lfs      f0, 0x4c(r24)
    stfs     f0, 0x54(r24)
    b        .L_801ED878
.L_801ED818:
    lwz      r0, sSynthDecibelFloor@sda21(r0)
    slwi     r15, r18, 16
    cmpw     r15, r0
    bgt      .L_801ED830
    lfs      f0, "@234"@sda21(r0)
    b        .L_801ED86C
.L_801ED830:
    cmpwi    r15, 0x0
    blt      .L_801ED840
    lfs      f0, "@223"@sda21(r0)
    b        .L_801ED86C
.L_801ED840:
    xoris    r3, r15, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x24(r1)
    lfd      f1, "@237"@sda21(r0)
    stw      r0, 0x20(r1)
    lfd      f2, "@235"@sda21(r0)
    lfd      f0, 0x20(r1)
    fsub     f0, f0, f1
    fmul     f1, f2, f0
    bl       exp
    frsp     f0, f1
.L_801ED86C:
    stfs     f0, 0x4c(r24)
    stw      r15, 0x54(r24)
    stw      r15, 0x58(r24)
.L_801ED878:
    lbz      r0, 0x8(r1)
    li       r4, 0x0
    stw      r14, 0x50(r24)
    li       r3, 0xfe
    cmplwi   r0, 0xfe
    stw      r4, 0x5c(r24)
    bge      .L_801ED898
    mr       r3, r0
.L_801ED898:
    lbz      r0, 0x9(r1)
    stb      r3, 0x64(r24)
    li       r3, 0xfe
    cmplwi   r0, 0xfe
    bge      .L_801ED8B0
    mr       r3, r0
.L_801ED8B0:
    stb      r3, 0x65(r24)
    cmpwi    r20, 0x0
    lbz      r0, 0xa(r1)
    stb      r0, 0x66(r24)
    lbz      r0, 0xab(r1)
    stb      r0, 0x67(r24)
    lwz      r0, 0xb8(r1)
    stfs     f29, 0x68(r24)
    stfs     f30, 0x6c(r24)
    stfs     f31, 0x70(r24)
    stb      r0, 0x16(r24)
    beq      .L_801ED954
    lwz      r6, 0x18(r1)
    addi     r21, r29, 0x8c0
    li       r0, -0x2
    lwz      r4, 0x18(r6)
    mulli    r3, r4, 0x98
    add      r21, r21, r3
    stw      r0, 0x0(r21)
    sth      r22, 0x14(r21)
    sth      r4, 0x14(r24)
    lwz      r0, 0x8(r25)
    cmpwi    r0, 0x2
    bne      .L_801ED938
    lwz      r5, 0x1c(r1)
    addi     r3, r29, 0x8c0
    li       r4, -0x3
    lwz      r0, 0x18(r5)
    mulli    r0, r0, 0x98
    stwx     r4, r3, r0
    lwz      r0, 0x18(r5)
    mulli    r0, r0, 0x98
    add      r3, r3, r0
    stw      r6, 0xc(r3)
.L_801ED938:
    lwz      r0, 0x8(r25)
    lwz      r3, 0x18(r1)
    stb      r0, 0xa(r21)
    lwz      r0, 0x1c(r1)
    stw      r3, 0xc(r21)
    stw      r0, 0x10(r21)
    b        .L_801ED95C
.L_801ED954:
    li       r0, -0x1
    sth      r0, 0x14(r24)
.L_801ED95C:
    lbz      r0, 0xb(r24)
    cmplwi   r0, 0x3f
    bge      .L_801ED978
    lfs      f1, sSynthStreamVolume@sda21(r0)
    lfs      f0, 0x33c0(r29)
    fmuls    f4, f1, f0
    b        .L_801ED97C
.L_801ED978:
    lfs      f4, sSynthStreamVolume@sda21(r0)
.L_801ED97C:
    mulli    r4, r0, 0x18
    addi     r3, r29, 0x33d8
    li       r0, 0x0
    lfs      f1, 0x34(r24)
    lfs      f2, 0x4c(r24)
    cmpwi    r20, 0x0
    lfsx     f0, r3, r4
    lfs      f3, lbl_8065FEC4@sda21(r0)
    fmuls    f0, f0, f4
    sth      r0, 0xe(r1)
    fmuls    f0, f1, f0
    fmuls    f0, f2, f0
    fmuls    f0, f3, f0
    fctiwz   f0, f0
    stfd     f0, 0x20(r1)
    lwz      r0, 0x24(r1)
    sth      r0, 0xc(r1)
    sth      r0, 0x2c(r24)
    beq      .L_801ED9CC
    sth      r0, 0x2c(r21)
.L_801ED9CC:
    lfs      f1, lbl_8065FEC8@sda21(r0)
    lfs      f0, 0x18(r24)
    fmuls    f1, f1, f0
    bl       __cvt_fp2unsigned
    lis      r4, lbl_8025BE34@ha
    li       r17, 0x0
    addi     r26, r4, lbl_8025BE34@l
    lwz      r14, 0xac(r1)
    stw      r3, 0x0(r26)
    mr       r15, r17
    addi     r16, r1, 0x10
    addi     r19, r20, 0x1
    b        .L_801EDADC
.L_801EDA00:
    cmpwi    r17, 0x1
    bne      .L_801EDA28
    lhz      r4, 0xc(r1)
    mr       r3, r21
    lha      r0, 0xe(r1)
    sth      r4, 0x4a(r21)
    sth      r0, 0x4c(r21)
    lfs      f0, 0x18(r24)
    stfs     f0, 0x50(r21)
    bl       HSD_SynthSFXLinkUpdateRearPitch
.L_801EDA28:
    mr       r20, r15
    mr       r27, r25
    slwi     r18, r15, 2
    li       r28, 0x0
    b        .L_801EDAC8
.L_801EDA3C:
    lwzx     r3, r16, r18
    mr       r4, r14
    bl       AXSetVoicePriority
    lwzx     r3, r16, r18
    addi     r4, r1, 0xc
    bl       AXSetVoiceVe
    lwzx     r3, r16, r18
    li       r4, 0x2
    bl       AXSetVoiceSrcType
    lwzx     r3, r16, r18
    mr       r4, r26
    bl       AXSetVoiceSrc
    lwzx     r3, r16, r18
    addi     r4, r27, 0x10
    bl       AXSetVoiceAddr
    lwzx     r3, r16, r18
    addi     r4, r27, 0x20
    bl       AXSetVoiceAdpcm
    lwzx     r3, r16, r18
    addi     r4, r27, 0x48
    bl       AXSetVoiceAdpcmLoop
    lwz      r0, sSynthSoundMode@sda21(r0)
    cmpwi    r0, 0x1
    bne      .L_801EDAA4
    lwzx     r3, r16, r18
    bl       AXSetVoiceItdOn
.L_801EDAA4:
    cmpwi    r17, 0x0
    bne      .L_801EDAB8
    lwzx     r3, r16, r18
    li       r4, 0x1
    bl       AXSetVoiceState
.L_801EDAB8:
    addi     r20, r20, 0x1
    addi     r18, r18, 0x4
    addi     r27, r27, 0x40
    addi     r28, r28, 0x1
.L_801EDAC8:
    lwz      r0, 0x8(r25)
    cmpw     r28, r0
    blt      .L_801EDA3C
    addi     r15, r15, 0x2
    addi     r17, r17, 0x1
.L_801EDADC:
    cmpw     r17, r19
    blt      .L_801EDA00
    mr       r3, r24
    bl       HSD_SynthSFXUpdateVolume
    mr       r3, r24
    bl       HSD_SynthSFXInitMix
    mr       r3, r24
    li       r4, 0x0
    bl       HSD_SynthSFXUpdateMix
    mr       r3, r24
    bl       ApplySynthMixTargetImmediately
    lwz      r3, sSynthSfxIdBase@sda21(r0)
    addic.   r0, r3, 0x40
    stw      r0, sSynthSfxIdBase@sda21(r0)
    bge      .L_801EDB20
    li       r0, 0x40
    stw      r0, sSynthSfxIdBase@sda21(r0)
.L_801EDB20:
    lwz      r0, sSynthSfxIdBase@sda21(r0)
    mr       r3, r23
    add      r0, r0, r22
    stw      r0, 0x0(r24)
    bl       OSRestoreInterrupts
    lwz      r3, 0x0(r24)
    b        .L_801EDB54
.L_801EDB3C:
    lwz      r25, 0x0(r25)
.L_801EDB40:
    cmplwi   r25, 0x0
    bne      .L_801ED514
    mr       r3, r23
    bl       OSRestoreInterrupts
    li       r3, -0x1
.L_801EDB54:
    psq_l    f31, 0x98(r1), 0, qr0
    lfd      f31, 0x90(r1)
    psq_l    f30, 0x88(r1), 0, qr0
    lfd      f30, 0x80(r1)
    psq_l    f29, 0x78(r1), 0, qr0
    addi     r11, r1, 0x70
    lfd      f29, 0x70(r1)
    bl       _restgpr_14
    lwz      r0, 0xa4(r1)
    mtlr     r0
    addi     r1, r1, 0xa0
    blr
*/

/**
 * @note Address: 0x801EDB84
 * @note Size: 0x284
 */
// HSD_SynthSFXKeyOff
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    stw      r0, 0x34(r1)
    addi     r11, r1, 0x30
    bl       _savegpr_27
    lis      r4, hsd_SynthSFXLoadBuf@ha
    mr       r28, r3
    addi     r31, r4, hsd_SynthSFXLoadBuf@l
    bl       OSDisableInterrupts
    clrlwi   r0, r28, 26
    cmpwi    r28, 0x0
    mulli    r0, r0, 0x98
    addi     r30, r31, 0x8c0
    mr       r27, r3
    add      r30, r30, r0
    ble      .L_801EDBD4
    lwz      r0, 0x0(r30)
    cmpw     r0, r28
    bne      .L_801EDBD4
    b        .L_801EDBD8
.L_801EDBD4:
    li       r30, 0x0
.L_801EDBD8:
    cmplwi   r30, 0x0
    beq      .L_801EDDE8
    lbz      r3, 0x9(r30)
    rlwinm.  r0, r3, 0, 28, 28
    beq      .L_801EDBF8
    mr       r3, r30
    bl       HSD_SynthSFXFreeVoice
    b        .L_801EDDE8
.L_801EDBF8:
    clrlwi.  r0, r3, 31
    bne      .L_801EDDE8
    ori      r0, r3, 0x1
    mr       r3, r30
    stb      r0, 0x9(r30)
    bl       HSD_SynthSFXUpdateVolume
    mr       r28, r30
    li       r29, 0x0
    b        .L_801EDC30
.L_801EDC1C:
    lwz      r3, 0xc(r28)
    li       r4, 0x1
    bl       AXSetVoicePriority
    addi     r28, r28, 0x4
    addi     r29, r29, 0x1
.L_801EDC30:
    lbz      r0, 0xa(r30)
    cmpw     r29, r0
    blt      .L_801EDC1C
    lwz      r28, 0x30(r30)
    cmplwi   r28, 0x0
    beq      .L_801EDDE8
    lwz      r3, 0xc(r28)
    li       r29, sSynthActivePStreams@sda21
    cmpwi    r3, 0x0
    blt      .L_801EDC64
    bl       ReleaseDvdStreamingSlot
    b        .L_801EDC64
.L_801EDC60:
    mr       r29, r0
.L_801EDC64:
    lwz      r0, 0x0(r29)
    cmplwi   r0, 0x0
    beq      .L_801EDC78
    cmplw    r0, r28
    bne      .L_801EDC60
.L_801EDC78:
    cmplwi   r0, 0x0
    beq      .L_801EDD0C
    addi     r6, r31, 0x480
    addi     r4, r31, 0x424
    b        .L_801EDCF0
.L_801EDC8C:
    slwi     r3, r7, 6
    addi     r9, r3, 0x20
    lwzx     r5, r6, r9
    b        .L_801EDCB0
.L_801EDC9C:
    lwz      r3, 0x0(r5)
    lwz      r0, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r0, 0x0(r5)
    stw      r5, sSynthFreeBeatMarkerBlocks@sda21(r0)
    mr       r5, r3
.L_801EDCB0:
    cmplwi   r5, 0x0
    bne      .L_801EDC9C
    li       r0, 0x0
    stwx     r0, r6, r9
    b        .L_801EDCD0
.L_801EDCC4:
    slwi     r0, r0, 2
    addi     r8, r31, 0x424
    add      r8, r8, r0
.L_801EDCD0:
    lwz      r0, 0x0(r8)
    cmpw     r0, r7
    bne      .L_801EDCC4
    slwi     r5, r0, 2
    li       r0, -0x1
    lwzx     r3, r4, r5
    stw      r3, 0x0(r8)
    stwx     r0, r4, r5
.L_801EDCF0:
    lwz      r3, 0x0(r29)
    addi     r8, r3, 0x10
    lwz      r7, 0x10(r3)
    cmpwi    r7, -0x2
    bne      .L_801EDC8C
    lwz      r0, 0x0(r3)
    stw      r0, 0x0(r29)
.L_801EDD0C:
    lwz      r0, sSynthFreePStreams@sda21(r0)
    stw      r0, 0x0(r28)
    stw      r28, sSynthFreePStreams@sda21(r0)
    lwz      r0, sSynthQueuedStreamPlaybackId@sda21(r0)
    lwz      r3, 0x4(r28)
    lwz      r3, 0x0(r3)
    cmpw     r3, r0
    bne      .L_801EDDE0
    lwz      r3, sSynthQueuedPStreamRequests@sda21(r0)
    li       r4, sSynthQueuedPStreamRequests@sda21
    b        .L_801EDD50
.L_801EDD38:
    cmplw    r0, r3
    bne      .L_801EDD4C
    lwz      r0, 0x0(r3)
    stw      r0, 0x0(r4)
    b        .L_801EDD5C
.L_801EDD4C:
    mr       r4, r0
.L_801EDD50:
    lwz      r0, 0x0(r4)
    cmplwi   r0, 0x0
    bne      .L_801EDD38
.L_801EDD5C:
    lwz      r0, sSynthFreePStreamRequests@sda21(r0)
    stw      r0, 0x0(r3)
    stw      r3, sSynthFreePStreamRequests@sda21(r0)
    bl       OSDisableInterrupts
    lwz      r11, sSynthQueuedPStreamRequests@sda21(r0)
    mr       r28, r3
    cmplwi   r11, 0x0
    beq      .L_801EDDD0
    lbz      r0, 0x11(r11)
    stw      r0, 0x8(r1)
    lha      r0, 0x28(r11)
    stw      r0, 0xc(r1)
    lha      r0, 0x2a(r11)
    stw      r0, 0x10(r1)
    lwz      r3, 0x4(r11)
    lha      r4, 0x8(r11)
    lbz      r5, 0xc(r11)
    lha      r6, 0xa(r11)
    lbz      r7, 0xd(r11)
    lbz      r8, 0xe(r11)
    lbz      r9, 0xf(r11)
    lbz      r10, 0x10(r11)
    lfs      f1, 0x14(r11)
    lfs      f2, 0x18(r11)
    lfs      f3, 0x1c(r11)
    lfs      f4, 0x20(r11)
    lfs      f5, 0x24(r11)
    bl       HSD_SynthPStreamStart
    b        .L_801EDDD4
.L_801EDDD0:
    li       r3, -0x1
.L_801EDDD4:
    stw      r3, sSynthQueuedStreamPlaybackId@sda21(r0)
    mr       r3, r28
    bl       OSRestoreInterrupts
.L_801EDDE0:
    li       r0, 0x0
    stw      r0, 0x30(r30)
.L_801EDDE8:
    mr       r3, r27
    bl       OSRestoreInterrupts
    addi     r11, r1, 0x30
    bl       _restgpr_27
    lwz      r0, 0x34(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: 0x801EDE08
 * @note Size: 0x8C
 */
void HSD_SynthSFXStopRange(u32 lo, u32 hi)
{
	s32 i;
	u32 enabled = OSDisableInterrupts();

	for (i = 0; i < HSD_SYNTHSFXNODE_MAX; i++) {
		HSD_SynthSFXNode* node = &sSynthSfxNodes[i];
		if (node->sfxID > 0) {
			u32 addr = AXVPB_CURRENT_ADDR(node->voice[0]);
			if (addr >= lo && addr < hi) {
				HSD_SynthSFXFreeVoice(node);
			}
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EDE94
 * @note Size: 0x40
 */
void HSD_SynthSFXStopBank(s32 bankID)
{
	HSD_SynthSFXStopRange(hsd_SynthSFXBankHead[bankID] * 2, hsd_SynthSFXBankHead[bankID + 1] * 2);
}

/**
 * @note Address: 0x801EDED4
 * @note Size: 0xA8
 */
void HSD_SynthSFXPause(s32 id)
{
	HSD_SynthSFXNode* node;
	u32 enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL) {
		node->flags |= 2;
		HSD_SynthSFXUpdateVolume(node);
		if (node->flags & 8) {
			node->flags |= 6;
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EDF7C
 * @note Size: 0xA4
 */
void HSD_SynthSFXResume(s32 id)
{
	HSD_SynthSFXNode* node;
	u32 enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL) {
		if (node->flags & 2) {
			node->flags &= ~6;
			HSD_SynthSFXUpdatePitch(node);
			HSD_SynthSFXUpdateVolume(node);
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EE020
 * @note Size: 0xBC
 */
s32 HSD_SynthSFXCheck(s32 id)
{
	HSD_SynthSFXNode* node;
	s32 i;
	s32 result  = id;
	u32 enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL) {
		if (node->flags & 1) {
			result = -1;
		} else if (!(node->flags & 8)) {
			for (i = 0; i < node->voice_count; i++) {
				if (AXVPB_PB_STATE(node->voice[i]) == 0) {
					result = -1;
				}
			}
		}
	} else {
		result = -1;
	}
	OSRestoreInterrupts(enabled);
	return result;
}

/**
 * @note Address: 0x801EE0DC
 * @note Size: 0x220
 */
void HSD_SynthSetVolFader(s32 id, s32 vol, s32 fadeFrames, s32 flag, s32 dbMode)
{
	HSD_SynthSFXNode* node;
	s32 frames;
	s32 changed;
	u32 enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL) {
		ASSERT_SYNTH(0x7EB, flag >= 0 && flag < USERVOL_NUM);

		frames  = fadeFrames;
		changed = 0;
		if (frames < 1) {
			frames = 1;
		}
		if (dbMode == 0) {
			node->u.user_vol[flag].target.f = (f32)(0.00392156862745098 * (f64)vol);
			if (node->u.user_vol[flag].volume != node->u.user_vol[flag].target.f) {
				node->u.user_vol[flag].fadeCount = frames;
				changed                          = 1;
			}
		} else {
			node->u.user_vol[flag].target.i = HSD_SynthVolToDBx(node->u.user_vol[flag].volume);
			node->u.user_vol[flag].dbxRaw   = vol << 0x10;
			if (node->u.user_vol[flag].volume != HSD_SynthDBxToVol(node->u.user_vol[flag].dbxRaw)) {
				changed                          = 1;
				node->u.user_vol[flag].fadeCount = frames;
			}
		}
		node->u.user_vol[flag].mode = dbMode;
		if (changed) {
			HSD_SynthSFXUpdateVolume(node);
		}
	}
	OSRestoreInterrupts(enabled);
}

// HSD_SynthSetVolFader
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    stw      r0, 0x44(r1)
    addi     r11, r1, 0x40
    bl       _savegpr_23
    mr       r28, r3
    mr       r23, r4
    mr       r27, r5
    mr       r24, r6
    mr       r25, r7
    bl       OSDisableInterrupts
    clrlwi   r0, r28, 26
    lis      r4, sSynthSfxNodes@ha
    mulli    r5, r0, 0x98
    cmpwi    r28, 0x0
    addi     r0, r4, sSynthSfxNodes@l
    mr       r26, r3
    add      r31, r0, r5
    ble      .L_801EE138
    lwz      r0, 0x0(r31)
    cmpw     r0, r28
    bne      .L_801EE138
    b        .L_801EE13C
.L_801EE138:
    li       r31, 0x0
.L_801EE13C:
    cmplwi   r31, 0x0
    beq      .L_801EE2DC
    cmpwi    r24, 0x0
    li       r0, 0x0
    blt      .L_801EE15C
    cmpwi    r24, 0x2
    bge      .L_801EE15C
    li       r0, 0x1
.L_801EE15C:
    cmpwi    r0, 0x0
    bne      .L_801EE178
    lis      r4, lbl_8025BE60@ha
    li       r3, sPathSynthC@sda21
    addi     r5, r4, lbl_8025BE60@l
    li       r4, 0x7eb
    bl       __assert
.L_801EE178:
    cmpwi    r27, 0x1
    mr       r29, r27
    li       r30, 0x0
    bge      .L_801EE18C
    li       r29, 0x1
.L_801EE18C:
    cmpwi    r25, 0x0
    bne      .L_801EE1E4
    xoris    r3, r23, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc(r1)
    mulli    r3, r24, 0x18
    lfd      f1, lbl_8065FEB0@sda21(r0)
    stw      r0, 0x8(r1)
    lfd      f2, lbl_8065FED0@sda21(r0)
    add      r3, r31, r3
    lfd      f0, 0x8(r1)
    fsub     f0, f0, f1
    fmul     f0, f2, f0
    frsp     f0, f0
    stfs     f0, 0x3c(r3)
    lfs      f1, 0x34(r3)
    lfs      f0, 0x3c(r3)
    fcmpu    cr0, f1, f0
    beq      .L_801EE2C0
    stw      r29, 0x44(r3)
    li       r30, 0x1
    b        .L_801EE2C0
.L_801EE1E4:
    mulli    r28, r24, 0x18
    lfs      f0, lbl_8065FE90@sda21(r0)
    add      r27, r31, r28
    lfsu     f1, 0x34(r27)
    fcmpo    cr0, f1, f0
    bge      .L_801EE204
    lwz      r0, sSynthDecibelFloor@sda21(r0)
    b        .L_801EE244
.L_801EE204:
    lfs      f0, lbl_8065FE94@sda21(r0)
    fcmpo    cr0, f1, f0
    cror     eq, gt, eq
    bne      .L_801EE21C
    li       r0, 0x0
    b        .L_801EE244
.L_801EE21C:
    bl       log
    lfd      f0, lbl_8065FE98@sda21(r0)
    lwz      r3, sSynthDecibelFloor@sda21(r0)
    fmul     f0, f0, f1
    fctiwz   f0, f0
    stfd     f0, 0x8(r1)
    lwz      r0, 0xc(r1)
    cmpw     r0, r3
    bge      .L_801EE244
    mr       r0, r3
.L_801EE244:
    add      r4, r31, r28
    slwi     r3, r23, 16
    stw      r0, 0x3c(r4)
    lwz      r0, sSynthDecibelFloor@sda21(r0)
    stw      r3, 0x40(r4)
    lwz      r3, 0x40(r4)
    cmpw     r3, r0
    bgt      .L_801EE26C
    lfs      f1, lbl_8065FEA0@sda21(r0)
    b        .L_801EE2A8
.L_801EE26C:
    cmpwi    r3, 0x0
    blt      .L_801EE27C
    lfs      f1, lbl_8065FE94@sda21(r0)
    b        .L_801EE2A8
.L_801EE27C:
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc(r1)
    lfd      f1, lbl_8065FEB0@sda21(r0)
    stw      r0, 0x8(r1)
    lfd      f2, lbl_8065FEA8@sda21(r0)
    lfd      f0, 0x8(r1)
    fsub     f0, f0, f1
    fmul     f1, f2, f0
    bl       exp
    frsp     f1, f1
.L_801EE2A8:
    lfs      f0, 0x0(r27)
    fcmpu    cr0, f0, f1
    beq      .L_801EE2C0
    add      r3, r31, r28
    li       r30, 0x1
    stw      r29, 0x44(r3)
.L_801EE2C0:
    mulli    r0, r24, 0x18
    cmpwi    r30, 0x0
    add      r3, r31, r0
    stw      r25, 0x38(r3)
    beq      .L_801EE2DC
    mr       r3, r31
    bl       HSD_SynthSFXUpdateVolume
.L_801EE2DC:
    mr       r3, r26
    bl       OSRestoreInterrupts
    addi     r11, r1, 0x40
    bl       _restgpr_23
    lwz      r0, 0x44(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

/**
 * @note Address: 0x801EE2FC
 * @note Size: 0xD8
 */
void HSD_SynthSFXSetPan(s32 id, u8 pan, s32 flag)
{
	HSD_SynthSFXNode* node;
	u32 enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL) {
		ASSERT_SYNTH(0x80F, flag >= 0 && flag < USERPAN_NUM);
		if (pan > 0xFE) {
			pan = 0xFE;
		}
		node->userPan[flag] = pan;
		HSD_SynthSFXUpdateMix(node, 1);
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EE3D4
 * @note Size: 0xC8
 */
void HSD_SynthSFXSetSPan(s32 id, s8 span, s32 flag)
{
	HSD_SynthSFXNode* node;
	u32 enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL) {
		ASSERT_SYNTH(0x824, flag >= 0 && flag < USERPAN_NUM);
		node->userSPan[flag] = span;
		HSD_SynthSFXUpdateMix(node, 1);
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EE49C
 * @note Size: 0xB0
 */
void HSD_SynthSFXSetMix(s32 id, f32 mixMain, f32 mixAuxA, f32 mixAuxB)
{
	HSD_SynthSFXNode* node;
	u32 enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL) {
		node->mixMain = mixMain;
		node->mixAuxA = mixAuxA;
		node->mixAuxB = mixAuxB;
		HSD_SynthSFXUpdateMix(node, 1);
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EE54C
 * @note Size: 0x70
 */
void HSD_SynthSFXLinkUpdateRearPitch(HSD_SynthSFXNode* rearEntry, u16 unused)
{
	HSD_SynthSFXNode* it;

	ASSERT_SYNTH(0x845, rearEntry->sfxID == -2);
	for (it = sSynthPendingRearPitchUpdates; it != NULL; it = it->link.rearNext) {
		if (it == rearEntry) {
			return;
		}
	}
	rearEntry->link.rearNext = sSynthPendingRearPitchUpdates;
	sSynthPendingRearPitchUpdates             = rearEntry;
}

#define AX_SYNC_FLAG_COPYRATIO (1 << 19)
#define AX_SYNC_FLAG_COPYSRC   (1 << 18)

void AXSetVoiceState(AXVPB* p, u16 state);
void AXSetVoiceSrcRatio(AXVPB* p, f32 ratio);

/**
 * @note Address: 0x801EE5BC
 * @note Size: 0x1D0
 */
void HSD_SynthSFXUpdatePitch(HSD_SynthSFXNode* node)
{
	f32 pitch;
	s32 i;

	if (node->sfxID != -2) {
		if (node->flags & 4) {
			pitch = 0.0f;
		} else {
			pitch = node->pitch[1] * (node->freqBase * node->pitch[0]);
		}
	} else {
		pitch = node->u.rear.pitch;
		if (AXVPB_PB_STATE(node->voice[0]) == 0) {
			for (i = 0; i < node->voice_count; i++) {
				AXSetVoiceState(node->voice[i], 1);
			}
		}
	}
	if (!(node->flags & 8)) {
		for (i = 0; i < node->voice_count; i++) {
			s32 copysrc = 0;
			if (node->voice[i]->sync & AX_SYNC_FLAG_COPYSRC) {
				copysrc = 1;
			}
			AXSetVoiceSrcRatio(node->voice[i], pitch);
			// a full src copy already carries the ratio, keep just that flag
			if (copysrc) {
				node->voice[i]->sync &= ~AX_SYNC_FLAG_COPYRATIO;
				node->voice[i]->sync |= AX_SYNC_FLAG_COPYSRC;
			}
		}
	}
	if (node->sfxID != -2 && node->rearIndex != -1) {
		HSD_SynthSFXNode* rearEntry = &sSynthSfxNodes[node->rearIndex];
		ASSERT_SYNTH(0x86F, rearEntry->sfxID == -2);
		rearEntry->u.rear.pitch = pitch;
		HSD_SynthSFXLinkUpdateRearPitch(rearEntry, 0);
	}
}

/**
 * @note Address: 0x801EE78C
 * @note Size: 0xF4
 */
void HSD_SynthSFXSetPitch(s32 id, s32 flag, f32 ratio)
{
	HSD_SynthSFXNode* node;
	u32 enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL && !(node->flags & 1)) {
		ASSERT_SYNTH(0x88A, flag == 0 || flag == 1);
		node->pitch[flag] = ratio;
		HSD_SynthSFXUpdatePitch(node);
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EE880
 * @note Size: 0xB8
 */
void HSD_SynthSFXSetPriority(s32 id, s32 prio)
{
	HSD_SynthSFXNode* node;
	s32 i;
	u32 enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL && !(node->flags & 1)) {
		for (i = 0; i < node->voice_count; i++) {
			AXSetVoicePriority(node->voice[i], prio);
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline s32 tickFader(HSD_SynthVolFader* f)
{
	f32 vol;

	if (f->delay != 0) {
		f->delay -= 1;
		return 1;
	}
	if (f->duration != 0) {
		if (f->mode == 0) {
			f->volume = (f->volume * ((f32)f->duration - 1.0f)) / (f32)f->duration + f->target.f / (f32)f->duration;
		} else {
			// in db mode the target slot holds the current dbx and dbxRaw the goal
			f->target.i = ((f32)f->target.i * ((f32)f->duration - 1.0f)) / (f32)f->duration + (f32)f->dbxRaw / (f32)f->duration;
			if (f->target.i <= sSynthDecibelFloor) {
				vol = 0.0f;
			} else if (f->target.i >= 0) {
				vol = 1.0f;
			} else {
				vol = exp(1.75673301162265e-07 * f->target.i);
			}
			f->volume = vol;
		}
		f->duration -= 1;
		if (f->duration == 0) {
			return 0;
		}
		return 1;
	}
	return 0;
}

#define AX_SYNC_USER_VE      0x200
#define AX_SYNC_USER_VEDELTA 0x400

extern void AXSetVoiceVe(AXVPB* vpb, AXPBVE* ve);
extern void AXSetVoiceVeDelta(AXVPB* vpb, s16 delta);

// current output volume, the header only has padding here
// a rear node reuses the user volume area for last frame's ramp

/**
 * @note Address: N/A
 * @note Size: 0x9B8
 */
void UpdateSynthVolumeFadersAndVoices(void)
{
	s32 i;
	u32 enabled;
	HSD_SynthSFXNode* rearEntry;
	s32 masterActive;
	s32 group;
	s32 chan;
	HSD_SynthSFXNode** it;
	HSD_SynthSFXNode* entry;
	s32 changed;
	s32 vol;
	s32 delta;
	s32 lastVol;
	s32 lastDelta;
	AXPBVE rearVe;
	AXPBVE ve;

	enabled      = OSDisableInterrupts();
	masterActive = 0;
	if (lbl_80659300.duration != 0 && tickFader(&lbl_80659300)) {
		masterActive = 1;
	}

	for (group = 0; group < 2; group++) {
		if (sSynthActiveChannelFaderMasks[group] != 0) {
			for (i = 0; i < 0x20; i++) {
				chan = group * 0x20 + i;
				if (sSynthActiveChannelFaderMasks[group] & (1 << i)) {
					if (!tickFader(&lbl_80659318[chan])) {
						sSynthActiveChannelFaderMasks[group] &= ~(1 << i);
					}
				}
			}
		}
	}

	it = &sSynthPendingVolumeUpdates;
	while (*it != NULL) {
		entry   = *it;
		changed = 0;
		if (entry->sfxID <= 0) {
			entry->volUpdatePending = 0;
			*it                     = entry->volUpdateNext;
			continue;
		}

		for (i = 0; i < USERVOL_NUM; i++) {
			if (entry->u.user_vol[i].fadeCount != 0) {
				if (tickFader((HSD_SynthVolFader*)&entry->u.user_vol[i])) {
					changed = 1;
				}
			}
		}
		if (lbl_80659318[entry->channel].duration != 0) {
			changed = 1;
		}
		if (masterActive && entry->channel < 0x3F) {
			changed = 1;
		}

		if (!(entry->flags & 3)) {
			vol = 32767.0f
			    * (entry->u.user_vol[1].volume
			       * (entry->u.user_vol[0].volume
			          * (lbl_80659318[entry->channel].volume
			             * (entry->channel < 0x3F ? sSynthStreamVolume * lbl_80659300.volume : sSynthStreamVolume))));
		} else {
			vol     = 0;
			changed = 0;
		}

		delta = (vol - entry->curVol) / 160;
		if (delta > 20) {
			delta = 20;
		} else if (delta < -20) {
			delta = -20;
		}
		for (i = 0; i < entry->voice_count; i++) {
			ASSERT_SYNTH(0x919, (entry->voice[i]->sync & AX_SYNC_USER_VE) == 0);
			AXSetVoiceVeDelta(entry->voice[i], delta);
		}
		entry->curVol += delta * 160;

		if (entry->rearIndex != -1) {
			rearEntry = &sSynthSfxNodes[entry->rearIndex];
			ASSERT_SYNTH(0x920, rearEntry->sfxID == -2);
			// the rearEntry voice plays last frame's ramp, phase flipped
			lastVol   = rearEntry->u.rear.lastVol;
			lastDelta = rearEntry->u.rear.lastDelta;
			if (rearEntry->voice_count == 1) {
				if (rearEntry->u.rear.side == 0) {
					rearVe.currentVolume = -lastVol;
					rearVe.currentDelta  = -lastDelta;
				} else {
					rearVe.currentVolume = lastVol;
					rearVe.currentDelta  = lastDelta;
				}
				AXSetVoiceVe(rearEntry->voice[0], &rearVe);
			} else {
				rearVe.currentVolume = -lastVol;
				rearVe.currentDelta  = -lastDelta;
				ASSERT_SYNTH(0x92F, (rearEntry->voice[0]->sync & AX_SYNC_USER_VE) == 0);
				AXSetVoiceVe(rearEntry->voice[0], &rearVe);
				rearVe.currentVolume = lastVol;
				rearVe.currentDelta  = lastDelta;
				ASSERT_SYNTH(0x933, (rearEntry->voice[1]->sync & AX_SYNC_USER_VE) == 0);
				AXSetVoiceVe(rearEntry->voice[1], &rearVe);
			}
			rearEntry->curVol += lastDelta * 160;
			if (rearEntry->u.rear.lastVol != vol || rearEntry->u.rear.lastDelta != delta) {
				changed = 1;
			}
			rearEntry->u.rear.lastVol   = vol;
			rearEntry->u.rear.lastDelta = delta;
		}

		if (delta == 0 && (!changed || 0.0f == (f32)vol)) {
			entry->curVol    = vol;
			ve.currentVolume = vol;
			ve.currentDelta  = 0;
			for (i = 0; i < entry->voice_count; i++) {
				entry->voice[i]->sync &= ~AX_SYNC_USER_VEDELTA;
				AXSetVoiceVe(entry->voice[i], &ve);
			}
			if (entry->rearIndex != -1) {
				rearEntry->u.rear.lastVol   = vol;
				rearEntry->u.rear.lastDelta = 0;
			}
			if (!changed) {
				u8 flags;

				entry->volUpdatePending = 0;
				*it                     = entry->volUpdateNext;
				flags                   = entry->flags;
				if (flags & 1) {
					if (entry->rearIndex == -1 || lastVol == 0) {
						HSD_SynthSFXFreeVoice(entry);
					}
				} else if ((flags & 6) == 2) {
					entry->flags = flags | 6;
					for (i = 0; i < entry->voice_count; i++) {
						HSD_SynthSFXUpdatePitch(entry);
					}
					if (sSynthDriverPauseCallback != NULL && entry->link.pstream == NULL) {
						sSynthDriverPauseCallback(entry->sfxID);
					}
				}
				continue;
			}
		}
		it = &entry->volUpdateNext;
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EE938
 * @note Size: 0x9B8
 */
// UpdateSynthVolumeFadersAndVoices
/*
    stwu     r1, -0x80(r1)
    mflr     r0
    stw      r0, 0x84(r1)
    addi     r11, r1, 0x80
    bl       _savegpr_17
    lis      r4, sSynthHeapMessageTable@ha
    lis      r3, hsd_SynthSFXLoadBuf@ha
    addi     r26, r4, sSynthHeapMessageTable@l
    addi     r27, r3, hsd_SynthSFXLoadBuf@l
    bl       OSDisableInterrupts
    addi     r23, r27, 0x33c0
    mr       r31, r3
    lwz      r4, 0x10(r23)
    li       r29, 0x0
    cmpwi    r4, 0x0
    beq      .L_801EEB18
    lwz      r3, 0x14(r23)
    cmpwi    r3, 0x0
    beq      .L_801EE994
    subi     r0, r3, 0x1
    li       r3, 0x1
    stw      r0, 0x14(r23)
    b        .L_801EEB0C
.L_801EE994:
    cmpwi    r4, 0x0
    beq      .L_801EEB08
    lwz      r0, 0x4(r23)
    cmpwi    r0, 0x0
    bne      .L_801EEA0C
    xoris    r3, r4, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x14(r1)
    lfd      f4, lbl_8065FEB0@sda21(r0)
    stw      r0, 0x10(r1)
    lfs      f0, lbl_8065FE94@sda21(r0)
    lfd      f1, 0x10(r1)
    stw      r3, 0x1c(r1)
    fsubs    f2, f1, f4
    lfs      f5, 0x33c0(r27)
    stw      r0, 0x18(r1)
    lfs      f1, 0x8(r23)
    fsubs    f3, f2, f0
    lfd      f0, 0x18(r1)
    stw      r3, 0x24(r1)
    fsubs    f2, f0, f4
    stw      r0, 0x20(r1)
    fmuls    f3, f5, f3
    lfd      f0, 0x20(r1)
    fdivs    f2, f3, f2
    fsubs    f0, f0, f4
    fdivs    f0, f1, f0
    fadds    f0, f2, f0
    stfs     f0, 0x33c0(r27)
    b        .L_801EEAE8
.L_801EEA0C:
    lis      r5, 0x4330
    xoris    r4, r4, 0x8000
    lwz      r0, 0x8(r23)
    stw      r4, 0x1c(r1)
    xoris    r6, r0, 0x8000
    lfd      f3, lbl_8065FEB0@sda21(r0)
    stw      r5, 0x18(r1)
    lwz      r0, 0xc(r23)
    lfd      f0, 0x18(r1)
    xoris    r3, r0, 0x8000
    stw      r6, 0x24(r1)
    fsubs    f0, f0, f3
    lfs      f4, lbl_8065FE94@sda21(r0)
    stw      r5, 0x20(r1)
    lwz      r0, sSynthDecibelFloor@sda21(r0)
    lfd      f2, 0x20(r1)
    fsubs    f1, f0, f4
    stw      r4, 0x14(r1)
    fsubs    f2, f2, f3
    stw      r5, 0x10(r1)
    lfd      f0, 0x10(r1)
    fmuls    f2, f2, f1
    stw      r3, 0x2c(r1)
    fsubs    f0, f0, f3
    stw      r5, 0x28(r1)
    lfd      f1, 0x28(r1)
    fdivs    f2, f2, f0
    stw      r4, 0x34(r1)
    stw      r5, 0x30(r1)
    lfd      f0, 0x30(r1)
    fsubs    f1, f1, f3
    fsubs    f0, f0, f3
    fdivs    f0, f1, f0
    fadds    f0, f2, f0
    fctiwz   f0, f0
    stfd     f0, 0x38(r1)
    lwz      r3, 0x3c(r1)
    cmpw     r3, r0
    stw      r3, 0x8(r23)
    bgt      .L_801EEAB4
    lfs      f4, lbl_8065FEA0@sda21(r0)
    b        .L_801EEAE4
.L_801EEAB4:
    cmpwi    r3, 0x0
    blt      .L_801EEAC0
    b        .L_801EEAE4
.L_801EEAC0:
    xoris    r0, r3, 0x8000
    stw      r5, 0x38(r1)
    lfd      f1, lbl_8065FEA8@sda21(r0)
    stw      r0, 0x3c(r1)
    lfd      f0, 0x38(r1)
    fsub     f0, f0, f3
    fmul     f1, f1, f0
    bl       exp
    frsp     f4, f1
.L_801EEAE4:
    stfs     f4, 0x33c0(r27)
.L_801EEAE8:
    lwz      r3, 0x10(r23)
    subic.   r0, r3, 0x1
    stw      r0, 0x10(r23)
    bne      .L_801EEB00
    li       r3, 0x0
    b        .L_801EEB0C
.L_801EEB00:
    li       r3, 0x1
    b        .L_801EEB0C
.L_801EEB08:
    li       r3, 0x0
.L_801EEB0C:
    cmpwi    r3, 0x0
    beq      .L_801EEB18
    li       r29, 0x1
.L_801EEB18:
    li       r18, 0x0
    li       r24, sSynthActiveChannelFaderMasks@sda21
    li       r23, 0x0
.L_801EEB24:
    lwz      r0, 0x0(r24)
    cmplwi   r0, 0x0
    beq      .L_801EED28
    mulli    r17, r23, 0x18
    mr       r25, r23
    li       r19, 0x0
.L_801EEB3C:
    li       r4, 0x1
    lwz      r0, 0x0(r24)
    slw      r20, r4, r19
    and.     r0, r20, r0
    beq      .L_801EED14
    addi     r22, r27, 0x33d8
    add      r22, r22, r17
    addi     r5, r22, 0x14
    lwz      r3, 0x14(r22)
    cmpwi    r3, 0x0
    beq      .L_801EEB74
    subi     r0, r3, 0x1
    stw      r0, 0x0(r5)
    b        .L_801EED00
.L_801EEB74:
    addi     r21, r22, 0x10
    lwz      r3, 0x10(r22)
    cmpwi    r3, 0x0
    beq      .L_801EECFC
    lwz      r0, 0x4(r22)
    cmpwi    r0, 0x0
    bne      .L_801EEBF4
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x3c(r1)
    lfd      f4, lbl_8065FEB0@sda21(r0)
    stw      r0, 0x38(r1)
    lfs      f0, lbl_8065FE94@sda21(r0)
    lfd      f1, 0x38(r1)
    stw      r3, 0x34(r1)
    fsubs    f2, f1, f4
    lfs      f5, 0x0(r22)
    stw      r0, 0x30(r1)
    lfs      f1, 0x8(r22)
    fsubs    f3, f2, f0
    lfd      f0, 0x30(r1)
    stw      r3, 0x2c(r1)
    fsubs    f2, f0, f4
    stw      r0, 0x28(r1)
    fmuls    f3, f5, f3
    lfd      f0, 0x28(r1)
    fdivs    f2, f3, f2
    fsubs    f0, f0, f4
    fdivs    f0, f1, f0
    fadds    f0, f2, f0
    stfs     f0, 0x0(r22)
    b        .L_801EECD4
.L_801EEBF4:
    lis      r5, 0x4330
    xoris    r4, r3, 0x8000
    lwz      r0, 0x8(r22)
    stw      r4, 0x34(r1)
    xoris    r6, r0, 0x8000
    lfd      f3, lbl_8065FEB0@sda21(r0)
    stw      r5, 0x30(r1)
    lwz      r0, 0xc(r22)
    lfd      f0, 0x30(r1)
    stw      r6, 0x3c(r1)
    xoris    r3, r0, 0x8000
    fsubs    f0, f0, f3
    lfs      f4, lbl_8065FE94@sda21(r0)
    stw      r5, 0x38(r1)
    lwz      r0, sSynthDecibelFloor@sda21(r0)
    lfd      f2, 0x38(r1)
    fsubs    f1, f0, f4
    stw      r4, 0x2c(r1)
    fsubs    f2, f2, f3
    stw      r5, 0x28(r1)
    lfd      f0, 0x28(r1)
    fmuls    f2, f2, f1
    stw      r3, 0x24(r1)
    fsubs    f0, f0, f3
    stw      r5, 0x20(r1)
    lfd      f1, 0x20(r1)
    fdivs    f2, f2, f0
    stw      r4, 0x1c(r1)
    stw      r5, 0x18(r1)
    lfd      f0, 0x18(r1)
    fsubs    f1, f1, f3
    fsubs    f0, f0, f3
    fdivs    f0, f1, f0
    fadds    f0, f2, f0
    fctiwz   f0, f0
    stfd     f0, 0x10(r1)
    lwz      r3, 0x14(r1)
    stw      r3, 0x8(r22)
    lwz      r3, 0x8(r22)
    cmpw     r3, r0
    bgt      .L_801EECA0
    lfs      f4, lbl_8065FEA0@sda21(r0)
    b        .L_801EECD0
.L_801EECA0:
    cmpwi    r3, 0x0
    blt      .L_801EECAC
    b        .L_801EECD0
.L_801EECAC:
    xoris    r0, r3, 0x8000
    stw      r5, 0x38(r1)
    lfd      f1, lbl_8065FEA8@sda21(r0)
    stw      r0, 0x3c(r1)
    lfd      f0, 0x38(r1)
    fsub     f0, f0, f3
    fmul     f1, f1, f0
    bl       exp
    frsp     f4, f1
.L_801EECD0:
    stfs     f4, 0x0(r22)
.L_801EECD4:
    lwz      r3, 0x0(r21)
    subi     r0, r3, 0x1
    stw      r0, 0x0(r21)
    lwz      r0, 0x0(r21)
    cmpwi    r0, 0x0
    bne      .L_801EECF4
    li       r4, 0x0
    b        .L_801EED00
.L_801EECF4:
    li       r4, 0x1
    b        .L_801EED00
.L_801EECFC:
    li       r4, 0x0
.L_801EED00:
    cmpwi    r4, 0x0
    bne      .L_801EED14
    lwz      r3, 0x0(r24)
    andc     r0, r3, r20
    stw      r0, 0x0(r24)
.L_801EED14:
    addi     r19, r19, 0x1
    addi     r17, r17, 0x18
    cmpwi    r19, 0x20
    addi     r25, r25, 0x1
    blt      .L_801EEB3C
.L_801EED28:
    addi     r18, r18, 0x1
    addi     r23, r23, 0x20
    cmpwi    r18, 0x2
    addi     r24, r24, 0x4
    blt      .L_801EEB24
    li       r18, sSynthPendingVolumeUpdates@sda21
    b        .L_801EF2C4
.L_801EED44:
    lwz      r0, 0x0(r20)
    mr       r19, r20
    li       r22, 0x0
    cmpwi    r0, 0x0
    bgt      .L_801EED6C
    li       r0, 0x0
    stb      r0, 0x2e(r20)
    lwz      r0, 0x24(r20)
    stw      r0, 0x0(r18)
    b        .L_801EF2C4
.L_801EED6C:
    li       r21, 0x0
    mr       r23, r20
.L_801EED74:
    lwz      r5, 0x44(r23)
    cmpwi    r5, 0x0
    beq      .L_801EEF2C
    lwz      r3, 0x48(r23)
    cmpwi    r3, 0x0
    beq      .L_801EED9C
    subi     r0, r3, 0x1
    li       r3, 0x1
    stw      r0, 0x48(r23)
    b        .L_801EEF20
.L_801EED9C:
    cmpwi    r5, 0x0
    beq      .L_801EEF1C
    lwz      r0, 0x38(r23)
    cmpwi    r0, 0x0
    bne      .L_801EEE14
    xoris    r3, r5, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x3c(r1)
    lfd      f4, lbl_8065FEB0@sda21(r0)
    stw      r0, 0x38(r1)
    lfs      f0, lbl_8065FE94@sda21(r0)
    lfd      f1, 0x38(r1)
    stw      r3, 0x34(r1)
    fsubs    f2, f1, f4
    lfs      f5, 0x34(r23)
    stw      r0, 0x30(r1)
    lfs      f1, 0x3c(r23)
    fsubs    f3, f2, f0
    lfd      f0, 0x30(r1)
    stw      r3, 0x2c(r1)
    fsubs    f2, f0, f4
    stw      r0, 0x28(r1)
    fmuls    f3, f5, f3
    lfd      f0, 0x28(r1)
    fdivs    f2, f3, f2
    fsubs    f0, f0, f4
    fdivs    f0, f1, f0
    fadds    f0, f2, f0
    stfs     f0, 0x34(r23)
    b        .L_801EEEF4
.L_801EEE14:
    lis      r4, 0x4330
    xoris    r3, r5, 0x8000
    lwz      r0, 0x3c(r23)
    stw      r3, 0x34(r1)
    xoris    r5, r0, 0x8000
    lfd      f3, lbl_8065FEB0@sda21(r0)
    stw      r4, 0x30(r1)
    lwz      r0, 0x40(r23)
    lfd      f0, 0x30(r1)
    xoris    r0, r0, 0x8000
    stw      r5, 0x3c(r1)
    fsubs    f0, f0, f3
    lfs      f4, lbl_8065FE94@sda21(r0)
    stw      r4, 0x38(r1)
    lfd      f2, 0x38(r1)
    fsubs    f1, f0, f4
    stw      r3, 0x2c(r1)
    fsubs    f2, f2, f3
    stw      r4, 0x28(r1)
    lfd      f0, 0x28(r1)
    fmuls    f2, f2, f1
    stw      r0, 0x24(r1)
    fsubs    f0, f0, f3
    stw      r4, 0x20(r1)
    lfd      f1, 0x20(r1)
    fdivs    f2, f2, f0
    stw      r3, 0x1c(r1)
    stw      r4, 0x18(r1)
    lfd      f0, 0x18(r1)
    fsubs    f1, f1, f3
    fsubs    f0, f0, f3
    fdivs    f0, f1, f0
    fadds    f0, f2, f0
    fctiwz   f0, f0
    stfd     f0, 0x10(r1)
    lwz      r0, 0x14(r1)
    stw      r0, 0x3c(r23)
    lwz      r3, 0x3c(r23)
    lwz      r0, sSynthDecibelFloor@sda21(r0)
    cmpw     r3, r0
    bgt      .L_801EEEC0
    lfs      f4, lbl_8065FEA0@sda21(r0)
    b        .L_801EEEF0
.L_801EEEC0:
    cmpwi    r3, 0x0
    blt      .L_801EEECC
    b        .L_801EEEF0
.L_801EEECC:
    xoris    r0, r3, 0x8000
    stw      r4, 0x38(r1)
    lfd      f1, lbl_8065FEA8@sda21(r0)
    stw      r0, 0x3c(r1)
    lfd      f0, 0x38(r1)
    fsub     f0, f0, f3
    fmul     f1, f1, f0
    bl       exp
    frsp     f4, f1
.L_801EEEF0:
    stfs     f4, 0x34(r23)
.L_801EEEF4:
    lwz      r3, 0x44(r23)
    subi     r0, r3, 0x1
    stw      r0, 0x44(r23)
    lwz      r0, 0x44(r23)
    cmpwi    r0, 0x0
    bne      .L_801EEF14
    li       r3, 0x0
    b        .L_801EEF20
.L_801EEF14:
    li       r3, 0x1
    b        .L_801EEF20
.L_801EEF1C:
    li       r3, 0x0
.L_801EEF20:
    cmpwi    r3, 0x0
    beq      .L_801EEF2C
    li       r22, 0x1
.L_801EEF2C:
    addi     r21, r21, 0x1
    addi     r23, r23, 0x18
    cmpwi    r21, 0x2
    blt      .L_801EED74
    lbz      r3, 0xb(r20)
    addi     r4, r27, 0x33d8
    mulli    r0, r3, 0x18
    add      r4, r4, r0
    lwz      r0, 0x10(r4)
    cmpwi    r0, 0x0
    beq      .L_801EEF5C
    li       r22, 0x1
.L_801EEF5C:
    cmpwi    r29, 0x0
    beq      .L_801EEF70
    cmplwi   r3, 0x3f
    bge      .L_801EEF70
    li       r22, 0x1
.L_801EEF70:
    lbz      r0, 0x9(r20)
    clrlwi.  r0, r0, 30
    bne      .L_801EEFC8
    cmplwi   r3, 0x3f
    lfs      f5, lbl_8065FEC4@sda21(r0)
    lfs      f4, 0x4c(r20)
    lfs      f3, 0x34(r20)
    lfs      f2, 0x0(r4)
    bge      .L_801EEFA4
    lfs      f1, sSynthStreamVolume@sda21(r0)
    lfs      f0, 0x33c0(r27)
    fmuls    f0, f1, f0
    b        .L_801EEFA8
.L_801EEFA4:
    lfs      f0, sSynthStreamVolume@sda21(r0)
.L_801EEFA8:
    fmuls    f0, f2, f0
    fmuls    f0, f3, f0
    fmuls    f0, f4, f0
    fmuls    f0, f5, f0
    fctiwz   f0, f0
    stfd     f0, 0x38(r1)
    lwz      r24, 0x3c(r1)
    b        .L_801EEFD0
.L_801EEFC8:
    li       r24, 0x0
    li       r22, 0x0
.L_801EEFD0:
    lhz      r0, 0x2c(r20)
    lis      r3, 0x6666
    addi     r3, r3, 0x6667
    subf     r0, r0, r24
    mulhw    r0, r3, r0
    srawi    r0, r0, 6
    srwi     r3, r0, 31
    add      r25, r0, r3
    cmpwi    r25, 0x14
    ble      .L_801EF000
    li       r25, 0x14
    b        .L_801EF00C
.L_801EF000:
    cmpwi    r25, -0x14
    bge      .L_801EF00C
    li       r25, -0x14
.L_801EF00C:
    mr       r23, r20
    li       r21, 0x0
    b        .L_801EF04C
.L_801EF018:
    lwz      r3, 0xc(r23)
    lwz      r0, 0x1c(r3)
    rlwinm.  r0, r0, 0, 22, 22
    beq      .L_801EF038
    addi     r5, r26, 0x668
    li       r3, sPathSynthC@sda21
    li       r4, 0x919
    bl       __assert
.L_801EF038:
    lwz      r3, 0xc(r23)
    extsh    r4, r25
    bl       AXSetVoiceVeDelta
    addi     r23, r23, 0x4
    addi     r21, r21, 0x1
.L_801EF04C:
    lbz      r0, 0xa(r19)
    cmpw     r21, r0
    blt      .L_801EF018
    mulli    r0, r25, 0xa0
    lhz      r3, 0x2c(r20)
    add      r0, r3, r0
    sth      r0, 0x2c(r20)
    lha      r0, 0x14(r20)
    cmpwi    r0, -0x1
    beq      .L_801EF18C
    mulli    r0, r0, 0x98
    addi     r30, r27, 0x8c0
    add      r30, r30, r0
    lwz      r0, 0x0(r30)
    cmpwi    r0, -0x2
    beq      .L_801EF09C
    addi     r5, r26, 0x638
    li       r3, sPathSynthC@sda21
    li       r4, 0x920
    bl       __assert
.L_801EF09C:
    lbz      r0, 0xa(r30)
    lhz      r28, 0x4a(r30)
    cmplwi   r0, 0x1
    lha      r21, 0x4c(r30)
    bne      .L_801EF0E8
    lha      r0, 0x34(r30)
    cmpwi    r0, 0x0
    bne      .L_801EF0D0
    neg      r3, r28
    neg      r0, r21
    sth      r3, 0xc(r1)
    sth      r0, 0xe(r1)
    b        .L_801EF0D8
.L_801EF0D0:
    sth      r28, 0xc(r1)
    sth      r21, 0xe(r1)
.L_801EF0D8:
    lwz      r3, 0xc(r30)
    addi     r4, r1, 0xc
    bl       AXSetVoiceVe
    b        .L_801EF158
.L_801EF0E8:
    neg      r3, r28
    neg      r0, r21
    sth      r3, 0xc(r1)
    lwz      r3, 0xc(r30)
    sth      r0, 0xe(r1)
    lwz      r0, 0x1c(r3)
    rlwinm.  r0, r0, 0, 22, 22
    beq      .L_801EF118
    addi     r5, r26, 0x698
    li       r3, sPathSynthC@sda21
    li       r4, 0x92f
    bl       __assert
.L_801EF118:
    lwz      r3, 0xc(r30)
    addi     r4, r1, 0xc
    bl       AXSetVoiceVe
    sth      r28, 0xc(r1)
    lwz      r3, 0x10(r30)
    sth      r21, 0xe(r1)
    lwz      r0, 0x1c(r3)
    rlwinm.  r0, r0, 0, 22, 22
    beq      .L_801EF14C
    addi     r5, r26, 0x6cc
    li       r3, sPathSynthC@sda21
    li       r4, 0x933
    bl       __assert
.L_801EF14C:
    lwz      r3, 0x10(r30)
    addi     r4, r1, 0xc
    bl       AXSetVoiceVe
.L_801EF158:
    mulli    r0, r21, 0xa0
    lhz      r3, 0x2c(r30)
    add      r0, r3, r0
    sth      r0, 0x2c(r30)
    lhz      r0, 0x4a(r30)
    cmpw     r0, r24
    bne      .L_801EF180
    lha      r0, 0x4c(r30)
    cmpw     r0, r25
    beq      .L_801EF184
.L_801EF180:
    li       r22, 0x1
.L_801EF184:
    sth      r24, 0x4a(r30)
    sth      r25, 0x4c(r30)
.L_801EF18C:
    cmpwi    r25, 0x0
    bne      .L_801EF2C0
    cmpwi    r22, 0x0
    beq      .L_801EF1C4
    xoris    r3, r24, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x3c(r1)
    lfd      f1, lbl_8065FEB0@sda21(r0)
    stw      r0, 0x38(r1)
    lfs      f2, lbl_8065FEA0@sda21(r0)
    lfd      f0, 0x38(r1)
    fsubs    f0, f0, f1
    fcmpu    cr0, f2, f0
    bne      .L_801EF2C0
.L_801EF1C4:
    sth      r24, 0x2c(r20)
    li       r21, 0x0
    mr       r17, r20
    sth      r24, 0x8(r1)
    sth      r21, 0xa(r1)
    b        .L_801EF200
.L_801EF1DC:
    lwz      r3, 0xc(r17)
    addi     r4, r1, 0x8
    lwz      r0, 0x1c(r3)
    rlwinm   r0, r0, 0, 22, 20
    stw      r0, 0x1c(r3)
    lwz      r3, 0xc(r17)
    bl       AXSetVoiceVe
    addi     r17, r17, 0x4
    addi     r21, r21, 0x1
.L_801EF200:
    lbz      r0, 0xa(r19)
    cmpw     r21, r0
    blt      .L_801EF1DC
    lha      r0, 0x14(r20)
    cmpwi    r0, -0x1
    beq      .L_801EF224
    sth      r24, 0x4a(r30)
    li       r0, 0x0
    sth      r0, 0x4c(r30)
.L_801EF224:
    cmpwi    r22, 0x0
    bne      .L_801EF2C0
    li       r17, 0x0
    stb      r17, 0x2e(r20)
    lwz      r0, 0x24(r20)
    stw      r0, 0x0(r18)
    lbz      r3, 0x9(r20)
    clrlwi.  r0, r3, 31
    beq      .L_801EF268
    lha      r0, 0x14(r20)
    cmpwi    r0, -0x1
    beq      .L_801EF25C
    cmpwi    r28, 0x0
    bne      .L_801EF2C4
.L_801EF25C:
    mr       r3, r20
    bl       HSD_SynthSFXFreeVoice
    b        .L_801EF2C4
.L_801EF268:
    rlwinm   r0, r3, 0, 29, 30
    cmpwi    r0, 0x2
    bne      .L_801EF2C4
    ori      r0, r3, 0x6
    stb      r0, 0x9(r20)
    b        .L_801EF28C
.L_801EF280:
    mr       r3, r19
    bl       HSD_SynthSFXUpdatePitch
    addi     r17, r17, 0x1
.L_801EF28C:
    lbz      r0, 0xa(r19)
    cmpw     r17, r0
    blt      .L_801EF280
    lwz      r12, sSynthDriverPauseCallback@sda21(r0)
    cmplwi   r12, 0x0
    beq      .L_801EF2C4
    lwz      r0, 0x30(r20)
    cmplwi   r0, 0x0
    bne      .L_801EF2C4
    lwz      r3, 0x0(r20)
    mtctr    r12
    bctrl
    b        .L_801EF2C4
.L_801EF2C0:
    addi     r18, r20, 0x24
.L_801EF2C4:
    lwz      r20, 0x0(r18)
    cmplwi   r20, 0x0
    bne      .L_801EED44
    mr       r3, r31
    bl       OSRestoreInterrupts
    addi     r11, r1, 0x80
    bl       _restgpr_17
    lwz      r0, 0x84(r1)
    mtlr     r0
    addi     r1, r1, 0x80
    blr
*/

/**
 * @note Address: 0x801EF2F0
 * @note Size: 0x5C
 */
void HSD_SynthSFXUpdateVolume(HSD_SynthSFXNode* node)
{
	u32 enabled = OSDisableInterrupts();
	if (!node->volUpdatePending && !(node->flags & 8)) {
		node->volUpdatePending = 1;
		node->volUpdateNext    = sSynthPendingVolumeUpdates;
		sSynthPendingVolumeUpdates           = node;
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void updateMixEnvelopeStep(u16* target, u16* current, s16* out)
{
	s16 delta = (s16)((*target - *current) / 160);
	if (delta > 0x14) {
		delta = 0x14;
	} else if (delta < -0x14) {
		delta = -0x14;
	}
	out[1] = delta;
	if (delta != 0) {
		out[0]   = *current;
		*current = (u16)(*current + delta * 0xA0);
	} else {
		out[0]   = *target;
		*current = *target;
	}
}

// the header's node layout is stale past 0x34, this is what the mix code
// actually touches: a packed mix with no deltas, current at 0x74 and target
// at 0x86, and rear entries reuse 0x34 for their side plus a parked mix

#define VOICE_NODE(v) (&sSynthSfxNodes[(v)->index])

#define AXVPB_SYNC(v)    (*(u32*)((u8*)(v) + 0x1C))
#define AXVPB_ITDFLAG(v) (*(u16*)((u8*)(v) + 0x16E))
#define AXVPB_SHIFTL(v)  (*(u16*)((u8*)(v) + 0x174))
#define AXVPB_SHIFTR(v)  (*(u16*)((u8*)(v) + 0x176))

extern void AXSetVoiceMix(AXVPB* voice, AXPBMIX* mix);

/**
 * @note Address: 0x801EF34C
 * @note Size: 0x250
 */
void AdvancePendingSynthMixUpdates(void)
{
	HSD_SynthSFXNode* node;
	s32 j;
	s32 i;
	s32 busy;
	s32 changed;
	u32 enabled;
	HSD_SynthSFXNode** it;
	HSD_SynthSFXNode* base;
	AXVPB* voice;
	AXPBMIX mix;
	s16 delta;
	u16* cur;
	u16* tgt;
	u16* out;
	s32 k;
	s32 moving;

	enabled = OSDisableInterrupts();
	it      = &sSynthPendingMixUpdates;
	while ((node = *it) != NULL) {
		busy = 0;
		for (i = 0; i < (node->rearIndex != -1) + 1; i++) {
			if (i == 0) {
				base = node;
			} else {
				base = &sSynthSfxNodes[node->rearIndex];
			}

			for (j = 0; j < node->voice_count; j++) {
				voice = base->voice[j];
				if (j == 1) {
					base = VOICE_NODE(voice);
				}
				cur     = &base->mix.vL;
				tgt     = &base->mixDst.vL;
				out     = &mix.vL;
				changed = 0;
				for (k = 0; k < 9; k++) {
					delta = (*tgt - *cur) / 160;
					if (delta > 20) {
						delta = 20;
					} else if (delta < -20) {
						delta = -20;
					}
					out[1] = delta;
					if (delta != 0) {
						out[0] = *cur;
						*cur   = *cur + delta * 160;
						moving = 1;
					} else {
						out[0] = *tgt;
						*cur   = *tgt;
						moving = 0;
					}
					changed |= moving;
					cur++;
					tgt++;
					out += 2;
				}
				busy |= changed;
				AXSetVoiceMix(voice, &mix);
				if (i == 1 && node->voice_count == 1 && base->u.rear.side != base->u.rear.newSide && changed == 0) {
					base->mixDst      = base->u.rear.mix;
					base->u.rear.side = base->u.rear.newSide;
					HSD_SynthSFXUpdateVolume(*it);
					busy = 1;
					j--;
				}
			}
		}
		if (!busy) {
			*it                    = node->mixUpdateNext;
			node->mixUpdatePending = 0;
		} else {
			it = &node->mixUpdateNext;
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: N/A
 * @note Size: 0x194
 */
void ApplySynthMixTargetImmediately(HSD_SynthSFXNode* node)
{
	HSD_SynthSFXNode** it = &sSynthPendingMixUpdates;

	while (*it != NULL) {
		if (node == *it) {
			HSD_SynthSFXNode* base;
			s32 j;
			s32 i;

			for (i = 0; i < (node->rearIndex != -1) + 1; i++) {
				if (i == 0) {
					base = node;
				} else {
					base = &sSynthSfxNodes[node->rearIndex];
				}

				for (j = 0; j < node->voice_count; j++) {
					AXVPB* voice = base->voice[j];
					AXPBMIX mix;
					u16* src;
					u16* out;
					int k;

					if (j == 1) {
						base = VOICE_NODE(voice);
					}
					// copy the targets straight in, zero deltas, so the
					// mix jumps there in one frame
					src = &base->mixDst.vL;
					out = &mix.vL;
					for (k = 0; k < 9; k++) {
						*out++ = *src++;
						*out++ = 0;
					}
					base->mix = base->mixDst;
					AXSetVoiceMix(voice, &mix);
				}
			}
			*it                    = node->mixUpdateNext;
			node->mixUpdatePending = 0;
			return;
		}
		it = &(*it)->mixUpdateNext;
	}
}

/**
 * @note Address: 0x801EF730
 * @note Size: 0xCC
 */
void HSD_SynthSFXInitMix(HSD_SynthSFXNode* node)
{
	s32 i;

	for (i = 0; i < (node->rearIndex != -1) + 1; i++) {
		HSD_SynthSFXNode* base;

		if (i == 0) {
			base = node;
		} else {
			base = &sSynthSfxNodes[node->rearIndex];
		}

		base->mix.vL        = 0;
		base->mix.vR        = 0;
		base->mix.vAuxAL    = 0;
		base->mix.vAuxAR    = 0;
		base->mix.vAuxBL    = 0;
		base->mix.vAuxBR    = 0;
		base->mix.vAuxBS    = 0;
		base->mix.vS        = 0;
		base->mix.vAuxAS    = 0;
		base->mixDst.vL     = 0;
		base->mixDst.vR     = 0;
		base->mixDst.vAuxAL = 0;
		base->mixDst.vAuxAR = 0;
		base->mixDst.vAuxBL = 0;
		base->mixDst.vAuxBR = 0;
		base->mixDst.vAuxBS = 0;
		base->mixDst.vS     = 0;
		base->mixDst.vAuxAS = 0;
		if (i != 0) {
			base->u.rear.side       = 0;
			base->u.rear.mix.vL     = 0;
			base->u.rear.mix.vR     = 0;
			base->u.rear.mix.vAuxAL = 0;
			base->u.rear.mix.vAuxAR = 0;
			base->u.rear.mix.vAuxBL = 0;
			base->u.rear.mix.vAuxBR = 0;
			base->u.rear.mix.vAuxBS = 0;
			base->u.rear.mix.vS     = 0;
			base->u.rear.mix.vAuxAS = 0;
		}
	}
}

void AXSetVoiceItdTarget(AXVPB* v, u16 shiftL, u16 shiftR);

// itd shift by pan distance from center
static u8 lbl_8025BF68[0x80] = {
	0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08, 0x08, 0x09, 0x09,
	0x0A, 0x0A, 0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F, 0x10, 0x10, 0x11, 0x11, 0x12, 0x12, 0x13, 0x13, 0x14, 0x14,
	0x15, 0x15, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x17, 0x18, 0x18, 0x18, 0x18, 0x19, 0x19, 0x19, 0x19, 0x19, 0x1A, 0x1A, 0x1A, 0x1A,
	0x1A, 0x1A, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D,
	0x1D, 0x1D, 0x1D, 0x1D, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
};

static inline void clearFrontMix(HSD_SynthMix* m)
{
	m->vL     = 0;
	m->vR     = 0;
	m->vAuxAL = 0;
	m->vAuxAR = 0;
	m->vAuxBL = 0;
	m->vAuxBR = 0;
}

/**
 * @note Address: 0x801EF7FC
 * @note Size: 0xFC4
 */
void HSD_SynthSFXUpdateMix(HSD_SynthSFXNode* node, s32 interpolate)
{
	s32 pan;
	s32 span;
	s32 i;
	s32 shiftL;
	s32 shiftR;
	HSD_SynthSFXNode* rearEntry;
	HSD_SynthMix* m;
	f32 panL;
	f32 panR;
	f32 front;
	f32 sur;
	f32 rearL;
	f32 rearR;

	if (node->voice_count == 1) {
		pan = node->userPan[0] + node->userPan[1] - 0x7F;
		if (pan < 0) {
			pan = 0;
		}
		if (pan > 0xFE) {
			pan = 0xFE;
		}
		if (sSynthSoundMode == 0) {
			pan = 0x7F;
		}
		panL = sSynthSqrtLookup[0xFE - pan];
		panR = sSynthSqrtLookup[pan];
		if (sSynthSoundMode == 2 || sSynthSoundMode == 3 || sSynthSoundMode == 4) {
			span = node->userSPan[0] + node->userSPan[1];
			if (span > 0xFF) {
				span = 0xFF;
			}
			front = lbl_8025BA10[span];
			sur   = lbl_8025BA10[0xFF - span];
			if (span > 0) {
				s32 idx = 0.527559042f * pan + 60.0f;
				rearL   = sSynthSqrtLookup[0xFE - idx];
				rearR   = sSynthSqrtLookup[idx];
			} else {
				rearL = rearR = 0.0f;
			}
		} else {
			front = 1.0f;
			sur   = 0.0f;
		}

		if (node->rearIndex != -1) {
			rearEntry = &sSynthSfxNodes[node->rearIndex];
			ASSERT_SYNTH(0xA60, rearEntry->sfxID == -2);
			if (sSynthSoundMode == 4) {
				m = &rearEntry->mixDst;
				if (rearL >= rearR) {
					rearEntry->u.rear.newSide = 0;
					if (rearEntry->u.rear.side == 1) {
						clearFrontMix(m);
						m = &rearEntry->u.rear.mix;
					}
					m->vL     = 32767.0f * (node->mixMain * (sur * (rearL - rearR)));
					m->vR     = 0;
					m->vAuxAL = 32767.0f * (node->mixAuxA * (sur * (rearL - rearR)));
					m->vAuxAR = 0;
					m->vAuxBL = 32767.0f * (node->mixAuxB * (sur * (rearL - rearR)));
					m->vAuxBR = 0;
					sur *= rearR;
				} else {
					rearEntry->u.rear.newSide = 1;
					if (rearEntry->u.rear.side == 0) {
						clearFrontMix(m);
						m = &rearEntry->u.rear.mix;
					}
					m->vL     = 0;
					m->vR     = 32767.0f * (node->mixMain * (sur * (rearR - rearL)));
					m->vAuxAL = 0;
					m->vAuxAR = 32767.0f * (node->mixAuxA * (sur * (rearR - rearL)));
					m->vAuxBL = 0;
					m->vAuxBR = 32767.0f * (node->mixAuxB * (sur * (rearR - rearL)));
					sur *= rearL;
				}
			} else {
				clearFrontMix(&rearEntry->mixDst);
				if (sSynthSoundMode == 2) {
					sur *= 0.70710677f;
				}
			}
		} else if (sSynthSoundMode == 2) {
			sur *= 0.70710677f;
		}

		m = &node->mixDst;
		if (sSynthSoundMode != 3) {
			f32 l     = panL * front;
			f32 r     = panR * front;
			m->vL     = 32767.0f * (node->mixMain * l);
			m->vR     = 32767.0f * (node->mixMain * r);
			m->vS     = 32767.0f * (sur * node->mixMain);
			m->vAuxAL = 32767.0f * (node->mixAuxA * l);
			m->vAuxAR = 32767.0f * (node->mixAuxA * r);
			m->vAuxAS = 32767.0f * (sur * node->mixAuxA);
			m->vAuxBL = 32767.0f * (node->mixAuxB * l);
			m->vAuxBR = 32767.0f * (node->mixAuxB * r);
			m->vAuxBS = 32767.0f * (sur * node->mixAuxB);
		} else {
			f32 l     = panL * front;
			f32 r     = panR * front;
			f32 sl    = rearL * sur;
			f32 sr    = rearR * sur;
			m->vL     = 32767.0f * (node->mixMain * l);
			m->vR     = 32767.0f * (node->mixMain * r);
			m->vAuxBL = 32767.0f * (node->mixMain * sl);
			m->vAuxBR = 32767.0f * (node->mixMain * sr);
			m->vAuxAL = 32767.0f * (node->mixAuxA * l);
			m->vAuxAR = 32767.0f * (node->mixAuxA * r);
			m->vAuxAS = 32767.0f * (node->mixAuxA * sl);
			m->vAuxBS = 32767.0f * (node->mixAuxA * sr);
			m->vS     = 0;
		}
	} else {
		HSD_SynthMix* m2;

		pan = node->userPan[0] + node->userPan[1] - 0x7F;
		if (pan < 0) {
			pan = 0;
		}
		if (pan > 0xFE) {
			pan = 0xFE;
		}
		if (sSynthSoundMode == 0) {
			pan = 0x7F;
		}
		if (pan <= 0x7F) {
			panL = 1.0f;
		} else {
			panL = sSynthSqrtLookup[0xFE - (pan - 0x7F) * 2];
		}
		if (pan >= 0x7F) {
			panR = 1.0f;
		} else {
			panR = sSynthSqrtLookup[pan * 2];
		}
		if (sSynthSoundMode == 2 || sSynthSoundMode == 3 || sSynthSoundMode == 4) {
			span = node->userSPan[0] + node->userSPan[1];
			if (span > 0xFF) {
				span = 0xFF;
			}
			front = lbl_8025BA10[span];
			sur   = lbl_8025BA10[0xFF - span];
		} else {
			front = 1.0f;
			sur   = 0.0f;
		}

		if (node->rearIndex != -1) {
			rearEntry = &sSynthSfxNodes[node->rearIndex];
			ASSERT_SYNTH(0xB13, rearEntry->sfxID == -2);
			if (sSynthSoundMode == 4) {
				m         = &rearEntry->mixDst;
				m->vL     = 32767.0 * (node->mixMain * (0.38 * panL * sur));
				m->vR     = 0;
				m->vAuxAL = 32767.0 * (node->mixAuxA * (0.38 * panL * sur));
				m->vAuxAR = 0;
				m->vAuxBL = 32767.0 * (node->mixAuxB * (0.38 * panL * sur));
				m->vAuxBR = 0;
				m         = &(VOICE_NODE(rearEntry->voice[1]))->mixDst;
				m->vL     = 0;
				m->vR     = 32767.0 * (node->mixMain * (0.38 * panR * sur));
				m->vAuxAL = 0;
				m->vAuxAR = 32767.0 * (node->mixAuxA * (0.38 * panR * sur));
				m->vAuxBL = 0;
				m->vAuxBR = 32767.0 * (node->mixAuxB * (0.38 * panR * sur));
				sur *= 0.49;
			} else {
				clearFrontMix(&rearEntry->mixDst);
				clearFrontMix(&(VOICE_NODE(rearEntry->voice[1]))->mixDst);
				if (sSynthSoundMode == 2) {
					sur *= 0.5f;
				}
			}
		} else if (sSynthSoundMode == 2) {
			sur *= 0.5f;
		}

		m  = &node->mixDst;
		m2 = &(VOICE_NODE(node->voice[1]))->mixDst;
		if (sSynthSoundMode == 0) {
			f32 l      = panL / 2;
			f32 r      = panR / 2;
			m->vL      = 32767.0f * (l * node->mixMain);
			m->vR      = 32767.0f * (l * node->mixMain);
			m->vAuxAL  = 32767.0f * (l * node->mixAuxA);
			m->vAuxAR  = 32767.0f * (l * node->mixAuxA);
			m->vAuxBL  = 32767.0f * (l * node->mixAuxB);
			m->vAuxBR  = 32767.0f * (l * node->mixAuxB);
			m->vS      = 0;
			m->vAuxAS  = 0;
			m->vAuxBS  = 0;
			m2->vL     = 32767.0f * (r * node->mixMain);
			m2->vR     = 32767.0f * (r * node->mixMain);
			m2->vAuxAL = 32767.0f * (r * node->mixAuxA);
			m2->vAuxAR = 32767.0f * (r * node->mixAuxA);
			m2->vAuxBL = 32767.0f * (r * node->mixAuxB);
			m2->vAuxBR = 32767.0f * (r * node->mixAuxB);
			m2->vS     = 0;
			m2->vAuxAS = 0;
			m2->vAuxBS = 0;
		} else if (sSynthSoundMode != 3) {
			f32 l      = panL * front;
			f32 sl     = panL * sur;
			f32 r      = panR * front;
			f32 sr     = panR * sur;
			m->vL      = 32767.0f * (node->mixMain * l);
			m->vAuxAL  = 32767.0f * (node->mixAuxA * l);
			m->vAuxBL  = 32767.0f * (node->mixAuxB * l);
			m->vR      = 0;
			m->vAuxAR  = 0;
			m->vAuxBR  = 0;
			m->vS      = 32767.0f * (node->mixMain * sl);
			m->vAuxAS  = 32767.0f * (node->mixAuxA * sl);
			m->vAuxBS  = 32767.0f * (node->mixAuxB * sl);
			m2->vL     = 0;
			m2->vAuxAL = 0;
			m2->vAuxBL = 0;
			m2->vR     = 32767.0f * (node->mixMain * r);
			m2->vAuxAR = 32767.0f * (node->mixAuxA * r);
			m2->vAuxBR = 32767.0f * (node->mixAuxB * r);
			m2->vS     = 32767.0f * (node->mixMain * sr);
			m2->vAuxAS = 32767.0f * (node->mixAuxA * sr);
			m2->vAuxBS = 32767.0f * (node->mixAuxB * sr);
		} else {
			m->vL      = 32767.0f * (node->mixMain * (panL * front));
			m->vR      = 0;
			m->vAuxBL  = 32767.0 * (node->mixMain * (0.87 * panL * sur));
			m->vAuxBR  = 32767.0 * (node->mixMain * (0.49 * panL * sur));
			m->vAuxAL  = 32767.0f * (node->mixAuxA * (panL * front));
			m->vAuxAR  = 0;
			m->vAuxAS  = 32767.0 * (node->mixAuxA * (0.87 * panL * sur));
			m->vAuxBS  = 32767.0 * (node->mixAuxA * (0.49 * panL * sur));
			m->vS      = 0;
			m2->vL     = 0;
			m2->vR     = 32767.0f * (node->mixMain * (panR * front));
			m2->vAuxBL = 32767.0 * (node->mixMain * (0.49 * panR * sur));
			m2->vAuxBR = 32767.0 * (node->mixMain * (0.87 * panR * sur));
			m2->vAuxAL = 0;
			m2->vAuxAR = 32767.0f * (node->mixAuxA * (panR * front));
			m2->vAuxAS = 32767.0 * (node->mixAuxA * (0.49 * panR * sur));
			m2->vAuxBS = 32767.0 * (node->mixAuxA * (0.87 * panR * sur));
			m2->vS     = 0;
		}
	}

	if (interpolate == 0) {
		node->mix = node->mixDst;
		if (node->voice_count == 2) {
			HSD_SynthSFXNode* n = VOICE_NODE(node->voice[1]);
			n->mix              = n->mixDst;
		}
		if (node->rearIndex != -1) {
			rearEntry = &sSynthSfxNodes[node->rearIndex];
			ASSERT_SYNTH(0xBF5, rearEntry->sfxID == -2);
			if (rearEntry->u.rear.side != rearEntry->u.rear.newSide) {
				rearEntry->u.rear.side = rearEntry->u.rear.newSide;
				rearEntry->mixDst      = rearEntry->u.rear.mix;
			}
			rearEntry->mix = rearEntry->mixDst;
			if (rearEntry->voice_count == 2) {
				rearEntry = VOICE_NODE(rearEntry->voice[1]);
				ASSERT_SYNTH(0xBFD, rearEntry->sfxID == -3);
				rearEntry->mix = rearEntry->mixDst;
			}
		}
	}

	{
		u32 enabled = OSDisableInterrupts();
		if (!node->mixUpdatePending && !(node->flags & 8)) {
			node->mixUpdatePending = 1;
			node->mixUpdateNext    = sSynthPendingMixUpdates;
			sSynthPendingMixUpdates           = node;
		}
		OSRestoreInterrupts(enabled);
	}

	shiftR = 0;
	shiftL = 0;
	if (sSynthSoundMode == 1 && node->voice_count == 1 && node->unk16 != 0) {
		pan = node->userPan[0] + node->userPan[1] - 0x7F;
		if (pan < 0) {
			pan = 0;
		}
		if (pan > 0xFE) {
			pan = 0xFE;
		}
		if (pan < 0x7F) {
			shiftL = lbl_8025BF68[0x7E - pan];
		} else {
			shiftR = lbl_8025BF68[pan - 0x80];
		}
	}
	for (i = 0; i < node->voice_count; i++) {
		if (AXVPB_ITDFLAG(node->voice[i]) == 1) {
			if ((AXVPB_SYNC(node->voice[i]) & 0x20) || interpolate == 0) {
				AXSetVoiceItdTarget(node->voice[i], shiftL, shiftR);
				AXVPB_SHIFTR(node->voice[i]) = shiftR;
				AXVPB_SHIFTL(node->voice[i]) = shiftL;
				AXVPB_SYNC(node->voice[i]) &= ~0x40;
				AXVPB_SYNC(node->voice[i]) |= 0x20;
			} else {
				AXSetVoiceItdTarget(node->voice[i], shiftL, shiftR);
			}
		}
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x27C
 */
void HSD_SynthSFXUpdateAllVolume(s32 vol, u16 fadeFrames, s32 channel, s32 dbMode)
{
	HSD_SynthVolFader* f;
	s32 frames;
	s32 changed = 0;
	u32 enabled;

	if (channel < 0 || channel >= 0x40) {
		return;
	}
	f       = &lbl_80659318[channel];
	enabled = OSDisableInterrupts();
	frames  = fadeFrames < 1 ? 1 : fadeFrames;
	if (dbMode == 0) {
		f32 target = (f32)(0.00392156862745098 * (f64)vol);
		if (f->volume != target) {
			f->duration = frames;
			changed     = 1;
		}
		f->target.f = target;
	} else {
		f->target.i = HSD_SynthVolToDBx(f->volume);
		f->dbxRaw   = vol << 0x10;
		if (f->volume != HSD_SynthDBxToVol(f->dbxRaw)) {
			changed     = 1;
			f->duration = frames;
		}
	}
	f->mode = dbMode;
	if (changed) {
		s32 i;
		u32 enabled2;
		sSynthActiveChannelFaderMasks[channel / 32] |= 1 << (channel & 0x1F);
		enabled2 = OSDisableInterrupts();
		for (i = 0; i < HSD_SYNTHSFXNODE_MAX; i++) {
			HSD_SynthSFXNode* node = &sSynthSfxNodes[i];
			if (node->sfxID > 0 && node->channel == channel) {
				HSD_SynthSFXUpdateVolume(node);
			}
		}
		OSRestoreInterrupts(enabled2);
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: N/A
 * @note Size: 0x210
 */
void HSD_SynthSFXSetMasterVolume(s32 vol, u16 fadeFrames, s32 dbMode)
{
	s32 frames;
	s32 changed = 0;
	u32 enabled = OSDisableInterrupts();

	frames = fadeFrames == 0 ? 1 : fadeFrames;
	if (dbMode == 0) {
		f32 target            = (f32)(0.00392156862745098 * (f64)vol);
		lbl_80659300.target.f = target;
		if (lbl_80659300.volume != target) {
			lbl_80659300.duration = frames;
			changed               = 1;
		}
	} else {
		lbl_80659300.target.i = HSD_SynthVolToDBx(lbl_80659300.volume);
		lbl_80659300.dbxRaw   = vol << 0x10;
		if (lbl_80659300.volume != HSD_SynthDBxToVol(lbl_80659300.dbxRaw)) {
			lbl_80659300.duration = frames;
			changed               = 1;
		}
	}
	lbl_80659300.mode = dbMode;
	if (changed) {
		s32 i;
		u32 enabled2 = OSDisableInterrupts();
		for (i = 0; i < HSD_SYNTHSFXNODE_MAX; i++) {
			HSD_SynthSFXNode* node = &sSynthSfxNodes[i];
			if (node->sfxID > 0) {
				HSD_SynthSFXUpdateVolume(node);
			}
		}
		OSRestoreInterrupts(enabled2);
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801F07C0
 * @note Size: 0x8
 */
void HSD_SynthSFXSetDriverInactivatedCallback(void (*callback)(s32))
{
	sSynthDriverInactivatedCallback = callback;
}

/**
 * @note Address: 0x801F07C8
 * @note Size: 0x8
 */
void HSD_SynthSFXSetDriverMasterClockCallback(void (*callback)(u32))
{
	sSynthDriverMasterClockCallback = callback;
}

/**
 * @note Address: 0x801F07D0
 * @note Size: 0x8
 */
void HSD_SynthSFXSetDriverPauseCallback(void (*callback)(s32))
{
	sSynthDriverPauseCallback = callback;
}

// driver hooks run before and after each frame, only this function reads them

void HSD_SynthPStreamMasterClockCallback(void);

/**
 * @note Address: 0x801F07D8
 * @note Size: 0x1D0
 */
void HSD_SynthCallback(void)
{
	s32 i;
	u32 enabled;
	s32 base;

	base = sSynthDriverFrame % 8 * 8;

	enabled = OSDisableInterrupts();
	if (sSynthPreFrameCallback != NULL) {
		sSynthPreFrameCallback();
	}

	while (sSynthPendingVoiceFreeCount-- > 0) {
		if (sSynthVoicesPendingFree[sSynthPendingVoiceFreeCount] != NULL) {
			AXFreeVoice(sSynthVoicesPendingFree[sSynthPendingVoiceFreeCount]);
		}
	}
	sSynthPendingVoiceFreeCount = 0;

	for (i = 0; i < 8; i++) {
		HSD_SynthSFXNode* node = &sSynthSfxNodes[base + i];

		if (node->sfxID > 0 && !(node->flags & 8) && AXVPB_PB_STATE(node->voice[0]) == 0
		    && (node->voice_count == 1 || AXVPB_PB_STATE(node->voice[1]) == 0)) {
			if (node->rearIndex != -1) {
				HSD_SynthSFXNode* rear = &sSynthSfxNodes[node->rearIndex];

				if (AXVPB_PB_STATE(rear->voice[0]) != 0) {
					continue;
				}
				if (node->voice_count != 1 && AXVPB_PB_STATE(rear->voice[1]) != 0) {
					continue;
				}
			}
			HSD_SynthSFXFreeVoice(node);
		}
	}

	while (sSynthPendingRearPitchUpdates != NULL) {
		HSD_SynthSFXUpdatePitch(sSynthPendingRearPitchUpdates);
		sSynthPendingRearPitchUpdates = sSynthPendingRearPitchUpdates->link.rearNext;
	}

	AdvancePendingSynthMixUpdates();
	UpdateSynthVolumeFadersAndVoices();
	if (sSynthDriverMasterClockCallback != NULL) {
		sSynthDriverMasterClockCallback(sSynthDriverFrame);
	}
	HSD_SynthPStreamMasterClockCallback();
	sSynthDriverFrame++;
	if (sSynthPostFrameCallback != NULL) {
		sSynthPostFrameCallback();
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801F09A8
 * @note Size: 0x10C
 */
void QueueLoadedSynthStreamBuffer(s32 result, s32 pstreamID)
{
	HSD_SynthSFXNode* node = getNode(pstreamID);
	s32 buf;

	if (node != NULL && node->link.pstream != NULL) {
		buf                                              = PSTREAM(node)->_1C;
		*getBufTableTail(&PSTREAM(node)->bufTableHeadIx) = buf;
		lbl_80656364[buf]                                = -2;
		PSTREAM(node)->_18++;
		SetDvdStreamingSlotKey(PSTREAM(node)->schedulerSlot, PSTREAM(node)->_18);
		if (sSynthStreamHakoBuffers[PSTREAM(node)->_1C].next == -1) {
			ReleaseDvdStreamingSlot(PSTREAM(node)->schedulerSlot);
			PSTREAM(node)->schedulerSlot = -1;
		}
		PSTREAM(node)->_1C = -1;
	}
}

static inline HSD_SynthBeatMarkerBlock* allocBmb(void)
{
	HSD_SynthBeatMarkerBlock* bmb = sSynthFreeBeatMarkerBlocks;
	HSD_SynthBeatMarkerBlock* p;
	s32 i;

	if (bmb != NULL) {
		sSynthFreeBeatMarkerBlocks = bmb->next;
	} else {
		// one allocation makes eight blocks, seven go on the free list
		bmb = HSD_AudioMalloc(0x100);
		for (i = 0; i < 7; i++) {
			p                          = &bmb[i];
			p->next                    = sSynthFreeBeatMarkerBlocks;
			sSynthFreeBeatMarkerBlocks = p;
		}
		bmb += 7;
	}
	return bmb;
}

static inline HSD_SynthBeatMarker* getBeatMarker(s32 buf, s32 ix)
{
	HSD_SynthBeatMarkerBlock* bmb = sSynthStreamHakoBuffers[buf].markers;
	HSD_SynthBeatMarkerBlock* bmbnext;
	s32 i;

	if (bmb == NULL) {
		bmb = allocBmb();
		ASSERT_SYNTH(0x4BC, bmb);
		sSynthStreamHakoBuffers[buf].markers = bmb;
		bmb->next                            = NULL;
	}
	while (ix >= 3) {
		bmbnext = bmb->next;
		if (bmbnext == NULL) {
			bmbnext = allocBmb();
			ASSERT_SYNTH(0x4C4, bmbnext);
			bmb->next     = bmbnext;
			bmbnext->next = NULL;
			for (i = 0; i < 3; i++) {
				// stored twice, the first one is dead
				bmbnext->marker[i].pos = -1;
				bmbnext->marker[i].pos = 0;
			}
		}
		bmb = bmbnext;
		ix -= 3;
	}
	return &bmb->marker[ix];
}

static inline void linkBeatMarkers(HSD_SynthPStream* pst, HSD_SynthBeatMarker* markers, s32 count)
{
	s32 i;
	u32 enabled;

	enabled = OSDisableInterrupts();
	for (i = 0; i < count; i++) {
		*getBeatMarker(pst->_1C, i) = markers[i];
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: N/A
 * @note Size: 0x2EC
 */
void HSD_SynthPStreamHakoMarkerCallback(s32 unused, s32 id, HSD_SynthBeatMarker* markers)
{
	HSD_SynthSFXNode* node = getNode(id);
	s32 buf;
	s32 n;

	if (node != NULL && node->link.pstream != NULL) {
		buf = PSTREAM(node)->_1C;
		n   = sSynthStreamHakoBuffers[buf].markerNum;
		linkBeatMarkers(PSTREAM(node), markers, n);
		sSynthStreamHakoBuffers[buf].markerIx = 0;
	}
}

/**
 * @note Address: 0x801F0DA0
 * @note Size: 0x16C
 */
void HSD_SynthPStreamHakoHeaderCallback(s32 result, s32 pstreamID)
{
	HSD_SynthSFXNode* node = getNode(pstreamID);
	HSD_SynthPStream* pst;
	s32 buf;
	s32 offset;
	s32 nMarkers;

	if (node != NULL && node->link.pstream != NULL) {
		pst    = PSTREAM(node);
		offset = sSynthStreamHakoBuffers[lastBuf(pst)].next;
		buf    = pst->_1C;
		if (pst->_28 != 0 && sSynthStreamHakoBuffers[buf].next <= offset) {
			sSynthStreamHakoBuffers[buf].next = -1;
		}
		nMarkers = sSynthStreamHakoBuffers[buf].markerNum;
		if (nMarkers > 0) {
			HSD_DevComRequest(PSTREAM(node)->entrynum, offset + 0x20, NULL, OSRoundUp32B(nMarkers * 8), 0x22, 0,
			                  (void (*)(s32, void*, s32, s32))HSD_SynthPStreamHakoMarkerCallback, (void*)pstreamID);
		}
		HSD_DevComRequest(PSTREAM(node)->entrynum, offset + 0x20 + OSRoundUp32B(nMarkers * 8), (void*)(sSynthStreamBufferAramBase + (buf << 16)),
		                  sSynthStreamHakoBuffers[buf].size, 0x23, 0, (void (*)(s32, void*, s32, s32))QueueLoadedSynthStreamBuffer, (void*)pstreamID);
	}
}

/**
 * @note Address: 0x801F0F0C
 * @note Size: 0x16C
 */
void HSD_SynthPStreamNextHakoCallback(s32 pstreamID)
{
	HSD_SynthSFXNode* node;
	s32 offset;
	s32 buf;
	u32 enabled = OSDisableInterrupts();

	node = getNode(pstreamID);
	if (node != NULL && node->link.pstream != NULL) {
		offset = sSynthStreamHakoBuffers[lastBuf(PSTREAM(node))].next;
		if (offset == -1) {
			ASSERT_SYNTH(0xD7B, 0);
		}
		buf                = getFreeBuf();
		PSTREAM(node)->_1C = buf;
		HSD_DevComRequest(PSTREAM(node)->entrynum, offset, &sSynthStreamHakoBuffers[buf], 0x20, 0x21, 0,
		                  (void (*)(s32, void*, s32, s32))HSD_SynthPStreamHakoHeaderCallback, (void*)pstreamID);
	}
	OSRestoreInterrupts(enabled);
}

/* pstream bookkeeping as these two functions see it, the shared header
   struct is missing most of these fields */

void AXSetVoiceVe(AXVPB* p, AXPBVE* ve);
void AXSetVoiceSrcType(AXVPB* p, u32 type);
void AXSetVoiceSrc(AXVPB* p, AXPBSRC* src);
void AXSetVoiceCurrentAddr(AXVPB* p, u32 addr);
void AXSetVoiceEndAddr(AXVPB* p, u32 addr);
void AXSetVoiceLoopAddr(AXVPB* p, u32 addr);
void AXSetVoiceLoop(AXVPB* p, u16 loop);
void AXSetVoiceAdpcmLoop(AXVPB* p, void* loop);
void AXSetVoiceItdOn(AXVPB* p);
void AXSetVoiceState(AXVPB* p, u16 state);
void SetDvdStreamingSlotCallback(s32 id, void (*callback)(s32, void*));
void HSD_SynthPStreamNextHakoCallback(s32 pstreamID);

static inline BOOL isPStreamActive(HSD_SynthPStream* pstream)
{
	HSD_SynthPStream* p;

	for (p = (HSD_SynthPStream*)sSynthActivePStreams; p != NULL; p = p->next) {
		if (p == pstream) {
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * @note Address: 0x801F1078
 * @note Size: 0x934
 */
void HSD_SynthPStreamMasterClockCallback(void)
{
	HSD_SynthSFXNode* node;
	HSD_SynthPStream* pstream;
	HSD_SynthPStream* next;
	HSD_SynthSFXNode* n;
	HSD_SynthBeatMarker* marker;
	s32 a;
	s32 buf;
	s32 pos;
	s32 i;
	s32 j;

	lbl_8065BBD0 = 0;
	for (pstream = (HSD_SynthPStream*)sSynthActivePStreams; pstream != NULL; pstream = next) {
		node = pstream->node;
		next = pstream->next;
		if (node->flags & 8) {
			continue;
		}
		if (AXVPB_CURRENT_ADDR(node->voice[0]) < sSynthStreamBufferAramBase * 2) {
			continue;
		}
		a = (AXVPB_CURRENT_ADDR(node->voice[0]) - sSynthStreamBufferAramBase * 2) >> 17;
		if (a == pstream->bufTableHeadIx) {
			if (pstream->_18 == 1 && sSynthStreamHakoBuffers[lastBuf(pstream)].next != -1
			    && *(u32*)((u8*)node->voice[0] + 0x1AE) - AXVPB_CURRENT_ADDR(node->voice[0]) < 0xE60) {
				lbl_8065BBD0 = 1;
			}
			pos = AXVPB_CURRENT_ADDR(node->voice[0]) - sSynthStreamBufferAramBase * 2 - (a << 17);
			if (sSynthStreamHakoBuffers[a].markerIx < sSynthStreamHakoBuffers[a].markerNum) {
				while (sSynthStreamHakoBuffers[a].markerIx < sSynthStreamHakoBuffers[a].markerNum
				       && (marker = getBeatMarker(a, sSynthStreamHakoBuffers[a].markerIx))->pos < pos * 14 / 16) {
					if (pstream->callback != NULL) {
						pstream->callback(node->sfxID, marker->data, pstream->_24);
						if (!isPStreamActive(pstream)) {
							goto next_pstream;
						}
					}
					sSynthStreamHakoBuffers[a].markerIx++;
				}
			}
		} else {
			while (sSynthStreamHakoBuffers[pstream->bufTableHeadIx].markerIx < sSynthStreamHakoBuffers[pstream->bufTableHeadIx].markerNum) {
				marker = getBeatMarker(pstream->bufTableHeadIx, sSynthStreamHakoBuffers[pstream->bufTableHeadIx].markerIx);
				if (pstream->callback != NULL) {
					pstream->callback(node->sfxID, marker->data, pstream->_24);
					if (!isPStreamActive(pstream)) {
						goto next_pstream;
					}
				}
				sSynthStreamHakoBuffers[pstream->bufTableHeadIx].markerIx++;
			}
			buf = pstream->bufTableHeadIx;
			freeBeatMarkers(buf);
			unlinkBufTable(&pstream->bufTableHeadIx, buf);
			if (pstream->bufTableHeadIx != a) {
				OSReport("pst=%08x: pst->buftableheadix = %d, a=%d\n cadhi= %d pstbuf=%d\n", pstream, pstream->bufTableHeadIx, a,
				         AXVPB_CURRENT_ADDR(node->voice[0]), sSynthStreamBufferAramBase * 2);
				__assert(__FILE__, 0xDCB, "pstream->bufTableHeadIx == a");
			}
			pstream->_18--;
			if (pstream->schedulerSlot >= 0) {
				SetDvdStreamingSlotKey(pstream->schedulerSlot, pstream->_18);
			}
			for (j = 0; j < (node->rearIndex != -1) + 1; j++) {
				if (j == 0) {
					n = node;
				} else {
					n = &sSynthSfxNodes[node->rearIndex];
				}
				for (i = 0; i < node->voice_count; i++) {
					AXSetVoiceEndAddr(n->voice[i], (sSynthStreamBufferAramBase + (pstream->bufTableHeadIx << 16)) * 2
					                                   + i * sSynthStreamHakoBuffers[pstream->bufTableHeadIx].size
					                                   + sSynthStreamHakoBuffers[pstream->bufTableHeadIx].end);
				}
			}
		}
		if (a == pstream->_14) {
			if (pstream->_18 >= 2) {
				pstream->_14 = lbl_80656364[pstream->bufTableHeadIx];
				for (j = 0; j < (node->rearIndex != -1) + 1; j++) {
					if (j == 0) {
						n = node;
					} else {
						n = &sSynthSfxNodes[node->rearIndex];
					}
					for (i = 0; i < node->voice_count; i++) {
						AXSetVoiceLoopAddr(n->voice[i],
						                   (sSynthStreamBufferAramBase + (pstream->_14 << 16)) * 2 + i * sSynthStreamHakoBuffers[pstream->_14].size + 2);
						AXSetVoiceAdpcmLoop(n->voice[i], sSynthStreamHakoBuffers[pstream->_14].loop[i]);
					}
				}
			} else if (sSynthStreamHakoBuffers[a].next == -1) {
				pstream->_14 = -2;
				for (j = 0; j < (node->rearIndex != -1) + 1; j++) {
					if (j == 0) {
						n = node;
					} else {
						n = &sSynthSfxNodes[node->rearIndex];
					}
					for (i = 0; i < node->voice_count; i++) {
						AXSetVoiceLoopAddr(n->voice[i], (u32)lbl_8065BBE4);
						AXSetVoiceLoop(n->voice[i], 0);
					}
				}
			}
		}
	next_pstream:;
	}
}

/**
 * @note Address: 0x801F19AC
 * @note Size: 0x408
 */
void HSD_SynthPStreamFirstHakoDataCallback(s32 result, s32 pstreamID)
{
	HSD_SynthSFXNode* node = getNode(pstreamID);
	HSD_SynthSFXNode* n;
	AXPBVE ve;
	s32 buf;
	u32 enabled;
	s32 i;
	s32 j;

	if (node == NULL) {
		return;
	}
	buf = PSTREAM(node)->_1C;
	if (!(node->flags & 2)) {
		ve.currentVolume
		    = 32767.0f
		    * (node->u.user_vol[1].volume
		       * (node->u.user_vol[0].volume
		          * (lbl_80659318[node->channel].volume * (node->channel < 0x3F ? sSynthStreamVolume * lbl_80659300.volume : sSynthStreamVolume))));
	} else {
		ve.currentVolume = 0;
	}
	ve.currentDelta = 0;
	node->curVol    = ve.currentVolume;
	{
		s32 index                                        = PSTREAM(node)->_1C;
		*getBufTableTail(&PSTREAM(node)->bufTableHeadIx) = index;
		lbl_80656364[index]                              = -2;
	}
	PSTREAM(node)->_14 = buf;
	for (j = 0; j < (node->rearIndex != -1) + 1; j++) {
		HSD_SynthSFXNode* rear;
		if (j == 1) {
			rear                   = &sSynthSfxNodes[node->rearIndex];
			rear->u.rear.lastVol   = ve.currentVolume;
			rear->u.rear.lastDelta = ve.currentDelta;
			HSD_SynthSFXLinkUpdateRearPitch(rear, 0);
			if (node->flags & 4) {
				rear->u.rear.pitch = 0.0f;
			} else {
				rear->u.rear.pitch = node->pitch[1] * (node->freqBase * node->pitch[0]);
			}
		}
		if (j == 0) {
			n = node;
		} else {
			n = rear;
		}
		for (i = 0; i < node->voice_count; i++) {
			AXSetVoiceVe(n->voice[i], &ve);
			if (node->flags & 4) {
				*(u32*)&lbl_8050932C.ratioHi = 0;
			} else {
				*(u32*)&lbl_8050932C.ratioHi = 65536.0f * (node->pitch[1] * (node->freqBase * node->pitch[0]));
			}
			AXSetVoiceSrcType(n->voice[i], 2);
			AXSetVoiceSrc(n->voice[i], &lbl_8050932C);
			AXSetVoiceCurrentAddr(n->voice[i], (sSynthStreamBufferAramBase + (buf << 16)) * 2 + i * sSynthStreamHakoBuffers[buf].size + 2);
			AXSetVoiceEndAddr(n->voice[i], (sSynthStreamBufferAramBase + (buf << 16)) * 2 + i * sSynthStreamHakoBuffers[buf].size + sSynthStreamHakoBuffers[buf].end);
			AXSetVoiceLoopAddr(n->voice[i], (sSynthStreamBufferAramBase + (buf << 16)) * 2 + i * sSynthStreamHakoBuffers[buf].size + 2);
			if (sSynthSoundMode == 1) {
				AXSetVoiceItdOn(n->voice[i]);
			}
			if (j == 0) {
				AXSetVoiceState(n->voice[i], 1);
			}
		}
	}
	node->flags &= ~8;
	enabled = OSDisableInterrupts();
	if (!node->volUpdatePending && !(node->flags & 8)) {
		node->volUpdatePending = 1;
		node->volUpdateNext    = sSynthPendingVolumeUpdates;
		sSynthPendingVolumeUpdates           = node;
	}
	OSRestoreInterrupts(enabled);
	HSD_SynthSFXInitMix(node);
	HSD_SynthSFXUpdateMix(node, 0);
	ApplySynthMixTargetImmediately(node);
	PSTREAM(node)->_18 = 1;
	SetDvdStreamingSlotKey(PSTREAM(node)->schedulerSlot, 1);
	if (sSynthStreamHakoBuffers[buf].next == -1) {
		ReleaseDvdStreamingSlot(PSTREAM(node)->schedulerSlot);
		PSTREAM(node)->schedulerSlot = -1;
	} else {
		SetDvdStreamingSlotCallback(PSTREAM(node)->schedulerSlot, (void (*)(s32, void*))HSD_SynthPStreamNextHakoCallback);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x2D0
 */
void HSD_SynthPStreamFirstHakoMarkerCallback(s32 unused, s32 id, HSD_SynthBeatMarker* markers)
{
	HSD_SynthSFXNode* node = getNode(id);
	s32 n;

	if (node == NULL) {
		return;
	}
	// no null check on the link here, unlike HSD_SynthPStreamHakoMarkerCallback
	n = sSynthStreamHakoBuffers[PSTREAM(node)->_1C].markerNum;
	linkBeatMarkers(PSTREAM(node), markers, n);
}

/**
 * @note Address: 0x801F2084
 * @note Size: 0x14C
 */
void HSD_SynthPStreamFirstHakoHeaderCallback(s32 result, s32 pstreamID)
{
	HSD_SynthSFXNode* node = getNode(pstreamID);
	s32 buf;
	s32 nMarkers;

	if (node != NULL) {
		buf = PSTREAM(node)->_1C;
		if (PSTREAM(node)->_28 != 0 && sSynthStreamHakoBuffers[buf].next <= 0xA0) {
			sSynthStreamHakoBuffers[buf].next = -1;
		}
		nMarkers = sSynthStreamHakoBuffers[buf].markerNum;
		if (nMarkers > 0) {
			HSD_DevComRequest(PSTREAM(node)->entrynum, 0xA0, NULL, OSRoundUp32B(nMarkers * 8), 0x22, 0,
			                  (void (*)(s32, void*, s32, s32))HSD_SynthPStreamFirstHakoMarkerCallback, (void*)pstreamID);
		}
		HSD_DevComRequest(PSTREAM(node)->entrynum, OSRoundUp32B(nMarkers * 8) + 0xA0, (void*)(sSynthStreamBufferAramBase + (buf << 16)),
		                  sSynthStreamHakoBuffers[buf].size, 0x23, 0, (void (*)(s32, void*, s32, s32))HSD_SynthPStreamFirstHakoDataCallback,
		                  (void*)pstreamID);
	}
	sSynthStreamHakoBuffers[buf].markerIx = 0;
}

/**
 * @note Address: 0x801F0AB4
 * @note Size: 0x2EC
 */
// HSD_SynthPStreamHakoMarkerCallback
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    stw      r0, 0x44(r1)
    addi     r11, r1, 0x40
    bl       _savegpr_21
    clrlwi   r0, r4, 26
    lis      r3, sSynthSfxNodes@ha
    mulli    r6, r0, 0x98
    cmpwi    r4, 0x0
    addi     r0, r3, sSynthSfxNodes@l
    mr       r21, r5
    add      r3, r0, r6
    ble      .L_801F0AF8
    lwz      r0, 0x0(r3)
    cmpw     r0, r4
    bne      .L_801F0AF8
    b        .L_801F0AFC
.L_801F0AF8:
    li       r3, 0x0
.L_801F0AFC:
    cmplwi   r3, 0x0
    beq      .L_801F0D88
    lwz      r26, 0x30(r3)
    cmplwi   r26, 0x0
    beq      .L_801F0D88
    lwz      r0, 0x1c(r26)
    lis      r3, sSynthStreamHakoBuffers@ha
    addi     r3, r3, sSynthStreamHakoBuffers@l
    slwi     r24, r0, 6
    add      r3, r3, r24
    lbz      r23, 0x1c(r3)
    bl       OSDisableInterrupts
    lis      r4, sSynthStreamHakoBuffers@ha
    mr       r30, r3
    mr       r27, r21
    li       r29, 0x0
    addi     r31, r4, sSynthStreamHakoBuffers@l
    b        .L_801F0D64
.L_801F0B44:
    lwz      r0, 0x1c(r26)
    mr       r28, r29
    slwi     r0, r0, 6
    add      r3, r31, r0
    addi     r21, r3, 0x20
    lwz      r25, 0x20(r3)
    cmplwi   r25, 0x0
    bne      .L_801F0D38
    lwz      r25, sSynthFreeBeatMarkerBlocks@sda21(r0)
    cmplwi   r25, 0x0
    beq      .L_801F0B7C
    lwz      r0, 0x0(r25)
    stw      r0, sSynthFreeBeatMarkerBlocks@sda21(r0)
    b        .L_801F0C10
.L_801F0B7C:
    lwz      r12, sSynthMallocProc@sda21(r0)
    li       r3, 0x100
    mtctr    r12
    bctrl
    mr.      r22, r3
    bne      .L_801F0BB8
    lis      r3, sSynthHeapMessageTable@ha
    li       r4, 0x100
    addi     r3, r3, sSynthHeapMessageTable@l
    crclr    cr1eq
    bl       OSReport
    li       r3, sPathSynthC@sda21
    li       r4, 0xae
    li       r5, sSynthHeapPointerAssert@sda21
    bl       __assert
.L_801F0BB8:
    lwz      r0, sSynthFreeBeatMarkerBlocks@sda21(r0)
    addi     r8, r22, 0x20
    addi     r7, r22, 0x40
    addi     r6, r22, 0x60
    stw      r0, 0x0(r22)
    addi     r5, r22, 0x80
    addi     r4, r22, 0xa0
    addi     r3, r22, 0xc0
    stw      r22, sSynthFreeBeatMarkerBlocks@sda21(r0)
    addi     r25, r22, 0xe0
    stw      r22, 0x20(r22)
    stw      r8, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r8, 0x40(r22)
    stw      r7, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r7, 0x60(r22)
    stw      r6, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r6, 0x80(r22)
    stw      r5, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r5, 0xa0(r22)
    stw      r4, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r4, 0xc0(r22)
    stw      r3, sSynthFreeBeatMarkerBlocks@sda21(r0)
.L_801F0C10:
    cmplwi   r25, 0x0
    bne      .L_801F0C28
    li       r3, sPathSynthC@sda21
    li       r4, 0x4bc
    li       r5, sSynthBeatMarkerAssert@sda21
    bl       __assert
.L_801F0C28:
    stw      r25, 0x0(r21)
    li       r0, 0x0
    stw      r0, 0x0(r25)
    b        .L_801F0D38
.L_801F0C38:
    lwz      r21, 0x0(r25)
    cmplwi   r21, 0x0
    bne      .L_801F0D30
    lwz      r21, sSynthFreeBeatMarkerBlocks@sda21(r0)
    cmplwi   r21, 0x0
    beq      .L_801F0C5C
    lwz      r0, 0x0(r21)
    stw      r0, sSynthFreeBeatMarkerBlocks@sda21(r0)
    b        .L_801F0CF0
.L_801F0C5C:
    lwz      r12, sSynthMallocProc@sda21(r0)
    li       r3, 0x100
    mtctr    r12
    bctrl
    mr.      r22, r3
    bne      .L_801F0C98
    lis      r3, sSynthHeapMessageTable@ha
    li       r4, 0x100
    addi     r3, r3, sSynthHeapMessageTable@l
    crclr    cr1eq
    bl       OSReport
    li       r3, sPathSynthC@sda21
    li       r4, 0xae
    li       r5, sSynthHeapPointerAssert@sda21
    bl       __assert
.L_801F0C98:
    lwz      r0, sSynthFreeBeatMarkerBlocks@sda21(r0)
    addi     r8, r22, 0x20
    addi     r7, r22, 0x40
    addi     r6, r22, 0x60
    stw      r0, 0x0(r22)
    addi     r5, r22, 0x80
    addi     r4, r22, 0xa0
    addi     r3, r22, 0xc0
    stw      r22, sSynthFreeBeatMarkerBlocks@sda21(r0)
    addi     r21, r22, 0xe0
    stw      r22, 0x20(r22)
    stw      r8, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r8, 0x40(r22)
    stw      r7, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r7, 0x60(r22)
    stw      r6, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r6, 0x80(r22)
    stw      r5, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r5, 0xa0(r22)
    stw      r4, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r4, 0xc0(r22)
    stw      r3, sSynthFreeBeatMarkerBlocks@sda21(r0)
.L_801F0CF0:
    cmplwi   r21, 0x0
    bne      .L_801F0D08
    li       r3, sPathSynthC@sda21
    li       r4, 0x4c4
    li       r5, sSynthNextBeatMarkerAssert@sda21
    bl       __assert
.L_801F0D08:
    stw      r21, 0x0(r25)
    li       r3, 0x0
    li       r0, -0x1
    stw      r3, 0x0(r21)
    stw      r0, 0x8(r21)
    stw      r3, 0x8(r21)
    stw      r0, 0x10(r21)
    stw      r3, 0x10(r21)
    stw      r0, 0x18(r21)
    stw      r3, 0x18(r21)
.L_801F0D30:
    mr       r25, r21
    subi     r28, r28, 0x3
.L_801F0D38:
    cmpwi    r28, 0x3
    bge      .L_801F0C38
    slwi     r4, r28, 3
    lwz      r3, 0x0(r27)
    addi     r4, r4, 0x8
    lwz      r0, 0x4(r27)
    add      r4, r25, r4
    addi     r27, r27, 0x8
    stw      r3, 0x0(r4)
    addi     r29, r29, 0x1
    stw      r0, 0x4(r4)
.L_801F0D64:
    cmpw     r29, r23
    blt      .L_801F0B44
    mr       r3, r30
    bl       OSRestoreInterrupts
    lis      r3, sSynthStreamHakoBuffers@ha
    li       r4, 0x0
    addi     r0, r3, sSynthStreamHakoBuffers@l
    add      r3, r0, r24
    stw      r4, 0x24(r3)
.L_801F0D88:
    addi     r11, r1, 0x40
    bl       _restgpr_21
    lwz      r0, 0x44(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

/**
 * @note Address: 0x801F1078
 * @note Size: 0x934
 */
// HSD_SynthPStreamMasterClockCallback
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    stw      r0, 0x44(r1)
    addi     r11, r1, 0x40
    bl       _savegpr_18
    li       r0, 0x0
    lis      r4, sSynthHeapMessageTable@ha
    lis      r3, hsd_SynthSFXLoadBuf@ha
    stw      r0, lbl_8065BBD0@sda21(r0)
    lwz      r28, sSynthActivePStreams@sda21(r0)
    addi     r30, r4, sSynthHeapMessageTable@l
    addi     r31, r3, hsd_SynthSFXLoadBuf@l
    b        .L_801F198C
.L_801F10AC:
    lwz      r29, 0x4(r28)
    lwz      r26, 0x0(r28)
    lbz      r0, 0x9(r29)
    rlwinm.  r0, r0, 0, 28, 28
    bne      .L_801F1988
    lwz      r7, 0xc(r29)
    lwz      r0, sSynthStreamBufferAramBase@sda21(r0)
    lwz      r6, 0x1b2(r7)
    slwi     r0, r0, 1
    cmplw    r6, r0
    blt      .L_801F1988
    subf     r4, r0, r6
    lwz      r3, 0x10(r28)
    srwi     r27, r4, 17
    cmpw     r27, r3
    bne      .L_801F1408
    lwz      r3, 0x18(r28)
    cmpwi    r3, 0x1
    bne      .L_801F1154
    addi     r5, r28, 0x10
    addi     r4, r31, 0x424
    b        .L_801F1110
.L_801F1104:
    slwi     r3, r5, 2
    addi     r5, r31, 0x424
    add      r5, r5, r3
.L_801F1110:
    lwz      r5, 0x0(r5)
    slwi     r3, r5, 2
    lwzx     r3, r4, r3
    cmpwi    r3, -0x2
    bne      .L_801F1104
    slwi     r3, r5, 6
    addi     r4, r31, 0x480
    addi     r3, r3, 0x8
    lwzx     r3, r4, r3
    cmpwi    r3, -0x1
    beq      .L_801F1154
    lwz      r3, 0x1ae(r7)
    subf     r3, r6, r3
    cmplwi   r3, 0xe60
    bge      .L_801F1154
    li       r3, 0x1
    stw      r3, lbl_8065BBD0@sda21(r0)
.L_801F1154:
    slwi     r3, r27, 6
    addi     r7, r31, 0x480
    add      r7, r7, r3
    lwz      r5, 0xc(r29)
    addi     r23, r7, 0x24
    slwi     r6, r27, 17
    addi     r22, r7, 0x1c
    lwz      r4, 0x24(r7)
    lbz      r3, 0x1c(r7)
    lwz      r5, 0x1b2(r5)
    cmpw     r4, r3
    subf     r0, r0, r5
    subf     r0, r6, r0
    bge      .L_801F1800
    mulli    r0, r0, 0xe
    addi     r24, r7, 0x20
    srawi    r0, r0, 4
    addze    r25, r0
    b        .L_801F11FC
.L_801F11A0:
    lwz      r12, 0x20(r28)
    cmplwi   r12, 0x0
    beq      .L_801F11F0
    lwz      r3, 0x0(r29)
    lwz      r4, 0x4(r4)
    lwz      r5, 0x24(r28)
    mtctr    r12
    bctrl
    lwz      r3, sSynthActivePStreams@sda21(r0)
    b        .L_801F11DC
.L_801F11C8:
    cmplw    r3, r28
    bne      .L_801F11D8
    li       r0, 0x1
    b        .L_801F11E8
.L_801F11D8:
    lwz      r3, 0x0(r3)
.L_801F11DC:
    cmplwi   r3, 0x0
    bne      .L_801F11C8
    li       r0, 0x0
.L_801F11E8:
    cmpwi    r0, 0x0
    beq      .L_801F1988
.L_801F11F0:
    lwz      r3, 0x0(r23)
    addi     r0, r3, 0x1
    stw      r0, 0x0(r23)
.L_801F11FC:
    lwz      r21, 0x0(r23)
    lbz      r0, 0x0(r22)
    cmpw     r21, r0
    bge      .L_801F1800
    lwz      r20, 0x0(r24)
    cmplwi   r20, 0x0
    bne      .L_801F13E4
    lwz      r20, sSynthFreeBeatMarkerBlocks@sda21(r0)
    cmplwi   r20, 0x0
    beq      .L_801F1230
    lwz      r0, 0x0(r20)
    stw      r0, sSynthFreeBeatMarkerBlocks@sda21(r0)
    b        .L_801F12C0
.L_801F1230:
    lwz      r12, sSynthMallocProc@sda21(r0)
    li       r3, 0x100
    mtctr    r12
    bctrl
    mr.      r18, r3
    bne      .L_801F1268
    addi     r3, r30, 0x0
    li       r4, 0x100
    crclr    cr1eq
    bl       OSReport
    li       r3, sPathSynthC@sda21
    li       r4, 0xae
    li       r5, sSynthHeapPointerAssert@sda21
    bl       __assert
.L_801F1268:
    lwz      r0, sSynthFreeBeatMarkerBlocks@sda21(r0)
    addi     r8, r18, 0x20
    addi     r7, r18, 0x40
    addi     r6, r18, 0x60
    stw      r0, 0x0(r18)
    addi     r5, r18, 0x80
    addi     r4, r18, 0xa0
    addi     r3, r18, 0xc0
    stw      r18, sSynthFreeBeatMarkerBlocks@sda21(r0)
    addi     r20, r18, 0xe0
    stw      r18, 0x20(r18)
    stw      r8, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r8, 0x40(r18)
    stw      r7, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r7, 0x60(r18)
    stw      r6, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r6, 0x80(r18)
    stw      r5, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r5, 0xa0(r18)
    stw      r4, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r4, 0xc0(r18)
    stw      r3, sSynthFreeBeatMarkerBlocks@sda21(r0)
.L_801F12C0:
    cmplwi   r20, 0x0
    bne      .L_801F12D8
    li       r3, sPathSynthC@sda21
    li       r4, 0x4bc
    li       r5, sSynthBeatMarkerAssert@sda21
    bl       __assert
.L_801F12D8:
    stw      r20, 0x0(r24)
    li       r0, 0x0
    stw      r0, 0x0(r20)
    b        .L_801F13E4
.L_801F12E8:
    lwz      r19, 0x0(r20)
    cmplwi   r19, 0x0
    bne      .L_801F13DC
    lwz      r19, sSynthFreeBeatMarkerBlocks@sda21(r0)
    cmplwi   r19, 0x0
    beq      .L_801F130C
    lwz      r0, 0x0(r19)
    stw      r0, sSynthFreeBeatMarkerBlocks@sda21(r0)
    b        .L_801F139C
.L_801F130C:
    lwz      r12, sSynthMallocProc@sda21(r0)
    li       r3, 0x100
    mtctr    r12
    bctrl
    mr.      r18, r3
    bne      .L_801F1344
    addi     r3, r30, 0x0
    li       r4, 0x100
    crclr    cr1eq
    bl       OSReport
    li       r3, sPathSynthC@sda21
    li       r4, 0xae
    li       r5, sSynthHeapPointerAssert@sda21
    bl       __assert
.L_801F1344:
    lwz      r0, sSynthFreeBeatMarkerBlocks@sda21(r0)
    addi     r8, r18, 0x20
    addi     r7, r18, 0x40
    addi     r6, r18, 0x60
    stw      r0, 0x0(r18)
    addi     r5, r18, 0x80
    addi     r4, r18, 0xa0
    addi     r3, r18, 0xc0
    stw      r18, sSynthFreeBeatMarkerBlocks@sda21(r0)
    addi     r19, r18, 0xe0
    stw      r18, 0x20(r18)
    stw      r8, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r8, 0x40(r18)
    stw      r7, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r7, 0x60(r18)
    stw      r6, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r6, 0x80(r18)
    stw      r5, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r5, 0xa0(r18)
    stw      r4, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r4, 0xc0(r18)
    stw      r3, sSynthFreeBeatMarkerBlocks@sda21(r0)
.L_801F139C:
    cmplwi   r19, 0x0
    bne      .L_801F13B4
    li       r3, sPathSynthC@sda21
    li       r4, 0x4c4
    li       r5, sSynthNextBeatMarkerAssert@sda21
    bl       __assert
.L_801F13B4:
    stw      r19, 0x0(r20)
    li       r3, 0x0
    li       r0, -0x1
    stw      r3, 0x0(r19)
    stw      r0, 0x8(r19)
    stw      r3, 0x8(r19)
    stw      r0, 0x10(r19)
    stw      r3, 0x10(r19)
    stw      r0, 0x18(r19)
    stw      r3, 0x18(r19)
.L_801F13DC:
    mr       r20, r19
    subi     r21, r21, 0x3
.L_801F13E4:
    cmpwi    r21, 0x3
    bge      .L_801F12E8
    slwi     r3, r21, 3
    addi     r4, r3, 0x8
    add      r4, r20, r4
    lwz      r0, 0x0(r4)
    cmpw     r0, r25
    blt      .L_801F11A0
    b        .L_801F1800
.L_801F1408:
    addi     r25, r31, 0x480
    b        .L_801F1670
.L_801F1410:
    slwi     r3, r7, 6
    lwz      r24, 0x24(r5)
    addi     r18, r3, 0x20
    lwzx     r19, r25, r18
    cmplwi   r19, 0x0
    bne      .L_801F15F4
    lwz      r19, sSynthFreeBeatMarkerBlocks@sda21(r0)
    cmplwi   r19, 0x0
    beq      .L_801F1440
    lwz      r0, 0x0(r19)
    stw      r0, sSynthFreeBeatMarkerBlocks@sda21(r0)
    b        .L_801F14D0
.L_801F1440:
    lwz      r12, sSynthMallocProc@sda21(r0)
    li       r3, 0x100
    mtctr    r12
    bctrl
    mr.      r20, r3
    bne      .L_801F1478
    addi     r3, r30, 0x0
    li       r4, 0x100
    crclr    cr1eq
    bl       OSReport
    li       r3, sPathSynthC@sda21
    li       r4, 0xae
    li       r5, sSynthHeapPointerAssert@sda21
    bl       __assert
.L_801F1478:
    lwz      r0, sSynthFreeBeatMarkerBlocks@sda21(r0)
    addi     r8, r20, 0x20
    addi     r7, r20, 0x40
    addi     r6, r20, 0x60
    stw      r0, 0x0(r20)
    addi     r5, r20, 0x80
    addi     r4, r20, 0xa0
    addi     r3, r20, 0xc0
    stw      r20, sSynthFreeBeatMarkerBlocks@sda21(r0)
    addi     r19, r20, 0xe0
    stw      r20, 0x20(r20)
    stw      r8, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r8, 0x40(r20)
    stw      r7, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r7, 0x60(r20)
    stw      r6, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r6, 0x80(r20)
    stw      r5, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r5, 0xa0(r20)
    stw      r4, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r4, 0xc0(r20)
    stw      r3, sSynthFreeBeatMarkerBlocks@sda21(r0)
.L_801F14D0:
    cmplwi   r19, 0x0
    bne      .L_801F14E8
    li       r3, sPathSynthC@sda21
    li       r4, 0x4bc
    li       r5, sSynthBeatMarkerAssert@sda21
    bl       __assert
.L_801F14E8:
    stwx     r19, r25, r18
    li       r0, 0x0
    stw      r0, 0x0(r19)
    b        .L_801F15F4
.L_801F14F8:
    lwz      r18, 0x0(r19)
    cmplwi   r18, 0x0
    bne      .L_801F15EC
    lwz      r18, sSynthFreeBeatMarkerBlocks@sda21(r0)
    cmplwi   r18, 0x0
    beq      .L_801F151C
    lwz      r0, 0x0(r18)
    stw      r0, sSynthFreeBeatMarkerBlocks@sda21(r0)
    b        .L_801F15AC
.L_801F151C:
    lwz      r12, sSynthMallocProc@sda21(r0)
    li       r3, 0x100
    mtctr    r12
    bctrl
    mr.      r20, r3
    bne      .L_801F1554
    addi     r3, r30, 0x0
    li       r4, 0x100
    crclr    cr1eq
    bl       OSReport
    li       r3, sPathSynthC@sda21
    li       r4, 0xae
    li       r5, sSynthHeapPointerAssert@sda21
    bl       __assert
.L_801F1554:
    lwz      r0, sSynthFreeBeatMarkerBlocks@sda21(r0)
    addi     r8, r20, 0x20
    addi     r7, r20, 0x40
    addi     r6, r20, 0x60
    stw      r0, 0x0(r20)
    addi     r5, r20, 0x80
    addi     r4, r20, 0xa0
    addi     r3, r20, 0xc0
    stw      r20, sSynthFreeBeatMarkerBlocks@sda21(r0)
    addi     r18, r20, 0xe0
    stw      r20, 0x20(r20)
    stw      r8, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r8, 0x40(r20)
    stw      r7, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r7, 0x60(r20)
    stw      r6, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r6, 0x80(r20)
    stw      r5, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r5, 0xa0(r20)
    stw      r4, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r4, 0xc0(r20)
    stw      r3, sSynthFreeBeatMarkerBlocks@sda21(r0)
.L_801F15AC:
    cmplwi   r18, 0x0
    bne      .L_801F15C4
    li       r3, sPathSynthC@sda21
    li       r4, 0x4c4
    li       r5, sSynthNextBeatMarkerAssert@sda21
    bl       __assert
.L_801F15C4:
    stw      r18, 0x0(r19)
    li       r3, 0x0
    li       r0, -0x1
    stw      r3, 0x0(r18)
    stw      r0, 0x8(r18)
    stw      r3, 0x8(r18)
    stw      r0, 0x10(r18)
    stw      r3, 0x10(r18)
    stw      r0, 0x18(r18)
    stw      r3, 0x18(r18)
.L_801F15EC:
    mr       r19, r18
    subi     r24, r24, 0x3
.L_801F15F4:
    cmpwi    r24, 0x3
    bge      .L_801F14F8
    lwz      r12, 0x20(r28)
    slwi     r3, r24, 3
    addi     r4, r3, 0x8
    cmplwi   r12, 0x0
    add      r4, r19, r4
    beq      .L_801F1658
    lwz      r3, 0x0(r29)
    lwz      r4, 0x4(r4)
    lwz      r5, 0x24(r28)
    mtctr    r12
    bctrl
    lwz      r3, sSynthActivePStreams@sda21(r0)
    b        .L_801F1644
.L_801F1630:
    cmplw    r3, r28
    bne      .L_801F1640
    li       r0, 0x1
    b        .L_801F1650
.L_801F1640:
    lwz      r3, 0x0(r3)
.L_801F1644:
    cmplwi   r3, 0x0
    bne      .L_801F1630
    li       r0, 0x0
.L_801F1650:
    cmpwi    r0, 0x0
    beq      .L_801F1988
.L_801F1658:
    lwz      r0, 0x10(r28)
    slwi     r3, r0, 6
    addi     r4, r3, 0x24
    lwzx     r3, r25, r4
    addi     r0, r3, 0x1
    stwx     r0, r25, r4
.L_801F1670:
    lwz      r7, 0x10(r28)
    addi     r5, r31, 0x480
    slwi     r4, r7, 6
    add      r5, r5, r4
    lwz      r3, 0x24(r5)
    lbz      r0, 0x1c(r5)
    cmpw     r3, r0
    blt      .L_801F1410
    addi     r3, r31, 0x480
    addi     r8, r4, 0x20
    lwzx     r5, r3, r8
    addi     r6, r28, 0x10
    b        .L_801F16B8
.L_801F16A4:
    lwz      r4, 0x0(r5)
    lwz      r0, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r0, 0x0(r5)
    stw      r5, sSynthFreeBeatMarkerBlocks@sda21(r0)
    mr       r5, r4
.L_801F16B8:
    cmplwi   r5, 0x0
    bne      .L_801F16A4
    li       r0, 0x0
    stwx     r0, r3, r8
    b        .L_801F16D8
.L_801F16CC:
    slwi     r0, r0, 2
    addi     r6, r31, 0x424
    add      r6, r6, r0
.L_801F16D8:
    lwz      r0, 0x0(r6)
    cmpw     r0, r7
    bne      .L_801F16CC
    slwi     r5, r0, 2
    addi     r4, r31, 0x424
    lwzx     r3, r4, r5
    li       r0, -0x1
    stw      r3, 0x0(r6)
    stwx     r0, r4, r5
    lwz      r5, 0x10(r28)
    cmpw     r5, r27
    beq      .L_801F173C
    lwz      r3, 0xc(r29)
    mr       r4, r28
    lwz      r0, sSynthStreamBufferAramBase@sda21(r0)
    mr       r6, r27
    lwz      r7, 0x1b2(r3)
    addi     r3, r30, 0x798
    slwi     r8, r0, 1
    crclr    cr1eq
    bl       OSReport
    addi     r5, r30, 0x7d8
    li       r3, sPathSynthC@sda21
    li       r4, 0xdcb
    bl       __assert
.L_801F173C:
    lwz      r3, 0x18(r28)
    subi     r0, r3, 0x1
    stw      r0, 0x18(r28)
    lwz      r3, 0xc(r28)
    cmpwi    r3, 0x0
    blt      .L_801F175C
    lwz      r4, 0x18(r28)
    bl       SetDvdStreamingSlotKey
.L_801F175C:
    li       r20, 0x0
    b        .L_801F17E0
.L_801F1764:
    cmpwi    r20, 0x0
    bne      .L_801F1774
    mr       r3, r29
    b        .L_801F1780
.L_801F1774:
    mulli    r0, r4, 0x98
    addi     r3, r31, 0x8c0
    add      r3, r3, r0
.L_801F1780:
    mr       r18, r3
    li       r19, 0x0
    b        .L_801F17D0
.L_801F178C:
    lwz      r3, 0x10(r28)
    addi     r6, r31, 0x480
    lwz      r5, sSynthStreamBufferAramBase@sda21(r0)
    slwi     r0, r3, 6
    slwi     r4, r3, 16
    add      r6, r6, r0
    lwz      r3, 0xc(r18)
    lwz      r0, 0x0(r6)
    add      r4, r5, r4
    lwz      r6, 0x4(r6)
    slwi     r5, r4, 1
    mullw    r0, r19, r0
    add      r4, r0, r6
    add      r4, r5, r4
    bl       AXSetVoiceEndAddr
    addi     r18, r18, 0x4
    addi     r19, r19, 0x1
.L_801F17D0:
    lbz      r0, 0xa(r29)
    cmpw     r19, r0
    blt      .L_801F178C
    addi     r20, r20, 0x1
.L_801F17E0:
    lha      r4, 0x14(r29)
    subfic   r3, r4, -0x1
    addi     r0, r4, 0x1
    or       r0, r3, r0
    srwi     r3, r0, 31
    addi     r0, r3, 0x1
    cmpw     r20, r0
    blt      .L_801F1764
.L_801F1800:
    lwz      r0, 0x14(r28)
    cmpw     r27, r0
    bne      .L_801F1988
    lwz      r0, 0x18(r28)
    cmpwi    r0, 0x2
    blt      .L_801F18EC
    lwz      r0, 0x10(r28)
    addi     r3, r31, 0x424
    addi     r27, r31, 0x480
    li       r19, 0x0
    slwi     r0, r0, 2
    lwzx     r0, r3, r0
    stw      r0, 0x14(r28)
    b        .L_801F18C8
.L_801F1838:
    cmpwi    r19, 0x0
    bne      .L_801F1848
    mr       r20, r29
    b        .L_801F1854
.L_801F1848:
    mulli    r0, r4, 0x98
    addi     r20, r31, 0x8c0
    add      r20, r20, r0
.L_801F1854:
    li       r21, 0x0
    mr       r24, r21
    b        .L_801F18B8
.L_801F1860:
    lwz      r3, 0x14(r28)
    lwz      r4, sSynthStreamBufferAramBase@sda21(r0)
    slwi     r0, r3, 6
    slwi     r3, r3, 16
    lwzx     r0, r27, r0
    add      r3, r4, r3
    slwi     r5, r3, 1
    lwz      r3, 0xc(r20)
    mullw    r4, r21, r0
    addi     r4, r4, 0x2
    add      r4, r5, r4
    bl       AXSetVoiceLoopAddr
    lwz      r0, 0x14(r28)
    lwz      r3, 0xc(r20)
    slwi     r0, r0, 6
    add      r4, r0, r24
    addi     r4, r4, 0xc
    add      r4, r27, r4
    bl       AXSetVoiceAdpcmLoop
    addi     r20, r20, 0x4
    addi     r24, r24, 0x8
    addi     r21, r21, 0x1
.L_801F18B8:
    lbz      r0, 0xa(r29)
    cmpw     r21, r0
    blt      .L_801F1860
    addi     r19, r19, 0x1
.L_801F18C8:
    lha      r4, 0x14(r29)
    subfic   r3, r4, -0x1
    addi     r0, r4, 0x1
    or       r0, r3, r0
    srwi     r3, r0, 31
    addi     r0, r3, 0x1
    cmpw     r19, r0
    blt      .L_801F1838
    b        .L_801F1988
.L_801F18EC:
    slwi     r4, r27, 6
    addi     r3, r31, 0x480
    addi     r0, r4, 0x8
    lwzx     r0, r3, r0
    cmpwi    r0, -0x1
    bne      .L_801F1988
    li       r0, -0x2
    li       r18, 0x0
    stw      r0, 0x14(r28)
    b        .L_801F1968
.L_801F1914:
    cmpwi    r18, 0x0
    bne      .L_801F1924
    mr       r19, r29
    b        .L_801F1930
.L_801F1924:
    mulli    r0, r4, 0x98
    addi     r19, r31, 0x8c0
    add      r19, r19, r0
.L_801F1930:
    li       r20, 0x0
    b        .L_801F1958
.L_801F1938:
    lwz      r3, 0xc(r19)
    lwz      r4, lbl_8065BBE4@sda21(r0)
    bl       AXSetVoiceLoopAddr
    lwz      r3, 0xc(r19)
    li       r4, 0x0
    bl       AXSetVoiceLoop
    addi     r19, r19, 0x4
    addi     r20, r20, 0x1
.L_801F1958:
    lbz      r0, 0xa(r29)
    cmpw     r20, r0
    blt      .L_801F1938
    addi     r18, r18, 0x1
.L_801F1968:
    lha      r4, 0x14(r29)
    subfic   r3, r4, -0x1
    addi     r0, r4, 0x1
    or       r0, r3, r0
    srwi     r3, r0, 31
    addi     r0, r3, 0x1
    cmpw     r18, r0
    blt      .L_801F1914
.L_801F1988:
    mr       r28, r26
.L_801F198C:
    cmplwi   r28, 0x0
    bne      .L_801F10AC
    addi     r11, r1, 0x40
    bl       _restgpr_18
    lwz      r0, 0x44(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

/**
 * @note Address: 0x801F1DB4
 * @note Size: 0x2D0
 */
// HSD_SynthPStreamFirstHakoMarkerCallback
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    stw      r0, 0x34(r1)
    addi     r11, r1, 0x30
    bl       _savegpr_22
    clrlwi   r0, r4, 26
    lis      r3, sSynthSfxNodes@ha
    mulli    r6, r0, 0x98
    cmpwi    r4, 0x0
    addi     r0, r3, sSynthSfxNodes@l
    mr       r22, r5
    add      r3, r0, r6
    ble      .L_801F1DF8
    lwz      r0, 0x0(r3)
    cmpw     r0, r4
    bne      .L_801F1DF8
    b        .L_801F1DFC
.L_801F1DF8:
    li       r3, 0x0
.L_801F1DFC:
    cmplwi   r3, 0x0
    beq      .L_801F206C
    lwz      r26, 0x30(r3)
    lis      r3, sSynthStreamHakoBuffers@ha
    addi     r3, r3, sSynthStreamHakoBuffers@l
    lwz      r0, 0x1c(r26)
    slwi     r0, r0, 6
    add      r3, r3, r0
    lbz      r24, 0x1c(r3)
    bl       OSDisableInterrupts
    lis      r4, sSynthStreamHakoBuffers@ha
    mr       r30, r3
    mr       r27, r22
    li       r29, 0x0
    addi     r31, r4, sSynthStreamHakoBuffers@l
    b        .L_801F205C
.L_801F1E3C:
    lwz      r0, 0x1c(r26)
    mr       r28, r29
    slwi     r0, r0, 6
    add      r3, r31, r0
    addi     r22, r3, 0x20
    lwz      r25, 0x20(r3)
    cmplwi   r25, 0x0
    bne      .L_801F2030
    lwz      r25, sSynthFreeBeatMarkerBlocks@sda21(r0)
    cmplwi   r25, 0x0
    beq      .L_801F1E74
    lwz      r0, 0x0(r25)
    stw      r0, sSynthFreeBeatMarkerBlocks@sda21(r0)
    b        .L_801F1F08
.L_801F1E74:
    lwz      r12, sSynthMallocProc@sda21(r0)
    li       r3, 0x100
    mtctr    r12
    bctrl
    mr.      r23, r3
    bne      .L_801F1EB0
    lis      r3, sSynthHeapMessageTable@ha
    li       r4, 0x100
    addi     r3, r3, sSynthHeapMessageTable@l
    crclr    cr1eq
    bl       OSReport
    li       r3, sPathSynthC@sda21
    li       r4, 0xae
    li       r5, sSynthHeapPointerAssert@sda21
    bl       __assert
.L_801F1EB0:
    lwz      r0, sSynthFreeBeatMarkerBlocks@sda21(r0)
    addi     r8, r23, 0x20
    addi     r7, r23, 0x40
    addi     r6, r23, 0x60
    stw      r0, 0x0(r23)
    addi     r5, r23, 0x80
    addi     r4, r23, 0xa0
    addi     r3, r23, 0xc0
    stw      r23, sSynthFreeBeatMarkerBlocks@sda21(r0)
    addi     r25, r23, 0xe0
    stw      r23, 0x20(r23)
    stw      r8, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r8, 0x40(r23)
    stw      r7, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r7, 0x60(r23)
    stw      r6, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r6, 0x80(r23)
    stw      r5, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r5, 0xa0(r23)
    stw      r4, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r4, 0xc0(r23)
    stw      r3, sSynthFreeBeatMarkerBlocks@sda21(r0)
.L_801F1F08:
    cmplwi   r25, 0x0
    bne      .L_801F1F20
    li       r3, sPathSynthC@sda21
    li       r4, 0x4bc
    li       r5, sSynthBeatMarkerAssert@sda21
    bl       __assert
.L_801F1F20:
    stw      r25, 0x0(r22)
    li       r0, 0x0
    stw      r0, 0x0(r25)
    b        .L_801F2030
.L_801F1F30:
    lwz      r22, 0x0(r25)
    cmplwi   r22, 0x0
    bne      .L_801F2028
    lwz      r22, sSynthFreeBeatMarkerBlocks@sda21(r0)
    cmplwi   r22, 0x0
    beq      .L_801F1F54
    lwz      r0, 0x0(r22)
    stw      r0, sSynthFreeBeatMarkerBlocks@sda21(r0)
    b        .L_801F1FE8
.L_801F1F54:
    lwz      r12, sSynthMallocProc@sda21(r0)
    li       r3, 0x100
    mtctr    r12
    bctrl
    mr.      r23, r3
    bne      .L_801F1F90
    lis      r3, sSynthHeapMessageTable@ha
    li       r4, 0x100
    addi     r3, r3, sSynthHeapMessageTable@l
    crclr    cr1eq
    bl       OSReport
    li       r3, sPathSynthC@sda21
    li       r4, 0xae
    li       r5, sSynthHeapPointerAssert@sda21
    bl       __assert
.L_801F1F90:
    lwz      r0, sSynthFreeBeatMarkerBlocks@sda21(r0)
    addi     r8, r23, 0x20
    addi     r7, r23, 0x40
    addi     r6, r23, 0x60
    stw      r0, 0x0(r23)
    addi     r5, r23, 0x80
    addi     r4, r23, 0xa0
    addi     r3, r23, 0xc0
    stw      r23, sSynthFreeBeatMarkerBlocks@sda21(r0)
    addi     r22, r23, 0xe0
    stw      r23, 0x20(r23)
    stw      r8, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r8, 0x40(r23)
    stw      r7, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r7, 0x60(r23)
    stw      r6, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r6, 0x80(r23)
    stw      r5, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r5, 0xa0(r23)
    stw      r4, sSynthFreeBeatMarkerBlocks@sda21(r0)
    stw      r4, 0xc0(r23)
    stw      r3, sSynthFreeBeatMarkerBlocks@sda21(r0)
.L_801F1FE8:
    cmplwi   r22, 0x0
    bne      .L_801F2000
    li       r3, sPathSynthC@sda21
    li       r4, 0x4c4
    li       r5, sSynthNextBeatMarkerAssert@sda21
    bl       __assert
.L_801F2000:
    stw      r22, 0x0(r25)
    li       r3, 0x0
    li       r0, -0x1
    stw      r3, 0x0(r22)
    stw      r0, 0x8(r22)
    stw      r3, 0x8(r22)
    stw      r0, 0x10(r22)
    stw      r3, 0x10(r22)
    stw      r0, 0x18(r22)
    stw      r3, 0x18(r22)
.L_801F2028:
    mr       r25, r22
    subi     r28, r28, 0x3
.L_801F2030:
    cmpwi    r28, 0x3
    bge      .L_801F1F30
    slwi     r4, r28, 3
    lwz      r3, 0x0(r27)
    addi     r4, r4, 0x8
    lwz      r0, 0x4(r27)
    add      r4, r25, r4
    addi     r27, r27, 0x8
    stw      r3, 0x0(r4)
    addi     r29, r29, 0x1
    stw      r0, 0x4(r4)
.L_801F205C:
    cmpw     r29, r24
    blt      .L_801F1E3C
    mr       r3, r30
    bl       OSRestoreInterrupts
.L_801F206C:
    addi     r11, r1, 0x30
    bl       _restgpr_22
    lwz      r0, 0x34(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

static u32 sSynthActiveChannelFaderMasks[2] = { 0, 0 };
static f32 sSynthStreamVolume    = 1.0f;

/**
 * @note Address: 0x801F21D0
 * @note Size: 0x2CC
 */
void HSD_SynthPStreamHeaderCallback(s32 result, s32 pstreamID, u32* header)
{
	HSD_SynthSFXNode* entry = getNode(pstreamID);
	s32 i;
	s32 bufReading;

	if (entry == NULL) {
		return;
	}
	entry->voice_count = header[3];
	if (entry->voice_count == 2) {
		entry->voice[1] = AXAcquireVoice(0x1F, dropcallback, 0);
		ASSERT_SYNTH(0xE94, entry->voice[1]);
		if (entry->rearIndex != -1) {
			AXVPB* voice            = AXAcquireVoice(0x1F, dropcallback, 0);
			HSD_SynthSFXNode* rear2 = &sSynthSfxNodes[voice->index];

			rear2->sfxID                               = -3;
			rear2->voice[0]                            = sSynthSfxNodes[entry->rearIndex].voice[0];
			sSynthSfxNodes[entry->rearIndex].voice[1]    = voice;
			sSynthSfxNodes[entry->rearIndex].voice_count = 2;
			rear2->voice_count                         = 2;
		}
	}
	entry->freqBase = 0.00003125f * header[2];
	for (i = 0; i < entry->voice_count; i++) {
		// ratioHi and ratioLo go out as one word
		*(u32*)&lbl_8050932C = 65536.0f * entry->freqBase;
		AXSetVoiceAddr(entry->voice[i], &header[i * 14 + 4]);
		AXSetVoiceAdpcm(entry->voice[i], &header[i * 14 + 8]);
	}
	if (entry->rearIndex != -1) {
		HSD_SynthSFXNode* rear = &sSynthSfxNodes[entry->rearIndex];

		for (i = 0; i < entry->voice_count; i++) {
			AXSetVoiceAddr(rear->voice[i], &header[i * 14 + 4]);
			AXSetVoiceAdpcm(rear->voice[i], &header[i * 14 + 8]);
		}
	}
	bufReading          = getFreeBuf();
	PSTREAM(entry)->_1C = bufReading;
	ASSERT_SYNTH(0xEB4, bufReading != -1);
	HSD_DevComRequest(PSTREAM(entry)->entrynum, 0x80, &sSynthStreamHakoBuffers[bufReading], 0x20, 0x21, 0,
	                  (void (*)(s32, void*, s32, s32))HSD_SynthPStreamFirstHakoHeaderCallback, (void*)pstreamID);
}

/**
 * @note Address: 0x801F249C
 * @note Size: 0xA0
 */
void HSD_SynthPStreamStartCallback(s32 pstreamID)
{
	HSD_SynthSFXNode* node = getNode(pstreamID);

	ASSERT_SYNTH_MSG(0xEC2, node != NULL, "entry");
	HSD_DevComRequest(node->link.pstream->entrynum, 0, NULL, 0x80, 0x22, 0, (void (*)(s32, void*, s32, s32))HSD_SynthPStreamHeaderCallback,
	                  (void*)(u32)pstreamID);
}

/**
 * @note Address: N/A
 * @note Size: 0x3B8
 */
static inline HSD_SynthPStream* allocPStream(void)
{
	HSD_SynthPStream* pst = sSynthFreePStreams;

	if (pst != NULL) {
		sSynthFreePStreams = pst->next;
		return pst;
	}
	return NULL;
}

s32 HSD_SynthPStreamStart(s32 entrynum, s32 vol, s32 volMode, s32 vol2, s32 vol2Mode, u8 vol1Clamp, u8 vol2Clamp, u8 pan, f32 pitch1,
                          f32 pitch2, f32 mixMain, f32 mixAuxA, f32 mixAuxB, s32 unk67, s32 channel, s32 useRear)
{
	u32 index;
	u32 enabled;
	AXVPB* voice;
	HSD_SynthSFXNode* node;
	HSD_SynthPStream* pst;

	if (entrynum < 0) {
		return 0;
	}
	if (channel < 0 || channel >= 0x40) {
		return 0;
	}
	enabled = OSDisableInterrupts();
	if (sSynthSoundMode != 4) {
		useRear = 0;
	}
	voice = AXAcquireVoice(0x1F, dropcallback, 0);
	index = voice->index;
	node  = &sSynthSfxNodes[index];

	sSynthSfxIdBase += 0x40;
	if (sSynthSfxIdBase < 0) {
		sSynthSfxIdBase = 0x40;
	}

	node->link.pstream            = allocPStream();
	PSTREAM(node)->entrynum       = entrynum;
	PSTREAM(node)->node           = node;
	PSTREAM(node)->bufTableHeadIx = -2;
	PSTREAM(node)->callback       = 0;
	PSTREAM(node)->_24            = 0;
	PSTREAM(node)->_28            = 0;
	pst                           = PSTREAM(node);
	pst->next                     = sSynthActivePStreams;
	sSynthActivePStreams                  = pst;

	node->sfxID       = sSynthSfxIdBase + index;
	node->next        = NULL;
	node->flags       = 8;
	node->voice_count = 1;
	node->channel     = channel;
	node->voice[0]    = voice;
	node->rearIndex   = -1;
	node->pitch[0]    = pitch1;
	node->pitch[1]    = pitch2;

	if (volMode == 0) {
		node->u.user_vol[0].volume   = 0.003921569f * vol;
		node->u.user_vol[0].target.f = node->u.user_vol[0].volume;
	} else {
		s32 raw                      = vol << 16;
		node->u.user_vol[0].volume   = HSD_SynthDBxToVol(raw);
		node->u.user_vol[0].target.i = raw;
		node->u.user_vol[0].dbxRaw   = raw;
	}
	node->u.user_vol[0].mode      = volMode;
	node->u.user_vol[0].fadeCount = 0;

	if (vol2Mode == 0) {
		node->u.user_vol[1].volume   = 0.003921569f * vol2;
		node->u.user_vol[1].target.f = node->u.user_vol[1].volume;
	} else {
		s32 raw                      = vol2 << 16;
		node->u.user_vol[1].volume   = HSD_SynthDBxToVol(raw);
		node->u.user_vol[1].target.i = raw;
		node->u.user_vol[1].dbxRaw   = raw;
	}
	node->u.user_vol[1].mode      = vol2Mode;
	node->u.user_vol[1].fadeCount = 0;

	node->userPan[0]  = vol1Clamp;
	node->userPan[1]  = vol2Clamp;
	node->userSPan[0] = pan;
	node->userSPan[1] = unk67;
	node->mixMain     = mixMain;
	node->mixAuxA     = mixAuxA;
	node->mixAuxB     = mixAuxB;

	if (useRear != 0) {
		AXVPB* rearVoice       = AXAcquireVoice(0x1F, dropcallback, 0);
		u32 rearIdx            = rearVoice->index;
		HSD_SynthSFXNode* rear = &sSynthSfxNodes[rearIdx];

		rear->sfxID       = -2;
		rear->rearIndex   = index;
		node->rearIndex   = rearIdx;
		rear->voice_count = 1;
		rear->voice[0]    = rearVoice;
	} else {
		node->rearIndex = -1;
	}
	PSTREAM(node)->schedulerSlot = AcquireDvdStreamingSlot((void (*)(s32, void*))HSD_SynthPStreamStartCallback, node->sfxID);
	OSRestoreInterrupts(enabled);
	return node->sfxID;
}

/**
 * @note Address: N/A
 * @note Size: 0x24
 */
void HSD_SynthStreamPause(void)
{
	AISetStreamPlayState(0);
}

/**
 * @note Address: N/A
 * @note Size: 0x24
 */
void HSD_SynthStreamResume(void)
{
	AISetStreamPlayState(1);
}

/**
 * @note Address: N/A
 * @note Size: 0x118
 */
void HSD_SynthStreamSetVolume(f32 volume)
{
	s32 i;
	u32 enabled;

	sSynthStreamVolume = volume;
	AISetStreamVolLeft(volume * (f32)sSynthStreamVolumeScale);
	AISetStreamVolRight(sSynthStreamVolume * (f32)sSynthStreamVolumeScale);
	enabled = OSDisableInterrupts();
	for (i = 0; i < HSD_SYNTHSFXNODE_MAX; i++) {
		HSD_SynthSFXNode* node = &sSynthSfxNodes[i];
		if (node->sfxID > 0) {
			HSD_SynthSFXUpdateVolume(node);
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801F28FC
 * @note Size: 0x14
 */
s32 HSD_SynthPStreamIsActive(void)
{
	return sSynthActivePStreams != NULL;
}

/**
 * @note Address: 0x801F2910
 * @note Size: 0x98
 */
void HSD_SynthPStreamSetBeatCallback(s32 id, s32 a, s32 b)
{
	HSD_SynthSFXNode* node;
	u32 enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL && node->link.pstream != NULL) {
		PSTREAM(node)->callback = (void (*)(s32, s32, s32))a;
		PSTREAM(node)->_24      = b;
	}
	OSRestoreInterrupts(enabled);
}

extern void AXInit(void);

/**
 * @note Address: N/A
 * @note Size: 0x4D4
 */
void HSD_SynthInit(s32 heapSize, s32 pstNum, s32 pstBufNum)
{
	s32 i;

	AXInit();
	ASSERT_SYNTH_REPORT(0xFDC, pstNum >= 1 && pstNum <= HSD_SYNTHPSTREAM_MAX, "HSD_AudioInit*: pstNum out of range.\n");
	ASSERT_SYNTH_REPORT(0xFDD, pstBufNum >= 1 && pstBufNum <= 3, "HSD_AudioInit*: pstBufNum out of range.\n");
	SetDvdStreamingLimits(pstNum, pstBufNum);
	sSynthStreamBufferCount = pstNum * (pstBufNum + 1) + 1;
	AISetDSPSampleRate(0);
	lbl_8065BBE4 = ARAlloc(0x500);
	HSD_DevComRequest(0, 0, lbl_8065BBE4, 0x500, 3, 0, NULL, 0);
	lbl_8065BBE4            = (void*)((s32)lbl_8065BBE4 * 2);
	hsd_SynthSFXBankHead[0] = (s32)ARAlloc(heapSize);
	AXRegisterCallback(HSD_SynthCallback);
	sSynthStreamVolumeScale = 0xFF;
	AISetStreamVolLeft(sSynthStreamVolume * (f32)sSynthStreamVolumeScale);
	AISetStreamVolRight(sSynthStreamVolume * (f32)sSynthStreamVolumeScale);

	lbl_80659300.volume   = 1.0f;
	lbl_80659300.target.f = 1.0f;
	lbl_80659300.duration = 0;
	for (i = 0; i < ARRAY_SIZE(lbl_80659318); i++) {
		lbl_80659318[i].volume   = 1.0f;
		lbl_80659318[i].target.f = 1.0f;
		lbl_80659318[i].duration = 0;
	}

	sSynthSfxBankHeapEnd = (void*)(hsd_SynthSFXBankHead[0] + heapSize);
	sSynthStreamBufferAramBase = (s32)ARAlloc(sSynthStreamBufferCount << 0x10);
	sSynthSoundMode = OSGetSoundMode();

	HSD_SynthInitPantable();

	for (i = 0; i < pstNum; i++) {
		lbl_80659918[i].next = sSynthFreePStreams;
		sSynthFreePStreams         = &lbl_80659918[i];
	}

	for (i = 0; i < sSynthStreamBufferCount; i++) {
		lbl_80656364[i] = -1;
	}
}

/**
 * @note Address: 0x801F253C
 * @note Size: 0x3C0
 */
// HSD_SynthPStreamStart
/*
    stwu     r1, -0xa0(r1)
    mflr     r0
    stw      r0, 0xa4(r1)
    stfd     f31, 0x90(r1)
    psq_st   f31, 0x98(r1), 0, qr0
    stfd     f30, 0x80(r1)
    psq_st   f30, 0x88(r1), 0, qr0
    stfd     f29, 0x70(r1)
    psq_st   f29, 0x78(r1), 0, qr0
    stfd     f28, 0x60(r1)
    psq_st   f28, 0x68(r1), 0, qr0
    stfd     f27, 0x50(r1)
    psq_st   f27, 0x58(r1), 0, qr0
    addi     r11, r1, 0x50
    bl       _savegpr_18
    mr.      r30, r3
    fmr      f27, f1
    fmr      f28, f2
    lwz      r24, 0xa8(r1)
    fmr      f29, f3
    lwz      r25, 0xac(r1)
    fmr      f30, f4
    fmr      f31, f5
    lwz      r26, 0xb0(r1)
    mr       r31, r4
    mr       r18, r5
    mr       r19, r6
    mr       r20, r7
    mr       r21, r8
    mr       r22, r9
    mr       r23, r10
    bge      .L_801F25C4
    li       r3, 0x0
    b        .L_801F28BC
.L_801F25C4:
    cmpwi    r25, 0x0
    blt      .L_801F25D4
    cmpwi    r25, 0x40
    blt      .L_801F25DC
.L_801F25D4:
    li       r3, 0x0
    b        .L_801F28BC
.L_801F25DC:
    bl       OSDisableInterrupts
    lwz      r0, sSynthSoundMode@sda21(r0)
    mr       r28, r3
    cmpwi    r0, 0x4
    beq      .L_801F25F4
    li       r26, 0x0
.L_801F25F4:
    lis      r4, dropcallback@ha
    li       r3, 0x1f
    addi     r4, r4, dropcallback@l
    li       r5, 0x0
    bl       AXAcquireVoice
    lwz      r4, sSynthSfxIdBase@sda21(r0)
    lis      r5, sSynthSfxNodes@ha
    lwz      r29, 0x18(r3)
    addi     r5, r5, sSynthSfxNodes@l
    addic.   r0, r4, 0x40
    mulli    r6, r29, 0x98
    stw      r0, sSynthSfxIdBase@sda21(r0)
    add      r27, r5, r6
    bge      .L_801F2634
    li       r0, 0x40
    stw      r0, sSynthSfxIdBase@sda21(r0)
.L_801F2634:
    lwz      r4, sSynthFreePStreams@sda21(r0)
    cmplwi   r4, 0x0
    beq      .L_801F264C
    lwz      r0, 0x0(r4)
    stw      r0, sSynthFreePStreams@sda21(r0)
    b        .L_801F2650
.L_801F264C:
    li       r4, 0x0
.L_801F2650:
    stw      r4, 0x30(r27)
    li       r8, -0x2
    li       r7, 0x0
    li       r5, 0x8
    lwz      r6, 0x30(r27)
    li       r4, 0x1
    li       r0, -0x1
    cmpwi    r18, 0x0
    stw      r30, 0x8(r6)
    lwz      r6, 0x30(r27)
    stw      r27, 0x4(r6)
    lwz      r6, 0x30(r27)
    stw      r8, 0x10(r6)
    lwz      r6, 0x30(r27)
    stw      r7, 0x20(r6)
    lwz      r6, 0x30(r27)
    stw      r7, 0x24(r6)
    lwz      r6, 0x30(r27)
    stw      r7, 0x28(r6)
    lwz      r8, 0x30(r27)
    lwz      r6, sSynthActivePStreams@sda21(r0)
    stw      r6, 0x0(r8)
    lwz      r6, sSynthSfxIdBase@sda21(r0)
    stw      r8, sSynthActivePStreams@sda21(r0)
    add      r6, r6, r29
    stw      r6, 0x0(r27)
    stw      r7, 0x4(r27)
    stb      r5, 0x9(r27)
    stb      r4, 0xa(r27)
    stb      r25, 0xb(r27)
    stw      r3, 0xc(r27)
    sth      r0, 0x14(r27)
    stfs     f27, 0x1c(r27)
    stfs     f28, 0x20(r27)
    bne      .L_801F2710
    xoris    r3, r31, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc(r1)
    lfd      f1, "@237"@sda21(r0)
    stw      r0, 0x8(r1)
    lfs      f2, lbl_8065FEC0@sda21(r0)
    lfd      f0, 0x8(r1)
    fsubs    f0, f0, f1
    fmuls    f0, f2, f0
    stfs     f0, 0x34(r27)
    lfs      f0, 0x34(r27)
    stfs     f0, 0x3c(r27)
    b        .L_801F2770
.L_801F2710:
    lwz      r0, sSynthDecibelFloor@sda21(r0)
    slwi     r25, r31, 16
    cmpw     r25, r0
    bgt      .L_801F2728
    lfs      f0, "@234"@sda21(r0)
    b        .L_801F2764
.L_801F2728:
    cmpwi    r25, 0x0
    blt      .L_801F2738
    lfs      f0, "@223"@sda21(r0)
    b        .L_801F2764
.L_801F2738:
    xoris    r3, r25, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc(r1)
    lfd      f1, "@237"@sda21(r0)
    stw      r0, 0x8(r1)
    lfd      f2, "@235"@sda21(r0)
    lfd      f0, 0x8(r1)
    fsub     f0, f0, f1
    fmul     f1, f2, f0
    bl       exp
    frsp     f0, f1
.L_801F2764:
    stfs     f0, 0x34(r27)
    stw      r25, 0x3c(r27)
    stw      r25, 0x40(r27)
.L_801F2770:
    stw      r18, 0x38(r27)
    li       r0, 0x0
    cmpwi    r20, 0x0
    stw      r0, 0x44(r27)
    bne      .L_801F27B8
    xoris    r3, r19, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc(r1)
    lfd      f1, "@237"@sda21(r0)
    stw      r0, 0x8(r1)
    lfs      f2, lbl_8065FEC0@sda21(r0)
    lfd      f0, 0x8(r1)
    fsubs    f0, f0, f1
    fmuls    f0, f2, f0
    stfs     f0, 0x4c(r27)
    lfs      f0, 0x4c(r27)
    stfs     f0, 0x54(r27)
    b        .L_801F2818
.L_801F27B8:
    lwz      r0, sSynthDecibelFloor@sda21(r0)
    slwi     r18, r19, 16
    cmpw     r18, r0
    bgt      .L_801F27D0
    lfs      f0, "@234"@sda21(r0)
    b        .L_801F280C
.L_801F27D0:
    cmpwi    r18, 0x0
    blt      .L_801F27E0
    lfs      f0, "@223"@sda21(r0)
    b        .L_801F280C
.L_801F27E0:
    xoris    r3, r18, 0x8000
    lis      r0, 0x4330
    stw      r3, 0xc(r1)
    lfd      f1, "@237"@sda21(r0)
    stw      r0, 0x8(r1)
    lfd      f2, "@235"@sda21(r0)
    lfd      f0, 0x8(r1)
    fsub     f0, f0, f1
    fmul     f1, f2, f0
    bl       exp
    frsp     f0, f1
.L_801F280C:
    stfs     f0, 0x4c(r27)
    stw      r18, 0x54(r27)
    stw      r18, 0x58(r27)
.L_801F2818:
    stw      r20, 0x50(r27)
    li       r0, 0x0
    cmpwi    r26, 0x0
    stw      r0, 0x5c(r27)
    stb      r21, 0x64(r27)
    stb      r22, 0x65(r27)
    stb      r23, 0x66(r27)
    stb      r24, 0x67(r27)
    stfs     f29, 0x68(r27)
    stfs     f30, 0x6c(r27)
    stfs     f31, 0x70(r27)
    beq      .L_801F2890
    lis      r4, dropcallback@ha
    li       r3, 0x1f
    addi     r4, r4, dropcallback@l
    li       r5, 0x0
    bl       AXAcquireVoice
    lwz      r7, 0x18(r3)
    lis      r4, sSynthSfxNodes@ha
    addi     r5, r4, sSynthSfxNodes@l
    li       r0, 0x1
    mulli    r6, r7, 0x98
    li       r4, -0x2
    add      r5, r5, r6
    stw      r4, 0x0(r5)
    sth      r29, 0x14(r5)
    sth      r7, 0x14(r27)
    stb      r0, 0xa(r5)
    stw      r3, 0xc(r5)
    b        .L_801F2898
.L_801F2890:
    li       r0, -0x1
    sth      r0, 0x14(r27)
.L_801F2898:
    lis      r3, HSD_SynthPStreamStartCallback@ha
    lwz      r4, 0x0(r27)
    addi     r3, r3, HSD_SynthPStreamStartCallback@l
    bl       AcquireDvdStreamingSlot
    lwz      r4, 0x30(r27)
    stw      r3, 0xc(r4)
    mr       r3, r28
    bl       OSRestoreInterrupts
    lwz      r3, 0x0(r27)
.L_801F28BC:
    psq_l    f31, 0x98(r1), 0, qr0
    lfd      f31, 0x90(r1)
    psq_l    f30, 0x88(r1), 0, qr0
    lfd      f30, 0x80(r1)
    psq_l    f29, 0x78(r1), 0, qr0
    lfd      f29, 0x70(r1)
    psq_l    f28, 0x68(r1), 0, qr0
    lfd      f28, 0x60(r1)
    psq_l    f27, 0x58(r1), 0, qr0
    addi     r11, r1, 0x50
    lfd      f27, 0x50(r1)
    bl       _restgpr_18
    lwz      r0, 0xa4(r1)
    mtlr     r0
    addi     r1, r1, 0xa0
    blr
*/

/**
 * @note Address: 0x801F29A8
 * @note Size: 0x468
 */
// HSD_SynthInit
/*
    stwu     r1, -0x60(r1)
    mflr     r0
    stw      r0, 0x64(r1)
    stfd     f31, 0x50(r1)
    psq_st   f31, 0x58(r1), 0, qr0
    stfd     f30, 0x40(r1)
    psq_st   f30, 0x48(r1), 0, qr0
    addi     r11, r1, 0x40
    bl       _savegpr_27
    lis      r7, sSynthHeapMessageTable@ha
    lis      r6, hsd_SynthSFXLoadBuf@ha
    mr       r29, r3
    mr       r30, r4
    mr       r27, r5
    addi     r28, r7, sSynthHeapMessageTable@l
    addi     r31, r6, hsd_SynthSFXLoadBuf@l
    bl       fn_80176F50
    cmpwi    r30, 0x1
    li       r0, 0x0
    blt      .L_801F2A04
    cmpwi    r30, 0x4
    bgt      .L_801F2A04
    li       r0, 0x1
.L_801F2A04:
    cmpwi    r0, 0x0
    bne      .L_801F2A28
    addi     r3, r28, 0x860
    crclr    cr1eq
    bl       OSReport
    addi     r5, r28, 0x888
    li       r3, sPathSynthC@sda21
    li       r4, 0xfdc
    bl       __assert
.L_801F2A28:
    cmpwi    r27, 0x1
    li       r0, 0x0
    blt      .L_801F2A40
    cmpwi    r27, 0x3
    bgt      .L_801F2A40
    li       r0, 0x1
.L_801F2A40:
    cmpwi    r0, 0x0
    bne      .L_801F2A64
    addi     r3, r28, 0x8b8
    crclr    cr1eq
    bl       OSReport
    addi     r5, r28, 0x8e4
    li       r3, sPathSynthC@sda21
    li       r4, 0xfdd
    bl       __assert
.L_801F2A64:
    mr       r3, r30
    mr       r4, r27
    bl       SetDvdStreamingLimits
    addi     r0, r27, 0x1
    li       r3, 0x0
    mullw    r4, r30, r0
    addi     r0, r4, 0x1
    stw      r0, sSynthStreamBufferCount@sda21(r0)
    bl       AISetDSPSampleRate
    li       r3, 0x500
    bl       ARAlloc
    mr       r5, r3
    li       r3, 0x0
    stw      r5, lbl_8065BBE4@sda21(r0)
    li       r4, 0x0
    li       r6, 0x500
    li       r7, 0x3
    li       r8, 0x0
    li       r9, 0x0
    li       r10, 0x0
    bl       HSD_DevComRequest
    lwz      r0, lbl_8065BBE4@sda21(r0)
    mr       r3, r29
    slwi     r0, r0, 1
    stw      r0, lbl_8065BBE4@sda21(r0)
    bl       ARAlloc
    lis      r4, HSD_SynthCallback@ha
    stw      r3, 0x3a0(r31)
    addi     r3, r4, HSD_SynthCallback@l
    bl       fn_801788FC
    lfs      f1, sSynthStreamVolume@sda21(r0)
    li       r0, 0xff
    lfs      f0, lbl_8065FF20@sda21(r0)
    stw      r0, sSynthStreamVolumeScale@sda21(r0)
    fmuls    f0, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x8(r1)
    lwz      r3, 0xc(r1)
    bl       AISetStreamVolLeft
    lwz      r3, sSynthStreamVolumeScale@sda21(r0)
    lis      r0, 0x4330
    stw      r0, 0x10(r1)
    xoris    r0, r3, 0x8000
    lfd      f1, lbl_8065FEB0@sda21(r0)
    stw      r0, 0x14(r1)
    lfs      f2, sSynthStreamVolume@sda21(r0)
    lfd      f0, 0x10(r1)
    fsubs    f0, f0, f1
    fmuls    f0, f2, f0
    fctiwz   f0, f0
    stfd     f0, 0x18(r1)
    lwz      r3, 0x1c(r1)
    bl       AISetStreamVolRight
    lfs      f0, lbl_8065FE94@sda21(r0)
    addi     r4, r31, 0x33c0
    li       r3, 0x0
    li       r0, 0x4
    stfs     f0, 0x33c0(r31)
    addi     r5, r31, 0x33d8
    stfs     f0, 0x8(r4)
    stw      r3, 0x10(r4)
    mtctr    r0
.L_801F2B5C:
    stfs     f0, 0x0(r5)
    li       r0, 0x0
    stfs     f0, 0x8(r5)
    stw      r0, 0x10(r5)
    stfs     f0, 0x18(r5)
    stfs     f0, 0x20(r5)
    stw      r0, 0x28(r5)
    stfs     f0, 0x30(r5)
    stfs     f0, 0x38(r5)
    stw      r0, 0x40(r5)
    stfs     f0, 0x48(r5)
    stfs     f0, 0x50(r5)
    stw      r0, 0x58(r5)
    stfs     f0, 0x60(r5)
    stfs     f0, 0x68(r5)
    stw      r0, 0x70(r5)
    stfs     f0, 0x78(r5)
    stfs     f0, 0x80(r5)
    stw      r0, 0x88(r5)
    stfs     f0, 0x90(r5)
    stfs     f0, 0x98(r5)
    stw      r0, 0xa0(r5)
    stfs     f0, 0xa8(r5)
    stfs     f0, 0xb0(r5)
    stw      r0, 0xb8(r5)
    stfs     f0, 0xc0(r5)
    stfs     f0, 0xc8(r5)
    stw      r0, 0xd0(r5)
    stfs     f0, 0xd8(r5)
    stfs     f0, 0xe0(r5)
    stw      r0, 0xe8(r5)
    stfs     f0, 0xf0(r5)
    stfs     f0, 0xf8(r5)
    stw      r0, 0x100(r5)
    stfs     f0, 0x108(r5)
    stfs     f0, 0x110(r5)
    stw      r0, 0x118(r5)
    stfs     f0, 0x120(r5)
    stfs     f0, 0x128(r5)
    stw      r0, 0x130(r5)
    stfs     f0, 0x138(r5)
    stfs     f0, 0x140(r5)
    stw      r0, 0x148(r5)
    stfs     f0, 0x150(r5)
    stfs     f0, 0x158(r5)
    stw      r0, 0x160(r5)
    stfs     f0, 0x168(r5)
    stfs     f0, 0x170(r5)
    stw      r0, 0x178(r5)
    addi     r5, r5, 0x180
    bdnz     .L_801F2B5C
    lwz      r3, 0x3a0(r31)
    lwz      r0, sSynthStreamBufferCount@sda21(r0)
    add      r3, r3, r29
    stw      r3, sSynthSfxBankHeapEnd@sda21(r0)
    slwi     r3, r0, 16
    bl       ARAlloc
    stw      r3, sSynthStreamBufferAramBase@sda21(r0)
    bl       OSGetSoundMode
    stw      r3, sSynthSoundMode@sda21(r0)
    addi     r27, r31, 0x2ec0
    lfs      f30, lbl_8065FEB8@sda21(r0)
    li       r28, 0x0
    lfd      f31, lbl_8065FEB0@sda21(r0)
    lis      r29, 0x4330
.L_801F2C60:
    xoris    r0, r28, 0x8000
    stw      r29, 0x18(r1)
    stw      r0, 0x1c(r1)
    lfd      f0, 0x18(r1)
    fsubs    f0, f0, f31
    fmuls    f1, f30, f0
    bl       sqrt
    frsp     f0, f1
    addi     r28, r28, 0x1
    cmpwi    r28, 0xfe
    stfs     f0, 0x0(r27)
    addi     r27, r27, 0x4
    ble      .L_801F2C60
    addi     r3, r31, 0x2ec0
    cmpwi    r30, 0x0
    lfs      f0, 0x3f8(r3)
    li       r6, 0x0
    stfs     f0, 0x3fc(r3)
    ble      .L_801F2D54
    cmpwi    r30, 0x8
    subi     r3, r30, 0x8
    ble      .L_801F2D20
    addi     r0, r3, 0x7
    addi     r4, r31, 0x39d8
    srwi     r0, r0, 3
    lwz      r5, sSynthFreePStreams@sda21(r0)
    mtctr    r0
    cmpwi    r3, 0x0
    ble      .L_801F2D1C
.L_801F2CD4:
    stw      r5, 0x0(r4)
    addi     r3, r4, 0x2c
    addi     r10, r4, 0x58
    addi     r9, r4, 0x84
    stw      r4, 0x2c(r4)
    addi     r8, r4, 0xb0
    addi     r7, r4, 0xdc
    addi     r0, r4, 0x108
    stw      r3, 0x58(r4)
    addi     r5, r4, 0x134
    addi     r6, r6, 0x8
    stw      r10, 0x84(r4)
    stw      r9, 0xb0(r4)
    stw      r8, 0xdc(r4)
    stw      r7, 0x108(r4)
    stw      r0, 0x134(r4)
    addi     r4, r4, 0x160
    bdnz     .L_801F2CD4
.L_801F2D1C:
    stw      r5, sSynthFreePStreams@sda21(r0)
.L_801F2D20:
    mulli    r3, r6, 0x2c
    addi     r5, r31, 0x39d8
    subf     r0, r6, r30
    lwz      r4, sSynthFreePStreams@sda21(r0)
    add      r5, r5, r3
    mtctr    r0
    cmpw     r6, r30
    bge      .L_801F2D50
.L_801F2D40:
    stw      r4, 0x0(r5)
    mr       r4, r5
    addi     r5, r5, 0x2c
    bdnz     .L_801F2D40
.L_801F2D50:
    stw      r4, sSynthFreePStreams@sda21(r0)
.L_801F2D54:
    lwz      r3, sSynthStreamBufferCount@sda21(r0)
    li       r7, 0x0
    cmpwi    r3, 0x0
    ble      .L_801F2DE8
    cmpwi    r3, 0x8
    subi     r4, r3, 0x8
    ble      .L_801F2DB8
    addi     r0, r4, 0x7
    addi     r5, r31, 0x424
    srwi     r0, r0, 3
    li       r3, -0x1
    mtctr    r0
    cmpwi    r4, 0x0
    ble      .L_801F2DB8
.L_801F2D8C:
    stw      r3, 0x0(r5)
    addi     r7, r7, 0x8
    stw      r3, 0x4(r5)
    stw      r3, 0x8(r5)
    stw      r3, 0xc(r5)
    stw      r3, 0x10(r5)
    stw      r3, 0x14(r5)
    stw      r3, 0x18(r5)
    stw      r3, 0x1c(r5)
    addi     r5, r5, 0x20
    bdnz     .L_801F2D8C
.L_801F2DB8:
    lwz      r6, sSynthStreamBufferCount@sda21(r0)
    slwi     r4, r7, 2
    addi     r5, r31, 0x424
    li       r3, -0x1
    subf     r0, r7, r6
    add      r5, r5, r4
    mtctr    r0
    cmpw     r7, r6
    bge      .L_801F2DE8
.L_801F2DDC:
    stw      r3, 0x0(r5)
    addi     r5, r5, 0x4
    bdnz     .L_801F2DDC
.L_801F2DE8:
    psq_l    f31, 0x58(r1), 0, qr0
    lfd      f31, 0x50(r1)
    psq_l    f30, 0x48(r1), 0, qr0
    addi     r11, r1, 0x40
    lfd      f30, 0x40(r1)
    bl       _restgpr_27
    lwz      r0, 0x64(r1)
    mtlr     r0
    addi     r1, r1, 0x60
    blr
*/
