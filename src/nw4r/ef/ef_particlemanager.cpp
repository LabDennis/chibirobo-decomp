#include "nw4r/ef.h"
#include "nw4r/math.h"
#include "nw4r/ut/ut_LinkList.h"
#include "nw4r/ut/ut_algorithm.h"
#include "nw4r/ut/ut_list.h"

#include <cmath>
#include <cstring>

namespace nw4r {
namespace ef {

math::MTX34 ParticleManager::smDrawMtxPMtoEM;
math::MTX34 ParticleManager::smMtxInv;

s32 ParticleManager::smMtxInvId = 0;

ParticleManager::ParticleManager()
    : mActivityList(offsetof(Particle, mActivityLink)) {}

void ParticleManager::SendClosing() {
    mManagerEM->Closing(this);
}

void ParticleManager::DestroyFunc() {}

bool ParticleManager::Closing(Particle* pParticle) {
    pParticle->mParticleManager->UnRef();

    mActivityList.ToClosing(pParticle);
    pParticle->mLifeStatus = NW4R_EF_LS_CLOSING;

    return true;
}

int ParticleManager::RetireParticleAll() {
    int num = 0;

    Particle* pIt;
    Particle* pNext;

    for (pIt = static_cast<Particle*>(mActivityList.mActiveList.headObject);
         pIt != NULL; pIt = pNext) {

        pNext = static_cast<Particle*>(
            NW4R_UT_LIST_GET_LINK(mActivityList.mActiveList, pIt)->nextObject);

        if (pIt->mLifeStatus == NW4R_EF_LS_ACTIVE) {
            num += RetireParticle(pIt);
        }
    }

    return num;
}

bool ParticleManager::Initialize(Emitter* pParent, EmitterResource* pResource) {
    ReferencedObject::Initialize();

    mActivityList.Initialize();

    mModifier.mScale.x = 1.0f;
    mModifier.mScale.y = 1.0f;

    mModifier.mRotate.x = 0.0f;
    mModifier.mRotate.y = 0.0f;
    mModifier.mRotate.z = 0.0f;

    mManagerEM = pParent;
    pParent->Ref();

    mResource = pResource;
    mFlag = 0;
    SetMtxDirty();

    EmitterDesc* pDesc = mResource->GetEmitterDesc();

    mDrawStrategy =
        mManagerEM->mManagerEF->mManagerES->mDrawStrategyBuilder->Create(
            pDesc->drawSetting.ptcltype);

    mLastCalced = NULL;

    Modifier_SetSimpleLightParameter(pDesc->drawSetting);

    return true;
}

ParticleManager::~ParticleManager() {}

Particle*
ParticleManager::CreateParticle(u16 life, math::VEC3 pos, math::VEC3 vel,
                                const math::MTX34* pSpace, f32 momentum,
                                const EmitterInheritSetting* pSetting,
                                Particle* pReferencePtcl, u16 calcRemain) {

    Particle* pParticle =
        mManagerEM->mManagerEF->mManagerES->GetMemoryManager()->AllocParticle();

    if (pParticle == NULL) {
        return NULL;
    }

    if (!pParticle->Initialize(life, pos, vel, this, pSpace, momentum, pSetting,
                               pReferencePtcl)) {
        return NULL;
    }

    pParticle->mCalcRemain += calcRemain;
    mActivityList.ToActive(pParticle);
    pParticle->mLifeStatus = NW4R_EF_LS_ACTIVE;

    return pParticle;
}

// The particle as the field sees it: in the emitter's space, or in the world
// with the emitter as origin
static inline math::VEC3 CalcFieldPos(AnimCurveFieldParam* pParam,
                                      const math::MTX34* pLocToEm,
                                      const math::MTX34* pLocToGlb,
                                      const math::MTX34* pEmToGlb,
                                      const math::VEC3* pPrevPos) {
    math::VEC3 pos;

    if (pParam->space == AnimCurveFieldParam::SPACE_EMITTER) {
        math::VEC3Transform(&pos, pLocToEm, pPrevPos);
    } else {
        math::VEC3Transform(&pos, pLocToGlb, pPrevPos);

        math::VEC3 origin(0.0f, 0.0f, 0.0f);
        math::VEC3Transform(&origin, pEmToGlb, &origin);
        math::VEC3Sub(&pos, &pos, &origin);
    }

    return pos;
}

static inline void CalcFieldSpeed(math::VEC3* pOut, AnimCurveFieldParam* pParam,
                                  const math::VEC3* pVel) {
    math::VEC3Scale(pOut, pVel, pParam->power - 1.0f);
}

static inline void CalcFieldGravity(math::VEC3* pOut,
                                    AnimCurveFieldParam* pParam) {
    Rotation2VecY(pParam->gravity.rot, pOut);
    *pOut *= pParam->power;
}

static inline void CalcFieldRandom(math::VEC3* pOut,
                                   AnimCurveFieldParam* pParam,
                                   AnimCurveHeader* pHeader, Particle* pParticle,
                                   u32 tick, u16 seed,
                                   const math::VEC3* pPrevVel,
                                   const math::VEC3* pPrevDir,
                                   const math::MTX34* pLocToEmNoTrans) {
    f32 sinPhi;
    f32 cosPhi;

    union {
        u32 value;
        u8 bytes[4];
    } rndSeed;

    rndSeed.value = seed * 0x3F81F635 +
                    pHeader->randomSeed * 0x30A74193 +
                    static_cast<u16>(tick) * 0x371097E7 +
                    0x4BF53;

    rndSeed.bytes[2] ^= rndSeed.bytes[3];
    rndSeed.bytes[1] ^= rndSeed.bytes[2];
    rndSeed.bytes[0] ^= rndSeed.bytes[1];

    if (!(pHeader->processFlag &
          AnimCurveHeader::PROC_FLAG_TIMING)) {
        if (tick == 0) {
            return;
        }

        if (tick % (pParam->random.interval + 1) != 0) {
            return;
        }
    } else {
        if (pParticle->mTick != 0 &&
            tick % (pParam->random.interval + 1) != 0) {
            return;
        }
    }

    // The power is the particle's speed
    if (pParam->option & AnimCurveFieldParam::RANDOM_OPT_SPEED) {
        math::VEC3 dir;

        switch (pParam->type) {
        case AnimCurveFieldParam::TYPE_VELOCITY: {
            dir = *pPrevVel;
            break;
        }

        case AnimCurveFieldParam::TYPE_POSITION: {
            if (pParticle->mTick == 0) {
                dir = *pPrevVel;
            } else {
                dir = *pPrevDir / pParticle->mParameter.mMomentum;
            }
            break;
        }
        }

        math::VEC3Transform(&dir, pLocToEmNoTrans, &dir);
        pParam->power = math::VEC3Len(&dir);
    }

    if (pParam->option & AnimCurveFieldParam::RANDOM_OPT_CUBE) {
        u32 rnd0 = rndSeed.value;
        pOut->x = static_cast<s16>(rnd0 >> 16) / 32768.0f;
        u32 rnd1 = rnd0 * 0x343FD + 0x269EC3;
        pOut->y = static_cast<s16>(rnd1 >> 16) / 32768.0f;
        u32 rnd2 = rnd1 * 0x343FD + 0x269EC3;
        pOut->z = static_cast<s16>(rnd2 >> 16) / 32768.0f;

        if (pParam->option &
            AnimCurveFieldParam::RANDOM_OPT_SPEED) {
            if (!VecNormalize(pOut, pOut)) {
                pOut->y = 1.0f;
            }
        }

        math::VEC3Scale(pOut, pOut, pParam->power);
    } else {
        math::VEC3 dir;

        if (pParticle->mTick == 0) {
            dir = *pPrevVel;
        } else {
            dir = *pPrevDir;
        }

        math::VEC3Transform(&dir, pLocToEmNoTrans, &dir);

        if (!VecNormalize(&dir, &dir)) {
            dir.y = 1.0f;
        }

        math::MTX34 mtxDir;
        GetDirMtxY(&mtxDir, dir);

        if (pParam->random.diffuse != 0.0f) {
            u32 rnd0 = rndSeed.value;
            f32 theta = (rnd0 >> 16) / 65535.0f *
                        pParam->random.diffuse;
            u32 rnd1 = rnd0 * 0x343FD + 0x269EC3;
            f32 phi = 2.0f * (NW4R_MATH_PI *
                              ((rnd1 >> 16) / 65535.0f));
            u32 rnd2 = rnd1 * 0x343FD + 0x269EC3;

            SinCosRad(&pOut->x, theta);
            pOut->z = pOut->x;
            SinCosRad(&sinPhi, &cosPhi, phi);
            pOut->x *= sinPhi;
            pOut->z *= cosPhi;

            if (pParam->option &
                AnimCurveFieldParam::RANDOM_OPT_SPEED) {
                math::VEC3Scale(pOut, pOut, pParam->power);
            } else {
                math::VEC3Scale(
                    pOut, pOut,
                    (rnd2 >> 16) / 65535.0f * pParam->power);
            }
        } else {
            pOut->x = 0.0f;

            if (pParam->option &
                AnimCurveFieldParam::RANDOM_OPT_SPEED) {
                pOut->y = pParam->power;
            } else {
                pOut->y = static_cast<s16>(rndSeed.value >> 16) /
                      32768.0f * pParam->power;
            }

            pOut->z = 0.0f;
        }

        math::VEC3Transform(pOut, &mtxDir, pOut);
    }

    if (!(pParam->option & AnimCurveFieldParam::RANDOM_OPT_X)) {
        pOut->x = 0.0f;
    }
    if (!(pParam->option & AnimCurveFieldParam::RANDOM_OPT_Y)) {
        pOut->y = 0.0f;
    }
    if (!(pParam->option & AnimCurveFieldParam::RANDOM_OPT_Z)) {
        pOut->z = 0.0f;
    }
}

static inline void CalcFieldSpin(math::VEC3* pOut, AnimCurveFieldParam* pParam,
 const math::VEC3* pPos) {
    math::VEC3 axis;
    Rotation2VecY(pParam->spin.rot, &axis);

    math::MTX34 mtxRot;
    math::MTX34RotAxisRad(&mtxRot, &axis, pParam->power);

    math::VEC3Transform(pOut, &mtxRot, pPos);
    math::VEC3Sub(pOut, pOut, pPos);
}

static inline void CalcFieldMagnet(math::VEC3* pOut, AnimCurveFieldParam* pParam,
 const math::VEC3* pPos) {
    math::VEC3Sub(
        pOut, static_cast<math::VEC3*>(&pParam->magnet.pos),
        pPos);
    VecNormalize(pOut, pOut);
    math::VEC3Scale(pOut, pOut, pParam->power);
}

static inline void CalcFieldNewton(math::VEC3* pOut, AnimCurveFieldParam* pParam,
 const math::VEC3* pPos) {
    math::VEC3Sub(
        pOut, static_cast<math::VEC3*>(&pParam->newton.pos),
        pPos);
    f32 lenSq = math::VEC3LenSq(pOut);

    VecNormalize(pOut, pOut);
    math::VEC3Scale(pOut, pOut, pParam->power);

    f32 rangeSq = pParam->newton.range * pParam->newton.range;
    if (lenSq > rangeSq) {
        math::VEC3Scale(pOut, pOut, rangeSq / lenSq);
    }
}

static inline void CalcFieldVortex(math::VEC3* pOut, AnimCurveFieldParam* pParam,
 const math::VEC3* pPos) {
    math::VEC3 axis;
    Rotation2VecY(pParam->vortex.rot, &axis);

    pParam->vortex.radius *= pParam->vortex.radius;

    math::VEC3 dist;
    math::VEC3Scale(&dist, &axis,
                    math::VEC3Dot(&axis, pPos));
    math::VEC3Sub(&dist, pPos, &dist);
    f32 distSq = math::VEC3LenSq(&dist);

    if (distSq == 0.0f) {
        return;
    }

    f32 speed;

    if (distSq >= pParam->vortex.radius) {
        speed = pParam->vortex.outerSpeed;
    } else {
        f32 ratio = distSq / pParam->vortex.radius;
        speed = (1.0f - ratio) * pParam->power +
                ratio * pParam->vortex.outerSpeed;
    }

    VecNormalize(&dist, &dist);
    math::VEC3Cross(pOut, &dist, &axis);
    math::VEC3Scale(pOut, pOut, speed);
}

// NONMATCHING: two things are left, the code is otherwise retail's instruction
// for instruction (same frame, same stack slots).
// - Retail calls __fpclassifyf (0x8022A450) for the isnan of the tail field;
//   our MSL header defines it inline, so it is expanded here (+0x58 bytes).
// - With that call forced, six registers are rotated: retail keeps kind in r29
//   above the five hoisted addresses (&pos, &prevPos, &mtxLocToEmNoTrans,
//   &fieldPos, &v in r28-r24), ours has them in r29-r25 and kind in r24.
// The field helpers above are inlines in retail too: their locals sit behind
// Calc's own in the frame and their pointer arguments are the hoisted
// addresses. Their names and parameter lists are ours.
void ParticleManager::Calc() {
    Particle* pFirst = static_cast<Particle*>(
        ut::List_GetNext(&mActivityList.mActiveList, mLastCalced));

    if (pFirst == NULL) {
        return;
    }

    u8 kind;
    AnimCurveHeader* pHeader;
    Particle* pIt = pFirst;
    u16 i;
    bool postField;
    AnimCurvePostFieldInfo* pPostInfo;
    EmitterResource* pPostChild;
    u32 tick;
    u32 life;
    u16 seed;

    if (mManagerEM->mManagerEF->mCallBack.mPrevPtclCalc != NULL) {
        mManagerEM->mManagerEF->mCallBack.mPrevPtclCalc(
            this, &mActivityList.mActiveList, pFirst);
    }

    math::MTX34 mtxLocToGlb;
    CalcGlobalMtx(&mtxLocToGlb);

    math::MTX34 mtxGlbToLoc;
    math::MTX34Inv(&mtxGlbToLoc, &mtxLocToGlb);

    math::MTX34 mtxEmToGlb;
    mManagerEM->CalcGlobalMtx(&mtxEmToGlb);

    // The emitter's scale: the length of its (1, 1, 1) over sqrt(3)
    math::VEC3 emScale(mtxEmToGlb._02 + (mtxEmToGlb._00 + mtxEmToGlb._01),
                       mtxEmToGlb._12 + (mtxEmToGlb._10 + mtxEmToGlb._11),
                       mtxEmToGlb._22 + (mtxEmToGlb._20 + mtxEmToGlb._21));
    f32 scale = 0.57735026f * math::VEC3Len(&emScale);

    math::MTX34 mtxEmToLoc;
    math::MTX34Mult(&mtxEmToLoc, &mtxGlbToLoc, &mtxEmToGlb);

    math::MTX34 mtxLocToEm;
    math::MTX34Inv(&mtxLocToEm, &mtxEmToLoc);

    math::MTX34 mtxGlbToLocNoTrans = mtxGlbToLoc;
    mtxGlbToLocNoTrans._03 = 0.0f;
    mtxGlbToLocNoTrans._13 = 0.0f;
    mtxGlbToLocNoTrans._23 = 0.0f;

    math::MTX34 mtxLocToGlbNoTrans;
    math::MTX34Inv(&mtxLocToGlbNoTrans, &mtxGlbToLocNoTrans);

    math::MTX34 mtxLocToEmNoTrans = mtxLocToEm;
    mtxLocToEmNoTrans._03 = 0.0f;
    mtxLocToEmNoTrans._13 = 0.0f;
    mtxLocToEmNoTrans._23 = 0.0f;

    math::MTX34 mtxEmToLocNoTrans = mtxEmToLoc;
    mtxEmToLocNoTrans._03 = 0.0f;
    mtxEmToLocNoTrans._13 = 0.0f;
    mtxEmToLocNoTrans._23 = 0.0f;

    Particle* pNext;

    for (; pIt != NULL; pIt = pNext) {
        pNext = static_cast<Particle*>(
            NW4R_UT_LIST_GET_LINK(mActivityList.mActiveList, pIt)->nextObject);

        if (pIt->mLifeStatus != NW4R_EF_LS_ACTIVE) {
            continue;
        }

        if (pIt->mEvalStatus != NW4R_EF_ES_WAIT) {
            continue;
        }

        pIt->mEvalStatus = NW4R_EF_ES_DONE;

        if (pIt->mCalcRemain != 0) {
            mManagerEM->mManagerEF->SetFlagExistCalcRemain(true);
        }

        math::VEC3 prevPos = pIt->mParameter.mPosition;
        math::VEC3 prevVel = pIt->mParameter.mVelocity;

        math::VEC3 prevDir;
        pIt->GetMoveDir(&prevDir);

        pIt->mParameter.mPrevPosition = pIt->mParameter.mPosition;

        if (pIt->mLife <= pIt->mTick) {
            RetireParticle(pIt);
            continue;
        }

        math::VEC3 addVel(0.0f, 0.0f, 0.0f);
        math::VEC3 addPos(0.0f, 0.0f, 0.0f);

        AnimCurvePostFieldParam postParam;
        postField = false;
        pPostChild = NULL;

        for (i = pIt->mTick == 0 ? 0 : mResource->NumPtclInitTrack();
             i < mResource->NumPtclTrack(); i++) {

            pHeader = reinterpret_cast<AnimCurveHeader*>(
                mResource->GetPtclTrack(i));

            if (pHeader->processFlag & AnimCurveHeader::PROC_FLAG_STOP) {
                continue;
            }


            if (pHeader->processFlag & AnimCurveHeader::PROC_FLAG_TIMING) {
                tick = pIt->mParticleManager->mManagerEM->mTick;

                if (pIt->mParticleManager->mManagerEM->mParameter.mComFlags &
                    EmitterDesc::CMN_FLAG_MAX_LIFE) {
                    life = 0xFFFFFFFF;
                } else {
                    life =
                        pIt->mParticleManager->mManagerEM->mParameter.mEmitSpan;
                }

                seed = pIt->mParticleManager->mManagerEM->mRandSeed;
            } else {
                tick = pIt->mTick;
                life = pIt->mLife;
                seed = pIt->mRandSeed;
            }

            if (pHeader->magic == NW4R_EF_MAGIC_ANIMBAKED ||
                pHeader->magic == NW4R_EF_MAGIC_ANIMCURVE) {

            u8 type = pHeader->curveFlag;
            kind = pHeader->kindType;

            switch (type) {
            case AC_TYPE_PARTICLE_TEXTURE: {
                AnimCurveExecuteTexture(reinterpret_cast<u8*>(pHeader), pIt, tick, seed, life);
                break;
            }

            case AC_TYPE_CHILD: {
                AnimCurveExecuteChild(reinterpret_cast<u8*>(pHeader), pIt, tick, seed, life);
                break;
            }

            case AC_TYPE_PARTICLE_ROTATE: {
                AnimCurveExecuteRotate(
                    reinterpret_cast<u8*>(pHeader),
                    reinterpret_cast<f32*>(&pIt->mParameter.mRotate), tick,
                    seed, life);
                break;
            }

            case AC_TYPE_PARTICLE_U8: {
                AnimCurveExecuteU8(
                    reinterpret_cast<u8*>(pHeader),
                    reinterpret_cast<u8*>(&pIt->mParameter) + kind, tick, seed,
                    life);
                break;
            }

            case AC_TYPE_PARTICLE_F32: {
                AnimCurveExecuteF32(
                    reinterpret_cast<u8*>(pHeader),
                    reinterpret_cast<f32*>(
                        reinterpret_cast<u8*>(&pIt->mParameter) + kind),
                    tick, seed, life);
                break;
            }

            case AC_TYPE_FIELD: {
                AnimCurveFieldParam param;
                std::memcpy(&param,
                            reinterpret_cast<u8*>(pHeader) + sizeof(AnimCurveHeader) +
                                pHeader->keyTable + pHeader->rangeTable +
                                pHeader->randomTable + pHeader->nameTable,
                            sizeof(param));

                if (pHeader->keyTable != 0) {
                    AnimCurveExecuteF32(reinterpret_cast<u8*>(pHeader), &param.power, tick, seed,
                                        life);
                }

                // The particle from the field's point of view
                math::VEC3 fieldPos;

                if (kind == AnimCurveFieldParam::KIND_SPIN ||
                    kind == AnimCurveFieldParam::KIND_MAGNET ||
                    kind == AnimCurveFieldParam::KIND_NEWTON ||
                    kind == AnimCurveFieldParam::KIND_VORTEX) {
                    fieldPos = CalcFieldPos(&param, &mtxLocToEm, &mtxLocToGlb,
                                            &mtxEmToGlb, &prevPos);
                }

                math::VEC3 v(0.0f, 0.0f, 0.0f);

                switch (kind) {
                case AnimCurveFieldParam::KIND_SPEED: {
                    CalcFieldSpeed(&v, &param, &prevVel);
                    break;
                }

                case AnimCurveFieldParam::KIND_GRAVITY: {
                    CalcFieldGravity(&v, &param);
                    break;
                }

                case AnimCurveFieldParam::KIND_RANDOM: {
                    CalcFieldRandom(&v, &param, pHeader, pIt, tick, seed,
                                    &prevVel, &prevDir, &mtxLocToEmNoTrans);
                    break;
                }

                case AnimCurveFieldParam::KIND_SPIN: {
                    CalcFieldSpin(&v, &param, &fieldPos);
                    break;
                }

                case AnimCurveFieldParam::KIND_MAGNET: {
                    CalcFieldMagnet(&v, &param, &fieldPos);
                    break;
                }

                case AnimCurveFieldParam::KIND_NEWTON: {
                    CalcFieldNewton(&v, &param, &fieldPos);
                    break;
                }

                case AnimCurveFieldParam::KIND_VORTEX: {
                    CalcFieldVortex(&v, &param, &fieldPos);
                    break;
                }

                // Pulls along the way the emitter went in this frame
                case AnimCurveFieldParam::KIND_TAIL: {
                    if (fpclassify(mManagerEM->mPrevGlobalPos.x) == FP_NAN) {
                        break;
                    }

                    math::MTX34 mtx;
                    mManagerEM->CalcGlobalMtx(&mtx);

                    math::VEC3 pos(mtx._03, mtx._13, mtx._23);
                    math::VEC3Sub(&v, &pos, &mManagerEM->mPrevGlobalPos);
                    math::VEC3Scale(&v, &v, param.power);
                    break;
                }
                }

                switch (param.space) {
                case AnimCurveFieldParam::SPACE_EMITTER: {
                    math::VEC3Transform(&v, &mtxEmToLocNoTrans, &v);
                    break;
                }

                case AnimCurveFieldParam::SPACE_GLOBAL: {
                    math::VEC3Transform(&v, &mtxGlbToLocNoTrans, &v);
                    break;
                }

                case AnimCurveFieldParam::SPACE_GLOBAL_SCALE: {
                    v *= scale;
                    math::VEC3Transform(&v, &mtxGlbToLocNoTrans, &v);
                    break;
                }
                }

                switch (param.type) {
                case AnimCurveFieldParam::TYPE_VELOCITY: {
                    addVel += v;
                    break;
                }

                case AnimCurveFieldParam::TYPE_POSITION: {
                    addPos += v;
                    break;
                }
                }
                break;
            }

            case AC_TYPE_POSTFIELD: {
                if (pHeader->infoTable != 0) {
                    AnimCurveNameTable* pNameTbl =
                        reinterpret_cast<AnimCurveNameTable*>(
                            reinterpret_cast<u8*>(pHeader) + sizeof(AnimCurveHeader) +
                            pHeader->keyTable + pHeader->rangeTable +
                            pHeader->randomTable);

                    pPostInfo = reinterpret_cast<AnimCurvePostFieldInfo*>(
                        reinterpret_cast<u8*>(pNameTbl) + pHeader->nameTable);

                    std::memcpy(&postParam, pPostInfo, sizeof(postParam));

                    if (pHeader->nameTable > 4) {
                        pPostChild = reinterpret_cast<EmitterResource*>(
                            pNameTbl->datas[pPostInfo->childName].work);
                    }
                }

                if (pHeader->keyTable != 0) {
                    AnimCurveExecuteF32(
                        reinterpret_cast<u8*>(pHeader),
                        reinterpret_cast<f32*>(
                            reinterpret_cast<u8*>(&postParam) + kind),
                        tick, seed, life);
                }

                postField = true;
                break;
            }
            }
            }
        }

        if (postField) {
            math::VEC3 vel = pIt->mParameter.mVelocity;
            vel += addVel;

            bool retire = false;

            BOOL collided = AnimCurveExecutePostField(
                pIt, &postParam, pPostInfo, pPostChild, &mtxEmToGlb,
                &mtxGlbToLoc, &prevPos, addPos, &vel, &retire);

            if (retire) {
                RetireParticle(pIt);
                continue;
            }

            if (collided) {
                pIt->mParameter.mVelocity = vel;

                pIt->AddPosition(&addPos);
                pIt->AddPosition(&pIt->mParameter.mVelocity);
            }

            // Particles that leave the box come back in on the other side
            if (pPostInfo->wrap & AnimCurvePostFieldInfo::WRAP_ENABLE) {
                math::MTX34 mtxBox;
                math::MTX34Identity(&mtxBox);
                MtxScale(&mtxBox,
                         static_cast<math::VEC3*>(&pPostInfo->wrapScale),
                         &mtxBox);

                math::MTX34 mtxRot;
                MtxRotXYZRad(&mtxRot, pPostInfo->wrapRotate.x,
                             pPostInfo->wrapRotate.y, pPostInfo->wrapRotate.z);
                math::MTX34Mult(&mtxBox, &mtxRot, &mtxBox);

                math::MTX34Trans(
                    &mtxBox,
                    static_cast<math::VEC3*>(&pPostInfo->wrapTranslate),
                    &mtxBox);

                if (pPostInfo->wrap &
                    AnimCurvePostFieldInfo::WRAP_CENTER_EMITTER) {
                    math::VEC3 emPos(mtxEmToGlb._03, mtxEmToGlb._13,
                                     mtxEmToGlb._23);
                    math::MTX34Trans(&mtxBox, &emPos, &mtxBox);
                }

                math::MTX34Mult(&mtxBox, &mtxGlbToLoc, &mtxBox);

                math::MTX34 mtxBoxInv;
                math::MTX34Inv(&mtxBoxInv, &mtxBox);

                math::VEC3 pos;
                math::VEC3Transform(&pos, &mtxBoxInv,
                                    &pIt->mParameter.mPosition);

                bool wrapped = false;

                if (pos.x > 1.0f) {
                    pos.x = std::fmodf(1.0f + pos.x, 2.0f) - 1.0f;
                    wrapped = true;
                } else if (pos.x < -1.0f) {
                    pos.x = 1.0f + std::fmodf(pos.x - 1.0f, 2.0f);
                    wrapped = true;
                }

                if (pos.y > 1.0f) {
                    pos.y = std::fmodf(1.0f + pos.y, 2.0f) - 1.0f;
                    wrapped = true;
                } else if (pos.y < -1.0f) {
                    pos.y = 1.0f + std::fmodf(pos.y - 1.0f, 2.0f);
                    wrapped = true;
                }

                if (pos.z > 1.0f) {
                    pos.z = std::fmodf(1.0f + pos.z, 2.0f) - 1.0f;
                    wrapped = true;
                } else if (pos.z < -1.0f) {
                    pos.z = 1.0f + std::fmodf(pos.z - 1.0f, 2.0f);
                    wrapped = true;
                }

                if (wrapped) {
                    math::VEC3Transform(&pIt->mParameter.mPosition, &mtxBox,
                                        &pos);
                }
            }
        } else {
            pIt->mParameter.mVelocity += addVel;

            pIt->AddPosition(&addPos);
            pIt->AddPosition(&pIt->mParameter.mVelocity);
        }

        pIt->mTick++;
    }

    mLastCalced =
        static_cast<Particle*>(ut::List_GetLast(&mActivityList.mActiveList));

    if (mManagerEM->mManagerEF->mCallBack.mPostPtclCalc != NULL) {
        mManagerEM->mManagerEF->mCallBack.mPostPtclCalc(
            this, &mActivityList.mActiveList, pFirst);
    }
}

void ParticleManager::Draw(const DrawInfo& rInfo) {
    const EmitterDesc* pDesc = mResource->GetEmitterDesc();

    if ((pDesc->drawSetting.mFlags & EmitterDrawSetting::FLAG_HIDDEN) ||
        (mManagerEM->mParameter.mComFlags &
         EmitterDesc::CMN_FLAG_DISABLE_DRAW)) {
        return;
    }

    DrawInfo info = rInfo;

    f32 zOffset = mResource->GetEmitterDrawSetting()->zOffset;
    if (zOffset != 0.0f) {
        math::MTX34 mtx;
        mManagerEM->CalcGlobalMtx(&mtx);

        math::VEC3 pos(mtx._03, mtx._13, mtx._23);
        info.SetZOffset(zOffset, pos);
    }

    mDrawStrategy->Draw(info, this);
}

math::MTX34* ParticleManager::CalcGlobalMtx(math::MTX34* pResult) {
    if (mMtxDirty) {
        math::MTX34 orig;
        mManagerEM->CalcGlobalMtx(&orig);

        mManagerEM->RestructMatrix(&mMtx, &orig, mFlag & FLAG_MTX_INHERIT_SCALE,
                                   mFlag & FLAG_MTX_INHERIT_ROT,
                                   mInheritTranslate,
                                   mFlag & FLAG_MTX_INHERIT_PIVOT);

        mMtxDirty = false;
    }

    *pResult = mMtx;
    return pResult;
}

void ParticleManager::BeginCalc(bool onlyIfRemain) {
    mLastCalced = NULL;

    Particle* pIt =
        static_cast<Particle*>(mActivityList.mActiveList.headObject);

    // clang-format off
    for (; pIt != NULL; pIt = static_cast<Particle*>(
            NW4R_UT_LIST_GET_LINK(mActivityList.mActiveList, pIt)->nextObject))
    // clang-format on
    {
        if (!onlyIfRemain || pIt->mCalcRemain != 0) {
            if (pIt->mCalcRemain != 0) {
                pIt->mCalcRemain--;
            }

            if (pIt->GetLifeStatus() == NW4R_EF_LS_ACTIVE &&
                pIt->mEvalStatus == NW4R_EF_ES_DONE) {

                pIt->mEvalStatus = NW4R_EF_ES_WAIT;
            }
        }
    }
}

void ParticleManager::EndCalc() {
    Particle* pIt =
        static_cast<Particle*>(mActivityList.mActiveList.headObject);

    // clang-format off
    for (; pIt != NULL; pIt = static_cast<Particle*>(
            NW4R_UT_LIST_GET_LINK(mActivityList.mActiveList, pIt)->nextObject))
    // clang-format on
    {
        if (pIt->GetLifeStatus() == NW4R_EF_LS_ACTIVE &&
            pIt->mEvalStatus == NW4R_EF_ES_SKIP) {

            pIt->mEvalStatus = NW4R_EF_ES_DONE;
        }
    }
}

void ParticleManager::BeginDraw() {
    math::MTX34 emMtx;
    math::MTX34 pmMtx;

    mManagerEM->CalcGlobalMtx(&emMtx);
    CalcGlobalMtx(&pmMtx);

    math::MTX34Inv(&emMtx, &emMtx);
    math::MTX34Mult(&smDrawMtxPMtoEM, &emMtx, &pmMtx);
}

void ParticleManager::EndDraw() {}

void ParticleManager::Draw_ModifyColor(Particle* pParticle, GXColor* pColorPri,
                                       GXColor* pColorSec) {
    switch (mModifier.mLight.mType) {
    case ParticleModifier::SIMPLELIGHT_AMBIENT: {
        pColorPri->r = (pColorPri->r * mModifier.mLight.mAmbient.r + 128) >> 8;
        pColorPri->g = (pColorPri->g * mModifier.mLight.mAmbient.g + 128) >> 8;
        pColorPri->b = (pColorPri->b * mModifier.mLight.mAmbient.b + 128) >> 8;
        pColorPri->a = (pColorPri->a * mModifier.mLight.mAmbient.a + 128) >> 8;

        pColorSec->r = (pColorSec->r * mModifier.mLight.mAmbient.r + 128) >> 8;
        pColorSec->g = (pColorSec->g * mModifier.mLight.mAmbient.g + 128) >> 8;
        pColorSec->b = (pColorSec->b * mModifier.mLight.mAmbient.b + 128) >> 8;
        pColorSec->a = (pColorSec->a * mModifier.mLight.mAmbient.a + 128) >> 8;
        break;
    }

    case ParticleModifier::SIMPLELIGHT_DIFFUSE: {
        if (mModifier.mLight.mRadius < NW4R_MATH_FLT_EPSILON) {
            // clang-format off
            pColorPri->r = (pColorPri->r * mModifier.mLight.mAmbient.r + 128) >> 8;
            pColorPri->g = (pColorPri->g * mModifier.mLight.mAmbient.g + 128) >> 8;
            pColorPri->b = (pColorPri->b * mModifier.mLight.mAmbient.b + 128) >> 8;
            pColorPri->a = (pColorPri->a * mModifier.mLight.mAmbient.a + 128) >> 8;

            pColorSec->r = (pColorSec->r * mModifier.mLight.mAmbient.r + 128) >> 8;
            pColorSec->g = (pColorSec->g * mModifier.mLight.mAmbient.g + 128) >> 8;
            pColorSec->b = (pColorSec->b * mModifier.mLight.mAmbient.b + 128) >> 8;
            pColorSec->a = (pColorSec->a * mModifier.mLight.mAmbient.a + 128) >> 8;
            // clang-format on
        } else {
            const math::MTX34* pMtxPMtoEM = Draw_GetMtxPMtoEM();

            math::VEC3 pos;
            math::VEC3Transform(&pos, pMtxPMtoEM,
                                &pParticle->mParameter.mPosition);

            math::VEC3Sub(&pos, &pos, &mModifier.mLight.mPosition);
            f32 dist = math::VEC3Len(&pos);

            if (dist > mModifier.mLight.mRadius) {
                // clang-format off
                pColorPri->r = (pColorPri->r * mModifier.mLight.mAmbient.r + 128) >> 8;
                pColorPri->g = (pColorPri->g * mModifier.mLight.mAmbient.g + 128) >> 8;
                pColorPri->b = (pColorPri->b * mModifier.mLight.mAmbient.b + 128) >> 8;
                pColorPri->a = (pColorPri->a * mModifier.mLight.mAmbient.a + 128) >> 8;

                pColorSec->r = (pColorSec->r * mModifier.mLight.mAmbient.r + 128) >> 8;
                pColorSec->g = (pColorSec->g * mModifier.mLight.mAmbient.g + 128) >> 8;
                pColorSec->b = (pColorSec->b * mModifier.mLight.mAmbient.b + 128) >> 8;
                pColorSec->a = (pColorSec->a * mModifier.mLight.mAmbient.a + 128) >> 8;
                // clang-format on
            } else {
                s32 attn = (256 * dist) / mModifier.mLight.mRadius;

                u16 lr = mModifier.mLight.mDiffuse.r * 256 +
                         attn * (mModifier.mLight.mAmbient.r -
                                 mModifier.mLight.mDiffuse.r);

                u16 lg = mModifier.mLight.mDiffuse.g * 256 +
                         attn * (mModifier.mLight.mAmbient.g -
                                 mModifier.mLight.mDiffuse.g);

                u16 lb = mModifier.mLight.mDiffuse.b * 256 +
                         attn * (mModifier.mLight.mAmbient.b -
                                 mModifier.mLight.mDiffuse.b);

                u16 la = mModifier.mLight.mDiffuse.a * 256 +
                         attn * (mModifier.mLight.mAmbient.a -
                                 mModifier.mLight.mDiffuse.a);

                pColorPri->r = (pColorPri->r * lr + 128) >> 16;
                pColorPri->g = (pColorPri->g * lg + 128) >> 16;
                pColorPri->b = (pColorPri->b * lb + 128) >> 16;
                pColorPri->a = (pColorPri->a * la + 128) >> 16;

                pColorSec->r = (pColorSec->r * lr + 128) >> 16;
                pColorSec->g = (pColorSec->g * lg + 128) >> 16;
                pColorSec->b = (pColorSec->b * lb + 128) >> 16;
                pColorSec->a = (pColorSec->a * la + 128) >> 16;
            }
        }
        break;
    }

    default: {
        break;
    }
    }
}

} // namespace ef
} // namespace nw4r
