#ifndef GAME_WII_XOBJS_H
#define GAME_WII_XOBJS_H

#include "types.h"
#include "game/xobjpool.h"

class XObj;
class CoordInfo;

extern "C" {
void ResetPooledObjects(XObjPool* pools);
s32 UpdateStageObjectLoading(XOBJS* list);
void ReleaseStageObjectList(XOBJS* list);
XObj* SpawnObjectFromPlacement(XOBJS* list, CoordInfo* info, s32 direct, BOOL async);
s32 FindStageObjectIndex(XOBJS* list, XObj* obj);
}

#endif
