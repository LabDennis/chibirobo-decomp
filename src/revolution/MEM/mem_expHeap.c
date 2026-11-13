#include "revolution/mem_exp_heap.h"
#include "mem_heap_internal.h"

#define MEM_EXP_HEAP_MAGIC       0x45585048U /* EXPH */
#define MEM_EXP_HEAP_MBLOCK_FREE 0x4652      /* FR */
#define MEM_EXP_HEAP_MBLOCK_USED 0x5544      /* UD */

// Size of base and expandable heap head
#define MEM_EXP_HEAP_HEAD_SIZE (sizeof(MEMiHeapHead) + sizeof(MEMiExpHeapHead))

static MEMiExpHeapHead* GetExpHeapHeadPtrFromHeapHead_(const MEMiHeapHead* heap)
{
	return AddU32ToPtr(heap, sizeof(MEMiHeapHead));
}

static MEMiHeapHead* GetHeapHeadPtrFromExpHeapHead_(const MEMiExpHeapHead* exp)
{
	return SubU32ToPtr(exp, sizeof(MEMiHeapHead));
}

static void SetAllocMode_(MEMiExpHeapHead* exp, u16 mode)
{
	exp->allocMode = mode;
}

static u16 GetAllocMode_(const MEMiExpHeapHead* exp)
{
	return exp->allocMode;
}

static MEMiExpHeapMBlock* GetMBlockHeadPtr_(void* memBlock)
{
	return SubU32ToPtr(memBlock, sizeof(MEMiExpHeapMBlock));
}

static void* GetMemPtrForMBlock_(MEMiExpHeapMBlock* mblock)
{
	return AddU32ToPtr(mblock, sizeof(MEMiExpHeapMBlock));
}

static void* GetMBlockEndAddr_(MEMiExpHeapMBlock* mblock)
{
	return AddU32ToPtr(GetMemPtrForMBlock_(mblock), mblock->size);
}

static s32 GetAlignmentForMBlock_(const MEMiExpHeapMBlock* mblock)
{
	return mblock->align;
}

static MEMiExpHeapMBlock* RemoveMBlock_(MEMiExpHeapMBlockList* list, MEMiExpHeapMBlock* mblock)
{
	MEMiExpHeapMBlock* prev;
	MEMiExpHeapMBlock* next;

	prev = mblock->prev;
	next = mblock->next;

	// Fix prev link
	if (prev != NULL) {
		prev->next = next;
	} else {
		list->head = next;
	}

	// Fix next link
	if (next != NULL) {
		next->prev = prev;
	} else {
		list->tail = prev;
	}

	return prev;
}

static MEMiExpHeapMBlock* InsertMBlock_(MEMiExpHeapMBlockList* list, MEMiExpHeapMBlock* prev, MEMiExpHeapMBlock* block)
{
	MEMiExpHeapMBlock* next;
	block->prev = prev;
	if (prev != NULL) {
		next       = prev->next;
		prev->next = block;
	} else {
		next       = list->head;
		list->head = block;
	}
	block->next = next;
	if (next != NULL) {
		next->prev = block;
	} else {
		list->tail = block;
	}
	return block;
}

static void GetRegionOfMBlock_(void** region, MEMiExpHeapMBlock* mblock)
{
	region[0] = SubU32ToPtr(mblock, GetAlignmentForMBlock_(mblock));
	region[1] = GetMBlockEndAddr_(mblock);
}

static MEMiExpHeapMBlock* InitMBlock_(void** region, u16 state)
{
	MEMiExpHeapMBlock* mblock;
	mblock = (MEMiExpHeapMBlock*)region[0];

	mblock->state    = state;
	mblock->settings = 0;
	mblock->size     = GetOffsetFromPtr(GetMemPtrForMBlock_(mblock), region[1]);
	mblock->prev     = NULL;
	mblock->next     = NULL;

	return mblock;
}

static MEMiExpHeapMBlock* InitFreeMBlock_(void** region)
{
	return InitMBlock_(region, MEM_EXP_HEAP_MBLOCK_FREE);
}

static MEMiHeapHead* InitExpHeap_(MEMiHeapHead* heap, void* end, u16 opt)
{
	MEMiExpHeapMBlock* mblock;
	MEMiExpHeapHead* exp;
	void* region[2];

	exp = GetExpHeapHeadPtrFromHeapHead_(heap);
	MEMiInitHeapHead(heap, MEM_EXP_HEAP_MAGIC, AddU32ToPtr(heap, MEM_EXP_HEAP_HEAD_SIZE), end, opt);
	exp->group    = 0;
	exp->settings = 0;
	SetAllocMode_(exp, MEM_EXP_HEAP_ALLOC_FAST);

	region[0] = heap->start;
	region[1] = heap->end;

	mblock                = InitFreeMBlock_(region);
	exp->freeMBlocks.head = mblock;
	exp->freeMBlocks.tail = mblock;

	exp->usedMBlocks.head = NULL;
	exp->usedMBlocks.tail = NULL;

	return heap;
}

// NONMATCHING: register allocation differs from retail, and one load and a subtraction come earlier
static void* AllocUsedBlockFromFreeBlock_(MEMiExpHeapHead* exp, MEMiExpHeapMBlock* mblock, void* memPtr, u32 size, u16 allocDir)
{
	void* region[2];
	void* leading[2];
	void* trailing[2];
	MEMiExpHeapMBlock* used = GetMBlockHeadPtr_(memPtr);
	MEMiExpHeapMBlock* prev;
	void* usedStart = used;
	void* usedEnd   = AddU32ToPtr(memPtr, size);

	GetRegionOfMBlock_(region, mblock);
	prev = RemoveMBlock_(&exp->freeMBlocks, mblock);

	leading[0] = region[0];
	leading[1] = used;
	if ((u32)GetOffsetFromPtr(leading[0], leading[1]) < sizeof(*used) + 4 || (allocDir == 0 && !exp->reuseAlignmentSpace)) {
		usedStart = leading[0];
	} else {
		prev = InsertMBlock_(&exp->freeMBlocks, prev, InitFreeMBlock_(leading));
	}

	trailing[0] = usedEnd;
	trailing[1] = region[1];
	if ((u32)GetOffsetFromPtr(trailing[0], trailing[1]) < sizeof(*used) + 4 || (allocDir == 1 && !exp->reuseAlignmentSpace)) {
		usedEnd = trailing[1];
	} else {
		InsertMBlock_(&exp->freeMBlocks, prev, InitFreeMBlock_(trailing));
	}

	FillAllocMemory(GetHeapHeadPtrFromExpHeapHead_(exp), usedStart, GetOffsetFromPtr(usedStart, usedEnd));
	region[0] = used;
	region[1] = usedEnd;
	InitMBlock_(region, MEM_EXP_HEAP_MBLOCK_USED);
	used->allocDir = allocDir;
	used->align    = GetOffsetFromPtr(usedStart, used);
	used->group    = exp->group;
	InsertMBlock_(&exp->usedMBlocks, exp->usedMBlocks.tail, used);
	return memPtr;
}

// NONMATCHING: register allocation differs from retail
static void* AllocFromHead_(MEMiHeapHead* heap, u32 size, s32 align)
{
	MEMiExpHeapMBlock* it;
	MEMiExpHeapHead* exp;
	MEMiExpHeapMBlock* bestBlk;
	u32 bestBlkSize;
	void* bestBlkMemPtr;
	BOOL isFastAlloc;

	exp           = GetExpHeapHeadPtrFromHeapHead_(heap);
	isFastAlloc   = GetAllocMode_(exp) == MEM_EXP_HEAP_ALLOC_FAST;
	bestBlk       = 0;
	bestBlkSize   = 0xFFFFFFFF;
	bestBlkMemPtr = 0;

	for (it = exp->freeMBlocks.head; it != NULL; it = it->next) {
		void* start      = GetMemPtrForMBlock_(it);
		void* alignStart = ROUND_UP_PTR(GetUIntPtr(start), align);
		u32 padding      = GetOffsetFromPtr(start, alignStart);

		if (it->size >= size + padding && bestBlkSize > it->size) {
			bestBlk       = it;
			bestBlkSize   = it->size;
			bestBlkMemPtr = alignStart;

			if (isFastAlloc || bestBlkSize == size) {
				break;
			}
		}
	}

	return bestBlk != NULL ? AllocUsedBlockFromFreeBlock_(exp, bestBlk, bestBlkMemPtr, size, 0) : NULL;
}

// NONMATCHING: register allocation differs from retail
static void* AllocFromTail_(MEMiHeapHead* heap, u32 size, s32 align)
{
	MEMiExpHeapMBlock* it;
	MEMiExpHeapHead* exp;
	MEMiExpHeapMBlock* bestBlk;
	u32 bestBlkSize;
	void* bestBlkMemPtr;
	BOOL isFastAlloc;

	exp           = GetExpHeapHeadPtrFromHeapHead_(heap);
	isFastAlloc   = GetAllocMode_(exp) == MEM_EXP_HEAP_ALLOC_FAST;
	bestBlk       = 0;
	bestBlkSize   = 0xFFFFFFFF;
	bestBlkMemPtr = 0;

	for (it = exp->freeMBlocks.tail; it != NULL; it = it->prev) {
		void* start = GetMemPtrForMBlock_(it);
		void* end   = ROUND_DOWN_PTR(GetUIntPtr(SubU32ToPtr(AddU32ToPtr(start, it->size), size)), align);

		if (ComparePtr(end, start) >= 0 && bestBlkSize > it->size) {
			bestBlk       = it;
			bestBlkSize   = it->size;
			bestBlkMemPtr = end;

			if (isFastAlloc || bestBlkSize == size) {
				break;
			}
		}
	}

	return bestBlk != NULL ? AllocUsedBlockFromFreeBlock_(exp, bestBlk, bestBlkMemPtr, size, 1) : NULL;
}

// NONMATCHING: retail sets up a stack frame this version does not need, and register allocation differs
static BOOL RecycleRegion_(MEMiExpHeapHead* exp, void** region)
{
	void* merged[2];
	MEMiExpHeapMBlock* it;
	MEMiExpHeapMBlock* prev = NULL;
	merged[0]               = region[0];
	merged[1]               = region[1];
	for (it = exp->freeMBlocks.head; it != NULL; it = it->next) {
		if ((uintptr_t)it < (uintptr_t)region[0]) {
			prev = it;
			continue;
		}
		if ((void*)it == region[1]) {
			merged[1] = GetMBlockEndAddr_(it);
			RemoveMBlock_(&exp->freeMBlocks, it);
		}
		break;
	}
	if (prev != NULL && GetMBlockEndAddr_(prev) == region[0]) {
		merged[0] = prev;
		prev      = RemoveMBlock_(&exp->freeMBlocks, prev);
	}
	if ((u32)GetOffsetFromPtr(merged[0], merged[1]) < sizeof(*it)) {
		return FALSE;
	}
	InsertMBlock_(&exp->freeMBlocks, prev, InitFreeMBlock_(merged));
	return TRUE;
}

MEMiHeapHead* MEMCreateExpHeapEx(void* start, u32 size, u16 opt)
{
	void* end = AddU32ToPtr(start, size);

	// Heap range
	end   = ROUND_DOWN_PTR(end, 4);
	start = ROUND_UP_PTR(start, 4);

	// Ensure valid heap
	if (GetUIntPtr(start) > GetUIntPtr(end) || GetOffsetFromPtr(start, end) < MEM_EXP_HEAP_MIN_SIZE) {
		return NULL;
	}

	return InitExpHeap_(start, end, opt);
}

void* MEMAllocFromExpHeapEx(MEMiHeapHead* heap, u32 size, s32 align)
{
	void* memBlock;

	if (size == 0) {
		size = 1;
	}
	size = ROUND_UP(size, 4);

	LockHeap(heap);

	// Alignment sign determines alloc direction
	if (align >= 0) {
		memBlock = AllocFromHead_(heap, size, align);
	} else {
		memBlock = AllocFromTail_(heap, size, -align);
	}

	UnlockHeap(heap);

	return memBlock;
}

// NONMATCHING: the saved registers and register allocation differ from retail
void MEMFreeToExpHeap(MEMiHeapHead* heap, void* memBlock)
{
	MEMiExpHeapMBlock* mblock;
	MEMiExpHeapHead* exp;
	void* region[2];

	if (memBlock == NULL) {
		return;
	}

	exp    = GetExpHeapHeadPtrFromHeapHead_(heap);
	mblock = GetMBlockHeadPtr_(memBlock);

	LockHeap(heap);

	GetRegionOfMBlock_(region, mblock);
	RemoveMBlock_(&exp->usedMBlocks, mblock);
	RecycleRegion_(exp, region);

	UnlockHeap(heap);
}

// NONMATCHING: register allocation and the scheduling of the |align| computation around the register saves
// differ from retail
u32 MEMGetAllocatableSizeForExpHeapEx(MEMiHeapHead* heap, s32 align)
{
	MEMiExpHeapMBlock* it;
	MEMiExpHeapHead* exp;
	u32 smallestPadding;
	u32 biggestSize;

	align = align < 0 ? -align : align;
	exp   = GetExpHeapHeadPtrFromHeapHead_(heap);

	LockHeap(heap);

	biggestSize     = 0;
	smallestPadding = 0xFFFFFFFF;
	for (it = exp->freeMBlocks.head; it != NULL; it = it->next) {
		// MBlock buffer region
		void* start      = GetMemPtrForMBlock_(it);
		void* alignStart = ROUND_UP_PTR(GetUIntPtr(start), align);
		void* end        = GetMBlockEndAddr_(it);

		if (alignStart < end) {
			u32 size    = GetOffsetFromPtr(alignStart, end);
			u32 padding = GetOffsetFromPtr(start, alignStart);

			// MBlock with new best size, or same as best size + less alignment
			// padding
			if (biggestSize < size || (biggestSize == size && smallestPadding > padding)) {
				biggestSize     = size;
				smallestPadding = padding;
			}
		}
	}

	UnlockHeap(heap);
	return biggestSize;
}
