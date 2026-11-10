#include "game/xobj.h"

/**
 * @note Address: 0x8000DA50
 * @note Size: 0x2C
 */
// XObj's DrawReflection, the reflection pass draws it like any other view
extern "C" void DrawReflection__4XObjFPA4_f(XObj* object, Mtx view)
{
	object->DrawWithView(view);
}
