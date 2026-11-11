#include "game/cgame.h"
#include "game/arcache.h"
#include "game/cthread.h"
#include "PowerPC_EABI_Support/Runtime/MWCPlusLib.h"
#include "PowerPC_EABI_Support/Runtime/global_destructor_chain.h"

// The game destructor node follows this storage, at 0x134 in JP and 0x140 in US/PAL.
extern "C" {
extern CGame* __ct__5CGameFv(CGame*);
extern CGame* __dt__5CGameFv(CGame*, s32);
extern void* __vt__11ARCacheInfo[];
extern void* __vt__10ARPreCache[];
extern void* __vt__5CFile[];
extern void* __vt__7CThread[];
extern FbMirrorEx* __ct__10FbMirrorExFv(FbMirrorEx*);
extern FbMirrorEx* __dt__10FbMirrorExFv(FbMirrorEx*, s32);
extern void DestroyFbMirrorExArray();
extern ARCacheInfo* __dt__11ARCacheInfoFv(ARCacheInfo*, s32);
extern ARPreCache* __dt__10ARPreCacheFv(ARPreCache*, s32);
extern CThread* __dt__7CThreadFv(CThread*, s32);
u8 lbl_8025C200[0xF8];
Vec gJointTranslateZero;
Vec gJointScaleZero;
Vec gJointRotationZero;
#if defined(VERSION_JP)
u8 sJointPoolRemainder[0x18];
#else
u8 sJointPoolRemainder[0x24];
#endif
}

CGame gGame __attribute__((aligned(256)));

// The cache destructor node follows these 0x190 bytes.
#if defined(VERSION_JP)
u8 lbl_80339470[0x190];
#elif defined(VERSION_PAL)
u8 lbl_80362470[0x190];
#else
u8 lbl_8035B870[0x190];
#endif
ARCacheInfo gARAMCacheInfo;
FbMirrorEx gFramebufferMirrors[5];
ARPreCache gAramPrecache;
CThread gMovieUpdateThread;
u8 gMovieUpdateThreadStack[0x1000];

/**
 * @note Address: 0x8001933C
 * @note Size: 0x144
 */
extern "C" void __sinit_cgameinit_cpp(void)
{
	u8* storage = lbl_8025C200;
	CGame* game = __ct__5CGameFv(&gGame);
#if defined(VERSION_JP)
	__register_global_object(game, (void*)__dt__5CGameFv, storage + 0x134);
#else
	__register_global_object(game, (void*)__dt__5CGameFv, storage + 0x140);
#endif

	ARCacheInfo* info              = &gARAMCacheInfo;
	info->mBaseAddress             = 0;
	*(void***)((u8*)info + 0x4008) = __vt__11ARCacheInfo;
	info->mCount                   = 0;
	__register_global_object(info, (void*)__dt__11ARCacheInfoFv, (u8*)info - 0xC);

	__construct_array(gFramebufferMirrors, (ConstructorDestructor)__ct__10FbMirrorExFv, (ConstructorDestructor)__dt__10FbMirrorExFv,
	                  sizeof(FbMirrorEx), 5);
	__register_global_object(NULL, (void*)DestroyFbMirrorExArray, (u8*)gFramebufferMirrors - 0xC);

	ARPreCache* cache                      = &gAramPrecache;
	*(void***)((u8*)cache + 0x1E4)         = __vt__10ARPreCache;
	*(void***)((u8*)&cache->mFile0 + 0xE8) = __vt__5CFile;
	cache->mFile0.mArchive                 = NULL;
	cache->mFile0.mEntrynum                = -1;
	cache->mFile0.mBuffer                  = NULL;
	cache->mFile0.mResult                  = 0;
	cache->mFile0.mState                   = 1;
	*(void***)((u8*)&cache->mFile1 + 0xE8) = __vt__5CFile;
	cache->mFile1.mArchive                 = NULL;
	cache->mFile1.mEntrynum                = -1;
	cache->mFile1.mBuffer                  = NULL;
	cache->mFile1.mResult                  = 0;
	cache->mFile1.mState                   = 1;
	cache->_00                             = 0;
	__register_global_object(cache, (void*)__dt__10ARPreCacheFv, (u8*)cache - 0xC);

	CThread* thread                 = &gMovieUpdateThread;
	*(void***)((u8*)thread + 0x320) = __vt__7CThread;
	thread->mStack                  = NULL;
	__register_global_object(thread, (void*)__dt__7CThreadFv, (u8*)thread - 0xC);
}
