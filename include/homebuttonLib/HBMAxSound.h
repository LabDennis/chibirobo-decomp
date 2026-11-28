#ifndef HOME_BUTTON_LIB_AX_SOUND_H
#define HOME_BUTTON_LIB_AX_SOUND_H
#include <types.h>

namespace homebutton {

// The menu's own sequence player on top of AX: the sounds are MIDI files in
// the sound archive, played through the library's MIX/SYN/SEQ copy.
void PlaySeq(int num);
void InitAxSound(const void* pSoundData, void* pMem, u32 memSize);
void ShutdownAxSound();
void AxSoundMain();
void StopAllSeq();
void SetVolumeAllSeq(f32 gain);
void SetSoundMode(u32 mode);

} // namespace homebutton

#endif
