#ifndef NW4R_EF_POST_FIELD_H
#define NW4R_EF_POST_FIELD_H
#include "nw4r/types_nw4r.h"

#include "nw4r/ef/ef_emitter.h"
#include "nw4r/math.h"

namespace nw4r {
namespace ef {

// Forward declarations
class Particle;
struct EmitterResource;

// AC_TYPE_POSTFIELD: collision of the particles with a shape. The post field curve
// animates the head of the info block (kindType is the byte offset of the animated
// value in it); the rest of the block is constant.
struct AnimCurvePostFieldParam {
    math::_VEC3 scale;     // at 0x0
    math::_VEC3 rotate;    // at 0xC
    math::_VEC3 translate; // at 0x18
};

struct AnimCurvePostFieldInfo {
    enum ControlSpeedType {
        CONTROLSPEED_NONE,
        CONTROLSPEED_LIMIT,     // the speed is cut down to referenceSpeed
        CONTROLSPEED_TERMINATE, // a faster particle is deleted
    };

    enum CollisionShapeType {
        SHAPE_PLANE,
        SHAPE_RECTANGLE,
        SHAPE_CIRCLE,
        SHAPE_CUBE,
        SHAPE_SPHERE,
        SHAPE_CYLINDER,
    };

    // Plane, rectangle and circle: the plane they lie in
    enum PlaneType {
        PLANE_XZ,
        PLANE_XY,
        PLANE_YZ,
    };

    // Sphere: which half exists
    enum SphereType {
        SPHERE_WHOLE,
        SPHERE_TOP,
        SPHERE_BOTTOM,
    };

    enum CollisionType {
        COLLISION_BORDER, // the surface stops the particle
        COLLISION_INNER,  // acts on every particle inside
        COLLISION_OUTER,  // acts on every particle outside
    };

    enum CollisionOption {
        OPTION_EMITTER_ORIENTED = (1 << 0),
        OPTION_BOUNCE = (1 << 1),
        OPTION_CONTROL_SPEED = (1 << 3),
        OPTION_CREATE_CHILD_PTCL = (1 << 4),
        OPTION_CREATE_CHILD_EMIT = (1 << 5),
        OPTION_DELETE = (1 << 6),
    };

    // Where the particle is, in Particle::mParameter.mCollisionStatus
    enum CollisionStatus {
        STATUS_ON = 1,
        STATUS_INSIDE = 2,  // plane: above
        STATUS_OUTSIDE = 3, // plane: below
        STATUS_BESIDE = 4,  // in the plane of a rectangle or circle, but not on it
        STATUS_COLLIDED = (1 << 7),
    };

    enum WrapFlag {
        WRAP_ENABLE = (1 << 0),
        WRAP_CENTER_EMITTER = (1 << 1)
    };

    AnimCurvePostFieldParam param;             // at 0x0
    f32 referenceSpeed;                        // at 0x24
    u8 controlSpeedType;                       // at 0x28
    u8 shapeType;                              // at 0x29
    u8 shapeOption;                            // at 0x2A, PlaneType or SphereType
    u8 collisionType;                          // at 0x2B
    u16 collisionOption;                       // at 0x2C
    u16 startFrame;                            // at 0x2E
    math::_VEC3 speedFactor;                   // at 0x30
    EmitterInheritSetting childInheritSetting; // at 0x3C
    u16 childName;                             // at 0x46
    u8 wrap;                                   // at 0x48
    math::_VEC3 wrapScale;                     // at 0x4C
    math::_VEC3 wrapRotate;                    // at 0x58
    math::_VEC3 wrapTranslate;                 // at 0x64
};

} // namespace ef
} // namespace nw4r

#endif
