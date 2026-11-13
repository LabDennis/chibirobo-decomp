#ifndef _GAME_CLIGHT_H
#define _GAME_CLIGHT_H

#include "game/cshadow.h"
#include "game/hsd_lobj.h"
#include "game/ut.h"

// An HSD_LObj with its own position, interest, colors and shadow on top. Class name and
// base from RTTI. The setters are inline, most copies are in game part 1.
class CLight : public HSD_LObj {
public:
	CLight();

	HSD_LObj* mLoadedLObj;  // _D4, the light Init made from a desc
	CVec mPosition;         // _D8
	CVec mInterest;         // _E8
	CVec _F8;               // _F8
	CColor mColor;          // _108
	CColor mSpecColor;      // _110
	HSD_WObj mPositionWObj; // _118, what position points at when Init built the light by hand
	HSD_WObj mInterestWObj; // _138
	HSD_AObj* mLongestAObj; // _158, whichever of the lobj, position and interest anims ends last
	CShadow mShadow;        // _15C
	// _354 vtable

	virtual ~CLight() { }                                 // _08
	virtual void SetAnim(HSD_LightAnim* anim, f32 frame); // _0C
	virtual void SetAnimRate(f32 rate)                    // _10
	{
		HSD_ForeachAnim(this, LOBJ_TYPE, (HSD_TypeMask)(LOBJ_MASK | WOBJ_MASK), HSD_AObjSetRate, AOBJ_ARG_AF, rate);
	}
	virtual void Anim();                          // _14
	virtual void SetPosition(f32 x, f32 y, f32 z) // _18
	{
		mPosition.x = x;
		mPosition.y = y;
		mPosition.z = z;
		HSD_LObjSetPosition(this, &mPosition);
	}
	virtual void SetPosition(const Vec& position) { SetPosition(position.x, position.y, position.z); } // _1C
	virtual void SetInterest(f32 x, f32 y, f32 z)                                                      // _20
	{
		mInterest.x = x;
		mInterest.y = y;
		mInterest.z = z;
		HSD_LObjSetInterest(this, &mInterest);
	}
	virtual void SetInterest(const Vec& interest) { SetInterest(interest.x, interest.y, interest.z); } // _24

	void Init(u32 flags, BOOL useShadow);
	void Init(HSD_LightDesc* desc, BOOL useShadow);
	void Release();
	void SetCurrent(CCamera* camera);
};
typedef char CLight_size_check[sizeof(CLight) == 0x358 ? 1 : -1];

extern "C" {
// set up CBase's two light lists, cbasesetup.cpp
CLight* InitFourLightSet(CLight* lights);
CLight* InitTwoLightSet(CLight* lights);
}

#endif
