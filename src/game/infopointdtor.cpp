#include "Version.h"
#include "game/infopoint.h"

#if defined(VERSION_R24J01)
// the weak dtor copy the GameCube file spells out by hand is a plain out-of-line
// definition here - retail 0x80092244 stores no vtable (compiler elision, empty body)
InfoPoint::~InfoPoint()
{
}
#else
/**
 * @note Address: 0x8006DB80
 * @note Size: 0x48
 */
// todo: weak copy
extern "C" InfoPoint* __dt__9InfoPointFv(InfoPoint* point, s16 flags)
{
	if (point != NULL) {
		point->InfoPoint::~InfoPoint();
		if (flags > 0)
			::operator delete(point);
	}
	return point;
}
#endif
