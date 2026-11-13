#include "game/title.h"

extern "C" {
s32 gTitleStateData[26] = { 0, 3, 0, 0, 3, 0, 0, 2, 0, 0, 2, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 3, 0, 0, 3, 0 };

#if !defined(VERSION_JP)
const char* gTitleTextureSuffixes[5] = { "_en", "_ge", "_fr", "_sp", "_it" };
#endif
}

TitleModeUpdate gTitleModeUpdates[26] = {
	&Title::State0,  &Title::State1,  &Title::State2,  &Title::State3,  &Title::State4,  &Title::State5,  &Title::State6,
	&Title::State7,  &Title::State8,  &Title::State9,  &Title::State10, &Title::State11, &Title::State12, &Title::State13,
	&Title::State14, &Title::State15, &Title::State16, &Title::State17, &Title::State18, &Title::State19, &Title::State20,
	&Title::State21, &Title::State22, &Title::State23, &Title::State24, &Title::State25,
};
