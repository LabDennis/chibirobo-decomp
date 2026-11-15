#ifndef _MSL_BUFFER_IO_H
#define _MSL_BUFFER_IO_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <stdio.h>

int __flush_buffer(FILE* file, size_t* length);
void __prep_buffer(FILE* file);

#endif
