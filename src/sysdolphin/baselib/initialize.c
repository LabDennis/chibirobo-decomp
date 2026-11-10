#include <global.h>

#include <dolphin/dvd.h>
#include <dolphin/gx/gx.h>
#include <dolphin/os.h>
#include <sysdolphin/class.h>
#include <sysdolphin/aobj_alloc.h>
#include <sysdolphin/displayfunc_alloc.h>
#include <sysdolphin/fobj.h>
#include <sysdolphin/gobjproc.h>
#include <sysdolphin/id.h>
#include <sysdolphin/initialize.h>
#include <sysdolphin/list.h>
#include <sysdolphin/lobj.h>
#include <sysdolphin/memory.h>
#include <sysdolphin/mtx.h>
#include <sysdolphin/objalloc.h>
#include <sysdolphin/random.h>
#include <sysdolphin/robj_alloc.h>
#include <sysdolphin/shadow.h>
#include <sysdolphin/tev_state.h>
#include <sysdolphin/video.h>
#include <sysdolphin/zlist.h>

#include <stdarg.h>

#define OSRoundDown32B(x) (((u32)(x)) & ~(32 - 1))
#define OSRoundUp32B(x)   (((u32)(x) + 32 - 1) & ~(32 - 1))

extern void _HSD_AObjForgetMemory(void);

// only the current status is read here, the full struct lives in video.c
typedef struct _HSD_VICurrent {
	HSD_VIStatus vi;
	u8 chg_flag;
} HSD_VICurrent;

extern struct _HSD_VIInfo {
	HSD_VICurrent current;
} HSD_VIData;

extern GXRenderModeObj GXNtsc480IntDf;

#define assert_line_objalloc(line, cond) ((cond) ? (void)0 : __assert("objalloc.h", line, "data"))

#define assert_line_memcb(line)      ((lbl_8065BA38 == 0) ? (void)0 : __assert("initialize.c", line, "!iparam_memory_callbacks"))
#define assert_line_addr(line, cond) ((cond) ? (void)0 : __assert("initialize.c", line, "addr"))

#define assert_line_pixfmt(line, cond) ((cond) ? (void)0 : __assert("initialize.c", line, "pix_fmt != GX_PF_RGB565_Z16"))


HSD_MemReport gHsdMemoryReport;
void* lbl_8064AF00[HSD_VI_XFB_MAX];

volatile OSHeapHandle lbl_8065AF18 = -1;
GXRenderModeObj* lbl_8065AF1C      = &GXNtsc480IntDf;
s32 lbl_8065AF20                   = GX_ZC_MID;
u32 lbl_8065AF24                   = 0x40000;
s32 lbl_8065AF28                   = 2;
GXColor lbl_8065AF2C               = { 0 };

s32 lbl_8065BA44;
s32 lbl_8065BA40;
s32 lbl_8065BA3C;
s32 lbl_8065BA38;
s32 lbl_8065BA34;
s32 lbl_8065BA30;
s32 lbl_8065BA2C;
void* lbl_8065BA28;
void* lbl_8065BA24;
void* lbl_8065BA20;
void* lbl_8065BA1C;
void* lbl_8065BA18;

void* _HSD_MemAllocDefaultCB(u32 size, u32 align, u32 flags);
void _HSD_MemFreeDefaultCB(void* ptr);
u32 _HSD_MemGetRemainDefaultCB(void);
void _HSD_MemClearDefaultCB(void);
s32 _HSD_MemCheckOwnDefaultCB(void* ptr);

/**
 * @note Address: N/A
 */
static void* GetFrameBuffer(s32 index)
{
	return lbl_8064AF00[index];
}

/**
 * @note Address: 0x801CA87C
 * @note Size: 0x13C
 */
void HSD_Init(void)
{
	HSD_VIStatus vi_status;
	void* fifo;

	DVDInit();
	HSD_AllocateXFB(lbl_8065AF28, lbl_8065AF1C);
	fifo                = HSD_AllocateFIFO(lbl_8065AF24);
	lbl_8065BA28        = GXInit(fifo, lbl_8065AF24);
	gHsdMemoryReport.gxfifo = lbl_8065AF24;
	HSD_OSInit();

	vi_status.rmode        = *lbl_8065AF1C;
	vi_status.black        = GX_TRUE;
	vi_status.vf           = GX_TRUE;
	vi_status.gamma        = GX_GM_1_0;
	vi_status.clear_clr    = (GXColor) { 0, 0, 0, 0 }; // lands in .sbss2, read straight from there
	vi_status.clear_z      = GX_MAX_Z24;
	vi_status.update_clr   = GX_ENABLE;
	vi_status.update_alpha = GX_ENABLE;
	vi_status.update_z     = GX_ENABLE;
	HSD_VIInit(&vi_status, GetFrameBuffer(0), GetFrameBuffer(1), GetFrameBuffer(2));

	HSD_GXInit();
	HSD_DVDInit();
	HSD_IDSetup();
	VIWaitForRetrace();
	HSD_ObjInit();
	lbl_8065BA30 = TRUE;
}

/**
 * @note Address: 0x801CA9B8
 * @note Size: 0x4
 */
void HSD_DVDInit(void)
{
}

/**
 * @note Address: 0x801CA9BC
 * @note Size: 0x134
 */
void** HSD_AllocateXFB(s32 nbuffer, GXRenderModeObj* rmode)
{
	s32 i;
	u32 fb_size;

	if (rmode == NULL) {
		return NULL;
	}

	fb_size          = (((rmode->fbWidth + 15) & 0xFFF0) * rmode->xfbHeight) << 1;
	gHsdMemoryReport.xfb = fb_size * nbuffer;

	for (i = 0; i < nbuffer; i++) {
		if ((lbl_8064AF00[i] = OSAllocFromArenaLo(fb_size, 0x20)) == NULL) {
			HSD_Panic("initialize.c", 0xF0, "No memory space remains for XFB.\n");
		}
	}
	for (i = nbuffer; i < HSD_VI_XFB_MAX; i++) {
		lbl_8064AF00[i] = NULL;
	}
	return lbl_8064AF00;
}

/**
 * @note Address: 0x801CAAF0
 * @note Size: 0x50
 */
void* HSD_AllocateFIFO(u32 size)
{
	void* fifo = OSAllocFromArenaLo(size, 0x20);
	if (fifo == NULL) {
		HSD_Panic("initialize.c", 0x103, "no space remains for gx fifo.\n");
	}
	return fifo;
}

/**
 * @note Address: 0x801CAB40
 * @note Size: 0xAC
 */
void HSD_GXInit(void)
{
	GXLightObj lightobj;
	s32 i;

	GXInitLightPos(&lightobj, 1.0F, 0.0F, 0.0F);
	GXInitLightDir(&lightobj, 1.0F, 0.0F, 0.0F);
	GXInitLightAttn(&lightobj, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F);
	GXInitLightColor(&lightobj, lbl_8065AF2C);

	for (i = 0; i < 8; i++) {
		GXLoadLightObjImm(&lightobj, HSD_Index2LightID(i));
	}
	HSD_StateInvalidate(-1);
}

/**
 * @note Address: 0x801CABEC
 * @note Size: 0x1A4
 */
void HSD_OSInit(void)
{
	u32 new_arena_lo;
	u32 new_arena_hi;
	u32 old_arena_lo = (u32)OSGetArenaLo();
	u32 old_arena_hi = (u32)OSGetArenaHi();

	gHsdMemoryReport.total  = OSGetPhysicalMemSize();
	gHsdMemoryReport.system = gHsdMemoryReport.total - (u32)OSGetArenaHi() + (u32)OSGetArenaLo() - gHsdMemoryReport.xfb - gHsdMemoryReport.gxfifo;

	if (!lbl_8065BA38) {
		HSD_MemCallbacks cb;
		cb.alloc      = _HSD_MemAllocDefaultCB;
		cb.free       = _HSD_MemFreeDefaultCB;
		cb.get_remain = _HSD_MemGetRemainDefaultCB;
		cb.clear      = _HSD_MemClearDefaultCB;
		cb.check_own  = _HSD_MemCheckOwnDefaultCB;
		_HSD_MemSetCallbacks(&cb, sizeof(cb));

		if (lbl_8065BA20 != NULL && lbl_8065BA24 != NULL) {
			lbl_8065BA18 = lbl_8065BA20;
			lbl_8065BA1C = lbl_8065BA24;
			lbl_8065BA20 = NULL;
			lbl_8065BA24 = NULL;
			OSSetCurrentHeap(lbl_8065AF18 = OSCreateHeap(lbl_8065BA18, lbl_8065BA1C));
			gHsdMemoryReport.heap = (u32)lbl_8065BA1C - (u32)lbl_8065BA18;
			HSD_ObjSetHeap(gHsdMemoryReport.heap, NULL);
		} else {
			new_arena_lo = OSRoundUp32B((u32)OSGetArenaLo());
			new_arena_hi = (u32)OSGetArenaHi();
			lbl_8065BA18 = (void*)new_arena_lo;
			if (lbl_8065BA34 > 0) {
				u32 want     = new_arena_lo + lbl_8065BA34;
				lbl_8065BA1C = (void*)want;
				if (want > new_arena_hi) {
					lbl_8065BA1C = (void*)new_arena_hi;
				}
				lbl_8065BA1C = (void*)OSRoundDown32B(lbl_8065BA1C);
			} else {
				lbl_8065BA1C = (void*)OSRoundDown32B(new_arena_hi);
			}
			OSSetCurrentHeap(lbl_8065AF18 = OSCreateHeap(lbl_8065BA18, lbl_8065BA1C));
			gHsdMemoryReport.heap = (u32)lbl_8065BA1C - (u32)lbl_8065BA18;
			HSD_ObjSetHeap(gHsdMemoryReport.heap, NULL);
			OSSetArenaLo(lbl_8065BA1C);
		}
	}
}

/**
 * @note Address: 0x801CAD90
 * @note Size: 0x60
 */
void* _HSD_MemAllocDefaultCB(u32 size, u32 align, u32 flags)
{
	void* addr;

	if (size == 0) {
		return NULL;
	}
	addr = OSAllocFromHeap(HSD_GetHeap(), size);
	assert_line_addr(0x179, addr);
	return addr;
}

/**
 * @note Address: 0x801CADF0
 * @note Size: 0x34
 */
void _HSD_MemFreeDefaultCB(void* ptr)
{
	OSFreeToHeap(HSD_GetHeap(), ptr);
}

/**
 * @note Address: 0x801CAE24
 * @note Size: 0x24
 */
u32 _HSD_MemGetRemainDefaultCB(void)
{
	return OSCheckHeap(HSD_GetHeap());
}

/**
 * @note Address: 0x801CAE48
 * @note Size: 0x64
 */
void _HSD_MemClearDefaultCB(void)
{
	OSDestroyHeap(lbl_8065AF18);
	if (lbl_8065BA20 != NULL && lbl_8065BA24 != NULL) {
		lbl_8065BA18 = lbl_8065BA20;
		lbl_8065BA1C = lbl_8065BA24;
		lbl_8065BA20 = NULL;
		lbl_8065BA24 = NULL;
	}
	OSSetCurrentHeap(lbl_8065AF18 = OSCreateHeap(lbl_8065BA18, lbl_8065BA1C));
}

/**
 * @note Address: 0x801CAEAC
 * @note Size: 0x28
 */
s32 _HSD_MemCheckOwnDefaultCB(void* ptr)
{
	s32 within = 0;
	if ((u32)lbl_8065BA18 <= (u32)ptr) {
		if ((u32)ptr < (u32)lbl_8065BA1C) {
			within = 1;
		}
	}
	return within;
}

/**
 * @note Address: 0x801CAED4
 * @note Size: 0x44
 */
OSHeapHandle HSD_GetHeap(void)
{
	assert_line_memcb(0x1B5);
	return lbl_8065AF18;
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
void HSD_SetHeap(OSHeapHandle handle)
{
	lbl_8065AF18 = handle;
}

/**
 * @note Address: N/A
 * @note Size: 0x98
 */
void HSD_SetNextArena(void* lo, void* hi)
{
	assert_line_memcb(0x1ED);

	if ((lo == NULL && hi != NULL) || (lo != NULL && hi == NULL)) {
		OSReport("missing argument.\n");
	} else {
		lbl_8065BA20 = lo;
		lbl_8065BA24 = hi;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x78
 */
void HSD_GetNextArena(void** lo, void** hi)
{
	assert_line_memcb(0x205);
	if (lo != NULL) {
		*lo = lbl_8065BA18;
	}
	if (hi != NULL) {
		*hi = lbl_8065BA1C;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0xA8
 */
void HSD_ClearHeap(void)
{
	void (*cb_table[])(void) = {
		_HSD_AObjForgetMemory,
		_HSD_DispForgetMemory,
		_HSD_IDForgetMemory,
		_HSD_ObjAllocForgetMemory,
		_HSD_RandForgetMemory,
		_HSD_RObjForgetMemory,
		NULL,
	};
	s32 i;

	hsdForgetClassLibrary("sysdolphin_base_library");
	for (i = 0; cb_table[i] != NULL; i++) {
		cb_table[i]();
	}
	HSD_ObjInit();
	_HSD_MemClear();
	gHsdMemoryReport.heap = _HSD_MemGetRemain();
}

/**
 * @note Address: 0x801CAF18
 * @note Size: 0x8
 */
HSD_RenderPass HSD_GetCurrentRenderPass(void)
{
	return lbl_8065BA44;
}

/**
 * @note Address: 0x801CAF20
 * @note Size: 0xA0
 */
void HSD_StartRender(HSD_RenderPass pass)
{
	GXRenderModeObj* rmode = &HSD_VIData.current.vi.rmode;

	lbl_8065BA44 = pass;
	if (rmode->aa) {
		GXSetPixelFmt(GX_PF_RGB565_Z16, lbl_8065AF20);
	} else {
		GXSetPixelFmt(lbl_8065BA2C, GX_ZC_LINEAR);
	}
	GXSetFieldMode(rmode->field_rendering, rmode->xfbHeight < rmode->viHeight);

	if (lbl_8065BA40 != 0) {
		if (lbl_8065BA40 & 1) {
			GXInvalidateVtxCache();
		}
		if (lbl_8065BA40 & 2) {
			GXInvalidateTexAll();
		}
		lbl_8065BA40 = 0;
	}
}

/**
 * @note Address: 0x801CAFC0
 * @note Size: 0x4
 */
void HSD_EndRender(void)
{
}

/**
 * @note Address: 0x801CAFC4
 * @note Size: 0x10
 */
void _HSD_NeedCacheInvalidate(u32 flags)
{
	lbl_8065BA40 |= flags;
}

/**
 * @note Address: 0x801CAFD4
 * @note Size: 0x5C
 */
void HSD_SetPixelFormat(s32 pix_fmt, s32 z_fmt)
{
	assert_line_pixfmt(0x2ED, pix_fmt != GX_PF_RGB565_Z16);
	lbl_8065BA2C = pix_fmt;
	lbl_8065AF20 = z_fmt;
}

/**
 * @note Address: 0x801CB030
 * @note Size: 0x44
 */
void HSD_ObjInit(void)
{
	HSD_ListInitAllocData();
	HSD_AObjInitAllocData();
	HSD_FObjInitAllocData();
	HSD_IDInitAllocData();
	HSD_VecInitAllocData();
	HSD_MtxInitAllocData();
	HSD_RObjInitAllocData();
	HSD_RenderInitAllocData();
	HSD_ShadowInitAllocData();
	HSD_ZListInitAllocData();
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline u32 HSD_ObjAllocGetUsing(HSD_ObjAllocData* data)
{
	assert_line_objalloc(0xCD, data);
	return data->used;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline u32 HSD_ObjAllocGetFreed(HSD_ObjAllocData* data)
{
	assert_line_objalloc(0xDD, data);
	return data->free;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline u32 HSD_ObjAllocGetPeak(HSD_ObjAllocData* data)
{
	assert_line_objalloc(0xED, data);
	return data->peak;
}

/**
 * @note Address: N/A
 * @note Size: 0x114
 */
void HSD_ObjDumpStat(void)
{
	struct {
		HSD_ObjAllocData* (*func)(void);
		char* label;
	} types[] = { { HSD_AObjGetAllocData, "aobj" },      { HSD_FObjGetAllocData, "fobj" },
		          { HSD_IDGetAllocData, "id" },          { HSD_SListGetAllocData, "slist" },
		          { HSD_DListGetAllocData, "dlist" },    { HSD_VecGetAllocData, "vec" },
		          { HSD_MtxGetAllocData, "mtx" },        { HSD_RObjGetAllocData, "robj" },
		          { HSD_RvalueObjGetAllocData, "rval" }, { HSD_ShadowGetAllocData, "shadow" },
		          { HSD_RenderGetAllocData, "render" },  { HSD_ChanGetAllocData, "chan" },
		          { HSD_TevRegGetAllocData, "tevreg" },  { NULL, NULL } };
	s32 i;

	for (i = 0; types[i].label; i++) {
		OSReport("objalloc: %s\tusing %d\tfreed %d\tpeak %d\n", types[i].label, HSD_ObjAllocGetUsing(types[i].func()),
		         HSD_ObjAllocGetFreed(types[i].func()), HSD_ObjAllocGetPeak(types[i].func()));
	}
}

/**
 * @note Address: 0x801CB074
 * @note Size: 0x2B8
 */
bool HSD_SetInitParameter(HSD_InitParam param, ...)
{
	va_list ap;
	bool ok = FALSE;

	if (lbl_8065BA30) {
		if (!lbl_8065BA3C) {
			OSReport("init parameter should be set before invoking HSD_Init().\n");
			lbl_8065BA3C = TRUE;
		}
		return FALSE;
	}

	va_start(ap, param);
	switch (param) {
	case HSD_INIT_FIFO_SIZE: {
		u32 fifo_size = va_arg(ap, u32);
		if (fifo_size > 0) {
			lbl_8065AF24 = fifo_size;
			ok           = TRUE;
		}
	} break;

	case HSD_INIT_XFB_MAX_NUM: {
		u32 xfb_max_num = va_arg(ap, u32);
		if (xfb_max_num > 0) {
			lbl_8065AF28 = xfb_max_num;
			ok           = TRUE;
		}
	} break;

	case HSD_INIT_HEAP_MAX_SIZE: {
		u32 heap_max_size = va_arg(ap, u32);
		if (heap_max_size < OSGetPhysicalMemSize()) {
			lbl_8065BA34 = heap_max_size;
			ok           = TRUE;
		}
	} break;

	case HSD_INIT_NEXT_ARENA: {
		void* lo     = va_arg(ap, void*);
		void* hi     = va_arg(ap, void*);
		lbl_8065BA20 = lo;
		lbl_8065BA24 = hi;
		ok           = TRUE;
	} break;

	case HSD_INIT_MEMORY_CALLBACKS: {
		HSD_MemCallbacks cb;
		u32 v;
		cb.alloc      = va_arg(ap, void*);
		cb.free       = va_arg(ap, void*);
		cb.clear      = va_arg(ap, void*);
		cb.get_remain = va_arg(ap, void*);
		cb.check_own  = va_arg(ap, void*);
		v             = va_arg(ap, u32);
		if (v != 0) {
			OSReport("ERROR in HSD_SetInitParameter():\n");
			OSReport("  HSD_INIT_MEMORY_CALLBACKS was given invalid arguments.\n");
		} else {
			_HSD_MemSetCallbacks(&cb, sizeof(cb));
			lbl_8065BA38 = 1;
			ok           = TRUE;
		}
	} break;

	case HSD_INIT_RENDER_MODE_OBJ: {
		GXRenderModeObj* rmode = va_arg(ap, GXRenderModeObj*);
		if (rmode != NULL) {
			lbl_8065AF1C = rmode;
			ok           = TRUE;
		}
	} break;

	case HSD_INIT_HEAP_MAX_NUM: {
		s32 v;
		OSReport("ERROR in HSD_SetInitParameter():\n");
		OSReport("  HSD_INIT_HEAP_MAX_NUM is obsolete since 1.3.0.0. \n");
		v = va_arg(ap, s32);
		if (v == 0) {
			ok = TRUE;
		}
	} break;

	case HSD_INIT_AUDIO_HEAP_SIZE:
		OSReport("ERROR in HSD_SetInitParameter():\n");
		OSReport("  HSD_INIT_AUDIO_HEAP_SIZE is obsolete since 1.3.0.0. \n");
		break;
	}
	va_end(ap);

	return ok;
}
