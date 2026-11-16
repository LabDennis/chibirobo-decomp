// The Wii's cgs_scrn.cpp (the name is in its assert), .text 0x802A977C-0x802A9D9C: the screen mode and
// the render state with its twelve GS settings. Included by cgs.cpp, not a unit of its own. Whether the
// render state is this file's or another included file's is not known; it has no assert.

#include "game/wii/cgs.h"
#include "Dolphin/os.h"
#include "Dolphin/vi.h"

extern "C" {
extern GXRenderModeObj GXNtsc240Ds;
extern GXRenderModeObj GXNtsc240DsAa;
extern GXRenderModeObj GXNtsc240Int;
extern GXRenderModeObj GXNtsc240IntAa;
extern GXRenderModeObj GXNtsc480IntDf;
extern GXRenderModeObj GXNtsc480Int;
extern GXRenderModeObj GXNtsc480IntAa;
extern GXRenderModeObj GXNtsc480Prog;
extern GXRenderModeObj GXNtsc480ProgSoft;
extern GXRenderModeObj GXNtsc480ProgAa;
extern GXRenderModeObj GXMpal240Ds;
extern GXRenderModeObj GXMpal240DsAa;
extern GXRenderModeObj GXMpal240Int;
extern GXRenderModeObj GXMpal240IntAa;
extern GXRenderModeObj GXMpal480IntDf;
extern GXRenderModeObj GXMpal480Int;
extern GXRenderModeObj GXMpal480IntAa;
extern GXRenderModeObj GXMpal480Prog;
extern GXRenderModeObj GXMpal480ProgSoft;
extern GXRenderModeObj GXMpal480ProgAa;
extern GXRenderModeObj GXPal264Ds;
extern GXRenderModeObj GXPal264DsAa;
extern GXRenderModeObj GXPal264Int;
extern GXRenderModeObj GXPal264IntAa;
extern GXRenderModeObj GXPal528IntDf;
extern GXRenderModeObj GXPal528Int;
extern GXRenderModeObj GXPal524IntAa;
extern GXRenderModeObj GXEurgb60Hz240Ds;
extern GXRenderModeObj GXEurgb60Hz240DsAa;
extern GXRenderModeObj GXEurgb60Hz240Int;
extern GXRenderModeObj GXEurgb60Hz240IntAa;
extern GXRenderModeObj GXEurgb60Hz480IntDf;
extern GXRenderModeObj GXEurgb60Hz480Int;
extern GXRenderModeObj GXEurgb60Hz480IntAa;
extern GXRenderModeObj GXEurgb60Hz480Prog;
extern GXRenderModeObj GXEurgb60Hz480ProgSoft;
extern GXRenderModeObj GXEurgb60Hz480ProgAa;
}

// include/nw4r/g3d/g3d_state.h on all-releases; this branch's base does not have the g3d headers yet
namespace nw4r {
namespace g3d {
namespace G3DState {
void SetRenderModeObj(const GXRenderModeObj& rObj);
}
} // namespace g3d
} // namespace nw4r

// the game's own PAL modes: 480 lines stretched to the full 574, with and without the deflicker filter
static GXRenderModeObj sPal574IntDf = {
	VI_TVMODE_PAL_INT,
	640,
	480,
	574,
	40,
	0,
	640,
	574,
	VI_XFBMODE_DF,
	GX_FALSE,
	GX_FALSE,
	{ { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 } },
	{ 8, 8, 10, 12, 10, 8, 8 },
};

static GXRenderModeObj sPal574Int = {
	VI_TVMODE_PAL_INT,
	640,
	480,
	574,
	40,
	0,
	640,
	574,
	VI_XFBMODE_DF,
	GX_FALSE,
	GX_FALSE,
	{ { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 }, { 6, 6 } },
	{ 0, 0, 21, 22, 21, 0, 0 },
};

// the SDK's modes in the SDK's order; the two after the PAL ones are the game's
static GXRenderModeObj* sRenderModes[] = {
	&GXNtsc240Ds,          &GXNtsc240DsAa,
	&GXNtsc240Int,         &GXNtsc240IntAa,
	&GXNtsc480IntDf,       &GXNtsc480Int,
	&GXNtsc480IntAa,       &GXNtsc480Prog,
	&GXNtsc480ProgSoft,    &GXNtsc480ProgAa,
	&GXMpal240Ds,          &GXMpal240DsAa,
	&GXMpal240Int,         &GXMpal240IntAa,
	&GXMpal480IntDf,       &GXMpal480Int,
	&GXMpal480IntAa,       &GXMpal480Prog,
	&GXMpal480ProgSoft,    &GXMpal480ProgAa,
	&GXPal264Ds,           &GXPal264DsAa,
	&GXPal264Int,          &GXPal264IntAa,
	&GXPal528IntDf,        &GXPal528Int,
	&GXPal524IntAa,        &sPal574IntDf,
	&sPal574Int,           &GXEurgb60Hz240Ds,
	&GXEurgb60Hz240DsAa,   &GXEurgb60Hz240Int,
	&GXEurgb60Hz240IntAa,  &GXEurgb60Hz480IntDf,
	&GXEurgb60Hz480Int,    &GXEurgb60Hz480IntAa,
	&GXEurgb60Hz480Prog,   &GXEurgb60Hz480ProgSoft,
	&GXEurgb60Hz480ProgAa,
};

GXRenderModeObj* CGsScreen::GetRenderMode(s32 mode)
{
	return sRenderModes[mode];
}

void CGsScreen::SetMode(s32 mode, u16 hor, u16 ver)
{
	GXAdjustForOverscan(sRenderModes[mode], this, hor, ver);
	Configure();

	GXColor black = { 0, 0, 0, 0 };
	GXSetCopyClear(black, 0xFFFFFF);
	GXSetCopyClamp(GX_CLAMP_BOTH);
	GXSetDispCopyGamma(GX_GM_1_0);
	GXSetClipMode(GX_CLIP_ENABLE);
	GXSetCopyFilter(aa, sample_pattern, GX_TRUE, vfilter);
	GXSetDispCopySrc(0, 0, fbWidth, efbHeight);
	u32 lines = GXSetDispCopyYScale(GXGetYScaleFactor(efbHeight, xfbHeight));
	GXSetDispCopyDst(fbWidth, lines);
	GXSetDispCopyYScale(GXGetYScaleFactor(efbHeight, xfbHeight));
}

void CGsScreen::Configure()
{
	VIConfigure(this);
	VIFlush();
	nw4r::g3d::G3DState::SetRenderModeObj(*this);
}

u32 CGsScreen::GetXfbSize()
{
	u16 width  = GetXfbWidth();
	u16 height = GetXfbHeight();
	return (u16)((width + 15) & ~15) * height * 2;
}

u16 CGsScreen::GetXfbWidth()
{
	return 640;
}

u16 CGsScreen::GetXfbHeight()
{
	switch (VIGetTvFormat()) {
	case VI_NTSC:
		return 480;
	case VI_PAL:
		return 574;
	case VI_EURGB60:
		return 480;
	case VI_MPAL:
		return 480;
	default:
		OSPanic(__FILE__, 204, "invalid TV format\n");
		return 480;
	}
}

void CGsRenderState::Set(GSBlendMode blendMode, GSZMode zMode, GSAlphaCompare alphaCompare, GSColorUpdate colorUpdate,
                         GSAlphaUpdate alphaUpdate, GSDstAlpha dstAlpha, GSZCompLoc zCompLoc, GSDither dither, GSLineWidth lineWidth,
                         GSPointSize pointSize, GSCullMode cullMode, GSClipMode clipMode)
{
	mBlendMode.Set(blendMode);
	mZMode.Set(zMode);
	mAlphaCompare.Set(alphaCompare);
	mColorUpdate.Set(colorUpdate);
	mAlphaUpdate.Set(alphaUpdate);
	mDstAlpha.Set(dstAlpha);
	mZCompLoc.Set(zCompLoc);
	mDither.Set(dither);
	mLineWidth.Set(lineWidth);
	mPointSize.Set(pointSize);
	mCullMode.Set(cullMode);
	mClipMode.Set(clipMode);
}

void GSBlendMode::Set(GSBlendMode src)
{
	Set(src.mType, src.mSrc, src.mDst, src.mOp);
}

void GSZMode::Set(GSZMode src)
{
	Set(src.mEnable, src.mFunc, src.mUpdate);
}

void GSAlphaCompare::Set(GSAlphaCompare src)
{
	Set(src.mComp0, src.mRef0, src.mOp, src.mComp1, src.mRef1);
}

void GSColorUpdate::Set(GSColorUpdate src)
{
	Set(src.mEnable);
}

void GSAlphaUpdate::Set(GSAlphaUpdate src)
{
	Set(src.mEnable);
}

void GSDstAlpha::Set(GSDstAlpha src)
{
	Set(src.mEnable, src.mAlpha);
}

void GSZCompLoc::Set(GSZCompLoc src)
{
	Set(src.mBeforeTex);
}

void GSDither::Set(GSDither src)
{
	Set(src.mEnable);
}

void GSLineWidth::Set(GSLineWidth src)
{
	Set(src.mWidth, src.mOffset);
}

void GSPointSize::Set(GSPointSize src)
{
	Set(src.mSize, src.mOffset);
}

void GSCullMode::Set(GSCullMode src)
{
	Set(src.mMode);
}

void GSClipMode::Set(GSClipMode src)
{
	Set(src.mMode);
}

void CGsRenderState::Apply()
{
	GXSetBlendMode(mBlendMode.mType, mBlendMode.mSrc, mBlendMode.mDst, mBlendMode.mOp);
	GXSetZMode(mZMode.mEnable, mZMode.mFunc, mZMode.mUpdate);
	GXSetAlphaCompare(mAlphaCompare.mComp0, mAlphaCompare.mRef0, mAlphaCompare.mOp, mAlphaCompare.mComp1, mAlphaCompare.mRef1);
	GXSetColorUpdate(mColorUpdate.mEnable);
	GXSetAlphaUpdate(mAlphaUpdate.mEnable);
	GXSetDstAlpha(mDstAlpha.mEnable, mDstAlpha.mAlpha);
	GXSetZCompLoc(mZCompLoc.mBeforeTex);
	GXSetDither(mDither.mEnable);
	GXSetLineWidth(mLineWidth.mWidth, mLineWidth.mOffset);
	GXSetPointSize(mPointSize.mSize, mPointSize.mOffset);
	GXSetCullMode(mCullMode.mMode);
	GXSetClipMode(mClipMode.mMode);
}
