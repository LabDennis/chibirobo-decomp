#include "Version.h"
#include "game/cpad.h"

#if defined(VERSION_R24J01)
// on Wii the emitted body is the minimal one (retail 0x800365D8, past game/wii/init.cpp's range);
// the GameCube copy is the weak one game/fbmirrorex.cpp emits
CPad::~CPad()
{
}
#endif
