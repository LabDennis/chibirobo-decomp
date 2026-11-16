// the Wii's helper unit, .text 0x802A5358-0x802A7478: the Shift-JIS, matrix and
// collision helpers the GameCube kept in game/ut.cpp, plus the HSD matrix
// routines of sysdolphin/baselib/mtx.c folded in with it. Functions are in
// retail order; the GameCube file's deferred-inline ordering does not apply.

#include <Dolphin/mtx.h>
#include "game/ut.h"
#include "game/gmath.h"
#include "game/cvec.h"
#include "arithmetic.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern "C" {
// the Wii unit carries the two HSD matrix helpers itself; their GameCube twins
// are sysdolphin/baselib/mtx.c. Its HSD_MtxSRT has no parent_scale argument and
// stores the transpose of the GameCube result (the scale row per row, not per
// column), the inverse rotation of the GameCube convention.
void HSD_MtxGetScale(Mtx m, Vec* out);
void HSD_MtxSRT(Mtx m, Vec* scale, Vec* rotation, Vec* translation);
// the GameCube's static LinePlane inline plus its t-class return, a real
// function here: UpdatePlayerPopperAction calls it with a line of its own
int IntersectLineWithPlane(Vec* p0, Vec* p1, Vec* point, Vec* normal, Vec* out);
CVec ReflectVectorAboutNormal(const Vec* v, const Vec* normal);
}

// HSD_MtxGetScale's degenerate test; the GameCube's mtx.c uses EPSILON 1e-10F
#define FLT_EPSILON 1.1920929e-07F

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

// reads len hex digits, or the whole string when len is 0. any other character is skipped but
// still counts toward len
extern "C" u32 ParseHexDigits(const char* str, s32 len)
{
	u32 val = 0;
	if (len == 0) {
		len = strlen(str);
	}
	for (int i = 0; i < len; i++) {
		char c = *str;
		if (c >= '0' && c <= '9') {
			val = val * 16 + (c - '0');
		} else if (c >= 'A' && c <= 'F') {
			val = val * 16 + (c - 'A' + 10);
		} else if (c >= 'a' && c <= 'f') {
			val = val * 16 + (c - 'a' + 10);
		}
		str++;
	}
	return val;
}

// src with each character from sHalfWidthCharacters swapped for its full width twin, the rest copied
// as is. when dst is NULL the static buffer is used and returned
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

extern "C" char* FormatToStaticBuffer(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsprintf(sFormattedTextBuffer, fmt, args);
	return sFormattedTextBuffer;
}

// HSD_MtxGetScale from sysdolphin/baselib/mtx.c; on the Wii the square test uses
// FLT_EPSILON and the square root is the FSqrt inline of arithmetic.h
extern "C" void HSD_MtxGetScale(Mtx m, Vec* out)
{
	float square;
	float inverse_length;
	float dot;
	double sign;
	Vec x, y, z, projection;
	x.x    = m[0][0];
	x.y    = m[1][0];
	x.z    = m[2][0];
	square = PSVECSquareMag(&x);
	if (square > FLT_EPSILON) {
		inverse_length = FSqrt(1.0F / square);
		out->x         = 1.0F / inverse_length;
		PSVECScale(&x, &x, inverse_length);
		y.x = m[0][1];
		y.y = m[1][1];
		y.z = m[2][1];
		PSVECScale(&x, &projection, dot = PSVECDotProduct(&x, &y));
		PSVECSubtract(&y, &projection, &y);
		square = PSVECSquareMag(&y);
		if (square > FLT_EPSILON) {
			inverse_length = FSqrt(1.0F / square);
			out->y         = 1.0F / inverse_length;
			dot *= inverse_length;
			PSVECScale(&y, &y, inverse_length);
			z.x = m[0][2];
			z.y = m[1][2];
			z.z = m[2][2];
			PSVECScale(&y, &projection, dot = PSVECDotProduct(&y, &z));
			PSVECSubtract(&z, &projection, &z);
			PSVECScale(&x, &projection, dot = PSVECDotProduct(&x, &z));
			PSVECSubtract(&z, &projection, &z);
			square = PSVECSquareMag(&z);
			if (square > FLT_EPSILON) {
				out->z = FSqrt(square);
				PSVECCrossProduct(&y, &z, &projection);
				if (PSVECDotProduct(&x, &projection) < 0.0) {
					sign = -1.0;
					out->x *= sign;
					out->y *= sign;
					out->z *= sign;
				}
			} else {
				out->z = 0.0F;
			}
		} else {
			out->y = 0.0F;
			z.x    = m[0][2];
			z.y    = m[1][2];
			z.z    = m[2][2];
			PSVECScale(&x, &projection, dot = PSVECDotProduct(&x, &z));
			PSVECSubtract(&z, &projection, &z);
			square = PSVECSquareMag(&z);
			if (square > FLT_EPSILON)
				out->z = FSqrt(square);
			else
				out->z = 0.0F;
		}
	} else {
		out->x = 0.0F;
		y.x    = m[0][1];
		y.y    = m[1][1];
		y.z    = m[2][1];
		PSVECScale(&x, &projection, dot = PSVECDotProduct(&x, &y));
		PSVECSubtract(&y, &projection, &y);
		square = PSVECSquareMag(&y);
		if (square > FLT_EPSILON) {
			inverse_length = FSqrt(1.0F / square);
			out->y         = 1.0F / inverse_length;
			dot *= inverse_length;
			PSVECScale(&y, &y, inverse_length);
			z.x = m[0][2];
			z.y = m[1][2];
			z.z = m[2][2];
			PSVECScale(&y, &projection, dot = PSVECDotProduct(&y, &z));
			PSVECSubtract(&z, &projection, &z);
			PSVECScale(&x, &projection, dot = PSVECDotProduct(&x, &z));
			PSVECSubtract(&z, &projection, &z);
			square = PSVECSquareMag(&z);
			if (square > FLT_EPSILON) {
				out->z = FSqrt(square);
				PSVECCrossProduct(&y, &z, &projection);
				if (PSVECDotProduct(&x, &projection) < 0.0) {
					sign = -1.0;
					out->x *= sign;
					out->y *= sign;
					out->z *= sign;
				}
			} else {
				out->z = 0.0F;
			}
		} else {
			out->y = 0.0F;
			z.x    = m[0][2];
			z.y    = m[1][2];
			z.z    = m[2][2];
			PSVECScale(&x, &projection, dot = PSVECDotProduct(&x, &z));
			PSVECSubtract(&z, &projection, &z);
			square = PSVECSquareMag(&z);
			if (square > FLT_EPSILON)
				out->z = FSqrt(square);
			else
				out->z = 0.0F;
		}
	}
}

// HSD_MtxSRT from sysdolphin/baselib/mtx.c, the Wii variant: no parent_scale
// and the result stored transposed
extern "C" void HSD_MtxSRT(Mtx m, Vec* scale, Vec* rotation, Vec* translation)
{
	float sin_x, sin_y, sin_z, cos_x, cos_y, cos_z;
	float scale_xx, scale_xy, scale_xz;
	float scale_yx, scale_yy, scale_yz;
	float scale_zx, scale_zy, scale_zz;

	sin_x = (float)sin(rotation->x);
	cos_x = (float)cos(rotation->x);
	sin_y = (float)sin(rotation->y);
	cos_y = (float)cos(rotation->y);
	sin_z = (float)sin(rotation->z);
	cos_z = (float)cos(rotation->z);

	scale_xx = scale_xy = scale_xz = scale->x;
	scale_yx = scale_yy = scale_yz = scale->y;
	scale_zx = scale_zy = scale_zz = scale->z;

	m[0][0] = cos_z * (scale_xx * cos_y);
	m[0][1] = sin_z * (scale_xy * cos_y);
	m[0][2] = -scale_xz * sin_y;
	m[1][0] = scale_yx * ((cos_z * (sin_x * sin_y)) - (cos_x * sin_z));
	m[1][1] = scale_yy * ((sin_z * (sin_x * sin_y)) + (cos_x * cos_z));
	m[1][2] = cos_y * (scale_yz * sin_x);
	m[2][0] = scale_zx * ((cos_z * (cos_x * sin_y)) + (sin_x * sin_z));
	m[2][1] = scale_zy * ((sin_z * (cos_x * sin_y)) - (sin_x * cos_z));
	m[2][2] = cos_y * (scale_zz * cos_x);
	m[0][3] = translation->x;
	m[1][3] = translation->y;
	m[2][3] = translation->z;
}

// the GameCube's BuildBillboardMatrixFromUp; on the Wii the scales go through
// FSqrt and the crosses are plain PSVECCrossProduct calls
void BuildBillboardMatrixFromUp(Mtx src, Mtx dst)
{
	Vec x, y;
	Vec z  = { 0.0f, 0.0f, 1.0f };
	f32 sx = FSqrt(src[0][0] * src[0][0] + src[1][0] * src[1][0] + src[2][0] * src[2][0]);
	f32 sz = FSqrt(src[0][2] * src[0][2] + src[1][2] * src[1][2] + src[2][2] * src[2][2]);
	y.x    = src[0][1];
	y.y    = src[1][1];
	y.z    = src[2][1];
	f32 sy = PSVECMag(&y);
	PSVECCrossProduct(&y, &z, &x);
	PSVECCrossProduct(&z, &x, &y);
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

void BuildBillboardMatrixWithZRotation(Mtx src, Mtx dst, f32 angle)
{
	Mtx rot, scale;
	f32 sx = FSqrt(src[0][0] * src[0][0] + src[1][0] * src[1][0] + src[2][0] * src[2][0]);
	f32 sy = FSqrt(src[0][1] * src[0][1] + src[1][1] * src[1][1] + src[2][1] * src[2][1]);
	f32 sz = FSqrt(src[0][2] * src[0][2] + src[1][2] * src[1][2] + src[2][2] * src[2][2]);
	PSMTXScale(scale, sx, sy, sz);
	PSMTXRotRad(rot, 'z', angle);
	rot[0][3] = src[0][3];
	rot[1][3] = src[1][3];
	rot[2][3] = src[2][3];
	PSMTXConcat(rot, scale, dst);
}

void CalcTriangleNormal(Vec* a, Vec* b, Vec* c, Vec* normal)
{
	Vec ab, bc;
	PSVECSubtract(a, b, &ab);
	PSVECSubtract(b, c, &bc);
	PSVECCrossProduct(&bc, &ab, normal);
}

f32 SignedTriangleArea2XZ(Vec* a, Vec* b, Vec* c)
{
	return a->x * (b->z - c->z) + b->x * (c->z - a->z) + c->x * (a->z - b->z);
}

BOOL IsPointInsideTriangleXZ(Vec* pos, Vec* a, Vec* b, Vec* c)
{
	Vec up = { 0.0f, -1.0f, 0.0f };
	Vec edge, rel, n;

	f32 ex = a->x - b->x;
	f32 rx = b->x - pos->x;
	f32 rz = b->z - pos->z;
	f32 ez = a->z - b->z;

	edge.y = 0.0f;
	rel.y  = 0.0f;

	edge.x = ex;
	edge.z = ez;
	rel.x  = rx;
	rel.z  = rz;
	PSVECCrossProduct(&rel, &edge, &n);
	if (PSVECDotProduct(&up, &n) > 0.0f) {
		return FALSE;
	}

	ex     = b->x - c->x;
	ez     = b->z - c->z;
	rx     = c->x - pos->x;
	rz     = c->z - pos->z;
	edge.x = ex;
	edge.z = ez;
	rel.x  = rx;
	rel.z  = rz;
	PSVECCrossProduct(&rel, &edge, &n);
	if (PSVECDotProduct(&up, &n) > 0.0f) {
		return FALSE;
	}

	ex     = c->x - a->x;
	ez     = c->z - a->z;
	rx     = a->x - pos->x;
	rz     = a->z - pos->z;
	edge.x = ex;
	edge.z = ez;
	rel.x  = rx;
	rel.z  = rz;
	PSVECCrossProduct(&rel, &edge, &n);
	if (PSVECDotProduct(&up, &n) > 0.0f) {
		return FALSE;
	}
	return TRUE;
}

BOOL IsPointInsideQuadXZ(Vec* pos, Vec* a, Vec* b, Vec* c, Vec* d)
{
	Vec up = { 0.0f, -1.0f, 0.0f };
	Vec edge, rel, n;

	f32 ex = a->x - b->x;
	f32 ez = a->z - b->z;
	f32 rx = b->x - pos->x;
	f32 rz = b->z - pos->z;

	edge.y = 0.0f;
	rel.y  = 0.0f;

	edge.x = ex;
	edge.z = ez;
	rel.x  = rx;
	rel.z  = rz;
	PSVECCrossProduct(&rel, &edge, &n);
	if (PSVECDotProduct(&up, &n) > 0.0f) {
		return FALSE;
	}

	ex     = b->x - c->x;
	ez     = b->z - c->z;
	rx     = c->x - pos->x;
	rz     = c->z - pos->z;
	edge.x = ex;
	edge.z = ez;
	rel.x  = rx;
	rel.z  = rz;
	PSVECCrossProduct(&rel, &edge, &n);
	if (PSVECDotProduct(&up, &n) > 0.0f) {
		return FALSE;
	}

	ex     = c->x - d->x;
	ez     = c->z - d->z;
	rx     = d->x - pos->x;
	rz     = d->z - pos->z;
	edge.x = ex;
	edge.z = ez;
	rel.x  = rx;
	rel.z  = rz;
	PSVECCrossProduct(&rel, &edge, &n);
	if (PSVECDotProduct(&up, &n) > 0.0f) {
		return FALSE;
	}

	ex     = d->x - a->x;
	ez     = d->z - a->z;
	rx     = a->x - pos->x;
	rz     = a->z - pos->z;
	edge.x = ex;
	edge.z = ez;
	rel.x  = rx;
	rel.z  = rz;
	PSVECCrossProduct(&rel, &edge, &n);
	if (PSVECDotProduct(&up, &n) > 0.0f) {
		return FALSE;
	}
	return TRUE;
}

// the GameCube takes the triangle first, the Wii pos first; the GameCube's
// static LinePlane inline is the exported IntersectLineWithPlane here
extern "C" int IntersectVerticalLineWithTrianglePlane(Vec* pos, Vec* a, Vec* b, Vec* c, Vec* out)
{
	Vec normal;
	CalcTriangleNormal(a, b, c, &normal);

	Vec top    = { pos->x, 1.0f, pos->z };
	Vec bottom = { pos->x, -1.0f, pos->z };
	return IntersectLineWithPlane(&top, &bottom, a, &normal, out);
}

extern "C" int IntersectLineWithPlane(Vec* p0, Vec* p1, Vec* point, Vec* normal, Vec* out)
{
	Vec rel, dir;
	PSVECSubtract(p0, point, &rel);
	PSVECSubtract(p1, p0, &dir);
	f32 d = PSVECDotProduct(&dir, normal);
	f32 t = -1.0f * (PSVECDotProduct(&rel, normal) / d);
	PSVECScale(&dir, out, t);
	PSVECAdd(out, p0, out);

	if (Abs(t) <= 0.00001f || Abs(1.0f - t) <= 0.00001f) {
		return 1;
	}
	if (t > 0.0f && t < 1.0f) {
		return 2;
	}
	return 3;
}

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

// a real function in another unit (0x800D57E4); inlined into
// ResolveCircleWallCollisionXZ here
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

// moves a circle of radius from from to to against the wall ab, flat on the xz plane.
// pos gets where it ends up, contact where it touched and push what's left of the
// move. returns 0 for no hit, 1 when it got stopped and 3 when it bounced, then push
// is the move reflected off the wall. the GameCube version also had a dist out and a
// scale on the bounce; the Wii dropped both
extern "C" int ResolveCircleWallCollisionXZ(const Vec* fromPos, const Vec* toPos, const Vec* wallA, const Vec* wallB, Vec* pos,
                                            Vec* contact, Vec* push, int corners, f32 radius)
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

	// one sided: from must be on the wall's LineSide <= 0 and the circle at to touch or cross it
	if (SignedTriangleArea2XZ(&toEdge, &a, &b) >= 0.0f && SignedTriangleArea2XZ(&from, &a, &b) <= 0.0f) {
		CVec v;
		CVec hit;
		if (IntersectLinesXZ(&from, &toEdge, &a, &b, &hit, 1)) {
			if (contact) {
				*contact = hit;
			}
			CVec end;
			IntersectLinesXZ(&toEdge, &to, &a, &b, &end, 0);
			*pos   = end - n * radius;
			pos->y = fromPos->y;
			if (push) {
				*push = ReflectVectorAboutNormal(&dir, &n) * len;
			}
			ret = 3;
		} else if (corners) {
			if (PSVECDistance(&to, &a) <= radius) {
				if (contact) {
					*contact = a;
				}
				v      = (to - a).Normalize();
				*pos   = a + v * radius + (to - from);
				pos->y = fromPos->y;
				if (push) {
					*push = len > 0.3f ? to - from : CVec(0.0f, 0.0f, 0.0f);
				}
				ret = 1;
			}
			if (PSVECDistance(&to, &b) <= radius) {
				if (contact) {
					*contact = b;
				}
				v      = (to - b).Normalize();
				*pos   = b + v * radius + (to - from);
				pos->y = fromPos->y;
				if (push) {
					*push = len > 0.3f ? to - from : CVec(0.0f, 0.0f, 0.0f);
				}
				ret = 1;
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
					if (push) {
						*push = v * len;
					}
					ret = 3;
				} else {
					if (push) {
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
					if (push) {
						*push = v * len;
					}
					ret = 3;
				} else {
					if (push) {
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
					}
					ret = 1;
				}
			}
		}
	}
	return ret;
}
