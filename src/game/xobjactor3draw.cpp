#include "game/xobjactor.h"

extern "C" {
void DrawStage02TimerPanel(XObjActor3* actor);
void DrawStage02ActorCounterPanel(XObjActor3* actor);
}

/**
 * @note Address: 0x80109DD8
 * @note Size: 0x6C
 */
void XObjActor3::vfC8()
{
	UnkObjData* data = mAttr._00;
	s16 type         = data != NULL ? (data != NULL ? data->_16 : 0) : -1; // todo: cleanup, probably an inline
	switch (type) {
	case 14:
		DrawStage02TimerPanel(this);
		DrawStage02ActorCounterPanel(this);
		break;
	}
}

/**
 * @note Address: 0x80109E44
 * @note Size: 0x3C
 */
void XObjActor3::vfAC()
{
	if (!(_B8C.mFlags & 0x10)) {
		DrawWithView(NULL);
	}
}
