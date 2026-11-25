#ifndef DOLPHIN_DSP_REGS_H
#define DOLPHIN_DSP_REGS_H

/* Halfword indices in __DSPRegs. */
#define DSP_MAILBOX_IN_HI   (0)
#define DSP_MAILBOX_IN_LO   (1)
#define DSP_MAILBOX_OUT_HI  (2)
#define DSP_MAILBOX_OUT_LO  (3)
#define DSP_CONTROL_STATUS (5)

#define DSP_ARAM_SIZE        (9)
#define DSP_ARAM_MODE        (11)
#define DSP_ARAM_REFRESH     (13)
#define DSP_ARAM_DMA_MM_HI   (16)
#define DSP_ARAM_DMA_MM_LO   (17)
#define DSP_ARAM_DMA_ARAM_HI (18)
#define DSP_ARAM_DMA_ARAM_LO (19)
#define DSP_ARAM_DMA_SIZE_HI (20)
#define DSP_ARAM_DMA_SIZE_LO (21)

/* Word indices in the 32-bit register view. */
#define DSP_ARAM_DMA_MM_WORD   (DSP_ARAM_DMA_MM_HI / 2)
#define DSP_ARAM_DMA_ARAM_WORD (DSP_ARAM_DMA_ARAM_HI / 2)
#define DSP_ARAM_DMA_SIZE_WORD (DSP_ARAM_DMA_SIZE_HI / 2)

#define DSP_ARAM_DMA_DIRECTION_BIT (15)
#define DSP_ARAM_DMA_DIRECTION_MASK (1 << DSP_ARAM_DMA_DIRECTION_BIT)

#define DSP_DMA_START_HI    (24)
#define DSP_DMA_START_LO    (25)
#define DSP_DMA_CONTROL_LEN (27)
#define DSP_DMA_BYTES_LEFT  (29)

/* DMA addresses and the ARAM byte count use the same split field. */
#define DSP_DMA_HI_FIELD_MASK (0x03FF)
#define DSP_DMA_LO_FIELD_MASK (0xFFE0)
#define DSP_DMA_BLOCK_SHIFT   (5)
#define DSP_DMA_BLOCK_SIZE    (1 << DSP_DMA_BLOCK_SHIFT)

/* Audio DMA length counts blocks. */
#define DSP_DMA_LENGTH_MASK (0x7FFF)
#define DSP_DMA_START_FLAG  (0x8000)

/* Control and status bits. Write 1 to acknowledge a pending interrupt. */
#define DSP_CONTROL_RESET           (0x0001)
#define DSP_CONTROL_ASSERT_INT      (0x0002)
#define DSP_CONTROL_HALT            (0x0004)
#define DSP_STATUS_AI_INTERRUPT     (0x0008)
#define DSP_CONTROL_AI_INT_ENABLE   (0x0010)
#define DSP_STATUS_ARAM_INTERRUPT   (0x0020)
#define DSP_CONTROL_ARAM_INT_ENABLE (0x0040)
#define DSP_STATUS_DSP_INTERRUPT    (0x0080)
#define DSP_CONTROL_DSP_INT_ENABLE  (0x0100)
#define DSP_STATUS_DMA_BUSY         (0x0200)
#define DSP_STATUS_INIT_CODE        (0x0400)
#define DSP_CONTROL_INIT            (0x0800)

#define DSP_STATUS_INTERRUPTS (DSP_STATUS_AI_INTERRUPT | DSP_STATUS_ARAM_INTERRUPT | DSP_STATUS_DSP_INTERRUPT)

#define DSP_MAILBOX_FULL_BIT (15)
#define DSP_MAILBOX_FULL_HI  (1 << DSP_MAILBOX_FULL_BIT)

#endif
