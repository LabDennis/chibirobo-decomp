#ifndef _GAME_ACTORPOOLSTORAGE_H
#define _GAME_ACTORPOOLSTORAGE_H

#include "game/xobjfree.h"
#include "game/xobjhbox.h"
#include "PowerPC_EABI_Support/Runtime/global_destructor_chain.h"

typedef u8 HBoxPoolStorage[sizeof(XObjHBox) * 16];
typedef u8 FreePoolStorage[sizeof(XObjFree) * 288];

extern "C" {
extern HBoxPoolStorage gHBoxPool;
extern DestructorChain sHBoxDestructor;
extern FreePoolStorage gFreePool;
extern DestructorChain sFreeDestructor;
}

#endif
