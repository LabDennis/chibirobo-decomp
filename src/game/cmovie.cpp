#include "game/cmovie.h"
#include "game/mm.h"
#include "game/THPSimple.h"
#include "Dolphin/os.h"
#include <string.h>

/**
 * @note Address: 0x801A9F30
 * @note Size: 0x4C
 */
void CMovie::Init()
{
	THPSimpleInit(1);
	mState  = CMOVIE_STATE_IDLE;
	mLoop   = TRUE;
	mBuffer = NULL;
	mFrame  = -1;
}

#if defined(VERSION_R24J01)
/**
 * @note Address: 0x802C22C4
 * @note Size: 0x68
 */
void CMovie::Start(const char* path, f32 volume, BOOL loop, s32 volumeTime, BOOL sound)
{
	mState      = CMOVIE_STATE_OPEN;
	mLoop       = loop;
	mVolume     = volume * 127.0f;
	mVolumeTime = volumeTime;
	strcpy(mPath, path);
	THPSimpleSetSound(sound);
	mFrame = -1;
}
#else
/**
 * @note Address: 0x801A9ED4
 * @note Size: 0x5C
 */
void CMovie::Start(const char* path, BOOL loop, BOOL alpha, BOOL sound)
{
	mState = CMOVIE_STATE_OPEN;
	mLoop  = loop;
	mAlpha = alpha;
	strcpy(mPath, path);
	THPSimpleSetSound(sound);
	mFrame = -1;
}
#endif

#if defined(VERSION_R24J01)
/**
 * @note Address: 0x802C232C
 * @note Size: 0x58
 */
void CMovie::Stop()
{
	mState = CMOVIE_STATE_IDLE;
	THPSimpleAudioStop();
	THPSimpleLoadStop();
	THPSimpleClose();
	if (mBuffer != NULL) {
		mm::Free(mBuffer);
		mBuffer = NULL;
	}
}
#else
/**
 * @note Address: 0x801A9E7C
 * @note Size: 0x58
 */
void CMovie::Stop()
{
	mState = CMOVIE_STATE_IDLE;
	THPSimpleAudioStop();
	THPSimpleLoadStop();
	THPSimpleClose();
	if (mBuffer != NULL) {
		mm::Free(mBuffer, 1);
		mBuffer = NULL;
	}
}
#endif

#if defined(VERSION_R24J01)
/**
 * @note Address: 0x802C2384
 * @note Size: 0x1E0
 */
s32 CMovie::Update()
{
	s32 result;
	u32 size;

	switch (mState) {
	case CMOVIE_STATE_OPEN:
		if (THPSimpleOpen(mPath) > 0) {
			mState = CMOVIE_STATE_INFO;
		} else {
			mState = -2; // negative states are error codes
			break;
		}
		// fall through
	case CMOVIE_STATE_INFO:
		result = THPSimpleOpenWait();
		if (result > 0) {
			THPSimpleGetVideoInfo(&mVideoInfo);
			THPSimpleGetAudioInfo(&mAudioInfo);
			size    = THPSimpleCalcNeedMemory();
			mBuffer = mm::Alloc(size, "(movie)");
			if (mBuffer != NULL) {
				mState = CMOVIE_STATE_PRELOAD;
				THPSimpleSetBuffer(mBuffer);
			} else {
				mState = -1; // out of memory
			}
			mLastFrame = THPSimpleGetTotalFrame() - 1;
			THPSimpleSetVolume(mVolume, 0);
			// what is left of the GameCube's size report, the kilobyte and megabyte figures.
			// NONMATCHING (data): retail's .sdata2 holds neither 1024.0f, 1048576.0f nor the
			// conversion constant, so its report took the figures some other way; the code is retail's
			f32 kb = size / 1024.0f;
			f32 mb = size / 1048576.0f;
		} else if (result < 0) {
			mState = -2;
		}
		break;
	case CMOVIE_STATE_PRELOAD:
		result = THPSimplePreLoad(mLoop);
		if (result > 0) {
			mState = CMOVIE_STATE_READY;
		} else if (result < 0) {
			mState = -3;
		}
		break;
	case CMOVIE_STATE_READY:
		result = THPSimplePreLoadWait();
		if (result > 0) {
			mState = CMOVIE_STATE_START;
		} else {
			if (result < 0) {
				mState = -3;
			}
			break;
		}
		// fall through
	case CMOVIE_STATE_START:
		mState = CMOVIE_STATE_PLAY;
		THPSimpleAudioStart();
		// fall through
	case CMOVIE_STATE_PLAY:
		if (mFrame >= mLastFrame && !mLoop) { // the last frame ends a non-loop
			Stop();
		} else {
			switch (THPSimpleDecode(mAudioInfo.sndNumTracks - 1)) {
			case 1:
				mState = -4;
				break;
			case 2:
			case 3:
				break;
			}
		}
		break;
	}
	return 0;
}
#else
/**
 * @note Address: 0x801A9C18
 * @note Size: 0x264
 */
s32 CMovie::Update()
{
	s32 result;
	u32 size;

	switch (mState) {
	case CMOVIE_STATE_OPEN:
		if (THPSimpleOpen(mPath) > 0) {
			mState = CMOVIE_STATE_INFO;
		} else {
			OSReport("file not found - %s\n", mPath);
			mState = -2; // negative states are error codes
			break;
		}
		// fall through
	case CMOVIE_STATE_INFO:
		result = THPSimpleOpenWait();
		if (result > 0) {
			THPSimpleGetVideoInfo(&mVideoInfo);
			THPSimpleGetAudioInfo(&mAudioInfo);
			size    = THPSimpleCalcNeedMemory();
			mBuffer = mm::Alloc(size, "(movie)");
			if (mBuffer != NULL) {
				mState = CMOVIE_STATE_PRELOAD;
				THPSimpleSetBuffer(mBuffer);
			} else {
				OSReport("Cannot allocate the memory for movie - %d\n", size);
				mState = -1; // out of memory
			}
			mLastFrame = THPSimpleGetTotalFrame() - 1;
			OSReport("[%-26s] (%dx%d) %7d(%6.1fK)(%5.3fM)\n", mPath, mVideoInfo.xSize, mVideoInfo.ySize, size, size / 1024.0f,
			         size / 1048576.0f);
		} else if (result < 0) {
			mState = -2;
		}
		break;
	case CMOVIE_STATE_PRELOAD:
		result = THPSimplePreLoad(mLoop);
		if (result > 0) {
			mState = CMOVIE_STATE_READY;
		} else if (result < 0) {
			mState = -3;
		}
		break;
	case CMOVIE_STATE_READY:
		result = THPSimplePreLoadWait();
		if (result > 0) {
			mState = CMOVIE_STATE_START;
		} else {
			if (result < 0) {
				mState = -3;
			}
			break;
		}
		// fall through
	case CMOVIE_STATE_START:
		mState = CMOVIE_STATE_PLAY;
		THPSimpleAudioStart();
		// fall through
	case CMOVIE_STATE_PLAY:
		if (mFrame >= mLastFrame && !mLoop) { // the last frame ends a non-loop
			Stop();
		} else {
			switch (THPSimpleDecode(mAudioInfo.sndNumTracks - 1)) {
			case 1:
				mState = -4;
				break;
			case 2:
			case 3:
				// probably compiled-out prints
				break;
			}
		}
		break;
	}
	return 0;
}
#endif

#if !defined(VERSION_R24J01)
/**
 * @note Address: 0x801A9B9C
 * @note Size: 0x7C
 */
s32 CMovie::DrawFrame(f32 x, f32 y, f32 w, f32 h)
{
	if (mState >= CMOVIE_STATE_START) {
		mFrame = THPSimpleDrawCurrentFrame(x, y, w, h, mAlpha);
		return mFrame;
	}
	return -1;
}

/**
 * @note Address: 0x801A9B40
 * @note Size: 0x5C
 */
void CMovie::Draw(f32 x, f32 y, f32 w, f32 h)
{
	if (mState >= CMOVIE_STATE_START) {
		THPSimpleDrawQuad(x, y, w, h);
	}
}

/**
 * @note Address: 0x801A9AF4
 * @note Size: 0x4C
 */
s32 CMovie::BeginDraw()
{
	if (mState >= CMOVIE_STATE_START) {
		mFrame = THPSimpleDrawBegin(mAlpha);
		return mFrame;
	}
	return -1;
}

/**
 * @note Address: 0x801A9AD4
 * @note Size: 0x20
 */
void CMovie::EndDraw()
{
	THPSimpleDrawEnd();
}
#endif

#if defined(VERSION_R24J01)
/**
 * @note Address: 0x802C2564
 * @note Size: 0x84
 */
s32 CMovie::DrawFrame(f32 x, f32 y, f32 z, f32 w, f32 h)
{
	if (mState >= CMOVIE_STATE_START) {
		mFrame = THPSimpleDrawCurrentFrame(x, y, z, w, h, mVolumeTime);
		return mFrame;
	}
	return -1;
}

/**
 * @note Address: 0x802C25E8
 * @note Size: 0x68
 */
void CMovie::Draw(f32 x, f32 y, f32 z, f32 w, f32 h)
{
	if (mState >= CMOVIE_STATE_START) {
		THPSimpleDrawQuad(x, y, z, w, h);
	}
}

/**
 * @note Address: 0x802C2650
 * @note Size: 0x48
 */
s32 CMovie::BeginDraw()
{
	if (mState >= CMOVIE_STATE_START) {
		mFrame = THPSimpleDrawBegin(mVolumeTime);
		return mFrame;
	}
	return -1;
}

/**
 * @note Address: 0x802C2698
 * @note Size: 0x4
 */
void CMovie::EndDraw()
{
	THPSimpleDrawEnd();
}
#endif
