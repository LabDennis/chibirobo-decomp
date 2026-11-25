#ifndef DOLPHIN_EXI_STATE_H
#define DOLPHIN_EXI_STATE_H

// Software state bits returned by EXIGetState.
#define EXI_STATE_IDLE     0x00
#define EXI_STATE_DMA      0x01
#define EXI_STATE_IMM      0x02
#define EXI_STATE_BUSY     (EXI_STATE_DMA | EXI_STATE_IMM)
#define EXI_STATE_SELECTED 0x04
#define EXI_STATE_ATTACHED 0x08
#define EXI_STATE_LOCKED   0x10

#endif
