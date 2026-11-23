#include "game/cgame.h"
#include "game/cfont.h"
#include "game/msg.h"
#include <stdio.h>
#include <string.h>

struct MessageFormatBuffers {
	char format[128];
	char text[128];
};

static inline void CopyMessageBytes(u8*& dst, u8*& src, int length)
{
	for (int i = 0; i < length; ++i)
		*dst++ = *src++;
}

/**
 * @note Address: 0x800869AC
 * @note Size: 0x578
 */
void CMsgData::Format(u8* script, MsgFormatArgs* args)
{
	u8* src = script;
	u8* dst = mText;
	MessageFormatBuffers buffers;
	BOOL done = FALSE;
	while (!done) {
		switch (*src) {
		case MSG_CODE_END:
			*dst = 0;
			done = TRUE;
			break;
		case MSG_CODE_FORMAT: { // %fmt% inline format; 'z' makes it wide, "'" groups digits
			char* out = buffers.format;
			bool wide = false;
#if !defined(VERSION_JP)
			bool grouped = false;
#endif
			++src;
			for (;;) {
				if (*src == MSG_CODE_FORMAT)
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
				char separator = ',';
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
				s32 len = strlen(buffers.text);
#if defined(VERSION_PAL)
				u8 lang = gGame.mLanguage;
#endif
				s32 i, count;
				for (i = 0, count = 0; i < len; ++i, ++count) {
					buffers.format[count] = buffers.text[(len - 1) - i];
					if ((i + 1) % 3 == 0 && i + 1 < len) {
#if defined(VERSION_PAL)
						if ((lang == 1 || lang == 3 || lang == 4) && len < 5) // four digits stay ungrouped there
							continue;
#endif
						buffers.format[++count] = separator; // every third digit
					}
					buffers.format[count + 1] = 0;
				}
				// an empty value leaves the format string in the scratch buffer
				count = strlen(buffers.format);
				for (i = 0; i < count; ++i)
					buffers.text[i] = buffers.format[(count - 1) - i];
				buffers.text[count] = 0;
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
		case MSG_CODE_NEWLINE:
		case MSG_CODE_NEWLINE_VT:
		case MSG_CODE_STAY:
			CopyMessageBytes(dst, src, 1);
			break;
		case MSG_CODE_SPEED:
		case MSG_CODE_WAIT:
		case MSG_CODE_VOICE:
		case MSG_CODE_SIZE:
		case MSG_CODE_COLOR:
		case MSG_CODE_LAYOUT:
		case MSG_CODE_ICON:
		case MSG_CODE_CENTER:
			CopyMessageBytes(dst, src, 2);
			break;
		case MSG_CODE_ROTATE:
		case MSG_CODE_QUESTION:
			CopyMessageBytes(dst, src, 3);
			break;
		case MSG_CODE_SCALE:
			CopyMessageBytes(dst, src, 5);
			break;
		case MSG_CODE_SHAKE:
		case MSG_CODE_NUM_INPUT:
			CopyMessageBytes(dst, src, 4);
			break;
		case MSG_CODE_DEFAULT:
			CopyMessageBytes(dst, src, 6);
			break;
		case MSG_CODE_ANIM:
			CopyMessageBytes(dst, src, 8);
			break;
		case MSG_CODE_SFX:
			CopyMessageBytes(dst, src, 6);
			if (dst[-1] == MSG_SFX_FADE_OUT || dst[-1] == MSG_SFX_FADE_IN)
				CopyMessageBytes(dst, src, 2);
			else if (dst[-1] == MSG_SFX_FADE)
				CopyMessageBytes(dst, src, 3);
			break;
		case MSG_CODE_RGBA:
			CopyMessageBytes(dst, src, 5);
			break;
		default:
#if defined(VERSION_JP)
			if (IsSjisLead(*src)) {
#else
			if (gGame.mLanguage == 6 && IsSjisLead(*src)) {
#endif
				CopyMessageBytes(dst, src, 2);
			} else if (*src == '$') { // '$' becomes '"' ('/' in Japan)
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
