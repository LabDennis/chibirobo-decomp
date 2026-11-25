#include <dolphin/exi.h>
#include <dolphin/os.h>

#include "__os.h"

// Number of days before each month in a common year.
static const u16 MonthStartDays[MONTH_MAX] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

/**
 * @note Address: N/A
 * @note Size: ?
 */
void __OSSetTime(OSTime time) {
    BOOL enabled = OSDisableInterrupts();
    OSTime oldTime = OSGetTime();

    // Changing the hardware timebase must preserve the adjusted system time.
    *__OSSystemTime += oldTime - time;
    __OSWriteTimeBase(time);
    EXIProbeReset();
    OSRestoreInterrupts(enabled);
}

/**
 * @note Address: 0x8016B090
 * @note Size: 0x64
 */
OSTime __OSGetSystemTime() {
    BOOL enabled = OSDisableInterrupts();
    OSTime ticks = OSGetTime();
    OSTime result = ticks + *__OSSystemTime;
    OSRestoreInterrupts(enabled);
    return result;
}

/**
 * @note Address: 0x8016B0F4
 * @note Size: 0x58
 */
OSTime __OSTimeToSystemTime(OSTime time) {
    BOOL enabled = OSDisableInterrupts();
    OSTime result = time + *__OSSystemTime;
    OSRestoreInterrupts(enabled);
    return result;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static int IsLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static int GetMonthStartDay(int month, BOOL leapYear) {
    // Leap day moves March and all later months one day forward.
    return MonthStartDays[month] + (leapYear && month >= 2);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static int GetLeapDays(int year) {
    ASSERTLINE(286, 0 <= year);
    
    if (year < 1) {
        return 0;
    }
    return (year + 3) / 4 - (year - 1) / 100 + (year - 1) / 400;
}

/**
 * @note Address: 0x8016B14C
 * @note Size: 0x19C
 */
static void GetDates(int days, OSCalendarTime* td) {
    int year;
    int yearStart;
    int month;
    BOOL leapYear;

    ASSERTLINE(311, 0 <= days);

    td->wday = (days + 6) % WEEK_DAY_MAX;

    for (year = days / YEAR_DAY_MAX; ; --year) {
        yearStart = year * YEAR_DAY_MAX + GetLeapDays(year);
        if (yearStart <= days) {
            break;
        }
    }

    days -= yearStart;
    td->year = year;
    td->yday = days;

    leapYear = IsLeapYear(year);
    month = MONTH_MAX - 1;
    while (days < GetMonthStartDay(month, leapYear)) {
        --month;
    }
    td->mon = month;
    td->mday = days - GetMonthStartDay(month, leapYear) + 1;
}

/**
 * @note Address: 0x8016B2E8
 * @note Size: 0x204
 */
void OSTicksToCalendarTime(OSTime ticks, OSCalendarTime* td) {
    int days;
    int secs;
    OSTime d;

    d = ticks % OS_SEC_TO_TICKS(1);    
    if (d < 0) {
        d += OS_SEC_TO_TICKS(1);
        ASSERTLINE(356, 0 <= d);
    }

    td->usec = OS_TICKS_TO_USEC(d) % USEC_MAX;
    td->msec = OS_TICKS_TO_MSEC(d) % MSEC_MAX;

    ASSERTLINE(360, 0 <= td->usec);
    ASSERTLINE(361, 0 <= td->msec);

    ticks -= d;

    ASSERTLINE(364, ticks % OSSecondsToTicks(1) == 0);
    ASSERTLINE(368, 0 <= OSTicksToSeconds(ticks) / 86400 + BIAS && OSTicksToSeconds(ticks) / 86400 + BIAS <= INT_MAX);

    days = (OS_TICKS_TO_SEC(ticks) / SECS_IN_DAY) + BIAS;    
    secs = OS_TICKS_TO_SEC(ticks) % SECS_IN_DAY;
    if (secs < 0) {
        days -= 1;
        secs += SECS_IN_DAY;
        ASSERTLINE(375, 0 <= secs);
    }

    GetDates(days, td);
    td->hour = secs / 60 / 60;
    td->min = secs / 60 % 60;
    td->sec = secs % 60;
}

/**
 * @note Address: 0x8016B4EC
 * @note Size: 0x2E0
 */
OSTime OSCalendarTimeToTicks(OSCalendarTime* td) {
    OSTime secs;
    int ov_mon;
    int mon;
    int year;

    ov_mon = td->mon / MONTH_MAX;
    mon = td->mon - (ov_mon * MONTH_MAX);

    if (mon < 0) {
        mon += MONTH_MAX;
        ov_mon--;
    }

    ASSERTLINE(412, (ov_mon <= 0 && 0 <= td->year + ov_mon) || (0 < ov_mon && td->year <= INT_MAX - ov_mon));
    
    year = td->year + ov_mon;

    secs = (OSTime)SECS_IN_YEAR * year +
           (OSTime)SECS_IN_DAY * (GetLeapDays(year) + GetMonthStartDay(mon, IsLeapYear(year)) + td->mday - 1) +
           (OSTime)SECS_IN_HOUR * td->hour +
           (OSTime)SECS_IN_MIN * td->min +
           td->sec -
           (OSTime)BIAS * SECS_IN_DAY;

    return OS_SEC_TO_TICKS(secs) + OS_MSEC_TO_TICKS((OSTime)td->msec) +
           OS_USEC_TO_TICKS((OSTime)td->usec);
}
