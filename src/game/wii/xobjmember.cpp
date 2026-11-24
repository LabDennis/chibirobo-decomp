#include "game/wii/xobjflags.h" // first: its AnimSfx needs the real nw4r::math, ahead of xobj.h's shim
#include "game/wii/xobj.h"
#include "game/wii/xact.h"
#include "game/wii/xobjcamera.h"
#include "game/xobjattachment.h"
#include "game/wii/cgame.h"

// retail .text 0x800BEB4C-0x800C0EF0 (wii_units.md Nr. 49, guessed "CPtcl"):
// the XObjMapList record types, XObjMap's dtor, XObjChr's ctor and the run of
// member-class ctor/deleting-dtor weak emissions those ctors reference

XObjMapListA::XObjMapListA()
    : mItems(NULL)
    , mUsed(0)
    , mCap(0)
{
}

// NONMATCHING: retail keeps three dead beq's to the delete-skip on the this==0
// flags after the null-return (B has two); the member shape that emits them is
// not recovered. mUsed clears as a self-subtract in retail
XObjMapListA::~XObjMapListA()
{
	if (mItems != NULL) {
		mUsed -= mUsed;
		delete mItems;
	}
}

XObjMapListB::XObjMapListB()
    : mItems(NULL)
    , mUsed(0)
    , mCap(0)
{
}

// NONMATCHING: two dead beq's like ~XObjMapListA
XObjMapListB::~XObjMapListB()
{
	if (mItems != NULL) {
		mUsed -= mUsed;
		delete mItems;
	}
}

// the two record arrays unwind through __destroy_arr, list B first
XObjMap::~XObjMap()
{
}

// retail 0x800BECE8: the player aggregate. Retail calls an out-of-line copy of
// the XObj base ctor (fn_800BEECC), stores its two vtables and runs the member
// chain in declaration order; the recovered member types (CObjVec/CVec records,
// XPad, XObjChrMenu, CPtclSet, CTexObj[2], the CJArchive pair, AnimSfxActor) get
// built from this body, the gaps stay padding until their classes -- most only
// declared in cgame.h's WiiPlayerState -- become visible to xobj.h
// NONMATCHING: MWCC expands XObj() inline here instead of calling fn_800BEECC,
// and inlines the leaf member ctors retail calls out of line
XObjChr::XObjChr()
{
}

// the member classes XObjChr's constructor builds out of line: the action kinds
// (wii/xact.h, wii/cgame.h) get one weak copy each here, the deleting destructors
// ride along with the vtables. The CStatus32 copy at 0x800BF148 stays missing: its
// retail callers inline it, so an out-of-line declaration would break them
XActLift::XActLift()
{
}

XActLift::~XActLift()
{
}

XActHang::XActHang()
{
}

XActHang::~XActHang()
{
}

XActJump::XActJump()
{
}

XActJump::~XActJump()
{
}

XActLadder::XActLadder()
{
}

XActLadder::~XActLadder()
{
}

XActRope::XActRope()
{
}

XActRope::~XActRope()
{
}

XActPush::XActPush()
{
}

XActPush::~XActPush()
{
}

XActTumble::XActTumble()
{
}

XActTumble::~XActTumble()
{
}

XActCopter::XActCopter()
{
}

XActCopter::~XActCopter()
{
}

XActPopper::XActPopper()
{
}

// destroys the ten shots and the two charge sound actors through __destroy_arr,
// which pulls each element class's deleting destructor into this unit
XActPopper::~XActPopper()
{
}

// one of an object's sound sources; the popper's charge builds its array out of line
AnimSfxActor::AnimSfxActor()
    : mActor(NULL)
{
}

AnimSfxActor::~AnimSfxActor()
{
}

// the radar action's members build out of line here: XObjChr's ctor calls it last,
// the two CScrnColor of mBeam come along as __construct_array's element ctor
XActRadar::XActRadar()
{
}

XActRadar::~XActRadar()
{
}

// the player state keeps two of these camera aim records (mEyeAimRecord/mHeadAimRecord
// in cgame.h); the ctor fills the CVec/TVec members' vtables, the dtor rides along
XObjScnCamInitRec::XObjScnCamInitRec()
{
}

XObjScnCamInitRec::~XObjScnCamInitRec()
{
}

// retail 0x800BFA08 on: the XObjChr aggregate's member ctors in address order.
// The bodies stay empty: retail's ctors are pure member-expansions -- the
// XObj/CObj base chain, the member vtables and the cord node array the
// compiler builds with __construct_array.

Params::Params()
{
}

Params::~Params()
{
}

XObjPlug::XObjPlug()
{
}

XObjPlugCordNode::XObjPlugCordNode()
{
}

XObjPlugCordNode::~XObjPlugCordNode()
{
}

// the deleting dtors of the classes above emit weak: each ctor's extab unwind
// table references them for partial-construction cleanup, so this TU pulls a
// weak copy like retail's (defining them out of line steals the vtable home
// and the @4@ thunks in xobjattachment/xobjplug)
XObjCopter::XObjCopter()
{
}

XObjPopper::XObjPopper()
{
}

XObjRadar::XObjRadar()
{
}

XObjOX::XObjOX()
{
}

XObjChrBase::XObjChrBase()
{
}

XObjChrBase::~XObjChrBase()
{
}

// retail 0x800C0E98, last emission of the unit: XObjChr's mPad member builds it
// (CPadOne base, the motion reset fields), its deleting dtor is in xpaddtor.cpp
XPad::XPad()
{
}
