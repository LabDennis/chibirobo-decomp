#ifndef JSYSTEM_JMATH_INLINE_H
#define JSYSTEM_JMATH_INLINE_H

namespace JMathInlineVEC {
inline void PSVECAdd(const Vec* vec1, const Vec* vec2, Vec* dst)
{
	dst->x = vec1->x + vec2->x;
	dst->y = vec1->y + vec2->y;
	dst->z = vec1->z + vec2->z;
}

inline void PSVECSubtract(const Vec* vec1, const Vec* vec2, Vec* dst)
{
	dst->x = vec1->x - vec2->x;
	dst->y = vec1->y - vec2->y;
	dst->z = vec1->z - vec2->z;
}

inline void PSVECScale(const Vec* src, Vec* dst, f32 scalar)
{
	dst->x = src->x * scalar;
	dst->y = src->y * scalar;
	dst->z = src->z * scalar;
}

inline f32 PSVECDotProduct(const Vec* vec1, const Vec* vec2)
{
	return vec1->x * vec2->x + vec1->y * vec2->y + vec1->z * vec2->z;
}

inline f32 PSVECSquareMag(const Vec* src)
{
	return src->x * src->x + src->y * src->y + src->z * src->z;
}

} // namespace JMathInlineVEC

#endif
