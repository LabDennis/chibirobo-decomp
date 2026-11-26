#include "game/hsd_jobj.h"
#include "game/hsd_dobj.h"
#include "game/ut.h"
#include "game/gmath.h"
extern "C" {
#include <sysdolphin/mtx.h>
#include <sysdolphin/jobj.h>
#include <sysdolphin/mobj.h>
#include <sysdolphin/tobj.h>
#include <dolphin/ar.h>
#include <dolphin/mtx/vec.h>
#include <dolphin/mtx/mtx.h>
#include "game/cvec.h"
}
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// geometry, joint and string helpers, backwards for deferred inlining

// half width characters and their full width twins, two bytes each in shift jis
static const u8* sHalfWidthCharacters = (const u8*)" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklm"
                                                   "nopqrstuvwxyz{|}~\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0"
                                                   "\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4"
                                                   "\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8"
                                                   "\xD9\xDA\xDB\xDC\xDD";
static const char* sFullWidthCharacters
    = "\x81\x40\x81\x49\x81\x68\x81\x94\x81\x90\x81\x93\x81\x95\x81\x66\x81\x69\x81\x6A\x81\x96\x81\x7B\x81\x43\x81\x7C\x81\x44\x81\x5E"
      "\x82\x4F\x82\x50\x82\x51\x82\x52\x82\x53\x82\x54\x82\x55\x82\x56\x82\x57\x82\x58\x81\x46\x81\x47\x81\x83\x81\x81\x81\x84\x81\x48"
      "\x81\x97\x82\x60\x82\x61\x82\x62\x82\x63\x82\x64\x82\x65\x82\x66\x82\x67\x82\x68\x82\x69\x82\x6A\x82\x6B\x82\x6C\x82\x6D\x82\x6E"
      "\x82\x6F\x82\x70\x82\x71\x82\x72\x82\x73\x82\x74\x82\x75\x82\x76\x82\x77\x82\x78\x82\x79\x81\x6D\x81\x8F\x81\x6E\x81\x4F\x81\x51"
      "\x81\x65\x82\x81\x82\x82\x82\x83\x82\x84\x82\x85\x82\x86\x82\x87\x82\x88\x82\x89\x82\x8A\x82\x8B\x82\x8C\x82\x8D\x82\x8E\x82\x8F"
      "\x82\x90\x82\x91\x82\x92\x82\x93\x82\x94\x82\x95\x82\x96\x82\x97\x82\x98\x82\x99\x82\x9A\x81\x6F\x81\x62\x81\x70\x81\x60\x81\x42"
      "\x81\x75\x81\x76\x81\x41\x81\x45\x83\x92\x83\x40\x83\x42\x83\x44\x83\x46\x83\x48\x83\x83\x83\x85\x83\x87\x83\x62\x81\x5B\x83\x41"
      "\x83\x43\x83\x45\x83\x47\x83\x49\x83\x4A\x83\x4C\x83\x4E\x83\x50\x83\x52\x83\x54\x83\x56\x83\x58\x83\x5A\x83\x5C\x83\x5E\x83\x60"
      "\x83\x63\x83\x65\x83\x67\x83\x69\x83\x6A\x83\x6B\x83\x6C\x83\x6D\x83\x6E\x83\x71\x83\x74\x83\x77\x83\x7A\x83\x7D\x83\x7E\x83\x80"
      "\x83\x81\x83\x82\x83\x84\x83\x86\x83\x88\x83\x89\x83\x8A\x83\x8B\x83\x8C\x83\x8D\x83\x8F\x83\x93";

static char sFullWidthTextBuffer[0x80];
// the formatted text, panics and the event debug lines all go through here
static char sFormattedTextBuffer[0x100];

extern "C" {
CVec ReflectVectorAboutNormal(const Vec* v, const Vec* normal);
}

// which side of the line ab p is on, looking down from above
static inline f32 LineSide(Vec& a, Vec& b, Vec& p)
{
	return p.x * (a.z - b.z) + a.x * (b.z - p.z) + b.x * (p.z - a.z);
}

// how far p is from the line ab. never negative, n is taken from p toward the line
static inline f32 LineDist(Vec* p, Vec* a, Vec* b)
{
	Vec n;
	GetOutwardPerpendicularToEdge(p, a, b, &n);
	PSVECNormalize(&n, &n);
	Vec d;
	PSVECSubtract(a, p, &d);
	return PSVECDotProduct(&n, &d);
}

// todo: copied inline, clean up
// where the line p0 to p1 meets the plane through point, as a fraction of the way along it
static inline f32 LinePlane(Vec* p0, Vec* p1, Vec* point, Vec* normal, Vec* out)
{
	Vec rel, dir;
	PSVECSubtract(p0, point, &rel);
	PSVECSubtract(p1, p0, &dir);
	f32 d = PSVECDotProduct(&dir, normal);
	f32 t = -1.0f * (PSVECDotProduct(&rel, normal) / d);
	PSVECScale(&dir, out, t);
	PSVECAdd(out, p0, out);
	return t;
}

extern "C" {
void BlendJointPose(HSD_JObj* a, HSD_JObj* b, HSD_JObj* out, f32 t);
void CopyJointPoseAsQuaternion(HSD_JObj* src, HSD_JObj* dst);
}

static void ReadJointRotationAsQuaternion(HSD_JObj* joint, Quaternion* rotation)
{
	HSD_JObjGetRotation(joint, rotation);
	if (!(joint->flags & USE_QUATERNION)) {
		Vec euler = { rotation->x, rotation->y, rotation->z };
		EulerToQuat(&euler, rotation);
	}
}

extern "C" {
// puts the joint's rotation, scale and position back on the jobj
inline void RestoreJointLocalPose(HSD_Joint* joint, HSD_JObj* jobj)
{
	if (jobj == NULL || joint == NULL) {
		return;
	}
	jobj->rotate.x  = joint->rotation.x;
	jobj->rotate.y  = joint->rotation.y;
	jobj->rotate.z  = joint->rotation.z;
	jobj->scale     = joint->scale;
	jobj->translate = joint->position;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
	jobj->flags &= ~USE_QUATERNION;
}

// turns the euler rotation into a quaternion
inline void ConvertJointRotationToQuaternion(HSD_JObj* jobj)
{
	if (!(jobj->flags & USE_QUATERNION)) {
		Quaternion q;
		ReadJointRotationAsQuaternion(jobj, &q);
		HSD_JObjSetRotation(jobj, &q);
		jobj->flags |= USE_QUATERNION;
	}
}
}

/**
 * @note Address: 0x80191310
 * @note Size: 0xC8
 */
// reads len hex digits, or the whole string when len is 0. any other character is skipped but
// still counts toward len
extern "C" u32 ParseHexDigits(const char* str, s32 len)
{
	u32 val = 0;
	if (len == 0) {
		len = strlen(str);
	}
	for (int i = 0; i < len; i++) {
		if (*str >= '0' && *str <= '9') {
			val = val * 16 + (*str - '0');
		} else if (*str >= 'A' && *str <= 'F') {
			val = val * 16 + (*str - 'A' + 10);
		} else if (*str >= 'a' && *str <= 'f') {
			val = val * 16 + (*str - 'a' + 10);
		}
		str++;
	}
	return val;
}

/**
 * @note Address: 0x80191268
 * @note Size: 0xA8
 */
// src with each character from sHalfWidthCharacters swapped for its full width twin, the rest copied
// as they are, into dst or a static buffer when dst is NULL
extern "C" char* ConvertHalfWidthToFullWidthSJIS(const char* src, char* dst)
{
	u8* out      = (u8*)(dst != NULL ? dst : sFullWidthTextBuffer);
	const u8* in = (const u8*)src;
	while (*in != 0) {
		*out = *in;
		for (int i = 0; sHalfWidthCharacters[i] != '\0'; i++) {
			if (*in == sHalfWidthCharacters[i]) {
				*out   = sFullWidthCharacters[i * 2];
				*++out = sFullWidthCharacters[i * 2 + 1];
				break;
			}
		}
		in++;
		*++out = 0;
	}
	return dst != NULL ? dst : sFullWidthTextBuffer;
}
/**
 * @note Address: 0x801911B4
 * @note Size: 0xB4
 */
extern "C" char* FormatToStaticBuffer(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsprintf(sFormattedTextBuffer, fmt, args);
	return sFormattedTextBuffer;
}

/**
 * @note Address: 0x80190E60
 * @note Size: 0x354
 */
// src turned so its z axis is (0, 0, 1). its y axis only loses its z part, scale and position stay
void BuildBillboardMatrixFromUp(Mtx src, Mtx dst)
{
	Vec x, y;
	Vec z  = { 0.0f, 0.0f, 1.0f };
	f32 sx = sqrtf(src[0][0] * src[0][0] + src[1][0] * src[1][0] + src[2][0] * src[2][0]);
	f32 sz = sqrtf(src[0][2] * src[0][2] + src[1][2] * src[1][2] + src[2][2] * src[2][2]);
	y.x    = src[0][1];
	y.y    = src[1][1];
	y.z    = src[2][1];
	f32 sy = PSVECMag(&y);
	C_VECCrossProduct(&y, &z, &x);
	C_VECCrossProduct(&z, &x, &y);
	sx /= PSVECMag(&x);
	sy /= PSVECMag(&y);
	dst[0][0] = sx * x.x;
	dst[1][0] = sx * x.y;
	dst[2][0] = sx * x.z;
	dst[0][1] = sy * y.x;
	dst[1][1] = sy * y.y;
	dst[2][1] = sy * y.z;
	dst[0][2] = sz * z.x;
	dst[1][2] = sz * z.y;
	dst[2][2] = sz * z.z;
	dst[0][3] = src[0][3];
	dst[1][3] = src[1][3];
	dst[2][3] = src[2][3];
}

/**
 * @note Address: 0x80190AF0
 * @note Size: 0x370
 */
// src's scale and position, its rotation replaced by angle about z
void BuildBillboardMatrixWithZRotation(Mtx src, Mtx dst, f32 angle)
{
	Mtx rot, scale;
	f32 sx = sqrtf(src[0][0] * src[0][0] + src[1][0] * src[1][0] + src[2][0] * src[2][0]);
	f32 sy = sqrtf(src[0][1] * src[0][1] + src[1][1] * src[1][1] + src[2][1] * src[2][1]);
	f32 sz = sqrtf(src[0][2] * src[0][2] + src[1][2] * src[1][2] + src[2][2] * src[2][2]);
	PSMTXScale(scale, sx, sy, sz);
	PSMTXRotRad(rot, 'z', angle);
	rot[0][3] = src[0][3];
	rot[1][3] = src[1][3];
	rot[2][3] = src[2][3];
	PSMTXConcat(rot, scale, dst);
}

/**
 * @note Address: 0x80190AB0
 * @note Size: 0x40
 */
// how much aram is still free, alloc a block and see where it lands
extern "C" u32 GetFreeARAMSize(void)
{
	u32 start = ARAlloc(0x20);
	ARFree(NULL);
	return ARGetSize() - start;
}

/**
 * @note Address: 0x801908F8
 * @note Size: 0x1B8
 */
// the first jobj aobj in the tree: a jobj, then its later siblings' trees, then its children
extern "C" HSD_AObj* FindJointAnimationInTree(HSD_JObj* jobj)
{
	HSD_AObj* aobj = NULL;
	for (; jobj != NULL; jobj = jobj->child) {
		if ((aobj = jobj->aobj) != NULL) {
			break;
		}
		if ((aobj = FindJointAnimationInTree(jobj->next)) != NULL) {
			break;
		}
	}
	return aobj;
}

/**
 * @note Address: 0x801905D4
 * @note Size: 0x30C
 */
// the first aobj of a material, or of its first texture when it has none, walking the tree like
// FindJointAnimationInTree
extern "C" HSD_AObj* FindMaterialOrTextureAnimationInJointTree(HSD_JObj* jobj)
{
	HSD_AObj* aobj = NULL;
	for (; jobj != NULL; jobj = jobj->child) {
		for (HSD_DObj* dobj = HSD_JObjGetDObj(jobj); dobj != NULL; dobj = HSD_DObjGetNext(dobj)) {
			HSD_MObj* mobj = HSD_DObjGetMObj(dobj);
			if (mobj) {
				if ((aobj = mobj->aobj)) {
					return aobj;
				}
				HSD_TObj* tobj = HSD_MObjGetTObj(mobj);
				if (tobj && (aobj = tobj->aobj)) {
					return aobj;
				}
			}
		}
		aobj = FindMaterialOrTextureAnimationInJointTree(jobj->next);
		if (aobj) {
			break;
		}
	}
	return aobj;
}
/**
 * @note Address: 0x801902C4
 * @note Size: 0x2E0
 */
// the first aobj on a dobj's first pobj, walking the tree like FindJointAnimationInTree. later pobjs
// of a dobj aren't looked at
extern "C" HSD_AObj* FindPolygonAnimationInJointTree(HSD_JObj* jobj)
{
	HSD_AObj* aobj = NULL;
	for (; jobj != NULL; jobj = jobj->child) {
		for (HSD_DObj* dobj = HSD_JObjGetDObj(jobj); dobj != NULL; dobj = HSD_DObjGetNext(dobj)) {
			HSD_PObj* pobj = HSD_DObjGetPObj(dobj);
			if (pobj && (aobj = pobj->aobj)) {
				return aobj;
			}
		}
		aobj = FindPolygonAnimationInJointTree(jobj->next);
		if (aobj) {
			break;
		}
	}
	return aobj;
}
/**
 * @note Address: 0x801920A8 in Japan, stripped elsewhere
 * @note Size: 0x40
 */
// drops a jobj's own dobjs
extern "C" inline void RemoveJointDisplayObjectsJP(HSD_JObj* jobj)
{
	if (jobj->u.dobj) {
		HSD_DObjRemoveAll(jobj->u.dobj);
		jobj->u.dobj = NULL;
	}
}

/**
 * @note Address: 0x80191E90 in Japan, stripped elsewhere
 * @note Size: 0x218
 */
// drops every dobj in a jobj tree, instances keep their children's
extern "C" void RemoveJointTreeDisplayObjects(HSD_JObj* jobj)
{
	RemoveJointDisplayObjectsJP(jobj);
	if (!(jobj->flags & INSTANCE)) {
		for (HSD_JObj* child = jobj->child; child; child = child->next) {
			RemoveJointTreeDisplayObjects(child);
		}
	}
}

/**
 * @note Address: 0x8018FD80
 * @note Size: 0x2A4
 */
// converts the joint and its immediate children to quaternion rotation
void ConvertJointTreeRotationToQuaternion(HSD_JObj* jobj)
{
	HSD_JObj* child;

	ConvertJointRotationToQuaternion(jobj);
	if (jobj->flags & INSTANCE) {
		return;
	}
	for (child = jobj->child; child != NULL; child = child->next) {
		ConvertJointTreeRotationToQuaternion(child);
	}
}

/**
 * @note Address: 0x8018F9B4
 * @note Size: 0x27C
 */
// back to the joint's own pose, the whole tree
void RestoreJointTreePose(HSD_Joint* joint, HSD_JObj* jobj)
{
	HSD_JObj* child;
	HSD_Joint* cjoint;

	if (jobj == NULL || joint == NULL) {
		return;
	}
	RestoreJointLocalPose(joint, jobj);
	if (jobj->flags & INSTANCE) {
		return;
	}
	for (child = jobj->child, cjoint = joint->child; child != NULL && cjoint != NULL; child = child->next, cjoint = cjoint->next) {
		RestoreJointTreePose(cjoint, child);
	}
}

/**
 * @note Address: 0x8018F6EC
 * @note Size: 0x2C8
 */
// one joint of the copy, the rotation ends up a quaternion
void CopyJointPoseAsQuaternion(HSD_JObj* src, HSD_JObj* dst)
{
	Vec v;
	Quaternion q;

	HSD_JObjGetTranslation(src, &v);
	HSD_JObjSetTranslate(dst, &v);
	HSD_JObjGetScale(src, &v);
	HSD_JObjSetScale(dst, &v);
	ReadJointRotationAsQuaternion(src, &q);
	HSD_JObjSetRotation(dst, &q);
}

/**
 * @note Address: 0x8018F658
 * @note Size: 0x94
 */
// copies the pose of a whole tree onto another
// NONMATCHING: retail repeats both child NULL tests at the top of the loop body in addition to its loop condition.
void CopyJointPoseTree(HSD_JObj* src, HSD_JObj* dst)
{
	HSD_JObj* s;
	HSD_JObj* d;

	if (src == NULL || dst == NULL) {
		return;
	}
	CopyJointPoseAsQuaternion(src, dst);
	if (src->flags & INSTANCE) {
		return;
	}
	for (s = src->child, d = dst->child; s != NULL && d != NULL; s = s->next, d = d->next) {
		CopyJointPoseTree(s, d);
	}
}

/**
 * @note Address: 0x8018F14C
 * @note Size: 0x50C
 */
// one joint of the blend, translate and scale linear, rotation slerped
void BlendJointPose(HSD_JObj* a, HSD_JObj* b, HSD_JObj* out, f32 t)
{
	f32 s = 1.0f - t;
	Vec va;
	Vec vb;
	Vec v;
	Quaternion q;
	Quaternion qa;
	Quaternion qb;

	HSD_JObjGetTranslation(a, &va);
	HSD_JObjGetTranslation(b, &vb);
	v.x = va.x * s + vb.x * t;
	v.y = va.y * s + vb.y * t;
	v.z = va.z * s + vb.z * t;
	HSD_JObjSetTranslate(out, &v);

	HSD_JObjGetScale(a, &va);
	HSD_JObjGetScale(b, &vb);
	v.x = va.x * s + vb.x * t;
	v.y = va.y * s + vb.y * t;
	v.z = va.z * s + vb.z * t;
	HSD_JObjSetScale(out, &v);

	ReadJointRotationAsQuaternion(a, &qa);
	ReadJointRotationAsQuaternion(b, &qb);
	// take the short way round
	if ((qa.x - qb.x) * (qa.x - qb.x) + (qa.y - qb.y) * (qa.y - qb.y) + (qa.z - qb.z) * (qa.z - qb.z) + (qa.w - qb.w) * (qa.w - qb.w)
	    > (qa.x + qb.x) * (qa.x + qb.x) + (qa.y + qb.y) * (qa.y + qb.y) + (qa.z + qb.z) * (qa.z + qb.z) + (qa.w + qb.w) * (qa.w + qb.w)) {
		qb.x = -qb.x;
		qb.y = -qb.y;
		qb.z = -qb.z;
		qb.w = -qb.w;
	}
	QuatSlerp(&qa, &qb, &q, t);
	HSD_JObjSetRotation(out, &q);
	out->flags |= USE_QUATERNION;
}
/**
 * @note Address: 0x8018EEC8
 * @note Size: 0x284
 */
// blends the poses of two trees into a third, t from a to b
void BlendJointTreePoses(HSD_JObj* a, HSD_JObj* b, HSD_JObj* out, f32 t)
{
	HSD_JObj* ca;
	HSD_JObj* cb;
	HSD_JObj* cout;

	if (a == NULL || b == NULL || out == NULL) {
		return;
	}
	BlendJointPose(a, b, out, t);
	if (a->flags & INSTANCE) {
		return;
	}
	for (ca = a->child, cb = b->child, cout = out->child; ca != NULL && cb != NULL && cout != NULL;
	     ca = ca->next, cb = cb->next, cout = cout->next) {
		BlendJointTreePoses(ca, cb, cout, t);
	}
}

/**
 * @note Address: 0x8018EE60
 * @note Size: 0x68
 */
void CalcTriangleNormal(Vec* a, Vec* b, Vec* c, Vec* normal)
{
	Vec ab, bc;
	PSVECSubtract(a, b, &ab);
	PSVECSubtract(b, c, &bc);
	C_VECCrossProduct(&bc, &ab, normal);
}

/**
 * @note Address: 0x8018EE2C
 * @note Size: 0x34
 */
f32 SignedTriangleArea2XZ(Vec* a, Vec* b, Vec* c)
{
	return a->x * (b->z - c->z) + b->x * (c->z - a->z) + c->x * (a->z - b->z);
}

/**
 * @note Address: 0x8018EC6C
 * @note Size: 0x1C0
 */
// whether pos lies inside abc seen from above, corners wound so CalcTriangleNormal points up. pos is
// out as soon as one edge's rel x edge points down: its dot with up, which is (0, -1, 0), goes positive
BOOL IsPointInsideTriangleXZ(Vec* pos, Vec* a, Vec* b, Vec* c)
{
	f32 ex = a->x - b->x;
	f32 ez = a->z - b->z;
	f32 rx = b->x - pos->x;
	f32 rz = b->z - pos->z;
	Vec up = { 0.0f, -1.0f, 0.0f };
	Vec edge, rel, n;

	edge.x = ex;
	edge.z = ez;
	edge.y = 0.0f;
	rel.x  = rx;
	rel.z  = rz;
	rel.y  = 0.0f;
	C_VECCrossProduct(&rel, &edge, &n);
	if (PSVECDotProduct(&up, &n) > 0.0f) {
		return FALSE;
	}

	ex     = b->x - c->x;
	ez     = b->z - c->z;
	rx     = c->x - pos->x;
	rz     = c->z - pos->z;
	edge.x = ex;
	edge.z = ez;
	edge.y = 0.0f;
	rel.x  = rx;
	rel.z  = rz;
	rel.y  = 0.0f;
	C_VECCrossProduct(&rel, &edge, &n);
	if (PSVECDotProduct(&up, &n) > 0.0f) {
		return FALSE;
	}

	ex     = c->x - a->x;
	ez     = c->z - a->z;
	rx     = a->x - pos->x;
	rz     = a->z - pos->z;
	edge.x = ex;
	edge.z = ez;
	edge.y = 0.0f;
	rel.x  = rx;
	rel.z  = rz;
	rel.y  = 0.0f;
	C_VECCrossProduct(&rel, &edge, &n);
	if (PSVECDotProduct(&up, &n) > 0.0f) {
		return FALSE;
	}
	return TRUE;
}

// IsPointInsideTriangleXZ
/**
 * @note Address: 0x8018EA50
 * @note Size: 0x21C
 */
// the IsPointInsideTriangleXZ test with four corners, so the quad has to be convex and wound the same way
BOOL IsPointInsideQuadXZ(Vec* pos, Vec* a, Vec* b, Vec* c, Vec* d)
{
	f32 ex = a->x - b->x;
	f32 ez = a->z - b->z;
	f32 rx = b->x - pos->x;
	f32 rz = b->z - pos->z;
	Vec up = { 0.0f, -1.0f, 0.0f };
	Vec edge, rel, n;

	edge.x = ex;
	edge.z = ez;
	edge.y = 0.0f;
	rel.x  = rx;
	rel.z  = rz;
	rel.y  = 0.0f;
	C_VECCrossProduct(&rel, &edge, &n);
	if (PSVECDotProduct(&up, &n) > 0.0f) {
		return FALSE;
	}

	ex     = b->x - c->x;
	ez     = b->z - c->z;
	rx     = c->x - pos->x;
	rz     = c->z - pos->z;
	edge.x = ex;
	edge.z = ez;
	edge.y = 0.0f;
	rel.x  = rx;
	rel.z  = rz;
	rel.y  = 0.0f;
	C_VECCrossProduct(&rel, &edge, &n);
	if (PSVECDotProduct(&up, &n) > 0.0f) {
		return FALSE;
	}

	ex     = c->x - d->x;
	ez     = c->z - d->z;
	rx     = d->x - pos->x;
	rz     = d->z - pos->z;
	edge.x = ex;
	edge.z = ez;
	edge.y = 0.0f;
	rel.x  = rx;
	rel.z  = rz;
	rel.y  = 0.0f;
	C_VECCrossProduct(&rel, &edge, &n);
	if (PSVECDotProduct(&up, &n) > 0.0f) {
		return FALSE;
	}

	ex     = d->x - a->x;
	ez     = d->z - a->z;
	rx     = a->x - pos->x;
	rz     = a->z - pos->z;
	edge.x = ex;
	edge.z = ez;
	edge.y = 0.0f;
	rel.x  = rx;
	rel.z  = rz;
	rel.y  = 0.0f;
	C_VECCrossProduct(&rel, &edge, &n);
	if (PSVECDotProduct(&up, &n) > 0.0f) {
		return FALSE;
	}
	return TRUE;
}

// IsPointInsideQuadXZ
/**
 * @note Address: 0x8018E8A4
 * @note Size: 0x1AC
 */
// out gets the point of abc's plane straight above or below pos. returns 1 when it lies at y 1 or -1,
// 2 between them and 3 beyond
int IntersectVerticalLineWithTrianglePlane(Vec* a, Vec* b, Vec* c, Vec* pos, Vec* out)
{
	Vec normal;
	CalcTriangleNormal(a, b, c, &normal);

	Vec top    = { pos->x, 1.0f, pos->z };
	Vec bottom = { pos->x, -1.0f, pos->z };
	f32 t      = LinePlane(&top, &bottom, a, &normal, out);

	if (Abs(t) <= 0.00001f || Abs(1.0f - t) <= 0.00001f) {
		return 1;
	}
	if (t > 0.0f && t < 1.0f) {
		return 2;
	}
	return 3;
}

// IntersectVerticalLineWithTrianglePlane
/**
 * @note Address: 0x8018E824
 * @note Size: 0x80
 */
// the direction square to bc pointing from a toward the line, so out of the triangle abc
extern "C" void GetOutwardPerpendicularToEdge(Vec* a, Vec* b, Vec* c, Vec* out)
{
	Vec bc;
	Vec ba;
	Vec n;
	PSVECSubtract(b, c, &bc);
	PSVECSubtract(b, a, &ba);
	C_VECCrossProduct(&ba, &bc, &n);
	C_VECCrossProduct(&bc, &n, out);
}

/**
 * @note Address: 0x8018E640
 * @note Size: 0x1E4
 */
extern "C" int IntersectLinesXZ(const Vec* a, const Vec* b, const Vec* c, const Vec* d, Vec* out, int check)
{
	f32 abx = b->x - a->x;
	f32 abz = b->z - a->z;
	f32 cdz = d->z - c->z;
	f32 cdx = d->x - c->x;
	f32 acx = c->x - a->x;
	f32 acz = c->z - a->z;

	f32 sideC = abx * acz - abz * acx;
	f32 sideD = abx * (d->z - a->z) - abz * (d->x - a->x);
	f32 sideA = cdx * (a->z - c->z) - cdz * (a->x - c->x);
	f32 sideB = cdx * (b->z - c->z) - cdz * (b->x - c->x);

	int hit;
	if (check == 1) {
		hit = sideC * sideD <= 0.0f && sideA * sideB <= 0.0f;
	} else {
		hit = true;
	}
	if (hit) {
		if (b->x == a->x) {
			out->x = a->x;
			out->z = c->z + (a->x - c->x) * ((d->z - c->z) / (d->x - c->x));
		} else if (d->x == c->x) {
			out->x = c->x;
			out->z = a->z + (c->x - a->x) * ((b->z - a->z) / (b->x - a->x));
		} else {
			f32 slopeAB = abz / abx;
			f32 slopeCD = cdz / cdx;
			if (slopeAB == slopeCD) {
				return 0;
			}
			f32 dx = (acz - slopeCD * acx) / (slopeAB - slopeCD);
			f32 dz = dx * slopeAB;
			out->x = dx + a->x;
			out->z = dz + a->z;
		}
		out->y = 0.0f;
		return 1;
	}
	return 0;
}
/**
 * @note Address: 0x8018D43C
 * @note Size: 0x1204
 */
// moves a circle of radius from from to to against the wall ab, flat on the xz plane.
// pos gets where it ends up, contact where it touched, dist how far the start is from the
// wall and push what's left of the move. returns 0 for no hit, 1 when it got stopped
// and 3 when it bounced, then push is the move reflected off the wall times scale
// the wall's two ends get checked like posts, corners picks the softer version of that
extern "C" int ResolveCircleWallCollisionXZ(const Vec* fromPos, const Vec* toPos, const Vec* wallA, const Vec* wallB, Vec* push, Vec* pos,
                                            Vec* contact, f32* dist, f32 radius, f32 scale, int corners)
{
	CVec from(*fromPos);
	from.y = 0.0f;
	CVec to(*toPos);
	to.y     = 0.0f;
	f32 len  = (to - from).Length();
	CVec dir = (to - from).Normalize();
	CVec a(*wallA);
	a.y = 0.0f;
	CVec b(*wallB);
	b.y = 0.0f;
	CVec n;
	GetOutwardPerpendicularToEdge(&from, &a, &b, &n);
	PSVECNormalize(&n, &n);
	CVec toEdge = to + n * radius;
	int ret     = 0;

	// one sided: from must be on the wall's LineSide <= 0 and the circle at to touch or cross it
	if (LineSide(a, b, toEdge) >= 0.0f && LineSide(a, b, from) <= 0.0f) {
		CVec v;
		CVec hit;
		if (IntersectLinesXZ(&from, &toEdge, &a, &b, &hit, 1)) {
			if (contact) {
				*contact = hit;
			}
			if (dist && *dist < 0.0f) {
				*dist = LineDist(&from, &a, &b);
			}
			CVec end;
			IntersectLinesXZ(&toEdge, &to, &a, &b, &end, 0);
			*pos   = end - n * radius;
			pos->y = fromPos->y;
			if (dist && ABS(*dist - LineDist(&from, &a, &b)) < 0.1f) {
				push->x = 0.0f;
				push->y = 0.0f;
				push->z = 0.0f;
				ret     = 1;
			} else {
				*push = ReflectVectorAboutNormal(&dir, &n) * len * scale;
				ret   = 3;
			}
		} else if (corners) {
			if (PSVECDistance(&to, &a) <= radius) {
				if (contact) {
					*contact = a;
				}
				v      = (to - a).Normalize();
				*pos   = a + v * radius + (to - from);
				pos->y = fromPos->y;
				*push  = len > 0.3f ? to - from : CVec(0.0f, 0.0f, 0.0f);
				ret    = 1;
			}
			if (PSVECDistance(&to, &b) <= radius) {
				if (contact) {
					*contact = b;
				}
				v      = (to - b).Normalize();
				*pos   = b + v * radius + (to - from);
				pos->y = fromPos->y;
				*push  = len > 0.3f ? to - from : CVec(0.0f, 0.0f, 0.0f);
				ret    = 1;
			}
		} else {
			if (PSVECDistance(&to, &a) < radius) {
				if (contact) {
					*contact = a;
				}
				v      = (to - a).Normalize();
				*pos   = a + v * radius * 1.001f;
				pos->y = fromPos->y;
				if (PSVECDistance(&from, &a) >= 0.01f + radius) {
					*push = v * len * scale;
					ret   = 3;
				} else {
					IntersectLinesXZ(&from, &toEdge, &a, &b, &hit, 0);
					CVec moveEnd     = hit + dir * len;
					CVec alongNormal = moveEnd - n;
					CVec slideEnd;
					IntersectLinesXZ(&moveEnd, &alongNormal, &a, &b, &slideEnd, 0);
					v = slideEnd - hit;
					if (v.Length() > len) {
						*push = (slideEnd - hit).Normalize() * len;
					} else {
						*push = v;
					}
					ret = 1;
				}
			} else if (PSVECDistance(&to, &b) < radius) {
				if (contact) {
					*contact = b;
				}
				v      = (to - b).Normalize();
				*pos   = b + v * radius * 1.001f;
				pos->y = fromPos->y;
				if (PSVECDistance(&from, &b) >= 0.01f + radius) {
					*push = v * len * scale;
					ret   = 3;
				} else {
					IntersectLinesXZ(&from, &toEdge, &a, &b, &hit, 0);
					CVec moveEnd     = hit + dir * len;
					CVec alongNormal = moveEnd - n;
					CVec slideEnd;
					IntersectLinesXZ(&moveEnd, &alongNormal, &a, &b, &slideEnd, 0);
					v = slideEnd - hit;
					if (v.Length() > len) {
						*push = (slideEnd - hit).Normalize() * len;
					} else {
						*push = v;
					}
					ret = 1;
				}
			}
		}
	}
	if (dist) {
		*dist = LineDist(&from, &a, &b);
	}
	return ret;
}
