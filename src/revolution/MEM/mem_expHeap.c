#include "revolution/mem_exp_heap.h"
#include "mem_heap_internal.h"

#define MEM_EXP_HEAP_MAGIC       0x45585048U /* EXPH */
#define MEM_EXP_HEAP_MBLOCK_FREE 0x4652      /* FR */
#define MEM_EXP_HEAP_MBLOCK_USED 0x5544      /* UD */

// Size of base and expandable heap head
#define MEM_EXP_HEAP_HEAD_SIZE (sizeof(MEMiHeapHead) + sizeof(MEMiExpHeapHead))

typedef struct MemRegion {
	void* start; // at 0x0
	void* end;   // at 0x4
} MemRegion;

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

static MEMiExpHeapMBlock* InsertMBlock_(MEMiExpHeapMBlockList* list, MEMiExpHeapMBlock* block, MEMiExpHeapMBlock* prev)
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

static MEMiExpHeapMBlock* AppendMBlock_(MEMiExpHeapMBlockList* list, MEMiExpHeapMBlock* block)
{
	return InsertMBlock_(list, block, list->tail);
}

static void GetRegionOfMBlock_(MemRegion* region, MEMiExpHeapMBlock* mblock)
{
	region->start = SubU32ToPtr(mblock, GetAlignmentForMBlock_(mblock));
	region->end   = GetMBlockEndAddr_(mblock);
}

static MEMiExpHeapMBlock* InitMBlock_(const MemRegion* region, u16 state)
{
	MEMiExpHeapMBlock* mblock;
	mblock = (MEMiExpHeapMBlock*)region->start;

	mblock->state    = state;
	mblock->settings = 0;
	mblock->size     = GetOffsetFromPtr(GetMemPtrForMBlock_(mblock), region->end);
	mblock->prev     = NULL;
	mblock->next     = NULL;

	return mblock;
}

static MEMiExpHeapMBlock* InitFreeMBlock_(const MemRegion* region)
{
	return InitMBlock_(region, MEM_EXP_HEAP_MBLOCK_FREE);
}

static MEMiHeapHead* InitExpHeap_(MEMiHeapHead* heap, void* end, u16 opt)
{
	MEMiExpHeapMBlock* mblock;
	MEMiExpHeapHead* exp;
	MemRegion region;

	exp = GetExpHeapHeadPtrFromHeapHead_(heap);
	MEMiInitHeapHead(heap, MEM_EXP_HEAP_MAGIC, AddU32ToPtr(heap, MEM_EXP_HEAP_HEAD_SIZE), end, opt);
	exp->group    = 0;
	exp->settings = 0;
	SetAllocMode_(exp, MEM_EXP_HEAP_ALLOC_FAST);

	region.start = heap->start;
	region.end   = heap->end;

	mblock                = InitFreeMBlock_(&region);
	exp->freeMBlocks.head = mblock;
	exp->freeMBlocks.tail = mblock;

	exp->usedMBlocks.head = NULL;
	exp->usedMBlocks.tail = NULL;

	return heap;
}

static void* AllocUsedBlockFromFreeBlock_(MEMiExpHeapHead* exp, MEMiExpHeapMBlock* mblock, void* memPtr, u32 size, u16 allocDir)
{
	MemRegion leading;
	MemRegion trailing;
	MEMiExpHeapMBlock* prev;

	GetRegionOfMBlock_(&leading, mblock);
	trailing.end   = leading.end;
	trailing.start = AddU32ToPtr(memPtr, size);
	leading.end    = SubU32ToPtr(memPtr, sizeof(MEMiExpHeapMBlock));

	prev = RemoveMBlock_(&exp->freeMBlocks, mblock);

	if ((u32)GetOffsetFromPtr(leading.start, leading.end) < sizeof(MEMiExpHeapMBlock) + 4 || (allocDir == 0 && !exp->reuseAlignmentSpace)) {
		leading.end = leading.start;
	} else {
		prev = InsertMBlock_(&exp->freeMBlocks, InitFreeMBlock_(&leading), prev);
	}

	if ((u32)GetOffsetFromPtr(trailing.start, trailing.end) < sizeof(MEMiExpHeapMBlock) + 4 || (allocDir == 1 && !exp->reuseAlignmentSpace)) {
		trailing.start = trailing.end;
	} else {
		InsertMBlock_(&exp->freeMBlocks, InitFreeMBlock_(&trailing), prev);
	}

	FillAllocMemory(GetHeapHeadPtrFromExpHeapHead_(exp), leading.end, GetOffsetFromPtr(leading.end, trailing.start));

	{
		MEMiExpHeapMBlock* used;
		MemRegion region;

		region.start = SubU32ToPtr(memPtr, sizeof(MEMiExpHeapMBlock));
		region.end   = trailing.start;

		used           = InitMBlock_(&region, MEM_EXP_HEAP_MBLOCK_USED);
		used->allocDir = allocDir;
		used->align    = GetOffsetFromPtr(leading.end, used);
		used->group    = exp->group;
		AppendMBlock_(&exp->usedMBlocks, used);
	}
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

// NONMATCHING: retail loads merged.end and merged.start once for the size check and keeps them in
// registers for the new block; this version loads them a second time
static BOOL RecycleRegion_(MEMiExpHeapHead* exp, const MemRegion* region)
{
	MEMiExpHeapMBlock* prev = NULL;
	MemRegion merged        = *region;
	MEMiExpHeapMBlock* it;

	for (it = exp->freeMBlocks.head; it != NULL; it = it->next) {
		if ((void*)it < region->start) {
			prev = it;
			continue;
		}
		if ((void*)it == region->end) {
			merged.end = GetMBlockEndAddr_(it);
			RemoveMBlock_(&exp->freeMBlocks, it);
		}
		break;
	}
	if (prev != NULL && GetMBlockEndAddr_(prev) == region->start) {
		merged.start = prev;
		prev         = RemoveMBlock_(&exp->freeMBlocks, prev);
	}
	if ((u32)GetOffsetFromPtr(merged.start, merged.end) < sizeof(MEMiExpHeapMBlock)) {
		return FALSE;
	}
	InsertMBlock_(&exp->freeMBlocks, InitFreeMBlock_(&merged), prev);
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

void MEMFreeToExpHeap(MEMiHeapHead* heap, void* memBlock)
{
	if (memBlock == NULL) {
		return;
	}

	LockHeap(heap);
	{
		MEMiExpHeapHead* exp      = GetExpHeapHeadPtrFromHeapHead_(heap);
		MEMiExpHeapMBlock* mblock = GetMBlockHeadPtr_(memBlock);
		MemRegion region;

		GetRegionOfMBlock_(&region, mblock);
		RemoveMBlock_(&exp->usedMBlocks, mblock);
		RecycleRegion_(exp, &region);
	}
	UnlockHeap(heap);
}

u32 MEMGetAllocatableSizeForExpHeapEx(MEMiHeapHead* heap, s32 align)
{
	MEMiExpHeapMBlock* it;
	MEMiExpHeapHead* exp;
	u32 smallestPadding;
	u32 biggestSize;

	// the intrinsic, as in ogws: the arithmetic form computes into r4 and moves, retail computes into r31
	align = __abs(align);
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
