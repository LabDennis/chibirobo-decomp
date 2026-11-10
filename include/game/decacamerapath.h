#pragma once

typedef struct DecaJointPath2 {
	long indices[2];
} DecaJointPath2;

#ifdef __cplusplus
extern "C" {
#endif
extern const DecaJointPath2 sDecaCameraJointPath;
extern const DecaJointPath2 sDecaHoldJointPath;
#ifdef __cplusplus
}
#endif
