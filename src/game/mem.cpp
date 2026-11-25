#include "game/mm.h"
#include "game/evt.h"
#include <dolphin/os.h>
#include <string.h>

extern "C" {
void GetObjectHeapBounds(u32* size, void** top);
u32 GetObjectPoolCapacityBytes(void);
u32 GetObjectHeapRemainingBytes(void);
u32 GetObjectPoolUsedBytes(void);
u32 GetObjectPoolFreeBytes(void);
}

// one entry per block, used or freed. a freed block keeps its address in mFree so Alloc
// can hand it out again, the name is the tag the caller passed. MEMINFO is from the wii
// build's "no space for MEMINFO array" message
struct MEMINFO {
	u8* mAddr;        // _00
	u8* mFree;        // _04
	u32 mSize;        // _08
	char mName[0x80]; // _0C
	s32 mStatic;      // _8C, set blocks get reused whole instead of split
};
STATIC_ASSERT(sizeof(MEMINFO) == 0x90);

#define MEMINFO_MAX 0x400

static MEMINFO sInfo[MEMINFO_MAX];
static u8* sTop;
static u8* sCur;
static s32 sNum;

namespace mm {

static inline u32 GetAllocSize()
{
	BOOL enabled = OSDisableInterrupts();
	u32 size     = sTop - sCur; // the arena grows down from sTop
	OSRestoreInterrupts(enabled);
	return size;
}

// what's left of the arena once the game's blocks are taken out of it
static inline s32 GetRemainSize()
{
	BOOL enabled = OSDisableInterrupts();
	s32 remain   = GetObjectHeapRemainingBytes();
	s32 size;
	if (remain == -1) {
		size = 0;
	} else {
		size = remain - GetAllocSize();
	}
	OSRestoreInterrupts(enabled);
	return size;
}

// the arena left plus what HSD's object heap has freed
static inline s32 GetFreeSize()
{
	BOOL enabled = OSDisableInterrupts();
	u32 freed    = GetObjectPoolFreeBytes();
	s32 remain   = GetObjectHeapRemainingBytes();
	s32 size;
	if (remain == -1) {
		size = 0;
	} else {
		size = freed + remain - GetAllocSize();
	}
	OSRestoreInterrupts(enabled);
	return size;
}

} // namespace mm

#define KB(x) ((x) / 1024.0f)
#define MB(x) ((x) / 1048576.0f)

/**
 * @note Address: 0x801913D8
 * @note Size: 0x4EC
 */
void mm::Dump()
{
	OSReport("===============================================================================\n");

	u32 total = 0;
	for (int i = 0; i < MEMINFO_MAX; ++i) {
		MEMINFO* info = &sInfo[i];
		if (info->mAddr != 0) { // u lines are used blocks, f freed
			total += info->mSize;
			OSReport("-----(%4d) u %08X,%9d(%6.1f) %c %s\n", i, info->mAddr, info->mSize, KB(info->mSize),
			         (s8)(info->mStatic != 0 ? '*' : ' '), info->mName);
		} else if (info->mFree != 0) {
			total += info->mSize;
			OSReport("-----(%4d) f %08X,%9d(%6.1f) %c %s\n", i, info->mFree, info->mSize, KB(info->mSize),
			         (s8)(info->mStatic != 0 ? '*' : ' '), info->mName);
		}
	}
	OSReport("-----(%4d) * %08X,%9d(%6.1f)   <total>\n", sNum, sCur, total, KB(total));
	if (total != GetAllocSize()) {
		OSReport("mm::Dump>> total(%d) != GetAllocSize(%d)\n", total, GetAllocSize());
	}
	OSReport("\n");

	u32 size;
	GetObjectHeapBounds(&size, NULL);
	OSReport("ObjHeap   :%9d(%7.1fK)(%6.3fM)\n", size, KB(size), MB(size));
	OSReport(" remain   :%9d(%7.1fK)(%6.3fM)\n", GetObjectHeapRemainingBytes(), KB(GetObjectHeapRemainingBytes()),
	         MB(GetObjectHeapRemainingBytes()));
	OSReport(" alloc    :%9d(%7.1fK)(%6.3fM)\n", GetObjectPoolCapacityBytes(), KB(GetObjectPoolCapacityBytes()),
	         MB(GetObjectPoolCapacityBytes()));
	OSReport(" using    :%9d(%7.1fK)(%6.3fM)\n", GetObjectPoolUsedBytes(), KB(GetObjectPoolUsedBytes()), MB(GetObjectPoolUsedBytes()));
	OSReport(" freed    :%9d(%7.1fK)(%6.3fM)\n", GetObjectPoolFreeBytes(), KB(GetObjectPoolFreeBytes()), MB(GetObjectPoolFreeBytes()));
	OSReport("\n");

	size = GetFreeSize();
	OSReport("FreeHeap  :%9d(%7.1fK)(%6.3fM)\n", (s32)size, KB((s32)size), MB((s32)size));
	size = GetRemainSize();
	OSReport("RemainHeap:%9d(%7.1fK)(%6.3fM)\n", (s32)size, KB((s32)size), MB((s32)size));
	OSReport("\n");
}
/**
 * @note Address: 0x801918C4
 * @note Size: 0xB0
 */
void mm::SetName(void* ptr, const char* name)
{
	BOOL enabled = OSDisableInterrupts();
	for (int i = 0; i < sNum; i++) {
		if (sInfo[i].mAddr != 0 && sInfo[i].mAddr == ptr) {
			strcpy(sInfo[i].mName, name);
			OSRestoreInterrupts(enabled);
			return;
		}
	}
	OSRestoreInterrupts(enabled);
}
/**
 * @note Address: 0x80191974
 * @note Size: 0x94
 */
char* mm::GetName(void* ptr)
{
	BOOL enabled = OSDisableInterrupts();
	for (int i = 0; i < sNum; i++) {
		if (sInfo[i].mAddr != 0 && sInfo[i].mAddr == ptr) {
			OSRestoreInterrupts(enabled);
			return sInfo[i].mName;
		}
	}
	OSRestoreInterrupts(enabled);
	return NULL;
}
/**
 * @note Address: 0x80191A08
 * @note Size: 0x2F8
 */
void mm::Compact(BOOL dump)
{
	if (dump) {
		Dump();
	}

	BOOL enabled = OSDisableInterrupts();

	// glue each freed block to any freed block right after it
	for (int i = 0; i < sNum; i++) {
		if (sInfo[i].mAddr == 0 && sInfo[i].mFree != 0) {
			for (int j = 0; j < MEMINFO_MAX; j++) {
				if (i != j && sInfo[j].mAddr == 0 && sInfo[j].mFree != 0 && sInfo[i].mFree + sInfo[i].mSize == sInfo[j].mFree) {
					sInfo[i].mSize += sInfo[j].mSize;
					sInfo[j].mFree = 0;
					sInfo[j].mSize = 0;
				}
			}
		}
	}

	// find the lowest used block, every freed block below it goes back to the arena
	u8* low  = sTop;
	s32 last = 0;
	for (int i = 0; i < MEMINFO_MAX; i++) {
		if (sInfo[i].mAddr != 0) {
			if (sInfo[i].mAddr < low) {
				low = sInfo[i].mAddr;
			}
			last = i;
		}
	}
	for (int i = 0; i < MEMINFO_MAX; i++) {
		if (sInfo[i].mAddr == 0 && sInfo[i].mFree != 0) {
			if (sInfo[i].mFree < low) {
				sInfo[i].mFree = 0;
				sInfo[i].mSize = 0;
			}
			if (i > last) {
				last = i;
			}
		}
	}
	sCur = low;
	sNum = last + 1;

	OSRestoreInterrupts(enabled);
	if (dump) {
		Dump();
	}
}
/**
 * @note Address: 0x80191D00
 * @note Size: 0x104
 */
void mm::Free(void* ptr, s32 flag)
{
	BOOL enabled = OSDisableInterrupts();
	for (int i = 0; i < sNum; i++) {
		if (sInfo[i].mAddr != 0 && sInfo[i].mAddr == ptr) {
			if (sInfo[i].mAddr == sCur) {
				// the lowest block, give it straight back to the arena
				// (sCur only ever points at the lowest used block)
				sCur           = sInfo[i].mAddr + sInfo[i].mSize;
				sInfo[i].mFree = 0;
				sInfo[i].mAddr = 0;
				sInfo[i].mSize = 0;
			} else {
				sInfo[i].mAddr = 0;
			}
			if (flag) { // also clears the static marker
				sInfo[i].mStatic = 0;
			}
			Compact(FALSE);
			OSRestoreInterrupts(enabled);
			return;
		}
	}
	// freeing a pointer it doesn't track dumps the table
	Dump();
	OSRestoreInterrupts(enabled);
}
/**
 * @note Address: 0x80191E04
 * @note Size: 0x324
 */
void* mm::Alloc(u32 size, const char* name)
{
	BOOL enabled = OSDisableInterrupts();
	if (size & 31) { // blocks are 32-byte aligned
		OSReport("mm::Alloc>> alloc size must be 32byte alignment -- %s(%d)!\n", name, size);
		size = (size + 31) & ~31;
	}

	BOOL found = FALSE;
	if (size != 0) {
		int i;
		s32 num = sNum;
		u8* cur = sCur;
		for (i = 0; i < sNum; i++) {
			if (sInfo[i].mAddr == 0 && sInfo[i].mFree != 0 && sInfo[i].mSize >= size) {
				if (sInfo[i].mStatic != 0) { // a static block is reused whole
					sInfo[i].mAddr = sInfo[i].mFree;
					strcpy(sInfo[i].mName, name);
					found = TRUE;
					break;
				}
				s32 rest = sInfo[i].mSize - size;
				if (rest == 0) {
					sInfo[i].mAddr = sInfo[i].mFree;
					strcpy(sInfo[i].mName, name);
					found = TRUE;
					break;
				}
				if (rest > 0) {
					// split it, the low part stays free in a new entry
					for (int j = 0; j < MEMINFO_MAX; j++) {
						if (j != i && sInfo[j].mAddr == 0 && sInfo[j].mFree == 0) {
							sInfo[j].mAddr = 0;
							sInfo[j].mFree = sInfo[i].mFree;
							sInfo[j].mSize = rest;
							strcpy(sInfo[j].mName, "(subd)");
							if (sNum < j + 1) {
								sNum = j + 1;
							}
							sInfo[i].mAddr = sInfo[i].mFree + (sInfo[i].mSize - size);
							sInfo[i].mFree = sInfo[i].mAddr;
							sInfo[i].mSize = size;
							strcpy(sInfo[i].mName, name);
							found = TRUE;
							break;
						}
					}
				}
				break;
			}
		}

		if (!found) {
			// nothing reusable: carve a fresh block off the arena's bottom
			for (i = 0; i < MEMINFO_MAX; i++) {
				if (sInfo[i].mAddr == 0 && sInfo[i].mFree == 0) {
					sInfo[i].mFree = sCur - size;
					sInfo[i].mAddr = sCur - size;
					sInfo[i].mSize = size;
					strcpy(sInfo[i].mName, name);
					sCur -= size;
					if (sNum < i + 1) {
						sNum = i + 1;
					}
					found = TRUE;
					break;
				}
			}
		}

		if (found) {
			if (GetRemainSize() < 0) {
				// out of memory, undo it
				sCur           = cur;
				sInfo[i].mFree = 0;
				sInfo[i].mAddr = 0;
				sInfo[i].mSize = 0;
				sNum           = num;
			} else {
				OSRestoreInterrupts(enabled);
				return (void*)sInfo[i].mAddr;
			}
		}
	}

	OSRestoreInterrupts(enabled);
	return NULL;
}
/**
 * @note Address: 0x80192128
 * @note Size: 0xEC
 */
void mm::Init()
{
	for (int i = 0; i < MEMINFO_MAX; i++) {
		sInfo[i].mAddr    = 0;
		sInfo[i].mFree    = 0;
		sInfo[i].mSize    = 0;
		sInfo[i].mName[0] = '\0';
		sInfo[i].mStatic  = 0;
	}
	sTop = sCur = (u8*)OSGetArenaHi(); // the arena fills downward from here
	sNum        = 0;
}

/**
 * @note Address: 0x80192214
 * @note Size: 0x7C
 */
extern "C" void AllocateWordBuffer(CStack* stack, s32 capacity)
{
	if (stack->mBuf != NULL) {
		OSFreeToHeap(__OSCurrHeap, stack->mBuf);
		stack->mBuf   = NULL;
		stack->mSize  = 0;
		stack->mCount = 0;
	}
	stack->mBuf   = static_cast<s32*>(OSAllocFromHeap(__OSCurrHeap, capacity * sizeof(*stack->mBuf)));
	stack->mCount = 0;
	stack->mSize  = capacity;
}
