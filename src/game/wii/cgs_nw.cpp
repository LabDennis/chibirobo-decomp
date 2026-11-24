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
	scene->_04 = NULL;
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
	if (head[1] == NULL || ((nw4r::g3d::AnmScn*)head[1])->GetNumFog() == 0) {
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

// NONMATCHING: the calls and projection cases match. Retail uses a 0x100 frame; the remaining PostureInfo temporary
// layout and ordering differ, and the named viewport bounds raise this frame to 0x110 while matching the f30/f31 lifetime.
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
		nwCamera.SetPosition(camera->mView.mEye);
		nw4r::g3d::Camera::PostureInfo posture = { nw4r::g3d::Camera::POSTURE_LOOKAT };
		posture.cameraUp                       = camera->mView.mUp;
		posture.cameraTarget                   = camera->mView.mInterest;
		posture.cameraRotate                   = nw4r::math::VEC3(0.0f, 0.0f, 0.0f);
		posture.cameraTwist                    = 0.0f;
		nwCamera.SetPosture(posture);
	}
	f32 viewportNear = camera->mViewport[4];
	f32 viewportFar  = camera->mViewport[5];
	nwCamera.SetViewport(camera->mViewport[0], camera->mViewport[1], camera->mViewport[2], camera->mViewport[3]);
	nwCamera.SetViewportZRange(viewportNear, viewportFar);
	nwCamera.GXSetViewport();
	nwCamera.GXSetProjection();
	nwCamera.GXSetScissor();
	nwCamera.GXSetScissorBoxOffset();
}

extern "C" nw4r::g3d::LightSet fn_802AF8B0(WiiGsScene* scene, int index);

// NONMATCHING: all calls and control flow match, but retail uses a 0x210 frame and saves r24; this source uses 0x1d0
// and r23, with different temporary spill ordering around the light position, direction and attenuation calls.
extern "C" CLight* fn_802AF1D0(WiiGsScene* scene, CLight* light, MtxP view, BOOL flag)
{
	if (scene->_04 != NULL && flag) {
		nw4r::g3d::LightSet set = fn_802AF8B0(scene, 0);
		if (set.IsValid()) {
			nw4r::g3d::AmbLightObj* ambient = set.GetAmbLightObj();
			CLight* current                 = &scene->mLights[0];
			if (ambient != NULL) {
				current->InitAmbient(ambient->r, ambient->g, ambient->b, ambient->a);
			} else {
				current->InitAmbient(0, 0, 0, 0xFF);
			}
			current->vf2C();

			u32 numLights = 0;
			for (u32 i = 0; i < 8; i++) {
				nw4r::g3d::LightObj* lightObj = set.GetLightObj(i);
				if (lightObj != NULL && lightObj->IsEnable() && !lightObj->IsSpecularLight()) {
					GXColor color;
					lightObj->GetLightColor(&color);
					WiiGsSlot* slot = &scene->mSlots[i + 1];
					if (slot->mColorSet) {
						color = slot->mColor;
						lightObj->InitLightColor(color);
					}
					current->SetColor(color.r, color.g, color.b, color.a);

					nw4r::math::VEC3 pos;
					lightObj->GetLightPos(&pos);
					if (slot->mPosSet) {
						pos = slot->mPos;
						lightObj->InitLightPos(pos.x, pos.y, pos.z);
					}
					GXInitLightPos(current, pos.x, pos.y, pos.z);

					nw4r::math::VEC3 dir;
					lightObj->GetLightDir(&dir);
					if (slot->_18) {
						dir = slot->mDir;
						lightObj->InitLightDir(dir.x, dir.y, dir.z);
					}
					GXInitLightDir(current, dir.x, dir.y, dir.z);

					f32 a0, a1, a2;
					f32 k0, k1, k2;
					lightObj->GetLightAttnA(&a0, &a1, &a2);
					lightObj->GetLightAttnK(&k0, &k1, &k2);
					GXInitLightAttn(current, a0, a1, a2, k0, k1, k2);
					current->vf2C();

					nw4r::g3d::LightObj viewLight = *lightObj;
					Vec viewPos                   = current->GetViewPosition(view);
					Vec viewDir                   = current->GetViewDirection(view);
					viewLight.InitLightPos(viewPos.x, viewPos.y, viewPos.z);
					viewLight.InitLightDir(viewDir.x, viewDir.y, viewDir.z);
					GXLoadLightObjImm(viewLight, (GXLightID)(1 << numLights));
					numLights++;
					current++;
				}
			}
			for (u32 i = 0; i < numLights; i++) {
				scene->mLights[i].mNext = &scene->mLights[i + 1];
			}
			scene->mLights[numLights].mNext = NULL;
		}
		return &scene->mLights[0];
	}

	nw4r::g3d::LightSet set = fn_802AF8B0(scene, 0);
	u32 numLights           = 0;
	u32 lightIndex          = 0;
	for (CLight* current = light; current != NULL; current = current->mNext) {
		if (!(current->mFlags & 0x8000)) {
			continue;
		}
		GXColor color = current->GetColor();
		if ((u8)current->mFlags == 1) {
			set.SelectAmbLightObj(0);
			nw4r::g3d::AmbLightObj* ambient = set.GetAmbLightObj();
			ambient->r                      = color.r;
			ambient->g                      = color.g;
			ambient->b                      = color.b;
			ambient->a                      = color.a;
			continue;
		}

		set.SelectLightObj(numLights, lightIndex);
		nw4r::g3d::LightObj* lightObj = set.GetLightObj(numLights);
		lightObj->InitLightColor(color);
		CVec pos = current->GetPosition();
		lightObj->InitLightPos(pos.x, pos.y, pos.z);
		CVec dir = current->mInterest;
		PSVECNormalize(&dir, &dir);
		lightObj->InitLightDir(dir.x, dir.y, dir.z);
		f32 a0, a1, a2;
		f32 k0, k1, k2;
		GXGetLightAttnA(current, &a0, &a1, &a2);
		lightObj->InitLightAttnA(a0, a1, a2);
		GXGetLightAttnK(current, &k0, &k1, &k2);
		lightObj->InitLightAttnK(k0, k1, k2);
		lightObj->Enable();

		nw4r::g3d::LightObj viewLight = *lightObj;
		Vec viewPos                   = current->GetViewPosition(view);
		Vec viewDir                   = current->GetViewDirection(view);
		viewLight.InitLightPos(viewPos.x, viewPos.y, viewPos.z);
		viewLight.InitLightDir(viewDir.x, viewDir.y, viewDir.z);
		GXLoadLightObjImm(viewLight, (GXLightID)(1 << numLights));
		numLights++;
		lightIndex++;
	}
	for (u32 i = numLights; i < 8; i++) {
		set.SelectLightObj(numLights, lightIndex);
		set.GetLightObj(numLights)->Disable();
		numLights++;
		lightIndex++;
	}
	return light;
}

extern "C" nw4r::g3d::LightSet fn_802AF8B0(WiiGsScene* scene, int index)
{
	return ((nw4r::g3d::ScnRoot*)scene->mHead)->GetLightSet(index);
}
