#include "game/cgame.h"
#include "game/cgameutil.h"

extern "C" {
void PSMTXReflect(Mtx m, const Vec* p, const Vec* n);
void PSMTXTranspose(const Mtx src, Mtx xPose);
}

// the floor's reflection transform starts as the identity
static const Vec sFloorReflectionRotation    = { 0.0f, 0.0f, 0.0f };
static const Vec sFloorReflectionTranslation = { 0.0f, 0.0f, 0.0f };
static const Vec sFloorReflectionPoint       = { 0.0f, 0.0f, 0.0f };
static const Vec sFloorReflectionNormal      = { 0.0f, 1.0f, 0.0f };

// the quad that covers the floor afterwards
static Vec sFloorQuad[4] = {
	{ -1000.0f, -0.1f, -1000.0f },
	{ 1000.0f, -0.1f, -1000.0f },
	{ 1000.0f, -0.1f, 1000.0f },
	{ -1000.0f, -0.1f, 1000.0f },
};

/**
 * @note Address: 0x8000DA7C
 * @note Size: 0x4FC
 */
// NONMATCHING: the rodata pool anchor/offsets (retail pools the unit's rodata behind
// sGameVectorDefaults at the section start) and the first light loop's r26/r27 swap
// draws the 0x20000000 objects mirrored in the floor with the lights flipped under it, then
// covers the floor with a quad
extern "C" void RenderFloorReflection(CGame* game)
{
	if ((game->mStatus.mFlags & 1) && (game->mStatus.mFlags & 0x2000000000)) {
		game->mGs.mCamera = game->GetCamera();
		if (BeginGsCamera(game)) {
			for (CLight* light = game->mLightList; light != NULL; light = (CLight*)light->next) {
				light->SetPosition(light->mPosition.x, -light->mPosition.y, light->mPosition.z);
			}
			game->mLightList->SetCurrent(game->GetCamera());

			Vec rot   = sFloorReflectionRotation;
			Vec trans = sFloorReflectionTranslation;
			Mtx mtx;
			Mtx rotX;
			Mtx rotY;
			Mtx rotMtx;
			Mtx transMtx;
			PSMTXRotRad(rotX, 'X', DEG2RAD * rot.x);
			PSMTXRotRad(rotY, 'Y', DEG2RAD * rot.y);
			PSMTXConcat(rotY, rotX, rotMtx);
			PSMTXTrans(transMtx, trans.x, trans.y, trans.z);
			PSMTXConcat(transMtx, rotMtx, mtx);

			Vec point  = sFloorReflectionPoint;
			Vec normal = sFloorReflectionNormal;
			PSMTXMultVec(mtx, &point, &point);
			PSMTXInverse(mtx, mtx);
			PSMTXTranspose(mtx, mtx);
			PSMTXMultVec(mtx, &normal, &normal);
			PSVECNormalize(&normal, &normal);
			Mtx reflect;
			Mtx view;
			PSMTXReflect(reflect, &point, &normal);
			PSMTXConcat(game->GetCamera()->mViewMtx, reflect, view);
			for (s32 i = 0; i < game->mDrawLists[1].mCount; i++) {
				XObj* obj = game->mDrawLists[1].mObjects[i];
				if (!(obj->mStatus.mFlags & 0x200000) && (obj->mStatus.mFlags & 0x20000000)) {
					obj->SetCullMode(1);
					obj->DrawReflection(view);
				}
			}

			for (CLight* light = game->mLightList; light != NULL; light = (CLight*)light->next) {
				light->SetPosition(light->mPosition.x, -light->mPosition.y, light->mPosition.z);
			}
			game->mLightList->SetCurrent(game->GetCamera());
			EndGsCamera(&game->mGs);
			for (s32 i = 0; i < game->mDrawLists[1].mCount; i++) {
				XObj* obj = game->mDrawLists[1].mObjects[i];
				if (!(obj->mStatus.mFlags & 0x200000) && (obj->mStatus.mFlags & 0x20000000)) {
					obj->SetCullMode(2);
				}
			}
		}

		if (game->mStatus.mFlags & 0x20000002000000) {
			game->mGs.mCamera = game->GetCamera();
			if (BeginGsCamera(game)) {
				SetupAll(TRUE);
				SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
				SetGsColorUpdate(gGame.mGs.mCurRenderMode, 0);
				DrawGsVertexQuad(&gGame.mGs, sFloorQuad[0].x, sFloorQuad[0].y, sFloorQuad[0].z, 0, sFloorQuad[1].x, sFloorQuad[1].y, sFloorQuad[1].z, 0,
				                 sFloorQuad[2].x, sFloorQuad[2].y, sFloorQuad[2].z, 0, sFloorQuad[3].x, sFloorQuad[3].y, sFloorQuad[3].z, 0);
				SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
				SetGsColorUpdate(gGame.mGs.mCurRenderMode, 1);
				EndGsCamera(&game->mGs);
			}
		}
	}
}
