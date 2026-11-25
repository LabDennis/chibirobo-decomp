#include <global.h>
#include <sysdolphin/hash.h>

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline HSD_HashEntry* HashSearchEntry(HSD_Hash* hash, u32 idx, void* key)
{
    HSD_HashEntry* entry;

    if (hash->table[idx] == NULL) {
        return NULL;
    }

    for (entry = hash->table[idx]; entry != NULL; entry = entry->next) {
        if (hash->parent.class_info->keycheck(hash, entry->key, key) == 0) {
            return entry;
        }
    }
    return NULL;
}

/**
 * @note Address: 0x801D64E0
 * @note Size: 0x100
 */
void* HSD_HashSearch(HSD_Hash* hash, void* key, int* success)
{
    HSD_HashEntry* entry;
    u32 idx;

    idx = hash->parent.class_info->getidx(hash, key);
    assert_line(0x71, idx < hash->table_size);
    entry = HashSearchEntry(hash, idx, key);
    if (success != NULL) {
        *success = !!entry;
    }
    if (entry != NULL) {
        return entry->value;
    }
    return NULL;
}
