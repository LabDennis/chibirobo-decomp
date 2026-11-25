#include "game/cgameutil.h"
#include <math.h>

namespace {

const u32 WindowFill = 0xff000000;

struct WindowEdges {
	f32 left;
	f32 top;
	f32 right;
	f32 bottom;
};

struct FrameTile {
	f32 texS;
	f32 texT;
	f32 texScaleS;
	f32 texScaleT;
	f32 x;
	f32 y;
	f32 width;
	f32 height;
};

CTexObj& WindowFrameTexture()
{
	CTpl& tpl           = gGame.mTpl;
	TPLPalette* palette = static_cast<TPLPalette*>(tpl.mBuffer);
	return tpl.mTex.Init(&palette->descriptorArray[158 - StatusTextureOffset], 0);
}

void DrawQuad(CGs& gs, CTexObj& tex, u32 color, f32 x, f32 y, f32 width, f32 height)
{
	DrawGsTexturedQuad(&gs, &tex, color, color, color, color, x, y, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
}

void DrawCorner(CGs& gs, u32 color, f32 x, f32 y, f32 depth, f32 width, f32 height)
{
	DrawGsCircle(&gs, 16, color, color, x, y, depth, width, height, 0.0f, 0.0f, 0.0f);
}

WindowEdges GetWindowEdges(f32 x, f32 y, f32 halfWidth, f32 halfHeight)
{
	WindowEdges edges = { x - halfWidth, y - halfHeight, x + halfWidth, y + halfHeight };
	return edges;
}

WindowEdges GetCornerCenters(const WindowEdges& edges, f32 inset, f32 corner)
{
	WindowEdges centers = {
		corner + (inset + edges.left),
		corner + (inset + edges.top),
		(edges.right - inset) - corner,
		(edges.bottom - inset) - corner,
	};
	return centers;
}

void DrawCorners(CGs& gs, const WindowEdges& centers, u32 color, f32 depth, f32 diameter)
{
	DrawCorner(gs, color, centers.left, centers.top, depth, diameter, diameter);
	DrawCorner(gs, color, centers.right, centers.top, depth, diameter, diameter);
	DrawCorner(gs, color, centers.left, centers.bottom, depth, diameter, diameter);
	DrawCorner(gs, color, centers.right, centers.bottom, depth, diameter, diameter);
}

void DrawWindowImage(CGs& gs, FbTexture& texture, u32 alpha, f32 x, f32 y, f32 width, f32 height)
{
	SetGsDepthMode(gs.mCurRenderMode, 1, 2, 0);
	SetGsTevMode(gs.mCurTevStage, 0);
	if (texture.mImage != NULL) {
		DrawQuad(gs, texture.mTex, 0xffffff00 | alpha, x, y, width, height);
	}
	SetGsTevMode(gs.mCurTevStage, 4);
	SetGsDepthMode(gs.mCurRenderMode, 1, 3, 0);
}

void DrawCircularWindow(CGs& gs, FbSubwin& window, f32 fade, u32 alpha)
{
	CTexObj frame = WindowFrameTexture();
	f32 width     = fade * GXGetTexObjWidth(&frame);
	f32 height    = fade * GXGetTexObjHeight(&frame);
	f32 aspect    = static_cast<f32>(window.mTexture.mWidth) / window.mTexture.mHeight;
	const f32 x   = 160.0f;
	const f32 y   = 80.0f;
	DrawCorner(gs, WindowFill, x, y, 0.0f, width, height);
	DrawWindowImage(gs, window.mTexture, alpha, x, y, width * aspect, height);
	SetGsTevMode(gs.mCurTevStage, 0);
	DrawQuad(gs, frame, 0xc8c8c800 | alpha, x, y, 1.1f * width, 1.1f * height);
	SetGsTevMode(gs.mCurTevStage, 4);
}

void DrawRectangularWindow(CGs& gs, FbSubwin& window, f32 fade, f32 amount, u32 alpha)
{
	f32 width           = window.mRectWidth * fade;
	f32 height          = window.mRectHeight * fade;
	f32 corner          = 24.0f * fade;
	f32 diameter        = 2.0f * corner;
	f32 x               = window.mRectX;
	f32 y               = window.mRectY;
	f32 halfWidth       = width * 0.5f;
	f32 halfHeight      = height * 0.5f;
	WindowEdges edges   = GetWindowEdges(x, y, halfWidth, halfHeight);
	WindowEdges centers = GetCornerCenters(edges, fade, corner);
	f32 left            = fade + edges.left;
	f32 top             = fade + edges.top;
	f32 right           = edges.right - fade;
	f32 bottom          = edges.bottom - fade;
	Vec points[]        = {
        { x, y, 0.0f },
        { left, centers.top, 0.0f },
        { centers.left, top, 0.0f },
        { centers.right, top, 0.0f },
        { right, centers.top, 0.0f },
        { right, centers.bottom, 0.0f },
        { centers.right, bottom, 0.0f },
        { centers.left, bottom, 0.0f },
        { left, centers.bottom, 0.0f },
        { left, centers.top, 0.0f },
	};
	DrawGsTrianglePrimitive(&gs, GX_TRIANGLEFAN, gs.mCamera->mViewMtx, points, WindowFill, ARRAY_SIZE(points));
	SetGsDepthMode(gs.mCurRenderMode, 1, 1, 1);
	DrawCorners(gs, centers, WindowFill, 0.0f, diameter);

	const f32 shadowX     = 6.0f;
	const f32 shadowY     = 4.0f;
	const f32 shadowDepth = -0.1f;
	u32 shadow            = static_cast<u32>(96.0f * amount);
	for (u32 i = 0; i < ARRAY_SIZE(points); ++i) {
		points[i].x += shadowX;
		points[i].y += shadowY;
		points[i].z = shadowDepth;
	}
	DrawGsTrianglePrimitive(&gs, GX_TRIANGLEFAN, gs.mCamera->mViewMtx, points, shadow, ARRAY_SIZE(points));
	WindowEdges shadowEdges = GetWindowEdges(x + shadowX, y + shadowY, halfWidth, halfHeight);
	DrawCorners(gs, GetCornerCenters(shadowEdges, fade, corner), shadow, shadowDepth, diameter);
	DrawWindowImage(gs, window.mTexture, alpha, x, y, width, height);

	CTexObj frame           = WindowFrameTexture();
	u16 texWidth            = GXGetTexObjWidth(&frame);
	u16 texHeight           = GXGetTexObjHeight(&frame);
	f32 texelWidth          = 1.0f / texWidth;
	f32 texelHeight         = 1.0f / texHeight;
	f32 halfCorner          = corner * 0.5f;
	f32 leftTile            = edges.left + halfCorner;
	f32 rightTile           = edges.right - halfCorner;
	f32 topTile             = edges.top + halfCorner;
	f32 bottomTile          = edges.bottom - halfCorner;
	f32 horizontalSpan      = width - diameter;
	f32 verticalSpan        = height - diameter;
	const FrameTile tiles[] = {
		{ 0.0f, 0.0f, 0.5f, 0.5f, leftTile, topTile, corner, corner },
		{ 0.5f, 0.0f, texelWidth, 0.5f, x, topTile, horizontalSpan, corner },
		{ 0.5f, 0.0f, 0.5f, 0.5f, rightTile, topTile, corner, corner },
		{ 0.0f, 0.5f, 0.5f, texelHeight, leftTile, y, corner, verticalSpan },
		{ 0.5f, 0.5f, 0.5f, texelHeight, rightTile, y, corner, verticalSpan },
		{ 0.0f, 0.5f, 0.5f, 0.5f, leftTile, bottomTile, corner, corner },
		{ 0.5f, 0.5f, texelWidth, 0.5f, x, bottomTile, horizontalSpan, corner },
		{ 0.5f, 0.5f, 0.5f, 0.5f, rightTile, bottomTile, corner, corner },
	};
	SetGsTevMode(gs.mCurTevStage, 0);
	for (u32 i = 0; i < ARRAY_SIZE(tiles); ++i) {
		const FrameTile& tile = tiles[i];
		PSMTXTrans(frame.mMtx[0], tile.texS, tile.texT, 0.0f);
		PSMTXScale(frame.mMtx[1], tile.texScaleS, tile.texScaleT, 1.0f);
		frame.mMtxDirty = TRUE;
		DrawQuad(gs, frame, 0xffffff00 | alpha, tile.x, tile.y, tile.width, tile.height);
	}
	SetGsTevMode(gs.mCurTevStage, 4);
}

} // namespace

/**
 * @note Address: 0x800137C0
 * @note Size: 0xF10
 */
void FbSubwin::Draw()
{
	if ((mFlags.mFlags & 4) || mAmount == 0.0f) {
		return;
	}

	CGs& gs = gGame.mGs;
	SetGsChannelEnabled(&gs, 4, 0);
	SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
	SetGsAlphaCompare(gs.mCurRenderMode, 7, 0, 0, 7, 255);
	SetGsDepthMode(gs.mCurRenderMode, 1, 7, 1);
	SetGsColorUpdate(gs.mCurRenderMode, 1);
	SetGsAlphaUpdate(gs.mCurRenderMode, 0);
	SetupAll(TRUE);

	const f32 amount = mAmount;
	f32 fade         = sin(0.5f * (PI * amount));
	u32 alpha        = static_cast<u32>(255.0f * amount);
	if (fpclassify(mRectWidth) == FP_NAN || fpclassify(mRectHeight) == FP_NAN) {
		DrawCircularWindow(gs, *this, fade, alpha);
	} else {
		DrawRectangularWindow(gs, *this, fade, amount, alpha);
	}

	SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
	SetGsAlphaCompare(gs.mCurRenderMode, 4, 0, 0, 3, 255);
	SetGsDepthMode(gs.mCurRenderMode, 1, 3, 0);
}
