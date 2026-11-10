#include "game/hsd_jobj.h"
#include "game/ccameraaction.h"

extern "C" {
#if !defined(VERSION_PAL)
extern const char sMapCameraDebugIndexFormat[] = "DEBUG:CCameraActionMap::DrawDebug3d():%d\n";
#endif
extern const char sTypeNameCamInfo[];
const void* gTypeInfoCamInfo[] = { sTypeNameCamInfo, NULL };
class CamInfo;
CamInfo* __dt__7CamInfoFv(CamInfo*, s32);
struct CamInfoVtable {
	const void* typeInfo;
	s32 offset;
	CamInfo* (*destroy)(CamInfo*, s32);
};
CamInfoVtable __vt__7CamInfo = { gTypeInfoCamInfo, 0, __dt__7CamInfoFv };
}

/**
 * @note Address: 0x801381C0
 * @note Size: 0x70
 */
CCameraActionAreaEx::~CCameraActionAreaEx()
{
}

// 0x80138230
CamAreaSlot::~CamAreaSlot()
{
}

/**
 * @note Address: 0x80138284
 * @note Size: 0x10
 */
CamAreaSlot::CamAreaSlot()
{
}
