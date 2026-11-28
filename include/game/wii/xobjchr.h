#ifndef _GAME_WII_XOBJCHR_H
#define _GAME_WII_XOBJCHR_H

#include "game/wii/xobj.h"
#include "game/wii/cgame.h"
#include "game/wii/xact.h"
#include "game/xobjattachment.h"
#include "game/octamenu.h"

// the Chibi-Robo character object: full Wii member layout matching retail's
// ctor (0x800BECE8) and dtor (0x800C14A0) member construction/unwind chain.
class XObjChr : public XObj {
public:
	// the dust pool the character's own effects draw from: retail RTTI names it
	// XObjChr::CPtclSet, a DustParticlePool that adds a virtual at _08
	class CPtclSet : public DustParticlePool {
	public:
		CPtclSet();
		~CPtclSet();
		virtual void vf08(CGs* gs); // _08, fn_800C6418: mPtcl's render state, set before each draw
	};

	XObjChr();
	virtual ~XObjChr();

	virtual void Release();                             // _40, the GameCube's plus the suit teardown RemovePlayerSuit did
	virtual void SetMoveDir(const Vec* dir, f32 speed); // _100
	using XObj::SetAnim;
	virtual void SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend,
	                     f32 blendRate);                               // _108
	virtual void UpdateAnim(f32 speed);                                // _118
	virtual void InitializeObject();                                   // _14C
	virtual void ResetForNewGame();                                    // _150
	virtual void ResetObjectState();                                   // _154, the player state back to its defaults
	virtual BOOL LoadFromPlacement(const CoordInfo& info);             // _158, the stage load, one step a frame
	virtual void UpdateObject(u32 frames);                             // _164
	virtual void DrawWithView(Mtx view);                               // _178, himself or the current suit, then the attachments
	virtual void DrawReflection(Mtx view);                             // _17C
	virtual void DrawGroundShadow(f32 opacity);                        // _180, nothing up close to the camera
	virtual void DrawGroundShadowAt(const Vec* origin, f32 opacity);   // _184, the mug's blob or the joint's
	virtual void DrawObjectEffects();                                  // _188
	virtual void DrawCollisionShapes();                                // _18C
	virtual void DrawScreenOverlay();                                  // _190
	virtual void DrawDebugStatus();                                    // _194
	virtual void ApplyMaterialPresets(const u32* flags);               // _1A0, forwards the flags to everything he wears
	virtual void SetDestinationAlpha(BOOL enabled, u8 value);          // _1A4, the fade on himself and his attachments
	virtual void vf1A8(f32);                                           // _1A8, empty on the player
	virtual Vec GetCameraTarget();                                     // _1B0, the animated look target and its position history
	virtual void SetBaseMtx(Mtx mtx);                                  // _1B4, his own plus the nine suits'
	virtual void ClearBaseMtx();                                       // _1B8, same
	virtual void vf1BC(s32 mode);                                      // _1BC, culls him and the visible attachments
	virtual void SetCullMode(s32 mode);                                // _1C0
	virtual void ResetCullMode();                                      // _1C4
	virtual void SetColor(s32 blend, u8 r, u8 g, u8 b, f32 time);      // _1F0
	virtual s32 HandleEventCommand(s32 count, s32* args);              // _1E8
	virtual void OnHitFromPosition(const Vec& position, f32 strength); // _1FC
	virtual void PauseSounds(f32 fade);                                // _20C
	virtual void ResumeSounds(f32 fade);                               // _210

private:
	u8 _9A8[0x9CC - 0x9A8];
	CStatus32 mChrStatus; // _9CC
	CObjVec mMotion;      // _9D4
	u8 _A04[0xA34 - 0xA04];
	CVec mContactPos; // _A34
	u8 _A44[4];
	CVec mEdgePos; // _A48
	u8 _A58[0xAA0 - 0xA58];
	XActLift mLift;     // _AA0
	XActHang mHang;     // _B50
	XActJump mJump;     // _BA0
	XActLadder mLadder; // _BF4
	XActRope mRope;     // _C38
	XActPush mPush;     // _C78
	XActTumble mTumble; // _CD0
	XActCopter mCopter; // _D18
	XActPopper mPopper; // _D50
	XActRadar mRadar;   // _95D4
	u8 _9694[0x9810 - 0x9694];
	XObjScnCamInitRec mEyeAimRec;  // _9810
	XObjScnCamInitRec mHeadAimRec; // _987C
	u8 _98E8[0x995C - 0x98E8];
	CObjVec mSlopeMotion; // _995C
	CVec mFloorSlope;     // _998C
	Params mParams;       // _999C
	u8 _9DE0[0x9E20 - 0x9DE0];
	XObjPlug mPlug;        // _9E20
	XObjCopter mCopterObj; // _B580
	XObjPopper mPopperObj; // _BF30
	XObjRadar mRadarObj;   // _C8E0
	XObjOX mOXObj;         // _D2A0
	XObjChrBase mSuits;    // _DC50
	XPad mPad;             // _18BE8
	u8 _19E98[0x19EE4 - 0x19E98];
	XObjChrMenu mMenu; // _19EE4
	OXMenu mOXMenu;    // _1A420
	u8 _1B7B8[0x1B7BC - 0x1B7B8];
	CPtclSet mDustPool; // _1B7BC
	CTexObj mTex[2];    // _1C774
	u8 _1C97C[0x1C9A8 - 0x1C97C];
	CVec _1C9A8;
	u8 _1C9B8[0x1C9CC - 0x1C9B8];
	CObjVec mEyeAim;             // _1C9CC
	CJArchive mAttachmentAnm;    // _1C9FC
	CJArchive mSuitAnm;          // _1CB44
	nw4r::g3d::ScnRoot* m_pRoot; // _1CC8C, retail's assert names it
	AnimSfxActor mActor[1];      // _1CC90, retail builds/destroys via __construct_array/__destroy_arr(count 1)
	u8 _1CC9C[0x1CCA0 - 0x1CC9C];
};
STATIC_ASSERT(sizeof(XObjChr) == 0x1CCA0);

#endif
