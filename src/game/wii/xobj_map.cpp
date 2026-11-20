// the map object's file: the XObjMap overrides in 0x800C7DC0-0x800C9E84
// (wii_units.md Nr. 59) plus the scene-proc draw helper and the record lists
#include "Version.h"

#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h" // the Wii MSL's out-of-line __fpclassifyf

#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/globals.h"

#include "arithmetic.h"

#include "Dolphin/mtx.h"

#include <math.h>

extern MEMAllocator gWiiPadAllocator;

// registers the object on the scene's draw list, same helper the scene pass uses
void fn_802AEE44(WiiGsSceneObj** head, XObj* obj);
// the map's translucent scene proc: draws the record lists the unit keeps
void fn_800C83DC(nw4r::g3d::ScnProc* proc, bool opa);
// stage_data's always-false stub; the second list reset is dead code behind it
s32 fn_800929EC();

// nw4r's db pointer check (db_assert.h) as the game's sources use it; the line is the game's own
#define NW4R_ASSERT_VALID_PTR_LINE(line, exp)                                                                                              \
	((void)((((u32)(exp) & 0xFF000000) == 0x80000000 || ((u32)(exp) & 0xFF800000) == 0x81000000 || ((u32)(exp) & 0xF8000000) == 0x90000000 \
	         || ((u32)(exp) & 0xFF000000) == 0xC0000000 || ((u32)(exp) & 0xFF800000) == 0xC1000000                                         \
	         || ((u32)(exp) & 0xF8000000) == 0xD0000000 || ((u32)(exp) & 0xFFFFC000) == 0xE0000000)                                        \
	        || (nw4r::db::Panic(__FILE__, line, "NW4R:Pointer Error\n" #exp "(=%p) is not valid pointer.", (exp)), 0)))

/**
 * @note Address: 0x800C7DC0
 * @note Size: 0x1B0
 */
BOOL XObjMap::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info) != FALSE) {
		m_1374 = nw4r::g3d::ScnProc::Construct(&gWiiPadAllocator, NULL, fn_800C83DC, false, true, 0);
		NW4R_ASSERT_VALID_PTR_LINE(78, m_1374);
		m_1374->SetUserData(_1378);
		if (mObject.m_resFile.HasResAnmScn() && mObject.m_resFile.GetResAnmScnNumEntries() != 0) {
			SetAnmScn(0, 0.0f, 0.0f);
		}
		fn_802AEE44(&gGame.mGs.mScene.mHead, this);
		return TRUE;
	}
	return FALSE;
}

// NONMATCHING: retail keeps each element's mUsed clear as the family's
// self-subtract (lwz + subf r0, r0, r0, like ~XObjMapListA's) and walks a
// pointer induction variable; our build folds x - x to a constant store and
// indexes instead. the float loop bounds and the rest of the body match
/**
 * @note Address: 0x800C81C4
 * @note Size: 0x178
 */
void XObjMap::Release()
{
	if (m_1374 != NULL) {
		m_1374->Destroy();
		m_1374 = NULL;
	}
	fn_802AEE44(&gGame.mGs.mScene.mHead, NULL);
	if (_9EC[0] != 0) {
		for (s32 i = 0; i < 10.0f; i++) {
			for (s32 k = 0; k < 10.0f; k++) {
				mListB[i * 10 + k].mUsed = mListB[i * 10 + k].mUsed - mListB[i * 10 + k].mUsed;
			}
		}
	}
	if (fn_800929EC() != 0 && _9EC[0] != 0) {
		for (s32 i = 0; i < 10.0f; i++) {
			for (s32 k = 0; k < 10.0f; k++) {
				mListA[i * 10 + k].mUsed -= mListA[i * 10 + k].mUsed;
			}
		}
	}
	mTrigon.Release();
	XObj::Release();
}

/**
 * @note Address: 0x800C833C
 * @note Size: 0x34
 */
void XObjMap::UpdateObject(u32 frames)
{
	XObj::UpdateObject(frames);
	mObject.m_resFile.HasResAnmScn();
}

/**
 * @note Address: 0x800C8370
 * @note Size: 0x58
 */
void XObjMap::DrawWithView(Mtx view)
{
	XObj::DrawWithView(view);
	if (m_1374 != NULL) {
		WiiGsSceneObj* head = gGame.mGs.mScene.mHead;
		head->vf034(head->mPending, m_1374);
	}
}

/**
 * @note Address: 0x800C83C8
 * @note Size: 0x14
 */
void XObjMap::DrawGroundShadow(f32 opacity)
{
	vf170(NULL);
}
