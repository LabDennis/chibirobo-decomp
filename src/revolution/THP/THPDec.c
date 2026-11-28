#include "revolution/thp.h"

#include <string.h>

const char* __THPVersion = "<< RVL_SDK - THP \trelease build: Jul 30 2008 19:01:42 (0x4199_60831) >>";

static THPHuffmanTab* Ydchuff ATTRIBUTE_ALIGN(32);
static THPHuffmanTab* Udchuff ATTRIBUTE_ALIGN(32);
static THPHuffmanTab* Vdchuff ATTRIBUTE_ALIGN(32);
static THPHuffmanTab* Yachuff ATTRIBUTE_ALIGN(32);
static THPHuffmanTab* Uachuff ATTRIBUTE_ALIGN(32);
static THPHuffmanTab* Vachuff ATTRIBUTE_ALIGN(32);
static f32 __THPIDCTWorkspace[64] ATTRIBUTE_ALIGN(32);
static u8* __THPHuffmanBits;
static u8* __THPHuffmanSizeTab;
static u16* __THPHuffmanCodeTab;
static THPSample* Gbase ATTRIBUTE_ALIGN(32);
static u32 Gwid ATTRIBUTE_ALIGN(32);
static f32* Gq ATTRIBUTE_ALIGN(32);
static u8* __THPLCWork512[3];
static u8* __THPLCWork672[3];
static u8* __THPWorkArea;
static THPCoeff* __THPMCUBuffer[6];
static THPFileInfo* __THPInfo;
static BOOL __THPInitFlag = FALSE;

#define THPROUNDUP(a, b) ((((s32)(a)) + ((s32)(b) - 1L)) / ((s32)(b)))

void __THPInverseDCTY8(THPCoeff*, u32);

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
static void __THPDecompressiMCURow512x448(void);
static void __THPDecompressiMCURow640x480(void);
static void __THPDecompressiMCURowNxN(void);
static void __THPHuffDecodeDCTCompY(THPFileInfo*, THPCoeff*);
static void __THPHuffDecodeDCTCompU(THPFileInfo*, THPCoeff*);
static void __THPHuffDecodeDCTCompV(THPFileInfo*, THPCoeff*);

/**
 * Decodes a THP video file.
 *
 * @param file   Pointer to the THP video file.
 * @param tileY  Pointer to the output tile for Y component.
 * @param tileU  Pointer to the output tile for U component.
 * @param tileV  Pointer to the output tile for V component.
 * @param work   Pointer to the work area.
 * @return       Error code indicating the success or failure of the decoding process.
 *
 */
s32 THPVideoDecode(void* file, void* tileY, void* tileU, void* tileV, void* workArea)
{
	u8 all_done, status;
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

	if (!(PPCMfhid2() & 0x10000000)) {
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

	all_done = FALSE;

	for (;;) {
		if ((*(__THPInfo->file)++) != 0xFF) {
			goto _err_bad_syntax;
		}

		while (*__THPInfo->file == 0xFF) {
			((__THPInfo->file)++);
		}

		status = (*(__THPInfo->file)++);

		if (status <= 0xD7) {
			if (status == 0xC4) {
				status = __THPReadHuffmanTableSpecification();

				if (status != 0) {
					goto _err_bad_status;
				}
			}

			else if (status == 192) {
				status = __THPReadFrameHeader();
				if (status != 0) {
					goto _err_bad_status;
				}
			}

			else {
				goto _err_unsupported_marker;
			}
		}

		else if (0xD8 <= status && status <= 0xDF) {
			if (status == 221) {
				__THPRestartDefinition();
			}

			else if (status == 219) {
				status = __THPReadQuantizationTable();
				if (status != 0) {
					goto _err_bad_status;
				}
			}

			else if (status == 218) {
				status = __THPReadScaneHeader();
				if (status != 0) {
					goto _err_bad_status;
				}

				all_done = TRUE;
			} else if (status == 216) {
				// empty but required for match
			} else {
				goto _err_unsupported_marker;
			}
		}

		else if (0xE0 <= status) {
			if ((224 <= status && status <= 239) || status == 254) {
				__THPInfo->file += (__THPInfo->file)[0] << 8 | (__THPInfo->file)[1];
			} else {
				goto _err_unsupported_marker;
			}
		}

		if (all_done) {
			break;
		}
	}

	__THPSetupBuffers();
	__THPDecompressYUV(tileY, tileU, tileV);
	return 0;

_err_no_input:
	errorCode = 25;
	goto _err_exit;

_err_no_output:
	errorCode = 27;
	goto _err_exit;

_err_no_work:
	errorCode = 26;
	goto _err_exit;

_err_unsupported_marker:
	errorCode = 11;
	goto _err_exit;

_err_bad_resource:
	errorCode = 1;
	goto _err_exit;

_err_no_mem:
	errorCode = 6;
	goto _err_exit;

_err_bad_syntax:
	errorCode = 3;
	goto _err_exit;

_err_bad_status:
	errorCode = status;
	goto _err_exit;

_err_lc_not_enabled:
	errorCode = 28;
	goto _err_exit;

_err_not_initialized:
	errorCode = 29;
	goto _err_exit;

_err_exit:
	return errorCode;
}

static void __THPSetupBuffers(void)
{
	u8 i;
	THPCoeff* buffer;

	buffer = (THPCoeff*)OSRoundUp32B(__THPWorkArea);

	for (i = 0; i < 6; i++) {
		__THPMCUBuffer[i] = &buffer[i * 64];
	}
}

static u8 __THPReadFrameHeader(void)
{
	u8 i, utmp8;

	__THPInfo->file += 2;

	utmp8 = (*(__THPInfo->file)++);

	if (utmp8 != 8) {
		return 10;
	}

	__THPInfo->yPixelSize = (u16)((__THPInfo->file)[0] << 8 | (__THPInfo->file)[1]);
	__THPInfo->file += 2;
	__THPInfo->xPixelSize = (u16)((__THPInfo->file)[0] << 8 | (__THPInfo->file)[1]);
	__THPInfo->file += 2;

	utmp8 = (*(__THPInfo->file)++);
	if (utmp8 != 3) {
		return 12;
	}

	for (i = 0; i < 3; i++) {
		utmp8 = (*(__THPInfo->file)++);
		utmp8 = (*(__THPInfo->file)++);
		if ((i == 0 && utmp8 != 0x22) || (i > 0 && utmp8 != 0x11)) {
			return 19;
		}

		__THPInfo->components[i].quantizationTableSelector = (*(__THPInfo->file)++);
	}

	return 0;
}

static u8 __THPReadScaneHeader(void)
{
	u8 i, utmp8;
	__THPInfo->file += 2;

	utmp8 = (*(__THPInfo->file)++);

	if (utmp8 != 3) {
		return 12;
	}

	for (i = 0; i < 3; i++) {
		utmp8 = (*(__THPInfo->file)++);

		utmp8                                    = (*(__THPInfo->file)++);
		__THPInfo->components[i].DCTableSelector = (u8)(utmp8 >> 4);
		__THPInfo->components[i].ACTableSelector = (u8)(utmp8 & 15);

		if ((__THPInfo->validHuffmanTabs & (1 << ((utmp8 >> 4)))) == 0) {
			return 15;
		}

		if ((__THPInfo->validHuffmanTabs & (1 << ((utmp8 & 15) + 1))) == 0) {
			return 15;
		}
	}

	__THPInfo->file += 3;
	__THPInfo->MCUsPerRow           = (u16)THPROUNDUP(__THPInfo->xPixelSize, 16);
	__THPInfo->components[0].predDC = 0;
	__THPInfo->components[1].predDC = 0;
	__THPInfo->components[2].predDC = 0;
	return 0;
}

static u8 __THPReadQuantizationTable(void)
{
	u16 length, id, i, row, col;
	f32 q_temp[64];

	length = (u16)((__THPInfo->file)[0] << 8 | (__THPInfo->file)[1]);
	__THPInfo->file += 2;
	length -= 2;

	for (;;) {
		id = (*(__THPInfo->file)++);

		for (i = 0; i < 64; i++) {
			q_temp[__THPJpegNaturalOrder[i]] = (f32)(*(__THPInfo->file)++);
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

	return 0;
}

static u8 __THPReadHuffmanTableSpecification(void)
{
	u8 t_class, id, i, tab_index;
	u16 length, num_Vij;

	__THPHuffmanSizeTab = __THPWorkArea;
	__THPHuffmanCodeTab = (u16*)((u32)__THPWorkArea + 256 + 1);
	length              = (u16)((__THPInfo->file)[0] << 8 | (__THPInfo->file)[1]);
	__THPInfo->file += 2;
	length -= 2;

	for (;;) {
		i                = (*(__THPInfo->file)++);
		id               = (u8)(i & 15);
		t_class          = (u8)(i >> 4);
		__THPHuffmanBits = __THPInfo->file;
		tab_index        = (u8)((id << 1) + t_class);
		num_Vij          = 0;

		for (i = 0; i < 16; i++) {
			num_Vij += (*(__THPInfo->file)++);
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

	return 0;
}

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

static void __THPRestartDefinition(void)
{
	__THPInfo->RST = TRUE;
	__THPInfo->file += 2;
	__THPInfo->nMCU = (u16)((__THPInfo->file)[0] << 8 | (__THPInfo->file)[1]);
	__THPInfo->file += 2;
	__THPInfo->currMCU = __THPInfo->nMCU;
}

void __THPPrepBitStream(void)
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
					} else {
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

static void __THPDecompressYUV(void* tileY, void* tileU, void* tileV)
{
	u16 currentY, targetY;
	__THPInfo->dLC[0] = tileY;
	__THPInfo->dLC[1] = tileU;
	__THPInfo->dLC[2] = tileV;

	currentY = __THPInfo->decompressedY;
	targetY  = __THPInfo->yPixelSize;

	__THPPrepBitStream();

	if (__THPInfo->xPixelSize == 512 && targetY == 448) {
		while (currentY < targetY) {
			__THPDecompressiMCURow512x448();
			currentY += 16;
		}
	} else if (__THPInfo->xPixelSize == 640 && targetY == 480) {
		while (currentY < targetY) {
			__THPDecompressiMCURow640x480();
			currentY += 16;
		}
	} else {
		while (currentY < targetY) {
			__THPDecompressiMCURowNxN();
			currentY += 16;
		}
	}
}

// NONMATCHING: the row loops are ordinary C in the references, but retail keeps the six
// inverse-DCT calls out of line. With the scalar C inverse transform below, MWCC inlines
// those wrappers and roughly doubles each row function, so their 0% rows are a consequence
// of the paired-single inverse-DCT dead end documented below, not a separate source gap.
static void __THPDecompressiMCURow512x448(void)
{
	u8 cl_num;
	u32 x_pos;
	THPComponent* comp;

	LCQueueWait(3);

	for (cl_num = 0; cl_num < __THPInfo->MCUsPerRow; cl_num++) {
		__THPHuffDecodeDCTCompY(__THPInfo, __THPMCUBuffer[0]);
		__THPHuffDecodeDCTCompY(__THPInfo, __THPMCUBuffer[1]);
		__THPHuffDecodeDCTCompY(__THPInfo, __THPMCUBuffer[2]);
		__THPHuffDecodeDCTCompY(__THPInfo, __THPMCUBuffer[3]);
		__THPHuffDecodeDCTCompU(__THPInfo, __THPMCUBuffer[4]);
		__THPHuffDecodeDCTCompV(__THPInfo, __THPMCUBuffer[5]);

		comp  = &__THPInfo->components[0];
		Gbase = __THPLCWork512[0];
		Gwid  = 512;
		Gq    = __THPInfo->quantTabs[comp->quantizationTableSelector];
		x_pos = (u32)(cl_num * 16);
		__THPInverseDCTNoYPos(__THPMCUBuffer[0], x_pos);
		__THPInverseDCTNoYPos(__THPMCUBuffer[1], x_pos + 8);
		__THPInverseDCTY8(__THPMCUBuffer[2], x_pos);
		__THPInverseDCTY8(__THPMCUBuffer[3], x_pos + 8);

		comp  = &__THPInfo->components[1];
		Gbase = __THPLCWork512[1];
		Gwid  = 256;
		Gq    = __THPInfo->quantTabs[comp->quantizationTableSelector];
		x_pos /= 2;
		__THPInverseDCTNoYPos(__THPMCUBuffer[4], x_pos);
		comp  = &__THPInfo->components[2];
		Gbase = __THPLCWork512[2];
		Gq    = __THPInfo->quantTabs[comp->quantizationTableSelector];
		__THPInverseDCTNoYPos(__THPMCUBuffer[5], x_pos);

		if (__THPInfo->RST != 0) {
			if ((--__THPInfo->currMCU) == 0) {
				__THPInfo->currMCU = __THPInfo->nMCU;
				__THPInfo->cnt     = 1 + ((__THPInfo->cnt + 6) & 0xFFFFFFF8);

				if (__THPInfo->cnt > 33) {
					__THPInfo->cnt = 33;
				}

				__THPInfo->components[0].predDC = 0;
				__THPInfo->components[1].predDC = 0;
				__THPInfo->components[2].predDC = 0;
			}
		}
	}

	LCStoreData(__THPInfo->dLC[0], __THPLCWork512[0], 0x2000);
	LCStoreData(__THPInfo->dLC[1], __THPLCWork512[1], 0x800);
	LCStoreData(__THPInfo->dLC[2], __THPLCWork512[2], 0x800);

	__THPInfo->dLC[0] += 0x2000;
	__THPInfo->dLC[1] += 0x800;
	__THPInfo->dLC[2] += 0x800;
}

/* Twice the relevant cosines and the combinations used by the AAN inverse DCT. */
static const f32 THP_SQRT2             = 1.414213562f;
static const f32 THP_COS_2             = 1.847759065f;
static const f32 THP_COS_2_PLUS_COS_6  = 2.613125930f;
static const f32 THP_COS_2_MINUS_COS_6 = 1.082392200f;

static void __THPIDCTRow(const THPCoeff* coeff, const f32* quant, f32* out)
{
	f32 dc  = coeff[0] * quant[0];
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

			odd1   = __fmadds(ac1, THP_COS_2, -ac1);
			odd2   = __fmadds(ac1, THP_SQRT2, -odd1);
			odd3   = __fnmsubs(ac1, THP_COS_2 - THP_COS_2_MINUS_COS_6, odd2);
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

		ac2   = coeff[2] * quant[2];
		ac3   = coeff[3] * quant[3];
		even0 = dc + ac2;
		even3 = dc - ac2;
		even1 = __fmadds(ac2, THP_SQRT2, even3);
		even2 = __fnmsubs(ac2, THP_SQRT2, even0);

		odd0     = ac1 + ac3;
		rotation = (ac1 - ac3) * THP_COS_2;
		odd1     = __fnmsubs(ac3, -THP_COS_2_PLUS_COS_6, rotation) - odd0;
		odd2     = __fmadds(ac1 - ac3, THP_SQRT2, -odd1);
		odd3     = __fnmsubs(ac1, THP_COS_2_MINUS_COS_6, rotation) - odd2;
		out[0]   = even0 + odd0;
		out[1]   = even1 + odd1;
		out[2]   = even2 + odd2;
		out[3]   = even3 + odd3;
		out[4]   = even3 - odd3;
		out[5]   = even2 - odd2;
		out[6]   = even1 - odd1;
		out[7]   = even0 - odd0;
		return;
	}

	ac2       = coeff[2] * quant[2];
	ac3       = coeff[3] * quant[3];
	evenSum0  = __fmadds(coeff[4], quant[4], dc);
	evenDiff0 = __fnmsubs(coeff[4], quant[4], dc);
	evenSum1  = __fmadds(coeff[6], quant[6], ac2);
	evenDiff1 = __fnmsubs(coeff[6], quant[6], ac2);
	even0     = evenSum0 + evenSum1;
	even3     = evenSum0 - evenSum1;
	rotation  = __fmadds(evenDiff1, THP_SQRT2, -evenSum1);
	even1     = evenDiff0 + rotation;
	even2     = evenDiff0 - rotation;

	oddSum0  = __fmadds(coeff[5], quant[5], ac3);
	oddDiff0 = __fnmsubs(coeff[5], quant[5], ac3);
	oddSum1  = __fmadds(coeff[7], quant[7], ac1);
	oddDiff1 = __fnmsubs(coeff[7], quant[7], ac1);
	odd0     = oddSum0 + oddSum1;
	rotation = (oddDiff1 - oddDiff0) * THP_COS_2;
	odd1     = __fnmsubs(oddDiff0, -THP_COS_2_PLUS_COS_6, rotation) - odd0;
	odd2     = __fmadds(oddSum0 - oddSum1, THP_SQRT2, odd1);
	odd3     = __fmadds(oddDiff1, THP_COS_2_MINUS_COS_6, -rotation) - odd2;

	out[0] = even0 + odd0;
	out[1] = even1 + odd1;
	out[2] = even2 - odd2;
	out[3] = even3 - odd3;
	out[4] = even3 + odd3;
	out[5] = even2 + odd2;
	out[6] = even1 - odd1;
	out[7] = even0 - odd0;
}

static inline THPSample __THPIDCTSample(f32 value)
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

static inline void __THPIDCTColumn(const f32* in, THPSample* top, THPSample* bottom)
{
	// The unnormalized transform yields eight times the pixel value.
	f32 evenSum0     = (in[0 * 8] + in[4 * 8]) + 1024.0f;
	f32 evenDiff0    = (in[0 * 8] - in[4 * 8]) + 1024.0f;
	f32 evenSum1     = in[2 * 8] + in[6 * 8];
	f32 evenDiff1    = in[2 * 8] - in[6 * 8];
	f32 evenRotation = __fmadds(evenDiff1, THP_SQRT2, -evenSum1);
	f32 oddSum0      = in[5 * 8] + in[3 * 8];
	f32 oddDiff0     = in[5 * 8] - in[3 * 8];
	f32 oddSum1      = in[1 * 8] + in[7 * 8];
	f32 oddDiff1     = in[1 * 8] - in[7 * 8];
	f32 oddRotation  = (oddDiff0 + oddDiff1) * THP_COS_2;
	f32 even0        = evenSum0 + evenSum1;
	f32 even1        = evenDiff0 + evenRotation;
	f32 even2        = evenDiff0 - evenRotation;
	f32 even3        = evenSum0 - evenSum1;
	f32 odd0         = oddSum1 + oddSum0;
	f32 odd1         = __fmadds(oddDiff0, -THP_COS_2_PLUS_COS_6, oddRotation) - odd0;
	f32 odd2         = __fmadds(oddSum1 - oddSum0, THP_SQRT2, -odd1);
	f32 odd3         = __fmadds(oddDiff1, THP_COS_2_MINUS_COS_6, -oddRotation) + odd2;

	top[0]     = __THPIDCTSample(even0 + odd0);
	top[8]     = __THPIDCTSample(even1 + odd1);
	top[16]    = __THPIDCTSample(even2 + odd2);
	top[24]    = __THPIDCTSample(even3 - odd3);
	bottom[0]  = __THPIDCTSample(even3 + odd3);
	bottom[8]  = __THPIDCTSample(even2 - odd2);
	bottom[16] = __THPIDCTSample(even1 - odd1);
	bottom[24] = __THPIDCTSample(even0 - odd0);
}

// Each 8x8 block is stored in two stacked groups of four scanlines: the
// top half at tile and the bottom half four full rows of the plane later.
void __THPInverseDCTNoYPos(THPCoeff* in, u32 xPos)
{
	f32* ws           = __THPIDCTWorkspace;
	const f32* quant  = Gq;
	THPSample* tile   = Gbase + xPos * 4;
	THPSample* bottom = tile + 4 * Gwid;
	int i;

	for (i = 0; i < 8; i++) {
		__THPIDCTRow(in + i * 8, quant + i * 8, ws + i * 8);
	}
	for (i = 0; i < 8; i++) {
		__THPIDCTColumn(ws + i, tile + i, bottom + i);
	}
}

void __THPInverseDCTY8(THPCoeff* in, u32 xPos)
{
	f32* ws           = __THPIDCTWorkspace;
	const f32* quant  = Gq;
	THPSample* tile   = Gbase + Gwid * 8 + xPos * 4;
	THPSample* bottom = tile + 4 * Gwid;
	int i;

	for (i = 0; i < 8; i++) {
		__THPIDCTRow(in + i * 8, quant + i * 8, ws + i * 8);
	}
	for (i = 0; i < 8; i++) {
		__THPIDCTColumn(ws + i, tile + i, bottom + i);
	}
}

static inline u32 __THPNextWord(THPFileInfo* info)
{
	u32* file  = (u32*)info->file;
	u32 word   = *++file;
	info->file = (u8*)file;
	return word;
}

static inline s32 __THPHuffDecodeFast(THPFileInfo* info, THPHuffmanTab* h, s32* pCnt, u32* pCurrByte, BOOL syncInfo)
{
	s32 cnt          = *pCnt;
	u32 currByte     = *pCurrByte;
	const u8* quick  = h->quick;
	const u8* incTab = h->increment;
	s32 code;
	s32 len;
	s32 val;
	s32 inc;

	if (cnt <= 28) {
		code = (s32)((currByte << (cnt - 1)) >> 27);
		val  = quick[code];
		inc  = incTab[code];
		if (val != 0xFF) {
			cnt += inc;
			*pCnt = cnt;
			if (syncInfo) {
				info->cnt = (u32)cnt;
			}
			return val;
		}
		{
			const s32* maxCode = &h->maxCode[5];
			len                = 5;
			cnt += 5;
			for (;;) {
				code <<= 1;
				if (cnt != 33) {
					s32 mc = *++maxCode;
					code |= (s32)((currByte >> (32 - cnt)) & 1);
					cnt++;
					len++;
					if (code <= mc) {
						break;
					}
				} else {
					s32 mc;
					currByte = __THPNextWord(info);
					if (syncInfo) {
						info->currByte = currByte;
					}
					mc = *++maxCode;
					code |= (s32)(currByte >> 31);
					cnt = 1;
					while (code > mc) {
						code = (code << 1) | (s32)((currByte >> (31 - cnt)) & 1);
						mc   = *++maxCode;
						cnt++;
						len++;
					}
					cnt++;
					len++;
					break;
				}
			}
		}
		*pCnt      = cnt;
		*pCurrByte = currByte;
		if (syncInfo) {
			info->cnt = (u32)cnt;
		}
		return (s32)h->Vij[code + h->valPtr[len]];
	}

	if (cnt != 33) {
		code = (s32)((currByte << (cnt - 1)) >> 27);
		if (cnt != 32) {
			val = quick[code];
			inc = incTab[code];
			if (val != 0xFF) {
				s32 nextCnt = cnt + inc;
				if (nextCnt <= 33) {
					*pCnt = nextCnt;
					if (syncInfo) {
						info->cnt = (u32)nextCnt;
					}
					return val;
				}
			}
			goto cross_word;
		}
		{
			u32 topBit = currByte & 1;
			currByte   = __THPNextWord(info);
			code       = (s32)((topBit << 4) | (currByte >> 28));
			val        = quick[code];
			inc        = incTab[code];
			*pCurrByte = currByte;
			if (syncInfo) {
				info->currByte = currByte;
			}
			if (val != 0xFF) {
				*pCnt = inc;
				if (syncInfo) {
					info->cnt = (u32)inc;
				}
				return val;
			}
			{
				u32 shifted        = (topBit << 31) | (currByte >> 1);
				const s32* maxCode = &h->maxCode[5];
				len                = 5;
				do {
					len++;
					code = (s32)(shifted >> (32 - len));
				} while (code > *++maxCode);
				*pCnt = len;
				if (syncInfo) {
					info->cnt = (u32)len;
				}
			}
			return (s32)h->Vij[code + h->valPtr[len]];
		}
	}

	currByte   = __THPNextWord(info);
	code       = (s32)(currByte >> 27);
	val        = quick[code];
	inc        = incTab[code];
	*pCurrByte = currByte;
	if (syncInfo) {
		info->currByte = currByte;
	}
	if (val != 0xFF) {
		cnt   = inc + 1;
		*pCnt = cnt;
		if (syncInfo) {
			info->cnt = (u32)cnt;
		}
		return val;
	}
	len = 5;
	do {
		len++;
		code = (s32)(currByte >> (32 - len));
	} while (code > h->maxCode[len]);
	cnt   = len + 1;
	*pCnt = cnt;
	if (syncInfo) {
		info->cnt = (u32)cnt;
	}
	return (s32)h->Vij[code + h->valPtr[len]];

cross_word: {
	s32 rem            = 33 - cnt;
	u32 mask           = (u32)(-1) << rem;
	const s32* maxCode = &h->maxCode[rem];
	s32 mc;
	code     = (s32)(currByte & ~mask);
	currByte = __THPNextWord(info);
	if (syncInfo) {
		info->currByte = currByte;
	}
	len  = rem + 1;
	code = (code << 1) | (s32)(currByte >> 31);
	cnt  = 2;
	mc   = *++maxCode;
	while (code > mc) {
		code = (code << 1) | (s32)((currByte >> (32 - cnt)) & 1);
		mc   = *++maxCode;
		cnt++;
		len++;
	}
	*pCnt      = cnt;
	*pCurrByte = currByte;
	if (syncInfo) {
		info->cnt = (u32)cnt;
	}
	return (s32)h->Vij[code + h->valPtr[len]];
}
}

static inline s32 __THPReadCoeffFast(THPFileInfo* info, s32 bits, s32* pCnt, u32* pCurrByte, BOOL isDC)
{
	s32 cnt      = isDC ? (s32)info->cnt : *pCnt;
	u32 currByte = isDC ? info->currByte : *pCurrByte;
	s32 rem      = 33 - cnt;
	s32 extra    = bits - rem;
	u32 val;

	if (extra <= 0) {
		val = (currByte << (cnt - 1)) >> (32 - bits);
		cnt += bits;
		if (isDC) {
			info->cnt = (u32)cnt;
		} else {
			*pCnt = cnt;
		}
	} else {
		u32 hi   = currByte << (cnt - 1);
		currByte = __THPNextWord(info);
		if (isDC) {
			info->currByte = currByte;
		} else {
			*pCurrByte = currByte;
		}
		val = (hi + (currByte >> rem)) >> (32 - bits);
		cnt = extra + 1;
		if (isDC) {
			info->cnt = (u32)cnt;
		} else {
			*pCnt = cnt;
		}
	}
	if (isDC) {
		s16 sval = (s16)val;
		if ((s32)__cntlzw((u32)(s32)sval) > 32 - bits) {
			sval = (s16)(((u32)(-1) << bits) + val + 1);
		}
		return sval;
	}
	if ((s32)__cntlzw(val) > 32 - bits) {
		val = ((u32)(-1) << bits) + val + 1;
	}
	return (s32)val;
}

static inline void __THPHuffDecodeDCTComp(THPFileInfo* info, THPCoeff* block, THPHuffmanTab** dcTabPtr, THPHuffmanTab** acTabPtr,
                                          s32 compIdx)
{
	s32 cnt;
	u32 currByte;
	s32 t;
	s16 diff;
	s32 k;
	THPHuffmanTab* acTab;

	__dcbz((void*)block, 0);
	cnt      = (s32)info->cnt;
	currByte = info->currByte;
	t        = __THPHuffDecodeFast(info, *dcTabPtr, &cnt, &currByte, TRUE);

	__dcbz((void*)block, 32);
	__dcbz((void*)block, 64);

	diff = 0;
	if (t) {
		diff = (s16)__THPReadCoeffFast(info, t, &cnt, &currByte, TRUE);
	}

	__dcbz((void*)block, 96);

	block[0] = info->components[compIdx].predDC = (THPCoeff)(info->components[compIdx].predDC + diff);

	acTab    = *acTabPtr;
	cnt      = (s32)info->cnt;
	currByte = info->currByte;
	for (k = 1; k < 64; k++) {
		s32 r = __THPHuffDecodeFast(info, acTab, &cnt, &currByte, FALSE);
		s32 s = r & 15;
		r >>= 4;

		if (s) {
			k += r;
			block[__THPJpegNaturalOrder[k]] = (THPCoeff)__THPReadCoeffFast(info, s, &cnt, &currByte, FALSE);
		} else {
			if (r != 15) {
				break;
			}
			k += 15;
		}
	}

	info->cnt      = (u32)cnt;
	info->currByte = currByte;
}

static void __THPDecompressiMCURow640x480(void)
{
	u8 cl_num;
	u32 x_pos;
	THPComponent* comp;

	LCQueueWait(3);

	for (cl_num = 0; cl_num < __THPInfo->MCUsPerRow; cl_num++) {
		__THPHuffDecodeDCTCompY(__THPInfo, __THPMCUBuffer[0]);
		__THPHuffDecodeDCTCompY(__THPInfo, __THPMCUBuffer[1]);
		__THPHuffDecodeDCTCompY(__THPInfo, __THPMCUBuffer[2]);
		__THPHuffDecodeDCTCompY(__THPInfo, __THPMCUBuffer[3]);
		__THPHuffDecodeDCTCompU(__THPInfo, __THPMCUBuffer[4]);
		__THPHuffDecodeDCTCompV(__THPInfo, __THPMCUBuffer[5]);

		comp  = &__THPInfo->components[0];
		Gbase = __THPLCWork672[0];
		Gwid  = 640;
		Gq    = __THPInfo->quantTabs[comp->quantizationTableSelector];
		x_pos = (u32)(cl_num * 16);
		__THPInverseDCTNoYPos(__THPMCUBuffer[0], x_pos);
		__THPInverseDCTNoYPos(__THPMCUBuffer[1], x_pos + 8);
		__THPInverseDCTY8(__THPMCUBuffer[2], x_pos);
		__THPInverseDCTY8(__THPMCUBuffer[3], x_pos + 8);

		comp  = &__THPInfo->components[1];
		Gbase = __THPLCWork672[1];
		Gwid  = 320;
		Gq    = __THPInfo->quantTabs[comp->quantizationTableSelector];
		x_pos /= 2;
		__THPInverseDCTNoYPos(__THPMCUBuffer[4], x_pos);

		comp  = &__THPInfo->components[2];
		Gbase = __THPLCWork672[2];
		Gq    = __THPInfo->quantTabs[comp->quantizationTableSelector];
		__THPInverseDCTNoYPos(__THPMCUBuffer[5], x_pos);

		if (__THPInfo->RST != 0) {
			__THPInfo->currMCU--;
			if (__THPInfo->currMCU == 0) {
				__THPInfo->currMCU = __THPInfo->nMCU;

				__THPInfo->cnt = 1 + ((__THPInfo->cnt + 6) & 0xFFFFFFF8);

				if (__THPInfo->cnt > 32) {
					__THPInfo->cnt = 33;
				}

				__THPInfo->components[0].predDC = 0;
				__THPInfo->components[1].predDC = 0;
				__THPInfo->components[2].predDC = 0;
			}
		}
	}

	LCStoreData(__THPInfo->dLC[0], __THPLCWork672[0], 0x2800);
	LCStoreData(__THPInfo->dLC[1], __THPLCWork672[1], 0xA00);
	LCStoreData(__THPInfo->dLC[2], __THPLCWork672[2], 0xA00);

	__THPInfo->dLC[0] += 0x2800;
	__THPInfo->dLC[1] += 0xA00;
	__THPInfo->dLC[2] += 0xA00;
}

static void __THPDecompressiMCURowNxN(void)
{
	u8 cl_num;
	u32 x_pos, x;
	THPComponent* comp;

	x = __THPInfo->xPixelSize;

	LCQueueWait(3);

	for (cl_num = 0; cl_num < __THPInfo->MCUsPerRow; cl_num++) {
		__THPHuffDecodeDCTCompY(__THPInfo, __THPMCUBuffer[0]);
		__THPHuffDecodeDCTCompY(__THPInfo, __THPMCUBuffer[1]);
		__THPHuffDecodeDCTCompY(__THPInfo, __THPMCUBuffer[2]);
		__THPHuffDecodeDCTCompY(__THPInfo, __THPMCUBuffer[3]);
		__THPHuffDecodeDCTCompU(__THPInfo, __THPMCUBuffer[4]);
		__THPHuffDecodeDCTCompV(__THPInfo, __THPMCUBuffer[5]);

		comp  = &__THPInfo->components[0];
		Gbase = __THPLCWork672[0];
		Gwid  = x;
		Gq    = __THPInfo->quantTabs[comp->quantizationTableSelector];
		x_pos = (u32)(cl_num * 16);
		__THPInverseDCTNoYPos(__THPMCUBuffer[0], x_pos);
		__THPInverseDCTNoYPos(__THPMCUBuffer[1], x_pos + 8);
		__THPInverseDCTY8(__THPMCUBuffer[2], x_pos);
		__THPInverseDCTY8(__THPMCUBuffer[3], x_pos + 8);

		comp  = &__THPInfo->components[1];
		Gbase = __THPLCWork672[1];
		Gwid  = x / 2;
		Gq    = __THPInfo->quantTabs[comp->quantizationTableSelector];
		x_pos /= 2;
		__THPInverseDCTNoYPos(__THPMCUBuffer[4], x_pos);

		comp  = &__THPInfo->components[2];
		Gbase = __THPLCWork672[2];
		Gq    = __THPInfo->quantTabs[comp->quantizationTableSelector];
		__THPInverseDCTNoYPos(__THPMCUBuffer[5], x_pos);

		if (__THPInfo->RST != 0) {
			__THPInfo->currMCU--;
			if (__THPInfo->currMCU == 0) {
				__THPInfo->currMCU = __THPInfo->nMCU;
				__THPInfo->cnt     = 1 + ((__THPInfo->cnt + 6) & 0xFFFFFFF8);

				if (__THPInfo->cnt > 32) {
					__THPInfo->cnt = 33;
				}

				__THPInfo->components[0].predDC = 0;
				__THPInfo->components[1].predDC = 0;
				__THPInfo->components[2].predDC = 0;
			}
		}
	}

	LCStoreData(__THPInfo->dLC[0], __THPLCWork672[0], ((4 * sizeof(u8) * 64) * (x / 16)));
	LCStoreData(__THPInfo->dLC[1], __THPLCWork672[1], ((sizeof(u8) * 64) * (x / 16)));
	LCStoreData(__THPInfo->dLC[2], __THPLCWork672[2], ((sizeof(u8) * 64) * (x / 16)));
	__THPInfo->dLC[0] += ((4 * sizeof(u8) * 64) * (x / 16));
	__THPInfo->dLC[1] += ((sizeof(u8) * 64) * (x / 16));
	__THPInfo->dLC[2] += ((sizeof(u8) * 64) * (x / 16));
}

static void __THPHuffDecodeDCTCompY(THPFileInfo* info, THPCoeff* block)
{
	__THPHuffDecodeDCTComp(info, block, &Ydchuff, &Yachuff, 0);
}

static void __THPHuffDecodeDCTCompU(THPFileInfo* info, THPCoeff* block)
{
	__THPHuffDecodeDCTComp(info, block, &Udchuff, &Uachuff, 1);
}

static void __THPHuffDecodeDCTCompV(THPFileInfo* info, THPCoeff* block)
{
	__THPHuffDecodeDCTComp(info, block, &Vdchuff, &Vachuff, 2);
}

BOOL THPInit(void)
{
	u8* base;
	OSRegisterVersion(__THPVersion);
	base = (u8*)(0xE000 << 16);

	__THPLCWork512[0] = base;
	base += 0x2000;
	__THPLCWork512[1] = base;
	base += 0x800;
	__THPLCWork512[2] = base;
	base += 0x200;

	base              = (u8*)(0xE000 << 16);
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
