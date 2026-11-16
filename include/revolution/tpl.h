// The Wii's TPL interface: the palette layout of the GameCube header plus the two functions the Wii SDK exports.
#ifndef _REVOLUTION_TPL_H_
#define _REVOLUTION_TPL_H_

#include "Dolphin/tpl.h"

#ifdef __cplusplus
extern "C" {
#endif

void TPLBind(TPLPalette* pal);
TPLDescriptor* TPLGet(TPLPalette* pal, u32 id);

#ifdef __cplusplus
}
#endif

#endif
