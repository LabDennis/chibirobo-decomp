	.set CACHED_RAM_BASE, 0x80000000
	.set MEM_SIZE_8MB, 0x00800000
	.set MEM_SIZE_16MB, 0x01000000
	.set MEM_SIZE_32MB, 0x02000000
	.set BAT_VALID_BOTH_MODES, 3
	.set BAT_READ_WRITE, 2
	.set BAT_LENGTH_MASK, 0x1ffc
	.set BAT_LENGTH_SHIFT, 15
	.set MSR_IR_BIT, 26
	.set MSR_DR_BIT, 27
	.set MSR_TRANSLATION, 0x30

	.macro load_immediate reg, value
	lis \reg, \value@ha
	addi \reg, \reg, \value@l
	.endm

	# Program matching data and instruction mappings for each RAM bank.
	.macro map_memory name, first_bank, second_bank
	.set .Lfirst_length\@, ((\first_bank - 1) >> BAT_LENGTH_SHIFT) & BAT_LENGTH_MASK
	.set .Lsecond_length\@, ((\second_bank - 1) >> BAT_LENGTH_SHIFT) & BAT_LENGTH_MASK
	.set .Lbat0_upper\@, CACHED_RAM_BASE | .Lfirst_length\@ | BAT_VALID_BOTH_MODES
	.set .Lbat2_upper\@, (CACHED_RAM_BASE + \first_bank) | .Lsecond_length\@ | BAT_VALID_BOTH_MODES
	.set .Lbat2_lower\@, \first_bank | BAT_READ_WRITE

	.globl \name
	.type \name, @function
\name:
	li 7, 0
	load_immediate 4, BAT_READ_WRITE
	load_immediate 3, .Lbat0_upper\@
	load_immediate 6, .Lbat2_lower\@
	load_immediate 5, .Lbat2_upper\@

	isync

	mtdbatu 0, 7
	mtdbatl 0, 4
	mtdbatu 0, 3
	isync

	mtibatu 0, 7
	mtibatl 0, 4
	mtibatu 0, 3
	isync

	mtdbatu 2, 7
	mtdbatl 2, 6
	mtdbatu 2, 5
	isync

	mtibatu 2, 7
	mtibatl 2, 6
	mtibatu 2, 5
	isync

	# Resume the caller at its cached address with translation enabled.
	mfmsr 3
	ori 3, 3, MSR_TRANSLATION
	mtsrr1 3
	mflr 3
	mtsrr0 3
	rfi
	.size \name, . - \name
	.endm

	.text
	.balign 4
	map_memory Config24MB, MEM_SIZE_16MB, MEM_SIZE_8MB
	map_memory Config48MB, MEM_SIZE_32MB, MEM_SIZE_16MB

	.globl RealMode
	.type RealMode, @function
RealMode:
	# Enter the mapping routine at its physical address with translation off.
	clrlwi 3, 3, 2
	mtsrr0 3
	mfmsr 3
	rlwinm 3, 3, 0, MSR_DR_BIT + 1, MSR_IR_BIT - 1
	mtsrr1 3
	rfi
	.size RealMode, . - RealMode
