#include <Dolphin/OS/OSUtil.h>
#include <sysdolphin/class_new.h>
#include <sysdolphin/hash.h>
#include <sysdolphin/memory.h>
#include <string.h>

HSD_ClassInfo hsdClass = { _hsdClassInfoInit };

/**
 * @note Address: 0x801D5C34
 * @note Size: 0xE8
 */
void hsdInitClassInfo(HSD_ClassInfo* class_info, HSD_ClassInfo* parent_info,
                      char* base_class_library, char* type, s32 info_size,
                      s32 class_size)
{

    class_info->head.flags = 1;
    class_info->head.library_name = base_class_library;
    class_info->head.class_name = type;
    class_info->head.obj_size = (s16) class_size;
    class_info->head.info_size = (s16) info_size;
    class_info->head.parent = parent_info;
    class_info->head.child = NULL;
    class_info->head.next = NULL;
    class_info->head.nb_exist = 0;
    class_info->head.nb_peak = 0;
    if (parent_info != NULL) {
        if ((parent_info->head.flags & 1) == 0) {
            parent_info->head.info_init();
        }
        assert_line(95, class_info->head.obj_size >= parent_info->head.obj_size);
        assert_line(96, class_info->head.info_size >= parent_info->head.info_size);
        memcpy(&class_info->alloc, &parent_info->alloc, parent_info->head.info_size - sizeof(HSD_ClassInfoHead));
        class_info->head.next = parent_info->head.child;
        parent_info->head.child = class_info;
    }
}

/**
 * @note Address: N/A
 * @note Size: 0x50
 */
void OSReport_PrintSpaces(s32 count) {
    s32 i;

    for (i = 0; i < count; i++) {
        OSReport(" ");
    }
}

typedef struct _HSD_FreeList {
    struct _HSD_FreeList* next;
} HSD_FreeList;

typedef struct _HSD_MemoryEntry {
    u32 size;
    u32 nb_alloc;
    u32 nb_free;
    struct _HSD_FreeList* free_list;
    struct _HSD_MemoryEntry* next;
} HSD_MemoryEntry;

enum {
    INITIAL_MEMORY_ENTRY_COUNT = 32,
    MEMORY_ENTRY_SIZE_STEP = 32,
};

static HSD_Hash* ClassInfoHash;
s32 ClassMemoryEntryCount;
HSD_MemoryEntry** ClassMemoryEntries;

static inline s32 MemoryEntryIndex(s32 size)
{
    return (size + MEMORY_ENTRY_SIZE_STEP - 1) / MEMORY_ENTRY_SIZE_STEP - 1;
}

/**
 * @note Address: 0x801D5D1C
 * @note Size: 0x264
 */
HSD_MemoryEntry* GetMemoryEntry(s32 idx)
{
    assert_line(174, idx >= 0);

    if (idx >= ClassMemoryEntryCount) {
        if (ClassMemoryEntryCount == 0) {
            s32 new_count;
            new_count = INITIAL_MEMORY_ENTRY_COUNT;
            while (idx >= new_count) {
                new_count *= 2;
            }
            ClassMemoryEntries = _HSD_MemAlloc(new_count * sizeof(*ClassMemoryEntries), 4, 1);
            if (ClassMemoryEntries == NULL) {
                return NULL;
            }
            memset(ClassMemoryEntries, 0, new_count * sizeof(*ClassMemoryEntries));
            ClassMemoryEntryCount = new_count;
        } else {
            s32 new_count;
            HSD_MemoryEntry** old_list;
            HSD_MemoryEntry** new_list;
            s32 old_size;

            new_count = ClassMemoryEntryCount * 2;
            while (idx >= new_count) {
                new_count *= 2;
            }

            new_list = _HSD_MemAlloc(new_count * sizeof(*new_list), 4, 1);
            if (new_list == NULL) {
                return NULL;
            }

            memcpy(new_list, ClassMemoryEntries, ClassMemoryEntryCount * sizeof(*new_list));
            memset(&new_list[ClassMemoryEntryCount], 0, (new_count - ClassMemoryEntryCount) * sizeof(*new_list));

            old_list = ClassMemoryEntries;
            old_size = OSRoundDown32B(ClassMemoryEntryCount * sizeof(*old_list));
            ClassMemoryEntries = new_list;
            ClassMemoryEntryCount = new_count;

            hsdFreeMemPiece(old_list, old_size);
            ClassMemoryEntries[OSRoundUp32B(old_size) / MEMORY_ENTRY_SIZE_STEP - 1]->nb_alloc += 1;
        }
    }
    {
        int i;
        BOOL found;
        HSD_MemoryEntry* entry;
        if (ClassMemoryEntries[idx] == NULL) {
            entry = _HSD_MemAlloc(sizeof(*entry), 4, 1);
            if (entry == NULL) {
                return NULL;
            }
            memset(entry, 0, sizeof(*entry));
            entry->size = (idx + 1) * MEMORY_ENTRY_SIZE_STEP;
            ClassMemoryEntries[idx] = entry;

            found = FALSE;
            for (i = idx - 1; i >= 0; --i) {
                if (ClassMemoryEntries[i] != NULL) {
                    found = TRUE;
                    entry->next = ClassMemoryEntries[i]->next;
                    ClassMemoryEntries[i]->next = entry;
                    break;
                }
            }

            if (found == FALSE) {
                for (i = idx + 1; i < ClassMemoryEntryCount; i++) {
                    if (ClassMemoryEntries[i] != NULL) {
                        entry->next = ClassMemoryEntries[i];
                        break;
                    }
                }
            }
        }
    }
    return ClassMemoryEntries[idx];
}

static inline void PushFreeBlock(HSD_MemoryEntry* entry, HSD_FreeList* block, BOOL new_block)
{
    block->next = entry->free_list;
    entry->free_list = block;
    if (new_block) {
        entry->nb_alloc += 1;
    }
    entry->nb_free += 1;
}

/**
 * @note Address: 0x801D5F80
 * @note Size: 0x1C8
 */
void* hsdAllocMemPiece(s32 size)
{
    HSD_MemoryEntry* entry;
    HSD_MemoryEntry* larger_entry;
    HSD_MemoryEntry* remainder_entry;
    HSD_FreeList* block;
    s32 entry_index;
    s32 remainder_index;
    void* allocation;

    entry_index = MemoryEntryIndex(size);
    entry = GetMemoryEntry(entry_index);
    if (entry == NULL) {
        return NULL;
    }
    block = entry->free_list;
    if (block != NULL) {
        entry->free_list = block->next;
        entry->nb_free -= 1;
        return block;
    }
    larger_entry = entry->next;
    while (larger_entry != NULL) {
        if (larger_entry->free_list != NULL) {
            HSD_FreeList* larger_block;
            HSD_FreeList* split_block;
            HSD_MemoryEntry* split_entry = GetMemoryEntry(MemoryEntryIndex((s32) (larger_entry->size - entry->size)));
            if (split_entry == NULL) {
                return NULL;
            }
            larger_block = larger_entry->free_list;
            larger_entry->free_list = larger_block->next;
            larger_entry->nb_free -= 1;
            larger_entry->nb_alloc -= 1;
            split_block = (HSD_FreeList*) ((u8*) larger_block + entry->size);
            PushFreeBlock(split_entry, split_block, TRUE);
            entry->nb_alloc += 1;
            return larger_block;
        }
        larger_entry = larger_entry->next;
    }
    remainder_index = (ClassMemoryEntryCount - entry_index) - 2;
    if (remainder_index >= 0) {
        remainder_entry = GetMemoryEntry(remainder_index);
        if (remainder_entry == NULL) {
            return NULL;
        }
    }
    allocation = _HSD_MemAlloc(ClassMemoryEntryCount * MEMORY_ENTRY_SIZE_STEP, 4, 1);
    if (allocation == NULL) {
        return NULL;
    }
    if (remainder_index >= 0) {
        HSD_FreeList* remainder = (HSD_FreeList*) ((u8*) allocation + entry->size);
        PushFreeBlock(remainder_entry, remainder, TRUE);
    }
    entry->nb_alloc += 1;
    return allocation;
}

/**
 * @note Address: 0x801D6148
 * @note Size: 0x58
 */
void hsdFreeMemPiece(void* block, s32 size)
{
    HSD_FreeList* list = block;
    if (list != NULL) {
        HSD_MemoryEntry* entry = GetMemoryEntry(MemoryEntryIndex(size));
        PushFreeBlock(entry, list, FALSE);
    }
}

/**
 * @note Address: 0x801D61A0
 * @note Size: 0x58
 */
HSD_Class* _hsdClassAlloc(HSD_ClassInfo* info)
{
    HSD_Class* ret = hsdAllocMemPiece(info->head.obj_size);
    if (ret != NULL) {
        info->head.nb_exist += 1;
        if (info->head.nb_exist > info->head.nb_peak) {
            info->head.nb_peak = info->head.nb_exist;
        }
    }
    return ret;
}

/**
 * @note Address: 0x801D61F8
 * @note Size: 0x8
 */
int _hsdClassInit(HSD_Class* object)
{
    return 0;
}

/**
 * @note Address: 0x801D6200
 * @note Size: 0x4
 */
void _hsdClassRelease(HSD_Class* object)
{
}

/**
 * @note Address: 0x801D6204
 * @note Size: 0x6C
 */
void _hsdClassDestroy(HSD_Class* c)
{
    HSD_ClassInfo* info = c->class_info;

    info->head.nb_exist -= 1;
    hsdFreeMemPiece(c, info->head.obj_size);
}

/**
 * @note Address: 0x801D6270
 * @note Size: 0x2C
 */
void _hsdClassAmnesia(HSD_ClassInfo* info)
{
    info->head.nb_exist = 0;
    info->head.nb_peak = 0;
    if (info == &hsdClass) {
        ClassMemoryEntryCount = 0;
        ClassMemoryEntries = NULL;
        ClassInfoHash = NULL;
    }
}

/**
 * @note Address: 0x801D629C
 * @note Size: 0x9C
 */
void _hsdClassInfoInit(void)
{
    hsdClass.head.flags = 1;
    hsdClass.head.library_name = "sysdolphin_base_library";
    hsdClass.head.class_name = "hsd_class";
    hsdClass.head.obj_size = sizeof(HSD_Class);
    hsdClass.head.info_size = sizeof(HSD_ClassInfo);
    hsdClass.head.parent = NULL;
    hsdClass.head.child = NULL;
    hsdClass.head.next = NULL;
    hsdClass.head.nb_exist = 0;
    hsdClass.head.nb_peak = 0;

    hsdClass.alloc = _hsdClassAlloc;
    hsdClass.init = _hsdClassInit;
    hsdClass.release = _hsdClassRelease;
    hsdClass.destroy = _hsdClassDestroy;
    hsdClass.amnesia = _hsdClassAmnesia;
}

/**
 * @note Address: 0x801D6338
 * @note Size: 0xC8
 */
void* hsdNew(HSD_ClassInfo* info)
{
    HSD_Class* cls;

    if (!(info->head.flags & 1)) {
        info->head.info_init();
    }
    cls = info->alloc(info);
    if (cls == NULL) {
        return NULL;
    }
    if (!(info->head.flags & 1)) {
        info->head.info_init();
    }
    memset(cls, 0, info->head.obj_size);
    cls->class_info = info;
    if (info->init(cls) < 0) {
        info->destroy(cls);
        return NULL;
    }
    return cls;
}

/**
 * @note Address: N/A
 * @note Size: 0x138
 */
BOOL hsdChangeClass(HSD_Class* object, HSD_ClassInfo* class_info)
{
    HSD_ClassInfo* info1;
    HSD_ClassInfo* info2;

    assert_line(586, object);
    assert_line(587, class_info);

    info1 = object->class_info;
    info2 = class_info;
    if (!(class_info->head.flags & 1)) {
        class_info->head.info_init();
    }
    if (info1->head.obj_size != info2->head.obj_size) {
        return FALSE;
    }
    while (info1->head.parent != NULL && info1->head.parent->head.obj_size == info1->head.obj_size) {
        info1 = info1->head.parent;
    }
    while (info2->head.parent != NULL && info2->head.parent->head.obj_size == info2->head.obj_size) {
        info2 = info2->head.parent;
    }
    if (info1 == info2) {
        info1->head.nb_exist -= 1;
        class_info->head.nb_exist += 1;
        if (class_info->head.nb_exist > class_info->head.nb_peak) {
            class_info->head.nb_peak = class_info->head.nb_exist;
        }
        object->class_info = class_info;
        return TRUE;
    }
    return FALSE;
}

/**
 * @note Address: 0x801D6400
 * @note Size: 0xA0
 */
BOOL hsdIsDescendantOf(HSD_ClassInfo* info, HSD_ClassInfo* class_info)
{
    HSD_ClassInfo* ancestor;

    if (info == NULL || class_info == NULL) {
        return FALSE;
    }
    ancestor = info;
    if (!(ancestor->head.flags & 1)) {
        ancestor->head.info_init();
    }
    if (!(class_info->head.flags & 1)) {
        class_info->head.info_init();
    }
    for (; ancestor != NULL; ancestor = ancestor->head.parent) {
        if (ancestor == class_info) {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * @note Address: N/A
 * @note Size: 0x88
 */
BOOL hsdObjIsDescendantOf(HSD_Class* cls, HSD_ClassInfo* class_info)
{
    HSD_ClassInfo* ancestor;

    if (cls == NULL || class_info == NULL) {
        return FALSE;
    }
    ancestor = cls->class_info;
    if (!(class_info->head.flags & 1)) {
        class_info->head.info_init();
    }
    while (ancestor != NULL) {
        if (ancestor == class_info) {
            return TRUE;
        }
        ancestor = ancestor->head.parent;
    }
    return FALSE;
}

/**
 * @note Address: N/A
 * @note Size: 0x14
 */
void class_set_flags(HSD_ClassInfo* class_info, s32 arg1, s32 arg2)
{
    class_info->head.flags = class_info->head.flags & ~arg2 | arg1;
}

/**
 * @note Address: N/A
 * @note Size: 0x1E4
 */
void ForgetClassLibraryReal(HSD_ClassInfo* class_info)
{
    HSD_ClassInfo* cur = class_info->head.child;
    HSD_ClassInfo* next;
    while (cur != NULL) {
        next = cur->head.next;
        cur->head.next = NULL;
        ForgetClassLibraryReal(cur);
        cur = next;
    }
    class_info->amnesia(class_info);
    class_info->head.child = NULL;
    class_info->head.parent = NULL;
    class_set_flags(class_info, 0, 1);
}

/**
 * @note Address: N/A
 * @note Size: 0x78
 */
void ForgetClassLibraryChild(char* name, HSD_ClassInfo* class_info)
{
    HSD_ClassInfo** cur = &class_info->head.child;
    while (*cur != NULL) {
        if (strcmp(name, (*cur)->head.library_name) == 0) {
            ForgetClassLibraryReal(*cur);
            *cur = (*cur)->head.next;
        } else {
            cur = &(*cur)->head.next;
        }
    }
}

/**
 * @note Address: N/A
 * @note Size: 0x88
 */
void hsdForgetClassLibrary(char* name)
{
    if (name == NULL) {
        name = "sysdolphin_base_library";
    }
    if (!(hsdClass.head.flags & 1)) {
        return;
    }
    if (strcmp(name, hsdClass.head.library_name) == 0) {
        ClassInfoHash = NULL;
        ForgetClassLibraryReal(&hsdClass);
    } else {
        ForgetClassLibraryChild(name, &hsdClass);
    }
}

/**
 * @note Address: 0x801D64A0
 * @note Size: 0x40
 */
HSD_ClassInfo* hsdSearchClassInfo(char* class_name)
{
    if (ClassInfoHash != NULL) {
        return HSD_HashSearch(ClassInfoHash, class_name, NULL);
    }
    return NULL;
}

/**
 * @note Address: N/A
 * @note Size: 0xB0
 */
void DumpClassStat(HSD_ClassInfo* info, s32 indent)
{
    OSReport_PrintSpaces(indent);
    OSReport("<class %s>\n", info->head.class_name);
    OSReport_PrintSpaces(indent);
    OSReport("    info %d object %d nb_exist %d nb_peak %d\n",
        info->head.info_size, info->head.obj_size, info->head.nb_exist, info->head.nb_peak);
}

/**
 * @note Address: N/A
 * @note Size: 0x31C
 */
void hsdDumpClassStat(HSD_ClassInfo* info, BOOL recursive, s32 indent)
{
    if (info == NULL) {
        hsdDumpClassStat(&hsdClass, TRUE, indent);
    } else if (info->head.flags & 1) {
        DumpClassStat(info, indent);
        if (recursive) {
            indent += 2;
            info = info->head.child;
            while (info != NULL) {
                hsdDumpClassStat(info, TRUE, indent);
                info = info->head.next;
            }
        }
    }
}
