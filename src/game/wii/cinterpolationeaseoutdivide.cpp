#include "game/ccameraaction.h"
#include "game/cthread.h"
#include "game/wii/cpadact.h"
#include "revolution/mem.h"

// The Wii unit 0x80032C48-0x8003366C (wii_units.md #19): the tail of the game's main file, the one
// that holds gGame. The divide interpolations come first, in the reverse of the GameCube file's
// order; behind them are the weak virtuals of the classes gGame is made of, and the file's static
// initializer. The weak virtuals whose bodies are in the headers (CFile::IsLoaded, the nw4r::snd
// ones) come out of this file once gGame's constructor does.

extern MEMiHeapHead* gWiiMem1Heap;
extern MEMiHeapHead* gWiiGameHeap;

// operator delete has an inline body on the Wii, so the deleting destructors free to the MEM1 heap
inline void operator delete(void* ptr) throw()
{
	MEMFreeToExpHeap(gWiiMem1Heap, ptr);
}

// NONMATCHING: the static initializer (0x800332D8, 0x394) is the compiler's. Retail's constructs
// gGame ahead of these two, which unit_8003366C.cpp defines, and sets gCmdLineArgs' crosshair
// (retail _54 and _58) to the middle of the pointer's screen.
CPadAct gWiiPointer;
extern "C" {
CThread gMovieUpdateThread;
}

// The weak virtuals of this unit that are written in their classes, and so are emitted by the file
// that constructs gGame (CThread::Create, 0x800331D4, among them):
//   0x80033058 CFile::IsLoaded                          include/game/cfile.h
//   0x80033080 CScrnWipeTile::Start                     include/game/cscrn.h
//   0x80033084 CSoundFxReverbHiDpl2::SetParam           include/game/wii/csoundfx.h
//   0x80033140 CSoundFx::SetParam                       include/game/wii/csoundfx.h
//   0x80033148 nw4r::snd::SoundArchivePlayer::detail_ConvertLabelStringToSoundId
//   0x800331B0 nw4r::snd::DvdSoundArchive::detail_GetWaveDataFileAddress
//   0x800331B8 nw4r::snd::DvdSoundArchive::detail_GetFileAddress
//
// NONMATCHING: five more are not written, because include/game/wii/xobj.h and title.h model their
// classes with another shape:
//   0x800331C0 the second virtual of NW_Obj, `return mData != NULL;` with mData at _00. NW_Obj is
//              0x5C bytes with its vtable at _58.
//   0x80033044 the override NW_Anm has for it, `return mFile.IsLoaded();`. NW_Anm is the 0x148 bytes
//              xobj.h calls CJArchive: NW_Obj and the CFile at _5C.
//   0x800332D4, 0x800332D0 the second and third virtual of nw4r::g3d::IScnObjCallback, both empty.
//              Its vtable (0x802E6A10) has six slots, not the 75 of xobj.h.
//   0x80033040 an empty virtual, the fourth of the base Title and MenuSave share (RTTI:
//              CContainCallback, MenuPad); title.h derives Title from MenuUtil alone.
