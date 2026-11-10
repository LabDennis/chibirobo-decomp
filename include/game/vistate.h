#ifndef _GAME_VISTATE_H
#define _GAME_VISTATE_H

extern "C" {
#include <sysdolphin/video.h>
}

// HSD's video state, partial. the full struct is in video.c
typedef struct _HSD_VICurrent {
	HSD_VIStatus vi;
	u8 chg_flag;
} HSD_VICurrent;
extern "C" struct _HSD_VIInfo {
	HSD_VICurrent current;
} HSD_VIData;

#endif
