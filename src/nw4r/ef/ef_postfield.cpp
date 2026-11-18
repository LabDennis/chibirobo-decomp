#include "nw4r/ef.h"

#include <cmath>

// Retail starts a second literal pool at 0x805A6DF0 for the code from 0x8023E9C4 on (it repeats
// constants the animation curve pool already holds), so the post field code is a file of its own.
// Its original name is not known.

namespace nw4r {
namespace ef {

/******************************************************************************
 *
 * Post field: collision of a particle with a shape
 *
 * Retail has its own literal pool from here on (.text 0x8023E9C4, .sdata2
 * 0x805A6DF0), so this is a file of its own in the original; its name is not known.
 * Two things about how it was built show in the code: the six shape tests are called,
 * never expanded, although they are small and stand above their callers (no automatic
 * inlining), and math::FSqrt has no "out of the domain" warning.
 *
 ******************************************************************************/

// The collision shapes are unit shapes: the post field matrix scales them.
// Every test takes the position in the space of the shape.

int CheckCollisionSphere(const math::VEC3* pPos, u8 type) {
    math::VEC3 pos = *pPos;

    if (pos.y < -1.0f || pos.y > 1.0f) {
        return AnimCurvePostFieldInfo::STATUS_OUTSIDE;
    }

    if ((type & AnimCurvePostFieldInfo::SPHERE_TOP) && pos.y < 0.0f) {
        return AnimCurvePostFieldInfo::STATUS_OUTSIDE;
    }

    if ((type & AnimCurvePostFieldInfo::SPHERE_BOTTOM) && pos.y > 0.0f) {
        return AnimCurvePostFieldInfo::STATUS_OUTSIDE;
    }

    if (pos.x < -1.0f || pos.x > 1.0f) {
        return AnimCurvePostFieldInfo::STATUS_OUTSIDE;
    }

    if (pos.z < -1.0f || pos.z > 1.0f) {
        return AnimCurvePostFieldInfo::STATUS_OUTSIDE;
    }

    f32 distSq = math::VEC3LenSq(&pos);

    if (distSq > 1.0f) {
        return AnimCurvePostFieldInfo::STATUS_OUTSIDE;
    }

    if (1.0f == distSq) {
        return AnimCurvePostFieldInfo::STATUS_ON;
    }

    if (type != AnimCurvePostFieldInfo::SPHERE_WHOLE && 0.0f == pos.y) {
        return AnimCurvePostFieldInfo::STATUS_ON;
    }

    return AnimCurvePostFieldInfo::STATUS_INSIDE;
}

int CheckCollisionCube(const math::VEC3* pPos, u8 type) {
    math::VEC3 pos = *pPos;
    bool on = false;

    if (pos.x < -1.0f || pos.x > 1.0f) {
        return AnimCurvePostFieldInfo::STATUS_OUTSIDE;
    }

    if (1.0f == pos.x || -1.0f == pos.x) {
        on = true;
    }

    if (pos.y < -1.0f || pos.y > 1.0f) {
        return AnimCurvePostFieldInfo::STATUS_OUTSIDE;
    }

    if (1.0f == pos.y || -1.0f == pos.y) {
        on = true;
    }

    if (pos.z < -1.0f || pos.z > 1.0f) {
        return AnimCurvePostFieldInfo::STATUS_OUTSIDE;
    }

    if (on || 1.0f == pos.z || -1.0f == pos.z) {
        return AnimCurvePostFieldInfo::STATUS_ON;
    }

    return AnimCurvePostFieldInfo::STATUS_INSIDE;
}

int CheckCollisionCylinder(const math::VEC3* pPos, u8 type) {
    math::VEC3 pos = *pPos;

    if (pos.y < -1.0f || pos.y > 1.0f) {
        return AnimCurvePostFieldInfo::STATUS_OUTSIDE;
    }

    if (pos.x < -1.0f || pos.x > 1.0f) {
        return AnimCurvePostFieldInfo::STATUS_OUTSIDE;
    }

    if (pos.z < -1.0f || pos.z > 1.0f) {
        return AnimCurvePostFieldInfo::STATUS_OUTSIDE;
    }

    f32 distSq = pos.x * pos.x + pos.z * pos.z;

    if (distSq > 1.0f) {
        return AnimCurvePostFieldInfo::STATUS_OUTSIDE;
    }

    if (1.0f == distSq) {
        return AnimCurvePostFieldInfo::STATUS_ON;
    }

    if (-1.0f == pos.y || 1.0f == pos.y) {
        return AnimCurvePostFieldInfo::STATUS_ON;
    }

    return AnimCurvePostFieldInfo::STATUS_INSIDE;
}

// Turns the position so that the plane of the shape is y = 0
static inline void ToPlaneSpace(math::VEC3* pOut, const math::VEC3* pPos,
                                u8 plane) {
    switch (plane) {
    case AnimCurvePostFieldInfo::PLANE_XZ: {
        pOut->x = pPos->x;
        pOut->y = pPos->y;
        pOut->z = pPos->z;
        break;
    }
    case AnimCurvePostFieldInfo::PLANE_XY: {
        pOut->x = pPos->x;
        pOut->y = pPos->z;
        pOut->z = -pPos->y;
        break;
    }
    case AnimCurvePostFieldInfo::PLANE_YZ: {
        pOut->x = -pPos->y;
        pOut->y = pPos->x;
        pOut->z = pPos->z;
        break;
    }
    }
}

int CheckCollisionCircle(const math::VEC3* pPos, u8 plane) {
    math::VEC3 pos;
    ToPlaneSpace(&pos, pPos, plane);

    if (pos.y > 0.0f) {
        return AnimCurvePostFieldInfo::STATUS_OUTSIDE;
    }

    if (pos.y < 0.0f) {
        return AnimCurvePostFieldInfo::STATUS_OUTSIDE;
    }

    if (pos.x < -1.0f || pos.x > 1.0f) {
        return AnimCurvePostFieldInfo::STATUS_BESIDE;
    }

    if (pos.z < -1.0f || pos.z > 1.0f) {
        return AnimCurvePostFieldInfo::STATUS_BESIDE;
    }

    if (pos.x * pos.x + pos.z * pos.z > 1.0f) {
        return AnimCurvePostFieldInfo::STATUS_BESIDE;
    }

    return AnimCurvePostFieldInfo::STATUS_ON;
}

int CheckCollisionRectangle(const math::VEC3* pPos, u8 plane) {
    math::VEC3 pos;
    ToPlaneSpace(&pos, pPos, plane);

    if (pos.y > 0.0f) {
        return AnimCurvePostFieldInfo::STATUS_OUTSIDE;
    }

    if (pos.y < 0.0f) {
        return AnimCurvePostFieldInfo::STATUS_OUTSIDE;
    }

    if (pos.x < -1.0f || pos.x > 1.0f) {
        return AnimCurvePostFieldInfo::STATUS_BESIDE;
    }

    if (pos.z < -1.0f || pos.z > 1.0f) {
        return AnimCurvePostFieldInfo::STATUS_BESIDE;
    }

    return AnimCurvePostFieldInfo::STATUS_ON;
}

int CheckCollisionPlane(const math::VEC3* pPos, u8 plane) {
    math::VEC3 pos;
    ToPlaneSpace(&pos, pPos, plane);

    if (pos.y > 0.0f) {
        return AnimCurvePostFieldInfo::STATUS_INSIDE;
    }

    if (pos.y < 0.0f) {
        return AnimCurvePostFieldInfo::STATUS_OUTSIDE;
    }

    return AnimCurvePostFieldInfo::STATUS_ON;
}

// The collision functions return where on the way from pStart to pEnd the particle hits
// the shape (0 to 1), or 2 when it does not. pHit is the place in the space of the shape.
f32 CollidePlane(const math::MTX34* pMtx, const AnimCurvePostFieldInfo* pInfo,
                 const math::VEC3* pEnd, const math::VEC3* pStart,
                 math::VEC3* pDiff, math::VEC3* pVel, math::VEC3* pHit,
                 math::VEC3* pHitWorld) {
    switch (pInfo->shapeOption) {
    case AnimCurvePostFieldInfo::PLANE_XZ: {
        if (math::FAbs(pStart->y) < NW4R_MATH_FLT_EPSILON &&
            math::FAbs(pEnd->y) < NW4R_MATH_FLT_EPSILON) {
            return 2.0f;
        }

        if (pStart->y > 0.0f && pEnd->y < NW4R_MATH_FLT_EPSILON) {
            f32 t = -pStart->y / pDiff->y;
            math::VEC3Lerp(pHit, pStart, pEnd, t);

            if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                pVel->y *= -1.0f;
                pDiff->y *= -1.0f;
                pHit->y += NW4R_MATH_FLT_EPSILON;
            }

            math::VEC3Transform(pHitWorld, pMtx, pHit);
            return t;
        }

        if (pStart->y < 0.0f && pEnd->y > NW4R_MATH_FLT_EPSILON) {
            f32 t = -pStart->y / pDiff->y;
            math::VEC3Lerp(pHit, pStart, pEnd, t);

            if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                pVel->y *= -1.0f;
                pDiff->y *= -1.0f;
                pHit->y -= NW4R_MATH_FLT_EPSILON;
            }

            math::VEC3Transform(pHitWorld, pMtx, pHit);
            return t;
        }

        return 2.0f;
    }
    case AnimCurvePostFieldInfo::PLANE_XY: {
        if (math::FAbs(pStart->z) < NW4R_MATH_FLT_EPSILON &&
            math::FAbs(pEnd->z) < NW4R_MATH_FLT_EPSILON) {
            return 2.0f;
        }

        if (pStart->z > 0.0f && pEnd->z < NW4R_MATH_FLT_EPSILON) {
            f32 t = -pStart->z / pDiff->z;
            math::VEC3Lerp(pHit, pStart, pEnd, t);

            if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                pVel->z *= -1.0f;
                pDiff->z *= -1.0f;
                pHit->z += NW4R_MATH_FLT_EPSILON;
            }

            math::VEC3Transform(pHitWorld, pMtx, pHit);
            return t;
        }

        if (pStart->z < 0.0f && pEnd->z > NW4R_MATH_FLT_EPSILON) {
            f32 t = -pStart->z / pDiff->z;
            math::VEC3Lerp(pHit, pStart, pEnd, t);

            if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                pVel->z *= -1.0f;
                pDiff->z *= -1.0f;
                pHit->z -= NW4R_MATH_FLT_EPSILON;
            }

            math::VEC3Transform(pHitWorld, pMtx, pHit);
            return t;
        }

        return 2.0f;
    }
    case AnimCurvePostFieldInfo::PLANE_YZ: {
        if (math::FAbs(pStart->x) < NW4R_MATH_FLT_EPSILON &&
            math::FAbs(pEnd->x) < NW4R_MATH_FLT_EPSILON) {
            return 2.0f;
        }

        if (pStart->x > 0.0f && pEnd->x < NW4R_MATH_FLT_EPSILON) {
            f32 t = -pStart->x / pDiff->x;
            math::VEC3Lerp(pHit, pStart, pEnd, t);

            if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                pVel->x *= -1.0f;
                pDiff->x *= -1.0f;
                pHit->x += NW4R_MATH_FLT_EPSILON;
            }

            math::VEC3Transform(pHitWorld, pMtx, pHit);
            return t;
        }

        if (pStart->x < 0.0f && pEnd->x > NW4R_MATH_FLT_EPSILON) {
            f32 t = -pStart->x / pDiff->x;
            math::VEC3Lerp(pHit, pStart, pEnd, t);

            if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                pVel->x *= -1.0f;
                pDiff->x *= -1.0f;
                pHit->x -= NW4R_MATH_FLT_EPSILON;
            }

            math::VEC3Transform(pHitWorld, pMtx, pHit);
            return t;
        }

        return 2.0f;
    }
    }

    return 2.0f;
}

f32 CollideRectangle(const math::MTX34* pMtx, const AnimCurvePostFieldInfo* pInfo,
                     const math::VEC3* pEnd, const math::VEC3* pStart,
                     math::VEC3* pDiff, math::VEC3* pVel, math::VEC3* pHit,
                     math::VEC3* pHitWorld) {
    switch (pInfo->shapeOption) {
    case AnimCurvePostFieldInfo::PLANE_XZ: {
        if (math::FAbs(pStart->y) < NW4R_MATH_FLT_EPSILON &&
            math::FAbs(pEnd->y) < NW4R_MATH_FLT_EPSILON) {
            return 2.0f;
        }

        if (pStart->y > 0.0f && pEnd->y < NW4R_MATH_FLT_EPSILON) {
            f32 t = -pStart->y / pDiff->y;
            math::VEC3Lerp(pHit, pStart, pEnd, t);

            if (math::FAbs(pHit->x) < 1.0f + NW4R_MATH_FLT_EPSILON &&
                    math::FAbs(pHit->z) < 1.0f + NW4R_MATH_FLT_EPSILON) {
                if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                    pVel->y *= -1.0f;
                    pDiff->y *= -1.0f;
                    pHit->y += NW4R_MATH_FLT_EPSILON;
                }

                math::VEC3Transform(pHitWorld, pMtx, pHit);
                return t;
            }

            return 2.0f;
        }

        if (pStart->y < 0.0f && pEnd->y > NW4R_MATH_FLT_EPSILON) {
            f32 t = -pStart->y / pDiff->y;
            math::VEC3Lerp(pHit, pStart, pEnd, t);

            if (math::FAbs(pHit->x) < 1.0f + NW4R_MATH_FLT_EPSILON &&
                    math::FAbs(pHit->z) < 1.0f + NW4R_MATH_FLT_EPSILON) {
                if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                    pVel->y *= -1.0f;
                    pDiff->y *= -1.0f;
                    pHit->y -= NW4R_MATH_FLT_EPSILON;
                }

                math::VEC3Transform(pHitWorld, pMtx, pHit);
                return t;
            }

            return 2.0f;
        }

        return 2.0f;
    }
    case AnimCurvePostFieldInfo::PLANE_XY: {
        if (math::FAbs(pStart->z) < NW4R_MATH_FLT_EPSILON &&
            math::FAbs(pEnd->z) < NW4R_MATH_FLT_EPSILON) {
            return 2.0f;
        }

        if (pStart->z > 0.0f && pEnd->z < NW4R_MATH_FLT_EPSILON) {
            f32 t = -pStart->z / pDiff->z;
            math::VEC3Lerp(pHit, pStart, pEnd, t);

            if (math::FAbs(pHit->x) < 1.0f + NW4R_MATH_FLT_EPSILON &&
                    math::FAbs(pHit->y) < 1.0f + NW4R_MATH_FLT_EPSILON) {
                if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                    pVel->z *= -1.0f;
                    pDiff->z *= -1.0f;
                    pHit->z += NW4R_MATH_FLT_EPSILON;
                }

                math::VEC3Transform(pHitWorld, pMtx, pHit);
                return t;
            }

            return 2.0f;
        }

        if (pStart->z < 0.0f && pEnd->z > NW4R_MATH_FLT_EPSILON) {
            f32 t = -pStart->z / pDiff->z;
            math::VEC3Lerp(pHit, pStart, pEnd, t);

            if (math::FAbs(pHit->x) < 1.0f + NW4R_MATH_FLT_EPSILON &&
                    math::FAbs(pHit->y) < 1.0f + NW4R_MATH_FLT_EPSILON) {
                if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                    pVel->z *= -1.0f;
                    pDiff->z *= -1.0f;
                    pHit->z -= NW4R_MATH_FLT_EPSILON;
                }

                math::VEC3Transform(pHitWorld, pMtx, pHit);
                return t;
            }

            return 2.0f;
        }

        return 2.0f;
    }
    case AnimCurvePostFieldInfo::PLANE_YZ: {
        if (math::FAbs(pStart->x) < NW4R_MATH_FLT_EPSILON &&
            math::FAbs(pEnd->x) < NW4R_MATH_FLT_EPSILON) {
            return 2.0f;
        }

        if (pStart->x > 0.0f && pEnd->x < NW4R_MATH_FLT_EPSILON) {
            f32 t = -pStart->x / pDiff->x;
            math::VEC3Lerp(pHit, pStart, pEnd, t);

            if (math::FAbs(pHit->y) < 1.0f + NW4R_MATH_FLT_EPSILON &&
                    math::FAbs(pHit->z) < 1.0f + NW4R_MATH_FLT_EPSILON) {
                if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                    pVel->x *= -1.0f;
                    pDiff->x *= -1.0f;
                    pHit->x += NW4R_MATH_FLT_EPSILON;
                }

                math::VEC3Transform(pHitWorld, pMtx, pHit);
                return t;
            }

            return 2.0f;
        }

        if (pStart->x < 0.0f && pEnd->x > NW4R_MATH_FLT_EPSILON) {
            f32 t = -pStart->x / pDiff->x;
            math::VEC3Lerp(pHit, pStart, pEnd, t);

            if (math::FAbs(pHit->y) < 1.0f + NW4R_MATH_FLT_EPSILON &&
                    math::FAbs(pHit->z) < 1.0f + NW4R_MATH_FLT_EPSILON) {
                if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                    pVel->x *= -1.0f;
                    pDiff->x *= -1.0f;
                    pHit->x -= NW4R_MATH_FLT_EPSILON;
                }

                math::VEC3Transform(pHitWorld, pMtx, pHit);
                return t;
            }

            return 2.0f;
        }

        return 2.0f;
    }
    }

    return 2.0f;
}

f32 CollideCircle(const math::MTX34* pMtx, const AnimCurvePostFieldInfo* pInfo,
                  const math::VEC3* pEnd, const math::VEC3* pStart,
                  math::VEC3* pDiff, math::VEC3* pVel, math::VEC3* pHit,
                  math::VEC3* pHitWorld) {
    switch (pInfo->shapeOption) {
    case AnimCurvePostFieldInfo::PLANE_XZ: {
        if (math::FAbs(pStart->y) < NW4R_MATH_FLT_EPSILON &&
            math::FAbs(pEnd->y) < NW4R_MATH_FLT_EPSILON) {
            return 2.0f;
        }

        if (pStart->y > 0.0f && pEnd->y < NW4R_MATH_FLT_EPSILON) {
            f32 t = -pStart->y / pDiff->y;
            math::VEC3Lerp(pHit, pStart, pEnd, t);

            if (pHit->x * pHit->x + pHit->z * pHit->z <
                    1.0f + 2.0f * NW4R_MATH_FLT_EPSILON) {
                if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                    pVel->y *= -1.0f;
                    pDiff->y *= -1.0f;
                    pHit->y += NW4R_MATH_FLT_EPSILON;
                }

                math::VEC3Transform(pHitWorld, pMtx, pHit);
                return t;
            }

            return 2.0f;
        }

        if (pStart->y < 0.0f && pEnd->y > NW4R_MATH_FLT_EPSILON) {
            f32 t = -pStart->y / pDiff->y;
            math::VEC3Lerp(pHit, pStart, pEnd, t);

            if (pHit->x * pHit->x + pHit->z * pHit->z <
                    1.0f + 2.0f * NW4R_MATH_FLT_EPSILON) {
                if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                    pVel->y *= -1.0f;
                    pDiff->y *= -1.0f;
                    pHit->y -= NW4R_MATH_FLT_EPSILON;
                }

                math::VEC3Transform(pHitWorld, pMtx, pHit);
                return t;
            }

            return 2.0f;
        }

        return 2.0f;
    }
    case AnimCurvePostFieldInfo::PLANE_XY: {
        if (math::FAbs(pStart->z) < NW4R_MATH_FLT_EPSILON &&
            math::FAbs(pEnd->z) < NW4R_MATH_FLT_EPSILON) {
            return 2.0f;
        }

        if (pStart->z > 0.0f && pEnd->z < NW4R_MATH_FLT_EPSILON) {
            f32 t = -pStart->z / pDiff->z;
            math::VEC3Lerp(pHit, pStart, pEnd, t);

            if (pHit->x * pHit->x + pHit->y * pHit->y <
                    1.0f + 2.0f * NW4R_MATH_FLT_EPSILON) {
                if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                    pVel->z *= -1.0f;
                    pDiff->z *= -1.0f;
                    pHit->z += NW4R_MATH_FLT_EPSILON;
                }

                math::VEC3Transform(pHitWorld, pMtx, pHit);
                return t;
            }

            return 2.0f;
        }

        if (pStart->z < 0.0f && pEnd->z > NW4R_MATH_FLT_EPSILON) {
            f32 t = -pStart->z / pDiff->z;
            math::VEC3Lerp(pHit, pStart, pEnd, t);

            if (pHit->x * pHit->x + pHit->y * pHit->y <
                    1.0f + 2.0f * NW4R_MATH_FLT_EPSILON) {
                if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                    pVel->z *= -1.0f;
                    pDiff->z *= -1.0f;
                    pHit->z -= NW4R_MATH_FLT_EPSILON;
                }

                math::VEC3Transform(pHitWorld, pMtx, pHit);
                return t;
            }

            return 2.0f;
        }

        return 2.0f;
    }
    case AnimCurvePostFieldInfo::PLANE_YZ: {
        if (math::FAbs(pStart->x) < NW4R_MATH_FLT_EPSILON &&
            math::FAbs(pEnd->x) < NW4R_MATH_FLT_EPSILON) {
            return 2.0f;
        }

        if (pStart->x > 0.0f && pEnd->x < NW4R_MATH_FLT_EPSILON) {
            f32 t = -pStart->x / pDiff->x;
            math::VEC3Lerp(pHit, pStart, pEnd, t);

            if (pHit->y * pHit->y + pHit->z * pHit->z <
                    1.0f + 2.0f * NW4R_MATH_FLT_EPSILON) {
                if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                    pVel->x *= -1.0f;
                    pDiff->x *= -1.0f;
                    pHit->x += NW4R_MATH_FLT_EPSILON;
                }

                math::VEC3Transform(pHitWorld, pMtx, pHit);
                return t;
            }

            return 2.0f;
        }

        if (pStart->x < 0.0f && pEnd->x > NW4R_MATH_FLT_EPSILON) {
            f32 t = -pStart->x / pDiff->x;
            math::VEC3Lerp(pHit, pStart, pEnd, t);

            if (pHit->y * pHit->y + pHit->z * pHit->z <
                    1.0f + 2.0f * NW4R_MATH_FLT_EPSILON) {
                if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                    pVel->x *= -1.0f;
                    pDiff->x *= -1.0f;
                    pHit->x -= NW4R_MATH_FLT_EPSILON;
                }

                math::VEC3Transform(pHitWorld, pMtx, pHit);
                return t;
            }

            return 2.0f;
        }

        return 2.0f;
    }
    }

    return 2.0f;
}

// NONMATCHING: retail's code (0x638 bytes both) but for registers: a and result swap
// f30 and f29 (retail a f30, result f29).
f32 CollideSphere(const math::MTX34* pMtx, const AnimCurvePostFieldInfo* pInfo,
                  const math::VEC3* pEnd, const math::VEC3* pStart,
                  math::VEC3* pDiff, math::VEC3* pVel, math::VEC3* pHit,
                  math::VEC3* pHitWorld) {
    // |start + t * diff| = 1
    f32 startSq = math::VEC3LenSq(pStart);
    f32 a = math::VEC3LenSq(pDiff);
    f32 b = math::VEC3Dot(pStart, pDiff);
    f32 result = 2.0f;

    f32 d = 4.0f * b * b - 4.0f * a * (startSq - 1.0f);
    if (d < 0.0f) {
        return result;
    }

    f32 root = math::FSqrt(d);
    f32 t0 = (-2.0f * b - root) / (2.0f * a);
    f32 t1 = (-2.0f * b + root) / (2.0f * a);

    if (t0 > 0.0f && t0 < 1.0f + NW4R_MATH_FLT_EPSILON) {
        math::VEC3Lerp(pHit, pStart, pEnd, t0);

        if (!(pInfo->shapeOption == AnimCurvePostFieldInfo::SPHERE_TOP &&
              pHit->y < -NW4R_MATH_FLT_EPSILON) &&
            !(pInfo->shapeOption == AnimCurvePostFieldInfo::SPHERE_BOTTOM &&
              pHit->y > NW4R_MATH_FLT_EPSILON)) {
            result = t0;
        }
    }

    if (t1 > 0.0f && t1 < 1.0f + NW4R_MATH_FLT_EPSILON && t1 < result) {
        math::VEC3 hit;
        math::VEC3Lerp(&hit, pStart, pEnd, t1);

        if (!(pInfo->shapeOption == AnimCurvePostFieldInfo::SPHERE_TOP &&
              hit.y < -NW4R_MATH_FLT_EPSILON) &&
            !(pInfo->shapeOption == AnimCurvePostFieldInfo::SPHERE_BOTTOM &&
              hit.y > NW4R_MATH_FLT_EPSILON)) {
            result = t1;
            *pHit = hit;
        }
    }

    // The flat side of a half sphere
    if (pInfo->shapeOption == AnimCurvePostFieldInfo::SPHERE_TOP ||
        pInfo->shapeOption == AnimCurvePostFieldInfo::SPHERE_BOTTOM) {

        if (!(math::FAbs(pStart->y) < NW4R_MATH_FLT_EPSILON &&
              math::FAbs(pEnd->y) < NW4R_MATH_FLT_EPSILON)) {

            if (pStart->y > 0.0f && pEnd->y < NW4R_MATH_FLT_EPSILON) {
                f32 t = -pStart->y / pDiff->y;
                math::VEC3 hit;
                math::VEC3Lerp(&hit, pStart, pEnd, t);

                if (t < result && hit.x * hit.x + hit.z * hit.z <
                                      1.0f + 2.0f * NW4R_MATH_FLT_EPSILON) {
                    *pHit = hit;

                    if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                        pVel->y *= -1.0f;
                        pDiff->y *= -1.0f;
                        pHit->y += NW4R_MATH_FLT_EPSILON;
                    }

                    math::VEC3Transform(pHitWorld, pMtx, pHit);
                    return t;
                }
            } else if (pStart->y < 0.0f && pEnd->y > NW4R_MATH_FLT_EPSILON) {
                f32 t = -pStart->y / pDiff->y;
                math::VEC3 hit;
                math::VEC3Lerp(&hit, pStart, pEnd, t);

                if (t < result && hit.x * hit.x + hit.z * hit.z <
                                      1.0f + 2.0f * NW4R_MATH_FLT_EPSILON) {
                    *pHit = hit;

                    if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                        pVel->y *= -1.0f;
                        pDiff->y *= -1.0f;
                        pHit->y -= NW4R_MATH_FLT_EPSILON;
                    }

                    math::VEC3Transform(pHitWorld, pMtx, pHit);
                    return t;
                }
            }
        }
    }

    if (result < 2.0f) {
        if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
            // A frame whose y axis is the normal of the sphere at the hit
            f32 len = math::FSqrt(1.0f - pHit->z * pHit->z);
            f32 c;
            f32 s;

            if (len > NW4R_MATH_FLT_EPSILON) {
                c = pHit->y / len;
                s = -pHit->x / len;
            } else {
                c = 1.0f;
                s = 0.0f;
            }

            math::MTX34 mtx;
            mtx._00 = c;
            mtx._01 = pHit->x;
            mtx._02 = pHit->z * s;
            mtx._03 = 0.0f;
            mtx._10 = s;
            mtx._11 = pHit->y;
            mtx._12 = -c * pHit->z;
            mtx._13 = 0.0f;
            mtx._20 = 0.0f;
            mtx._21 = pHit->z;
            mtx._22 = len;
            mtx._23 = 0.0f;
            math::MTX34Inv(&mtx, &mtx);

            math::VEC3 local;
            math::VEC3 normal;

            math::VEC3Transform(&local, &mtx, pDiff);
            math::VEC3Scale(&normal, pHit, local.y);
            *pDiff -= normal;
            *pDiff -= normal;

            math::VEC3Transform(&local, &mtx, pVel);
            math::VEC3Scale(&normal, pHit, local.y);
            *pVel -= normal;
            *pVel -= normal;

            // Off the surface, to the side the particle came from
            if (startSq > 1.0f) {
                math::VEC3Scale(pHit, pHit, 1.00001f);
            } else {
                math::VEC3Scale(pHit, pHit, 0.99999f);
            }
        }

        math::VEC3Transform(pHitWorld, pMtx, pHit);
    }

    return result;
}

// NONMATCHING: retail's code (0x7D4 bytes both) but for registers: startSq and a swap
// f31 and f29 (retail a f31, startSq f29).
f32 CollideCylinder(const math::MTX34* pMtx, const AnimCurvePostFieldInfo* pInfo,
                    const math::VEC3* pEnd, const math::VEC3* pStart,
                    math::VEC3* pDiff, math::VEC3* pVel, math::VEC3* pHit,
                    math::VEC3* pHitWorld) {
    // The same as the sphere, in the xz plane
    f32 startSq = pStart->x * pStart->x + pStart->z * pStart->z;
    f32 a = pDiff->x * pDiff->x + pDiff->z * pDiff->z;
    f32 b = pStart->x * pDiff->x + pStart->z * pDiff->z;
    f32 result = 2.0f;

    f32 d = 4.0f * b * b - 4.0f * a * (startSq - 1.0f);
    if (d < 0.0f) {
        return result;
    }

    f32 root = math::FSqrt(d);
    f32 t0 = (-2.0f * b - root) / (2.0f * a);
    f32 t1 = (-2.0f * b + root) / (2.0f * a);

    if (t0 > 0.0f && t0 < 1.0f + NW4R_MATH_FLT_EPSILON) {
        math::VEC3Lerp(pHit, pStart, pEnd, t0);

        if (math::FAbs(pHit->y) < 1.0f + NW4R_MATH_FLT_EPSILON) {
            result = t0;
        }
    }

    if (t1 > 0.0f && t1 < 1.0f + NW4R_MATH_FLT_EPSILON && t1 < result) {
        math::VEC3 hit;
        math::VEC3Lerp(&hit, pStart, pEnd, t1);

        if (math::FAbs(hit.y) < 1.0f + NW4R_MATH_FLT_EPSILON) {
            result = t1;
            *pHit = hit;
        }
    }

    // Top
    {
        f32 y = 1.0f;

        if (!(math::FAbs(pStart->y - y) < NW4R_MATH_FLT_EPSILON &&
              math::FAbs(pEnd->y - y) < NW4R_MATH_FLT_EPSILON)) {

            if (pStart->y > y && pEnd->y < NW4R_MATH_FLT_EPSILON + y) {
                f32 t = (y - pStart->y) / pDiff->y;
                math::VEC3 hit;
                math::VEC3Lerp(&hit, pStart, pEnd, t);

                if (t < result && hit.x * hit.x + hit.z * hit.z <
                                      1.0f + 2.0f * NW4R_MATH_FLT_EPSILON) {
                    *pHit = hit;

                    if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                        pVel->y *= -1.0f;
                        pDiff->y *= -1.0f;
                        pHit->y += NW4R_MATH_FLT_EPSILON;
                    }

                    math::VEC3Transform(pHitWorld, pMtx, pHit);
                    return t;
                }
            } else if (pStart->y < y &&
                       pEnd->y > NW4R_MATH_FLT_EPSILON + y) {
                f32 t = (y - pStart->y) / pDiff->y;
                math::VEC3 hit;
                math::VEC3Lerp(&hit, pStart, pEnd, t);

                if (t < result && hit.x * hit.x + hit.z * hit.z <
                                      1.0f + 2.0f * NW4R_MATH_FLT_EPSILON) {
                    *pHit = hit;

                    if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                        pVel->y *= -1.0f;
                        pDiff->y *= -1.0f;
                        pHit->y -= NW4R_MATH_FLT_EPSILON;
                    }

                    math::VEC3Transform(pHitWorld, pMtx, pHit);
                    return t;
                }
            }
        }
    }

    // Bottom
    {
        f32 y = -1.0f;

        if (!(math::FAbs(pStart->y - y) < NW4R_MATH_FLT_EPSILON &&
              math::FAbs(pEnd->y - y) < NW4R_MATH_FLT_EPSILON)) {

            if (pStart->y > y && pEnd->y < NW4R_MATH_FLT_EPSILON + y) {
                f32 t = (y - pStart->y) / pDiff->y;
                math::VEC3 hit;
                math::VEC3Lerp(&hit, pStart, pEnd, t);

                if (t < result && hit.x * hit.x + hit.z * hit.z <
                                      1.0f + 2.0f * NW4R_MATH_FLT_EPSILON) {
                    *pHit = hit;

                    if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                        pVel->y *= -1.0f;
                        pDiff->y *= -1.0f;
                        pHit->y += NW4R_MATH_FLT_EPSILON;
                    }

                    math::VEC3Transform(pHitWorld, pMtx, pHit);
                    return t;
                }
            } else if (pStart->y < y &&
                       pEnd->y > NW4R_MATH_FLT_EPSILON + y) {
                f32 t = (y - pStart->y) / pDiff->y;
                math::VEC3 hit;
                math::VEC3Lerp(&hit, pStart, pEnd, t);

                if (t < result && hit.x * hit.x + hit.z * hit.z <
                                      1.0f + 2.0f * NW4R_MATH_FLT_EPSILON) {
                    *pHit = hit;

                    if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
                        pVel->y *= -1.0f;
                        pDiff->y *= -1.0f;
                        pHit->y -= NW4R_MATH_FLT_EPSILON;
                    }

                    math::VEC3Transform(pHitWorld, pMtx, pHit);
                    return t;
                }
            }
        }
    }

    if (result < 2.0f) {
        if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
            f32 len;
            f32 dot;
            math::VEC3 normal;

            // The length is computed and not used
            len = math::FSqrt(pDiff->x * pDiff->x + pDiff->z * pDiff->z);
            dot = pDiff->x * pHit->x + pDiff->z * pHit->z;
            normal.x = pHit->x;
            normal.y = 0.0f;
            normal.z = pHit->z;
            math::VEC3Normalize(&normal, &normal);
            math::VEC3Scale(&normal, &normal, dot);
            *pDiff -= normal;
            *pDiff -= normal;

            len = math::FSqrt(pVel->x * pVel->x + pVel->z * pVel->z);
            dot = pVel->x * pHit->x + pVel->z * pHit->z;
            normal.x = pHit->x;
            normal.y = 0.0f;
            normal.z = pHit->z;
            math::VEC3Normalize(&normal, &normal);
            math::VEC3Scale(&normal, &normal, dot);
            *pVel -= normal;
            *pVel -= normal;

            if (startSq > 1.0f) {
                math::VEC3Scale(pHit, pHit, 1.00001f);
            } else {
                math::VEC3Scale(pHit, pHit, 0.99999f);
            }
        }

        math::VEC3Transform(pHitWorld, pMtx, pHit);
    }

    return result;
}

f32 CollideCube(const math::MTX34* pMtx, const AnimCurvePostFieldInfo* pInfo,
                const math::VEC3* pEnd, const math::VEC3* pStart,
                math::VEC3* pDiff, math::VEC3* pVel, math::VEC3* pHit,
                math::VEC3* pHitWorld) {
    f32 result = 2.0f;
    f32 xPlus = 2.0f;
    f32 xMinus = 2.0f;
    f32 yPlus = 2.0f;
    f32 yMinus = 2.0f;
    f32 zPlus = 2.0f;
    f32 zMinus = 2.0f;
    math::VEC3 hit;
    f32 t;

    t = (1.0f - pStart->x) / pDiff->x;
    if (t > 0.0f && t <= 1.0f + NW4R_MATH_FLT_EPSILON) {
        math::VEC3Lerp(&hit, pStart, pEnd, t);
        if (math::FAbs(hit.y) <= 1.0f + NW4R_MATH_FLT_EPSILON &&
            math::FAbs(hit.z) <= 1.0f + NW4R_MATH_FLT_EPSILON) {
            xPlus = t;
            if (t < result) {
                result = t;
            }
        }
    }

    t = (-1.0f - pStart->x) / pDiff->x;
    if (t > 0.0f && t <= 1.0f + NW4R_MATH_FLT_EPSILON) {
        math::VEC3Lerp(&hit, pStart, pEnd, t);
        if (math::FAbs(hit.y) <= 1.0f + NW4R_MATH_FLT_EPSILON &&
            math::FAbs(hit.z) <= 1.0f + NW4R_MATH_FLT_EPSILON) {
            xMinus = t;
            if (t < result) {
                result = t;
            }
        }
    }

    t = (1.0f - pStart->y) / pDiff->y;
    if (t > 0.0f && t <= 1.0f + NW4R_MATH_FLT_EPSILON) {
        math::VEC3Lerp(&hit, pStart, pEnd, t);
        if (math::FAbs(hit.x) <= 1.0f + NW4R_MATH_FLT_EPSILON &&
            math::FAbs(hit.z) <= 1.0f + NW4R_MATH_FLT_EPSILON) {
            yPlus = t;
            if (t < result) {
                result = t;
            }
        }
    }

    t = (-1.0f - pStart->y) / pDiff->y;
    if (t > 0.0f && t <= 1.0f + NW4R_MATH_FLT_EPSILON) {
        math::VEC3Lerp(&hit, pStart, pEnd, t);
        if (math::FAbs(hit.x) <= 1.0f + NW4R_MATH_FLT_EPSILON &&
            math::FAbs(hit.z) <= 1.0f + NW4R_MATH_FLT_EPSILON) {
            yMinus = t;
            if (t < result) {
                result = t;
            }
        }
    }

    t = (1.0f - pStart->z) / pDiff->z;
    if (t > 0.0f && t <= 1.0f + NW4R_MATH_FLT_EPSILON) {
        math::VEC3Lerp(&hit, pStart, pEnd, t);
        if (math::FAbs(hit.x) <= 1.0f + NW4R_MATH_FLT_EPSILON &&
            math::FAbs(hit.y) <= 1.0f + NW4R_MATH_FLT_EPSILON) {
            zPlus = t;
            if (t < result) {
                result = t;
            }
        }
    }

    t = (-1.0f - pStart->z) / pDiff->z;
    if (t > 0.0f && t <= 1.0f + NW4R_MATH_FLT_EPSILON) {
        math::VEC3Lerp(&hit, pStart, pEnd, t);
        if (math::FAbs(hit.x) <= 1.0f + NW4R_MATH_FLT_EPSILON &&
            math::FAbs(hit.y) <= 1.0f + NW4R_MATH_FLT_EPSILON) {
            zMinus = t;
            if (t < result) {
                result = t;
            }
        }
    }

    if (2.0f == result) {
        return 2.0f;
    }

    pHit->x = pStart->x + result * (pEnd->x - pStart->x);
    pHit->y = pStart->y + result * (pEnd->y - pStart->y);
    pHit->z = pStart->z + result * (pEnd->z - pStart->z);
    math::VEC3Transform(pHitWorld, pMtx, pHit);

    if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) {
        if (math::FAbs(xPlus - result) <= NW4R_MATH_FLT_EPSILON ||
            math::FAbs(xMinus - result) <= NW4R_MATH_FLT_EPSILON) {
            pVel->x *= -1.0f;
            pDiff->x *= -1.0f;
        }

        if (math::FAbs(yPlus - result) <= NW4R_MATH_FLT_EPSILON ||
            math::FAbs(yMinus - result) <= NW4R_MATH_FLT_EPSILON) {
            pVel->y *= -1.0f;
            pDiff->y *= -1.0f;
        }

        if (math::FAbs(zPlus - result) <= NW4R_MATH_FLT_EPSILON ||
            math::FAbs(zMinus - result) <= NW4R_MATH_FLT_EPSILON) {
            pVel->z *= -1.0f;
            pDiff->z *= -1.0f;
        }

        // Exactly onto the face
        if (math::FAbs(xPlus - result) <= NW4R_MATH_FLT_EPSILON) {
            pHit->x = 1.0f;
        }
        if (math::FAbs(xMinus - result) <= NW4R_MATH_FLT_EPSILON) {
            pHit->x = -1.0f;
        }
        if (math::FAbs(yPlus - result) <= NW4R_MATH_FLT_EPSILON) {
            pHit->y = 1.0f;
        }
        if (math::FAbs(yMinus - result) <= NW4R_MATH_FLT_EPSILON) {
            pHit->y = -1.0f;
        }
        if (math::FAbs(zPlus - result) <= NW4R_MATH_FLT_EPSILON) {
            pHit->z = 1.0f;
        }
        if (math::FAbs(zMinus - result) <= NW4R_MATH_FLT_EPSILON) {
            pHit->z = -1.0f;
        }
    }

    return result;
}

// What a post field does to a particle that it catches
static inline void ExecuteCollisionOption(AnimCurvePostFieldInfo* pInfo,
                                          Particle* pParticle,
                                          EmitterResource* pChild,
                                          bool* pDelete) {
    if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_DELETE) {
        *pDelete = true;
    }

    if ((pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_CREATE_CHILD_PTCL) &&
        pChild != NULL) {
        pParticle->mParticleManager->mManagerEM->mManagerEF->mManagerES
            ->mCreationQueue.AddParticleCreation(&pInfo->childInheritSetting,
                                                 pParticle, pChild,
                                                 pParticle->mCalcRemain, NULL,
                                                 NULL);
    }

    if ((pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_CREATE_CHILD_EMIT) &&
        pChild != NULL) {
        pParticle->mParticleManager->mManagerEM->mManagerEF->mManagerES
            ->mCreationQueue.AddEmitterCreation(&pInfo->childInheritSetting,
                                                pParticle, pChild,
                                                pParticle->mCalcRemain, NULL,
                                                NULL);
    }
}

static inline int CheckCollision(const math::VEC3* pPos, u8 shape, u8 option) {
    switch (shape) {
    case AnimCurvePostFieldInfo::SHAPE_PLANE: {
        return CheckCollisionPlane(pPos, option);
    }
    case AnimCurvePostFieldInfo::SHAPE_RECTANGLE: {
        return CheckCollisionRectangle(pPos, option);
    }
    case AnimCurvePostFieldInfo::SHAPE_CIRCLE: {
        return CheckCollisionCircle(pPos, option);
    }
    case AnimCurvePostFieldInfo::SHAPE_CUBE: {
        return CheckCollisionCube(pPos, option);
    }
    case AnimCurvePostFieldInfo::SHAPE_SPHERE: {
        return CheckCollisionSphere(pPos, option);
    }
    case AnimCurvePostFieldInfo::SHAPE_CYLINDER: {
        return CheckCollisionCylinder(pPos, option);
    }
    default: {
        return AnimCurvePostFieldInfo::STATUS_ON;
    }
    }
}

// One piece of the way of a particle, from pStart to pEnd in the space of the shape.
// On a hit: the new position and velocity in the space of the particles.
// NONMATCHING: only the calls of the six CheckCollision functions: retail calls them, this
// compiler expands them here with -inline auto (identical with -inline noauto, which the
// rest of the library does not tolerate, so it is no unit flag).
bool CollideShape(const math::MTX34* pMtx, const math::MTX34* pRotMtx,
                  const AnimCurvePostFieldInfo* pInfo, EmitterResource* pChild,
                  Particle* pParticle, const math::VEC3* pStart,
                  const math::VEC3* pEnd, math::VEC3 vel, bool* pDelete,
                  math::VEC3* pHit, math::VEC3* pHitWorld,
                  math::VEC3* pNewPos, math::VEC3* pNewVel) {
    CheckCollision(pEnd, pInfo->shapeType, pInfo->shapeOption);

    math::VEC3 diff;
    math::VEC3Sub(&diff, pEnd, pStart);

    f32 t = 2.0f;

    switch (pInfo->shapeType) {
    case AnimCurvePostFieldInfo::SHAPE_PLANE: {
        t = CollidePlane(pMtx, pInfo, pEnd, pStart, &diff, &vel, pHit,
                         pHitWorld);
        break;
    }
    case AnimCurvePostFieldInfo::SHAPE_RECTANGLE: {
        t = CollideRectangle(pMtx, pInfo, pEnd, pStart, &diff, &vel, pHit,
                             pHitWorld);
        break;
    }
    case AnimCurvePostFieldInfo::SHAPE_CIRCLE: {
        t = CollideCircle(pMtx, pInfo, pEnd, pStart, &diff, &vel, pHit,
                          pHitWorld);
        break;
    }
    case AnimCurvePostFieldInfo::SHAPE_CUBE: {
        t = CollideCube(pMtx, pInfo, pEnd, pStart, &diff, &vel, pHit,
                        pHitWorld);
        break;
    }
    case AnimCurvePostFieldInfo::SHAPE_SPHERE: {
        t = CollideSphere(pMtx, pInfo, pEnd, pStart, &diff, &vel, pHit,
                          pHitWorld);
        break;
    }
    case AnimCurvePostFieldInfo::SHAPE_CYLINDER: {
        t = CollideCylinder(pMtx, pInfo, pEnd, pStart, &diff, &vel, pHit,
                            pHitWorld);
        break;
    }
    }

    if (t > 1.5f) {
        return false;
    }

    if ((pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_BOUNCE) ||
        (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_CONTROL_SPEED)) {
        math::VEC3Transform(pNewVel, pRotMtx, &vel);
        pNewVel->x *= pInfo->speedFactor.x;
        pNewVel->y *= pInfo->speedFactor.y;
        pNewVel->z *= pInfo->speedFactor.z;

        // The rest of the way goes on from the hit
        math::VEC3 rest;
        math::VEC3Scale(&rest, &diff, 1.0f - t);
        math::VEC3Transform(&rest, pRotMtx, &rest);
        rest.x *= pInfo->speedFactor.x;
        rest.y *= pInfo->speedFactor.y;
        rest.z *= pInfo->speedFactor.z;
        math::VEC3Add(pNewPos, pHitWorld, &rest);
    }

    if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_DELETE) {
        *pDelete = true;
        pNewPos->x = pHitWorld->x;
        pNewPos->y = pHitWorld->y;
        pNewPos->z = pHitWorld->z;
    }

    if ((pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_CREATE_CHILD_PTCL) &&
        pChild != NULL) {
        pParticle->mParticleManager->mManagerEM->mManagerEF->mManagerES
            ->mCreationQueue.AddParticleCreation(&pInfo->childInheritSetting,
                                                 pParticle, pChild,
                                                 pParticle->mCalcRemain,
                                                 pHitWorld, NULL);
    }

    if ((pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_CREATE_CHILD_EMIT) &&
        pChild != NULL) {
        pParticle->mParticleManager->mManagerEM->mManagerEF->mManagerES
            ->mCreationQueue.AddEmitterCreation(&pInfo->childInheritSetting,
                                                pParticle, pChild,
                                                pParticle->mCalcRemain,
                                                pHitWorld, NULL);
    }

    return true;
}

// Copy of a matrix without its translation
static inline void GetRotationScaleMtx(math::MTX34* pOut,
                                       const math::MTX34* pIn) {
    *pOut = *pIn;
    pOut->_03 = 0.0f;
    pOut->_13 = 0.0f;
    pOut->_23 = 0.0f;
}

// addPos is passed by value: the compiler knows that nothing else points to it, so its
// loads move ahead of the stores to `next` (retail 0x80241C40) and the scaling by
// speedFactor may write it. The collision functions take a const AnimCurvePostFieldInfo*
// for the same reason (loads through a pointer to const are moved ahead of stores); this
// function reloads the info after every store, so its pointer is not const.
// NONMATCHING: only the calls of the six CheckCollision functions: retail calls them, this
// compiler expands them here with -inline auto (identical with -inline noauto, which the
// rest of the library does not tolerate, so it is no unit flag).
BOOL AnimCurveExecutePostField(Particle* pParticle,
                               AnimCurvePostFieldParam* pParam,
                               AnimCurvePostFieldInfo* pInfo,
                               EmitterResource* pChild,
                               const math::MTX34* pEmToGlb,
                               const math::MTX34* pGlbToLoc,
                               const math::VEC3* pPrevPos, math::VEC3 addPos,
                               math::VEC3* pVel, bool* pRetire) {
    // Shape space to particle space
    math::MTX34 mtx;
    math::MTX34Identity(&mtx);
    MtxScale(&mtx, static_cast<const math::VEC3*>(&pParam->scale), &mtx);

    math::MTX34 rot;
    MtxRotXYZRad(&rot, pParam->rotate.x, pParam->rotate.y, pParam->rotate.z);
    math::MTX34Mult(&mtx, &rot, &mtx);
    math::MTX34Trans(&mtx, static_cast<const math::VEC3*>(&pParam->translate),
                     &mtx);

    if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_EMITTER_ORIENTED) {
        math::VEC3 emitterPos(pEmToGlb->_03, pEmToGlb->_13,
                              pEmToGlb->_23);
        math::MTX34Trans(&mtx, &emitterPos, &mtx);
    }

    math::MTX34Mult(&mtx, pGlbToLoc, &mtx);

    math::MTX34 rotMtx;
    GetRotationScaleMtx(&rotMtx, &mtx);

    math::MTX34 invMtx;
    math::MTX34Inv(&invMtx, &mtx);

    math::MTX34 invRotMtx;
    GetRotationScaleMtx(&invRotMtx, &invMtx);

    math::VEC3 pos;
    math::VEC3Transform(&pos, &invMtx, pPrevPos);

    u8 status =
        CheckCollision(&pos, pInfo->shapeType, pInfo->shapeOption);
    u8 prevStatus = pParticle->mParameter.mCollisionStatus;
    pParticle->mParameter.mCollisionStatus = status;

    f32 speedSq = math::VEC3LenSq(pVel);

    switch (pInfo->controlSpeedType) {
    case AnimCurvePostFieldInfo::CONTROLSPEED_LIMIT: {
        if (speedSq > pInfo->referenceSpeed * pInfo->referenceSpeed) {
            VecNormalize(pVel, pVel);
            math::VEC3Scale(pVel, pVel, pInfo->referenceSpeed);
        }
        break;
    }
    case AnimCurvePostFieldInfo::CONTROLSPEED_TERMINATE: {
        if (speedSq > pInfo->referenceSpeed * pInfo->referenceSpeed) {
            *pRetire = true;
        }
        break;
    }
    }

    if (*pRetire) {
        return true;
    }

    if (pParticle->mTick < pInfo->startFrame) {
        return true;
    }

    if (pInfo->collisionOption == 0) {
        return true;
    }

    if (pInfo->collisionType == AnimCurvePostFieldInfo::COLLISION_INNER ||
        pInfo->collisionType == AnimCurvePostFieldInfo::COLLISION_OUTER) {

        if (pInfo->collisionType == AnimCurvePostFieldInfo::COLLISION_INNER &&
            status != AnimCurvePostFieldInfo::STATUS_INSIDE) {
            return true;
        }

        if (pInfo->collisionType == AnimCurvePostFieldInfo::COLLISION_OUTER &&
            status != AnimCurvePostFieldInfo::STATUS_OUTSIDE) {
            return true;
        }

        if (pInfo->collisionOption & AnimCurvePostFieldInfo::OPTION_CONTROL_SPEED) {
            addPos.x *= pInfo->speedFactor.x;
            addPos.y *= pInfo->speedFactor.y;
            addPos.z *= pInfo->speedFactor.z;
            pVel->x *= pInfo->speedFactor.x;
            pVel->y *= pInfo->speedFactor.y;
            pVel->z *= pInfo->speedFactor.z;

            switch (pInfo->controlSpeedType) {
            case AnimCurvePostFieldInfo::CONTROLSPEED_LIMIT: {
                if (math::VEC3LenSq(pVel) >
                    pInfo->referenceSpeed * pInfo->referenceSpeed) {
                    VecNormalize(pVel, pVel);
                    math::VEC3Scale(pVel, pVel, pInfo->referenceSpeed);
                }
                break;
            }
            case AnimCurvePostFieldInfo::CONTROLSPEED_TERMINATE: {
                if (math::VEC3LenSq(pVel) >
                    pInfo->referenceSpeed * pInfo->referenceSpeed) {
                    *pRetire = true;
                }
                break;
            }
            }
        }

        ExecuteCollisionOption(pInfo, pParticle, pChild, pRetire);
        return true;
    }

    // The way of this frame, in the space of the shape
    math::VEC3 next;
    next.x = pPrevPos->x + pParticle->mParameter.mMomentum * (addPos.x + pVel->x);
    next.y = pPrevPos->y + pParticle->mParameter.mMomentum * (addPos.y + pVel->y);
    next.z = pPrevPos->z + pParticle->mParameter.mMomentum * (addPos.z + pVel->z);
    math::VEC3Transform(&next, &invMtx, &next);

    math::VEC3 vel;
    math::VEC3Transform(&vel, &invRotMtx, pVel);

    math::VEC3 hit;
    math::VEC3 hitWorld;
    math::VEC3 newPos;
    math::VEC3 newVel;

    // A particle that bounces may hit the shape again on the same frame
    for (int i = 0; i < 5; i++) {
        bool collided;

        if (math::VEC3DistSq(&pos, &next) <
            NW4R_MATH_FLT_EPSILON * NW4R_MATH_FLT_EPSILON) {
            collided = false;
        } else {
            collided =
                CollideShape(&mtx, &rotMtx, pInfo, pChild, pParticle, &pos,
                             &next, vel, pRetire, &hit, &hitWorld, &newPos,
                             &newVel);
        }

        if (collided) {
            pParticle->mParameter.mPosition.x = newPos.x;
            pParticle->mParameter.mPosition.y = newPos.y;
            pParticle->mParameter.mPosition.z = newPos.z;
            pParticle->mParameter.mVelocity.x = newVel.x;
            pParticle->mParameter.mVelocity.y = newVel.y;
            pParticle->mParameter.mVelocity.z = newVel.z;
            pParticle->mParameter.mCollisionStatus |=
                AnimCurvePostFieldInfo::STATUS_COLLIDED;

            if (*pRetire) {
                return false;
            }

            pos.x = hit.x;
            pos.y = hit.y;
            pos.z = hit.z;
            status = AnimCurvePostFieldInfo::STATUS_ON;
            math::VEC3Transform(&next, &invMtx, &newPos);
            math::VEC3Transform(&vel, &invRotMtx, &newVel);
        } else {
            if (i == 0) {
                // Nothing hit, but the particle changed sides
                if (pParticle->mTick > pInfo->startFrame &&
                    ((prevStatus == AnimCurvePostFieldInfo::STATUS_INSIDE &&
                      (status == AnimCurvePostFieldInfo::STATUS_OUTSIDE ||
                       status == AnimCurvePostFieldInfo::STATUS_ON)) ||
                     (prevStatus == AnimCurvePostFieldInfo::STATUS_OUTSIDE &&
                      (status == AnimCurvePostFieldInfo::STATUS_ON ||
                       status == AnimCurvePostFieldInfo::STATUS_INSIDE)))) {
                    ExecuteCollisionOption(pInfo, pParticle, pChild, pRetire);
                }
                return true;
            }

            switch (pInfo->controlSpeedType) {
            case AnimCurvePostFieldInfo::CONTROLSPEED_LIMIT: {
                if (math::VEC3LenSq(&pParticle->mParameter.mVelocity) >
                    pInfo->referenceSpeed * pInfo->referenceSpeed) {
                    VecNormalize(&pParticle->mParameter.mVelocity,
                                 &pParticle->mParameter.mVelocity);
                    math::VEC3Scale(&pParticle->mParameter.mVelocity,
                                    &pParticle->mParameter.mVelocity,
                                    pInfo->referenceSpeed);
                }
                break;
            }
            case AnimCurvePostFieldInfo::CONTROLSPEED_TERMINATE: {
                if (math::VEC3LenSq(&pParticle->mParameter.mVelocity) >
                    pInfo->referenceSpeed * pInfo->referenceSpeed) {
                    *pRetire = true;
                }
                break;
            }
            }
            return false;
        }
    }

    *pRetire = true;
    return false;
}

} // namespace ef
} // namespace nw4r
