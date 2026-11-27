#include "THP/THP.h"
#include "Dolphin/os.h"
#include <string.h>

#ifndef __MWERKS__
f32 __fmadds(f32, f32, f32);
f32 __fnmsubs(f32, f32, f32);
#endif

static void __THPSetupBuffers(void);
static u8 __THPReadFrameHeader(void);
static u8 __THPReadScaneHeader(void);
static u8 __THPReadQuantizationTable(void);
static u8 __THPReadHuffmanTableSpecification(void);
static void __THPHuffGenerateSizeTable(void);
static void __THPHuffGenerateCodeTable(void);
static void __THPHuffGenerateDecoderTables(u8 tabIndex);
static void __THPRestartDefinition(void);
static void __THPDecompressYUV(void*, void*, void*);

static const u8 __THPJpegNaturalOrder[80] = {
	0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
	12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63,
	63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
};

static const f64 __THPAANScaleFactor[8] = {
	1.0f, 1.387039845f, 1.306562965f, 1.175875602f, 1.0f, 0.785694958f, 0.541196100f, 0.275899379f,
};

char* __THPVersion = "<< Dolphin SDK - THP\trelease build: Jan  9 2004 13:06:55 (0x2301) >>";

static THPHuffmanTab* Ydchuff ATTRIBUTE_ALIGN(32);
static THPHuffmanTab* Udchuff ATTRIBUTE_ALIGN(32);
static THPHuffmanTab* Vdchuff ATTRIBUTE_ALIGN(32);
static THPHuffmanTab* Yachuff ATTRIBUTE_ALIGN(32);
static THPHuffmanTab* Uachuff ATTRIBUTE_ALIGN(32);
static THPHuffmanTab* Vachuff ATTRIBUTE_ALIGN(32);
static u8* __THPHuffmanBits;
static u8* __THPHuffmanSizeTab;
static u16* __THPHuffmanCodeTab;
// Retail keeps the transform workspace in .bss; melee and mkdd keep the same SDK global.
static f32 __THPIDCTWorkspace[64] ATTRIBUTE_ALIGN(32);
static u8* __THPLCWork512[3];
static u8* __THPLCWork672[3];
static u8* __THPWorkArea;
static THPCoeff* __THPMCUBuffer[6];
static THPFileInfo* __THPInfo;
static BOOL __THPInitFlag = FALSE;

#define THPROUNDUP(a, b) ((((s32)(a)) + ((s32)(b) - 1L)) / ((s32)(b)))

enum {
	JPEG_MARKER_SOF0    = 0xC0,
	JPEG_MARKER_DHT     = 0xC4,
	JPEG_MARKER_RST7    = 0xD7,
	JPEG_MARKER_SOI     = 0xD8,
	JPEG_MARKER_SOS     = 0xDA,
	JPEG_MARKER_DQT     = 0xDB,
	JPEG_MARKER_DRI     = 0xDD,
	JPEG_MARKER_EXP     = 0xDF,
	JPEG_MARKER_APP0    = 0xE0,
	JPEG_MARKER_APP15   = 0xEF,
	JPEG_MARKER_COM     = 0xFE,
	JPEG_MARKER_PREFIX  = 0xFF,
};

enum {
	THP_VIDEO_OK = 0,
	THP_VIDEO_BAD_MARKER_PREFIX = 3,
	THP_VIDEO_UNSUPPORTED_PRECISION = 10,
	THP_VIDEO_UNSUPPORTED_MARKER = 11,
	THP_VIDEO_UNSUPPORTED_COMPONENTS = 12,
	THP_VIDEO_MISSING_HUFFMAN_TABLE = 15,
	THP_VIDEO_UNSUPPORTED_SAMPLING = 19,
	THP_VIDEO_NO_INPUT = 25,
	THP_VIDEO_NO_WORK_AREA = 26,
	THP_VIDEO_NO_OUTPUT = 27,
	THP_VIDEO_LOCKED_CACHE_DISABLED = 28,
	THP_VIDEO_NOT_INITIALIZED = 29,
};

/**
 * Decodes one THP video frame.
 *
 * @param file      Pointer to the encoded frame.
 * @param tileY     Pointer to the output tile for Y component.
 * @param tileU     Pointer to the output tile for U component.
 * @param tileV     Pointer to the output tile for V component.
 * @param workArea  Pointer to the decoder work area.
 * @return          Zero on success or a THP error code.
 *
 * @note Address: 0x801F5C7C
 * @note Size: 0x244
 */
s32 THPVideoDecode(void* file, void* tileY, void* tileU, void* tileV, void* workArea)
{
	u8 allDone;
	u8 marker;
	u8 status;
	s32 errorCode;

	if (!file) {
		goto _err_no_input;
	}

	if (tileY == NULL || tileU == NULL || tileV == NULL) {
		goto _err_no_output;
	}

	if (!workArea) {
		goto _err_no_work;
	}

	if (!(PPCMfhid2() & HID2_LCE)) {
		goto _err_lc_not_enabled;
	}

	if (__THPInitFlag == FALSE) {
		goto _err_not_initialized;
	}

	__THPWorkArea = (u8*)workArea;
	__THPInfo     = (THPFileInfo*)OSRoundUp32B(__THPWorkArea);
	__THPWorkArea = (u8*)OSRoundUp32B(__THPWorkArea) + sizeof(THPFileInfo);

	DCZeroRange(__THPInfo, sizeof(THPFileInfo));
	__THPInfo->cnt           = 33;
	__THPInfo->decompressedY = 0;
	__THPInfo->file          = (u8*)file;

	allDone = FALSE;

	for (;;) {
		if (*__THPInfo->file++ != JPEG_MARKER_PREFIX) {
			goto _err_bad_syntax;
		}

		while (*__THPInfo->file == JPEG_MARKER_PREFIX) {
			__THPInfo->file++;
		}

		marker = *__THPInfo->file++;

		if (marker <= JPEG_MARKER_RST7) {
			if (marker == JPEG_MARKER_DHT) {
				status = __THPReadHuffmanTableSpecification();

				if (status != THP_VIDEO_OK) {
					goto _err_bad_status;
				}
			} else if (marker == JPEG_MARKER_SOF0) {
				status = __THPReadFrameHeader();
				if (status != THP_VIDEO_OK) {
					goto _err_bad_status;
				}
			} else {
				goto _err_unsupported_marker;
			}
		} else if (JPEG_MARKER_SOI <= marker && marker <= JPEG_MARKER_EXP) {
			if (marker == JPEG_MARKER_DRI) {
				__THPRestartDefinition();
			} else if (marker == JPEG_MARKER_DQT) {
				status = __THPReadQuantizationTable();
				if (status != THP_VIDEO_OK) {
					goto _err_bad_status;
				}
			} else if (marker == JPEG_MARKER_SOS) {
				status = __THPReadScaneHeader();
				if (status != THP_VIDEO_OK) {
					goto _err_bad_status;
				}

				allDone = TRUE;
			} else if (marker == JPEG_MARKER_SOI) {
				// The start-of-image marker has no segment payload.
			} else {
				goto _err_unsupported_marker;
			}
		} else if (JPEG_MARKER_APP0 <= marker) {
			if ((JPEG_MARKER_APP0 <= marker && marker <= JPEG_MARKER_APP15) || marker == JPEG_MARKER_COM) {
				__THPInfo->file += (__THPInfo->file[0] << 8) | __THPInfo->file[1];
			} else {
				goto _err_unsupported_marker;
			}
		}

		if (allDone) {
			break;
		}
	}

	__THPSetupBuffers();
	__THPDecompressYUV(tileY, tileU, tileV);
	return THP_VIDEO_OK;

_err_no_input:
	errorCode = THP_VIDEO_NO_INPUT;
	goto _err_exit;

_err_no_output:
	errorCode = THP_VIDEO_NO_OUTPUT;
	goto _err_exit;

_err_no_work:
	errorCode = THP_VIDEO_NO_WORK_AREA;
	goto _err_exit;

_err_unsupported_marker:
	errorCode = THP_VIDEO_UNSUPPORTED_MARKER;
	goto _err_exit;

_err_bad_syntax:
	errorCode = THP_VIDEO_BAD_MARKER_PREFIX;
	goto _err_exit;

_err_bad_status:
	errorCode = status;
	goto _err_exit;

_err_lc_not_enabled:
	errorCode = THP_VIDEO_LOCKED_CACHE_DISABLED;
	goto _err_exit;

_err_not_initialized:
	errorCode = THP_VIDEO_NOT_INITIALIZED;
	goto _err_exit;

_err_exit:
	return errorCode;
}

/**
 * @note Address: 0x801F5EC0
 * @note Size: 0x44
 */
static void __THPSetupBuffers(void)
{
	u8 i;
	THPCoeff* buffer;

	buffer = (THPCoeff*)OSRoundUp32B(__THPWorkArea);

	for (i = 0; i < 6; i++) {
		__THPMCUBuffer[i] = &buffer[i * 64];
	}
}

/**
 * @note Address: 0x801F5F04
 * @note Size: 0x13C
 */
static u8 __THPReadFrameHeader(void)
{
	u8 i, utmp8;

	__THPInfo->file += 2;

	utmp8 = *__THPInfo->file++;

	if (utmp8 != 8) {
		return THP_VIDEO_UNSUPPORTED_PRECISION;
	}

	__THPInfo->yPixelSize = (u16)((__THPInfo->file[0] << 8) | __THPInfo->file[1]);
	__THPInfo->file += 2;
	__THPInfo->xPixelSize = (u16)((__THPInfo->file[0] << 8) | __THPInfo->file[1]);
	__THPInfo->file += 2;

	utmp8 = *__THPInfo->file++;
	if (utmp8 != 3) {
		return THP_VIDEO_UNSUPPORTED_COMPONENTS;
	}

	for (i = 0; i < 3; i++) {
		utmp8 = *__THPInfo->file++;
		utmp8 = *__THPInfo->file++;
		if ((i == 0 && utmp8 != 0x22) || (i > 0 && utmp8 != 0x11)) {
			return THP_VIDEO_UNSUPPORTED_SAMPLING;
		}

		__THPInfo->components[i].quantizationTableSelector = *__THPInfo->file++;
	}

	return THP_VIDEO_OK;
}

/**
 * @note Address: 0x801F6040
 * @note Size: 0x11C
 */
static u8 __THPReadScaneHeader(void)
{
	u8 i, utmp8;
	__THPInfo->file += 2;

	utmp8 = *__THPInfo->file++;

	if (utmp8 != 3) {
		return THP_VIDEO_UNSUPPORTED_COMPONENTS;
	}

	for (i = 0; i < 3; i++) {
		utmp8 = *__THPInfo->file++;

		utmp8                                    = *__THPInfo->file++;
		__THPInfo->components[i].DCTableSelector = (u8)(utmp8 >> 4);
		__THPInfo->components[i].ACTableSelector = (u8)(utmp8 & 15);

		if ((__THPInfo->validHuffmanTabs & (1 << ((utmp8 >> 4)))) == 0) {
			return THP_VIDEO_MISSING_HUFFMAN_TABLE;
		}

		if ((__THPInfo->validHuffmanTabs & (1 << ((utmp8 & 15) + 1))) == 0) {
			return THP_VIDEO_MISSING_HUFFMAN_TABLE;
		}
	}

	__THPInfo->file += 3;
	__THPInfo->MCUsPerRow           = (u16)THPROUNDUP(__THPInfo->xPixelSize, 16);
	__THPInfo->components[0].predDC = 0;
	__THPInfo->components[1].predDC = 0;
	__THPInfo->components[2].predDC = 0;
	return THP_VIDEO_OK;
}

/**
 * @note Address: 0x801F615C
 * @note Size: 0x3BC
 */
static u8 __THPReadQuantizationTable(void)
{
	u16 length, id, i, row, col;
	f32 q_temp[64];

	length = (u16)((__THPInfo->file[0] << 8) | __THPInfo->file[1]);
	__THPInfo->file += 2;
	length -= 2;

	for (;;) {
		id = *__THPInfo->file++;

		for (i = 0; i < 64; i++) {
			q_temp[__THPJpegNaturalOrder[i]] = (f32)*__THPInfo->file++;
		}

		i = 0;
		for (row = 0; row < 8; row++) {
			for (col = 0; col < 8; col++) {
				__THPInfo->quantTabs[id][i] = (f32)((f64)q_temp[i] * __THPAANScaleFactor[row] * __THPAANScaleFactor[col]);
				i++;
			}
		}

		length -= 65;
		if (!length) {
			break;
		}
	}

	return THP_VIDEO_OK;
}

/**
 * @note Address: 0x801F6518
 * @note Size: 0x1E0
 */
static u8 __THPReadHuffmanTableSpecification(void)
{
	u8 t_class, id, i, tab_index;
	u16 length, num_Vij;

	__THPHuffmanSizeTab = __THPWorkArea;
	__THPHuffmanCodeTab = (u16*)(__THPWorkArea + 256 + 1);
	length              = (u16)((__THPInfo->file[0] << 8) | __THPInfo->file[1]);
	__THPInfo->file += 2;
	length -= 2;

	for (;;) {
		i                = *__THPInfo->file++;
		id               = (u8)(i & 15);
		t_class          = (u8)(i >> 4);
		__THPHuffmanBits = __THPInfo->file;
		tab_index        = (u8)((id << 1) + t_class);
		num_Vij          = 0;

		for (i = 0; i < 16; i++) {
			num_Vij += *__THPInfo->file++;
		}

		__THPInfo->huffmanTabs[tab_index].Vij = __THPInfo->file;
		__THPInfo->file += num_Vij;
		__THPHuffGenerateSizeTable();
		__THPHuffGenerateCodeTable();
		__THPHuffGenerateDecoderTables(tab_index);
		__THPInfo->validHuffmanTabs |= 1 << tab_index;
		length -= 17 + num_Vij;

		if (length == 0) {
			break;
		}
	}

	return THP_VIDEO_OK;
}

/**
 * @note Address: 0x801F66F8
 * @note Size: 0xF0
 */
static void __THPHuffGenerateSizeTable(void)
{
	s32 p, l, i;
	p = 0;

	for (l = 1; l <= 16; l++) {
		i = (s32)__THPHuffmanBits[l - 1];
		while (i--) {
			__THPHuffmanSizeTab[p++] = (u8)l;
		}
	}

	__THPHuffmanSizeTab[p] = 0;
}

/**
 * @note Address: 0x801F67E8
 * @note Size: 0x68
 */
static void __THPHuffGenerateCodeTable(void)
{
	u8 si;
	u16 p, code;

	p    = 0;
	code = 0;
	si   = __THPHuffmanSizeTab[0];

	while (__THPHuffmanSizeTab[p]) {
		while (__THPHuffmanSizeTab[p] == si) {
			__THPHuffmanCodeTab[p++] = code;
			code++;
		}

		code <<= 1;
		si++;
	}
}

/**
 * @note Address: 0x801F6850
 * @note Size: 0x1BC
 */
static void __THPHuffGenerateDecoderTables(u8 tabIndex)
{
	s32 p, l;
	THPHuffmanTab* h;

	p = 0;
	h = &__THPInfo->huffmanTabs[tabIndex];
	for (l = 1; l <= 16; l++) {
		if (__THPHuffmanBits[l - 1]) {
			h->valPtr[l] = p - __THPHuffmanCodeTab[p];
			p += __THPHuffmanBits[l - 1];
			h->maxCode[l] = __THPHuffmanCodeTab[p - 1];
		} else {
			h->maxCode[l] = -1;
			h->valPtr[l]  = -1;
		}
	}

	h->maxCode[17] = 0xfffffL;
}

/**
 * @note Address: 0x801F6A0C
 * @note Size: 0x54
 */
static void __THPRestartDefinition(void)
{
	__THPInfo->RST = TRUE;
	__THPInfo->file += 2;
	__THPInfo->nMCU = (u16)((__THPInfo->file[0] << 8) | __THPInfo->file[1]);
	__THPInfo->file += 2;
	__THPInfo->currMCU = __THPInfo->nMCU;
}

static u32 __THPReadStreamWord(const u8* data)
{
	return ((u32)data[0] << 24) | ((u32)data[1] << 16) | ((u32)data[2] << 8) | data[3];
}

/*
 * file points to the cached big-endian word. cnt is the one-based index of
 * the next bit, with 33 meaning that the word has been exhausted.
 */
static inline u32 __THPReadBits(THPFileInfo* info, u32 count)
{
	u32 remaining = 33 - info->cnt;
	u32 bits = 0;

	if (count > remaining) {
		bits = info->currByte & ((1UL << remaining) - 1);
		info->file += 4;
		info->currByte = __THPReadStreamWord(info->file);
		info->cnt = 1;
		count -= remaining;
		remaining = 32;
	}

	bits = (bits << count) | ((info->currByte >> (remaining - count)) & ((1UL << count) - 1));
	info->cnt += count;
	return bits;
}

static void __THPPrepBitStream(void)
{
	u32* ptr;
	u32 offset, i, j, k;

	ptr    = (u32*)((u32)__THPInfo->file & 0xFFFFFFFC);
	offset = (u32)__THPInfo->file & 3;

	if (__THPInfo->cnt != 33) {
		__THPInfo->cnt -= (3 - offset) * 8;
	} else {
		__THPInfo->cnt = (offset * 8) + 1;
	}

	__THPInfo->file     = (u8*)ptr;
	__THPInfo->currByte = *ptr;

	for (i = 0; i < 4; i++) {
		if (__THPInfo->validHuffmanTabs & (1 << i)) {
			for (j = 0; j < 32; j++) {
				__THPInfo->huffmanTabs[i].quick[j] = 0xFF;

				for (k = 0; k < 5; k++) {
					s32 code = (s32)(j >> (5 - k - 1));

					if (code <= __THPInfo->huffmanTabs[i].maxCode[k + 1]) {
						__THPInfo->huffmanTabs[i].quick[j]
						    = __THPInfo->huffmanTabs[i].Vij[(s32)(code + __THPInfo->huffmanTabs[i].valPtr[k + 1])];
						__THPInfo->huffmanTabs[i].increment[j] = (u8)(k + 1);
						k                                      = 99;
					}
				}
			}
		}
	}

	{
		s32 YdcTab, UdcTab, VdcTab, YacTab, UacTab, VacTab;

		YdcTab = (__THPInfo->components[0].DCTableSelector << 1);
		UdcTab = (__THPInfo->components[1].DCTableSelector << 1);
		VdcTab = (__THPInfo->components[2].DCTableSelector << 1);

		YacTab = (__THPInfo->components[0].ACTableSelector << 1) + 1;
		UacTab = (__THPInfo->components[1].ACTableSelector << 1) + 1;
		VacTab = (__THPInfo->components[2].ACTableSelector << 1) + 1;

		Ydchuff = &__THPInfo->huffmanTabs[YdcTab];
		Udchuff = &__THPInfo->huffmanTabs[UdcTab];
		Vdchuff = &__THPInfo->huffmanTabs[VdcTab];

		Yachuff = &__THPInfo->huffmanTabs[YacTab];
		Uachuff = &__THPInfo->huffmanTabs[UacTab];
		Vachuff = &__THPInfo->huffmanTabs[VacTab];
	}
}

static int __THPReadHuffmanSymbol(THPFileInfo* info, const THPHuffmanTab* table)
{
	s32 code = 0;
	int length = 0;

	if (info->cnt <= 28) {
		u32 prefix = (info->currByte >> (28 - info->cnt)) & 31;
		if (table->quick[prefix] != 0xFF) {
			info->cnt += table->increment[prefix];
			return table->quick[prefix];
		}
		info->cnt += 5;
		code = (s32)prefix;
		length = 5;
	}

	while (length < 16) {
		code = (code << 1) | __THPReadBits(info, 1);
		length++;
		if (code <= table->maxCode[length]) {
			return table->Vij[code + table->valPtr[length]];
		}
	}

	return -1;
}

static inline s32 __THPReadCoefficient(THPFileInfo* info, u32 width)
{
	s32 bits = (s32)__THPReadBits(info, width);

	if (bits < (1L << (width - 1))) {
		bits -= (1L << width) - 1;
	}
	return bits;
}

static void __THPDecodeBlock(THPFileInfo* info, THPComponent* component, THPCoeff* block)
{
	const THPHuffmanTab* dc = &info->huffmanTabs[component->DCTableSelector * 2];
	const THPHuffmanTab* ac = &info->huffmanTabs[component->ACTableSelector * 2 + 1];
	int width;
	int position;

	memset(block, 0, 64 * sizeof(*block));
	width = __THPReadHuffmanSymbol(info, dc);
	if (width < 0 || width > 16) {
		return;
	}

	if (width != 0) {
		component->predDC = (THPCoeff)(component->predDC + __THPReadCoefficient(info, (u32)width));
	}
	block[0] = component->predDC;

	for (position = 1; position < 64; position++) {
		int symbol = __THPReadHuffmanSymbol(info, ac);

		if (symbol == 0xF0) {
			position += 15;
			continue;
		}
		if (symbol <= 0 || !(symbol & 15)) {
			break;
		}

		position += symbol >> 4;
		if (position >= 64) {
			break;
		}
		block[__THPJpegNaturalOrder[position]] = (THPCoeff)__THPReadCoefficient(info, (u32)(symbol & 15));
	}
}

/* Twice the relevant cosines and the combinations used by the AAN inverse DCT. */
static const f32 THP_SQRT2 = 1.414213562f;
static const f32 THP_COS_2 = 1.847759065f;
static const f32 THP_COS_2_PLUS_COS_6 = 2.613125930f;
static const f32 THP_COS_2_MINUS_COS_6 = 1.082392200f;

static void __THPIDCTRow(const THPCoeff* coeff, const f32* quant, f32* out)
{
	f32 dc = coeff[0] * quant[0];
	f32 ac1 = coeff[1] * quant[1];
	f32 ac2, ac3;
	f32 evenSum0, evenDiff0, evenSum1, evenDiff1;
	f32 oddSum0, oddDiff0, oddSum1, oddDiff1;
	f32 even0, even1, even2, even3;
	f32 odd0, odd1, odd2, odd3;
	f32 rotation;

	if (!coeff[4] && !coeff[5] && !coeff[6] && !coeff[7]) {
		if (!coeff[2] && !coeff[3]) {
			if (!coeff[1]) {
				int i;

				for (i = 0; i < 8; i++) {
					out[i] = dc;
				}
				return;
			}

			odd1 = __fmadds(ac1, THP_COS_2, -ac1);
			odd2 = __fmadds(ac1, THP_SQRT2, -odd1);
			odd3 = __fnmsubs(ac1, THP_COS_2 - THP_COS_2_MINUS_COS_6, odd2);
			out[0] = dc + ac1;
			out[1] = dc + odd1;
			out[2] = dc + odd2;
			out[3] = dc + odd3;
			out[4] = dc - odd3;
			out[5] = dc - odd2;
			out[6] = dc - odd1;
			out[7] = dc - ac1;
			return;
		}

		ac2 = coeff[2] * quant[2];
		ac3 = coeff[3] * quant[3];
		even0 = dc + ac2;
		even3 = dc - ac2;
		even1 = __fmadds(ac2, THP_SQRT2, even3);
		even2 = __fnmsubs(ac2, THP_SQRT2, even0);

		odd0 = ac1 + ac3;
		rotation = (ac1 - ac3) * THP_COS_2;
		odd1 = __fnmsubs(ac3, -THP_COS_2_PLUS_COS_6, rotation) - odd0;
		odd2 = __fmadds(ac1 - ac3, THP_SQRT2, -odd1);
		odd3 = __fnmsubs(ac1, THP_COS_2_MINUS_COS_6, rotation) - odd2;
		out[0] = even0 + odd0;
		out[1] = even1 + odd1;
		out[2] = even2 + odd2;
		out[3] = even3 + odd3;
		out[4] = even3 - odd3;
		out[5] = even2 - odd2;
		out[6] = even1 - odd1;
		out[7] = even0 - odd0;
		return;
	}

	ac2 = coeff[2] * quant[2];
	ac3 = coeff[3] * quant[3];
	evenSum0 = __fmadds(coeff[4], quant[4], dc);
	evenDiff0 = __fnmsubs(coeff[4], quant[4], dc);
	evenSum1 = __fmadds(coeff[6], quant[6], ac2);
	evenDiff1 = __fnmsubs(coeff[6], quant[6], ac2);
	even0 = evenSum0 + evenSum1;
	even3 = evenSum0 - evenSum1;
	rotation = __fmadds(evenDiff1, THP_SQRT2, -evenSum1);
	even1 = evenDiff0 + rotation;
	even2 = evenDiff0 - rotation;

	oddSum0 = __fmadds(coeff[5], quant[5], ac3);
	oddDiff0 = __fnmsubs(coeff[5], quant[5], ac3);
	oddSum1 = __fmadds(coeff[7], quant[7], ac1);
	oddDiff1 = __fnmsubs(coeff[7], quant[7], ac1);
	odd0 = oddSum0 + oddSum1;
	rotation = (oddDiff1 - oddDiff0) * THP_COS_2;
	odd1 = __fnmsubs(oddDiff0, -THP_COS_2_PLUS_COS_6, rotation) - odd0;
	odd2 = __fmadds(oddSum0 - oddSum1, THP_SQRT2, odd1);
	odd3 = __fmadds(oddDiff1, THP_COS_2_MINUS_COS_6, -rotation) - odd2;

	out[0] = even0 + odd0;
	out[1] = even1 + odd1;
	out[2] = even2 - odd2;
	out[3] = even3 - odd3;
	out[4] = even3 + odd3;
	out[5] = even2 + odd2;
	out[6] = even1 - odd1;
	out[7] = even0 - odd0;
}

static THPSample __THPIDCTSample(f32 value)
{
	value *= 0.125f;
	if (value <= 0.0f) {
		return 0;
	}
	if (value >= 255.0f) {
		return 255;
	}
	return (THPSample)value;
}

static void __THPIDCTColumn(const f32* in, THPSample* top, THPSample* bottom)
{
	// The unnormalized transform yields eight times the pixel value.
	f32 evenSum0 = (in[0 * 8] + in[4 * 8]) + 1024.0f;
	f32 evenDiff0 = (in[0 * 8] - in[4 * 8]) + 1024.0f;
	f32 evenSum1 = in[2 * 8] + in[6 * 8];
	f32 evenDiff1 = in[2 * 8] - in[6 * 8];
	f32 evenRotation = __fmadds(evenDiff1, THP_SQRT2, -evenSum1);
	f32 oddSum0 = in[5 * 8] + in[3 * 8];
	f32 oddDiff0 = in[5 * 8] - in[3 * 8];
	f32 oddSum1 = in[1 * 8] + in[7 * 8];
	f32 oddDiff1 = in[1 * 8] - in[7 * 8];
	f32 oddRotation = (oddDiff0 + oddDiff1) * THP_COS_2;
	f32 even0 = evenSum0 + evenSum1;
	f32 even1 = evenDiff0 + evenRotation;
	f32 even2 = evenDiff0 - evenRotation;
	f32 even3 = evenSum0 - evenSum1;
	f32 odd0 = oddSum1 + oddSum0;
	f32 odd1 = __fmadds(oddDiff0, -THP_COS_2_PLUS_COS_6, oddRotation) - odd0;
	f32 odd2 = __fmadds(oddSum1 - oddSum0, THP_SQRT2, -odd1);
	f32 odd3 = __fmadds(oddDiff1, THP_COS_2_MINUS_COS_6, -oddRotation) + odd2;

	top[0] = __THPIDCTSample(even0 + odd0);
	top[8] = __THPIDCTSample(even1 + odd1);
	top[16] = __THPIDCTSample(even2 + odd2);
	top[24] = __THPIDCTSample(even3 - odd3);
	bottom[0] = __THPIDCTSample(even3 + odd3);
	bottom[8] = __THPIDCTSample(even2 - odd2);
	bottom[16] = __THPIDCTSample(even1 - odd1);
	bottom[24] = __THPIDCTSample(even0 - odd0);
}

static void __THPInverseDCT(const THPCoeff* coeff, const f32* quant, THPSample* tile, u32 width)
{
	f32* workspace = __THPIDCTWorkspace;
	THPSample* bottom = tile + 4 * width;
	int i;

	for (i = 0; i < 8; i++) {
		__THPIDCTRow(coeff + i * 8, quant + i * 8, workspace + i * 8);
	}
	// Each 8x8 block is stored in two stacked groups of four scanlines.
	for (i = 0; i < 8; i++) {
		__THPIDCTColumn(workspace + i, tile + i, bottom + i);
	}
}

static void __THPDecompressRow(THPFileInfo* info, u8* const work[3])
{
	u32 x;
	u32 width = info->xPixelSize;
	u32 chromaWidth = width / 2;
	u32 yBytes = 256 * (width / 16);
	u32 chromaBytes = yBytes / 4;
	const f32* lumaQuant = info->quantTabs[info->components[0].quantizationTableSelector];
	int i;

	LCQueueWait(3);
	for (x = 0; x < info->MCUsPerRow; x++) {
		for (i = 0; i < 6; i++) {
			__THPDecodeBlock(info, &info->components[i < 4 ? 0 : i - 3], __THPMCUBuffer[i]);
		}

		for (i = 0; i < 4; i++) {
			u8* tile = work[0] + x * 64 + (i & 1) * 32 + (i / 2) * 8 * width;

			__THPInverseDCT(__THPMCUBuffer[i], lumaQuant, tile, width);
		}
		for (i = 1; i < 3; i++) {
			const f32* quant = info->quantTabs[info->components[i].quantizationTableSelector];

			__THPInverseDCT(__THPMCUBuffer[i + 3], quant, work[i] + x * 32, chromaWidth);
		}

		if (info->RST && --info->currMCU == 0) {
			info->currMCU = info->nMCU;
			info->cnt = 1 + ((info->cnt + 6) & ~7UL);

			for (i = 0; i < 3; i++) {
				info->components[i].predDC = 0;
			}
		}
	}

	LCStoreData(info->dLC[0], work[0], yBytes);
	LCStoreData(info->dLC[1], work[1], chromaBytes);
	LCStoreData(info->dLC[2], work[2], chromaBytes);
	info->dLC[0] += yBytes;
	info->dLC[1] += chromaBytes;
	info->dLC[2] += chromaBytes;
}

static void __THPDecompressYUV(void* tileY, void* tileU, void* tileV)
{
	THPFileInfo* info = __THPInfo;
	u8** work = __THPLCWork672;
	u32 y;

	info->dLC[0] = tileY;
	info->dLC[1] = tileU;
	info->dLC[2] = tileV;
	__THPPrepBitStream();

	if (info->xPixelSize == 512 && info->yPixelSize == 448) {
		work = __THPLCWork512;
	}
	for (y = info->decompressedY; y < info->yPixelSize; y += 16) {
		__THPDecompressRow(info, work);
	}
}

/**
 * @note Address: 0x801FD144
 * @note Size: 0xA0
 */

BOOL THPInit(void)
{
	u8* base;
	OSRegisterVersion(__THPVersion);
	base = (u8*)0xE0000000u;

	__THPLCWork512[0] = base;
	base += 0x2000;
	__THPLCWork512[1] = base;
	base += 0x800;
	__THPLCWork512[2] = base;
	base += 0x200;

	base              = (u8*)0xE0000000u;
	__THPLCWork672[0] = base;
	base += 0x2A00;
	__THPLCWork672[1] = base;
	base += 0xA80;
	__THPLCWork672[2] = base;
	base += 0xA80;

	OSInitFastCast();

	__THPInitFlag = TRUE;
	return TRUE;
}
