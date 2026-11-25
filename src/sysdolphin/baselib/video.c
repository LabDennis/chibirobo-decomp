#include <global.h>
#include <dolphin/gx/gx.h>
#include <dolphin/os.h>

#include <sysdolphin/gobjproc.h>
#include <sysdolphin/tev.h>
#include <sysdolphin/video.h>

#define HSD_ANTIALIAS_OVERLAP      4
#define HSD_ANTIALIAS_GARBAGE_SIZE (640 * HSD_ANTIALIAS_OVERLAP * VI_DISPLAY_PIX_SZ)

#define GX_CLAMP_TOP    1
#define GX_CLAMP_BOTTOM 2

HSD_VIInfo HSD_VIData;
static u8 sAntialiasOverlapBuffer[HSD_ANTIALIAS_GARBAGE_SIZE] __attribute__((aligned(32)));

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void* HSD_VIGetXFBPtr(s32 idx)
{
	return HSD_VIData.xfb[idx].buffer;
}

#define assert_line_named(line, cond, condstr) ((cond) ? ((void)0) : __assert("video.c", line, condstr))

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline s32 HSD_VISearchXFBByStatus(HSD_VIXFBDrawDispStatus status)
{
	s32 i;

	for (i = 0; i < HSD_VI_XFB_MAX; i++) {
		if (HSD_VIData.xfb[i].status == status) {
			return i;
		}
	}
	return -1;
}

/**
 * @note Address: N/A
 * @note Size: 0x54
 */
HSD_VIRetraceCallback HSD_VISetUserPreRetraceCallback(HSD_VIRetraceCallback cb)
{
	bool intr;
	HSD_VIRetraceCallback old = HSD_VIData.pre_cb;

	intr       = OSDisableInterrupts();
	HSD_VIData.pre_cb = cb;
	OSRestoreInterrupts(intr);

	return old;
}

/**
 * @note Address: 0x801CB32C
 * @note Size: 0x54
 */
HSD_VIRetraceCallback HSD_VISetUserPostRetraceCallback(HSD_VIRetraceCallback cb)
{
	bool intr;
	HSD_VIRetraceCallback old = HSD_VIData.post_cb;

	intr        = OSDisableInterrupts();
	HSD_VIData.post_cb = cb;
	OSRestoreInterrupts(intr);

	return old;
}

/**
 * @note Address: 0x801CB380
 * @note Size: 0x54
 */
HSD_VIGXDrawDoneCallback HSD_VISetUserGXDrawDoneCallback(HSD_VIGXDrawDoneCallback cb)
{
	bool intr;
	HSD_VIGXDrawDoneCallback old = HSD_VIData.drawdone.cb;

	intr            = OSDisableInterrupts();
	HSD_VIData.drawdone.cb = cb;
	OSRestoreInterrupts(intr);

	return old;
}

/**
 * @note Address: 0x801CB3D4
 * @note Size: 0x270
 */
void HSD_VIPreRetraceCB(u32 retraceCount)
{
	s32 next;
	s32 idx;
	s32 flush = 0;
	s32 renew = 0;

	next = HSD_VISearchXFBByStatus(HSD_VI_XFB_NEXT);
	if (next != -1) {
		VISetNextFrameBuffer(HSD_VIData.xfb[next].buffer);
		if (HSD_VIData.xfb[next].vi_all.chg_flag) {
			VIConfigure(&HSD_VIData.xfb[next].vi_all.vi.rmode);
			VISetBlack(HSD_VIData.xfb[next].vi_all.vi.black);
		}
		flush = 1;
		renew = 1;
	} else if (HSD_VIData.nb_xfb == 1 && HSD_VIData.efb.status == HSD_VI_EFB_DRAWDONE) {
		if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DISPLAY)) == -1) {
			idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_FREE);
			assert_line_named(0xFC, idx != -1, "idx != -1");
			VISetNextFrameBuffer(HSD_VIData.xfb[idx].buffer);
			flush = 1;
		}
		HSD_VIData.xfb[idx].status = HSD_VI_XFB_COPYEFB;
		if (HSD_VIData.efb.vi_all.chg_flag) {
			VIConfigure(&HSD_VIData.efb.vi_all.vi.rmode);
			VISetBlack(HSD_VIData.efb.vi_all.vi.black);
			flush = 1;
		}
		renew = 1;
	}

	if (flush) {
		VIFlush();
	}

	{
		static int vr_count    = 0;
		static int renew_count = 0;

		if (renew) {
			renew_count++;
		}
		if (++vr_count >= HSD_VIData.perf.frame_period) {
			HSD_VIData.perf.frame_renew = renew_count;
			vr_count = renew_count = 0;
		}
	}

	if (HSD_VIData.pre_cb) {
		HSD_VIData.pre_cb(retraceCount);
	}
}

/**
 * @note Address: 0x801CB644
 * @note Size: 0x20C
 */
void HSD_VIPostRetraceCB(u32 retraceCount)
{
	s32 idx;
	s32 next;

	if ((next = HSD_VISearchXFBByStatus(HSD_VI_XFB_NEXT)) != -1) {
		if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DISPLAY)) != -1) {
			HSD_VIData.xfb[idx].status = HSD_VI_XFB_FREE;
		}
		HSD_VIData.xfb[next].status = HSD_VI_XFB_DISPLAY;
		if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DRAWDONE)) != -1) {
			HSD_VIData.xfb[idx].status = HSD_VI_XFB_NEXT;
		}
	} else if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_COPYEFB)) != -1) {
		HSD_VICopyEFB2XFBPtr(&HSD_VIData.efb.vi_all.vi, HSD_VIData.xfb[idx].buffer, HSD_RP_SCREEN);
		HSD_VIData.xfb[idx].status = HSD_VI_XFB_DISPLAY;
		HSD_VIData.efb.status      = HSD_VI_EFB_FREE;
	}

	if (HSD_VIData.post_cb) {
		HSD_VIData.post_cb(retraceCount);
	}
}

/**
 * @note Address: 0x801CB850
 * @note Size: 0x44
 */
void HSD_VIGXDrawDoneCB(void)
{
	HSD_VIData.drawdone.waiting = 0;

	if (HSD_VIData.drawdone.cb) {
		HSD_VIData.drawdone.cb(HSD_VIData.drawdone.arg);
	}
}

/**
 * @note Address: 0x801CB894
 * @note Size: 0x10
 */
s32 HSD_VIGetDrawDoneWaitingFlag(void)
{
	return HSD_VIData.drawdone.waiting;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_VICopyEFB2XFBHiResoAA(GXRenderModeObj* rmode)
{
	s32 n_xfb_lines;

	GXSetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight - HSD_ANTIALIAS_OVERLAP);
	n_xfb_lines = GXSetDispCopyYScale(1.0F);
	GXSetDispCopyDst(rmode->fbWidth, n_xfb_lines);
}

/**
 * @note Address: 0x801CB8A4
 * @note Size: 0x258
 */
void HSD_VICopyEFB2XFBPtr(HSD_VIStatus* vi, void* buffer, HSD_RenderPass rpass)
{
	GXRenderModeObj* rmode = &vi->rmode;
	s32 n_xfb_lines;
	u16 lines;
	u32 offset;

	GXSetCopyFilter(rmode->aa, rmode->sample_pattern, vi->vf, rmode->vfilter);
	GXSetDispCopyGamma(vi->gamma);

	HSD_StateSetColorUpdate(vi->update_clr);
	HSD_StateSetAlphaUpdate(vi->update_alpha);
	HSD_StateSetZMode(vi->update_z, GX_LEQUAL, GX_TRUE);

	GXSetCopyClear(vi->clear_clr, vi->clear_z);

	switch (rpass) {
	case HSD_RP_SCREEN:
		GXSetCopyClamp(GX_CLAMP_TOP | GX_CLAMP_BOTTOM);
		GXSetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
		n_xfb_lines = GXSetDispCopyYScale((f32)(rmode->xfbHeight) / (f32)(rmode->efbHeight));
		GXSetDispCopyDst(rmode->fbWidth, n_xfb_lines);
		GXCopyDisp(buffer, GX_TRUE);
		break;

	case HSD_RP_TOPHALF:
		HSD_VICopyEFB2XFBHiResoAA(rmode);
		GXSetCopyClamp(GX_CLAMP_TOP);
		lines = rmode->efbHeight - HSD_ANTIALIAS_OVERLAP;
		GXSetDispCopySrc(0, 0, rmode->fbWidth, lines);
		GXCopyDisp(buffer, GX_TRUE);
		GXPixModeSync();
		return;

	case HSD_RP_BOTTOMHALF:
		HSD_VICopyEFB2XFBHiResoAA(rmode);
		GXSetCopyClamp(GX_CLAMP_BOTTOM);
		lines = rmode->efbHeight - HSD_ANTIALIAS_OVERLAP;
		GXSetDispCopySrc(0, HSD_ANTIALIAS_OVERLAP, rmode->fbWidth, lines);
		offset = (VIPadFrameBufferWidth(rmode->fbWidth) * lines * (u32)VI_DISPLAY_PIX_SZ);
		GXCopyDisp((void*)((u32)buffer + offset), GX_TRUE);
		GXSetDispCopySrc(0, 0, rmode->fbWidth, HSD_ANTIALIAS_OVERLAP);
		GXSetCopyClamp(GX_CLAMP_TOP | GX_CLAMP_BOTTOM);
		GXCopyDisp((void*)sAntialiasOverlapBuffer, GX_TRUE);
		break;

	default:
		HSD_Panic("video.c", 0x207, "unexpected type of render pass.\n");
	}

	GXPixModeSync();
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline s32 HSD_VIGetXFBDrawEnable(void)
{
	bool intr;
	s32 idx = -1;

	if (HSD_VIData.nb_xfb < 2) {
		goto ret;
	}

	intr = OSDisableInterrupts();

	if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DRAWING)) == -1) {
		if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_FREE)) != -1) {
			HSD_VIData.xfb[idx].status = HSD_VI_XFB_DRAWING;
		}
	}

	OSRestoreInterrupts(intr);

ret:
	return idx;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline s32 HSD_VIWaitXFBDrawEnable(void)
{
	s32 idx = -1;

	if (HSD_VIData.nb_xfb < 2) {
		goto ret;
	}

	while ((idx = HSD_VIGetXFBDrawEnable()) == -1) {
		VIWaitForRetrace();
	}

ret:
	return idx;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_VISetXFBWaitDone(s32 idx)
{
	bool intr;

	intr = OSDisableInterrupts();

	assert_line_named(0x24E, HSD_VIData.xfb[idx].status == HSD_VI_XFB_DRAWING, "_p->xfb[idx].status == HSD_VI_XFB_DRAWING");

	HSD_VIData.xfb[idx].status  = HSD_VI_XFB_WAITDONE;
	HSD_VIData.xfb[idx].vi_all  = HSD_VIData.current;
	HSD_VIData.current.chg_flag = 0;

	OSRestoreInterrupts(intr);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_VIGXSetDrawDone(s32 arg)
{
	while (HSD_VIGetDrawDoneWaitingFlag()) {
		GXWaitDrawDone();
	}
	HSD_VIData.drawdone.waiting = 1;
	HSD_VIData.drawdone.arg     = arg;
	GXSetDrawDone();
}

/**
 * @note Address: 0x801CBAFC
 * @note Size: 0x208
 */
void HSD_VICopyXFBAsync(HSD_RenderPass rpass)
{
	s32 idx;

	if (HSD_VIData.nb_xfb < 2) {
		return;
	}

	idx = HSD_VIWaitXFBDrawEnable();
	HSD_VICopyEFB2XFBPtr(&HSD_VIData.current.vi, HSD_VIGetXFBPtr(idx), rpass);
	HSD_VISetXFBWaitDone(idx);

	HSD_VIGXSetDrawDone(idx);
}

/**
 * @note Address: 0x801CBD04
 * @note Size: 0xD4
 */
void HSD_VIDrawDoneXFB(s32 idx)
{
	HSD_VIXFBDrawDispStatus* status;
	bool intr;

	intr = OSDisableInterrupts();

	status = &HSD_VIData.xfb[idx].status;
	assert_line_named(0x2D2, *status == HSD_VI_XFB_WAITDONE, "_p->xfb[idx].status == HSD_VI_XFB_WAITDONE");

	*status = HSD_VISearchXFBByStatus(HSD_VI_XFB_NEXT) != -1 ? HSD_VI_XFB_DRAWDONE : HSD_VI_XFB_NEXT;

	OSRestoreInterrupts(intr);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline s32 HSD_VIWaitXFBFlush_sub(void)
{
	bool intr;
	s32 val;

	intr = OSDisableInterrupts();

	val = (HSD_VISearchXFBByStatus(HSD_VI_XFB_WAITDONE) != -1 || HSD_VISearchXFBByStatus(HSD_VI_XFB_DRAWDONE) != -1
	       || HSD_VISearchXFBByStatus(HSD_VI_XFB_NEXT) != -1)
	        ? 1
	        : 0;

	OSRestoreInterrupts(intr);

	return val;
}

/**
 * @note Address: N/A
 * @note Size: 0x178
 */
void HSD_VIWaitXFBFlush(void)
{
	if (HSD_VIData.nb_xfb < 2) {
		return;
	}

	while (HSD_VIWaitXFBFlush_sub()) {
		VIWaitForRetrace();
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x16C
 */
void HSD_VIWaitXFBFlushNoYield(void)
{
	if (HSD_VIData.nb_xfb < 2) {
		return;
	}

	while (HSD_VIWaitXFBFlush_sub()) { }
}

/**
 * @note Address: N/A
 * @note Size: 0x170
 */
s32 HSD_VIGetXFBLastDrawDone(void)
{
	bool intr;
	s32 idx = -1;

	intr = OSDisableInterrupts();

	if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_WAITDONE)) == -1) {
		if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DRAWDONE)) == -1) {
			if ((idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_NEXT)) == -1) {
				idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_DISPLAY);
			}
		}
	}

	OSRestoreInterrupts(intr);

	return idx;
}

/**
 * @note Address: 0x801CBDD8
 * @note Size: 0x88
 */
void HSD_VISetConfigure(GXRenderModeObj* rmode)
{
	HSD_VIData.current.vi.rmode = *rmode;
	HSD_VIData.current.chg_flag = 1;
}

/**
 * @note Address: 0x801CBE60
 * @note Size: 0x18
 */
void HSD_VISetBlack(bool black)
{
	HSD_VIData.current.vi.black = black;
	HSD_VIData.current.chg_flag = 1;
}

/**
 * @note Address: 0x801CBE78
 * @note Size: 0x228
 */
void HSD_VIInit(HSD_VIStatus* vi, void* xfb0, void* xfb1, void* xfb2)
{
	s32 i, fbnum, idx;

	VIInit();

	HSD_VIData.current.vi       = *vi;
	HSD_VIData.current.chg_flag = 0;
	HSD_VIData.xfb[0].buffer    = xfb0;
	HSD_VIData.xfb[1].buffer    = xfb1;
	HSD_VIData.xfb[2].buffer    = xfb2;

	for (i = 0, fbnum = 0; i < HSD_VI_XFB_MAX; i++) {
		HSD_VIData.xfb[i].vi_all = HSD_VIData.current;
		if (HSD_VIData.xfb[i].buffer) {
			fbnum++;
			HSD_VIData.xfb[i].status = HSD_VI_XFB_FREE;
		} else {
			HSD_VIData.xfb[i].status = HSD_VI_XFB_NONE;
		}
	}

	HSD_VIData.nb_xfb = fbnum;

	HSD_VIData.efb.status = HSD_VI_EFB_FREE;
	HSD_VIData.efb.vi_all = HSD_VIData.current;

	VISetPreRetraceCallback(HSD_VIPreRetraceCB);
	VISetPostRetraceCallback(HSD_VIPostRetraceCB);

	HSD_VIData.pre_cb  = NULL;
	HSD_VIData.post_cb = NULL;

	HSD_VIData.drawdone.waiting = 0;
	HSD_VIData.drawdone.arg     = 0;

	GXSetDrawDoneCallback(HSD_VIGXDrawDoneCB);
	HSD_VIData.drawdone.cb = NULL;

	HSD_VIData.perf.frame_period = VIGetTvFormat() == VI_NTSC ? 60 : 50;
	HSD_VIData.perf.frame_renew  = 0;

	VIConfigure(&HSD_VIData.current.vi.rmode);
	VISetBlack(HSD_VIData.current.vi.black);
	VIFlush();

	idx = HSD_VISearchXFBByStatus(HSD_VI_XFB_FREE);
	HSD_VICopyEFB2XFBPtr(&HSD_VIData.current.vi, HSD_VIGetXFBPtr(idx), HSD_RP_SCREEN);
}
