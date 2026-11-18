#include <cstddef>
#include <cstring>
#include <dolphin/base/PPCArch.h>
#include "revolution/gx.h"
#include <dolphin/os.h>

#include "revolution/gx/__gx.h"

static __GXFifoObj CPUFifo;
static __GXFifoObj GPFifo;

static GXBool CPGPLinked;
static OSThread* __GXCurrentThread;
static BOOL GXOverflowSuspendInProgress;
static GXBreakPtCallback BreakPointCB;
static void* __GXCurrentBP;
static u32 __GXOverflowCount;
static GXBool GPFifoReady;
static GXBool CPUFifoReady;

static void __GXFifoReadEnable(void);
static void __GXFifoReadDisable(void);
static void __GXFifoLink(u8 en);
static void __GXWriteFifoIntEnable(u8 hiWatermarkEn, u8 loWatermarkEn);
static void __GXWriteFifoIntReset(u8 hiWatermarkClr, u8 loWatermarkClr);

static void GXOverflowHandler(__OSInterrupt interrupt, OSContext* context) {
    __GXOverflowCount++;
    __GXWriteFifoIntEnable(0, 1);
    __GXWriteFifoIntReset(1, 0);
    GXOverflowSuspendInProgress = TRUE;

    OSSuspendThread(__GXCurrentThread);
}

static void GXUnderflowHandler(s16 interrupt, OSContext* context) {
    OSResumeThread(__GXCurrentThread);
    GXOverflowSuspendInProgress = FALSE;
    __GXWriteFifoIntReset(1, 1);
    __GXWriteFifoIntEnable(1, 0);
}

static void GXBreakPointHandler(__OSInterrupt interrupt, OSContext* context) {
    OSContext exceptionContext;

    SET_REG_FIELD(0, __GXData->cpEnable, 1, 5, 0);
    GX_SET_CP_REG(1, __GXData->cpEnable);
    if (BreakPointCB != NULL) {
        OSClearContext(&exceptionContext);
        OSSetCurrentContext(&exceptionContext);
        BreakPointCB();
        OSClearContext(&exceptionContext);
        OSSetCurrentContext(context);
    }
}

static void GXCPInterruptHandler(__OSInterrupt interrupt, OSContext* context) {
    __GXData->cpStatus = GX_GET_CP_REG(0);
    if (GET_REG_FIELD(__GXData->cpEnable, 1, 3) && GET_REG_FIELD(__GXData->cpStatus, 1, 1)) {
        GXUnderflowHandler(interrupt, context);
    }
    if (GET_REG_FIELD(__GXData->cpEnable, 1, 2) && GET_REG_FIELD(__GXData->cpStatus, 1, 0)) {
        GXOverflowHandler(interrupt, context);
    }
    if (GET_REG_FIELD(__GXData->cpEnable, 1, 5) && GET_REG_FIELD(__GXData->cpStatus, 1, 4)) {
        GXBreakPointHandler(interrupt, context);
    }
}

void GXInitFifoBase(GXFifoObj* fifo, void* base, u32 size) {
    __GXFifoObj* realFifo = (__GXFifoObj*)fifo;

    realFifo->base = base;
    realFifo->top = (u8*)base + size - 4;
    realFifo->size = size;
    realFifo->count = 0;
    GXInitFifoLimits(fifo, size - 0x4000, (size >> 1) & ~0x1F);
    GXInitFifoPtrs(fifo, base, base);
}

void GXInitFifoPtrs(GXFifoObj* fifo, void* readPtr, void* writePtr) {
    __GXFifoObj* realFifo = (__GXFifoObj*)fifo;
    BOOL enabled;

    enabled = OSDisableInterrupts();
    realFifo->rdPtr = readPtr;
    realFifo->wrPtr = writePtr;
    realFifo->count = (u8*)writePtr - (u8*)readPtr;
    if (realFifo->count < 0) {
        realFifo->count += realFifo->size;
    }
    OSRestoreInterrupts(enabled);
}

void GXInitFifoLimits(GXFifoObj* fifo, u32 hiWatermark, u32 loWatermark) {
    __GXFifoObj* realFifo = (__GXFifoObj*)fifo;

    realFifo->hiWatermark = hiWatermark;
    realFifo->loWatermark = loWatermark;
}

#define GX_SET_PI_REG(offset, val) (*(volatile u32*)((volatile u32*)(__piReg) + (offset)) = val)

// The Wii keeps its own copies of both FIFOs; they are linked when they span the same memory.
static GXBool CPGPLinkCheck(void) {
    u32 check = 0;
    BOOL overlap;
    s32 cpuTopToGpBase;
    s32 gpTopToCpuBase;

    if (!CPUFifoReady || !GPFifoReady) {
        return GX_FALSE;
    }

    if (CPUFifo.base == GPFifo.base) {
        check++;
    }

    if (CPUFifo.top == GPFifo.top) {
        check++;
    }

    if (check == 2) {
        return GX_TRUE;
    }

    cpuTopToGpBase = CPUFifo.top - GPFifo.base;
    gpTopToCpuBase = GPFifo.top - CPUFifo.base;
    overlap = FALSE;
    if ((cpuTopToGpBase > 0 && gpTopToCpuBase > 0) || (cpuTopToGpBase < 0 && gpTopToCpuBase < 0)) {
        overlap = TRUE;
    }

    if (overlap) {
        OSReport("CPUFifo: %08X - %08X\n", CPUFifo.base, CPUFifo.top);
        OSReport("GP Fifo: %08X - %08X\n", GPFifo.base, GPFifo.top);
    }

    return GX_FALSE;
}

void GXSetCPUFifo(GXFifoObj* fifo) {
    BOOL enabled = OSDisableInterrupts();

    if (fifo == NULL) {
        CPUFifoReady = GX_FALSE;
        CPGPLinked = GX_FALSE;
        CPUFifo.bind_gp = GX_FALSE;
        CPUFifo.bind_cpu = GX_FALSE;
        OSRestoreInterrupts(enabled);
        return;
    }

    CPUFifo = *(__GXFifoObj*)fifo;
    CPUFifoReady = GX_TRUE;
    CPUFifo.bind_cpu = GX_TRUE;

    if (CPGPLinkCheck()) {
        u32 reg = 0;

        CPGPLinked = GX_TRUE;
        CPUFifo.bind_gp = GX_TRUE;

        GX_SET_PI_REG(3, (u32)CPUFifo.base & 0x3FFFFFFF);
        GX_SET_PI_REG(4, (u32)CPUFifo.top & 0x3FFFFFFF);
        SET_REG_FIELD(0, reg, 24, 5, (u32)CPUFifo.wrPtr >> 5);
        GX_SET_PI_REG(5, reg);

        __GXWriteFifoIntReset(1, 1);
        __GXWriteFifoIntEnable(1, 0);
        __GXFifoLink(1);
    } else {
        u32 reg;

        CPUFifo.bind_gp = GX_FALSE;
        if (CPGPLinked) {
            __GXFifoLink(0);
            CPGPLinked = GX_FALSE;
        }

        __GXWriteFifoIntEnable(0, 0);
        reg = 0;
        GX_SET_PI_REG(3, (u32)CPUFifo.base & 0x3FFFFFFF);
        GX_SET_PI_REG(4, (u32)CPUFifo.top & 0x3FFFFFFF);
        SET_REG_FIELD(0, reg, 24, 5, (u32)CPUFifo.wrPtr >> 5);
        GX_SET_PI_REG(5, reg);
    }

    PPCSync();
    OSRestoreInterrupts(enabled);
}

void GXSetGPFifo(GXFifoObj* fifo) {
    BOOL enabled = OSDisableInterrupts();
    u32 stbtmp;

    __GXFifoReadDisable();
    __GXWriteFifoIntEnable(0, 0);

    if (fifo == NULL) {
        GPFifoReady = GX_FALSE;
        CPGPLinked = GX_FALSE;
        __GXFifoLink(0);
        GPFifo.bind_cpu = GX_FALSE;
        GPFifo.bind_gp = GX_FALSE;
        OSRestoreInterrupts(enabled);
        return;
    }

    GPFifo = *(__GXFifoObj*)fifo;
    GPFifoReady = GX_TRUE;
    GPFifo.bind_gp = GX_TRUE;

    GX_SET_CP_REG(16, (u32)GPFifo.base & 0xFFFF);
    GX_SET_CP_REG(18, (u32)GPFifo.top & 0xFFFF);
    GX_SET_CP_REG(24, GPFifo.count & 0xFFFF);
    GX_SET_CP_REG(26, (u32)GPFifo.wrPtr & 0xFFFF);
    GX_SET_CP_REG(28, (u32)GPFifo.rdPtr & 0xFFFF);
    GX_SET_CP_REG(20, (u32)GPFifo.hiWatermark & 0xFFFF);
    GX_SET_CP_REG(22, (u32)GPFifo.loWatermark & 0xFFFF);
    GX_SET_CP_REG(17, ((u32)GPFifo.base & 0x3FFFFFFF) >> 16);
    GX_SET_CP_REG(19, ((u32)GPFifo.top & 0x3FFFFFFF) >> 16);
    GX_SET_CP_REG(25, GPFifo.count >> 16);
    GX_SET_CP_REG(27, ((u32)GPFifo.wrPtr & 0x3FFFFFFF) >> 16);
    GX_SET_CP_REG(29, ((u32)GPFifo.rdPtr & 0x3FFFFFFF) >> 16);
    GX_SET_CP_REG(21, (u32)GPFifo.hiWatermark >> 16);
    GX_SET_CP_REG(23, (u32)GPFifo.loWatermark >> 16);

    PPCSync();

    if (CPGPLinkCheck()) {
        CPGPLinked = GX_TRUE;
        GPFifo.bind_cpu = GX_TRUE;
        __GXWriteFifoIntEnable(1, 0);
        __GXFifoLink(1);
    } else {
        CPGPLinked = GX_FALSE;
        GPFifo.bind_cpu = GX_FALSE;
        __GXWriteFifoIntEnable(0, 0);
        __GXFifoLink(0);
    }

    stbtmp = __GXData->cpEnable;
    SET_REG_FIELD(0, stbtmp, 1, 1, 0);
    SET_REG_FIELD(0, stbtmp, 1, 5, 0);
    GX_SET_CP_REG(1, stbtmp);
    GX_SET_CP_REG(1, __GXData->cpEnable);
    __GXWriteFifoIntReset(1, 1);
    __GXFifoReadEnable();
    OSRestoreInterrupts(enabled);
}

static void __GXSaveFifo(void) {
    BOOL enabled = OSDisableInterrupts();

    if (CPUFifoReady) {
        u32 reg = GX_GET_PI_REG(5);

        CPUFifo.wrPtr = OSPhysicalToCached(reg & 0x1FFFFFE0);
        CPUFifo.wrap = (reg >> 29) & 1;
    }

    if (GPFifoReady) {
        u32 temp;

        GPFifo.rdPtr = OSPhysicalToCached((GX_GET_CP_REG(29) << 16) | GX_GET_CP_REG(28));

        temp = GX_GET_CP_REG(25) << 16;
        temp |= GX_GET_CP_REG(24);
        GPFifo.count = temp;
    }

    if (CPGPLinked) {
        CPUFifo.rdPtr = GPFifo.rdPtr;
        CPUFifo.count = GPFifo.count;
        GPFifo.wrPtr = CPUFifo.wrPtr;
        GPFifo.wrap = CPUFifo.wrap;
    } else if (CPUFifoReady) {
        CPUFifo.count = (u8*)CPUFifo.wrPtr - (u8*)CPUFifo.rdPtr;
        if (CPUFifo.count < 0) {
            CPUFifo.count += CPUFifo.size;
        }
    }

    OSRestoreInterrupts(enabled);
}

GXBool __GXIsGPFifoReady(void) {
    return GPFifoReady;
}

GXBool GXGetCPUFifo(GXFifoObj* fifo) {
    if (!CPUFifoReady) {
        return GX_FALSE;
    }

    __GXSaveFifo();
    *(__GXFifoObj*)fifo = CPUFifo;
    return GX_TRUE;
}

// NONMATCHING: retail reads both pointers before it stores either, the read pointer first (into r6);
// every order of these locals reads the write pointer first.
void GXGetFifoPtrs(GXFifoObj* fifo, void** readPtr, void** writePtr) {
    __GXFifoObj* realFifo = (__GXFifoObj*)fifo;
    void* rd = realFifo->rdPtr;
    void* wr = realFifo->wrPtr;

    *readPtr = rd;
    *writePtr = wr;
}

GXBreakPtCallback GXSetBreakPtCallback(GXBreakPtCallback cb) {
    GXBreakPtCallback oldcb = BreakPointCB;
    BOOL enabled = OSDisableInterrupts();

    BreakPointCB = cb;
    OSRestoreInterrupts(enabled);
    return oldcb;
}

void GXEnableBreakPt(void* break_pt) {
    BOOL enabled = OSDisableInterrupts();

    __GXFifoReadDisable();
    GX_SET_CP_REG(30, (u32)break_pt);
    GX_SET_CP_REG(31, ((u32)break_pt >> 16) & 0x3FFF);
    SET_REG_FIELD(0, __GXData->cpEnable, 1, 1, 0);
    SET_REG_FIELD(0, __GXData->cpEnable, 1, 5, 0);
    GX_SET_CP_REG(1, __GXData->cpEnable);
    SET_REG_FIELD(0, __GXData->cpEnable, 1, 1, 1);
    SET_REG_FIELD(0, __GXData->cpEnable, 1, 5, 1);
    GX_SET_CP_REG(1, __GXData->cpEnable);
    __GXCurrentBP = break_pt;
    __GXFifoReadEnable();
    OSRestoreInterrupts(enabled);
}

void GXDisableBreakPt(void) {
    BOOL enabled = OSDisableInterrupts();

    SET_REG_FIELD(0, __GXData->cpEnable, 1, 1, 0);
    SET_REG_FIELD(0, __GXData->cpEnable, 1, 5, 0);
    GX_SET_CP_REG(1, __GXData->cpEnable);
    __GXCurrentBP = NULL;
    OSRestoreInterrupts(enabled);
}

void __GXFifoInit(void) {
    __OSSetInterruptHandler(0x11, GXCPInterruptHandler);
    __OSUnmaskInterrupts(0x4000);
    __GXCurrentThread = OSGetCurrentThread();
    GXOverflowSuspendInProgress = FALSE;
    memset(&CPUFifo, 0, sizeof(CPUFifo));
    memset(&GPFifo, 0, sizeof(GPFifo));
    CPUFifoReady = GX_FALSE;
    GPFifoReady = GX_FALSE;
}

static void __GXFifoReadEnable(void) {
    SET_REG_FIELD(0, __GXData->cpEnable, 1, 0, 1);
    GX_SET_CP_REG(1, __GXData->cpEnable);
}

static void __GXFifoReadDisable(void) {
    SET_REG_FIELD(0, __GXData->cpEnable, 1, 0, 0);
    GX_SET_CP_REG(1, __GXData->cpEnable);
}

static void __GXFifoLink(u8 en) {
    SET_REG_FIELD(0, __GXData->cpEnable, 1, 4, (en != 0) ? 1 : 0);
    GX_SET_CP_REG(1, __GXData->cpEnable);
}

static void __GXWriteFifoIntEnable(u8 hiWatermarkEn, u8 loWatermarkEn) {
    SET_REG_FIELD(0, __GXData->cpEnable, 1, 2, hiWatermarkEn);
    SET_REG_FIELD(0, __GXData->cpEnable, 1, 3, loWatermarkEn);
    GX_SET_CP_REG(1, __GXData->cpEnable);
}

static void __GXWriteFifoIntReset(u8 hiWatermarkClr, u8 loWatermarkClr) {
    SET_REG_FIELD(0, __GXData->cpClr, 1, 0, hiWatermarkClr);
    SET_REG_FIELD(0, __GXData->cpClr, 1, 1, loWatermarkClr);
    GX_SET_CP_REG(2, __GXData->cpClr);
}

// Empties the GP FIFO in place: the read pointer catches up with the write pointer.
void __GXCleanGPFifo(void) {
    BOOL enabled;

    if (!GPFifoReady) {
        return;
    }

    enabled = OSDisableInterrupts();
    __GXFifoReadDisable();
    __GXWriteFifoIntEnable(0, 0);

    GPFifo.rdPtr = GPFifo.wrPtr;
    GPFifo.count = 0;

    GX_SET_CP_REG(24, GPFifo.count & 0xFFFF);
    GX_SET_CP_REG(26, (u32)GPFifo.wrPtr & 0xFFFF);
    GX_SET_CP_REG(28, (u32)GPFifo.rdPtr & 0xFFFF);
    GX_SET_CP_REG(25, GPFifo.count >> 16);
    GX_SET_CP_REG(27, ((u32)GPFifo.wrPtr & 0x3FFFFFFF) >> 16);
    GX_SET_CP_REG(29, ((u32)GPFifo.rdPtr & 0x3FFFFFFF) >> 16);

    PPCSync();

    if (CPGPLinked) {
        u32 reg = 0;

        CPUFifo.rdPtr = GPFifo.rdPtr;
        CPUFifo.wrPtr = GPFifo.wrPtr;
        CPUFifo.count = GPFifo.count;
        SET_REG_FIELD(0, reg, 24, 5, (u32)CPUFifo.wrPtr >> 5);
        GX_SET_PI_REG(5, reg);

        __GXWriteFifoIntEnable(1, 0);
        __GXFifoLink(1);
    }

    SET_REG_FIELD(0, __GXData->cpEnable, 1, 1, 0);
    SET_REG_FIELD(0, __GXData->cpEnable, 1, 5, 0);
    GX_SET_CP_REG(1, __GXData->cpEnable);
    __GXCurrentBP = NULL;

    __GXWriteFifoIntReset(1, 1);
    __GXFifoReadEnable();
    OSRestoreInterrupts(enabled);
}
