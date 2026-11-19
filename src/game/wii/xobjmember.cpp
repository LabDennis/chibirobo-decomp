#include "game/wii/xobjflags.h" // first: its AnimSfx needs the real nw4r::math, ahead of xobj.h's shim
#include "game/wii/xobj.h"

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
