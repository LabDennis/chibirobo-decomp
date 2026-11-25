#include "Dolphin/mtx.h"
#include "math.h"

static f32 FlushSubnormal(f32 value)
{
	union {
		f32 value;
		u32 bits;
	} result;

	result.value = value;
	if ((result.bits & 0x7F800000u) == 0) {
		result.bits &= 0x80000000u;
	}
	return result.value;
}

static void StoreTransformedVector(Vec* out, const Vec* result)
{
	// The paired floating-point stores flush subnormal results to signed zero.
	out->x = FlushSubnormal(result->x);
	out->y = FlushSubnormal(result->y);
	out->z = FlushSubnormal(result->z);
}

static inline f32 TransformPointRow(const f32 row[4], const Vec* in)
{
	f32 xz = __fmadds(row[2], in->z, row[0] * in->x);
	f32 yw = __fmadds(row[3], 1.0f, row[1] * in->y);
	return xz + yw;
}

/**
 * @note Address: 0x8016C2C0
 * @note Size: 0x54
 */
void PSMTXMultVec(const Mtx m, const Vec* in, Vec* out)
{
	Vec result;
	result.x = TransformPointRow(m[0], in);
	result.y = TransformPointRow(m[1], in);
	result.z = TransformPointRow(m[2], in);
	StoreTransformedVector(out, &result);
}

static inline f32 TransformPointArrayRow(const f32 row[4], const Vec* in)
{
	f32 value = __fmadds(row[0], in->x, row[3]);
	value     = __fmadds(row[1], in->y, value);
	return __fmadds(row[2], in->z, value);
}

/**
 * @note Address: 0x8016C314
 * @note Size: 0x8C
 */
void PSMTXMultVecArray(const Mtx m, const Vec* in, Vec* out, u32 count)
{
	u32 i;

	// The first two rows accumulate from the translation term. The last row sums two pairs.
	for (i = 0; i < count; ++i) {
		Vec result;
		result.x = TransformPointArrayRow(m[0], &in[i]);
		result.y = TransformPointArrayRow(m[1], &in[i]);
		result.z = TransformPointRow(m[2], &in[i]);
		StoreTransformedVector(&out[i], &result);
	}
}

static inline f32 TransformDirectionRow(const f32 row[4], const Vec* in)
{
	f32 xy = row[0] * in->x + row[1] * in->y;
	return __fmadds(row[2], in->z, xy);
}

/**
 * @note Address: 0x8016C3A0
 * @note Size: 0x54
 */
void PSMTXMultVecSR(const Mtx m, const Vec* in, Vec* out)
{
	Vec result;
	result.x = TransformDirectionRow(m[0], in);
	result.y = TransformDirectionRow(m[1], in);
	result.z = TransformDirectionRow(m[2], in);
	StoreTransformedVector(out, &result);
}

static inline f32 TransformDirectionArrayRow(const f32 row[4], const Vec* in)
{
	f32 xy = __fmadds(row[1], in->y, row[0] * in->x);
	return __fmadds(row[2], in->z, xy);
}

/**
 * @note Address: N/A
 * @note Size: 0x88
 */
void PSMTXMultVecArraySR(const Mtx m, const Vec* in, Vec* out, u32 count)
{
	u32 i;

	for (i = 0; i < count; ++i) {
		const Vec* src = &in[i];
		// The last row combines the X and Z products before adding Y.
		f32 xz         = __fmadds(m[2][2], src->z, m[2][0] * src->x);
		Vec result;
		result.x = TransformDirectionArrayRow(m[0], src);
		result.y = TransformDirectionArrayRow(m[1], src);
		result.z = xz + m[2][1] * src->y;
		StoreTransformedVector(&out[i], &result);
	}
}
