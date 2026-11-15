#include "revolution/hw_regs.h"
#include "Dolphin/os.h"
#include "revolution/vi/vi3in1.h"

// Hollywood GPIO lines of the A/V encoder's I2C bus
#define HW_GPIOB_OUT (*(vu32*)0xCD8000C0)
#define HW_GPIOB_DIR (*(vu32*)0xCD8000C4)
#define HW_GPIOB_IN  (*(vu32*)0xCD8000C8)

#define I2C_SCL 0x4000
#define I2C_SDA 0x8000

// the timer runs at a quarter of the 243 MHz bus clock
#define I2C_TIMER_CLOCK               (243000000 / 4)
#define I2CTicksToMicroseconds(ticks) (((ticks) * 8) / (I2C_TIMER_CLOCK / 125000))

static BOOL __i2c_ident_first = FALSE;
// FALSE: the data line is driven through an inverter
static volatile u32 __i2c_ident_flag = TRUE;

void WaitMicroTime(s32 usec)
{
	OSTime start;

	start = __OSGetSystemTime();
	while (I2CTicksToMicroseconds(__OSGetSystemTime() - start) < usec) { }
}

static void __i2c_set_scl(u32 level)
{
	if (level) {
		HW_GPIOB_OUT = (HW_GPIOB_OUT & ~I2C_SCL) | I2C_SCL;
	} else {
		HW_GPIOB_OUT = HW_GPIOB_OUT & ~I2C_SCL;
	}
}

static void __i2c_set_sda(u32 level)
{
	if (level) {
		if (__i2c_ident_flag == 0) {
			HW_GPIOB_OUT = HW_GPIOB_OUT & ~I2C_SDA;
		} else {
			HW_GPIOB_OUT = (HW_GPIOB_OUT & ~I2C_SDA) | I2C_SDA;
		}
	} else {
		if (__i2c_ident_flag == 0) {
			HW_GPIOB_OUT = (HW_GPIOB_OUT & ~I2C_SDA) | I2C_SDA;
		} else {
			HW_GPIOB_OUT = HW_GPIOB_OUT & ~I2C_SDA;
		}
	}
}

static void __i2c_set_sda_dir(u32 out)
{
	if (out) {
		HW_GPIOB_DIR = (HW_GPIOB_DIR & ~I2C_SDA) | I2C_SDA | I2C_SCL;
	} else {
		HW_GPIOB_DIR = (HW_GPIOB_DIR & ~I2C_SDA) | I2C_SCL;
	}
}

static BOOL sendSlaveAddr(u8 slaveAddr)
{
	s32 i;

	__i2c_set_sda(0);
	WaitMicroTime(2);
	__i2c_set_scl(0);

	for (i = 0; i < 8; i++) {
		if (slaveAddr & 0x80) {
			__i2c_set_sda(1);
		} else {
			__i2c_set_sda(0);
		}
		WaitMicroTime(2);
		__i2c_set_scl(1);
		WaitMicroTime(2);
		__i2c_set_scl(0);
		slaveAddr <<= 1;
	}

	__i2c_set_sda_dir(0);
	WaitMicroTime(2);
	__i2c_set_scl(1);
	WaitMicroTime(2);

	if (__i2c_ident_flag == 1 && ((HW_GPIOB_IN & I2C_SDA) >> 15)) {
		return FALSE;
	}

	__i2c_set_sda(0);
	__i2c_set_sda_dir(1);
	__i2c_set_scl(0);
	return TRUE;
}

static void __i2c_init(void)
{
	if (!__i2c_ident_first) {
		__i2c_ident_flag  = TRUE;
		__i2c_ident_first = TRUE;
	}
}

// NONMATCHING: same instructions, other callee-saved registers. Retail keeps the GPIO base in r31, i in r30, the
// byte in r29, enabled/nBytes/pData in r28-r26 and the hoisted (s64)2 of the waits in r25/r24; here the constant
// pair takes r31/r30 and everything else moves down two. Tried: every order of the locals, for/while/do/goto
// forms of both loops, the byte loop as a helper (plain and inline), the byte in slaveAddr, other forms of
// WaitMicroTime's loop. Without the outer loop the numbering is retail's, as in sendSlaveAddr.
BOOL __VISendI2CData(u8 slaveAddr, u8* pData, s32 nBytes)
{
	s32 i;
	u8 data;
	BOOL enabled;

	__i2c_init();

	enabled = OSDisableInterrupts();
	__i2c_set_sda_dir(1);
	__i2c_set_scl(1);
	__i2c_set_sda(1);
	WaitMicroTime(2);
	WaitMicroTime(2);

	if (!sendSlaveAddr(slaveAddr)) {
		OSRestoreInterrupts(enabled);
		return FALSE;
	}

	__i2c_set_sda_dir(1);
	for (; nBytes != 0; nBytes--) {
		data = *pData++;
		for (i = 0; i < 8; i++) {
			if (data & 0x80) {
				__i2c_set_sda(1);
			} else {
				__i2c_set_sda(0);
			}
			WaitMicroTime(2);
			__i2c_set_scl(1);
			WaitMicroTime(2);
			__i2c_set_scl(0);
			data <<= 1;
		}

		__i2c_set_sda_dir(0);
		WaitMicroTime(2);
		__i2c_set_scl(1);
		WaitMicroTime(2);

		if (__i2c_ident_flag == 1 && ((HW_GPIOB_IN & I2C_SDA) >> 15)) {
			OSRestoreInterrupts(enabled);
			return FALSE;
		}

		__i2c_set_sda(0);
		__i2c_set_sda_dir(1);
		__i2c_set_scl(0);
	}

	__i2c_set_sda_dir(1);
	__i2c_set_sda(0);
	WaitMicroTime(2);
	__i2c_set_scl(1);
	WaitMicroTime(2);
	__i2c_set_sda(1);
	OSRestoreInterrupts(enabled);
	return TRUE;
}
