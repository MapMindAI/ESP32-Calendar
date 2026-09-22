#ifndef CALENDAR_CALC_H
#define CALENDAR_CALC_H

#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Everything the dashboard draws. The UI reads this struct and nothing else:
   no task touches an LVGL widget with a sensor handle or a time_t in hand. */
typedef struct {
  int year;
  int month; /* 1..12 */
  int day;   /* 1..31 */

  int hour;   /* 0..23 */
  int minute; /* 0..59 */

  int weekday;     /* 0 = Monday ... 6 = Sunday */
  int week_number; /* ISO 8601 week number */

  int day_of_year;  /* 1..366 */
  int days_of_year; /* 365 or 366 */

  /* 十二值日口径的黄道日状态；the generated lookup covers 2026..2030. */
  bool lunar_data_valid;
  bool lunar_auspicious;
  unsigned char lunar_deity_index;   /* 青龙..勾陈, 0..11 */
  unsigned char lunar_officer_index; /* 建..闭, 0..11    */
  const char* day_yi;
  const char* day_ji;
  int lunar_year_gan; /* 0 = 甲 ... 9 = 癸 */
  int lunar_year_zhi; /* 0 = 子 ... 11 = 亥 */

  bool time_valid;

  float temperature_c;
  float humidity_percent;

  bool environment_valid;
} calendar_ui_data_t;

bool calendar_calc_is_leap_year(int year);
int calendar_calc_days_in_month(int year, int month);
int calendar_calc_days_in_year(int year);
int calendar_calc_day_of_year(int year, int month, int day);

/* 0 = Monday ... 6 = Sunday, so the calendar grid and the ISO week agree. */
int calendar_calc_weekday(int year, int month, int day);
int calendar_calc_iso_week(int year, int month, int day);

const char* calendar_calc_month_name(int month);     /* "SEPTEMBER" */
const char* calendar_calc_weekday_name(int weekday); /* "MONDAY" */

/* Fill the date/time half of the struct from broken-down local time.
   The environment fields are left untouched. */
void calendar_calc_fill(calendar_ui_data_t* data, const struct tm* local);

#ifdef __cplusplus
}
#endif

#endif
