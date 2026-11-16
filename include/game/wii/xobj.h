#ifndef GAME_WII_XOBJ_H
#define GAME_WII_XOBJ_H

#include "Dolphin/mtx.h"
#include "game/coordinfo.h"
#include "game/ut.h"

struct XObjFloor;

// The Wii's object family (RTTI: XObj, XObjActor and the rest). The GameCube's XObj wrapped
// sysdolphin joints and an HSD archive in its head; the Wii renders through nw4r::g3d callbacks
// (ICalcWorldCallback's vtable sits at +0 of the complete object, the CXObj chain's 155-slot
// vtable at +4), which drops the joint/archive head and shifts every member. Members are
// declared only where the ported code reaches them, offsets verified against the retail dol.
// The chain vtable's order is not yet reconstructed; virtuals arrive with the functions that
// need them.

// a pose channel of a Wii XObj: current value, target and step rate, the GameCube's CObjVec
class CObjVec : public CVec {
public:
	CVec mTarget; // _10
	CVec mSpeed;  // _20
};
STATIC_ASSERT(sizeof(CObjVec) == 0x30);

// the object state flags shared by the chain (retail reads a 64 bit word here)
struct XObjStatus {
	u64 mFlags; // _00
};

// the chain subobject view: what a pointer passed to an XObj method sees (the complete object
// carries the nw4r callback base four bytes in front). The vtable below is the secondary
// chain vtable (retail __vt__4XObj secondary array, 155 slots, offset-to-top -4): slots are
// named where a retail symbol or the ported code identifies them, vfNNN (byte offset) where
// not yet known.
// The retail Wii toolchain stores a base vptr at the first function slot, so retail calls
// index vtable entries by 4*index. Our GC toolchain's vptr carries the 8 byte vtable head
// and calls index by 8+4*index, so the retail head's first two slots (XObj's destructor
// pair at _00/_04) are undeclared here: every entry below then sits two slots ahead of its
// retail index and our calls land on the retail offset.
class XObj {
public:
	virtual void vf008();
	virtual void vf00C();
	virtual void vf010();
	virtual void vf014();
	virtual void vf018();
	virtual void vf01C();
	virtual void Release(); // _20
	virtual void vf024();
	virtual void vf028();
	virtual void vf02C();
	virtual void vf030();
	virtual void vf034();
	virtual void vf038();
	virtual void vf03C();
	virtual void vf040(); // _40, releases the carried object (part calls it)
	virtual void vf044();
	virtual void vf048();
	virtual void vf04C();
	virtual void vf050();
	virtual void vf054();
	virtual void vf058();
	virtual void vf05C();
	virtual void vf060();
	virtual void vf064();
	virtual void vf068();
	virtual void vf06C();
	virtual void vf070(f32 rot); // _70, sets the pose rotation (SetRotationY)
	virtual void vf074();
	virtual void vf078();
	virtual void vf07C();
	virtual void vf080();
	virtual void vf084();
	virtual void vf088();
	virtual void vf08C();
	virtual void vf090(const Vec& pos); // _90, copies the owner's pose position (SetPos)
	virtual void vf094();
	virtual void vf098();
	virtual void vf09C(f32 scale); // _9C, scales the pose (SetScale)
	virtual void vf0A0();
	virtual void vf0A4();
	virtual void vf0A8();
	virtual void vf0AC();
	virtual void vf0B0();
	virtual void vf0B4();
	virtual void vf0B8();
	virtual void vf0BC();
	virtual void vf0C0();
	virtual void vf0C4();
	virtual void vf0C8();
	virtual void vf0CC();
	virtual void vf0D0();
	virtual void vf0D4();
	virtual void vf0D8();
	virtual void vf0DC();
	virtual void SetMoveDir(const Vec* dir, f32 speed); // _E0, CXObj impl
	virtual void SetRotateY(f32 a, f32 b);              // _E4, CXObj impl
	virtual void vf0E8();
	virtual void vf0EC();
	virtual void ClearAnim(); // _F0
	virtual void vf0F4();
	virtual void vf0F8();
	virtual void vf0FC();
	virtual void vf100();
	virtual void vf104();
	virtual void SetAnim(s32 id, BOOL force, f32 frame, f32 rate, f32 blend, f32 duration); // _108
	virtual void vf10C();
	virtual void vf110();
	virtual void vf114(f32 rate); // _114, sets the anim rate (SetAnimRate)
	virtual void vf118();
	virtual s32 vf11C(); // _11C, current anim id
	virtual void vf120();
	virtual void vf124();
	virtual void vf128();
	virtual void InitializeObject(u32 id);                 // _12C
	virtual void ResetForNewGame();                        // _130
	virtual s32 ResetObjectState();                        // _134
	virtual BOOL LoadFromPlacement(const CoordInfo& info); // _138
	virtual void vf13C();
	virtual void vf140();
	virtual void UpdateObject(u32 frames); // _144
	virtual void vf148();
	virtual void vf14C();
	virtual void vf150();
	virtual s32 vf154();                      // _154, forwards to vf178(0); callers return its result
	virtual s32 vf158(const CoordInfo& info); // _158, the carried object's placement loader
	virtual void vf15C();
	virtual void vf160();
	virtual void vf164(u32 frames); // _164
	virtual void vf168();
	virtual void DrawCollisionShapes();       // _16C
	virtual void DrawScreenOverlay();         // _170
	virtual void DrawDebugStatus();           // _174
	virtual s32 vf178(const CoordInfo& info); // _178
	virtual void vf17C();
	virtual void vf180();
	virtual void vf184();
	virtual void vf188();
	virtual void vf18C();
	virtual void vf190();
	virtual void vf194();
	virtual void vf198();
	virtual void vf19C();
	virtual void vf1A0();
	virtual void vf1A4();
	virtual u32 TestFloorCollision(const Vec* from, const Vec* to, f32 radius, f32 height, Vec* push, Vec* contact, s32 arg6,
	                               s32 arg7);                         // _1A8
	virtual s32 OnPlayerContact(s32 unused, s32 value);               // _1AC
	virtual BOOL TestFloorCrossing(const Vec& start, const Vec& end); // _1B0
	virtual void vf1B4(const Mtx mtx);                                // _1B4, copies the pose matrix into the model
	virtual void vf1B8();
	virtual void vf1BC();
	virtual void vf1C0();
	virtual void vf1C4();
	virtual void vf1C8();
	virtual const void* GetEventScript();                                // _1CC
	virtual void SetColor(s32 blend, u8 r, u8 g, u8 b, f32 time);        // _1D0
	virtual void OnPushDisplacement(const Vec& displacement);            // _1D4
	virtual BOOL ReceivePlugPower(f32 amount);                           // _1D8
	virtual void OnHitFromPosition(const Vec& position, f32 strength);   // _1DC
	virtual void OnSquirterHit(const Vec* contact, s32 value, s32 tint); // _1E0
	virtual BOOL OnItemUsed(const Vec* position, s32 value);             // _1E4
	virtual void vf1E8();
	virtual void vf1EC();
	virtual void vf1F0();
	virtual void vf1F4();
	virtual void vf1F8();
	virtual void vf1FC();
	virtual void vf200();
	virtual void vf204();
	virtual void vf208();
	virtual void vf20C();
	virtual void vf210();
	virtual void vf214();
	virtual void vf218();
	virtual void vf21C();
	virtual void vf220();
	virtual void vf224();
	virtual void vf228();
	virtual void* vf22C(const char* name); // _22C, finds a named joint in the model ("obj2")
	virtual Vec vf230(void* joint);        // _230, reads a joint's world position
	virtual void vf234();
	virtual void vf238();
	virtual void vf23C();
	virtual void vf240();
	virtual CObjVec* vf244(); // _244, the object's pose position channel
	virtual void vf248();
	virtual void vf24C();
	virtual void vf250();
	virtual void vf254();
	virtual CObjVec* vf258(); // _258, the object's pose scale channel
	virtual void vf25C();
	virtual void vf260();
	virtual void vf264();
	virtual void vf268();

	u8 _004[0x030 - 0x004];
	CObjVec mMoveTranslate; // _030, the object's pose position
	CObjVec mMoveRotate;    // _060
	CObjVec mMoveScale;     // _090
	u8 _0C0[0x150 - 0x0C0];
	XObjStatus mStatus; // _150, 64 bit state flags
	u8 _158[0x15C - 0x158];
	AttrInfo mCoord; // _15C, the object's placement record
	u8 _168[0x174 - 0x168];
	XObjFloor* mFloors; // _174, the object's collision floor list
	u32 mFloorMasks[2]; // _178, per-floor collision enables
	u8 _180[0x790 - 0x180];
	u32 mEventScript; // _790, event script started on use, NULL if none
	u8 _794[0x9A8 - 0x794];
};
STATIC_ASSERT(sizeof(XObj) == 0x9A8);

// the base of the people and toys, the GameCube's members shifted by +0x28
class XObjActor : public XObj {
public:
	f32 mHeading;   // _9A8, the way it walks
	f32 mMoveSpeed; // _9AC, move speed
	u8 _9B0[0x9B8 - 0x9B0];
	s32 mKindStep;    // _9B8, the kind's own step
	u32 mKindCounter; // _9BC, cleared with mKindStep

	void OnPushDisplacement(const Vec& displacement);
	Vec GetCameraTarget();
};

// the plain object an XObjHBox carries
struct XObjHBoxPart {
	XObj mObj;              // _00
	f32 mHit;               // _9A8, set to 1 when chibi robo hits the box
	u8 _9AC[0x9B0 - 0x9AC]; // _9AC
	f32 mRange;             // _9B0
	s32 mSfx;               // _9B4, keyed off on a hit
};
STATIC_ASSERT(sizeof(XObjHBoxPart) == 0x9B8);

// a box that reads a second object (placement 36) and draws it inside itself
class XObjHBox : public XObj {
public:
	s32 mLoadState;     // _9A8
	f32 mRing;          // _9AC, draws the two rings while above 0
	XObjHBoxPart mPart; // _9B0

	void Release();                                // _20
	void UpdateObject(u32 frames);                 // _144
	s32 ResetObjectState();                        // _134
	BOOL LoadFromPlacement(const CoordInfo& info); // _138
	s32 vf158(const CoordInfo& info);              // _158
	void vf164(u32 frames);                        // _164
	void DrawScreenOverlay();                      // _170
};
STATIC_ASSERT(sizeof(XObjHBox) == 0x1368);

#endif
