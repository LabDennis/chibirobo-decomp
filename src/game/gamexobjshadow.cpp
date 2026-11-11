#include "game/cgame.h"
#include "game/xobjmap.h"
#include "game/hsd_dobj.h"
#include "game/cgameutil.h"
#include "game/xchr.h"
#include "game/texturecopy.h"
#include "game/fbtexeff.h"
#include "game/cshadow.h"

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

/**
 * @note Address: 0x8000CB4C
 * @note Size: 0x74
 */
void XObj::ExpandShadowBounds(CShadow* shadow)
{
	f32 size = GetShadowRadius();
	Vec pos  = mMoveTranslate;
	shadow->AddRect(&pos, size, -size, -size, size);
}

/**
 * @note Address: 0x8000CBC0
 * @note Size: 0x8
 */
f32 XObj::GetShadowRadius()
{
	return 40.0f;
}

/**
 * @note Address: 0x8000CBC8
 * @note Size: 0x2C
 */
void XObj::RegisterShadowObjects(CShadow* shadow)
{
	HSD_ShadowAddObject(shadow->mShadow, (HSD_Obj*)mJObj);
}

/**
 * @note Address: 0x8000CBF4
 * @note Size: 0x1A8
 */
// a joint walk: finds the highest joint at or under the limit in limits[0] and keeps it in
// limits[1], nan to start, and marks the materials of those joints
extern "C" void FindHighestJointBelowHeightLimit(HSD_JObj* jobj, f32* limits)
{
	Vec pos;
	HSD_MtxGetTranslate(HSD_JObjGetMtxPtr(jobj), &pos);
	BOOL under = pos.y <= limits[0];
	if (under) {
		if (fpclassify(limits[1]) == FP_NAN) {
			limits[1] = pos.y;
		} else if (limits[1] < pos.y) {
			limits[1] = pos.y;
		}
	}
	for (HSD_DObj* dobj = HSD_JObjGetDObj(jobj); dobj != NULL; dobj = HSD_DObjGetNext(dobj)) {
		HSD_MObj* mobj = HSD_DObjGetMObj(dobj);
		if (mobj != NULL) {
			if (under) {
				HSD_MObjSetFlags(mobj, 0x4000000);
			} else {
				HSD_MObjClearFlags(mobj, 0x4000000);
			}
		}
	}
}

/**
 * @note Address: 0x8000CD9C
 * @note Size: 0x4
 */
void XObj::DrawObjectEffects()
{
}

/**
 * @note Address: 0x8000CDA0
 * @note Size: 0x28C
 */
// CGame::vf20, draws the objects in _C074 from the main camera, kind 3 ones first
extern "C" void vf20__5CGameFv(CGame* game)
{
	if (game->mStatus.mFlags & 1) {
		s32 i;
		game->mGs.mCamera = game->GetCamera();
		if (BeginGsCamera(game)) {
			for (i = 0; i < game->mDrawLists[1].mCount; i++) {
				if (game->mDrawLists[1].mObjects[i]->GetCategory() == 3) {
					game->mDrawLists[1].mObjects[i]->vfAC();
				}
			}
			EndGsCamera(&game->mGs);
		}
		game->mGs.mCamera = game->GetCamera();
		if (BeginGsCamera(game)) {
			for (i = 0; i < game->mDrawLists[1].mCount; i++) {
				if (game->mDrawLists[1].mObjects[i]->GetCategory() != 3) {
					game->mDrawLists[1].mObjects[i]->vfAC();
				}
			}
			CallStageModule(2, 0, NULL);
			EndGsCamera(&game->mGs);
		}
		InvalidateAll();
	}
}

/**
 * @note Address: 0x8000D02C
 * @note Size: 0x30
 */
void XObj::vfAC()
{
	DrawWithView(NULL);
}

/**
 * @note Address: 0x8000D05C
 * @note Size: 0x2C
 */
CCamera* CBase::GetCamera()
{
	return vf30();
}

/**
 * @note Address: 0x8000D088
 * @note Size: 0x88
 */
f32 XObj::GetCollisionHeight()
{
	if (fpclassify(mMaxFloorHeight) == FP_NAN) {
		return 0.0f;
	}
	return mMaxFloorHeight - mMoveTranslate.y;
}

/**
 * @note Address: 0x8000D110
 * @note Size: 0x8
 */
f32 XObj::GetFloorXZExtent()
{
	return mFloorXZExtent;
}

/**
 * @note Address: 0x8000D118
 * @note Size: 0x5EC
 */
// NONMATCHING: the second loop's registers rotate
// clears the screen's alpha from the second camera, draws the 0x40 objects and the two the
// player is busy with into it, then sets the 0x40 objects back up for the main pass
extern "C" void DrawObjectAlphaMask(CGame* game)
{
	XObj* obj;
	s32 i;
	XObjChr* player;
	if ((game->mStatus.mFlags & 1) && (game->mStatus.mFlags & 0x4000000000)) {
		game->mGs.mCamera = game->vf34();
		if (BeginGsCamera(game)) {
			InvalidateAll();
			SetGsChannelEnabled(&gGame.mGs, 4, 0);
			SetGsBlendMode(gGame.mGs.mCurRenderMode, 0, 4, 5, 15);
			SetGsColorUpdate(gGame.mGs.mCurRenderMode, 0);
			SetGsAlphaUpdate(gGame.mGs.mCurRenderMode, 1);
			SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 7, 0);
			SetGsDestinationAlpha(gGame.mGs.mCurRenderMode, 0, 0);
			SetupAll(TRUE);
			Vec quad[4] = { { 0.0f, 0.0f, 0.0f }, { 640.0f, 0.0f, 0.0f }, { 640.0f, 448.0f, 0.0f }, { 0.0f, 448.0f, 0.0f } };
			DrawGsVertexQuad(&gGame.mGs, quad[0].x, quad[0].y, quad[0].z, 4, quad[1].x, quad[1].y, quad[1].z, 4, quad[2].x, quad[2].y,
			                 quad[2].z, 4, quad[3].x, quad[3].y, quad[3].z, 4);
			SetGsColorUpdate(gGame.mGs.mCurRenderMode, 1);
			InvalidateAll();
			EndGsCamera(&game->mGs);
		}

		game->mGs.mCamera = game->GetCamera();
		if (BeginGsCamera(game)) {
			XObj* targets[2] = { NULL, NULL };
			player           = (XObjChr*)game->mXObjs.mPools[2].First();
			if (targets[0] == NULL) {
				targets[0] = player->mLift.mPlugTarget;
			}
			if (targets[1] == NULL) {
				targets[1] = player->mLift.mTarget;
			}
			if (targets[1] == NULL) {
				targets[1] = player->mPush.mTarget;
			}
			if (targets[1] == NULL) {
				targets[1] = player->mLadder.mTarget;
			}
			if (targets[1] == NULL) {
				targets[1] = player->mRope.mTarget;
			}
			for (i = 0; i < game->mDrawLists[1].mCount; i++) {
				obj = game->mDrawLists[1].mObjects[i];
				if (obj->mStatus.mFlags & 0x40) {
					obj->vfDC(1, 0);
					obj->vfAC();
				} else if ((targets[0] == obj || targets[1] == obj) && !(obj->mStatus.mFlags & 0x800000)) {
					obj->vfAC();
				}
			}
			EndGsCamera(&game->mGs);
		}

		for (i = 0; i < game->mDrawLists[1].mCount; i++) {
			obj = game->mDrawLists[1].mObjects[i];
			if (obj->mStatus.mFlags & 0x40) {
				if (obj->mStatus.mFlags & 0x800000) {
					obj->vfDC(1, 8);
				} else {
					obj->vfDC(1, 4);
				}
			}
		}
	}
}

// copies what was drawn into the effect's texture
static inline void CopyToTexture(FbMirrorEx* effect)
{
	SetGsColorUpdate(gGame.mGs.mCurRenderMode, 1);
	SetGsAlphaUpdate(gGame.mGs.mCurRenderMode, 1);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 7, 1);
	SetupAll(TRUE);
	if (effect->mTexture.mImage != NULL) {
		u16 width, height;
		if (effect->mTexture.mMipmap) {
			width  = effect->mTexture.mWidth << 1;
			height = effect->mTexture.mHeight << 1;
		} else {
			width  = effect->mTexture.mWidth;
			height = effect->mTexture.mHeight;
		}
		GXSetTexCopySrc(0, 0, width, height);
		GXSetTexCopyDst(effect->mTexture.mWidth, effect->mTexture.mHeight, effect->mTexture.mFormat, effect->mTexture.mMipmap);
		GXCopyTex(effect->mTexture.mImage, GX_TRUE);
		GXPixModeSync();
		InvalidateAll();
	}
}

/**
 * @note Address: 0x8000D704
 * @note Size: 0x34C
 */
// NONMATCHING: the mirror, its view and the object list pointer rotate registers
// draws what each mirror shows into its texture
extern "C" void RenderMirrorTextures(CGame* game)
{
	MtxPtr view;
	FbMirrorEx* mirror;
	s32 i;
	s32 j;
	XObj* obj;
	if ((game->mStatus.mFlags & 1) && (game->mStatus.mFlags & 0x100000000000)) {
		for (i = 0; i < 5; i++) {
			mirror = &gFramebufferMirrors[i];
			if (mirror->mJoint != NULL && mirror->mVisible && !IsMirrorOff(mirror)) {
				game->mGs.mCamera = &mirror->mCamera;
				if (BeginGsCamera(game)) {
					view = mirror->mView;
					for (j = 0; j < game->mDrawLists[2].mCount; j++) {
						obj = game->mDrawLists[2].mObjects[j];
						if (!(obj->mStatus.mFlags & 0x200000) && (obj->mStatus.mFlags & 0x200000000000)) {
							obj->DrawReflection(view);
						}
					}
					EndGsCamera(&game->mGs);
					if (mirror->mVisible) {
						CopyToTexture(mirror);
					}
				}
			}
		}
	}
}
