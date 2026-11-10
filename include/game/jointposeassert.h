#ifndef GAME_JOINTPOSEASSERT_H
#define GAME_JOINTPOSEASSERT_H

extern "C" {
extern const char sJointPoseAssertFile[8];
extern const char sJointPoseAssertJoint[8];
extern const char sJointPoseAssertRotation[8];
}

#define JOBJ_H_ASSERT_JOINT(line, joint)     ((joint) ? (void)0 : __assert(sJointPoseAssertFile, line, sJointPoseAssertJoint))
#define JOBJ_H_ASSERT_ROTATION(line, rotate) ((rotate) ? (void)0 : __assert(sJointPoseAssertFile, line, sJointPoseAssertRotation))
#include "game/hsd_jobj.h"

#endif
