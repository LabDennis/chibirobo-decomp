#include "game/cgameutil.h"
#include "game/xobjlight.h"
#include "game/xobjmap.h"

namespace {

const u64 ObjectActive     = 0x1;
const u64 DrawLast         = 0x40;
const u64 UpdateFirst      = 0x4000;
const u64 InScene          = 0x10000;
const u64 DrawLate         = 0xA00000;
const u64 CheckObjectDepth = 0x80000000;
const u64 MirrorOnly       = 0x1000000000000;

const u64 SkipDepthChecks = 0x100;
const u64 CheckSceneDepth = 0x8000000000;

void AddToScene(CGame* game, XObj* obj, s32& sceneCount, s32& mirrorCount)
{
	if (!(obj->mStatus.mFlags & MirrorOnly)) {
		game->mDrawLists[1].mObjects[sceneCount++] = obj;
	}
	game->mDrawLists[2].mObjects[mirrorCount++] = obj;
}

bool IsInScene(const XObj* obj)
{
	return (obj->mStatus.mFlags & (ObjectActive | InScene)) == (ObjectActive | InScene);
}

void AddObjectsByDrawOrder(CGame* game, u64 required, u64 excluded, s32& sceneCount, s32& mirrorCount)
{
	for (s32 i = 0; i < game->mXObjs.mObjectCount; ++i) {
		XObj* obj = game->GetObject(i);
		u64 flags = obj->mStatus.mFlags;
		if (obj->GetCategory() != 3 && IsInScene(obj) && (required == 0 || (flags & required)) && !(flags & excluded)) {
			AddToScene(game, obj, sceneCount, mirrorCount);
		}
	}
}

void AddObjectsByUpdateOrder(CGame* game, const XObj* player, const XObj* camera, bool updateFirst, s32& count)
{
	XObjDrawList& frame = game->mDrawLists[0];
	for (s32 i = 0; i < game->mXObjs.mObjectCount; ++i) {
		XObj* obj = game->GetObject(i);
		if (obj != player && obj != camera && (obj->mStatus.mFlags & ObjectActive)
		    && ((obj->mStatus.mFlags & UpdateFirst) != 0) == updateFirst) {
			frame.mIndices[count]   = i;
			frame.mObjects[count++] = obj;
		}
	}
}

} // namespace

extern "C" void BuildSceneDrawLists(CGame* game)
{
	if (game->mStatus.mFlags & CheckSceneDepth) {
		for (s32 i = 0; i < game->mXObjs.mObjectCount; ++i) {
			XObj* obj = game->GetObject(i);
			if (!IsInScene(obj)) {
				continue;
			}
			if (!(game->mStatus.mFlags & SkipDepthChecks) && (obj->mStatus.mFlags & CheckObjectDepth)
			    && obj->IsOutsideCameraDepth(&game->mMotionControlCamera.mCamera)) {
				obj->mStatus.mFlags |= MirrorOnly;
			} else {
				obj->mStatus.mFlags &= ~MirrorOnly;
			}
		}
	}

	// Maps precede the three draw-priority groups of other objects.
	s32 sceneCount          = 0;
	s32 mirrorCount         = 0;
	const XObjPool& mapPool = game->mXObjs.mPools[3];
	for (s32 i = 0; i < mapPool.mUsed; ++i) {
		XObj* obj = mapPool.Get(i);
		if (IsInScene(obj) && !(obj->mStatus.mFlags & (DrawLate | DrawLast))) {
			AddToScene(game, obj, sceneCount, mirrorCount);
		}
	}
	AddObjectsByDrawOrder(game, 0, DrawLate | DrawLast, sceneCount, mirrorCount);
	AddObjectsByDrawOrder(game, DrawLate, DrawLast, sceneCount, mirrorCount);
	AddObjectsByDrawOrder(game, DrawLast, 0, sceneCount, mirrorCount);
	game->mDrawLists[1].mCount = sceneCount;
	game->mDrawLists[2].mCount = mirrorCount;
}

extern "C" void BuildObjectDrawList(CGame* game)
{
	XObjDrawList& frame = game->mDrawLists[0];
	XObj* player        = game->mXObjs.mPools[2].First();
	XObj* camera        = game->mXObjs.mPools[0].First();
	s32 count           = 0;
	AddObjectsByUpdateOrder(game, player, camera, true, count);
	AddObjectsByUpdateOrder(game, player, camera, false, count);
	// The player and camera run last. They have no draw-list index.
	if (player->mStatus.mFlags & ObjectActive) {
		frame.mObjects[count++] = player;
	}
	if (camera->mStatus.mFlags & ObjectActive) {
		frame.mObjects[count++] = camera;
	}
	frame.mCount = count;
}

extern "C" void SelectSceneLights(CGame* game)
{
	XObjMap* map   = static_cast<XObjMap*>(game->mXObjs.mPools[3].First());
	CLight* lights = (map->mStatus.mFlags & ObjectActive) ? map->GetLights() : NULL;
	if (lights == NULL) {
		XObjLight* sceneLight = static_cast<XObjLight*>(game->mXObjs.mPools[1].First());
		if (sceneLight->mStatus.mFlags & ObjectActive) {
			lights = sceneLight->GetLights();
		}
	}

	game->mLightList = lights != NULL ? lights : game->mLights;
	if (lights == NULL) {
		return;
	}

	for (CLight* light = lights; light != NULL; light = static_cast<CLight*>(light->next)) {
		if (light->mShadow.IsActive()) {
			HSD_CObjSetProjectionType(light->mShadow.mShadow->camera, PROJ_ORTHO);
		}
	}
	for (CLight* light = game->mLights; light != NULL; light = static_cast<CLight*>(light->next)) {
		if (light->mShadow.IsActive()) {
			light->mShadow.SetActive(FALSE);
		}
	}
}
