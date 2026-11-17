#ifndef NW4R_EF_EFFECT_SYSTEM_H
#define NW4R_EF_EFFECT_SYSTEM_H
#include "nw4r/types_nw4r.h"

#include "nw4r/ef/ef_creationqueue.h"
#include "nw4r/ef/ef_random.h"
#include "nw4r/math.h"

namespace nw4r {
namespace ef {

// Forward declarations
class ActivityList;
class DrawInfo;
class DrawOrderBase;
class DrawStrategyBuilder;
class EmitFormBuilder;
class MemoryManagerBase;

class EffectSystem {
private:
    MemoryManagerBase* mMemoryManager; // at 0x0

public:
    DrawOrderBase* mDrawOrderFunc;             // at 0x4
    DrawStrategyBuilder* mDrawStrategyBuilder; // at 0x8
    EmitFormBuilder* mEmitFormBuilder;         // at 0xC
    CreationQueue mCreationQueue;              // at 0x10
    u32 mMaxGroupID;                           // at 0xC014
    ActivityList* mActivityList;               // at 0xC018
    Random mRandom;                            // at 0xC01C
    math::VEC3 mProcessCameraPos;              // at 0xC020
    math::MTX34 mProcessCameraMtx;             // at 0xC02C
    f32 mProcessCameraFar;                     // at 0xC05C
    f32 mProcessCameraNear;                    // at 0xC060
    bool mXFFlushSafe;                         // at 0xC064

    // Set once the version text is registered
    static BOOL mDisplayVersion;
    static EffectSystem instance;

private:
    static void SwapEffect(ut::List* pList, Effect* pFront, Effect* pBack);

public:
    static EffectSystem* GetInstance();

    EffectSystem();
    ~EffectSystem();

    bool Initialize(u32 maxGroupID);
    bool Closing(Effect* pEffect);

    Effect* CreateEffect(EmitterResource* pResource, u32 groupID,
                         u16 calcRemain);
    u32 RetireEffect(Effect* pEffect);

    u32 RetireEffectAll(u32 groupID);
    u32 RetireEmitterAll(u32 groupID);
    u32 RetireParticleAll(u32 groupID);

    void Calc(u32 groupID, bool onlyBillboard);
    void Draw(const DrawInfo& rInfo, u32 groupID);

    void SetProcessCamera(const math::VEC3& rPos, const math::MTX34& rMtx,
                          f32 near, f32 far);

    typedef bool (*EffectCompareFunc)(const Effect* pLhs, const Effect* pRhs);
    void SortEffect(EffectCompareFunc pFunc, u32 groupID);

    MemoryManagerBase* GetMemoryManager() const {
        return mMemoryManager;
    }
    void SetMemoryManager(MemoryManagerBase* pManager, u32 maxGroupID) {
        mMemoryManager = pManager;

        if (mMemoryManager != NULL) {
            Initialize(maxGroupID);
        }
    }

    void SetXFFlushSafe(bool safe) {
        mXFFlushSafe = safe;
    }
};

} // namespace ef
} // namespace nw4r

#endif
