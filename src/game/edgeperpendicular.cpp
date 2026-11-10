#include "game/hsd_jobj.h"
#include "game/ut.h"
#include "game/edgeperpendicular.h"

/**
 * @note Address: 0x8018E824
 * @note Size: 0x80
 */
// the direction from the line bc out toward a, square to bc
extern "C" void GetOutwardPerpendicularToEdge(Vec* a, Vec* b, Vec* c, Vec* out)
{
	Vec bc, ba, n;
	CalculateOutwardPerpendicular(a, b, c, out, bc, ba, n);
}
