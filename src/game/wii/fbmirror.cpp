#include "game/fbtexeff.h"
#include "game/mm.h"
#include "game/wii/cgame.h"
#include "game/wii/cgs.h"
#include "Dolphin/OS/OSCache.h"
#include <string.h>

extern s32 gScreenSafeTop;
extern s32 gScreenSafeRight;

// the names the mirror binds through, kept as pointers in .sdata
static const char* sMirrorNodeName = "mirror";
static const char* sMirrorMatName  = "mirror_mat";

// the nw4r resource lookups SetMirror runs through and the assertion printer behind
// g3d_resmat_ac.h's checks; only the addresses are retail's, the signatures are ours
extern "C" {
void* fn_8025CFC8(void* mdl);                   // the model's resource data
void* fn_8025D564(void* mdl, const char* name); // ResMdl named material lookup
void* fn_8027E5CC(void* out, void* model, void* mat);
void* fn_8027E3D4(void* search, s32 index);
s32 fn_8025F66C(void* anms, s32 index);   // has anm
void* fn_8025F638(void* anms, s32 index); // the nth 0x20-byte anm record
}

namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}
} // namespace nw4r

/**
 * @note Address: 0x80019BF4
 * @note Size: 0xC8
 */
// creates the texture's image (or takes ownership of the given one) and inits its texobj
void CreateFbTexture(WiiFbTexture* texture, u16 width, u16 height, GXTexFmt format, BOOL mipmap, void* image, const char* name)
{
	texture->mWidth  = width;
	texture->mHeight = height;
	texture->mFormat = format;
	texture->_0C     = 0;
	texture->mX      = 0.0f;
	texture->mY      = 0.0f;
	texture->mMipmap = mipmap != 0;
	if (image != NULL) {
		texture->mImage     = image;
		texture->mOwnsImage = FALSE;
	} else {
		texture->mImage     = mm::Alloc(GXGetTexBufferSize(width, height, format, GX_FALSE, 0), name);
		texture->mOwnsImage = TRUE;
	}
	texture->mTex.Init(texture->mImage, texture->mWidth, texture->mHeight, texture->mFormat, GX_CLAMP, GX_CLAMP, GX_FALSE);
}

/**
 * @note Address: 0x80019CBC
 * @note Size: 0x4C
 */
void FreeFbTexture(WiiFbTexture* texture)
{
	if (texture->mOwnsImage && texture->mImage != NULL) {
		mm::Free(texture->mImage);
		texture->mImage = NULL;
	}
}

/**
 * @note Address: 0x80019D08
 * @note Size: 0xE4
 */
// copies the efb into the texture; mipmap textures grab it at twice the size
void CopyFbTexture(WiiFbTexture* texture, GXTexFmt format, BOOL clear, BOOL flush)
{
	if (texture->mImage == NULL) {
		return;
	}
	u16 width;
	u16 height;
	if (texture->mMipmap) {
		width  = texture->mWidth * 2;
		height = texture->mHeight * 2;
	} else {
		width  = texture->mWidth;
		height = texture->mHeight;
	}
	GXSetTexCopySrc(0, 0, width, height);
	GXSetTexCopyDst(texture->mWidth, texture->mHeight, format, texture->mMipmap);
	GXCopyTex(texture->mImage, clear != 0);
	GXPixModeSync();
	if (flush) {
		DCFlushRange(texture->mImage, GXGetTexBufferSize(texture->mWidth, texture->mHeight, format, GX_FALSE, 0));
	}
}

/**
 * @note Address: 0x80019DEC
 * @note Size: 0x38
 */
void DrawFbTexture(WiiFbTexture* texture, u32 color, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx, f32 ry, f32 rz)
{
	if (texture->mImage == NULL) {
		return;
	}
	DrawGsTexturedQuad(&gGame.mGs, &texture->mTex, color, color, color, color, x, y, z, w, h, rx, ry, rz);
}

// the mirror camera's orthographic frustum: half-extent 11.5, near 0.1, far 32768
static const f32 sMirrorFrustum[6] = { 11.5f, -11.5f, -11.5f, 11.5f, 0.1f, 32768.0f };

/**
 * @note Address: 0x80019E24
 * @note Size: 0xD8
 */
// FbMirror::Init - the mirror renders a 256 square through an orthographic camera
void FbMirror::Init()
{
	mJoint    = NULL;
	mJointAnm = NULL;
	mCamera.vf10(sMirrorFrustum[4], sMirrorFrustum[5], sMirrorFrustum[0], sMirrorFrustum[1], sMirrorFrustum[2], sMirrorFrustum[3], 1.0f,
	             &gGame.mGs.mScreen);
	u32 size = 0x100;
	mCamera.vf88(0.0f, 0.0f, size, size, 0.0f, 1.0f);
	Reset();
}

/**
 * @note Address: 0x80019F20
 * @note Size: 0x4
 */
void FbMirror::Reset()
{
}

/**
 * @note Address: 0x80019F24
 * @note Size: 0x1BC
 */
// NONMATCHING: binds the model's "mirror" node and the "mirror_mat" ResAnmTexPat, copies the
// anm in at _130 and patches our texobj over its head so it samples the mirror's copy. Retail's
// frame is 0x50 - real nw4r ResMat/iterator/ResAnmTexPat locals - so its CreateFbTexture call
// stays a call; ours is small enough that ipa inlines it. The nw4r calls go through externed
// addresses - the real types and the g3d_resmat_ac.h inlines they came from aren't known.
void FbMirror::SetMirror(CMirrorObj* obj)
{
	mJoint   = obj;
	mVisible = FALSE;
	if (mJoint == NULL) {
		return;
	}
	CreateFbTexture(&mTexture, 0x100, 0x100, GX_TF_RGB565, FALSE, NULL, "(mirror)");
	void* mat = fn_8025D564(&mJoint->mMdl, sMirrorMatName);
	if (mat == NULL) {
		return;
	}
	if (mat == NULL) { // ResMat::ref's validity check
		nw4r::db::Panic("g3d_resmat_ac.h", 0x26D, "%s::%s: Object not valid.", "ResMat", "ref");
	}
	void* search;
	fn_8027E5CC(&search, mJoint->mF0, *(void**)((u8*)mat + 0xC));
	void* scn = fn_8027E3D4(&search, 0);
	if (scn == NULL) {
		if (mat == NULL) { // ResMat::ref's validity check
			nw4r::db::Panic("g3d_resmat_ac.h", 0x26D, "%s::%s: Object not valid.", "ResMat", "ref");
		}
		scn = (u8*)mat + 0x40;
		if (((u32)scn & 3) != 0) {
			nw4r::db::Panic("g3d_resmat_ac.h", 0x28, "NW4R:Failed assertion !((u32)p & 0x3)");
		}
	}
	if (fn_8025F66C(&scn, 0)) {
		void* anm = fn_8025F638(&scn, 0);
		mJointAnm = anm;
		memcpy(mJointAnmCopy, anm, 0x20);
		memcpy(mJointAnm, &mTexture.mTex, 0x20);
	} else {
		mJointAnm = NULL;
	}
}

/**
 * @note Address: 0x8001A0E0
 * @note Size: 0x2D8
 */
// FbMirror::Update, puts the mirror camera at the main camera's eye reflected in the surface,
// looking at the surface through a frustum that just fits it
void FbMirror::Update(u32 frames)
{
	if (IsUsed()) {
		nw4r::g3d::ResNode node = mJoint->vf50(sMirrorNodeName);
		if (!node.IsValid()) {
			return;
		}
		Mtx mtx;
		Mtx view;
		Vec eye;
		Vec pos;
		Vec diff;
		Vec x;
		Vec y;
		Vec normal;

		PSMTXCopy(mJoint->vfA0(node), mtx);
		x.x = mtx[0][0];
		x.y = mtx[1][0];
		x.z = mtx[2][0];
		y.x = mtx[0][1];
		y.y = mtx[1][1];
		y.z = mtx[2][1];
		PSVECScale(&x, &x, -1.0f); // the surface normal is -x cross y
		PSVECCrossProduct(&x, &y, &normal);
		if (normal.x == 0.0f && normal.y == 0.0f && normal.z == 0.0f) {
			return; // a degenerate plane has no mirror
		}
		PSVECNormalize(&normal, &normal);
		mtx[0][0] = x.x;
		mtx[1][0] = x.y;
		mtx[2][0] = x.z;
		mtx[0][2] = normal.x;
		mtx[1][2] = normal.y;
		mtx[2][2] = normal.z;

		CMotionControlCamera* main = &gGame.mMotionControlCamera;
		eye.x                      = main->mView.mEye.x;
		eye.y                      = main->mView.mEye.y;
		eye.z                      = main->mView.mEye.z;
		pos.x                      = mtx[0][3];
		pos.y                      = mtx[1][3];
		pos.z                      = mtx[2][3];
		PSVECSubtract(&pos, &eye, &diff);
		f32 dist = PSVECDotProduct(&normal, &diff);
		if (dist <= 0.0f) { // behind the surface, the mirror hides
			mVisible = FALSE;
		} else {
			PSVECScale(&normal, &diff, 2.0f * dist); // the eye mirrored across the plane
			PSVECAdd(&diff, &eye, &diff);
			mtx[0][3] = diff.x;
			mtx[1][3] = diff.y;
			mtx[2][3] = diff.z;
			PSMTXInverse(mtx, view);
			PSMTXMultVec(view, &pos, &pos);
			mCamera.vf1C(-pos.z + sMirrorFrustum[4]); // near starts at the surface's depth, far 32768 beyond it
			mCamera.vf20(-pos.z + sMirrorFrustum[5]);
			mCamera.vf24(pos.y + sMirrorFrustum[0], pos.y + sMirrorFrustum[1], pos.x + sMirrorFrustum[2], pos.x + sMirrorFrustum[3]);
			mCamera.SetEye(diff);
			mCamera.vf64(view);
			PSMTXCopy(view, mView);
			mVisible = TRUE;
		}
	}
}

// XObj's own copy of CObj::GetResNode(name); it sits in every XObj-derived vtable
nw4r::g3d::ResNode XObj::GetResNode(const char* name)
{
	return mObject.m_resMdl.GetResNode(name);
}

// NONMATCHING: retail reads the node's data pointer once, ahead of the mF0 test, and tests that
// register for both the assertion and the index; ours reads it again after the mF0 test.
MtxP CMirrorObj::vfA0(nw4r::g3d::ResNode node)
{
	if (mF0 != NULL) {
		return mF0->mpWorldMtxArray[node.GetMtxID()];
	}
	return NULL;
}

/**
 * @note Address: 0x8001A450
 * @note Size: 0xC
 */
CCamera* CGame::GetCamera()
{
	return &mMotionControlCamera;
}

/**
 * @note Address: 0x8001A4D4
 * @note Size: 0x140
 */
// FbMirror's Copy - applies the pending gx state and copies the efb into the mirror texture
void FbMirror::Copy()
{
	if (mVisible) {
		CGs* gs = &gGame.mGs;
		gs->mCurRenderState->SetColorUpdate(GX_TRUE);
		gs->mCurRenderState->SetAlphaUpdate(GX_TRUE);
		gs->mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_TRUE);
		if (gs->mCurRenderState != NULL) {
			gs->mCurRenderState->Apply();
		}
		if (gGame.mGs.mCurTevStage != NULL) {
			gGame.mGs.mCurTevStage->Apply();
		}
		if (gGame.mGs.mCurChannel != NULL) {
			gGame.mGs.mCurChannel->Apply();
		}
		CopyFbTexture(&mTexture, mTexture.mFormat, GX_TRUE, GX_FALSE);
	}
}

/**
 * @note Address: 0x8001A614
 * @note Size: 0xF0
 */
// FbMirror's Draw - draws the mirror's texture as a 256 square in the screen's top right
void FbMirror::Draw()
{
	if (mJoint != NULL && mVisible) {
		gGame.mGs.mCurTevStage->SetMode((GXTevMode)0);
		f32 w = 256.0f;
		DrawFbTexture(&mTexture, 0xFFFFFFC8, gScreenSafeRight - w / 2, gScreenSafeTop + w / 2, 0.0f, w, w, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode((GXTevMode)4);
	}
}
