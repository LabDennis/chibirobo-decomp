#ifndef _REVOLUTION_EXI_H
#define _REVOLUTION_EXI_H

// Wii variant of the EXI API: the Revolution SDK adds EXIWriteReg.
#include "Dolphin/exi.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL EXIWriteReg(s32 chan, u32 dev, u32 exiCmd, void* reg, s32 size);

#ifdef __cplusplus
};
#endif

#endif
