#include "Version.h"
#if !defined(VERSION_R24J01)
#include "game/hsd_jobj.h"
#endif
#include "game/ccameraaction.h"

// Written in the original order: retail's Wii code (-ipa file) has the functions in this order,
// the GameCube code (deferred inlining emits them last first) in the reverse one.

/**
 * @note Address: 0x80135398
 * @note Size: 0x4C
 */
bool CInterpolationEaseInOut::Start(f32 from, f32 to, f32 time)
{
	if (time <= 0.0f) {
		return false;
	}
	mValue    = from;
	mTarget   = to;
	mSpeed    = 0.0f;
	mAccel    = (to - from) / (time * time / 4.0f);
	mDuration = time;
	mTime     = time;
	return true;
}

/**
 * @note Address: 0x80135314
 * @note Size: 0x84
 */
bool CInterpolationEaseInOut::Update(u32 step, f32 dt)
{
	bool done = false;
	for (u32 i = 0; i < step; ++i) {
		done = Step(dt);
		if (done == true) {
			break;
		}
	}
	return done;
}

/**
 * @note Address: 0x80135280
 * @note Size: 0x94
 */
bool CInterpolationEaseInOut::Step(f32 dt)
{
	mTime -= dt;
	if (mTime < mDuration / 2.0f) { // accelerate to the midpoint, then brake
		mSpeed -= mAccel * dt;
	} else {
		mSpeed += mAccel * dt;
	}
	if (mTime <= 0.0f) {
		mValue = mTarget;
	} else {
		mValue += mSpeed * dt;
	}
	return mTime <= 0.0f;
}

/**
 * @note Address: 0x80135278
 * @note Size: 0x8
 */
f32 CInterpolationEaseInOut::Get()
{
	return mValue;
}

/**
 * @note Address: 0x80135220
 * @note Size: 0x58
 */
bool CInterpolationEaseOut::Start(f32 from, f32 to, f32 time)
{
	if (time <= 0.0f) {
		return false;
	}
	mValue    = from;
	mStart    = from;
	mTarget   = to;
	mSpeed    = (2.0f * (to - from)) / time;
	mAccel    = -mSpeed / time; // the speed decays to zero at the end
	mDuration = time;
	mTime     = time;
	return true;
}

/**
 * @note Address: 0x8013519C
 * @note Size: 0x84
 */
bool CInterpolationEaseOut::Update(u32 step, f32 dt)
{
	bool done = false;
	for (u32 i = 0; i < step; ++i) {
		done = Step(dt);
		if (done == true) {
			break;
		}
	}
	return done;
}

/**
 * @note Address: 0x80135120
 * @note Size: 0x7C
 */
bool CInterpolationEaseOut::Step(f32 dt)
{
	mTime -= dt;
	f32 elapsed = mDuration - mTime;
	mSpeed += mAccel * dt / 2.0f;
	if (mTime <= 0.0f) {
		mValue = mTarget;
	} else {
		mValue = mSpeed * elapsed + mStart;
	}
	return mTime <= 0.0f;
}

/**
 * @note Address: 0x80135118
 * @note Size: 0x8
 */
f32 CInterpolationEaseOut::Get()
{
	return mValue;
}
