#ifndef _mtx_transform_h_
#define _mtx_transform_h_

#include <dolphin/mtx/mtxtypes.h>

void HSD_MtxGetRotation(Mtx m, Vec* vec);
void HSD_MtxGetTranslate(Mtx mat, Vec* vec);
void HSD_MtxGetScale(Mtx arg0, Vec* arg1);
void HSD_MtxSRT(Mtx m, Vec* scale, Vec* rotation, Vec* translation, Vec* parent_scale);
void HSD_MtxSRTQuat(Mtx arg0, Vec* arg1, Quaternion* arg2, Vec* arg3,
                    Vec* arg4);

#endif
