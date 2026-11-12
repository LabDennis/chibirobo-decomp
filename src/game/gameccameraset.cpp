#include "game/cgame.h"
#include "game/cgameutil.h"
#include "game/xchr.h"
#include "game/globals.h"
#include "game/mic.h"
extern "C" {
#include <sysdolphin/initialize.h>
void GXSetScissor(u32, u32, u32, u32);
void BuildSceneDrawLists(CGame*);
void RenderSceneShadow(CGame*);
void RenderMirrorTextures(CGame*);
void RenderFloorReflection(CGame*);
void DrawObjectAlphaMask(CGame*);
void DrawStageNameBubbles(CGame*);
void DrawTitleScreen(Title*);
void DrawGameMovie(CGame*);
void DrawPlayerEquipmentRings(XObjChrMenu*);
void DrawOXMenu(OXMenu*);
void DrawScanModePrompt();
void CMsg_Draw(CMsg*);
void fn_8007E978();
void DrawObjectLoadingProgress(InfoScrn*);
void DrawHudWithTransitions(InfoScrn*);
void DrawHudPauseLabel(InfoScrn*);
void DrawOpenMenus(MenuEx*);
void ClearActiveObjectFloors(XObj*);
}

static inline void DrawFrameQuad(const Vec (&v)[4], u32 color)
{
	DrawGsVertexQuad(&gGame.mGs, v[0].x, v[0].y, v[0].z, v[1].x, v[1].y, v[1].z, v[2].x, v[2].y, v[2].z, v[3].x, v[3].y, v[3].z, color,
	                 color, color, color);
}

static inline void DrawSubwindowObjects(const XOBJS& objects, CCamera* camera)
{
	for (s32 i = 0; i < objects.mObjectCount; ++i) {
		XObj* object = objects.mObjects[i];
		BOOL draw    = FALSE;
		if ((object->mStatus.mFlags & 1) && (object->mStatus.mFlags & 0x10000)) {
			if (object->mStatus.mFlags & 0x02000000)
				draw = TRUE;
			else {
				f32 radius = object->GetFloorXZExtent() * .5f;
				f32 height = object->GetCollisionHeight();
				if (object->ProjectVisibleBounds(NULL, NULL, camera, radius, height))
					draw = TRUE;
			}
		}
		if (draw)
			object->vfAC();
	}
}

/**
 * @note Address: 0x8000DFA8
 * @note Size: 0x194C (US, JP), 0x196C (PAL)
 */
// NONMATCHING: retail does the float-to-int conversions without __cvt_fp2unsigned calls,
// a different cast form than the current source
void CGame::Draw()
{
	CVec interest = GetCamera()->mInterest;
	if (mQuake.mShake[0].x != 0 || mQuake.mShake[0].y != 0 || mQuake.mShake[0].z != 0) {
		GetCamera()->SetInterest(interest + mQuake.mShake[0] * (f32)mQuake.mSign);
		GetCamera()->CalcEye();
	}
	mGs.mCamera = GetCamera();
	if (BeginGsCamera(this)) {
		BuildSceneDrawLists(this);
		EndGsCamera(&mGs);
	}
	RenderSceneShadow(this);
	HSD_StartRender(HSD_RP_SCREEN);
	gGame.mGs.mLight = mLightList;
	if ((mStatus.mFlags & 1) && (mStatus.mFlags & 0x10000000000ULL) && mSubwin.mAmount > 0) {
		CCamera* camera = &mSubwin.mCamera;
		mGs.mCamera     = camera;
		if (BeginGsCamera(this)) {
#if defined(VERSION_PAL)
			f32 scale = gGame.mGs.mScreen._40;
			GXSetScissor(0, (u32)(0.f * scale), 320, (u32)(224.f * scale));
#else
			GXSetScissor(0, 0, 320, 224);
#endif
			DrawSubwindowObjects(mXObjs, camera);
			EndGsCamera(&mGs);
		}
		mSubwin.Copy();
	}
	RenderMirrorTextures(this);
	RenderFloorReflection(this);
	if ((mStatus.mFlags & 0x4000000000ULL) && !(GetPlayer()->mStatus.mFlags & 0x200000))
		DrawObjectAlphaMask(this);
	vf20();
	mGs.mCamera = vf34();
	if (BeginGsCamera(this)) {
		if (mStatus.mFlags & 1) {
			if (mStatus.mFlags & 0x4000000000ULL) {
				if (GetPlayer()->mStatus.mFlags & 0x200000)
					mOutline.Draw(9);
				else
					mOutline.Draw(17);
			}
			if (mStatus.mFlags & 0x10000)
				mOutline.Draw(4);
		}
#if defined(VERSION_PAL)
		mBlur.Draw();
#else
		mBlur.Draw(TRUE);
#endif
		InvalidateAll();
		EndGsCamera(&mGs);
	}
	mGs.mCamera = GetCamera();
	if (BeginGsCamera(this)) {
		SetupAll(TRUE);
		if (mStatus.mFlags & 1) {
			for (s32 i = 0; i < mDrawLists[2].mCount; ++i) {
				XObj* object = mDrawLists[2].mObjects[i];
				if ((object->mStatus.mFlags & 1)
				    && !(mColor0.mColor.mColor.a > 0
				         && ((object->mStatus.mFlags & 0x200)
				             || ((object->mStatus.mFlags & 0x0400000000000000ULL) && (object->mStatus.mFlags & 0x400000000000ULL)))))
					object->DrawObjectEffects();
			}
			CallStageModule(3, 0, NULL);
		}
		mParticle.Draw(1);
		SetupAll(TRUE);
		EndGsCamera(&mGs);
	}
	if (mColor0.mColor.mColor.a > 0 && (mStatus.mFlags & 1)) {
		mOutline.Draw(0x22);
		mGs.mCamera = GetCamera();
		if (BeginGsCamera(this)) {
			InvalidateAll();
			for (s32 i = 0; i < mDrawLists[1].mCount; ++i) {
				XObj* object = mDrawLists[1].mObjects[i];
				if (object->mStatus.mFlags & 0x200)
					object->vfAC();
				else if ((object->mStatus.mFlags & 0x0400000000000000ULL) && (object->mStatus.mFlags & 0x400000000000ULL))
					object->vfAC();
			}
			EndGsCamera(&mGs);
		}
	}
	mGs.mCamera = GetCamera();
	if (BeginGsCamera(this)) {
		SetGsColorUpdate(gGame.mGs.mCurRenderMode, 1);
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetupAll(TRUE);
		if ((mStatus.mFlags & 1) && mColor0.mColor.mColor.a > 0) {
			for (s32 i = 0; i < mDrawLists[2].mCount; ++i) {
				XObj* object = mDrawLists[2].mObjects[i];
				if (!(object->mStatus.mFlags & 1))
					continue;
				if (object->mStatus.mFlags & 0x200)
					object->DrawObjectEffects();
				else if ((object->mStatus.mFlags & 0x0400000000000000ULL) && (object->mStatus.mFlags & 0x400000000000ULL))
					object->DrawObjectEffects();
			}
		}
		mParticle.Draw(2);
		EndGsCamera(&mGs);
	}
	gGame.mGs.mLight = vf40();
	DrawTitleScreen(&mTitle);
	mGs.mCamera = vf34();
	if (BeginGsCamera(this)) {
		InvalidateAll();
		SetupAll(TRUE);
		vf24();
		Vec corners[4] = { (Vec) { 0, 0, -1 }, (Vec) { 640, 0, -1 }, (Vec) { 640, 448, -1 }, (Vec) { 0, 448, -1 } };
		SetGsColorUpdate(gGame.mGs.mCurRenderMode, 0);
		SetGsAlphaUpdate(gGame.mGs.mCurRenderMode, 0);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
		DrawFrameQuad(corners, 0);
		SetGsColorUpdate(gGame.mGs.mCurRenderMode, 1);
		SetGsAlphaUpdate(gGame.mGs.mCurRenderMode, 0);
		if ((mStatus.mFlags & 1) && (mStatus.mFlags & 0x10000000000ULL) && mSubwin.mAmount > 0 && !(mSubwin.mFlags.mFlags & 4))
			mSubwin.Draw();
		mRoboeyes.Draw(&gGame.mGs, 0);
		DrawHudWithTransitions(&mInfoScrn);
		if (mStatus.mFlags & 1)
			mMotionControlCamera.Draw();
		if (mMovieFlags & 2)
			DrawGameMovie(this);
		DrawScreenColorOverlay(&mFade, &gGame.mGs);
		mWipeCircle.Draw(&gGame.mGs, 0);
		mWipeTex.Draw(&gGame.mGs, 1);
		mWipeCustom.Draw(&gGame.mGs, 1);
		mWipeTile0.Draw(&gGame.mGs, 1);
		mWipeTile1.Draw(&gGame.mGs, 1);
		if (!(mMovieFlags & 2))
			DrawGameMovie(this);
		DrawStageNameBubbles(this);
		if (mStatus.mFlags & 0x400)
			DrawPlayerEquipmentRings(&GetPlayer()->mMenu);
		if (mStatus.mFlags & 0x800)
			DrawOXMenu(&GetPlayer()->mOXMenu);
		if (!gGame.mMotionControlCamera.IsAction(4)) {
			fn_8007E978();
			for (s32 i = 0; i < 2; ++i)
				CMsg_Draw(&mCMsgs[i]);
		}
		DrawScanModePrompt();
		EndGsCamera(&mGs);
	}
	if (mColor1.mColor.mColor.a > 0) {
#if defined(VERSION_PAL)
		mBlur.Copy();
#else
		mBlur.Copy(TRUE);
#endif
		mOutline.Draw(0x42);
		mBlur.DrawTex();
		mOutline.Draw(0x80);
		InvalidateAll();
		mGs.mCamera = vf34();
		if (BeginGsCamera(this)) {
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			CTexObj tex = mTpl.mTex.Init(&((TPLPalette*)mTpl.mBuffer)->descriptorArray[121 - StatusTextureOffset], 0);
			u16 width = GXGetTexObjWidth(&tex), height = GXGetTexObjHeight(&tex);
			PSMTXScale(tex.mMtx[1], 640 / width, 448 / height, 1);
			tex.mMtxDirty = TRUE;
			u32 c         = 0xffffff00 | (u32)(48.f * (mColor1.mColor.mColor.a / 255.f));
			DrawGsTexturedQuad(&gGame.mGs, &tex, c, c, c, c, 320, 224, 0, 640, 448, 0, 0, 0);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
			EndGsCamera(&mGs);
		}
	}
	mGs.mCamera = vf34();
	if (BeginGsCamera(this)) {
		SetupAll(TRUE);
		DrawObjectLoadingProgress(&mInfoScrn);
		DrawHudPauseLabel(&mInfoScrn);
		DrawCurrentLogo(&mLogo);
		DrawOpenMenus(&mMenu);
		if (gActiveMicChannel >= 0 && (gGame.mStatus.mFlags & 0x80000000) && gMicMeterPeak > 0) {
			gMicMeterPeak = (s32)(gMicMeterPeak * .0608f);
			if (gMicMeterPeak > 608)
				gMicMeterPeak = 608;
			SetGsChannelEnabled(&gGame.mGs, 0, 0);
			SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
			SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
			Vec v[4] = { { 16, 414, 0 }, { 0, 414, 0 }, { 0, 419, 0 }, { 16, 419, 0 } };
			v[1].x   = 16 + gMicMeterPeak;
			v[2].x   = 16 + gMicMeterPeak;
			DrawFrameQuad(v, 0xff000080);
			gMicMeterPeak = 0;
		}
		if (IsDiscError()) {
#if defined(VERSION_JP)
			const int first = 141;
#else
			const int first = 84;
#endif
			s32 errors[][2]  = { { 5, first }, { 4, first + 1 }, { 6, first + 2 }, { 11, first + 3 }, { 1, first + 4 }, { 0, -1 } };
			const char* text = NULL;
			for (s32 i = 0; errors[i][1] >= 0; ++i)
				if (errors[i][0] == mDvdStatus)
					text = GetLocalizedInterfaceText(errors[i][1]);
			if (text) {
				f32 w = MeasureGameTextWidth(&gGame.mGs, text, TRUE, 24), h = MeasureGameTextHeight(&gGame.mGs, text, 24);
				f32 x = 320 - .5f * w, y = 224 - .5f * h;
				SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
				Vec v[4] = {
					{ x - 24, y - 24, 0 }, { 24 + (x + w), y - 24, 0 }, { 24 + (x + w), 24 + (y + h), 0 }, { x - 24, 24 + (y + h), 0 }
				};
				DrawFrameQuad(v, 200);
				DrawGsProportionalTextFormat(&gGame.mGs, 0xffffffff, text, x, y, 0, 24, 24);
				SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
			}
		}
		EndGsCamera(&mGs);
	}
	HSD_EndRender();
	for (s32 i = 0; i < mXObjs.mObjectCount; ++i) {
		XObj* object = mXObjs.mObjects[i];
		if ((object->mStatus.mFlags & 1) && (object->mStatus.mFlags & 0x10000))
			ClearActiveObjectFloors(object);
	}
	GetCamera()->SetInterest(interest);
}

/**
 * @note Address: 0x8000F8F4
 * @note Size: 0x38
 */
// todo: weak copy
extern "C" void SetInterest__7CCameraFRC3Vec(CCamera* camera, const Vec* interest)
{
	camera->SetInterest(interest->x, interest->y, interest->z);
}

/**
 * @note Address: 0x8000F92C
 * @note Size: 0x10
 */
// todo: weak copy
extern "C" void SetInterest__7CCameraFfff(CCamera* camera, f32 x, f32 y, f32 z)
{
	camera->mInterest.x = x;
	camera->mInterest.y = y;
	camera->mInterest.z = z;
}

/**
 * @note Address: 0x8000F93C
 * @note Size: 0x84
 */
CVec CVec::operator+(Vec v) const
{
	CVec sum = *this;
	PSVECAdd(&sum, &v, &sum);
	return sum;
}

/**
 * @note Address: 0x8000F9C0
 * @note Size: 0x80
 */
CVec CVec::operator*(f32 s) const
{
	CVec product = *this;
	PSVECScale(&product, &product, s);
	return product;
}
