#include <global.h>
#include <math.h>
#include <string.h>

// hsd sound macro driver on top of synth.c and the sdk ax library. this revision
// is newer than the 2001 one: dynamic voice pool, sound groups, angle panning
// and stream slots

#define HSD_ASSERT(line, cond) ((cond) ? ((void)0) : __assert("axdriver.c", line, #cond))
// the msg form is for conditions whose original spelling clang-format would
// respace, or that name symbols still under placeholder names
#define HSD_ASSERTMSG(line, cond, msg) ((cond) ? ((void)0) : __assert("axdriver.c", line, msg))

#define SMSTATE_MASK   0xC0000000
#define SMSTATE_ACTIVE 0x40000000
#define SMSTATE_SLEEP  0x80000000

#define AXDRIVER_PIDMASK 0x3F

#define MIN2(x, y)           ((x) < (y) ? (x) : (y))
#define MAX2(x, y)           ((y) > (x) ? (y) : (x))
#define CLAMP(min, val, max) MAX2(MIN2(val, max), min)

typedef struct HSD_SM {
	struct HSD_SM* prev; // _00
	struct HSD_SM* next; // _04
	u32 flags;           // _08
	s32 _0C;             // _0C
	s32 pid;             // _10
	s32 _14;             // _14
	u16 _18;             // _18
	u8 _1A;              // _1A
	u8 _1B;              // _1B
	u8 _1C;              // _1C
	u8 _1D;              // _1D
	u8 _1E;              // _1E
	u8 _1F;              // _1F
	s16 _20;             // _20
	s16 _22;             // _22
	u16 _24;             // _24
	u8 _26;              // _26
	u8 _27;              // _27
	s16 _28;             // _28
	u8 _2A;              // _2A
	u8 _2B;              // _2B
	u8 _2C;              // _2C
	u8 _2D;              // _2D
	u16 _2E;             // _2E
	s16 _30;             // _30
	s16 _32;             // _32
	u8 _34[2];           // _34
	u8 _36;              // _36
	u8 _37;              // _37
	u8 _38;              // _38
	u8 _39;              // _39
	u16 _3A;             // _3A
	u32* _3C;            // _3C
	s32 _40;             // _40
} HSD_SM;

// voice pool element size, from the mulli 0x44 indexing and the 0xF0F0F0F1 divide
STATIC_ASSERT(sizeof(HSD_SM) == 0x44);

// synth.c
void* HSD_AudioMalloc(u32 size);
s32 HSD_SynthVolToDBx(f32 vol);
f32 HSD_SynthDBxToVol(s32 dbx);
s32 StartSynthSoundEffect(s32 fid, s32 vol, s32 vol_mode, s32 vol2, s32 vol2_mode, u8 pan, u8 x27, u8 span, f32 mix_main, f32 mix_a, f32 mix_b,
                u8 x2D, s32 pri, s32 track, s32 x2A, s32 x2B);
void HSD_SynthSFXKeyOff(s32 pid);
void HSD_SynthSFXPause(s32 pid);
void HSD_SynthSFXResume(s32 pid);
s32 HSD_SynthSFXCheck(s32 pid);
void HSD_SynthSetVolFader(s32 pid, s32 vol, s32 frames, s32 flag, s32 mode);
void HSD_SynthSFXSetPan(s32 pid, u8 pan, s32 flag);
void HSD_SynthSFXSetSPan(s32 pid, u8 span, s32 flag);
void HSD_SynthSFXSetMix(s32 pid, f32 mix_main, f32 mix_a, f32 mix_b);
void HSD_SynthSFXSetPitch(s32 pid, s32 which, f32 ratio);
void HSD_SynthSFXSetPriority(s32 pid, s32 pri);
void HSD_SynthSFXSetDriverInactivatedCallback(void (*cb)(s32));
void HSD_SynthSFXSetDriverMasterClockCallback(void (*cb)(void));
void HSD_SynthSFXSetDriverPauseCallback(void (*cb)(s32));
s32 HSD_SynthPStreamStart(s32 entrynum, s32 vol, s32 vol_mode, s32 vol2, s32 vol2_mode, u8 x7F, u8 b, u8 c, f32 pitch1, f32 pitch2,
                          f32 mix_main, f32 mix_a, f32 mix_b, u8 d, s32 e, s32 f);
s32 HSD_SynthPStreamIsActive(void);
void HSD_SynthPStreamSetBeatCallback(s32 pid, s32 a, s32 b);
void HSD_SynthInit(s32 heap_size, s32 slots, s32 bufs);
extern s32 sSynthDecibelFloor;

BOOL OSDisableInterrupts(void);
BOOL OSRestoreInterrupts(BOOL level);

// sdk, the real axfx header clashes with the hsd include path so only the
// sizes are spelled out here
typedef struct AXFX_REVERBHI {
	u8 data[0x1E0];
} AXFX_REVERBHI;
typedef struct AXFX_REVERBSTD {
	u8 data[0x154];
} AXFX_REVERBSTD;
typedef struct AXFX_CHORUS {
	u8 data[0x9C];
} AXFX_CHORUS;
typedef struct AXFX_DELAY {
	u8 data[0x60];
} AXFX_DELAY;
int AXFXReverbHiInit(AXFX_REVERBHI* rev);
int AXFXReverbHiShutdown(AXFX_REVERBHI* rev);
void AXFXReverbHiCallback(void* update, AXFX_REVERBHI* rev);
int AXFXReverbStdInit(AXFX_REVERBSTD* rev);
int AXFXReverbStdShutdown(AXFX_REVERBSTD* rev);
void AXFXReverbStdCallback(void* update, AXFX_REVERBSTD* rev);
int AXFXChorusInit(AXFX_CHORUS* chorus);
int AXFXChorusShutdown(AXFX_CHORUS* chorus);
void AXFXChorusCallback(void* update, AXFX_CHORUS* chorus);
int AXFXDelayInit(AXFX_DELAY* delay);
int AXFXDelayShutdown(AXFX_DELAY* delay);
void AXFXDelayCallback(void* update, AXFX_DELAY* delay);
s32 DVDConvertPathToEntrynum(const char* path);
void AXRegisterAuxACallback(void* callback, void* context);
void AXRegisterAuxBCallback(void* callback, void* context);
void AXFXSetHooks(void* (*alloc)(u32), void (*free)(void*));

#define FP_NAN       1
#define FP_INFINITE  2
#define FP_ZERO      3
#define FP_NORMAL    4
#define FP_SUBNORMAL 5

static inline int axd_fpclassifyf(f32 x)
{
	switch ((*(s32*)&x) & 0x7F800000) {
	case 0x7F800000:
		if ((*(s32*)&x) & 0x007FFFFF) {
			return FP_NAN;
		} else {
			return FP_INFINITE;
		}
	case 0:
		if ((*(s32*)&x) & 0x007FFFFF) {
			return FP_SUBNORMAL;
		} else {
			return FP_ZERO;
		}
	}
	return FP_NORMAL;
}

// msl cmath sqrtf, the one with the nan checks. it converts x to double once,
// which keeps an int argument from being converted again at every use
static inline f32 sqrtf(f32 x)
{
	if (x > 0.0f) {
		f64 dx    = x;
		f64 guess = __frsqrte(dx);
		guess     = 0.5 * guess * (3.0 - guess * guess * dx);
		guess     = 0.5 * guess * (3.0 - guess * guess * dx);
		guess     = 0.5 * guess * (3.0 - guess * guess * dx);
		return (f32)(dx * guess);
	} else if (x < 0.0) {
		return NAN;
	} else if (axd_fpclassifyf(x) == FP_NAN) {
		return NAN;
	}
	return x;
}

static inline f32 powf(f32 x, f32 y)
{
	return pow(x, y);
}

static HSD_SM* pvoices[0x40];
static u8 sTrackAuxSends[2][0x40];
static AXFX_REVERBHI lbl_8064D5B0[2];
static AXFX_REVERBSTD lbl_8064D970[2];
static AXFX_CHORUS lbl_8064DC18[2];
static AXFX_DELAY lbl_8064DD50[2];

u32 sAXAuxConfig = 2;
s32 sAXFrontRightAngle = 30;
s32 sAXRearRightAngle = 120;
s32 sAXRearLeftAngle = 240;
s32 sAXFrontLeftAngle = 330;

u8* sAXEffectHeapBase;
u32 axfxallocsize;
u16* sAXGroupVVoiceLimits;
s32 sAXMaxGroupIndex;
u32 axfxmaxsize;
s32 sAXActiveVVoiceCount;
u32 sAXPausedTrackMask;
s32 lbl_8065BAEC;
u32** lbl_8065BAF0;
s32 lbl_8065BAF4;
u32** sSoundCommandStreams;
s32 sSoundCommandStreamCount;
u32* sSoundBankFirstCommandIndex;
s32 sSoundBankCount;
u32* lbl_8065BB08;
s32 lbl_8065BB0C;
u32* lbl_8065BB10;
s32 lbl_8065BB14;
HSD_SM* sAXActiveVVoices;
HSD_SM* sAXFreeVVoices;
s32 sAXDriverTick;
HSD_SM* sAXVVoicePool;
s32 vidhigh;
s32 sAXVVoiceIndexMask;
s32 sAXVVoiceIndexBits;
s32 sAXMaxVVoiceIndex;

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

// appends at the tail now, the 2001 version pushed at the head
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
		lbl_8065BAEC--;
	}
	v->flags &= ~0x20000000;
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
	if (v->_0C != vid || !(v->flags & SMSTATE_MASK)) {
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
			if (v->_14 == sfx && (v->flags & SMSTATE_MASK)) {
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

	if (track >= 0 && track <= 0x3F) {
		enabled = OSDisableInterrupts();
		for (v = sAXActiveVVoices; v != NULL; v = v->next) {
			if ((v->flags & SMSTATE_MASK) && v->_39 == track) {
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
		if ((p->flags & SMSTATE_MASK) && p->_1A == v->_1A) {
			count++;
			if (p->_1B < low->_1B) {
				low = p;
			}
			if (p->_1B == same->_1B) {
				same = p;
			}
		}
	}
	if (count >= sAXGroupVVoiceLimits[v->_1A]) {
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
		if ((p->flags & SMSTATE_MASK) && p->_14 == v->_14) {
			if (++count > v->_2E) {
				return FALSE;
			}
		}
	}
	return TRUE;
}

// volume byte and db conversions for the relative volume commands. plain
// static, so their constants and the int to float magic land before
// StartAxDriverSynthVoice's in .sdata2
static s32 AXDriverDBToVol(s16 db)
{
	return 255.0f * HSD_SynthDBxToVol(db << 16);
}

static s32 AXDriverVolToDB(s16 vol)
{
	return HSD_SynthVolToDBx((1.0f / 255.0f) * vol) >> 16;
}

/**
 * @note Address: 0x801E64E8
 * @note Size: 0x6C4
 */
s32 StartAxDriverSynthVoice(HSD_SM* v)
{
	f32 left;
	f32 right;
	f32 left_sqrt;
	f32 left_inv;
	f32 right_sqrt;
	f32 right_inv;
	f32 mix_b;

	if (v->_1A != 0 && !AXDriverCheckGroup(v)) {
		return -1;
	}
	if (v->_2E != 0 && !AXDriverCheckSfxLimit(v)) {
		return -1;
	}
	left       = (f32)(v->_36 * v->_34[0]) / 65025.0f;
	right      = (f32)(v->_37 * v->_34[1]) / 65025.0f;
	left_sqrt  = sqrtf(left);
	left_inv   = sqrtf(1.0f - left);
	right_sqrt = sqrtf(right);
	mix_b      = left_inv * right_sqrt;
	right_inv  = sqrtf(1.0f - right);
	return StartSynthSoundEffect(v->_18, v->_20, v->_1E, v->_22, v->_1F, v->_26, v->_27, v->_2C, left_inv * right_inv, left_sqrt, mix_b, v->_2D,
	                   v->_1B, v->_39, v->_2A, v->_2B);
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
			case 0x1:
				if ((v->pid = StartAxDriverSynthVoice(v)) != -1) {
					HSD_ASSERTMSG(495, pvoices[v->pid & AXDRIVER_PIDMASK] == NULL, "pvoices[v->pid&AXDRIVER_PIDMASK] == NULL");
					pvoices[v->pid & AXDRIVER_PIDMASK] = v;
					HSD_SynthSetVolFader(v->pid, v->_22, v->_24, 1, v->_1F);
					v->flags &= 0xFFE8FFE1;
					if (v->flags & 0x80000) {
						HSD_SynthSFXSetPitch(v->pid, 1, powf(2.0f, v->_32 / 1200.0f));
						v->flags &= ~0x80000;
					}
					lbl_8065BAEC++;
				} else {
					AXDriverKeyOff(v);
					return;
				}
				break;
			case 0x2:
				HSD_SynthSFXSetPriority(v->pid, v->_1B);
				break;
			case 0x4:
				HSD_SynthSetVolFader(v->pid, v->_20, 1, 0, v->_1E);
				break;
			case 0x8:
				HSD_SynthSFXSetPan(v->pid, v->_26, 0);
				break;
			case 0x10:
				HSD_SynthSFXSetSPan(v->pid, v->_2C, 0);
				break;
			case 0x20:
				HSD_SynthSFXSetPitch(v->pid, 0, powf(2.0f, v->_30 / 1200.0f));
				break;
			case 0x40: {
				f32 left       = (f32)(v->_36 * v->_34[0]) / 65025.0f;
				f32 right      = (f32)(v->_37 * v->_34[1]) / 65025.0f;
				f32 left_sqrt  = sqrtf(left);
				f32 left_inv   = sqrtf(1.0f - left);
				f32 right_sqrt = sqrtf(right);
				f32 mix_b      = left_inv * right_sqrt;
				f32 right_inv  = sqrtf(1.0f - right);

				HSD_SynthSFXSetMix(v->pid, left_inv * right_inv, left_sqrt, mix_b);
				break;
			}
			case 0x80:
				v->flags = (v->flags & ~SMSTATE_MASK) | SMSTATE_SLEEP;
				return;
			case 0x100:
				AXDriverKeyOff(v);
				return;
			}
		}
		v->flags &= ~flag;
	}
}

static inline s32 AXDriverParseWait(s32 type, u32 cmd)
{
	switch (type) {
	case 0:
	case 14:
	case 15:
		return cmd & 0xFFFFFF;
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
		return (cmd >> 8) & 0xFFFF;
	case 1:
	case 2:
	case 3:
	case 12:
	case 13:
	case 23:
	case 24:
	case 25:
	case 26:
		return (cmd >> 16) & 0xFF;
	default:
		return 0;
	}
}

static inline void AXDriverUpdatePri(HSD_SM* v)
{
	v->_1B = v->_1C + v->_1D + 2;
	if (v->_1B < 2) {
		v->_1B = 2;
	} else if (v->_1B > 0x1D) {
		v->_1B = 0x1D;
	}
}

// pan and surround pan out of a 0 to 359 angle
static inline void AXDriverUpdateAngle(HSD_SM* v)
{
	s32 angle;
	s32 pan;
	s32 span;

	if (v->_28 < 0) {
		v->_28 += ((359 - v->_28) / 360) * 360;
	} else if (v->_28 >= 360) {
		v->_28 %= 360;
	}

	angle = v->_28;
	if (angle < sAXFrontRightAngle) {
		pan = angle * 127 / sAXFrontRightAngle + 127;
	} else if (angle <= sAXRearRightAngle) {
		pan = 254;
	} else if (angle < sAXRearLeftAngle) {
		pan = 254 - (angle - sAXRearRightAngle) * 254 / (sAXRearLeftAngle - sAXRearRightAngle);
	} else if (angle <= sAXFrontLeftAngle) {
		pan = 0;
	} else {
		pan = (angle - sAXFrontLeftAngle) * 127 / (360 - sAXFrontLeftAngle);
	}
	v->_26 = pan;

	if (v->_28 <= sAXFrontRightAngle) {
		span = 0;
	} else if (v->_28 < sAXRearRightAngle) {
		span = (v->_28 - sAXFrontRightAngle) * 255 / (sAXRearRightAngle - sAXFrontRightAngle);
	} else if (v->_28 <= sAXRearLeftAngle) {
		span = 255;
	} else if (v->_28 < sAXFrontLeftAngle) {
		span = (sAXFrontLeftAngle - v->_28) * 255 / (sAXFrontLeftAngle - sAXRearLeftAngle);
	} else {
		span = 0;
	}
	v->_2C = span;
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

	while (v->_40 == sAXDriverTick) {
		cmd  = *v->_3C;
		type = cmd >> 24;
		if (v->flags & 0x180) {
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
		v->_40 += wait;
		switch (type) {
		case 2:
			v->_3A = *v->_3C;
			if (v->_3A == 0) {
				v->flags |= 0x200000;
			}
			break;
		case 3:
			if ((v->flags & 0x200000) || v->_3A != 0) {
				v->_3C -= *v->_3C & 0xFFFF;
				v->_3A--;
			}
			break;
		case 1:
			v->flags |= 0x1;
			v->_18 = *v->_3C;
			break;
		case 4:
			v->flags |= 0x2;
			cmd_val = (s32)(*v->_3C & 0xFF);
			v->_1C  = CLAMP(0, cmd_val, 0x1B);
			AXDriverUpdatePri(v);
			break;
		case 5:
			v->flags |= 0x2;
			cmd_val = v->_1C + (s8)(u8)*v->_3C;
			v->_1C  = CLAMP(0, cmd_val, 0x1B);
			AXDriverUpdatePri(v);
			break;
		case 6:
			v->flags |= 0x4;
			v->_1E = 0;
			v->_20 = (u8)*v->_3C;
			break;
		case 7:
			v->flags |= 0x4;
			if (v->_1E == 0) {
				cmd_val = v->_20 + (s8)(u8)*v->_3C;
				v->_20  = (u8)CLAMP(0, cmd_val, 0xFF);
			} else {
				v->_1E  = 0;
				lin     = AXDriverDBToVol(v->_20);
				cmd_val = (s8)(u8)*v->_3C + lin;
				v->_20  = (u8)CLAMP(0, cmd_val, 0xFF);
			}
			break;
		case 23:
			v->flags |= 0x4;
			v->_1E  = 1;
			cmd_val = (s16)(u16)*v->_3C;
			v->_20  = CLAMP(sSynthDecibelFloor >> 16, cmd_val, 0);
			break;
		case 24:
			v->flags |= 0x4;
			if (v->_1E == 1) {
				cmd_val = v->_20 + (s16)(u16)*v->_3C;
				v->_20  = CLAMP(sSynthDecibelFloor >> 16, cmd_val, 0);
			} else {
				v->_1E = 1;
				db     = AXDriverVolToDB(v->_20);
				v->_20 = CLAMP(sSynthDecibelFloor >> 16, (s16)(u16)*v->_3C + db, 0);
			}
			break;
		case 25:
			v->flags |= 0x18;
			v->_28 = *v->_3C;
			AXDriverUpdateAngle(v);
			break;
		case 26:
			v->flags |= 0x18;
			v->_28 += (s16)(u16)*v->_3C;
			AXDriverUpdateAngle(v);
			break;
		case 8:
			v->flags |= 0x8;
			v->_26 = *v->_3C;
			break;
		case 9:
			v->flags |= 0x8;
			cmd_val = v->_26 + (s8)(u8)*v->_3C;
			v->_26  = CLAMP(0, cmd_val, 0xFE);
			break;
		case 10:
			v->flags |= 0x10;
			v->_2C = *v->_3C;
			break;
		case 11:
			v->flags |= 0x10;
			cmd_val = v->_2C + (s8)(u8)*v->_3C;
			v->_2C  = CLAMP(0, cmd_val, 0xFF);
			break;
		case 12:
			v->flags |= 0x20;
			v->_30 = *v->_3C;
			break;
		case 13:
			v->flags |= 0x20;
			cmd_val = v->_30 + (s16)(u16)*v->_3C;
			v->_30  = CLAMP(-0x2A30, cmd_val, 0x960);
			break;
		case 16:
			if (!(sAXAuxConfig & 1)) {
				v->flags |= 0x40;
				v->_34[0] = *v->_3C;
			}
			break;
		case 20:
			if (!(sAXAuxConfig & 1)) {
				v->_36 = *v->_3C;
			}
			break;
		case 21:
			if (!((sAXAuxConfig >> 1) & 1)) {
				v->_37 = *v->_3C;
			}
			break;
		case 17:
			if (!(sAXAuxConfig & 1)) {
				v->flags |= 0x40;
				cmd_val   = v->_34[0] + (s8)(u8)*v->_3C;
				v->_34[0] = CLAMP(0, cmd_val, 0xFF);
			}
			break;
		case 18:
			if (!((sAXAuxConfig >> 1) & 1)) {
				v->flags |= 0x40;
				v->_34[1] = *v->_3C;
			}
			break;
		case 19:
			if (!((sAXAuxConfig >> 1) & 1)) {
				v->flags |= 0x40;
				cmd_val   = v->_34[1] + (s8)(u8)*v->_3C;
				v->_34[1] = CLAMP(0, cmd_val, 0xFF);
			}
			break;
		case 22:
			v->_2E = (u8)*v->_3C;
			break;
		case 27:
			v->_2A = *v->_3C;
			break;
		case 28:
			v->_2B = *v->_3C;
			break;
		case 15:
			v->flags |= 0x100;
			if (wait == 0) {
				AXDriverExec(v);
			}
			return;
		case 14:
			v->flags |= 0x80;
			if (wait == 0) {
				AXDriverExec(v);
			}
			return;
		}
		v->_3C++;
	}
}

/**
 * @note Address: 0x801E7E84
 * @note Size: 0x208
 */
void AXDriverCallback(void)
{
	HSD_SM* v;
	HSD_SM* next;

	v = sAXActiveVVoices;
	sAXDriverTick++;
	while (v != NULL) {
		next = v->next;
		if (v->_40 == -1) {
			v->_40 = sAXDriverTick;
		} else if (v->flags & 0x20000000) {
			v->_40++;
		}
		switch (v->flags & SMSTATE_MASK) {
		case SMSTATE_ACTIVE:
			if (v->_40 == sAXDriverTick) {
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
		if ((v->flags & SMSTATE_MASK) == SMSTATE_ACTIVE && v->_40 == sAXDriverTick) {
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
	v->flags &= ~0x20000000;
	v->flags &= ~SMSTATE_MASK;
	lbl_8065BAEC--;
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
	v->flags |= 0x20000000;
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
	v->pid   = -1;
	v->_40   = -1;
	v->flags = 0;
	v->flags &= ~SMSTATE_MASK;
	return v;
}

/**
 * @note Address: 0x801E8204
 * @note Size: 0x3A8
 */
s32 AXDriverStartVVoice(u32 sfx, u8 vol, u8 x27, u8 x2D, s16 x32, u8 aux0, u8 aux1, s32 group, s32 track)
{
	HSD_SM* v;
	s32 bank;
	s32 sample;
	BOOL enabled;

	bank   = sfx >> 16;
	sample = sfx & 0xFFFF;
	if (sSoundBankCount <= bank) {
		return -1;
	}
	sample = sSoundBankFirstCommandIndex[bank] + sample;
	if (sSoundCommandStreamCount <= sample) {
		return -1;
	}
	if (bank < sSoundBankCount - 1 && sSoundBankFirstCommandIndex[bank + 1] <= sample) {
		return -1;
	}
	if (group < 0 || sAXMaxGroupIndex < group) {
		return -1;
	}
	if (track < 0 || track > 0x3F) {
		return -1;
	}
	if (sAXPausedTrackMask & (1 << track)) {
		return -1;
	}

	v = AXDriverAssignVVoice();
	if (v == NULL) {
		return -1;
	}

	v->_14 = sfx;
	v->_3C = sSoundCommandStreams[sample];
	if (vol > 0xFF) {
		vol = 0xFF;
	}
	v->_20    = 0xFF;
	v->_22    = vol;
	v->_1E    = 0;
	v->_1F    = 0;
	v->_24    = 1;
	v->_26    = 0x7F;
	v->_27    = x27;
	v->_2C    = 0;
	v->_2D    = x2D;
	v->_30    = 0;
	v->_32    = x32;
	v->_28    = 0;
	v->_2A    = 1;
	v->_2B    = 1;
	v->_34[0] = aux0;
	v->_34[1] = aux1;
	v->_36    = 0xFF;
	v->_37    = 0xFF;
	v->_1C    = 0;
	v->_1D    = 0;
	AXDriverUpdatePri(v);
	v->_1A = group;
	v->_39 = track;
	v->_2E = 0;
	v->flags |= 0x1F0000;

	if (vidhigh & ~((1 << (32 - sAXVVoiceIndexBits)) - 1)) {
		OSReport("vidhigh exceeds the max value\n");
		HSD_ASSERT(1067, 0);
	}
	v->_0C = ((u8*)v - (u8*)sAXVVoicePool) / sizeof(HSD_SM) | (vidhigh << sAXVVoiceIndexBits);
	vidhigh++;

	enabled  = OSDisableInterrupts();
	v->flags = (v->flags & ~SMSTATE_MASK) | SMSTATE_ACTIVE;
	AXDriverLink(v, &sAXActiveVVoices);
	sAXActiveVVoiceCount++;
	OSRestoreInterrupts(enabled);
	return v->_0C;
}

/**
 * @note Address: 0x801E85AC
 * @note Size: 0x44
 */
s32 HSD_AudioSFXStartParam(u32 sfx, u8 vol, u8 pan, s32 group, s32 track)
{
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
	if (v->_0C != vid || !(v->flags & SMSTATE_MASK)) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}
	if (v->pid != -1) {
		HSD_SynthSFXSetPan(v->pid, CLAMP(0, pan, 0xFE), 1);
	} else {
		HSD_ASSERTMSG(1149, (v->flags & SMSTATE_MASK) == SMSTATE_ACTIVE, "(v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE");
		v->_27 = pan;
		v->flags |= 0x20000;
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
			if ((v->flags & SMSTATE_MASK) && v->_14 == sfx) {
				AXDriverSetPan(v->_0C, pan);
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
	if (clamped > 0xFF) {
		clamped = 0xFF;
	}
	idx = vid & sAXVVoiceIndexMask;
	if (vid < 0 || idx > sAXMaxVVoiceIndex) {
		return FALSE;
	}
	enabled = OSDisableInterrupts();
	v       = &sAXVVoicePool[idx];
	if (v->_0C != vid || !(v->flags & SMSTATE_MASK)) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}
	if (v->pid != -1) {
		HSD_SynthSetVolFader(v->pid, clamped, frames, 1, 0);
	} else {
		HSD_ASSERTMSG(1474, (v->flags & SMSTATE_MASK) == SMSTATE_ACTIVE, "(v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE");
		v->_22 = clamped;
		v->_1F = 0;
		v->_24 = frames;
		v->flags |= 0x10000;
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
			if ((v->flags & SMSTATE_MASK) && v->_14 == sfx) {
				AXDriverSetVolume(v->_0C, vol, frames);
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
	if (v->_0C != vid || !(v->flags & SMSTATE_MASK)) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}
	clamped = CLAMP(-0x2A30, pitch, 0x960);
	if (v->pid != -1) {
		HSD_SynthSFXSetPitch(v->pid, 1, powf(2.0f, clamped / 1200.0f));
		v->_32 = clamped;
	} else {
		HSD_ASSERTMSG(1665, (v->flags & SMSTATE_MASK) == SMSTATE_ACTIVE, "(v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE");
		v->_32 = clamped;
		v->flags |= 0x80000;
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
			if ((v->flags & SMSTATE_MASK) && v->_14 == sfx) {
				AXDriverSetPitch(v->_0C, pitch);
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
	if (v->_0C != vid || !(v->flags & SMSTATE_MASK)) {
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
			if ((v->flags & SMSTATE_MASK) && v->_14 == sfx) {
				count += HSD_AudioSFXCheck(v->_0C);
			}
		}
		OSRestoreInterrupts(enabled);
		return count;
	}
	return -1;
}

/**
 * @note Address: 0x801E8C18
 * @note Size: 0x16C
 */
BOOL LoadSoundSampleMetadata(void* data)
{
	s32 offset;
	s32 i;

	offset       = 0;
	lbl_8065BB14 = *(s32*)((u8*)data + offset);
	offset += 4;
	lbl_8065BB10 = lbl_8065BB14 ? (u32*)((u8*)data + offset) : NULL;
	offset += lbl_8065BB14 * 4;

	lbl_8065BB0C = *(s32*)((u8*)data + offset);
	offset += 4;
	lbl_8065BB08 = lbl_8065BB0C ? (u32*)((u8*)data + offset) : NULL;
	for (i = 0; i < lbl_8065BB0C; i++) {
		lbl_8065BB08[i] += (u32)data & ~3;
	}
	offset += lbl_8065BB0C * 4;

	sSoundBankCount = *(s32*)((u8*)data + offset);
	offset += 4;
	sSoundBankFirstCommandIndex = sSoundBankCount ? (u32*)((u8*)data + offset) : NULL;
	offset += sSoundBankCount * 4;

	sSoundCommandStreamCount = *(s32*)((u8*)data + offset);
	offset += 4;
	sSoundCommandStreams = sSoundCommandStreamCount ? (u32**)((u8*)data + offset) : NULL;
	for (i = 0; i < sSoundCommandStreamCount; i++) {
		*(u32*)&sSoundCommandStreams[i] += (u32)data & ~3;
	}
	offset += sSoundCommandStreamCount * 4;

	lbl_8065BAF4 = *(s32*)((u8*)data + offset);
	offset += 4;
	lbl_8065BAF0 = lbl_8065BAF4 ? (u32**)((u8*)data + offset) : NULL;
	for (i = 0; i < lbl_8065BAF4; i++) {
		*(u32*)&lbl_8065BAF0[i] += (u32)data & ~3;
	}
	return TRUE;
}

/**
 * @note Address: 0x801E8D84
 * @note Size: 0x30C
 */
BOOL AXDriverSetupAux(s32 channel, s32 type, void* param)
{
	AXFX_REVERBHI* aux_hi;
	AXFX_REVERBSTD* aux_std;
	AXFX_CHORUS* aux_chorus;
	AXFX_DELAY* aux_delay;
	BOOL result;
	void* callback;
	void* callback_data;
	s32 old_type;

	aux_hi     = &lbl_8064D5B0[channel];
	aux_std    = &lbl_8064D970[channel];
	aux_chorus = &lbl_8064DC18[channel];
	aux_delay  = &lbl_8064DD50[channel];

	result        = FALSE;
	callback      = NULL;
	callback_data = NULL;

	HSD_ASSERT(2273, channel == 0 || channel == 1);
	HSD_ASSERT(2274, 0 <= type && type <= 4);
	HSD_ASSERT(2275, type == 0 || param != NULL);

	if (channel == 0) {
		old_type = sAXAuxConfig >> 8 & 0xF;
		sAXAuxConfig &= ~(old_type << 8);
		sAXAuxConfig |= type << 8;
		AXRegisterAuxACallback(NULL, NULL);
	} else {
		old_type = sAXAuxConfig >> 12 & 0xF;
		sAXAuxConfig &= ~(old_type << 12);
		sAXAuxConfig |= type << 12;
		AXRegisterAuxBCallback(NULL, NULL);
	}

	switch (old_type) {
	case 1:
		AXFXReverbHiShutdown(aux_hi);
		break;
	case 2:
		AXFXReverbStdShutdown(aux_std);
		break;
	case 3:
		AXFXChorusShutdown(aux_chorus);
		break;
	case 4:
		AXFXDelayShutdown(aux_delay);
		break;
	}

	switch (type) {
	case 0:
		result = TRUE;
		break;
	case 1:
		memcpy(aux_hi, param, sizeof(AXFX_REVERBHI));
		if (AXFXReverbHiInit(aux_hi) == 1) {
			callback      = AXFXReverbHiCallback;
			callback_data = aux_hi;
			result        = TRUE;
		}
		break;
	case 2:
		memcpy(aux_std, param, sizeof(AXFX_REVERBSTD));
		if (AXFXReverbStdInit(aux_std) == 1) {
			callback      = AXFXReverbStdCallback;
			callback_data = aux_std;
			result        = TRUE;
		}
		break;
	case 3:
		memcpy(aux_chorus, param, sizeof(AXFX_CHORUS));
		if (AXFXChorusInit(aux_chorus) == 1) {
			callback      = AXFXChorusCallback;
			callback_data = aux_chorus;
			result        = TRUE;
		}
		break;
	case 4:
		memcpy(aux_delay, param, sizeof(AXFX_DELAY));
		if (AXFXDelayInit(aux_delay) == 1) {
			callback      = AXFXDelayCallback;
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

	axfxallocsize = 0;
	sAXEffectHeapBase  = NULL;
	axfxmaxsize   = 0;
	AXDriverSetupAux(0, 0, NULL);

	axfxallocsize = 0;
	sAXEffectHeapBase  = NULL;
	axfxmaxsize   = 0;
	AXDriverSetupAux(1, 0, NULL);

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
	if (v->_0C != vid || !(v->flags & SMSTATE_MASK)) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}
	if (v->pid != -1) {
		HSD_SynthSFXPause(v->pid);
	} else {
		HSD_ASSERTMSG(2797, (v->flags & SMSTATE_MASK) == SMSTATE_ACTIVE, "(v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE");
		v->flags |= 0x20000000;
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
	if (v->_0C != vid || !(v->flags & SMSTATE_MASK)) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}
	if (v->pid != -1) {
		HSD_SynthSFXResume(v->pid);
	} else {
		HSD_ASSERTMSG(2933, (v->flags & SMSTATE_MASK) == SMSTATE_ACTIVE, "(v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE");
	}
	v->flags &= ~0x20000000;
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

	if (track < 0 || track > 0x3F) {
		return FALSE;
	}
	enabled = OSDisableInterrupts();
	for (v = sAXActiveVVoices; v != NULL; v = v->next) {
		if ((v->flags & SMSTATE_MASK) && v->_39 == track) {
			AXDriverPause(v->_0C);
		}
	}
	sAXPausedTrackMask |= 1 << track;
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
			if ((v->flags & SMSTATE_MASK) && v->_14 == sfx) {
				AXDriverPause(v->_0C);
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
			if ((v->flags & SMSTATE_MASK) && v->_14 == sfx) {
				AXDriverResume(v->_0C);
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

	if (track < 0 || track > 0x3F) {
		return FALSE;
	}
	enabled = OSDisableInterrupts();
	for (v = sAXActiveVVoices; v != NULL; v = v->next) {
		if ((v->flags & SMSTATE_MASK) && v->_39 == track) {
			AXDriverResume(v->_0C);
		}
	}
	sAXPausedTrackMask &= ~(1 << track);
	OSRestoreInterrupts(enabled);
	return TRUE;
}

static s32 sStreamVoiceIds[5] = { 0, -1, -1, -1, -1 };

/**
 * @note Address: 0x801E9860
 * @note Size: 0x5FC
 */
s32 StartAudioStreamInSlot(const char* path, u8 vol, s32 b, s32 c, s16 pitch, u8 aux0, u8 aux1, s32 d, s32 slot, s32 e)
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
	// no division by 255 here, so any send above 1 gives nan, cool
	a_sqrt   = sqrtf(aux0);
	a_inv    = sqrtf(1.0f - aux0);
	b_sqrt   = sqrtf(aux1);
	mix_b    = a_inv * b_sqrt;
	b_inv    = sqrtf(1.0f - aux1);
	mix_main = a_inv * b_inv;
	if (vol > 0xFF) {
		vol = 0xFF;
	}
	sStreamVoiceIds[slot]
	    = HSD_SynthPStreamStart(entrynum, 0xFF, 0, vol, 0, 0x7F, b, 0, 1.0f, powf(2.0f, pitch / 1200.0f), mix_main, a_sqrt, mix_b, c, d, e);
	if (sStreamVoiceIds[slot] == -1) {
		return 0;
	}
	return slot;
}

BOOL IsActiveStreamSlot(s32 slot);

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
s32 HSD_AudioPStreamStartChParam(const char* path, u8 vol, s32 channel)
{
	BOOL enabled;
	s32 slot;
	s32 result;

	enabled = OSDisableInterrupts();
	slot    = AXDriverSelectSlot();
	result  = StartAudioStreamInSlot(path, vol, 0x7F, 0, 0, 0, 0, channel, slot, 0);
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
	if (vol > 0xFF) {
		vol = 0xFF;
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
	HSD_SynthSFXSetPitch(sStreamVoiceIds[slot], 1, powf(2.0f, CLAMP(-0x2A30, pitch, 0x960) / 1200.0f));
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
BOOL IsActiveStreamSlot(s32 slot)
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

static inline BOOL AXDriverSetSlotParam(s32 slot, s32 a, s32 b)
{
	if (slot < 1 || slot > sAXStreamSlotCount) {
		return FALSE;
	}
	HSD_SynthPStreamSetBeatCallback(sStreamVoiceIds[slot], a, b);
	return TRUE;
}

/**
 * @note Address: 0x801EA308
 * @note Size: 0x90
 */
BOOL AXDriverSetAllStreamParams(s32 a, s32 b)
{
	s32 i;

	for (i = 1; i <= sAXStreamSlotCount; i++) {
		AXDriverSetSlotParam(i, a, b);
	}
	return TRUE;
}
