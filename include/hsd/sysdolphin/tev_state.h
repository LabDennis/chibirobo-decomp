#ifndef _tev_state_h_
#define _tev_state_h_

#include <global.h>

#include <sysdolphin/objalloc.h>

#define HSD_STATE_ALL           -1
#define HSD_STATE_NONE          0
#define HSD_STATE_PRIMITIVE     0x1
#define HSD_STATE_VTX_ATTR      0x2
#define HSD_STATE_COLOR_CHANNEL 0x4
#define HSD_STATE_TEV_STAGE     0x8
#define HSD_STATE_TEV_REGISTER  0x10
#define HSD_STATE_TEX_COORD_GEN 0x20
#define HSD_STATE_RENDER_MODE   0x40

void HSD_StateInvalidate(s32 mask);
void HSD_RenderInitAllocData(void);
HSD_ObjAllocData* HSD_RenderGetAllocData(void);
HSD_ObjAllocData* HSD_TevRegGetAllocData(void);
HSD_ObjAllocData* HSD_ChanGetAllocData(void);
u32 HSD_StateAssignTev(void);
void HSD_StateRegisterTexGen(s32 coord);

#endif
