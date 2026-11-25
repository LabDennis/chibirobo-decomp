#include <global.h>
#include <sysdolphin/id.h>

#include <dolphin/types.h>

#include <string.h>

HSD_ObjAllocData hsd_iddata;
HSD_IDTable default_table;

/**
 * @note Address: 0x801D2C88
 * @note Size: 0x30
 */
void HSD_IDInitAllocData(void)
{
    HSD_ObjAllocInit(&hsd_iddata, sizeof(IDEntry), 4);
}

/**
 * @note Address: N/A
 * @note Size: 0xC
 */
HSD_ObjAllocData* HSD_IDGetAllocData(void)
{
    return &hsd_iddata;
}

/**
 * @note Address: 0x801D2CB8
 * @note Size: 0x30
 */
void HSD_IDSetup(void)
{
    memset(&default_table, 0, sizeof(default_table));
}

static inline u32 IDHash(u32 id)
{
    return id % ARRAY_SIZE(default_table.table);
}

static inline IDEntry* IDEntryAlloc(void)
{
    IDEntry* entry;

    entry = HSD_ObjAlloc(&hsd_iddata);
    assert_line(67, entry);
    memset(entry, 0, sizeof(*entry));

    return entry;
}

/**
 * @note Address: 0x801D2CE8
 * @note Size: 0xEC
 */
void HSD_IDInsertToTable(HSD_IDTable* table, u32 id, void* data)
{
    IDEntry* entry;

    if (table == NULL) {
        table = &default_table;
    }

    entry = table->table[IDHash(id)];
    while (entry != NULL) {
        if (entry->id == id) {
            break;
        }
        entry = entry->next;
    }

    if (entry != NULL) {
        entry->id = id;
        entry->data = data;
    } else {
        entry = IDEntryAlloc();
        entry->id = id;
        entry->data = data;
        entry->next = table->table[IDHash(id)];
        table->table[IDHash(id)] = entry;
    }
}

static inline void IDEntryFree(IDEntry* entry)
{
    HSD_ObjFree(&hsd_iddata, entry);
}

/**
 * @note Address: 0x801D2DD4
 * @note Size: 0xAC
 */
void HSD_IDRemoveByIDFromTable(HSD_IDTable* table, u32 id)
{
    IDEntry* entry;
    IDEntry* prev;

    if (table == NULL) {
        table = &default_table;
    }

    prev = NULL;
    for (entry = table->table[IDHash(id)]; entry != NULL; entry = entry->next) {
        if (entry->id == id) {
            if (prev != NULL) {
                prev->next = entry->next;
            } else {
                table->table[IDHash(id)] = entry->next;
            }
            IDEntryFree(entry);
            return;
        }
        prev = entry;
    }
}

/**
 * @note Address: 0x801D2E80
 * @note Size: 0x88
 */
void* HSD_IDGetDataFromTable(HSD_IDTable* table, u32 id, s32* success)
{
    IDEntry* entry;

    if (table == NULL) {
        table = &default_table;
    }

    entry = table->table[IDHash(id)];
    while (entry != NULL) {
        if (entry->id == id) {
            if (success != NULL) {
                *success = 1;
            }
            return entry->data;
        }
        entry = entry->next;
    }

    if (success != NULL) {
        *success = 0;
    }
    return NULL;
}

/**
 * @note Address: N/A
 * @note Size: 0x30
 */
void _HSD_IDForgetMemory(void)
{
    HSD_IDSetup();
}
