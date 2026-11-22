// The unit between mem.cpp and cgs.cpp, .text 0x802A7FB4-0x802A80FC with its one
// float in .sdata2 0x805A78C0: CObjVec::Step, which the GameCube keeps in cobj.cpp.
// The original file name is unknown. The step count arrives as a float here; every
// caller converts its frame count before the call.
#include "game/wii/xobj.h"

inline bool CObjVec::IsMoving() const
{
	bool result = false;
	if ((mValue.x != mTarget.x && 0.0f != mSpeed.x) || (mValue.y != mTarget.y && 0.0f != mSpeed.y)
	    || (mValue.z != mTarget.z && 0.0f != mSpeed.z)) {
		result = true;
	}
	return result;
}

/**
 * @note Address: 0x802A7FB4
 * @note Size: 0x148
 */
u8 CObjVec::Step(f32 frames)
{
	u8 moving = IsMoving();

	if (mValue.x < mTarget.x) {
		if ((mValue.x += mSpeed.x * frames) > mTarget.x) {
			mValue.x = mTarget.x;
		}
	} else if (mValue.x > mTarget.x) {
		if ((mValue.x -= mSpeed.x * frames) < mTarget.x) {
			mValue.x = mTarget.x;
		}
	}
	if (mValue.y < mTarget.y) {
		if ((mValue.y += mSpeed.y * frames) > mTarget.y) {
			mValue.y = mTarget.y;
		}
	} else if (mValue.y > mTarget.y) {
		if ((mValue.y -= mSpeed.y * frames) < mTarget.y) {
			mValue.y = mTarget.y;
		}
	}
	if (mValue.z < mTarget.z) {
		if ((mValue.z += mSpeed.z * frames) > mTarget.z) {
			mValue.z = mTarget.z;
		}
	} else if (mValue.z > mTarget.z) {
		if ((mValue.z -= mSpeed.z * frames) < mTarget.z) {
			mValue.z = mTarget.z;
		}
	}
	return moving;
}
