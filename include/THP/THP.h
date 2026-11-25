#ifndef THP_H
#define THP_H

#include "types.h"
#include <sdk/dolphin/thp_api.h>

typedef u8 THPSample;
typedef s16 THPCoeff;
typedef f32 THPQuantTab[64];

typedef struct _THPHuffmanTab {
	u8 quick[32];
	u8 increment[32];
	u8* Vij;
	s32 maxCode[18];
	s32 valPtr[18];
	u8 Vij1;
	u8 pad[11];
} THPHuffmanTab;

typedef struct _THPComponent {
	u8 quantizationTableSelector;
	u8 DCTableSelector;
	u8 ACTableSelector;
	THPCoeff predDC;
} THPComponent;

typedef struct _THPFileInfo {
	THPQuantTab quantTabs[3];
	THPHuffmanTab huffmanTabs[4];
	THPComponent components[3];
	u16 xPixelSize;
	u16 yPixelSize;
	u16 MCUsPerRow;
	u16 decompressedY;
	u8* file;
	u32 currByte;
	u32 cnt;
	u8 validHuffmanTabs;
	u8 RST;
	u16 nMCU;
	u16 currMCU;
	u8* dLC[3];
} THPFileInfo;

typedef struct THPAudioRecordHeader {
	u32 offsetNextChannel;
	u32 sampleSize;
	s16 lCoef[8][2];
	s16 rCoef[8][2];
	s16 lYn1;
	s16 lYn2;
	s16 rYn1;
	s16 rYn2;
} THPAudioRecordHeader;

typedef struct THPAudioDecodeInfo {
	u8* encodeData;
	u32 offsetNibbles;
	u8 predictor;
	u8 scale;
	s16 yn1;
	s16 yn2;
} THPAudioDecodeInfo;

#endif
