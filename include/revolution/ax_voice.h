#ifndef _REVOSDK_AX_VOICE_H
#define _REVOSDK_AX_VOICE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AX_PRIORITY_FREE 0
#define AX_PRIORITY_MIN  1
#define AX_PRIORITY_MAX  31

typedef enum { AX_VOICE_STOP, AX_VOICE_RUN } AXVOICESTATE;

typedef struct _AXPBMIX {
	u16 vL;          // _00
	u16 vDeltaL;     // _02
	u16 vR;          // _04
	u16 vDeltaR;     // _06
	u16 vAuxAL;      // _08
	u16 vDeltaAuxAL; // _0A
	u16 vAuxAR;      // _0C
	u16 vDeltaAuxAR; // _0E
	u16 vAuxBL;      // _10
	u16 vDeltaAuxBL; // _12
	u16 vAuxBR;      // _14
	u16 vDeltaAuxBR; // _16
	u16 vAuxCL;      // _18
	u16 vDeltaAuxCL; // _1A
	u16 vAuxCR;      // _1C
	u16 vDeltaAuxCR; // _1E
	u16 vS;          // _20
	u16 vDeltaS;     // _22
	u16 vAuxAS;      // _24
	u16 vDeltaAuxAS; // _26
	u16 vAuxBS;      // _28
	u16 vDeltaAuxBS; // _2A
	u16 vAuxCS;      // _2C
	u16 vDeltaAuxCS; // _2E
} AXPBMIX;

typedef struct _AXPBITD {
	u16 flag;         // _00
	u16 bufferHi;     // _02
	u16 bufferLo;     // _04
	u16 shiftL;       // _06
	u16 shiftR;       // _08
	u16 targetShiftL; // _0A
	u16 targetShiftR; // _0C
} AXPBITD;

typedef struct _AXPBDPOP {
	s16 aL;     // _00
	s16 aAuxAL; // _02
	s16 aAuxBL; // _04
	s16 aAuxCL; // _06
	s16 aR;     // _08
	s16 aAuxAR; // _0A
	s16 aAuxBR; // _0C
	s16 aAuxCR; // _0E
	s16 aS;     // _10
	s16 aAuxAS; // _12
	s16 aAuxBS; // _14
	s16 aAuxCS; // _16
} AXPBDPOP;

typedef struct _AXPBVE {
	u16 currentVolume; // _00
	s16 currentDelta;  // _02
} AXPBVE;

typedef struct _AXPBADDR {
	u16 loopFlag;         // _00
	u16 format;           // _02
	u16 loopAddressHi;    // _04
	u16 loopAddressLo;    // _06
	u16 endAddressHi;     // _08
	u16 endAddressLo;     // _0A
	u16 currentAddressHi; // _0C
	u16 currentAddressLo; // _0E
} AXPBADDR;

typedef struct _AXPBADPCM {
	u16 a[8][2];    // _00
	u16 gain;       // _20
	u16 pred_scale; // _22
	u16 yn1;        // _24
	u16 yn2;        // _26
} AXPBADPCM;

typedef struct _AXPBSRC {
	u16 ratioHi;            // _00
	u16 ratioLo;            // _02
	u16 currentAddressFrac; // _04
	u16 last_samples[4];    // _06
} AXPBSRC;

typedef struct _AXPBADPCMLOOP {
	u16 loop_pred_scale; // _00
	u16 loop_yn1;        // _02
	u16 loop_yn2;        // _04
} AXPBADPCMLOOP;

typedef struct _AXPBLPF {
	u16 on;  // _00
	u16 yn1; // _02
	u16 a0;  // _04
	u16 b0;  // _06
} AXPBLPF;

typedef struct _AXPBBIQUAD {
	u16 on;  // _00
	u16 xn1; // _02
	u16 xn2; // _04
	u16 yn1; // _06
	u16 yn2; // _08
	u16 b0;  // _0A
	u16 b1;  // _0C
	u16 b2;  // _0E
	u16 a1;  // _10
	u16 a2;  // _12
} AXPBBIQUAD;

typedef struct _AXPBRMTMIX {
	u16 vMain0;      // _00
	u16 vDeltaMain0; // _02
	u16 vAux0;       // _04
	u16 vDeltaAux0;  // _06
	u16 vMain1;      // _08
	u16 vDeltaMain1; // _0A
	u16 vAux1;       // _0C
	u16 vDeltaAux1;  // _0E
	u16 vMain2;      // _10
	u16 vDeltaMain2; // _12
	u16 vAux2;       // _14
	u16 vDeltaAux2;  // _16
	u16 vMain3;      // _18
	u16 vDeltaMain3; // _1A
	u16 vAux3;       // _1C
	u16 vDeltaAux3;  // _1E
} AXPBRMTMIX;

typedef struct _AXPBRMTDPOP {
	s16 aMain0; // _00
	s16 aMain1; // _02
	s16 aMain2; // _04
	s16 aMain3; // _06
	s16 aAux0;  // _08
	s16 aAux1;  // _0A
	s16 aAux2;  // _0C
	s16 aAux3;  // _0E
} AXPBRMTDPOP;

typedef struct _AXPBRMTSRC {
	u16 currentAddressFrac; // _00
	u16 last_samples[4];    // _02
} AXPBRMTSRC;

typedef union __AXPBRMTIIR {
	AXPBLPF lpf;
	AXPBBIQUAD biquad;
} AXPBRMTIIR;

typedef struct _AXPB {
	u16 nextHi;                // _00
	u16 nextLo;                // _02
	u16 currHi;                // _04
	u16 currLo;                // _06
	u16 srcSelect;             // _08
	u16 coefSelect;            // _0A
	u32 mixerCtrl;             // _0C
	u16 state;                 // _10
	u16 type;                  // _12
	AXPBMIX mix;               // _14
	AXPBITD itd;               // _44
	AXPBDPOP dpop;             // _52
	AXPBVE ve;                 // _6A
	AXPBADDR addr;             // _6E
	AXPBADPCM adpcm;           // _7E
	AXPBSRC src;               // _A6
	AXPBADPCMLOOP adpcmLoop;   // _B4
	AXPBLPF lpf;               // _BA
	AXPBBIQUAD biquad;         // _C2
	u16 remote;                // _D6
	u16 rmtMixerCtrl;          // _D8
	AXPBRMTMIX rmtMix;         // _DA
	AXPBRMTDPOP rmtDpop;       // _FA
	AXPBRMTSRC rmtSrc;         // _10A
	AXPBRMTIIR rmtIIR;         // _114
	u8 padding[0x140 - 0x128]; // DSP parameter blocks are 0x140 bytes apart.
} AXPB;

typedef void (*AXVoiceCallback)(void* vpb);

typedef struct _AXVPB {
	struct _AXVPB* next;      // _0
	struct _AXVPB* prev;      // _4
	struct _AXVPB* next1;     // _8
	u32 priority;             // _C
	AXVoiceCallback callback; // _10
	u32 userContext;          // _14
	u32 index;                // _18
	u32 sync;                 // _1C
	u32 depop;                // _20
	void* itdBuffer;          // _24
	AXPB pb;                  // _28
} AXVPB;

typedef enum {
	AX_PBSYNC_SELECT              = (1U << 0),
	AX_PBSYNC_MIXER_CTRL          = (1U << 1),
	AX_PBSYNC_STATE               = (1U << 2),
	AX_PBSYNC_TYPE                = (1U << 3),
	AX_PBSYNC_MIX                 = (1U << 4),
	AX_PBSYNC_ITD                 = (1U << 5),
	AX_PBSYNC_ITD_SHIFT           = (1U << 6),
	AX_PBSYNC_DPOP                = (1U << 7),
	AX_PBSYNC_VE                  = (1U << 8),
	AX_PBSYNC_VE_DELTA            = (1U << 9),
	AX_PBSYNC_ADDR                = (1U << 10),
	AX_PBSYNC_LOOP_FLAG           = (1U << 11),
	AX_PBSYNC_LOOP_ADDR           = (1U << 12),
	AX_PBSYNC_END_ADDR            = (1U << 13),
	AX_PBSYNC_CURR_ADDR           = (1U << 14),
	AX_PBSYNC_ADPCM               = (1U << 15),
	AX_PBSYNC_SRC                 = (1U << 16),
	AX_PBSYNC_SRC_RATIO           = (1U << 17),
	AX_PBSYNC_ADPCM_LOOP          = (1U << 18),
	AX_PBSYNC_LPF                 = (1U << 19),
	AX_PBSYNC_LPF_COEFS           = (1U << 20),
	AX_PBSYNC_BIQUAD              = (1U << 21),
	AX_PBSYNC_BIQUAD_COEFS        = (1U << 22),
	AX_PBSYNC_REMOTE              = (1U << 23),
	AX_PBSYNC_RMT_MIXER_CTRL      = (1U << 24),
	AX_PBSYNC_RMTMIX              = (1U << 25),
	AX_PBSYNC_RMTDPOP             = (1U << 26),
	AX_PBSYNC_RMTSRC              = (1U << 27),
	AX_PBSYNC_RMTIIR              = (1U << 28),
	AX_PBSYNC_RMTIIR_LPF_COEFS    = (1U << 29),
	AX_PBSYNC_RMTIIR_BIQUAD_COEFS = (1U << 30),
	AX_PBSYNC_ALL                 = (1U << 31),
};

typedef enum { AX_SAMPLE_FORMAT_DSP_ADPCM = 0, AX_SAMPLE_FORMAT_PCM_S16 = 10, AX_SAMPLE_FORMAT_PCM_S8 = 25 } AXSAMPLETYPE;
enum { AX_PB_LPF_ON = 1, AX_PB_BIQUAD_ON };

void __AXSyncPBs(u32 baseCycles);
s32 __AXGetNumVoices(void);
s32 AXGetMaxVoices(void);
void AXSetVoiceState(AXVPB* voice, u16 state);
void AXSetVoiceAddr(AXVPB* voice, AXPBADDR* addr);
void AXGetLpfCoefs(u16 freq, u16* a, u16* b);
AXVPB* __AXGetStackHead(u32 priority);
void __AXServiceCallbackStack(void);
void __AXPushFreeStack(AXVPB* voice);
void __AXPushCallbackStack(AXVPB* voice);
void __AXRemoveFromStack(AXVPB* voice);
void __AXSetPBDefault(AXVPB* voice);
AXPB* __AXGetPBs(void);
void AXFreeVoice(AXVPB* voice);
AXVPB* AXAcquireVoice(u32 priority, AXVoiceCallback callback, u32 userContext);
void AXSetVoicePriority(AXVPB* voice, u32 priority);

#ifdef __cplusplus
}
#endif
#endif
