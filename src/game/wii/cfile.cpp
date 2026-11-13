#if defined(WII_LANGUAGE) || defined(WII_LANGUAGE_SYSTEM)
#include "game/cfile.h"
#include "game/wii/cgame.h"
#include <stdio.h>
#include <string.h>

extern "C" {
extern CFileArchive* lbl_805A28A0;
s32 fn_80190238(const char*);
s32 fn_801FEE48(ARCHandle*, const char*);
s32 fn_802B438C(CFile*, const char*);
void OSReport(const char*, ...);
}

static s32 LocalizedDiscEntry(const char* path, char* localized, size_t capacity)
{
	static const char* const codes[] = { "", "en", "de", "fr", "es", "it" };
	if (!gGame.mLanguage)
		return -1;
	if (*path == '/')
		++path;
	if (strlen(path) + sizeof("/lang/en/") > capacity)
		return -1;
	sprintf(localized, "/lang/%s/%s", codes[gGame.mLanguage], path);
	return fn_80190238(localized);
}

s32 CFile::ConvertPathToEntrynum(const char* path)
{
	char localized[sizeof(mPath)];
	s32 entry = LocalizedDiscEntry(path, localized, sizeof(localized));
	if (entry < 0 && lbl_805A28A0)
		entry = fn_801FEE48(&lbl_805A28A0->mHandle, path);
	return entry < 0 ? fn_80190238(path) : entry;
}

s32 CFile::OpenPath(const char* path)
{
	char localized[sizeof(mPath)];
	if (LocalizedDiscEntry(path, localized, sizeof(localized)) >= 0) {
		strcpy(mPath, localized);
		mInArchive = false;
	} else {
		if (strlen(path) >= sizeof(mPath))
			return -3;
		strcpy(mPath, path);
		mInArchive = lbl_805A28A0 && fn_801FEE48(&lbl_805A28A0->mHandle, mPath) >= 0;
	}
	s32 result = fn_802B438C(this, mPath);
	if (result < 0)
		OSReport("Cannot open %s (error %d)\n", mPath, result);
	return result;
}
#endif
