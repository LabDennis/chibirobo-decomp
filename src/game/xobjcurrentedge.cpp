#include "BuildSettings.h"
#include "game/hsd_jobj.h"
#include "game/xobj.h"

extern "C" const Vec* GetCurrentEdgeWorldOffsetPosition(const XObj* object)
{
	const Vec edge = object->mEdges[object->mSelectedFloorIndex].mContactOffset;
#ifdef BUGFIX
	static Vec position;
	position = object->mMoveTranslate + edge;
	return &position;
#else
	// Retail returns a pointer into its expired stack frame. The player immediately copies all three coordinates.
	return &static_cast<const Vec&>(object->mMoveTranslate + edge);
#endif
}
