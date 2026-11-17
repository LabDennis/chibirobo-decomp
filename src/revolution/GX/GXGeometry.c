#include <cstddef>
#include "revolution/gx.h"
#include <dolphin/os.h>

#include "revolution/gx/__gx.h"

// NONMATCHING: retail holds the channel and texgen bit masks in r7 and r8 and the register address in
// r6; here the mask takes r6 and the address r7 (same instructions). Inline helpers per group do not help.
void __GXSetDirtyState(void) {
    u32 dState = __GXData->dirtyState;

    if (dState & 1) {
        __GXSetSUTexRegs();
    }
    if (dState & 2) {
        __GXUpdateBPMask();
    }
    if (dState & 4) {
        __GXSetGenMode();
    }
    if (dState & 8) {
        __GXSetVCD();
    }
    if (dState & 0x10) {
        __GXSetVAT();
    }
    if (dState & 0x18) {
        __GXCalculateVLim();
    }

    // the Wii shadows the XF colour, channel and texgen registers in GXData and sends the dirty ones here
    dState &= 0xFFFFFF00;
    if (dState != 0) {
        u32 dirty;
        u32 i;

        dirty = dState & 0x00000F00;
        if (dirty != 0) {
            if (dirty & 0x00000100) {
                GX_WRITE_XF_REG(0xA, __GXData->ambColor[0]);
            }
            if (dirty & 0x00000200) {
                GX_WRITE_XF_REG(0xB, __GXData->ambColor[1]);
            }
            if (dirty & 0x00000400) {
                GX_WRITE_XF_REG(0xC, __GXData->matColor[0]);
            }
            if (dirty & 0x00000800) {
                GX_WRITE_XF_REG(0xD, __GXData->matColor[1]);
            }
        }

        dirty = dState & 0x0100F000;
        if (dirty != 0) {
            u32 addr = 0x100E;

            if (dirty & 0x01000000) {
                u32 nChans = GET_REG_FIELD(__GXData->genMode, 3, 4);

                GX_WRITE_XF_REG(9, nChans);
            }

            dirty = (dirty >> 12) & 0xF;
            i = 0;
            while (dirty != 0) {
                if (dirty & 1) {
                    GX_WRITE_U8(0x10);
                    GX_WRITE_U32(addr);
                    GX_WRITE_U32(__GXData->chanCtrl[i]);
                }
                dirty >>= 1;
                i++;
                addr++;
            }
        }

        dirty = dState & 0x02FF0000;
        if (dirty != 0) {
            u32 addr = 0x1040;

            if (dirty & 0x02000000) {
                u32 nTexGens = GET_REG_FIELD(__GXData->genMode, 4, 0);

                GX_WRITE_XF_REG(0x3F, nTexGens);
            }

            dirty = (dirty >> 16) & 0xFF;
            i = 0;
            while (dirty != 0) {
                u32 dualAddr = addr + 0x10;

                if (dirty & 1) {
                    GX_WRITE_U8(0x10);
                    GX_WRITE_U32(addr);
                    GX_WRITE_U32(__GXData->texGenCtrl[i]);
                    GX_WRITE_U8(0x10);
                    GX_WRITE_U32(dualAddr);
                    GX_WRITE_U32(__GXData->dualTexGenCtrl[i]);
                }
                dirty >>= 1;
                addr++;
                i++;
            }
        }

        if (dState & 0x04000000) {
            __GXSetMatrixIndex(GX_VA_PNMTXIDX);
            __GXSetMatrixIndex(GX_VA_TEX4MTXIDX);
        }
        if (dState & 0x10000000) {
            __GXSetViewport();
        }
        if (dState & 0x08000000) {
            __GXSetProjection();
        }

        __GXData->bpSentNot = 1;
    }

    __GXData->dirtyState = 0;
}

void GXBegin(GXPrimitive type, GXVtxFmt vtxfmt, u16 nverts) {
    if (__GXData->dirtyState != 0) {
        __GXSetDirtyState();
    }

    if (*(u32*)&__GXData->vNumNot == 0) {  // checks both vNum and bpSentNot
        __GXSendFlushPrim();
    }
    GX_WRITE_U8(vtxfmt | type);
    GX_WRITE_U16(nverts);
}

void __GXSendFlushPrim(void) {
    u32 i;
    u32 numD = __GXData->vNum * __GXData->vLim;

    GX_WRITE_U8(0x98);
    GX_WRITE_U16(__GXData->vNum);
    for (i = 0; i < numD; i += 4) {
        GX_WRITE_U32(0);
    }
    __GXData->bpSentNot = 1;
}

void GXSetLineWidth(u8 width, GXTexOffset texOffsets) {
    SET_REG_FIELD(0, __GXData->lpSize, 8, 0, width);
    SET_REG_FIELD(0, __GXData->lpSize, 3, 16, texOffsets);
    GX_WRITE_RAS_REG(__GXData->lpSize);
    __GXData->bpSentNot = 0;
}

void GXSetPointSize(u8 pointSize, GXTexOffset texOffsets) {
    SET_REG_FIELD(0, __GXData->lpSize, 8, 8, pointSize);
    SET_REG_FIELD(0, __GXData->lpSize, 3, 19, texOffsets);
    GX_WRITE_RAS_REG(__GXData->lpSize);
    __GXData->bpSentNot = 0;
}

void GXEnableTexOffsets(GXTexCoordID coord, u8 line_enable, u8 point_enable) {
    SET_REG_FIELD(0, __GXData->suTs0[coord], 1, 18, line_enable);
    SET_REG_FIELD(0, __GXData->suTs0[coord], 1, 19, point_enable);
    GX_WRITE_RAS_REG(__GXData->suTs0[coord]);
    __GXData->bpSentNot = 0;
}

void GXSetCullMode(GXCullMode mode) {
    // front and back swap places for the hardware
    GXCullMode hwMode = (GXCullMode)(mode << 1 & 2 | mode >> 1 & 1);

    SET_REG_FIELD(0, __GXData->genMode, 2, 14, hwMode);
    __GXData->dirtyState |= 4;
}

void GXSetCoPlanar(GXBool enable) {
    u32 reg;

    SET_REG_FIELD(0, __GXData->genMode, 1, 19, enable);
    reg = 0xFE080000;
    GX_WRITE_RAS_REG(reg);
    GX_WRITE_RAS_REG(__GXData->genMode);
    __GXData->bpSentNot = 0;
}

void __GXSetGenMode(void) {
    GX_WRITE_RAS_REG(__GXData->genMode);
    __GXData->bpSentNot = 0;
}
