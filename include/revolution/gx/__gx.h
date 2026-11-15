// The Wii's private GX header: include/sdk/private/gx/__gx.h with the Wii's GXData, which shadows
// the XF channel and texgen registers (0x50 bytes between matColor and suTs0; retail reads
// iref at 0x170, genMode at 0x254 and dirtyState at 0x5FC where the GameCube has 0x120, 0x204, 0x5AC).
#ifndef _REVOLUTION_GX_INTERNAL_H_
#define _REVOLUTION_GX_INTERNAL_H_

#include "revolution/gx.h"
#include "global.h" // IWYU pragma: export

#ifdef __cplusplus
extern "C" {
#endif

// FIFO WRITE

#define GX_WRITE_U8(ub) GXWGFifo.u8 = (u8)(ub)
#define GX_WRITE_U16(us) GXWGFifo.u16 = (u16)(us)
#define GX_WRITE_U32(ui) GXWGFifo.u32 = (u32)(ui)
#define GX_WRITE_F32(f) GXWGFifo.f32 = (f32)(f);

// WRITE REG

#define GX_WRITE_XF_REG(addr, value) \
    do { \
        GX_WRITE_U8(0x10); \
        GX_WRITE_U32(0x1000 + (addr)); \
        GX_WRITE_U32(value); \
    } while (0)

#define GX_WRITE_XF_REG_2(addr, value) \
    do { \
        GX_WRITE_U32(value); \
    } while (0)

#define GX_WRITE_XF_REG_F(addr, value) \
    do { \
        GX_WRITE_F32(value); \
    } while (0)

#define GX_WRITE_RAS_REG(value) \
    do { \
        GX_WRITE_U8(0x61); \
        GX_WRITE_U32(value); \
    } while (0)

#define GX_WRITE_CP_REG(addr, value) \
    do { \
        GX_WRITE_U8(8); \
        GX_WRITE_U8(addr); \
        GX_WRITE_U32(value); \
    } while (0)

// REG MACROS

#define GET_REG_FIELD(reg, size, shift) ((int)((reg) >> (shift)) & ((1 << (size)) - 1))

// TODO: reconcile reg macro differences
// this one is needed to match non GX libs
#define OLD_SET_REG_FIELD(line, reg, size, shift, val) \
do { \
    ASSERTMSGLINE(line, ((u32)(val) & ~((1 << (size)) - 1)) == 0, "GX Internal: Register field out of range"); \
    (reg) = ((u32)(reg) & ~(((1 << (size)) - 1) << (shift))) | ((u32)(val) << (shift)); \
} while (0)

// above doesn't seem to work with GX, only can get it to work with this
#define SET_REG_FIELD(line, reg, size, shift, val) \
do { \
    ASSERTMSGLINE(line, ((u32)(val) & ~((1 << (size)) - 1)) == 0, "GX Internal: Register field out of range"); \
    (reg) = ((u32)__rlwimi((u32)(reg), (val), (shift), 32 - (shift) - (size), 31 - (shift))); \
} while (0)

#define CHECK_GXBEGIN(line, name) ASSERTMSGLINE(line, !__GXinBegin, "'" name "' is not allowed between GXBegin/GXEnd")

/* GXAttr */
void __GXSetVCD(void);
void __GXSetVAT(void);

/* GXBump */
void __GXUpdateBPMask(void);
void __GXFlushTextureState(void);

/* GXFifo */
// GXFifoObj private data
typedef struct __GXFifoObj {
    u8* base;
    u8* top;
    u32 size;
    u32 hiWatermark;
    u32 loWatermark;
    void* rdPtr;
    void* wrPtr;
    s32 count;
    GXBool wrap;
    GXBool bind_cpu;
    GXBool bind_gp;
} __GXFifoObj;

GXBool __GXIsGPFifoReady(void);
void __GXFifoInit(void);
void __GXCleanGPFifo(void);

/* GXGeometry */
void __GXSetDirtyState(void);
void __GXSendFlushPrim(void);
void __GXSetGenMode(void);

/* GXInit */
void __GXInitGX();
void __GXInitRevisionBits(void);

typedef struct __GXData_struct {
    u16 vNumNot;
    u16 bpSentNot;
    u16 vNum;
    u16 vLim;
    u32 cpEnable;
    u32 cpStatus;
    u32 cpClr;
    u32 vcdLo;
    u32 vcdHi;
    u32 vatA[8];
    u32 vatB[8];
    u32 vatC[8];
    u32 lpSize;
    u32 matIdxA;
    u32 matIdxB;
    u32 indexBase[4];
    u32 indexStride[4];
    u32 ambColor[2];
    u32 matColor[2];
    u32 chanCtrl[4];
    u32 texGenCtrl[8];
    u32 dualTexGenCtrl[8];
    u32 suTs0[8];
    u32 suTs1[8];
    u32 suScis0;
    u32 suScis1;
    u32 tref[8];
    u32 iref;
    u32 bpMask;
    u32 IndTexScale0;
    u32 IndTexScale1;
    u32 tevc[16];
    u32 teva[16];
    u32 tevKsel[8];
    u32 cmode0;
    u32 cmode1;
    u32 zmode;
    u32 peCtrl;
    u32 cpDispSrc;
    u32 cpDispSize;
    u32 cpDispStride;
    u32 cpDisp;
    u32 cpTexSrc;
    u32 cpTexSize;
    u32 cpTexStride;
    u32 cpTex;
    u8 cpTexZ;
    u32 genMode;
    GXTexRegion TexRegions0[8];
    GXTexRegion TexRegions1[8];
    GXTexRegion TexRegions2[8];
    GXTlutRegion TlutRegions[20];
    GXTexRegion* (*texRegionCallback)(GXTexObj*, GXTexMapID);
    GXTlutRegion* (*tlutRegionCallback)(u32);
    GXAttrType nrmType;
    u8 hasNrms;
    u8 hasBiNrms;
    u32 projType;
    f32 projMtx[6];
    f32 vpLeft;
    f32 vpTop;
    f32 vpWd;
    f32 vpHt;
    f32 vpNearz;
    f32 vpFarz;
    f32 zOffset;
    f32 zScale;
    u32 tImage0[8];
    u32 tMode0[8];
    u32 texmapId[16];
    u32 tcsManEnab;
    u32 tevTcEnab;
    GXPerf0 perf0;
    GXPerf1 perf1;
    u32 perfSel;
    u8 inDispList;
    u8 dlSaveContext;
    u8 abtWaitPECopy;
    u8 dirtyVAT;
    u32 dirtyState;
} GXData;

extern GXData* const __GXData;
extern void* __memReg;
extern void* __peReg;
extern void* __cpReg;
extern void* __piReg;

#define GX_GET_MEM_REG(offset) (*(volatile u16*)((volatile u16*)(__memReg) + (offset)))
#define GX_GET_CP_REG(offset)  (*(volatile u16*)((volatile u16*)(__cpReg) + (offset)))
#define GX_GET_PE_REG(offset)  (*(volatile u16*)((volatile u16*)(__peReg) + (offset)))
#define GX_GET_PI_REG(offset)  (*(volatile u32*)((volatile u32*)(__piReg) + (offset)))

#define GX_SET_MEM_REG(offset, val) (*(volatile u16*)((volatile u16*)(__memReg) + (offset)) = val)
#define GX_SET_CP_REG(offset, val)  (*(volatile u16*)((volatile u16*)(__cpReg) + (offset)) = val)
#define GX_SET_PE_REG(offset, val)  (*(volatile u16*)((volatile u16*)(__peReg) + (offset)) = val)
#define GX_SET_PI_REG(offset, val)  (*(volatile u32*)((volatile u32*)(__piReg) + (offset)) = val)

/* GXMisc */
void __GXBypass(u32 reg);
u16 __GXReadPEReg(u32 reg);
void __GXPEInit(void);
void __GXAbort();

/* GXPerf */
void __GXSetBWDials(u16 cpDial, u16 tcDial, u16 peDial, u16 cpuRdDial, u16 cpuWrDial);

static inline u32 __GXReadCPCounterU32(u32 regAddrL, u32 regAddrH) {
    u32 ctrH0;
    u32 ctrH1;
    u32 ctrL;

    ctrH0 = GX_GET_CP_REG(regAddrH);

    do {
        ctrH1 = ctrH0;
        ctrL = GX_GET_CP_REG(regAddrL);
        ctrH0 = GX_GET_CP_REG(regAddrH);
    } while (ctrH0 != ctrH1);

    return (ctrH0 << 0x10) | ctrL;
}

static inline u32 __GXReadMEMCounterU32(u32 regAddrL, u32 regAddrH) {
    u32 ctrH0;
    u32 ctrH1;
    u32 ctrL;

    ctrH0 = GX_GET_MEM_REG(regAddrH);

    do {
        ctrH1 = ctrH0;
        ctrL = GX_GET_MEM_REG(regAddrL);
        ctrH0 = GX_GET_MEM_REG(regAddrH);
    } while (ctrH0 != ctrH1);

    return (ctrH0 << 0x10) | ctrL;
}

static inline u32 __GXReadPECounterU32(u32 regAddrL, u32 regAddrH) {
    u32 ctrH0;
    u32 ctrH1;
    u32 ctrL;

    ctrH0 = GX_GET_PE_REG(regAddrH);

    do {
        ctrH1 = ctrH0;
        ctrL = GX_GET_PE_REG(regAddrL);
        ctrH0 = GX_GET_PE_REG(regAddrH);
    } while (ctrH0 != ctrH1);

    return (ctrH0 << 0x10) | ctrL;
}

/* GXSave */
void __GXShadowDispList(void* list, u32 nbytes);
void __GXShadowIndexState(u32 idx_reg, u32 reg_data);
void __GXPrintShadowState(void);

/* GXStubs */
void __GXSetRange(f32 nearz, f32 fgSideX);

/* GXTexture */
void __GetImageTileCount(GXTexFmt fmt, u16 wd, u16 ht, u32* rowTiles, u32* colTiles, u32* cmpTiles);
void __GXSetSUTexRegs(void);
void __GXGetSUTexSize(GXTexCoordID coord, u16* width, u16* height);
void __GXSetTmemConfig(u32 config);

/* GXTransform */
void __GXSetMatrixIndex(GXAttr matIdxAttr);
void __GXSetProjection(void);
void __GXSetViewport();


#ifdef __cplusplus
}
#endif

#endif
