#ifndef NW4R_G3D_PLATFORM_CPU_H
#define NW4R_G3D_PLATFORM_CPU_H
#include <nw4r/types_nw4r.h>

#include "revolution/os/OSAlarm.h"
#include "Dolphin/os.h"
#include "revolution/os/OSFastCastGQR.h"

namespace nw4r {
namespace g3d {

/******************************************************************************
 *
 * Fastcast
 *
 ******************************************************************************/
namespace fastcast {

/******************************************************************************
 *
 * Convert from U8
 *
 ******************************************************************************/
inline f32 U8_0ToF32(const u8* pPtr) {
    f32 x;
    OSu8tof32(const_cast<u8*>(pPtr), &x);
    return x;
}

/******************************************************************************
 *
 * Convert from U16
 *
 ******************************************************************************/
inline f32 U16_0ToF32(const u16* pPtr) {
    f32 x;
    OSu16tof32(const_cast<u16*>(pPtr), &x);
    return x;
}

/******************************************************************************
 *
 * Convert from S16
 *
 ******************************************************************************/
inline f32 S7_8ToF32(const s16* pPtr) {
    return static_cast<f32>(*pPtr) * (1.0f / 256.0f);
}

inline f32 S10_5ToF32(const s16* pPtr) {
    return static_cast<f32>(*pPtr) * (1.0f / 32.0f);
}

/******************************************************************************
 *
 * Convert from F32
 *
 ******************************************************************************/
inline u8 F32ToU8_0(f32 f) {
    u8 x;
    OSf32tou8(&f, &x);
    return x;
}

inline s16 F32ToS10_5(f32 f) {
    return static_cast<s16>(f * 32.0f);
}

/******************************************************************************
 *
 * GQR
 *
 ******************************************************************************/
inline void SetGQR6_S10_5() {
    OSSetGQR6(OS_GQR_TYPE_S16, 5);
}
inline void SetGQR7_S7_8() {
    OSSetGQR7(OS_GQR_TYPE_S16, 8);
}

/******************************************************************************
 *
 * Initialization
 *
 ******************************************************************************/
namespace detail {

inline void Init() {
    OSInitFastCast();
    SetGQR6_S10_5();
    SetGQR7_S7_8();
}

} // namespace detail
} // namespace fastcast

/******************************************************************************
 *
 * Cache
 *
 ******************************************************************************/
namespace DC {

inline void StoreRange(void* pBase, u32 size) {
    DCStoreRange(pBase, size);
}

inline void StoreRangeNoSync(void* pBase, u32 size) {
    DCStoreRangeNoSync(pBase, size);
}

inline void FlushRangeNoSync(void* pBase, u32 size) {
    DCFlushRangeNoSync(pBase, size);
}

inline void InvalidateRange(void* pBase, u32 size) {
    DCInvalidateRange(pBase, size);
}

} // namespace DC

/******************************************************************************
 *
 * Memory
 *
 ******************************************************************************/
namespace detail {

void Copy32ByteBlocks(void* pDst, const void* pSrc, u32 size);
void ZeroMemory32ByteBlocks(void* pDst, u32 size);
void ZeroMemory16ByteBlocks(void* pDst, u32 size);

} // namespace detail

/******************************************************************************
 *
 * Initialize fastcast
 *
 ******************************************************************************/
inline void InitFastCast() {
    fastcast::detail::Init();
}

} // namespace g3d
} // namespace nw4r

#endif
