#include "game/ccameraaction.h"

// The Wii unit 0x80032C48-0x8003366C (wii_units.md #19, candidate
// game/cinterpolationeaseoutdivide.cpp): the divide interpolations plus the
// Linear/None Update tail-calls, in the reverse of the GameCube file's order.
// The Wii-only tail (0x80032FD4 on: deleting destructors, CThread::Create, the
// unit's __sinit) is not reconstructed yet.

/**
 * @note Address: 0x80032C48
 * @note Size: 0x8
 */
f32 CInterpolationEaseOutDivide::Get()
{
	return mValue;
}

/**
 * @note Address: 0x80032C50
 * @note Size: 0xB4
 */
bool CVectorEaseOutDivideInterpolation::Step(f32 dt)
{
	bool x = mAxes[0].Step(dt);
	bool y = mAxes[1].Step(dt);
	bool z = mAxes[2].Step(dt);
	return x && y && z;
}

/**
 * @note Address: 0x80032D04
 * @note Size: 0x50
 */
bool CInterpolationEaseOutDivide::Step(f32 dt)
{
	f32 last = mValue;
	if (mDivisor > 0.0f) {
		mValue = (last * (mDivisor - 1.0f) + mTarget) / mDivisor;
	}
	return (f32)fabs(mValue - last) < 0.001f;
}

/**
 * @note Address: 0x80032D54
 * @note Size: 0xE0
 */
bool CVectorEaseOutDivideInterpolation::Update(u32 step, f32 dt)
{
	bool x = mAxes[0].Update(step, dt);
	bool y = mAxes[1].Update(step, dt);
	bool z = mAxes[2].Update(step, dt);
	if (x && z) { // x and z have both arrived
		mXZDone = TRUE;
	}
	return x && y && z;
}

/**
 * @note Address: 0x80032E34
 * @note Size: 0x80
 */
bool CInterpolationEaseOutDivide::Update(u32 step, f32 dt)
{
	bool done = false;
	for (u32 i = 0; i < step; i++) {
		done = Step(dt);
		if (done == true) {
			break;
		}
	}
	return done;
}

/**
 * @note Address: 0x80032EB4
 * @note Size: 0xD4
 */
bool CVectorEaseOutDivideInterpolation::Start(Vec from, Vec to, f32 divisor)
{
	if (divisor <= 0.0f) {
		return false;
	}
	mAxes[0].Start(from.x, to.x, divisor);
	mAxes[1].Start(from.y, to.y, divisor);
	mAxes[2].Start(from.z, to.z, divisor);
	mXZDone = FALSE;
	return true;
}

/**
 * @note Address: 0x80032F88
 * @note Size: 0x2C
 */
bool CInterpolationEaseOutDivide::Start(f32 from, f32 to, f32 divisor)
{
	if (divisor <= 0.0f) {
		return false;
	}
	mValue   = from;
	mTarget  = to;
	mDivisor = divisor;
	return true;
}

/**
 * @note Address: 0x80032FB4
 * @note Size: 0x10
 */
bool CInterpolationLinear::Update(u32 step, f32 dt)
{
	return Step(dt);
}

/**
 * @note Address: 0x80032FC4
 * @note Size: 0x10
 */
bool CInterpolationNone::Update(u32 step, f32 dt)
{
	return Step(dt);
}
