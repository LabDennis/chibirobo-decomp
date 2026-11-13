#include "game/cgame.h"
#include "game/xchr.h"
#include "types.h"
#include "Version.h"
#include "game/ut.h"

extern "C" {
#include <sysdolphin/random.h>
}

extern "C" void OSReport(const char* msg, ...);

extern "C" s32 CmdSetSoundPitch(s32 cmd, s32 count, s32* args);
extern "C" s32 CmdStartRandomAnims(s32 cmd, s32 count, s32* args);
extern "C" void UpdateStage(void* arg);
extern "C" void Init(void* arg);

typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 count, s32* args);

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
		mObject = obj;
		if (mObject == NULL) {
			_08 = 0;
		}
		_08    = func;
		mState = 0;
	}

	void Start(XObj* obj) { Set(obj, &RandomAnimCtrl::UpdateRandomAnim); }

	XObj* mObject; // _00, the XObj whose anims are picked at random
	s32 mTimer;    // _04, frames left on the current anim, 180-359 per pick, counts down each update
	Func _08;      // _08, member function pointer Start stores (&RandomAnimCtrl::UpdateRandomAnim); nothing in hori calls it
	s32 mState;    // _14, 0 picks one of three anims at random, 10/20/30 start anim 0/3/4, 11/21/31 wait for mTimer
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

	if (mTimer >= 0) {
		mTimer--;
	}
	switch (mState) {
	case 0:
		mTimer = HSD_Randi(180) + 180;
		if (rnd < 0.333f) {
			mState = 10;
		} else if (rnd < 0.666f) {
			mState = 20;
		} else {
			mState = 30;
		}
		break;
	case 10:
		mObject->SetAnim(0, 0, 0, 0, 0, 0.0f, 1.0f, 0.0f, 10.0f);
		mState++;
		break;
	case 11:
		if (mTimer <= 0) {
			mState = 0;
		}
		break;
	case 20:
		mObject->SetAnim(3, 0, 0, 0, 0, 0.0f, 1.0f, 0.0f, 10.0f);
		mState++;
		break;
	case 21:
		if (mTimer <= 0) {
			mState = 0;
		}
		break;
	case 30:
		mObject->SetAnim(4, 0, 0, 0, 0, 0.0f, 1.0f, 0.0f, 10.0f);
		mState++;
		break;
	case 31:
		if (mTimer <= 0) {
			mState = 0;
		}
		break;
	}
	return 0;
}

/**
 * @note Address: hori .text 0x21C
 * @note Size: 0x50
 */
extern "C" s32 CmdSetSoundPitch(s32 cmd, s32 count, s32* args)
{
	s16 pitch = args[0];
	s32 time  = args[1] * 60 / 100;
	XSound* sound;

	if (time <= 0) {
		time = 1;
	}
	sound                    = &gGame.mSound;
	sound->mTargetMusicPitch = pitch;
	sound->mMusicPitchFrames = time;
	return 0;
}

/**
 * @note Address: hori .text 0x26C
 * @note Size: 0xBC
 */
extern "C" s32 CmdStartRandomAnims(s32 cmd, s32 count, s32* args)
{
	s32* ids = args;
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
