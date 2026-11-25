#include <dolphin/dolphin.h>
#include <dolphin/os.h>

#include "__os.h"

#define SHN_UNDEF 0
//      Name                    Value       Field   Calculation
#define R_PPC_NONE 0            //  none    none
#define R_PPC_ADDR32 1          //  word32  S + A
#define R_PPC_ADDR24 2          //  low24*  (S + A) >> 2
#define R_PPC_ADDR16 3          //  half16* S + A
#define R_PPC_ADDR16_LO 4       //  half16  #lo(S + A)
#define R_PPC_ADDR16_HI 5       //  half16  #hi(S + A)
#define R_PPC_ADDR16_HA 6       //  half16  #ha(S + A)
#define R_PPC_ADDR14 7          //  low14*  (S + A) >> 2
#define R_PPC_ADDR14_BRTAKEN 8  //  low14*  (S + A) >> 2
#define R_PPC_ADDR14_BRNTAKEN 9 //  low14*  (S + A) >> 2
#define R_PPC_REL24 10          //  low24*  (S + A - P) >> 2
#define R_PPC_REL14 11          //  low14*  (S + A - P) >> 2
#define R_PPC_REL14_BRTAKEN 12  //  low14*  (S + A - P) >> 2
#define R_PPC_REL14_BRNTAKEN 13 //  low14*  (S + A - P) >> 2

#define PPC_BRANCH_DISPLACEMENT 0x03fffffcu
#define PPC_COND_BRANCH_DISPLACEMENT 0x0000fffcu

OSModuleQueue __OSModuleInfoList AT_ADDRESS(OS_BASE_CACHED | 0x30C8);
const void* __OSStringTable AT_ADDRESS(OS_BASE_CACHED | 0x30D0);

/**
 * @note Address: N/A
 * @note Size: ?
 */
void OSSetStringTable(void* stringTable) {
    __OSStringTable = stringTable;
}

/**
 * @note Address: 0x801676C4
 * @note Size: 0x2BC
 */
static BOOL Relocate(OSModuleHeader* newModule, OSModuleHeader* module) {
    OSModuleID idNew;
    OSImportInfo* imp;
    OSRel* rel;
    OSSectionInfo* si;
    OSSectionInfo* siFlush;
    u32* p;
    u32 offset;
    u32 x;

    idNew = newModule ? newModule->info.id : 0;
    for (imp = (OSImportInfo*)module->impOffset;
         imp < (OSImportInfo*)(module->impOffset + module->impSize); imp++)
    {
        if (imp->id == idNew) {
            goto Found;
        }
    }
    return FALSE;

Found:
    siFlush = 0;
    for (rel = (OSRel*)imp->offset; rel->type != R_DOLPHIN_END; rel++) {
        p = (u32*)((u8*)p + rel->offset);
        if (idNew) {
            si = &OSGetSectionInfo(newModule)[rel->section];
            offset = OS_SECTIONINFO_OFFSET(si->offset);
        } else {
            offset = 0;
        }

        switch (rel->type) {
        case R_PPC_NONE:
            break;
        case R_PPC_ADDR32:
            x = offset + rel->addend;
            *p = x;
            break;
        case R_PPC_ADDR24:
            x = offset + rel->addend;
            *p = (*p & ~PPC_BRANCH_DISPLACEMENT) | (x & PPC_BRANCH_DISPLACEMENT);
            break;
        case R_PPC_ADDR16:
            x = offset + rel->addend;
            *(u16*)p = (u16)(x & 0xffff);
            break;
        case R_PPC_ADDR16_LO:
            x = offset + rel->addend;
            *(u16*)p = (u16)(x & 0xffff);
            break;
        case R_PPC_ADDR16_HI:
            x = offset + rel->addend;
            *(u16*)p = (u16)(((x >> 16) & 0xffff));
            break;
        case R_PPC_ADDR16_HA:
            x = offset + rel->addend;
            *(u16*)p = (u16)(((x >> 16) + ((x & 0x8000) ? 1 : 0)) & 0xffff);
            break;
        case R_PPC_ADDR14:
        case R_PPC_ADDR14_BRTAKEN:
        case R_PPC_ADDR14_BRNTAKEN:
            x = offset + rel->addend;
            *p = (*p & ~PPC_COND_BRANCH_DISPLACEMENT) | (x & PPC_COND_BRANCH_DISPLACEMENT);
            break;
        case R_PPC_REL24:
            x = offset + rel->addend - (u32)p;
            *p = (*p & ~PPC_BRANCH_DISPLACEMENT) | (x & PPC_BRANCH_DISPLACEMENT);
            break;
        case R_PPC_REL14:
        case R_PPC_REL14_BRTAKEN:
        case R_PPC_REL14_BRNTAKEN:
            x = offset + rel->addend - (u32)p;
            *p = (*p & ~PPC_COND_BRANCH_DISPLACEMENT) | (x & PPC_COND_BRANCH_DISPLACEMENT);
            break;
        case R_DOLPHIN_NOP:
            break;
        case R_DOLPHIN_SECTION:
            si = &OSGetSectionInfo(module)[rel->section];
            p = (u32*)OS_SECTIONINFO_OFFSET(si->offset);
            if (siFlush) {
                offset = OS_SECTIONINFO_OFFSET(siFlush->offset);
                DCFlushRange((void*)offset, siFlush->size);
                ICInvalidateRange((void*)offset, siFlush->size);
            }
            siFlush = (si->offset & OS_SECTIONINFO_EXEC) ? si : 0;
            break;
        default:
            OSReport("OSLink: unknown relocation type %3d\n", rel->type);
            break;
        }
    }

    if (siFlush) {
        offset = OS_SECTIONINFO_OFFSET(siFlush->offset);
        DCFlushRange((void*)offset, siFlush->size);
        ICInvalidateRange((void*)offset, siFlush->size);
    }

    return TRUE;
}

/**
 * @note Address: 0x80167980
 * @note Size: 0x2E0
 */
static BOOL Link(OSModuleInfo* newModule, void* bss, BOOL fixed) {
    u32 i;
    OSSectionInfo* si;
    OSModuleHeader* moduleHeader;
    OSModuleInfo* moduleInfo;
    OSImportInfo* imp;

    ASSERTLINE(282, newModule->version <= OS_MODULE_VERSION);

    moduleHeader = (OSModuleHeader*)newModule;
    moduleHeader->bssSection = 0;

    ASSERTLINE(290, newModule->version < 2 || moduleHeader->align == 0 || (u32) newModule % moduleHeader->align == 0);
    ASSERTLINE(293, newModule->version < 2 || moduleHeader->bssAlign == 0 || (u32) bss % moduleHeader->bssAlign == 0);

    if (OS_MODULE_VERSION < newModule->version ||
        (newModule->version >= 2 &&
         ((moduleHeader->align && (u32)newModule % moduleHeader->align != 0) ||
          (moduleHeader->bssAlign && (u32)bss % moduleHeader->bssAlign != 0))))
    {
        return FALSE;
    }

    {
        OSModuleInfo* prev = __OSModuleInfoList.tail;

        if (prev == NULL) {
            __OSModuleInfoList.head = newModule;
        } else {
            prev->link.next = newModule;
        }
        newModule->link.prev = prev;
        newModule->link.next = NULL;
        __OSModuleInfoList.tail = newModule;
    }
    newModule->sectionInfoOffset += (u32)moduleHeader;
    moduleHeader->relOffset += (u32)moduleHeader;
    moduleHeader->impOffset += (u32)moduleHeader;
    if (3 <= newModule->version) {
        moduleHeader->fixSize += (u32)moduleHeader;
    }

    for (i = 1; i < newModule->numSections; i++) {
        si = &OSGetSectionInfo(newModule)[i];
        if (si->offset != 0) {
            si->offset += (u32)moduleHeader;
        } else if (si->size != 0) {
            ASSERTLINE(326, moduleHeader->bssSection == 0);
            moduleHeader->bssSection = (u8)i;
            si->offset = (u32)bss;
        }
    }

    for (imp = (OSImportInfo*)moduleHeader->impOffset;
         imp < (OSImportInfo*)(moduleHeader->impOffset + moduleHeader->impSize); imp++)
    {
        imp->offset += (u32)moduleHeader;
    }

    if (moduleHeader->prologSection != SHN_UNDEF) {
        moduleHeader->prolog +=
            OS_SECTIONINFO_OFFSET(OSGetSectionInfo(newModule)[moduleHeader->prologSection].offset);
    }

    if (moduleHeader->epilogSection != SHN_UNDEF) {
        moduleHeader->epilog +=
            OS_SECTIONINFO_OFFSET(OSGetSectionInfo(newModule)[moduleHeader->epilogSection].offset);
    }

    if (moduleHeader->unresolvedSection != SHN_UNDEF) {
        moduleHeader->unresolved += OS_SECTIONINFO_OFFSET(
            OSGetSectionInfo(newModule)[moduleHeader->unresolvedSection].offset);
    }

    if (__OSStringTable) {
        newModule->nameOffset += (u32)__OSStringTable;
    }

    Relocate(0, moduleHeader);

    for (moduleInfo = __OSModuleInfoList.head; moduleInfo; moduleInfo = moduleInfo->link.next) {
        Relocate(moduleHeader, (OSModuleHeader*)moduleInfo);
        if (moduleInfo != newModule) {
            Relocate((OSModuleHeader*)moduleInfo, moduleHeader);
        }
    }

    if (fixed) {
        for (imp = (OSImportInfo*)moduleHeader->impOffset;
             imp < (OSImportInfo*)(moduleHeader->impOffset + moduleHeader->impSize); imp++)
        {
            if (imp->id == 0 || imp->id == newModule->id) {
                moduleHeader->impSize = (u32)((u8*)imp - (u8*)moduleHeader->impOffset);
                break;
            }
        }
    }

    memset(bss, 0, moduleHeader->bssSize);

    OSNotifyLink(newModule);
    return TRUE;
}

/**
 * @note Address: 0x80167C60
 * @note Size: 0x24
 */
BOOL OSLink(OSModuleInfo* newModule, void* bss) {
    return Link(newModule, bss, FALSE);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
BOOL OSLinkFixed(OSModuleInfo* newModule, void* bss) {
    ASSERTLINE(400, newModule->version <= OS_MODULE_VERSION && 3 <= newModule->version);

    if (OS_MODULE_VERSION < newModule->version || newModule->version < 3) {
        return FALSE;
    }
    return Link(newModule, bss, TRUE);
}

/**
 * @note Address: 0x80167C84
 * @note Size: 0x238
 */
static BOOL Undo(OSModuleHeader* newModule, OSModuleHeader* module) {
    OSModuleID idNew;
    OSImportInfo* imp;
    OSRel* rel;
    OSSectionInfo* si;
    OSSectionInfo* siFlush;
    u32* p;
    u32 offset;
    u32 x;

    ASSERTLINE(434, newModule);

    idNew = newModule->info.id;
    ASSERTLINE(436, idNew);
    
    for (imp = (OSImportInfo*)module->impOffset;
         imp < (OSImportInfo*)(module->impOffset + module->impSize); imp++)
    {
        if (imp->id == idNew) {
            goto Found;
        }
    }
    return FALSE;

Found:
    siFlush = 0;
    for (rel = (OSRel*)imp->offset; rel->type != R_DOLPHIN_END; rel++) {
        p = (u32*)((u8*)p + rel->offset);
        x = 0;
        switch (rel->type) {
        case R_PPC_NONE:
            break;
        case R_PPC_ADDR32:
            *p = x;
            break;
        case R_PPC_ADDR24:
            *p = (*p & ~PPC_BRANCH_DISPLACEMENT) | (x & PPC_BRANCH_DISPLACEMENT);
            break;
        case R_PPC_ADDR16:
            *(u16*)p = (u16)(x & 0xffff);
            break;
        case R_PPC_ADDR16_LO:
            *(u16*)p = (u16)(x & 0xffff);
            break;
        case R_PPC_ADDR16_HI:
            *(u16*)p = (u16)(((x >> 16) & 0xffff));
            break;
        case R_PPC_ADDR16_HA:
            *(u16*)p = (u16)(((x >> 16) + ((x & 0x8000) ? 1 : 0)) & 0xffff);
            break;
        case R_PPC_ADDR14:
        case R_PPC_ADDR14_BRTAKEN:
        case R_PPC_ADDR14_BRNTAKEN:
            *p = (*p & ~PPC_COND_BRANCH_DISPLACEMENT) | (x & PPC_COND_BRANCH_DISPLACEMENT);
            break;
        case R_PPC_REL24:
            if (module->unresolvedSection != SHN_UNDEF) {
                x = (u32)module->unresolved - (u32)p;
            }
            *p = (*p & ~PPC_BRANCH_DISPLACEMENT) | (x & PPC_BRANCH_DISPLACEMENT);
            break;
        case R_PPC_REL14:
        case R_PPC_REL14_BRTAKEN:
        case R_PPC_REL14_BRNTAKEN:
            *p = (*p & ~PPC_COND_BRANCH_DISPLACEMENT) | (x & PPC_COND_BRANCH_DISPLACEMENT);
            break;
        case R_DOLPHIN_NOP:
            break;
        case R_DOLPHIN_SECTION:
            si = &OSGetSectionInfo(module)[rel->section];
            p = (u32*)OS_SECTIONINFO_OFFSET(si->offset);
            if (siFlush) {
                offset = OS_SECTIONINFO_OFFSET(siFlush->offset);
                DCFlushRange((void*)offset, siFlush->size);
                ICInvalidateRange((void*)offset, siFlush->size);
            }
            siFlush = (si->offset & OS_SECTIONINFO_EXEC) ? si : 0;
            break;
        default:
            OSReport("OSUnlink: unknown relocation type %3d\n", rel->type);
            break;
        }
    }

    if (siFlush) {
        offset = OS_SECTIONINFO_OFFSET(siFlush->offset);
        DCFlushRange((void*)offset, siFlush->size);
        ICInvalidateRange((void*)offset, siFlush->size);
    }

    return TRUE;
}

/**
 * @note Address: 0x80167EBC
 * @note Size: 0x1D4
 */
BOOL OSUnlink(OSModuleInfo* oldModule) {
    OSModuleHeader* moduleHeader;
    OSModuleInfo* moduleInfo;
    u32 i;
    OSSectionInfo* si;
    OSImportInfo* imp;

    ASSERTLINE(546, oldModule->version <= OS_MODULE_VERSION);

    moduleHeader = (OSModuleHeader*)oldModule;

    {
        OSModuleInfo* next = oldModule->link.next;
        OSModuleInfo* prev = oldModule->link.prev;

        if (next == NULL) {
            __OSModuleInfoList.tail = prev;
        } else {
            next->link.prev = prev;
        }
        if (prev == NULL) {
            __OSModuleInfoList.head = next;
        } else {
            prev->link.next = next;
        }
    }

    for (moduleInfo = __OSModuleInfoList.head; moduleInfo; moduleInfo = moduleInfo->link.next) {
        Undo(moduleHeader, (OSModuleHeader*)moduleInfo);
    }

    OSNotifyUnlink(oldModule);

    if (__OSStringTable) {
        oldModule->nameOffset -= (u32)__OSStringTable;
    }

    if (moduleHeader->prologSection != SHN_UNDEF) {
        moduleHeader->prolog -=
            OS_SECTIONINFO_OFFSET(OSGetSectionInfo(oldModule)[moduleHeader->prologSection].offset);
    }

    if (moduleHeader->epilogSection != SHN_UNDEF) {
        moduleHeader->epilog -=
            OS_SECTIONINFO_OFFSET(OSGetSectionInfo(oldModule)[moduleHeader->epilogSection].offset);
    }

    if (moduleHeader->unresolvedSection != SHN_UNDEF) {
        moduleHeader->unresolved -= OS_SECTIONINFO_OFFSET(
            OSGetSectionInfo(oldModule)[moduleHeader->unresolvedSection].offset);
    }

    for (imp = (OSImportInfo*)moduleHeader->impOffset;
         imp < (OSImportInfo*)(moduleHeader->impOffset + moduleHeader->impSize); imp++)
    {
        imp->offset -= (u32)moduleHeader;
    }

    for (i = 1; i < oldModule->numSections; i++) {
        si = &OSGetSectionInfo(oldModule)[i];
        if (i == moduleHeader->bssSection) {
            ASSERTLINE(589, si->size != 0);
            moduleHeader->bssSection = 0;
            si->offset = 0;
        } else if (si->offset != 0) {
            si->offset -= (u32)moduleHeader;
        }
    }
    moduleHeader->relOffset -= (u32)moduleHeader;
    moduleHeader->impOffset -= (u32)moduleHeader;
    oldModule->sectionInfoOffset -= (u32)moduleHeader;

    return TRUE;
}

/**
 * @note Address: 0x80168090
 * @note Size: 0x18
 */
void __OSModuleInit(void) {
    __OSModuleInfoList.head = __OSModuleInfoList.tail = 0;
    __OSStringTable = 0;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
OSModuleInfo* OSSearchModule(void* ptr, u32* section, u32* offset) {
    OSModuleInfo* moduleInfo;
    OSSectionInfo* sectionInfo;
    u32 i;
    u32 baseSection;

    if (ptr == NULL) {
        return NULL;
    }

    moduleInfo = __OSModuleInfoList.head;
    while (moduleInfo != 0) {
        sectionInfo = (OSSectionInfo*)moduleInfo->sectionInfoOffset;
        for (i = 0; i < moduleInfo->numSections; i++) {
            if (sectionInfo->size != 0) {
                baseSection = OS_SECTIONINFO_OFFSET(sectionInfo->offset);
                if (baseSection <= (u32)ptr && (u32)ptr < baseSection + sectionInfo->size) {
                    if (section != 0) {
                        *section = i;
                    }

                    if (offset != 0) {
                        *offset = (u32)ptr - baseSection;
                    }

                    return moduleInfo;
                }
            }
            sectionInfo++;
        }
        moduleInfo = moduleInfo->link.next;
    }

    return NULL;
}
