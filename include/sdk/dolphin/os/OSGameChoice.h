#ifndef _DOLPHIN_OSGAMECHOICE_H_
#define _DOLPHIN_OSGAMECHOICE_H_

#include <types.h>

// The low five bits specify the reset timer in minutes.
#define OS_GAME_CHOICE_RESET_MINUTES_MASK 0x1F
#define OS_GAME_CHOICE_NO_PAD_MOTOR       0x20
#define OS_GAME_CHOICE_NO_PAD_RECAL       0x40
#define OS_GAME_CHOICE_NO_MEMORY_CARD    0x80

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __MWERKS__
u8 GameChoice AT_ADDRESS(0x800030E3);
#else
extern u8 GameChoice;
#endif

#ifdef __cplusplus
}
#endif

#endif
