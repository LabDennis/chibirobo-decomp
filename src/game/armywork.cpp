#include "game/xobj.h"

extern "C" {
Vec gArmyPatrolMin;
Vec gArmyPatrolMax;
XObj* gArmyShotModel;
s32 gArmyNextShotSound;
s32 gArmyKnockdownCount;
XObj* gArmyShotTarget;
CStatus gArmyShotStatus; // bit 0 records a hit, bit 1 prevents firing
}
