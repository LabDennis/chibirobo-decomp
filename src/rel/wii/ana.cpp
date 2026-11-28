#if defined(VERSION_R24J01)
#include "game/wii/cgame.h"
#include "game/wii/xobjflags.h"

extern "C" CVec fn_800E5A2C(XObj* player);

class Ana {
public:
	Ana() { }
	~Ana() { }
	virtual bool IsActive() { return mStatus.mFlags & 1; }

	void Set(f32 x, f32 y, f32 z)
	{
		mStatus.mFlags |= 1;
		mPos.x = x;
		mPos.y = y;
		mPos.z = z;
	}

	void SetJObj(s32 kind, s32 which)
	{
		mJObjKind  = kind;
		mJObjWhich = which;
	}

	Vec mPos;
	f32 mRotY;
	CStatus32 mStatus;
	s32 mId;
	s32 mJObjKind;
	s32 mJObjWhich;
};

class AnaNull : public Ana {
public:
	virtual bool IsActive() { return false; }
};

static Ana anas[20];
static AnaNull anaNull;
static XObj* flowerObjs[3];
static XObj anaXObjs[20][5];

s32 activeAnaNum;
static XObj* tutiAnaObj;
static XObj* tutiHutaObj;
static u8 anaXObjsInitialized;

inline Ana* GetAna(s32 id)
{
	if (id < 0 || id >= 20) {
		return &anaNull;
	}
	return &anas[id];
}

inline void ReleaseHeld(WiiPlayerState* player)
{
	if (player->mLift.mTarget != NULL) {
		player->mLift.mTarget->mStatus.mFlags &= ~0x0400000000000000ULL;
		player->mLift.mTarget = NULL;
	}
	player->mLift.mState = 0;
}

inline void DropObj(XObj* obj)
{
	if (obj->GetCategory() == XOBJS::ITEMS) {
		static_cast<XObjItem*>(obj)->SetFlag();
	}
	obj->mReleaseCountdown = 2;
}

Ana* GetNearestAna(f32 x, f32 y, f32 z);

extern "C" s32 AddAna(s32 id, f32 x, f32 y, f32 z)
{
	CVec pos(x, y, z);

	if (id >= 0 && id < 20) {
		Ana* ana = &anas[id];
		if (!(ana->mStatus.mFlags & 1)) {
			activeAnaNum++;
		}
		ana->Set(x, y, z);
		ana->mId = id;
		return id;
	}

	for (u32 i = 0; i < 20; i++) {
		if ((anas[i].mStatus.mFlags & 1) && PSVECDistance(&pos, &anas[i].mPos) < 5.0f) {
			return -1;
		}
	}
	for (u32 i = 0; i < 20; i++) {
		if (!(anas[i].mStatus.mFlags & 1)) {
			activeAnaNum++;
			anas[i].Set(x, y, z);
			anas[i].mId = i;
			return i;
		}
	}
	return -1;
}

s32 CmdAddAna(s32 cmd, s32 count, s32* args)
{
	if (count >= 4) {
		return AddAna(args[3], args[0] / 100.0f, args[1] / 100.0f, args[2] / 100.0f);
	}
	return AddAna(-1, args[0] / 100.0f, args[1] / 100.0f, args[2] / 100.0f);
}

// NONMATCHING: retail calls GetNearestAna out of line twice; this build inlines both calls. The helper itself and the three GetPos calls
// match.
s32 CmdFindNearestAna(s32 cmd, s32 count, s32* args)
{
	Ana* best = NULL;

	if (count == 3) {
		f32 x = args[0] / 100.0f;
		f32 y = args[1] / 100.0f;
		f32 z = args[2] / 100.0f;
		best  = GetNearestAna(x, y, z);
	} else {
		XObj* contact = gGame.mXObjs.mObjects[gGame.GetPlayer()->mFloorContactObjects[2]];
		if (contact != NULL) {
			best = GetNearestAna(contact->GetPos()->x, contact->GetPos()->y, contact->GetPos()->z);
			if (best != NULL) {
				return best->mId;
			}
		}
	}
	if (best != NULL) {
		return best->mId;
	}
	return -1;
}

Ana* GetNearestAna(f32 x, f32 y, f32 z)
{
	Ana* nearest = NULL;
	CVec pos(x, y, z);
	for (u32 i = 0; i < 20; i++) {
		if (anas[i].mStatus.mFlags & 1) {
			f32 dist = PSVECDistance(&pos, &anas[i].mPos);
			if (nearest == NULL || dist < PSVECDistance(&pos, &nearest->mPos)) {
				nearest = &anas[i];
			}
		}
	}
	return nearest;
}

s32 CmdSetTutiAna(s32 cmd, s32 count, s32* args)
{
	Ana* ana = GetAna(args[0]);
	if (!ana->IsActive()) {
		return 0;
	}
	ana->SetJObj(1, 0);
	return 1;
}

s32 CmdPlantTane(s32 cmd, s32 count, s32* args)
{
	Ana* ana = GetAna(args[0]);
	if (!ana->IsActive()) {
		return 0;
	}
	ana->SetJObj(2, 0);
	XObj* obj = gGame.GetPlayer()->mLift.mTarget;
	if (obj != NULL) {
		ReleaseHeld(gGame.GetPlayer());
		DropObj(obj);
	}
	return 1;
}

s32 CmdSetAnaJObj(s32 cmd, s32 count, s32* args)
{
	Ana* ana = GetAna(args[0]);
	if (!ana->IsActive()) {
		return 0;
	}
	ana->SetJObj(args[1], args[2]);
	return 1;
}

s32 CmdSetAnaRotY(s32 cmd, s32 count, s32* args)
{
	Ana* ana   = GetAna(args[0]);
	ana->mRotY = 57.295776f * args[1];
	return 1;
}

s32 CmdDropHeldObj(s32 cmd, s32 count, s32* args)
{
	XObj* obj = gGame.GetPlayer()->mLift.mTarget;
	if (obj != NULL) {
		obj->mStatus.mFlags &= ~0x10000ULL;
		ReleaseHeld(gGame.GetPlayer());
		DropObj(obj);
	}
	return 0;
}

s32 CmdAttachAnaToRobo(s32 cmd, s32 count, s32* args)
{
	Ana* ana = GetAna(args[0]);
	if (!ana->IsActive()) {
		return 0;
	}
	ana->mStatus.mFlags |= 8;
	return 0;
}

extern "C" void UpdateAnas(s32 dt)
{
}

// NONMATCHING: reset and placement block layout differs; all five load paths and the status/JObj resets match.
extern "C" s32 InitAnas(void)
{
	tutiAnaObj  = NULL;
	tutiHutaObj = NULL;
	for (s32 i = 0; i < 3; i++) {
		flowerObjs[i] = NULL;
	}
	activeAnaNum = 0;
	for (u32 i = 0; i < 20; i++) {
		anas[i].mStatus.mFlags = 0;
		anas[i].SetJObj(0, 0);
	}

	if (!anaXObjsInitialized) {
		anaXObjsInitialized = TRUE;
		for (s32 i = 0; i < 20; i++) {
			for (s32 j = 0; j < 5; j++) {
				anaXObjs[i][j].InitializeObject();
			}
		}
	}

	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; i++) {
		XObj* obj = gGame.mXObjs.mObjects[i];
		if (!(obj->mStatus.mFlags & 1)) {
			continue;
		}
		if (obj->mCoord.mPlacement->mObjectId == 0x2A9) {
			tutiAnaObj = obj;
			for (s32 j = 0; j < 20; j++) {
				if (!(anaXObjs[j][0].mStatus.mFlags & 1)) {
					anaXObjs[j][0].LoadFromPlacement(obj->mCoord);
				}
			}
		}
		if (obj->mCoord.mPlacement->mObjectId == 0x178) {
			tutiHutaObj = obj;
			for (s32 j = 0; j < 20; j++) {
				if (!(anaXObjs[j][1].mStatus.mFlags & 1)) {
					anaXObjs[j][1].LoadFromPlacement(obj->mCoord);
				}
			}
		}
		if (obj->mCoord.mPlacement->mObjectId == 0x2B7) {
			flowerObjs[0] = obj;
			for (s32 j = 0; j < 20; j++) {
				if (!(anaXObjs[j][2].mStatus.mFlags & 1)) {
					anaXObjs[j][2].LoadFromPlacement(obj->mCoord);
				}
			}
		}
		if (obj->mCoord.mPlacement->mObjectId == 0x2B8) {
			flowerObjs[1] = obj;
			for (s32 j = 0; j < 20; j++) {
				if (!(anaXObjs[j][3].mStatus.mFlags & 1)) {
					anaXObjs[j][3].LoadFromPlacement(obj->mCoord);
				}
			}
		}
		if (obj->mCoord.mPlacement->mObjectId == 0x2B9) {
			flowerObjs[2] = obj;
			for (s32 j = 0; j < 20; j++) {
				if (!(anaXObjs[j][4].mStatus.mFlags & 1)) {
					anaXObjs[j][4].LoadFromPlacement(obj->mCoord);
				}
			}
		}
	}
	return 1;
}

extern "C" void ExitAnas(void)
{
	for (s32 i = 0; i < 20; i++) {
		for (s32 j = 0; j < 5; j++) {
			anaXObjs[i][j].Release();
		}
	}
}

// NONMATCHING: case/local register layout differs; index selection and animation IDs 1-5 match.
extern "C" XObj* GetAnaJObj(s32 index, s32 kind, s32 which)
{
	if (tutiAnaObj == NULL) {
		return NULL;
	}
	if (which < 0 || which >= 3) {
		which = 0;
	}

	XObj* group = anaXObjs[index];
	XObj* obj   = &group[which + 2];
	switch (kind) {
	case 1:
		return &group[0];
	case 2:
		return &group[1];
	case 3:
		if (obj != NULL) {
			obj = &group[2];
			obj->SetAnim(1, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		return obj;
	case 4:
		if (obj != NULL) {
			obj->SetAnim(2, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		return obj;
	case 5:
		if (obj != NULL) {
			obj->SetAnim(3, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		return obj;
	case 6:
		if (obj != NULL) {
			obj->SetAnim(4, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		return obj;
	case 7:
		if (obj != NULL) {
			obj->SetAnim(5, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		return obj;
	}
	return NULL;
}

// NONMATCHING: position/scene-list register layout differs; carry/position selection and all render calls match.
extern "C" void DrawAnas(void)
{
	for (u32 i = 0; i < 20; i++) {
		if (!(anas[i].mStatus.mFlags & 1)) {
			continue;
		}
		XObj* obj = GetAnaJObj(i, anas[i].mJObjKind, anas[i].mJObjWhich);
		if (obj == NULL) {
			continue;
		}
		Vec pos;
		if (anas[i].mStatus.mFlags & 8) {
			pos = fn_800E5A2C(gGame.GetPlayer());
		} else {
			pos = anas[i].mPos;
		}
		obj->SetPos(pos);
		gGame.mGs.mScene.GetRoot()->Insert(gGame.mGs.mScene.GetRoot()->mNumScnObj, obj->mObject.m_pScnMdl);
	}

	gGame.mGs.mScene.GetRoot()->UpdateFrame();
	gGame.mGs.mScene.GetRoot()->CalcWorld();
	gGame.mGs.mScene.GetRoot()->CalcView();
	gGame.mGs.mScene.GetRoot()->GatherDrawScnObj();
	gGame.mGs.mScene.GetRoot()->DrawOpa();
	gGame.mGs.mScene.GetRoot()->DrawXlu();
	while (!gGame.mGs.mScene.GetRoot()->Empty()) {
		gGame.mGs.mScene.GetRoot()->PopBack();
	}
}

extern XObj trashObjs[10];
extern XObj* trashTarget;
extern XObjFlags* trashObj;
extern s32 trashStackNum;
extern f32 trashStackScale;
extern s32 trashState;
extern s32 trashResult;
extern u8 trashActive;

// NONMATCHING: argument conversion and target setup are scheduled differently; all stores and the gauge call match.
extern "C" s32 CmdStartTrash(s32 cmd, s32 count, s32* args)
{
	f32 scale       = args[2] / 100.0f;
	s32 idx         = args[0];
	s32 num         = args[1];
	trashTarget     = gGame.mXObjs.mObjects[idx];
	trashStackNum   = num;
	trashStackScale = scale;
	if (trashTarget != NULL) {
		trashResult = 0;
		trashState  = 0;
		trashActive = TRUE;
		if (gGame.GetPlayer()->mLift.mTarget != NULL) {
			InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 1, 1, 1);
		}
	}
	return 0;
}

extern "C" s32 CmdEndTrash(s32 cmd, s32 count, s32* args)
{
	if (trashTarget != NULL) {
		trashObj    = NULL;
		trashActive = FALSE;
		InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 0, 1, 1);
	}
	return 0;
}

// NONMATCHING: loop-register selection differs; the active test, both release paths and the 0x9A8 array stride match.
extern "C" s32 CmdCancelTrash(s32 cmd, s32 count, s32* args)
{
	WiiPlayerState* robot = gGame.GetPlayer();
	if (robot->mLift.mTarget != NULL) {
		robot->mLift.mState = 6;
	}
	if (trashObj != NULL && (trashObj->mStatus.mFlags & 1)) {
		trashObj->Release();
		trashObj = NULL;
	}
	for (s32 i = 0; i < 10; i++) {
		if ((trashObjs[i].mStatus.mFlags & 1) == 1) {
			trashObjs[i].Release();
		}
	}
	return 0;
}

extern "C" s32 CmdGetTrashResult(s32 cmd, s32 count, s32* args)
{
	return trashResult;
}

extern "C" s32 CmdIsNearTrashbox(s32 cmd, s32 count, s32* args);

extern "C" s32 fn_8016C398(s32 cmd, s32 count, s32* args)
{
	return CmdIsNearTrashbox(cmd, count, args);
}
#else
#include "game/relana.h"
#include "Version.h"

// the holes the player digs and plants. linked into five stages. the functions are in the order they
// were written, retail's code runs the other way (-inline deferred)

#include "game/hsd_jobj.h"
#include "game/ut.h"
#include "game/cgame.h"
#include "game/xchr.h"
#include "game/xobjflags.h"

extern "C" {
void OSReport(const char* msg, ...);
}

class Ana {
public:
	Ana() { }
	~Ana() { }
	virtual bool IsActive() { return mStatus.mFlags & 1; }

	void Set(f32 x, f32 y, f32 z);

	void SetJObj(s32 kind, s32 which)
	{
		mJObjKind  = kind;
		mJObjWhich = which;
	}

	Vec mPos;        // _04
	f32 mRotY;       // _10
	CStatus mStatus; // _14
	s32 mId;         // _1C
	s32 mJObjKind;   // _20
	s32 mJObjWhich;  // _24
};

class AnaNull : public Ana {
public:
	virtual bool IsActive() { return false; }
};

#define ANA_NUM 20

s32 activeAnaNum;
static Ana anas[ANA_NUM];
static AnaNull anaNull;
XObj* tutiAnaObj;
XObj* tutiHutaObj;
XObj* flowerObjs[3];

inline Ana* GetNearestAna(f32 x, f32 y, f32 z);

inline Ana* GetAna(s32 id)
{
	if (id < 0 || id >= ARRAY_SIZE(anas)) {
		return &anaNull;
	}
	return &anas[id];
}

inline void ReleaseHeld(XObjChr* player)
{
	if (player->mLift.mTarget != NULL) {
		player->mLift.mTarget->mStatus.mFlags &= ~0x0400000000000000ULL; // no longer held
		player->mLift.mTarget = NULL;
	}
	player->mLift.mState = 0;
}

inline void DropObj(XObj* obj)
{
	if (obj->GetCategory() == 8) {
		static_cast<XObjFlags*>(obj)->SetFlag();
	}
	obj->Release();
}

extern "C" HSD_JObj* GetAnaJObj(s32 kind, s32 which);

// defined further down
extern "C" void Ana_Draw(Ana* ana);
extern "C" s32 AddAna(s32 id, f32 x, f32 y, f32 z);

/**
 * @note Address: stage04 .text 0x1818
 * @note Size: 0x24
 */
// Ana::Ana
/**
 * @note Address: stage04 .text 0x17B8
 * @note Size: 0x60
 */
// Ana::~Ana

/**
 * @note Address: stage04 .text 0x1780
 * @note Size: 0x38
 */
// destroys anas at exit, compiler made

/**
 * @note Address: stage04 .text 0x1738
 * @note Size: 0x48
 */
// CStatus::~CStatus, inline in the class above

/**
 * @note Address: stage04 .text 0x16C8
 * @note Size: 0x70
 */
// AnaNull::~AnaNull, compiler made

/**
 * @note Address: stage04 .text 0x16C0
 * @note Size: 0x8
 */
// AnaNull::IsActive, inline in the class above

/**
 * @note Address: stage04 .text 0x1624
 * @note Size: 0x9C
 */
// static initializer for anas and anaNull

/**
 * @note Address: stage04 .text 0x1518
 * @note Size: 0x10C
 */
extern "C" s32 CmdAddAna(s32 cmd, s32 count, s32* args)
{
	if (count >= 4) {
		return AddAna(args[3], args[0] / 100.0f, args[1] / 100.0f, args[2] / 100.0f);
	}
	return AddAna(-1, args[0] / 100.0f, args[1] / 100.0f, args[2] / 100.0f);
}

/**
 * @note Address: stage04 .text 0x148C
 * @note Size: 0x8C
 */
extern "C" s32 CmdSetTutiAna(s32 cmd, s32 count, s32* args)
{
	Ana* ana = GetAna(args[0]);
	if (!ana->IsActive()) {
		return 0;
	}
	ana->SetJObj(1, 0); // kind 1 shows the dug hole
	return 1;
}

/**
 * @note Address: stage04 .text 0x124C
 * @note Size: 0x240
 */
extern "C" s32 CmdFindNearestAna(s32 cmd, s32 count, s32* args)
{
	Ana* best = NULL;

	if (count == 3) {
		f32 x = args[0] / 100.0f;
		f32 y = args[1] / 100.0f;
		f32 z = args[2] / 100.0f;
		best  = GetNearestAna(x, y, z);
	} else {
		XObj* contact = gGame.GetObject(gGame.GetPlayer()->mFloorContacts[2].objectIndex);
		if (contact != NULL) {
			const Vec& pos = contact->mMoveTranslate;
			best           = GetNearestAna(pos.x, pos.y, pos.z);
		}
	}
	return best != NULL ? best->mId : -1;
}

/**
 * @note Address: stage04 .text 0x1100
 * @note Size: 0x14C
 */
extern "C" s32 CmdPlantTane(s32 cmd, s32 count, s32* args)
{
	XObj* obj;
	Ana* ana = GetAna(args[0]);
	if (!ana->IsActive()) {
		return 0;
	}
	ana->SetJObj(2, 0); // kind 2 shows the lid
	obj = gGame.GetPlayer()->mLift.mTarget;
	if (obj != NULL) {
		ReleaseHeld(gGame.GetPlayer()); // the planted object leaves the hand
		DropObj(obj);
	}
	return 1;
}

/**
 * @note Address: stage04 .text 0x1068
 * @note Size: 0x98
 */
extern "C" s32 CmdSetAnaJObj(s32 cmd, s32 count, s32* args)
{
	Ana* ana = GetAna(args[0]);
	if (!ana->IsActive()) {
		return 0;
	}
	ana->SetJObj(args[1], args[2]);
	return 1;
}

/**
 * @note Address: stage04 .text 0xFF4
 * @note Size: 0x74
 */
extern "C" s32 CmdSetAnaRotY(s32 cmd, s32 count, s32* args)
{
	Ana* ana   = GetAna(args[0]);
	ana->mRotY = 57.295776f * args[1]; // scaled from the script's unit
	return 1;
}

/**
 * @note Address: stage04 .text 0xEDC
 * @note Size: 0x118
 */
extern "C" s32 CmdDropHeldObj(s32 cmd, s32 count, s32* args)
{
	XObj* obj = gGame.GetPlayer()->mLift.mTarget;
	if (obj != NULL) {
		obj->mStatus.mFlags &= ~0x10000ULL;
		ReleaseHeld(gGame.GetPlayer());
		DropObj(obj);
	}
	return 0;
}

/**
 * @note Address: stage04 .text 0xED0
 * @note Size: 0xC
 */
// Ana::IsActive, inline in the class above

/**
 * @note Address: stage04 .text 0xE48
 * @note Size: 0x88
 */
extern "C" s32 CmdAttachAnaToRobo(s32 cmd, s32 count, s32* args)
{
	Ana* ana = GetAna(args[0]);
	if (!ana->IsActive()) {
		return 0;
	}
	ana->mStatus.mFlags |= 8; // flag 8 draws it in the player's hand
	return 0;
}

/**
 * @note Address: stage04 .text 0xCFC
 * @note Size: 0x14C
 */
extern "C" void InitAnas(void)
{
	s32 num;
	s32 i;
	u32 j;

	tutiAnaObj  = NULL;
	tutiHutaObj = NULL;
	for (j = 0; j < ARRAY_SIZE(flowerObjs); j++) {
		flowerObjs[j] = NULL;
	}
	activeAnaNum = 0;
	for (j = 0; j < ANA_NUM; j++) {
		anas[j].mStatus.mFlags = 0;
	}
	num = gGame.mXObjs.mObjectCount;

	// find the hole, its lid and the three flowers among the placed objects
	for (i = 0; i < num; i++) {
		XObj* obj = gGame.GetObject(i);
		if (obj->mStatus.mFlags & 1) {
			if (obj->mCoord.mPlacement->mObjectId == 0x2A9) {
				tutiAnaObj = obj;
			}
			if (obj->mCoord.mPlacement->mObjectId == 0x178) {
				tutiHutaObj = obj;
			}
			if (obj->mCoord.mPlacement->mObjectId == 0x2B7) {
				flowerObjs[0] = obj;
			}
			if (obj->mCoord.mPlacement->mObjectId == 0x2B8) {
				flowerObjs[1] = obj;
			}
			if (obj->mCoord.mPlacement->mObjectId == 0x2B9) {
				flowerObjs[2] = obj;
			}
		}
	}
}

/**
 * @note Address: stage04 .text 0xCF8
 * @note Size: 0x4
 */
extern "C" void UpdateAnas(s32 frames)
{
}

/**
 * @note Address: stage04 .text 0xC9C
 * @note Size: 0x5C
 */
extern "C" void DrawAnas(void)
{
	u32 i;
	for (i = 0; i < ANA_NUM; i++) {
		if (anas[i].mStatus.mFlags & 1) {
			Ana_Draw(&anas[i]);
		}
	}
}

/**
 * @note Address: stage04 .text 0xA4C
 * @note Size: 0x250
 */
extern "C" s32 AddAna(s32 id, f32 x, f32 y, f32 z)
{
	CVec pos(x, y, z);
	u32 i;

	if (id >= 0 && id < ARRAY_SIZE(anas)) {
		Ana& ana = anas[id];
		if (!(ana.mStatus.mFlags & 1)) {
			activeAnaNum++;
		}
#if !defined(VERSION_GGTP01)
		OSReport("id=%d,%4.2f,%4.2f,%4.2f\n", id, x, y, z);
#endif
		ana.Set(x, y, z);
		ana.mId = id;
		return id;
	}

	for (i = 0; i < ARRAY_SIZE(anas); i++) {
		if ((anas[i].mStatus.mFlags & 1) && PSVECDistance(&pos, &anas[i].mPos) < 5.0f) {
			return -1; // too close to an existing hole
		}
	}
	for (i = 0; i < ARRAY_SIZE(anas); i++) {
		if (!(anas[i].mStatus.mFlags & 1)) {
			activeAnaNum++;
			anas[i].Set(x, y, z);
			anas[i].mId = i;
			return i;
		}
	}
	return -1;
}

// no stage has this function's code: both calls are inlined and the linker dropped the rest. its
// string lies between Ana::Set's and AddAna's in retail, so it was compiled here
inline Ana* GetNearestAna(f32 x, f32 y, f32 z)
{
	Ana* nearest = NULL;
	u32 i;
	CVec pos(x, y, z);
#if !defined(VERSION_GGTP01)
	OSReport("%4.2f %4.2f %4.2f\n", x, y, z);
#endif
	for (i = 0; i < ARRAY_SIZE(anas); i++) {
		if (anas[i].mStatus.mFlags & 1) {
			f32 dist = PSVECDistance(&pos, &anas[i].mPos);
			if (nearest == NULL || dist < PSVECDistance(&pos, &nearest->mPos)) {
				nearest = &anas[i];
			}
		}
	}
	return nearest;
}

// takes the hole into use at a place. only its inlined copies are left, like GetNearestAna's
inline void Ana::Set(f32 x, f32 y, f32 z)
{
#if !defined(VERSION_GGTP01)
	OSReport("Ana::%4.2f, %4.2f, %4.2f\n", x, y, z);
#endif
	mStatus.mFlags |= 1;
	mPos.x = x;
	mPos.y = y;
	mPos.z = z;
}

/**
 * @note Address: stage04 .text 0x5EC
 * @note Size: 0x460
 */
extern "C" HSD_JObj* GetAnaJObj(s32 kind, s32 which)
{
	HSD_JObj* jobj0;
	HSD_JObj* jobj1;
	XObj* obj;

	if (tutiAnaObj == NULL) {
		return NULL;
	}
	jobj0 = tutiAnaObj->mJObj;
	jobj1 = NULL;
	if (tutiHutaObj != NULL) {
		jobj1 = tutiHutaObj->mJObj;
	}
	if (which < 0 || which >= ARRAY_SIZE(flowerObjs)) {
		which = 0;
	}
	obj = flowerObjs[which];
	switch (kind) {
	case 1: // the hole itself
		return jobj0;
	case 2: // its lid
		return jobj1;
	case 3: { // the flower's joints
		if (obj == NULL) {
			return NULL;
		}
		s32 path[2] = { 0, 0 };
		return GetJointByPath(obj->mJObj, path, 1);
	}
	case 4: {
		if (obj == NULL) {
			return NULL;
		}
		s32 path[2] = { 1, 0 };
		return GetJointByPath(obj->mJObj, path, 1);
	}
	case 5: {
		if (obj == NULL) {
			return NULL;
		}
		s32 path[2] = { 2, 0 };
		return GetJointByPath(obj->mJObj, path, 1);
	}
	case 6: // joints 3 and 4, swapped between 6 and 7
		if (obj == NULL) {
			return NULL;
		}
		if (which == 0) {
			s32 path[2] = { 4, 0 };
			return GetJointByPath(obj->mJObj, path, 1);
		} else {
			s32 path[2] = { 3, 0 };
			return GetJointByPath(obj->mJObj, path, 1);
		}
	case 7:
		if (obj == NULL) {
			return NULL;
		}
		if (which == 0) {
			s32 path[2] = { 3, 0 };
			return GetJointByPath(obj->mJObj, path, 1);
		} else {
			s32 path[2] = { 4, 0 };
			return GetJointByPath(obj->mJObj, path, 1);
		}
	}
	return NULL;
}

/**
 * @note Address: stage04 .text 0x2F0
 * @note Size: 0x2FC
 */
extern "C" void Ana_Draw(Ana* ana)
{
	HSD_JObj* jobj = GetAnaJObj(ana->mJObjKind, ana->mJObjWhich);
	f32 x, y, z;

	if (jobj == NULL) {
		return;
	}
	if (ana->mStatus.mFlags & 8) { // carried: draw it in the player's hand
		const CVec& pos = GetPlayerCarryJointWorldPosition(gGame.GetPlayer());
		x               = pos.x;
		y               = pos.y;
		z               = pos.z;
	} else {
		x = ana->mPos.x;
		y = ana->mPos.y;
		z = ana->mPos.z;
	}
	HSD_JObjSetTranslateX(jobj, x);
	HSD_JObjSetTranslateY(jobj, y);
	HSD_JObjSetTranslateZ(jobj, z);
	HSD_JObjSetRotationY(jobj, ana->mRotY);
	HSD_JObjDispAll(jobj, NULL, 7, 0);
}

/**
 * @note Address: stage04 .text 0x2A8
 * @note Size: 0x48
 */
// CVec::~CVec
#endif
