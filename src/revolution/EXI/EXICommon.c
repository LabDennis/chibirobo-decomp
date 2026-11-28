#include "revolution/exi.h"

const u32 __EXIFreq = EXI_FREQ_16M;

BOOL EXIWriteReg(s32 chan, u32 dev, u32 exiCmd, void* reg, s32 size) {
    BOOL err;
    u32 regval;

    switch (size) {
    case 1:
        regval = (*(u8*)reg & 0xFF) << 24;
        break;
    case 2:
        regval = ((*(u16*)reg & 0xFF) << 24) | ((*(u16*)reg & 0xFF00) << 8);
        break;
    default:
        regval = ((*(u32*)reg >> 24) & 0xFF) | ((*(u32*)reg >> 8) & 0xFF00)
               | ((*(u32*)reg & 0xFF00) << 8) | ((*(u32*)reg & 0xFF) << 24);
        break;
    }

    err = !EXILock(chan, dev, NULL);
    if (err) {
        return FALSE;
    }

    err |= !EXISelect(chan, dev, EXI_FREQ_16M);
    if (err) {
        EXIUnlock(chan);
        return FALSE;
    }

    err |= !EXIImm(chan, &exiCmd, sizeof(exiCmd), EXI_WRITE, NULL);
    err |= !EXISync(chan);
    err |= !EXIImm(chan, &regval, sizeof(regval), EXI_WRITE, NULL);
    err |= !EXISync(chan);
    err |= !EXIDeselect(chan);
    err |= !EXIUnlock(chan);

    return !err;
}
