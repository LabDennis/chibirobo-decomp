	# C layouts in ppc_reg.h and trktypes.h. Checked in targimpl.c.
	.set TRK_CPU_GPR,                 0x000
	.set TRK_CPU_PC,                  0x080
	.set TRK_CPU_LR,                  0x084
	.set TRK_CPU_CR,                  0x088
	.set TRK_CPU_CTR,                 0x08c
	.set TRK_CPU_XER,                 0x090
	.set TRK_CPU_EXTENDED1,           0x1a8
	.set TRK_CPU_EXCEPTION_ID,        0x2f8
	.set TRK_CPU_TRANSPORT_RETURN,    0x42c

	.set TRK_STATE_GPR,               0x00
	.set TRK_STATE_LR,                0x80
	.set TRK_STATE_CTR,               0x84
	.set TRK_STATE_XER,               0x88
	.set TRK_STATE_MSR,               0x8c
	.set TRK_STATE_DAR,               0x90
	.set TRK_STATE_DSISR,             0x94
	.set TRK_STATE_INPUT_ACTIVATED,   0x9c
	.set TRK_STATE_INPUT_PENDING_PTR, 0xa0
