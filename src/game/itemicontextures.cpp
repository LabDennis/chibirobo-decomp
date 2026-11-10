#include "types.h"

#if defined(VERSION_JP)
enum { DefaultItemIcon = 124, Item32Icon = 242 };
#else
enum { DefaultItemIcon = 126, Item32Icon = 244 };
#endif

// clang-format off
s32 gItemIconTextureIndices[] = {
	/* 0x00 */ 0, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon,
	/* 0x08 */ 1, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, 2, DefaultItemIcon,
	/* 0x10 */ 3, DefaultItemIcon, 4, 5, DefaultItemIcon, 6, 7, 8,
	/* 0x18 */ DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon,
	/* 0x20 */ Item32Icon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon,
	/* 0x28 */ DefaultItemIcon, 9, 10, 11, 12, 13, DefaultItemIcon, DefaultItemIcon,
	/* 0x30 */ 14, 15, 16, 18, 17, 19, 19, 20,
	/* 0x38 */ 21, 22, 23, 24, 25, 26, 19, DefaultItemIcon,
	/* 0x40 */ DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon,
	/* 0x48 */ DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon,
	/* 0x50 */ DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, 27,
	/* 0x58 */ 28, 27, 27, 29, 30, 31, 32, 33,
	/* 0x60 */ 34, 35, 36, 37, 38, 39, 40, 41,
	/* 0x68 */ DefaultItemIcon, 42, DefaultItemIcon, 43, 44, 45, 46, 47,
	/* 0x70 */ 48, 49, 50, 51, 52, 53, 54, 55,
	/* 0x78 */ 56, 57, 58, 59, 60, 61, 62, 63,
	/* 0x80 */ DefaultItemIcon, 64, DefaultItemIcon, 65, 66, 67, 68, 69,
	/* 0x88 */ 70, 71, 72, 73, 74, DefaultItemIcon, DefaultItemIcon, DefaultItemIcon,
	/* 0x90 */ DefaultItemIcon, DefaultItemIcon, DefaultItemIcon, 75, 76, 77, 78, 79,
	/* 0x98 */ 80, 81, 82, 83, 84, 85, 86,
};
// clang-format on
