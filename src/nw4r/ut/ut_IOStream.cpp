#include <nw4r/ut/ut_IOStream.h>

namespace nw4r {
namespace ut {

NW4R_UT_RTTI_DEF_BASE(IOStream);

bool IOStream::ReadAsync(void* pDst, u32 size, StreamCallback pCallback,
                         void* pCallbackArg) {
    return false;
}

bool IOStream::Write(const void* pSrc, u32 size) {
    return false;
}

bool IOStream::WriteAsync(const void* pSrc, u32 size, StreamCallback pCallback,
                          void* pCallbackArg) {
    return false;
}

bool IOStream::IsBusy() const {
    return false;
}

} // namespace ut
} // namespace nw4r
