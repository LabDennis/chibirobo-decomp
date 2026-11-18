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
	bool operator==(Vec v) const { return x == v.x && y == v.y && z == v.z; }

	void Set(f32 x, f32 y, f32 z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}
	void Set(const Vec& v) { Set(v.x, v.y, v.z); }
	// in game/cvec.h, they need the sdk's vec functions
	void operator+=(const Vec& v);
	void operator-=(const Vec& v);
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
#if defined(VERSION_R24J01)
	// the Wii sets a color one changed component at a time (the object color setters around
	// 0x800AE340 do it on the member). The conversion runs that on the new object's own,
	// still unset components: CObj::Init and CObj::Load compare against registers nothing wrote
	CColor(GXColor c) { Set(c.r, c.g, c.b, c.a); }
	void Set(int r, int g, int b, int a)
	{
		if (this->r != r) {
			this->r = r;
		}
		if (this->g != g) {
			this->g = g;
		}
		if (this->b != b) {
			this->b = b;
		}
		if (this->a != a) {
			this->a = a;
		}
	}
#endif
	virtual ~CColor() { } // _08
#if defined(VERSION_R24J01)
	// a component is only written when it changes
	void Set(s32 r, s32 g, s32 b, s32 a)
	{
		if (this->r != r) {
			this->r = r;
		}
		if (this->g != g) {
			this->g = g;
		}
		if (this->b != b) {
			this->b = b;
		}
		if (this->a != a) {
			this->a = a;
		}
	}
	void Set(GXColor color) { Set(color.r, color.g, color.b, color.a); }
#endif

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
	void Clear() { mFlags = 0; }
};
typedef char CStatus_size_check[sizeof(CStatus) == 0x8 ? 1 : -1];

#if defined(VERSION_R24J01)
// the Wii splits the 32-bit CStatus like CStatus64 (cstatus64.h): RTTI names CStatus32, derived from CBitStatus32,
// each with a vtable of its own (0x802E6948, 0x802E6960). The base's constructor clears the flags before the derived
// vtable goes in, the order the inlined constructors show (FbSubwin, 0x80033E0C)
class CBitStatus32 {
public:
	CBitStatus32() { mFlags = 0; }

	u32 mFlags; // _00
	// _04 vtable

	virtual ~CBitStatus32() { } // _08

	bool Check(u32 mask) const { return (mFlags & mask) ? true : false; }
	void Clear() { mFlags = 0; }
};

class CStatus32 : public CBitStatus32 {
public:
	virtual ~CStatus32() { }
};
typedef char CStatus32_size_check[sizeof(CStatus32) == 0x8 ? 1 : -1];
#endif

#include "game/cstatus64.h"

#if defined(VERSION_R24J01)
#include "game/wii/cpad.h"
#else
#include "game/cpad.h"
#endif

// game/ut.cpp. hsd types go by struct tag since this header can't pull in the hsd headers,
// Mtx comes with Vec from whatever got included first
struct _HSD_JObj;
struct _HSD_Joint;
struct _HSD_AObj;

extern "C" {
// walls, lines and triangles, mostly flat on the xz plane
#if defined(VERSION_R24J01)
int ResolveCircleWallCollisionXZ(const Vec* fromPos, const Vec* toPos, const Vec* wallA, const Vec* wallB, Vec* pos, Vec* contact,
                                 Vec* push, int corners, f32 radius);
int IntersectLineWithPlane(Vec* p0, Vec* p1, Vec* point, Vec* normal, Vec* out);
// the Wii put the point first; the GameCube takes the triangle first
int IntersectVerticalLineWithTrianglePlane(Vec* pos, Vec* a, Vec* b, Vec* c, Vec* out);
#else
u32 ResolveCircleWallCollisionXZ(const Vec* fromPos, const Vec* toPos, const Vec* wallA, const Vec* wallB, Vec* push, Vec* pos,
                                 Vec* contact, f32* dist, f32 radius, f32 scale, int corners);
int IntersectVerticalLineWithTrianglePlane(Vec* a, Vec* b, Vec* c, Vec* pos, Vec* out);
#endif
int IntersectLinesXZ(const Vec* a, const Vec* b, const Vec* c, const Vec* d, Vec* out, int check);
void GetOutwardPerpendicularToEdge(Vec* a, Vec* b, Vec* c, Vec* out);
BOOL IsPointInsideQuadXZ(Vec* pos, Vec* a, Vec* b, Vec* c, Vec* d);
BOOL IsPointInsideTriangleXZ(Vec* pos, Vec* a, Vec* b, Vec* c);
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
