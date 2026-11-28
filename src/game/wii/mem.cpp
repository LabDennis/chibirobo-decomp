#include "game/mm.h"
#include <dolphin/os.h>
#include <revolution/mem_exp_heap.h>
#include <revolution/mem_heap.h>
#include <string.h>

extern "C" {
void* OSGetMEM2ArenaHi(void);
void OSSetMEM2ArenaLo(void* addr);
}

// The Wii memory manager keeps the GameCube's table scheme (blocks handed out from the top of
// the arena downward, one named MEMINFO entry each) but rides on MEM2: the entry table's arena
// ends at OSGetMEM2ArenaHi and an expanded heap each for MEM1 and MEM2 is set up in InitMem1 /
// InitMem2. Free lost its clear-static argument and Dump only checks the table, its reports are
// the empty Report() calls below.

// one entry per block, used or freed. a freed block keeps its address in mFree so Alloc
// can hand it out again, the name is the tag the caller passed
struct MEMINFO {
	u8* mAddr;        // _00
	u8* mFree;        // _04
	u32 mSize;        // _08
	char mName[0x80]; // _0C
	s32 mStatic;      // _8C, set blocks get reused whole instead of split
};
STATIC_ASSERT(sizeof(MEMINFO) == 0x90);

#define MEMINFO_MAX 0x400

// the allocators over the two heaps, the model code allocates its scene objects through them
MEMAllocator gWiiMem1Allocator;
MEMAllocator gWiiPadAllocator;
static MEMINFO sInfo[MEMINFO_MAX];
MEMiHeapHead* gWiiMem1Heap;
MEMiHeapHead* gWiiGameHeap;
static u8* sTop;
static u8* sCur;
static s32 sNum;
static bool sEnabled;

namespace mm {

static u32 GetAllocSize(void);
static s32 GetFreeSize(void);

// the reports are compiled out, their arguments still run: the size conversion stores and the
// GetAllocSize calls inside the check are all that is left of them in retail - the formats were
// passed as NULL, none of the table dump's format strings exist anywhere in the binary.
// NONMATCHING: on this compiler the inline has to be non-variadic to actually disappear;
// retail inlined it away with its formats gone entirely, so the calls carry no literal here
static inline void Report(const char* format, s32 i = 0, const void* a = NULL, u32 s = 0, f32 kb = 0.0f, s8 c = 0, const char* n = NULL)
{
}

} // namespace mm

// byte count in kilobytes for the reports
static inline f32 KB(u32 x)
{
	return x / 1024.0f;
}

/**
 * @note Address: 0x802A7478
 * @note Size: 0x5C
 */
void mm::InitMem1(void* start, u32 size)
{
	MEMiHeapHead* heap = MEMCreateExpHeapEx(start, size, MEM_HEAP_OPT_CLEAR_ALLOC | MEM_HEAP_OPT_DEBUG_FILL | MEM_HEAP_OPT_CAN_LOCK);
	gWiiMem1Heap       = heap;
	MEMInitAllocatorForExpHeap(&gWiiMem1Allocator, heap, 0x20);
	OSSetArenaLo((u8*)start + size);
}

/**
 * @note Address: 0x802A74D4
 * @note Size: 0x5C
 */
void mm::InitMem2(void* start, u32 size)
{
	MEMiHeapHead* heap = MEMCreateExpHeapEx(start, size, MEM_HEAP_OPT_CLEAR_ALLOC | MEM_HEAP_OPT_DEBUG_FILL | MEM_HEAP_OPT_CAN_LOCK);
	gWiiGameHeap       = heap;
	MEMInitAllocatorForExpHeap(&gWiiPadAllocator, heap, 0x20);
	OSSetMEM2ArenaLo((u8*)start + size);
}

/**
 * @note Address: 0x802A7530
 * @note Size: 0xF4
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
	sTop = sCur = (u8*)OSGetMEM2ArenaHi(); // the table arena fills downward from MEM2's top
	sNum        = 0;
	sEnabled    = true;
}

/**
 * @note Address: 0x802A7624
 * @note Size: 0x2F8
 */
void* mm::Alloc(u32 size, const char* name)
{
	BOOL enabled = OSDisableInterrupts();
	if (size & 31) { // blocks are 32-byte aligned
		size = (size + 31) & ~31;
	}

	BOOL found = FALSE;
	int i;
	if (size != 0) {
		for (i = 0; i < sNum; i++) {
			if (sInfo[i].mAddr == 0 && sInfo[i].mFree != 0 && sInfo[i].mSize >= size) {
				if (sInfo[i].mStatic != 0) { // a static block is reused whole
					sInfo[i].mAddr = sInfo[i].mFree;
					strcpy(sInfo[i].mName, name);
					found = TRUE;
				} else {
					s32 rest = sInfo[i].mSize - size;
					if (rest == 0) {
						sInfo[i].mAddr = sInfo[i].mFree;
						strcpy(sInfo[i].mName, name);
						found = TRUE;
					} else if (rest > 0) {
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
			if (GetFreeSize() < 0) {
				Dump();
				OSPanic("mem.cpp", 0x108, "mm::Alloc>> overflow -- %s %08X(%d)\n", sInfo[i].mName, sInfo[i].mAddr, sInfo[i].mSize);
			}
			OSRestoreInterrupts(enabled);
			return (void*)sInfo[i].mAddr;
		}
		Dump();
		OSPanic("mem.cpp", 0x11C, "mm::Alloc>> no space for MEMINFO array -- %s!\n", name);
	}

	OSRestoreInterrupts(enabled);
	return NULL;
}

/**
 * @note Address: 0x802A791C
 * @note Size: 0x108
 */
void mm::Free(void* ptr)
{
	BOOL enabled = OSDisableInterrupts();
	for (int i = 0; i < sNum; i++) {
		if (sInfo[i].mAddr != 0 && sInfo[i].mAddr == ptr) {
			MEMINFO* info = &sInfo[i];
			if (sInfo[i].mAddr == sCur && info->mStatic == 0) {
				// the lowest block goes straight back to the arena
				info->mFree    = 0;
				sCur           = sInfo[i].mAddr + info->mSize;
				sInfo[i].mAddr = 0;
				info->mSize    = 0;
			} else {
				sInfo[i].mAddr = 0;
			}
			Compact(FALSE);
			OSRestoreInterrupts(enabled);
			return;
		}
	}
	Dump();
	OSPanic("mem.cpp", 0x147, "mm::Free>> error (%d)!\n", sNum);
	OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x802A7A24
 * @note Size: 0x2CC
 */
void mm::Compact(BOOL dump)
{
	if (dump) {
		Dump();
	}

	BOOL enabled = OSDisableInterrupts();

	// glue each freed block to any freed block right after it
	for (int i = 0; i < sNum; i++) {
		if (sInfo[i].mAddr == 0 && sInfo[i].mFree != 0 && sInfo[i].mStatic == 0) {
			for (int j = 0; j < MEMINFO_MAX; j++) {
				if (i != j && sInfo[j].mAddr == 0 && sInfo[j].mFree != 0 && sInfo[j].mStatic == 0
				    && sInfo[i].mFree + sInfo[i].mSize == sInfo[j].mFree) {
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
				if (sInfo[i].mStatic == 0) {
					sInfo[i].mFree = 0;
					sInfo[i].mSize = 0;
				} else {
					// static blocks can't go back, the arena bottom stays below them
					low = sInfo[i].mFree;
				}
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
 * @note Address: 0x802A7CF0
 * @note Size: 0x3C
 */
u32 mm::GetAllocSize(void)
{
	BOOL enabled = OSDisableInterrupts();
	u32 size     = sTop - sCur; // the arena grows down from sTop
	OSRestoreInterrupts(enabled);
	return size;
}

/**
 * @note Address: 0x802A7D2C
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
 * @note Address: 0x802A7DDC
 * @note Size: 0x164
 */
void mm::Dump()
{
	if (sEnabled) {
		u32 total = 0;
		for (int i = 0; i < MEMINFO_MAX; i++) {
			MEMINFO* info = &sInfo[i];
			if (info->mAddr != 0) {
				total += info->mSize;
				Report(NULL, i, info->mAddr, info->mSize, KB(info->mSize), (s8)(info->mStatic != 0 ? '*' : ' '), info->mName);
			} else if (info->mFree != 0) {
				total += info->mSize;
				Report(NULL, i, info->mFree, info->mSize, KB(info->mSize), (s8)(info->mStatic != 0 ? '*' : ' '), info->mName);
			}
		}
		Report(NULL, sNum, sCur, total, KB(total));
		if (total != GetAllocSize()) {
			Report(NULL, total, NULL, GetAllocSize());
		}
		// NONMATCHING: the pool ends with "leak(%s)", referenced by nothing in retail. as the
		// last literal it came from a call textually after the total check - a pass that tags
		// the still-allocated entries as leaks when the table is dumped on an OOM panic
		for (int i = 0; i < sNum; i++) {
			MEMINFO* info = &sInfo[i];
			if (info->mAddr != 0) {
				Report("leak(%s)", i, NULL, 0, 0.0f, 0, info->mName);
			}
		}
	}
}

/**
 * @note Address: 0x802A7F40
 * @note Size: 0x74
 */
s32 mm::GetFreeSize(void)
{
	BOOL enabled = OSDisableInterrupts();
	s32 size     = 0;
	if (gWiiGameHeap != NULL) {
		size = MEMGetAllocatableSizeForExpHeapEx(gWiiGameHeap, 4);
	}
	if (sEnabled) {
		size -= GetAllocSize();
	}
	OSRestoreInterrupts(enabled);
	return size;
}
