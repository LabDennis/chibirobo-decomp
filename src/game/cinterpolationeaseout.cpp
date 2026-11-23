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
// todo: Start stays a stand-in under its mangled name, as upstream has it: defining the key function here
// makes the vtables (US 0x8022E504, 0x8022E52C) come out in this unit
extern "C" bool Start__23CInterpolationEaseInOutFfff(CInterpolationEaseInOut* interp, f32 from, f32 to, f32 time)
{
	if (time <= 0.0f) {
		return false;
	}
	interp->mValue    = from;
	interp->mTarget   = to;
	interp->mSpeed    = 0.0f;
	interp->mAccel    = (to - from) / (time * time / 4.0f);
	interp->mDuration = time;
	interp->mTime     = time;
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
extern "C" bool Start__21CInterpolationEaseOutFfff(CInterpolationEaseOut* interp, f32 from, f32 to, f32 time)
{
	if (time <= 0.0f) {
		return false;
	}
	interp->mValue    = from;
	interp->mStart    = from;
	interp->mTarget   = to;
	interp->mSpeed    = (2.0f * (to - from)) / time;
	interp->mAccel    = -interp->mSpeed / time; // the speed decays to zero at the end
	interp->mDuration = time;
	interp->mTime     = time;
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
