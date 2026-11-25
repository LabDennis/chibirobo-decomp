#ifndef _class_h_
#define _class_h_

#include <global.h>
#include <dolphin/types.h>

#define HSD_CLASS_INFO(o) ((HSD_ClassInfo*)o)
#define HSD_CLASS_METHOD(o) (((HSD_Class*)o)->class_info)
#define HSD_PARENT_INFO(o) ((o)->parent.head.parent)

#define next_p(p) (p != NULL ? p->next : NULL)

typedef struct _HSD_Class {
    struct _HSD_ClassInfo* class_info;
} HSD_Class;

typedef struct _HSD_ClassInfoHead {
    void (*info_init)();
    u32 flags;
    char* library_name;
    char* class_name;
    s16 obj_size;
    s16 info_size;
    struct _HSD_ClassInfo* parent;
    struct _HSD_ClassInfo* next;
    struct _HSD_ClassInfo* child;
    u32 nb_exist;
    u32 nb_peak;
} HSD_ClassInfoHead;

typedef struct _HSD_ClassInfo {
    struct _HSD_ClassInfoHead head;
    HSD_Class* (*alloc)(struct _HSD_ClassInfo* c);
    int (*init)(struct _HSD_Class* c);
    void (*release)(struct _HSD_Class* c);
    void (*destroy)(struct _HSD_Class* c);
    void (*amnesia)(struct _HSD_ClassInfo* c);
} HSD_ClassInfo;

static inline void hsdDelete(void* object)
{
    HSD_Class* instance = (HSD_Class*)object;
    if (instance != NULL) {
        instance->class_info->release(instance);
        instance->class_info->destroy(instance);
    }
}

extern HSD_ClassInfo hsdClass;

void _hsdClassInfoInit(void);
void hsdInitClassInfo(HSD_ClassInfo* class_info, HSD_ClassInfo* parent_info,
                      char* base_class_library, char* type,
                      s32 info_size, s32 class_size);
BOOL hsdIsDescendantOf(HSD_ClassInfo* info, HSD_ClassInfo* class_info);
BOOL hsdObjIsDescendantOf(HSD_Class* cls, HSD_ClassInfo* class_info);
BOOL hsdChangeClass(HSD_Class* object, HSD_ClassInfo* class_info);
HSD_ClassInfo* hsdSearchClassInfo(char* class_name);
void hsdForgetClassLibrary(char* name);
void hsdFreeMemPiece(void* block, s32 size);

#endif
