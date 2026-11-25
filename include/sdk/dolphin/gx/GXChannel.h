#ifndef _DOLPHIN_GX_GXCHANNEL_H_
#define _DOLPHIN_GX_GXCHANNEL_H_

#include <sdk/dolphin/gx/GXRenderTypes.h>

#ifdef __cplusplus
extern "C" {
#endif

void GXSetChanAmbColor(GXChannelID chan, GXColor color);
void GXSetChanMatColor(GXChannelID chan, GXColor color);
void GXSetNumChans(u8 count);
void GXSetChanCtrl(GXChannelID chan, u8 enable, GXColorSrc amb_src, GXColorSrc mat_src, u32 light_mask,
                   GXDiffuseFn diff_fn, GXAttnFn attn_fn);

#ifdef __cplusplus
}
#endif

#endif
