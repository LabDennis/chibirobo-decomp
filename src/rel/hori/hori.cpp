#include "game/cgame.h"
#include "game/xchr.h"
#include "types.h"
#include "Version.h"
#include "game/ut.h"

extern "C" {
#include <sysdolphin/random.h>
}

extern "C" void OSReport(const char* msg, ...);

extern "C" s32 CmdSetSoundPitch(s32 cmd, void* arg, void* arg2);
extern "C" s32 CmdStartRandomAnims(s32 cmd, s32 count, void* arg2);
extern "C" void UpdateStage(void* arg);
extern "C" void Init(void* arg);

typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, void* arg, void* arg2);

extern "C" {
RelSlotFunc _prolog_slots[7] = { Init, UpdateStage };
RelCmdFunc _prolog_cmds[]    = { CmdSetSoundPitch, (RelCmdFunc)CmdStartRandomAnims };
}

class RandomAnimCtrl {
public:
	typedef s32 (RandomAnimCtrl::*Func)();

	s32 UpdateRandomAnim();

	void Set(XObj* obj, const Func& func)
	{
		_00 = obj;
		if (_00 == NULL) {
			_08 = 0;
		}
		_08 = func;
		_14 = 0;
	}

	void Start(XObj* obj) { Set(obj, &RandomAnimCtrl::UpdateRandomAnim); }

	XObj* _00; // _00
	s32 _04;   // _04
	Func _08;  // _08
	s32 _14;   // _14
};

static RandomAnimCtrl randomAnims[10];
static s32 randomAnimCount;
static s32 prevPlayerObjNo;
static s32 tuta_objno;
static CVec lbl_2_bss_108;

/**
 * @note Address: hori .text 0x0
 * @note Size: 0x21C
 */
s32 RandomAnimCtrl::UpdateRandomAnim()
{
	f32 rnd = HSD_Randf();

	if (_04 >= 0) {
		_04--;
	}
	switch (_14) {
	case 0:
		_04 = HSD_Randi(180) + 180;
		if (rnd < 0.333f) {
			_14 = 10;
		} else if (rnd < 0.666f) {
			_14 = 20;
		} else {
			_14 = 30;
		}
		break;
	case 10:
		_00->SetAnim(0, 0, 0, 0, 0, 0.0f, 1.0f, 0.0f, 10.0f);
		_14++;
		break;
	case 11:
		if (_04 <= 0) {
			_14 = 0;
		}
		break;
	case 20:
		_00->SetAnim(3, 0, 0, 0, 0, 0.0f, 1.0f, 0.0f, 10.0f);
		_14++;
		break;
	case 21:
		if (_04 <= 0) {
			_14 = 0;
		}
		break;
	case 30:
		_00->SetAnim(4, 0, 0, 0, 0, 0.0f, 1.0f, 0.0f, 10.0f);
		_14++;
		break;
	case 31:
		if (_04 <= 0) {
			_14 = 0;
		}
		break;
	}
	return 0;
}

/**
 * @note Address: hori .text 0x21C
 * @note Size: 0x50
 */
extern "C" s32 CmdSetSoundPitch(s32 cmd, void* arg, void* arg2)
{
	s32* args = (s32*)arg2;
	s16 pitch = args[0];
	s32 time  = args[1] * 60 / 100;
	XSound* sound;

	if (time <= 0) {
		time = 1;
	}
	sound                    = &gGame.mSound;
	sound->mTargetMusicPitch = pitch;
	sound->_188              = time;
	return 0;
}

/**
 * @note Address: hori .text 0x26C
 * @note Size: 0xBC
 */
extern "C" s32 CmdStartRandomAnims(s32 cmd, s32 count, void* arg2)
{
	s32* ids = (s32*)arg2;
	s32 i;

	for (i = 0; i < count; i++) {
		randomAnims[randomAnimCount].Start(gGame.mXObjs.mObjects[*ids]);
		ids++;
		randomAnimCount++;
	}
	return randomAnimCount;
}

/**
 * @note Address: hori .text 0x328
 * @note Size: 0xB0
 */
extern "C" void UpdateStage(void* arg)
{
	s32 cur = gGame.GetPlayer()->mFloorContacts[0].objectIndex;

	if (prevPlayerObjNo != cur && cur == tuta_objno) {
		gGame.mXObjs.mObjects[tuta_objno]->SetAnim(1, 1, 0, 0, 0, 0.0f, 1.0f, 0.0f, 10.0f);
	}
	prevPlayerObjNo = cur;
}

/**
 * @note Address: hori .text 0x3D8
 * @note Size: 0xB8
 */
extern "C" void Init(void* arg)
{
	s32 i;

	OSReport("Init\n");
	prevPlayerObjNo = -1;
	for (i = 0; i < gGame.mXObjs.mObjectCount; i++) {
		if (gGame.mXObjs.mObjects[i]->GetKind() == 0x2EC) {
			tuta_objno = i;
			OSReport("tuta_objno = %d\n", i);
		}
	}
}

/**
 * @note Address: hori .text 0x490
 * @note Size: 0x44
 */

/**
 * @note Address: hori .text 0x4D4
 * @note Size: 0x48
 */
