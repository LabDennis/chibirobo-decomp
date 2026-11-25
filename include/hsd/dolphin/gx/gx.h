#ifndef _GX_H_
#define _GX_H_

#include <dolphin/gx/gxtypes.h>
#include <dolphin/mtx/mtxtypes.h>
#include <sdk/dolphin/gx/GXChannel.h>
#include <sdk/dolphin/gx/GXPixel.h>
#include <sdk/dolphin/gx/GXTev.h>
#include <sdk/dolphin/gx/GXTexture.h>

void GXBegin(u8 prim, u8 vtxfmt, u16 nverts);
void GXClearVtxDesc(void);
void GXCopyDisp(void* dest, u32 clear);
void GXCopyTex(void* dest, u8 clear);
void GXEnableTexOffsets(GXTexCoordID coord, u8 enable_line, u8 enable_point);
void GXGetProjectionv(f32* ptr);
void GXGetViewportv(f32* vp);
void GXInvalidateVtxCache(void);
void* GXInit(void* fifo, u32 size);
void GXInitLightAttn(GXLightObj* light, f32 a0, f32 a1, f32 a2, f32 k0,
                     f32 k1, f32 k2);
void GXInitLightColor(GXLightObj* light, GXColor color);
void GXInitLightDistAttn(GXLightObj* light, f32 ref_dist, f32 ref_br, GXDistAttnFn dist_func);
void GXInitLightDir(GXLightObj* light, f32 x, f32 y, f32 z);
void GXInitLightPos(GXLightObj* light, f32 x, f32 y, f32 z);
void GXInitLightSpot(GXLightObj* light, f32 cutoff, GXSpotFn spot_func);
void GXCallDisplayList(void* list, u32 nbytes);
void GXLoadLightObjImm(GXLightObj* light, s32 light_id);
void GXLoadNrmMtxImm(Mtx mtx, u32 id);
void GXLoadPosMtxImm(MtxPtr mtx, u32 id);
void GXLoadTexMtxImm(MtxPtr mtx, u32 id, u32 type);
void GXPixModeSync(void);
void GXSetProjection(const Mtx44 mtx, s32 projection_type);
void GXSetScissor(u32 left, u32 top, u32 width, u32 height);
void GXSetCopyClear(GXColor clear_clr, u32 clear_z);
void GXSetCopyClamp(u32 clamp);
void GXSetCopyFilter(u32 aa, const u8 sample_pattern[12][2], u32 vf,
                     const u8 vfilter[7]);
void GXSetCullMode(u32 mode);
void GXSetCurrentMtx(u32 id);
void GXSetDispCopyDst(u16 wd, u16 ht);
void GXSetDispCopyGamma(u32 gamma);
void GXSetDispCopySrc(u16 left, u16 top, u16 wd, u16 ht);
u32 GXSetDispCopyYScale(f32 vscale);
void GXSetDrawDone(void);
void* GXSetDrawDoneCallback(void (*cb)(void));
void GXSetLineWidth(u8 width, u32 tex_offsets);
void GXSetArray(GXAttr attr, void* base_ptr, u8 stride);
void GXSetNumTexGens(u8 n);
void GXSetPointSize(u8 size, u32 tex_offsets);
void GXSetTexCoordGen2(GXTexCoordID coord, GXTexGenType func, GXTexGenSrc source, u32 mtx, u8 normalize, u32 post_mtx);
void GXSetTexCopyDst(u16 width, u16 height, GXTexFmt format, u8 mipmap);
void GXSetTexCopySrc(u16 left, u16 top, u16 width, u16 height);
void GXSetViewport(f32 left, f32 top, f32 width, f32 height, f32 near_z, f32 far_z);
void GXSetViewportJitter(f32 left, f32 top, f32 width, f32 height, f32 near_z,
                         f32 far_z, u32 field);
void GXSetVtxAttrFmt(u32 vtxfmt, GXAttr attr, u32 comp_cnt, u32 comp_type, u8 frac);
void GXSetVtxDesc(GXAttr attr, u32 type);
void GXWaitDrawDone(void);

#endif
