#include "Version.h"

#if defined(VERSION_R24J01)
#include "types.h"
#include "revolution/mem.h"
#include "game/cscrn.h"
#include "game/cfile.h"

// the expanded heap mm::InitMem1 creates over the MEM1 arena; every operator delete site reads it
extern MEMiHeapHead* gWiiMem1Heap;

// this unit's deletes go straight to the MEM1 heap: the Wii port gives operator delete an
// inline body, so the emitted deleting destructors inline MEMFreeToExpHeap
inline void operator delete(void* ptr) throw()
{
	MEMFreeToExpHeap(gWiiMem1Heap, ptr);
}

// RTTI names CScrnWipeCine; no header declares it, so it is declared here as the
// polymorphic trivially-destroyed class its vtable and deleting dtor prove it to be
class CScrnWipeCine {
public:
	virtual ~CScrnWipeCine();
};

// retail lays these weak deleting dtors out contiguously at 0x8002FE00-0x80030088
CScrnRoboeyes::~CScrnRoboeyes()
{
}
CScrnWipeCine::~CScrnWipeCine()
{
}
CScrnWipeCustom::~CScrnWipeCustom()
{
}
CScrnWipeTex::~CScrnWipeTex()
{
}
CScrnWipeCircle::~CScrnWipeCircle()
{
}
CFileArchive::~CFileArchive()
{
}
#else
#include "game/hsd_jobj.h"
#include "game/cprimitive.h"

void operator delete(void* object);
extern "C" {
extern void* __vt__7CSprite[];
extern void* __vt__10CPrimitive[];

// todo: weak copy
/**
 * @note Address: 0x8001AE9C
 * @note Size: 0x5C
 */
CSprite* __dt__7CSpriteFv(CSprite* prim, s32 flags)
{
	if (prim != NULL) {
		*(void***)((u8*)prim + 0x164) = __vt__7CSprite; // _164 vtable
		prim->CPrimitive::~CPrimitive();
		if ((s16)flags > 0) {
			::operator delete(prim);
		}
	}
	return prim;
}

// todo: weak copy
/**
 * @note Address: 0x8001AEF8
 * @note Size: 0x48
 */
CPrimitive* __dt__10CPrimitiveFv(CPrimitive* prim, s32 flags)
{
	if (prim != NULL) {
		*(void***)((u8*)prim + 0x164) = __vt__10CPrimitive; // _164 vtable
		if ((s16)flags > 0) {
			::operator delete(prim);
		}
	}
	return prim;
}
}
#endif
