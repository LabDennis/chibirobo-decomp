// The Wii's public GX interface: the GameCube SDK's headers, minus the two FIFO getters the Wii changed.
#ifndef _REVOLUTION_GX_H_
#define _REVOLUTION_GX_H_

// On the Wii both copy the FIFO out and report whether it is set up (retail 0x801A6EA8 writes
// nine words through r3 and returns 0 or 1); the GameCube header declares them as returning the pointer.
#define GXGetCPUFifo __GXGetCPUFifo_dolphin
#define GXGetGPFifo __GXGetGPFifo_dolphin
#include <dolphin/gx.h>
#undef GXGetCPUFifo
#undef GXGetGPFifo

#ifdef __cplusplus
extern "C" {
#endif

GXBool GXGetCPUFifo(GXFifoObj* fifo);
GXBool GXGetGPFifo(GXFifoObj* fifo);

#ifdef __cplusplus
}
#endif

#endif
