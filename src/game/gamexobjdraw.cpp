#include "game/xobj.h"

/**
 * @note Address: 0x8000DA50
 * @note Size: 0x2C
 */
// XObj's DrawReflection, the reflection pass draws it like any other view
void XObj::DrawReflection(Mtx view)
{
	DrawWithView(view);
}
