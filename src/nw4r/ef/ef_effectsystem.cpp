#include "nw4r/ef.h"
#include "nw4r/ut/ut_LinkList.h"
#include "nw4r/ut/ut_algorithm.h"
#include "nw4r/ut/ut_list.h"

#include "revolution/os/OSAlarm.h"
#include "Dolphin/os.h"

namespace {

NW4R_LIB_VERSION(EF, "Jan 23 2009", "03:31:37", "0x4199_60831");

} // namespace

namespace nw4r {
namespace ef {

static DrawOrder gBasicDrawOrder;
static DrawStrategyBuilder gBasicDrawStrategyBuilder;
static EmitFormBuilder gBasicEmitFormBuilder;

static DrawInfo gBasicDrawInfo;

BOOL EffectSystem::mDisplayVersion;
EffectSystem EffectSystem::instance;

EffectSystem::EffectSystem() {
    if (!mDisplayVersion) {
        mDisplayVersion = TRUE;
        OSRegisterVersion(NW4R_EF_Version_);
    }

    mMemoryManager = NULL;
    mMaxGroupID = 0;
    mXFFlushSafe = false;
}

EffectSystem::~EffectSystem() {
    for (u32 i = 0; i < mMaxGroupID; i++) {
        RetireEffectAll(i);
    }
}

EffectSystem* EffectSystem::GetInstance() {
    return &instance;
}

bool EffectSystem::Initialize(u32 maxGroupID) {
    mMaxGroupID = maxGroupID;

    mActivityList =
        new (mMemoryManager->AllocHeap(mMaxGroupID * sizeof(ActivityList) + 32))
            ActivityList[mMaxGroupID];

    for (int i = 0; i < mMaxGroupID; i++) {
        mActivityList[i].SetOffset(offsetof(Effect, mActivityLink));
        mActivityList[i].Initialize();
    }

    mRandom.Srand(0);

    mDrawOrderFunc = &gBasicDrawOrder;
    mDrawStrategyBuilder = &gBasicDrawStrategyBuilder;
    mEmitFormBuilder = &gBasicEmitFormBuilder;

    return true;
}

bool EffectSystem::Closing(Effect* pEffect) {
    mActivityList[pEffect->mGroupID].ToClosing(pEffect);
    pEffect->mLifeStatus = ReferencedObject::NW4R_EF_LS_CLOSING;

    return true;
}

Effect* EffectSystem::CreateEffect(EmitterResource* pResource, u32 groupID,
                                   u16 calcRemain) {

    if (pResource == NULL) {
        return NULL;
    }

    if (!(pResource->GetEmitterDesc()->commonFlag &
          EmitterDesc::CMN_FLAG_RELOCATED)) {
        return NULL;
    }

    Effect* pEffect = GetMemoryManager()->AllocEffect();
    if (pEffect == NULL) {
        return NULL;
    }

    if (!pEffect->Initialize(this, pResource, calcRemain)) {
        GetMemoryManager()->FreeEffect(pEffect);
        return NULL;
    }

    pEffect->mGroupID = groupID;

    mActivityList[groupID].ToActive(pEffect);
    pEffect->mLifeStatus = ReferencedObject::NW4R_EF_LS_ACTIVE;

    mActivityList[groupID].ToWait(pEffect);
    pEffect->mLifeStatus = ReferencedObject::NW4R_EF_LS_WAIT;

    return pEffect;
}

u32 EffectSystem::RetireEffect(Effect* pEffect) {
    if (pEffect->mLifeStatus != ReferencedObject::NW4R_EF_LS_ACTIVE) {
        return 0;
    }

    mActivityList[pEffect->mGroupID].ToWait(pEffect);
    pEffect->Destroy();
    return 1;
}

u32 EffectSystem::RetireEffectAll(u32 groupID) {
    u32 num = 0;
    void* pArray[NW4R_EF_MAX_EFFECT];

    u16 size = UtlistToArray(&mActivityList[groupID].mActiveList, pArray,
                             UtlistSize(&mActivityList[groupID].mActiveList));

    for (u16 i = 0; i < size; i++) {
        Effect* pEffect = static_cast<Effect*>(pArray[i]);

        if (pEffect->mLifeStatus == ReferencedObject::NW4R_EF_LS_ACTIVE) {
            num += RetireEffect(pEffect);
        }
    }

    return num;
}

u32 EffectSystem::RetireEmitterAll(u32 groupID) {
    u32 num = 0;
    void* pArray[NW4R_EF_MAX_EFFECT];

    u16 size = UtlistToArray(&mActivityList[groupID].mActiveList, pArray,
                             UtlistSize(&mActivityList[groupID].mActiveList));

    for (u16 i = 0; i < size; i++) {
        Effect* pEffect = static_cast<Effect*>(pArray[i]);
        num += pEffect->RetireEmitterAll();
    }

    return num;
}

u32 EffectSystem::RetireParticleAll(u32 groupID) {
    u32 num = 0;
    void* pArray[NW4R_EF_MAX_EFFECT];

    u16 size = UtlistToArray(&mActivityList[groupID].mActiveList, pArray,
                             UtlistSize(&mActivityList[groupID].mActiveList));

    for (u16 i = 0; i < size; i++) {
        Effect* pEffect = static_cast<Effect*>(pArray[i]);
        num += pEffect->RetireParticleAll();
    }

    return num;
}

void EffectSystem::Calc(u32 groupID, bool onlyBillboard) {
    void* pArray[NW4R_EF_MAX_EFFECT];

    u16 size = UtlistToArray(&mActivityList[groupID].mActiveList, pArray,
                             UtlistSize(&mActivityList[groupID].mActiveList));

    for (u16 i = 0; i < size; i++) {
        Effect* pEffect = static_cast<Effect*>(pArray[i]);
        pEffect->Calc(onlyBillboard);
    }

    mMemoryManager->GarbageCollection();
}

void EffectSystem::Draw(const DrawInfo& rInfo, u32 groupID) {
    NW4R_UT_LIST_FOREACH (Effect, it, mActivityList[groupID].mActiveList,
                          { it->Draw(rInfo); });
}

void EffectSystem::SetProcessCamera(const math::VEC3& rPos,
                                    const math::MTX34& rMtx, f32 near,
                                    f32 far) {
    mProcessCameraPos = rPos;
    math::MTX34Copy(&mProcessCameraMtx, &rMtx);
    mProcessCameraNear = near;
    mProcessCameraFar = far;
}

// pFront stands directly before pBack in the list; they change places
void EffectSystem::SwapEffect(ut::List* pList, Effect* pFront, Effect* pBack) {
    ut::Link* pFrontLink = &pFront->mActivityLink;
    ut::Link* pBackLink = &pBack->mActivityLink;
    Effect* pPrev = static_cast<Effect*>(pFrontLink->prevObject);
    Effect* pNext = static_cast<Effect*>(pBackLink->nextObject);

    if (pPrev == NULL) {
        pList->headObject = pBack;
    } else {
        pPrev->mActivityLink.nextObject = pBack;
    }

    pBackLink->prevObject = pPrev;
    pBackLink->nextObject = pFront;
    pFrontLink->prevObject = pBack;
    pFrontLink->nextObject = pNext;

    if (pNext == NULL) {
        pList->tailObject = pFront;
    } else {
        pNext->mActivityLink.prevObject = pFront;
    }
}

// Shaker sort of one group's active effects
void EffectSystem::SortEffect(EffectCompareFunc pFunc, u32 groupID) {
    ut::List* pList = &mActivityList[groupID].mActiveList;
    u16 begin;
    u16 end;
    Effect* pBack;
    u16 i;
    u32 n;
    Effect* pStart;
    Effect* pFront;
    u16 last;

    if (pList->numObjects <= 1) {
        return;
    }

    begin = 0;
    end = pList->numObjects - 1;
    pStart = NULL;

    for (n = 0; n < 0xFFFF; n++) {
        last = begin;

        if (pStart == NULL) {
            pStart = static_cast<Effect*>(List_GetFirst(pList));
        }

        pFront = pStart;
        for (i = begin; i < end; i++) {
            pBack = static_cast<Effect*>(List_GetNext(pList, pFront));

            if (pFunc(pBack, pFront)) {
                SwapEffect(pList, pFront, pBack);
                last = i;
                pStart = pBack;
            } else {
                pFront = pBack;
            }
        }

        end = last;
        if (end == begin) {
            break;
        }

        pBack = pStart;
        for (i = last; i > begin; i--) {
            pFront = static_cast<Effect*>(List_GetPrev(pList, pBack));

            if (pFunc(pBack, pFront)) {
                SwapEffect(pList, pFront, pBack);
                last = i;
                pStart = pFront;
            } else {
                pBack = pFront;
            }
        }

        begin = last;
        if (begin == end) {
            break;
        }
    }
}

} // namespace ef
} // namespace nw4r
