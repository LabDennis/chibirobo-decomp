#include "game/cmodule.h"
#include "game/mm.h"

/**
 * @note Address: 0x801A9050
 * @note Size: 0x1EC
 */
void CModule::Print() const
{
#if !defined(VERSION_R24J01)
	if (mModule != NULL) {
		OSReport("----------------------\n");
		OSReport("module=%08X\n", mModule);
		OSReport("bss   =%08X\n", mBss);
		OSReport("\n");
		OSReport("id                %d\n", mModule->info.id);
		OSReport("numSections       %d\n", mModule->info.numSections);
		OSReport("sectionInfoOffset %08xh\n", mModule->info.sectionInfoOffset);
		OSReport("nameOffset        %08xh\n", mModule->info.nameOffset);
		OSReport("nameSize          %d\n", mModule->info.nameSize);
		OSReport("version           %d\n", mModule->info.version);
		OSReport("\n");
		OSReport("bssSize           %d\n", mModule->bssSize);
		OSReport("relOffset         %08xh\n", mModule->relOffset);
		OSReport("impOffset         %08xh\n", mModule->impOffset);
		OSReport("impSize           %08xh\n", mModule->impSize);
		OSReport("prolog            %08xh\n", mModule->prolog);
		OSReport("epilog            %08xh\n", mModule->epilog);
		OSReport("unresolved        %08xh\n", mModule->unresolved);
		if (mModule->info.version >= 2) {
			OSReport("align             %08xh\n", mModule->align);
			OSReport("bssAlign          %08xh\n", mModule->bssAlign);
		}
		if (mModule->info.version >= 3) {
			OSReport("fixSize           %08xh\n", mModule->fixSize);
		}
		OSReport("\n");
	}
#endif
}

/**
 * @note Address: 0x801A923C
 * @note Size: 0x88
 */
void CModule::Unload()
{
	if (IsLoaded()) {
		Epilog();
		OSUnlink(&mModule->info);
		if (mBss != NULL) {
#if defined(VERSION_R24J01)
			mm::Free(mBss);
#else
			mm::Free(mBss, 1);
#endif
		}
		mFile.Close();
		mModule = NULL;
		mBss    = NULL;
	}
}

/**
 * @note Address: 0x801A92C4
 * @note Size: 0x104
 */
BOOL CModule::Load(const char* path)
{
	// no rel on the disc counts as done, FALSE means still reading, call again
	if (mFile.ConvertPathToEntrynum(path) < 0) {
		return TRUE;
	}

	if (mFile.ReadEx(path, 0, 0)) {
		mFile.CloseHandle();

		OSModuleHeader* module = (OSModuleHeader*)mFile.mBuffer;
		void* bss              = NULL;
		if (module->bssSize != 0) {
			bss = mm::Alloc(OSRoundUp32B(module->bssSize), "(bss)");
			OSLink(&module->info, bss);
		} else {
			OSLink(&module->info, NULL);
		}
		mModule = module;
		mBss    = bss;

		if (mModule != NULL) {
			((CModuleProlog)mModule->prolog)(-1, NULL, NULL);
		}
		return TRUE;
	}
	return FALSE;
}
