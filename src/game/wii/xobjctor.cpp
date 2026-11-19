#include "game/wii/xobj.h"
#include "game/wii/xobjflags.h"

// the Wii pool subclasses' constructors and destructors, emitted together in
// retail .text 0x800C13B0-0x800C5424; this unit is the Wii counterpart of the
// GameCube xobjctor.cpp that carried the same block

// NONMATCHING: the ctor bodies' interior member initialization is only partly
// recovered; the ctors below emit the XObj prologue plus the member ctors the
// declarations provide, the rest of retail's field setup is not modeled yet

XObjChr::CPtclSet::CPtclSet()
{
}

XObjChr::CPtclSet::~CPtclSet()
{
}

// NONMATCHING: retail unwinds ~17 member lists (OXMenu, XObjChrMenu, XPad,
// Params and the anonymous records at _9CC.._D50 and _95D4.._1CB44); only the
// CPtclSet, CTexObj[2] and AnimSfxActor members are declared so far
XObjChr::~XObjChr()
{
}

XObjActor3::XObjActor3()
{
}

XObjActor3::~XObjActor3()
{
}

XObjSound::XObjSound()
{
}

XObjSound::~XObjSound()
{
}

XObjCoin::XObjCoin()
{
}

XObjCoin::~XObjCoin()
{
}

XObjItem::XObjItem()
{
}

XObjItem::~XObjItem()
{
}

XObjActorToy::XObjActorToy()
{
}

// NONMATCHING: layout and member set are recovered (0xD78: CStatus32@0x9C0,
// CVec@0xAC4, CObjVec@0xAF0, AnimSfx@0xB20, AnimSfxActor[2]@0xB7C,
// XObjJointPart[8]@0xBAC), but retail expands __ct__4XObjFv, __ct__9CJArchiveFv
// and __ct__7CObjVecFv inline while MWCC calls them here. Giving CJArchive an
// in-class definition would drop its weak out-of-line copy, whose only retail
// caller is the not-yet-reconstructed __ct__7XObjChrFv. The mParts element loop
// also schedules differently (peeled/bdnz vs cmplw/blt).
XObjActor::XObjActor()
{
}

XObjActor::~XObjActor()
{
}

XObjActorToy::~XObjActorToy()
{
}

XObjActorJenny::XObjActorJenny()
{
}

XObjActorJenny::~XObjActorJenny()
{
}

XObjActorMama::XObjActorMama()
{
}

XObjActorMama::~XObjActorMama()
{
}

XObjActorPapa::XObjActorPapa()
{
}

XObjActorPapa::~XObjActorPapa()
{
}

XObjActorTao::XObjActorTao()
{
}

XObjActorTao::~XObjActorTao()
{
}

XObjActorDeca::XObjActorDeca()
{
}

XObjActorDeca::~XObjActorDeca()
{
}

// NONMATCHING: retail has out-of-line deleting dtors for the spiders at
// 0x800C3CB4/0x800C3FAC/0x800C429C in this block, but the classes declare their
// dtors inline so the spider units keep emitting their vtables and thunks; an
// out-of-line decl here pulls the vtables out of xobjspider*.cpp
XObjSpider::XObjSpider()
{
}

XObjSpiderSmall::XObjSpiderSmall()
{
}

XObjSpiderBoss::XObjSpiderBoss()
{
}

XObjDust::XObjDust()
{
}

XObjDust::~XObjDust()
{
}

XObjHBox::XObjHBox()
{
}

XObjHBox::~XObjHBox()
{
}

XObjScnCam::XObjScnCam()
{
}

XObjScnCam::~XObjScnCam()
{
}

XObjLeticker::XObjLeticker()
{
}

XObjLeticker::~XObjLeticker()
{
}

XObjArmy::XObjArmy()
{
}

XObjArmySlot::XObjArmySlot()
{
}

XObjArmySlot::~XObjArmySlot()
{
}

XObjArmy::~XObjArmy()
{
}

XObjFree::XObjFree()
{
}

XObjFree::~XObjFree()
{
}
