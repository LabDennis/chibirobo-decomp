#include "types.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern "C" {
extern const u8* sHalfWidthCharacters;
extern const char* sFullWidthCharacters;
extern char sFullWidthTextBuffer[];
extern char sFormattedTextBuffer[];
}

/**
 * @note Address: 0x80191310
 * @note Size: 0xC8
 */
// reads len hex digits, or the whole string when len is 0
extern "C" u32 ParseHexDigits(const char* str, s32 len)
{
	u32 val = 0;
	if (len == 0) {
		len = strlen(str);
	}
	for (int i = 0; i < len; i++) {
		if (*str >= '0' && *str <= '9') {
			val = val * 16 + (*str - '0');
		} else if (*str >= 'A' && *str <= 'F') {
			val = val * 16 + (*str - 'A' + 10);
		} else if (*str >= 'a' && *str <= 'f') {
			val = val * 16 + (*str - 'a' + 10);
		}
		str++;
	}
	return val;
}

/**
 * @note Address: 0x80191268
 * @note Size: 0xA8
 */
extern "C" char* ConvertHalfWidthToFullWidthSJIS(const char* src, char* dst)
{
	u8* out      = (u8*)(dst != NULL ? dst : sFullWidthTextBuffer);
	const u8* in = (const u8*)src;
	while (*in != 0) {
		*out = *in;
		for (int i = 0; (s32)sHalfWidthCharacters[i] != 0; i++) {
			if (*in == sHalfWidthCharacters[i]) {
				*out   = sFullWidthCharacters[i * 2];
				*++out = sFullWidthCharacters[i * 2 + 1];
				break;
			}
		}
		in++;
		*++out = 0;
	}
	return dst != NULL ? dst : sFullWidthTextBuffer;
}

/**
 * @note Address: 0x801911B4
 * @note Size: 0xB4
 */
extern "C" char* FormatToStaticBuffer(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsprintf(sFormattedTextBuffer, fmt, args);
	return sFormattedTextBuffer;
}
