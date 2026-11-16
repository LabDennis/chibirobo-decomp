#ifndef _REVOSDK_AX_API_H
#define _REVOSDK_AX_API_H

// AX names NW4R uses that revolution/ax.h and revolution/ax_voice.h do not declare
// (after the AX headers of xenoblade's and ss's RVL_SDK, CC0).

#include "revolution/ax.h"
#include "revolution/ax_voice.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AX_SAMPLE_RATE 32000
#define AX_VOICE_MAX   96
#define AX_MAX_VOLUME  32768

// DSP-ADPCM: 8-byte frames, one header byte and 14 samples
#define AX_ADPCM_FRAME_SIZE             8
#define AX_ADPCM_SAMPLE_BYTES_PER_FRAME (AX_ADPCM_FRAME_SIZE - 1)
#define AX_ADPCM_SAMPLES_PER_BYTE       2
#define AX_ADPCM_SAMPLES_PER_FRAME      (AX_ADPCM_SAMPLE_BYTES_PER_FRAME * AX_ADPCM_SAMPLES_PER_BYTE)
#define AX_ADPCM_NIBBLES_PER_FRAME      (AX_ADPCM_FRAME_SIZE * 2)

#define AX_PB_OFF 0

typedef enum { AX_VOICE_NORMAL, AX_VOICE_STREAM } AXVOICETYPE;

typedef enum {
	AX_SRC_TYPE_NONE,
	AX_SRC_TYPE_LINEAR,
	AX_SRC_TYPE_4TAP_8K,
	AX_SRC_TYPE_4TAP_12K,
	AX_SRC_TYPE_4TAP_16K,
	AX_SRC_TYPE_4TAP_AUTO
} AXPBSRCTYPE;

typedef enum {
	AX_MIXER_CTRL_L       = 1 << 0,
	AX_MIXER_CTRL_R       = 1 << 1,
	AX_MIXER_CTRL_DELTA   = 1 << 2,
	AX_MIXER_CTRL_S       = 1 << 3,
	AX_MIXER_CTRL_DELTA_S = 1 << 4,

	AX_MIXER_CTRL_A_L       = 1 << 16,
	AX_MIXER_CTRL_A_R       = 1 << 17,
	AX_MIXER_CTRL_A_DELTA   = 1 << 18,
	AX_MIXER_CTRL_A_S       = 1 << 19,
	AX_MIXER_CTRL_A_DELTA_S = 1 << 20,

	AX_MIXER_CTRL_B_L       = 1 << 21,
	AX_MIXER_CTRL_B_R       = 1 << 22,
	AX_MIXER_CTRL_B_DELTA   = 1 << 23,
	AX_MIXER_CTRL_B_S       = 1 << 24,
	AX_MIXER_CTRL_B_DELTA_S = 1 << 25,

	AX_MIXER_CTRL_C_L       = 1 << 26,
	AX_MIXER_CTRL_C_R       = 1 << 27,
	AX_MIXER_CTRL_C_DELTA   = 1 << 28,
	AX_MIXER_CTRL_C_S       = 1 << 29,
	AX_MIXER_CTRL_C_DELTA_S = 1 << 30
} AXMixerCtrlFlags;

typedef enum {
	AX_MIXER_CTRL_RMT_M0       = 1 << 0,
	AX_MIXER_CTRL_RMT_DELTA_M0 = 1 << 1,
	AX_MIXER_CTRL_RMT_A0       = 1 << 2,
	AX_MIXER_CTRL_RMT_DELTA_A0 = 1 << 3,

	AX_MIXER_CTRL_RMT_M1       = 1 << 4,
	AX_MIXER_CTRL_RMT_DELTA_M1 = 1 << 5,
	AX_MIXER_CTRL_RMT_A1       = 1 << 6,
	AX_MIXER_CTRL_RMT_DELTA_A1 = 1 << 7,

	AX_MIXER_CTRL_RMT_M2       = 1 << 8,
	AX_MIXER_CTRL_RMT_DELTA_M2 = 1 << 9,
	AX_MIXER_CTRL_RMT_A2       = 1 << 10,
	AX_MIXER_CTRL_RMT_DELTA_A2 = 1 << 11,

	AX_MIXER_CTRL_RMT_M3       = 1 << 12,
	AX_MIXER_CTRL_RMT_DELTA_M3 = 1 << 13,
	AX_MIXER_CTRL_RMT_A3       = 1 << 14,
	AX_MIXER_CTRL_RMT_DELTA_A3 = 1 << 15
} AXRmtMixerCtrlFlags;

void AXSetMaxDspCycles(u32 cycles);

#ifdef __cplusplus
}
#endif

#endif
