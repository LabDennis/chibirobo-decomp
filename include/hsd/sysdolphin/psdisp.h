#ifndef _psdisp_h_
#define _psdisp_h_

#include <sysdolphin/particle.h>

#ifdef __cplusplus
extern "C" {
#endif

void _psDispParticlesSub(s32 bankMask, s32 kind);
void psSetupTevCommon(void);
void psSetupTevInvalidState(void);
void psSetupTev(HSD_Particle* pp);

#ifdef __cplusplus
}
#endif

#endif
