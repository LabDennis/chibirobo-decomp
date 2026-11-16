#include "nw4r/snd/snd_FxReverbHiDpl2.h"

#include "nw4r/types_nw4r.h"
#include "nw4r/snd/snd_AxManager.h"
#include "nw4r/snd/snd_global.h"
#include "nw4r/ut/ut_algorithm.h"

/* After ss's snd_FxReverbStdDpl2.cpp (the same class around AXFX's standard reverb) and
 * wiiplay's snd_FxReverbHiDpl2.cpp (the 2006 form of this class, without the stereo/DPL2 switch).
 */

namespace nw4r {
namespace snd {

FxReverbHiDpl2::FxReverbHiDpl2() : mIsActive(0) {
    detail::FxReverbHiParam p;
    SetParam(p);
}

u32 FxReverbHiDpl2::GetRequiredMemSize() {
    return ut::Max(
        ut::RoundUp(AXFXReverbHiExpGetMemSizeDpl2(&mAxfxParamDpl) + detail::AxfxImpl::HEAP_SIZE_MIN, 32),
        ut::RoundUp(AXFXReverbHiExpGetMemSize(&mAxfxParam) + detail::AxfxImpl::HEAP_SIZE_MIN, 32)
    );
}

bool FxReverbHiDpl2::AssignWorkBuffer(void *pBuffer, u32 size) {
    return mImpl.CreateHeap(pBuffer, size);
}

void FxReverbHiDpl2::ReleaseWorkBuffer() {
    mImpl.DestroyHeap();
}

bool FxReverbHiDpl2::StartUp() {
    u32 memSize = GetRequiredMemSize();
    if (memSize > mImpl.GetHeapTotalSize()) {
        return false;
    }

    AXFXAllocHook allocHook;
    AXFXFreeHook freeHook;
    BOOL success;
    if (detail::AxManager::GetInstance().GetOutputMode() == OUTPUT_MODE_DPL2) {
        mOutputMode = 1;

        mImpl.HookAlloc(&allocHook, &freeHook);
        success = AXFXReverbHiExpInitDpl2(&mAxfxParamDpl);
        mImpl.RestoreAlloc(allocHook, freeHook);

        (void)AXFXReverbHiExpGetMemSizeDpl2(&mAxfxParamDpl); // debug leftover
    } else {
        mOutputMode = 0;

        mImpl.HookAlloc(&allocHook, &freeHook);
        success = AXFXReverbHiExpInit(&mAxfxParam);
        mImpl.RestoreAlloc(allocHook, freeHook);

        (void)AXFXReverbHiExpGetMemSize(&mAxfxParam); // debug leftover
    }

    mIsActive = 1;

    return success;
}

void FxReverbHiDpl2::Shutdown() {
    if (!mIsActive) {
        return;
    }
    AXFXAllocHook allocHook;
    AXFXFreeHook freeHook;
    mIsActive = 0;
    mImpl.HookAlloc(&allocHook, &freeHook);

    if (mOutputMode == 1) {
        AXFXReverbHiExpShutdownDpl2(&mAxfxParamDpl);
    } else {
        AXFXReverbHiExpShutdown(&mAxfxParam);
    }
    mImpl.RestoreAlloc(allocHook, freeHook);
}

bool FxReverbHiDpl2::SetParam(const detail::FxReverbHiParam &rParam) {
    mParam = rParam;

    f32 preDelayTimeMax = ut::Max(rParam.preDelayTimeMax, 0.0f);
    bool needSettings = preDelayTimeMax != mAxfxParam.preDelayTimeMax;

    mAxfxParam.earlyMode = mAxfxParamDpl.earlyMode = rParam.earlyMode;
    mAxfxParam.preDelayTimeMax = mAxfxParamDpl.preDelayTimeMax = preDelayTimeMax;

    mAxfxParam.preDelayTime = mAxfxParamDpl.preDelayTime =
        ut::Clamp(rParam.preDelayTime, 0.0f, rParam.preDelayTimeMax);
    mAxfxParam.fusedMode = mAxfxParamDpl.fusedMode = rParam.fusedMode;
    mAxfxParam.fusedTime = mAxfxParamDpl.fusedTime = ut::Max(rParam.fusedTime, 0.0f);
    mAxfxParam.coloration = mAxfxParamDpl.coloration = ut::Clamp(rParam.coloration, 0.0f, 1.0f);
    mAxfxParam.damping = mAxfxParamDpl.damping = ut::Clamp(rParam.damping, 0.0f, 1.0f);
    mAxfxParam.crosstalk = mAxfxParamDpl.crosstalk = ut::Clamp(rParam.crossTalk, 0.0f, 1.0f);
    mAxfxParam.earlyGain = mAxfxParamDpl.earlyGain = ut::Clamp(rParam.earlyGain, 0.0f, 1.0f);
    mAxfxParam.fusedGain = mAxfxParamDpl.fusedGain = ut::Clamp(rParam.fusedGain, 0.0f, 1.0f);
    mAxfxParam.outGain = mAxfxParamDpl.outGain = ut::Clamp(rParam.outGain, 0.0f, 1.0f);

    mAxfxParam.busIn = NULL;
    mAxfxParam.busOut = NULL;
    mAxfxParam.sendGain = 0.0f;

    mAxfxParamDpl.busIn = NULL;
    mAxfxParamDpl.busOut = NULL;
    mAxfxParamDpl.sendGain = 0.0f;

    if (!mIsActive) {
        return true;
    }

    u32 memSize = GetRequiredMemSize();
    if (memSize > mImpl.GetHeapTotalSize()) {
        return false;
    }

    AXFXAllocHook allocHook;
    AXFXFreeHook freeHook;
    BOOL success;
    if (needSettings) {
        mImpl.HookAlloc(&allocHook, &freeHook);
        if (mOutputMode == 1) {
            success = AXFXReverbHiExpSettingsDpl2(&mAxfxParamDpl);
        } else {
            success = AXFXReverbHiExpSettings(&mAxfxParam);
        }

        mImpl.RestoreAlloc(allocHook, freeHook);
    } else {
        if (mOutputMode == 1) {
            success = AXFXReverbHiExpSettingsUpdateDpl2(&mAxfxParamDpl);
        } else {
            success = AXFXReverbHiExpSettingsUpdate(&mAxfxParam);
        }
    }

    return success;
}

void FxReverbHiDpl2::UpdateBuffer(
    int channels, void **ppBuffer, u32 size, SampleFormat sampleFormat, f32 sampleRate, OutputMode outputMode
) {
    if (!mIsActive) {
        return;
    }

    if ((outputMode == OUTPUT_MODE_DPL2) != mOutputMode) {
        return;
    }

    if (mOutputMode == 1) {
        AXFX_BUFFERUPDATE_DPL2 chans;
        chans.left = static_cast<s32 *>(ppBuffer[AX_DPL2_L]);
        chans.right = static_cast<s32 *>(ppBuffer[AX_DPL2_R]);
        chans.left_surround = static_cast<s32 *>(ppBuffer[AX_DPL2_LS]);
        chans.right_surround = static_cast<s32 *>(ppBuffer[AX_DPL2_RS]);
        AXFXReverbHiExpCallbackDpl2(&chans, &mAxfxParamDpl);
    } else {
        AXFX_BUFFERUPDATE chans;
        chans.left = static_cast<s32 *>(ppBuffer[AX_STEREO_L]);
        chans.right = static_cast<s32 *>(ppBuffer[AX_STEREO_R]);
        chans.surround = static_cast<s32 *>(ppBuffer[AX_STEREO_S]);
        AXFXReverbHiExpCallback(&chans, &mAxfxParam);
    }
}

void FxReverbHiDpl2::OnChangeOutputMode() {
    if (mOutputMode != (detail::AxManager::GetInstance().GetOutputMode() == OUTPUT_MODE_DPL2)) {
        Shutdown();
        StartUp();
    }
}

} // namespace snd
} // namespace nw4r
