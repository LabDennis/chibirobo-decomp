#include "types.h"

// presumably, ptmf = pointer to member function

typedef struct PTMF {
	s32 this_delta; // self-explanatory
	s32 v_offset;   // vtable offset
	union {
		void* f_addr;  // function address
		s32 ve_offset; // virtual function entry offset (of vtable)
	} f_data;
} PTMF;

const PTMF __ptmf_null = { 0, 0, 0 };

s32 __ptmf_test(PTMF* ptmf);
void __ptmf_scall(...);

/**
 * @note Address: N/A
 * @note Size: 0x24
 */
void __ptmf_cast(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0x28
 */
void __ptmf_scall4(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0x34
 */
void __ptmf_call4(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0x34
 */
void __ptmf_call(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x80151434
 * @note Size: 0x30
 */
ASM s32 __ptmf_test(register PTMF* ptmf) {
#ifdef __MWERKS__ // clang-format off
	nofralloc
		lwz       r5, PTMF.this_delta(r3)
		lwz       r6, PTMF.v_offset(r3)
		lwz       r7, PTMF.f_data(r3)
		li        r3, 0x1
		cmpwi     r5, 0
		cmpwi     cr6, r6, 0
		cmpwi     cr7, r7, 0
		bnelr-
		bnelr-    cr6
		bnelr-    cr7
		li        r3, 0
		blr
#endif // clang-format on
}

/**
 * @note Address: 0x80151464
 * @note Size: 0x3C
 */
ASM s32 __ptmf_cmpr(register PTMF* ptmf1, register PTMF* ptmf2) {
#ifdef __MWERKS__ // clang-format off
	nofralloc
		lwz       r5, PTMF.this_delta(r3)
		lwz       r6, PTMF.this_delta(r4)
		lwz       r7, PTMF.v_offset(r3)
		lwz       r8, PTMF.v_offset(r4)
		lwz       r9, PTMF.f_data(r3)
		lwz       r10, PTMF.f_data(r4)
		li        r3, 0x1
		cmpw      r5, r6
		cmpw      cr6, r7, r8
		cmpw      cr7, r9, r10
		bnelr-
		bnelr-    cr6
		bnelr-    cr7
		li        r3, 0
		blr
#endif // clang-format on
}

/**
 * @note Address: 0x801514A0
 * @note Size: 0x28
 */
ASM void __ptmf_scall(...)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc
		lwz     r0, PTMF.this_delta(r12)
		lwz     r11, PTMF.v_offset(r12)
		lwz     r12, PTMF.f_data(r12)
		add     r3, r3, r0
		cmpwi   r11, 0
		blt-    cr0, loc_0x20
		lwzx    r12, r3, r12
		lwzx    r12, r12, r11
	loc_0x20:
		mtctr   r12
		bctr
#endif // clang-format on
}
