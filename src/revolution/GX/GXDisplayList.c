#include "revolution/gx.h"

#include "revolution/gx/__gx.h"

void GXCallDisplayList(void* list, u32 nbytes) {
    if (__GXData->dirtyState != 0) {
        __GXSetDirtyState();
    }

    if (*(u32*)&__GXData->vNumNot == 0) {
        __GXSendFlushPrim();
    }

    GX_WRITE_U8(GX_CMD_CALL_DL);
    GX_WRITE_U32(list);
    GX_WRITE_U32(nbytes);
}
