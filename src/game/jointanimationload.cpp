#include "game/cgame.h"
#include "game/cobj.h"

extern "C" CObjArchive* FindFreeObjectArchive(CObjArchive* archives);

/**
 * @note Address: 0x8008A360
 * @note Size: 0x2C
 */
// CJAnim::Release
extern "C" void Release__6CJAnimFv(CJAnim* anim)
{
	if (anim->mArchive != NULL)
		anim->CJArchive::Release();
}

/**
 * @note Address: 0x8008A38C
 * @note Size: 0x170
 */
// starts the read, then sets it up once it's in, TRUE when done
extern "C" BOOL LoadJointAnimationAsync(CJAnim* anim, const char* path)
{
	if (anim->mLoading == 0) {
		anim->Release();
		if (path == NULL || *path == '\0')
			return TRUE;
		CFile file;
		if (file.ConvertPathToEntrynum(path) < 0)
			return TRUE;
		CObjArchive* storage = FindFreeObjectArchive(gGame.mObjArchives.mArchives);
		if (storage != NULL) {
			anim->ReadAsync(storage, path, 0, 0, 0);
			anim->mLoading++;
		} else {
			return TRUE;
		}
	} else {
		if (anim->ReadCheck()) {
			anim->SetupScene(0, 0);
			anim->mLoading = 0;
		}
	}
	return anim->mLoading == 0;
}

/**
 * @note Address: 0x8008A4FC
 * @note Size: 0x34
 */
// CJAnim::Clear
extern "C" void Clear__6CJAnimFv(CJAnim* anim)
{
	anim->CJArchive::Clear();
	anim->mLoading = 0;
}
