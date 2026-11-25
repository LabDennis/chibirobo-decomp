#include <Dolphin/OS/OSUtil.h>
#include <global.h>
#include <string.h>
#include <sysdolphin/perf.h>

static OSTime start_time;

HSD_PerfStat HSD_PerfLastStat    = { 0 };
HSD_PerfStat HSD_PerfCurrentStat = { 0 };

static inline f32 ElapsedFrames(void)
{
    OSTime elapsed = OSGetTime() - start_time;

    return elapsed / (f32)(OS_TIMER_CLOCK / 60);
}

/**
 * @note Address: N/A
 * @note Size: 0x64
 */
void HSD_PerfInitStat(void)
{
    HSD_PerfLastStat = HSD_PerfCurrentStat;
    memset(&HSD_PerfCurrentStat, 0, sizeof(HSD_PerfCurrentStat));
}

/**
 * @note Address: N/A
 * @note Size: 0x28
 */
void HSD_PerfSetStartTime(void)
{
    start_time = OSGetTime();
}

/**
 * @note Address: N/A
 * @note Size: 0x74
 */
void HSD_PerfSetCPUTime(void)
{
    HSD_PerfCurrentStat.cpu_time = ElapsedFrames();
}

/**
 * @note Address: N/A
 * @note Size: 0x78
 */
void HSD_PerfSetDrawTime(void)
{
    HSD_PerfCurrentStat.draw_time = ElapsedFrames();
}

/**
 * @note Address: N/A
 * @note Size: 0x78
 */
void HSD_PerfSetTotalTime(void)
{
    HSD_PerfCurrentStat.total_time = ElapsedFrames();
}

/**
 * @note Address: 0x801D3AB4
 * @note Size: 0x5C
 */
void HSD_PerfCountEnvelopeBlending(s32 n)
{
    assert_line(164, n < 32);
    HSD_PerfCurrentStat.env_blend[n]++;
}
