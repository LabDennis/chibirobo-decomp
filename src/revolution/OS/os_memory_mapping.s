	.text
	.balign 4

	.globl Config24MB
	.type Config24MB, @function
Config24MB:
	li 7, 0x0
	lis 4, 0x0
	addi 4, 4, 0x2
	lis 3, 0x8000
	addi 3, 3, 0x1ff
	lis 6, 0x100
	addi 6, 6, 0x2
	lis 5, 0x8100
	addi 5, 5, 0xff
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
	mfmsr 3
	ori 3, 3, 0x30
	mtsrr1 3
	mflr 3
	mtsrr0 3
	rfi
	.size Config24MB, . - Config24MB

	.globl ConfigMEM2_52MB
	.type ConfigMEM2_52MB, @function
ConfigMEM2_52MB:
	li 7, 0x0
	lis 4, 0x1000
	addi 4, 4, 0x2
	lis 3, 0x9000
	addi 3, 3, 0x3ff
	lis 6, 0x1000
	addi 6, 6, 0x2a
	lis 5, 0xd000
	addi 5, 5, 0x7ff
	isync
	mtspr 568, 7
	mtspr 569, 4
	mtspr 568, 3
	isync
	mtspr 560, 7
	mtspr 561, 7
	isync
	mtspr 570, 7
	mtspr 571, 6
	mtspr 570, 5
	isync
	mtspr 562, 7
	mtspr 563, 7
	isync
	lis 4, 0x1200
	addi 4, 4, 0x2
	lis 3, 0x9200
	addi 3, 3, 0x1ff
	lis 6, 0x1300
	addi 6, 6, 0x2
	lis 5, 0x9300
	addi 5, 5, 0x7f
	isync
	mtspr 572, 7
	mtspr 573, 4
	mtspr 572, 3
	isync
	mtspr 564, 7
	mtspr 565, 7
	isync
	mtspr 574, 7
	mtspr 575, 6
	mtspr 574, 5
	isync
	mtspr 566, 7
	mtspr 567, 7
	isync
	mfmsr 3
	ori 3, 3, 0x30
	mtsrr1 3
	mflr 3
	mtsrr0 3
	rfi
	.size ConfigMEM2_52MB, . - ConfigMEM2_52MB

	.globl ConfigMEM2_56MB
	.type ConfigMEM2_56MB, @function
ConfigMEM2_56MB:
	li 7, 0x0
	lis 4, 0x1000
	addi 4, 4, 0x2
	lis 3, 0x9000
	addi 3, 3, 0x3ff
	lis 6, 0x1000
	addi 6, 6, 0x2a
	lis 5, 0xd000
	addi 5, 5, 0x7ff
	isync
	mtspr 568, 7
	mtspr 569, 4
	mtspr 568, 3
	isync
	mtspr 560, 7
	mtspr 561, 7
	isync
	mtspr 570, 7
	mtspr 571, 6
	mtspr 570, 5
	isync
	mtspr 562, 7
	mtspr 563, 7
	isync
	lis 4, 0x1200
	addi 4, 4, 0x2
	lis 3, 0x9200
	addi 3, 3, 0x1ff
	lis 6, 0x1300
	addi 6, 6, 0x2
	lis 5, 0x9300
	addi 5, 5, 0xff
	isync
	mtspr 572, 7
	mtspr 573, 4
	mtspr 572, 3
	isync
	mtspr 564, 7
	mtspr 565, 7
	isync
	mtspr 574, 7
	mtspr 575, 6
	mtspr 574, 5
	isync
	mtspr 566, 7
	mtspr 567, 7
	isync
	mfmsr 3
	ori 3, 3, 0x30
	mtsrr1 3
	mflr 3
	mtsrr0 3
	rfi
	.size ConfigMEM2_56MB, . - ConfigMEM2_56MB

	.globl ConfigMEM2_64MB
	.type ConfigMEM2_64MB, @function
ConfigMEM2_64MB:
	li 7, 0x0
	lis 4, 0x1000
	addi 4, 4, 0x2
	lis 3, 0x9000
	addi 3, 3, 0x7ff
	lis 6, 0x1000
	addi 6, 6, 0x2a
	lis 5, 0xd000
	addi 5, 5, 0x7ff
	isync
	mtspr 568, 7
	mtspr 569, 4
	mtspr 568, 3
	isync
	mtspr 560, 7
	mtspr 561, 7
	isync
	mtspr 570, 7
	mtspr 571, 6
	mtspr 570, 5
	isync
	mtspr 562, 7
	mtspr 563, 7
	isync
	mtspr 564, 7
	mtspr 565, 7
	isync
	mtspr 566, 7
	mtspr 567, 7
	isync
	mtspr 572, 7
	mtspr 573, 7
	isync
	mtspr 574, 7
	mtspr 575, 7
	isync
	mfmsr 3
	ori 3, 3, 0x30
	mtsrr1 3
	mflr 3
	mtsrr0 3
	rfi
	.size ConfigMEM2_64MB, . - ConfigMEM2_64MB

	.globl ConfigMEM2_112MB
	.type ConfigMEM2_112MB, @function
ConfigMEM2_112MB:
	li 7, 0x0
	lis 4, 0x1000
	addi 4, 4, 0x2
	lis 3, 0x9000
	addi 3, 3, 0x7ff
	lis 6, 0x1000
	addi 6, 6, 0x2a
	lis 5, 0xd000
	addi 5, 5, 0xfff
	isync
	mtspr 568, 7
	mtspr 569, 4
	mtspr 568, 3
	isync
	mtspr 560, 7
	mtspr 561, 7
	isync
	mtspr 570, 7
	mtspr 571, 6
	mtspr 570, 5
	isync
	mtspr 562, 7
	mtspr 563, 7
	isync
	lis 4, 0x1400
	addi 4, 4, 0x2
	lis 3, 0x9400
	addi 3, 3, 0x3ff
	lis 6, 0x1600
	addi 6, 6, 0x2
	lis 5, 0x9600
	addi 5, 5, 0x1ff
	isync
	mtspr 572, 7
	mtspr 573, 4
	mtspr 572, 3
	isync
	mtspr 564, 7
	mtspr 565, 7
	isync
	mtspr 574, 7
	mtspr 575, 6
	mtspr 574, 5
	isync
	mtspr 566, 7
	mtspr 567, 7
	isync
	mfmsr 3
	ori 3, 3, 0x30
	mtsrr1 3
	mflr 3
	mtsrr0 3
	rfi
	.size ConfigMEM2_112MB, . - ConfigMEM2_112MB

	.globl ConfigMEM2_128MB
	.type ConfigMEM2_128MB, @function
ConfigMEM2_128MB:
	li 7, 0x0
	lis 4, 0x1000
	addi 4, 4, 0x2
	lis 3, 0x9000
	addi 3, 3, 0xfff
	lis 6, 0x1000
	addi 6, 6, 0x2a
	lis 5, 0xd000
	addi 5, 5, 0xfff
	isync
	mtspr 568, 7
	mtspr 569, 4
	mtspr 568, 3
	isync
	mtspr 560, 7
	mtspr 561, 7
	isync
	mtspr 570, 7
	mtspr 571, 6
	mtspr 570, 5
	isync
	mtspr 562, 7
	mtspr 563, 7
	isync
	mtspr 564, 7
	mtspr 565, 7
	isync
	mtspr 566, 7
	mtspr 567, 7
	isync
	mtspr 572, 7
	mtspr 573, 7
	isync
	mtspr 574, 7
	mtspr 575, 7
	isync
	mfmsr 3
	ori 3, 3, 0x30
	mtsrr1 3
	mflr 3
	mtsrr0 3
	rfi
	.size ConfigMEM2_128MB, . - ConfigMEM2_128MB

	.globl RealMode
	.type RealMode, @function
RealMode:
	clrlwi 3, 3, 2
	mtsrr0 3
	mfmsr 3
	rlwinm 3, 3, 0, 28, 25
	mtsrr1 3
	rfi
	.size RealMode, . - RealMode
