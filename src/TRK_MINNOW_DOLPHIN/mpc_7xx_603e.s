	.include "sdk/private/trk/state.inc"

	# Register groups within the Extended1 CPU block.
	.set CPU_TBL, TRK_CPU_EXTENDED1 + 0x40
	.set CPU_TBU, TRK_CPU_EXTENDED1 + 0x44
	.set CPU_HID0, TRK_CPU_EXTENDED1 + 0x48
	.set CPU_DMISS, TRK_CPU_EXTENDED1 + 0x98
	.set CPU_SDR1, TRK_CPU_EXTENDED1 + 0xb4
	.set CPU_DEC, TRK_CPU_EXTENDED1 + 0xd0
	.set CPU_DABR, TRK_CPU_EXTENDED1 + 0xdc
	.set CPU_UMMCR2, TRK_CPU_EXTENDED1 + 0x110
	.set CPU_MMCR2, TRK_CPU_EXTENDED1 + 0x138
	.set CPU_GQR0, TRK_CPU_EXTENDED1 + 0x154

	.set RESTORE_TBR, 0
	.set RESTORE_DEC, 1

	.set SPR_PVR, 287
	.set SPR_UMMCR2, 928
	.set SPR_UBAMR, 935
	.set SPR_USDA, 943
	.set SPR_MMCR2, 944
	.set SPR_BAMR, 951
	.set SPR_SDA, 959
	.set SPR_DMISS, 976
	.set SPR_DCMP, 977
	.set SPR_HASH1, 978
	.set SPR_HASH2, 979
	.set SPR_IMISS, 980
	.set SPR_ICMP, 981
	.set SPR_RPA, 982
	.set SPR_MSSCR0, 1014
	.set SPR_MSSCR1, 1015
	.set SPR_PIR, 1023

	.text
	.balign 4

	# These routines run inside the TRK context switch. The callers
	# preserve the target GPRs, so r2 and the high GPRs are scratch here.
	.globl TRKSaveExtended1Block
	.type TRKSaveExtended1Block, @function
TRKSaveExtended1Block:
	lis 2, gTRKCPUState@h
	ori 2, 2, gTRKCPUState@l

	.irp sr, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	mfsr 16 + \sr, \sr
	.endr
	stmw 16, TRK_CPU_EXTENDED1(2)

	mftb 10
	mftbu 11
	mfhid0 12
	mfhid1 13
	mfsrr1 14
	mfpvr 15
	.irp bat, 0,1,2,3
	mfibatu 16 + 2 * \bat, \bat
	mfibatl 17 + 2 * \bat, \bat
	.endr
	.irp bat, 0,1,2,3
	mfdbatu 24 + 2 * \bat, \bat
	mfdbatl 25 + 2 * \bat, \bat
	.endr
	stmw 10, CPU_TBL(2)

	mfsdr1 22
	mfdar 23
	mfdsisr 24
	mfsprg 25, 0
	mfsprg 26, 1
	mfsprg 27, 2
	mfsprg 28, 3
	li 29, 0 # The saved decrementer slot starts at zero.
	mfiabr 30
	mfear 31
	stmw 22, CPU_SDR1(2)

	.irp gqr, 0,1,2,3,4,5,6,7
	mfgqr 20 + \gqr, \gqr
	.endr
	mfhid2 28
	mfwpar 29
	mfdmau 30
	mfdmal 31
	stmw 20, CPU_GQR0(2)

	# Gekko skips the legacy monitor and processor identification registers.
	b .Lsave_performance_monitor
	mfspr 16, SPR_UMMCR2
	mfspr 17, SPR_UBAMR
	mfummcr0 18
	mfupmc1 19
	mfupmc2 20
	mfusia 21
	mfummcr1 22
	mfupmc3 23
	mfupmc4 24
	mfspr 25, SPR_USDA
	mfspr 26, SPR_MMCR2
	mfspr 27, SPR_BAMR
	mfspr 28, SPR_SDA
	mfspr 29, SPR_MSSCR0
	mfspr 30, SPR_MSSCR1
	mfspr 31, SPR_PIR
	stmw 16, CPU_UMMCR2(2)

.Lsave_performance_monitor:
	mfdabr 19
	mfpmc1 20
	mfpmc2 21
	mfpmc3 22
	mfpmc4 23
	mfsia 24
	mfmmcr0 25
	mfmmcr1 26
	mfthrm1 27
	mfthrm2 28
	mfthrm3 29
	mfictc 30
	mfl2cr 31
	stmw 19, CPU_DABR(2)
	blr

	# Legacy 603e save sequence for software-managed TLBs and the decrementer.
	mfspr 25, SPR_DMISS
	mfspr 26, SPR_DCMP
	mfspr 27, SPR_HASH1
	mfspr 28, SPR_HASH2
	mfspr 29, SPR_IMISS
	mfspr 30, SPR_ICMP
	mfspr 31, SPR_RPA
	stmw 25, CPU_DMISS(2)
	mfdec 31
	stw 31, CPU_DEC(2)
	blr
	.size TRKSaveExtended1Block, . - TRKSaveExtended1Block

	.globl TRKRestoreExtended1Block
	.type TRKRestoreExtended1Block, @function
TRKRestoreExtended1Block:
	lis 2, gTRKCPUState@h
	ori 2, 2, gTRKCPUState@l
	lis 5, gTRKRestoreFlags@h
	ori 5, 5, gTRKRestoreFlags@l

	# Consume the pending time-base and decrementer restore requests.
	lbz 3, RESTORE_TBR(5)
	lbz 6, RESTORE_DEC(5)
	li 0, 0
	stb 0, RESTORE_TBR(5)
	stb 0, RESTORE_DEC(5)
	cmpwi 3, 0
	beq .Lrestore_quantization
	lwz 24, CPU_TBL(2)
	lwz 25, CPU_TBU(2)
	mttbl 24
	mttbu 25

.Lrestore_quantization:
	lmw 20, CPU_GQR0(2)
	.irp gqr, 0,1,2,3,4,5,6,7
	mtgqr \gqr, 20 + \gqr
	.endr
	mthid2 28
	mtdmau 30
	mtdmal 31

	# Gekko skips the legacy monitor registers.
	b .Lrestore_performance_monitor
	lmw 26, CPU_MMCR2(2)
	mtspr SPR_MMCR2, 26
	mtspr SPR_BAMR, 27
	mtspr SPR_MSSCR0, 29
	mtspr SPR_MSSCR1, 30
	mtspr SPR_PIR, 31

.Lrestore_performance_monitor:
	lmw 19, CPU_DABR(2)
	mtdabr 19
	mtpmc1 20
	mtpmc2 21
	mtpmc3 22
	mtpmc4 23
	mtsia 24
	mtmmcr0 25
	mtmmcr1 26
	mtthrm1 27
	mtthrm2 28
	mtthrm3 29
	mtictc 30
	mtl2cr 31

	# Gekko skips the decrementer and the 603e TLB-miss registers.
	b .Lrestore_translation
	cmpwi 6, 0
	beq .Lrestore_603e_tlb
	lwz 26, CPU_DEC(2)
	mtdec 26

.Lrestore_603e_tlb:
	lmw 25, CPU_DMISS(2)
	mtspr SPR_DMISS, 25
	mtspr SPR_DCMP, 26
	mtspr SPR_HASH1, 27
	mtspr SPR_HASH2, 28
	mtspr SPR_IMISS, 29
	mtspr SPR_ICMP, 30
	mtspr SPR_RPA, 31

.Lrestore_translation:
	lmw 16, TRK_CPU_EXTENDED1(2)
	.irp sr, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	mtsr \sr, 16 + \sr
	.endr
	lmw 12, CPU_HID0(2)
	mthid0 12
	mthid1 13
	mtsrr1 14
	mtspr SPR_PVR, 15
	.irp bat, 0,1,2,3
	mtibatu \bat, 16 + 2 * \bat
	mtibatl \bat, 17 + 2 * \bat
	.endr
	.irp bat, 0,1,2,3
	mtdbatu \bat, 24 + 2 * \bat
	mtdbatl \bat, 25 + 2 * \bat
	.endr
	lmw 22, CPU_SDR1(2)
	mtsdr1 22
	mtdar 23
	mtdsisr 24
	mtsprg 0, 25
	mtsprg 1, 26
	mtsprg 2, 27
	mtsprg 3, 28
	mtiabr 30
	mtear 31
	blr
	.size TRKRestoreExtended1Block, . - TRKRestoreExtended1Block
