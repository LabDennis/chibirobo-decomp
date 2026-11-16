#ifndef NW4HBM_TYPES_H
#define NW4HBM_TYPES_H
#include <types.h>

#ifdef __cplusplus
#include <new>
#endif

// the iterator type of a list, for the NW4R_UT_LINKLIST_FOREACH loops
#define DECLTYPE(x) __decltype__(x)

#define ROUND_UP(x, align) (((x) + (align) - 1) & (-(align)))

#define NW4R_BYTEORDER_BIG 0xFEFF
#define NW4R_BYTEORDER_LITTLE 0xFFFE

#if defined(NW4R_LITTLE_ENDIAN)
#define NW4R_BYTEORDER_NATIVE NW4R_BYTEORDER_LITTLE
#else
#define NW4R_BYTEORDER_NATIVE NW4R_BYTEORDER_BIG
#endif

// Multi-character constant of a binary file or block signature
#define FOURCC(c0, c1, c2, c3) (u32)((c0 & 0xFF) << 24 | (c1 & 0xFF) << 16 | (c2 & 0xFF) << 8 | (c3 & 0xFF))

#define NW4R_VERSION(major, minor) ((major & 0xFF) << 8 | minor & 0xFF)

#endif
