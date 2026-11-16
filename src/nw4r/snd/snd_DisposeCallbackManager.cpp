#include "nw4r/snd/snd_DisposeCallbackManager.h"

/* Original source:
 * kiwi515/ogws
 * src/nw4r/snd/snd_DisposeCallbackManager.cpp
 */

/*******************************************************************************
 * headers
 */

#include "nw4r/types_nw4r.h"

#include "nw4r/snd/snd_SoundThread.h"

#include "nw4r/ut/ut_lock.h"

/*******************************************************************************
 * functions
 */

namespace nw4r { namespace snd { namespace detail {

DisposeCallbackManager &DisposeCallbackManager::GetInstance()
{
	ut::AutoInterruptLock lock; // What

	static DisposeCallbackManager instance;

	return instance;
}

DisposeCallbackManager::DisposeCallbackManager() {}

void DisposeCallbackManager::RegisterDisposeCallback(DisposeCallback *callback)
{
	mCallbackList.PushBack(callback);
}

void DisposeCallbackManager::UnregisterDisposeCallback(
	DisposeCallback *callback)
{
	mCallbackList.Erase(callback);
}

void DisposeCallbackManager::Dispose(void *mem, u32 size, void *arg)
{
	void *start = mem;
	void *end = static_cast<u8 *>(mem) + size;

	SoundThread::AutoLock lock;

	// NOTE: unnecessary call to GetInstance from instance-method
	NW4R_RANGE_FOR_NO_AUTO_INC(itr, GetInstance().mCallbackList)
	{
		__decltype__(itr) curItr = itr++;

		curItr->InvalidateData(start, end);
	}
}

void DisposeCallbackManager::DisposeWave(void *mem, u32 size,
                                         void *arg)
{
	void *start = mem;
	void *end = static_cast<u8 *>(mem) + size;

	SoundThread::AutoLock lock;

	// same stuff here as the stuff over there
	NW4R_RANGE_FOR_NO_AUTO_INC(itr, GetInstance().mCallbackList)
	{
		__decltype__(itr) curItr = itr++;

		curItr->InvalidateWaveData(start, end);
	}
}

}}} // namespace nw4r::snd::detail
