#include "game/cgame.h"
#include "game/xobjmap.h"
#include "game/hsd_dobj.h"
#include "game/cgameutil.h"
#include "game/xchr.h"

extern "C" void FindHighestJointBelowHeightLimit(HSD_JObj* jobj, f32* limits);

static inline XObj* GetShadowSupportObject(XObjChr* player)
{
	if (player->mFloorContacts[3].object) {
		return player->mFloorContacts[3].object;
	}
	return NULL;
}

// an object that is drawn into the shadow, or onto it with the mask
static inline bool IsShadowObj(XObj* obj, u64 mask)
{
	bool on = false;
	if ((obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000) && (obj->mStatus.mFlags & mask)) {
		on = true;
	}
	return on;
}

/**
 * @note Address: 0x8000C450
 * @note Size: 0x6FC
 */
// NONMATCHING: the shadow object and the first loop's registers rotate
// renders the shadow of the first light that casts one, from above the main camera's
// interest. the player's shadow object gets the height of its highest joint under the player
extern "C" void RenderSceneShadow(CGame* game)
{
	XObjChr* player;
	XObj* target;
	s32 i;
	XObj* obj;
	if (game->mStatus.mFlags & 1) {
		player        = gGame.GetPlayer();
		target        = GetShadowSupportObject(player);
		f32 limits[2] = { 1.0f + player->mMoveTranslate.y, NAN };
		for (i = 0; i < game->mDrawLists[1].mCount; i++) {
			obj = game->mDrawLists[1].mObjects[i];
			if (obj->mStatus.mFlags & 2) {
				if (obj == target) {
					limits[1] = NAN;
					if (obj->mJObj != NULL) {
						HSD_JObjWalkTree(obj->mJObj, (HSD_JObjWalkTreeCallback)FindHighestJointBelowHeightLimit, (f32**)limits);
					}
				} else if (obj->mJObj != NULL) {
					HSD_JObjWalkTree(obj->mJObj, JObjClearMObjFlags, (f32**)0x4000000);
				}
			}
		}

		for (CLight* light = game->mLightList; light != NULL; light = (CLight*)light->next) {
			CShadow* shadow = &light->mShadow;
			if (shadow->IsActive()) {
				HSD_CObj* cobj = shadow->mShadow->camera;
				Vec interest   = game->GetCamera()->mInterest;
				Vec eye;
				eye.x = interest.x;
				eye.z = interest.z;
				eye.y = 200.0f + interest.y;
				HSD_CObjSetInterest(cobj, &interest);
				HSD_CObjSetEyePosition(cobj, &eye);
				HSD_ShadowDeleteObject(shadow->mShadow, NULL);
				shadow->InitRect();
				for (s32 i = 0; i < game->mDrawLists[1].mCount; i++) {
					XObj* obj = game->mDrawLists[1].mObjects[i];
					if (IsShadowObj(obj, 4)) {
						obj->RegisterShadowObjects(shadow);
					}
					if ((GetStageInfo()->mFlags & 1) && obj->GetCategory() == 2) {
						obj->ExpandShadowBounds(shadow);
					}
				}
				Vec center = game->GetCamera()->mInterest;
				shadow->AddRect(&center, 100.0f, -100.0f, -100.0f, 100.0f);
				game->mGs.mCamera = shadow->StartRender();
				if (BeginGsCamera(game)) {
					SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
					SetGsChannelEnabled(&gGame.mGs, 0, 0);
					SetGsCullMode(gGame.mGs.mCurRenderMode, 2);
					SetupAll(TRUE);
					for (s32 i = 0; i < game->mDrawLists[1].mCount; i++) {
						XObj* obj = game->mDrawLists[1].mObjects[i];
						if (IsShadowObj(obj, 8)) {
							f32 alpha;
							if (GetStageInfo()->mFlags & 1) {
								alpha = 1.0f;
							} else {
								CCamera* camera = game->GetCamera();
								f32 dz          = camera->mInterest.z - obj->mMoveTranslate.z;
								f32 dx          = camera->mInterest.x - obj->mMoveTranslate.x;
								alpha           = (100.0f - sqrtf(dx * dx + dz * dz)) / 100.0f;
								if (alpha < 0.0f) {
									alpha = 0.0f;
								}
								if (alpha > 0.8f) {
									alpha = 0.8f;
								}
							}
							obj->DrawGroundShadowAt(&obj->mMoveTranslate, sin(3.1415927f * alpha * 0.5f));
						}
					}
					EndGsCamera(&game->mGs);
				}
				shadow->EndRender();
				break;
			}
		}
		InvalidateAll();
	}
}
