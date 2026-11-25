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
extern "C" struct TRIGON_INFO* fn_802BA4F4(CTrigon* trigon, nw4r::g3d::ScnMdl* mdl);

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
				map->mListB[i][k].clear();
			}
		}
	}
	if (fn_800929EC() != 0 && map->mSortValid != 0) {
		for (s32 i = 0; i < 10.0f; i++) {
			for (s32 k = 0; k < 10.0f; k++) {
				map->mListA[i][k].mUsed -= map->mListA[i][k].mUsed;
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
				mListB[i][k].clear();
			}
		}
	}
	if (fn_800929EC() != 0 && mSortValid != 0) {
		for (s32 i = 0; i < 10.0f; i++) {
			for (s32 k = 0; k < 10.0f; k++) {
				mListA[i][k].mUsed -= mListA[i][k].mUsed;
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

// 0x800C85D4, the grid build's record order: entries sort by height,
// highest first
/**
 * @note Address: 0x800C85D4
 * @note Size: 0x18
 */
extern "C" s32 fn_800C85D4(const XObjMapSortRec& a, const XObjMapSortRec& b)
{
	return a.mY > b.mY;
}

// NONMATCHING: retail keeps this out-of-line with a full frame and an extab
// entry, and fn_800C7F70 reaches it through bl; our build sibling-calls the
// stub (b) and inlines the wrapper into its only caller. A throw() specification adds
// __unexpected paths retail lacks, and returning the stub's result still sibling-calls it.
// The frame implies an exception-region construct that the surviving instructions do not show
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

// the game's own heap ops the cell grow calls (gamemain.cpp/delete.cpp)
void* operator new(u32 size) throw();
void operator delete(void* ptr) throw();

// NONMATCHING: the real MSL vector push_back and all three length_error throws are present,
// including the temporary vector's unwind lifetime. The rest is frame/register allocation and
// the shared_ptr array cleanup on allocation failure. Retail passes the sort's iterators by
// address, so its vector iterator is a class type, not T*: the three sort names in symbols.txt
// (sort<P14XObjMapSortRec,...>, __sort132<...,P14XObjMapSortRec>) are unproven guesses.
// 0x800C85EC, the sort-grid build fn_800C7F70 runs once the trigon is big
// enough: every trigon triangle goes on the record list of each 10x10 cell
// it overlaps — a triangle counts when one of its corners or edge midpoints
// falls in the cell's rect — then every cell list is sorted, highest first
/**
 * @note Address: 0x800C85EC
 * @note Size: 0x814
 */
extern "C" void fn_800C85EC(XObjMap* map)
{
	if (map->mSortValid == 0) {
		return;
	}
	for (s32 i = 0; i < 10.0f; i++) {
		for (s32 k = 0; k < 10.0f; k++) {
			XObjMapListB* cell = &map->mListB[i][k];
			f32 cellX          = i * map->mSortCellX + map->mSortMin.x;
			f32 cellZ          = k * map->mSortCellZ + map->mSortMin.z;
			for (u32 t = 0; t < map->mTrigon.mNum; t++) {
				TRIGON_INFO* tri = &map->mTrigon.mTri[t];
				Vec v[3];
				tri->mGetVtx(tri, v);
				if ((v[0].x >= cellX && v[0].x < cellX + map->mSortCellX && v[0].z >= cellZ && v[0].z < cellZ + map->mSortCellZ)
				    || (v[1].x >= cellX && v[1].x < cellX + map->mSortCellX && v[1].z >= cellZ && v[1].z < cellZ + map->mSortCellZ)
				    || (v[2].x >= cellX && v[2].x < cellX + map->mSortCellX && v[2].z >= cellZ && v[2].z < cellZ + map->mSortCellZ)
				    || ((v[0].x + v[1].x) * 0.5f >= cellX && (v[0].x + v[1].x) * 0.5f < cellX + map->mSortCellX
				        && (v[0].z + v[1].z) * 0.5f >= cellZ && (v[0].z + v[1].z) * 0.5f < cellZ + map->mSortCellZ)
				    || ((v[1].x + v[2].x) * 0.5f >= cellX && (v[1].x + v[2].x) * 0.5f < cellX + map->mSortCellX
				        && (v[1].z + v[2].z) * 0.5f >= cellZ && (v[1].z + v[2].z) * 0.5f < cellZ + map->mSortCellZ)
				    || ((v[2].x + v[0].x) * 0.5f >= cellX && (v[2].x + v[0].x) * 0.5f < cellX + map->mSortCellX
				        && (v[2].z + v[0].z) * 0.5f >= cellZ && (v[2].z + v[0].z) * 0.5f < cellZ + map->mSortCellZ)) {
					XObjMapSortRec rec = { t, v[0].y + v[1].y + v[2].y };
					cell->push_back(rec);
				}
			}
			std::sort(cell->begin(), cell->end(), fn_800C85D4);
		}
	}
}

// NONMATCHING: the on-demand triangle addressing matches retail, which keeps the initial hit in a
// found flag before the sort-grid stage. The remaining difference is register and Vec-slot allocation:
// retail assigns the grid loop registers differently and uses the hint probe's Vec slots below the grid's
// 0x800C98E0, the Wii's out-of-line FindMapFloor: fills hit with the floor
// triangle under pos — first trying the start triangle itself, then the
// record list of the sort-grid cell the point falls in, then the trigon's
// own check — and reports whether any of them found a floor. hit->mTrigon
// keeps the found triangle's grid cell, -1 when there is no floor
/**
 * @note Address: 0x800C98E0
 * @note Size: 0x2A4
 */
extern "C" BOOL fn_800C98E0(XObjMap* map, const Vec* pos, s32 start, XObjMapHit* hit)
{
	BOOL found = FALSE;
	if (start >= 0 && start < map->mTrigon.mNum) {
		Vec v[3];
		Vec at;
		map->mTrigon.mTri[start].mGetVtx(&map->mTrigon.mTri[start], v);
		if (IsPointInsideTriangleXZ((Vec*)pos, &v[0], &v[1], &v[2])) {
			IntersectVerticalLineWithTrianglePlane((Vec*)pos, &v[0], &v[1], &v[2], &at);
			hit->mTriangle = start;
			hit->mTrigon   = map->mTrigon.mTri[start].mCell;
			hit->mHeight   = at.y;
			found          = TRUE;
		}
	}
	if (found) {
		return TRUE;
	}
	if (map->mSortValid != 0) {
		s32 cellX = (pos->x - map->mSortMin.x) / map->mSortCellX;
		s32 cellZ = (pos->z - map->mSortMin.z) / map->mSortCellZ;
		if (cellX >= 0 && cellX < 10.0f && cellZ >= 0 && cellZ < 10.0f) {
			f32 limit          = pos->y + 7.5f;
			XObjMapListB* cell = &map->mListB[cellX][cellZ];
			for (u32 i = 0; i < cell->size(); i++) {
				s32 index = cell->data()[i].mTri;
				Vec v[3];
				Vec at;
				map->mTrigon.mTri[index].mGetVtx(&map->mTrigon.mTri[index], v);
				if (IsPointInsideTriangleXZ((Vec*)pos, &v[0], &v[1], &v[2])) {
					IntersectVerticalLineWithTrianglePlane((Vec*)pos, &v[0], &v[1], &v[2], &at);
					if (limit >= at.y) {
						hit->mTriangle = index;
						hit->mTrigon   = map->mTrigon.mTri[index].mCell;
						hit->mHeight   = at.y;
						return TRUE;
					}
				}
			}
		}
	}
	hit->mTriangle = map->mTrigon.Check((Vec*)pos, -1, &hit->mHeight);
	if (hit->mTriangle >= 0) {
		hit->mTrigon = map->mTrigon.mTri[hit->mTriangle].mCell;
		return TRUE;
	}
	hit->mTrigon = -1;
	return FALSE;
}
