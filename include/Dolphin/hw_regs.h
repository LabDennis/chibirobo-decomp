#ifndef _DOLPHIN_HW_REGS_H
#define _DOLPHIN_HW_REGS_H

#include "types.h"
#include "Dolphin/OS/OSUtil.h"
#include <sdk/dolphin/dsp_regs.h>
#include <sdk/dolphin/exi_regs.h>

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

#define HW_REG(reg, type) *(volatile type*)(u32)(reg) // generic HW_REG macro (please do not use this)

/////// HARDWARE REGISTERS ///////
// Video Interface registers.
vu16 __VIRegs[59] AT_ADDRESS(0xCC002000);

// offsets for __VIRegs[i]
#define VI_VERT_TIMING        (0)
#define VI_DISP_CONFIG        (1)
#define VI_HORIZ_TIMING_0L    (2)
#define VI_HORIZ_TIMING_0U    (3)
#define VI_HORIZ_TIMING_1L    (4)
#define VI_HORIZ_TIMING_1U    (5)
#define VI_VERT_TIMING_ODD    (6)
#define VI_VERT_TIMING_ODD_U  (7)
#define VI_VERT_TIMING_EVEN   (8)
#define VI_VERT_TIMING_EVEN_U (9)

#define VI_BBI_ODD    (10) // burst blanking interval
#define VI_BBI_ODD_U  (11) // burst blanking interval
#define VI_BBI_EVEN   (12) // burst blanking interval
#define VI_BBI_EVEN_U (13) // burst blanking interval

#define VI_TOP_FIELD_BASE_LEFT   (14) // top in 2d, top of left pic in 3d
#define VI_TOP_FIELD_BASE_LEFT_U (15) // top in 2d, top of left pic in 3d

#define VI_TOP_FIELD_BASE_RIGHT   (16) // top of right pic in 3d
#define VI_TOP_FIELD_BASE_RIGHT_U (17) // top of right pic in 3d

#define VI_BTTM_FIELD_BASE_LEFT   (18) // bottom in 2d, bottom of left pic in 3d
#define VI_BTTM_FIELD_BASE_LEFT_U (19) // bottom in 2d, bottom of left pic in 3d

#define VI_BTTM_FIELD_BASE_RIGHT   (20) // bottom of right pic in 3d
#define VI_BTTM_FIELD_BASE_RIGHT_U (21) // bottom of right pic in 3d

#define VI_VERT_COUNT  (22) // vertical display position
#define VI_HORIZ_COUNT (23) // horizontal display position

#define VI_DISP_INT_0  (24) // display interrupt 0L
#define VI_DISP_INT_0U (25) // display interrupt 0U
#define VI_DISP_INT_1  (26) // display interrupt 1L
#define VI_DISP_INT_1U (27) // display interrupt 1U
#define VI_DISP_INT_2  (28) // display interrupt 2L
#define VI_DISP_INT_2U (29) // display interrupt 2U
#define VI_DISP_INT_3  (30) // display interrupt 3L
#define VI_DISP_INT_3U (31) // display interrupt 3U

#define VI_HSW (36) // horizontal scaling width
#define VI_HSR (37) // horizontal scaling register

#define VI_FCT_0  (38) // filter coefficient table 0L
#define VI_FCT_0U (39) // filter coefficient table 0U
#define VI_FCT_1  (40) // filter coefficient table 1L
#define VI_FCT_1U (41) // filter coefficient table 1U
#define VI_FCT_2  (42) // filter coefficient table 2L
#define VI_FCT_2U (43) // filter coefficient table 2U
#define VI_FCT_3  (44) // filter coefficient table 3L
#define VI_FCT_3U (45) // filter coefficient table 3U
#define VI_FCT_4  (46) // filter coefficient table 4L
#define VI_FCT_4U (47) // filter coefficient table 4U
#define VI_FCT_5  (48) // filter coefficient table 5L
#define VI_FCT_5U (49) // filter coefficient table 5U
#define VI_FCT_6  (50) // filter coefficient table 6L
#define VI_FCT_6U (51) // filter coefficient table 6U

#define VI_CLOCK_SEL (54) // clock select
#define VI_DTV_STAT  (55) // DTV status

#define VI_WIDTH (56)

// Processor Interface registers.
vu32 __PIRegs[12] AT_ADDRESS(0xCC003000);

// offsets for __PIRegs[i]
#define PI_INTRPT_SRC  (0) // interrupt cause
#define PI_INTRPT_MASK (1) // interrupt mask
#define PI_FIFO_START  (3) // FIFO base start
#define PI_FIFO_END    (4) // FIFO base end
#define PI_FIFO_PTR    (5) // FIFO current write pointer

#define PI_RESETCODE (9) // reset code, used by OSReset

// PI Interrupt causes.
#define PI_INTRPT_ERR       (0x1)     // GP runtime error
#define PI_INTRPT_RSW       (0x2)     // reset switch
#define PI_INTRPT_DVD       (0x4)     // DVD/DI interrupt
#define PI_INTRPT_SI        (0x8)     // serial/controller interrupt
#define PI_INTRPT_EXI       (0x10)    // external interface interrupt
#define PI_INTRPT_AI        (0x20)    // audio streaming interrupt
#define PI_INTRPT_DSP       (0x40)    // digital signal proc interrupt
#define PI_INTRPT_MEM       (0x80)    // memory interface interrupt
#define PI_INTRPT_VI        (0x100)   // video interface interrupt
#define PI_INTRPT_PE_TOKEN  (0x200)   // pixel engine token
#define PI_INTRPT_PE_FINISH (0x400)   // pixel engine finish
#define PI_INTRPT_CP        (0x800)   // command FIFO
#define PI_INTRPT_DEBUG     (0x1000)  // external debugger
#define PI_INTRPT_HSP       (0x2000)  // high speed port
#define PI_INTRPT_RSWST     (0x10000) // reset switch state (1 when pressed)

// Memory Interface registers.
vu16 __MEMRegs[64] AT_ADDRESS(0xCC004000);

// offsets for __MEMRegs[i]
#define MEM_PROT_START(channel) (2 * (channel))
#define MEM_PROT_END(channel)   (MEM_PROT_START(channel) + 1)
#define MEM_PROT_TYPE           (8) // protection type
#define MEM_PROT_ACCESS_BITS    (2)
#define MEM_PROT_ADDR_SHIFT     (10)

#define MEM_INTRPT_MASK    (14) // interrupt mask
#define MEM_INTRPT_SRC     (15) // interrupt cause
#define MEM_INTRPT_FLAG    (16) // set when interrupt happens
#define MEM_INTRPT_ADDR_LO (17) // address that caused interrupt
#define MEM_INTRPT_ADDR_HI (18) // address that caused interrupt
#define MEM_INTRPT_ADDR_HI_MASK (0x03FF)

#define MEM_UNK_FLAG (20) // unknown memory flag, set in __OSInitMemoryProtection

// Digital Signal Processor registers (for audio mixing).
vu16 __DSPRegs[32] AT_ADDRESS(0xCC005000);
// The ARAM bootstrap writes pairs of these halfword registers as words.
extern vu32 __DSPRegs32[16] AT_ADDRESS(0xCC005000);

// DVD Interface registers.
vu32 __DIRegs[16] AT_ADDRESS(0xCC006000);

// offsets for __DIRegs[i]
#define DI_STATUS       (0)
#define DI_COVER_STATUS (1) // cover status - 0=normal, 1=interrupt/open
#define DI_CMD_BUF_0    (2) // command buffer 0
#define DI_CMD_BUF_1    (3) // command buffer 1
#define DI_CMD_BUF_2    (4) // command buffer 2
#define DI_DMA_MEM_ADDR (5) // DMA address
#define DI_DMA_LENGTH   (6) // transfer length address
#define DI_CONTROL      (7)
#define DI_MM_BUF       (8) // Main memory buffer
#define DI_CONFIG       (9)

// Serial Interface registers.
vu32 __SIRegs[64] AT_ADDRESS(0xCC006400);

// offsets for __SIRegs[i]
// Channel 0/Joy-channel 1
#define SI_CHAN_0_BUF   (0) // output buffer
#define SI_CHAN_0_BTN_1 (1) // button 1
#define SI_CHAN_0_BTN_2 (2) // button 2
// Channel 1/Joy-channel 2
#define SI_CHAN_1_BUF   (3) // output buffer
#define SI_CHAN_1_BTN_1 (4) // button 1
#define SI_CHAN_1_BTN_2 (5) // button 2
// Channel 2/Joy-channel 3
#define SI_CHAN_2_BUF   (6) // output buffer
#define SI_CHAN_2_BTN_1 (7) // button 1
#define SI_CHAN_2_BTN_2 (8) // button 2
// Channel 3/Joy-channel 4
#define SI_CHAN_3_BUF   (9)  // output buffer
#define SI_CHAN_3_BTN_1 (10) // button 1
#define SI_CHAN_3_BTN_2 (11) // button 2

#define SI_POLL     (12)
#define SI_CC_STAT  (13) // communication control status
#define SI_STAT     (14)
#define SI_EXI_LOCK (15) // exi clock lock

#define SI_IO_BUFFER (32) // start of buffer (32 to 63)

// Expansion/External Interface registers.
vu32 __EXIRegs[16] AT_ADDRESS(0xCC006800);

// Audio Streaming Interface registers.
vu32 __AIRegs[8] AT_ADDRESS(0xCC006C00);

// offsets for __AIRegs[i]
#define AI_CONTROL        (0) // control
#define AI_VOLUME         (1) // volume
#define AI_SAMPLE_COUNTER (2) // number of stereo samples output
#define AI_INTRPT_TIMING  (3) // interrupt timing

#define AI_CONTROL_PLAY_STATE         (0x1)
#define AI_CONTROL_STREAM_SAMPLE_RATE (0x2)
#define AI_CONTROL_STREAM_INT_ENABLE  (0x4)
#define AI_STATUS_STREAM_INTERRUPT    (0x8) // Write 1 to acknowledge.
#define AI_CONTROL_STREAM_COUNT_RESET (0x20)
#define AI_CONTROL_DSP_SAMPLE_RATE    (0x40)

//////////////////////////////////

#ifdef __cplusplus
};
#endif // ifdef __cplusplus

#endif
