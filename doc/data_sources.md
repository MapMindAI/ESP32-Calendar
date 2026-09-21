# Dashboard data sources

This document describes how the calendar dashboard obtains its date, time,
temperature and humidity values, what happens when a source is unavailable,
and when a changed value reaches the display.

## Data flow

```
PCF85063 RTC ─┐
              ├─ time_manager ── POSIX local time ── Calendar_LoopTask ─┐
SNTP server ──┘                                                        │
                                                                         ├─ calendar_ui
SHTC3 sensor ── sensor_manager ── Sensor_LoopTask ─────────────────────┘
```

The dashboard does not read I2C devices or network services directly. The
application tasks use `time_manager` and `sensor_manager`; those services own
the source-specific behavior. LVGL calls made by the tasks are protected by
`Lvgl_lock()`.

## Date and time

### Boot source: PCF85063 RTC

`UserApp_AppInit()` calls `time_manager_init(&I2cbus)` before the display and
LVGL are started. The manager:

1. Applies `CONFIG_CALENDAR_TIMEZONE` as the POSIX `TZ` environment value and
   calls `tzset()`.
2. Initializes the PCF85063 at I2C address `0x51`.
3. Reads its date and time. A year from 2024 through 2099 is accepted.
4. Converts that local broken-down time with `mktime()` and seeds the ESP32's
   POSIX system clock with `settimeofday()`.

The RTC therefore provides the normal offline clock. If its year is outside
the accepted range, the system clock remains invalid and the dashboard shows
`--:--` and `WAITING FOR TIME` until a valid source becomes available.

### Network correction: SNTP

`Time_SyncTask` waits for the station interface to receive an IP address. The
address can come from saved Wi-Fi credentials at boot or from the captive
portal after setup. Only then does it start SNTP against
`CONFIG_CALENDAR_NTP_SERVER` (default: `pool.ntp.org`).

When SNTP completes a synchronization, its callback reads the corrected local
POSIX time and writes it back to the PCF85063. Subsequent boots can therefore
use the corrected RTC without requiring Wi-Fi.

Wi-Fi is optional for ordinary operation: no connection means no SNTP
correction, but a valid RTC continues to provide date and time.

### Calendar values and display cadence

`Calendar_LoopTask` calls `time_manager_get_local()` once per second. For a
valid `struct tm`, `calendar_calc_fill()` derives the dashboard fields:

* Gregorian year, month and day
* hour and minute
* Monday-first weekday
* ISO 8601 week number
* day number within the year and the number of days in that year

The large clock is updated only when the minute changes. Date labels, the
month grid and the day-of-year summary are refreshed only when the year, month
or day changes. This also handles an SNTP correction that changes the month or
year without relying solely on the day-of-month value.

## Temperature and humidity

`sensor_manager_init(&I2cbus)` constructs the SHTC3 port on the shared I2C0
bus (SDA GPIO13, SCL GPIO14, address `0x70`). `Sensor_LoopTask` calls
`sensor_manager_read()` once per minute.

Each reading is accepted only when it falls within the SHTC3 operating range
used by the firmware:

| Value | Accepted range |
|---|---|
| Temperature | −40.0 to 85.0 °C |
| Relative humidity | 0 to 100 %RH |

Accepted values are exponentially smoothed before publication:

```
filtered = previous × 0.8 + new_reading × 0.2
```

This prevents small sensor fluctuations from repeatedly causing full-panel
updates. A failed or out-of-range sample does not replace the stored good
measurement, but is published as invalid; the UI displays `--.-°C` and
`--% RH` rather than presenting a stale or zero value as current.

The environment row is redrawn when validity changes, temperature changes by
at least 0.2 °C, humidity changes by at least 1 %RH, or ten minutes have
passed without an environment redraw.

## Battery percentage

`Adc_PortInit()` configures ADC1 channel 3 during application initialization. `Battery_LoopTask`
samples the existing `Adc_GetBatteryLevel()` helper once per minute and updates the dashboard's
bottom-right percentage label only when the integer percentage changes. The helper maps 3.0 V or
below to 0%, 4.12 V or above to 100%, and interpolates linearly between those voltages.

## Rendering implication

The panel uses full refresh: every LVGL invalidation converts and transfers the
entire 400 × 300 frame. `RENDER_FPS` in `main/user_config.h` caps the active
LVGL render cadence; it does not cause periodic panel
transfers while the UI is idle. The date/time and sensor cadences above avoid
unnecessary invalidations.

`CONFIG_LV_USE_PERF_MONITOR=y` also enables LVGL's own bottom-right overlay, which reports LVGL's
FPS and CPU usage. No application-side performance counters are used.

## Relevant code

| Responsibility | File |
|---|---|
| RTC seed, timezone, SNTP and RTC write-back | `components/app_bsp/time_manager.cpp` |
| SHTC3 reads, validation and smoothing | `components/app_bsp/sensor_manager.cpp` |
| Battery voltage and percentage conversion | `components/port_bsp/adc_bsp.cpp` |
| Polling tasks and UI update thresholds | `components/user_app/user_app.cpp` |
| Calendar-derived values | `components/ui_bsp/custom/calendar_calc.c` |
| Dashboard rendering | `components/ui_bsp/custom/calendar_ui.c` |
| LVGL render cadence and LCD flush | `components/app_bsp/lvgl_bsp.cpp`, `main/main.cpp` |
