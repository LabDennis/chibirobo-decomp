#include "game/wii/saveslots.h"
#include "game/wii/cgame.h"
#include "game/globals.h"
#include "stl/mem.h"

extern "C" {
extern void* gWiiSaveNandWorkBuffer;
extern const char* gWiiSaveBannerPath;
}

extern "C" int CheckWiiSaveBanner(WiiSaveManager* manager, bool removeInvalid)
{
	WiiGameSaveFile* file = &manager->mFile;
	if (OpenNandGameFile(file, gWiiSaveBannerPath, 1, gWiiSaveNandWorkBuffer, 0x4000) != 0)
		return 0;
	u32 length;
	bool valid = GetNandGameFileLength(file, &length) == 0 && length == sizeof(manager->mBanner);
	if (CloseNandGameFile(file) != 0)
		valid = false;
	if (!valid && removeInvalid)
		DeleteNandGameFile(file, gWiiSaveBannerPath);
	return valid;
}

extern "C" void BuildWiiSaveBanner(WiiSaveManager* manager)
{
	NANDBanner& banner  = manager->mBanner;
	const u16* subtitle = reinterpret_cast<const u16*>(GetLocalizedInterfaceText(2));
	const u16* title    = reinterpret_cast<const u16*>(GetLocalizedInterfaceText(1));
	NANDInitBanner(&banner, 0, title, subtitle);
	CTpl& textures = gGame.mTpl;
	textures.mTex.Init(&textures.mPalette->descriptorArray[247], 0);
	memcpy(banner.bannerTexture, textures.mTex.mImageData, sizeof(banner.bannerTexture));
	for (int i = 0; i < ARRAY_SIZE(banner.iconTexture); ++i) {
		textures.mTex.Init(&textures.mPalette->descriptorArray[248 + i], 0);
		memcpy(banner.iconTexture[i], textures.mTex.mImageData, sizeof(banner.iconTexture[i]));
		banner.iconSpeed = (banner.iconSpeed & ~(3 << (2 * i))) | (1 << (2 * i));
	}
}

extern "C" s32 WriteWiiSaveBanner(WiiSaveManager* manager)
{
	BuildWiiSaveBanner(manager);
	WiiGameSaveFile* file = &manager->mFile;
	for (int attempt = 0; attempt < 3; ++attempt) {
		s32 result = CreateNandGameFile(file, gWiiSaveBannerPath, 0x30, 0);
		if (result == 0)
			break;
		if (result != NAND_RESULT_EXISTS)
			return result;
		result = DeleteNandGameFile(file, gWiiSaveBannerPath);
		if (result != 0)
			return result;
	}
	s32 result = OpenNandGameFile(file, gWiiSaveBannerPath, 2, gWiiSaveNandWorkBuffer, 0x4000);
	if (result != 0)
		return result;
	result = WriteNandGameFile(file, &manager->mBanner, sizeof(manager->mBanner));
	if (result < 0)
		return result;
	return CloseNandGameFile(file);
}
