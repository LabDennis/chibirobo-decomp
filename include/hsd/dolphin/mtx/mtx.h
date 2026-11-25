#ifndef _MTX_H_
#define _MTX_H_

#include <dolphin/mtx/mtxtypes.h>

#ifdef __cplusplus
extern "C" {
#endif

f32 mtx_tanf(f32 x);
f32 mtx_sinf(f32 x);
f32 mtx_cosf(f32 x);
void C_MTXFrustum(Mtx44 m, f32 top, f32 bottom, f32 left, f32 right, f32 near, f32 far);
void C_MTXPerspective(Mtx44 m, f32 fov, f32 aspect, f32 near, f32 far);
void C_MTXOrtho(Mtx44 m, f32 top, f32 bottom, f32 left, f32 right, f32 near, f32 far);
void C_MTXLightFrustum(Mtx m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 scaleS, f32 scaleT, f32 transS, f32 transT);
void C_MTXLightPerspective(Mtx m, f32 fovY, f32 aspect, f32 scaleS, f32 scaleT, f32 transS, f32 transT);
void C_MTXLightOrtho(Mtx m, f32 t, f32 b, f32 l, f32 r, f32 scaleS, f32 scaleT, f32 transS, f32 transT);

void PSMTXIdentity(Mtx m);
void PSMTXCopy(const Mtx src, Mtx dst);
void PSMTXConcat(const Mtx a, const Mtx b, Mtx ab);
u32 PSMTXInverse(const Mtx src, Mtx inv);
u32 PSMTXInvXpose(const Mtx src, Mtx inverse_transpose);
void PSMTXRotRad(Mtx m, char axis, f32 rad);
void PSMTXRotAxisRad(Mtx m, const Vec* axis, f32 angle);
void PSMTXTrans(Mtx m, f32 xT, f32 yT, f32 zT);
void PSMTXScale(Mtx m, f32 xS, f32 yS, f32 zS);
void PSMTXQuat(Mtx m, const Quaternion* q);
void C_MTXLookAt(Mtx m, const Vec* camPos, const Vec* camUp, const Vec* target);
void PSMTXMultVec(const Mtx m, const Vec* src, Vec* dst);
void PSMTXMultVecArray(const Mtx m, const Vec* srcBase, Vec* dstBase, u32 count);
void PSMTXMultVecSR(const Mtx m, const Vec* src, Vec* dst);

#ifdef __cplusplus
}
#endif

#endif
