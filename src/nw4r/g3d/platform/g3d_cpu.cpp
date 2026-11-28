#include <nw4r/g3d.h>

namespace nw4r {
namespace g3d {
namespace detail {

void Copy32ByteBlocks(void* pDst, const void* pSrc, u32 size) {
    f64* dst = static_cast<f64*>(pDst);
    const f64* src = static_cast<const f64*>(pSrc);

    for (size /= 32; size > 0; size--) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        dst += 4;
        src += 4;
    }
}

// NONMATCHING: retail uses paired-single psq_st stores
void ZeroMemory32ByteBlocks(void* pDst, u32 size) {
    f64* dst = static_cast<f64*>(pDst);

    for (size /= 32; size > 0; size--) {
        dst[0] = 0.0;
        dst[1] = 0.0;
        dst[2] = 0.0;
        dst[3] = 0.0;
        dst += 4;
    }
}

// NONMATCHING: retail uses paired-single psq_st stores
void ZeroMemory16ByteBlocks(void* pDst, u32 size) {
    f64* dst = static_cast<f64*>(pDst);

    for (size /= 16; size > 0; size--) {
        dst[0] = 0.0;
        dst[1] = 0.0;
        dst += 2;
    }
}

} // namespace detail
} // namespace g3d
} // namespace nw4r
