#include "game/xobjattachment.h"

// XObjPlug::Release
/**
 * @note Address: 0x800D93C8
 * @note Size: 0x4C
 */
void XObjPlug::Release()
{
	mWire.Release();
	XObj::Release();
	mLoadState = 0;
}
