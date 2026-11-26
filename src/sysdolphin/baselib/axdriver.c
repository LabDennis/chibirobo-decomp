#include <global.h>
#include <dolphin/os.h>
#include <sdk/dolphin/ax.h>
#include <sdk/dolphin/axfx.h>
#include <sdk/dolphin/dvd.h>
#include <sysdolphin/axdriver.h>
#include <math.h>
#include <string.h>

#define HSD_ASSERT(line, cond) ((cond) ? ((void)0) : __assert(__FILE__, line, #cond))
/* Keep the original diagnostic text when the condition uses different spelling. */
#define HSD_ASSERTMSG(line, cond, msg) ((cond) ? ((void)0) : __assert(__FILE__, line, msg))

#define SMSTATE_MASK   0xC0000000
#define SMSTATE_ACTIVE 0x40000000
#define SMSTATE_SLEEP  0x80000000

#define SMFLAG_PLAY_SAMPLE         0x1U
#define SMFLAG_UPDATE_PRIORITY     0x2U
#define SMFLAG_UPDATE_VOLUME       0x4U
#define SMFLAG_UPDATE_PAN          0x8U
#define SMFLAG_UPDATE_SURROUND_PAN 0x10U
#define SMFLAG_UPDATE_PITCH        0x20U
#define SMFLAG_UPDATE_MIX          0x40U
#define SMFLAG_SLEEP               0x80U
#define SMFLAG_KEY_OFF             0x100U
#define SMFLAG_USER_PARAM_MASK     0x1F0000U
#define SMFLAG_USER_VOLUME_PENDING 0x10000U
#define SMFLAG_USER_PAN_PENDING    0x20000U
#define SMFLAG_USER_PITCH_PENDING  0x80000U
#define SMFLAG_LOOP_FOREVER        0x200000U
#define SMFLAG_PAUSED              0x20000000U

#define AXDRIVER_PIDMASK 0x3F
#define AXDRIVER_TRACK_COUNT 64
#define AXDRIVER_TRACK_MASK_BITS 32
#define AXDRIVER_VOLUME_MAX 255
#define AXDRIVER_PAN_CENTER 127
#define AXDRIVER_PAN_MAX (2 * AXDRIVER_PAN_CENTER)
#define AXDRIVER_SURROUND_PAN_MAX 255
#define AXDRIVER_DB_FRACTION_BITS 16
#define AXDRIVER_AUX_SEND_MAX 255
#define AXDRIVER_AUX_GAIN_SCALE (AXDRIVER_AUX_SEND_MAX * AXDRIVER_AUX_SEND_MAX)
#define AXDRIVER_MIN_PITCH_CENTS (-10800)
#define AXDRIVER_MAX_PITCH_CENTS 2400

#define MIN2(x, y)           ((x) < (y) ? (x) : (y))
#define MAX2(x, y)           ((y) > (x) ? (y) : (x))
#define CLAMP(min, val, max) MAX2(MIN2(val, max), min)

typedef struct HSD_SM {
	struct HSD_SM* prev;    // _00
	struct HSD_SM* next;    // _04
	u32 flags;              // _08
	s32 voice_id;           // _0C
	s32 pid;                // _10
	s32 sound_id;           // _14
	u16 sample_id;          // _18
	u8 group;               // _1A
	u8 priority;            // _1B
	u8 macro_priority;      // _1C
	u8 priority_offset;     // _1D
	u8 macro_volume_db;     // _1E
	u8 user_volume_db;      // _1F
	s16 macro_volume;       // _20
	s16 user_volume;        // _22
	u16 volume_fade_frames; // _24
	u8 macro_pan;           // _26
	u8 user_pan;            // _27
	s16 pan_angle;          // _28
	u8 use_rear;            // _2A
	u8 use_itd;             // _2B
	u8 macro_surround_pan;  // _2C
	u8 user_surround_pan;   // _2D
	u16 sound_limit;        // _2E
	s16 macro_pitch;        // _30
	s16 user_pitch;         // _32
	u8 aux_send[2];         // _34
	u8 aux_a_scale;         // _36
	u8 aux_b_scale;         // _37
	u8 _38;                 // _38
	u8 track;               // _39
	u16 loop_count;         // _3A
	const u32* command;     // _3C
	s32 next_tick;          // _40
} HSD_SM;

STATIC_ASSERT(sizeof(HSD_SM) == 0x44);

static inline f32 AXDriverPitchRatio(s32 cents)
{
	return pow(2.0, (f32)cents / 1200.0f);
}

typedef union HSD_SoundCommandRef {
	u32 offset;
	const u32* commands;
} HSD_SoundCommandRef;

STATIC_ASSERT(sizeof(HSD_SoundCommandRef) == sizeof(u32));

static HSD_SM* pvoices[AXDRIVER_PIDMASK + 1];
static u8 sTrackAuxSends[2][AXDRIVER_TRACK_COUNT];
static AXFX_REVERBHI sAuxReverbHi[2];
static AXFX_REVERBSTD sAuxReverbStd[2];
static AXFX_CHORUS sAuxChorus[2];
static AXFX_DELAY sAuxDelay[2];

typedef enum AXDriverAuxEffect {
	AXDRIVER_AUX_NONE,
	AXDRIVER_AUX_REVERB_HI,
	AXDRIVER_AUX_REVERB_STD,
	AXDRIVER_AUX_CHORUS,
	AXDRIVER_AUX_DELAY,
} AXDriverAuxEffect;

enum {
	AXDRIVER_AUX_A_SEND_LOCK_BIT = 0,
	AXDRIVER_AUX_B_SEND_LOCK_BIT = 1,
	AXDRIVER_AUX_A_EFFECT_SHIFT = 8,
	AXDRIVER_AUX_B_EFFECT_SHIFT = 12,
	AXDRIVER_AUX_EFFECT_MASK = 0xF,
};

u32 sAXAuxConfig = 1U << AXDRIVER_AUX_B_SEND_LOCK_BIT;
s32 sAXFrontRightAngle = 30;
s32 sAXRearRightAngle = 120;
s32 sAXRearLeftAngle = 240;
s32 sAXFrontLeftAngle = 330;

/* CodeWarrior emits these uninitialized globals in reverse declaration order. */
s32 sAXMaxVVoiceIndex;
s32 sAXVVoiceIndexBits;
s32 sAXVVoiceIndexMask;
u32 vidhigh;
HSD_SM* sAXVVoicePool;
s32 sAXDriverTick;
HSD_SM* sAXFreeVVoices;
HSD_SM* sAXActiveVVoices;
s32 lbl_8065BB14;
u32* lbl_8065BB10;
s32 lbl_8065BB0C;
u32* lbl_8065BB08;
s32 sSoundBankCount;
const u32* sSoundBankFirstCommandIndex;
s32 sSoundCommandStreamCount;
HSD_SoundCommandRef* sSoundCommandStreams;
s32 lbl_8065BAF4;
u32* lbl_8065BAF0;
s32 sAXActiveSynthSfxCount;
// The pause mask prevents new starts on tracks 0 through 31.
u32 sAXPausedTrackMask;
s32 sAXActiveVVoiceCount;
u32 axfxmaxsize;
s32 sAXMaxGroupIndex;
u16* sAXGroupVVoiceLimits;
u32 axfxallocsize;
u8* sAXEffectHeapBase;

/**
 * @note Address: 0x801E60F8
 * @note Size: 0x60
 */
void* AXDriverAlloc(u32 size)
{
	void* ptr = &sAXEffectHeapBase[axfxallocsize];

	axfxallocsize += size;
	HSD_ASSERTMSG(74, axfxallocsize < axfxmaxsize, "axfxallocsize < axfxmaxsize");
	return ptr;
}

/**
 * @note Address: 0x801E6158
 * @note Size: 0x4
 */
void AXDriverFree(void* ptr)
{
}

static void AXDriverUnlink(HSD_SM* v, HSD_SM** head)
{
	HSD_SM* p;
	HSD_SM* n;

	if (v != NULL) {
		p       = v->prev;
		n       = v->next;
		v->next = NULL;
		v->prev = NULL;
		if (p != NULL) {
			p->next = n;
		}
		if (n != NULL) {
			n->prev = p;
		}
		if (*head == v) {
			*head = n;
		}
		HSD_ASSERT(109, *head != v);
	}
}

// append at the tail of the voice list
static void AXDriverLink(HSD_SM* v, HSD_SM** head)
{
	HSD_SM* p;

	if (v == NULL) {
		return;
	}
	HSD_ASSERT(118, *head != v);
	v->next = NULL;
	if (*head == NULL) {
		v->prev = NULL;
		*head   = v;
	} else {
		for (p = *head; p->next != NULL; p = p->next) { }
		p->next = v;
		v->prev = p;
	}
}

static inline BOOL AXDriverKeyOff(HSD_SM* v)
{
	s32 pid;
	u32 state;

	if (v == NULL) {
		return FALSE;
	}
	state = v->flags & SMSTATE_MASK;
	HSD_ASSERTMSG(149, state == SMSTATE_ACTIVE || state == SMSTATE_SLEEP,
	              "(v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE || (v->flags&SMSTATE_MASK) == SMSTATE_SLEEP");
	pid = v->pid;
	if (pid != -1) {
		pvoices[pid & AXDRIVER_PIDMASK] = NULL;
		v->pid                          = -1;
		HSD_SynthSFXKeyOff(pid);
		sAXActiveSynthSfxCount--;
	}
	v->flags &= ~SMFLAG_PAUSED;
	v->flags &= ~SMSTATE_MASK;
	return TRUE;
}

/**
 * @note Address: 0x801E615C
 * @note Size: 0x14C
 */
BOOL HSD_AudioSFXKeyOff(s32 vid)
{
	BOOL result;
	s32 idx;
	BOOL enabled;
	HSD_SM* v;

	idx = vid & sAXVVoiceIndexMask;
	if (vid < 0 || idx > sAXMaxVVoiceIndex) {
		return FALSE;
	}
	enabled = OSDisableInterrupts();
	v       = &sAXVVoicePool[idx];
	if (v->voice_id != vid || !(v->flags & SMSTATE_MASK)) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}
	result = AXDriverKeyOff(v);
	OSRestoreInterrupts(enabled);
	return result;
}

/**
 * @note Address: 0x801E62A8
 * @note Size: 0x11C
 */
BOOL AXDriverKeyOffSfx(s32 sfx)
{
	BOOL enabled;
	HSD_SM* v;

	if (sfx >= 0) {
		enabled = OSDisableInterrupts();
		for (v = sAXActiveVVoices; v != NULL; v = v->next) {
			if (v->sound_id == sfx && (v->flags & SMSTATE_MASK)) {
				AXDriverKeyOff(v);
			}
		}
		OSRestoreInterrupts(enabled);
		return TRUE;
	}
	return FALSE;
}

#if defined(VERSION_PAL)
/**
 * @note Address: PAL 0x801E8CC0
 * @note Size: 0xF8
 */
// Europe's reset keys off every sound instead of stopping the AX library
BOOL HSD_AudioSFXKeyOffAll(void)
{
	BOOL enabled;
	HSD_SM* v;

	enabled = OSDisableInterrupts();
	for (v = sAXActiveVVoices; v != NULL; v = v->next) {
		if (v->flags & SMSTATE_MASK) {
			AXDriverKeyOff(v);
		}
	}
	OSRestoreInterrupts(enabled);
	return TRUE;
}
#endif

/**
 * @note Address: 0x801E63C4
 * @note Size: 0x124
 */
BOOL HSD_AudioSFXKeyOffTrack(s32 track)
{
	BOOL enabled;
	HSD_SM* v;

	if (track >= 0 && track <= AXDRIVER_TRACK_COUNT - 1) {
		enabled = OSDisableInterrupts();
		for (v = sAXActiveVVoices; v != NULL; v = v->next) {
			if ((v->flags & SMSTATE_MASK) && v->track == track) {
				AXDriverKeyOff(v);
			}
		}
		OSRestoreInterrupts(enabled);
		return TRUE;
	}
	return FALSE;
}

// too many voices in this group, kick the lowest priority one or an equal one
static inline BOOL AXDriverCheckGroup(HSD_SM* v)
{
	HSD_SM* p;
	HSD_SM* low  = v;
	HSD_SM* same = v;
	s32 count    = 0;

	for (p = sAXActiveVVoices; p != v; p = p->next) {
		if ((p->flags & SMSTATE_MASK) && p->group == v->group) {
			count++;
			if (p->priority < low->priority) {
				low = p;
			}
			if (p->priority == same->priority) {
				same = p;
			}
		}
	}
	if (count >= sAXGroupVVoiceLimits[v->group]) {
		if (low != v) {
			AXDriverKeyOff(low);
			return TRUE;
		}
		if (same != v) {
			AXDriverKeyOff(same);
			return TRUE;
		}
		return FALSE;
	}
	return TRUE;
}

static inline BOOL AXDriverCheckSfxLimit(HSD_SM* v)
{
	HSD_SM* p;
	s32 count = 0;

	for (p = sAXActiveVVoices; p != NULL; p = p->next) {
		if ((p->flags & SMSTATE_MASK) && p->sound_id == v->sound_id) {
			if (++count > v->sound_limit) {
				return FALSE;
			}
		}
	}
	return TRUE;
}

/* Synth volume uses fixed-point tenths of a decibel. Sound commands omit the fractional bits. */
static s32 AXDriverDBToVol(s16 db)
{
	return (f32)AXDRIVER_VOLUME_MAX * HSD_SynthDBxToVol(db * (1 << AXDRIVER_DB_FRACTION_BITS));
}

static s32 AXDriverVolToDB(s16 vol)
{
	return HSD_SynthVolToDBx((1.0f / AXDRIVER_VOLUME_MAX) * vol) >> AXDRIVER_DB_FRACTION_BITS;
}

/**
 * @note Address: 0x801E64E8
 * @note Size: 0x6C4
 */
s32 StartAxDriverSynthVoice(HSD_SM* v)
{
	f32 send_a;
	f32 send_b;
	f32 mix_a;
	f32 dry_a;
	f32 sqrt_b;
	f32 dry_b;
	f32 mix_b;

	if (v->group != 0 && !AXDriverCheckGroup(v)) {
		return -1;
	}
	if (v->sound_limit != 0 && !AXDriverCheckSfxLimit(v)) {
		return -1;
	}
	send_a = (f32)(v->aux_a_scale * v->aux_send[0]) / AXDRIVER_AUX_GAIN_SCALE;
	send_b = (f32)(v->aux_b_scale * v->aux_send[1]) / AXDRIVER_AUX_GAIN_SCALE;
	mix_a  = msl_sqrtf(send_a);
	dry_a  = msl_sqrtf(1.0f - send_a);
	sqrt_b = msl_sqrtf(send_b);
	mix_b  = dry_a * sqrt_b;
	dry_b  = msl_sqrtf(1.0f - send_b);
	return StartSynthSoundEffect(v->sample_id, v->macro_volume, v->macro_volume_db, v->user_volume, v->user_volume_db, v->macro_pan,
	                             v->user_pan, v->macro_surround_pan, dry_a * dry_b, mix_a, mix_b, v->user_surround_pan, v->priority,
	                             v->track, v->use_rear, v->use_itd);
}

/**
 * @note Address: 0x801E6BAC
 * @note Size: 0x78C
 */
void AXDriverExec(HSD_SM* v)
{
	s32 i;
	u32 flag;

	for (i = 0; i <= 8; i++) {
		flag = 1 << i;
		if (v->flags & flag) {
			switch (flag) {
			case SMFLAG_PLAY_SAMPLE:
				if ((v->pid = StartAxDriverSynthVoice(v)) != -1) {
					HSD_ASSERTMSG(495, pvoices[v->pid & AXDRIVER_PIDMASK] == NULL, "pvoices[v->pid&AXDRIVER_PIDMASK] == NULL");
					pvoices[v->pid & AXDRIVER_PIDMASK] = v;
					HSD_SynthSetVolFader(v->pid, v->user_volume, v->volume_fade_frames, 1, v->user_volume_db);
					// The new synth voice already has these settings.
					v->flags &= ~(SMFLAG_UPDATE_PRIORITY | SMFLAG_UPDATE_VOLUME | SMFLAG_UPDATE_PAN | SMFLAG_UPDATE_SURROUND_PAN
					              | (SMFLAG_USER_PARAM_MASK & ~SMFLAG_USER_PITCH_PENDING));
					if (v->flags & SMFLAG_USER_PITCH_PENDING) {
						HSD_SynthSFXSetPitch(v->pid, 1, AXDriverPitchRatio(v->user_pitch));
						v->flags &= ~SMFLAG_USER_PITCH_PENDING;
					}
					sAXActiveSynthSfxCount++;
				} else {
					AXDriverKeyOff(v);
					return;
				}
				break;
			case SMFLAG_UPDATE_PRIORITY:
				HSD_SynthSFXSetPriority(v->pid, v->priority);
				break;
			case SMFLAG_UPDATE_VOLUME:
				HSD_SynthSetVolFader(v->pid, v->macro_volume, 1, 0, v->macro_volume_db);
				break;
			case SMFLAG_UPDATE_PAN:
				HSD_SynthSFXSetPan(v->pid, v->macro_pan, 0);
				break;
			case SMFLAG_UPDATE_SURROUND_PAN:
				HSD_SynthSFXSetSPan(v->pid, v->macro_surround_pan, 0);
				break;
			case SMFLAG_UPDATE_PITCH:
				HSD_SynthSFXSetPitch(v->pid, 0, AXDriverPitchRatio(v->macro_pitch));
				break;
			case SMFLAG_UPDATE_MIX: {
				f32 send_a = (f32)(v->aux_a_scale * v->aux_send[0]) / AXDRIVER_AUX_GAIN_SCALE;
				f32 send_b = (f32)(v->aux_b_scale * v->aux_send[1]) / AXDRIVER_AUX_GAIN_SCALE;
				f32 mix_a  = msl_sqrtf(send_a);
				f32 dry_a  = msl_sqrtf(1.0f - send_a);
				f32 sqrt_b = msl_sqrtf(send_b);
				f32 mix_b  = dry_a * sqrt_b;
				f32 dry_b  = msl_sqrtf(1.0f - send_b);

				HSD_SynthSFXSetMix(v->pid, dry_a * dry_b, mix_a, mix_b);
				break;
			}
			case SMFLAG_SLEEP:
				v->flags = (v->flags & ~SMSTATE_MASK) | SMSTATE_SLEEP;
				return;
			case SMFLAG_KEY_OFF:
				AXDriverKeyOff(v);
				return;
			}
		}
		v->flags &= ~flag;
	}
}

enum {
	SMCMD_WAIT             = 0,
	SMCMD_PLAY_SAMPLE      = 1,
	SMCMD_LOOP_BEGIN       = 2,
	SMCMD_LOOP_END         = 3,
	SMCMD_SET_PRIORITY     = 4,
	SMCMD_ADD_PRIORITY     = 5,
	SMCMD_SET_VOLUME       = 6,
	SMCMD_ADD_VOLUME       = 7,
	SMCMD_SET_PAN          = 8,
	SMCMD_ADD_PAN          = 9,
	SMCMD_SET_SURROUND_PAN = 10,
	SMCMD_ADD_SURROUND_PAN = 11,
	SMCMD_SET_PITCH        = 12,
	SMCMD_ADD_PITCH        = 13,
	SMCMD_SLEEP            = 14,
	SMCMD_KEY_OFF          = 15,
	SMCMD_SET_AUX_A        = 16,
	SMCMD_ADD_AUX_A        = 17,
	SMCMD_SET_AUX_B        = 18,
	SMCMD_ADD_AUX_B        = 19,
	SMCMD_SET_AUX_A_SCALE  = 20,
	SMCMD_SET_AUX_B_SCALE  = 21,
	SMCMD_SET_SOUND_LIMIT  = 22,
	SMCMD_SET_VOLUME_DB    = 23,
	SMCMD_ADD_VOLUME_DB    = 24,
	SMCMD_SET_PAN_ANGLE    = 25,
	SMCMD_ADD_PAN_ANGLE    = 26,
	SMCMD_SET_REAR_VOICE   = 27,
	SMCMD_SET_ITD          = 28,
};

// The opcode determines which bytes carry the delay.
static inline s32 AXDriverParseWait(s32 type, u32 cmd)
{
	switch (type) {
	case SMCMD_WAIT:
	case SMCMD_SLEEP:
	case SMCMD_KEY_OFF:
		return cmd & 0xFFFFFF;
	case SMCMD_SET_PRIORITY:
	case SMCMD_ADD_PRIORITY:
	case SMCMD_SET_VOLUME:
	case SMCMD_ADD_VOLUME:
	case SMCMD_SET_PAN:
	case SMCMD_ADD_PAN:
	case SMCMD_SET_SURROUND_PAN:
	case SMCMD_ADD_SURROUND_PAN:
	case SMCMD_SET_AUX_A:
	case SMCMD_ADD_AUX_A:
	case SMCMD_SET_AUX_B:
	case SMCMD_ADD_AUX_B:
	case SMCMD_SET_AUX_A_SCALE:
	case SMCMD_SET_AUX_B_SCALE:
	case SMCMD_SET_SOUND_LIMIT:
		return (cmd >> 8) & 0xFFFF;
	case SMCMD_PLAY_SAMPLE:
	case SMCMD_LOOP_BEGIN:
	case SMCMD_LOOP_END:
	case SMCMD_SET_PITCH:
	case SMCMD_ADD_PITCH:
	case SMCMD_SET_VOLUME_DB:
	case SMCMD_ADD_VOLUME_DB:
	case SMCMD_SET_PAN_ANGLE:
	case SMCMD_ADD_PAN_ANGLE:
		return (cmd >> 16) & 0xFF;
	default:
		return 0;
	}
}

static inline void AXDriverUpdatePri(HSD_SM* v)
{
	v->priority = v->macro_priority + v->priority_offset + 2;
	if (v->priority < 2) {
		v->priority = 2;
	} else if (v->priority > 0x1D) {
		v->priority = 0x1D;
	}
}

// pan and surround pan out of a 0 to 359 angle
static inline void AXDriverUpdateAngle(HSD_SM* v)
{
	s32 angle;
	s32 pan;
	s32 span;

	if (v->pan_angle < 0) {
		v->pan_angle += ((359 - v->pan_angle) / 360) * 360;
	} else if (v->pan_angle >= 360) {
		v->pan_angle %= 360;
	}

	angle = v->pan_angle;
	if (angle < sAXFrontRightAngle) {
		pan = angle * AXDRIVER_PAN_CENTER / sAXFrontRightAngle + AXDRIVER_PAN_CENTER;
	} else if (angle <= sAXRearRightAngle) {
		pan = AXDRIVER_PAN_MAX;
	} else if (angle < sAXRearLeftAngle) {
		pan = AXDRIVER_PAN_MAX - (angle - sAXRearRightAngle) * AXDRIVER_PAN_MAX / (sAXRearLeftAngle - sAXRearRightAngle);
	} else if (angle <= sAXFrontLeftAngle) {
		pan = 0;
	} else {
		pan = (angle - sAXFrontLeftAngle) * AXDRIVER_PAN_CENTER / (360 - sAXFrontLeftAngle);
	}
	v->macro_pan = pan;

	if (v->pan_angle <= sAXFrontRightAngle) {
		span = 0;
	} else if (v->pan_angle < sAXRearRightAngle) {
		span = (v->pan_angle - sAXFrontRightAngle) * AXDRIVER_SURROUND_PAN_MAX / (sAXRearRightAngle - sAXFrontRightAngle);
	} else if (v->pan_angle <= sAXRearLeftAngle) {
		span = AXDRIVER_SURROUND_PAN_MAX;
	} else if (v->pan_angle < sAXFrontLeftAngle) {
		span = (sAXFrontLeftAngle - v->pan_angle) * AXDRIVER_SURROUND_PAN_MAX / (sAXFrontLeftAngle - sAXRearLeftAngle);
	} else {
		span = 0;
	}
	v->macro_surround_pan = span;
}

/**
 * @note Address: 0x801E7338
 * @note Size: 0xB4C
 */
void AXDriverInterp(HSD_SM* v, BOOL exec)
{
	u32 cmd;
	u32 type;
	s32 wait;
	s32 lin;
	s32 db;
	s32 cmd_val;

	while (v->next_tick == sAXDriverTick) {
		cmd  = *v->command;
		type = cmd >> 24;
		if (v->flags & (SMFLAG_SLEEP | SMFLAG_KEY_OFF)) {
			AXDriverExec(v);
			return;
		}
		wait = AXDriverParseWait(type, cmd);
		if (wait != 0) {
			if (exec) {
				AXDriverExec(v);
			} else {
				return;
			}
		}
		v->next_tick += wait;
		switch (type) {
		case SMCMD_LOOP_BEGIN:
			v->loop_count = *v->command;
			if (v->loop_count == 0) {
				v->flags |= SMFLAG_LOOP_FOREVER;
			}
			break;
		case SMCMD_LOOP_END:
			if ((v->flags & SMFLAG_LOOP_FOREVER) || v->loop_count != 0) {
				v->command -= *v->command & 0xFFFF;
				v->loop_count--;
			}
			break;
		case SMCMD_PLAY_SAMPLE:
			v->flags |= SMFLAG_PLAY_SAMPLE;
			v->sample_id = *v->command;
			break;
		case SMCMD_SET_PRIORITY:
			v->flags |= SMFLAG_UPDATE_PRIORITY;
			cmd_val           = (s32)(*v->command & 0xFF);
			v->macro_priority = CLAMP(0, cmd_val, 0x1B);
			AXDriverUpdatePri(v);
			break;
		case SMCMD_ADD_PRIORITY:
			v->flags |= SMFLAG_UPDATE_PRIORITY;
			cmd_val           = v->macro_priority + (s8)(u8)*v->command;
			v->macro_priority = CLAMP(0, cmd_val, 0x1B);
			AXDriverUpdatePri(v);
			break;
		case SMCMD_SET_VOLUME:
			v->flags |= SMFLAG_UPDATE_VOLUME;
			v->macro_volume_db = 0;
			v->macro_volume    = (u8)*v->command;
			break;
		case SMCMD_ADD_VOLUME:
			v->flags |= SMFLAG_UPDATE_VOLUME;
			if (v->macro_volume_db == 0) {
				cmd_val         = v->macro_volume + (s8)(u8)*v->command;
				v->macro_volume = (u8)CLAMP(0, cmd_val, AXDRIVER_VOLUME_MAX);
			} else {
				v->macro_volume_db = 0;
				lin                = AXDriverDBToVol(v->macro_volume);
				cmd_val            = (s8)(u8)*v->command + lin;
				v->macro_volume    = (u8)CLAMP(0, cmd_val, AXDRIVER_VOLUME_MAX);
			}
			break;
		case SMCMD_SET_VOLUME_DB:
			v->flags |= SMFLAG_UPDATE_VOLUME;
			v->macro_volume_db = 1;
			cmd_val            = (s16)(u16)*v->command;
			v->macro_volume    = CLAMP(sSynthDecibelFloor >> AXDRIVER_DB_FRACTION_BITS, cmd_val, 0);
			break;
		case SMCMD_ADD_VOLUME_DB:
			v->flags |= SMFLAG_UPDATE_VOLUME;
			if (v->macro_volume_db == 1) {
				cmd_val         = v->macro_volume + (s16)(u16)*v->command;
				v->macro_volume = CLAMP(sSynthDecibelFloor >> AXDRIVER_DB_FRACTION_BITS, cmd_val, 0);
			} else {
				v->macro_volume_db = 1;
				db                 = AXDriverVolToDB(v->macro_volume);
				v->macro_volume    = CLAMP(sSynthDecibelFloor >> AXDRIVER_DB_FRACTION_BITS, (s16)(u16)*v->command + db, 0);
			}
			break;
		case SMCMD_SET_PAN_ANGLE:
			v->flags |= SMFLAG_UPDATE_PAN | SMFLAG_UPDATE_SURROUND_PAN;
			v->pan_angle = *v->command;
			AXDriverUpdateAngle(v);
			break;
		case SMCMD_ADD_PAN_ANGLE:
			v->flags |= SMFLAG_UPDATE_PAN | SMFLAG_UPDATE_SURROUND_PAN;
			v->pan_angle += (s16)(u16)*v->command;
			AXDriverUpdateAngle(v);
			break;
		case SMCMD_SET_PAN:
			v->flags |= SMFLAG_UPDATE_PAN;
			v->macro_pan = *v->command;
			break;
		case SMCMD_ADD_PAN:
			v->flags |= SMFLAG_UPDATE_PAN;
			cmd_val      = v->macro_pan + (s8)(u8)*v->command;
			v->macro_pan = CLAMP(0, cmd_val, AXDRIVER_PAN_MAX);
			break;
		case SMCMD_SET_SURROUND_PAN:
			v->flags |= SMFLAG_UPDATE_SURROUND_PAN;
			v->macro_surround_pan = *v->command;
			break;
		case SMCMD_ADD_SURROUND_PAN:
			v->flags |= SMFLAG_UPDATE_SURROUND_PAN;
			cmd_val               = v->macro_surround_pan + (s8)(u8)*v->command;
			v->macro_surround_pan = CLAMP(0, cmd_val, AXDRIVER_SURROUND_PAN_MAX);
			break;
		case SMCMD_SET_PITCH:
			v->flags |= SMFLAG_UPDATE_PITCH;
			v->macro_pitch = *v->command;
			break;
		case SMCMD_ADD_PITCH:
			v->flags |= SMFLAG_UPDATE_PITCH;
			cmd_val        = v->macro_pitch + (s16)(u16)*v->command;
			v->macro_pitch = CLAMP(AXDRIVER_MIN_PITCH_CENTS, cmd_val, AXDRIVER_MAX_PITCH_CENTS);
			break;
		case SMCMD_SET_AUX_A:
			if (!((sAXAuxConfig >> AXDRIVER_AUX_A_SEND_LOCK_BIT) & 1U)) {
				v->flags |= SMFLAG_UPDATE_MIX;
				v->aux_send[0] = *v->command;
			}
			break;
		case SMCMD_SET_AUX_A_SCALE:
			if (!((sAXAuxConfig >> AXDRIVER_AUX_A_SEND_LOCK_BIT) & 1U)) {
				v->aux_a_scale = *v->command;
			}
			break;
		case SMCMD_SET_AUX_B_SCALE:
			if (!((sAXAuxConfig >> AXDRIVER_AUX_B_SEND_LOCK_BIT) & 1U)) {
				v->aux_b_scale = *v->command;
			}
			break;
		case SMCMD_ADD_AUX_A:
			if (!((sAXAuxConfig >> AXDRIVER_AUX_A_SEND_LOCK_BIT) & 1U)) {
				v->flags |= SMFLAG_UPDATE_MIX;
				cmd_val        = v->aux_send[0] + (s8)(u8)*v->command;
				v->aux_send[0] = CLAMP(0, cmd_val, AXDRIVER_AUX_SEND_MAX);
			}
			break;
		case SMCMD_SET_AUX_B:
			if (!((sAXAuxConfig >> AXDRIVER_AUX_B_SEND_LOCK_BIT) & 1U)) {
				v->flags |= SMFLAG_UPDATE_MIX;
				v->aux_send[1] = *v->command;
			}
			break;
		case SMCMD_ADD_AUX_B:
			if (!((sAXAuxConfig >> AXDRIVER_AUX_B_SEND_LOCK_BIT) & 1U)) {
				v->flags |= SMFLAG_UPDATE_MIX;
				cmd_val        = v->aux_send[1] + (s8)(u8)*v->command;
				v->aux_send[1] = CLAMP(0, cmd_val, AXDRIVER_AUX_SEND_MAX);
			}
			break;
		case SMCMD_SET_SOUND_LIMIT:
			v->sound_limit = (u8)*v->command;
			break;
		case SMCMD_SET_REAR_VOICE:
			v->use_rear = *v->command;
			break;
		case SMCMD_SET_ITD:
			v->use_itd = *v->command;
			break;
		case SMCMD_KEY_OFF:
			v->flags |= SMFLAG_KEY_OFF;
			if (wait == 0) {
				AXDriverExec(v);
			}
			return;
		case SMCMD_SLEEP:
			v->flags |= SMFLAG_SLEEP;
			if (wait == 0) {
				AXDriverExec(v);
			}
			return;
		}
		v->command++;
	}
}

/**
 * @note Address: 0x801E7E84
 * @note Size: 0x208
 */
void AXDriverCallback(u32 frame)
{
	HSD_SM* v;
	HSD_SM* next;

	(void)frame;
	v = sAXActiveVVoices;
	sAXDriverTick++;
	while (v != NULL) {
		next = v->next;
		if (v->next_tick == -1) {
			v->next_tick = sAXDriverTick;
		} else if (v->flags & SMFLAG_PAUSED) {
			v->next_tick++;
		}
		switch (v->flags & SMSTATE_MASK) {
		case SMSTATE_ACTIVE:
			if (v->next_tick == sAXDriverTick) {
				AXDriverInterp(v, TRUE);
			}
			break;
		case 0:
			AXDriverUnlink(v, &sAXActiveVVoices);
			AXDriverLink(v, &sAXFreeVVoices);
			sAXActiveVVoiceCount--;
			break;
		case SMSTATE_SLEEP:
			break;
		default:
			HSD_ASSERT(865, 0);
			break;
		}
		v = next;
	}

	for (v = sAXActiveVVoices; v != NULL; v = next) {
		next = v->next;
		if ((v->flags & SMSTATE_MASK) == SMSTATE_ACTIVE && v->next_tick == sAXDriverTick) {
			AXDriverInterp(v, TRUE);
		}
	}
}

/**
 * @note Address: 0x801E808C
 * @note Size: 0xD4
 */
void AXDriverKillCallback(s32 pid)
{
	s32 idx = pid & AXDRIVER_PIDMASK;
	HSD_SM* v;

	HSD_ASSERT(890, pid > 0);
	v = pvoices[idx];
	HSD_ASSERT(892, v && v->pid == pid);
	pvoices[idx] = NULL;
	v->flags &= ~SMFLAG_PAUSED;
	v->flags &= ~SMSTATE_MASK;
	sAXActiveSynthSfxCount--;
}

/**
 * @note Address: 0x801E8160
 * @note Size: 0xA4
 */
void AXDriverPauseCallback(s32 pid)
{
	HSD_SM* v;
	s32 idx = pid & AXDRIVER_PIDMASK;

	HSD_ASSERT(909, pid > 0);
	v = pvoices[idx];
	HSD_ASSERT(911, v && v->pid == pid);
	v->flags |= SMFLAG_PAUSED;
}

static inline HSD_SM* AXDriverAssignVVoice(void)
{
	HSD_SM* v;
	BOOL enabled;

	if (sAXFreeVVoices == NULL) {
		return NULL;
	}
	enabled = OSDisableInterrupts();
	v       = sAXFreeVVoices;
	AXDriverUnlink(v, &sAXFreeVVoices);
	OSRestoreInterrupts(enabled);
	v->pid       = -1;
	v->next_tick = -1;
	v->flags     = 0;
	return v;
}

static inline u32 AXDriverTrackBit(u32 track)
{
	// Tracks 32 through 63 can pause active voices but have no mask bit.
	return track < AXDRIVER_TRACK_MASK_BITS ? 1U << track : 0;
}

/**
 * @note Address: 0x801E8204
 * @note Size: 0x3A8
 */
static s32 AXDriverStartVVoice(u32 sfx, u8 vol, u8 pan, u8 span, s16 pitch, u8 auxA, u8 auxB, s32 group, s32 track)
{
	HSD_SM* v;
	s32 bank;
	u16 sample;
	s32 command_index;
	BOOL enabled;

	bank   = sfx >> 16;
	sample = (u16)sfx;
	if (sSoundBankCount <= bank) {
		return -1;
	}
	command_index = sSoundBankFirstCommandIndex[bank] + sample;
	if (sSoundCommandStreamCount <= command_index) {
		return -1;
	}
	if (bank < sSoundBankCount - 1 && sSoundBankFirstCommandIndex[bank + 1] <= command_index) {
		return -1;
	}
	if (group < 0 || sAXMaxGroupIndex < group) {
		return -1;
	}
	if (sAXPausedTrackMask & AXDriverTrackBit(track)) {
		return -1;
	}

	v = AXDriverAssignVVoice();
	if (v == NULL) {
		return -1;
	}

	v->sound_id = sfx;
	v->command = sSoundCommandStreams[command_index].commands;
	v->macro_volume       = AXDRIVER_VOLUME_MAX;
	v->user_volume        = vol;
	v->macro_volume_db    = 0;
	v->user_volume_db     = 0;
	v->volume_fade_frames = 1;
	v->macro_pan          = AXDRIVER_PAN_CENTER;
	v->user_pan           = pan;
	v->macro_surround_pan = 0;
	v->user_surround_pan  = span;
	v->macro_pitch        = 0;
	v->user_pitch         = pitch;
	v->pan_angle          = 0;
	v->use_rear           = 1;
	v->use_itd            = 1;
	v->aux_send[0]        = auxA;
	v->aux_send[1]        = auxB;
	v->aux_a_scale        = AXDRIVER_AUX_SEND_MAX;
	v->aux_b_scale        = AXDRIVER_AUX_SEND_MAX;
	v->macro_priority     = 0;
	v->priority_offset    = 0;
	AXDriverUpdatePri(v);
	v->group       = group;
	v->track       = track;
	v->sound_limit = 0;
	v->flags |= SMFLAG_USER_PARAM_MASK;

	if (vidhigh & ~((1U << (32 - sAXVVoiceIndexBits)) - 1)) {
		OSReport("vidhigh exceeds the max value\n");
		HSD_ASSERT(1067, 0);
	}
	v->voice_id = ((u8*)v - (u8*)sAXVVoicePool) / sizeof(HSD_SM) | (vidhigh << sAXVVoiceIndexBits);
	vidhigh++;

	enabled  = OSDisableInterrupts();
	v->flags = (v->flags & ~SMSTATE_MASK) | SMSTATE_ACTIVE;
	AXDriverLink(v, &sAXActiveVVoices);
	sAXActiveVVoiceCount++;
	OSRestoreInterrupts(enabled);
	return v->voice_id;
}

/**
 * @note Address: 0x801E85AC
 * @note Size: 0x44
 */
s32 HSD_AudioSFXStartParam(u32 sfx, u8 vol, u8 pan, s32 group, s32 track)
{
	// Check the track before indexing the auxiliary sends.
	if ((u32)track >= AXDRIVER_TRACK_COUNT) {
		return -1;
	}
	return AXDriverStartVVoice(sfx, vol, pan, 0, 0, sTrackAuxSends[0][track], sTrackAuxSends[1][track], group, track);
}

static inline BOOL AXDriverSetPan(s32 vid, u8 pan)
{
	s32 idx;
	BOOL enabled;
	HSD_SM* v;

	idx = vid & sAXVVoiceIndexMask;
	if (vid < 0 || idx > sAXMaxVVoiceIndex) {
		return FALSE;
	}
	enabled = OSDisableInterrupts();
	v       = &sAXVVoicePool[idx];
	if (v->voice_id != vid || !(v->flags & SMSTATE_MASK)) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}
	if (v->pid != -1) {
		HSD_SynthSFXSetPan(v->pid, CLAMP(0, pan, 0xFE), 1);
	} else {
		HSD_ASSERTMSG(1149, (v->flags & SMSTATE_MASK) == SMSTATE_ACTIVE, "(v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE");
		v->user_pan = pan;
		v->flags |= SMFLAG_USER_PAN_PENDING;
	}
	OSRestoreInterrupts(enabled);
	return TRUE;
}

/**
 * @note Address: 0x801E85F0
 * @note Size: 0x16C
 */
BOOL HSD_AudioSFXSetPanByID(s32 sfx, u8 pan)
{
	BOOL enabled;
	HSD_SM* v;

	if (sfx >= 0) {
		enabled = OSDisableInterrupts();
		for (v = sAXActiveVVoices; v != NULL; v = v->next) {
			if ((v->flags & SMSTATE_MASK) && v->sound_id == sfx) {
				AXDriverSetPan(v->voice_id, pan);
			}
		}
		OSRestoreInterrupts(enabled);
		return TRUE;
	}
	return FALSE;
}

static inline BOOL AXDriverSetVolume(s32 vid, u8 vol, u16 frames)
{
	s32 idx;
	BOOL enabled;
	HSD_SM* v;
	u8 clamped;

	clamped = vol;
	if (clamped > AXDRIVER_VOLUME_MAX) {
		clamped = AXDRIVER_VOLUME_MAX;
	}
	idx = vid & sAXVVoiceIndexMask;
	if (vid < 0 || idx > sAXMaxVVoiceIndex) {
		return FALSE;
	}
	enabled = OSDisableInterrupts();
	v       = &sAXVVoicePool[idx];
	if (v->voice_id != vid || !(v->flags & SMSTATE_MASK)) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}
	if (v->pid != -1) {
		HSD_SynthSetVolFader(v->pid, clamped, frames, 1, 0);
	} else {
		HSD_ASSERTMSG(1474, (v->flags & SMSTATE_MASK) == SMSTATE_ACTIVE, "(v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE");
		v->user_volume        = clamped;
		v->user_volume_db     = 0;
		v->volume_fade_frames = frames;
		v->flags |= SMFLAG_USER_VOLUME_PENDING;
	}
	OSRestoreInterrupts(enabled);
	return TRUE;
}

/**
 * @note Address: 0x801E875C
 * @note Size: 0x164
 */
BOOL HSD_AudioSFXSetVolumeByID(s32 sfx, u8 vol, u16 frames)
{
	BOOL enabled;
	HSD_SM* v;

	if (sfx >= 0) {
		enabled = OSDisableInterrupts();
		for (v = sAXActiveVVoices; v != NULL; v = v->next) {
			if ((v->flags & SMSTATE_MASK) && v->sound_id == sfx) {
				AXDriverSetVolume(v->voice_id, vol, frames);
			}
		}
		OSRestoreInterrupts(enabled);
		return TRUE;
	}
	return FALSE;
}

static inline BOOL AXDriverSetPitch(s32 vid, s16 pitch)
{
	BOOL enabled;
	HSD_SM* v;
	s32 clamped;
	s32 idx;

	idx = vid & sAXVVoiceIndexMask;
	if (vid < 0 || idx > sAXMaxVVoiceIndex) {
		return FALSE;
	}
	enabled = OSDisableInterrupts();
	v       = &sAXVVoicePool[idx];
	if (v->voice_id != vid || !(v->flags & SMSTATE_MASK)) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}
	clamped = CLAMP(AXDRIVER_MIN_PITCH_CENTS, pitch, AXDRIVER_MAX_PITCH_CENTS);
	if (v->pid != -1) {
		HSD_SynthSFXSetPitch(v->pid, 1, AXDriverPitchRatio(clamped));
		v->user_pitch = clamped;
	} else {
		HSD_ASSERTMSG(1665, (v->flags & SMSTATE_MASK) == SMSTATE_ACTIVE, "(v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE");
		v->user_pitch = clamped;
		v->flags |= SMFLAG_USER_PITCH_PENDING;
	}
	OSRestoreInterrupts(enabled);
	return TRUE;
}

/**
 * @note Address: 0x801E88C0
 * @note Size: 0x1A0
 */
BOOL HSD_AudioSFXSetPitchByID(s32 sfx, s16 pitch)
{
	BOOL enabled;
	HSD_SM* v;

	if (sfx >= 0) {
		enabled = OSDisableInterrupts();
		for (v = sAXActiveVVoices; v != NULL; v = v->next) {
			if ((v->flags & SMSTATE_MASK) && v->sound_id == sfx) {
				AXDriverSetPitch(v->voice_id, pitch);
			}
		}
		OSRestoreInterrupts(enabled);
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x801E8A60
 * @note Size: 0xB0
 */
BOOL HSD_AudioSFXCheck(s32 vid)
{
	s32 idx;
	BOOL enabled;
	HSD_SM* v;
	BOOL result;

	idx = vid & sAXVVoiceIndexMask;
	if (vid < 0 || idx > sAXMaxVVoiceIndex) {
		return FALSE;
	}
	enabled = OSDisableInterrupts();
	v       = &sAXVVoicePool[idx];
	if (v->voice_id != vid || !(v->flags & SMSTATE_MASK)) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}
	result = HSD_SynthSFXCheck(v->pid) != -1 ? TRUE : FALSE;
	OSRestoreInterrupts(enabled);
	return result;
}

/**
 * @note Address: 0x801E8B10
 * @note Size: 0x108
 */
s32 HSD_AudioSFXCountByID(s32 sfx)
{
	s32 count = 0;
	BOOL enabled;
	HSD_SM* v;

	if (sfx >= 0) {
		enabled = OSDisableInterrupts();
		for (v = sAXActiveVVoices; v != NULL; v = v->next) {
			if ((v->flags & SMSTATE_MASK) && v->sound_id == sfx) {
				count += HSD_AudioSFXCheck(v->voice_id);
			}
		}
		OSRestoreInterrupts(enabled);
		return count;
	}
	return -1;
}

static inline u32 AXDriverMetadataBase(const void* data)
{
	return (u32)data & ~(sizeof(u32) - 1U);
}

static inline s32 AXDriverReadMetadataCount(const u8* bytes, s32* offset)
{
	s32 count = *(const s32*)(bytes + *offset);

	*offset += sizeof(count);
	return count;
}

/**
 * @note Address: 0x801E8C18
 * @note Size: 0x16C
 */
BOOL LoadSoundSampleMetadata(void* data)
{
	u8* bytes = data;
	s32 offset;
	s32 i;

	// Each table starts with an entry count. Address tables are rebased in place.
	lbl_8065BB14 = *(const s32*)bytes;
	offset = sizeof(lbl_8065BB14);
	lbl_8065BB10 = lbl_8065BB14 ? (u32*)(bytes + offset) : NULL;
	offset += lbl_8065BB14 * sizeof(*lbl_8065BB10);

	lbl_8065BB0C = AXDriverReadMetadataCount(bytes, &offset);
	lbl_8065BB08 = lbl_8065BB0C ? (u32*)(bytes + offset) : NULL;
	for (i = 0; i < lbl_8065BB0C; i++) {
		lbl_8065BB08[i] += AXDriverMetadataBase(data);
	}
	offset += lbl_8065BB0C * sizeof(*lbl_8065BB08);

	sSoundBankCount = AXDriverReadMetadataCount(bytes, &offset);
	sSoundBankFirstCommandIndex = sSoundBankCount ? (const u32*)(bytes + offset) : NULL;
	offset += sSoundBankCount * sizeof(*sSoundBankFirstCommandIndex);

	sSoundCommandStreamCount = AXDriverReadMetadataCount(bytes, &offset);
	sSoundCommandStreams = sSoundCommandStreamCount ? (HSD_SoundCommandRef*)(bytes + offset) : NULL;
	for (i = 0; i < sSoundCommandStreamCount; i++) {
		sSoundCommandStreams[i].offset += AXDriverMetadataBase(data);
	}
	offset += sSoundCommandStreamCount * sizeof(*sSoundCommandStreams);

	lbl_8065BAF4 = AXDriverReadMetadataCount(bytes, &offset);
	lbl_8065BAF0 = lbl_8065BAF4 ? (u32*)(bytes + offset) : NULL;
	for (i = 0; i < lbl_8065BAF4; i++) {
		lbl_8065BAF0[i] += AXDriverMetadataBase(data);
	}
	return TRUE;
}

/**
 * @note Address: 0x801E8D84
 * @note Size: 0x30C
 */
BOOL AXDriverSetupAux(s32 channel, AXDriverAuxEffect type, void* param)
{
	AXFX_REVERBHI* aux_hi;
	AXFX_REVERBSTD* aux_std;
	AXFX_CHORUS* aux_chorus;
	AXFX_DELAY* aux_delay;
	BOOL result;
	AXAuxCallback callback;
	void* callback_data;
	AXDriverAuxEffect old_type;

	aux_hi     = &sAuxReverbHi[channel];
	aux_std    = &sAuxReverbStd[channel];
	aux_chorus = &sAuxChorus[channel];
	aux_delay  = &sAuxDelay[channel];

	result        = FALSE;
	callback      = NULL;
	callback_data = NULL;

	HSD_ASSERT(2273, channel == 0 || channel == 1);
	HSD_ASSERTMSG(2274, AXDRIVER_AUX_NONE <= type && type <= AXDRIVER_AUX_DELAY, "0 <= type && type <= 4");
	HSD_ASSERTMSG(2275, type == AXDRIVER_AUX_NONE || param != NULL, "type == 0 || param != NULL");

	if (channel == 0) {
		old_type = (AXDriverAuxEffect)((sAXAuxConfig >> AXDRIVER_AUX_A_EFFECT_SHIFT) & AXDRIVER_AUX_EFFECT_MASK);
		sAXAuxConfig &= ~(old_type << AXDRIVER_AUX_A_EFFECT_SHIFT);
		sAXAuxConfig |= type << AXDRIVER_AUX_A_EFFECT_SHIFT;
		AXRegisterAuxACallback(NULL, NULL);
	} else {
		old_type = (AXDriverAuxEffect)((sAXAuxConfig >> AXDRIVER_AUX_B_EFFECT_SHIFT) & AXDRIVER_AUX_EFFECT_MASK);
		sAXAuxConfig &= ~(old_type << AXDRIVER_AUX_B_EFFECT_SHIFT);
		sAXAuxConfig |= type << AXDRIVER_AUX_B_EFFECT_SHIFT;
		AXRegisterAuxBCallback(NULL, NULL);
	}

	switch (old_type) {
	case AXDRIVER_AUX_REVERB_HI:
		AXFXReverbHiShutdown(aux_hi);
		break;
	case AXDRIVER_AUX_REVERB_STD:
		AXFXReverbStdShutdown(aux_std);
		break;
	case AXDRIVER_AUX_CHORUS:
		AXFXChorusShutdown(aux_chorus);
		break;
	case AXDRIVER_AUX_DELAY:
		AXFXDelayShutdown(aux_delay);
		break;
	}

	switch (type) {
	case AXDRIVER_AUX_NONE:
		result = TRUE;
		break;
	case AXDRIVER_AUX_REVERB_HI:
		memcpy(aux_hi, param, sizeof(AXFX_REVERBHI));
		if (AXFXReverbHiInit(aux_hi) == 1) {
			callback      = (AXAuxCallback)AXFXReverbHiCallback;
			callback_data = aux_hi;
			result        = TRUE;
		}
		break;
	case AXDRIVER_AUX_REVERB_STD:
		memcpy(aux_std, param, sizeof(AXFX_REVERBSTD));
		if (AXFXReverbStdInit(aux_std) == 1) {
			callback      = (AXAuxCallback)AXFXReverbStdCallback;
			callback_data = aux_std;
			result        = TRUE;
		}
		break;
	case AXDRIVER_AUX_CHORUS:
		memcpy(aux_chorus, param, sizeof(AXFX_CHORUS));
		if (AXFXChorusInit(aux_chorus) == 1) {
			callback      = (AXAuxCallback)AXFXChorusCallback;
			callback_data = aux_chorus;
			result        = TRUE;
		}
		break;
	case AXDRIVER_AUX_DELAY:
		memcpy(aux_delay, param, sizeof(AXFX_DELAY));
		if (AXFXDelayInit(aux_delay) == 1) {
			callback      = (AXAuxCallback)AXFXDelayCallback;
			callback_data = aux_delay;
			result        = TRUE;
		}
		break;
	}

	if (channel == 0) {
		AXRegisterAuxACallback(callback, callback_data);
	} else {
		AXRegisterAuxBCallback(callback, callback_data);
	}
	return result;
}

s32 sAXStreamSlotCount = 1;

static inline void AXDriverResetAux(s32 channel)
{
	axfxallocsize = 0;
	sAXEffectHeapBase = NULL;
	axfxmaxsize = 0;
	AXDriverSetupAux(channel, AXDRIVER_AUX_NONE, NULL);
}

static inline void AXDriverSetupVID(void)
{
	s32 mask;
	s32 bits;

	bits = 0;
	mask = 0;
	while ((1 << bits) < sAXMaxVVoiceIndex) {
		mask = (mask << 1) | 1;
		bits++;
	}
	sAXVVoiceIndexMask = mask;
	sAXVVoiceIndexBits = bits;
	vidhigh      = 32 - bits;
}

/**
 * @note Address: 0x801E9090
 * @note Size: 0x2F0
 */
BOOL HSD_AudioInitMultiPStream(u32 heap_size, u32 voices, u32 groups, s32 slots, s32 bufs)
{
	u32 i;

	if (heap_size & 0x1F) {
		return FALSE;
	}
	if (heap_size == 0) {
		heap_size = 0x800000;
	}
	if (groups == 0) {
		groups = 0x100;
	}
	sAXMaxGroupIndex = groups - 1;
	sAXGroupVVoiceLimits = HSD_AudioMalloc((groups * 2 + 0x1F) & ~0x1F);
	if (sAXGroupVVoiceLimits == NULL) {
		return FALSE;
	}
	for (i = 0; i < groups; i++) {
		sAXGroupVVoiceLimits[i] = 1;
	}

	if (voices == 0) {
		voices = 0x60;
	}
	sAXMaxVVoiceIndex = voices - 1;
	sAXVVoicePool = HSD_AudioMalloc((voices * sizeof(HSD_SM) + 0x1F) & ~0x1F);
	if (sAXVVoicePool == NULL) {
		return FALSE;
	}

	AXDriverSetupVID();

	for (i = 0; i < voices; i++) {
		HSD_SM* v = &sAXVVoicePool[i];

		v->flags &= ~SMSTATE_MASK;
		AXDriverLink(v, &sAXFreeVVoices);
	}

	sAXStreamSlotCount = slots;
	HSD_SynthInit(heap_size, slots, bufs);
	HSD_SynthSFXSetDriverMasterClockCallback(AXDriverCallback);
	HSD_SynthSFXSetDriverInactivatedCallback(AXDriverKillCallback);
	HSD_SynthSFXSetDriverPauseCallback(AXDriverPauseCallback);

	AXDriverResetAux(0);
	AXDriverResetAux(1);

	AXFXSetHooks(AXDriverAlloc, AXDriverFree);
	return TRUE;
}

/**
 * @note Address: 0x801E9380
 * @note Size: 0x28
 */
BOOL HSD_AudioInit(u32 heap_size, u32 voices, u32 groups)
{
	return HSD_AudioInitMultiPStream(heap_size, voices, groups, 1, 1);
}

static inline BOOL AXDriverPause(s32 vid)
{
	BOOL enabled;
	s32 idx;
	HSD_SM* v;

	idx = vid & sAXVVoiceIndexMask;
	if (vid < 0 || idx > sAXMaxVVoiceIndex) {
		return FALSE;
	}
	enabled = OSDisableInterrupts();
	v       = &sAXVVoicePool[idx];
	if (v->voice_id != vid || !(v->flags & SMSTATE_MASK)) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}
	if (v->pid != -1) {
		HSD_SynthSFXPause(v->pid);
	} else {
		HSD_ASSERTMSG(2797, (v->flags & SMSTATE_MASK) == SMSTATE_ACTIVE, "(v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE");
		v->flags |= SMFLAG_PAUSED;
	}
	OSRestoreInterrupts(enabled);
	return TRUE;
}

static inline BOOL AXDriverResume(s32 vid)
{
	BOOL enabled;
	s32 idx;
	HSD_SM* v;

	idx = vid & sAXVVoiceIndexMask;
	if (vid < 0 || idx > sAXMaxVVoiceIndex) {
		return FALSE;
	}
	enabled = OSDisableInterrupts();
	v       = &sAXVVoicePool[idx];
	if (v->voice_id != vid || !(v->flags & SMSTATE_MASK)) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}
	if (v->pid != -1) {
		HSD_SynthSFXResume(v->pid);
	} else {
		HSD_ASSERTMSG(2933, (v->flags & SMSTATE_MASK) == SMSTATE_ACTIVE, "(v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE");
	}
	v->flags &= ~SMFLAG_PAUSED;
	OSRestoreInterrupts(enabled);
	return TRUE;
}

/**
 * @note Address: 0x801E93A8
 * @note Size: 0x13C
 */
BOOL AXDriverPauseTrack(s32 track)
{
	BOOL enabled;
	HSD_SM* v;

	if (track < 0 || track > AXDRIVER_TRACK_COUNT - 1) {
		return FALSE;
	}
	enabled = OSDisableInterrupts();
	for (v = sAXActiveVVoices; v != NULL; v = v->next) {
		if ((v->flags & SMSTATE_MASK) && v->track == track) {
			AXDriverPause(v->voice_id);
		}
	}
	sAXPausedTrackMask |= AXDriverTrackBit(track);
	OSRestoreInterrupts(enabled);
	return TRUE;
}

/**
 * @note Address: 0x801E94E4
 * @note Size: 0x120
 */
BOOL AXDriverPauseSfx(s32 sfx)
{
	BOOL enabled;
	HSD_SM* v;

	if (sfx >= 0) {
		enabled = OSDisableInterrupts();
		for (v = sAXActiveVVoices; v != NULL; v = v->next) {
			if ((v->flags & SMSTATE_MASK) && v->sound_id == sfx) {
				AXDriverPause(v->voice_id);
			}
		}
		OSRestoreInterrupts(enabled);
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x801E9604
 * @note Size: 0x120
 */
BOOL AXDriverResumeSfx(s32 sfx)
{
	BOOL enabled;
	HSD_SM* v;

	if (sfx >= 0) {
		enabled = OSDisableInterrupts();
		for (v = sAXActiveVVoices; v != NULL; v = v->next) {
			if ((v->flags & SMSTATE_MASK) && v->sound_id == sfx) {
				AXDriverResume(v->voice_id);
			}
		}
		OSRestoreInterrupts(enabled);
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x801E9724
 * @note Size: 0x13C
 */
BOOL AXDriverResumeTrack(s32 track)
{
	BOOL enabled;
	HSD_SM* v;

	if (track < 0 || track > AXDRIVER_TRACK_COUNT - 1) {
		return FALSE;
	}
	enabled = OSDisableInterrupts();
	for (v = sAXActiveVVoices; v != NULL; v = v->next) {
		if ((v->flags & SMSTATE_MASK) && v->track == track) {
			AXDriverResume(v->voice_id);
		}
	}
	sAXPausedTrackMask &= ~AXDriverTrackBit(track);
	OSRestoreInterrupts(enabled);
	return TRUE;
}

static s32 sStreamVoiceIds[5] = { 0, -1, -1, -1, -1 };

/**
 * @note Address: 0x801E9860
 * @note Size: 0x5FC
 */
static s32 StartAudioStreamInSlot(const char* path, u8 vol, s32 pan, s32 span, s16 pitch, u8 auxA, u8 auxB, s32 channel,
                                  s32 slot, s32 useRear)
{
	s32 entrynum;
	f32 a_sqrt;
	f32 a_inv;
	f32 b_sqrt;
	f32 b_inv;
	f32 mix_b;
	f32 mix_main;

	entrynum = DVDConvertPathToEntrynum(path);
	if (slot < 1 || slot > sAXStreamSlotCount) {
		return 0;
	}
	if (entrynum == -1) {
		return 0;
	}
	if (sStreamVoiceIds[slot] != -1) {
		HSD_SynthSFXKeyOff(sStreamVoiceIds[slot]);
	}
	// Stream sends are used directly as fractions in the mix.
	a_sqrt   = msl_sqrtf(auxA);
	a_inv    = msl_sqrtf(1.0f - auxA);
	b_sqrt   = msl_sqrtf(auxB);
	mix_b    = a_inv * b_sqrt;
	b_inv    = msl_sqrtf(1.0f - auxB);
	mix_main = a_inv * b_inv;
	sStreamVoiceIds[slot] = HSD_SynthPStreamStart(entrynum, AXDRIVER_VOLUME_MAX, 0, vol, 0, AXDRIVER_PAN_CENTER, pan, 0,
	                                            1.0f, AXDriverPitchRatio(pitch),
	                                            mix_main, a_sqrt, mix_b, (u8)span, channel, useRear);
	if (sStreamVoiceIds[slot] == -1) {
		return 0;
	}
	return slot;
}

static BOOL IsActiveStreamSlot(s32 slot);

static inline s32 AXDriverFindSlot(void)
{
	s32 i;

	for (i = 1; i <= sAXStreamSlotCount; i++) {
		if (sStreamVoiceIds[i] == -1) {
			return i;
		}
	}
	for (i = 1; i <= sAXStreamSlotCount; i++) {
		if (!IsActiveStreamSlot(i)) {
			return i;
		}
	}
	return 0;
}

static inline s32 AXDriverSelectSlot(void)
{
	s32 slot = AXDriverFindSlot();
	s32 i;

	if (slot != 0) {
		return slot;
	}
	slot = 1;
	for (i = 2; i <= sAXStreamSlotCount; i++) {
		if (sStreamVoiceIds[i] < sStreamVoiceIds[slot]) {
			slot = i;
		}
	}
	return slot;
}

/**
 * @note Address: 0x801E9E5C
 * @note Size: 0x148
 */
// NONMATCHING: MWCC inlines AXDriverSelectSlot into this function; retail calls it separately.
s32 HSD_AudioPStreamStartChParam(const char* path, u8 vol, s32 channel)
{
	BOOL enabled;
	s32 slot;
	s32 result;

	enabled = OSDisableInterrupts();
	slot    = AXDriverSelectSlot();
	result  = StartAudioStreamInSlot(path, vol, AXDRIVER_PAN_CENTER, 0, 0, 0, 0, channel, slot, 0);
	OSRestoreInterrupts(enabled);
	return result;
}

static inline void AXDriverStopSlot(s32 slot)
{
	if (slot < 1 || slot > sAXStreamSlotCount) {
		return;
	}
	if (sStreamVoiceIds[slot] != -1) {
		HSD_SynthSFXKeyOff(sStreamVoiceIds[slot]);
		sStreamVoiceIds[slot] = -1;
	}
}

/**
 * @note Address: 0x801E9FA4
 * @note Size: 0x84
 */
void AXDriverStopAllStreams(void)
{
	s32 i;

	for (i = 1; i <= sAXStreamSlotCount; i++) {
		AXDriverStopSlot(i);
	}
}

static inline BOOL AXDriverSetSlotVolume(s32 slot, u8 vol, u16 frames)
{
	if (slot < 1 || slot > sAXStreamSlotCount) {
		return FALSE;
	}
	HSD_SynthSetVolFader(sStreamVoiceIds[slot], vol, frames, 1, 0);
	return TRUE;
}

/**
 * @note Address: 0x801EA028
 * @note Size: 0x9C
 */
BOOL AXDriverSetAllStreamVolumes(u8 vol, u16 frames)
{
	s32 i;

	for (i = 1; i <= sAXStreamSlotCount; i++) {
		AXDriverSetSlotVolume(i, vol, frames);
	}
	return TRUE;
}

static inline BOOL AXDriverPauseSlot(s32 slot)
{
	if (slot < 1 || slot > sAXStreamSlotCount) {
		return FALSE;
	}
	HSD_SynthSFXPause(sStreamVoiceIds[slot]);
	return TRUE;
}

/**
 * @note Address: 0x801EA0C4
 * @note Size: 0x70
 */
BOOL AXDriverPauseAllStreams(void)
{
	s32 i;

	for (i = 1; i <= sAXStreamSlotCount; i++) {
		AXDriverPauseSlot(i);
	}
	return TRUE;
}

static inline BOOL AXDriverResumeSlot(s32 slot)
{
	if (slot < 1 || slot > sAXStreamSlotCount) {
		return FALSE;
	}
	HSD_SynthSFXResume(sStreamVoiceIds[slot]);
	return TRUE;
}

/**
 * @note Address: 0x801EA134
 * @note Size: 0x70
 */
BOOL AXDriverResumeAllStreams(void)
{
	s32 i;

	for (i = 1; i <= sAXStreamSlotCount; i++) {
		AXDriverResumeSlot(i);
	}
	return TRUE;
}

static inline BOOL AXDriverSetSlotPitch(s32 slot, s16 pitch)
{
	if (slot < 1 || slot > sAXStreamSlotCount) {
		return FALSE;
	}
	HSD_SynthSFXSetPitch(sStreamVoiceIds[slot], 1, AXDriverPitchRatio(CLAMP(AXDRIVER_MIN_PITCH_CENTS, pitch, AXDRIVER_MAX_PITCH_CENTS)));
	return TRUE;
}

/**
 * @note Address: 0x801EA1A4
 * @note Size: 0xF4
 */
BOOL AXDriverSetAllStreamPitches(s16 pitch)
{
	s32 i;

	for (i = 1; i <= sAXStreamSlotCount; i++) {
		AXDriverSetSlotPitch(i, pitch);
	}
	return TRUE;
}

/**
 * @note Address: 0x801EA298
 * @note Size: 0x50
 */
static BOOL IsActiveStreamSlot(s32 slot)
{
	if (sStreamVoiceIds[slot] == -1) {
		return FALSE;
	}
	return HSD_SynthSFXCheck(sStreamVoiceIds[slot]) != -1;
}

/**
 * @note Address: 0x801EA2E8
 * @note Size: 0x20
 */
s32 AXDriverIsStreamActive(void)
{
	return HSD_SynthPStreamIsActive();
}

static inline BOOL AXDriverSetSlotParam(s32 slot, HSD_SynthBeatCallback callback, void* context)
{
	if (slot < 1 || slot > sAXStreamSlotCount) {
		return FALSE;
	}
	HSD_SynthPStreamSetBeatCallback(sStreamVoiceIds[slot], callback, context);
	return TRUE;
}

/**
 * @note Address: 0x801EA308
 * @note Size: 0x90
 */
BOOL AXDriverSetAllStreamParams(HSD_SynthBeatCallback callback, void* context)
{
	s32 i;

	for (i = 1; i <= sAXStreamSlotCount; i++) {
		AXDriverSetSlotParam(i, callback, context);
	}
	return TRUE;
}
