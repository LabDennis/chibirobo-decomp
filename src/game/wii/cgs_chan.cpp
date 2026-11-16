// The Wii's cgs_chan.cpp (the name is in its assert), .text 0x802A9D9C-0x802AA610: the tev stage and
// the lighting channel. Included by cgs.cpp, not a unit of its own. Whether the tev stage is this
// file's or another included file's is not known; it has no assert.

#include "game/wii/cgs.h"
#include "Dolphin/os.h"

void GSTevOrder::Set(GSTevOrder src)
{
	Set(src.mCoord, src.mMap, src.mColor);
}

void CGsTevStage::Set(GXTevStageID id, GSTevOrder order, GSTevColorOp colorOp, GSTevColorIn colorIn, GSTevAlphaOp alphaOp,
                      GSTevAlphaIn alphaIn)
{
	mId = id;
	mOrder.Set(order);
	mColorOp.Set(colorOp);
	mColorIn.Set(colorIn);
	mAlphaOp.Set(alphaOp);
	mAlphaIn.Set(alphaIn);
	mKColorSel.mSel       = GX_TEV_KCSEL_K0;
	mKAlphaSel.mSel       = GX_TEV_KASEL_K0_A;
	mSwapMode.mRas        = GX_TEV_SWAP0;
	mSwapMode.mTex        = GX_TEV_SWAP0;
	mSwapModeTable.mTable = GX_TEV_SWAP0;
	mSwapModeTable.mRed   = GX_CH_RED;
	mSwapModeTable.mGreen = GX_CH_GREEN;
	mSwapModeTable.mBlue  = GX_CH_BLUE;
	mSwapModeTable.mAlpha = GX_CH_ALPHA;
	mTexGens.mNum         = 0;
	mTexGen.mCoord        = GX_TEXCOORD0;
	mTexGen.mFunc         = GX_TG_MTX2x4;
	mTexGen.mSrc          = GX_TG_TEX0;
	mTexGen.mMtx          = GX_IDENTITY;
	mIndirect             = FALSE;
	SetNext(NULL);
}

void GSTevColorOp::Set(GSTevColorOp src)
{
	Set(src.mOp, src.mBias, src.mScale, src.mClamp, src.mReg);
}

void GSTevColorIn::Set(GSTevColorIn src)
{
	Set(src.mA, src.mB, src.mC, src.mD);
}

void GSTevAlphaOp::Set(GSTevAlphaOp src)
{
	Set(src.mOp, src.mBias, src.mScale, src.mClamp, src.mReg);
}

void GSTevAlphaIn::Set(GSTevAlphaIn src)
{
	Set(src.mA, src.mB, src.mC, src.mD);
}

void CGsTevStage::SetNext(CGsTevStage* next)
{
	if (mNext != next) {
		mNext = next;
	}
}

s32 CGsTevStage::GetNum()
{
	switch (mId) {
	case GX_TEVSTAGE0:
		return 1;
	case GX_TEVSTAGE1:
		return 2;
	case GX_TEVSTAGE2:
		return 3;
	case GX_TEVSTAGE3:
		return 4;
	case GX_TEVSTAGE4:
		return 5;
	case GX_TEVSTAGE5:
		return 6;
	case GX_TEVSTAGE6:
		return 7;
	case GX_TEVSTAGE7:
		return 8;
	case GX_TEVSTAGE8:
		return 9;
	case GX_TEVSTAGE9:
		return 10;
	case GX_TEVSTAGE10:
		return 11;
	case GX_TEVSTAGE11:
		return 12;
	case GX_TEVSTAGE12:
		return 13;
	case GX_TEVSTAGE13:
		return 14;
	case GX_TEVSTAGE14:
		return 15;
	case GX_TEVSTAGE15:
		return 16;
	}
	return 0;
}

void CGsTevStage::Apply()
{
	s32 num = 0;
	for (CGsTevStage* stage = this; stage != NULL; stage = stage->mNext) {
		s32 n = stage->GetNum();
		if (n > num) {
			num = n;
		}
		stage->Setup();
	}
	GXSetNumTevStages(num);
	GXSetTevSwapModeTable(mSwapModeTable.mTable, mSwapModeTable.mRed, mSwapModeTable.mGreen, mSwapModeTable.mBlue, mSwapModeTable.mAlpha);
	GXSetNumTexGens(mTexGens.mNum);
	GXSetTexCoordGen2(mTexGen.mCoord, mTexGen.mFunc, mTexGen.mSrc, mTexGen.mMtx, GX_FALSE, GX_PTIDENTITY);
}

void CGsTevStage::Setup()
{
	if (!mIndirect) {
		GXSetTevDirect(mId);
	}
	GXSetTevOrder(mId, mOrder.mCoord, mOrder.mMap, mOrder.mColor);
	if (!mCustom) {
		GXSetTevOp(mId, mMode.mMode);
		GXSetTevSwapMode(mId, mSwapMode.mRas, mSwapMode.mTex);
	} else {
		GXSetTevColorOp(mId, mColorOp.mOp, mColorOp.mBias, mColorOp.mScale, mColorOp.mClamp, mColorOp.mReg);
		GXSetTevColorIn(mId, mColorIn.mA, mColorIn.mB, mColorIn.mC, mColorIn.mD);
		GXSetTevAlphaOp(mId, mAlphaOp.mOp, mAlphaOp.mBias, mAlphaOp.mScale, mAlphaOp.mClamp, mAlphaOp.mReg);
		GXSetTevAlphaIn(mId, mAlphaIn.mA, mAlphaIn.mB, mAlphaIn.mC, mAlphaIn.mD);
		GXSetTevKColorSel(mId, mKColorSel.mSel);
		GXSetTevKAlphaSel(mId, mKAlphaSel.mSel);
		GXSetTevSwapMode(mId, mSwapMode.mRas, mSwapMode.mTex);
	}
}

void CGsChannel::SetCtrl(GXChannelID id, GSChanCtrl ctrl)
{
	mId              = id;
	mCtrl.mEnable    = ctrl.mEnable;
	mCtrl.mAmbSrc    = ctrl.mAmbSrc;
	mCtrl.mMatSrc    = ctrl.mMatSrc;
	mCtrl.mLightMask = ctrl.mLightMask;
	mCtrl.mDiffFn    = ctrl.mDiffFn;
	mCtrl.mAttnFn    = ctrl.mAttnFn;
	SetNext(NULL);
}

void CGsChannel::Set(GXChannelID id, GSChanCtrl ctrl, GXColor ambient, GXColor material)
{
	SetCtrl(id, ctrl);
	SetAmbColor(ambient);
	SetMatColor(material);
}

void CGsChannel::SetNext(CGsChannel* next)
{
	if (mNext != next) {
		mNext = next;
	}
}

s32 CGsChannel::GetNum()
{
	switch (mId) {
	case GX_COLOR0:
	case GX_ALPHA0:
	case GX_COLOR0A0:
		return 1;
	case GX_COLOR1:
	case GX_ALPHA1:
	case GX_COLOR1A1:
		return 2;
	case GX_COLOR_NULL:
		return 0;
	default:
		OSPanic(__FILE__, 89, "");
		return 0;
	}
}

void CGsChannel::Apply()
{
	s32 num = 0;
	for (CGsChannel* channel = this; channel != NULL; channel = channel->mNext) {
		s32 n = channel->GetNum();
		if (n > num) {
			num = n;
		}
		channel->Setup();
	}
	GXSetNumChans(num);
}

void CGsChannel::Setup()
{
	GXChannelID id = mId;
	s32 index      = id & 1;
	if (mCtrl.mEnable && mCtrl.mAmbSrc == GX_SRC_REG) {
		GXSetChanAmbColor((GXChannelID)(index + GX_COLOR0A0), mAmbColor);
	}
	if (mCtrl.mMatSrc == GX_SRC_REG) {
		GXSetChanMatColor((GXChannelID)(index + GX_COLOR0A0), mMatColor);
	}
	GXSetChanCtrl(id, mCtrl.mEnable, mCtrl.mAmbSrc, mCtrl.mMatSrc, mCtrl.mLightMask, mCtrl.mDiffFn, mCtrl.mAttnFn);
}
