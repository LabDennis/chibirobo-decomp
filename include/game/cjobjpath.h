#ifndef _GAME_CJOBJPATH_H
#define _GAME_CJOBJPATH_H

#include "game/cjobj.h"

// A joint of the object's model by its child/next path, an inline of the original headers. Retail has one copy of it,
// at 0x800B4478 in obja_chr's run of small out-of-line inline copies (0x800B400C-0x800B4ECC, with CVec's constructor
// __ct__4CVecFfff at its end), and
// objb_act and xobjactortoy call that copy without carrying their own: the linker keeps only the first weak copy, as
// with CJObj::Attach, whose one copy is xobjactortoy's and which Jenny, Papa and Tao call. As an inline it also counts
// at every call in the caller's size estimate: the toy's ProcessPendingEvents calls it 31 times, and that is what
// brings the inline limit down so far that retail calls CJObj's destructor (11 times) and the small command handlers
// there instead of expanding them.
// obja_chr still defines it as a plain function and objb_act declares it: with the inline in cjobj.h, five of obja_chr's
// player functions expanded it where retail calls it (their own estimates are still short), so for now only the files
// that show its effect include this header.
extern "C" inline HSD_JObj* GetObjectJointByPath(const CJObj* object, s32 depth, s32* path)
{
	return GetJointByPath(object->mJObj, path, depth);
}

#endif
