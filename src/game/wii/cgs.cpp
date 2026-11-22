// The Wii's cgs.cpp from the camera on, .text 0x802A80FC-0x802AEA44: camera, light and fog, the
// included cgs_scrn.cpp and cgs_chan.cpp, then the GameCube cgs.cpp functions in the GameCube file's
// order, plus the Wii's own text helpers.
//
// NONMATCHING: in the frame buffer queue QueueFrameCopy (98 %) emits the identical instruction stream and only colours the new
// entry's address and the popped request into other registers than retail; OnCopyDone (95 %) materialises the Peek() result twice
// in retail (an inline sret slot plus the caller's copy, one copy is elided for us). The perspective CCamera::Init (79 %) reads the
// frame buffer height before the width for the aspect. CLight::GetPosition (83 %) shrink-wraps its callee saves behind the light-kind
// test in retail and CLight::GetViewDirection (93 %): retail zeroes a second, unused Vec on the stack (0x8-0x10) that this source
// form does not produce.
// DrawGsCircle, DrawGsQuad and DrawGsTexturedQuad (99.9 %)
// read the camera's bottom before its top for the flip test where we read top first (an inline for the test makes it worse). GetXfbHeight
// (cgs_scrn.cpp) waits for the two xfb name strings of the frame buffer queue in front of it, which shift its assert in the string pool.
// GetGlyph (99 %) differs only in callee-saved colouring; DrawGsText (92 %) and DrawGsStrokeText (87 %) diverge in float allocation and
// stack frame size (retail spills ~0x10 more, plus two f64 spill slots the allocator gives us as held constants). No behavioural difference
// in any of them. SetGsChannelMaterialAndLighting (93 %): retail colours the light color's components for the color channel in other
// registers and computes the material components where it tests them, not ahead of the tests. CGs::Init (98 %): every call, argument and
// store matches; retail packs the GS* by-value temporaries one slot higher (a 0x78 hole that here holds the second GSTevOrder) and pools
// the tev index and channel ids into .sdata2 where we emit li, so the frame stays 0x270 instead of 0x280.

// the revolution GX header first: the Wii's FIFO getters take the object to fill. GXReadDrawSync gets
// the same treatment, the Dolphin header under it declares it void
#define GXReadDrawSync __GXReadDrawSync_dolphin
#include "revolution/gx.h"
#undef GXReadDrawSync
extern "C" u16 GXReadDrawSync(void);

#include "game/wii/cgs.h"
#include "game/cfont.h"
#include "game/cprimitive.h"
#include "game/wii/cgame.h"
#include <stdarg.h>
#include <stdio.h>
#include "Dolphin/vi.h"

#include "arithmetic.h"
#include "game/gmath.h"
#include "nw4r/g3d/g3d_init.h"
#include <math.h>
#include <cmath>
#include <string.h>
#include "game/cthread.h"
#include "game/mm.h"

extern "C" {
void C_MTXFrustum(Mtx44 mtx, f32 t, f32 b, f32 l, f32 r, f32 n, f32 f);
void PSMTX44Identity(Mtx44 mtx);
void PSMTX44Concat(Mtx44 a, Mtx44 b, Mtx44 ab);
u32 C_MTX44Inverse(Mtx44 src, Mtx44 inv);
void fn_802AEBF0(WiiGsSceneObj** head, u32 renderStates, u32 tevStages, u32 channels, u32 texGens);
void fn_802AEF34(WiiGsSceneObj** head, CCamera* camera, BOOL flag);
CLight* fn_802AF1D0(WiiGsSceneObj** head, CLight* light, MtxP view, BOOL flag);
extern MEMiHeapHead* gWiiMem1Heap;
}

// The camera, light and fog code in front of the screen code, 0x802A80FC-0x802A90C8. It belongs to this
// translation unit (the camera and light vtables sit in its data); which included files it came from is
// not known, none of it asserts.

void CCamera::InitView(const GXRenderModeObj* rmode)
{
	mView.mDirty      = TRUE;
	mView.mEye.x      = 0.0f;
	mView.mEye.y      = 0.0f;
	mView.mEye.z      = 0.0f;
	mView.mInterest.x = 0.0f;
	mView.mInterest.y = 0.0f;
	mView.mInterest.z = 0.0f;
	mView.mAngle.x    = 0.0f;
	mView.mAngle.y    = 0.0f;
	mView.mAngle.z    = 0.0f;
	mView.mUp.x       = 0.0f;
	mView.mUp.y       = 1.0f;
	mView.mUp.z       = 0.0f;
	mView.mDirty      = TRUE;
	PSMTX44Identity(mViewMtx);
	mViewSource = NULL;
	CCamera::vf88(0.0f, 0.0f, rmode->fbWidth, rmode->efbHeight, 0.0f, 1.0f);
	PSMTX44Identity(mViewportMtx);
}

void CCamera::Init(f32 near, f32 far, f32 fov, f32 scale, const GXRenderModeObj* rmode)
{
	mFov              = fov;
	mType             = 0;
	mNear             = near;
	mFar              = far;
	mProjectionDirty  = TRUE;
	mIsOrtho          = 0.0f;
	mProjectionSource = NULL;
	f32 width         = rmode->fbWidth;
	f32 height        = rmode->efbHeight;
	mAspect           = width * scale / height;
	mScale            = scale;
	InitView(rmode);
}

void CCamera::vf10(f32 near, f32 far, f32 top, f32 bottom, f32 left, f32 right, f32 scale, const GXRenderModeObj* rmode)
{
	mType             = 1;
	mTop              = top;
	mBottom           = bottom;
	mLeft             = left * scale;
	mRight            = right * scale;
	mNear             = near;
	mFar              = far;
	mProjectionDirty  = TRUE;
	mIsOrtho          = 0.0f;
	mProjectionSource = NULL;
	mScale            = scale;
	InitView(rmode);
}

void CCamera::Init(f32 top, f32 bottom, f32 left, f32 right, f32 near, f32 far, f32 scale, const GXRenderModeObj* rmode)
{
	mType             = 2;
	mTop              = top;
	mBottom           = bottom;
	mLeft             = left * scale;
	mRight            = right * scale;
	mNear             = near;
	mFar              = far;
	mProjectionDirty  = TRUE;
	mIsOrtho          = 1.0f;
	mProjectionSource = NULL;
	mScale            = scale;
	InitView(rmode);
}

BOOL CCamera::SetCurrent()
{
	BOOL projection;
	if (mProjectionDirty && mProjectionSource == NULL) {
		switch (mType) {
		case 0:
			C_MTXPerspective(mProjection, mFov, mAspect, mNear, mFar);
			break;
		case 1:
			C_MTXFrustum(mProjection, mTop, mBottom, mLeft, mRight, mNear, mFar);
			break;
		case 2:
			C_MTXOrtho(mProjection, mTop, mBottom, mLeft, mRight, mNear, mFar);
			break;
		}
		mProjectionDirty = FALSE;
	} else if (mProjectionSource != NULL) {
		PSMTX44Copy(mProjectionSource, mProjection);
	} else {
		projection = FALSE;
		goto view;
	}
	mProjParams[0] = mProjection[0][0];
	mProjParams[1] = mProjection[0][2];
	mProjParams[2] = mProjection[1][1];
	mProjParams[3] = mProjection[1][2];
	mProjParams[4] = mProjection[2][2];
	mProjParams[5] = mProjection[2][3];
	projection     = TRUE;
view:
	BOOL changed = projection != FALSE;

	BOOL view;
	if (mView.mDirty && mViewSource == NULL) {
		C_MTXLookAt(mViewMtx, &mView.mEye, &mView.mUp, &mView.mInterest);
		mView.mDirty = FALSE;
		view         = TRUE;
	} else if (mViewSource != NULL) {
		PSMTXCopy(mViewSource, mViewMtx);
		view = TRUE;
	} else {
		view = FALSE;
	}
	changed = (changed | view) != FALSE;

	BOOL viewport;
	if (mViewportDirty) {
		mViewportDirty     = FALSE;
		mViewportMtx[0][0] = 0.5f * mViewport[2];
		mViewportMtx[1][1] = 0.5f * -mViewport[3];
		mViewportMtx[0][3] = 0.5f * mViewport[2] + mViewport[0];
		mViewportMtx[1][3] = 0.5f * mViewport[3] + mViewport[1];
		viewport           = TRUE;
	} else {
		viewport = FALSE;
	}

	changed = (changed | viewport) != FALSE;
	if (changed) {
		Mtx44 viewMtx;
		Mtx44 viewportMtx;
		Mtx44 clip;
		PSMTX44Copy(mViewMtx, viewMtx);
		PSMTX44Copy(mViewportMtx, viewportMtx);
		PSMTX44Concat(viewportMtx, mProjection, clip);
		PSMTX44Concat(clip, viewMtx, mScreenMtx);
		C_MTX44Inverse(mScreenMtx, mScreenInv);
	}
	return TRUE;
}

void CCamera::EndCurrent()
{
}

void CCameraView::CalcEye()
{
	f32 y = HALF_PI + mAngle.y;
	f32 x = mAngle.x;

	mEye.x       = mInterest.x + mDistance * (f32)cos(y) * (f32)cos(x);
	f32 cosPitch = (f32)cos(x);
	f32 sinYaw   = (f32)sin(y);
	f32 zOffset  = mDistance * -sinYaw;
	mEye.z       = mInterest.z + zOffset * cosPitch;
	mEye.y       = mInterest.y + mDistance * (f32)sin(x);
	mDirty       = TRUE;
}

// mirrors CalcEye with the pitch pushed past PI
void CCameraView::CalcInterest()
{
	f32 y = HALF_PI + mAngle.y;
	f32 x = PI + mAngle.x;

	mInterest.x  = mEye.x + mDistance * (f32)cos(y) * (f32)cos(x);
	f32 cosPitch = (f32)cos(x);
	f32 sinYaw   = (f32)sin(y);
	f32 zOffset  = mDistance * -sinYaw;
	mInterest.z  = mEye.z + zOffset * cosPitch;
	mInterest.y  = mEye.y + mDistance * (f32)sin(x);
	mDirty       = TRUE;
}

void CCameraView::CalcAngle()
{
	f32 dy    = mInterest.y - mEye.y;
	f32 dz    = mInterest.z - mEye.z;
	f32 dx    = mInterest.x - mEye.x;
	f32 angle = (f32)atan2(FSqrt(dx * dx + dz * dz), FSqrt(dy * dy)); // horizontal run against |rise|

	mAngle.x = dy < 0.0f ? HALF_PI - angle : angle - HALF_PI;
	mAngle.y = (f32)atan2(mInterest.x - mEye.x, mInterest.z - mEye.z);
	mDirty   = TRUE;
}

void CCameraView::CalcUp()
{
	mUp.x  = -std::cosf(mAngle.y) * std::sinf(mAngle.z);
	mUp.z  = std::sinf(mAngle.y) * std::sinf(mAngle.z);
	mUp.y  = std::cosf(mAngle.z);
	mDirty = TRUE;
}

void CCameraView::CalcDistance()
{
	mDistance = PSVECDistance(&mEye, &mInterest);
	mDirty    = TRUE;
}

CMtxP CCamera::GetViewSource()
{
	return mViewSource;
}

void CCamera::SetAngleZ(f32 z)
{
	mView.mAngle.z = z;
	mView.mDirty   = TRUE;
}

MtxP CCamera::GetProjectionSource()
{
	return mProjectionSource;
}

void CCamera::SetProjectionSource(MtxP projection)
{
	mProjectionSource = projection;
	if (projection == NULL) {
		mProjectionDirty = TRUE;
	}
}

void CLight::Reset(u8 r, u8 g, u8 b, u8 a)
{
	memset(this, 0, sizeof(GXLightObj));
	mFlags      = 0;
	mPosition.x = 1.0f;
	mPosition.y = 1.0f;
	mPosition.z = 1.0f;
	mInterest.x = 0.0f;
	mInterest.y = 0.0f;
	mInterest.z = 0.0f;
	mNext       = NULL;
	SetColor(r, g, b, a);
	vf2C();
}

void CLight::SetColor(u8 r, u8 g, u8 b, u8 a)
{
	GXColor color = { r, g, b, a };
	GXInitLightColor(this, color);
}

void CLight::vf2C()
{
	mFlags |= 0x8000;
}

void CLight::InitAmbient(u8 r, u8 g, u8 b, u8 a)
{
	Reset(r, g, b, a);
	mFlags |= 1;
}

void CLight::InitInfinite(const Vec& position, u8 r, u8 g, u8 b, u8 a)
{
	Reset(r, g, b, a);
	mFlags |= 2;
	SetPosition(position);
	GXInitLightAttn(this, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
}

void CLight::SetPosition(const Vec& position)
{
	mPosition.Set(position);
}

void CLight::InitPoint(const Vec& position, u8 r, u8 g, u8 b, u8 a)
{
	Reset(r, g, b, a);
	mFlags |= 4;
	SetPosition(position);
	GXInitLightSpot(this, 0.0f, GX_SP_OFF);
	GXInitLightDistAttn(this, 10000.0f, 0.99f, GX_DA_STEEP);
}

void CLight::InitSpot(const Vec& position, const Vec& interest, u8 r, u8 g, u8 b, u8 a)
{
	Reset(r, g, b, a);
	mFlags |= 8;
	SetPosition(position);
	SetInterest(interest);
	GXInitLightSpot(this, 30.0f, GX_SP_COS2);
	GXInitLightDistAttn(this, 10000.0f, 0.99f, GX_DA_STEEP);
}

void CLight::SetInterest(const Vec& interest)
{
	mInterest.Set(interest);
}

CVec CLight::GetViewPosition(Mtx view)
{
	Vec position = GetPosition();
	PSMTXMultVec(view, &position, &position);
	CVec result;
	result.Set(position.x, position.y, position.z);
	return result;
}

CVec CLight::GetPosition()
{
	if (mFlags & 2) {
		CVec position;
		PSVECNormalize(&mPosition, &position);
		position.x += 1.0e19f;
		position.y += 1.0e19f;
		position.z += 1.0e19f;
		return position;
	}
	return mPosition;
}

CVec CLight::GetViewDirection(Mtx view)
{
	CVec direction;
	if (PSVECMag(&mInterest) != 0.0f) {
		PSMTXMultVecSR(view, &mInterest, &direction);
		PSVECNormalize(&direction, &direction);
	} else {
		Vec zero = { 0.0f, 0.0f, 0.0f };
		direction.Set(zero);
	}
	return direction;
}

CColor CLight::GetColor()
{
	CColor color;
	GXGetLightColor(this, &color);
	return color;
}

void CLight::SetInterest(f32 x, f32 y, f32 z)
{
	mInterest.x = x;
	mInterest.y = y;
	mInterest.z = z;
}

void CLight::InitSpot(const Vec& position, const Vec& interest, u32 color)
{
	InitSpot(position, interest, color >> 24, color >> 16, color >> 8, color);
}

void CLight::InitPoint(const Vec& position, u32 color)
{
	InitPoint(position, color >> 24, color >> 16, color >> 8, color);
}

void CLight::InitInfinite(const Vec& position, u32 color)
{
	InitInfinite(position, color >> 24, color >> 16, color >> 8, color);
}

void CLight::InitAmbient(u32 color)
{
	InitAmbient(color >> 24, color >> 16, color >> 8, color);
}

void CFog::Init(GXFogType type, const GXColor& color, f32 startZ, f32 endZ, f32 nearZ, f32 farZ)
{
	mType   = type;
	mStartZ = startZ;
	mEndZ   = endZ;
	mNearZ  = nearZ;
	mFarZ   = farZ;
	mColor.Set(color);
	GXSetFog(mType, mStartZ, mEndZ, mNearZ, mFarZ, mColor);
}

// The frame buffer queue, 0x802A90C8-0x802A977C. Two external frame buffers take turns: a copy is queued
// with the FIFO position it waits for, a GX breakpoint there tells when the GPU got that far, and the
// retrace callbacks put the finished buffer on screen. Class names from RTTI; the queue entry's is ours.

// a ring of s32 values, its storage on the game heap
class CQueueS32 {
public:
	s32* mBuf;  // _00
	u32 mSize;  // _04
	u32 mHead;  // _08
	u32 mCount; // _0C
	// _10 vtable

	CQueueS32()
	{
		mBuf   = NULL;
		mCount = 0;
		mHead  = 0;
		mSize  = 0;
	}
	virtual ~CQueueS32() { } // _08

	void Init(u32 size)
	{
		mSize = size;
		mBuf  = (s32*)MEMAllocFromExpHeapEx(gWiiGameHeap, size * sizeof(s32), 4);
		for (u32 i = 0; i < mSize; i++) {
			mBuf[i] = 0;
		}
		mCount = 0;
		mHead  = 0;
	}
	// where the next value goes, -1 when the queue is full
	s32 GetTailIndex() const
	{
		if (mCount < mSize) {
			return (mHead + mCount) % mSize;
		}
		return -1;
	}
	void Push(s32 value)
	{
		if (mCount < mSize) {
			mBuf[(mHead + mCount) % mSize] = value;
			mCount++;
		}
	}
	s32 Pop()
	{
		s32 value   = mBuf[mHead];
		mBuf[mHead] = 0;
		mHead       = (mHead + 1) % mSize;
		mCount--;
		return value;
	}
	s32 Peek() const { return mBuf[mHead % mSize]; }
};

// the same ring holding pointers
class CQueuePtr : public CQueueS32 {
public:
	void Push(void* ptr) { CQueueS32::Push((s32)ptr); }
	void* Pop() { return (void*)CQueueS32::Pop(); }
	void* Peek() const { return (void*)CQueueS32::Peek(); }
};

namespace _fb {
// one display copy: the FIFO write position it waits for and the buffer it copies into
struct CopyRequest {
	void* mFifoPos; // _00
	u32 _04;        // _04, always 0
	void* mXfb;     // _08
};

// a ring of copy requests that keeps the requests themselves
class Queue : public CQueuePtr {
public:
	CopyRequest mEntries[5]; // _14

	void Push(const CopyRequest& request)
	{
		s32 index       = GetTailIndex();
		mEntries[index] = request;
		CQueuePtr::Push(&mEntries[index]);
	}
	CopyRequest Pop() { return *(CopyRequest*)CQueuePtr::Pop(); }
	CopyRequest Peek() const { return *(CopyRequest*)CQueuePtr::Peek(); }
};
} // namespace _fb

// an OSThreadQueue with a vtable
class CThreadQueue : public OSThreadQueue {
public:
	// _08 vtable

	virtual ~CThreadQueue() { } // _08
	virtual void Init();        // _0C
};

static _fb::Queue sCopyQueue; // the copies the GPU has not reached yet
static _fb::Queue sDoneQueue; // the copies it finished, drained by the frame thread
static CThreadQueue sFrameQueue;
static CThread sFrameThread;
static void* sXfb[2];
static void* sCopyXfb;    // the buffer the next copy goes to
static void* sDisplayXfb; // the buffer on screen
static u8 sBreakPtSet;
static u8 sCopyPending; // the breakpoint fired for the buffer on screen; finish it after the retrace
static u8 sRetraced;
static u16 sDoneToken;
static u16 sToken = 1;

static void OnCopyDone();
static void OnBreakPt();
static void OnPreRetrace(u32 count);
static void OnPostRetrace(u32 count);
static void* FrameThreadProc(void* arg);

void AllocFrameBuffers(u32 size)
{
	sXfb[0]     = mm::Alloc(size, "(xfb_1)");
	sXfb[1]     = mm::Alloc(size, "(xfb_2)");
	sCopyXfb    = sXfb[1];
	sDisplayXfb = sXfb[0];
	sCopyQueue.Init(5);
	sDoneQueue.Init(5);
	OSInitThreadQueue(&sFrameQueue);
}

void CThreadQueue::Init()
{
	OSInitThreadQueue(this);
}

void StartFrameThread()
{
	sFrameThread.Create(FrameThreadProc, NULL, 0x1000, 14, OS_THREAD_ATTR_DETACH);
	OSResumeThread(&sFrameThread.mThread);
	VISetPreRetraceCallback(OnPreRetrace);
	VISetPostRetraceCallback(OnPostRetrace);
	GXSetBreakPtCallback(OnBreakPt);
	GXCopyDisp(sCopyXfb, GX_TRUE);
}

void QueueFrameCopy()
{
	GXFifoObj fifo;
	void* readPtr;
	void* writePtr;

	GXFlush();
	GXGetCPUFifo(&fifo);
	GXGetFifoPtrs(&fifo, &readPtr, &writePtr);
	void* fifoPos = writePtr;
	void* xfb     = sCopyXfb;
	BOOL enabled  = OSDisableInterrupts();
	_fb::CopyRequest request;
	request.mFifoPos = fifoPos;
	request._04      = 0;
	request.mXfb     = xfb;
	sCopyQueue.Push(request);
	OSRestoreInterrupts(enabled);
	if (!sBreakPtSet) {
		sBreakPtSet = TRUE;
		GXEnableBreakPt(writePtr);
	}
	GXSetDrawSync(sToken);
	GXCopyDisp(sCopyXfb, GX_TRUE);
	GXFlush();
}

void SwapFrameBuffer()
{
	sToken++;
	sCopyXfb     = sCopyXfb == sXfb[0] ? sXfb[1] : sXfb[0];
	BOOL enabled = OSDisableInterrupts();
	if (sCopyQueue.mCount > 1) {
		OSSleepThread(&sFrameQueue);
	}
	OSRestoreInterrupts(enabled);
}

static void OnCopyDone()
{
	sDoneQueue.Push(sCopyQueue.Pop());
	OSWakeupThread(&sFrameQueue);
	if (sCopyQueue.mCount == 0) {
		GXDisableBreakPt();
		sBreakPtSet = FALSE;
	} else {
		GXEnableBreakPt(sCopyQueue.Peek().mFifoPos);
	}
}

static void OnBreakPt()
{
	if (sCopyQueue.Peek().mXfb == sDisplayXfb) {
		sCopyPending = TRUE;
	} else {
		OnCopyDone();
	}
}

static void OnPreRetrace(u32 count)
{
	u16 token = GXReadDrawSync();
	if (token == (u16)(sDoneToken + 1)) {
		sDoneToken  = token;
		sDisplayXfb = sDisplayXfb == sXfb[0] ? sXfb[1] : sXfb[0];
		VISetNextFrameBuffer(sDisplayXfb);
		VIFlush();
		sRetraced = TRUE;
	}
}

static void OnPostRetrace(u32 count)
{
	if (sCopyPending && sRetraced) {
		OnCopyDone();
		sCopyPending = FALSE;
		sRetraced    = FALSE;
	}
}

static void* FrameThreadProc(void* arg)
{
	for (;;) {
		OSSleepThread(&sFrameQueue);
		sDoneQueue.CQueueS32::Pop();
	}
}

// The screen, render state, tev stage and channel code sits in front of this file's own and is part
// of the same translation unit: the asserts of cgs_scrn.cpp and cgs_chan.cpp share one string pool.
#include "cgs_scrn.cpp"
#include "cgs_chan.cpp"

// sends whatever render, tev and channel state changed since the last draw
static inline void SetupDrawState(CGs* gs)
{
	if (gs->mCurRenderState != NULL) {
		gs->mCurRenderState->Apply();
	}
	if (gs->mCurTevStage != NULL) {
		gs->mCurTevStage->Apply();
	}
	if (gs->mCurChannel != NULL) {
		gs->mCurChannel->Apply();
	}
}

// one character out of a text string, \C and \B followed by eight hex digits set the text
// and background colors and give no character
struct TextGlyph {
	char mStr[4]; // _00, one or two bytes and a terminator
	u32 mColor;   // _04
	u32 mBgColor; // _08
};

// one character's sprite and its background tile, Init allocates 256 of them
struct TextChar {
	CTexObj mTex;    // _000
	CSprite mSprite; // _104
	CTile mBg;       // _28C
	u32 mHasBg;      // _410
};
STATIC_ASSERT(sizeof(TextChar) == 0x414);

static inline void SetBillboard(CPrimitive* prim, BOOL on)
{
	if (on) {
		prim->mBillboard = 1;
	} else {
		prim->mBillboard = 0;
	}
	prim->mDirty = TRUE;
}

// where the quad sits in its own unit space and its four corner colors
static inline void SetCenter(CSprite* sprite, f32 x, f32 y, f32 z)
{
	sprite->mCenter.x = x;
	sprite->mCenter.y = y;
	sprite->mCenter.z = z;
}

static inline void SetCenter(CTile* tile, f32 x, f32 y, f32 z)
{
	tile->mCenter.x = x;
	tile->mCenter.y = y;
	tile->mCenter.z = z;
}

static inline void SetColor(CSprite* sprite, u32 color)
{
	sprite->mColor0 = sprite->mColor1 = sprite->mColor2 = sprite->mColor3 = color;
}

static inline void SetColor(CTile* tile, u32 color)
{
	tile->mColor0 = tile->mColor1 = tile->mColor2 = tile->mColor3 = color;
}

void CGs::Init(u32 renderStates, u32 tevStages, u32 channels, u32 texGens)
{
	mFifoBuffer = MEMAllocFromExpHeapEx(gWiiMem1Heap, 0x80000, 4);
	mFifo       = GXInit(mFifoBuffer, 0x80000);
	mCamera     = NULL;
	mLight      = NULL;

	SetRenderState(NULL, GSBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET), GSZMode(GX_FALSE, GX_LEQUAL, GX_FALSE),
	               GSAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_LEQUAL, 0xFF), GSColorUpdate(GX_TRUE), GSAlphaUpdate(GX_FALSE),
	               GSDstAlpha(GX_FALSE, 0), GSZCompLoc(GX_TRUE), GSDither(GX_TRUE), GSLineWidth(12, GX_TO_ONE), GSPointSize(24, GX_TO_ZERO),
	               GSCullMode(GX_CULL_BACK), GSClipMode(GX_CLIP_ENABLE));

	SetTevStage(NULL, 0, GSTevOrder(GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0),
	            GSTevColorOp(GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV),
	            GSTevColorIn(GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO),
	            GSTevAlphaOp(GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV),
	            GSTevAlphaIn(GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO));
	SetTevStage(&mTevStages[1], 1, GSTevOrder(GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR1A1),
	            GSTevColorOp(GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV),
	            GSTevColorIn(GX_CC_ZERO, GX_CC_RASC, GX_CC_ONE, GX_CC_CPREV),
	            GSTevAlphaOp(GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV),
	            GSTevAlphaIn(GX_CA_ZERO, GX_CA_RASA, GX_CA_KONST, GX_CA_APREV));

	SetChannel(NULL, GX_COLOR0, GSChanCtrl(GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_SPOT), (GXColor) { 0, 0, 0, 0xFF },
	           (GXColor) { 0xFF, 0xFF, 0xFF, 0xFF });
	SetChannel(NULL, GX_ALPHA0, GSChanCtrl(GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE, GX_AF_NONE), (GXColor) { 0, 0, 0, 0xFF },
	           (GXColor) { 0xFF, 0xFF, 0xFF, 0xFF });
	SetChannel(NULL, GX_COLOR1, GSChanCtrl(GX_TRUE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE, GX_AF_SPEC), (GXColor) { 0, 0, 0, 0xFF },
	           (GXColor) { 0xFF, 0xFF, 0xFF, 0xFF });
	SetChannel(NULL, GX_ALPHA1, GSChanCtrl(GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE, GX_AF_NONE), (GXColor) { 0, 0, 0, 0xFF },
	           (GXColor) { 0xFF, 0xFF, 0xFF, 0xFF });

	mBillboardEnabled = FALSE;
	mSprite.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	mSprite.mTex = &mTex;
	mPoint.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	mLine.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	mTriangle.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	mCircline.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	mCircle.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	mTile.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	mSprite2.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	mCube.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	mOctahedron.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	mDodeca.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	mIcosahedron.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	mSphere.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	mCylinder.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	mTorus.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	mRotOrder = 5;

	mTextChars            = (TextChar*)MEMAllocFromExpHeapEx(gWiiGameHeap, 0x41400, 4);
	mShiftJisWidthScale   = 1.0f;
	mSingleByteWidthScale = 1.0f;
	Vec zero;
	zero.x    = 0.0f;
	zero.y    = 0.0f;
	zero.z    = 0.0f;
	mTextRot  = zero;
	mTextFont = 0;

	nw4r::g3d::G3dInit(false);
	fn_802AEBF0(&mScene.mHead, renderStates, tevStages, channels, texGens);
}

void CGs::SetScreenMode(s32 mode, u16 hor, u16 ver)
{
	mScreen.SetMode(mode, hor, ver);
	VISetBlack(TRUE);
}

void CGs::InitFrameBuffers()
{
	AllocFrameBuffers(mScreen.GetXfbSize());
}

void CGs::StartFrameThread()
{
	::StartFrameThread();
}

void CGs::SetRenderState(CGsRenderState* state, GSBlendMode blend, GSZMode zMode, GSAlphaCompare alphaCompare, GSColorUpdate colorUpdate,
                         GSAlphaUpdate alphaUpdate, GSDstAlpha dstAlpha, GSZCompLoc zCompLoc, GSDither dither, GSLineWidth lineWidth,
                         GSPointSize pointSize, GSCullMode cullMode, GSClipMode clipMode)
{
	if (state != NULL) {
		mCurRenderState = state;
	} else {
		mCurRenderState = &mRenderState;
	}
	mCurRenderState->Set(blend, zMode, alphaCompare, colorUpdate, alphaUpdate, dstAlpha, zCompLoc, dither, lineWidth, pointSize, cullMode,
	                     clipMode);
}

void CGs::SetTevStage(CGsTevStage* stage, s32 index, GSTevOrder order, GSTevColorOp colorOp, GSTevColorIn colorIn, GSTevAlphaOp alphaOp,
                      GSTevAlphaIn alphaIn)
{
	if (stage != NULL) {
		mCurTevStage = stage;
	} else {
		mCurTevStage = &mTevStages[0];
	}
	mCurTevStage->Set((GXTevStageID)index, order, colorOp, colorIn, alphaOp, alphaIn);
}

void CGs::SetChannel(CGsChannel* channel, GXChannelID id, GSChanCtrl ctrl, GXColor ambient, GXColor material)
{
	if (channel != NULL) {
		mCurChannel = channel;
		mCurChannel->Set(id, ctrl, ambient, material);
		return;
	}
	switch (id) {
	case GX_COLOR0:
	case GX_COLOR1:
	case GX_ALPHA0:
	case GX_ALPHA1:
		mChannels[id].Set(id, ctrl, ambient, material);
		break;
	case GX_COLOR0A0:
		mChannels[0].Set(GX_COLOR0, ctrl, ambient, material);
		mChannels[2].Set(GX_ALPHA0, ctrl, ambient, material);
		break;
	case GX_COLOR1A1:
		mChannels[1].Set(GX_COLOR1, ctrl, ambient, material);
		mChannels[3].Set(GX_ALPHA1, ctrl, ambient, material);
		break;
	}
	// the color channels chain to their alpha halves
	mCurChannel = &mChannels[0];
	mCurChannel->SetNext(&mChannels[2]);
}

extern "C" void SetGsChannelEnabled(CGs* gs, s32 chan, u8 enable)
{
	if (gs->IsOwnChannel()) {
		switch (chan) {
		case GX_COLOR0:
		case GX_ALPHA0:
			gs->mChannels[chan].mCtrl.mEnable = enable;
			break;
		case GX_COLOR0A0:
			gs->mChannels[0].mCtrl.mEnable = enable;
			gs->mChannels[2].mCtrl.mEnable = enable;
			break;
		}
	} else {
		gs->mCurChannel->mCtrl.mEnable = enable;
	}
}

BOOL CGs::BeginCamera()
{
	if (mCamera->SetCurrent()) {
		fn_802AEF34(&mScene.mHead, mCamera, FALSE);
		return TRUE;
	}
	return FALSE;
}

void CGs::fn_802AB81C(MtxP view, BOOL flag)
{
	mLight = fn_802AF1D0(&mScene.mHead, mLight, view, flag);
}

extern "C" void EndGsCamera(CGs* gs)
{
	gs->mCamera->EndCurrent();
	gs->mCamera = NULL;
}

// the one call CGame::Draw makes between RenderSceneShadow and SetCameraLight is a no-op on the
// Wii; on the GameCube that slot is HSD_StartRender(HSD_RP_SCREEN)
void CGs::fn_802AB860()
{
}

void CGs::ResetZMode()
{
	mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	mCurRenderState->SetColorUpdate(GX_TRUE);
}

void CGs::EndFrame()
{
	QueueFrameCopy();
	VIWaitForRetrace();
}

void CGs::SwapBuffers()
{
	SwapFrameBuffer();
}

// the own color channel (and the alpha one behind it) takes matSrc and rgba as its material; when
// it is lit, the ambient lights of the list give its ambient color and the enabled lights other
// than ambient its light mask
extern "C" s32 SetGsChannelMaterialAndLighting(CGs* gs, s32 matSrc, u32 rgba)
{
	CGsChannel* color0 = &gs->mChannels[0];
	if (gs->mCurChannel == color0) {
		color0->mCtrl.mMatSrc          = (GXColorSrc)matSrc;
		gs->mChannels[2].mCtrl.mMatSrc = (GXColorSrc)matSrc;
		if (color0->mCtrl.mEnable) {
			if (color0->mCtrl.mAmbSrc == GX_SRC_REG) {
				for (CLight* light = gs->mLight; light != NULL; light = light->mNext) {
					if ((u8)light->mFlags == 1) {
						const GXColor& ambient = light->GetColor();
						color0->mAmbColor.Set(ambient);
						gs->mChannels[2].mAmbColor.Set(ambient);
					}
				}
			}
			u8 r = rgba >> 24;
			u8 g = rgba >> 16;
			u8 b = rgba >> 8;
			u8 a = rgba;
			color0->mMatColor.Set(r, g, b, a);
			gs->mChannels[2].mMatColor.Set(r, g, b, a);
			u32 mask = 0;
			int n    = 0;
			for (CLight* light = gs->mLight; light != NULL; light = light->mNext) {
				if ((light->mFlags & 0x8000) && (u8)light->mFlags != 1) {
					mask |= 1 << n++;
				}
			}
			color0->mCtrl.mLightMask          = mask;
			gs->mChannels[2].mCtrl.mLightMask = mask;
		} else {
			u8 r = rgba >> 24;
			u8 g = rgba >> 16;
			u8 b = rgba >> 8;
			u8 a = rgba;
			color0->mMatColor.Set(r, g, b, a);
			color0->mCtrl.mLightMask = 0;
			gs->mChannels[2].mMatColor.Set(r, g, b, a);
			gs->mChannels[2].mCtrl.mLightMask = 0;
		}
	}
	return 0;
}

extern "C" {
u16 GXGetTexObjWidth(const GXTexObj* obj);
u16 GXGetTexObjHeight(const GXTexObj* obj);
}

// places a flat shape on screen, w and h are its full size. an ortho camera has y growing
// down the screen, so the shape gets flipped back
static inline void SetFlatTransform(CGs* gs, CPrimitive* prim, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx, f32 ry, f32 rz)
{
	if (gs->mCamera->mTop < gs->mCamera->mBottom) {
		prim->SetTrans(x, y, z);
		prim->SetScale(w / 2.0f, -h / 2.0f, 1.0f);
		prim->SetRot(-rx, ry, -rz);
	} else {
		prim->SetTrans(x, y, z);
		prim->SetScale(w / 2.0f, h / 2.0f, 1.0f);
		prim->SetRot(rx, ry, rz);
	}
	if (gs->mBillboardEnabled) {
		prim->mBillboard  = 2;
		prim->mBillboardZ = rz;
	} else {
		prim->mBillboard = 0;
	}
	prim->mDirty    = TRUE;
	prim->mRotOrder = gs->mRotOrder;
}

extern "C" {
void DrawGsLineSegment(CGs* gs, Mtx view, f32 x0, f32 y0, f32 z0, u32 color0, f32 x1, f32 y1, f32 z1, u32 color1)
{
	// no matrix means the camera's
	MtxP mtx;
	if (view == NULL) {
		mtx = gs->mCamera->mViewMtx;
	} else {
		mtx = view;
	}
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);
	gs->mLine.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	PrimVertex vtx[2] = {
		{ { x0, y0, z0 }, color0 },
		{ { x1, y1, z1 }, color1 },
	};
	gs->mLine.Draw(mtx, vtx, 2);
}

void DrawGsLineStrip(CGs* gs, Vec* pos, u32 color, u16 count)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);
	gs->mLine.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->mLine.Draw(gs->mCamera->mViewMtx, pos, color, count);
}

void DrawGsColoredLineStrip(CGs* gs, Vec* pos, u32* color, u16 count)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);
	gs->mLine.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->mLine.Draw(gs->mCamera->mViewMtx, pos, color, count);
}

void DrawGsTrianglePrimitive(CGs* gs, GXPrimitive type, Mtx view, Vec* pos, u32 color, u16 count)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);
	gs->mTriangle.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	if (gs->IsOwnChannel() && gs->mChannels[0].mCtrl.mEnable) {
		gs->mTriangle.DrawLit(view, pos, color, count, type);
	} else {
		gs->mTriangle.Draw(view, pos, color, count, type);
	}
}

void DrawGsColoredTrianglePrimitive(CGs* gs, GXPrimitive type, Mtx view, Vec* pos, u32* color, u16 count)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);
	gs->mTriangle.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	if (gs->IsOwnChannel() && gs->mChannels[0].mCtrl.mEnable) {
		gs->mTriangle.DrawLit(view, pos, color, count, type);
	} else {
		gs->mTriangle.Draw(view, pos, color, count, type);
	}
}

void DrawGsTriangleVertices(CGs* gs, GXPrimitive type, Mtx view, PrimVertex* vtx, u16 count)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);
	gs->mTriangle.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	if (gs->IsOwnChannel() && gs->mChannels[0].mCtrl.mEnable) {
		gs->mTriangle.DrawLit(view, vtx, count, type);
	} else {
		gs->mTriangle.Draw(view, vtx, count, type);
	}
}

void DrawGsVertexQuad(CGs* gs, f32 x0, f32 y0, f32 z0, u32 color0, f32 x1, f32 y1, f32 z1, u32 color1, f32 x2, f32 y2, f32 z2, u32 color2,
                      f32 x3, f32 y3, f32 z3, u32 color3)
{
	PrimVertex vtx[4] = {
		{ { x0, y0, z0 }, color0 },
		{ { x1, y1, z1 }, color1 },
		{ { x2, y2, z2 }, color2 },
		{ { x3, y3, z3 }, color3 },
	};
	DrawGsTriangleVertices(gs, GX_TRIANGLEFAN, gs->mCamera->mViewMtx, vtx, 4);
}

void DrawGsCircle(CGs* gs, u32 segments, u32 centerColor, u32 edgeColor, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx, f32 ry, f32 rz)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);
	SetFlatTransform(gs, &gs->mCircle, x, y, z, w, h, rx, ry, rz);
	gs->mCircle.mFixedMtx = NULL;
	if (gs->IsOwnChannel() && gs->mChannels[0].mCtrl.mEnable) {
		gs->mCircle.DrawLit(gs->mCamera->mViewMtx, segments, centerColor, edgeColor);
	} else {
		gs->mCircle.Draw(gs->mCamera->mViewMtx, segments, centerColor, edgeColor);
	}
}

void DrawGsQuad(CGs* gs, u32 color0, u32 color1, u32 color2, u32 color3, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx, f32 ry, f32 rz)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);
	SetFlatTransform(gs, &gs->mTile, x, y, z, w, h, rx, ry, rz);
	gs->mTile.mFixedMtx = NULL;
	if (gs->IsOwnChannel() && gs->mChannels[0].mCtrl.mEnable) {
		gs->mTile.DrawLit(gs->mCamera->mViewMtx, color0, color1, color2, color3);
	} else {
		gs->mTile.Draw(gs->mCamera->mViewMtx, color0, color1, color2, color3);
	}
}

void DrawGsTexturedQuad(CGs* gs, CTexObj* tex, u32 color0, u32 color1, u32 color2, u32 color3, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx,
                        f32 ry, f32 rz)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	if (gs->IsOwnTevStage()) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		gs->mCurTevStage->SetNumTexGens(1);
		gs->mCurTevStage->SetTexGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);
	}
	SetupDrawState(gs);

	// negative size means use the texture's own
	if (w < 0.0f) {
		u16 size = GXGetTexObjWidth(tex);
		w        = size;
	}
	if (h < 0.0f) {
		u16 size = GXGetTexObjHeight(tex);
		h        = size;
	}

	SetFlatTransform(gs, &gs->mSprite2, x, y, z, w, h, rx, ry, rz);
	gs->mSprite2.mTex      = tex;
	gs->mSprite2.mFixedMtx = NULL;
	if (gs->IsOwnChannel() && gs->mChannels[0].mCtrl.mEnable) {
		gs->mSprite2.DrawLit(gs->mCamera->mViewMtx, GX_TEXMAP0, GX_TEXMTX0, GX_MTX2x4, color0, color1, color2, color3);
	} else {
		gs->mSprite2.Draw(gs->mCamera->mViewMtx, GX_TEXMAP0, GX_TEXMTX0, GX_MTX2x4, color0, color1, color2, color3);
	}

	if (gs->IsOwnTevStage()) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		gs->mCurTevStage->SetNumTexGens(0);
	}
}

void DrawGsCube(CGs* gs, u32 color, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_REG, color);
	SetupDrawState(gs);
	gs->mCube.SetTrans(tx, ty, tz);
	gs->mCube.SetScale(sx, sy, sz);
	gs->mCube.SetRot(rx, ry, rz);
	gs->mCube.mRotOrder = gs->mRotOrder;
	gs->mCube.mFixedMtx = NULL;
	gs->mCube.Draw(gs->mCamera->mViewMtx);
}

void DrawGsSphere(CGs* gs, u32 color, u8 major, u8 minor, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_REG, color);
	SetupDrawState(gs);
	gs->mSphere.SetTrans(tx, ty, tz);
	gs->mSphere.SetScale(sx, sy, sz);
	gs->mSphere.SetRot(rx, ry, rz);
	gs->mSphere.mRotOrder = gs->mRotOrder;
	gs->mSphere.mFixedMtx = NULL;
	gs->mSphere.Draw(gs->mCamera->mViewMtx, major, minor);
}

void DrawGsSphereWithMatrix(CGs* gs, Mtx mtx, u32 color, u8 major, u8 minor)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_REG, color);
	SetupDrawState(gs);
	gs->mSphere.mFixedMtx = mtx;
	gs->mSphere.Draw(gs->mCamera->mViewMtx, major, minor);
}

void DrawGsCylinder(CGs* gs, u32 color, u8 sides, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_REG, color);
	SetupDrawState(gs);
	gs->mCylinder.SetTrans(tx, ty, tz);
	gs->mCylinder.SetScale(sx, sy, sz);
	gs->mCylinder.SetRot(rx, ry, rz);
	gs->mCylinder.mRotOrder = gs->mRotOrder;
	gs->mCylinder.mFixedMtx = NULL;
	gs->mCylinder.Draw(gs->mCamera->mViewMtx, sides);
}

void DrawGsCylinderWithMatrix(CGs* gs, Mtx mtx, u32 color, u8 sides)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_REG, color);
	SetupDrawState(gs);
	gs->mCylinder.mFixedMtx = mtx;
	gs->mCylinder.Draw(gs->mCamera->mViewMtx, sides);
}
}

/**
 * @note Address: 0x802AD1C0
 * @note Size: 0x150
 */
// fills glyph with the next character, returns where the one after starts or null at the end
extern "C" {
const char* GetGlyph(CGs* gs, const char* text, TextGlyph* glyph)
{
	switch (*text) {
	case '\0':
		return NULL;
	case '\n':
		glyph->mStr[0] = '\n';
		break;
	case '\\':
		text++;
		switch (*text) {
		case 'C':
		case 'c':
			glyph->mColor = ParseHexDigits(text + 1, 8);
			text += 8;
			glyph->mStr[0] = '\0';
			break;
		case 'B':
		case 'b':
			glyph->mBgColor = ParseHexDigits(text + 1, 8);
			text += 8;
			glyph->mStr[0] = '\0';
			break;
		default:
			glyph->mStr[0] = *--text;
			glyph->mStr[1] = '\0';
			break;
		}
		break;
	default:
		if (gs->mUseShiftJis && ((u8)*text >= 0x81 && (u8)*text <= 0x9F || (u8)*text >= 0xE0 && (u8)*text <= 0xFC)) {
			glyph->mStr[0] = *text;
			glyph->mStr[1] = *++text;
		} else {
			glyph->mStr[0] = *text;
			glyph->mStr[1] = '\0';
		}
		break;
	}
	return text + 1;
}
}

/**
 * @note Address: 0x802AD310
 * @note Size: 0x220
 */
// the widest line of the text, in the units the draw uses
extern "C" f32 MeasureGameTextWidth(CGs* gs, const char* text, BOOL proportional, f32 size)
{
	CFont font;
	s32 charWidth;
	f32 width[2] = { 0.0f, 0.0f };
	TextGlyph glyph;

	while ((text = GetGlyph(gs, text, &glyph)) != NULL) {
		if (glyph.mStr[0] == '\0') {
			continue;
		}
		if (glyph.mStr[0] == '\n') {
			width[0] = 0.0f;
			continue;
		}
		GetFontGlyphTexture(&font, glyph.mStr, gs->mTextFont, &charWidth);
		if (size > 0.0f) {
			if (proportional) {
				width[0] += charWidth * (size / 24.0f);
			} else if (gs->mUseShiftJis) {
				if ((u8)glyph.mStr[0] >= 0x81 && (u8)glyph.mStr[0] <= 0x9F || (u8)glyph.mStr[0] >= 0xE0 && (u8)glyph.mStr[0] <= 0xFC) {
					width[0] += size * gs->mShiftJisWidthScale;
				} else {
					width[0] += 0.7f * size * gs->mShiftJisWidthScale;
				}
			} else {
				width[0] += 0.7f * size * gs->mSingleByteWidthScale;
			}
		} else {
			width[0] += charWidth;
		}
		if (width[0] > width[1]) {
			width[1] = width[0];
		}
	}
	return width[1];
}

/**
 * @note Address: 0x802AD530
 * @note Size: 0xBC
 */
// one line plus one per newline, a non-positive size falls back to the default cell
extern "C" f32 MeasureGameTextHeight(CGs* gs, const char* text, f32 size)
{
	TextGlyph glyph;
	f32 height = size > 0.0f ? size : 24.0f;
	while ((text = GetGlyph(gs, text, &glyph)) != NULL) {
		if (glyph.mStr[0] == '\n') {
			height += size > 0.0f ? size : 24.0f;
		}
	}
	return height;
}

/**
 * @note Address: 0x802AD5EC
 * @note Size: 0xF8
 */
// printf into a buffer and draw it with each character as wide as its glyph
extern "C" void DrawGsProportionalTextFormat(CGs* gs, u32 color, const char* format, f32 x, f32 y, f32 z, f32 size, f32 height, ...)
{
	char buf[256];
	va_list args;
	va_start(args, height);
	vsprintf(buf, format, args);
	DrawGsText(gs, color, TRUE, buf, x, y, z, size, height);
}

/**
 * @note Address: 0x802AD6E4
 * @note Size: 0xF8
 */
// printf into a buffer and draw it with fixed width characters
extern "C" void DrawGsFixedWidthTextFormat(CGs* gs, u32 color, const char* format, f32 x, f32 y, f32 z, f32 size, f32 height, ...)
{
	char buf[256];
	va_list args;
	va_start(args, height);
	vsprintf(buf, format, args);
	DrawGsText(gs, color, FALSE, buf, x, y, z, size, height);
}

/**
 * @note Address: 0x802AD7DC
 * @note Size: 0x828
 */
// draws text with one sprite per character and a background tile behind each when the
// background color isn't zero
extern "C" void DrawGsText(CGs* gs, u32 color, BOOL proportional, const char* text, f32 x, f32 y, f32 z, f32 size, f32 height)
{
	f32 offsX;
	f32 curX = x;
	f32 curY = y;
	TextGlyph glyph;

	// an ortho camera has y growing down the screen, so the line step flips
	f32 lineHeight = gs->mCamera->mTop < gs->mCamera->mBottom ? -height : height;
	glyph.mColor   = color;
	glyph.mBgColor = 0;

	f32 textWidth = MeasureGameTextWidth(gs, text, proportional, size);

	f32 centerX = x + textWidth / 2.0f;
	f32 centerY = y - lineHeight;
	f32 scaleX  = size * 0.5f;
	f32 scaleY  = lineHeight * 0.5f;
	s32 count   = 0;
	while ((text = GetGlyph(gs, text, &glyph)) != NULL) {
		if (glyph.mStr[0] == '\0') {
			continue;
		}
		if (glyph.mStr[0] == '\n') {
			curX = x;
			curY += lineHeight;
			continue;
		}

		TextChar* chr     = &gs->mTextChars[count];
		chr->mSprite.mTex = &chr->mTex;
		s32 charWidth     = chr->mSprite.mTex->SetChar(glyph.mStr, gs->mTextFont);
		BOOL wide = (u8)glyph.mStr[0] >= 0x81 && (u8)glyph.mStr[0] <= 0x9F || (u8)glyph.mStr[0] >= 0xE0 && (u8)glyph.mStr[0] <= 0xFC;
		f32 advance;
		if (proportional) {
			// the glyph's left bearing shifts it inside its cell
			CFont font;
			f32 bearing = HasFontGlyph(&font, glyph.mStr);
			curX += bearing * (size / 24.0f);
			charWidth -= bearing;
			advance = charWidth * (size / 24.0f);
		} else if (gs->mUseShiftJis) {
			if (wide) {
				advance = size * gs->mShiftJisWidthScale;
			} else {
				advance = size * (0.7f * gs->mShiftJisWidthScale);
			}
		} else {
			advance = size * (0.7f * gs->mSingleByteWidthScale);
		}

		offsX = curX - x;
		f32 u = 2.0f * (offsX / size) + 1.0f - textWidth / size;
		f32 v = 2.0f * (-(curY - y) / lineHeight) + 1.0f;
		if (!proportional) {
			// a character narrower than its cell gets centered in it
			u += (size - charWidth) / 2.0f / size;
		}

		chr->mSprite.SetTrans(centerX, centerY, z);
		chr->mSprite.SetScale(scaleX, scaleY, 1.0f);
		chr->mSprite.SetRot(gs->mTextRot.x, gs->mTextRot.y, gs->mTextRot.z);
		SetCenter(&chr->mSprite, u, v, 0.0f);
		SetColor(&chr->mSprite, glyph.mColor);
		SetBillboard(&chr->mSprite, gs->mBillboardEnabled);

		if (chr->mHasBg = glyph.mBgColor != 0) {
			chr->mBg.SetTrans(centerX, centerY, z);
			chr->mBg.SetScale(advance / 2.0f, scaleY, 1.0f);
			chr->mBg.SetRot(gs->mTextRot.x, gs->mTextRot.y, gs->mTextRot.z);
			SetCenter(&chr->mBg, 2.0f * (offsX / advance) + 1.0f - textWidth / advance, v, 0.0f);
			SetColor(&chr->mBg, glyph.mBgColor);
			SetBillboard(&chr->mBg, gs->mBillboardEnabled);
		}
		curX += advance;
		count++;
	}

	TextChar* textChars = gs->mTextChars;

	// backgrounds first, flat color
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	if (gs->IsOwnTevStage()) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		gs->mCurTevStage->SetMode(GX_PASSCLR);
		gs->mCurTevStage->SetNumTexGens(0);
	}
	SetupDrawState(gs);
	if (gs->IsOwnChannel() && gs->mChannels[0].mCtrl.mEnable) {
		for (s32 i = 0; i < count; i++) {
			if (textChars[i].mHasBg) {
				textChars[i].mBg.DrawLit(gs->mCamera->mViewMtx, textChars[i].mBg.mColor0, textChars[i].mBg.mColor1,
				                         textChars[i].mBg.mColor2, textChars[i].mBg.mColor3);
			}
		}
	} else {
		for (s32 i = 0; i < count; i++) {
			if (textChars[i].mHasBg) {
				textChars[i].mBg.Draw(gs->mCamera->mViewMtx, textChars[i].mBg.mColor0, textChars[i].mBg.mColor1, textChars[i].mBg.mColor2,
				                      textChars[i].mBg.mColor3);
			}
		}
	}

	// then the characters, textured
	if (gs->IsOwnTevStage()) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		gs->mCurTevStage->SetMode(GX_MODULATE);
		gs->mCurTevStage->SetNumTexGens(1);
		gs->mCurTevStage->SetTexGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);
	}
	SetupDrawState(gs);
	if (gs->IsOwnChannel() && gs->mChannels[0].mCtrl.mEnable) {
		for (s32 i = 0; i < count; i++) {
			textChars[i].mSprite.DrawLit(gs->mCamera->mViewMtx, GX_TEXMAP0, GX_TEXMTX0, GX_MTX2x4, textChars[i].mSprite.mColor0,
			                             textChars[i].mSprite.mColor1, textChars[i].mSprite.mColor2, textChars[i].mSprite.mColor3);
		}
	} else {
		for (s32 i = 0; i < count; i++) {
			textChars[i].mSprite.Draw(gs->mCamera->mViewMtx, GX_TEXMAP0, GX_TEXMTX0, GX_MTX2x4, textChars[i].mSprite.mColor0,
			                          textChars[i].mSprite.mColor1, textChars[i].mSprite.mColor2, textChars[i].mSprite.mColor3);
		}
	}

	if (gs->IsOwnTevStage()) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		gs->mCurTevStage->SetMode(GX_PASSCLR);
		gs->mCurTevStage->SetNumTexGens(0);
	}
}

/**
 * @note Address: 0x802AE004
 * @note Size: 0xE0
 */
// the longest run of drawable characters between newlines, scaled when a size is given
extern "C" f32 MeasureStrokeTextWidth(CGs* gs, const char* text, f32 size)
{
	f32 len[2]    = { 0.0f, 0.0f };
	const char* p = text;
	TextGlyph glyph;
	while ((p = GetGlyph(gs, p, &glyph)) != NULL) {
		if (glyph.mStr[0] != '\0') {
			if (glyph.mStr[0] == '\n') {
				len[0] = 0.0f;
			} else {
				len[0] += 1.0f;
				if (len[0] > len[1]) {
					len[1] = len[0];
				}
			}
		}
	}
	return size > 0.0f ? len[1] * size : len[1];
}

/**
 * @note Address: 0x802AE0E4
 * @note Size: 0xF4
 */
extern "C" void DrawGsStrokeTextFormat(CGs* gs, u32 color, const char* fmt, f32 x, f32 y, f32 z, f32 w, f32 h, ...)
{
	char buf[256];
	va_list args;
	va_start(args, h);
	vsprintf(buf, fmt, args);
	DrawGsStrokeText(gs, color, buf, x, y, z, w, h);
}

// per glyph the stroke segments it draws, each byte pair a from,to point with x in the high
// nibble and y (0 at the glyph's top) in the low one, 0xFF ends the row. rows 0-9 are the
// digits, 10-35 the letters and 36-46 the punctuation DrawGsStrokeText looks up
u8 gGlyphStrokeTable[47][13] = {
	{ 0x00, 0x60, 0x60, 0x66, 0x66, 0x06, 0x06, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x30, 0x36, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x60, 0x60, 0x63, 0x63, 0x03, 0x03, 0x06, 0x06, 0x66, 0xFF, 0x00, 0x00 },
	{ 0x00, 0x60, 0x60, 0x66, 0x66, 0x06, 0x13, 0x63, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x03, 0x03, 0x63, 0x40, 0x46, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0x00, 0x00, 0x03, 0x03, 0x63, 0x63, 0x66, 0x66, 0x06, 0xFF, 0x00, 0x00 },
	{ 0x00, 0x06, 0x06, 0x66, 0x66, 0x63, 0x63, 0x03, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x60, 0x60, 0x62, 0x62, 0x43, 0x43, 0x46, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x60, 0x60, 0x66, 0x66, 0x06, 0x06, 0x00, 0x03, 0x63, 0xFF, 0x00, 0x00 },
	{ 0x63, 0x03, 0x03, 0x00, 0x00, 0x60, 0x60, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x06, 0x30, 0x30, 0x66, 0x14, 0x54, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x40, 0x40, 0x61, 0x61, 0x66, 0x66, 0x06, 0x06, 0x00, 0x03, 0x63, 0xFF },
	{ 0x60, 0x20, 0x20, 0x03, 0x03, 0x26, 0x26, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x30, 0x30, 0x63, 0x63, 0x36, 0x36, 0x06, 0x10, 0x16, 0xFF, 0x00, 0x00 },
	{ 0x60, 0x00, 0x00, 0x06, 0x06, 0x66, 0x03, 0x53, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0x00, 0x00, 0x06, 0x02, 0x42, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0x30, 0x30, 0x02, 0x02, 0x06, 0x06, 0x66, 0x66, 0x63, 0x63, 0x33, 0xFF },
	{ 0x00, 0x06, 0x60, 0x66, 0x03, 0x63, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x10, 0x50, 0x16, 0x56, 0x30, 0x36, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x50, 0x56, 0x56, 0x16, 0x16, 0x14, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x06, 0x60, 0x03, 0x03, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x06, 0x06, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x06, 0x00, 0x00, 0x33, 0x33, 0x60, 0x60, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x06, 0x00, 0x00, 0x66, 0x66, 0x60, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x30, 0x63, 0x63, 0x36, 0x36, 0x03, 0x03, 0x30, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x06, 0x00, 0x00, 0x50, 0x50, 0x61, 0x61, 0x62, 0x62, 0x53, 0x53, 0x03, 0xFF },
	{ 0x06, 0x02, 0x02, 0x60, 0x60, 0x64, 0x64, 0x06, 0x24, 0x46, 0x46, 0x66, 0xFF },
	{ 0x06, 0x00, 0x00, 0x60, 0x60, 0x62, 0x62, 0x02, 0x32, 0x66, 0xFF, 0x00, 0x00 },
	{ 0x60, 0x00, 0x00, 0x66, 0x66, 0x06, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x60, 0x30, 0x36, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x06, 0x06, 0x66, 0x66, 0x60, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x02, 0x02, 0x36, 0x36, 0x62, 0x62, 0x60, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x16, 0x16, 0x56, 0x56, 0x60, 0x32, 0x36, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x66, 0x60, 0x06, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x32, 0x32, 0x60, 0x32, 0x36, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x60, 0x60, 0x06, 0x06, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x03, 0x63, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x06, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x03, 0x22, 0x22, 0x44, 0x44, 0x63, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x63, 0x63, 0x06, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x50, 0x16, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x40, 0x22, 0x22, 0x24, 0x24, 0x46, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x20, 0x42, 0x42, 0x44, 0x44, 0x26, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x44, 0x26, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0x03, 0x03, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
};

static inline void DebugVertex(f32 x, f32 y, u32 color)
{
	f32 depth = 0.0f;
	GXPosition3f32(x, y, depth);
	GXColor1u32(color);
}

// a stroke point is on a grid of 0.11 steps inside the character cell
static inline f32 StrokePos(s32 n)
{
	f32 pos = 0.11f * n;
	return 0.2f + pos;
}

/**
 * @note Address: 0x802AE1D8
 * @note Size: 0x86C
 */
extern "C" void DrawGsStrokeText(CGs* gs, u32 color, const char* text, f32 x, f32 y, f32 z, f32 w, f32 h)
{
	if (gs->mCurRenderState == &gs->mRenderState) {
		gs->mRenderState.SetLineWidth(6, GX_TO_ONE);
		gs->mRenderState.SetPointSize(12, GX_TO_ONE);
	}
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	SetupDrawState(gs);

	f32 width = MeasureStrokeTextWidth(gs, text, 0.0f);

	Mtx trans;
	Mtx rotX;
	Mtx rotY;
	Mtx rotZ;
	Mtx scale;
	Mtx mtx;
	PSMTXTrans(trans, x + width * w * 0.5f, y, z);
	PSMTXRotRad(rotX, 'x', gs->mTextRot.x);
	PSMTXRotRad(rotY, 'y', gs->mTextRot.y);
	PSMTXRotRad(rotZ, 'z', gs->mTextRot.z);
	if (gs->mCamera->mBottom > gs->mCamera->mTop) {
		PSMTXScale(scale, w, h, 1.0f);
	} else {
		PSMTXScale(scale, w, -h, 1.0f);
	}
	PSMTXConcat(trans, rotX, mtx);
	PSMTXConcat(mtx, rotY, mtx);
	PSMTXConcat(mtx, rotZ, mtx);
	PSMTXConcat(mtx, scale, mtx);
	PSMTXConcat(gs->mCamera->mViewMtx, mtx, mtx);

	f32 startX;
	f32 curY;
	f32 unit;
	if (gs->mBillboardEnabled) {
		BuildBillboardMatrixFromUp(mtx, mtx);
		startX = width * 0.5f;
		curY   = 0.0f;
		unit   = -1.0f;
	} else {
		startX = -width * 0.5f;
		curY   = 0.0f;
		unit   = 1.0f;
	}
	f32 curX = startX;
	GXLoadPosMtxImm(mtx, 0);
	GXSetCurrentMtx(0);

	// position xyz f32 and color rgba8, both direct
	GXVtxAttrFmtList fmts[] = {
		{ GX_VA_POS, GX_POS_XYZ, GX_F32, 0 },
		{ GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 },
		{ GX_VA_NULL, GX_POS_XY, GX_U8, 0 },
	};
	GXVtxDescList descs[] = {
		{ GX_VA_POS, GX_DIRECT },
		{ GX_VA_CLR0, GX_DIRECT },
		{ GX_VA_NULL, GX_NONE },
	};
	GXClearVtxDesc();
	GXSetVtxAttrFmtv(GX_VTXFMT0, fmts);
	GXSetVtxDescv(descs);

	// second pass draws, each character is one unit wide in the scaled space
	TextGlyph glyph;
	glyph.mColor   = color;
	glyph.mBgColor = 0;
	while ((text = GetGlyph(gs, text, &glyph)) != NULL) {
		if (glyph.mStr[0] == 0) {
			continue;
		}
		if (glyph.mStr[0] == '\n') {
			curX = startX;
			curY += 1.0f;
			continue;
		}
		if (glyph.mBgColor != 0) {
			GXBegin(GX_QUADS, GX_VTXFMT0, 4);
			DebugVertex(curX, curY, glyph.mBgColor);
			DebugVertex(curX + unit, curY, glyph.mBgColor);
			DebugVertex(curX + unit, 1.0f + curY, glyph.mBgColor);
			DebugVertex(curX, 1.0f + curY, glyph.mBgColor);
		}
		s32 index = -1;
		switch (glyph.mStr[0]) {
		case '.':
			GXBegin(GX_POINTS, GX_VTXFMT0, 1);
			DebugVertex(curX + 0.25f * unit, 0.84999996f + curY, glyph.mColor);
			break;
		case ':':
			GXBegin(GX_POINTS, GX_VTXFMT0, 2);
			DebugVertex(curX + 0.35000002f * unit, 0.35000002f + curY, glyph.mColor);
			DebugVertex(curX + 0.35000002f * unit, 0.75f + curY, glyph.mColor);
			break;
		case '%':
			GXBegin(GX_POINTS, GX_VTXFMT0, 2);
			DebugVertex(curX + 0.35000002f * unit, 0.35000002f + curY, glyph.mColor);
			DebugVertex(curX + 0.75f * unit, 0.75f + curY, glyph.mColor);
			GXBegin(GX_LINES, GX_VTXFMT0, 2);
			DebugVertex(curX + 0.95f * unit, 0.15f + curY, glyph.mColor);
			DebugVertex(curX + 0.15f * unit, 0.95f + curY, glyph.mColor);
			break;
		case '-':
			index = 36;
			break;
		case '_':
			index = 37;
			break;
		case '~':
			index = 38;
			break;
		case '>':
			index = 39;
			break;
		case '/':
			index = 40;
			break;
		case '(':
			index = 41;
			break;
		case ')':
			index = 42;
			break;
		case ',':
			index = 43;
			break;
		case '<':
			index = 44;
			break;
		case '+':
			index = 45;
			break;
		case '$':
			index = 46;
			break;
		default:
			if (glyph.mStr[0] >= '0' && glyph.mStr[0] <= '9') {
				index = glyph.mStr[0] - '0';
			} else if (glyph.mStr[0] >= 'A' && glyph.mStr[0] <= 'Z') {
				index = glyph.mStr[0] - 'A' + 10;
			} else if (glyph.mStr[0] >= 'a' && glyph.mStr[0] <= 'z') {
				index = glyph.mStr[0] - 'a' + 10;
			}
			break;
		}
		if (index >= 0) {
			u8* stroke = gGlyphStrokeTable[index];
			// 41 is way past the 13 bytes a glyph has, the 0xFF always comes first
			for (s32 i = 0; i < 41 && *stroke != 0xFF;) {
				u8 from = stroke[0];
				u8 to   = stroke[1];
				i += 2;
				stroke += 2;
				GXBegin(GX_LINES, GX_VTXFMT0, 2);
				DebugVertex(curX + unit * StrokePos(from >> 4), curY + StrokePos(from & 0xF), glyph.mColor);
				DebugVertex(curX + unit * StrokePos(to >> 4), curY + StrokePos(to & 0xF), glyph.mColor);
			}
		}
		curX += unit;
	}
}
