#ifndef HSD_SYNTH_INTERNAL_H
#define HSD_SYNTH_INTERNAL_H

#include <global.h>
#include <sdk/dolphin/ax.h>
#include <sdk/msl/c/cstddef>

struct HSD_SynthPStream;

// Bus order in the AX mixer parameter block.
enum {
	SYNTH_MIX_MAIN_L,
	SYNTH_MIX_MAIN_R,
	SYNTH_MIX_AUX_A_L,
	SYNTH_MIX_AUX_A_R,
	SYNTH_MIX_AUX_B_L,
	SYNTH_MIX_AUX_B_R,
	SYNTH_MIX_AUX_B_S,
	SYNTH_MIX_MAIN_S,
	SYNTH_MIX_AUX_A_S,
	SYNTH_MIX_BUS_COUNT,
};

typedef struct HSD_SynthMix {
	u16 volume[SYNTH_MIX_BUS_COUNT];
} HSD_SynthMix;

// AX stores each bus volume followed by its signed ramp delta.
typedef union HSD_SynthAXMix {
	AXPBMIX params;
	AXPBVE bus[SYNTH_MIX_BUS_COUNT];
} HSD_SynthAXMix;

STATIC_ASSERT(sizeof(AXPBVE) == 2 * sizeof(u16));
STATIC_ASSERT(offsetof(AXPBVE, currentDelta) == sizeof(u16));
STATIC_ASSERT(sizeof(AXPBVE) * SYNTH_MIX_BUS_COUNT == sizeof(AXPBMIX));

typedef struct HSD_SynthVolFader {
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

STATIC_ASSERT(sizeof(HSD_SynthVolFader) == 0x18);

enum {
	SYNTH_VOICE_KEY_OFF = 1 << 0,
	SYNTH_VOICE_PAUSED = 1 << 1,
	SYNTH_VOICE_PITCH_PAUSED = 1 << 2,
	SYNTH_VOICE_STREAM_PENDING = 1 << 3,
};

// a rear node shares the pool with the front ones, but keeps its own
// state where a front node has its user volumes
typedef struct HSD_SynthSFXNode {
	s32 sfxID;                              // _00
	s32 sampleId;                          // _04
	u8 unk8;                                // _08
	u8 flags;                               // _09
	u8 voice_count;                         // _0A
	u8 channel;                             // _0B
	AXVPB* voice[2];                        // _0C
	s16 rearIndex;                          // _14
	s8 useItd;                              // _16
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
		HSD_SynthVolFader user_vol[2]; // front nodes
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

STATIC_ASSERT(sizeof(HSD_SynthSFXNode) == 0x98);

#define ASSERT_SYNTH(line, cond)             ((cond) ? ((void)0) : __assert(__FILE__, line, #cond))
#define ASSERT_SYNTH_MSG(line, cond, msg)    ((cond) ? ((void)0) : __assert(__FILE__, line, msg))
#define ASSERT_SYNTH_REPORT(line, cond, ...) ((cond) ? (void)0 : (OSReport(__VA_ARGS__), __assert(__FILE__, line, #cond)))

#define USERVOL_NUM          2
#define USERPAN_NUM          2
#define HSD_SYNTHSFXNODE_MAX AX_MAX_VOICES

enum {
	SYNTH_CHANNEL_COUNT = 64,
	SYNTH_MASTER_BYPASS_CHANNEL = SYNTH_CHANNEL_COUNT - 1,
	SYNTH_FADER_MASK_BITS = 8 * sizeof(u32),
	SYNTH_SAMPLES_PER_FRAME = 160,
};

#define SYNTH_SAMPLE_RATE 32000.0f
#define SYNTH_SRC_RATIO_SCALE 65536.0f

// On PowerPC, the first word of AXPBSRC holds the 16.16 ratio.
typedef union HSD_SynthAXSrc {
	AXPBSRC params;
	u32 ratio;
} HSD_SynthAXSrc;

STATIC_ASSERT(sizeof(HSD_SynthAXSrc) == 0x10);
STATIC_ASSERT(offsetof(AXPBSRC, ratioHi) == 0);
STATIC_ASSERT(offsetof(AXPBSRC, ratioLo) == sizeof(u16));

extern s32 sSynthPendingVoiceFreeCount;
extern void (*sSynthDriverInactivatedCallback)(s32);
extern void (*sSynthDriverMasterClockCallback)(u32);
extern void (*sSynthDriverPauseCallback)(s32);
extern void (*sSynthPreFrameCallback)(void);
extern void (*sSynthPostFrameCallback)(void);
extern HSD_SynthSFXNode* sSynthPendingVolumeUpdates;
extern HSD_SynthSFXNode* sSynthPendingMixUpdates;
extern HSD_SynthSFXNode* sSynthPendingRearPitchUpdates;
extern s32 sSynthDriverFrame;
extern s32 sSynthSoundMode;
extern AXVPB* sSynthVoicesPendingFree[AX_MAX_VOICES];
extern HSD_SynthSFXNode sSynthSfxNodes[HSD_SYNTHSFXNODE_MAX];
extern f32 sSynthSqrtLookup[0x100];
extern HSD_SynthVolFader sSynthMasterFader;
extern HSD_SynthVolFader sSynthChannelFaders[SYNTH_CHANNEL_COUNT];
extern u32 sSynthActiveChannelFaderMasks[SYNTH_CHANNEL_COUNT / SYNTH_FADER_MASK_BITS];
extern f32 sSynthStreamVolume;
extern f32 sSynthSurroundPanGain[0x100];
extern HSD_SynthAXSrc sSynthSrc;
extern u8 sSynthItdShift[0x80];

void HSD_SynthSFXFreeVoice(HSD_SynthSFXNode* node);
void HSD_SynthSFXStopRange(u32 lo, u32 hi);
void HSD_SynthSFXUpdatePitch(HSD_SynthSFXNode* node);
void HSD_SynthSFXUpdateVolume(HSD_SynthSFXNode* node);
void HSD_SynthSFXInitMix(HSD_SynthSFXNode* node);
void HSD_SynthSFXUpdateMix(HSD_SynthSFXNode* node, s32 interpolate);
void ApplySynthMixTargetImmediately(HSD_SynthSFXNode* node);
void HSD_SynthSFXLinkUpdateRearPitch(HSD_SynthSFXNode* rear);
void UpdateSynthVolumeFadersAndVoices(void);
void AdvancePendingSynthMixUpdates(void);
void HSD_SynthCallback(void);
void HSD_SynthPStreamMasterClockCallback(void);
void releasePStream(struct HSD_SynthPStream* stream);
void dropcallback(void* dropped);
s32 nextPlaybackId(s32 voiceIndex);

// A DSP address is stored as a high halfword followed by a low halfword.
static inline u32 readAXAddress(const AXAddressPair* address)
{
	return ((u32)address->hi << 16) | address->lo;
}

static inline u32 getAXCurrentAddress(const AXVPB* voice)
{
	return readAXAddress(&voice->pb.addr.currentAddress);
}

static inline HSD_SynthSFXNode* getNode(s32 id)
{
	HSD_SynthSFXNode* node = &sSynthSfxNodes[id & (HSD_SYNTHSFXNODE_MAX - 1)];
	if (id > 0 && node->sfxID == id) {
		return node;
	}
	return NULL;
}

static inline void setSynthSrcRatio(f32 ratio)
{
	sSynthSrc.ratio = SYNTH_SRC_RATIO_SCALE * ratio;
}

static inline s32 decibelsToDBx(s32 db)
{
	// Whole decibels occupy the high half of the signed 16.16 value.
	return (s32)((u32)db << 16);
}

#endif
