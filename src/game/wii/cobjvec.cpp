// The unit between mem.cpp and cgs.cpp, .text 0x802A7FB4-0x802A80FC with its one
// float in .sdata2 0x805A78C0: CObjVec::Step, which the GameCube keeps in cobj.cpp.
// The original file name is unknown. The step count arrives as a float here; every
// caller converts its frame count before the call.
#include "game/wii/xobj.h"

inline bool CObjVec::IsMoving() const
{
	bool result = false;
	if ((x != mTarget.x && 0.0f != mSpeed.x) || (y != mTarget.y && 0.0f != mSpeed.y) || (z != mTarget.z && 0.0f != mSpeed.z)) {
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

	if (x < mTarget.x) {
		if ((x += mSpeed.x * frames) > mTarget.x) {
			x = mTarget.x;
		}
	} else if (x > mTarget.x) {
		if ((x -= mSpeed.x * frames) < mTarget.x) {
			x = mTarget.x;
		}
	}
	if (y < mTarget.y) {
		if ((y += mSpeed.y * frames) > mTarget.y) {
			y = mTarget.y;
		}
	} else if (y > mTarget.y) {
		if ((y -= mSpeed.y * frames) < mTarget.y) {
			y = mTarget.y;
		}
	}
	if (z < mTarget.z) {
		if ((z += mSpeed.z * frames) > mTarget.z) {
			z = mTarget.z;
		}
	} else if (z > mTarget.z) {
		if ((z -= mSpeed.z * frames) < mTarget.z) {
			z = mTarget.z;
		}
	}
	return moving;
}
