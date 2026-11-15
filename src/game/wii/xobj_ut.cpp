#include "Version.h"
#if defined(VERSION_R24J01)
#include "game/wii/xobj.h"

// retail's xobj_ut.cpp: the small XObj classes (XObj, XObjLight, XObjMap and the
// CustomShadow helper) plus the object pools and their two reader threads

CustomShadow::~CustomShadow()
{
}

// builds the 23 pools from heap arrays and starts the two reader threads
extern "C" void InitializeObjectPools(XOBJS* list)
{
#define XOBJ_POOL(slot, type, count, name)   \
	{                                        \
		type* objs     = new type[count];    \
		XObjPool& pool = list->mPools[slot]; \
		pool.mObjects  = (u8*)objs;          \
		pool.mUsed     = 0;                  \
		pool.mCount    = count;              \
		pool.mName     = name;               \
		pool.mStride   = sizeof(type);       \
		for (s32 i = 0; i < count; i++) {    \
			pool.Get(i)->InitializeObject(); \
		}                                    \
	}

	XOBJ_POOL(0, XObjScnCam, 1, "SCNCAM");
	XOBJ_POOL(1, XObjLight, 1, "LIGHT");
	XOBJ_POOL(3, XObjMap, 1, "MAP");
	XOBJ_POOL(2, XObjChr, 1, "CHR");
	XOBJ_POOL(4, XObj, 2, "ACTOR2");
	XOBJ_POOL(5, XObjActor3, 48, "ACTOR3");
	XOBJ_POOL(6, XObjSound, 8, "SOUND");
	XOBJ_POOL(7, XObjCoin, 96, "COIN");
	XOBJ_POOL(8, XObjItem, 64, "ITEM");
	XOBJ_POOL(9, XObjLeticker, 12, "LETICKER");
	XOBJ_POOL(10, XObjActorToy, 15, "ACTOR_TOY");
	XOBJ_POOL(11, XObjActorJenny, 1, "ACTOR_JENNY");
	XOBJ_POOL(12, XObjActorMama, 1, "ACTOR_MAMA");
	XOBJ_POOL(13, XObjActorPapa, 1, "ACTOR_PAPA");
	XOBJ_POOL(14, XObjActorTao, 1, "ACTOR_TAO");
	XOBJ_POOL(15, XObjActorDeca, 2, "ACTOR_DECA");
	XOBJ_POOL(16, XObjArmy, 21, "ARMY");
	XOBJ_POOL(17, XObjSpider, 16, "SPIDER");
	XOBJ_POOL(18, XObjSpiderSmall, 16, "SPIDER_SMALL");
	XOBJ_POOL(19, XObjSpiderBoss, 1, "SPIDER_BOSS");
	XOBJ_POOL(20, XObjDust, 64, "DUST");
	XOBJ_POOL(21, XObjHBox, 16, "HBOX");
	XOBJ_POOL(22, XObjFree, 400, "FREE");
#undef XOBJ_POOL

	list->mLoadState = 0;

	gAllObjectsReader.Create(ReadObjectListThread, NULL, 0x2000, 17, 1);
	gSingleObjectReader.Create(ReadSingleObjectThread, NULL, 0x2000, 16, 1);
}
#endif
