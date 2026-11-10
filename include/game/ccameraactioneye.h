#ifndef _GAME_CCAMERAACTIONEYE_H
#define _GAME_CCAMERAACTIONEYE_H

#include "game/ccameraaction.h"
#include "game/cobj.h"

// split out since cobj.h puts .rodata in every file that includes it

// the first person view, the player looks around with the stick
class CCameraActionEye : public CInterfaceCameraAction {
public:
	CObjVec mLook;  // _40, x up and down, y around, z the fov offset
	CVec mOverride; // _70, a non zero part replaces the target the stick gives

	virtual void Update(u32 step); // _10
	virtual void vf14();           // _14
	virtual void Init();           // _1C
	virtual u32 GetCameraFlags();  // _54
	virtual ~CCameraActionEye();
};
STATIC_ASSERT(sizeof(CCameraActionEye) == 0x80);

#ifndef DEFINE_GAME_MEMBER_LIFETIMES
inline CCameraActionEye::~CCameraActionEye()
{
}
#endif

#endif
