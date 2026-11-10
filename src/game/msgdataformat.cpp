#include "game/cgame.h"
#include "game/cfont.h"
#include "game/msg.h"
#include <stdio.h>
#include <string.h>

struct MessageFormatBuffers {
	char format[128];
	char text[128];
};

#if !defined(VERSION_JP)
static inline void GroupDigits(MessageFormatBuffers& buffers)
{
	u8 separator = ',';
#if defined(VERSION_PAL)
	switch (gGame.mLanguage) {
	case 1:
	case 2:
	case 3:
		separator = ' ';
		break;
	case 4:
		separator = '.';
		break;
	case 0:
	default:
		separator = ',';
		break;
	}
#endif
	s32 len   = strlen(buffers.text);
	s32 count = 0;
#if defined(VERSION_PAL)
	u8 lang = gGame.mLanguage;
#endif
	for (s32 i = 0; i < len; ++i, ++count) {
		buffers.format[count] = buffers.text[(len - 1) - i];
		if ((i + 1) % 3 == 0 && i + 1 < len) {
#if defined(VERSION_PAL)
			if ((lang == 1 || lang == 3 || lang == 4) && len < 5)
				continue;
#endif
			buffers.format[++count] = separator;
		}
		buffers.format[count + 1] = 0;
	}
	// When the formatted value is empty, retail reverses the format string left in the scratch buffer.
	count = strlen(buffers.format);
	for (s32 i = 0; i < count; ++i)
		buffers.text[i] = buffers.format[(count - 1) - i];
	buffers.text[count] = 0;
}
#endif

template <int N>
static inline void CopyMessageBytes(u8*& dst, u8*& src)
{
	for (int i = 0; i < N; ++i)
		*dst++ = *src++;
}

/**
 * @note Address: 0x800869AC
 * @note Size: 0x578
 */
extern "C" void CMsgData_Format(u8* output, u8* input, MsgFormatArgs* args)
{
	u8* src = input;
	u8* dst = output;
	MessageFormatBuffers buffers;
	BOOL done = FALSE;
	while (!done) {
		switch (*src) {
		case 0:
			*dst = 0;
			done = TRUE;
			break;
		case 12: {

			char* out = buffers.format;
			bool wide = false;
#if !defined(VERSION_JP)
			bool grouped = false;
#endif
			++src;
			for (;;) {
				if (*src == 12)
					break;
				if (*src == 'z') {
					wide = true;
					++src;
					continue;
				}
#if !defined(VERSION_JP)
				if (*src == '\'') {
					grouped = true;
					++src;
					continue;
				}
#endif
				*out++ = *src++;
			}
			*out = 0;
			++src;

#if defined(VERSION_JP)
			if (wide) {
				sprintf(buffers.text, buffers.format, args->values[--args->count]);
				strcpy((char*)dst, ConvertHalfWidthToFullWidthSJIS(buffers.text, NULL));
			} else {
				sprintf((char*)dst, buffers.format, args->values[--args->count]);
			}
#else
			sprintf(buffers.text, buffers.format, args->values[--args->count]);
			if (grouped) {
				GroupDigits(buffers);
				strcpy((char*)dst, buffers.text);
			}
			if (wide && gGame.mLanguage == 6)
				strcpy((char*)dst, ConvertHalfWidthToFullWidthSJIS(buffers.text, NULL));
			else
				strcpy((char*)dst, buffers.text);
#endif
			dst += strlen((char*)dst);
			break;
		}
		case 10:
		case 11:
		case 24:
			CopyMessageBytes<1>(dst, src);
			break;
		case 1:
		case 2:
		case 5:
		case 13:
		case 14:
		case 16:
		case 17:
		case 19:
			CopyMessageBytes<2>(dst, src);
			break;
		case 20:
		case 23:
			CopyMessageBytes<3>(dst, src);
			break;
		case 21:
			CopyMessageBytes<5>(dst, src);
			break;
		case 18:
		case 22:
			CopyMessageBytes<4>(dst, src);
			break;
		case 6:
			CopyMessageBytes<6>(dst, src);
			break;
		case 3:
			CopyMessageBytes<8>(dst, src);
			break;
		case 4:
			CopyMessageBytes<6>(dst, src);
			if (dst[-1] == 2 || dst[-1] == 3)
				CopyMessageBytes<2>(dst, src);
			else if (dst[-1] == 4)
				CopyMessageBytes<3>(dst, src);
			break;
		case 15:
			CopyMessageBytes<5>(dst, src);
			break;
		default:
#if defined(VERSION_JP)
			if (IsSjisLead(*src)) {
#else
			if (gGame.mLanguage == 6 && IsSjisLead(*src)) {
#endif
				CopyMessageBytes<2>(dst, src);
			} else if (*src == '$') {
				++src;
#if defined(VERSION_JP)
				*dst++ = '/';
#else
				*dst++ = '"';
#endif
			} else {
				*dst++ = *src++;
			}
		}
	}
}
