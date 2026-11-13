#ifndef _REVOSDK_AX_H
#define _REVOSDK_AX_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { AX_OUTPUT_STEREO, AX_OUTPUT_SURROUND, AX_OUTPUT_DPL2 } AXOutputMode;

u32 AXGetMode(void);

#ifdef __cplusplus
}
#endif

#endif
