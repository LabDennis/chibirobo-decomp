#ifndef GAME_WII_CSOUNDFX_H
#define GAME_WII_CSOUNDFX_H

#include "types.h"
#include "nw4r/snd/snd_FxReverbHiDpl2.h"

// an effect XSound owns, class names from RTTI. The work buffer comes from SoundHeapAlloc
class CSoundFx {
public:
	void* mWorkBuffer; // _00

	virtual BOOL Setup(const nw4r::snd::detail::FxReverbHiParam& param) { return TRUE; } // _08, 0x80033140
};

// XSound keeps one at _B60; Setup (0x80033084) sets the parameters and assigns the work buffer
class CSoundFxReverbHiDpl2 : public CSoundFx, public nw4r::snd::FxReverbHiDpl2 {
public:
	virtual BOOL Setup(const nw4r::snd::detail::FxReverbHiParam& param); // _08
};

#endif
