#ifndef _GAME_XOBJCHR_H
#define _GAME_XOBJCHR_H

#include "types.h"

// chibi robo's own object, the units from 0x80093000 to 0x800BC000. the class is in
// game/xchr.h, this one stays light

struct Vec;
class CJObj;
class CPad;
class CVec;
class CXObj;
class XObj;
class XObjChr;
class OXMenu;
struct AttachmentValues;

extern "C" {
// the yes/no and number choice the msg and select commands open
void HandleOXMenuInput(OXMenu* menu, CPad* pad);
s32 GetAttachmentCountById(AttachmentValues* values, s32 id);
void UpdateAttachmentAvailability(AttachmentValues* values);
void ResetPlayerPlug(XObjChr* chr);
BOOL IsPlayerBlockingEvents(const XObjChr* player);
CVec CJObj_GetJointLocalTranslationByPath(const CJObj* object, s32 depth, s32* path);
CVec GetPlayerCarryJointLocalTranslation(CJObj* object);
CVec GetPlayerCarryJointWorldPosition(CJObj* object);
void SetPlayerEyeCameraJointsHidden(XObj* player, BOOL hidden);
void OpenPlayerYesNoMenu(XObjChr* chr, s32 first, s32 second);
void SelectPlayerEquipmentRing(XObjChr* chr, s32 selection);
void OpenPlayerEquipmentRings(XObjChr* chr);
void ResetPlayerHeadOffset(XObjChr* chr);
BOOL LoadPlayerAttachmentAnimation(XObjChr* chr);

// Initial values copied before decoding floor-record coordinates in hundredths.
extern Vec gPlayerFloorCenterDefault;
extern Vec gPlayerFloorExtentDefault;
}

#endif
