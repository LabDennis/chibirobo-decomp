#ifndef REL_STAGE01_HAMBURG_H
#define REL_STAGE01_HAMBURG_H

#include "game/wii/xobj.h"
#include "game/wii/ceffect.h"

struct HamDish;

struct HamObj {
	typedef void (HamObj::*StateFunc)();

	void SetState(StateFunc func)
	{
		mState     = func;
		mStateTime = 0;
	}
	void StateFlip();
	void StateSlide();
	void StatePutDown();
	void StateStackLand();

	f32 GetStackHeight();

	void SetRot(f32 rot)
	{
		mRotZ = rot;
		mActor->SetRotationZ(mRotZ);
	}

	void StartSlide(f32 distance, f32 speed, f32 angle)
	{
		if (mState != &HamObj::StateFlip) {
			mMoveAngle     = angle;
			mSlideDistance = distance;
			mMoveSpeed     = speed;
			SetState(&HamObj::StateSlide);
		}
	}

	HamObj* mAbove;
	XObj* mActor;
	s32 mId;
	s32 mType;
	f32 mRotZ;
	f32 mFlipFrames;
	f32 mMoveAngle;
	f32 mSlideDistance;
	f32 mMoveSpeed;
	s32 mSide;
	s32 mYakiTime[2];
	s32 mStackLevel;
	HamDish* mDish;
	u8 mSpawned;
	u8 _39[3];
	EffectInstance* mYakiEffect;
	StateFunc mState;
	s32 mStateTime;
};

struct HamDish {
	XObj* mActor;
	s32 mId;
	HamObj* mObjs[4];
};

#endif
