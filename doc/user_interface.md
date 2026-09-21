# User interface and interaction

How the firmware presents information on the reflective LCD and what the two user buttons do.
Describes the **current** (factory-demo derived) behaviour — see §7 for the parts a calendar build
should change.

Code map:

| Concern | File |
|---|---|
| Widget tree, positions, styles, fonts | `components/ui_bsp/generated/setup_scr_screen.c` (GUI Guider output — regenerate, don't hand-edit) |
| Widget handles (`lv_ui` struct) | `components/ui_bsp/generated/gui_guider.h` |
| Hand-written UI hooks | `components/ui_bsp/custom/custom.c` |
| View switching, label updates, all behaviour | `components/user_app/user_app.cpp` |
| Button decoding | `components/port_bsp/button_bsp.c` |
| LVGL port, flush, refresh policy | `components/app_bsp/lvgl_bsp.cpp`, `main/main.cpp` |

## 1. Display characteristics that shape the design

* **400 × 300 landscape, 1 bit per pixel.** There is no grey. LVGL renders RGB565 and the flush
  callback in `main/main.cpp` thresholds every pixel at `0x7fff` — anything darker than mid-grey
  becomes black. Anti-aliased font edges, gradients and shadows all collapse; design in pure black
  and white and check contrast at the threshold, not in the GUI Guider preview.
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
GUI Guider's screen-animation helpers are not used.

| Container | View | Initial state |
|---|---|---|
| `screen_cont_2` | Status dashboard (the home view) | visible |
| `screen_cont_3` | Full-screen image | hidden |
| `screen_cont_4` | Wi-Fi setup / configuration | hidden |

The invariant is *exactly one container visible at a time*. Every switch must both clear the flag on
the incoming container and set it on both others; the toggle helpers in `BOOT_LoopTask` and
`KEY_LoopTask` do this explicitly. Adding a fourth view means extending each of those lists.

## 3. The views

### 3.1 Status dashboard — `screen_cont_2`

The home view. Left half is a large clock and the local self-test results; right half is sensors and
radio scan counts.

```
 (0,0)                                                                  (400,0)
 ┌──────────────────────────────┬───────────────────────────────────────┐
 │  ███████████                 │        [icon]  25%      humidity      │
 │  ██  88   ██   minutes       │        [icon]  25°      temperature   │
 │  ███████████   100px white   │                                       │
 │                on black      │        BLE  :  30                     │
 │                              │        WIFI :  20                     │
 │  sdcard Test:                │                                       │
 │      No Card                 │            ███████████                │
 │  [bat]  100%                 │            ██  88   ██   seconds      │
 │  [bat]  ON                   │            ███████████                │
 └──────────────────────────────┴───────────────────────────────────────┘
 (0,300)                                                              (400,300)
```

| Widget | Position (x, y) | Content | Written by | Cadence |
|---|---|---|---|---|
| `screen_label_3` | 8, 9 (190×135) | RTC **minute**, `%02d`, 100 px white on black | `Lvgl_UserTask` | 1 s |
| `screen_label_4` | 201, 154 (190×135) | RTC **second**, `%02d`, 100 px white on black | `Lvgl_UserTask` | 1 s |
| `screen_label_5` | 7, 164 | static `"sdcard Test: "` | — | — |
| `screen_label_6` | 74, 192 | `"No Card"` / `"passed"` / `"failed"` | `Lvgl_SDcardTask` | once at boot |
| `screen_img_1` + `screen_label_7` | 29, 221 / 73, 227 | battery icon + charge percentage | `Lvgl_UserTask` | 2 s |
| `screen_img_2` + `screen_label_8` | 29, 258 / 73, 262 | second battery icon + `"ON"` | `UserApp_UiInit` | never updated |
| `screen_img_3` + `screen_label_11` | 235, 10 / 272, 16 | humidity icon + SHTC3 relative humidity `%d%%` | `Lvgl_UserTask` | 5 s |
| `screen_img_4` + `screen_label_12` | 235, 48 / 272, 53 | temperature icon + SHTC3 temperature `%d°` | `Lvgl_UserTask` | 5 s |
| `screen_label_9` + `screen_label_13` | 205, 90 / 277, 92 | `"BLE : "` + count of BLE devices seen | `Lvgl_BleScanTask` | once at boot |
| `screen_label_10` + `screen_label_14` | 205, 118 / 277, 118 | `"WIFI : "` + the STA IP once connected, `"OFFLINE"` if a stored network did not come up, `"SETUP"` if no credentials are stored | `Lvgl_BleScanTask`, `Config_LoopTask` | once at boot, again on connect |

The radio snapshot is produced once by `Lvgl_BleScanTask`, which is now BLE-only: if stored
credentials exist it instead waits up to 20 s for the STA IP and reports that (Wi-Fi and BLE are not
coexistent — see `AGENTS.md` §3). With no credentials, it scans BLE until it has 20 devices or goes
3.5 s without a new one, shows `SETUP` on the Wi-Fi row, then tears BLE down and exits.

### 3.2 Image view — `screen_cont_3`

A single full-bleed 400 × 300 image (`_ein_alpha_400x300`), nothing else. Reached by a long press on
**KEY**, dismissed by another long press. This is the slot a calendar/photo page would take over.

### 3.3 Wi-Fi setup view — `screen_cont_4`

The network configuration view. It is reached by a long press on **BOOT** and shows what the
device's own configuration hotspot is doing; all text entry happens on the phone (§3.4).

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
| view opened | `Starting hotspot...` | `Hotspot: ESP32-Calendar\nPassword: calendar` |
| hotspot + portal up | `Hotspot ready\nJoin it, page pops up` | `Hotspot: ESP32-Calendar\nPassword: calendar\nPortal: 192.168.4.1` |
| phone joined the hotspot | `Phone connected\nOpen the setup page` | unchanged |
| credentials submitted | `Connecting to\n<ssid>...` | `Portal: 192.168.4.1` |
| connected | `Connected!\nIP <addr>` | `Settings saved\nLong-press BOOT to exit` |
| connect failed | `Connect failed\nCheck password, retry` | `Hotspot: ...\nPassword: ...\nPortal: 192.168.4.1` |

These are the only app labels written **with** the LVGL lock. The state text is English-only: the
only CJK glyphs present in `lv_font_MISANSMEDIUM_25` are those the audio strings used, so a Chinese
line here would render blank until the font is regenerated in GUI Guider (see §5).

### 3.4 Wi-Fi setup flow

Nothing runs until the user opens the view. `Config_LoopTask` then:

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
6. On the next boot `espwifi_connect_stored()` reads that namespace and connects as a plain STA. With
   nothing stored, Wi-Fi is not started at all and the dashboard shows `SETUP`.

Long-pressing BOOT again closes the view: `espwifi_config_stop()` stops the DNS/HTTP servers and the
hotspot, keeps the STA connection if one is working, and otherwise tears Wi-Fi down.

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
| BOOT | long press | Toggle the **Wi-Fi setup view** (`cont_4`) on/off; off returns to the dashboard and stops the hotspot/portal |
| KEY | long press | Toggle the **image view** (`cont_3`) on/off; off returns to the dashboard |
| BOOT | single / double click | unused |
| KEY | single / double click | unused |

Notes on the semantics as implemented:

* Long press fires on **press start**, not release — the view flips while the button is still down.
* The two view toggles are independent booleans (`is_CfgViewOn`, `is_cont3en`). Turning one off always
  returns to the dashboard, so entering the image view from the setup view and then leaving lands on
  the dashboard rather than back where you came from.
* The setup view owns the radio while it is open: opening it starts the hotspot and captive portal,
  closing it stops them and tears Wi-Fi down if the STA never associated. The button tasks do the
  visibility flip and signal `ConfigGroups`; `Config_LoopTask` does the Wi-Fi work.
* The audio recorder/player demo (BOOT single = play recording, BOOT double = record, KEY single/
  double = `canon.pcm`) was removed together with the audio view and `Codec_LoopTask`.

### 4.2 Navigation state machine

```
                            boot
                              │
                              ▼
                      ┌───────────────┐
                      │   dashboard   │◀─────────────┐
                      │    cont_2     │              │
                      └───────┬───────┘              │
                   BOOT long  │  KEY long            │
               ┌──────────────┴──────────────┐       │
               ▼                             ▼       │
       ┌───────────────┐             ┌───────────────┤
       │  Wi-Fi setup  │             │  image view   │
       │    cont_4     │             │    cont_3     │
       └───────┬───────┘             └───────┬───────┘
               └── BOOT long ────────────────┴─ KEY long ─┘
```

## 5. Typography and assets

| Font | Size | Used for |
|---|---|---|
| `lv_font_MISANSMEDIUM_100` | 100 px | the two big clock digits |
| `lv_font_MISANSMEDIUM_25` | 25 px | Wi-Fi setup title and state line |
| `lv_font_MISANSMEDIUM_20` | 20 px | all dashboard labels |
| `lv_font_MISANSMEDIUM_18` | 18 px | Wi-Fi setup portal/hint lines |

Every font in the UI is a MiSans subset. **Any new Chinese string must use a MiSans face**, and the
glyph must be in the subset GUI Guider generated — adding characters means regenerating the font,
not just typing them. The 25 px subset holds only the 13 CJK glyphs the audio
strings needed (`等待操作正在录音完成播放音乐`), which is why the setup view is English-only; the
Chinese half of its strings lives in the phone-facing portal page instead.

Images are LVGL C arrays under `components/ui_bsp/generated/images/`: two 30×30 sensor icons
(`_wendu` temperature, `_shidu` humidity), a 30×30 battery icon used twice and a 400×300 full-screen
image. All are 1-bit-friendly line art; photographic content will posterize badly at the flush
threshold.

## 6. Adding to the UI

1. Lay the widget out in the GUI Guider project and regenerate `components/ui_bsp/generated/`.
   Hand-edits there are lost on the next regeneration.
2. Drive it from `user_app.cpp`. Keep the split: generated code builds the tree, `user_app.cpp` owns
   all text and visibility changes.
3. **Take the LVGL lock.** Every `lv_*` call from an app task must sit between `Lvgl_lock(-1)` and
   `Lvgl_unlock()` — the LVGL task runs on core 0 while app tasks run on core 1. Several inherited
   tasks in `user_app.cpp` update labels without the lock; that is a bug to fix as you touch them, not
   a pattern to copy (see `AGENTS.md` §3).
4. Respect the refresh cost: pick the slowest cadence that looks right, and update several labels in
   one locked section rather than taking the lock per label.
5. If you add a view, extend the hide-everything-else lists in both button tasks.

## 7. Known rough edges

Inherited from the factory demo, or left by the setup-view change; flag rather than preserve:

* The clock shows **minute and second**, not hour and minute, and the two digit blocks are placed
  diagonally rather than as one `MM:SS` field. A calendar needs date, weekday and hour:minute.
* `screen_label_8` is set once to `"ON"` and never touched again, and `screen_img_2` reuses the
  battery icon next to it — the row has no defined meaning.
* The BLE device count is a one-shot boot diagnostic displayed permanently.
* `Lvgl_UserTask` schedules with equality tests (`times - adc_time == 10`). It works only because the
  counter increments by exactly one per iteration; use `>=` if you change that loop.
* The setup view is English-only, because the generated MiSans subsets do not cover the Chinese it
  needs (§5). Regenerate the fonts in GUI Guider if the bilingual policy has to hold there.
* `Rtc_SetTime(2026,1,5,14,30,30)` still runs on every boot in `UserApp_AppInit()`, so the RTC never
  keeps time across a power cycle.
* The codec hardware, `canon.pcm` and the 288 KB PSRAM audio buffer are no longer used by anything —
  `CodecPort` is not instantiated; only the `codec_bsp` component remains linked.
