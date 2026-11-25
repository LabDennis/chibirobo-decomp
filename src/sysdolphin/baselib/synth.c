#include <dolphin/os.h>
#include <math.h>
#include <sdk/dolphin/ax.h>
#include <sdk/dolphin/os/OSRtc.h>
#include <sdk/msl/c/climits>
#include <sysdolphin/synth.h>
#include <sysdolphin/synth_bank.h>
#include <sysdolphin/synth_internal.h>

s32 sSynthPendingVoiceFreeCount = 0;
void* (*sSynthMallocProc)(s32) = NULL;
void (*sSynthFreeProc)(void*) = NULL;
s32 hsd_SynthSFXBankNum = 0;
volatile s32 sSynthSfxLoadQueueLength = 0;
BOOL sSynthSfxLoadCancelled = FALSE;
void (*sSynthDriverInactivatedCallback)(s32) = NULL;
void (*sSynthDriverMasterClockCallback)(u32) = NULL;
void (*sSynthDriverPauseCallback)(s32) = NULL;
void (*sSynthPreFrameCallback)(void) = NULL; /* driver pre-frame hook */
void (*sSynthPostFrameCallback)(void) = NULL; /* driver post-frame hook */
HSD_SynthSFXNode* sSynthPendingVolumeUpdates = NULL; /* vol-update pending list head */
HSD_SynthSFXNode* sSynthPendingMixUpdates = NULL; /* mix-update pending list head */
HSD_SynthSFXNode* sSynthPendingRearPitchUpdates = NULL; /* rear-link update list head */
static s32 sSynthSfxIdBase = 0;
s32 sSynthDriverFrame = 0;
s32 sSynthSoundMode = 0; /* sound mode */
// DevCom requests for the ARAM samples and the MRAM metadata.
s32 sSynthLoadRequestIds[2] = { 0, 0 };
HSD_SynthSFXGroup* sSynthSfxLoadDestination = NULL;
u32 sSynthSfxBankHeapEnd = 0;

HSD_SynthSFXFileStart hsd_SynthSFXLoadBuf __attribute__((aligned(32)));
HSD_SynthSFXGroup* sSynthSampleGroupsByBank[SYNTH_SFX_BANK_COUNT];
HSD_SynthSFXLoadRequest sSynthSfxLoadRequests[SYNTH_SFX_LOAD_QUEUE_SIZE];
s32 hsd_SynthSFXBank[SYNTH_SFX_BANK_COUNT];
HSD_SynthSFXEntry* sSynthSfxIdBuckets[SYNTH_SFX_ID_BUCKET_COUNT];
s32 hsd_SynthSFXBankHead[SYNTH_SFX_BANK_COUNT + 1];
AXVPB* sSynthVoicesPendingFree[AX_MAX_VOICES];
HSD_SynthSFXNode sSynthSfxNodes[HSD_SYNTHSFXNODE_MAX];
f32 sSynthSqrtLookup[0x100];
HSD_SynthVolFader sSynthMasterFader;
HSD_SynthVolFader sSynthChannelFaders[SYNTH_CHANNEL_COUNT];

u32 sSynthActiveChannelFaderMasks[SYNTH_CHANNEL_COUNT / SYNTH_FADER_MASK_BITS] = { 0, 0 };
f32 sSynthStreamVolume = 1.0f;

// surround pan curve, front gain by span
f32 sSynthSurroundPanGain[0x100] = {
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

u8 sSynthItdShift[0x80] = {
	0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08, 0x08, 0x09, 0x09,
	0x0A, 0x0A, 0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F, 0x10, 0x10, 0x11, 0x11, 0x12, 0x12, 0x13, 0x13, 0x14, 0x14,
	0x15, 0x15, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x17, 0x18, 0x18, 0x18, 0x18, 0x19, 0x19, 0x19, 0x19, 0x19, 0x1A, 0x1A, 0x1A, 0x1A,
	0x1A, 0x1A, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D,
	0x1D, 0x1D, 0x1D, 0x1D, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
};

HSD_SynthAXSrc sSynthSrc = { { 1, 0, 0, { 0, 0, 0, 0 } } };

s32 sSynthDecibelFloor = -0x03880000;

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

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
u32 HSD_SynthGetSoundMode(void)
{
	return sSynthSoundMode;
}

/**
 * @note Address: 0x801ECD30
 * @note Size: 0xC8
 */
void HSD_SynthSetSoundMode(s32 mode)
{
	s32 i;
	BOOL enabled;

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

// tells the driver the sound is gone and pulls the node off the update lists
static inline void unlinkNode(HSD_SynthSFXNode* node)
{
	HSD_SynthSFXNode** it;
	HSD_SynthSFXNode* rear;

	if (!(node->flags & SYNTH_VOICE_KEY_OFF) && node->link.pstream == NULL && sSynthDriverInactivatedCallback != NULL) {
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
	BOOL enabled = OSDisableInterrupts();

	unlinkNode(node);

	if (node->link.pstream != NULL) {
		releasePStream(node->link.pstream);
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
	BOOL enabled;
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

static inline void linkStereoVoice(AXVPB* primary, AXVPB* secondary, s32 sfxID)
{
	sSynthSfxNodes[secondary->index].sfxID = sfxID;
	sSynthSfxNodes[secondary->index].voice[0] = primary;
}

s32 nextPlaybackId(s32 voiceIndex)
{
	// The low bits identify the AX voice. Keep playback IDs positive.
	if (sSynthSfxIdBase > LONG_MAX - HSD_SYNTHSFXNODE_MAX) {
		sSynthSfxIdBase = HSD_SYNTHSFXNODE_MAX;
	} else {
		sSynthSfxIdBase += HSD_SYNTHSFXNODE_MAX;
	}
	return sSynthSfxIdBase + voiceIndex;
}

/**
 * @note Address: 0x801ED444
 * @note Size: 0x740
 */
s32 StartSynthSoundEffect(s32 sfxID, s32 vol, s32 volMode, s32 vol2, s32 vol2Mode, u8 macroPan, u8 userPan, u8 macroSPan, f32 mixMain,
                          f32 mixAuxA, f32 mixAuxB, u8 userSPan, s32 priority, s32 channel, s32 rear, s32 useItd)
{
	AXVPB* voices[4] = { NULL, NULL, NULL, NULL };
	AXPBVE ve;
	HSD_SynthSFXEntry* entry;
	s32 uid;
	HSD_SynthSFXNode* node;
	s32 i;
	BOOL enabled;
	s32 dup;
	HSD_SynthSFXNode* rearNode;
	s32 j;
	s32 k;
	s32 raw;

	dup = 0;
	if (channel < 0 || channel >= SYNTH_CHANNEL_COUNT) {
		return 0;
	}

	enabled = OSDisableInterrupts();
	entry   = sSynthSfxIdBuckets[sfxID & (ARRAY_SIZE(sSynthSfxIdBuckets) - 1)];
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
			linkStereoVoice(voices[0], voices[1], -1);
		}

		uid                = voices[0]->index;
		node               = &sSynthSfxNodes[uid];
		node->link.pstream = NULL;
		node->sampleId     = sfxID;
		node->flags        = 0;
		node->voice_count  = entry->count;
		node->channel      = channel;
		node->voice[0]     = voices[0];
		node->voice[1]     = voices[1];
		node->freqBase     = entry->rate / SYNTH_SAMPLE_RATE;
		node->pitch[0]     = 1.0f;
		node->pitch[1]     = 1.0f;

		if (volMode == 0) {
			node->u.user_vol[0].volume   = 0.003921569f * vol;
			node->u.user_vol[0].target.f = node->u.user_vol[0].volume;
		} else {
			raw                          = decibelsToDBx(vol);
			node->u.user_vol[0].volume   = HSD_SynthDBxToVol(raw);
			node->u.user_vol[0].target.i = raw;
			node->u.user_vol[0].dbxRaw   = raw;
		}
		node->u.user_vol[0].mode      = volMode;
		node->u.user_vol[0].duration = 0;

		if (vol2Mode == 0) {
			node->u.user_vol[1].volume   = 0.003921569f * vol2;
			node->u.user_vol[1].target.f = node->u.user_vol[1].volume;
		} else {
			raw                          = decibelsToDBx(vol2);
			node->u.user_vol[1].volume   = HSD_SynthDBxToVol(raw);
			node->u.user_vol[1].target.i = raw;
			node->u.user_vol[1].dbxRaw   = raw;
		}
		node->u.user_vol[1].mode      = vol2Mode;
		node->u.user_vol[1].duration = 0;

		node->userPan[0]  = macroPan < 0xFE ? macroPan : 0xFE;
		node->userPan[1]  = userPan < 0xFE ? userPan : 0xFE;
		node->userSPan[0] = macroSPan;
		node->userSPan[1] = userSPan;
		node->mixMain     = mixMain;
		node->mixAuxA     = mixAuxA;
		node->mixAuxB     = mixAuxB;
		node->useItd      = useItd;

		if (rear != 0) {
			AXVPB* rearVoice = voices[2];

			k = rearVoice->index;
			rearNode = &sSynthSfxNodes[k];
			rearNode->sfxID = -2;
			rearNode->rearIndex = uid;
			node->rearIndex = k;
			if (entry->count == 2) {
				linkStereoVoice(rearVoice, voices[3], -3);
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
		                       * (sSynthChannelFaders[node->channel].volume
		                          * (node->channel < SYNTH_MASTER_BYPASS_CHANNEL ? sSynthStreamVolume * sSynthMasterFader.volume : sSynthStreamVolume))));
		ve.currentDelta = 0;
		node->curVol    = ve.currentVolume;
		if (rear != 0) {
			rearNode->curVol = ve.currentVolume;
		}

		setSynthSrcRatio(node->freqBase);

		for (k = 0; k < rear + 1; k++) {
			if (k == 1) {
				rearNode->u.rear.lastVol   = ve.currentVolume;
				rearNode->u.rear.lastDelta = ve.currentDelta;
				rearNode->u.rear.pitch     = node->freqBase;
				HSD_SynthSFXLinkUpdateRearPitch(rearNode);
			}
			for (j = 0; j < entry->count; j++) {
				AXSetVoicePriority(voices[k * 2 + j], priority);
				AXSetVoiceVe(voices[k * 2 + j], &ve);
				AXSetVoiceSrcType(voices[k * 2 + j], 2);
				AXSetVoiceSrc(voices[k * 2 + j], &sSynthSrc.params);
				AXSetVoiceAddr(voices[k * 2 + j], &entry->sample[j].addr);
				AXSetVoiceAdpcm(voices[k * 2 + j], &entry->sample[j].adpcm);
				AXSetVoiceAdpcmLoop(voices[k * 2 + j], &entry->sample[j].adpcmLoop);
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

		node->sfxID = nextPlaybackId(uid);
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
	BOOL enabled = OSDisableInterrupts();

	node = getNode(id);
	if (node != NULL) {
		if (node->flags & SYNTH_VOICE_STREAM_PENDING) {
			HSD_SynthSFXFreeVoice(node);
		} else if (!(node->flags & SYNTH_VOICE_KEY_OFF)) {
			node->flags |= SYNTH_VOICE_KEY_OFF;
			HSD_SynthSFXUpdateVolume(node);
			for (i = 0; i < node->voice_count; i++) {
				AXSetVoicePriority(node->voice[i], 1);
			}
			if (node->link.pstream != NULL) {
				releasePStream(node->link.pstream);
				node->link.pstream = NULL;
			}
		}
	}
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x801EDE08
 * @note Size: 0x8C
 */
void HSD_SynthSFXStopRange(u32 lo, u32 hi)
{
	s32 i;
	BOOL enabled = OSDisableInterrupts();

	for (i = 0; i < HSD_SYNTHSFXNODE_MAX; i++) {
		HSD_SynthSFXNode* node = &sSynthSfxNodes[i];
		if (node->sfxID > 0) {
			u32 addr = getAXCurrentAddress(node->voice[0]);
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
