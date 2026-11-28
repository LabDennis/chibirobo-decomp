// The unit between cpadact.cpp and savemanager.cpp, .text 0x800A92CC-0x800A9320:
// an out-of-line copy of CCamera::SetAngle(const Vec&) (vtable slot _50) and the
// static initializer of a CThread global at 0x80453610 that nothing else
// references - a leftover of the port.
// Its .bss (0x80453600-0x80453938) holds the 0xC-byte global-object record,
// 4 bytes of alignment padding, and the CThread itself at 0x80453610.
#include "game/wii/cgame.h"
#include "game/cthread.h"

CThread gWiiCameraThread;

void CCamera::SetAngle(const Vec& angle)
{
	CCamera::SetAngle(angle.x, angle.y, angle.z);
}
