#include "game/ccameraaction.h"

// The Wii interpolation unit 0x80032038-0x80032C48 (wii_units.md #18, candidate
// game/cinterpolationnone.cpp): all the vector interpolation methods and the
// Linear/None scalar methods live in one Wii TU whose functions sit in the
// reverse of the GameCube files' order. The bodies are shared with the
// GameCube files (game/cinterfacecameraaction.cpp, game/cinterpolationnone.cpp).

/**
 * @note Address: 0x80032038
 * @note Size: 0xB8
 */
Vec CVectorEaseOutInterpolation::Get()
{
	return CVec(mAxes[0].Get(), mAxes[1].Get(), mAxes[2].Get());
}

/**
 * @note Address: 0x800320F0
 * @note Size: 0xB4
 */
bool CVectorEaseOutInterpolation::Step(f32 dt)
{
	bool x = mAxes[0].Step(dt);
	bool y = mAxes[1].Step(dt);
	bool z = mAxes[2].Step(dt);
	return x && y && z;
}

/**
 * @note Address: 0x800321A4
 * @note Size: 0xC8
 */
bool CVectorEaseOutInterpolation::Update(u32 step, f32 dt)
{
	bool x = mAxes[0].Update(step, dt);
	bool y = mAxes[1].Update(step, dt);
	bool z = mAxes[2].Update(step, dt);
	return x && y && z;
}

/**
 * @note Address: 0x8003226C
 * @note Size: 0xCC
 */
bool CVectorEaseOutInterpolation::Start(Vec from, Vec to, f32 time)
{
	if (time <= 0.0f) {
		return false;
	}
	mAxes[0].Start(from.x, to.x, time);
	mAxes[1].Start(from.y, to.y, time);
	mAxes[2].Start(from.z, to.z, time);
	return true;
}

/**
 * @note Address: 0x80032338
 * @note Size: 0xB8
 */
Vec CVectorEaseInOutInterpolation::Get()
{
	return CVec(mAxes[0].Get(), mAxes[1].Get(), mAxes[2].Get());
}

/**
 * @note Address: 0x800323F0
 * @note Size: 0xB4
 */
bool CVectorEaseInOutInterpolation::Step(f32 dt)
{
	bool x = mAxes[0].Step(dt);
	bool y = mAxes[1].Step(dt);
	bool z = mAxes[2].Step(dt);
	return x && y && z;
}

/**
 * @note Address: 0x800324A4
 * @note Size: 0xE0
 */
bool CVectorEaseInOutInterpolation::Update(u32 step, f32 dt)
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
 * @note Address: 0x80032584
 * @note Size: 0xD4
 */
bool CVectorEaseInOutInterpolation::Start(Vec from, Vec to, f32 time)
{
	if (time <= 0.0f) {
		return false;
	}
	mAxes[0].Start(from.x, to.x, time);
	mAxes[1].Start(from.y, to.y, time);
	mAxes[2].Start(from.z, to.z, time);
	mXZDone = FALSE;
	return true;
}

/**
 * @note Address: 0x80032658
 * @note Size: 0xB8
 */
Vec CVectorLinearInterpolation::Get()
{
	return CVec(mAxes[0].Get(), mAxes[1].Get(), mAxes[2].Get());
}

/**
 * @note Address: 0x80032710
 * @note Size: 0x8
 */
f32 CInterpolationLinear::Get()
{
	return mValue;
}

/**
 * @note Address: 0x80032718
 * @note Size: 0x10
 */
bool CVectorLinearInterpolation::Update(u32 step, f32 dt)
{
	return Step(dt);
}

/**
 * @note Address: 0x80032728
 * @note Size: 0xB4
 */
bool CVectorLinearInterpolation::Step(f32 dt)
{
	bool x = mAxes[0].Step(dt);
	bool y = mAxes[1].Step(dt);
	bool z = mAxes[2].Step(dt);
	return x && y && z;
}

/**
 * @note Address: 0x800327DC
 * @note Size: 0x34
 */
bool CInterpolationLinear::Step(f32 dt)
{
	mTime -= dt;
	mValue += mSpeed * dt;
	return mTime <= 0.0f;
}

/**
 * @note Address: 0x80032810
 * @note Size: 0xC8
 */
bool CVectorLinearInterpolation::Start(Vec from, Vec to, f32 time)
{
	if (time < 0.0f) {
		return false;
	}
	mAxes[0].Start(from.x, to.x, time);
	mAxes[1].Start(from.y, to.y, time);
	mAxes[2].Start(from.z, to.z, time);
	return true;
}

/**
 * @note Address: 0x800328D8
 * @note Size: 0x30
 */
bool CInterpolationLinear::Start(f32 from, f32 to, f32 time)
{
	if (time < 0.0f) { // a negative time means no interp
		return false;
	}
	mTime  = time;
	mValue = from;
	mSpeed = (to - from) / time;
	return true;
}

/**
 * @note Address: 0x80032908
 * @note Size: 0xB8
 */
Vec CVectorNoneInterpolation::Get()
{
	return CVec(mAxes[0].Get(), mAxes[1].Get(), mAxes[2].Get());
}

/**
 * @note Address: 0x800329C0
 * @note Size: 0x8
 */
f32 CInterpolationNone::Get()
{
	return mValue;
}

/**
 * @note Address: 0x800329C8
 * @note Size: 0x10
 */
bool CVectorNoneInterpolation::Update(u32 step, f32 dt)
{
	return Step(dt);
}

/**
 * @note Address: 0x800329D8
 * @note Size: 0xB4
 */
bool CVectorNoneInterpolation::Step(f32 dt)
{
	bool x = mAxes[0].Step(dt);
	bool y = mAxes[1].Step(dt);
	bool z = mAxes[2].Step(dt);
	return x && y && z;
}

/**
 * @note Address: 0x80032A8C
 * @note Size: 0x20
 */
bool CInterpolationNone::Step(f32 dt)
{
	mTime = 1.0f;
	return mTime <= 0.0f;
}

/**
 * @note Address: 0x80032AAC
 * @note Size: 0xC8
 */
bool CVectorNoneInterpolation::Start(Vec from, Vec to, f32 time)
{
	if (time < 0.0f) {
		return false;
	}
	mAxes[0].Start(from.x, to.x, time);
	mAxes[1].Start(from.y, to.y, time);
	mAxes[2].Start(from.z, to.z, time);
	return true;
}

/**
 * @note Address: 0x80032B74
 * @note Size: 0x1C
 */
bool CInterpolationNone::Start(f32 from, f32 to, f32 time)
{
	mTime  = 1.0f;
	mValue = to;
	mSpeed = 0.0f;
	return true;
}

/**
 * @note Address: 0x80032B90
 * @note Size: 0xB8
 */
Vec CVectorEaseOutDivideInterpolation::Get()
{
	return CVec(mAxes[0].Get(), mAxes[1].Get(), mAxes[2].Get());
}
