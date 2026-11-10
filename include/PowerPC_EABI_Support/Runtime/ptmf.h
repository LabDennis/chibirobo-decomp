#ifndef _RUNTIME_PTMF_H
#define _RUNTIME_PTMF_H

#include "types.h"

typedef struct PTMF {
	s32 this_delta; // self-explanatory
	s32 v_offset;   // vtable offset
	union {
		void* f_addr;  // function address
		s32 ve_offset; // virtual function entry offset (of vtable)
	} f_data;
} PTMF;

#endif
