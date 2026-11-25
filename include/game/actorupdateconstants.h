#pragma once

#include <dolphin/mtx/mtxtypes.h>

typedef struct XObjActorLiterals {
	Vec mLeadingVectors[2][3];
	Vec mReset[4];
	Vec mInit[4];
} XObjActorLiterals;

#ifdef __cplusplus
extern "C" {
#endif
extern XObjActorLiterals gActorUpdateConstants;
#ifdef __cplusplus
}
#endif
