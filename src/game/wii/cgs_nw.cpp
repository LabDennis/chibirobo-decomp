#include "nw4r/g3d/g3d_camera.h"
#include "nw4r/g3d/g3d_fog.h"
#include "nw4r/g3d/g3d_init.h"
#include "game/wii/cgs.h"
#include "game/wii/cgame.h"
#include "game/mm.h"

extern MEMAllocator gWiiPadAllocator;

namespace nw4r {
namespace g3d {
namespace G3DState {
void Invalidate(u32 flag);
}
} // namespace g3d
} // namespace nw4r

#define NW4R_ASSERT_VALID_ROOT(exp)                                                                                                        \
	((void)((((u32)(exp) & 0xFF000000) == 0x80000000 || ((u32)(exp) & 0xFF800000) == 0x81000000 || ((u32)(exp) & 0xF8000000) == 0x90000000 \
	         || ((u32)(exp) & 0xFF000000) == 0xC0000000 || ((u32)(exp) & 0xFF800000) == 0xC1000000                                         \
	         || ((u32)(exp) & 0xF8000000) == 0xD0000000 || ((u32)(exp) & 0xFFFFC000) == 0xE0000000)                                        \
	        || (nw4r::db::Panic("cgs_nw.cpp", 0x1A, "NW4R:Pointer Error\nm_pRoot(=%p) is not valid pointer.", (exp)), 0)))

extern "C" void fn_802AEBF0(WiiGsScene* scene, u32 renderStates, u32 tevStages, u32 channels, u32 texGens)
{
	nw4r::g3d::ScnRoot* m_pRoot = nw4r::g3d::ScnRoot::Construct(&gWiiPadAllocator, NULL, renderStates, tevStages, channels, texGens);
	scene->mHead                = (WiiGsSceneObj*)m_pRoot;
	NW4R_ASSERT_VALID_ROOT(m_pRoot);
	scene->_04 = FALSE;
	for (int i = 0; i < 9; i++) {
		scene->mSlots[i]._18       = FALSE;
		scene->mSlots[i].mPosSet   = FALSE;
		scene->mSlots[i].mColorSet = FALSE;
	}
}

extern "C" void fn_802AEDA0(WiiGsSceneObj** head, u32)
{
	nw4r::g3d::G3dReset();
	((nw4r::g3d::ScnRoot*)*head)->UpdateFrame();
	((nw4r::g3d::ScnRoot*)*head)->CalcVtx();
	((nw4r::g3d::ScnRoot*)*head)->CalcWorld();
	((nw4r::g3d::ScnRoot*)*head)->CalcMaterial();
}

extern "C" void fn_802AEDEC(WiiGsSceneObj** head, Mtx)
{
	((nw4r::g3d::ScnRoot*)*head)->CalcView();
	((nw4r::g3d::ScnRoot*)*head)->GatherDrawScnObj();
	((nw4r::g3d::ScnRoot*)*head)->ZSort();
	nw4r::g3d::G3DState::Invalidate(0x7FF);
	((nw4r::g3d::ScnRoot*)*head)->DrawOpa();
	((nw4r::g3d::ScnRoot*)*head)->DrawXlu();
}

extern "C" void fn_802AEE44(WiiGsSceneObj** head, XObj* obj)
{
	if (obj != NULL) {
		head[1] = obj->mObject.m_resFile.HasResAnmScn() ? (WiiGsSceneObj*)obj->mObject.m_resources.scn : NULL;
	} else {
		head[1] = NULL;
	}
	if (head[1] != NULL && ((nw4r::g3d::AnmScn*)head[1])->GetNumFog() == 0) {
		nw4r::g3d::Fog fog = ((nw4r::g3d::ScnRoot*)*head)->GetFog(0);
		if (!fog.IsValid()) {
			nw4r::db::Panic("g3d_fog.h", 0x41, "NW4R:Failed assertion IsValid()");
		}
		fog.SetFogType((GXFogType)0);
	}
	if (head[1] != NULL) {
		((nw4r::g3d::ScnRoot*)*head)->SetAnmScn((nw4r::g3d::AnmScn*)head[1]);
	} else {
		((nw4r::g3d::ScnRoot*)*head)->RemoveAnmScn();
	}
}

extern "C" void fn_802AEF34(WiiGsSceneObj** head, CCamera* camera, BOOL flag)
{
	nw4r::g3d::Camera nwCamera = ((nw4r::g3d::ScnRoot*)*head)->GetCamera(flag);
	((nw4r::g3d::ScnRoot*)*head)->SetCurrentCamera(flag);
	if (camera->GetProjectionSource() != NULL) {
		nw4r::math::MTX44 projection;
		nw4r::math::MTX44Copy(&projection, (const nw4r::math::MTX44*)camera->GetProjectionSource());
		nwCamera.SetProjectionMtxDirectly(&projection);
	} else {
		switch (camera->mType) {
		case 0:
			nwCamera.SetPerspective(camera->mFov, camera->mAspect, camera->mNear, camera->mFar);
			break;
		case 1:
			nwCamera.SetFrustum(camera->mTop, camera->mBottom, camera->mLeft, camera->mRight, camera->mNear, camera->mFar);
			break;
		case 2:
			nwCamera.SetOrtho(camera->mTop, camera->mBottom, camera->mLeft, camera->mRight, camera->mNear, camera->mFar);
			break;
		}
	}
	if (camera->GetViewSource() != NULL) {
		nw4r::math::MTX34 view;
		PSMTXCopy(camera->GetViewSource(), view);
		nwCamera.SetCameraMtxDirectly(view);
	} else {
		nw4r::g3d::Camera::PostureInfo posture = { nw4r::g3d::Camera::POSTURE_LOOKAT };
		nwCamera.SetPosition(camera->mView.mEye);
		posture.cameraUp     = camera->mView.mUp;
		posture.cameraTarget = camera->mView.mInterest;
		posture.cameraRotate = nw4r::math::VEC3(0.0f, 0.0f, 0.0f);
		posture.cameraTwist  = 0.0f;
		nwCamera.SetPosture(posture);
	}
	nwCamera.SetViewport(camera->mViewport[0], camera->mViewport[1], camera->mViewport[2], camera->mViewport[3]);
	nwCamera.SetViewportZRange(camera->mViewport[4], camera->mViewport[5]);
	nwCamera.GXSetViewport();
	nwCamera.GXSetProjection();
	nwCamera.GXSetScissor();
	nwCamera.GXSetScissorBoxOffset();
}
