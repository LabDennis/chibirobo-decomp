#include "game/cgame.h"
#include "game/cthread.h"
#include <Dolphin/vi.h>

extern "C" {
void* UpdateMovie(void* arg);
extern CThread gMovieUpdateThread;
extern u8 gMovieUpdateThreadStack[0x1000];
}

#if defined(VERSION_JP)
enum { MovieErrorLine = 3780 };
#elif defined(VERSION_PAL)
enum { MovieErrorLine = 3917 };
#else
enum { MovieErrorLine = 3832 };
#endif

/**
 * @note Address: 0x8000AB90
 * @note Size: 0x28
 */
extern "C" void StopGameMovie(CGame* game)
{
	game->mMovie.mMovie.Stop();
}

/**
 * @note Address: 0x8000ABB8
 * @note Size: 0xAC
 */
// starts a movie and the thread that plays it, bit 0 of the flags plays it with alpha
extern "C" void StartGameMovie(CGame* game, const char* path, BOOL loop, u32 flags)
{
	game->mMovieFlags = flags;
	game->mMovie.mMovie.Start(path, loop, (flags & 1) != 0, TRUE);
	game->mMovieRect[0] = game->mMovieRect[1] = game->mMovieRect[2] = game->mMovieRect[3] = NAN;
	gMovieUpdateThread.Start(UpdateMovie, game, gMovieUpdateThreadStack, sizeof(gMovieUpdateThreadStack));
}

/**
 * @note Address: 0x8000AC64
 * @note Size: 0xD0
 */
// UpdateMovie, the movie thread: steps the movie once a retrace until it's done
extern "C" void* UpdateMovie(void* arg)
{
	s32 result;
	CMovie* movie;
	u32 count = VIGetRetraceCount();
	movie     = &((CGame*)arg)->mMovie.mMovie;
	do {
		while (count == VIGetRetraceCount()) {
			VIWaitForRetrace();
		}
		count = VIGetRetraceCount();
		if (movie->mState != 0) {
			result = movie->Update();
			if (result < 0) {
				movie->Stop();
				OSPanic("game.cpp", MovieErrorLine, "%s%s", FormatToStaticBuffer("error on play movie! - %d\n", result), __FUNCTION__);
			}
		}
	} while (movie->mState != 0);
	return NULL;
}
