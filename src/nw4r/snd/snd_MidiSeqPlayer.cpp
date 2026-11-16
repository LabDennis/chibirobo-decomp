/* Nothing of MidiSeqPlayer itself is in the dol: the linker dropped the class. What retail keeps of this
 * file (0x8028EF34-0x8028EF88) are SeqPlayer's weak inline virtuals and thunks, which it emits as the
 * first file in link order that needs them. The constructor is as in ss's snd_MidiSeqPlayer.cpp;
 * wii-ipl's libs/NW4R has the whole class. */

#include "nw4r/snd/snd_MidiSeqPlayer.h"

nw4r::snd::detail::MidiSeqPlayer::MidiSeqPlayer() {}
