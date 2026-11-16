// The tail of the GameCube gxb.cpp on the Wii: the small virtuals of the object classes this file
// is the first to use (Cursor, CObj, XObj, CFileLogo) and the boot logos behind them.
//
// Retail's first two functions, 0x8004328C and 0x800432CC, are the destructors of CPtclDust3 and
// CPtclDust2 (their vtables' first slots). cptcl.h defines both inside the class, so they come out
// with the vtables, in the file that defines the classes' Update and Draw: unit_80040E4C.cpp, the
// part of the same original file that lies ahead of this one. The three this-adjusting thunks at
// 0x800444F0-0x80044507 come out with Cursor's vtable, which Cursor::InitializeObject places here.
#include "types.h"
#include "game/wii/cgame.h"
#include "game/wii/cgs.h"
#include "game/wii/cpadact.h"
#include "game/wii/cursor.h"
#include "game/ctpl.h"
#include "game/gmath.h"
#include "game/logo.h"
#include "Dolphin/mtx.h"
#include <math.h>
#include <stdio.h>

extern "C" {
extern s32 gPointerScreenWidth, gPointerScreenHeight, gRefreshRate;
void HSD_MtxGetScale(Mtx m, Vec* out);
}

// CObj's model members are still untyped in xobj.h: the nw4r::g3d::ResMdl at _EC and the NW_Obj
// that most likely starts at _E8 are reached through these two
#define COBJ_RESMDL ((nw4r::g3d::ResMdl*)&_EC)
#define COBJ_NW_OBJ ((NW_Obj*)&_E8)

/**
 * @note Address: 0x8004330C
 * @note Size: 0x4
 */
void Cursor::InitializeObject()
{
	XObj::InitializeObject();
}

/**
 * @note Address: 0x80043310
 * @note Size: 0x38
 */
void Cursor::ResetObjectState()
{
	XObj::ResetObjectState();
	mAngleSpeed = 0.0f;
	mAngle      = 0.0f;
}

/**
 * @note Address: 0x80043348
 * @note Size: 0x2C
 */
BOOL Cursor::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info)) {
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80043374
 * @note Size: 0x88
 */
// Cursor's update, spins it
void Cursor::UpdateObject(u32 frames)
{
	mAngle = fmod(mAngleSpeed * (f32)frames + mAngle, TWO_PI);
	SetRotationZ(mAngle);
	XObj::UpdateObject(frames);
}

/**
 * @note Address: 0x800433FC
 * @note Size: 0xC
 */
void CObj::SetRotationZ(f32 z)
{
	mMoveRotate.z  = z;
	mMoveTarget1.z = z;
}

/**
 * @note Address: 0x80043408
 * @note Size: 0x14
 */
void Cursor::DrawObject()
{
	DrawWithView(NULL);
}

/**
 * @note Address: 0x8004341C
 * @note Size: 0x4
 */
void CObj::_unk_00C()
{
}

/**
 * @note Address: 0x80043420
 * @note Size: 0x4
 */
void CObj::ExecCallbackC()
{
}

/**
 * @note Address: 0x80043424
 * @note Size: 0x8
 */
void CObj::ReleaseModelWork()
{
	COBJ_NW_OBJ->ReleaseWork();
}

/**
 * @note Address: 0x8004342C
 * @note Size: 0x8C
 */
bool CObj::IsAnimEnd(f32 frames)
{
	f32 frame = GetAnimFrame();
	f32 end   = GetAnimEndFrame();
	return frame + mAnimRate[0] * frames >= end;
}

/**
 * @note Address: 0x800434B8
 * @note Size: 0x14
 */
f32 CObj::GetAnimRate()
{
	return GetAnimRate(0);
}

/**
 * @note Address: 0x800434CC
 * @note Size: 0x10
 */
f32 CObj::GetAnimRate(s32 index)
{
	return mAnimRate[index];
}

/**
 * @note Address: 0x800434DC
 * @note Size: 0x3C
 */
f32 CObj::GetNodeRotateZ(nw4r::g3d::ResNode node)
{
	return COBJ_NW_OBJ->GetNodeRotate(node).z;
}

/**
 * @note Address: 0x80043518
 * @note Size: 0xB8
 */
// NONMATCHING: retail builds the vec in a temporary of its own (z first) and copy-constructs the
// result from it; this build constructs the result in place, 0x1C bytes shorter. The source form
// that keeps the temporary is not found (a named local, a const local and an inline operator ruled out)
CVec NW_Obj::GetNodeRotate(nw4r::g3d::ResNode node) const
{
	const nw4r::g3d::ResNodeData& data = node.ref();
	CVec rotate(DEG2RAD * data.mRotate.x, DEG2RAD * data.mRotate.y, DEG2RAD * data.mRotate.z);
	return rotate;
}

/**
 * @note Address: 0x800435D0
 * @note Size: 0x54
 */
f32 CObj::GetNodeRotateZ(const char* name)
{
	return GetNodeRotateZ(GetResNode(name));
}

/**
 * @note Address: 0x80043624
 * @note Size: 0x8
 */
nw4r::g3d::ResNode CObj::GetResNode(const char* name)
{
	return COBJ_RESMDL->GetResNode(name);
}

/**
 * @note Address: 0x8004362C
 * @note Size: 0x54
 */
f32 CObj::GetNodeRotateZ(u32 index)
{
	return GetNodeRotateZ(GetResNode(index));
}

/**
 * @note Address: 0x80043680
 * @note Size: 0x8
 */
nw4r::g3d::ResNode CObj::GetResNode(u32 index)
{
	return COBJ_RESMDL->GetResNode(index);
}

/**
 * @note Address: 0x80043688
 * @note Size: 0x54
 */
f32 CObj::GetNodeRotateZ()
{
	return GetNodeRotateZ(GetResNode());
}

/**
 * @note Address: 0x800436DC
 * @note Size: 0xC
 */
nw4r::g3d::ResNode CObj::GetResNode()
{
	return COBJ_RESMDL->GetResNode((u32)0);
}

/**
 * @note Address: 0x800436E8
 * @note Size: 0x3C
 */
f32 CObj::GetNodeRotateY(nw4r::g3d::ResNode node)
{
	return COBJ_NW_OBJ->GetNodeRotate(node).y;
}

/**
 * @note Address: 0x80043724
 * @note Size: 0x54
 */
f32 CObj::GetNodeRotateY(const char* name)
{
	return GetNodeRotateY(GetResNode(name));
}

/**
 * @note Address: 0x80043778
 * @note Size: 0x54
 */
f32 CObj::GetNodeRotateY(u32 index)
{
	return GetNodeRotateY(GetResNode(index));
}

/**
 * @note Address: 0x800437CC
 * @note Size: 0x54
 */
f32 CObj::GetNodeRotateY()
{
	return GetNodeRotateY(GetResNode());
}

/**
 * @note Address: 0x80043820
 * @note Size: 0x3C
 */
f32 CObj::GetNodeRotateX(nw4r::g3d::ResNode node)
{
	return COBJ_NW_OBJ->GetNodeRotate(node).x;
}

/**
 * @note Address: 0x8004385C
 * @note Size: 0x54
 */
f32 CObj::GetNodeRotateX(const char* name)
{
	return GetNodeRotateX(GetResNode(name));
}

/**
 * @note Address: 0x800438B0
 * @note Size: 0x54
 */
f32 CObj::GetNodeRotateX(u32 index)
{
	return GetNodeRotateX(GetResNode(index));
}

/**
 * @note Address: 0x80043904
 * @note Size: 0x54
 */
f32 CObj::GetNodeRotateX()
{
	return GetNodeRotateX(GetResNode());
}

/**
 * @note Address: 0x80043958
 * @note Size: 0x30
 */
CVec CObj::GetNodeTranslate(nw4r::g3d::ResNode node)
{
	return COBJ_NW_OBJ->GetNodeTranslate(node);
}

/**
 * @note Address: 0x80043988
 * @note Size: 0x8C
 */
// NONMATCHING: same instructions, but retail stores the result's vtable ahead of loading the node's
// data pointer and this build behind it
CVec NW_Obj::GetNodeTranslate(nw4r::g3d::ResNode node) const
{
	const nw4r::g3d::ResNodeData& data = node.ref();
	return CVec(data.mTranslate.x, data.mTranslate.y, data.mTranslate.z);
}

/**
 * @note Address: 0x80043A14
 * @note Size: 0x6C
 */
CVec CObj::GetNodeTranslate(const char* name)
{
	return GetNodeTranslate(GetResNode(name));
}

/**
 * @note Address: 0x80043A80
 * @note Size: 0x6C
 */
CVec CObj::GetNodeTranslate(u32 index)
{
	return GetNodeTranslate(GetResNode(index));
}

/**
 * @note Address: 0x80043AEC
 * @note Size: 0x68
 */
CVec CObj::GetNodeTranslate()
{
	return GetNodeTranslate(GetResNode());
}

/**
 * @note Address: 0x80043B54
 * @note Size: 0xF4
 */
// NONMATCHING: retail reads mScnMdl and tests it ahead of saving the registers, and loads the three
// floats of the temporary CVec in the opposite registers (z in f0); the frame and the rest are retail's
// the translation column of the node's world matrix
CVec CObj::GetWorldPos(nw4r::g3d::ResNode node)
{
	MtxP mtx = GetNodeWorldMtx(node);
	Vec pos  = CVec(mtx[0][3], mtx[1][3], mtx[2][3]);
	return CVec(pos.x, pos.y, pos.z);
}

/**
 * @note Address: 0x80043C48
 * @note Size: 0x6C
 */
CVec CObj::GetWorldPos(const char* name)
{
	return GetWorldPos(GetResNode(name));
}

/**
 * @note Address: 0x80043CB4
 * @note Size: 0x6C
 */
CVec CObj::GetWorldPos(u32 index)
{
	return GetWorldPos(GetResNode(index));
}

/**
 * @note Address: 0x80043D20
 * @note Size: 0x68
 */
CVec CObj::GetWorldPos()
{
	return GetWorldPos(GetResNode());
}

/**
 * @note Address: 0x80043D88
 * @note Size: 0x68
 */
// NONMATCHING: one instruction out of place, retail reads mScnMdl ahead of saving r31 (an early
// return, a conditional expression and an inline test were tried)
MtxP CObj::GetWorldMtx(const char* name)
{
	if (mScnMdl != NULL) {
		return GetWorldMtx(GetResNode(name));
	}
	return NULL;
}

/**
 * @note Address: 0x80043DF0
 * @note Size: 0x68
 */
// NONMATCHING: one instruction out of place, retail reads mScnMdl ahead of saving r31 (an early
// return, a conditional expression and an inline test were tried)
MtxP CObj::GetWorldMtx(u32 index)
{
	if (mScnMdl != NULL) {
		return GetWorldMtx(GetResNode(index));
	}
	return NULL;
}

/**
 * @note Address: 0x80043E58
 * @note Size: 0x68
 */
// NONMATCHING: one instruction out of place, retail reads mScnMdl ahead of saving r31 (an early
// return, a conditional expression and an inline test were tried)
MtxP CObj::GetWorldMtx()
{
	if (mScnMdl != NULL) {
		return GetWorldMtx(GetResNode());
	}
	return NULL;
}

/**
 * @note Address: 0x80043EC0
 * @note Size: 0x1C
 */
void CObj::SetScale(const Vec& scale)
{
	SetScale(scale.x, scale.y, scale.z);
}

/**
 * @note Address: 0x80043EDC
 * @note Size: 0x1C
 */
void CObj::SetRot(f32 x, f32 y, f32 z)
{
	mMoveRotate.x  = x;
	mMoveRotate.y  = y;
	mMoveRotate.z  = z;
	mMoveTarget1.x = x;
	mMoveTarget1.y = y;
	mMoveTarget1.z = z;
}

/**
 * @note Address: 0x80043EF8
 * @note Size: 0x1C
 */
void CObj::SetRot(const Vec& rot)
{
	SetRot(rot.x, rot.y, rot.z);
}

/**
 * @note Address: 0x80043F14
 * @note Size: 0x1C
 */
void CObj::SetPos(const Vec& pos)
{
	SetPos(pos.x, pos.y, pos.z);
}

/**
 * @note Address: 0x80043F30
 * @note Size: 0xC
 */
void CObj::SetScaleZ(f32 z)
{
	mMoveScale.z   = z;
	mMoveTarget2.z = z;
}

/**
 * @note Address: 0x80043F3C
 * @note Size: 0xC
 */
void CObj::SetScaleY(f32 y)
{
	mMoveScale.y   = y;
	mMoveTarget2.y = y;
}

/**
 * @note Address: 0x80043F48
 * @note Size: 0xC
 */
void CObj::SetScaleX(f32 x)
{
	mMoveScale.x   = x;
	mMoveTarget2.x = x;
}

/**
 * @note Address: 0x80043F54
 * @note Size: 0xC
 */
void CObj::SetRotationY(f32 y)
{
	mMoveRotate.y  = y;
	mMoveTarget1.y = y;
}

/**
 * @note Address: 0x80043F60
 * @note Size: 0xC
 */
void CObj::SetRotationX(f32 x)
{
	mMoveRotate.x  = x;
	mMoveTarget1.x = x;
}

/**
 * @note Address: 0x80043F6C
 * @note Size: 0xC
 */
void CObj::SetTranslateZ(f32 z)
{
	mMoveTranslate.z = z;
	mMoveTarget0.z   = z;
}

/**
 * @note Address: 0x80043F78
 * @note Size: 0xC
 */
void CObj::SetTranslateY(f32 y)
{
	mMoveTranslate.y = y;
	mMoveTarget0.y   = y;
}

/**
 * @note Address: 0x80043F84
 * @note Size: 0xC
 */
void CObj::SetTranslateX(f32 x)
{
	mMoveTranslate.x = x;
	mMoveTarget0.x   = x;
}

/**
 * @note Address: 0x80043F90
 * @note Size: 0x10
 */
nw4r::g3d::ResNode CObj::GetRootResNode()
{
	return GetResNode();
}

// the three steppers: each vec, its target and its speed lie as one CObjVec, which xobj.h still
// spells as three members

/**
 * @note Address: 0x80043FA0
 * @note Size: 0x8
 */
u8 CObj::StepScale(u32 frames)
{
	return ((CObjVec*)&mMoveScale)->Step(frames);
}

/**
 * @note Address: 0x80043FA8
 * @note Size: 0x8
 */
u8 CObj::StepRotate(u32 frames)
{
	return ((CObjVec*)&mMoveRotate)->Step(frames);
}

/**
 * @note Address: 0x80043FB0
 * @note Size: 0x8
 */
u8 CObj::StepTranslate(u32 frames)
{
	return ((CObjVec*)&mMoveTranslate)->Step(frames);
}

/**
 * @note Address: 0x80043FB8
 * @note Size: 0x8
 */
nw4r::g3d::ResNode XObj::GetResNode(u32 index)
{
	return COBJ_RESMDL->GetResNode(index);
}

/**
 * @note Address: 0x80043FC0
 * @note Size: 0xC
 */
nw4r::g3d::ResNode XObj::GetResNode()
{
	return COBJ_RESMDL->GetResNode((u32)0);
}

/**
 * @note Address: 0x80043FCC
 * @note Size: 0x30
 */
f32 XObj::GetScaleZ()
{
	return GetScale()->z;
}

/**
 * @note Address: 0x80043FFC
 * @note Size: 0x8
 */
CVec* XObj::GetScale()
{
	return &mMoveScale;
}

/**
 * @note Address: 0x80044004
 * @note Size: 0x30
 */
f32 XObj::GetScaleY()
{
	return GetScale()->y;
}

/**
 * @note Address: 0x80044034
 * @note Size: 0x30
 */
f32 XObj::GetScaleX()
{
	return GetScale()->x;
}

/**
 * @note Address: 0x80044064
 * @note Size: 0x8
 */
f32 XObj::GetRotZ()
{
	return mMoveRotate.z;
}

/**
 * @note Address: 0x8004406C
 * @note Size: 0x8
 */
f32 XObj::GetRotY()
{
	return mMoveRotate.y;
}

/**
 * @note Address: 0x80044074
 * @note Size: 0x8
 */
f32 XObj::GetRotX()
{
	return mMoveRotate.x;
}

/**
 * @note Address: 0x8004407C
 * @note Size: 0x30
 */
f32 XObj::GetPosZ()
{
	return GetPos()->z;
}

/**
 * @note Address: 0x800440AC
 * @note Size: 0x30
 */
f32 XObj::GetPosY()
{
	return GetPos()->y;
}

/**
 * @note Address: 0x800440DC
 * @note Size: 0x30
 */
f32 XObj::GetPosX()
{
	return GetPos()->x;
}

// the scale a matrix carries. retail expands it in both GetJointScale: each keeps the vec
// HSD_MtxGetScale fills in and, behind it in the frame, an unnamed copy it reads the result from.
// the name is ours
static inline Vec GetMtxScale(MtxP mtx)
{
	Vec scale;
	HSD_MtxGetScale(mtx, &scale);
	return scale;
}

/**
 * @note Address: 0x8004410C
 * @note Size: 0x7C
 */
void XObj::GetJointScale(nw4r::g3d::ResNode node, Vec* out)
{
	Vec scale = GetMtxScale(GetWorldMtx(node));
	out->x    = scale.x;
	out->y    = scale.y;
	out->z    = scale.z;
}

/**
 * @note Address: 0x80044188
 * @note Size: 0x8C
 */
CVec XObj::GetJointScale(nw4r::g3d::ResNode node)
{
	Vec scale = GetMtxScale(GetWorldMtx(node));
	return CVec(scale.x, scale.y, scale.z);
}

/**
 * @note Address: 0x80044214
 * @note Size: 0x10
 */
CVec XObj::GetJointTranslate(const char* name)
{
	return GetNodeTranslate(name);
}

/**
 * @note Address: 0x80044224
 * @note Size: 0x10
 */
CVec XObj::GetJointTranslate(u32 index)
{
	return GetNodeTranslate(index);
}

/**
 * @note Address: 0x80044234
 * @note Size: 0x10
 */
CVec XObj::GetJointWorldPos(const char* name)
{
	return GetWorldPos(name);
}

/**
 * @note Address: 0x80044244
 * @note Size: 0x10
 */
CVec XObj::GetJointWorldPos(u32 index)
{
	return GetWorldPos(index);
}

/**
 * @note Address: 0x80044254
 * @note Size: 0x10
 */
CVec XObj::GetJointPos(const char* name)
{
	return GetWorldPos(name);
}

/**
 * @note Address: 0x80044264
 * @note Size: 0x10
 */
CVec XObj::GetJointPos(u32 index)
{
	return GetWorldPos(index);
}

/**
 * @note Address: 0x80044274
 * @note Size: 0x38
 */
CVec XObj::GetJointPos(nw4r::g3d::ResNode node)
{
	return GetWorldPos(node);
}

/**
 * @note Address: 0x800442AC
 * @note Size: 0x14
 */
BOOL XObj::HasModel()
{
	return mScnMdl != NULL;
}

/**
 * @note Address: 0x800442C0
 * @note Size: 0x78
 */
nw4r::g3d::ResNode XObj::GetResNodeOrFirst(u32 index)
{
	if (index < COBJ_RESMDL->GetResNodeNumEntries()) {
		return GetResNode(index);
	}
	return GetResNode((u32)0);
}

/**
 * @note Address: 0x80044338
 * @note Size: 0x10
 */
nw4r::g3d::ResNode XObj::FindResNode(const char* name)
{
	return GetResNode(name);
}

/**
 * @note Address: 0x80044348
 * @note Size: 0x10
 */
void XObj::vf284()
{
	vf280();
}

/**
 * @note Address: 0x80044358
 * @note Size: 0x6C
 */
void XObj::RequestMoveDir(const Vec* dir, f32 speed)
{
	OnMoveRequest();
	SetMoveDir(dir, speed);
}

/**
 * @note Address: 0x800443C4
 * @note Size: 0x4
 */
void XObj::OnMoveRequest()
{
}

/**
 * @note Address: 0x800443C8
 * @note Size: 0x6C
 */
void XObj::RequestRotateY(f32 target, f32 speed)
{
	OnMoveRequest();
	SetRotateY(target, speed);
}

/**
 * @note Address: 0x80044434
 * @note Size: 0x5C
 */
void XObj::RequestRotationY(f32 y)
{
	OnMoveRequest();
	SetRotationY(y);
}

/**
 * @note Address: 0x80044490
 * @note Size: 0x4
 */
void XObj::OnPushDisplacement(const Vec& displacement)
{
}

/**
 * @note Address: 0x80044494
 * @note Size: 0x8
 */
const void* XObj::GetEventScript()
{
	return mEventScript;
}

/**
 * @note Address: 0x8004449C
 * @note Size: 0x8
 */
s32 XObj::HandleEventCommand(s32 count, s32* args)
{
	return 0;
}

/**
 * @note Address: 0x800444A4
 * @note Size: 0x8
 */
s32 XObj::OnPlayerContact(s32 unused, s32 value)
{
	return value;
}

/**
 * @note Address: 0x800444AC
 * @note Size: 0xC
 */
void XObj::ClearBaseMtx()
{
	mBaseMtxPtr = NULL;
}

/**
 * @note Address: 0x800444B8
 * @note Size: 0x14
 */
void XObj::SetBaseMtx(Mtx mtx)
{
	mBaseMtxPtr = mBaseMtx;
	PSMTXCopy(mtx, mBaseMtx);
}

/**
 * @note Address: 0x800444CC
 * @note Size: 0x1C
 */
Vec XObj::GetCameraTarget()
{
	return mMoveTranslate;
}

/**
 * @note Address: 0x800444E8
 * @note Size: 0x4
 */
void XObj::DrawDebugStatus()
{
}

/**
 * @note Address: 0x800444EC
 * @note Size: 0x4
 */
void XObj::ResetForNewGame()
{
}

/**
 * @note Address: 0x80044508
 * @note Size: 0x14
 */
void* CFileLogo::Alloc(u32 size)
{
	return (u8*)gGame.mMem1ArenaHi - size;
}

/**
 * @note Address: 0x8004451C
 * @note Size: 0x18
 */
void CFileLogo::Free()
{
	if (mBuffer != NULL) {
		mBuffer = NULL;
	}
}

/**
 * @note Address: 0x80044534
 * @note Size: 0x1C
 */
// Logo's init
extern "C" void ResetLogoFadeState(Logo* logo)
{
	logo->mState = 0;
	logo->ResetFade();
}

/**
 * @note Address: 0x80044550
 * @note Size: 0x33C
 */
// Logo::Update, reads the logos in, then fades the first in and out. it stays until a button
// other than HOME is pressed; the two logos behind it are skipped on the Wii
extern "C" void UpdateBootLogos(Logo* logo, u32 frames)
{
	switch (logo->mState) {
	case 1:
		logo->mState = 2;
	case 2: {
		char path[64];
		const char* suffix = "";
		sprintf(path, "tpl/logo%s%s.tpl", gGame.mAspectRatio == 1 ? "_w" : "", suffix);
		if (!logo->mFile.ReadEx(path, 0, 0)) {
			break;
		}
		logo->mTpl.Bind(logo->mFile.mBuffer);
		logo->mState = 3;
		logo->ResetFade();
	}
	case 3:
		if (logo->mAlpha < 1.0f && (logo->mAlpha += (1.0f / (0.5f * (f32)gRefreshRate)) * frames) > 1.0f) {
			logo->mAlpha = 1.0f;
		}
		if (logo->mAlpha != 1.0f) {
			break;
		}
		logo->mState = 4;
	case 4:
		logo->mTimer += frames;
		if (logo->mTimer >= (f32)gRefreshRate) {
			if ((logo->mAngle += (DEG2RAD * (180.0f / (f32)gRefreshRate)) * frames) >= 3.1415927f) {
				logo->mAngle = 0.0f;
			}
			BOOL done = logo->mTimer >= 20.0f * (f32)gRefreshRate
			              ? TRUE
			              : (gWiiPointer.mTrigger != 0 && !(gWiiPointer.mTrigger & ((u64)1 << RemoteHome)));
			if (!done) {
				break;
			}
		} else {
			break;
		}
		logo->mState = 5;
	case 5:
		if (logo->mAlpha > 0.0f && (logo->mAlpha -= (1.0f / (0.5f * (f32)gRefreshRate)) * frames) < 0.0f) {
			logo->mAlpha = 0.0f;
		}
		if (logo->mAlpha == 0.0f) {
			logo->mState = 6;
			logo->ResetFade();
		}
		break;
	case 6:
		logo->mState = 9;
	case 9:
		if (logo->mAlpha == 0.0f) {
			logo->mState = 12;
			logo->ResetFade();
			logo->mFile.Close();
			logo->mTpl.Close();
		}
		break;
	case 12:
		break;
	}
}

/**
 * @note Address: 0x8004488C
 * @note Size: 0x74C
 */
// NONMATCHING: float regalloc, as in the GameCube function: retail keeps width, height, x and y in
// f30, f31, f28, f29 for the first logo and in f31, f30, f29, f28 for the other two; this build in
// f31, f30, f29, f28 and f28, f29, f30, f31
// Logo::Draw, the current logo centred on the screen at its alpha
extern "C" void DrawCurrentLogo(Logo* logo)
{
	if (logo->IsActive() && ((CBase*)&gGame)->vf5C()) {
		if (logo->mAlpha > 0.0f) {
			SetGsChannelEnabled(&gGame.mGs, 0, 0);
			gGame.mGs.mCurRenderState->SetColorUpdate(GX_TRUE);
			gGame.mGs.mCurRenderState->SetAlphaUpdate(GX_FALSE);
			gGame.mGs.mCurRenderState->SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_LEQUAL, 255);
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			CTexObj tex;
			if (logo->mState == 3 || logo->mState == 4 || logo->mState == 5) {
				s32 index;
				switch (logo->mState) {
				case 3:
					index = 0;
					break;
				case 4:
				case 5:
					if (logo->mTimer < 5.0f * (f32)gRefreshRate) {
						index = 0;
					} else if (logo->mTimer < 10.0f * (f32)gRefreshRate) {
						index = 1;
					} else {
						index = !(logo->mTimer < 15.0f * (f32)gRefreshRate);
					}
					break;
				}
				logo->mTpl.mTex.Init(logo->GetDesc(index), 0);
				tex       = logo->mTpl.mTex;
				f32 w     = GXGetTexObjWidth(&tex);
				f32 h     = GXGetTexObjHeight(&tex);
				f32 x     = gPointerScreenWidth / 2;
				f32 y     = gPointerScreenHeight / 2;
				u32 color = (u32)(255.0f * logo->mAlpha) | 0xFFFFFF00;
				DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			}
			if (logo->mState == 6 || logo->mState == 7 || logo->mState == 8) {
				logo->mTpl.mTex.Init(logo->GetDesc(2), 0);
				tex       = logo->mTpl.mTex;
				f32 w     = GXGetTexObjWidth(&tex);
				f32 h     = GXGetTexObjHeight(&tex);
				f32 x     = gPointerScreenWidth / 2;
				f32 y     = h / 2.0f + 154.0f;
				u32 color = (u32)(255.0f * logo->mAlpha) | 0x0046FF00;
				DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			}
			if (logo->mState == 9 || logo->mState == 10 || logo->mState == 11) {
				logo->mTpl.mTex.Init(logo->GetDesc(3), 0);
				tex       = logo->mTpl.mTex;
				f32 w     = GXGetTexObjWidth(&tex);
				f32 h     = GXGetTexObjHeight(&tex);
				f32 x     = gPointerScreenWidth / 2;
				f32 y     = gPointerScreenHeight / 2;
				u32 color = (u32)(255.0f * logo->mAlpha) | 0xFFFFFF00;
				DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			}
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
		((CBase*)&gGame)->vf54();
	}
}
