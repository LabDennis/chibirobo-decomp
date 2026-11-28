	.set SPR_HID4, 1011
	.set CR1_EQ, 6

	.text
	.balign 4
	.globl PPCMthid4
	.type PPCMthid4, @function
PPCMthid4:
	stwu 1, -0x10(1)
	mflr 0
	stw 0, 0x14(1)
	clrrwi. 0, 3, 31
	stw 31, 0xc(1)
	mr 31, 3
	beq .Lclear_h4a

	mtspr SPR_HID4, 3
	b .Ldone

.Lclear_h4a:
	lis 3, .Lh4a_errata@ha
	addi 3, 3, .Lh4a_errata@l
	crclr CR1_EQ
	bl OSReport
	oris 31, 31, 0x8000
	mtspr SPR_HID4, 31

.Ldone:
	lwz 0, 0x14(1)
	lwz 31, 0xc(1)
	mtlr 0
	addi 1, 1, 0x10
	blr
	.size PPCMthid4, . - PPCMthid4

	.data
	.balign 8
.Lh4a_errata:
	.asciz "H4A should not be cleared because of Broadway errata.\n"
