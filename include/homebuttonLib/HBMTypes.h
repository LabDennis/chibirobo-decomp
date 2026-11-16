#ifndef HOME_BUTTON_LIB_TYPES_H
#define HOME_BUTTON_LIB_TYPES_H

// The library's view of its public types: KPADStatus and Vec2 from the KPAD header, the public
// interface from revolution/hbm.h, and the sound ids the menu passes to the sound callback.
#include <types.h>

#include "revolution/kpad.h"
#include "revolution/mem.h"
#include "revolution/wpad.h"

#include "revolution/hbm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum HBMSoundEvent {
    HBM_SOUND_INIT,
    HBM_SOUND_POST_INIT,
    HBM_SOUND_GOTO_MENU,
    HBM_SOUND_RETURN_APP,
    HBM_SOUND_STOP,
    HBM_SOUND_PLAY,
} HBMSoundEvent;

// Maps to the HomeButtonSe.brsar sound ID
typedef enum HBMSound {
    /* 0x00 */ HBM_SE_HOME_BUTTON,
    /* 0x01 */ HBM_SE_RETURN_APP,
    /* 0x02 */ HBM_SE_GOTO_MENU,
    /* 0x03 */ HBM_SE_RESET_APP,
    /* 0x04 */ HBM_SE_FOCUS,
    /* 0x05 */ HBM_SE_SELECT,
    /* 0x06 */ HBM_SE_CANCEL,
    /* 0x07 */ HBM_SE_OPEN_CONTROLLER,
    /* 0x08 */ HBM_SE_CLOSE_CONTROLLER,
    /* 0x09 */ HBM_SE_VOLUME_PLUS,
    /* 0x0A */ HBM_SE_VOLUME_MINUS,
    /* 0x0B */ HBM_SE_VOLUME_PLUS_LIMIT,
    /* 0x0C */ HBM_SE_VOLUME_MINUS_LIMIT,
    /* 0x0D */ HBM_SE_NOTHING_DONE,
    /* 0x0E */ HBM_SE_VIBE_ON,
    /* 0x0F */ HBM_SE_VIBE_OFF,
    /* 0x10 */ HBM_SE_START_CONNECT_WINDOW,
    /* 0x11 */ HBM_SE_CONNECTED,
    /* 0x12 */ HBM_SE_CONNECTED2,
    /* 0x13 */ HBM_SE_CONNECTED3,
    /* 0x14 */ HBM_SE_CONNECTED4,
    /* 0x15 */ HBM_SE_END_CONNECT_WINDOW
} HBMSound;

// Maps to the SpeakerSe.arc sound ID
typedef enum HBMSpeakerSound {
    /* 0x00 */ HBM_SPK_SE_VOLUME,
    /* 0x01 */ HBM_SPK_SE_CONNECT1,
    /* 0x02 */ HBM_SPK_SE_CONNECT2,
    /* 0x03 */ HBM_SPK_SE_CONNECT3,
    /* 0x04 */ HBM_SPK_SE_CONNECT4,
} HBMSpeakerSound;

#ifdef __cplusplus
}
#endif
#endif
