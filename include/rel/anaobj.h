#ifndef REL_ANAOBJ_H
#define REL_ANAOBJ_H

#include "Version.h"
#include "game/hsd_jobj.h"

struct UnkIdData {
	s32 mId; // _00
};

struct UnkKindData {
	u8 _00[0x14]; // _00
	s16 mKind;    // _14
};

// the parts of the dol's AnaObj this rel reads
class AnaObj {
public:
	HSD_JObj* mJObj; // _00
	u8 _04[0x30];    // _04
	// _34 vtable
	virtual void vf08();  // _08
	virtual void vf0C();  // _0C
	virtual void vf10();  // _10
	virtual void vf14();  // _14
	virtual void vf18();  // _18
	virtual void vf1C();  // _1C
	virtual void vf20();  // _20
	virtual void vf24();  // _24
	virtual void vf28();  // _28
	virtual void vf2C();  // _2C
	virtual void vf30();  // _30
	virtual void vf34();  // _34
	virtual void vf38();  // _38
	virtual void vf3C();  // _3C
	virtual void vf40();  // _40
	virtual void vf44();  // _44
	virtual void vf48();  // _48
	virtual void vf4C();  // _4C
	virtual void vf50();  // _50
	virtual void vf54();  // _54
	virtual void vf58();  // _58
	virtual void vf5C();  // _5C
	virtual void vf60();  // _60
	virtual void vf64();  // _64
	virtual void vf68();  // _68
	virtual void vf6C();  // _6C
	virtual void vf70();  // _70
	virtual void vf74();  // _74
	virtual void vf78();  // _78
	virtual void vf7C();  // _7C
	virtual void vf80();  // _80
	virtual void vf84();  // _84
	virtual void vf88();  // _88
	virtual void vf8C();  // _8C
	virtual void vf90();  // _90
	virtual void vf94();  // _94
	virtual void vf98();  // _98
	virtual void vf9C();  // _9C
	virtual void vfA0();  // _A0
	virtual void vfA4();  // _A4
	virtual void vfA8();  // _A8
	virtual void vfAC();  // _AC
	virtual void vfB0();  // _B0
	virtual void vfB4();  // _B4
	virtual void vfB8();  // _B8
	virtual void vfBC();  // _BC
	virtual void vfC0();  // _C0
	virtual void vfC4();  // _C4
	virtual void vfC8();  // _C8
	virtual void vfCC();  // _CC
	virtual void vfD0();  // _D0
	virtual void vfD4();  // _D4
	virtual void vfD8();  // _D8
	virtual void vfDC();  // _DC
	virtual void vfE0();  // _E0
	virtual void vfE4();  // _E4
	virtual void vfE8();  // _E8
	virtual void vfEC();  // _EC
	virtual void vfF0();  // _F0
	virtual void vfF4();  // _F4
	virtual void vfF8();  // _F8
	virtual void vfFC();  // _FC
	virtual void vf100(); // _100
	virtual void vf104(); // _104
	virtual void vf108(); // _108
	virtual void vf10C(); // _10C
	virtual void vf110(); // _110
	virtual void vf114(); // _114
	virtual void vf118(); // _118
	virtual void vf11C(); // _11C
	virtual void vf120(); // _120
	virtual void vf124(); // _124
	virtual void vf128(); // _128
	virtual void vf12C(); // _12C
	virtual void OnPushDisplacement(const Vec&);
	virtual void vf134(); // _134
	virtual void vf138(); // _138
	virtual void vf13C(); // _13C
	virtual void vf140(); // _140
	virtual void vf144(); // _144
	virtual u32 vf148();  // _148

#if defined(VERSION_JP)
	u8 _38[0xC0 - 0x38]; // _38
#else
	u8 _38[0xC8 - 0x38]; // _38
#endif
	Vec mPos;             // _C8 (JP: _C0)
	u8 _D4[0x228 - 0xD4]; // _D4 (JP: _CC)
	u64 mFlags;           // _228 (JP: _220)
	u8 _230[0x8];         // _230 (JP: _228)
	UnkIdData* mIdData;   // _238 (JP: _230)
#if defined(VERSION_JP)
	u8 _234[0x4]; // _234
#else
	u8 _23C[0x4]; // _23C
#endif
	UnkKindData* mKind; // _240 (JP: _238)
#if defined(VERSION_JP)
	u8 _23C[0x970 - 0x23C]; // _23C
#else
	u8 _244[0x980 - 0x244]; // _244
#endif
	u32* mBits; // _980 (JP: _970)
};

extern AnaObj* tutiAnaObj;
extern AnaObj* tutiHutaObj;
extern AnaObj* flowerObjs[3];
extern "C" HSD_JObj* GetAnaJObj(s32 kind, s32 which);

#endif
