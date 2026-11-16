#include "game/xchr.h"

/**
 * @note Address: 0x800BC524
 * @note Size: 0x694
 */
// NONMATCHING: retail reaches the XObjChr, DrsMenu, OctaMenu and AtcMenu vtables through its file's
// .data base (lbl_802260C8 + 0x1E20 etc.), so they are defined in the same file; this split unit
// loads each by name and needs one register less
XObjChr::~XObjChr()
{
}
