#ifndef MSL_WMEM_H
#define MSL_WMEM_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

wchar_t* wmemcpy(wchar_t* dest, const wchar_t* src, size_t n);
wchar_t* wmemchr(wchar_t* s, wchar_t c, int n);
void* memmove(void*, const void*, size_t);

#endif
