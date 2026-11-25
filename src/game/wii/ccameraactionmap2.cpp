#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h" // the Wii MSL's out-of-line __fpclassifyf
#include "nw4r/math/math_types.h"
#include "nw4r/math/math_arithmetic.h"
#include "nw4r/db/db_assert.h"
#include "game/ccameraaction.h"
#if defined(VERSION_R24J01)
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#endif
#include "game/cameracollisionpolygons.h"
#include "game/caminfo.h"
#include "game/stagedata.h"
#include "game/gmath.h"
#include "game/ut.h"
#include "game/cvec.h"
#include <string.h>
#include <math.h>
#include <cmath>
#include <stdlib.h>
extern "C" {
#include <dolphin/os.h>
}

extern s32 gRefreshRate;

// the pointer's screen resolution, cpadact.cpp
extern s32 gPointerScreenWidth, gPointerScreenHeight;

// the Wii builds divide by the current refresh rate, the GameCube header's FRAME_TIME is the 60 Hz constant
#undef FRAME_TIME
#define FRAME_TIME (1.0f / gRefreshRate)

// the map camera's half of the Wii camera code; the area camera's half is ccameraactionarea.cpp

extern "C" void DrawMapCameraAreaDebug3D(CCameraActionAreaEx* state, s32 mode);

extern "C" {
void UpdateCameraAreaIndexAndBaseFov(CamAreaState* state, s32 unused);
void StartMapCameraTargetHeightInterpolation(CCameraActionMap* map, f32 from, f32 to, f32 time);
void InitCameraAreaEntries(CamAreaState* state);
void SelectAreaCameraEntryByViewDirection(CCameraActionArea* area, s32 mode, f32 time, s32 slotIndex);
void SelectAreaCameraEntryWithControl(CCameraActionArea* area, s32 slotIndex, s32 entryIndex, s32 mode, s32 control, f32 time);
void SetMapCameraModeFlags(CCameraActionMap* map, u32 flags);
f32 MapAreaHeight(CCameraActionMap* map, s32 index);
void UpdateMapCameraPadControls(CCameraActionMap* map, u32 step);
void UpdateMapCameraOrbit(CCameraActionMap* map, u32 step);
void GetMapCameraEyeBehindTarget(CCameraActionMap* map, Vec& out, s32 mode);
// clamps the screen point into the per-axis limits, in cmotioncontrolcamera.cpp
Vec fn_8016AA78(Vec in, f32* limits);
CVec SpringDamp(CVec& position, Vec& target, const CVec& previous, f32 step, f32 stiffness, f32 damping, f32 restDistance);
s32 CollectCameraAreasContainingPoint(CamAreaState* state, Vec pos, s32* ids);
}

extern s32 sMapCameraCenterReset;
extern s32 sMapCameraOrbitAtRestDistance;
extern "C" CVec sMapCameraTestEye;
extern "C" CVec sMapCameraTestCenter;
extern "C" CVec lbl_8047F170;

// the map camera's spring stiffnesses, by mode
extern "C" f32 sMapCameraSpringParameters[] = { 0.5f, 2.5f, 4.5f };
// the collision modes Update hands to ResolveMapCameraEyeCollision, for the spring eye and the orbit eye
extern "C" s32 sMapCameraCollisionMode0[1];
extern "C" s32 sMapCameraCollisionMode1 = 1;

static inline void SetMapCameraAnglesFromEyeAndInterest(CCameraActionMap* map);

extern s32 lbl_805A1D14;
extern s32 lbl_805A1D18;
extern s32 lbl_805A1D1C;
extern s32 lbl_805A1D24;
extern s32 lbl_805A1D2C;
extern f32 lbl_805A1D30;
extern f32 lbl_805A1D34;
extern s32 lbl_805A1D38;
extern s32 lbl_805A1D3C;
extern s32 lbl_805A1D40;
extern s32 lbl_805A1D44;
extern f32 lbl_805A1288;
extern f32 lbl_805A1290[2];
extern const f32 lbl_802DB578[4];

// the look response curves, two hermite segments a piece; the same shape
// cmotioncontrolcamera evaluates for the pointer look, applied to the orbit input here
struct WiiCameraCurve {
	f32 mP0X; // _00
	f32 mP0Y; // _04
	f32 mP1X; // _08
	f32 mP1Y; // _0C
	f32 mT0X; // _10
	f32 mT0Y; // _14
	f32 mT1X; // _18
	f32 mT1Y; // _1C
};

struct WiiCameraPoint {
	f32 x; // _00
	f32 y; // _04
};

extern "C" WiiCameraPoint fn_8016A844(const WiiCameraCurve* curve, f32 t);

// the area index the last turn interpolation aims at, Update skips starting another for it
extern s32 lbl_805A1D10;

// the Wii build resolves the current area's action in a second, C++ step that falls back to
// the map camera while the player stands outside every area
s32 GetCameraAreaMode(CamAreaState* state);

extern s32 gScreenSafeWidth;

/**
 * @note Address: 0x80157154
 * @note Size: 0x134
 */
void CCameraActionMap::Init()
{
	mSpringDamping      = 0.5f;
	mRestDistance       = 60.0f;
	mTargetRestDistance = 60.0f;
	mFlatDistance       = 60.0f;
	_C4                 = 18.0f;
	InitCameraAreaEntries(&gCameraAreaState);

	// the turn arrows, as the area camera's
	nw4r::math::_VEC2 pos[2] = { 0 };
	pos[0].x                 = gScreenSafeWidth / 2 - 30;
	pos[1].x                 = -(gScreenSafeWidth / 2 - 30);
	f32 rot[2]               = { 0.0f, PI };
	mArrows[0].mPos.x        = pos[0].x;
	mArrows[0].mPos.y        = pos[0].y;
	mArrows[0].mPos.z        = rot[0];
	mArrows[0].mShown        = 0;
	mArrows[0]._10           = 0.0f;
	mArrows[0]._14           = 0.0f;
	mArrows[1].mPos.x        = pos[1].x;
	mArrows[1].mPos.y        = pos[1].y;
	mArrows[1].mPos.z        = rot[1];
	mArrows[1].mShown        = 0;
	mArrows[1]._10           = 0.0f;
	mArrows[1]._14           = 0.0f;
	_126C                    = 0.0f;
	_1270                    = 0.0f;
	_127C                    = 0.0f;
	_1280                    = 0.0f;
	_1274                    = 1.0f;
	_1278                    = 1.0f;
	_1284                    = 0.0f;
	_1288                    = 3.0f;
}

extern s32 gScreenSafeRight;
extern s32 gScreenSafeTop;
extern s32 gScreenSafeBottom;
extern "C" void fn_8016AA74(const f32* params);
extern "C" f32 fn_8016A8D4(f32 from, f32 to, f32 t, f32 scale);
extern "C" Vec GetCornerIconPosition(CGame* game);
// the pulse scale the arrows fade with, 1.0f; cmotioncontrolcamera.cpp's data
extern f32 lbl_805A12B0;

void CCameraActionMap::Reset()
{
	OSPanic(__FILE__, 285, "actionMap::reset\n");
	// OSPanic ends in PPCHalt, so nothing below ever runs

	mTarget = (XObj*)gGame.GetPlayer();
	if (mTarget != NULL) {
		Mtx mtx;
		PSMTXCopy(mTarget->GetWorldMtx(), mtx);
		CVec eye(0.0f, 13.0f, -mRestDistance);
		PSMTXMultVec(mtx, &eye, &eye);
		SetEye(eye);
		SetInterest(mTarget->GetCameraTarget());
	}
	mPadControlEnabled = 1;
}

/**
 * @note Address: 0x80157384
 * @note Size: 0x320
 */
void CCameraActionMap::Reset2()
{
	SetInterest(mTarget->GetCameraTarget());
	Vec eye;
	GetMapCameraEyeBehindTarget(this, eye, 0);
	SetEye(eye);
	SetMapCameraAnglesFromEyeAndInterest(this);
	if (__fpclassifyf(mAngle.x) == FP_NAN || __fpclassifyf(mAngle.y) == FP_NAN || __fpclassifyf(mAngle.z) == FP_NAN) {
		OSPanic("ccameraactionmap2.cpp", 316,
		        FormatToStaticBuffer("EYE(%f, %f, %f)ANG(%f, %f, %f)DIST(%f)CAMSTAT(%8X)CAMSTATPRE(%8X)UNDER "
		                             "OP(%d)HANG(%d(%d))TGTEYE(%f, %f, %f)TGTEYE_PRE(%f, %f, %f)INTERP CAM(0x%x)INTERP "
		                             "TURN(0x%x)POS_CAMMODE(%f, %f, %f)ANG_CAMMODE(%f, %f, %f)ANG_CAMMODE_PRE(%f, %f, %f)FIX(%d)\n",
		                             mEye.x, mEye.y, mEye.z, mAngle.x, mAngle.y, mAngle.z, mDistance, mStatus.mFlags,
		                             mPreviousStatus.mFlags, mManualInput, lbl_805A1D1C, lbl_805A1D18, mTargetEye.x, mTargetEye.y,
		                             mTargetEye.z, mPrevTargetEye.x, mPrevTargetEye.y, mPrevTargetEye.z, mEyeInterp, mTurnInterpolation,
		                             mOrbitEye.x, mOrbitEye.y, mOrbitEye.z, mOrbitAngle.x, mOrbitAngle.y, mOrbitAngle.z, mPrevOrbitAngle.x,
		                             mPrevOrbitAngle.y, mPrevOrbitAngle.z, lbl_805A1D14));
	}
}

extern "C" void SetMapCameraKeepPreviousEye(CCameraActionMap* map)
{
	map->mKeepPreviousEye = TRUE;
}

extern "C" s32 FindEnabledCameraAreaContainingPoint(CamAreaState* state, Vec pos);
extern "C" s32 SelectMapCameraArea(CCameraActionMap* map);
extern "C" void StartMapCameraAreaHeightInterpolation(CCameraActionMap* map);

extern "C" void RefreshMapCameraArea(CCameraActionMap* map)
{
	map->mPrevMapArea = map->mMapArea;
	SelectMapCameraArea(map);
	StartMapCameraAreaHeightInterpolation(map);
	map->mPrevMapArea = map->mMapArea;
}

extern "C" void StartMapCameraTargetHeightInterpolation(CCameraActionMap* map, f32 from, f32 to, f32 time)
{
	if (0.0f != time) {
		map->mTargetHeightOffsetInterp = &map->mTargetHeightOffsetEase;
		map->mTargetHeightOffsetInterp->Start(from, to, time);
	}
}

static inline s32 ClassifyAreaHeight(f32 y, f32 bottom, f32 top)
{
	if (y >= bottom && y <= top) {
		return 1;
	}
	if (y < bottom) {
		return -2;
	}
	if (y > top) {
		return -1;
	}
	return 0;
}

// the outward normal of an area side from a to b, turned about the up or the down axis; the
// GameCube inlines it into both intersection collectors
static void GetMapAreaEdgeNormal(const Vec& a, const Vec& b, Vec* out, BOOL up)
{
	Vec axis;
	if (up) {
		Vec upAxis = { 0.0f, 1.0f, 0.0f };
		axis       = upAxis;
	} else {
		Vec downAxis = { 0.0f, -1.0f, 0.0f };
		axis         = downAxis;
	}
	Vec edge;
	edge.x = a.x - b.x;
	edge.z = a.z - b.z;
	edge.y = 0.0f;
	CVec normal;
	C_VECCrossProduct(&axis, &edge, &normal);
	PSVECNormalize(&normal, &normal);
	*out = normal;
}

extern "C" s32 CollectMapCameraAreaIntersections(CCameraActionMap* map, const Vec& from, const Vec& to, CamMapEntry* out, s32 type)
{
	if (out == NULL) {
		return -1;
	}
	memset(out, 0, 32 * sizeof(CamMapEntry));
	CamAreaRecord* record;
	s32 numRecords;
	if (type == 0) {
		record     = gCameraAreaState.mAreaExRecords;
		numRecords = gCameraAreaState.mAreaExCount;
	}
	if (type == 1) {
		record     = gCameraAreaState.mAreaRecords;
		numRecords = gCameraAreaState.mAreaCount;
	}
	CamMapEntry* entry = out;
	s32 count          = 0;
	const s32(*corner)[2];
	for (s32 i = 0; i < numRecords; i++) {
		if (CamAreaInfo(record).IsCameraOff()) {
			record++;
			continue;
		}
		if (CamAreaInfo(record).UsesAreaCamera()) {
			record++;
			continue;
		}
		f32 top    = CamAreaInfo(record).GetTop();
		f32 bottom = CamAreaInfo(record).GetBottom();
		if (ClassifyAreaHeight(from.y, bottom, top) < 0 && ClassifyAreaHeight(to.y, bottom, top) < 0) {
			record++;
			continue;
		}
		corner = record->_04;
		for (s32 side = 0; side < 4; side++) {
			Vec a, b;
			a.x = (*corner)[0] / 100.0f;
			a.y = 0.0f;
			a.z = (*corner)[1] / 100.0f;
			++corner;
			if (side == 3) {
				corner = record->_04;
			}
			b.x       = (*corner)[0] / 100.0f;
			b.y       = 0.0f;
			b.z       = (*corner)[1] / 100.0f;
			Vec end   = to;
			Vec start = from;
			Vec hit;
			if (IntersectLinesXZ(&a, &b, &end, &start, &hit, 1)) {
				Vec normal;
				GetMapAreaEdgeNormal(a, b, &normal, FALSE);
				count++;
				entry->mPos         = hit;
				entry->mSide        = side;
				entry->mRecordIndex = i;
				entry->mNormal      = normal;
				entry++;
			}
		}
		record++;
	}
	return count;
}

/**
 * @note Address: 0x8015CDFC
 * @note Size: 0x3E4
 */
s32 CCameraActionMap::CalcIntersectVec(const Vec& from, const Vec& to, CamMapEntry* out, s32 type)
{
	if (out == NULL) {
		return -1;
	}
	memset(out, 0, 32 * sizeof(CamMapEntry));
	CamAreaRecord* record;
	s32 numRecords;
	if (type == 0) {
		record     = gCameraAreaState.mAreaExRecords;
		numRecords = gCameraAreaState.mAreaExCount;
	}
	if (type == 1) {
		record     = gCameraAreaState.mAreaRecords;
		numRecords = gCameraAreaState.mAreaCount;
	}
	CamMapEntry* entry = out;
	s32 count          = 0;
	s32* corner;
	for (s32 i = 0; i < numRecords; i++) {
		u32 disabled = CamAreaInfo(record).IsDisabled();
		if (disabled != 0) {
			record++;
			continue;
		}
		f32 top    = CamAreaInfo(record).GetTop();
		f32 bottom = CamAreaInfo(record).GetBottom();
		if (ClassifyAreaHeight(from.y, bottom, top) < 0 && ClassifyAreaHeight(to.y, bottom, top) < 0) {
			record++;
			continue;
		}
		// while a turn runs, skip the area StartMapCameraTurnInterpolation stored in lbl_805A1D10
		if (mTurnInterpolation != NULL && i == lbl_805A1D10) {
			record++;
			continue;
		}
		corner = record->_04[0];
		for (s32 side = 0; side < 4; side++) {
			Vec a, b;
			a.x = corner[0] / 100.0f;
			a.y = 0.0f;
			a.z = corner[1] / 100.0f;
			corner += 2;
			if (side == 3) {
				corner = record->_04[0];
			}
			b.x       = corner[0] / 100.0f;
			b.y       = 0.0f;
			b.z       = corner[1] / 100.0f;
			Vec end   = to;
			Vec start = from;
			Vec hit;
			s32 intersects = IntersectLinesXZ(&a, &b, &end, &start, &hit, 1);
			if (intersects && (__fpclassifyf(hit.x) == FP_NAN || __fpclassifyf(hit.y) == FP_NAN || __fpclassifyf(hit.z) == FP_NAN)) {
				OSPanic("ccameraactionmap2.cpp", 2856,
				        FormatToStaticBuffer(
				            "CCameraActionMap ... GetCrossPoint\np1(%f, %f, %f) p2(%f, %f, %f)\nvs(%f, %f, %f) ve(%f, %f, %f)\n(1)\n", a.x,
				            a.y, a.z, b.x, b.y, b.z, end.x, end.y, end.z, start.x, start.y, start.z));
			}
			if (intersects) {
				Vec normal;
				GetMapAreaEdgeNormal(a, b, &normal, FALSE);
				count++;
				entry->mPos         = hit;
				entry->mSide        = side;
				entry->mRecordIndex = i;
				entry->mNormal      = normal;
				entry++;
			}
		}
		record++;
	}
	return count;
}

extern "C" s32 CountMapCameraAdjacentSideIntersections(CCameraActionMap* map, CamMapEntry* entry)
{
	// the two neighbours of each of the 4 sides, wraps around
	s32 sides[4][2] = {
		{ 3, 1 },
		{ 0, 2 },
		{ 1, 3 },
		{ 2, 0 },
	};
	s32 num   = map->mIntersectionCount;
	s32 group = entry->mRecordIndex;
	s32 side  = entry->mSide;
	s32 count = 0;
	if (num > 0) {
		for (s32 i = 0; i < num; i++) {
			if (map->mIntersectionEntries[i].mRecordIndex == group) {
				for (s32 j = 0; j < 2; j++) {
					if (map->mIntersectionEntries[i].mSide == sides[side][j]) {
						count++;
					}
				}
			}
		}
	}
	return count;
}

/**
 * @note Address: 0x8015D6B0
 * @note Size: 0x278
 */
extern "C" BOOL SetMapCameraSideEndpoints(CCameraActionMap* map)
{
	CamMapEntry* entry = map->mNearestEyeEntry;
	if (entry != NULL) {
		s32 group          = entry->mRecordIndex;
		s32 side           = entry->mSide;
		CVec dir           = entry->mNormal;
		CamAreaRecord* rec = gCameraAreaState.mAreaExRecords;
		rec += group;
		CVec toPlayer = *gGame.GetPlayer()->GetPos();
		toPlayer.y    = 0.0f;
		Vec eye       = map->mEye;
		eye.y         = 0.0f;
		toPlayer -= eye;
		PSVECNormalize(&toPlayer, &toPlayer);
		PSVECCrossProduct(&dir, &toPlayer, &dir);
		CVec cross = dir;

		s32* corner = &rec->_04[0][0];
		corner += side;
		corner += side;
		f32 x0 = corner[0];
		x0 /= 100.0f;
		f32 z0 = corner[1];
		z0 /= 100.0f;
		Vec point0;
		point0.x = x0;
		point0.y = 0.0f;
		point0.z = z0;
		if (side == 3) {
			corner = rec->_04[0];
		} else {
			corner += 2;
		}
		f32 x1 = corner[0];
		x1 /= 100.0f;
		f32 z1 = corner[1];
		z1 /= 100.0f;
		Vec point1;
		point1.x = x1;
		point1.y = 0.0f;
		point1.z = z1;

		if (cross.y > 0.0f) {
			map->mSideStart = point0;
			map->mSideEnd   = point1;
			map->mSideSign  = 1;
		}
		if (cross.y < 0.0f) {
			map->mSideStart = point1;
			map->mSideEnd   = point0;
			map->mSideSign  = -1;
		}
		return TRUE;
	}
	return FALSE;
}

// the game was built against an nw4r whose FSqrt still carried the domain warning
// (arithmetic.h:627); the shared header has the final, warning-free version
static inline f32 FSqrt(f32 x)
{
	if (!(x >= 0.0f)) {
		nw4r::db::Warning("arithmetic.h", 627, "FSqrt: Input is out of the domain.");
	}
	return x <= 0.0f ? 0.0f : x * nw4r::math::FrSqrt(x);
}

static inline BOOL CameraPolygonContains(const Vec& pos, CamCollisionPolygon& polygon)
{
	return IsPointInsideQuadXZ((Vec*)&pos, &polygon.point[0], &polygon.point[1], &polygon.point[2], &polygon.point[3]);
}

static inline BOOL CameraEdgeIntersect(const Vec& from, const Vec& to, Vec& a, Vec& b, Vec& hit, s32 segment)
{
	return IntersectLinesXZ(&from, &to, &a, &b, &hit, segment);
}

extern "C" BOOL TrySetUnobstructedMapCameraYaw(CCameraActionMap* map, f32 yaw);
extern "C" Vec sPreviousMapOrbitAngle;
extern "C" CamCollisionPolygon gCameraCollisionPolygons[256];

// the last turn direction, zeroed when the collision cancels every interpolation
static f32 lbl_805A1D28;

// reseats the orbit on the eye and drops every eye mode
static inline void ResetMapCameraInterpolation(CCameraActionMap* map)
{
	map->mOrbitEye       = map->mEye;
	map->mPrevOrbitEye   = map->mEye;
	map->mOrbitAngle     = map->mAngle;
	map->mPrevOrbitAngle = map->mAngle;
	map->mEyeInterp      = NULL;
	lbl_805A1D28         = 0.0f;
	map->mOrbitAngle.y   = sPreviousMapOrbitAngle.y;
	map->mStatus.mFlags &= ~0x3E;
}

// NONMATCHING: the logic matches throughout; retail inlines GetOutwardPerpendicularToEdge (0x800D57E4,
// defined in xobj_chr.cpp's unit), which the source calls; what remains is scheduling and register
// allocation (retail's frame is 0x50 larger and assigns the loop variables to different registers) (89,1 %)
/**
 * @note Address: 0x80159D70
 * @note Size: 0x308C
 */
inline void GetOutwardPerpendicularToEdge(Vec* a, Vec* b, Vec* c, Vec* out)
{
	Vec bc;
	Vec ba;
	Vec n;
	PSVECSubtract(b, c, &bc);
	PSVECSubtract(b, a, &ba);
	PSVECCrossProduct(&ba, &bc, &n);
	PSVECCrossProduct(&bc, &n, out);
}

extern "C" BOOL ResolveMapCameraEyeCollision(CCameraActionMap* map, CVec& eye, const CVec& previous, const CVec& interest,
                                             const CVec& previousInterest, const s32* mode)
{
	CVec target((const Vec&)interest);
	CVec old((const Vec&)previous);
	CVec candidate((const Vec&)eye);
	if (target == (Vec)old) {
		return FALSE;
	}

	CamAreaRecord* records        = gCameraAreaState.mAreaExRecords;
	s32 count                     = gCameraAreaState.mAreaExCount;
	CamCollisionPolygon* polygons = gCameraCollisionPolygons;
	BOOL changed                  = FALSE;
	BOOL targetInside             = FALSE;
	BOOL previousInside           = FALSE;
	BOOL candidateInside          = FALSE;
	for (s32 i = 0; i < count; i++) {
		CamAreaRecord* record        = &records[i];
		CamCollisionPolygon& polygon = polygons[i];
		CamAreaRecord* area          = CamAreaInfo(record).GetRecord();
		if (area->mFlags & 0x80) { // an event switched this area off
			continue;
		}
		f32 top      = area->GetTop();
		f32 bottom   = area->GetBottom();
		bool outside = ClassifyAreaHeight(target.y, bottom, top) < 0 && ClassifyAreaHeight(eye.y, bottom, top) < 0;
		if (outside) {
			continue;
		}
		polygon.point[0].x = polygon.point[4].x = record->_04[0][0] / 100.0f;
		polygon.point[0].z = polygon.point[4].z = record->_04[0][1] / 100.0f;
		polygon.point[0].y = polygon.point[4].y = 0.0f;
		polygon.point[1].x                      = record->_04[1][0] / 100.0f;
		polygon.point[1].z                      = record->_04[1][1] / 100.0f;
		polygon.point[1].y                      = 0.0f;
		polygon.point[2].x                      = record->_04[2][0] / 100.0f;
		polygon.point[2].z                      = record->_04[2][1] / 100.0f;
		polygon.point[2].y                      = 0.0f;
		polygon.point[3].x                      = record->_04[3][0] / 100.0f;
		polygon.point[3].z                      = record->_04[3][1] / 100.0f;
		polygon.point[3].y                      = 0.0f;
		targetInside |= CameraPolygonContains(target, polygon);
		previousInside |= CameraPolygonContains(old, polygon);
	}

	if (previousInside) {
		// the old eye sat inside an area: push the candidate back out to the nearest side
		f32 closest = NAN;
		CVec best;
		s32 bestSide;
		BOOL found = FALSE;
		for (s32 i = 0; i < count; i++) {
			CamCollisionPolygon& polygon = polygons[i];
			CamAreaRecord* area          = CamAreaInfo(&records[i]).GetRecord();
			if (area->mFlags & 0x80) {
				continue;
			}
			f32 top      = area->GetTop();
			f32 bottom   = area->GetBottom();
			bool outside = ClassifyAreaHeight(target.y, bottom, top) < 0 && ClassifyAreaHeight(eye.y, bottom, top) < 0;
			if (outside) {
				continue;
			}
			CVec hits[4];
			if (CameraPolygonContains(candidate, polygon)) {
				CVec normal, hit;
				for (s32 side = 0; side < 4; side++) {
					Vec& a = polygon.point[side];
					Vec& b = polygon.point[side + 1];
					GetOutwardPerpendicularToEdge(&candidate, &a, &b, &normal);
					normal.y = 0.0f;
					PSVECNormalize(&normal, &normal);
					CameraEdgeIntersect(candidate, candidate + (Vec)normal, a, b, hit, 0);
					hits[side].Set(hit);
					f32 distance = PSVECDistance(&candidate, &hit);
					if (__fpclassifyf(closest) == FP_NAN || closest > distance) {
						closest  = distance;
						bestSide = side;
						best.Set(hit + (Vec)(normal * 2.0f));
						found = TRUE;
					}
				}
				// some rooms pin the push-out to a fixed side
				if (found && gGame.mStage == 6) {
					if (i == 4) {
						if (bestSide == 1 && interest.x < -4.15f) {
							hit.Set(hits[3]);
							normal.Set(-1.0f, 0.0f, 0.0f);
							closest = PSVECDistance(&candidate, &hit);
							best.Set(hit + (Vec)(normal * 2.0f));
						} else if (bestSide == 3 && interest.x > -4.15f) {
							hit.Set(hits[1]);
							normal.Set(1.0f, 0.0f, 0.0f);
							closest = PSVECDistance(&candidate, &hit);
							best.Set(hit + (Vec)(normal * 2.0f));
						}
					} else if (i == 0x18) {
						if (interest.x < 5.85f && interest.x > -14.15f) {
							if (bestSide == 1 && candidate.x < -4.15f) {
								hit.Set(hits[3]);
								normal.Set(-1.0f, 0.0f, 0.0f);
								closest = PSVECDistance(&candidate, &hit);
								best.Set(hit + (Vec)(normal * 2.0f));
							} else if (bestSide == 3 && candidate.x > -4.15f) {
								hit.Set(hits[1]);
								normal.Set(1.0f, 0.0f, 0.0f);
								closest = PSVECDistance(&candidate, &hit);
								best.Set(hit + (Vec)(normal * 2.0f));
							}
						} else {
							if (bestSide == 1 && interest.x < -4.15f) {
								hit.Set(hits[3]);
								normal.Set(-1.0f, 0.0f, 0.0f);
								closest = PSVECDistance(&candidate, &hit);
								best.Set(hit + (Vec)(normal * 2.0f));
							} else if (bestSide == 3 && interest.x > -4.15f) {
								hit.Set(hits[1]);
								normal.Set(1.0f, 0.0f, 0.0f);
								closest = PSVECDistance(&candidate, &hit);
								best.Set(hit + (Vec)(normal * 2.0f));
							}
						}
					}
				}
				if (found && gGame.mStage == 6 && i == 0x12 && (bestSide == 1 || bestSide == 3)) {
					hit.Set(hits[2]);
					normal.Set(0.0f, 0.0f, 1.0f);
					closest = PSVECDistance(&candidate, &hit);
					best.Set(hit + (Vec)(normal * 2.0f));
				}
				if (found && gGame.mStage == 1) {
					if (i == 7 && (bestSide == 1 || bestSide == 3)) {
						hit.Set(hits[2]);
						normal.Set(0.0f, 0.0f, 1.0f);
						closest = PSVECDistance(&candidate, &hit);
						best.Set(hit + (Vec)(normal * 2.0f));
					}
					if (i == 0xA && bestSide == 0) {
						hit.Set(hits[1]);
						normal.Set(0.0f, 0.0f, 1.0f);
						closest = PSVECDistance(&candidate, &hit);
						best.Set(hit + (Vec)(normal * 2.0f));
					}
					if (i == 9) {
						hit.Set(hits[3]);
						normal.Set(0.0f, 0.0f, 1.0f);
						closest = PSVECDistance(&candidate, &hit);
						best.Set(hit + (Vec)(normal * 2.0f));
					}
					if (i == 8) {
						if (interest.x < 212.15f) {
							hit.Set(hits[3]);
							normal.Set(-1.0f, 0.0f, 0.0f);
							closest = PSVECDistance(&candidate, &hit);
							best.Set(hit + (Vec)(normal * 2.0f));
						} else if (interest.x > 212.15f) {
							hit.Set(hits[1]);
							normal.Set(1.0f, 0.0f, 0.0f);
							closest = PSVECDistance(&candidate, &hit);
							best.Set(hit + (Vec)(normal * 2.0f));
						}
					}
				}
			}
		}
		if (found) {
			// a second pass from the pushed-out candidate, then a third if that still sits inside
			candidate.x  = best.x;
			candidate.z  = best.z;
			f32 closest2 = NAN;
			s32 bestSide2;
			BOOL found2 = FALSE;
			for (s32 i = 0; i < count; i++) {
				CamCollisionPolygon& polygon = polygons[i];
				CamAreaRecord* area          = CamAreaInfo(&records[i]).GetRecord();
				if (area->mFlags & 0x80) {
					continue;
				}
				f32 top      = area->GetTop();
				f32 bottom   = area->GetBottom();
				bool outside = ClassifyAreaHeight(target.y, bottom, top) < 0 && ClassifyAreaHeight(eye.y, bottom, top) < 0;
				if (outside) {
					continue;
				}
				CVec hits[4];
				if (CameraPolygonContains(candidate, polygon)) {
					CVec normal, hit;
					for (s32 side = 0; side < 4; side++) {
						Vec& a = polygon.point[side];
						Vec& b = polygon.point[side + 1];
						GetOutwardPerpendicularToEdge(&candidate, &a, &b, &normal);
						normal.y = 0.0f;
						PSVECNormalize(&normal, &normal);
						CameraEdgeIntersect(candidate, candidate + (Vec)normal, a, b, hit, 0);
						hits[side].Set(hit);
						f32 distance = PSVECDistance(&candidate, &hit);
						if (__fpclassifyf(closest2) == FP_NAN || closest2 > distance) {
							closest2  = distance;
							bestSide2 = side;
							best.Set(hit + (Vec)(normal * 2.0f));
							found2 = TRUE;
						}
					}
					if (found2 && gGame.mStage == 6) {
						if (i == 4) {
							if (bestSide2 == 1 && interest.x < -4.15f) {
								hit.Set(hits[3]);
								normal.Set(-1.0f, 0.0f, 0.0f);
								closest2 = PSVECDistance(&candidate, &hit);
								best.Set(hit + (Vec)(normal * 2.0f));
							} else if (bestSide2 == 3 && interest.x > -4.15f) {
								hit.Set(hits[1]);
								normal.Set(1.0f, 0.0f, 0.0f);
								closest2 = PSVECDistance(&candidate, &hit);
								best.Set(hit + (Vec)(normal * 2.0f));
							}
						} else if (i == 0x18) {
							if (interest.x < 5.85f && interest.x > -14.15f) {
								if (bestSide2 == 1 && candidate.x < -4.15f) {
									hit.Set(hits[3]);
									normal.Set(-1.0f, 0.0f, 0.0f);
									closest2 = PSVECDistance(&candidate, &hit);
									best.Set(hit + (Vec)(normal * 2.0f));
								} else if (bestSide2 == 3 && candidate.x > -4.15f) {
									hit.Set(hits[1]);
									normal.Set(1.0f, 0.0f, 0.0f);
									closest2 = PSVECDistance(&candidate, &hit);
									best.Set(hit + (Vec)(normal * 2.0f));
								}
							} else {
								if (bestSide2 == 1 && interest.x < -4.15f) {
									hit.Set(hits[3]);
									normal.Set(-1.0f, 0.0f, 0.0f);
									closest2 = PSVECDistance(&candidate, &hit);
									best.Set(hit + (Vec)(normal * 2.0f));
								} else if (bestSide2 == 3 && interest.x > -4.15f) {
									hit.Set(hits[1]);
									normal.Set(1.0f, 0.0f, 0.0f);
									closest2 = PSVECDistance(&candidate, &hit);
									best.Set(hit + (Vec)(normal * 2.0f));
								}
							}
						}
					}
					if (found2 && gGame.mStage == 6 && i == 0x12 && (bestSide2 == 1 || bestSide2 == 3)) {
						hit.Set(hits[2]);
						normal.Set(0.0f, 0.0f, 1.0f);
						closest2 = PSVECDistance(&candidate, &hit);
						best.Set(hit + (Vec)(normal * 2.0f));
					}
					if (found2 && gGame.mStage == 1) {
						if (i == 7 && (bestSide2 == 1 || bestSide2 == 3)) {
							hit.Set(hits[2]);
							normal.Set(0.0f, 0.0f, 1.0f);
							closest2 = PSVECDistance(&candidate, &hit);
							best.Set(hit + (Vec)(normal * 2.0f));
						}
						if (i == 0xA && bestSide2 == 0) {
							hit.Set(hits[1]);
							normal.Set(0.0f, 0.0f, 1.0f);
							closest2 = PSVECDistance(&candidate, &hit);
							best.Set(hit + (Vec)(normal * 2.0f));
						}
						if (i == 9) {
							hit.Set(hits[3]);
							normal.Set(0.0f, 0.0f, 1.0f);
							closest2 = PSVECDistance(&candidate, &hit);
							best.Set(hit + (Vec)(normal * 2.0f));
						}
						if (i == 8) {
							if (interest.x < 212.15f) {
								hit.Set(hits[3]);
								normal.Set(-1.0f, 0.0f, 0.0f);
								closest2 = PSVECDistance(&candidate, &hit);
								best.Set(hit + (Vec)(normal * 2.0f));
							} else if (interest.x > 212.15f) {
								hit.Set(hits[1]);
								normal.Set(1.0f, 0.0f, 0.0f);
								closest2 = PSVECDistance(&candidate, &hit);
								best.Set(hit + (Vec)(normal * 2.0f));
							}
						}
					}
				}
			}
			if (__fpclassifyf(closest2) != FP_NAN) {
				closest = closest2;
			}
			if (found2) {
				candidate.x  = best.x;
				candidate.z  = best.z;
				f32 closest3 = NAN;
				s32 bestSide3;
				BOOL found3 = FALSE;
				for (s32 i = 0; i < count; i++) {
					CamCollisionPolygon& polygon = polygons[i];
					CamAreaRecord* area          = CamAreaInfo(&records[i]).GetRecord();
					if (area->mFlags & 0x80) {
						continue;
					}
					f32 top      = area->GetTop();
					f32 bottom   = area->GetBottom();
					bool outside = ClassifyAreaHeight(target.y, bottom, top) < 0 && ClassifyAreaHeight(eye.y, bottom, top) < 0;
					if (outside) {
						continue;
					}
					CVec hits[4];
					if (CameraPolygonContains(candidate, polygon)) {
						CVec normal, hit;
						for (s32 side = 0; side < 4; side++) {
							Vec& a = polygon.point[side];
							Vec& b = polygon.point[side + 1];
							GetOutwardPerpendicularToEdge(&candidate, &a, &b, &normal);
							normal.y = 0.0f;
							PSVECNormalize(&normal, &normal);
							CameraEdgeIntersect(candidate, candidate + (Vec)normal, a, b, hit, 0);
							hits[side].Set(hit);
							f32 distance = PSVECDistance(&candidate, &hit);
							if (__fpclassifyf(closest3) == FP_NAN || closest3 > distance) {
								closest3  = distance;
								bestSide3 = side;
								best.Set(hit + (Vec)(normal * 2.0f));
								found3 = TRUE;
							}
						}
						if (found3 && gGame.mStage == 6 && i == 0x12 && (bestSide3 == 1 || bestSide3 == 3)) {
							hit.Set(hits[2]);
							normal.Set(0.0f, 0.0f, 1.0f);
							closest3 = PSVECDistance(&candidate, &hit);
							best.Set(hit + (Vec)(normal * 2.0f));
						}
						if (found3 && gGame.mStage == 1) {
							if (i == 7 && (bestSide3 == 1 || bestSide3 == 3)) {
								hit.Set(hits[2]);
								normal.Set(0.0f, 0.0f, 1.0f);
								closest3 = PSVECDistance(&candidate, &hit);
								best.Set(hit + (Vec)(normal * 2.0f));
							}
							if (i == 0xA && bestSide3 == 0) {
								hit.Set(hits[1]);
								normal.Set(0.0f, 0.0f, 1.0f);
								closest3 = PSVECDistance(&candidate, &hit);
								best.Set(hit + (Vec)(normal * 2.0f));
							}
							if (i == 9) {
								hit.Set(hits[3]);
								normal.Set(0.0f, 0.0f, 1.0f);
								closest3 = PSVECDistance(&candidate, &hit);
								best.Set(hit + (Vec)(normal * 2.0f));
							}
							if (i == 8) {
								if (interest.x < 212.15f) {
									hit.Set(hits[3]);
									normal.Set(-1.0f, 0.0f, 0.0f);
									closest3 = PSVECDistance(&candidate, &hit);
									best.Set(hit + (Vec)(normal * 2.0f));
								} else if (interest.x > 212.15f) {
									hit.Set(hits[1]);
									normal.Set(1.0f, 0.0f, 0.0f);
									closest3 = PSVECDistance(&candidate, &hit);
									best.Set(hit + (Vec)(normal * 2.0f));
								}
							}
						}
					}
				}
				if (__fpclassifyf(closest3) != FP_NAN) {
					closest = closest3;
				}
			}
		}
		if (__fpclassifyf(closest) != FP_NAN) {
			candidate.Set(best);
			previousInside  = FALSE;
			candidateInside = FALSE;
			for (s32 i = 0; i < count; i++) {
				CamCollisionPolygon& polygon = polygons[i];
				CamAreaRecord* area          = CamAreaInfo(&records[i]).GetRecord();
				if (area->mFlags & 0x80) {
					continue;
				}
				f32 top      = area->GetTop();
				f32 bottom   = area->GetBottom();
				bool outside = ClassifyAreaHeight(target.y, bottom, top) < 0 && ClassifyAreaHeight(eye.y, bottom, top) < 0;
				if (outside) {
					continue;
				}
				if (CameraPolygonContains(candidate, polygon)) {
					candidateInside = TRUE;
					break;
				}
			}
			if (candidateInside) {
				// still inside: clamp the candidate to the side the eye crossed last
				for (s32 i = 0; i < count; i++) {
					CamCollisionPolygon& polygon = polygons[i];
					CamAreaRecord* area          = CamAreaInfo(&records[i]).GetRecord();
					if (area->mFlags & 0x80) {
						continue;
					}
					f32 top      = area->GetTop();
					f32 bottom   = area->GetBottom();
					bool outside = ClassifyAreaHeight(target.y, bottom, top) < 0 && ClassifyAreaHeight(eye.y, bottom, top) < 0;
					if (outside) {
						continue;
					}
					CVec from = targetInside ? (CVec&)*map->mTarget->GetPos() : target;
					CVec normal, hit;
					for (s32 side = 0; side < 4; side++) {
						Vec& a = polygon.point[side];
						Vec& b = polygon.point[side + 1];
						if (CameraEdgeIntersect(from, old, a, b, hit, 1)) {
							GetOutwardPerpendicularToEdge(&old, &a, &b, &normal);
							normal.y = 0.0f;
							PSVECNormalize(&normal, &normal);
							CameraEdgeIntersect(old, old + (Vec)normal, a, b, hit, 0);
							f32 distance = PSVECDistance(&target, &hit);
							if (__fpclassifyf(closest) == FP_NAN || closest > distance) {
								closest = distance;
								best.Set(hit + (Vec)(normal * 2.0f));
							}
						}
					}
				}
				if (__fpclassifyf(closest) != FP_NAN) {
					candidate.Set(best);
					previousInside = FALSE;
				}
			}
		}
	} else {
		// the old eye sat outside every area: test the way from the target's back to the candidate
		f32 yaw = map->mTarget->mMoveRotate.y;
		Vec behind;
		f32 back = (f32)sin(yaw);
		behind.x = map->mTarget->GetPos()->x - 2.0f * back;
		behind.y = target.y;
		back     = (f32)cos(yaw);
		behind.z = map->mTarget->GetPos()->z - 2.0f * back;
		for (s32 i = 0; i < count; i++) {
			CamCollisionPolygon& polygon = polygons[i];
			CamAreaRecord* area          = CamAreaInfo(&records[i]).GetRecord();
			if (area->mFlags & 0x80) {
				continue;
			}
			f32 top      = area->GetTop();
			f32 bottom   = area->GetBottom();
			bool outside = ClassifyAreaHeight(target.y, bottom, top) < 0 && ClassifyAreaHeight(eye.y, bottom, top) < 0;
			if (outside) {
				continue;
			}
			CVec normal, firstHit, push;
			BOOL inside = CameraPolygonContains(target, polygon);
			if (!inside) {
				inside = CameraPolygonContains(behind, polygon);
			}
			s32 crossedSide = -1;
			for (s32 side = 0; side < 4; side++) {
				if (ResolveCircleWallCollisionXZ(&old, &candidate, &polygon.point[side], &polygon.point[side + 1], &firstHit, NULL, NULL, 0,
				                                 2.0f)) {
					crossedSide = side;
					break;
				}
			}
			candidateInside |= CameraPolygonContains(candidate, polygon);
			if (!inside) {
				if (crossedSide >= 0) {
					CVec nearest, hit;
					f32 closest     = NAN;
					s32 nearestSide = -1;
					for (s32 side = 0; side < 4; side++) {
						if (ResolveCircleWallCollisionXZ(&target, &candidate, &polygon.point[side], &polygon.point[side + 1], &hit, NULL,
						                                 NULL, 0, 2.0f)) {
							f32 distance = PSVECDistance(&hit, &target);
							if (__fpclassifyf(closest) == FP_NAN || closest > distance) {
								closest = distance;
								nearest.Set(hit);
								nearestSide = side;
							}
						}
					}
					if (nearestSide >= 0) {
						if (*mode == 1) {
							candidate.Set(nearest);
							candidate.y        = eye.y;
							changed            = TRUE;
							CVec* pos          = map->mTarget->GetPos();
							f32 dz             = target.z - pos->z;
							f32 dx             = target.x - pos->x;
							f32 playerDistance = FSqrt(dz * dz + dx * dx);
							dz                 = target.z - candidate.z;
							dx                 = target.x - candidate.x;
							if (FSqrt(dz * dz + dx * dx) <= playerDistance) {
								ResetMapCameraInterpolation(map);
								TrySetUnobstructedMapCameraYaw(map, (f32)atan2(candidate.x - old.x, candidate.z - old.z));
							}
						} else {
							if (nearestSide != crossedSide && labs(nearestSide - crossedSide) % 2 != 0) {
								candidate.Set(firstHit);
								candidate.y = eye.y;
							} else {
								Vec a, b; // retail works these out and drops them
								GetOutwardPerpendicularToEdge(&target, &polygon.point[nearestSide], &polygon.point[nearestSide + 1], &a);
								GetOutwardPerpendicularToEdge(&nearest, &polygon.point[nearestSide], &polygon.point[nearestSide + 1], &b);
								candidate.Set(nearest);
								candidate.y = eye.y;
							}
						}
					}
				}
			} else if (crossedSide >= 0) {
				Vec& a = polygon.point[crossedSide];
				Vec& b = polygon.point[crossedSide + 1];
				if (*mode == 1) {
					CVec hit;
					GetOutwardPerpendicularToEdge(&old, &a, &b, &normal);
					if (CameraEdgeIntersect(old, old + (Vec)normal, a, b, hit, 1)) {
						normal.y = 0.0f;
						PSVECNormalize(&normal, &normal);
						candidate.Set(hit - (Vec)(normal * 2.0f));
						candidate.y = eye.y;
					} else {
						candidate.Set(old);
					}
					map->_11FC.Reset(map->mAngle);
					ResetMapCameraInterpolation(map);
					changed = TRUE;
				} else {
					CVec direction, hit;
					GetOutwardPerpendicularToEdge(&candidate, &a, &b, &direction);
					if (CameraEdgeIntersect(candidate, candidate + (Vec)direction, a, b, hit, 1)) {
						direction.y = 0.0f;
						PSVECNormalize(&direction, &direction);
						candidate.Set(hit - (Vec)(direction * 2.0f));
						candidate.y = eye.y;
					} else {
						candidate.Set(old);
					}
				}
			}
		}
	}
	if (!previousInside) {
		s32 insideCount = 0;
		for (s32 i = 0; i < count; i++) {
			CamAreaRecord* area = CamAreaInfo(&records[i]).GetRecord();
			if (area->mFlags & 0x80) {
				continue;
			}
			f32 top      = area->GetTop();
			f32 bottom   = area->GetBottom();
			bool outside = ClassifyAreaHeight(target.y, bottom, top) < 0 && ClassifyAreaHeight(eye.y, bottom, top) < 0;
			if (outside) {
				continue;
			}
			if (CameraPolygonContains(candidate, polygons[i])) {
				insideCount++;
			}
		}
		if (insideCount > 0) {
			candidate.Set(old);
			if (*mode == 1) {
				ResetMapCameraInterpolation(map);
				changed = TRUE;
			}
		}
	}
	eye = candidate;
	return changed;
}

// NONMATCHING: retail reads a debug flag (sbss 0x805A1D08) after storing the speed and reloads it, and keeps
// from and to in each other's registers (88.9 %)
extern "C" BOOL TrySetMapCameraTargetYaw(CCameraActionMap* map, f32 yaw, f32 time, f32 unused)
{
	f32 oldYaw   = map->mTargetYaw;
	f32 oldSpeed = map->mTurnSpeed;

	map->mTargetYaw = WrapAngle(yaw);
	f32 from        = WrapAngle(map->mAngle.y);
	f32 to          = WrapAngle(yaw);

	f32 diff = __fabsf(to - from);
	if (diff > 0.0f) {
		if (diff >= TWO_PI) {
			diff -= TWO_PI;
		}
		if (diff > PI) {
			if (to < from) {
				to += TWO_PI;
			} else {
				to -= TWO_PI;
			}
		}
	}

	map->mTurnSpeed = __fabsf(to - from) / time;
	if (map->mTurnSpeed < (2.0f * DEG2RAD)) {
		map->mTargetYaw = oldYaw;
		map->mTurnSpeed = oldSpeed;
		return FALSE;
	}
	return TRUE;
}

extern "C" s32 FindNearestMapCameraEntry(CCameraActionMap* map, const Vec& pos)
{
	if (map->mIntersectionCount > 1) {
		CVec p(pos.x, pos.y, pos.z);
		s32 nearest = -1;
		f32 minDist = 10000.0f;
		for (s32 i = 0; i < map->mIntersectionCount; i++) {
			f32 dist = PSVECDistance(&p, &map->mIntersectionEntries[i].mPos);
			if (dist < minDist) {
				minDist = dist;
				nearest = i;
			}
		}
		return nearest;
	}
	return -1;
}

// NONMATCHING: map and the eye's area sit in other saved registers (96.9 %)
extern "C" s32 SelectMapCameraArea(CCameraActionMap* map)
{
	CVec target = *map->mTarget->GetPos();
	CVec eye    = map->mEye;
	eye.y       = target.y;

	map->mAreaIntersectionCount = CollectMapCameraAreaIntersections(map, eye, target, map->mAreaIntersections, 1);

	s32 eyeArea = FindEnabledCameraAreaContainingPoint(&gCameraAreaState, eye);
	if (CamAreaInfo(&gCameraAreaState.mAreaRecords[eyeArea]).UsesAreaCamera()) {
		eyeArea = -1;
	}

	s32 targetArea = FindEnabledCameraAreaContainingPoint(&gCameraAreaState, target);
	if (CamAreaInfo(&gCameraAreaState.mAreaRecords[targetArea]).UsesAreaCamera()) {
		targetArea = -1;
	}

	s32 crossedArea = map->mAreaIntersectionCount ? map->mAreaIntersections[0].mRecordIndex : -1;

	if (eyeArea >= 0) {
		map->mMapArea = eyeArea;
	} else if (targetArea >= 0) {
		map->mMapArea = targetArea;
	} else if (crossedArea >= 0) {
		map->mMapArea = crossedArea;
	} else {
		map->mMapArea = -1;
	}
	return map->mMapArea;
}

/**
 * @note Address: 0x8015FB20
 * @note Size: 0x1D4
 */
extern "C" void StartMapCameraTurnInterpolation(CCameraActionMap* map, s32 unused)
{
	CVec to(map->mSideStart);
	CVec eyeNormal(map->mNearestEyeEntry->mNormal);
	CVec targetNormal(map->mNearestTargetEntry->mNormal);
	CVec move = eyeNormal * 20.0f;
	move += targetNormal * 5.0f;
	to += move;

	Vec from = map->mEye;
	from.y   = 0.0f;
	if (map->mTurnInterpolation == NULL) {
		map->mTurnInterpolation = &map->mTurnEaseInOut;
		map->mTurnInterpolation->Start(from, to, 0.7f);
		map->_DB0 = 0;
		gGame.GetPlayer()->ResetCameraValue();
		lbl_805A1D10 = map->mNearestTargetEntry->mRecordIndex;
		SetMapCameraModeFlags(map, 0x20004);
	}
}

extern "C" f32 GetMapCameraHorizontalTargetDistance(CCameraActionMap* map)
{
	CVec pos = *map->mTarget->GetPos();
	f32 y    = map->mTarget->GetCameraTarget().y;
	pos.y    = y;
	CVec eye = map->mEye;
	eye.y    = y;
	return PSVECDistance(&pos, &eye);
}

inline void SetFovStep(CCameraActionMap* map, s32 zoomLevel, f32 time)
{
	f32 fov = gCameraAreaState.mBaseFov;
	switch (zoomLevel) {
	case 0:
	default:
		map->mFovEaseOutDivide.Start(map->mFov, 0.8f * fov, time);
		break;
	case 1:
		map->mFovEaseOutDivide.Start(map->mFov, fov, time);
		break;
	case 2:
		map->mFovEaseOutDivide.Start(map->mFov, 1.5f * fov, time);
		break;
	case 3:
		map->mFovEaseOutDivide.Start(map->mFov, 2.0f * fov, time);
		break;
	}
	gCameraAreaState.mZoomLevel     = zoomLevel;
	gCameraAreaState.mPrevZoomLevel = zoomLevel;
}

extern "C" void SetMapCameraFovStep(CCameraActionMap* map, s32 zoomLevel, f32 time)
{
	SetFovStep(map, zoomLevel, time);
}

/**
 * @note Address: 0x8015F8A0
 * @note Size: 0x29C
 */
extern "C" void StartMapCameraAreaHeightInterpolation(CCameraActionMap* map)
{
	CamAreaInfo area = CamAreaInfo(&gCameraAreaState.mAreaRecords[map->mMapArea]);
	if (map->mMapArea >= 0) {
		// flag 1: the area's height is absolute, otherwise it is above the player's camera target
		if (!(area.mRecord->mFlags & 1)) {
			map->mHeightMode = 0;
		} else {
			map->mHeightMode = 1;
		}
		// flag 2: keep the area's height once no area is picked (mMapArea -1), otherwise fall back to 13
		if (!(area.mRecord->mFlags & 2)) {
			map->mUseAreaHeight = 0;
		} else {
			map->mUseAreaHeight = 1;
		}
		if (map->mHeightMode == 0) {
			f32 height         = MapAreaHeight(map, map->mMapArea);
			f32 from           = map->mEye.y - (gGame.GetPlayer())->GetCameraTarget().y - map->mEyeYOffset;
			map->mHeightInterp = &map->mHeightEaseInOut;
			map->mHeightInterp->Start(from, height, 1.0f);
			if (map->mUseAreaHeight == 0) {
				map->mTargetHeight = 13.0f;
			}
			if (map->mUseAreaHeight == 1) {
				map->mTargetHeight = height;
			}
		}
		if (map->mHeightMode == 1) {
			f32 height            = MapAreaHeight(map, map->mMapArea);
			f32 from              = map->mEye.y - map->mEyeYOffset;
			map->mAbsHeightInterp = &map->mAbsHeightEaseInOut;
			map->mAbsHeightInterp->Start(from, height, 1.0f);
			if (map->mUseAreaHeight == 0) {
				map->mTargetHeight = 13.0f;
			}
			if (map->mUseAreaHeight == 1) {
				map->mTargetHeight = height;
			}
		}
	} else if (map->mMapArea == -1) {
		if (map->mHeightMode == 0 || (map->mHeightMode == 1 && map->mUseAreaHeight == 0)) {
			map->mHeightMode   = 0;
			f32 from           = map->mEye.y - (gGame.GetPlayer())->GetCameraTarget().y - map->mEyeYOffset;
			map->mHeightInterp = &map->mHeightEaseInOut;
			map->mHeightInterp->Start(from, map->mTargetHeight, 1.0f);
		}
		if (map->mHeightMode == 1 && map->mUseAreaHeight == 1) {
			f32 from              = map->mEye.y - map->mEyeYOffset;
			map->mAbsHeightInterp = &map->mAbsHeightEaseInOut;
			map->mAbsHeightInterp->Start(from, map->mTargetHeight, 1.0f);
		}
	}
}

/**
 * @note Address: 0x8015EE1C
 * @note Size: 0x190
 */
extern "C" f32 MapAreaHeight(CCameraActionMap* map, s32 index)
{
	CamAreaRecord* records = GetStageCameraAreaRecords();
	CamAreaInfo area       = CamAreaInfo(&records[index]);
	if (area.mRecord->mCameraAction == -1) {
		return 0.23f * CamInfo(&gCameraAreaState.mCameraInfo[gCameraAreaState.mGroups[index].mEntries->mId]).GetEye().y;
	}
	return CamInfo(&gCameraAreaState.mCameraInfo[gCameraAreaState.mGroups[index].mEntries->mId]).GetEye().y;
}

// the point 13 above and 1 behind the target's origin, in world space; GetMapCameraTargetOffsetPosition is its
// out-of-line copy, GetMapCameraEyeBehindTarget inlines it
static inline BOOL CalcMapCameraTargetOffsetPosition(CCameraActionMap* map, Vec& out)
{
	XObj* target = map->mTarget;
	if (target != NULL && (target->mStatus.mFlags & 1)) {
		Mtx mtx;
		PSMTXCopy(target->GetWorldMtx(), mtx);
		CVec pos(0.0f, 13.0f, -1.0f);
		PSMTXMultVec(mtx, &pos, &pos);
		out.x = pos.x;
		out.y = pos.y;
		out.z = pos.z;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x8015F460
 * @note Size: 0x264
 */
extern "C" void GetMapCameraEyeBehindTarget(CCameraActionMap* map, Vec& out, s32 mode)
{
	CVec result;
	if (mode != 1) {
		if (map->mTarget != NULL) {
			Mtx mtx;
			PSMTXCopy(map->mTarget->GetWorldMtx(), mtx);
			CVec pos(0.0f, 13.0f, -map->mTargetRestDistance);
			PSMTXMultVec(mtx, &pos, &pos);
			pos.y  = map->mHeightInterp->Get();
			result = pos;
		}
	} else if (map->mTarget != NULL) {
		Vec offset;
		CalcMapCameraTargetOffsetPosition(map, offset);
		CMotionControlCamera* camera = &gGame.mMotionControlCamera;
		CVec eye                     = camera->mView.mEye;
		CVec interest                = camera->mView.mInterest;
		f32 angle                    = HALF_PI + (f32)atan2(interest.x - eye.x, interest.z - eye.z);
		f32 distance                 = map->mTargetRestDistance;
		result.x                     = offset.x + distance * (f32)cos(angle);
		result.z                     = offset.z + distance * -(f32)sin(angle);
		result.y                     = 13.0f + map->mTarget->GetCameraTarget().y;
	}
	out.x = result.x;
	out.y = result.y;
	out.z = result.z;
}

void CCameraActionMap::PreviousUpdate()
{
	UpdateCameraAreaIndexAndBaseFov(&gCameraAreaState, 0);
	s32 activeArea = gCameraAreaState.mActiveArea;
	// the player stepped into another camera area
	if (gCameraAreaState.mCurrentArea != gCameraAreaState.mPrevArea && gCameraAreaState.mCurrentArea != -1) {
		if (GetCameraAreaMode(&gCameraAreaState) == CAMACTION_AREA) {
			gGame.mMotionControlCamera.ChangeAction(CAMACTION_AREA);
			gCameraAreaState.mPrevZoomLevel = gCameraAreaState.mZoomLevel;
			SelectAreaCameraEntryByViewDirection((CCameraActionArea*)gGame.mCameraActions.mTable[CAMACTION_AREA], 1, 2.0f, activeArea);
			return;
		}
		// otherwise the mode is CAMACTION_MAP and this action stays
		SetFovStep(this, gCameraAreaState.mZoomLevel, 15.0f);
		SelectAreaCameraEntryWithControl((CCameraActionArea*)gGame.mCameraActions.mTable[CAMACTION_AREA], activeArea, 0, 3, 1, 1.0f);
	}
	mPrevMapArea = mMapArea;
	SelectMapCameraArea(this);
	if (mMapArea != mPrevMapArea) {
		StartMapCameraAreaHeightInterpolation(this);
	}
}

// the camera target plus the running height-offset interpolation
static inline Vec GetMapCameraTargetWithHeightOffset(CCameraActionMap* map)
{
	Vec target = map->mTarget->GetCameraTarget();
	if (map->mTargetHeightOffsetInterp != NULL) {
		target.y += map->mTargetHeightOffsetInterp->Get();
	}
	return target;
}

// the GameCube has this out of line; the Wii build inlines it into Update
static inline void SetMapCameraAnglesFromEyeAndInterest(CCameraActionMap* map)
{
	f32 dz    = map->mInterest.z - map->mEye.z;
	f32 dx    = map->mInterest.x - map->mEye.x;
	f32 dy    = map->mInterest.y - map->mEye.y;
	f32 pitch = atan2(FSqrt(dx * dx + dz * dz), FSqrt(dy * dy));
	Vec angle;
	angle.x = dy < 0.0f ? HALF_PI - pitch : pitch - HALF_PI;
	angle.y = atan2(map->mInterest.x - map->mEye.x, map->mInterest.z - map->mEye.z);
	angle.z = 0.0f;
	map->SetAngle(angle);
}

// horizontal distance; out of line on the GameCube, inlined here
static inline f32 GetHorizontalDistance(const Vec* from, const Vec* to)
{
	f32 dz = to->z - from->z;
	f32 dx = to->x - from->x;
	return FSqrt(dx * dx + dz * dz);
}

// horizontal heading from one position to another
static inline f32 GetHeadingBetweenPositions(const Vec* from, const Vec* to)
{
	return atan2(to->x - from->x, to->z - from->z);
}

/**
 * @note Address: 0x8015FCF4
 * @note Size: 0x498
 */
extern "C" void UpdateMapCameraTurnInterpolation(CCameraActionMap* map, u32 step)
{
	if (map->mTurnInterpolation != NULL) {
		if (!map->mTurnInterpolation->Update(step, FRAME_TIME)) {
			f32 radius = map->mOrbitRadius;
			CVec position(map->mTurnInterpolation->Get());
			{
				CVec origin(map->mSpringTarget);
				origin.y = 0.0f;
				CVec direction((position - origin).Normalize());
				position = origin + direction * radius;
			}
			position.y      = map->mEye.y;
			map->mOrbitEye  = position;
			map->mTargetEye = position;
			SetMapCameraAnglesFromEyeAndInterest(map);
			if (__fpclassifyf(map->mAngle.x) == FP_NAN || __fpclassifyf(map->mAngle.y) == FP_NAN
			    || __fpclassifyf(map->mAngle.z) == FP_NAN) {
				OSPanic("ccameraactionmap2.cpp", 5085,
				        FormatToStaticBuffer("EYE(%f, %f, %f)ANG(%f, %f, %f)DIST(%f)CAMSTAT(%8X)CAMSTATPRE(%8X)UNDER "
				                             "OP(%d)HANG(%d(%d))TGTEYE(%f, %f, %f)TGTEYE_PRE(%f, %f, %f)INTERP CAM(0x%x)INTERP "
				                             "TURN(0x%x)POS_CAMMODE(%f, %f, %f)ANG_CAMMODE(%f, %f, %f)ANG_CAMMODE_PRE(%f, %f, %f)FIX(%d)\n",
				                             map->mEye.x, map->mEye.y, map->mEye.z, map->mAngle.x, map->mAngle.y, map->mAngle.z,
				                             map->mDistance, map->mStatus.mFlags, map->mPreviousStatus.mFlags, map->mManualInput,
				                             lbl_805A1D1C, lbl_805A1D18, map->mTargetEye.x, map->mTargetEye.y, map->mTargetEye.z,
				                             map->mPrevTargetEye.x, map->mPrevTargetEye.y, map->mPrevTargetEye.z, map->mEyeInterp,
				                             map->mTurnInterpolation, map->mOrbitEye.x, map->mOrbitEye.y, map->mOrbitEye.z,
				                             map->mOrbitAngle.x, map->mOrbitAngle.y, map->mOrbitAngle.z, map->mPrevOrbitAngle.x,
				                             map->mPrevOrbitAngle.y, map->mPrevOrbitAngle.z, lbl_805A1D14));
			}
			map->mOrbitAngle  = map->mAngle;
			map->mManualInput = 1;
		} else {
			map->mStatus.mFlags &= ~4;
			map->mTurnInterpolation = NULL;
		}
	}
}

/**
 * @note Address: 0x8015E4F4
 * @note Size: 0x828
 */
extern "C" void UpdateMapCameraPadControls(CCameraActionMap* map, u32 step)
{
	if (gGame.GetStatusFlags(0xC00)) {
		map->mPadControlEnabled = FALSE;
	} else {
		map->mPadControlEnabled = TRUE;
	}

	// Update presses these back into gWiiPointer's direction-pad bits at its end
	lbl_805A1D38 = gWiiPointer.Check(0x40000000);
	lbl_805A1D3C = gWiiPointer.Check(0x80000000);
	lbl_805A1D40 = gWiiPointer.Check(0x08000000);
	lbl_805A1D44 = gWiiPointer.Check(0x04000000);
	gWiiPointer.mFlags &= ~0x40000000ULL;
	gWiiPointer.mFlags &= ~0x80000000ULL;
	gWiiPointer.mFlags &= ~0x08000000ULL;
	gWiiPointer.mFlags &= ~0x04000000ULL;

	map->mOrbitInput = 0.0f;
	map->_11F0       = 0.0f;

	if ((gWiiPointer.mButton & 4) && map->mPadControlEnabled) {
		if (map->mPointerHoldTime > lbl_805A1288) {
			if (!map->mPointerHeld) {
				map->mPointerHeld = TRUE;
				map->mOrbitReseed = TRUE;
			}
		} else {
			map->mPointerHoldTime += step;
		}
	}

	if (map->mPointerHeld) {
		CVec vec(0.0f, 0.0f, 0.0f);
		if (gWiiPointer.mPointerValid) {
			Vec in     = { 2.0f * (gWiiPointer.mPointer.x / gPointerScreenWidth) - 1.0f,
				           2.0f * (gWiiPointer.mPointer.y / gPointerScreenHeight) - 1.0f, gWiiPointer.mPointer.z };
			Vec out    = fn_8016AA78(in, (f32*)lbl_802DB578);
			Vec scaled = out;
			vec.Set(scaled);
			map->mOrbitPointer = scaled;
		} else {
			if (!(gGame._41B0 & 8)) {
				vec.Set(map->mOrbitPointer);
			}
		}
		map->mOrbitInput = -vec.y;
	} else {
		if (map->mPadControlEnabled) {
			if (gWiiPointer.mButton & 1) {
				map->mOrbitInput = 1.0f;
			}
			if (gWiiPointer.mButton & 2) {
				map->mOrbitInput = -1.0f;
			}
			if (gWiiPointer.mTrigger & 8) {
				if (gCameraAreaState.mZoomLevel > 1) {
					gWiiPointer.mFlags |= 0x40000000;
				}
				if (gCameraAreaState.mZoomLevel < 2) {
					gWiiPointer.mFlags |= 0x80000000;
				}
			}
			if (map->mPointerHoldTime != 0.0f && map->mPointerHoldTime <= lbl_805A1288) {
				gWiiPointer.mFlags |= 0x400000000ULL;
			}
		}
		map->mPointerHeld     = FALSE;
		map->mPointerHoldTime = 0.0f;
	}

	map->mArrows[0].mShown = 0;
	map->mArrows[1].mShown = 0;
	if (map->mOrbitInput != 0.0f) {
		if (map->mOrbitInput > 0.0f) {
			map->mArrows[1].mShown = 1;
		}
		if (map->mOrbitInput < 0.0f) {
			map->mArrows[0].mShown = 1;
		}
	}

	if (map->mPointerHeld || map->mArrows[0].mShown) {
		if (map->mArrows[0]._14 < 1.0f) {
			map->mArrows[0]._14 += 0.1f * lbl_805A1290[0] * step;
			if (map->mArrows[0]._14 > 1.0f) {
				map->mArrows[0]._14 = 1.0f;
			}
		}
	} else if (map->mArrows[0]._14 > 0.0f) {
		map->mArrows[0]._14 -= 0.1f * lbl_805A1290[1] * step;
		if (map->mArrows[0]._14 < 0.0f) {
			map->mArrows[0]._14 = 0.0f;
		}
	}
	if (map->mPointerHeld || map->mArrows[1].mShown) {
		if (map->mArrows[1]._14 < 1.0f) {
			map->mArrows[1]._14 += 0.1f * lbl_805A1290[0] * step;
			if (map->mArrows[1]._14 > 1.0f) {
				map->mArrows[1]._14 = 1.0f;
			}
		}
	} else if (map->mArrows[1]._14 > 0.0f) {
		map->mArrows[1]._14 -= 0.1f * lbl_805A1290[1] * step;
		if (map->mArrows[1]._14 < 0.0f) {
			map->mArrows[1]._14 = 0.0f;
		}
	}

	map->mManualInput = 0;
	if (map->mOrbitInput != 0.0f || map->_11F0 != 0.0f || map->mOrbiting) {
		map->mManualInput = 1;
		SetMapCameraModeFlags(map, 0x20008);
	} else if (map->mStatus.mFlags & 8) {
		map->mStatus.mFlags &= ~8;
	}

	if (map->mTarget != NULL && map->mTarget->HasModel() && gWiiPointer.Check(0x400000000ULL) && map->mPadControlEnabled
	    && gGame.GetPlayer()->mHang.mState != 3) {
		if (TrySetMapCameraTargetYaw(map, map->mTarget->mMoveTarget1.y, 10.0f, 0.0f)) {
			gGame.GetPlayer()->ResetCameraValue();
			SetMapCameraModeFlags(map, 0x20002);
		}
	}

	UpdateMapCameraTurnInterpolation(map, step);

	s32 zoomLevel = gCameraAreaState.mZoomLevel;
	if (gWiiPointer.Check(0x40000000) && map->mPadControlEnabled) {
		if (zoomLevel > 1) {
			zoomLevel--;
		}
	} else if (gWiiPointer.Check(0x80000000) && map->mPadControlEnabled) {
		if (zoomLevel < 2) {
			zoomLevel++;
		}
	}
	if (zoomLevel != gCameraAreaState.mPrevZoomLevel) {
		SetFovStep(map, zoomLevel, 15.0f);
	}

	if (!map->mFovEaseOutDivide.Update(step, 0.0f)) {
		map->SetFov(map->mFovEaseOutDivide.Get());
	}
}

static inline void StartCameraEyeInterpolation(CCameraActionMap* map, Vec to, f32 time)
{
	map->mEyeInterp = &map->mEyeEaseOutDivide;
	map->mEyeInterp->Start(map->mEye, to, time);
}

static inline void UpdateCameraEyeInterpolation(CCameraActionMap* map, u32 step)
{
	if (map->mEyeInterp != NULL) {
		if (!map->mEyeInterp->Update(step, FRAME_TIME)) {
			map->SetEye(map->mEyeInterp->Get());
		} else {
			map->mEyeInterp = NULL;
			map->mStatus.mFlags &= ~0x3E;
		}
	}
}

// the nearest intersection list entry to pos, NULL when the list is empty
static inline CamMapEntry* GetNearestMapCameraEntry(CCameraActionMap* map, const Vec& pos)
{
	s32 idx = FindNearestMapCameraEntry(map, pos);
	if (idx == -1) {
		return NULL;
	}
	return &map->mIntersectionEntries[idx];
}

// how many of the frame's intersection entries belong to the given area
static inline s32 CountMapCameraEntriesInGroup(CCameraActionMap* map, s32 recordIndex)
{
	s32 count = 0;
	if (map->mIntersectionCount > 0) {
		for (s32 i = 0; i < map->mIntersectionCount; i++) {
			if (map->mIntersectionEntries[i].mRecordIndex == recordIndex) {
				count++;
			}
		}
	}
	return count;
}

// NONMATCHING: the logic matches throughout; what remains is register allocation, stack-slot
// choice (retail's frame is 0x60 larger, more named temporaries) and literal-pool offsets (93,3 %)
/**
 * @note Address: 0x80157D84
 * @note Size: 0x1FEC
 */
void CCameraActionMap::Update(u32 step)
{
	if (step > 2)
		step = 2;
	if (mTarget == NULL)
		return;

	Vec pos          = *mTarget->GetPos();
	mInsideAreaCount = 0;
	for (u32 i = 0; i < 32; i++)
		mInsideAreas[i] = -2;
	pos.y += 10.0f;
	mInsideAreaCount  = CollectCameraAreasContainingPoint(&gCameraAreaState, pos, mInsideAreas);
	f32 dt            = 0.1f * step;
	mSpringPrevTarget = mSpringTarget;
	CalcMapCameraTargetOffsetPosition(this, mSpringTarget);
	UpdateMapCameraPadControls(this, step);
	// orbiting with no turn running and the target moved: go back to trailing it on the spring
	if ((mStatus.mFlags & 0x20000) && !(mStatus.mFlags & 0x3E) && mSpringTarget != mSpringPrevTarget) {
		SetMapCameraModeFlags(this, 0x10000);
	}

	// retail steps these twice
	for (s32 i = 0; i < 2; i++) {
		if (mHeightMode == 0) {
			if (mHeightInterp != NULL)
				mHeightInterp->Update(step, FRAME_TIME);
			f32 height     = mHeightInterp->Get();
			mCurrentHeight = height + gGame.GetPlayer()->GetCameraTarget().y;
		}
		if (mHeightMode == 1) {
			if (mAbsHeightInterp != NULL)
				mAbsHeightInterp->Update(step, FRAME_TIME);
			mCurrentHeight = mAbsHeightInterp->Get();
		}
	}
	if (mTargetHeightOffsetInterp != NULL)
		mTargetHeightOffsetInterp->Update(step, FRAME_TIME);
	if (mEyeYOffsetInterp != NULL) {
		if (!mEyeYOffsetInterp->Update(step, FRAME_TIME))
			mEyeYOffset = mEyeYOffsetInterp->Get();
	} else {
		mEyeYOffsetInterp = NULL;
	}

	// spring mode: the eye trails mSpringTarget on a damped spring
	if (mStatus.mFlags & 0x10000) {
		{
			CVec eye(mEye);
			f32 previousDistance = PSVECDistance(&eye, &mSpringPrevTarget);
			f32 distance         = PSVECDistance(&eye, &mSpringTarget);
			f32 rest             = mRestDistance;
			f32 targetRest       = mTargetRestDistance;
			if (rest != targetRest && distance > previousDistance) {
				if (distance < targetRest && rest != targetRest)
					rest = distance;
				if (distance >= targetRest)
					rest = targetRest;
			} else if (distance < previousDistance) {
				rest = targetRest;
			}
			mRestDistance = rest;
		}
		CVec eye(mEye);
		eye = SpringDamp(eye, mSpringTarget, mSpringPrevTarget, dt, sMapCameraSpringParameters[0], mSpringDamping, mRestDistance);
		SetEye(eye);
		SetInterest(GetMapCameraTargetWithHeightOffset(this));
		SetEye(mEye.x, mPreviousEye.y, mEye.z);
		SetMapCameraAnglesFromEyeAndInterest(this);
		if (__fpclassifyf(mAngle.x) == FP_NAN || __fpclassifyf(mAngle.y) == FP_NAN || __fpclassifyf(mAngle.z) == FP_NAN) {
			OSPanic("ccameraactionmap2.cpp", 660,
			        FormatToStaticBuffer("EYE(%f, %f, %f)ANG(%f, %f, %f)DIST(%f)CAMSTAT(%8X)CAMSTATPRE(%8X)UNDER "
			                             "OP(%d)HANG(%d(%d))TGTEYE(%f, %f, %f)TGTEYE_PRE(%f, %f, %f)INTERP CAM(0x%x)INTERP "
			                             "TURN(0x%x)POS_CAMMODE(%f, %f, %f)ANG_CAMMODE(%f, %f, %f)ANG_CAMMODE_PRE(%f, %f, %f)FIX(%d)\n",
			                             mEye.x, mEye.y, mEye.z, mAngle.x, mAngle.y, mAngle.z, mDistance, mStatus.mFlags,
			                             mPreviousStatus.mFlags, mManualInput, lbl_805A1D1C, lbl_805A1D18, mTargetEye.x, mTargetEye.y,
			                             mTargetEye.z, mPrevTargetEye.x, mPrevTargetEye.y, mPrevTargetEye.z, mEyeInterp, mTurnInterpolation,
			                             mOrbitEye.x, mOrbitEye.y, mOrbitEye.z, mOrbitAngle.x, mOrbitAngle.y, mOrbitAngle.z,
			                             mPrevOrbitAngle.x, mPrevOrbitAngle.y, mPrevOrbitAngle.z, lbl_805A1D14));
		}

		Vec movement;
		PSVECSubtract(&mSpringTarget, &mSpringPrevTarget, &movement);
		movement.y = 0.0f;
		if (movement.x != 0.0f || movement.z != 0.0f) {
			PSVECNormalize(&movement, &movement);
			Vec view;
			view.x  = (f32)sin(mAngle.y);
			view.y  = 0.0f;
			view.z  = (f32)cos(mAngle.y);
			f32 dot = PSVECDotProduct(&movement, &view);
			CVec current(mEye);
			f32 distance = GetHorizontalDistance(&current, &mSpringTarget);
			// the target heads back at the camera (over 120 degrees off the camera's view) within 20 of the eye
			if (dot < -0.5f && distance < 20.0f)
				sMapCameraOrbitAtRestDistance = 1;
			else
				sMapCameraOrbitAtRestDistance = 0;
		}
		// lbl_805A1D1C is the player's hang state, lbl_805A1D18 the one before it (HANG(%d(%d)) in the panic text)
		lbl_805A1D18 = lbl_805A1D1C;
		lbl_805A1D1C = gGame.GetPlayer()->mHang.mState;
		// on entering hang state 3, swing round in front of the target if no area edge is in the way
		if (gGame.GetPlayer()->mHang.mState == 3 && lbl_805A1D1C != lbl_805A1D18) {
			CVec center = *mTarget->GetPos();
			CVec angles(mAngle.x, PI + mTarget->mMoveTarget1.y, mAngle.z);
			f32 yaw   = HALF_PI + angles.y;
			f32 pitch = mAngle.x;
			f32 distance;
			{
				CVec target(mTarget->GetCameraTarget());
				CVec current(mEye);
				distance = PSVECDistance(&target, &current);
			}
			Vec candidate;
			candidate.x = center.x + distance * (f32)cos(yaw) * (f32)cos(pitch);
			candidate.z = center.z + distance * -(f32)sin(yaw) * (f32)cos(pitch);
			candidate.y = center.y;
			CVec testEye(candidate);
			CVec testCenter(center);
			sMapCameraTestEye    = testEye;
			sMapCameraTestCenter = testCenter;
			mIntersectionCount   = CalcIntersectVec(candidate, center, mIntersectionEntries, 0);
			if (mIntersectionCount == 0 && TrySetMapCameraTargetYaw(this, PI + mTarget->mMoveTarget1.y, 20.0f, 0.0f)) {
				SetMapCameraModeFlags(this, 0x20002);
				goto checkCollision;
			}
		}
		// the target came back at the camera: put the eye behind its facing, at least 31 away
		if (sMapCameraOrbitAtRestDistance != 0) {
			SetInterest(GetMapCameraTargetWithHeightOffset(this));
			CVec angles;
			angles = CVec(mAngle.x, mTarget->mMoveRotate.y, mAngle.z);
			SetAngle(angles);
			CVec current(mEye);
			f32 distance = PSVECDistance(&current, &mSpringTarget);
			if (distance < 31.0f)
				distance = 31.0f;
			f32 pitch = mAngle.x;
			f32 yaw   = HALF_PI + mAngle.y;
			Vec target;
			target.x = mSpringTarget.x + distance * (f32)cos(yaw) * (f32)cos(pitch);
			target.z = mSpringTarget.z + distance * -(f32)sin(yaw) * (f32)cos(pitch);
			target.y = mCurrentHeight;
			SetEye(target);
			gGame.GetPlayer()->ResetCameraValue();
			SetMapCameraAnglesFromEyeAndInterest(this);
			if (__fpclassifyf(mAngle.x) == FP_NAN || __fpclassifyf(mAngle.y) == FP_NAN || __fpclassifyf(mAngle.z) == FP_NAN) {
				OSPanic("ccameraactionmap2.cpp", 762,
				        FormatToStaticBuffer("EYE(%f, %f, %f)ANG(%f, %f, %f)DIST(%f)CAMSTAT(%8X)CAMSTATPRE(%8X)UNDER "
				                             "OP(%d)HANG(%d(%d))TGTEYE(%f, %f, %f)TGTEYE_PRE(%f, %f, %f)INTERP CAM(0x%x)INTERP "
				                             "TURN(0x%x)POS_CAMMODE(%f, %f, %f)ANG_CAMMODE(%f, %f, %f)ANG_CAMMODE_PRE(%f, %f, %f)FIX(%d)\n",
				                             mEye.x, mEye.y, mEye.z, mAngle.x, mAngle.y, mAngle.z, mDistance, mStatus.mFlags,
				                             mPreviousStatus.mFlags, mManualInput, lbl_805A1D1C, lbl_805A1D18, mTargetEye.x, mTargetEye.y,
				                             mTargetEye.z, mPrevTargetEye.x, mPrevTargetEye.y, mPrevTargetEye.z, mEyeInterp,
				                             mTurnInterpolation, mOrbitEye.x, mOrbitEye.y, mOrbitEye.z, mOrbitAngle.x, mOrbitAngle.y,
				                             mOrbitAngle.z, mPrevOrbitAngle.x, mPrevOrbitAngle.y, mPrevOrbitAngle.z, lbl_805A1D14));
			}
			sMapCameraOrbitAtRestDistance = 0;
		} else {
			SetMapCameraAnglesFromEyeAndInterest(this);
			if (__fpclassifyf(mAngle.x) == FP_NAN || __fpclassifyf(mAngle.y) == FP_NAN || __fpclassifyf(mAngle.z) == FP_NAN) {
				OSPanic("ccameraactionmap2.cpp", 772,
				        FormatToStaticBuffer("EYE(%f, %f, %f)ANG(%f, %f, %f)DIST(%f)CAMSTAT(%8X)CAMSTATPRE(%8X)UNDER "
				                             "OP(%d)HANG(%d(%d))TGTEYE(%f, %f, %f)TGTEYE_PRE(%f, %f, %f)INTERP CAM(0x%x)INTERP "
				                             "TURN(0x%x)POS_CAMMODE(%f, %f, %f)ANG_CAMMODE(%f, %f, %f)ANG_CAMMODE_PRE(%f, %f, %f)FIX(%d)\n",
				                             mEye.x, mEye.y, mEye.z, mAngle.x, mAngle.y, mAngle.z, mDistance, mStatus.mFlags,
				                             mPreviousStatus.mFlags, mManualInput, lbl_805A1D1C, lbl_805A1D18, mTargetEye.x, mTargetEye.y,
				                             mTargetEye.z, mPrevTargetEye.x, mPrevTargetEye.y, mPrevTargetEye.z, mEyeInterp,
				                             mTurnInterpolation, mOrbitEye.x, mOrbitEye.y, mOrbitEye.z, mOrbitAngle.x, mOrbitAngle.y,
				                             mOrbitAngle.z, mPrevOrbitAngle.x, mPrevOrbitAngle.y, mPrevOrbitAngle.z, lbl_805A1D14));
			}
			CVec target(*mTarget->GetPos());
			target.y += 10.0f;
			CVec current(mEye);
			current.y = target.y;
			CVec direction;
			PSVECSubtract(&target, &current, &direction);
			PSVECNormalize(&direction, &direction);
			target += direction * 3.0f;
			mIntersectionCount  = CalcIntersectVec(mEye, target, mIntersectionEntries, 0);
			mNearestTargetIndex = FindNearestMapCameraEntry(this, *mTarget->GetPos());
			mNearestTargetEntry = GetNearestMapCameraEntry(this, *mTarget->GetPos());
			mNearestEyeIndex    = FindNearestMapCameraEntry(this, mEye);
			mNearestEyeEntry    = GetNearestMapCameraEntry(this, mEye);
			SetMapCameraSideEndpoints(this);
			if (mNearestEyeEntry != NULL) {
				// turn only if the eye-to-target line cuts a corner of the area hit nearest the eye: two hits, on
				// neighbouring sides
				s32 count    = CountMapCameraEntriesInGroup(this, mNearestEyeEntry->mRecordIndex);
				s32 adjacent = CountMapCameraAdjacentSideIntersections(this, mNearestEyeEntry);
				if (count < 2)
					mTurnBlocked = 1;
				else if (adjacent == 0)
					mTurnBlocked = 1;
				else {
					bool allowsTurn = CamAreaInfo(&gCameraAreaState.mAreaExRecords[mNearestEyeEntry->mRecordIndex]).AllowsTurn();
					if (allowsTurn)
						mTurnBlocked = 0;
					else
						mTurnBlocked = 1;
					if (gGame.GetPlayer()->mCopter.mState != 0 || gGame.GetPlayer()->mJump.mState != 0) {
						mTurnBlocked = 1;
					}
				}
			}
			if (_8C.mValue.y == _8C.mTarget.y && mNearestEyeEntry != NULL && mTurnBlocked == 0) {
				StartMapCameraTurnInterpolation(this, 0);
			}
		}
	checkCollision:
		CVec current(mEye);
		CVec previous(mPreviousEye);
		ResolveMapCameraEyeCollision(this, current, previous, mInterest, mPreviousInterest, sMapCameraCollisionMode0);
		SetEye(current);
	}

	// orbit mode: UpdateMapCameraOrbit turns the eye round the interest, then the eye eases to mTargetEye
	if (mStatus.mFlags & 0x20000) {
		SetInterest(GetMapCameraTargetWithHeightOffset(this));
		if (!(mStatus.mFlags & 0x400))
			mTargetEye.y = mCurrentHeight;
		mTargetEye.y = mPrevTargetEye.y;
		UpdateMapCameraOrbit(this, step);
		mTargetEye.y = mPrevTargetEye.y;
		if (sMapCameraCenterReset != 0) {
			CVec previous(mPrevTargetEye);
			mPrevTargetEye = mTargetEye;
			CVec current(mTargetEye);
			current    = SpringDamp(current, mSpringTarget, mSpringPrevTarget, 0.1f * step, sMapCameraSpringParameters[0], mSpringDamping,
			                        mTargetRestDistance);
			mTargetEye = current;
			mPrevTargetEye = previous;
		}
		lbl_805A1D34 = GetHorizontalDistance(&mTargetEye, &mPrevTargetEye);
		// no area collision while mTurnInterpolation runs (eye mode 4)
		if (!(mStatus.mFlags & 4)) {
			s32 wasBlocked = lbl_805A1D24;
			f32 before     = GetHeadingBetweenPositions(&mTargetEye, &mInterest);
			BOOL hit
			    = ResolveMapCameraEyeCollision(this, mTargetEye, mPrevTargetEye, mInterest, mPreviousInterest, &sMapCameraCollisionMode1);
			// lbl_805A1D28 is the last turn direction: a heading the collision pushed back against it blocks the turn
			if (lbl_805A1D28 > 0.0f) {
				f32 after = GetHeadingBetweenPositions(&mTargetEye, &mInterest);
				GetHeadingBetweenPositions(&mPrevTargetEye, &mInterest);
				before = WrapAngle(before);
				after  = WrapAngle(after);
				if (after - before < 0.0f) {
					lbl_805A1D24 = 1;
					lbl_805A1D2C++;
				} else {
					lbl_805A1D24 = 0;
					lbl_805A1D2C = 0;
				}
			} else if (lbl_805A1D28 < 0.0f) {
				f32 after = GetHeadingBetweenPositions(&mTargetEye, &mInterest);
				GetHeadingBetweenPositions(&mPrevTargetEye, &mInterest);
				before = WrapAngle(before);
				after  = WrapAngle(after);
				if (after - before > 0.0f) {
					lbl_805A1D24 = 1;
					lbl_805A1D2C++;
				} else {
					lbl_805A1D24 = 0;
					lbl_805A1D2C = 0;
				}
			}
			if (lbl_805A1D24 == 1 && wasBlocked != lbl_805A1D24) {
				mTargetEye  = mPrevTargetEye;
				mOrbitAngle = mPrevOrbitAngle;
			}
			if (hit != 0) {
				if (sMapCameraCenterReset != 0)
					sMapCameraCenterReset = 0;
			} else {
				if (sMapCameraCenterReset == 0)
					sMapCameraCenterReset = 1;
			}
		}
		lbl_805A1D30 = GetHorizontalDistance(&mTargetEye, &mPrevTargetEye);
		if (lbl_805A1D30 > lbl_805A1D34)
			mEyeInterpTime = 0.0f;
		else {
			mEyeInterpTime = 3.0f;
			if (mStatus.mFlags & 0x10)
				mEyeInterpTime = 5.0f;
		}
		mPrevTargetEye.y = mTargetEye.y = mCurrentHeight;
		SetEye(CVec(mEye.x, mTargetEye.y, mEye.z));
		if (mPrevTargetEye != mTargetEye)
			StartCameraEyeInterpolation(this, mTargetEye, mEyeInterpTime);
	}

	UpdateCameraEyeInterpolation(this, step);
	f32 weight = GetHorizontalDistance(&mEye, &mSpringTarget) / 60.0f;
	if (weight > 1.0f)
		weight = 1.0f;
	if (weight < 0.0f)
		weight = 0.0f;
	if (gGame.mMotionControlCamera.GetMapDistance() <= 1.0f)
		weight = 0.0f;
	f32 interestHeight = mInterest.y;
	f32 height         = mCurrentHeight;
	f32 difference     = height - interestHeight;
	if (difference < 0.0f)
		difference = 0.0f;
	f32 weighted = difference * weight;
	f32 offset   = interestHeight + weighted;
	offset -= height;
	f32 from = mEyeYOffsetInterp->Get();
	if (0.0f != offset) {
		mEyeYOffsetInterp = &mEyeYOffsetEaseInOut;
		mEyeYOffsetInterp->Start(from, offset, 0.05f);
	}
	Vec eye = mEye;
	eye.y   = mCurrentHeight;
	eye.y += mEyeYOffset;
	SetEye(eye);
	CVec previousEye(mEye);
	CVec previousInterest(mInterest);
	mPreviousEye           = previousEye;
	mPrevTargetEye         = mTargetEye;
	mPrevOrbitEye          = mOrbitEye;
	mPreviousInterest      = previousInterest;
	mPreviousStatus.mFlags = mStatus.mFlags;

	// the Wii remote's direction pad: Update forces these bits from its own state
	if (lbl_805A1D38 != 0) {
		gWiiPointer.mFlags |= 0x40000000;
	} else {
		gWiiPointer.mFlags &= ~0x40000000;
	}
	if (lbl_805A1D3C != 0) {
		gWiiPointer.mFlags |= 0x80000000;
	} else {
		gWiiPointer.mFlags &= ~0x80000000ULL;
	}
	if (lbl_805A1D44 != 0) {
		gWiiPointer.mFlags |= 0x04000000;
	} else {
		gWiiPointer.mFlags &= ~0x04000000;
	}
	if (lbl_805A1D40 != 0) {
		gWiiPointer.mFlags |= 0x08000000;
	} else {
		gWiiPointer.mFlags &= ~0x08000000;
	}
}

// nudges target onto the shorter arc toward yaw, both wrapped into [0, 2pi) beforehand
static inline void AdjustOrbitTargetAngle(f32& target, f32 yaw)
{
	f32 diff = __fabsf(target - yaw);
	if (diff > 0.0f) {
		if (diff >= TWO_PI) {
			diff -= TWO_PI;
		}
		if (diff > PI) {
			if (target < yaw) {
				target += TWO_PI;
			} else {
				target -= TWO_PI;
			}
		}
	}
}

// NONMATCHING: the logic matches throughout, including retail's unread eye.y on the two driven
// paths; what remains is register allocation (retail rematerializes the CVec vtable pointer per
// block instead of keeping it in a saved register, so its frame is 0x10 smaller) and the usual
// stack-slot shuffling (87,5 %)
/**
 * @note Address: 0x8015D928
 * @note Size: 0xBCC
 */
void UpdateMapCameraOrbit(CCameraActionMap* map, u32 step)
{
	sPreviousMapOrbitAngle = map->mOrbitAngle;
	map->mOrbitEye         = map->mTargetEye;
	if (lbl_805A1D24 == 0) {
		map->mOrbitAngle = map->mAngle;
	}

	// the second orbit axis is not wired up in this build
	f32 input  = map->mOrbitInput;
	f32 input2 = 0.0f;

	WiiCameraPoint point = fn_8016A844((const WiiCameraCurve*)&map->_126C, __fabsf(input));
	if (input > 0.0f) {
		input = point.y;
	}
	if (input < 0.0f) {
		input = -point.y;
	}

	if (input != 0.0f || input2 != 0.0f) {
		map->mOrbitPointer.x = map->mOrbitInput;
		lbl_805A1D28         = input;
		if (map->mOrbitReseed) {
			map->mOrbitReseed    = 0;
			map->_11FC.mTarget.y = map->mOrbitAngle.y;
		}
		map->_11FC.mValue.y = map->mOrbitAngle.y;
		map->_11FC.mTarget.y += step * (0.040142573f * input);
		f32 target = WrapAngle(map->_11FC.mTarget.y);
		f32 yaw    = WrapAngle(map->_11FC.mValue.y);
		AdjustOrbitTargetAngle(target, yaw);
		map->_11FC.mValue.y  = yaw;
		map->_11FC.mTarget.y = target;
		if (yaw < target) {
			lbl_805A1D28 = 1.0f;
		} else if (yaw > target) {
			lbl_805A1D28 = -1.0f;
		}
		map->_11FC.mSpeed.x = 0.0f;
		map->_11FC.mSpeed.z = 0.0f;
		map->_11FC.mSpeed.y = __fabsf(map->_11FC.mTarget.y - map->_11FC.mValue.y) / lbl_8047F170.y;
		map->_11FC.Step(step);
		map->SetAngle(map->mOrbitAngle.x, map->_11FC.mValue.y, map->mOrbitAngle.z);
		map->mPrevOrbitAngle = map->mOrbitAngle;
		map->mOrbitAngle     = map->mAngle;
		map->SetInterest(GetMapCameraTargetWithHeightOffset(map));
		f32 yaw2  = HALF_PI + map->mAngle.y;
		f32 pitch = 0.0f;
		CVec flat(map->mTargetEye.x, map->mInterest.y, map->mTargetEye.z);
		f32 dist = PSVECDistance(&flat, &map->mInterest);
		Vec eye;
		eye.x = map->mInterest.x + dist * (f32)cos(yaw2) * (f32)cos(pitch);
		eye.z = map->mInterest.z + dist * -(f32)sin(yaw2) * (f32)cos(pitch);
		// retail never sets eye.y on this path; the assignments pass the stale word through
		map->mOrbitEye  = eye;
		map->mTargetEye = eye;
		map->mOrbiting  = 1;
	} else {
		if (map->mStatus.mFlags & 2) {
			map->mOrbiting    = 0;
			map->mOrbitReseed = 1;
		} else if (map->mOrbiting) {
			map->_11FC.mValue.y = map->mOrbitAngle.y;
			f32 target          = WrapAngle(map->_11FC.mTarget.y);
			f32 yaw             = WrapAngle(map->_11FC.mValue.y);
			AdjustOrbitTargetAngle(target, yaw);
			map->_11FC.mValue.y  = yaw;
			map->_11FC.mTarget.y = target;
			if (input2 == 0.0f && input == 0.0f) {
				f32 ease             = 0.3f;
				map->_11FC.mTarget.y = (1.0f - ease) * map->_11FC.mTarget.y + ease * map->_11FC.mValue.y;
			} else {
				map->_11FC.mTarget.y += step * (0.040142573f * input);
			}
			map->_11FC.mSpeed.x = 0.0f;
			map->_11FC.mSpeed.z = 0.0f;
			map->_11FC.mSpeed.y = __fabsf(map->_11FC.mTarget.y - map->_11FC.mValue.y) / lbl_8047F170.y;
			map->_11FC.Step(step);
			if (__fabsf(map->_11FC.mSpeed.y) > 0.01f) {
				if (yaw < target) {
					lbl_805A1D28 = 1.0f;
				} else if (yaw > target) {
					lbl_805A1D28 = -1.0f;
				}
				map->SetAngle(map->mOrbitAngle.x, map->_11FC.mValue.y, map->mOrbitAngle.z);
				map->mPrevOrbitAngle = map->mOrbitAngle;
				map->mOrbitAngle     = map->mAngle;
				map->SetInterest(GetMapCameraTargetWithHeightOffset(map));
				f32 yaw2  = HALF_PI + map->mAngle.y;
				f32 pitch = 0.0f;
				CVec flat(map->mTargetEye.x, map->mInterest.y, map->mTargetEye.z);
				f32 dist = PSVECDistance(&flat, &map->mInterest);
				Vec eye;
				eye.x = map->mInterest.x + dist * (f32)cos(yaw2) * (f32)cos(pitch);
				eye.z = map->mInterest.z + dist * -(f32)sin(yaw2) * (f32)cos(pitch);
				// same stale eye.y as the stick-driven path
				map->mOrbitEye  = eye;
				map->mTargetEye = eye;
				map->mOrbiting  = 1;
			} else {
				map->mOrbiting    = 0;
				map->mOrbitReseed = 1;
			}
		}
	}

	if (map->mStatus.mFlags & 2) {
		map->mOrbitEye = map->mTargetEye;
		if (lbl_805A1D24 == 0) {
			map->mOrbitAngle = map->mAngle;
		}
		f32 newYaw = WrapAngle(map->mOrbitAngle.y);
		f32 target = WrapAngle(map->mTargetYaw);
		AdjustOrbitTargetAngle(target, newYaw);
		if (newYaw < target) {
			lbl_805A1D28 = 1.0f;
			newYaw += step * map->mTurnSpeed;
			if (newYaw > target) {
				newYaw = target;
			}
		} else if (newYaw > target) {
			newYaw -= step * map->mTurnSpeed;
			if (newYaw < target) {
				newYaw = target;
			}
			lbl_805A1D28 = -1.0f;
		}
		if (__fabsf(target - newYaw) < 0.001f) {
			newYaw = target;
		}
		BOOL turning = TRUE;
		Vec angle(map->mOrbitAngle);
		angle.y = newYaw;
		map->SetAngle(angle);
		map->mPrevOrbitAngle = map->mOrbitAngle;
		map->mOrbitAngle     = map->mAngle;
		map->SetInterest(GetMapCameraTargetWithHeightOffset(map));
		f32 yaw2  = HALF_PI + map->mAngle.y;
		f32 pitch = 0.0f;
		CVec flat(map->mTargetEye.x, map->mInterest.y, map->mTargetEye.z);
		f32 dist = PSVECDistance(&flat, &map->mInterest);
		Vec eye;
		eye.x             = map->mInterest.x + dist * (f32)cos(yaw2) * (f32)cos(pitch);
		eye.y             = map->mTargetEye.y;
		eye.z             = map->mInterest.z + dist * -(f32)sin(yaw2) * (f32)cos(pitch);
		map->mOrbitEye    = eye;
		map->mTargetEye   = eye;
		map->mManualInput = 1;
		if (newYaw == target) {
			turning = FALSE;
		}
		if (!turning) {
			map->mStatus.mFlags &= ~2;
		}
	}
}

f32 CCameraActionMap::GetFollowDivisor()
{
	// CMotionControlCamera::Follow closes 1/6 of the remaining distance per call
	return 6.0f;
}

u32 CCameraActionMap::GetInterestMode()
{
	return mInterestMode;
}

u32 CCameraActionMap::GetCameraFlags()
{
	return CAMFLAG_EYE | CAMFLAG_INTEREST | CAMFLAG_FOV;
}

// set by ResetCenter; the spring update starts over from the new center
s32 sMapCameraCenterReset;
// the area index the last turn interpolation aims at, Update skips starting another for it
s32 lbl_805A1D10;
// the FIX(%d) value in Update's panic dump
s32 lbl_805A1D14;
// the player's hang state, last frame and this one (HANG(%d(%d)) in the panic dump)
s32 lbl_805A1D18;
s32 lbl_805A1D1C;
// 1: a new orbit keeps the target rest distance instead of the eye's current one
s32 sMapCameraOrbitAtRestDistance;
// 1 while the collision keeps pushing the orbit eye back against the turn direction
s32 lbl_805A1D24;
// how often the turn was blocked in a row
s32 lbl_805A1D2C;
// the orbit eye's horizontal move this frame, after and before the orbit step
f32 lbl_805A1D30;
f32 lbl_805A1D34;
// the Wii build presses these into gWiiPointer's direction-pad bits at the end of Update
s32 lbl_805A1D38;
s32 lbl_805A1D3C;
s32 lbl_805A1D40;
s32 lbl_805A1D44;

static inline f32 FlatDistance(const CVec& pos, CVec& v)
{
	CVec flat(pos.x, v.y, pos.z);
	return PSVECDistance(&flat, &v);
}

// the orbit starts from where the camera is
static inline void ResetMapCameraTargetEye(CCameraActionMap* map)
{
	map->mOrbitEye       = map->mEye;
	map->mPrevOrbitEye   = map->mEye;
	map->mOrbitAngle     = map->mAngle;
	map->mPrevOrbitAngle = map->mAngle;
	map->mPrevTargetEye  = map->mEye;
	map->mTargetEye      = map->mPrevTargetEye;
	map->mEyeInterp      = NULL;

	map->mFlatDistance = FlatDistance(map->mTargetEye, map->mInterest);
	FlatDistance(map->mTargetEye, map->mSpringTarget); // retail computes and drops it (bl PSVECDistance at 0x80160350)
}

static inline void SetMapCameraPreviousEye(CCameraActionMap* map, Vec v)
{
	map->mPreviousEye.Set(v);
}

static inline void SetMapCameraPreviousInterest(CCameraActionMap* map, Vec v)
{
	map->mPreviousInterest.Set(v);
}

/**
 * @note Address: 0x801576A4
 * @note Size: 0x4F4
 */
void CCameraActionMap::SetCamera(CCamera* camera)
{
	// CMotionControlCamera::Interpolate takes the interest's height at once and follows it in on x and z
	mInterestMode = 3;
	SetEye(camera->mView.mEye);
	SetMapCameraPreviousEye(this, camera->mView.mEye);
	SetInterest(camera->mView.mInterest);
	SetDistance(camera->mView.mDistance);
	SetFov(camera->mFov);
	SetAngle(camera->mView.mAngle);
	mTarget = gGame.GetPlayer();
	if (mTarget != NULL) {
		SetInterest(mTarget->GetCameraTarget());
	}
	mTargetHeightOffsetInterp = NULL;
	mTargetHeightOffsetEase.Start(0.0f, 0.0f, 0.1f);
	if (mTarget != NULL) {
		Mtx mtx;
		PSMTXCopy(mTarget->GetWorldMtx(), mtx);
		// start the spring at rest on the point Update tracks (GetMapCameraTargetOffsetPosition)
		CVec pos(0.0f, 13.0f, -1.0f);
		PSMTXMultVec(mtx, &pos, &pos);
		Vec target = pos;
		mSpringTarget.Set(target);
		mSpringPrevTarget.Set(target);
	}
	_8C.mValue.x        = 0.0f;
	_8C.mValue.y        = 0.0f;
	_8C.mValue.z        = 0.0f;
	_8C.mTarget.x       = 0.0f;
	_8C.mTarget.y       = 0.0f;
	_8C.mTarget.z       = 0.0f;
	_8C.mSpeed.x        = 0.0f;
	_8C.mSpeed.y        = 0.0f;
	_8C.mSpeed.z        = 0.0f;
	mTargetRestDistance = 60.0f;
	mRestDistance       = mTargetRestDistance;
	mFlatDistance       = mTargetRestDistance;
	SetFovStep(this, gCameraAreaState.mZoomLevel, 15.0f);
	mPadControlEnabled = 1;
	_DB0               = 1;
	mTurnInterpolation = NULL;
	mHeightInterp      = &mHeightEaseInOut;
	mHeightInterp->Start(13.0f, 13.0f, 1.0f);
	mMapArea         = -1;
	mPrevMapArea     = -1;
	mAbsHeightInterp = &mAbsHeightEaseInOut;
	mAbsHeightInterp->Start(mEye.y, mEye.y, 1.0f);
	mHeightMode    = 0;
	mUseAreaHeight = 0;
	mTargetHeight  = 13.0f;
	mTurnBlocked   = 1;
	ResetMapCameraTargetEye(this);
	mStatus.mFlags = 0;
	// mode bit 0x10000 (of SetMapCameraModeFlags's 0x70000): Update springs the eye after mSpringTarget
	mStatus.mFlags |= 0x10000;
	sMapCameraOrbitAtRestDistance = 0;
	mEyeYOffsetInterp             = &mEyeYOffsetEaseInOut;
	mEyeYOffsetEaseInOut.Start(0.0f, 0.0f, 1.0f);
	mEyeYOffset = 0.0f;
	SetMapCameraPreviousInterest(this, mInterest);
	// set by SetMapCameraKeepPreviousEye when memcardpos restores a saved camera: mPreviousEye then keeps camera->mEye.
	// the FALSE store only runs while the flag is already clear, so nothing here resets it
	if (!mKeepPreviousEye) {
		SetMapCameraPreviousEye(this, mInterest);
		mKeepPreviousEye = FALSE;
	}
	mPointerHeld   = FALSE;
	mOrbitReseed   = TRUE;
	mArrows[0]._14 = 0.0f;
	mArrows[1]._14 = 0.0f;
}

static inline void SetCenter(CCameraActionMap* map, const Vec& pos)
{
	map->mSpringTarget     = pos;
	map->mSpringPrevTarget = map->mSpringTarget;
}

static inline void ResetCenter(CCameraActionMap* map)
{
	Vec center = map->mSpringTarget;
	SetCenter(map, center);
	CVec eye              = map->mEye;
	map->mRestDistance    = PSVECDistance(&eye, &map->mSpringTarget);
	sMapCameraCenterReset = 1;
	map->mStatus.mFlags &= ~0x700;
	if (map->mEyeInterp != NULL) { // retail tests before clearing (0x801604C4)
		map->mEyeInterp = NULL;
	}
}

/**
 * @note Address: 0x80160220
 * @note Size: 0x2D4
 */
// NONMATCHING: the center reset's local Vec copy matches. The remaining delta is a second
// stack copy of the following CVec eye, plus register/scheduling differences in the mode blocks.
extern "C" void SetMapCameraModeFlags(CCameraActionMap* map, u32 flags)
{
	// eye modes (0x3E): 2 eases the yaw to mTargetYaw, 4 runs mTurnInterpolation, 8 turns by the pad
	// follow modes (0x70000): 0x10000 trails the target on a spring, 0x20000 orbits it (UpdateMapCameraOrbit)
	u32 eyeMode = flags & 0x3E;
	if (!map->mStatus.Check(eyeMode)) {
		map->mStatus.mFlags &= ~0x3E;
		map->mStatus.mFlags |= eyeMode;
		if (eyeMode != 4) {
			map->mTurnInterpolation = NULL;
		}
	}

	u32 mode = flags & 0x70000;
	if (!map->mStatus.Check(mode)) {
		map->mStatus.mFlags &= ~0x70000;
		map->mStatus.mFlags |= mode;
		if (mode == 0x20000) {
			ResetMapCameraTargetEye(map);
			if (sMapCameraOrbitAtRestDistance == 1) {
				map->mOrbitRadius = map->mTargetRestDistance;
			} else if (eyeMode == 4) {
				CVec pos          = map->mSpringTarget;
				CVec eye          = map->mEye;
				map->mOrbitRadius = PSVECDistance(&pos, &eye);
			} else {
				CVec target = map->mTarget->GetCameraTarget();
				CVec eye    = map->mEye;
				f32 dist    = PSVECDistance(&eye, &target);
				if (dist < 31.0f) {
					dist = 31.0f;
				}
				map->mOrbitRadius = dist;
			}
		} else if (mode == 0x10000) {
			ResetCenter(map);
		}
	}
}

// the probe the last unobstructed-yaw test cast, kept for the debug draw
extern "C" CVec sMapCameraTestEye;
extern "C" CVec sMapCameraTestCenter;

/**
 * @note Address: 0x80160520
 * @note Size: 0x268
 */
// NONMATCHING: retail builds the two probe CVecs in word-copied stack temporaries before storing
// them, and map and the vtable sit in swapped saved registers (83 %)
extern "C" BOOL TrySetUnobstructedMapCameraYaw(CCameraActionMap* map, f32 yaw)
{
	Vec center = *map->mTarget->GetPos();
	CVec angles(map->mAngle.x, yaw, map->mAngle.z);
	f32 angle = HALF_PI + angles.y;
	f32 pitch = angles.x;
	Vec pos;
	f32 distance;
	{
		CVec target(map->mTarget->GetCameraTarget());
		CVec eye(map->mEye);
		distance = PSVECDistance(&target, &eye);
	}
	pos.x                   = center.x + distance * std::cosf(angle) * std::cosf(pitch);
	pos.z                   = center.z + distance * -std::sinf(angle) * std::cosf(pitch);
	pos.y                   = center.y;
	sMapCameraTestEye       = CVec(pos);
	sMapCameraTestCenter    = CVec(center);
	map->mIntersectionCount = map->CalcIntersectVec(pos, center, map->mIntersectionEntries, 0);
	if (map->mIntersectionCount == 0 && TrySetMapCameraTargetYaw(map, yaw, 25.0f, 0.0f)) {
		SetMapCameraModeFlags(map, 0x20002);
		return TRUE;
	}
	return FALSE;
}

// the cinema bars' lower edge, clamped to the safe screen area; the arrows stay 58 + 40 + 50 above it
static inline f32 GetCineBarBottom()
{
	f32 y = gGame.mWipeCine.mHeight / 2.0f + gGame.mWipeCine.mY;
	if (y >= gScreenSafeBottom) {
		y = gScreenSafeBottom;
	}
	return y;
}

/**
 * @note Address: 0x80160788
 * @note Size: 0x494
 */
void CCameraActionMap::vf24()
{
	f32 y = GetCornerIconPosition(&gGame).y;
	if (y > GetCineBarBottom()) {
		y = GetCineBarBottom();
	}
	f32 minY = y - 58.0f - 40.0f - 50.0f;
	f32 zero = 0.0f;

	for (s32 i = 0; i < 2; i++) {
		CamEdgeArrow* arrow = &mArrows[i];
		if (mPointerHeld) {
			arrow->_10 = 0.5f;
		}
		if (arrow->mShown) {
			arrow->_10 = 1.0f;
		}
		// retail runs the same pulse call on both branches
		f32 pulse;
		if (mPointerHeld) {
			pulse = fn_8016A8D4(0.0f, 1.0f, arrow->_14, lbl_805A12B0);
		} else {
			pulse = fn_8016A8D4(0.0f, 1.0f, arrow->_14, lbl_805A12B0);
		}
		f32 fade = arrow->_10 * pulse;
		if (fade > zero) {
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
			gGame.mGs.mCurRenderState->SetAlphaUpdate(GX_FALSE);
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			if (i == 1) {
				gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_NONE);
			}
			gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[262], 0);
			CTexObj tex = gGame.mTpl.mTex;
			PSMTXTrans(tex.mTexMtx.mMtx[0], 0.0f, 0.0f, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			// white with the fade in the alpha byte
			u32 rgba             = 0xFFFFFFFF;
			((GXColor*)&rgba)->a = (u8)(((GXColor*)&rgba)->a * fade);
			f32 x                = gPointerScreenWidth / 2.0f + arrow->mPos.x;
			f32 arrowY           = gPointerScreenHeight / 2.0f + arrow->mPos.y;
			if (arrowY > minY) {
				arrowY = minY;
			}
			f32 w = GXGetTexObjWidth(&tex);
			f32 h = GXGetTexObjHeight(&tex);
			DrawGsTexturedQuad(&gGame.mGs, &tex, rgba, rgba, rgba, rgba, x, arrowY, zero, w, h, zero, arrow->mPos.z, zero);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			gGame.mGs.mCurRenderState->SetCullMode(GX_CULL_BACK);
		}
	}
}

/**
 * @note Address: 0x8015F0DC
 * @note Size: 0x270
 */
void CCameraActionMap::DrawDebug2d()
{
	static const f32 sDebugParams[4] = { 1.1f, 0.9f, 1.0f, 0.2f };
	fn_8016AA74(sDebugParams);
	f32 z = 0.0f;
	f32 y, x;
	x = gScreenSafeRight - 171;
	y = gScreenSafeTop + 18;
	DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B000000A0AREA(%3d,---)(%3d)\n", x, y, z, 9.0f, 9.0f, gCameraAreaState.mActiveArea,
	                       gCameraAreaState.mCurrentArea);
	DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B000000A0ACMAP2\n", x, y + 9.0f, z, 9.0f, 9.0f);
	DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B00000080CAMSTAT\t   :%8X\nCAMSTATPRE :%8X\n", x - 36.0f, y + 18.0f, z, 9.0f, 9.0f,
	                       mStatus.mFlags, mPreviousStatus.mFlags);
	DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B00000080CHR_InsideCll(%3d,%3d,%3d)\n", x - 63.0f, y + 36.0f, z, 9.0f, 9.0f, mInsideAreas[0],
	                       mInsideAreas[1], mInsideAreas[2]);
	DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B00000080CAM_InsideCll(%3d,%3d,%3d)\n", x - 63.0f, y + 45.0f, z, 9.0f, 9.0f, mInsideCll0,
	                       mInsideCll1, mInsideCll2);
	if (mInsideCllCount > 0) {
		DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B00000080CamHitCll(%3d,%3d,%3d)\n", x - 63.0f, y + 54.0f, z, 9.0f, 9.0f, mInsideCll0,
		                       mInsideCll1, mInsideCll2);
	}
	if (mNearestTargetEntry != NULL) {
		DrawGsStrokeTextFormat(&gGame.mGs, -1, "\\B00000080INTRSCT(%3d-%3d)\n", x - 63.0f, y + 63.0f, z, 9.0f, 9.0f,
		                       mNearestTargetEntry->mRecordIndex, mNearestTargetEntry->mSide);
	}
}

void CCameraActionMap::DrawDebug3d()
{
	DrawMapCameraAreaDebug3D(&gCameraAreaState, 1);
}

// the file's statics, in the bss layout retail has: five CVecs nothing references, the orbit's
// previous angle, one more unreferenced CVec and the two probes the unobstructed-yaw test keeps
extern "C" {
CVec lbl_8047F0C8;
CVec lbl_8047F0E8;
CVec lbl_8047F108;
CVec lbl_8047F128;
CVec lbl_8047F148;
Vec sPreviousMapOrbitAngle;
CVec lbl_8047F170(0.0f, 10.0f, 0.0f);
CVec sMapCameraTestEye;
CVec sMapCameraTestCenter;
CamCollisionPolygon gCameraCollisionPolygons[256];
}
