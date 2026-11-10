#ifndef _GAME_XACTION_H
#define _GAME_XACTION_H

#include "types.h"

// chibi robo's actions, the classes are in game/xact.h. this one stays light, units without
// the hsd headers include it

class XAction;
class XActLift;
class XActPush;

extern "C" {
void ReleasePushAction(XActPush* action);
void ApplyPlayerItemTintToTarget(XAction* action);
BOOL IsCarryingSquirter(XActLift* action);
BOOL IsCarryingMug(XActLift* action);
BOOL IsCarryingSpoon(XActLift* action);
BOOL IsCarryingToothbrush(XActLift* action);
BOOL IsLiftTargetPlug(XActLift* action);
void ResetPlayerLiftAction(XActLift* action);
}

#endif
