# User interface and interaction

How the firmware presents information on the reflective LCD and what the two user buttons do.
Describes the **current** behaviour — see §7 for what is still factory-demo residue.

Code map:

| Concern | File |
|---|---|
| Dashboard widget tree, positions, styles | `components/ui_bsp/page_calendar/calendar_ui.c` |
| Date maths and the UI data model | `components/ui_bsp/page_calendar/calendar_calc.c`, `calendar_calc.h` |
| UI fonts | `components/ui_bsp/fonts/`, declared in `calendar_fonts.h` or `wifi_setup_page.h` |
| Wi-Fi setup view | `components/ui_bsp/page_wifi_setup/wifi_setup_page_layout.c` |
| Widget handles (`wifi_setup_page_t` struct) | `components/ui_bsp/page_wifi_setup/wifi_setup_page.h` |
| Tarot view container and image widget | `components/ui_bsp/page_tarot/tarot_page.c` |
| Tarot SD scan, JPEG decode, mono dither | `components/app_bsp/tarot_manager.cpp` |
| View switching, refresh cadence, all behaviour | `components/user_app/user_app.cpp` |
| System clock, RTC backup, SNTP | `components/app_bsp/time_manager.cpp` |
| Station bring-up/teardown, captive portal | `components/app_bsp/esp_wifi_bsp.c`, `wifi_portal.c` |
| Temperature/humidity source | `components/app_bsp/sensor_manager.cpp` |
| Battery source | `components/port_bsp/adc_bsp.cpp` |
| Button decoding | `components/port_bsp/button_bsp.c` |
| LVGL port, flush, refresh policy | `components/app_bsp/lvgl_bsp.cpp`, `main/main.cpp` |

For source, validation and synchronization details, see
[`data_sources.md`](data_sources.md).

## 1. Display characteristics that shape the design

* **400 × 300 landscape, 1 bit per pixel.** There is no grey. LVGL renders RGB565 and the flush
  callback in `main/main.cpp` thresholds every pixel at `0x7fff` — anything darker than mid-grey
  becomes black. Anti-aliased font edges, gradients and shadows all collapse; design in pure black
  and white and check contrast at the threshold, not in a colour preview.
* **Reflective, no backlight.** Contrast comes from ambient light. Large type and solid fills read
  well; hairlines and light-grey chrome disappear.
* **Full refresh every flush.** `disp_drv.full_refresh = 1`, so the entire 400 × 300 frame is
  re-converted and re-sent on any change. Animations and per-second redraws are expensive; prefer
  layouts that update a small number of coarse values on a slow cadence.
* **No touch.** The panel is display-only. Every interaction goes through the two buttons in §4.
* The physical panel is fragile — handle the board by its edges, the mount and not the glass.

## 2. Screen model

There is exactly **one** LVGL screen (`ui->screen`, white background). Three full-size containers
sit on it, and navigation is done by toggling `LV_OBJ_FLAG_HIDDEN` on them — `lv_scr_load()` and
LVGL screen-animation helpers are not used.

| Container | View | Initial state |
|---|---|---|
| `calendar_ui_root()` | Status dashboard (the home view) | visible |
| `tarot_page_root()` | Tarot card | hidden |
| `screen_cont_wifi_setup` | Wi-Fi setup / configuration | hidden |

`UserApp_UiInit()` calls `wifi_setup_page_init()` and builds `calendar_ui` and `tarot_page` directly
on the same screen. The dashboard is laid out in hand-written code where 42 calendar cells are cheap
to express.

The invariant is *exactly one container visible at a time*. Every switch clears the incoming
container's hidden flag and hides the other two. Adding a view means extending `show_view()` in
`button_interface.cpp` and the `app_view_t` cycle.

## 3. The views

### 3.1 Status dashboard — `calendar_ui`

The home view: the clock and the date in a narrow left column, the current month across the right
~60% of the width, a rule and a summary bar along the bottom.

```
 (0,0)                                                                  (400,0)
 ┌────────────────────┬─────────────────────────────────────────────────┐
 │ 2026·09·21         │  SEPTEMBER 2026  丙午年                          │
 │ MONDAY             │                                                 │
 │                    │    M    T    W    T    F    S    S              │
 │ 10:24              │         1    2    3    4    5    6              │
 │                    │    7    8    9   10   11   12   13              │
 │ 24.8°C │ 56% RH    │   14   15   16   17   18   19   20              │
 │                    │  [21]  22   23   24   25   26   27              │
 │ WEEK 39 · DAY 264  │   28   29   30                                  │
 │ 建除：建 值神：青龙 │                                                 │
 │                    │                                                 │
 ├────────────────────┴─────────────────────────────────────────────────┤
 │ 宜：嫁娶、纳采、祭祀、解除、出行、修造…                           87%│
 │ 忌：造庙、行丧、安葬、伐木…                                        ((•✓│
 └──────────────────────────────────────────────────────────────────────┘
 (0,300)                                                              (400,300)
```

The column split is the constraint everything else follows from: the separator sits at x = 158, so
the month grid owns the right 242 px (≈ 60 %) and the left column has 134 px to work in. That is
what caps the clock at 48 px — `10:24` in the 72 px face is 196 px wide (see §5).

Visual hierarchy, in order: the clock, then the month grid, then the environment row, then
everything else. Nothing on this screen animates and nothing shows seconds.

| Widget | Position (x, y) | Content | Written by | Cadence |
|---|---|---|---|---|
| `date_label` | 16, 18 | `%04d·%02d·%02d`, 16 px | `Calendar_LoopTask` | on date change |
| `weekday_label` | 16, 42 | `MONDAY` … `SUNDAY`, 12 px | `Calendar_LoopTask` | on date change |
| `time_label` | 15, 68 (133 wide) | `%02d:%02d`, 48 px, fixed width | `Calendar_LoopTask` | on minute change |
| `temperature_label` | 16, 136 | `%.1f°`, 16 px | `Sensor_LoopTask` | on threshold |
| `env_separator` | 78, 137 (16 tall) | 1 px rule between the two readings | — | — |
| `humidity_label` | right-aligned to 148, 136 | `%.0f%%`, 16 px | `Sensor_LoopTask` | on threshold |
| `week_info_label` | 16, 180 | `WEEK %d · DAY %d`, ISO week, 12 px | `Calendar_LoopTask` | on date change |
| `almanac_label` | 16, 208 | `建除：<十二建除> 值神：<十二值神>`, one line, 12 px Chinese subset | `Calendar_LoopTask` | on date change |
| `vertical_separator` | 158, 15 (220 tall) | column rule | — | — |
| `month_label`, `ganzhi_label` | 168…384, 15 | `SEPTEMBER 2026` plus `丙午年`, 18 px / 12 px | `Calendar_LoopTask` | on date change |
| `weekday_header[7]` | 171 + 30·col, 45 | `M T W T F S S`, 16 px | — | — |
| `calendar_days[6][7]` | 171 + 30·col, 69 + 26·row | day numbers, 16 px, cells 30 × 26 | `Calendar_LoopTask` | on date change |
| `horizontal_separator` | 16, 247 (368 wide) | rule above the bar | — | — |
| `yi_label`, `ji_label` | 16, 254 / 269 (312 wide) | `宜：<list>` / `忌：<list>` — the whole almanac list on one line, clipped at the bar width, 12 px Chinese subset | `Calendar_LoopTask` | on date change |
| `battery_label` | right-aligned to 384, 254 (48 wide) | `%u%%`, 12 px | `Battery_LoopTask` | on percentage change, sampled every minute |
| `wifi_label` | right-aligned to 384, 268 (48 wide) | `LV_SYMBOL_WIFI` plus a state marker (`✓`, `...`, `!`, `?`), 14 px Montserrat | `Time_SyncTask` | at the start and end of every sync window |
| `uptime_label` | right-aligned to 384, 284 (48 wide) | elapsed boot duration `%02u:%02u`, 12 px | `Calendar_LoopTask` | once per minute, only with `LVGL_DEBUG_LOG` |

The month grid is **Monday-first and current-month-only**: no leading or trailing days from the
neighbouring months, and the selected day is the one inverted cell (black fill, white text, 3 px
radius). Normally that is today; a KEY single click browses forward one day at a time and a double
click one month at a time (§4.1), and
while browsing, the grid, `month_label`, `ganzhi_label`, `almanac_label` and the 宜/忌 lines follow
the selected day while `date_label`, `weekday_label`, `week_info_label` and the clock keep showing
real today. A KEY long press returns the view to today.

#### Refresh policy

`disp_drv.full_refresh = 1`, so any invalidated widget costs a whole 400 × 300 conversion and SPI
transfer. Both tasks therefore compare before they write, and a label is only touched when the value
behind it actually moved:

| What | Read cadence | Written to the panel |
|---|---|---|
| Clock | system clock, 1 s | when the minute changes |
| Date, weekday, week, month grid, bottom bar | system clock, 1 s | when the day changes (`calendar_ui_refresh_all`) |
| Temperature, humidity | `sensor_manager_read()`, 60 s | when \|Δt\| ≥ 0.2 °C or \|Δrh\| ≥ 1 % |
| Battery | ADC1 channel 3, 60 s | when the percentage changes |
| Wi-Fi icon | the sync window itself | twice per window: opened, and closed with its outcome |
| Almanac result | generated 2026--2046 day table, 1 s | when the day changes |

The LVGL task is event-driven: it runs a refresh only after one of these data
changes or a button-driven view switch. The panel stays idle between them.

#### Empty states

Neither the clock nor the sensor is trusted before it has answered:

| Condition | Shown |
|---|---|
| system clock below year 2024 | `--:--` for the time, `WAITING FOR TIME` on the weekday line, everything else blank |
| sensor never read, read failed, or out of range | `--.-°C` and `--% RH` |

A zero is never displayed for a missing reading — `0°C` reads as real data.

#### Wi-Fi icon

The radio is down except inside a sync window (§3.4), so the icon reports the
**last window's outcome** rather than a live link state. It is one glyph plus one
ASCII marker, because there is no colour on this panel and no room beside the
battery for a second icon:

| `calendar_wifi_state_t` | Shown | Meaning |
|---|---|---|
| `CALENDAR_WIFI_UNSET` | wifi `?` | no credentials stored — long-press BOOT and run setup |
| `CALENDAR_WIFI_ACTIVE` | wifi `...` | a window is open right now, the radio is up |
| `CALENDAR_WIFI_SYNCED` | wifi `✓` | the last window got the time and the radio is back down |
| `CALENDAR_WIFI_FAILED` | wifi `!` | credentials exist, but the last window got no answer |

Every state carries a marker on purpose: a bare wifi glyph reads as a live link,
and the radio is down in three of the four — only `...` means it is up.

The glyphs come from `lv_font_montserrat_14` — LVGL's default font, linked either
way, and the only face here that carries `LV_SYMBOL_WIFI` and `LV_SYMBOL_OK` (§5).

With `LVGL_DEBUG_LOG` enabled, the `HH:MM` immediately below the icon is elapsed
time since boot (hours and minutes), refreshed each minute. It is hidden when
`LVGL_DEBUG_LOG` is disabled.

#### Where the values come from

```
PCF85063 ──▶ time_manager ──▶ POSIX system clock ──▶ Calendar_LoopTask ─┐
SNTP     ──▶                         ▲                                 ├──▶ calendar_ui_data_t ──▶ calendar_ui
                                     └── written back on sync          │
SHTC3    ──▶ sensor_manager ──▶ Sensor_LoopTask ───────────────────────┤
ADC1     ──▶ Battery_LoopTask ─────────────────────────────────────────┘

Time_SyncTask ──▶ Wi-Fi up ──▶ SNTP ──▶ Wi-Fi down ──▶ wifi icon
```

* `time_manager` (`components/app_bsp/time_manager.cpp`) sets `TZ` from `CONFIG_CALENDAR_TIMEZONE`,
  seeds the system clock from the PCF85063 at boot, and writes the RTC back whenever
  `time_manager_sync_now()` lands an answer from `CONFIG_CALENDAR_NTP_SERVER`. `Time_SyncTask` is
  what calls it, inside a sync window (§3.4). With no network the clock still runs from the RTC, and
  the display never depends on Wi-Fi being up.
* `sensor_manager` (`components/app_bsp/sensor_manager.cpp`) is the only thing that knows the part is
  an SHTC3. It range-checks every reading (−40…85 °C, 0…100 %RH) and smooths it (α = 0.2) so a
  sensor dithering between 24.7 and 24.8 does not walk over the refresh threshold each minute.
* `calendar_calc` (`components/ui_bsp/page_calendar/calendar_calc.c`) turns a `struct tm` into the
  `calendar_ui_data_t` the UI draws: weekday (Monday-first), ISO 8601 week number, day of year.
  It also looks up the generated 2026--2046 almanac: the 黄道吉日 bit, the 十二建除 officer and the
  十二值神 deity, the last two as 4-bit indices into the name tables in `calendar_ui.c`. The UI
  reads that struct and nothing else.
* `lunar/lunar_utils.py` produces both the complete
  `lunar/auspicious_days_2026_2046.csv` reference list and the compact
  `components/ui_bsp/page_calendar/lunar_auspicious_days.h` firmware asset. “黄道吉日” here uses the
  traditional 十二值日 definition: 除、危、定、执、成、开. It is a calendar classification, not a
  recommendation for a particular activity. Dates outside 2026--2046 deliberately leave the
  centre status blank rather than presenting an invented result.
  `lunar/generate_ui_yiji.py` turns the CSV's 宜 / 忌 columns into
  `components/ui_bsp/page_calendar/lunar_yiji_data.h` — the complete list for every day, ~1.1 MiB of flash,
  because the bar clips each line rather than truncating the data — plus the 1 bpp font that covers
  every character those lists use and the 1 bpp 干支 font. Both scripts rebuild their firmware assets from the checked-in CSV
  with `--from-csv`; see [`almanac.md`](almanac.md) for the full regeneration procedure.
* `Battery_LoopTask` samples the existing ADC1 channel 3 driver once a minute and writes its
  percentage to the bottom-right status label. The driver maps 3.0 V or below to 0%, 4.12 V or above
  to 100%, with a linear value between those limits.

#### LVGL performance monitor

`CONFIG_LV_USE_PERF_MONITOR=y` enables LVGL's built-in performance overlay. It is created by LVGL on
the system layer at the bottom right and reports its FPS and CPU usage; the application does not
collect or format separate render statistics.

### 3.2 Wi-Fi setup view — `screen_cont_wifi_setup`

The network configuration view. It is reached by a long press on **BOOT** and shows what the
device's own configuration hotspot is doing; all text entry happens on the phone (§3.3).

| Widget | Position | Content |
|---|---|---|
| `screen_label_cfg_title` | 0, 6 (400×34) | static `"Wi-Fi Setup"`, 25 px |
| `screen_label_cfg_state` | 10, 44 (380×104) | the flow step, 25 px, wrapped |
| `screen_label_cfg_ap` | 10, 152 (380×86) | hotspot/portal reminder or saved-settings note, 18 px, wrapped |
| `screen_label_cfg_hint` | 10, 244 (380×48) | static `"Long-press BOOT to exit"`, 18 px |

`Config_LoopTask` owns the two dynamic labels and drives them through these states:

| State | `screen_label_cfg_state` | `screen_label_cfg_ap` |
|---|---|---|
| view closed / idle | `Long-press BOOT\nto configure` | unchanged |
| view opened / host disabled | `Long-press KEY\nto enable hotspot` | `Hotspot disabled\nLong-press BOOT to exit` |
| KEY long press | `Starting hotspot...` | `Hotspot: ESP32-Calendar\nPassword: calendar` |
| hotspot + portal up | `Hotspot ready\nJoin it, page pops up` | `Hotspot: ESP32-Calendar\nPassword: calendar\nPortal: 192.168.4.1` |
| phone joined the hotspot | `Phone connected\nOpen the setup page` | unchanged |
| credentials submitted | `Connecting to\n<ssid>...` | `Portal: 192.168.4.1` |
| connected | `Connected!\nIP <addr>` | `Settings saved\nLong-press BOOT to exit` |
| connect failed | `Connect failed\nCheck password, retry` | `Hotspot: ...\nPassword: ...\nPortal: 192.168.4.1` |

These are the only app labels written **with** the LVGL lock. The state text is English-only: the
only CJK glyphs present in `lv_font_MISANSMEDIUM_25` are those the audio strings used, so a Chinese
line here would render blank until the font subset is regenerated (see §5).

### 3.3 Wi-Fi setup flow

Opening the view does not bring up Wi-Fi. With the setup view visible, a long press on **KEY**
starts the configuration host; `Config_LoopTask` then:

1. `espwifi_config_start()` — NVS, netifs and the Wi-Fi driver (re-)initialise, the device goes
   `WIFI_MODE_APSTA`, starts a WPA2-PSK softAP named `ESP32-Calendar` (passphrase `calendar`, both
   from `ESPWIFI_AP_SSID`/`ESPWIFI_AP_PASS` in `esp_wifi_bsp.h`) on channel 1, runs a blocking scan
   as STA (APSTA allows scanning while the AP is up) and keeps the AP list for the portal.
2. The captive portal starts (`components/app_bsp/wifi_portal.c`): a UDP DNS server on port 53
   answers every A query with 192.168.4.1, and `esp_http_server` serves the setup page. A phone that
   joins the hotspot gets the page popped up automatically; the page is also reachable at
   `http://192.168.4.1`.
3. `GET /` renders the scanned SSIDs into a `<select>` plus a password field.
4. `POST /connect` decodes the form, copies the credentials into `espwifi_cfg_ssid/pass`, sets
   `WIFI_EV_CREDENTIALS` and redirects the phone to `/status`.
5. `Config_LoopTask` picks up that bit and calls `espwifi_config_connect()`, which blocks up to 20 s
   for `IP_EVENT_STA_GOT_IP`. On success the credentials are written to the NVS namespace
   `wificfg` (keys `ssid`, `pass`) and `/status` shows the IP; on failure the page returns to the
   scan list after 3 s and the AP stays up for a retry.
6. Long-pressing BOOT again closes the view. If the host is running, `espwifi_config_stop()` stops the DNS/HTTP servers and
   tears Wi-Fi **all the way down** — the radio is not kept up after setup — then `Config_LoopTask`
   sets `CFG_SYNC_NOW` when a running host closes, which makes `Time_SyncTask` open a sync window
   straight away with the new credentials. From then on the credentials are only used inside those
   windows (§3.4).

### 3.4 Daily Wi-Fi sync window

Wi-Fi is the largest current draw on the board, so the radio is **down except inside a sync window**.
`Time_SyncTask` owns the windows and is the only thing that brings the station up; `Config_LoopTask`
is the only other user of the radio, and the two are serialised by `WifiMutex`.

A window connects with the stored credentials, blocks on SNTP, writes the corrected time back to the
PCF85063 and calls `espwifi_deinit()`. The Wi-Fi icon in the bottom bar (§3.1) is written at both
ends of it, which is the only feedback the dashboard gives about the radio.

| Trigger | When |
|---|---|
| boot | as soon as `Time_SyncTask` starts |
| daily | `TIME_SYNC_HOUR`:`TIME_SYNC_MINUTE` local time — 03:30 by default |
| after setup | closing a running setup host sets `CFG_SYNC_NOW` |
| retry | `TIME_SYNC_RETRY_MINUTES` (30) after a window that got no answer, or one that ran while the clock was still invalid |

All four constants sit at the top of `user_app.cpp`. Moving the daily window means editing
`TIME_SYNC_HOUR`/`TIME_SYNC_MINUTE`, not a Kconfig option.

With nothing stored in the NVS namespace `wificfg`, a window ends immediately and the icon shows the
*unset* state; the clock then runs from the RTC alone. See
[`data_sources.md`](data_sources.md) for the step-by-step window and its timeouts.

### 3.5 Tarot view — `tarot_page_root()`

A full-screen view showing three random cards from the SD card, one per column, each with its name
underneath. It has no live data: it is redrawn only when a draw happens.

| Widget | Position | Content |
|---|---|---|
| `tarot_img[3]` | x = -8 / 120 / 248, y = 14 | the decoded card, 128 × 219, 1-bit black/white |
| `tarot_name[3]` | x = -8 / 120 / 248, y = 237 (128 × 46) | the card's name, 12 px, centred; a reversed card adds `v REV` on the following line |
| `tarot_msg` | centred | `No SD card` / `No images found` / `Card read failed`, 18 px |

The columns butt together with no gap (`TAROT_GAP 0`) and start at x = -8, so the first card is
clipped 8 px on the left and the row ends at x = 376.

`Tarot_ManagerInit()` mounts the card and lists `/sdcard/tarot/images/*.jpg`; `Tarot_ShowRandom()`
decodes three distinct cards down to the 1-bit panel and is called when the view is entered (BOOT
long press) and on a KEY single click. The decode runs outside the LVGL lock; only the widget update
is inside it.

The card source, the name table, the decode → ordered-dither pipeline, its tunables and the error
states are in [`tarot.md`](tarot.md).

## 4. Button interface

Two buttons are readable by firmware, both active-low, debounced and decoded by the vendored
`multi_button` library into single click, double click and long-press-start:

| Button | GPIO | Event group | Bits |
|---|---|---|---|
| **BOOT** | 0 | `BootButtonGroups` | bit 0 single, bit 1 double, bit 2 long press |
| **KEY** | 18 | `GP18ButtonGroups` | bit 0 single, bit 1 double, bit 2 long press |

The board's third button, **PWR**, is a hardware power control and is not visible to firmware.

Button callbacks only set event-group bits; all behaviour lives in `BOOT_LoopTask` and
`KEY_LoopTask` in `user_app.cpp`. Each task waits on its group with a 2 s timeout and handles at most
one bit per wake, in the priority order below.

### 4.1 Current bindings

| Button | Gesture | Action |
|---|---|---|
| BOOT | long press | Advance the view: dashboard → **tarot** → **Wi-Fi setup** → dashboard. Leaving the Wi-Fi view stops the hotspot/portal if it is running; entering the tarot view draws a random card |
| BOOT | single / double click | unused |
| KEY | single click | On the dashboard, move the calendar view one day forward and re-render the grid and the 宜忌 section; the top-left date/time block keeps showing today. On the tarot view, draw the next random card |
| KEY | double click | On the dashboard, move the calendar view to the 1st of the next month, clearing any single-day offset |
| KEY | long press | On the dashboard, jump the calendar view back to today; on the Wi-Fi setup view, start the configuration hotspot and captive portal |

Notes on the semantics as implemented:

* Long press fires on **press start**, not release — the view flips while the button is still down.
* KEY dispatches on `CurrentView` to `calendar_key_<gesture>()`, `tarot_key_<gesture>()` or
  `wifi_setup_key_<gesture>()`. The dashboard's single click steps a day
  offset (`CalendarView_AdvanceDay`), its double click steps a month offset and anchors the view
  on the 1st of that month, clearing the day offset (`CalendarView_AdvanceMonth`), and its long
  press clears both (`CalendarView_ResetDay`);
  all three re-render through `CalendarView_Render()` in `driver_update.cpp`, which logs the
  resulting view date (`cal_view` tag). While an offset is
  nonzero the month grid, the highlighted cell, 干支 and the 宜忌 lines follow the selected day,
  and the top-left date, weekday, week counter and clock stay on real today (the `browsing` /
  `today_*` fields of `calendar_ui_data_t`). The setup page's long press requests the hotspot; the
  tarot page's single click calls `Tarot_ShowRandom()` and its double/long press do nothing.
* The view switcher is `app_view_t CurrentView` (`user_app_internal.h`) with one case per view, not
  the old two-state boolean. `BOOT_LoopTask` advances it and `show_view()` hides all three
  containers before showing the incoming one.
* The setup view owns the radio only after its KEY long press: that request takes `WifiMutex` and
  starts the hotspot and captive portal. Closing the view stops them if running, tears Wi-Fi down
  and releases the mutex. If a sync window (§3.4) is in progress, the page sits on `Starting
  hotspot...` until it finishes — at most about 35 s. The button tasks do the visibility flip and
  signal `ConfigGroups`; `Config_LoopTask` does the Wi-Fi work.
* The audio recorder/player demo (BOOT single = play recording, BOOT double = record, KEY single/
  double = `canon.pcm`) was removed together with the audio view and `Codec_LoopTask`.

### 4.2 Navigation state machine

```
                            boot
                              │
                              ▼
                      ┌───────────────┐
                      │   dashboard   │◀──────────────┐
                      │  calendar_ui  │               │
                      └───────┬───────┘          BOOT long
                          BOOT long                 │
                              ▼                     │
                      ┌───────────────┐             │
                      │     tarot     │             │
                      │  tarot_page   │             │
                      └───────┬───────┘             │
                          BOOT long                 │
                              ▼                     │
                      ┌───────────────┐             │
                      │  Wi-Fi setup  │─────────────┘
                      │ wifi setup    │   BOOT long
                      └───────────────┘
```

KEY on the tarot view draws the next card in place; it does not change the view.

## 5. Typography and assets

The dashboard and the two inherited views use different font families, because they came from
different places.

| Font | Size | Bpp | Used for |
|---|---|---|---|
| `lv_font_calendar_clock_48` | 48 px | 1 | the clock; digits, `:` and `-` only |
| `lv_font_calendar_18` | 18 px | 1 | month heading |
| `lv_font_calendar_16` | 16 px | 1 | date line, temperature and humidity, calendar grid and its header |
| `lv_font_calendar_12` | 12 px | 1 | weekday, week counter, bottom bar |
| `lv_font_calendar_chinese_12` | 12 px | 1 | — (no caller; the 黄道吉日 state is not drawn) |
| `lv_font_calendar_ganzhi_12` | 12 px | 1 | year 干支 beside the month heading (2026--2046 stems and branches) |
| `lv_font_calendar_yiji_12` | 12 px | 1 | the `宜：` / `忌：` lines in the bottom bar (one clipped line each) and the single `建除：… 值神：…` line in the left column |
| `lv_font_montserrat_14` | 14 px | 4 | the bottom-bar Wi-Fi icon and its `✓` marker — LVGL's built-in default face, the only one here carrying `LV_SYMBOL_WIFI` / `LV_SYMBOL_OK` |
| `lv_font_MISANSMEDIUM_25` | 25 px | 4 | Wi-Fi setup title and state line |
| `lv_font_MISANSMEDIUM_18` | 18 px | 4 | Wi-Fi setup portal/hint lines |

The `lv_font_calendar_*` faces are DejaVu Sans Condensed Bold subsets generated with `lv_font_conv`
and checked in under `components/ui_bsp/fonts/`. They are **1 bpp on purpose**: the flush
callback thresholds every pixel, so a 4 bpp face's anti-aliased edges are discarded at runtime and
the extra bitmap data is pure flash cost. Condensed, because every size here is width-constrained.

The clock size is set by the column split, not by taste: `10:24` is 2.86 × the point size wide in
this face, so the 134 px left column allows 48 px and no more. Widening the clock means narrowing
the month grid — the two trade against each other directly. The exact `lv_font_conv` command line
sits in the header comment of each generated `.c` file; rerun it at a different size to move that
trade. The clock face carries digits, `:` and `-` only — enough for `10:24` and `--:--`.

`lv_font_calendar_*` covers ASCII plus U+00B0 (`°`) and U+00B7 (`·`). Chinese is served by three
1 bpp Noto Sans CJK subsets, each carrying only the glyphs its own strings need:
`lv_font_calendar_ganzhi_12` holds the 2026--2046 stems and branches, `lv_font_calendar_yiji_12` is
generated by `lunar/generate_ui_yiji.py` and covers every character that appears in any day's 宜 / 忌
list plus the fixed `宜忌无：十二建除值神` wording, a space to separate the two status fields, and all
十二建除 / 十二值神 names (224 glyphs, ~4.7 KB), and `lv_font_calendar_chinese_12` holds
今、吉、日、道、非、黄 for the 黄道吉日 wording. Add new Chinese dashboard wording by regenerating an
explicit 1 bpp subset first.

A hand-written `lv_font_fmt_txt_*` face must keep `glyph_id_start = 1` **and** reserve
`glyph_dsc[0]` for an all-zero placeholder, the way `lv_font_conv` does: LVGL looks up
`glyph_dsc[glyph_id_start + index]`, so a face without the placeholder draws each character with the
next character's glyph and reads past the end of the array for its last one.

The MiSans faces serve the setup view. Its 25 px subset holds
only the 13 CJK glyphs the removed audio strings needed (`等待操作正在录音完成播放音乐`), which is why
that view is English-only; the Chinese half of its strings lives in the phone-facing portal page.

All generated bitmap assets have been removed. The dashboard is text, 1 px rules and the single
`LV_SYMBOL_WIFI` glyph in the bottom bar — no bitmap icons, gauges or weather art.

## 6. Adding to the UI

Which half of the UI you are in decides the workflow:

* **Dashboard** — edit `components/ui_bsp/page_calendar/calendar_ui.c` directly. Layout constants live at
  the top of that file; every widget is built by `calendar_ui_create()` and written by one of the
  `calendar_ui_update_*` / `calendar_ui_refresh_all` entry points. Nothing else may touch those
  widgets.
* **Wi-Fi setup view** — edit `components/ui_bsp/page_wifi_setup/wifi_setup_page_layout.c`; it owns the
  setup view's static layout and initial labels.
* **Tarot view** — edit `components/ui_bsp/page_tarot/tarot_page.c` for the widget tree; the
  SD scan, decode and mono conversion belong in `components/app_bsp/tarot_manager.cpp`, which drives
  the page. Keep `ui_bsp` free of file IO and JPEG decoding.

Then, in every case:

1. Drive it from `user_app.cpp`. Keep the split: `calendar_ui.c` owns the widget tree and the
   formatting, `user_app.cpp` owns *when* things are written.
2. **Take the LVGL lock.** `ui_bsp` sits below `app_bsp` and cannot call `Lvgl_lock()` itself, so
   every `calendar_ui_*` call must sit between `Lvgl_lock(-1)` and `Lvgl_unlock()` in the calling
   task — the LVGL task runs on core 0 while app tasks run on core 1 (see `AGENTS.md` §3).
3. Respect the refresh cost: compare against what is already displayed and write only on a real
   change, as both dashboard tasks do. A repaint is a full 400 × 300 SPI transfer.
4. Feed the dashboard through `calendar_ui_data_t`, not through a sensor or clock handle. If a new
   value needs displaying, add a field there and a manager under `app_bsp` to produce it.
5. If you add a view, extend the hide-everything-else lists in both button tasks.

## 7. Known rough edges

Inherited from the factory demo, or left by the dashboard rewrite; flag rather than preserve:

* The setup view is English-only, because the MiSans subsets do not cover the Chinese it needs (§5).
  Regenerate the font subsets if the bilingual policy has to hold there.
* The codec hardware, `canon.pcm` and the 288 KB PSRAM audio buffer are no longer used by anything —
  `CodecPort` is not instantiated; only the `codec_bsp` component remains linked.
* `ble_scan_bsp` is still compiled and linked but no longer called: the BLE device count went with
  the old dashboard. Bluetooth is still enabled in `sdkconfig.defaults` and costs flash for nothing.
* The SD card is mounted at `/sdcard` for the tarot view only. `CustomSDPort` was originally written
  for the `sdcard Test:` self-test row; `fatfs` is linked for it, and nothing else on the board uses
  the card.
* `calendar_calc` still computes `lunar_auspicious` (黄道吉日) and still fills `days_of_year`, but
  nothing draws either: the 黄道吉日 state and the day-of-year readout went with the old bottom bar.
  `lv_font_calendar_chinese_12` (§5) is the face that would render the 今日黄道吉日 wording if it is
  ever wired up.
