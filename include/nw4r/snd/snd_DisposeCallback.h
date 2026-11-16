#ifndef NW4R_SND_DISPOSE_CALLBACK_H
#define NW4R_SND_DISPOSE_CALLBACK_H
#include "nw4r/types_nw4r.h"
#include "nw4r/snd/snd_global.h"
#include "nw4r/ut/ut_LinkList.h"
#include "nw4r/ut/ut_algorithm.h"
#include "nw4r/ut/ut_lock.h"
#include "nw4r/ut/ut_FileStream.h"


namespace nw4r {
namespace snd {
namespace detail {

class DisposeCallback {
public:
    NW4R_UT_LINKLIST_NODE_DECL(); // at 0x0

public:
    virtual ~DisposeCallback() {} // at 0x8

    virtual void InvalidateData(const void *pStart,
                                const void *pEnd) = 0; // at 0xC
    virtual void InvalidateWaveData(const void *pStart,
                                    const void *pEnd) = 0; // at 0x10
};

NW4R_UT_LINKLIST_TYPEDEF_DECL(DisposeCallback);

} // namespace detail
} // namespace snd
} // namespace nw4r

#endif
