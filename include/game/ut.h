#ifndef _GAME_UT_H
#define _GAME_UT_H

#include <dolphin/gx/gxtypes.h>
#include "game/gmath.h"

// include game/hsd_jobj.h or dolphin/mtx.h first for Vec, types.h after the HSD headers
// breaks them, so this header can't pick one itself

// inline classes every module shares, names from RTTI and vtable symbols. each module
// gets its own weak copies. layouts are partial and the real header name is unknown,
// see game/cjobj.h

class CVec : public Vec {
public:
	CVec() { }
	CVec(f32 x, f32 y, f32 z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}
	CVec(const Vec& v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
	}
	CVec(const CVec& v)
	    : Vec(v)
	{
	}
	virtual ~CVec() { } // _08

	CVec operator+(Vec v) const; // in the dol, 0x8000F93C
	CVec operator-(Vec v) const; // in the dol, 0x800244A0
	CVec operator*(f32 s) const; // in the dol, 0x8000F9C0
	CVec operator*(Vec v) const; // in the dol, 0x8008C1F4, per component
	CVec operator/(f32 s) const; // in the dol, 0x8002442C

	bool operator!=(Vec v) const { return x != v.x || y != v.y || z != v.z; }

	void Set(f32 x, f32 y, f32 z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}
	void Set(const Vec& v) { Set(v.x, v.y, v.z); }
	// in game/cvec.h, they need the sdk's vec functions
	void operator+=(const Vec& v);
	f32 Length() const;
	CVec& Normalize();

	// _0C vtable
};

// size checks by hand, STATIC_ASSERT is in types.h and most includers come in through the HSD headers
typedef char CVec_size_check[sizeof(CVec) == 0x10 ? 1 : -1];

// a GXColor with a vtable, RTTI gives the base as "_GXColor"
class CColor : public _GXColor {
public:
	CColor() { }
	virtual ~CColor() { } // _08

	// _04 vtable
};
typedef char CColor_size_check[sizeof(CColor) == 0x8 ? 1 : -1];

class CStatus {
public:
	u32 mFlags; // _00
	// _04 vtable

	CStatus() { mFlags = 0; }
	virtual ~CStatus() { } // _08

	// any of the mask's bits set
	// the ternary keeps the test and branch, a plain bool folds to a bare clrlwi in shun's CmdCheckOnOffFlag
	bool Check(u32 mask) const { return (mFlags & mask) ? true : false; }
};
typedef char CStatus_size_check[sizeof(CStatus) == 0x8 ? 1 : -1];

#include "game/cstatus64.h"

#include "game/cpad.h"

// game/ut.cpp. hsd types go by struct tag since this header can't pull in the hsd headers,
// Mtx comes with Vec from whatever got included first
struct _HSD_JObj;
struct _HSD_Joint;
struct _HSD_AObj;

extern "C" {
// walls, lines and triangles, mostly flat on the xz plane
int ResolveCircleWallCollisionXZ(const Vec* fromPos, const Vec* toPos, const Vec* wallA, const Vec* wallB, Vec* push, Vec* pos,
                                 Vec* contact, f32* dist, f32 radius, f32 scale, int corners);
int IntersectLinesXZ(const Vec* a, const Vec* b, const Vec* c, const Vec* d, Vec* out, int check);
void GetOutwardPerpendicularToEdge(Vec* a, Vec* b, Vec* c, Vec* out);
int IntersectVerticalLineWithTrianglePlane(Vec* a, Vec* b, Vec* c, Vec* pos, Vec* out);
BOOL IsPointInsideQuadXZ(const Vec* pos, const Vec* a, const Vec* b, const Vec* c, const Vec* d);
BOOL IsPointInsideTriangleXZ(const Vec* pos, const Vec* a, const Vec* b, const Vec* c);
f32 SignedTriangleArea2XZ(Vec* a, Vec* b, Vec* c);
void CalcTriangleNormal(Vec* a, Vec* b, Vec* c, Vec* normal);

// jobj trees: blend two poses into a third, copy a pose, back to the rest pose, all to
// quaternions, and the first animation of each kind in a tree
void BlendJointTreePoses(_HSD_JObj* a, _HSD_JObj* b, _HSD_JObj* out, f32 t);
void CopyJointPoseTree(_HSD_JObj* src, _HSD_JObj* dst);
void RestoreJointTreePose(_HSD_Joint* joint, _HSD_JObj* jobj);
#if defined(VERSION_JP)
void RemoveJointTreeDisplayObjects(_HSD_JObj* jobj);
#endif
void ConvertJointTreeRotationToQuaternion(_HSD_JObj* jobj);
_HSD_AObj* FindPolygonAnimationInJointTree(_HSD_JObj* jobj);
_HSD_AObj* FindMaterialOrTextureAnimationInJointTree(_HSD_JObj* jobj);
_HSD_AObj* FindJointAnimationInTree(_HSD_JObj* jobj);

// free aram, and two matrix builders for billboards
u32 GetFreeARAMSize(void);
void BuildBillboardMatrixWithZRotation(Mtx src, Mtx dst, f32 angle);
void BuildBillboardMatrixFromUp(Mtx src, Mtx dst);

// sprintf into a static buffer, half width to full width shift jis, and hex digits
char* FormatToStaticBuffer(const char* fmt, ...);
char* ConvertHalfWidthToFullWidthSJIS(const char* src, char* dst);
u32 ParseHexDigits(const char* str, s32 len);
}

// the first shape, material or joint anim in a tree, whichever turns up first
// todo: cleanup
static inline _HSD_AObj* GetAObj(_HSD_JObj* jobj)
{
	_HSD_AObj* aobj;
	if (jobj != NULL) {
		if ((aobj = FindJointAnimationInTree(jobj)) != NULL) {
			return aobj;
		}
		if ((aobj = FindMaterialOrTextureAnimationInJointTree(jobj)) != NULL) {
			return aobj;
		}
		if ((aobj = FindPolygonAnimationInJointTree(jobj)) != NULL) {
			return aobj;
		}
	}
	return NULL;
}

#endif
