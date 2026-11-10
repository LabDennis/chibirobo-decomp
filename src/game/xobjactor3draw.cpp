#include "game/xobjactor.h"

extern "C" {
void DrawStage02TimerPanel(XObjActor3* actor);
void DrawStage02ActorCounterPanel(XObjActor3* actor);
}

/**
 * @note Address: 0x80109DD8
 * @note Size: 0x6C
 */
extern "C" void vfC8__10XObjActor3Fv(XObjActor3* actor)
{
	UnkObjData* data = actor->mAttr._00;
	s16 type         = data != NULL ? (data != NULL ? data->_16 : 0) : -1; // todo: cleanup, probably an inline
	switch (type) {
	case 14:
		DrawStage02TimerPanel(actor);
		DrawStage02ActorCounterPanel(actor);
		break;
	}
}

/**
 * @note Address: 0x80109E44
 * @note Size: 0x3C
 */
extern "C" void vfAC__10XObjActor3Fv(XObjActor3* actor)
{
	if (!(actor->_B8C.mFlags & 0x10)) {
		actor->DrawWithView(NULL);
	}
}
