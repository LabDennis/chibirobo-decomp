#include <cstddef>
#include <dolphin/exi.h>
#include <dolphin/os.h>

#include "__os.h"

#define RTC_CHANNEL 0
#define RTC_DEVICE 1
#define RTC_CMD_READ 0x20000000U
#define RTC_CMD_WRITE 0xA0000000U
#define SRAM_ADDRESS 0x100U
#define RTC_ADDRESS_SHIFT 6

#define SRAM_CHECKSUM_FIRST (offsetof(OSSram, counterBias) / sizeof(u16))
#define SRAM_CHECKSUM_END (sizeof(OSSram) / sizeof(u16))

static union {
    SramControl control;
    struct {
        OSSram base;
        OSSramEx extended;
    } settings;
    u16 checksumWords[SRAM_CHECKSUM_END];
} Scb ATTRIBUTE_ALIGN(DOLPHIN_ALIGNMENT);

// prototypes
static int GetRTC(u32* rtc);
static int ReadSram(void* buffer);
static void WriteSramCallback(s32, OSContext*);
static int WriteSram(void* buffer, u32 offset, u32 size);
static void* LockSram(u32 offset);
static int UnlockSram(int commit, u32 offset);
static void __OSReadROMCallback(s32 chan, OSContext*);

/**
 * @note Address: N/A
 * @note Size: ?
 */
static int GetRTC(u32* rtc) {
    int err;
    u32 cmd;

    if (EXILock(RTC_CHANNEL, RTC_DEVICE, NULL) == 0) {
        return 0;
    }
    if (EXISelect(RTC_CHANNEL, RTC_DEVICE, EXI_FREQ_8M) == 0) {
        EXIUnlock(RTC_CHANNEL);
        return 0;
    }
    cmd = RTC_CMD_READ;
    err = 0;
    err |= !EXIImm(RTC_CHANNEL, &cmd, sizeof(cmd), EXI_WRITE, NULL);
    err |= !EXISync(RTC_CHANNEL);
    err |= !EXIImm(RTC_CHANNEL, &cmd, sizeof(cmd), EXI_READ, NULL);
    err |= !EXISync(RTC_CHANNEL);
    err |= !EXIDeselect(RTC_CHANNEL);
    EXIUnlock(RTC_CHANNEL);
    rtc[0] = cmd;
    return !err;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
int __OSGetRTC(u32* rtc) {
    int err;
    u32 t0;
    u32 t1;
    int i;

    for(i = 0; i < 16; i++) {
        err = 0;
        err |= !GetRTC(&t0);
        err |= !GetRTC(&t1);
        if (err) {
            break;
        }
        if (t0 == t1) {
            rtc[0] = t0;
            return 1;
        }
    }
    return 0;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
int __OSSetRTC(u32 rtc) {
    int err;
    u32 cmd;

    if (EXILock(RTC_CHANNEL, RTC_DEVICE, NULL) == 0) {
        return 0;
    }
    if (EXISelect(RTC_CHANNEL, RTC_DEVICE, EXI_FREQ_8M) == 0) {
        EXIUnlock(RTC_CHANNEL);
        return 0;
    }
    cmd = RTC_CMD_WRITE;
    err = 0;
    err |= !EXIImm(RTC_CHANNEL, &cmd, sizeof(cmd), EXI_WRITE, NULL);
    err |= !EXISync(RTC_CHANNEL);
    err |= !EXIImm(RTC_CHANNEL, &rtc, sizeof(rtc), EXI_WRITE, NULL);
    err |= !EXISync(RTC_CHANNEL);
    err |= !EXIDeselect(RTC_CHANNEL);
    EXIUnlock(RTC_CHANNEL);
    return !err;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static int ReadSram(void* buffer) {
    int err;
    u32 cmd;

    DCInvalidateRange(buffer, SRAM_SIZE);
    if (!EXILock(RTC_CHANNEL, RTC_DEVICE, NULL)) {
        return 0;
    }
    if (!EXISelect(RTC_CHANNEL, RTC_DEVICE, EXI_FREQ_8M)) {
        EXIUnlock(RTC_CHANNEL);
        return 0;
    }
    cmd = RTC_CMD_READ | SRAM_ADDRESS;
    err = 0;
    err |= !EXIImm(RTC_CHANNEL, &cmd, sizeof(cmd), EXI_WRITE, NULL);
    err |= !EXISync(RTC_CHANNEL);
    err |= !EXIDma(RTC_CHANNEL, buffer, SRAM_SIZE, EXI_READ, NULL);
    err |= !EXISync(RTC_CHANNEL);
    err |= !EXIDeselect(RTC_CHANNEL);
    EXIUnlock(RTC_CHANNEL);
    return !err;
}

/**
 * @note Address: 0x80168EC0
 * @note Size: 0x60
 */
static void WriteSramCallback(s32, OSContext*) {
    ASSERTLINE(258, !Scb.control.locked);
    Scb.control.sync = WriteSram(&Scb.control.sram[Scb.control.offset], Scb.control.offset, SRAM_SIZE - Scb.control.offset);
    if (Scb.control.sync != 0) {
        Scb.control.offset = SRAM_SIZE;
    }
    ASSERTLINE(264, Scb.control.sync);
}

/**
 * @note Address: 0x80168F20
 * @note Size: 0x118
 */
static int WriteSram(void* buffer, u32 offset, u32 size) {
    int err;
    u32 cmd;

    if (!EXILock(RTC_CHANNEL, RTC_DEVICE, WriteSramCallback)) {
        return 0;
    }
    if (!EXISelect(RTC_CHANNEL, RTC_DEVICE, EXI_FREQ_8M)) {
        EXIUnlock(RTC_CHANNEL);
        return 0;
    }
    offset <<= RTC_ADDRESS_SHIFT;
    cmd = RTC_CMD_WRITE | (SRAM_ADDRESS + offset);
    err = 0;
    err |= !EXIImm(RTC_CHANNEL, &cmd, sizeof(cmd), EXI_WRITE, NULL);
    err |= !EXISync(RTC_CHANNEL);
    err |= !EXIImmEx(RTC_CHANNEL, buffer, size, EXI_WRITE);
    err |= !EXIDeselect(RTC_CHANNEL);
    EXIUnlock(RTC_CHANNEL);
    return !err;
}

/**
 * @note Address: 0x80169038
 * @note Size: 0x13C
 */
void __OSInitSram(void) {
    Scb.control.locked = Scb.control.enabled = FALSE;
    Scb.control.sync = ReadSram(Scb.control.sram);
    ASSERTLINE(318, Scb.control.sync);
    Scb.control.offset = SRAM_SIZE;

    OSSetGbsMode(OSGetGbsMode());
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static void* LockSram(u32 offset) {
    BOOL enabled;

    enabled = OSDisableInterrupts();
    ASSERTLINE(341, !Scb.control.locked);
    if (Scb.control.locked) {
        OSRestoreInterrupts(enabled);
        return NULL;
    }
    Scb.control.enabled = enabled;
    Scb.control.locked = TRUE;
    return (u8*)&Scb.settings + offset;
}

/**
 * @note Address: 0x80169174
 * @note Size: 0x5C
 */
OSSram* __OSLockSram(void) {
    return (OSSram*)LockSram(0);
}

/**
 * @note Address: 0x801691D0
 * @note Size: 0x5C
 */
OSSramEx* __OSLockSramEx(void) {
    return (OSSramEx*)LockSram(sizeof(OSSram));
}

/**
 * @note Address: 0x8016922C
 * @note Size: 0x33C
 */
static int UnlockSram(int commit, u32 offset) {
    u16* p;

    ASSERTLINE(375, Scb.control.locked);
    if (commit != 0) {
        if (offset == 0) {
            OSSram* sram = &Scb.settings.base;
            if (2u < (sram->flags & 3)) {
                sram->flags &= ~3;
            }

            sram->checkSum = sram->checkSumInv = 0;
            for (p = Scb.checksumWords + SRAM_CHECKSUM_FIRST; p < Scb.checksumWords + SRAM_CHECKSUM_END; p++) {
                sram->checkSum += *p;
                sram->checkSumInv += ~(*p);
            }
        }
        if (offset < Scb.control.offset) {
            Scb.control.offset = offset;
        }

        if (Scb.control.offset <= sizeof(OSSram)) {
            OSSramEx* sram = &Scb.settings.extended;
            if (((u32)sram->gbs & 0x7c00) == 0x5000 || ((u32)sram->gbs & 0xc0) == 0xc0) {
                sram->gbs = 0;
            }
        }

        Scb.control.sync = WriteSram(&Scb.control.sram[Scb.control.offset], Scb.control.offset, SRAM_SIZE - Scb.control.offset);
        if (Scb.control.sync != 0) {
            Scb.control.offset = SRAM_SIZE;
        }
    }
    Scb.control.locked = FALSE;
    OSRestoreInterrupts(Scb.control.enabled);
    return Scb.control.sync;
}

/**
 * @note Address: 0x80169568
 * @note Size: 0x24
 */
int __OSUnlockSram(int commit) {
    return UnlockSram(commit, 0);
}

/**
 * @note Address: 0x8016958C
 * @note Size: 0x24
 */
int __OSUnlockSramEx(int commit) {
    UnlockSram(commit, sizeof(OSSram));
}

/**
 * @note Address: 0x801695B0
 * @note Size: 0x10
 */
int __OSSyncSram(void) {
    return Scb.control.sync;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
int __OSCheckSram(void) {
    u16* p;
    u16 checkSum;
    u16 checkSumInv;
    OSSram* sram;

    ASSERTLINE(466, Scb.control.locked);

    checkSum = checkSumInv = 0;

    sram = &Scb.settings.base;

    for (p = Scb.checksumWords + SRAM_CHECKSUM_FIRST; p < Scb.checksumWords + SRAM_CHECKSUM_END; p++) {
        checkSum += *p;
        checkSumInv += ~(*p);
    }

    return (sram->checkSum == checkSum && sram->checkSumInv == checkSumInv);
}

/**
 * @note Address: 0x801695C0
 * @note Size: 0x124
 */
int __OSReadROM(void * buffer, s32 length, s32 offset) {
    int err;
    u32 cmd;

    ASSERTLINE(497, length <= 1024);
    DCInvalidateRange(buffer, length);
    if (EXILock(RTC_CHANNEL, RTC_DEVICE, NULL) == 0) {
        return 0;
    }
    if (EXISelect(RTC_CHANNEL, RTC_DEVICE, EXI_FREQ_8M) == 0) {
        EXIUnlock(RTC_CHANNEL);
        return 0;
    }
    cmd = (u32)offset << RTC_ADDRESS_SHIFT;
    err = 0;
    err |= !EXIImm(RTC_CHANNEL, &cmd, sizeof(cmd), EXI_WRITE, NULL);
    err |= !EXISync(RTC_CHANNEL);
    err |= !EXIDma(RTC_CHANNEL, buffer, length, EXI_READ, NULL);
    err |= !EXISync(RTC_CHANNEL);
    err |= !EXIDeselect(RTC_CHANNEL);
    EXIUnlock(RTC_CHANNEL);
    return !err;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static void __OSReadROMCallback(s32 chan, OSContext*) {
    void (*callback)(void);

    EXIDeselect(chan);
    EXIUnlock(chan);
    callback = Scb.control.callback;
    if (callback) {
        Scb.control.callback = NULL;
        callback();
    }
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
int __OSReadROMAsync(void* buffer, s32 length, s32 offset, void (*callback)(void)) {
    int err;
    u32 cmd;

    ASSERTLINE(556, length <= 1024);
    ASSERTLINE(557, callback);
    DCInvalidateRange(buffer, length);
    Scb.control.callback = callback;
    if (EXILock(RTC_CHANNEL, RTC_DEVICE, NULL) == 0) {
        return 0;
    }
    if (EXISelect(RTC_CHANNEL, RTC_DEVICE, EXI_FREQ_8M) == 0) {
        EXIUnlock(RTC_CHANNEL);
        return 0;
    }
    cmd = (u32)offset << RTC_ADDRESS_SHIFT;
    err = 0;
    err |= !EXIImm(RTC_CHANNEL, &cmd, sizeof(cmd), EXI_WRITE, NULL);
    err |= !EXISync(RTC_CHANNEL);
    err |= !EXIDma(RTC_CHANNEL, buffer, length, EXI_READ, __OSReadROMCallback);
    return !err;
}

/**
 * @note Address: 0x801696E4
 * @note Size: 0x80
 */
u32 OSGetSoundMode(void) {
    OSSram* sram = __OSLockSram();
    u32 mode = (sram->flags & 4) ? 1 : 0;

    __OSUnlockSram(0);
    return mode;
}

/**
 * @note Address: 0x80169764
 * @note Size: 0xA4
 */
void OSSetSoundMode(u32 mode) {
    OSSram* sram;
    u8 currentFlags;
    u32 modeBit;

    ASSERTLINE(617, mode == OS_SOUND_MODE_MONO || mode == OS_SOUND_MODE_STEREO);
    modeBit = (mode << 2) & 4;
    sram = __OSLockSram();
    currentFlags = sram->flags & 4;
    if (modeBit == currentFlags) {
        __OSUnlockSram(0);
        return;
    }
    sram->flags &= 0xFFFFFFFB;
    sram->flags |= modeBit;
    __OSUnlockSram(1);
}

/**
 * @note Address: 0x80169808
 * @note Size: 0x70
 */
u32 OSGetProgressiveMode(void) {
    OSSram* sram;
    u32 on;

    sram = __OSLockSram();
    on = (sram->flags & 0x80) >> 7;
    __OSUnlockSram(FALSE);
    return on;
}

/**
 * @note Address: 0x80169878
 * @note Size: 0xA4
 */
void OSSetProgressiveMode(u32 on) {
    OSSram* sram;
    u8 currentFlags;
    u32 modeBit;

    ASSERTLINE(670, on == OS_PROGRESSIVE_MODE_OFF || on == OS_PROGRESSIVE_MODE_ON);

    modeBit = (on << 7) & 0x80;

    sram = __OSLockSram();
    currentFlags = sram->flags & 0x80;
    if (modeBit == currentFlags) {
        __OSUnlockSram(FALSE);
        return;
    }

    sram->flags &= ~0x80;
    sram->flags |= modeBit;
    __OSUnlockSram(TRUE);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
u32 OSGetVideoMode(void) {
    OSSram* sram = __OSLockSram();
    u32 mode = sram->flags & 3;

    __OSUnlockSram(0);

    if (mode > 2) {
        mode = 0;
    }

    return mode;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void OSSetVideoMode(u32 mode) {
    OSSram* sram;
    u8 currentMode;

    ASSERTLINE(731, OS_VIDEO_MODE_NTSC <= mode && mode <= OS_VIDEO_MODE_MPAL);

    if (mode > 2) {
        mode = 0;
    }

    sram = __OSLockSram();

    currentMode = sram->flags & 3;
    if (mode == currentMode) {
        __OSUnlockSram(0);
        return;
    }
    sram->flags &= 0xFFFFFFFC;
    sram->flags |= mode;
    __OSUnlockSram(1);
}

/**
 * @note Address: 0x8016991C
 * @note Size: 0x6C
 */
u8 OSGetLanguage(void) {
    OSSram* sram = __OSLockSram();
    u8 language = sram->language;

    __OSUnlockSram(0);
    return language;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void OSSetLanguage(u8 language) {
    OSSram* sram = __OSLockSram();
    u32 currentLanguage;

    currentLanguage = sram->language;
    if (language == currentLanguage) {
        __OSUnlockSram(0);
        return;
    }
    sram->language = language;
    __OSUnlockSram(1);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
u8 __OSGetBootMode(void) {
    OSSram* sram = __OSLockSram();
    u8 ntd = sram->ntd;
    __OSUnlockSram(0);
    return ntd & 0x80;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void __OSSetBootMode(u8 ntd) {
    OSSram* sram;
    u32 currentMode;

    ntd &= 0x80;
    sram = __OSLockSram();
    currentMode = sram->ntd & 0x80U;
    if (ntd == currentMode) {
        __OSUnlockSram(0);
        return;
    }
    sram->ntd &= 0xFFFFFF7F;
    sram->ntd |= ntd;
    __OSUnlockSram(1);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
u32 OSGetEuRgb60Mode(void) {
    OSSram* sram;
    u32 on;

    sram = __OSLockSram();
    on = (sram->ntd & 0x40) >> 6;
    __OSUnlockSram(0);
    return on;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void OSSetEuRgb60Mode(u32 on) {
    OSSram* sram;
    u8 currentFlags;
    u32 modeBit;

    ASSERTLINE(895, on == OS_EURGB60_OFF || on == OS_EURGB60_ON);
    modeBit = (on << 6) & 0x40;

    sram = __OSLockSram();
    currentFlags = sram->ntd & 0x40;
    if (modeBit == currentFlags) {
        __OSUnlockSram(0);
    } else {
        sram->ntd &= ~0x40;
        sram->ntd |= modeBit;
        __OSUnlockSram(1);
    }
}

/**
 * @note Address: 0x80169988
 * @note Size: 0x84
 */
u16 OSGetWirelessID(s32 chan) {
    OSSramEx* sram;
    u16 id;

    sram = __OSLockSramEx();
    id = sram->wirelessPadID[chan];
    __OSUnlockSramEx(FALSE);
    return id;
}

/**
 * @note Address: 0x80169A0C
 * @note Size: 0xAC
 */
void OSSetWirelessID(s32 chan, u16 id) {
    OSSramEx* sram;

    sram = __OSLockSramEx();
    if (sram->wirelessPadID[chan] != id) {
        sram->wirelessPadID[chan] = id;
        __OSUnlockSramEx(TRUE);
        return;
    }

    __OSUnlockSramEx(FALSE);
}

/**
 * @note Address: 0x80169AB8
 * @note Size: 0x70
 */
u16 OSGetGbsMode(void) {
    OSSramEx* sram;
    u16 mode;

    sram = __OSLockSramEx();
    mode = sram->gbs;
    __OSUnlockSramEx(FALSE);
    return mode;
}

/**
 * @note Address: 0x80169B28
 * @note Size: 0xB8
 */
void OSSetGbsMode(u16 mode) {
    u16 currentMode;
    OSSramEx* sram;

    if (((u32)mode & 0x7c00) == 0x5000 || ((u32)mode & 0xc0) == 0xc0) {
        mode = 0;
    }

    sram = __OSLockSramEx();

    currentMode = sram->gbs;
    if (mode == currentMode) {
        __OSUnlockSramEx(FALSE);
        return;
    }
    sram->gbs = mode;

    __OSUnlockSramEx(TRUE);
}
