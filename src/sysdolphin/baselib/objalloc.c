#include <functions.h>
#include <sysdolphin/memory.h>
#include <sysdolphin/objalloc.h>
#include <sysdolphin/util.h>

static objheap obj_heap = { 0, 0, -1, -1 };

static HSD_ObjAllocData* alloc_datas;

/**
 * @note Address: 0x801D0684
 * @note Size: 0x1C
 */
void HSD_ObjSetHeap(u32 size, void* ptr)
{
	obj_heap.curr   = (u32)ptr;
	obj_heap.top    = (u32)ptr;
	obj_heap.remain = size;
	obj_heap.size   = size;
}

/**
 * @note Address: 0x801D06A0
 * @note Size: 0x30
 */
void GetObjectHeapBounds(u32* size, void** top)
{
	if (size != NULL) {
		*size = obj_heap.size;
	}
	if (top != NULL) {
		*top = (void*)obj_heap.top;
	}
}

/**
 * @note Address: 0x801D06D0
 * @note Size: 0x34
 */
u32 GetObjectPoolCapacityBytes(void)
{
	HSD_ObjAllocData* data;
	u32 total = 0;

	for (data = alloc_datas; data != NULL; data = data->next) {
		total += data->size * (data->used + data->free);
	}
	return total;
}

/**
 * @note Address: 0x801D0704
 * @note Size: 0x38
 */
u32 GetObjectHeapRemainingBytes(void)
{
	if (obj_heap.top != 0) {
		return obj_heap.remain;
	}
	return _HSD_MemGetRemain();
}

/**
 * @note Address: 0x801D073C
 * @note Size: 0x2C
 */
u32 GetObjectPoolUsedBytes(void)
{
	HSD_ObjAllocData* data;
	u32 total = 0;

	for (data = alloc_datas; data != NULL; data = data->next) {
		total += data->size * data->used;
	}
	return total;
}

/**
 * @note Address: 0x801D0768
 * @note Size: 0x2C
 */
u32 GetObjectPoolFreeBytes(void)
{
	HSD_ObjAllocData* data;
	u32 total = 0;

	for (data = alloc_datas; data != NULL; data = data->next) {
		total += data->size * data->free;
	}
	return total;
}

/**
 * @note Address: 0x801D0794
 * @note Size: 0x250
 */
s32 HSD_ObjAllocAddFree(HSD_ObjAllocData* data, u32 num)
{
	u32 total_size;
	u8* obj;
	int i;
	u8* end;
	u8* heap_end;
	u32 remain;

	assert_line(0xEE, data);

	total_size = data->size * num;
	if (obj_heap.top != 0) {
		heap_end = (u8*)obj_heap.top + obj_heap.size;
		obj      = (u8*)((obj_heap.curr + (data->align - 1)) & ~(data->align - 1));
		if (obj > heap_end) {
			return 0;
		}

		remain = heap_end - obj;
		if (remain < total_size) {
			total_size = remain - (remain % data->size);
		}

		num = total_size / data->size;
		if (num == 0) {
			return 0;
		}

		end             = obj + total_size;
		obj_heap.curr   = (u32)end;
		obj_heap.remain = heap_end - end;
	} else {
		obj = _HSD_MemAlloc(total_size, data->align, 1);
		if (obj == NULL) {
			return 0;
		}
	}

	{
		HSD_ObjAllocLink* link;
		for (i = 0; (unsigned)i < num - 1; i++) {
			link       = (HSD_ObjAllocLink*)(obj + data->size * i);
			link->next = (HSD_ObjAllocLink*)(obj + data->size * (i + 1));
		}
		link       = (HSD_ObjAllocLink*)(obj + data->size * i);
		link->next = data->freehead;
	}
	data->freehead = (HSD_ObjAllocLink*)obj;
	data->free += num;

	return num;
}

/**
 * @note Address: 0x801D09E4
 * @note Size: 0x150
 */
void* HSD_ObjAlloc(HSD_ObjAllocData* data)
{
	HSD_ObjAllocLink* obj;
	u32 remain;

	if (data->num_limit_flag && data->used >= data->num_limit) {
		return NULL;
	}

	if (data->heap_limit_flag) {
		if (data->heap_limit_num == -1) {
			if (obj_heap.top != 0) {
				remain = obj_heap.remain;
			} else {
				remain = _HSD_MemGetRemain();
			}

			if (remain <= data->heap_limit_size) {
				data->heap_limit_num = data->used + data->free;
			}
		} else {
			if (obj_heap.top != 0) {
				remain = obj_heap.remain;
			} else {
				remain = _HSD_MemGetRemain();
			}

			if (remain > data->heap_limit_size) {
				data->heap_limit_num = -1;
			}
		}

		if (data->used >= data->heap_limit_num) {
			return NULL;
		}
	}

	if (data->free == 0) {
		HSD_ObjAllocAddFree(data, 1);
		if (data->free == 0) {
			return NULL;
		}
	}

	obj            = data->freehead;
	data->freehead = obj->next;
	data->used++;
	data->free--;
	if (data->used > data->peak) {
		data->peak = data->used;
	}

	return obj;
}

/**
 * @note Address: 0x801D0B34
 * @note Size: 0x28
 */
void HSD_ObjFree(HSD_ObjAllocData* data, void* obj)
{
	((HSD_ObjAllocLink*)obj)->next = data->freehead;
	data->freehead                 = obj;
	data->free++;
	data->used--;
}

static inline void removeAll(HSD_ObjAllocData* data)
{
	HSD_ObjAllocData** cur = &alloc_datas;
	while (*cur != NULL) {
		if (*cur == data) {
			*cur = (*cur)->next;
		} else {
			cur = &(*cur)->next;
		}
	}
}

/**
 * @note Address: 0x801D0B5C
 * @note Size: 0x124
 */
void HSD_ObjAllocInit(HSD_ObjAllocData* data, u32 size, u32 align)
{
	u32 aligned_size;

	assert_line(0x182, data);
	if (data != NULL) {
		removeAll(data);
	} else {
		alloc_datas = NULL;
	}

	memset(data, 0, sizeof(HSD_ObjAllocData));
	data->num_limit       = -1;
	data->heap_limit_size = 0;
	data->heap_limit_num  = -1;

	assert_line(0x18D, align <= 32);
	assert_line(0x18E, HSD_GetNbBits(align) == 1);

	data->align  = align;
	aligned_size = (size + (data->align - 1)) & ~(data->align - 1);
	data->size   = aligned_size;
	data->next   = alloc_datas;
	alloc_datas  = data;
}
