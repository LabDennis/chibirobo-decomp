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

// the sdk calls the hsd header tree doesn't declare
extern "C" {
void C_VECCrossProduct(const Vec* a, const Vec* b, Vec* axb);
void PSMTXRotRad(Mtx m, char axis, f32 rad);
}

// geometry, joint and string helpers, backwards for deferred inlining

extern "C" {
HSD_DObj* HSD_DObjGetNext__FP9_HSD_DObj(HSD_DObj* dobj);
HSD_PObj* HSD_DObjGetPObj__FP9_HSD_DObj(HSD_DObj* dobj);
HSD_MObj* HSD_DObjGetMObj__FP9_HSD_DObj(HSD_DObj* dobj);
}

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

// how far p is from the line ab, signed
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
		Vec euler;
		Quaternion q;
		// reads four floats into a three float vec, w lands past it. retail does this too
		HSD_JObjGetRotation(jobj, (Quaternion*)&euler);
		EulerToQuat(&euler, &q);
		HSD_JObjSetRotation(jobj, &q);
		jobj->flags |= USE_QUATERNION;
	}
}
}

/**
 * @note Address: 0x80191310
 * @note Size: 0xC8
 */
// reads len hex digits, or the whole string when len is 0
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
/*
    cmplwi   r4, 0x0
    beq      .L_80191278
    mr       r7, r4
    b        .L_801912E8
.L_80191278:
    lis      r5, sFullWidthTextBuffer@ha
    addi     r0, r5, sFullWidthTextBuffer@l
    mr       r7, r0
    b        .L_801912E8
.L_80191288:
    stb      r0, 0x0(r7)
    li       r8, 0x0
    lwz      r5, sHalfWidthCharacters@sda21(r0)
    b        .L_801912D0
.L_80191298:
    lbz      r0, 0x0(r3)
    cmplw    r0, r6
    bne      .L_801912C8
    lwz      r5, sFullWidthCharacters@sda21(r0)
    slwi     r6, r8, 1
    addi     r0, r6, 0x1
    lbzx     r5, r5, r6
    stb      r5, 0x0(r7)
    lwz      r5, sFullWidthCharacters@sda21(r0)
    lbzx     r0, r5, r0
    stbu     r0, 0x1(r7)
    b        .L_801912DC
.L_801912C8:
    addi     r5, r5, 0x1
    addi     r8, r8, 0x1
.L_801912D0:
    lbz      r6, 0x0(r5)
    cmpwi    r6, 0x0
    bne      .L_80191298
.L_801912DC:
    li       r0, 0x0
    addi     r3, r3, 0x1
    stbu     r0, 0x1(r7)
.L_801912E8:
    lbz      r0, 0x0(r3)
    cmplwi   r0, 0x0
    bne      .L_80191288
    cmplwi   r4, 0x0
    beq      .L_80191304
    mr       r3, r4
    blr
.L_80191304:
    lis      r3, sFullWidthTextBuffer@ha
    addi     r3, r3, sFullWidthTextBuffer@l
    blr
*/

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
 * @note Address: 0x801908E0
 * @note Size: 0x18
 */
extern "C" HSD_MObj* HSD_DObjGetMObj__FP9_HSD_DObj(HSD_DObj* dobj)
{
	return dobj != NULL ? dobj->mobj : NULL;
}

/**
 * @note Address: 0x801905D4
 * @note Size: 0x30C
 */
extern "C" HSD_AObj* FindMaterialOrTextureAnimationInJointTree(HSD_JObj* jobj)
{
	HSD_AObj* aobj = NULL;
	for (; jobj != NULL; jobj = jobj->child) {
		for (HSD_DObj* dobj = HSD_JObjGetDObj(jobj); dobj != NULL; dobj = HSD_DObjGetNext(dobj)) {
			HSD_MObj* mobj = HSD_DObjGetMObj(dobj);
			if (mobj != NULL) {
				if (mobj->aobj != NULL) {
					return mobj->aobj;
				}
				HSD_TObj* tobj = HSD_MObjGetTObj(mobj);
				if (tobj != NULL && tobj->aobj != NULL) {
					return tobj->aobj;
				}
			}
		}
		if ((aobj = FindMaterialOrTextureAnimationInJointTree(jobj->next)) != NULL) {
			break;
		}
	}
	return aobj;
}
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    stw      r0, 0x24(r1)
    li       r0, 0x0
    stmw     r26, 0x8(r1)
    mr       r29, r3
    b        .L_801908C0
.L_801905F0:
    mr       r3, r29
    bl       HSD_JObjGetDObj
    mr       r26, r3
    b        .L_80190664
.L_80190600:
    cmplwi   r26, 0x0
    beq      .L_80190610
    lwz      r4, 0x8(r26)
    b        .L_80190614
.L_80190610:
    li       r4, 0x0
.L_80190614:
    cmplwi   r4, 0x0
    beq      .L_8019064C
    lwz      r3, 0x14(r4)
    cmplwi   r3, 0x0
    beq      .L_8019062C
    b        .L_801908CC
.L_8019062C:
    mr       r3, r4
    bl       HSD_MObjGetTObj
    cmplwi   r3, 0x0
    beq      .L_8019064C
    lwz      r3, 0x64(r3)
    cmplwi   r3, 0x0
    beq      .L_8019064C
    b        .L_801908CC
.L_8019064C:
    cmplwi   r26, 0x0
    beq      .L_8019065C
    lwz      r0, 0x4(r26)
    b        .L_80190660
.L_8019065C:
    li       r0, 0x0
.L_80190660:
    mr       r26, r0
.L_80190664:
    cmplwi   r26, 0x0
    bne      .L_80190600
    lwz      r30, 0x8(r29)
    li       r0, 0x0
    b        .L_801908AC
.L_80190678:
    mr       r3, r30
    bl       HSD_JObjGetDObj
    mr       r26, r3
    b        .L_801906E4
.L_80190688:
    cmplwi   r26, 0x0
    beq      .L_80190698
    lwz      r3, 0x8(r26)
    b        .L_8019069C
.L_80190698:
    li       r3, 0x0
.L_8019069C:
    cmplwi   r3, 0x0
    beq      .L_801906D0
    lwz      r0, 0x14(r3)
    cmplwi   r0, 0x0
    beq      .L_801906B4
    b        .L_801908B4
.L_801906B4:
    bl       HSD_MObjGetTObj
    cmplwi   r3, 0x0
    beq      .L_801906D0
    lwz      r0, 0x64(r3)
    cmplwi   r0, 0x0
    beq      .L_801906D0
    b        .L_801908B4
.L_801906D0:
    cmplwi   r26, 0x0
    beq      .L_801906E0
    lwz      r26, 0x4(r26)
    b        .L_801906E4
.L_801906E0:
    li       r26, 0x0
.L_801906E4:
    cmplwi   r26, 0x0
    bne      .L_80190688
    lwz      r31, 0x8(r30)
    li       r0, 0x0
    b        .L_80190898
.L_801906F8:
    mr       r3, r31
    bl       HSD_JObjGetDObj
    mr       r26, r3
    b        .L_80190764
.L_80190708:
    cmplwi   r26, 0x0
    beq      .L_80190718
    lwz      r3, 0x8(r26)
    b        .L_8019071C
.L_80190718:
    li       r3, 0x0
.L_8019071C:
    cmplwi   r3, 0x0
    beq      .L_80190750
    lwz      r0, 0x14(r3)
    cmplwi   r0, 0x0
    beq      .L_80190734
    b        .L_801908A0
.L_80190734:
    bl       HSD_MObjGetTObj
    cmplwi   r3, 0x0
    beq      .L_80190750
    lwz      r0, 0x64(r3)
    cmplwi   r0, 0x0
    beq      .L_80190750
    b        .L_801908A0
.L_80190750:
    cmplwi   r26, 0x0
    beq      .L_80190760
    lwz      r26, 0x4(r26)
    b        .L_80190764
.L_80190760:
    li       r26, 0x0
.L_80190764:
    cmplwi   r26, 0x0
    bne      .L_80190708
    lwz      r28, 0x8(r31)
    li       r0, 0x0
    b        .L_80190884
.L_80190778:
    mr       r3, r28
    bl       HSD_JObjGetDObj
    mr       r26, r3
    b        .L_801907E4
.L_80190788:
    cmplwi   r26, 0x0
    beq      .L_80190798
    lwz      r3, 0x8(r26)
    b        .L_8019079C
.L_80190798:
    li       r3, 0x0
.L_8019079C:
    cmplwi   r3, 0x0
    beq      .L_801907D0
    lwz      r0, 0x14(r3)
    cmplwi   r0, 0x0
    beq      .L_801907B4
    b        .L_8019088C
.L_801907B4:
    bl       HSD_MObjGetTObj
    cmplwi   r3, 0x0
    beq      .L_801907D0
    lwz      r0, 0x64(r3)
    cmplwi   r0, 0x0
    beq      .L_801907D0
    b        .L_8019088C
.L_801907D0:
    cmplwi   r26, 0x0
    beq      .L_801907E0
    lwz      r26, 0x4(r26)
    b        .L_801907E4
.L_801907E0:
    li       r26, 0x0
.L_801907E4:
    cmplwi   r26, 0x0
    bne      .L_80190788
    lwz      r26, 0x8(r28)
    li       r3, 0x0
    b        .L_8019086C
.L_801907F8:
    mr       r3, r26
    bl       HSD_JObjGetDObj
    mr       r27, r3
    b        .L_80190850
.L_80190808:
    mr       r3, r27
    bl       HSD_DObjGetMObj__FP9_HSD_DObj
    cmplwi   r3, 0x0
    beq      .L_80190844
    lwz      r0, 0x14(r3)
    cmplwi   r0, 0x0
    beq      .L_80190828
    b        .L_80190878
.L_80190828:
    bl       HSD_MObjGetTObj
    cmplwi   r3, 0x0
    beq      .L_80190844
    lwz      r0, 0x64(r3)
    cmplwi   r0, 0x0
    beq      .L_80190844
    b        .L_80190878
.L_80190844:
    mr       r3, r27
    bl       HSD_DObjGetNext__FP9_HSD_DObj
    mr       r27, r3
.L_80190850:
    cmplwi   r27, 0x0
    bne      .L_80190808
    lwz      r3, 0x8(r26)
    bl       FindMaterialOrTextureAnimationInJointTree
    cmplwi   r3, 0x0
    bne      .L_80190874
    lwz      r26, 0x10(r26)
.L_8019086C:
    cmplwi   r26, 0x0
    bne      .L_801907F8
.L_80190874:
    mr       r0, r3
.L_80190878:
    cmplwi   r0, 0x0
    bne      .L_8019088C
    lwz      r28, 0x10(r28)
.L_80190884:
    cmplwi   r28, 0x0
    bne      .L_80190778
.L_8019088C:
    cmplwi   r0, 0x0
    bne      .L_801908A0
    lwz      r31, 0x10(r31)
.L_80190898:
    cmplwi   r31, 0x0
    bne      .L_801906F8
.L_801908A0:
    cmplwi   r0, 0x0
    bne      .L_801908B4
    lwz      r30, 0x10(r30)
.L_801908AC:
    cmplwi   r30, 0x0
    bne      .L_80190678
.L_801908B4:
    cmplwi   r0, 0x0
    bne      .L_801908C8
    lwz      r29, 0x10(r29)
.L_801908C0:
    cmplwi   r29, 0x0
    bne      .L_801905F0
.L_801908C8:
    mr       r3, r0
.L_801908CC:
    lmw      r26, 0x8(r1)
    lwz      r0, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: 0x801905BC
 * @note Size: 0x18
 */
extern "C" HSD_PObj* HSD_DObjGetPObj__FP9_HSD_DObj(HSD_DObj* dobj)
{
	return dobj != NULL ? dobj->pobj : NULL;
}

/**
 * @note Address: 0x801905A4
 * @note Size: 0x18
 */
extern "C" HSD_DObj* HSD_DObjGetNext__FP9_HSD_DObj(HSD_DObj* dobj)
{
	return dobj != NULL ? dobj->next : NULL;
}

/**
 * @note Address: 0x801902C4
 * @note Size: 0x2E0
 */
extern "C" HSD_AObj* FindPolygonAnimationInJointTree(HSD_JObj* jobj)
{
	HSD_AObj* aobj = NULL;
	for (; jobj != NULL; jobj = jobj->child) {
		for (HSD_DObj* dobj = HSD_JObjGetDObj(jobj); dobj != NULL; dobj = HSD_DObjGetNext(dobj)) {
			HSD_PObj* pobj = HSD_DObjGetPObj(dobj);
			if (pobj != NULL && (aobj = pobj->aobj) != NULL) {
				return aobj;
			}
		}
		if ((aobj = FindPolygonAnimationInJointTree(jobj->next)) != NULL) {
			break;
		}
	}
	return aobj;
}
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    stw      r0, 0x34(r1)
    li       r0, 0x0
    stmw     r25, 0x14(r1)
    mr       r28, r3
    b        .L_80190584
.L_801902E0:
    mr       r3, r28
    bl       HSD_JObjGetDObj
    b        .L_80190330
.L_801902EC:
    cmplwi   r3, 0x0
    beq      .L_801902FC
    lwz      r4, 0xc(r3)
    b        .L_80190300
.L_801902FC:
    li       r4, 0x0
.L_80190300:
    cmplwi   r4, 0x0
    beq      .L_8019031C
    lwz      r0, 0x18(r4)
    cmplwi   r0, 0x0
    beq      .L_8019031C
    mr       r3, r0
    b        .L_80190590
.L_8019031C:
    cmplwi   r3, 0x0
    beq      .L_8019032C
    lwz      r3, 0x4(r3)
    b        .L_80190330
.L_8019032C:
    li       r3, 0x0
.L_80190330:
    cmplwi   r3, 0x0
    bne      .L_801902EC
    lwz      r29, 0x8(r28)
    li       r0, 0x0
    b        .L_80190570
.L_80190344:
    mr       r3, r29
    bl       HSD_JObjGetDObj
    b        .L_80190390
.L_80190350:
    cmplwi   r3, 0x0
    beq      .L_80190360
    lwz      r4, 0xc(r3)
    b        .L_80190364
.L_80190360:
    li       r4, 0x0
.L_80190364:
    cmplwi   r4, 0x0
    beq      .L_8019037C
    lwz      r0, 0x18(r4)
    cmplwi   r0, 0x0
    beq      .L_8019037C
    b        .L_80190578
.L_8019037C:
    cmplwi   r3, 0x0
    beq      .L_8019038C
    lwz      r3, 0x4(r3)
    b        .L_80190390
.L_8019038C:
    li       r3, 0x0
.L_80190390:
    cmplwi   r3, 0x0
    bne      .L_80190350
    lwz      r31, 0x8(r29)
    li       r0, 0x0
    b        .L_8019055C
.L_801903A4:
    mr       r3, r31
    bl       HSD_JObjGetDObj
    b        .L_801903F0
.L_801903B0:
    cmplwi   r3, 0x0
    beq      .L_801903C0
    lwz      r4, 0xc(r3)
    b        .L_801903C4
.L_801903C0:
    li       r4, 0x0
.L_801903C4:
    cmplwi   r4, 0x0
    beq      .L_801903DC
    lwz      r0, 0x18(r4)
    cmplwi   r0, 0x0
    beq      .L_801903DC
    b        .L_80190564
.L_801903DC:
    cmplwi   r3, 0x0
    beq      .L_801903EC
    lwz      r3, 0x4(r3)
    b        .L_801903F0
.L_801903EC:
    li       r3, 0x0
.L_801903F0:
    cmplwi   r3, 0x0
    bne      .L_801903B0
    lwz      r30, 0x8(r31)
    li       r0, 0x0
    b        .L_80190548
.L_80190404:
    mr       r3, r30
    bl       HSD_JObjGetDObj
    b        .L_80190450
.L_80190410:
    cmplwi   r3, 0x0
    beq      .L_80190420
    lwz      r4, 0xc(r3)
    b        .L_80190424
.L_80190420:
    li       r4, 0x0
.L_80190424:
    cmplwi   r4, 0x0
    beq      .L_8019043C
    lwz      r0, 0x18(r4)
    cmplwi   r0, 0x0
    beq      .L_8019043C
    b        .L_80190550
.L_8019043C:
    cmplwi   r3, 0x0
    beq      .L_8019044C
    lwz      r3, 0x4(r3)
    b        .L_80190450
.L_8019044C:
    li       r3, 0x0
.L_80190450:
    cmplwi   r3, 0x0
    bne      .L_80190410
    lwz      r27, 0x8(r30)
    li       r3, 0x0
    b        .L_80190530
.L_80190464:
    mr       r3, r27
    bl       HSD_JObjGetDObj
    b        .L_801904B0
.L_80190470:
    cmplwi   r3, 0x0
    beq      .L_80190480
    lwz      r4, 0xc(r3)
    b        .L_80190484
.L_80190480:
    li       r4, 0x0
.L_80190484:
    cmplwi   r4, 0x0
    beq      .L_8019049C
    lwz      r0, 0x18(r4)
    cmplwi   r0, 0x0
    beq      .L_8019049C
    b        .L_8019053C
.L_8019049C:
    cmplwi   r3, 0x0
    beq      .L_801904AC
    lwz      r3, 0x4(r3)
    b        .L_801904B0
.L_801904AC:
    li       r3, 0x0
.L_801904B0:
    cmplwi   r3, 0x0
    bne      .L_80190470
    lwz      r25, 0x8(r27)
    li       r3, 0x0
    b        .L_8019051C
.L_801904C4:
    mr       r3, r25
    bl       HSD_JObjGetDObj
    mr       r26, r3
    b        .L_80190500
.L_801904D4:
    mr       r3, r26
    bl       HSD_DObjGetPObj__FP9_HSD_DObj
    cmplwi   r3, 0x0
    beq      .L_801904F4
    lwz      r3, 0x18(r3)
    cmplwi   r3, 0x0
    beq      .L_801904F4
    b        .L_80190524
.L_801904F4:
    mr       r3, r26
    bl       HSD_DObjGetNext__FP9_HSD_DObj
    mr       r26, r3
.L_80190500:
    cmplwi   r26, 0x0
    bne      .L_801904D4
    lwz      r3, 0x8(r25)
    bl       FindPolygonAnimationInJointTree
    cmplwi   r3, 0x0
    bne      .L_80190524
    lwz      r25, 0x10(r25)
.L_8019051C:
    cmplwi   r25, 0x0
    bne      .L_801904C4
.L_80190524:
    cmplwi   r3, 0x0
    bne      .L_80190538
    lwz      r27, 0x10(r27)
.L_80190530:
    cmplwi   r27, 0x0
    bne      .L_80190464
.L_80190538:
    mr       r0, r3
.L_8019053C:
    cmplwi   r0, 0x0
    bne      .L_80190550
    lwz      r30, 0x10(r30)
.L_80190548:
    cmplwi   r30, 0x0
    bne      .L_80190404
.L_80190550:
    cmplwi   r0, 0x0
    bne      .L_80190564
    lwz      r31, 0x10(r31)
.L_8019055C:
    cmplwi   r31, 0x0
    bne      .L_801903A4
.L_80190564:
    cmplwi   r0, 0x0
    bne      .L_80190578
    lwz      r29, 0x10(r29)
.L_80190570:
    cmplwi   r29, 0x0
    bne      .L_80190344
.L_80190578:
    cmplwi   r0, 0x0
    bne      .L_8019058C
    lwz      r28, 0x10(r28)
.L_80190584:
    cmplwi   r28, 0x0
    bne      .L_801902E0
.L_8019058C:
    mr       r3, r0
.L_80190590:
    lmw      r25, 0x14(r1)
    lwz      r0, 0x34(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: 0x801920A8 in Japan, stripped elsewhere
 * @note Size: 0x40
 */
// drops a jobj's own dobjs
extern "C" inline void RemoveJointDisplayObjectsJP(HSD_JObj* jobj)
{
	if (jobj->u.dobj != NULL) {
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
		for (HSD_JObj* child = jobj->child; child != NULL; child = child->next) {
			RemoveJointTreeDisplayObjects(child);
		}
	}
}

/**
 * @note Address: 0x8019019C
 * @note Size: 0x128
 */

/**
 * @note Address: 0x80190178
 * @note Size: 0x24
 */
// the compiler's Quaternion copy assignment

/**
 * @note Address: 0x801900F8
 * @note Size: 0x80
 */
// HSD_JObjGetRotation(_HSD_JObj*, Quaternion*)

/**
 * @note Address: 0x80190024
 * @note Size: 0xD4
 */
// HSD_JObjSetRotation(_HSD_JObj*, Quaternion*)

/**
 * @note Address: 0x8018FD80
 * @note Size: 0x2A4
 */
// every jobj in the tree to quaternion rotation
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
 * @note Address: 0x8018FCA0
 * @note Size: 0xE0
 */

/**
 * @note Address: 0x8018FC30
 * @note Size: 0x70
 */
// HSD_JObjSetMtxDirty(_HSD_JObj*)

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
	for (child = jobj->child, cjoint = joint->child; child != NULL && cjoint != NULL;
	     child = child->next, cjoint = cjoint != NULL ? cjoint->next : NULL) {
		if (child == NULL || cjoint == NULL) {
			break;
		}
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
	if (src->flags & USE_QUATERNION) {
		HSD_JObjGetRotation(src, &q);
	} else {
		HSD_JObjGetRotation(src, (Quaternion*)&v);
		EulerToQuat(&v, &q);
	}
	HSD_JObjSetRotation(dst, &q);
}

/**
 * @note Address: 0x8018F658
 * @note Size: 0x94
 */
// copies the pose of a whole tree onto another
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
		if (s == NULL || d == NULL) {
			break;
		}
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

	// the euler reads reuse v, w lands past it again
	if (a->flags & USE_QUATERNION) {
		HSD_JObjGetRotation(a, &qa);
	} else {
		HSD_JObjGetRotation(a, (Quaternion*)&v);
		EulerToQuat(&v, &qa);
	}
	if (b->flags & USE_QUATERNION) {
		HSD_JObjGetRotation(b, &qb);
	} else {
		HSD_JObjGetRotation(b, (Quaternion*)&v);
		EulerToQuat(&v, &qb);
	}
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
/*
    stwu     r1, -0xb0(r1)
    mflr     r0
    stw      r0, 0xb4(r1)
    stfd     f31, 0xa0(r1)
    psq_st   f31, 0xa8(r1), 0, qr0
    stfd     f30, 0x90(r1)
    psq_st   f30, 0x98(r1), 0, qr0
    stfd     f29, 0x80(r1)
    psq_st   f29, 0x88(r1), 0, qr0
    stfd     f28, 0x70(r1)
    psq_st   f28, 0x78(r1), 0, qr0
    stfd     f27, 0x60(r1)
    psq_st   f27, 0x68(r1), 0, qr0
    stw      r31, 0x5c(r1)
    stw      r30, 0x58(r1)
    stw      r29, 0x54(r1)
    fmr      f30, f1
    lfs      f0, lbl_8065F494@sda21(r0)
    mr.      r29, r3
    mr       r30, r4
    mr       r31, r5
    fsubs    f31, f0, f30
    bne      .L_8018F1B8
    li       r3, lbl_8065F498@sda21
    li       r4, 0x3e4
    li       r5, lbl_8065F4A0@sda21
    bl       __assert
.L_8018F1B8:
    cmplwi   r30, 0x0
    lfs      f29, 0x38(r29)
    lfs      f28, 0x3c(r29)
    lfs      f27, 0x40(r29)
    bne      .L_8018F1DC
    li       r3, lbl_8065F498@sda21
    li       r4, 0x3e4
    li       r5, lbl_8065F4A0@sda21
    bl       __assert
.L_8018F1DC:
    lfs      f0, 0x3c(r30)
    cmplwi   r31, 0x0
    lfs      f3, 0x40(r30)
    lfs      f2, 0x38(r30)
    fmuls    f1, f0, f30
    fmuls    f0, f3, f30
    fmuls    f2, f2, f30
    fmadds   f1, f28, f31, f1
    fmadds   f0, f27, f31, f0
    fmadds   f2, f29, f31, f2
    stfs     f1, 0x3c(r1)
    stfs     f2, 0x38(r1)
    stfs     f0, 0x40(r1)
    bne      .L_8018F224
    li       r3, lbl_8065F498@sda21
    li       r4, 0x3a9
    li       r5, lbl_8065F4A0@sda21
    bl       __assert
.L_8018F224:
    lfs      f0, 0x38(r1)
    stfs     f0, 0x38(r31)
    lfs      f0, 0x3c(r1)
    stfs     f0, 0x3c(r31)
    lfs      f0, 0x40(r1)
    stfs     f0, 0x40(r31)
    lwz      r0, 0x14(r31)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_8018F290
    cmplwi   r31, 0x0
    beq      .L_8018F290
    bne      .L_8018F264
    li       r3, lbl_8065F498@sda21
    li       r4, 0x25d
    li       r5, lbl_8065F4A0@sda21
    bl       __assert
.L_8018F264:
    lwz      r4, 0x14(r31)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_8018F280
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_8018F280
    li       r3, 0x1
.L_8018F280:
    clrlwi.  r0, r3, 24
    bne      .L_8018F290
    mr       r3, r31
    bl       HSD_JObjSetMtxDirtySub
.L_8018F290:
    cmplwi   r29, 0x0
    bne      .L_8018F2A8
    li       r3, lbl_8065F498@sda21
    li       r4, 0x351
    li       r5, lbl_8065F4A0@sda21
    bl       __assert
.L_8018F2A8:
    cmplwi   r30, 0x0
    lfs      f27, 0x2c(r29)
    lfs      f28, 0x30(r29)
    lfs      f29, 0x34(r29)
    bne      .L_8018F2CC
    li       r3, lbl_8065F498@sda21
    li       r4, 0x351
    li       r5, lbl_8065F4A0@sda21
    bl       __assert
.L_8018F2CC:
    lfs      f0, 0x30(r30)
    cmplwi   r31, 0x0
    lfs      f3, 0x34(r30)
    lfs      f2, 0x2c(r30)
    fmuls    f1, f0, f30
    fmuls    f0, f3, f30
    fmuls    f2, f2, f30
    fmadds   f1, f28, f31, f1
    fmadds   f0, f29, f31, f0
    fmadds   f2, f27, f31, f2
    stfs     f1, 0x3c(r1)
    stfs     f2, 0x38(r1)
    stfs     f0, 0x40(r1)
    bne      .L_8018F314
    li       r3, lbl_8065F498@sda21
    li       r4, 0x316
    li       r5, lbl_8065F4A0@sda21
    bl       __assert
.L_8018F314:
    lfs      f0, 0x38(r1)
    stfs     f0, 0x2c(r31)
    lfs      f0, 0x3c(r1)
    stfs     f0, 0x30(r31)
    lfs      f0, 0x40(r1)
    stfs     f0, 0x34(r31)
    lwz      r0, 0x14(r31)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_8018F380
    cmplwi   r31, 0x0
    beq      .L_8018F380
    bne      .L_8018F354
    li       r3, lbl_8065F498@sda21
    li       r4, 0x25d
    li       r5, lbl_8065F4A0@sda21
    bl       __assert
.L_8018F354:
    lwz      r4, 0x14(r31)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_8018F370
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_8018F370
    li       r3, 0x1
.L_8018F370:
    clrlwi.  r0, r3, 24
    bne      .L_8018F380
    mr       r3, r31
    bl       HSD_JObjSetMtxDirtySub
.L_8018F380:
    lwz      r0, 0x14(r29)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_8018F3C8
    cmplwi   r29, 0x0
    bne      .L_8018F3A4
    li       r3, lbl_8065F498@sda21
    li       r4, 0x2dd
    li       r5, lbl_8065F4A0@sda21
    bl       __assert
.L_8018F3A4:
    lfs      f0, 0x1c(r29)
    stfs     f0, 0x18(r1)
    lfs      f0, 0x20(r29)
    stfs     f0, 0x1c(r1)
    lfs      f0, 0x24(r29)
    stfs     f0, 0x20(r1)
    lfs      f0, 0x28(r29)
    stfs     f0, 0x24(r1)
    b        .L_8018F424
.L_8018F3C8:
    cmplwi   r29, 0x0
    bne      .L_8018F3E0
    li       r3, lbl_8065F498@sda21
    li       r4, 0x2dd
    li       r5, lbl_8065F4A0@sda21
    bl       __assert
.L_8018F3E0:
    addic.   r0, r1, 0x38
    bne      .L_8018F3F8
    li       r3, lbl_8065F498@sda21
    li       r4, 0x2de
    li       r5, lbl_8065F4A8@sda21
    bl       __assert
.L_8018F3F8:
    lfs      f0, 0x1c(r29)
    addi     r3, r1, 0x38
    addi     r4, r1, 0x18
    stfs     f0, 0x38(r1)
    lfs      f0, 0x20(r29)
    stfs     f0, 0x3c(r1)
    lfs      f0, 0x24(r29)
    stfs     f0, 0x40(r1)
    lfs      f0, 0x28(r29)
    stfs     f0, 0x44(r1)
    bl       EulerToQuat
.L_8018F424:
    lwz      r0, 0x14(r30)
    rlwinm.  r0, r0, 0, 14, 14
    beq      .L_8018F46C
    cmplwi   r30, 0x0
    bne      .L_8018F448
    li       r3, lbl_8065F498@sda21
    li       r4, 0x2dd
    li       r5, lbl_8065F4A0@sda21
    bl       __assert
.L_8018F448:
    lfs      f0, 0x1c(r30)
    stfs     f0, 0x8(r1)
    lfs      f0, 0x20(r30)
    stfs     f0, 0xc(r1)
    lfs      f0, 0x24(r30)
    stfs     f0, 0x10(r1)
    lfs      f0, 0x28(r30)
    stfs     f0, 0x14(r1)
    b        .L_8018F4C8
.L_8018F46C:
    cmplwi   r30, 0x0
    bne      .L_8018F484
    li       r3, lbl_8065F498@sda21
    li       r4, 0x2dd
    li       r5, lbl_8065F4A0@sda21
    bl       __assert
.L_8018F484:
    addic.   r0, r1, 0x38
    bne      .L_8018F49C
    li       r3, lbl_8065F498@sda21
    li       r4, 0x2de
    li       r5, lbl_8065F4A8@sda21
    bl       __assert
.L_8018F49C:
    lfs      f0, 0x1c(r30)
    addi     r3, r1, 0x38
    addi     r4, r1, 0x8
    stfs     f0, 0x38(r1)
    lfs      f0, 0x20(r30)
    stfs     f0, 0x3c(r1)
    lfs      f0, 0x24(r30)
    stfs     f0, 0x40(r1)
    lfs      f0, 0x28(r30)
    stfs     f0, 0x44(r1)
    bl       EulerToQuat
.L_8018F4C8:
    lfs      f7, 0xc(r1)
    lfs      f0, 0x1c(r1)
    lfs      f8, 0x8(r1)
    lfs      f1, 0x18(r1)
    fsubs    f2, f0, f7
    fadds    f0, f0, f7
    lfs      f9, 0x10(r1)
    lfs      f4, 0x20(r1)
    fsubs    f3, f1, f8
    fadds    f1, f1, f8
    fsubs    f10, f4, f9
    fadds    f5, f4, f9
    lfs      f11, 0x14(r1)
    lfs      f4, 0x24(r1)
    fmuls    f3, f3, f3
    fmuls    f2, f2, f2
    fsubs    f12, f4, f11
    fadds    f6, f4, f11
    fmuls    f1, f1, f1
    fmuls    f0, f0, f0
    fmuls    f4, f10, f10
    fadds    f3, f3, f2
    fmuls    f2, f5, f5
    fadds    f0, f1, f0
    fmuls    f5, f12, f12
    fadds    f3, f4, f3
    fmuls    f1, f6, f6
    fadds    f0, f2, f0
    fadds    f2, f5, f3
    fadds    f0, f1, f0
    fcmpo    cr0, f2, f0
    ble      .L_8018F568
    fneg     f3, f8
    fneg     f2, f7
    fneg     f1, f9
    fneg     f0, f11
    stfs     f3, 0x8(r1)
    stfs     f2, 0xc(r1)
    stfs     f1, 0x10(r1)
    stfs     f0, 0x14(r1)
.L_8018F568:
    fmr      f1, f30
    addi     r3, r1, 0x18
    addi     r4, r1, 0x8
    addi     r5, r1, 0x28
    bl       QuatSlerp
    cmplwi   r31, 0x0
    bne      .L_8018F594
    li       r3, lbl_8065F498@sda21
    li       r4, 0x290
    li       r5, lbl_8065F4A0@sda21
    bl       __assert
.L_8018F594:
    lfs      f0, 0x28(r1)
    stfs     f0, 0x1c(r31)
    lfs      f0, 0x2c(r1)
    stfs     f0, 0x20(r31)
    lfs      f0, 0x30(r1)
    stfs     f0, 0x24(r31)
    lfs      f0, 0x34(r1)
    stfs     f0, 0x28(r31)
    lwz      r0, 0x14(r31)
    rlwinm.  r0, r0, 0, 6, 6
    bne      .L_8018F608
    cmplwi   r31, 0x0
    beq      .L_8018F608
    bne      .L_8018F5DC
    li       r3, lbl_8065F498@sda21
    li       r4, 0x25d
    li       r5, lbl_8065F4A0@sda21
    bl       __assert
.L_8018F5DC:
    lwz      r4, 0x14(r31)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_8018F5F8
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_8018F5F8
    li       r3, 0x1
.L_8018F5F8:
    clrlwi.  r0, r3, 24
    bne      .L_8018F608
    mr       r3, r31
    bl       HSD_JObjSetMtxDirtySub
.L_8018F608:
    lwz      r0, 0x14(r31)
    oris     r0, r0, 0x2
    stw      r0, 0x14(r31)
    psq_l    f31, 0xa8(r1), 0, qr0
    lfd      f31, 0xa0(r1)
    psq_l    f30, 0x98(r1), 0, qr0
    lfd      f30, 0x90(r1)
    psq_l    f29, 0x88(r1), 0, qr0
    lfd      f29, 0x80(r1)
    psq_l    f28, 0x78(r1), 0, qr0
    lfd      f28, 0x70(r1)
    psq_l    f27, 0x68(r1), 0, qr0
    lfd      f27, 0x60(r1)
    lwz      r31, 0x5c(r1)
    lwz      r30, 0x58(r1)
    lwz      r0, 0xb4(r1)
    lwz      r29, 0x54(r1)
    mtlr     r0
    addi     r1, r1, 0xb0
    blr
*/

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
	for (ca = a->child, cb = b->child, cout = out->child; ca != NULL; ca = ca->next, cb = cb->next, cout = cout->next) {
		if (ca == NULL || cb == NULL || cout == NULL) {
			break;
		}
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
/*
    stwu     r1, -0x50(r1)
    mflr     r0
    lfs      f3, 0x0(r4)
    stw      r0, 0x54(r1)
    lfs      f1, 0x0(r5)
    stw      r31, 0x4c(r1)
    mr       r31, r6
    fsubs    f5, f3, f1
    lfs      f3, 0x8(r4)
    stw      r30, 0x48(r1)
    mr       r30, r5
    lfs      f4, 0x8(r30)
    stw      r29, 0x44(r1)
    mr       r29, r4
    fsubs    f3, f3, f4
    stw      r28, 0x40(r1)
    mr       r28, r3
    lfs      f0, 0x0(r28)
    lis      r3, sTriangleEdgeTestDown@ha
    lwzu     r5, sTriangleEdgeTestDown@l(r3)
    fsubs    f1, f1, f0
    lfs      f0, 0x8(r28)
    lfs      f2, lbl_8065F478@sda21(r0)
    addi     r4, r1, 0x20
    fsubs    f0, f4, f0
    lwz      r7, 0x4(r3)
    lwz      r0, 0x8(r3)
    addi     r3, r1, 0x14
    stw      r5, 0x2c(r1)
    addi     r5, r1, 0x8
    stw      r7, 0x30(r1)
    stw      r0, 0x34(r1)
    stfs     f5, 0x20(r1)
    stfs     f3, 0x28(r1)
    stfs     f2, 0x24(r1)
    stfs     f1, 0x14(r1)
    stfs     f0, 0x1c(r1)
    stfs     f2, 0x18(r1)
    bl       C_VECCrossProduct
    addi     r3, r1, 0x2c
    addi     r4, r1, 0x8
    bl       PSVECDotProduct
    lfs      f5, lbl_8065F478@sda21(r0)
    fcmpo    cr0, f1, f5
    ble      .L_8018ED28
    li       r3, 0x0
    b        .L_8018EE0C
.L_8018ED28:
    lfs      f2, 0x0(r30)
    addi     r3, r1, 0x14
    lfs      f1, 0x0(r31)
    addi     r4, r1, 0x20
    lfs      f0, 0x0(r28)
    addi     r5, r1, 0x8
    fsubs    f4, f2, f1
    lfs      f2, 0x8(r30)
    lfs      f3, 0x8(r31)
    fsubs    f1, f1, f0
    lfs      f0, 0x8(r28)
    fsubs    f2, f2, f3
    fsubs    f0, f3, f0
    stfs     f4, 0x20(r1)
    stfs     f2, 0x28(r1)
    stfs     f5, 0x24(r1)
    stfs     f1, 0x14(r1)
    stfs     f0, 0x1c(r1)
    stfs     f5, 0x18(r1)
    bl       C_VECCrossProduct
    addi     r3, r1, 0x2c
    addi     r4, r1, 0x8
    bl       PSVECDotProduct
    lfs      f5, lbl_8065F478@sda21(r0)
    fcmpo    cr0, f1, f5
    ble      .L_8018ED98
    li       r3, 0x0
    b        .L_8018EE0C
.L_8018ED98:
    lfs      f2, 0x0(r31)
    addi     r3, r1, 0x14
    lfs      f1, 0x0(r29)
    addi     r4, r1, 0x20
    lfs      f0, 0x0(r28)
    addi     r5, r1, 0x8
    fsubs    f4, f2, f1
    lfs      f2, 0x8(r31)
    lfs      f3, 0x8(r29)
    fsubs    f1, f1, f0
    lfs      f0, 0x8(r28)
    fsubs    f2, f2, f3
    fsubs    f0, f3, f0
    stfs     f4, 0x20(r1)
    stfs     f2, 0x28(r1)
    stfs     f5, 0x24(r1)
    stfs     f1, 0x14(r1)
    stfs     f0, 0x1c(r1)
    stfs     f5, 0x18(r1)
    bl       C_VECCrossProduct
    addi     r3, r1, 0x2c
    addi     r4, r1, 0x8
    bl       PSVECDotProduct
    lfs      f0, lbl_8065F478@sda21(r0)
    fcmpo    cr0, f1, f0
    mfcr     r0
    extrwi   r0, r0, 1, 1
    cntlzw   r0, r0
    srwi     r3, r0, 5
.L_8018EE0C:
    lwz      r0, 0x54(r1)
    lwz      r31, 0x4c(r1)
    lwz      r30, 0x48(r1)
    lwz      r29, 0x44(r1)
    lwz      r28, 0x40(r1)
    mtlr     r0
    addi     r1, r1, 0x50
    blr
*/

/**
 * @note Address: 0x8018EA50
 * @note Size: 0x21C
 */
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
/*
    stwu     r1, -0x50(r1)
    mflr     r0
    lfs      f3, 0x0(r4)
    stw      r0, 0x54(r1)
    lfs      f1, 0x0(r5)
    stmw     r27, 0x3c(r1)
    mr       r27, r3
    lfs      f2, lbl_8065F478@sda21(r0)
    mr       r29, r5
    fsubs    f5, f3, f1
    lfs      f0, 0x0(r27)
    mr       r28, r4
    lis      r3, sQuadEdgeTestDown@ha
    fsubs    f1, f1, f0
    lfs      f3, 0x8(r28)
    lfs      f4, 0x8(r29)
    mr       r30, r6
    lwzu     r4, sQuadEdgeTestDown@l(r3)
    mr       r31, r7
    lfs      f0, 0x8(r27)
    fsubs    f3, f3, f4
    lwz      r5, 0x4(r3)
    lwz      r0, 0x8(r3)
    fsubs    f0, f4, f0
    stw      r4, 0x2c(r1)
    addi     r3, r1, 0x14
    addi     r4, r1, 0x20
    stw      r5, 0x30(r1)
    addi     r5, r1, 0x8
    stw      r0, 0x34(r1)
    stfs     f5, 0x20(r1)
    stfs     f3, 0x28(r1)
    stfs     f2, 0x24(r1)
    stfs     f1, 0x14(r1)
    stfs     f0, 0x1c(r1)
    stfs     f2, 0x18(r1)
    bl       C_VECCrossProduct
    addi     r3, r1, 0x2c
    addi     r4, r1, 0x8
    bl       PSVECDotProduct
    lfs      f5, lbl_8065F478@sda21(r0)
    fcmpo    cr0, f1, f5
    ble      .L_8018EB04
    li       r3, 0x0
    b        .L_8018EC58
.L_8018EB04:
    lfs      f2, 0x0(r29)
    addi     r3, r1, 0x14
    lfs      f1, 0x0(r30)
    addi     r4, r1, 0x20
    lfs      f0, 0x0(r27)
    addi     r5, r1, 0x8
    fsubs    f4, f2, f1
    lfs      f2, 0x8(r29)
    lfs      f3, 0x8(r30)
    fsubs    f1, f1, f0
    lfs      f0, 0x8(r27)
    fsubs    f2, f2, f3
    fsubs    f0, f3, f0
    stfs     f4, 0x20(r1)
    stfs     f2, 0x28(r1)
    stfs     f5, 0x24(r1)
    stfs     f1, 0x14(r1)
    stfs     f0, 0x1c(r1)
    stfs     f5, 0x18(r1)
    bl       C_VECCrossProduct
    addi     r3, r1, 0x2c
    addi     r4, r1, 0x8
    bl       PSVECDotProduct
    lfs      f5, lbl_8065F478@sda21(r0)
    fcmpo    cr0, f1, f5
    ble      .L_8018EB74
    li       r3, 0x0
    b        .L_8018EC58
.L_8018EB74:
    lfs      f2, 0x0(r30)
    addi     r3, r1, 0x14
    lfs      f1, 0x0(r31)
    addi     r4, r1, 0x20
    lfs      f0, 0x0(r27)
    addi     r5, r1, 0x8
    fsubs    f4, f2, f1
    lfs      f2, 0x8(r30)
    lfs      f3, 0x8(r31)
    fsubs    f1, f1, f0
    lfs      f0, 0x8(r27)
    fsubs    f2, f2, f3
    fsubs    f0, f3, f0
    stfs     f4, 0x20(r1)
    stfs     f2, 0x28(r1)
    stfs     f5, 0x24(r1)
    stfs     f1, 0x14(r1)
    stfs     f0, 0x1c(r1)
    stfs     f5, 0x18(r1)
    bl       C_VECCrossProduct
    addi     r3, r1, 0x2c
    addi     r4, r1, 0x8
    bl       PSVECDotProduct
    lfs      f5, lbl_8065F478@sda21(r0)
    fcmpo    cr0, f1, f5
    ble      .L_8018EBE4
    li       r3, 0x0
    b        .L_8018EC58
.L_8018EBE4:
    lfs      f2, 0x0(r31)
    addi     r3, r1, 0x14
    lfs      f1, 0x0(r28)
    addi     r4, r1, 0x20
    lfs      f0, 0x0(r27)
    addi     r5, r1, 0x8
    fsubs    f4, f2, f1
    lfs      f2, 0x8(r31)
    lfs      f3, 0x8(r28)
    fsubs    f1, f1, f0
    lfs      f0, 0x8(r27)
    fsubs    f2, f2, f3
    fsubs    f0, f3, f0
    stfs     f4, 0x20(r1)
    stfs     f2, 0x28(r1)
    stfs     f5, 0x24(r1)
    stfs     f1, 0x14(r1)
    stfs     f0, 0x1c(r1)
    stfs     f5, 0x18(r1)
    bl       C_VECCrossProduct
    addi     r3, r1, 0x2c
    addi     r4, r1, 0x8
    bl       PSVECDotProduct
    lfs      f0, lbl_8065F478@sda21(r0)
    fcmpo    cr0, f1, f0
    mfcr     r0
    extrwi   r0, r0, 1, 1
    cntlzw   r0, r0
    srwi     r3, r0, 5
.L_8018EC58:
    lmw      r27, 0x3c(r1)
    lwz      r0, 0x54(r1)
    mtlr     r0
    addi     r1, r1, 0x50
    blr
*/

/**
 * @note Address: 0x8018E8A4
 * @note Size: 0x1AC
 */
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
/*
    stwu     r1, -0x90(r1)
    mflr     r0
    stw      r0, 0x94(r1)
    stfd     f31, 0x80(r1)
    psq_st   f31, 0x88(r1), 0, qr0
    stmw     r27, 0x6c(r1)
    mr       r29, r5
    mr       r27, r3
    mr       r28, r4
    mr       r30, r6
    mr       r31, r7
    addi     r5, r1, 0x20
    bl       PSVECSubtract
    mr       r3, r28
    mr       r4, r29
    addi     r5, r1, 0x2c
    bl       PSVECSubtract
    addi     r3, r1, 0x2c
    addi     r4, r1, 0x20
    addi     r5, r1, 0x50
    bl       C_VECCrossProduct
    lis      r4, sVerticalLineTop@ha
    lis      r3, sVerticalLineBottom@ha
    addi     r10, r4, sVerticalLineTop@l
    lfs      f1, 0x0(r30)
    addi     r7, r3, sVerticalLineBottom@l
    lwz      r9, 0x0(r10)
    lwz      r8, 0x8(r10)
    mr       r4, r27
    lwz      r6, 0x0(r7)
    addi     r3, r1, 0x44
    lwz      r0, 0x8(r7)
    addi     r5, r1, 0x8
    stw      r9, 0x44(r1)
    lwz      r9, 0x4(r10)
    stw      r6, 0x38(r1)
    lwz      r6, 0x4(r7)
    stw      r8, 0x4c(r1)
    lfs      f0, 0x8(r30)
    stw      r0, 0x40(r1)
    stw      r9, 0x48(r1)
    stfs     f1, 0x44(r1)
    stfs     f0, 0x4c(r1)
    stw      r6, 0x3c(r1)
    stfs     f1, 0x38(r1)
    stfs     f0, 0x40(r1)
    bl       PSVECSubtract
    addi     r3, r1, 0x38
    addi     r4, r1, 0x44
    addi     r5, r1, 0x14
    bl       PSVECSubtract
    addi     r3, r1, 0x14
    addi     r4, r1, 0x50
    bl       PSVECDotProduct
    fmr      f31, f1
    addi     r3, r1, 0x8
    addi     r4, r1, 0x50
    bl       PSVECDotProduct
    fdivs    f0, f1, f31
    lfs      f1, lbl_8065F48C@sda21(r0)
    mr       r4, r31
    addi     r3, r1, 0x14
    fmuls    f31, f1, f0
    fmr      f1, f31
    bl       PSVECScale
    mr       r3, r31
    mr       r5, r31
    addi     r4, r1, 0x44
    bl       PSVECAdd
    lfs      f0, lbl_8065F478@sda21(r0)
    fcmpo    cr0, f31, f0
    bge      .L_8018E9CC
    fneg     f1, f31
    b        .L_8018E9D0
.L_8018E9CC:
    fmr      f1, f31
.L_8018E9D0:
    lfs      f0, lbl_8065F490@sda21(r0)
    fcmpo    cr0, f1, f0
    cror     eq, lt, eq
    beq      .L_8018EA08
    lfs      f1, lbl_8065F494@sda21(r0)
    lfs      f0, lbl_8065F478@sda21(r0)
    fsubs    f1, f1, f31
    fcmpo    cr0, f1, f0
    bge      .L_8018E9F8
    fneg     f1, f1
.L_8018E9F8:
    lfs      f0, lbl_8065F490@sda21(r0)
    fcmpo    cr0, f1, f0
    cror     eq, lt, eq
    bne      .L_8018EA10
.L_8018EA08:
    li       r3, 0x1
    b        .L_8018EA34
.L_8018EA10:
    lfs      f0, lbl_8065F478@sda21(r0)
    fcmpo    cr0, f31, f0
    ble      .L_8018EA30
    lfs      f0, lbl_8065F494@sda21(r0)
    fcmpo    cr0, f31, f0
    bge      .L_8018EA30
    li       r3, 0x2
    b        .L_8018EA34
.L_8018EA30:
    li       r3, 0x3
.L_8018EA34:
    psq_l    f31, 0x88(r1), 0, qr0
    lfd      f31, 0x80(r1)
    lmw      r27, 0x6c(r1)
    lwz      r0, 0x94(r1)
    mtlr     r0
    addi     r1, r1, 0x90
    blr
*/

/**
 * @note Address: 0x8018E824
 * @note Size: 0x80
 */
// the direction from the line bc out toward a, square to bc
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
			f32 t  = (acz - slopeCD * acx) / (slopeAB - slopeCD);
			f32 dz = t * slopeAB;
			out->x = t + a->x;
			out->z = dz + a->z;
		}
		out->y = 0.0f;
		return 1;
	}
	return 0;
}
/*
    stwu     r1, -0x60(r1)
    stfd     f31, 0x50(r1)
    psq_st   f31, 0x58(r1), 0, qr0
    stfd     f30, 0x40(r1)
    psq_st   f30, 0x48(r1), 0, qr0
    stfd     f29, 0x30(r1)
    psq_st   f29, 0x38(r1), 0, qr0
    stfd     f28, 0x20(r1)
    psq_st   f28, 0x28(r1), 0, qr0
    stfd     f27, 0x10(r1)
    psq_st   f27, 0x18(r1), 0, qr0
    lfs      f10, 0x0(r4)
    cmpwi    r8, 0x1
    lfs      f6, 0x0(r3)
    lfs      f7, 0x0(r5)
    lfs      f8, 0x0(r6)
    fsubs    f12, f10, f6
    lfs      f11, 0x8(r4)
    fsubs    f29, f7, f6
    lfs      f2, 0x8(r3)
    fsubs    f4, f8, f6
    lfs      f9, 0x8(r5)
    lfs      f3, 0x8(r6)
    fsubs    f13, f11, f2
    fsubs    f1, f6, f7
    fsubs    f30, f3, f9
    fsubs    f0, f10, f7
    fsubs    f28, f9, f2
    fmuls    f5, f13, f29
    fsubs    f27, f3, f2
    fmuls    f4, f13, f4
    fsubs    f31, f8, f7
    fsubs    f3, f2, f9
    fmuls    f2, f30, f1
    fsubs    f1, f11, f9
    fmuls    f0, f30, f0
    fmsubs   f5, f12, f28, f5
    fmsubs   f4, f12, f27, f4
    fmsubs   f2, f31, f3, f2
    fmsubs   f3, f31, f1, f0
    bne      .L_8018E718
    fmuls    f0, f5, f4
    lfs      f1, lbl_8065F478@sda21(r0)
    li       r0, 0x0
    fcmpo    cr0, f0, f1
    cror     eq, lt, eq
    bne      .L_8018E710
    fmuls    f0, f2, f3
    fcmpo    cr0, f0, f1
    cror     eq, lt, eq
    bne      .L_8018E710
    li       r0, 0x1
.L_8018E710:
    clrlwi   r0, r0, 24
    b        .L_8018E71C
.L_8018E718:
    li       r0, 0x1
.L_8018E71C:
    cmpwi    r0, 0x0
    beq      .L_8018E7F0
    fcmpu    cr0, f10, f6
    bne      .L_8018E764
    lfs      f4, 0x8(r5)
    lfs      f1, 0x8(r6)
    lfs      f3, 0x0(r5)
    lfs      f0, 0x0(r6)
    fsubs    f1, f1, f4
    lfs      f2, 0x0(r3)
    fsubs    f0, f0, f3
    stfs     f6, 0x0(r7)
    fsubs    f2, f2, f3
    fdivs    f0, f1, f0
    fmuls    f0, f2, f0
    fadds    f0, f4, f0
    stfs     f0, 0x8(r7)
    b        .L_8018E7E0
.L_8018E764:
    fcmpu    cr0, f8, f7
    bne      .L_8018E7A4
    lfs      f4, 0x8(r3)
    lfs      f1, 0x8(r4)
    lfs      f3, 0x0(r3)
    lfs      f0, 0x0(r4)
    fsubs    f1, f1, f4
    lfs      f2, 0x0(r5)
    fsubs    f0, f0, f3
    stfs     f7, 0x0(r7)
    fsubs    f2, f2, f3
    fdivs    f0, f1, f0
    fmuls    f0, f2, f0
    fadds    f0, f4, f0
    stfs     f0, 0x8(r7)
    b        .L_8018E7E0
.L_8018E7A4:
    fdivs    f3, f13, f12
    fdivs    f1, f30, f31
    fcmpu    cr0, f3, f1
    bne      .L_8018E7BC
    li       r3, 0x0
    b        .L_8018E7F4
.L_8018E7BC:
    fnmsubs  f2, f1, f29, f28
    lfs      f0, 0x8(r3)
    fsubs    f1, f3, f1
    fdivs    f2, f2, f1
    fmuls    f1, f2, f3
    fadds    f2, f2, f6
    fadds    f0, f1, f0
    stfs     f2, 0x0(r7)
    stfs     f0, 0x8(r7)
.L_8018E7E0:
    lfs      f0, lbl_8065F478@sda21(r0)
    li       r3, 0x1
    stfs     f0, 0x4(r7)
    b        .L_8018E7F4
.L_8018E7F0:
    li       r3, 0x0
.L_8018E7F4:
    psq_l    f31, 0x58(r1), 0, qr0
    lfd      f31, 0x50(r1)
    psq_l    f30, 0x48(r1), 0, qr0
    lfd      f30, 0x40(r1)
    psq_l    f29, 0x38(r1), 0, qr0
    lfd      f29, 0x30(r1)
    psq_l    f28, 0x28(r1), 0, qr0
    lfd      f28, 0x20(r1)
    psq_l    f27, 0x18(r1), 0, qr0
    lfd      f27, 0x10(r1)
    addi     r1, r1, 0x60
    blr
*/

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
	CVec fromEdge = from + n * radius; // never used, it's still computed
	CVec toEdge   = to + n * radius;
	int ret       = 0;

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
					CVec e = hit + dir * len;
					CVec f = e - n;
					CVec g;
					IntersectLinesXZ(&e, &f, &a, &b, &g, 0);
					v = g - hit;
					if (v.Length() > len) {
						*push = (g - hit).Normalize() * len;
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
					CVec e = hit + dir * len;
					CVec f = e - n;
					CVec g;
					IntersectLinesXZ(&e, &f, &a, &b, &g, 0);
					v = g - hit;
					if (v.Length() > len) {
						*push = (g - hit).Normalize() * len;
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
