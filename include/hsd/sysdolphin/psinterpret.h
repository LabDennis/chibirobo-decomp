#ifndef _psinterpret_h_
#define _psinterpret_h_

#include <sysdolphin/particle.h>

#ifdef __cplusplus
extern "C" {
#endif

HSD_Particle* psInterpretParticle0(HSD_Particle* pp, HSD_Particle* prev);
void psInterpretParticles(u32 flags);

#ifdef __cplusplus
}
#endif

#endif
