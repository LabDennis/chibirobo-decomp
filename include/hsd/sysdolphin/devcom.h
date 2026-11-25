#ifndef _devcom_h_
#define _devcom_h_

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    HSD_DEVCOM_CLEAR_ARAM = 0x03,
    HSD_DEVCOM_MRAM_TO_ARAM = 0x0B,
    HSD_DEVCOM_ARAM_TO_MRAM = 0x19,
    HSD_DEVCOM_ARAM_TO_BUFFER = 0x1A,
    HSD_DEVCOM_ARAM_TO_ARAM = 0x1B,
    HSD_DEVCOM_DVD_TO_MRAM = 0x21,
    HSD_DEVCOM_DVD_TO_BUFFER = 0x22,
    HSD_DEVCOM_DVD_TO_ARAM = 0x23,
};

typedef struct HSD_DevCom HSD_DevCom;
// The buffer is supplied for transfers to a DevCom-owned buffer.
typedef void (*HSD_DevComCallback)(int requestID, void* context, void* buffer, BOOL cancelled);

HSD_DevCom* HSD_DevComIsBusy(int priority);
// Source and destination encode addresses or a DVD file offset according to type.
int HSD_DevComRequest(int file, u32 src, u32 dest, u32 size, int type, int priority, HSD_DevComCallback callback, void* context);

#ifdef __cplusplus
}
#endif

#endif
