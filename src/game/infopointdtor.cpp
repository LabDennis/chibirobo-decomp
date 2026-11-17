#include "Version.h"
#include "game/infopoint.h"

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
