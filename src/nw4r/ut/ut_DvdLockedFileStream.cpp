// after ss (Skyward Sword decomp) ut_DvdLockedFileStream.cpp: the variant with the cancel flag and thread queue
#include <nw4r/ut/ut_DvdLockedFileStream.h>

namespace nw4r {
namespace ut {

NW4R_UT_RTTI_DEF_DERIVED(DvdLockedFileStream, DvdFileStream);

OSThreadQueue DvdLockedFileStream::sThreadQueue;
OSMutex DvdLockedFileStream::sMutex;
bool DvdLockedFileStream::sInitialized = false;

void DvdLockedFileStream::InitMutex_() {
    BOOL enabled = OSDisableInterrupts();

    if (!sInitialized) {
        OSInitMutex(&sMutex);
        OSInitThreadQueue(&sThreadQueue);
        sInitialized = true;
    }

    OSRestoreInterrupts(enabled);
}

DvdLockedFileStream::DvdLockedFileStream(s32 entrynum) : DvdFileStream(entrynum) {
    mCancelFlag = false;
    InitMutex_();
}

DvdLockedFileStream::DvdLockedFileStream(const DVDFileInfo *info, bool close) : DvdFileStream(info, close) {
    mCancelFlag = false;
    InitMutex_();
}

DvdLockedFileStream::~DvdLockedFileStream() {}

void DvdLockedFileStream::Close() {
    DvdFileStream::Close();
    mCancelFlag = false;
}

s32 DvdLockedFileStream::Read(void *dst, u32 size) {
    if (!LockMutex()) {
        return DVD_RESULT_CANCELED;
    }
    s32 ret = DvdFileStream::Read(dst, size);
    UnlockMutex();
    return ret;
}

s32 DvdLockedFileStream::Peek(void *dst, u32 size) {
    if (!LockMutex()) {
        return DVD_RESULT_CANCELED;
    }
    s32 ret = DvdFileStream::Peek(dst, size);
    UnlockMutex();
    return ret;
}

void DvdLockedFileStream::Cancel() {
    CancelMutex();
    DvdFileStream::Cancel();
}

bool DvdLockedFileStream::LockMutex() {
    BOOL enabled = OSDisableInterrupts();
    while (!OSTryLockMutex(&sMutex)) {
        OSSleepThread(&sThreadQueue);
        if (mCancelFlag) {
            OSRestoreInterrupts(enabled);
            return false;
        }
    };
    OSRestoreInterrupts(enabled);
    return true;
}

void DvdLockedFileStream::UnlockMutex() {
    BOOL enabled = OSDisableInterrupts();
    OSUnlockMutex(&sMutex);
    OSWakeupThread(&sThreadQueue);
    OSRestoreInterrupts(enabled);
}

void DvdLockedFileStream::CancelMutex() {
    BOOL enabled = OSDisableInterrupts();
    mCancelFlag = true;
    OSWakeupThread(&sThreadQueue);
    OSRestoreInterrupts(enabled);
}

} // namespace ut
} // namespace nw4r
