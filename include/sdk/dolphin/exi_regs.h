#ifndef DOLPHIN_EXI_REGS_H
#define DOLPHIN_EXI_REGS_H

// Word offsets within each channel's register block.
#define EXI_REG_CPR          0
#define EXI_REG_DMA_ADDR     1
#define EXI_REG_DMA_LEN      2
#define EXI_REG_CONTROL      3
#define EXI_REG_DATA         4
#define EXI_REGS_PER_CHANNEL 5
#define EXI_REG(channel, offset) (__EXIRegs[(channel) * EXI_REGS_PER_CHANNEL + (offset)])

// Channel parameter register. Write 1 to an interrupt status bit to acknowledge it.
#define EXI_CPR_EXI_INT_ENABLE 0x0001
#define EXI_CPR_EXI_INT        0x0002
#define EXI_CPR_TC_INT_ENABLE  0x0004
#define EXI_CPR_TC_INT         0x0008
#define EXI_CPR_CLK_MASK       0x0070
#define EXI_CPR_CS_MASK        0x0380
#define EXI_CPR_EXT_INT_ENABLE 0x0400
#define EXI_CPR_EXT_INT        0x0800
#define EXI_CPR_EXT_PRESENT    0x1000
#define EXI_CPR_ROM_DISABLE    0x2000

#define EXI_CPR_INT_ENABLES (EXI_CPR_EXI_INT_ENABLE | EXI_CPR_TC_INT_ENABLE | EXI_CPR_EXT_INT_ENABLE)
#define EXI_CPR_INT_FLAGS   (EXI_CPR_EXI_INT | EXI_CPR_TC_INT | EXI_CPR_EXT_INT)
#define EXI_CPR_CS(device)  ((1u << (device)) << 7)
#define EXI_CPR_CLK(freq)   ((freq) << 4)

#define EXI_CR_START             0x1
#define EXI_CR_DMA               0x2
#define EXI_CR_RW(type)          ((u32)(type) << 2)
#define EXI_CR_IMM(type, length) (EXI_CR_START | EXI_CR_RW(type) | ((u32)((length) - 1) << 4))

#define EXI_DMA_ADDR_MASK 0x03FFFFE0

#endif
