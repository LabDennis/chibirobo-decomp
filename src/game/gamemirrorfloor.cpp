#include "game/cgame.h"
#include "game/cgameutil.h"

extern "C" {
void PSMTXReflect(Mtx m, const Vec* p, const Vec* n);
void PSMTXTranspose(const Mtx src, Mtx xPose);
}

/**
 * @note Address: 0x8000DA7C
 * @note Size: 0x4FC
 */
// NONMATCHING: the local initializers sit at their offset in the file's rodata pool, and the
// first object loop's registers swap
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

			Vec rot   = { 0.0f, 0.0f, 0.0f };
			Vec trans = { 0.0f, 0.0f, 0.0f };
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

			Vec point  = { 0.0f, 0.0f, 0.0f };
			Vec normal = { 0.0f, 1.0f, 0.0f };
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
				static Vec floor[4] = {
					{ -1000.0f, -0.1f, -1000.0f },
					{ 1000.0f, -0.1f, -1000.0f },
					{ 1000.0f, -0.1f, 1000.0f },
					{ -1000.0f, -0.1f, 1000.0f },
				};
				SetupAll(TRUE);
				SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
				SetGsColorUpdate(gGame.mGs.mCurRenderMode, 0);
				DrawGsVertexQuad(&gGame.mGs, floor[0].x, floor[0].y, floor[0].z, 0, floor[1].x, floor[1].y, floor[1].z, 0, floor[2].x,
				                 floor[2].y, floor[2].z, 0, floor[3].x, floor[3].y, floor[3].z, 0);
				SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
				SetGsColorUpdate(gGame.mGs.mCurRenderMode, 1);
				EndGsCamera(&game->mGs);
			}
		}
	}
}
