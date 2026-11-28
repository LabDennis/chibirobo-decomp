#ifndef _REVOLUTION_EXI_H
#define _REVOLUTION_EXI_H

// Wii variant of the EXI API: the Revolution SDK adds EXIWriteReg. It also
// defines the Dolphin hw_regs guard so the GameCube register addresses are
// skipped when this header is included first in a Wii unit.
#ifndef _DOLPHIN_HW_REGS_H
#define _DOLPHIN_HW_REGS_H
#endif

#include "revolution/hw_regs.h"
#include "Dolphin/exi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EXI_REG_MAX 5

#define EXI_0CR(tstart, dma, rw, tlen) ((((u32)(tstart)) << 0) | (((u32)(dma)) << 1) | (((u32)(rw)) << 2) | (((u32)(tlen)) << 4))

#define EXIREG(chan, idx) (__EXIRegs[((chan) * EXI_REG_MAX) + (idx)])

extern const u32 __EXIFreq;

#define EXI_CPR_CS(x)  ((1u << (x)) << 7)
#define EXI_CPR_CLK(x) ((x) << 4)

BOOL EXIWriteReg(s32 chan, u32 dev, u32 exiCmd, void* reg, s32 size);

#ifdef __cplusplus
};
#endif

#endif
