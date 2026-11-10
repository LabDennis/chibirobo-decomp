#include <string.h>
#include "Version.h"
#include "game/xsound.h"
#include "game/soundtracks.h"
extern "C" void OSReport(const char*, ...);

#include "game/soundbankregister.inl"

/**
 * @note Address: 0x8001E044
 * @note Size: 0xD8
 */
extern "C" BOOL RegisterGameSoundName(XSound* sound, const char* name)
{
	sound->registerName(name, sound->mStageBank);
	return FALSE;
}
