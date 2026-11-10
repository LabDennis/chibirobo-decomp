#include "types.h"
#include "Version.h"

// a member function pointer the way mwcc lays them out
struct TitleStateFunc {
	s32 mThisDelta;  // _00
	s32 mVtblOffset; // _04, -1 for a non-virtual one
	void* mFunc;     // _08
};
STATIC_ASSERT(sizeof(TitleStateFunc) == 0xC);

// title's data statics, the compiler reaches all of them off the first one
struct TitleData {
#if defined(VERSION_JP)
	u8 _00[0x68]; // _00
#else
	u8 _00[0x7C]; // _00, gTitleStateData, then gTitleTextureSuffixes's head
#endif
	TitleStateFunc mInit[26];   // _7C, _68 in japan, the state handlers from 0x8002BB7C, last first
	TitleStateFunc mStates[26]; // _1B4, _1A0 in japan, gTitleModeUpdates
};

extern "C" TitleData gTitleStateData;

// todo: title.cpp's __sinit filling the state table, goes away once Title's handlers are methods
/**
 * @note Address: 0x80031004
 * @note Size: 0x358
 */
extern "C" void __sinit_titlestates_cpp()
{
	const TitleStateFunc* init = gTitleStateData.mInit;
	TitleStateFunc* states     = gTitleStateData.mStates;
	states[0]                  = init[0];
	states[1]                  = init[1];
	states[2]                  = init[2];
	states[3]                  = init[3];
	states[4]                  = init[4];
	states[5]                  = init[5];
	states[6]                  = init[6];
	states[7]                  = init[7];
	states[8]                  = init[8];
	states[9]                  = init[9];
	states[10]                 = init[10];
	states[11]                 = init[11];
	states[12]                 = init[12];
	states[13]                 = init[13];
	states[14]                 = init[14];
	states[15]                 = init[15];
	states[16]                 = init[16];
	states[17]                 = init[17];
	states[18]                 = init[18];
	states[19]                 = init[19];
	states[20]                 = init[20];
	states[21]                 = init[21];
	states[22]                 = init[22];
	states[23]                 = init[23];
	states[24]                 = init[24];
	states[25]                 = init[25];
}
