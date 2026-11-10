// Constants shared by the original geometry and joint-pose routines.
const float sGeometryZero                = 0.0f;
const float sWallDistanceTolerance       = 0.1f;
const float sWallCornerMovementThreshold = 0.3f;
const float sWallCornerRadiusScale       = 1.001f;
const float sWallCornerClearance         = 0.01f;
const float sGeometryNegativeOne         = -1.0f;
const float sLinePlaneEndpointTolerance  = 0.00001f;
const float sGeometryOne                 = 1.0f;

#if defined(VERSION_GGTP01)
#define PAD_BYTE "\0"
#else
#define PAD_BYTE "0"
#endif
const char sJointPoseAssertFile[8]     = "jobj.h\0" PAD_BYTE;
const char sJointPoseAssertJoint[8]    = "jobj\0" PAD_BYTE PAD_BYTE PAD_BYTE;
const char sJointPoseAssertRotation[8] = "rotate\0" PAD_BYTE;

const double sGeometrySqrtHalf  = 0.5;
const double sGeometrySqrtThree = 3.0;
const double sGeometrySqrtZero  = 0.0;
