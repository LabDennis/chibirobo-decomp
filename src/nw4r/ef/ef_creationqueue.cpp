#include "nw4r/ef.h"

namespace nw4r {
namespace ef {

CreationQueue::CreationQueue() : mNumItem(0) {}

void CreationQueue::AddParticleCreation(const EmitterInheritSetting* pSetting,
                                        Particle* pParticle,
                                        EmitterResource* pResource,
                                        u16 calcRemain, const math::VEC3* pPos,
                                        const math::VEC3* pVel) {

    if (mNumItem >= QUEUE_SIZE) {
        return;
    }

    mQueueData[mNumItem].mType = CreationQueueData::TYPE_PARTICLE;
    mQueueData[mNumItem].mFlags = 0;
    mQueueData[mNumItem].mCalcRemain = calcRemain;
    mQueueData[mNumItem].mInheritSetting = *pSetting;
    mQueueData[mNumItem].mReferenceParticle = pParticle;

    pParticle->Ref();

    mQueueData[mNumItem].mEmitterResource = pResource;

    if (pPos != NULL) {
        mQueueData[mNumItem].mFlags |= CreationQueueData::FLAG_POS;
        mQueueData[mNumItem].mPos = *pPos;
    }

    if (pVel != NULL) {
        mQueueData[mNumItem].mFlags |= CreationQueueData::FLAG_VEL;
        mQueueData[mNumItem].mVel = *pVel;
    }

    mNumItem++;
}

void CreationQueue::AddEmitterCreation(const EmitterInheritSetting* pSetting,
                                       Particle* pParticle,
                                       EmitterResource* pResource,
                                       u16 calcRemain, const math::VEC3* pPos,
                                       const math::VEC3* pVel) {

    if (mNumItem >= QUEUE_SIZE) {
        return;
    }

    mQueueData[mNumItem].mType = CreationQueueData::TYPE_EMITTER;
    mQueueData[mNumItem].mFlags = 0;
    mQueueData[mNumItem].mCalcRemain = calcRemain;
    mQueueData[mNumItem].mInheritSetting = *pSetting;
    mQueueData[mNumItem].mReferenceParticle = pParticle;

    pParticle->Ref();

    mQueueData[mNumItem].mEmitterResource = pResource;

    if (pPos != NULL) {
        mQueueData[mNumItem].mFlags |= CreationQueueData::FLAG_POS;
        mQueueData[mNumItem].mPos = *pPos;
    }

    if (pVel != NULL) {
        mQueueData[mNumItem].mFlags |= CreationQueueData::FLAG_VEL;
        mQueueData[mNumItem].mVel = *pVel;
    }

    mNumItem++;
}

void CreationQueue::Execute() {
    for (int i = 0; i < mNumItem; i++) {
        CreationQueueData& rData = mQueueData[i];

        const math::VEC3* pPos = NULL;
        if (rData.mFlags & CreationQueueData::FLAG_POS) {
            pPos = &rData.mPos;
        }

        switch (rData.mType) {
        case CreationQueueData::TYPE_PARTICLE: {
            rData.mReferenceParticle->mParticleManager->mManagerEM
                ->CreateEmitterTmp(rData.mEmitterResource,
                                   &rData.mInheritSetting,
                                   rData.mReferenceParticle, rData.mCalcRemain,
                                   pPos);

            rData.mReferenceParticle->UnRef();
            break;
        }

        case CreationQueueData::TYPE_EMITTER: {
            rData.mReferenceParticle->mParticleManager->mManagerEM
                ->CreateEmitter(rData.mEmitterResource, &rData.mInheritSetting,
                                rData.mReferenceParticle, rData.mCalcRemain,
                                pPos);

            rData.mReferenceParticle->UnRef();
            break;
        }
        }
    }

    mNumItem = 0;
}

} // namespace ef
} // namespace nw4r
