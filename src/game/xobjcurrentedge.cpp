#include "game/hsd_jobj.h"
#include "game/xobj.h"

extern "C" const Vec* GetCurrentEdgeWorldOffsetPosition(const XObj* object)
{
	const Vec edge = object->mEdges[object->mSelectedFloorIndex].mContactOffset;
	static Vec position;
	position = object->mMoveTranslate + edge;
	return &position;
}
