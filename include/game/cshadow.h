#ifndef _GAME_CSHADOW_H
#define _GAME_CSHADOW_H

#include "game/ccamera.h"
#include "game/hsd_shadow.h"
#include "types.h"

extern "C" {
#include <sysdolphin/lobj.h>

// the empty function after HSD_StartRender in initialize.c
void HSD_EndRender(void);
}

// a light's shadow, an HSD_Shadow plus the viewing rect and a camera copy to render it
// from. Class name from RTTI. Only the dtor is virtual, its one copy is in game part 1.
class CShadow {
public:
	HSD_Shadow* mShadow;   // _00
	BOOL mActive;          // _04
	HSD_ViewingRect mRect; // _08
	CCamera mCamera;       // _5C, the shadow cobj copied over each render
	u8 mIntensity;         // _1F0
	// _1F4 vtable

	CShadow()
	{
		mActive = FALSE;
		mShadow = NULL;
	}
	virtual ~CShadow() { } // _08

	void Init(HSD_LObj* lobj);
	void Release();
	void InitRect();
	void AddRect(Vec* position, f32 top, f32 bottom, f32 left, f32 right);
	CCamera* StartRender();
	void EndRender();

	bool IsActive() { return mShadow != NULL && mActive; }
	void SetActive(BOOL active) { HSD_ShadowSetActive(mShadow, mActive = active); }
};
STATIC_ASSERT(sizeof(CShadow) == 0x1F8);

#endif
