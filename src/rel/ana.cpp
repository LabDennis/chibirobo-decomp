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

Ana* GetNearestAna(f32 x, f32 y, f32 z);

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

// CmdDropHeldObj

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

// InitAnas

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

// AddAna

// no stage has this function's code: both calls are inlined and the linker dropped the rest. its
// string lies between Ana::Set's and AddAna's in retail, so it was compiled here
Ana* GetNearestAna(f32 x, f32 y, f32 z)
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
void Ana::Set(f32 x, f32 y, f32 z)
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
