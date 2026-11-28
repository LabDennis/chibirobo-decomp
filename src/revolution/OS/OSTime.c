#include <dolphin/os.h>

#include "__os.h"

// End of each month in standard year. The two month tables and the countdown
// loops below are the Revolution SDK form also carried by xenoblade's
// libs/RVL_SDK/src/revolution/os/OSTime.c and ogws' src/revolution/OS/OSTime.c.
static s32 YearDays[MONTH_MAX] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};
// End of each month in leap year
static s32 LeapYearDays[MONTH_MAX] = {
    0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335
};

/**
 * @note Address: 0x8018B344
 * @note Size: 0x64
 */
OSTime __OSGetSystemTime() {
    BOOL enabled = OSDisableInterrupts();
    OSTime result = *__OSSystemTime + OSGetTime();
    OSRestoreInterrupts(enabled);
    return result;
}

/**
 * @note Address: 0x8018B3A8
 * @note Size: 0x58
 */
OSTime __OSTimeToSystemTime(OSTime time) {
    BOOL enabled = OSDisableInterrupts();
    OSTime result = *__OSSystemTime + time;
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
static s32 GetYearDays(s32 year, s32 mon) {
    return (IsLeapYear(year) ? LeapYearDays : YearDays)[mon];
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
 * @note Address: 0x8018B400
 * @note Size: 0x170
 */
static void GetDates(int days, OSCalendarTime* td) {
    int year;
    int totalDays;
    s32* p_days;
    int month;

    ASSERTLINE(311, 0 <= days);

    td->wday = (days + 6) % WEEK_DAY_MAX;

    for (year = days / YEAR_DAY_MAX;
         days < (totalDays = year * YEAR_DAY_MAX + GetLeapDays(year)); year--) {
        ;
    }

    days -= totalDays;
    td->year = year;
    td->yday = days;

    p_days = IsLeapYear(year) ? LeapYearDays : YearDays;
    for (month = MONTH_MAX; days < p_days[--month];) {
        ;
    }
    td->mon = month;
    td->mday = days - p_days[month] + 1;
}

/**
 * @note Address: 0x8018B570
 * @note Size: 0x1C8
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
 * @note Address: N/A
 * @note Size: ?
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
           (OSTime)SECS_IN_DAY * (GetLeapDays(year) + GetYearDays(year, mon) + td->mday - 1) +
           (OSTime)SECS_IN_HOUR * td->hour +
           (OSTime)SECS_IN_MIN * td->min +
           td->sec -
           (OSTime)BIAS * SECS_IN_DAY;

    return OS_SEC_TO_TICKS(secs) + OS_MSEC_TO_TICKS((OSTime)td->msec) +
           OS_USEC_TO_TICKS((OSTime)td->usec);
}
