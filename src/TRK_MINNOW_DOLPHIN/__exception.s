	.set MSR_TRANSLATION, 0x30
	.set MSR_TGPR_HI, 0x0002

	# Pass the vector offset in r3 and preserve the interrupted r2-r4 in SPRGs.
	.macro redirect vector
	mtsprg 1, 2
	mtsprg 2, 3
	mtsprg 3, 4
	mfsrr0 2
	mfsrr1 4
	mfmsr 3
	ori 3, 3, MSR_TRANSLATION
	mtsrr1 3
	lis 3, TRKInterruptHandler@h
	ori 3, 3, TRKInterruptHandler@l
	mtsrr0 3
	li 3, \vector
	rfi
	.endm

	.macro vector_entry vector
	.org gTRKInterruptVectorTable + \vector
	redirect \vector
	.endm

	# On a 603e TLB miss, the temporary GPR bank may be active on entry.
	.macro tlb_vector vector
	.org gTRKInterruptVectorTable + \vector
	mtsprg 1, 2
	mfcr 2
	mtsprg 2, 2
	mfmsr 2
	andis. 2, 2, MSR_TGPR_HI
	beq .Lrestore_cr\@
	mfmsr 2
	xoris 2, 2, MSR_TGPR_HI
	sync
	mtmsr 2
	sync
	mtsprg 1, 2
.Lrestore_cr\@:
	mfsprg 2, 2
	mtcr 2
	mfsprg 2, 1
	redirect \vector
	.endm

	.section .init, "ax", @progbits
	.balign 4
	.globl gTRKInterruptVectorTable, gTRKInterruptVectorTableEnd
	.type gTRKInterruptVectorTable, @object
gTRKInterruptVectorTable:
	.asciz "Metrowerks Target Resident Kernel for PowerPC"

	.org gTRKInterruptVectorTable + 0x100
	b __TRK_reset

	.org gTRKInterruptVectorTable + 0x200
	# Machine check. Invalidate the affected instruction and data lines.
	mtsprg 1, 2
	mfsrr0 2
	icbi 0, 2
	mfdar 2
	dcbi 0, 2
	mfsprg 2, 1
	redirect 0x200

	vector_entry 0x300  # Data storage
	vector_entry 0x400  # Instruction storage
	vector_entry 0x500  # External interrupt
	vector_entry 0x600  # Alignment
	vector_entry 0x700  # Program exception
	vector_entry 0x800  # FPU unavailable
	vector_entry 0x900  # Decrementer
	vector_entry 0xc00  # System call
	vector_entry 0xd00  # Trace
	vector_entry 0xe00  # Floating-point assist

	# The performance monitor entry jumps past the AltiVec unavailable vector.
	.org gTRKInterruptVectorTable + 0xf00
	b .Lperformance_monitor
	vector_entry 0xf20
.Lperformance_monitor:
	redirect 0xf00

	tlb_vector 0x1000  # Instruction TLB miss
	tlb_vector 0x1100  # Data-load TLB miss
	tlb_vector 0x1200  # Data-store TLB miss

	vector_entry 0x1300  # Instruction address breakpoint
	vector_entry 0x1400  # System management interrupt
	vector_entry 0x1600  # Denormal detection
	vector_entry 0x1700  # Thermal management interrupt
	vector_entry 0x1c00  # Data breakpoint
	vector_entry 0x1d00  # Instruction breakpoint
	vector_entry 0x1e00  # Peripheral breakpoint
	vector_entry 0x1f00  # Non-maskable development port

gTRKInterruptVectorTableEnd:
	.size gTRKInterruptVectorTable, . - gTRKInterruptVectorTable
