#ifndef _GAME_CMODULE_H
#define _GAME_CMODULE_H

#include "Dolphin/os.h"
#include "Dolphin/OS/OSModule.h"
#include "game/cfile.h"

// one rel module, read with its own file and linked in place. class name and the
// OSModuleHeader base are from RTTI. Stage holds one at +0x1AC and emits its vtable and dtor

// what a module's _prolog takes, a negative cmd runs its static constructors
typedef s32 (*CModuleProlog)(s32 cmd, void* arg, void* arg2);
typedef void (*CModuleEpilog)(OSModuleHeader* module);

class CModule : public OSModuleHeader {
public:
	CModule()
	    : mModule(NULL)
	    , mBss(NULL)
	{
	}

	// _00 OSModuleHeader
	CFileA mFile;            // _4C
	OSModuleHeader* mModule; // _138, the linked module, null when none is in
	void* mBss;              // _13C
	// _140 vtable

	virtual ~CModule() { } // _08 (weak)

	BOOL IsLoaded() const { return mModule != NULL; }
	void Epilog()
	{
		if (IsLoaded()) {
			((CModuleEpilog)mModule->epilog)(mModule);
		}
	}

	void Print() const;
	void Unload();
	BOOL Load(const char* path);
};
typedef char CModule_size_check[sizeof(CModule) == 0x144 ? 1 : -1];

#endif
