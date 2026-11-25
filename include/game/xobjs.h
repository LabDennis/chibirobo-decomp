#ifndef _GAME_XOBJS_H
#define _GAME_XOBJS_H

#include "game/xobj.h"

class XOBJS;
extern "C" BOOL QueueObjectAnimationRead(XObj* obj, char* path);
extern "C" void InitializeObjectPools(XOBJS* list);
// index of obj in the list, -1 when it isn't there
extern "C" s32 FindStageObjectIndex(XOBJS* list, CXObj* obj);
extern "C" XObj* SpawnObjectFromPlacement(XOBJS* list, CoordInfo* info, s32 unused, BOOL async);

#endif
