#include "game/xobjactor.h"

/**
 * @note Address: 0x8010A500
 * @note Size: 0x74
 */
void XObjActor3::Release()
{
	switch (GetKind()) {
	case 0x149:
	case 0x42B:
		_980.Close();
		break;
	}
	XObj::Release();
}
