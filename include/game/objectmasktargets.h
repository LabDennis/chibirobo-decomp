#pragma once

#ifdef __cplusplus
class XObj;
extern "C" {
#else
typedef struct XObj XObj;
#endif

typedef struct ObjectMaskTargets {
	XObj* plug;
	XObj* interaction;
} ObjectMaskTargets;

extern const ObjectMaskTargets sEmptyObjectMaskTargets;

#ifdef __cplusplus
}
#endif
