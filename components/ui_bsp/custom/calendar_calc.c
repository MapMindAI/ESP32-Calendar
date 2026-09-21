#include "calendar_calc.h"

static const char *const month_names[12] = {
    "JANUARY", "FEBRUARY", "MARCH",     "APRIL",   "MAY",      "JUNE",
    "JULY",    "AUGUST",   "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
};

static const char *const weekday_names[7] = {
    "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY", "SUNDAY"
};

bool calendar_calc_is_leap_year(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int calendar_calc_days_in_month(int year, int month)
{
    static const int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (month < 1 || month > 12) {
        return 0;
    }
    if (month == 2 && calendar_calc_is_leap_year(year)) {
        return 29;
    }
    return days[month - 1];
}

int calendar_calc_days_in_year(int year)
{
    return calendar_calc_is_leap_year(year) ? 366 : 365;
}

int calendar_calc_day_of_year(int year, int month, int day)
{
    int ordinal = day;

    for (int m = 1; m < month; m++) {
        ordinal += calendar_calc_days_in_month(year, m);
    }
    return ordinal;
}

int calendar_calc_weekday(int year, int month, int day)
{
    /* Sakamoto's method, which returns 0 = Sunday; shift to 0 = Monday. */
    static const int offsets[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    int y = year;

    if (month < 3) {
        y -= 1;
    }
    int sunday_first = (y + y / 4 - y / 100 + y / 400 + offsets[month - 1] + day) % 7;
    return (sunday_first + 6) % 7;
}

/* Weekday of 31 December, 0 = Monday. A year has 53 ISO weeks when it ends on a
   Thursday, or on a Friday if it is a leap year. */
static int weeks_in_iso_year(int year)
{
    int last = calendar_calc_weekday(year, 12, 31);

    if (last == 3 || (last == 4 && calendar_calc_is_leap_year(year))) {
        return 53;
    }
    return 52;
}

int calendar_calc_iso_week(int year, int month, int day)
{
    int ordinal  = calendar_calc_day_of_year(year, month, day);
    int iso_wday = calendar_calc_weekday(year, month, day) + 1; /* 1 = Monday */
    int week     = (ordinal - iso_wday + 10) / 7;

    if (week < 1) {
        return weeks_in_iso_year(year - 1);
    }
    if (week > weeks_in_iso_year(year)) {
        return 1;
    }
    return week;
}

const char *calendar_calc_month_name(int month)
{
    if (month < 1 || month > 12) {
        return "";
    }
    return month_names[month - 1];
}

const char *calendar_calc_weekday_name(int weekday)
{
    if (weekday < 0 || weekday > 6) {
        return "";
    }
    return weekday_names[weekday];
}

void calendar_calc_fill(calendar_ui_data_t *data, const struct tm *local)
{
    if (data == NULL || local == NULL) {
        return;
    }

    data->year   = local->tm_year + 1900;
    data->month  = local->tm_mon + 1;
    data->day    = local->tm_mday;
    data->hour   = local->tm_hour;
    data->minute = local->tm_min;

    data->weekday      = calendar_calc_weekday(data->year, data->month, data->day);
    data->week_number  = calendar_calc_iso_week(data->year, data->month, data->day);
    data->day_of_year  = calendar_calc_day_of_year(data->year, data->month, data->day);
    data->days_of_year = calendar_calc_days_in_year(data->year);
    data->time_valid   = true;
}
