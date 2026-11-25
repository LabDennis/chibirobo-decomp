#include <sysdolphin/archive.h>
#include <sysdolphin/memory.h>

static inline void RelocateArchive(HSD_Archive* archive)
{
    u32 i;

    for (i = 0; i < archive->header.nb_reloc; i++) {
        u32* address = (u32*)(archive->data + archive->reloc_info[i].offset);
        *address += (u32)archive->data;
    }
}

/**
 * @note Address: 0x801D4360
 * @note Size: 0x16C
 */
s32 HSD_ArchiveParse(HSD_Archive* archive, u8* src, u32 file_size)
{
    u32 offset;

    if (archive == NULL) {
        return -1;
    }

    memset(archive, 0, sizeof(HSD_Archive));
    archive->flags |= HSD_ARCHIVE_DONT_FREE;
    memcpy(archive, src, sizeof(HSD_ArchiveHeader));

    if (archive->header.file_size != file_size) {
        OSReport("HSD_ArchiveParse: byte-order mismatch! Please check data format\n");
        return -1;
    }

    offset = sizeof(HSD_ArchiveHeader);
    if (archive->header.data_size != 0) {
        archive->data = src + offset;
        offset += archive->header.data_size;
    }
    if (archive->header.nb_reloc != 0) {
        archive->reloc_info = (HSD_ArchiveRelocationInfo*)(src + offset);
        offset += archive->header.nb_reloc * sizeof(*archive->reloc_info);
    }
    if (archive->header.nb_public != 0) {
        archive->public_info = (HSD_ArchivePublicInfo*)(src + offset);
        offset += archive->header.nb_public * sizeof(*archive->public_info);
    }
    if (archive->header.nb_extern != 0) {
        archive->extern_info = (HSD_ArchiveExternInfo*)(src + offset);
        offset += archive->header.nb_extern * sizeof(*archive->extern_info);
    }
    if (offset < archive->header.file_size) {
        archive->symbols = (char*)(src + offset);
    }

    archive->top_ptr = src;
    RelocateArchive(archive);

    return 0;
}

/**
 * @note Address: 0x801D44CC
 * @note Size: 0x38
 */
char* HSD_ArchiveGetExtern(HSD_Archive* archive, s32 index)
{
    if (index < 0 || archive->header.nb_public <= index) {
        return NULL;
    }
    return archive->symbols + archive->public_info[index].symbol;
}

/**
 * @note Address: 0x801D4504
 * @note Size: 0xA4
 */
void* HSD_ArchiveGetPublicAddress(HSD_Archive* archive, char* name)
{
    u32 i;

    for (i = 0; i < archive->header.nb_public; i++) {
        if (strcmp(archive->symbols + archive->public_info[i].symbol, name) == 0) {
            return archive->data + archive->public_info[i].offset;
        }
    }

    return NULL;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void HSD_ArchiveLocateExtern(HSD_Archive* archive, char* name, void* addr)
{
    u32 offset;
    u32 i;

    offset = -1;
    for (i = 0; i < archive->header.nb_extern; i++) {
        if (strcmp(name, archive->symbols + archive->extern_info[i].symbol) == 0) {
            offset = archive->extern_info[i].offset;
            break;
        }
    }

    while (offset != -1 && offset < archive->header.data_size) {
        u32* reference = (u32*)(archive->data + offset);

        offset = *reference;
        *reference = (u32)addr;
    }
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void HSD_ArchiveFree(HSD_Archive* archive)
{
    if (archive != NULL) {
        if (!(archive->flags & HSD_ARCHIVE_DONT_FREE)) {
            if (archive->data != NULL) {
                HSD_Free(archive->data);
            }
            if (archive->reloc_info != NULL) {
                HSD_Free(archive->reloc_info);
            }
            if (archive->public_info != NULL) {
                HSD_Free(archive->public_info);
            }
            if (archive->extern_info != NULL) {
                HSD_Free(archive->extern_info);
            }
            if (archive->symbols != NULL) {
                HSD_Free(archive->symbols);
            }
            if (archive->name != NULL) {
                HSD_Free(archive->name);
            }
        }
        HSD_Free(archive);
    }
}
