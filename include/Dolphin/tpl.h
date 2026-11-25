#ifndef _DOLPHIN_TPL_H
#define _DOLPHIN_TPL_H

#include "types.h"
#ifdef _MTXTYPES_h_
#include <dolphin/gx/gxtypes.h>
#else
#include "Dolphin/gx.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

// sdk TPL texture palette layout. the file keeps offsets until it's bound, then pointers

typedef struct TPLHeader {
	u16 height;             // _00
	u16 width;              // _02
	u32 format;             // _04
	char* data;             // _08
	GXTexWrapMode wrapS;    // _0C
	GXTexWrapMode wrapT;    // _10
	GXTexFilter minFilter;  // _14
	GXTexFilter magFilter;  // _18
	f32 LODBias;            // _1C
	u8 edgeLODEnable;       // _20
	u8 minLOD;              // _21
	u8 maxLOD;              // _22
	u8 unpacked;            // _23
} TPLHeader, *TPLHeaderPtr; // size 0x24

typedef struct TPLClutHeader {
	u16 numEntries;                 // _00
	u8 unpacked;                    // _02
	u8 pad8;                        // _03
	GXTlutFmt format;               // _04
	char* data;                     // _08
} TPLClutHeader, *TPLClutHeaderPtr; // size 0xC

typedef struct TPLDescriptor {
	TPLHeaderPtr textureHeader;     // _00
	TPLClutHeaderPtr CLUTHeader;    // _04
} TPLDescriptor, *TPLDescriptorPtr; // size 0x8

typedef struct TPLPalette {
	u32 versionNumber;                // _00
	u32 numDescriptors;               // _04
	TPLDescriptorPtr descriptorArray; // _08
} TPLPalette, *TPLPalettePtr;         // size 0xC

#ifdef __cplusplus
};
#endif // ifdef __cplusplus

#endif
