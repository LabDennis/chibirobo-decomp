#include "PowerPC_EABI_Support/Runtime/ptmf.h"

struct HamObj;
extern void StateFlip__6HamObjFv(struct HamObj* self);
extern void StateSlide__6HamObjFv(struct HamObj* self);
extern void StatePutDown__6HamObjFv(struct HamObj* self);
extern void StateStackLand__6HamObjFv(struct HamObj* self);

PTMF sHamFlipState      = { 0, -1, { (void*)StateFlip__6HamObjFv } };
PTMF sHamFlipComparison = { 0, -1, { (void*)StateFlip__6HamObjFv } };
PTMF sHamSlideState     = { 0, -1, { (void*)StateSlide__6HamObjFv } };
PTMF sHamPutDownState   = { 0, -1, { (void*)StatePutDown__6HamObjFv } };
PTMF sHamStackLandState = { 0, -1, { (void*)StateStackLand__6HamObjFv } };
