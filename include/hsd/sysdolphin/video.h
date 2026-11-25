#ifndef _video_h_
#define _video_h_

#include <global.h>

#include <dolphin/gx/gxtypes.h>
#include <Dolphin/vi.h>
#include <sysdolphin/forward.h>

#define HSD_VI_XFB_MAX 3

typedef struct _HSD_VIStatus {
    GXRenderModeObj rmode;
    u32 black;
    u8 vf;
    u32 gamma;
    GXColor clear_clr;
    u32 clear_z;
    u8 update_clr;
    u8 update_alpha;
    u8 update_z;
} HSD_VIStatus;

typedef enum _HSD_RenderPass {
    HSD_RP_SCREEN,
    HSD_RP_TOPHALF,
    HSD_RP_BOTTOMHALF,
    HSD_RP_OFFSCREEN,
    HSD_RP_NUM
} HSD_RenderPass;

typedef VIRetraceCallback HSD_VIRetraceCallback;
typedef void (*HSD_VIGXDrawDoneCallback)(int);

typedef enum _HSD_VIXFBDrawDispStatus {
    HSD_VI_XFB_NONE,
    HSD_VI_XFB_NOUSE,
    HSD_VI_XFB_FREE,
    HSD_VI_XFB_DRAWING,
    HSD_VI_XFB_WAITDONE,
    HSD_VI_XFB_DRAWDONE,
    HSD_VI_XFB_NEXT,
    HSD_VI_XFB_DISPLAY,
    HSD_VI_XFB_COPYEFB,
    HSD_VI_XFB_TERMINATE
} HSD_VIXFBDrawDispStatus;

typedef enum _HSD_VIEFBDrawDispStatus {
    HSD_VI_EFB_FREE,
    HSD_VI_EFB_DRAWDONE,
    HSD_VI_EFB_TERMINATE
} HSD_VIEFBDrawDispStatus;

typedef struct _HSD_VICurrent {
    HSD_VIStatus vi;
    u8 chg_flag;
} HSD_VICurrent;

typedef struct _HSD_VIXFB {
    void* buffer;
    HSD_VIXFBDrawDispStatus status;
    HSD_VICurrent vi_all;
} HSD_VIXFB;

typedef struct _HSD_VIInfo {
    HSD_VICurrent current;
    HSD_VIXFB xfb[HSD_VI_XFB_MAX];

    struct {
        HSD_VIEFBDrawDispStatus status;
        HSD_VICurrent vi_all;
    } efb;

    s32 nb_xfb;

    HSD_VIRetraceCallback pre_cb;
    HSD_VIRetraceCallback post_cb;

    struct {
        volatile s32 waiting;
        s32 arg;
        HSD_VIGXDrawDoneCallback cb;
    } drawdone;

    struct {
        s32 frame_period;
        s32 frame_renew;
    } perf;
} HSD_VIInfo;

extern HSD_VIInfo HSD_VIData;

HSD_VIRetraceCallback HSD_VISetUserPreRetraceCallback(HSD_VIRetraceCallback cb);
HSD_VIRetraceCallback HSD_VISetUserPostRetraceCallback(HSD_VIRetraceCallback cb);
HSD_VIGXDrawDoneCallback HSD_VISetUserGXDrawDoneCallback(HSD_VIGXDrawDoneCallback cb);
s32 HSD_VIGetDrawDoneWaitingFlag(void);
void HSD_VICopyEFB2XFBPtr(HSD_VIStatus* vi, void* buffer, HSD_RenderPass rpass);
void HSD_VICopyXFBAsync(HSD_RenderPass rpass);
void HSD_VIDrawDoneXFB(s32 idx);
void HSD_VIWaitXFBFlush(void);
void HSD_VIWaitXFBFlushNoYield(void);
s32 HSD_VIGetXFBLastDrawDone(void);
void HSD_VISetConfigure(GXRenderModeObj* rmode);
void HSD_VISetBlack(bool black);
void HSD_VIInit(HSD_VIStatus* vi, void* xfb0, void* xfb1, void* xfb2);

#endif
