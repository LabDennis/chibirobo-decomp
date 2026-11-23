#ifndef _TYPES_H
#define _TYPES_H

#include "BuildSettings.h"

typedef int BOOL;
typedef unsigned int uint;

typedef signed char s8;
typedef signed short s16;
typedef signed long s32;
typedef signed long long s64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;

typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;
typedef volatile s8 vs8;
typedef volatile s16 vs16;
typedef volatile s32 vs32;
typedef volatile s64 vs64;

typedef float f32;
typedef double f64;
typedef long double f128;
typedef volatile f32 vf32;
typedef volatile f64 vf64;
typedef volatile f128 vf128;

typedef u32 unknown;

#ifndef _SIZE_T_DEF
#define _SIZE_T_DEF
typedef unsigned long size_t;
#endif

#ifndef __cplusplus
typedef u16 wchar_t;
#endif

#define TRUE  1
#define FALSE 0

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif
#endif

#if !defined(__cplusplus) || __cplusplus < 201103L
#define nullptr 0
#endif

#define SET_FLAG(x, val)   (x |= (val))
#define RESET_FLAG(x, val) (x &= ~(val))
#define IS_FLAG(x, val)    (x & val)
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(o) (sizeof((o)) / sizeof(*(o)))
#endif
#define ALIGN_PREV(X, N)     ((X) & ~((N) - 1))
#define ALIGN_NEXT(X, N)     ALIGN_PREV(((X) + (N) - 1), N)
#define IS_ALIGNED(X, N)     ((X & ((N) - 1)) == 0)
#define IS_NOT_ALIGNED(X, N) (((X) & ((N) - 1)) != 0)
#define MAX(a, b)            (((a) > (b)) ? (a) : (b))
#define MIN(a, b)            (((a) < (b)) ? (a) : (b))

#define MACRO_NOTHING
#define MACRO_CONCAT_INNER(a, b) a##b
#define MACRO_CONCAT(a, b)       MACRO_CONCAT_INNER(a, b)
#define TO_STRING(x)             STRINGIFY(x)
#define STRINGIFY(x)             #x

// compile time size checks, works in C and C++03
#define STATIC_ASSERT(cond) typedef char MACRO_CONCAT(static_assert_line_, __LINE__)[(cond) ? 1 : -1]

#ifdef __cplusplus
#define BEGIN_SCOPE_EXTERN_C extern "C" {
#define END_SCOPE_EXTERN_C   }
#else
#define BEGIN_SCOPE_EXTERN_C
#define END_SCOPE_EXTERN_C
#endif

// metrowerks extensions
#ifdef __MWERKS__
#define AT_ADDRESS(addr)     : (addr)
#define WEAK                 __declspec(weak)
#define DECL_SECT(name)      __declspec(section name)
#define ASM                  asm
#define ATTRIBUTE_ALIGN(num) __attribute__((aligned(num)))
#else
#define AT_ADDRESS(addr)
#define WEAK
#define DECL_SECT(name)
#define ASM
#define ATTRIBUTE_ALIGN(num)
#endif

#define INIT  DECL_SECT(".init")
#define CTORS DECL_SECT(".ctors")

#endif // _TYPES_H
