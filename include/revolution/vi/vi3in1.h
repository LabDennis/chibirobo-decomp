#ifndef _REVOLUTION_VI_VI3IN1_H
#define _REVOLUTION_VI_VI3IN1_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

// Internal to the VI library: the A/V encoder ("3in1") behind the I2C bus.
// Only __VISetRGBModeImm is a name the binary gives; the others describe what the code does.

// requests the retrace handler carries out after VIFlush (bits of __VI3in1Changed)
#define VI_3IN1_CGMS        0x01
#define VI_3IN1_WSS         0x02
#define VI_3IN1_CC          0x04
#define VI_3IN1_MACROVISION 0x08
#define VI_3IN1_GAMMA       0x10
#define VI_3IN1_TRAP_FILTER 0x20
#define VI_3IN1_OVERDRIVE   0x40
#define VI_3IN1_RGB         0x80

extern volatile u32 __VI3in1Changed;

// i2c.c
void WaitMicroTime(s32 usec);
BOOL __VISendI2CData(u8 slaveAddr, u8* pData, s32 nBytes);

// vi3in1.c
void __VISetYUVSEL(u8 dtvStatus);
void __VISetFilter4EURGB60(u8 enable);
void __VISetCGMS(void);
void __VISetWSS(void);
void __VISetClosedCaption(void);
void __VISetMacrovision(void);
void __VISetGamma1_0(void);
void __VISetGamma(void);
void __VISetTrapFilter(void);
void __VISetRGBOverDrive(void);
void __VIRequestRGBMode(void);
void __VISetRGBModeImm(void);
void __VIInit3in1(void);

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

#endif
