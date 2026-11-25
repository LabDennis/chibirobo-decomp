#include <cstddef>
#include <dolphin/base/PPCArch.h>
#include <dolphin/gx.h>
#include <dolphin/os.h>
#include <private/gx/GXCPRegs.h>

#include "__gx.h"

static __GXFifoObj* CPUFifo;
static __GXFifoObj* GPFifo;

static OSThread* __GXCurrentThread;
static GXBool CPGPLinked;
static BOOL GXOverflowSuspendInProgress;
static GXBreakPtCallback BreakPointCB;
static u32 __GXOverflowCount;

#if DEBUG
static BOOL IsWGPipeRedirected;
#endif

void* __GXCurrentBP;

static void __GXFifoReadEnable(void);
static void __GXFifoReadDisable(void);
static void __GXFifoLink(u8 en);
static void __GXWriteFifoIntEnable(u8 hiWatermarkEn, u8 loWatermarkEn);
static void __GXWriteFifoIntReset(u8 hiWatermarkClr, u8 loWatermarkClr);

#if DEBUG
static char __data_0[] = "[GXOverflowHandler]";
#endif

/**
 * @note Address: N/A
 * @note Size: ?
 */
static void GXOverflowHandler(__OSInterrupt interrupt, OSContext* context) {
#if DEBUG
    if (__gxVerif->verifyLevel > GX_WARN_SEVERE) {
        OSReport(__data_0);
    }
#endif
    ASSERTLINE(LINE(377, 377, 381), !GXOverflowSuspendInProgress);

    __GXOverflowCount++;
    __GXWriteFifoIntEnable(0, 1);
    __GXWriteFifoIntReset(1, 0);
    GXOverflowSuspendInProgress = TRUE;

#if DEBUG
    if (__gxVerif->verifyLevel > GX_WARN_SEVERE) {
        OSReport("[GXOverflowHandler Sleeping]");
    }
#endif
    OSSuspendThread(__GXCurrentThread);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static void GXUnderflowHandler(s16 interrupt, OSContext* context) {
#if DEBUG
    if (__gxVerif->verifyLevel > GX_WARN_SEVERE) {
        OSReport("[GXUnderflowHandler]");
    }
#endif
    ASSERTLINE(LINE(419, 419, 423), GXOverflowSuspendInProgress);

    OSResumeThread(__GXCurrentThread);
    GXOverflowSuspendInProgress = FALSE;
    __GXWriteFifoIntReset(1, 1);
    __GXWriteFifoIntEnable(1, 0);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static void GXBreakPointHandler(__OSInterrupt interrupt, OSContext* context) {
    OSContext exceptionContext;

    SET_REG_FIELD(0, __GXData->cpEnable, 1, GX_CP_ENABLE_BREAKPOINT_INT_SHIFT, 0);
    GX_SET_CP_REG(GX_CP_REG_ENABLE, __GXData->cpEnable);
    if (BreakPointCB != NULL) {
        OSClearContext(&exceptionContext);
        OSSetCurrentContext(&exceptionContext);
        BreakPointCB();
        OSClearContext(&exceptionContext);
        OSSetCurrentContext(context);
    }
}

/**
 * @note Address: 0x80185808
 * @note Size: 0x134
 */
static void GXCPInterruptHandler(__OSInterrupt interrupt, OSContext* context) {
    __GXData->cpStatus = GX_GET_CP_REG(GX_CP_REG_STATUS);
    if (GET_REG_FIELD(__GXData->cpEnable, 1, GX_CP_ENABLE_UNDERFLOW_INT_SHIFT)
        && GET_REG_FIELD(__GXData->cpStatus, 1, GX_CP_STATUS_UNDERFLOW_SHIFT)) {
        GXUnderflowHandler(interrupt, context);
    }
    if (GET_REG_FIELD(__GXData->cpEnable, 1, GX_CP_ENABLE_OVERFLOW_INT_SHIFT)
        && GET_REG_FIELD(__GXData->cpStatus, 1, GX_CP_STATUS_OVERFLOW_SHIFT)) {
        GXOverflowHandler(interrupt, context);
    }
    if (GET_REG_FIELD(__GXData->cpEnable, 1, GX_CP_ENABLE_BREAKPOINT_INT_SHIFT)
        && GET_REG_FIELD(__GXData->cpStatus, 1, GX_CP_STATUS_BREAKPOINT_SHIFT)) {
        GXBreakPointHandler(interrupt, context);
    }
}

/**
 * @note Address: 0x8018593C
 * @note Size: 0x6C
 */
void GXInitFifoBase(GXFifoObj* fifo, void* base, u32 size) {
    __GXFifoObj* realFifo = (__GXFifoObj*)fifo;

    ASSERTMSGLINE(LINE(542, 542, 546), realFifo != CPUFifo,     "GXInitFifoBase: fifo is attached to CPU");
    ASSERTMSGLINE(LINE(544, 544, 548), realFifo != GPFifo,      "GXInitFifoBase: fifo is attached to GP");
    ASSERTMSGLINE(LINE(546, 546, 550), ((u32)base & 0x1F) == 0, "GXInitFifoBase: base must be 32B aligned");
    ASSERTMSGLINE(LINE(548, 548, 552), base != NULL,            "GXInitFifoBase: base pointer is NULL");
    ASSERTMSGLINE(LINE(550, 550, 554), (size & 0x1F) == 0,      "GXInitFifoBase: size must be 32B aligned");
    ASSERTMSGLINE(LINE(552, 552, 556), size >= 0x10000,         "GXInitFifoBase: fifo is not large enough");

    realFifo->base = base;
    realFifo->top = (u8*)base + size - 4;
    realFifo->size = size;
    realFifo->count = 0;
    GXInitFifoLimits(fifo, size - 0x4000, (size >> 1) & ~0x1F);
    GXInitFifoPtrs(fifo, base, base);
}

/**
 * @note Address: 0x801859A8
 * @note Size: 0x70
 */
void GXInitFifoPtrs(GXFifoObj* fifo, void* readPtr, void* writePtr) {
    __GXFifoObj* realFifo = (__GXFifoObj *)fifo;
    BOOL enabled;

    ASSERTMSGLINE(LINE(592, 592, 596), realFifo != CPUFifo,         "GXInitFifoPtrs: fifo is attached to CPU");
    ASSERTMSGLINE(LINE(594, 594, 598), realFifo != GPFifo,          "GXInitFifoPtrs: fifo is attached to GP");
    ASSERTMSGLINE(LINE(596, 596, 600), ((u32)readPtr & 0x1F) == 0,  "GXInitFifoPtrs: readPtr not 32B aligned");
    ASSERTMSGLINE(LINE(598, 598, 602), ((u32)writePtr & 0x1F) == 0, "GXInitFifoPtrs: writePtr not 32B aligned");
    ASSERTMSGLINE(LINE(601, 601, 605), realFifo->base <= readPtr && readPtr < realFifo->top,   "GXInitFifoPtrs: readPtr not in fifo range");
    ASSERTMSGLINE(LINE(604, 604, 608), realFifo->base <= writePtr && writePtr < realFifo->top, "GXInitFifoPtrs: writePtr not in fifo range");

    enabled = OSDisableInterrupts();
    realFifo->rdPtr = readPtr;
    realFifo->wrPtr = writePtr;
    realFifo->count = (u8*)writePtr - (u8*)readPtr;
    if (realFifo->count < 0) {
        realFifo->count += realFifo->size;
    }
    OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x80185A18
 * @note Size: 0xC
 */
void GXInitFifoLimits(GXFifoObj* fifo, u32 hiWatermark, u32 loWatermark) {
    __GXFifoObj* realFifo = (__GXFifoObj*)fifo;

    ASSERTMSGLINE(LINE(641, 641, 645), realFifo != GPFifo,        "GXInitFifoLimits: fifo is attached to GP");
    ASSERTMSGLINE(LINE(643, 643, 647), (hiWatermark & 0x1F) == 0, "GXInitFifoLimits: hiWatermark not 32B aligned");
    ASSERTMSGLINE(LINE(645, 645, 649), (loWatermark & 0x1F) == 0, "GXInitFifoLimits: loWatermark not 32B aligned");
    ASSERTMSGLINE(LINE(647, 647, 651), hiWatermark < realFifo->top - realFifo->base, "GXInitFifoLimits: hiWatermark too large");
    ASSERTMSGLINE(LINE(649, 649, 653), loWatermark < hiWatermark, "GXInitFifoLimits: hiWatermark below lo watermark");

    realFifo->hiWatermark = hiWatermark;
    realFifo->loWatermark = loWatermark;
}

#define GX_SET_PI_REG(offset, val) (*(volatile u32*)((volatile u32*)(__piReg) + (offset)) = val)

// NONMATCHING DEBUG
/**
 * @note Address: 0x80185A24
 * @note Size: 0x128
 */
void GXSetCPUFifo(GXFifoObj* fifo) {
    __GXFifoObj* realFifo = (__GXFifoObj*)fifo;
    BOOL enabled = OSDisableInterrupts();

    CPUFifo = realFifo;
    if (CPUFifo == GPFifo) {
        u32 reg = 0;

        GX_SET_PI_REG(3, (u32)realFifo->base & 0x3FFFFFFF);
        GX_SET_PI_REG(4, (u32)realFifo->top & 0x3FFFFFFF);

        SET_REG_FIELD(LINE(691, 691, 695), reg, 21, 5, (u32)realFifo->wrPtr >> 5);
        SET_REG_FIELD(LINE(691, 691, 695), reg, 1, 26, 0);
        GX_SET_PI_REG(5, reg);

        CPGPLinked = GX_TRUE;

        __GXWriteFifoIntReset(1, 1);
        __GXWriteFifoIntEnable(1, 0);
        __GXFifoLink(1);
    } else {
        u32 reg;

        if (CPGPLinked) {
            __GXFifoLink(0);
            CPGPLinked = GX_FALSE;
        }

        __GXWriteFifoIntEnable(0, 0);
        reg = 0;
        GX_SET_PI_REG(3, (u32)realFifo->base & 0x3FFFFFFF);
        GX_SET_PI_REG(4, (u32)realFifo->top & 0x3FFFFFFF);
        SET_REG_FIELD(LINE(726, 726, 730), reg, 21, 5, (u32)realFifo->wrPtr >> 5);
        SET_REG_FIELD(LINE(726, 726, 730), reg, 1, 26, 0);
        GX_SET_PI_REG(5, reg);
    }

    PPCSync();
    OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x80185B4C
 * @note Size: 0x1A0
 */
void GXSetGPFifo(GXFifoObj* fifo) {
    __GXFifoObj* realFifo = (__GXFifoObj*)fifo;
    BOOL enabled = OSDisableInterrupts();
#if SDK_REVISION >= 2
    u32 cpEnable;
#endif

    __GXFifoReadDisable();
    __GXWriteFifoIntEnable(0, 0);
    GPFifo = realFifo;

    GX_SET_CP_REG(GX_CP_REG_FIFO_BASE_LO, (u32)realFifo->base & 0xFFFF);
    GX_SET_CP_REG(GX_CP_REG_FIFO_END_LO, (u32)realFifo->top & 0xFFFF);
    GX_SET_CP_REG(GX_CP_REG_FIFO_COUNT_LO, realFifo->count & 0xFFFF);
    GX_SET_CP_REG(GX_CP_REG_FIFO_WRITE_PTR_LO, (u32)realFifo->wrPtr & 0xFFFF);
    GX_SET_CP_REG(GX_CP_REG_FIFO_READ_PTR_LO, (u32)realFifo->rdPtr & 0xFFFF);
    GX_SET_CP_REG(GX_CP_REG_FIFO_HIGH_WATERMARK_LO, (u32)realFifo->hiWatermark & 0xFFFF);
    GX_SET_CP_REG(GX_CP_REG_FIFO_LOW_WATERMARK_LO, (u32)realFifo->loWatermark & 0xFFFF);
    GX_SET_CP_REG(GX_CP_REG_FIFO_BASE_HI, ((u32)realFifo->base & 0x3FFFFFFF) >> 16);
    GX_SET_CP_REG(GX_CP_REG_FIFO_END_HI, ((u32)realFifo->top & 0x3FFFFFFF) >> 16);
    GX_SET_CP_REG(GX_CP_REG_FIFO_COUNT_HI, realFifo->count >> 16);
    GX_SET_CP_REG(GX_CP_REG_FIFO_WRITE_PTR_HI, ((u32)realFifo->wrPtr & 0x3FFFFFFF) >> 16);
    GX_SET_CP_REG(GX_CP_REG_FIFO_READ_PTR_HI, ((u32)realFifo->rdPtr & 0x3FFFFFFF) >> 16);
    GX_SET_CP_REG(GX_CP_REG_FIFO_HIGH_WATERMARK_HI, (u32)realFifo->hiWatermark >> 16);
    GX_SET_CP_REG(GX_CP_REG_FIFO_LOW_WATERMARK_HI, (u32)realFifo->loWatermark >> 16);

    PPCSync();

    if (CPUFifo == GPFifo) {
        CPGPLinked = GX_TRUE;
        __GXWriteFifoIntEnable(1, 0);
        __GXFifoLink(1);
    } else {
        CPGPLinked = GX_FALSE;
        __GXWriteFifoIntEnable(0, 0);
        __GXFifoLink(0);
    }

#if SDK_REVISION >= 2
    cpEnable = __GXData->cpEnable;
    SET_REG_FIELD(0, cpEnable, 1, GX_CP_ENABLE_BREAKPOINT_SHIFT, 0);
    SET_REG_FIELD(0, cpEnable, 1, GX_CP_ENABLE_BREAKPOINT_INT_SHIFT, 0);
    GX_SET_CP_REG(GX_CP_REG_ENABLE, cpEnable);
    GX_SET_CP_REG(GX_CP_REG_ENABLE, __GXData->cpEnable);
#endif
    __GXWriteFifoIntReset(1, 1);
    __GXFifoReadEnable();
    OSRestoreInterrupts(enabled);
}

static inline void __GXReadFifoCPState(__GXFifoObj* fifo) {
    u32 readAddress = (u32)GX_GET_CP_REG(GX_CP_REG_FIFO_READ_PTR_HI) << 16;
    u32 count;

    readAddress |= GX_GET_CP_REG(GX_CP_REG_FIFO_READ_PTR_LO);
    fifo->rdPtr = OSPhysicalToCached(readAddress);

    count = (u32)GX_GET_CP_REG(GX_CP_REG_FIFO_COUNT_HI) << 16;
    count |= GX_GET_CP_REG(GX_CP_REG_FIFO_COUNT_LO);
    fifo->count = count;
}

#if SDK_REVISION >= 2
/**
 * @note Address: N/A
 * @note Size: ?
 */
static void __GXSaveFifoCPStat(__GXFifoObj* realFifo) {
    u32 cpStatus;
    u8 readIdle;

#if DEBUG
    if (__gxVerif->verifyLevel != GX_WARN_NONE) {
        cpStatus = GX_GET_CP_REG(GX_CP_REG_STATUS);
        readIdle = cpStatus & ((1 << GX_CP_STATUS_READ_IDLE_SHIFT) | (1 << GX_CP_STATUS_BREAKPOINT_SHIFT));
        ASSERTMSGLINE(856, readIdle, "GXSaveGPFifo: GP is not idle");
    }
#endif

    __GXReadFifoCPState(realFifo);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static void __GXSaveFifoPIStat(__GXFifoObj* realFifo) {
    realFifo->base = OSPhysicalToCached(GX_GET_PI_REG(3));
    realFifo->top = OSPhysicalToCached(GX_GET_PI_REG(4));
    realFifo->wrPtr = OSPhysicalToCached(GX_GET_PI_REG(5) & 0xFBFFFFFF);
}
#endif

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXSaveCPUFifo(GXFifoObj* fifo) {
    __GXFifoObj* realFifo = (__GXFifoObj*)fifo;
    ASSERTMSGLINE(LINE(835, 835, 900), realFifo == CPUFifo, "GXSaveCPUFifo: fifo is not attached to CPU");
    GXFlush();
    __GXSaveCPUFifoAux(realFifo);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void __GXSaveCPUFifoAux(__GXFifoObj* realFifo) {
    BOOL enabled = OSDisableInterrupts();

#if SDK_REVISION < 2
    realFifo->base = OSPhysicalToCached(GX_GET_PI_REG(3));
    realFifo->top = OSPhysicalToCached(GX_GET_PI_REG(4));
    realFifo->wrPtr = OSPhysicalToCached(GX_GET_PI_REG(5) & 0xFBFFFFFF);
#else
    __GXSaveFifoPIStat(realFifo);
#endif

    if (CPGPLinked) {
#if SDK_REVISION < 2
        __GXReadFifoCPState(realFifo);
#else
        __GXSaveFifoCPStat(realFifo);
#endif
    } else {
        realFifo->count = (u8*)realFifo->wrPtr - (u8*)realFifo->rdPtr;
        if (realFifo->count < 0)
            realFifo->count += realFifo->size;
    }
    OSRestoreInterrupts(enabled);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXSaveGPFifo(GXFifoObj* fifo) {
    __GXFifoObj* realFifo = (__GXFifoObj*)fifo;
#if SDK_REVISION < 2
    u32 cpStatus;
    u8 readIdle;
#else
    BOOL enabled = OSDisableInterrupts();
#endif

#if SDK_REVISION < 2
    ASSERTMSGLINE(908, realFifo == GPFifo, "GXSaveGPFifo: fifo is not attached to GP");
    cpStatus = ((u16*)__cpReg)[GX_CP_REG_STATUS];
    readIdle = GET_REG_FIELD(cpStatus, 1, GX_CP_STATUS_READ_IDLE_SHIFT);
    ASSERTMSGLINE(915, readIdle, "GXSaveGPFifo: GP is not idle");

    __GXReadFifoCPState(realFifo);
#else
    __GXSaveFifoCPStat(realFifo);
    if (CPGPLinked) {
        __GXSaveFifoPIStat(realFifo);
    }
    OSRestoreInterrupts(enabled);
#endif
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXGetGPStatus(GXBool* overhi, GXBool* underlow, GXBool* readIdle, GXBool* cmdIdle, GXBool* brkpt) {
    __GXData->cpStatus = GX_GET_CP_REG(GX_CP_REG_STATUS);
    *overhi   = GET_REG_FIELD(__GXData->cpStatus, 1, GX_CP_STATUS_OVERFLOW_SHIFT);
    *underlow = GET_REG_FIELD(__GXData->cpStatus, 1, GX_CP_STATUS_UNDERFLOW_SHIFT);
    *readIdle = GET_REG_FIELD(__GXData->cpStatus, 1, GX_CP_STATUS_READ_IDLE_SHIFT);
    *cmdIdle  = GET_REG_FIELD(__GXData->cpStatus, 1, GX_CP_STATUS_COMMAND_IDLE_SHIFT);
    *brkpt    = GET_REG_FIELD(__GXData->cpStatus, 1, GX_CP_STATUS_BREAKPOINT_SHIFT);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXGetFifoStatus(GXFifoObj* fifo, GXBool* overhi, GXBool* underflow, u32* fifoCount, GXBool* cpuWrite, GXBool* gpRead, GXBool* fifowrap) {
    __GXFifoObj* realFifo = (__GXFifoObj*)fifo;

    *underflow = GX_FALSE;
    *overhi    = GX_FALSE;
    *fifoCount = 0;
    *fifowrap  = GX_FALSE;

    if (realFifo == GPFifo) {
        __GXReadFifoCPState(realFifo);
    }

    if (realFifo == CPUFifo) {
        GXFlush();
        __GXSaveCPUFifoAux(realFifo);
        *fifowrap = (int)GET_REG_FIELD(GX_GET_PI_REG(5), 1, 26);
    }

    *overhi    = (realFifo->count > realFifo->hiWatermark);
    *underflow = (realFifo->count < realFifo->loWatermark);
    *fifoCount = (realFifo->count);
    *cpuWrite  = (CPUFifo == realFifo);
    *gpRead    = (GPFifo == realFifo);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXGetFifoPtrs(GXFifoObj* fifo, void** readPtr, void** writePtr) {
    __GXFifoObj* realFifo = (__GXFifoObj*)fifo;

    if (realFifo == CPUFifo) {
        realFifo->wrPtr = OSPhysicalToCached(GX_GET_PI_REG(5) & 0xFBFFFFFF);
    }

    if (realFifo == GPFifo) {
        __GXReadFifoCPState(realFifo);
    } else {
        realFifo->count = (u8*)realFifo->wrPtr - (u8*)realFifo->rdPtr;
        if (realFifo->count < 0) {
            realFifo->count += realFifo->size;
        }
    }

    *readPtr = realFifo->rdPtr;
    *writePtr = realFifo->wrPtr;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void* GXGetFifoBase(const GXFifoObj* fifo) {
    __GXFifoObj* realFifo = (__GXFifoObj*)fifo;

    return realFifo->base;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
u32 GXGetFifoSize(const GXFifoObj* fifo) {
    __GXFifoObj* realFifo = (__GXFifoObj*)fifo;

    return realFifo->size;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXGetFifoLimits(const GXFifoObj* fifo, u32* hi, u32* lo) {
    __GXFifoObj* realFifo = (__GXFifoObj*)fifo;

    *hi = realFifo->hiWatermark;
    *lo = realFifo->loWatermark;
}

/**
 * @note Address: 0x80185CEC
 * @note Size: 0x44
 */
GXBreakPtCallback GXSetBreakPtCallback(GXBreakPtCallback cb) {
    GXBreakPtCallback oldcb = BreakPointCB;
    BOOL enabled = OSDisableInterrupts();

    BreakPointCB = cb;
    OSRestoreInterrupts(enabled);
    return oldcb;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXEnableBreakPt(void* break_pt) {
    BOOL enabled = OSDisableInterrupts();

    __GXFifoReadDisable();
    GX_SET_CP_REG(GX_CP_REG_BREAKPOINT_LO, (u32)break_pt);
    GX_SET_CP_REG(GX_CP_REG_BREAKPOINT_HI, ((u32)break_pt >> 16) & 0x3FFF);
#if SDK_REVISION >= 2
    SET_REG_FIELD(0, __GXData->cpEnable, 1, GX_CP_ENABLE_BREAKPOINT_SHIFT, 0);
    SET_REG_FIELD(0, __GXData->cpEnable, 1, GX_CP_ENABLE_BREAKPOINT_INT_SHIFT, 0);
    GX_SET_CP_REG(GX_CP_REG_ENABLE, __GXData->cpEnable);
#endif
    SET_REG_FIELD(0, __GXData->cpEnable, 1, GX_CP_ENABLE_BREAKPOINT_SHIFT, 1);
    SET_REG_FIELD(0, __GXData->cpEnable, 1, GX_CP_ENABLE_BREAKPOINT_INT_SHIFT, 1);
    GX_SET_CP_REG(GX_CP_REG_ENABLE, __GXData->cpEnable);
    __GXCurrentBP = break_pt;
    __GXFifoReadEnable();
    OSRestoreInterrupts(enabled);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXDisableBreakPt(void) {
    BOOL enabled = OSDisableInterrupts();

    SET_REG_FIELD(0, __GXData->cpEnable, 1, GX_CP_ENABLE_BREAKPOINT_SHIFT, 0);
    SET_REG_FIELD(0, __GXData->cpEnable, 1, GX_CP_ENABLE_BREAKPOINT_INT_SHIFT, 0);
    GX_SET_CP_REG(GX_CP_REG_ENABLE, __GXData->cpEnable);
    __GXCurrentBP = NULL;
    OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x80185D30
 * @note Size: 0x4C
 */
void __GXFifoInit(void) {
    __OSSetInterruptHandler(__OS_INTERRUPT_PI_CP, GXCPInterruptHandler);
    __OSUnmaskInterrupts(OS_INTERRUPTMASK_PI_CP);
    __GXCurrentThread = OSGetCurrentThread();
    GXOverflowSuspendInProgress = FALSE;
    CPUFifo = NULL;
    GPFifo = NULL;
}

/**
 * @note Address: 0x80185D7C
 * @note Size: 0x24
 */
static void __GXFifoReadEnable(void) {
    SET_REG_FIELD(0, __GXData->cpEnable, 1, GX_CP_ENABLE_READ_SHIFT, 1);
    GX_SET_CP_REG(GX_CP_REG_ENABLE, __GXData->cpEnable);
}

/**
 * @note Address: 0x80185DA0
 * @note Size: 0x24
 */
static void __GXFifoReadDisable(void) {
    SET_REG_FIELD(0, __GXData->cpEnable, 1, GX_CP_ENABLE_READ_SHIFT, 0);
    GX_SET_CP_REG(GX_CP_REG_ENABLE, __GXData->cpEnable);
}

/**
 * @note Address: 0x80185DC4
 * @note Size: 0x34
 */
static void __GXFifoLink(u8 en) {
    SET_REG_FIELD(LINE(1242, 1242, 1299), __GXData->cpEnable, 1, GX_CP_ENABLE_FIFO_LINK_SHIFT, (en != 0) ? 1 : 0);
    GX_SET_CP_REG(GX_CP_REG_ENABLE, __GXData->cpEnable);
}

/**
 * @note Address: 0x80185DF8
 * @note Size: 0x30
 */
static void __GXWriteFifoIntEnable(u8 hiWatermarkEn, u8 loWatermarkEn) {
    SET_REG_FIELD(LINE(1264, 1264, 1321), __GXData->cpEnable, 1, GX_CP_ENABLE_OVERFLOW_INT_SHIFT, hiWatermarkEn);
    SET_REG_FIELD(LINE(1265, 1265, 1322), __GXData->cpEnable, 1, GX_CP_ENABLE_UNDERFLOW_INT_SHIFT, loWatermarkEn);
    GX_SET_CP_REG(GX_CP_REG_ENABLE, __GXData->cpEnable);
}

/**
 * @note Address: 0x80185E28
 * @note Size: 0x30
 */
static void __GXWriteFifoIntReset(u8 hiWatermarkClr, u8 loWatermarkClr) {
    SET_REG_FIELD(LINE(1288, 1288, 1345), __GXData->cpClr, 1, GX_CP_CLEAR_OVERFLOW_SHIFT, hiWatermarkClr);
    SET_REG_FIELD(LINE(1289, 1289, 1346), __GXData->cpClr, 1, GX_CP_CLEAR_UNDERFLOW_SHIFT, loWatermarkClr);
    GX_SET_CP_REG(GX_CP_REG_CLEAR, __GXData->cpClr);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void __GXInsaneWatermark(void) {
    __GXFifoObj* realFifo = GPFifo;

    realFifo->hiWatermark = realFifo->loWatermark + 512;
    GX_SET_CP_REG(GX_CP_REG_FIFO_HIGH_WATERMARK_LO, (realFifo->hiWatermark & 0x3FFFFFFF) & 0xFFFF);
    GX_SET_CP_REG(GX_CP_REG_FIFO_HIGH_WATERMARK_HI, (realFifo->hiWatermark & 0x3FFFFFFF) >> 16);
}

/**
 * @note Address: 0x80185E58
 * @note Size: 0x100
 */
void __GXCleanGPFifo(void) {
    GXFifoObj dummyFifo;
    GXFifoObj* gpFifo;
    GXFifoObj* cpuFifo;
    void* base;

    gpFifo = GXGetGPFifo();
    if (gpFifo == (GXFifoObj*)NULL)
        return;

    cpuFifo = GXGetCPUFifo();
    base = GXGetFifoBase(gpFifo);

    dummyFifo = *gpFifo;
    GXInitFifoPtrs(&dummyFifo, base, base);
    GXSetGPFifo(&dummyFifo);
    if (cpuFifo == gpFifo) {
        GXSetCPUFifo(&dummyFifo);
    }
    GXInitFifoPtrs(gpFifo, base, base);
    GXSetGPFifo(gpFifo);
    if (cpuFifo == gpFifo) {
        GXSetCPUFifo(cpuFifo);
    }
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
OSThread* GXSetCurrentGXThread(void) {
    BOOL enabled;
    OSThread* prev;

    enabled = OSDisableInterrupts();
    prev = __GXCurrentThread;
    ASSERTMSGLINE(LINE(1377, 1377, 1434), !GXOverflowSuspendInProgress, "GXSetCurrentGXThread: Two threads cannot generate GX commands at the same time!");
    __GXCurrentThread = OSGetCurrentThread();
    OSRestoreInterrupts(enabled);
    return prev;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
OSThread* GXGetCurrentGXThread(void) {
    return __GXCurrentThread;
}

/**
 * @note Address: 0x80185F58
 * @note Size: 0x8
 */
GXFifoObj* GXGetCPUFifo(void) {
    return (GXFifoObj*)CPUFifo;
}

/**
 * @note Address: 0x80185F60
 * @note Size: 0x8
 */
GXFifoObj* GXGetGPFifo(void) {
    return (GXFifoObj*)GPFifo;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
u32 GXGetOverflowCount(void) {
    return __GXOverflowCount;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
u32 GXResetOverflowCount(void) {
    u32 oldcount;

    oldcount = __GXOverflowCount;
    __GXOverflowCount = 0;
    return oldcount;
}

// NONMATCHING
/**
 * @note Address: N/A
 * @note Size: ?
 */
volatile void* GXRedirectWriteGatherPipe(void* ptr) {
    u32 reg = 0;
    BOOL enabled = OSDisableInterrupts();

    CHECK_GXBEGIN(LINE(1493, 1493, 1550), "GXRedirectWriteGatherPipe");
    ASSERTLINE(LINE(1494, 1494, 1551), OFFSET(ptr, 32) == 0);
    ASSERTLINE(LINE(1496, 1496, 1553), !IsWGPipeRedirected);

#if DEBUG
    IsWGPipeRedirected = TRUE;
#endif

    GXFlush();
    while (PPCMfwpar() & 1) {}
    PPCMtwpar((u32)OSUncachedToPhysical((void*)GXFIFO_ADDR));
    if (CPGPLinked) {
        __GXFifoLink(0);
        __GXWriteFifoIntEnable(0, 0);
    }
    CPUFifo->wrPtr = OSPhysicalToCached(GX_GET_PI_REG(5) & 0xFBFFFFFF);
    GX_SET_PI_REG(3, 0);
    GX_SET_PI_REG(4, 0x04000000);
    SET_REG_FIELD(LINE(1527, 1527, 1584), reg, 21, 5, ((u32)ptr & 0x3FFFFFFF) >> 5);
    reg &= 0xFBFFFFFF;
    GX_SET_PI_REG(5, reg);

    PPCSync();
    OSRestoreInterrupts(enabled);
    return (volatile void *)GXFIFO_ADDR;
}

// NONMATCHING
/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXRestoreWriteGatherPipe(void) {
    u32 reg = 0;
    u32 i;
    BOOL enabled;

    ASSERTLINE(1552, IsWGPipeRedirected);

#if DEBUG
    IsWGPipeRedirected = FALSE;
#endif

    enabled = OSDisableInterrupts();
    for (i = 0; i < 31; i++) {
        GXWGFifo.u8 = 0;
    }

    PPCSync();
    while (PPCMfwpar() & 1) {}
    PPCMtwpar((u32)OSUncachedToPhysical((void *)GXFIFO_ADDR));
    GX_SET_PI_REG(3, (u32)CPUFifo->base & 0x3FFFFFFF);
    GX_SET_PI_REG(4, (u32)CPUFifo->top & 0x3FFFFFFF);
    SET_REG_FIELD(1578, reg, 21, 5, ((u32)CPUFifo->wrPtr & 0x3FFFFFFF) >> 5);
    reg &= 0xFBFFFFFF;
    GX_SET_PI_REG(5, reg);
    if (CPGPLinked) {
        __GXWriteFifoIntReset(1, 1);
        __GXWriteFifoIntEnable(1, 0);
        __GXFifoLink(1);
    }

    PPCSync();
    OSRestoreInterrupts(enabled);
}
