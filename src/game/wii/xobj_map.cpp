// the map object's file: the XObjMap overrides in 0x800C7DC0-0x800C9E84
// (wii_units.md Nr. 59) plus the scene-proc draw helper and the record lists
#include "Version.h"

#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h" // the Wii MSL's out-of-line __fpclassifyf

#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/globals.h"

#include "arithmetic.h"

#include "game/cprimitive.h"
#include "Dolphin/mtx.h"

#include <math.h>

extern MEMAllocator gWiiPadAllocator;

// registers the object on the scene's draw list, same helper the scene pass uses
extern "C" void fn_802AEE44(WiiGsSceneObj** head, XObj* obj);
// the map's translucent scene proc: draws the record lists the unit keeps
extern "C" void fn_800C83DC(nw4r::g3d::ScnProc* proc, bool opa);
// stage_data's always-false stub; the second list reset is dead code behind it
extern "C" s32 fn_800929EC();
// the sort-grid build this file's rebuild runs once the trigon is big enough
extern "C" void fn_800C85EC(XObjMap* map);
// the map's stage-mode check, an out-of-line wrapper over stage_data's stub
extern "C" void fn_800C9B84(XObjMap* map);
// the ctrigon unit's ScnMdl triangle build (its own lane still has it as asm)
extern "C" void fn_802BA4F4(CTrigon* trigon, nw4r::g3d::ScnMdl* mdl);

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

// 0x800C7F70, the map draw rebuild the floor-reflection render runs on the map
// each frame: re-queues the model on the scene's draw list, rebuilds the floor
// trigon out of it and recomputes the sort grid once the trigon is big enough
/**
 * @note Address: 0x800C7F70
 * @note Size: 0x10C
 */
extern "C" void fn_800C7F70(XObjMap* map)
{
	if (map->mObject.m_pScnMdl == NULL) {
		return;
	}
	gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, map->mObject.m_pScnMdl);
	((nw4r::g3d::ScnRoot*)gGame.mGs.mScene.mHead)->CalcWorld();
	gGame.mGs.mScene.mHead->vf03C(map->mObject.m_pScnMdl);
	fn_802BA4F4(&map->mTrigon, map->mObject.m_pScnMdl);
	if (map->mTrigon.mNum > 1000) {
		map->mSortValid = 1;
		map->mSortMin   = map->mTrigon.mMin;
		map->mSortMax   = map->mTrigon.mMax;
		map->mSortCellX = (map->mSortMax.x - map->mSortMin.x) / 10.0f;
		map->mSortCellZ = (map->mSortMax.z - map->mSortMin.z) / 10.0f;
		fn_800C85EC(map);
		fn_800C9B84(map);
	} else {
		map->mSortValid = 0;
	}
}

// 0x800C807C, the map reset the event system's ANIM command runs on kind 3:
// empties both sort lists, releases the trigon and rebuilds the draw lists
/**
 * @note Address: 0x800C807C
 * @note Size: 0x148
 */
extern "C" void fn_800C807C(XObjMap* map)
{
	if (map->mSortValid != 0) {
		for (s32 i = 0; i < 10.0f; i++) {
			for (s32 k = 0; k < 10.0f; k++) {
				map->mListB[i * 10 + k].mUsed = map->mListB[i * 10 + k].mUsed - map->mListB[i * 10 + k].mUsed;
			}
		}
	}
	if (fn_800929EC() != 0 && map->mSortValid != 0) {
		for (s32 i = 0; i < 10.0f; i++) {
			for (s32 k = 0; k < 10.0f; k++) {
				map->mListA[i * 10 + k].mUsed -= map->mListA[i * 10 + k].mUsed;
			}
		}
	}
	map->mTrigon.Release();
	fn_800C7F70(map);
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
	if (mSortValid != 0) {
		for (s32 i = 0; i < 10.0f; i++) {
			for (s32 k = 0; k < 10.0f; k++) {
				mListB[i * 10 + k].mUsed = mListB[i * 10 + k].mUsed - mListB[i * 10 + k].mUsed;
			}
		}
	}
	if (fn_800929EC() != 0 && mSortValid != 0) {
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

// one of the sixteen {cell, triangle} marks _1378 keeps for the debug draw; the
// cell's first word is the triangle run it indexes into
struct XObjMapMark {
	TRIGON_CELL* mCell; // _00
	u32 mTri;           // _04
};
STATIC_ASSERT(sizeof(XObjMapMark) == 8);

// the mark colors the debug triangles draw in, alpha 0x40
const PrimVertex sMapMarkTri[3] = {
	{ { 0.0f, 0.0f, 0.0f }, 0xFF000040 },
	{ { 0.0f, 0.0f, 0.0f }, 0x00FF0040 },
	{ { 0.0f, 0.0f, 0.0f }, 0x0000FF40 },
};

// 0x800C83DC, the translucent proc's draw: paints every marked cell triangle
// lifted a tenth up through its own blend, then drops the mark
/**
 * @note Address: 0x800C83DC
 * @note Size: 0x1F8
 */
extern "C" void fn_800C83DC(nw4r::g3d::ScnProc* proc, bool opa)
{
	XObjMapMark* marks = (XObjMapMark*)proc->GetUserData();
	for (s32 i = 0; i < 16; i++) {
		if (marks[i].mCell == NULL) {
			continue;
		}
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetLineWidth(0xC, GX_TO_ONE);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
		TRIGON_INFO* tri = &marks[i].mCell->mTri[marks[i].mTri];
		Vec out[3];
		tri->mGetVtx(tri, out);
		PrimVertex vtx[3];
		for (s32 v = 0; v < 3; v++) {
			vtx[v].pos.x = out[v].x;
			vtx[v].pos.y = out[v].y + 0.1f;
			vtx[v].pos.z = out[v].z;
			vtx[v].color = sMapMarkTri[v].color;
		}
		DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, vtx, 3);
		marks[i].mCell = NULL;
	}
}

// NONMATCHING: retail keeps this out-of-line with a full frame and an extab
// entry, and fn_800C7F70 reaches it through bl; our build sibling-calls the
// stub (b) and inlines the wrapper into its only caller. the frame implies an
// exception-region construct in the original that the surviving instructions
// do not show
// 0x800C9B84, the map's stage-mode check: an out-of-line wrapper over
// stage_data's always-false stub, so the rebuild and the list reset agree
/**
 * @note Address: 0x800C9B84
 * @note Size: 0x20
 */
extern "C" void fn_800C9B84(XObjMap* map)
{
	fn_800929EC();
}
