#include "types.h"
#include "Dolphin/PPCArch.h"

union FpscrUnion {
	f64 f;
	struct {
		u32 fpscr_pad;
		u32 fpscr;
	} u;
};

#define HID0_SPD 0x00000200 // Speculative cache access enable (0 enable)

void PPCMthid0(u32 newHID0);

/**
 * @note Address: 0x80161150
 * @note Size: 0x8
 */
ASM u32 PPCMfmsr(void) {
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mfmsr r3
	blr
#endif // clang-format on
}

/**
 * @note Address: 0x80161158
 * @note Size: 0x8
 */
ASM void PPCMtmsr(register u32 newMSR)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mtmsr newMSR
	blr
#endif // clang-format on
}

/**
 * @note Address: N/A
 * @note Size: 0xC
 */
void PPCOrMsr(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0xC
 */
void PPCAndMsr(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0xC
 */
void PPCAndCMsr(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x80161160
 * @note Size: 0x8
 */
ASM u32 PPCMfhid0(void) {
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mfspr r3, HID0
	blr
#endif // clang-format on
}

/**
 * @note Address: 0x80161168
 * @note Size: 0x8
 */
ASM void PPCMthid0(register u32 newHID0)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mtspr HID0, newHID0
	blr
#endif // clang-format on
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
void PPCMfhid1(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x80161170
 * @note Size: 0x8
 */
ASM u32 PPCMfl2cr(void) {
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mfspr r3, L2CR
	blr
#endif // clang-format on
}

/**
 * @note Address: 0x80161178
 * @note Size: 0x8
 */
ASM void PPCMtl2cr(register u32 newL2cr) {
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mtspr L2CR, newL2cr
	blr
#endif // clang-format on
}

/**
 * @note Address: 0x80161180
 * @note Size: 0x8
 */
WEAK ASM void PPCMtdec(register u32 newDec)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mtdec newDec
	blr
#endif // clang-format on
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
void PPCMfdec(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x80161188
 * @note Size: 0x8
 */
ASM void PPCSync(void)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc
	sc
	blr
#endif // clang-format on
}

/**
 * @note Address: N/A
 * @note Size: 0x34
 */
void PPCEieio(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x80161190
 * @note Size: 0x14
 */
WEAK ASM void PPCHalt(void) // spins infinitely
{
#ifdef __MWERKS__ // clang-format off
	nofralloc

	sync

_spin:
	nop
	li r3, 0
	nop
	b _spin

	// NEVER REACHED
#endif // clang-format on
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
void PPCMfmmcr0(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x801611A4
 * @note Size: 0x8
 */
ASM void PPCMtmmcr0(register u32 newMmcr0)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mtspr       MMCR0, newMmcr0
	blr
#endif // clang-format on
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
void PPCMfmmcr1(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x801611AC
 * @note Size: 0x8
 */
ASM void PPCMtmmcr1(register u32 newMmcr1)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mtspr       MMCR1, newMmcr1
	blr
#endif // clang-format on
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
void PPCMfpmc1(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x801611B4
 * @note Size: 0x8
 */
ASM void PPCMtpmc1(register u32 newPmc1)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mtspr       PMC1, newPmc1
	blr
#endif // clang-format on
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
void PPCMfpmc2(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x801611BC
 * @note Size: 0x8
 */
ASM void PPCMtpmc2(register u32 newPmc2)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mtspr       PMC2, newPmc2
	blr
#endif // clang-format on
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
void PPCMfpmc3(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x801611C4
 * @note Size: 0x8
 */
ASM void PPCMtpmc3(register u32 newPmc3)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mtspr       PMC3, newPmc3
	blr
#endif // clang-format on
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
void PPCMfpmc4(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x801611CC
 * @note Size: 0x8
 */
ASM void PPCMtpmc4(register u32 newPmc4)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mtspr       PMC4, newPmc4
	blr
#endif // clang-format on
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
void PPCMfsia(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
void PPCMtsia(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x801611D4
 * @note Size: 0x20
 */
u32 PPCMffpscr(void)
{
	union FpscrUnion m;

#ifdef __MWERKS__ // clang-format off
	asm {
		mffs fp31
		stfd fp31, m.f;
	}
#endif // clang-format on

	return m.u.fpscr;
}

/**
 * @note Address: 0x801611F4
 * @note Size: 0x28
 */
void PPCMtfpscr(register u32 newFPSCR)
{
	union FpscrUnion m;

#ifdef __MWERKS__ // clang-format off
	asm {
		li    r4, 0
		stw   r4, m.u.fpscr_pad;
		stw   newFPSCR, m.u.fpscr
		lfd   fp31, m.f
		mtfsf 0xff, fp31
	}
#endif // clang-format on
}

/**
 * @note Address: 0x8016121C
 * @note Size: 0x8
 */
ASM u32 PPCMfhid2(void) {
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mfspr r3, 920
	blr
#endif // clang-format on
}

/**
 * @note Address: 0x80161224
 * @note Size: 0x8
 */
ASM void PPCMthid2(register u32 newhid2) {
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mtspr 920, newhid2
	blr
#endif // clang-format on
}

/**
 * @note Address: N/A
 * @note Size: 0xC
 * UNUSED
 */
ASM u32 PPCMfwpar(void) {
#ifdef __MWERKS__ // clang-format off
	nofralloc
	sync
	mfspr r3, 921
	blr
#endif // clang-format on
}

/**
 * @note Address: 0x8016122C
 * @note Size: 0x8
 */
ASM void PPCMtwpar(register u32 newwpar)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mtspr WPAR, newwpar
	blr
#endif // clang-format on
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
void PPCMfdmaU(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
void PPCMfdmaL(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
void PPCMtdmaU(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
void PPCMtdmaL(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
void PPCMfpvr(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0x28
 */
void PPCEnableSpeculation(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x80161234
 * @note Size: 0x28
 */
void PPCDisableSpeculation(void)
{
	PPCMthid0(PPCMfhid0() | HID0_SPD);
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 * UNUSED
 */
ASM void PPCSetFpIEEEMode(void) {
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mtfsb0      29
	blr
#endif // clang-format on
}

/**
 * @note Address: 0x8016125C
 * @note Size: 0x8
 */
ASM void PPCSetFpNonIEEEMode(void)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc
	mtfsb1      29
	blr
#endif // clang-format on
}

void OSReport(const char* msg, ...);

void PPCMthid4(register u32 newHID4)
{
	if (newHID4 & 0x80000000) {
#ifdef __MWERKS__
		asm { mtspr 1011, newHID4 }
#endif
	} else {
		OSReport("H4A should not be cleared because of Broadway errata.\n");
		newHID4 |= 0x80000000;
#ifdef __MWERKS__
		asm { mtspr 1011, newHID4 }
#endif
	}
}
