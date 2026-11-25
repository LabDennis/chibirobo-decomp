#include <sysdolphin/gobjproc.h>
#include <dolphin/os.h>

typedef void (*HSD_PanicCallback)(void* context, ...);

void OSReport(const char* msg, ...);
void HSD_SaveContext(void);

OSContext hsd_saved_context;

static HSD_PanicCallback panicCallback;

/**
 * @note Address: 0x801DD248
 * @note Size: 0x5C
 */
void __assert(const char* file, unsigned long line, const char* cond)
{
    OSReport("assertion \"%s\" failed", cond);
    HSD_Panic(file, line, "");
}

/**
 * @note Address: 0x801DD2A4
 * @note Size: 0x98
 */
void HSD_Panic(const char* file, s32 line, const char* msg)
{
    if (panicCallback != NULL) {
        HSD_SaveContext();
        OSReport("%s in %s on line %d.\n", msg, file, line);
        panicCallback(&hsd_saved_context);
    }
    OSPanic(file, line, msg);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void HSD_SetPanicCallback(HSD_PanicCallback cb) { panicCallback = cb; }
