#ifndef REVOLUTION_HBM_H
#define REVOLUTION_HBM_H

#include "types.h"
#include "revolution/mem.h"

// The HOME Button Menu library (homebuttonLib), its public interface.

#ifdef __cplusplus
extern "C" {
#endif

// KPADStatus, which the pad code declares as WiiPadSample (game/wii/cpad.h)
struct WiiPadSample;
typedef struct WiiPadSample KPADStatus;

typedef struct Vec2 {
	f32 x, y;
} Vec2;

// what the menu was closed with; HBMCalc returns HBM_SELECT_NULL while it is open
typedef enum HBMSelectBtnNum {
	HBM_SELECT_NULL = -1,
	HBM_SELECT_HOMEBTN, // closed again, back to the game
	HBM_SELECT_BTN1,    // Wii Menu
	HBM_SELECT_BTN2,    // Reset
	HBM_SELECT_BTN3,
	HBM_SELECT_BTN4,
	HBM_SELECT_MAX
} HBMSelectBtnNum;

typedef int (*HBMSoundCallback)(int evt, int num);

typedef struct HBMDataInfo {
	void* layoutBuf;                 // _00, homeBtn*.arc for the console language
	void* spkSeBuf;                  // _04, SpeakerSe.arc
	void* msgBuf;                    // _08, home.csv
	void* configBuf;                 // _0C, config.txt
	void* mem;                       // _10, the menu's work memory, or NULL to take it from pAllocator
	HBMSoundCallback sound_callback; // _14
	BOOL backFlag;                   // _18
	int region;                      // _1C, SCGetLanguage
	BOOL cursor;                     // _20
	int messageFlag;                 // _24
	u32 configBufSize;               // _28
	u32 memSize;                     // _2C
	f32 frameDelta;                  // _30, 1 at 60 frames a second
	Vec2 adjust;                     // _34, the layout's scale, x is 832/608 on a 16:9 screen
	MEMAllocator* pAllocator;        // _3C
} HBMDataInfo;

typedef struct HBMKPadData {
	KPADStatus* kpad; // _00, NULL without a controller
	Vec2 pos;         // _04, the cursor, -1 to 1 on both axes
	u32 use_devtype;  // _0C
} HBMKPadData;

typedef struct HBMControllerData {
	HBMKPadData wiiCon[4]; // _00
} HBMControllerData;

void HBMCreate(const HBMDataInfo* info);
void HBMInit(void);
HBMSelectBtnNum HBMCalc(const HBMControllerData* controllers);
void HBMDraw(void);
HBMSelectBtnNum HBMGetSelectBtnNum(void);
void HBMSetAdjustFlag(BOOL flag);
void HBMStartBlackOut(void);
void HBMCreateSound(void* soundData, void* memBuf, u32 memSize);
void HBMDeleteSound(void);
void HBMUpdateSound(void);

#ifdef __cplusplus
}
#endif
#endif
