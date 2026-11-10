#include "game/xobjactor.h"
#include "game/xobjfree.h"
#include "game/xobjhbox.h"
#include "game/xobjflags.h"
#include "game/xobjspiderboss.h"
#include "game/xobjspider.h"
#include "game/xobjspidersmall.h"
#include "game/xobjarmy.h"
#include "game/xobjleticker.h"
#include "game/xobjsound.h"
#include "game/xchr.h"
#include "game/xobjmap.h"
#include "game/xobjread.h"
#include "PowerPC_EABI_Support/Runtime/MWCPlusLib.h"

// the objects XOBJS hands out, their constructors and the weak destructors their arrays are torn
// down with when the game ends

extern "C" {
extern XObjFree gFreePool[0x120];
extern XObjHBox gHBoxPool[0x10];
extern XObjDust gDustPool[0x40];
extern XObjSpiderSmall gSpiderSmallPool[16];
extern XObjSpider gSpiderPool[16];
extern XObjArmy gArmyPool[21];
extern XObjActorDeca gActorDecaPool[2];
extern XObjActorToy gActorToyPool[15];
extern XObjLeticker gLetickerPool[12];
extern XObjItem gItemPool[0x40];
extern XObjCoin gCoinPool[0x60];
extern XObjSound gSoundPool[8];

XObjFree* __dt__8XObjFreeFv(XObjFree* object, s32 flags);
XObjHBox* __dt__8XObjHBoxFv(XObjHBox* object, s32 flags);
XObjDust* __dt__8XObjDustFv(XObjDust* object, s32 flags);
XObjItem* __dt__8XObjItemFv(XObjItem* object, s32 flags);
XObjCoin* __dt__8XObjCoinFv(XObjCoin* object, s32 flags);
XObjSound* __dt__9XObjSoundFv(XObjSound* object, s32 flags);
XObjSpiderSmall* __dt__15XObjSpiderSmallFv(XObjSpiderSmall* object, s32 flags);
XObjSpider* __dt__10XObjSpiderFv(XObjSpider* object, s32 flags);
XObjArmy* __dt__8XObjArmyFv(XObjArmy* object, s32 flags);
XObjActorDeca* __dt__13XObjActorDecaFv(XObjActorDeca* object, s32 flags);
XObjActorToy* __dt__12XObjActorToyFv(XObjActorToy* object, s32 flags);
XObjLeticker* __dt__12XObjLetickerFv(XObjLeticker* object, s32 flags);
}

/**
 * @note Address: 0x80132D08
 * @note Size: 0x94
 */
// vf94 on every object of the 23 pools
extern "C" void ResetPooledObjects(XObjPool* pools)
{
	XObjPool* pool = pools;
	for (s32 index = 0; index < 23; ++index, ++pool) {
		for (s32 i = 0; i < pool->mCount; ++i) {
			pools[index].Get(i)->vf94();
		}
	}
}

/**
 * @note Address: 0x80133B98
 * @note Size: 0x38
 */
extern "C" void DestroyXObjFreeArray()
{
	__destroy_arr(gFreePool, (ConstructorDestructor*)__dt__8XObjFreeFv, sizeof(XObjFree), ARRAY_SIZE(gFreePool));
}

/**
 * @note Address: 0x80133BD0
 * @note Size: 0x3C
 */
XObjFree::XObjFree()
{
}

/**
 * @note Address: 0x80133C0C
 * @note Size: 0x38
 */
extern "C" void DestroyXObjHBoxArray()
{
	__destroy_arr(gHBoxPool, (ConstructorDestructor*)__dt__8XObjHBoxFv, sizeof(XObjHBox), ARRAY_SIZE(gHBoxPool));
}

/**
 * @note Address: 0x80133C44
 * @note Size: 0x44
 */
XObjHBox::XObjHBox()
{
}

/**
 * @note Address: 0x80133C88
 * @note Size: 0x30
 */
XObjHBoxPart::XObjHBoxPart()
{
}

/**
 * @note Address: 0x80133CB8
 * @note Size: 0x38
 */
extern "C" void DestroyXObjDustArray()
{
	__destroy_arr(gDustPool, (ConstructorDestructor*)__dt__8XObjDustFv, sizeof(XObjDust), ARRAY_SIZE(gDustPool));
}

/**
 * @note Address: 0x80133CF0
 * @note Size: 0x48
 */
XObjDust::XObjDust()
{
	mFlags = &gDustEventFlags;
}

/**
 * @note Address: 0x80133D7C
 * @note Size: 0x5C
 */
XObjSpiderBoss::XObjSpiderBoss()
{
}

/**
 * @note Address: 0x80133DD8
 * @note Size: 0x38
 */
extern "C" void DestroyXObjSpiderSmallArray()
{
	__destroy_arr(gSpiderSmallPool, (ConstructorDestructor*)__dt__15XObjSpiderSmallFv, sizeof(XObjSpiderSmall),
	              ARRAY_SIZE(gSpiderSmallPool));
}

/**
 * @note Address: 0x80133E10
 * @note Size: 0x74
 */
XObjSpiderSmall::XObjSpiderSmall()
{
}

/**
 * @note Address: 0x80133E84
 * @note Size: 0x38
 */
extern "C" void DestroyXObjSpiderArray()
{
	__destroy_arr(gSpiderPool, (ConstructorDestructor*)__dt__10XObjSpiderFv, sizeof(XObjSpider), ARRAY_SIZE(gSpiderPool));
}

/**
 * @note Address: 0x80133EBC
 * @note Size: 0x84
 */
XObjSpider::XObjSpider()
{
}

/**
 * @note Address: 0x80133F40
 * @note Size: 0x38
 */
extern "C" void DestroyXObjArmyArray()
{
	__destroy_arr(gArmyPool, (ConstructorDestructor*)__dt__8XObjArmyFv, sizeof(XObjArmy), ARRAY_SIZE(gArmyPool));
}

/**
 * @note Address: 0x80133F78
 * @note Size: 0x88
 */
XObjArmy::XObjArmy()
{
}

/**
 * @note Address: 0x80134014
 * @note Size: 0x38
 */
// 0x80134000, XObjArmyShot's ctor, the compiler emits it here

extern "C" void DestroyXObjActorDecaArray()
{
	__destroy_arr(gActorDecaPool, (ConstructorDestructor*)__dt__13XObjActorDecaFv, sizeof(XObjActorDeca), ARRAY_SIZE(gActorDecaPool));
}

/**
 * @note Address: 0x8013404C
 * @note Size: 0x58
 */
XObjActorDeca::XObjActorDeca()
{
}

/**
 * @note Address: 0x80134100
 * @note Size: 0x50
 */
XObjActorTao::XObjActorTao()
{
}

/**
 * @note Address: 0x80134150
 * @note Size: 0x50
 */
XObjActorPapa::XObjActorPapa()
{
}

/**
 * @note Address: 0x801341A0
 * @note Size: 0x50
 */
XObjActorMama::XObjActorMama()
{
}

/**
 * @note Address: 0x801341F0
 * @note Size: 0x58
 */
XObjActorJenny::XObjActorJenny()
{
}

/**
 * @note Address: 0x80134248
 * @note Size: 0x38
 */
extern "C" void DestroyXObjActorToyArray()
{
	__destroy_arr(gActorToyPool, (ConstructorDestructor*)__dt__12XObjActorToyFv, sizeof(XObjActorToy), ARRAY_SIZE(gActorToyPool));
}

/**
 * @note Address: 0x80134280
 * @note Size: 0x80
 */
XObjActorToy::XObjActorToy()
{
}

/**
 * @note Address: 0x801340A4
 * @note Size: 0x5C
 */
XObjActor::XObjActor()
{
}

/**
 * @note Address: 0x80134300
 * @note Size: 0x38
 */
extern "C" void DestroyXObjLetickerArray()
{
	__destroy_arr(gLetickerPool, (ConstructorDestructor*)__dt__12XObjLetickerFv, sizeof(XObjLeticker), ARRAY_SIZE(gLetickerPool));
}

/**
 * @note Address: 0x80134338
 * @note Size: 0x48
 */
XObjLeticker::XObjLeticker()
{
}

/**
 * @note Address: 0x80134380
 * @note Size: 0x38
 */
extern "C" void DestroyXObjItemArray()
{
	__destroy_arr(gItemPool, (ConstructorDestructor*)__dt__8XObjItemFv, sizeof(XObjItem), ARRAY_SIZE(gItemPool));
}

/**
 * @note Address: 0x801343B8
 * @note Size: 0x48
 */
XObjItem::XObjItem()
{
	mFlags = &gItemEventFlags;
}

/**
 * @note Address: 0x80134400
 * @note Size: 0x38
 */
extern "C" void DestroyXObjCoinArray()
{
	__destroy_arr(gCoinPool, (ConstructorDestructor*)__dt__8XObjCoinFv, sizeof(XObjCoin), ARRAY_SIZE(gCoinPool));
}

/**
 * @note Address: 0x80134438
 * @note Size: 0x48
 */
XObjCoin::XObjCoin()
{
	mFlags = &gCoinEventFlags;
}

/**
 * @note Address: 0x80133D38
 * @note Size: 0x44
 */
XObjFlags::XObjFlags()
{
	mFlags = NULL;
}

/**
 * @note Address: 0x80134480
 * @note Size: 0x38
 */
extern "C" void DestroyXObjSoundArray()
{
	__destroy_arr(gSoundPool, (ConstructorDestructor*)__dt__9XObjSoundFv, sizeof(XObjSound), ARRAY_SIZE(gSoundPool));
}

/**
 * @note Address: 0x801344B8
 * @note Size: 0x48
 */
XObjSound::XObjSound()
{
}
