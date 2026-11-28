#include "types.h"
#include "game/ccameraaction.h"
#include "game/wii/cgame.h"
#include "game/cthread.h"
#include "game/wii/cpadact.h"
#include "game/wii/csoundfx.h"
#include "revolution/mem.h"

// NONMATCHING: data: retail's assertion file string is "SoundArchivePlayer.h"; __FILE__ gives the
// SDK header's name, "snd_SoundArchivePlayer.h"
// The Wii tail of the game's main translation unit (0x8002CFB8-0x800354DC): ~CGame, ~CPadEx, the
// weak destructors of CGame/CGs/CSound members emitted behind them, CInterfaceCameraAction and
// CCameraActionAreaExM methods, the vector and scalar interpolation methods (in reverse GameCube
// order), gGame and the other globals, weak virtuals, the static initializer, and the member
// constructors it emits.

extern MEMiHeapHead* gWiiMem1Heap;
extern MEMiHeapHead* gWiiGameHeap;

// operator delete has an inline body on the Wii, so the deleting destructors free to the MEM1 heap
inline void operator delete(void* ptr) throw()
{
	MEMFreeToExpHeap(gWiiMem1Heap, ptr);
}

/**
 * @note Address: 0x8002CFB8
 * @note Size: 0x264
 */
CGame::~CGame()
{
}

/**
 * @note Address: 0x80030A54
 * @note Size: 0x6C
 */
CPadEx::~CPadEx()
{
}

/**
 * @note Address: 0x80031E30
 * @note Size: 0x8
 */
f32 CInterfaceCameraAction::GetFollowDivisor()
{
	return 12.0f;
}

/**
 * @note Address: 0x80031F04
 * @note Size: 0x28
 */
BOOL CCameraActionAreaExM::IsAreaEnabled(s32 index)
{
	bool set = CamAreaInfo(&gCameraAreaState.mAreaExRecords[index]).IsDisabled();
	if (set) {
		return FALSE;
	}
	return TRUE;
}

/**
 * @note Address: 0x80031F98
 * @note Size: 0x2C
 */
BOOL CCameraActionAreaExM::IsCameraArea(s32 index)
{
	bool set = CamAreaInfo(&gCameraAreaState.mAreaRecords[index]).IsCameraArea();
	if (set) {
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80031FC4
 * @note Size: 0x44
 */
s32 CCameraActionAreaExM::FindAreaContainingPoint(Vec pos)
{
	return FindFirstCameraAreaContainingPoint(&gCameraAreaState, pos);
}

/**
 * @note Address: 0x80032008
 * @note Size: 0x10
 */
void CCameraActionAreaExM::SetZoomLevel(s32 mode)
{
	gCameraAreaState.mZoomLevel = mode;
}

/**
 * @note Address: 0x80032018
 * @note Size: 0x10
 */
s32 CCameraActionAreaExM::GetZoomLevel()
{
	return gCameraAreaState.mZoomLevel;
}

/**
 * @note Address: 0x80032028
 * @note Size: 0x10
 */
f32 CCameraActionAreaExM::GetBaseFov()
{
	return gCameraAreaState.mBaseFov;
}

/**
 * @note Address: 0x80032038
 * @note Size: 0xB8
 */
Vec CVectorEaseOutInterpolation::Get()
{
	return CVec(mAxes[0].Get(), mAxes[1].Get(), mAxes[2].Get());
}

/**
 * @note Address: 0x800320F0
 * @note Size: 0xB4
 */
bool CVectorEaseOutInterpolation::Step(f32 dt)
{
	bool x = mAxes[0].Step(dt);
	bool y = mAxes[1].Step(dt);
	bool z = mAxes[2].Step(dt);
	return x && y && z;
}

/**
 * @note Address: 0x800321A4
 * @note Size: 0xC8
 */
bool CVectorEaseOutInterpolation::Update(u32 step, f32 dt)
{
	bool x = mAxes[0].Update(step, dt);
	bool y = mAxes[1].Update(step, dt);
	bool z = mAxes[2].Update(step, dt);
	return x && y && z;
}

/**
 * @note Address: 0x8003226C
 * @note Size: 0xCC
 */
bool CVectorEaseOutInterpolation::Start(Vec from, Vec to, f32 time)
{
	if (time <= 0.0f) {
		return false;
	}
	mAxes[0].Start(from.x, to.x, time);
	mAxes[1].Start(from.y, to.y, time);
	mAxes[2].Start(from.z, to.z, time);
	return true;
}

/**
 * @note Address: 0x80032338
 * @note Size: 0xB8
 */
Vec CVectorEaseInOutInterpolation::Get()
{
	return CVec(mAxes[0].Get(), mAxes[1].Get(), mAxes[2].Get());
}

/**
 * @note Address: 0x800323F0
 * @note Size: 0xB4
 */
bool CVectorEaseInOutInterpolation::Step(f32 dt)
{
	bool x = mAxes[0].Step(dt);
	bool y = mAxes[1].Step(dt);
	bool z = mAxes[2].Step(dt);
	return x && y && z;
}

/**
 * @note Address: 0x800324A4
 * @note Size: 0xE0
 */
bool CVectorEaseInOutInterpolation::Update(u32 step, f32 dt)
{
	bool x = mAxes[0].Update(step, dt);
	bool y = mAxes[1].Update(step, dt);
	bool z = mAxes[2].Update(step, dt);
	if (x && z) { // x and z have both arrived
		mXZDone = TRUE;
	}
	return x && y && z;
}

/**
 * @note Address: 0x80032584
 * @note Size: 0xD4
 */
bool CVectorEaseInOutInterpolation::Start(Vec from, Vec to, f32 time)
{
	if (time <= 0.0f) {
		return false;
	}
	mAxes[0].Start(from.x, to.x, time);
	mAxes[1].Start(from.y, to.y, time);
	mAxes[2].Start(from.z, to.z, time);
	mXZDone = FALSE;
	return true;
}

/**
 * @note Address: 0x80032658
 * @note Size: 0xB8
 */
Vec CVectorLinearInterpolation::Get()
{
	return CVec(mAxes[0].Get(), mAxes[1].Get(), mAxes[2].Get());
}

/**
 * @note Address: 0x80032710
 * @note Size: 0x8
 */
f32 CInterpolationLinear::Get()
{
	return mValue;
}

/**
 * @note Address: 0x80032718
 * @note Size: 0x10
 */
bool CVectorLinearInterpolation::Update(u32 step, f32 dt)
{
	return Step(dt);
}

/**
 * @note Address: 0x80032728
 * @note Size: 0xB4
 */
bool CVectorLinearInterpolation::Step(f32 dt)
{
	bool x = mAxes[0].Step(dt);
	bool y = mAxes[1].Step(dt);
	bool z = mAxes[2].Step(dt);
	return x && y && z;
}

/**
 * @note Address: 0x800327DC
 * @note Size: 0x34
 */
bool CInterpolationLinear::Step(f32 dt)
{
	mTime -= dt;
	mValue += mSpeed * dt;
	return mTime <= 0.0f;
}

/**
 * @note Address: 0x80032810
 * @note Size: 0xC8
 */
bool CVectorLinearInterpolation::Start(Vec from, Vec to, f32 time)
{
	if (time < 0.0f) {
		return false;
	}
	mAxes[0].Start(from.x, to.x, time);
	mAxes[1].Start(from.y, to.y, time);
	mAxes[2].Start(from.z, to.z, time);
	return true;
}

/**
 * @note Address: 0x800328D8
 * @note Size: 0x30
 */
bool CInterpolationLinear::Start(f32 from, f32 to, f32 time)
{
	if (time < 0.0f) { // a negative time means no interp
		return false;
	}
	mTime  = time;
	mValue = from;
	mSpeed = (to - from) / time;
	return true;
}

/**
 * @note Address: 0x80032908
 * @note Size: 0xB8
 */
Vec CVectorNoneInterpolation::Get()
{
	return CVec(mAxes[0].Get(), mAxes[1].Get(), mAxes[2].Get());
}

/**
 * @note Address: 0x800329C0
 * @note Size: 0x8
 */
f32 CInterpolationNone::Get()
{
	return mValue;
}

/**
 * @note Address: 0x800329C8
 * @note Size: 0x10
 */
bool CVectorNoneInterpolation::Update(u32 step, f32 dt)
{
	return Step(dt);
}

/**
 * @note Address: 0x800329D8
 * @note Size: 0xB4
 */
bool CVectorNoneInterpolation::Step(f32 dt)
{
	bool x = mAxes[0].Step(dt);
	bool y = mAxes[1].Step(dt);
	bool z = mAxes[2].Step(dt);
	return x && y && z;
}

/**
 * @note Address: 0x80032A8C
 * @note Size: 0x20
 */
bool CInterpolationNone::Step(f32 dt)
{
	mTime = 1.0f;
	return mTime <= 0.0f;
}

/**
 * @note Address: 0x80032AAC
 * @note Size: 0xC8
 */
bool CVectorNoneInterpolation::Start(Vec from, Vec to, f32 time)
{
	if (time < 0.0f) {
		return false;
	}
	mAxes[0].Start(from.x, to.x, time);
	mAxes[1].Start(from.y, to.y, time);
	mAxes[2].Start(from.z, to.z, time);
	return true;
}

/**
 * @note Address: 0x80032B74
 * @note Size: 0x1C
 */
bool CInterpolationNone::Start(f32 from, f32 to, f32 time)
{
	mTime  = 1.0f;
	mValue = to;
	mSpeed = 0.0f;
	return true;
}

/**
 * @note Address: 0x80032B90
 * @note Size: 0xB8
 */
Vec CVectorEaseOutDivideInterpolation::Get()
{
	return CVec(mAxes[0].Get(), mAxes[1].Get(), mAxes[2].Get());
}

/**
 * @note Address: 0x80032C48
 * @note Size: 0x8
 */
f32 CInterpolationEaseOutDivide::Get()
{
	return mValue;
}

/**
 * @note Address: 0x80032C50
 * @note Size: 0xB4
 */
bool CVectorEaseOutDivideInterpolation::Step(f32 dt)
{
	bool x = mAxes[0].Step(dt);
	bool y = mAxes[1].Step(dt);
	bool z = mAxes[2].Step(dt);
	return x && y && z;
}

/**
 * @note Address: 0x80032D04
 * @note Size: 0x50
 */
bool CInterpolationEaseOutDivide::Step(f32 dt)
{
	f32 last = mValue;
	if (mDivisor > 0.0f) {
		mValue = (last * (mDivisor - 1.0f) + mTarget) / mDivisor;
	}
	return (f32)fabs(mValue - last) < 0.001f;
}

/**
 * @note Address: 0x80032D54
 * @note Size: 0xE0
 */
bool CVectorEaseOutDivideInterpolation::Update(u32 step, f32 dt)
{
	bool x = mAxes[0].Update(step, dt);
	bool y = mAxes[1].Update(step, dt);
	bool z = mAxes[2].Update(step, dt);
	if (x && z) { // x and z have both arrived
		mXZDone = TRUE;
	}
	return x && y && z;
}

/**
 * @note Address: 0x80032E34
 * @note Size: 0x80
 */
bool CInterpolationEaseOutDivide::Update(u32 step, f32 dt)
{
	bool done = false;
	for (u32 i = 0; i < step; i++) {
		done = Step(dt);
		if (done == true) {
			break;
		}
	}
	return done;
}

/**
 * @note Address: 0x80032EB4
 * @note Size: 0xD4
 */
bool CVectorEaseOutDivideInterpolation::Start(Vec from, Vec to, f32 divisor)
{
	if (divisor <= 0.0f) {
		return false;
	}
	mAxes[0].Start(from.x, to.x, divisor);
	mAxes[1].Start(from.y, to.y, divisor);
	mAxes[2].Start(from.z, to.z, divisor);
	mXZDone = FALSE;
	return true;
}

/**
 * @note Address: 0x80032F88
 * @note Size: 0x2C
 */
bool CInterpolationEaseOutDivide::Start(f32 from, f32 to, f32 divisor)
{
	if (divisor <= 0.0f) {
		return false;
	}
	mValue   = from;
	mTarget  = to;
	mDivisor = divisor;
	return true;
}

/**
 * @note Address: 0x80032FB4
 * @note Size: 0x10
 */
bool CInterpolationLinear::Update(u32 step, f32 dt)
{
	return Step(dt);
}

/**
 * @note Address: 0x80032FC4
 * @note Size: 0x10
 */
bool CInterpolationNone::Update(u32 step, f32 dt)
{
	return Step(dt);
}

// NONMATCHING: the static initializer (0x800332D8, 0x394) is the compiler's. It constructs gGame
// ahead of these two and sets gCmdLineArgs' crosshair (retail _54 and _58) to the middle of the
// pointer's screen. gGame's member-constructor emissions follow at 0x8003366C in this same unit.
CGame gGame;
CPadAct gWiiPointer;
extern "C" {
CThread gMovieUpdateThread;
}

// The weak virtuals of this unit that are written in their classes, and so are emitted by the file
// that constructs gGame (CThread::Create, 0x800331D4, among them):
//   0x80033080 CScrnWipeTile::Start                     include/game/cscrn.h
//   0x80033140 CSoundFx::Setup                          include/game/wii/csoundfx.h
//   0x80033148 nw4r::snd::SoundArchivePlayer::detail_ConvertLabelStringToSoundId
//   0x800331B0 nw4r::snd::DvdSoundArchive::detail_GetWaveDataFileAddress
//   0x800331B8 nw4r::snd::DvdSoundArchive::detail_GetFileAddress
//   0x800331C0 NW_Obj::IsLoaded                         include/game/wii/xobj.h
//   0x800332D0, 0x800332D4 nw4r::g3d::IScnObjCallback::ExecCallback_CALC_MAT/_CALC_WORLD
//
// NONMATCHING: two more are not emitted yet, because this file does not build NW_Anm's vtable as
// retail does (xobj.h does not model the class with retail's shape):
//   0x80033044 NW_Anm::IsLoaded, `return mFile.IsLoaded();`. NW_Anm is the 0x148 bytes xobj.h calls
//              CJArchive: NW_Obj and the CFile at _5C.
//   0x80033058 CFile::IsLoaded (include/game/cfile.h), emitted along with it.

// 0x80033084 sits among the weak virtuals above, so retail wrote it in the class. It stays out of line
// until the in-class form is tried now that this file holds gGame (csound.h reaches other units).
BOOL CSoundFxReverbHiDpl2::Setup(const nw4r::snd::detail::FxReverbHiParam& param)
{
	if (mWorkBuffer != NULL) {
		Shutdown();
		ReleaseWorkBuffer();
		SoundHeapFree(&mWorkBuffer);
	}
	if (SetParam(param)) {
		u32 size = GetRequiredMemSize();
		return AssignWorkBuffer(SoundHeapAlloc(&mWorkBuffer, size), size);
	}
	return FALSE;
}
