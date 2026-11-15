// The unit between cpadact.cpp and savemanager.cpp, .text 0x800A92CC-0x800A9320:
// an out-of-line copy of CCamera::SetAngle(const Vec&) (vtable slot _50) and the
// static initializer of a CThread global at 0x80453610 that nothing else
// references - a leftover of the port.
// NONMATCHING: both emitted functions match retail byte for byte, but the
// compiler's .bss layout differs (retail keeps a 0x10-byte global-object
// record at 0x80453600, we emit a 0xC-byte one), so the unit stays unlinked.
#include "game/wii/cgame.h"
#include "game/cthread.h"

CThread gWiiCameraThread;

void CCamera::SetAngle(const Vec& angle)
{
	CCamera::SetAngle(angle.x, angle.y, angle.z);
}
